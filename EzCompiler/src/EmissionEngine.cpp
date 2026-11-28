#include "EmissionEngine.h"

#include "DriverContext.h"

#include "PipelineTelemetry.h"

#include "Builder/MirBuilderContext.h"

#include "Debug/MirDebugMetadata.h"

#include "Debug/IDebugInfoEmitter.h"

#include "Function/MirFunction.h"

#include "Function/MirFunctionStackFrame.h"

#include "Function/CallingConvDesc.h"

#include "Block/MirBlock.h"

#include "Instruction/MirInstruction.h"

#include "Instruction/MirTargetInstructionDesc.h"

#include "CodeEmitterContext.h"

#include "CodeSection.h"

#include "GenericCodeEmitter.h"

#include "Descriptors/TargetDesc.h"

#include "Descriptors/TargetRelocationResolver.h"

#include "ObjectFormat/IObjectWriter.h"

#include "ObjectFormat/Elf64Writer.h"

#include "ObjectFormat/CoffWriter.h"

#include "Operand/MirOperands.h"

#include "Operand/MirRegisterBank.h"

#include "GlobalVar/MirGlobalVar.h"

#include "Type/MirType.h"

#include "FlexNumber/FlexInt.h"

#include "FlexNumber/FlexFloat.h"

#include <filesystem>

#include <fstream>

#include <memory>

#include <unordered_map>

#include <unordered_set>



namespace EzCompiler

{



namespace

{



/**

 * Builds the undefined-symbol record used for external function declarations and for callees that

 * are referenced but never defined in this module. Undefined symbols carry SectionType::Undefined,

 * which both object writers map to the "no section" index (SHN_UNDEF / COFF section 0).

 */

EzCodeEmitter::ObjectFormat::ObjectSymbol makeUndefinedFunctionSymbol(std::string_view name, bool isWeak = false)

{

    return { .m_name = name,

             .m_section = SectionType::Undefined,

             .m_offset = 0,

             .m_size = 0,

             .m_isGlobal = true,

             .m_isWeak = isWeak,

             .m_isFunction = true };

}



} // namespace



EmissionEngine::EmissionEngine(DriverContext &ctx) : m_ctx(ctx) {}



bool EmissionEngine::emitModule(MirBuilderContext &mirCtx, std::string_view outputPath)

{

    TargetBinaryDesc *binDesc = m_ctx.getBinaryDesc();

    if (!binDesc)

    {

        m_ctx.getDiagCollector()->error("EzCompiler", "No target binary descriptor configured for emission");

        return false;

    }



    const auto &sections = binDesc->getSections();

    CodeSection *textSection = binDesc->getSection(SectionType::Text);

    if (!textSection)

    {

        m_ctx.getDiagCollector()->error("EzCompiler", "Target binary descriptor missing .text section");

        return false;

    }



    // initialize() resolves the target descriptor and its binary view together, so a non-null

    // binary descriptor guarantees a target descriptor; no redundant re-validation here.

    TargetDesc *targetDesc = m_ctx.getTargetDesc();



    ::CodeEmitterContext emitterCtx(m_ctx.getDiagCollector(), sections, m_ctx.getSessionAllocator());

    // The target installs its own encoding resolution; this layer stays target-agnostic.

    std::unique_ptr<GenericCodeEmitter> emitter = targetDesc->createCodeEmitter();

    if (!emitter)

    {

        m_ctx.getDiagCollector()->error("EzCompiler", "Target did not provide a code emitter");

        return false;

    }



    std::vector<EzCodeEmitter::ObjectFormat::ObjectSymbol> symbols;

    std::unordered_map<size_t, MirFunction *> funcById;

    std::unordered_map<size_t, MirGlobalVar *> gvarById;

    funcById.reserve(mirCtx.getFunctions().size());

    gvarById.reserve(mirCtx.getGlobalVars().size());

    const bool emitDebug = m_ctx.getOptions().emitDebugInfo();
    MirDebugMetadata *debugMeta = emitDebug ? mirCtx.getOrCreateDebugMetadata() : nullptr;

    struct EmittedGlobalDebug
    {
        std::string sectionName;
        uint64_t sectionOffset{ 0 };
        uint64_t sizeInBytes{ 0 };
    };
    struct EmittedFuncDebug
    {
        uint64_t sectionOffset{ 0 };
        uint64_t codeSize{ 0 };
        uint32_t prologueSize{ 0 };
        std::vector<EzCore::Debug::DebugLineEntry> lineEntries;
    };
    std::unordered_map<MirId, EmittedGlobalDebug> emittedGlobals;
    std::unordered_map<MirId, EmittedFuncDebug> emittedFuncs;



    // 1. Emit Global Variables (.rodata, .data, .bss)

    for (MirGlobalVar *gvar : mirCtx.getGlobalVars())

    {

        if (!gvar)

        {

            continue;

        }



        gvarById[gvar->getId()] = gvar;



        if (gvar->isDeclaration())

        {

            symbols.push_back({ .m_name = gvar->getName(),

                                .m_section = SectionType::Undefined,

                                .m_offset = 0,

                                .m_size = 0,

                                .m_isGlobal = (gvar->getLinkage() != MirLinkage::Internal),

                                .m_isWeak = (gvar->getLinkage() == MirLinkage::Weak),

                                .m_isFunction = false });

            continue;

        }



        // Size the object from its value or pointee type, rounding bits up to whole bytes.
        MirType *valType = gvar->getType();
        size_t gvSize = valType ? (valType->getTotalSizeInBits() + 7) / 8 : 8;
        MirOperand *init = gvar->getInitializer();
        if (init && init->getMirType() && init->getMirType()->getTotalSizeInBits() > 0)
        {
            size_t initSize = (init->getMirType()->getTotalSizeInBits() + 7) / 8;
            if (initSize > gvSize)
            {
                gvSize = initSize;
            }
        }
        if (gvSize == 0)
        {
            gvSize = 8;
        }
        size_t align = std::min<size_t>(gvSize, 16);
        if (align < 1)
        {
            align = 1;
        }

        SectionType targetSecType = SectionType::Data;
        bool hasInit = (init != nullptr);
        bool isZero = false;


        if (init && init->getType() == MirOperandType::Integer)

        {

            if (init->get<MirInteger>()->getValue().isZero())

            {

                isZero = true;

            }

        }

        else if (init && init->getType() == MirOperandType::FloatingPoint)

        {

            if (init->get<MirFloat>()->getValue().isZero())

            {

                isZero = true;

            }

        }



        // Constants go to read-only storage; zero/uninitialized objects go to .bss.

        if (gvar->isConstant())

        {

            targetSecType = SectionType::ReadOnly;

        }

        else if (!hasInit || isZero)

        {

            targetSecType = SectionType::NonInitialized;

        }

        else

        {

            targetSecType = SectionType::Data;

        }



        CodeSection *sec = binDesc->getSection(targetSecType);

        if (!sec)

        {

            sec = binDesc->getSection(SectionType::Data);

        }



        uint64_t gvOffset = 0;

        if (sec)

        {

            sec->alignTo(align);

            gvOffset = sec->getCurrentOffset();



            // Materialize the initializer bytes: zero-fill, little-endian integer, or float bit pattern.

            if (targetSecType == SectionType::NonInitialized)

            {

                std::vector<uint8_t> zeros(gvSize, 0);

                sec->emitBytes(zeros.data(), zeros.size());

            }

            else if (init && init->getType() == MirOperandType::Integer)

            {

                std::vector<uint8_t> valBytes(gvSize, 0);

                const FlexInt &val = init->get<MirInteger>()->getValue();

                val.writeBytes(valBytes, Endianness::Little);

                sec->emitBytes(valBytes.data(), valBytes.size());

            }

            else if (init && init->getType() == MirOperandType::FloatingPoint)

            {

                std::vector<uint8_t> valBytes(gvSize, 0);

                const FlexFloat &fval = init->get<MirFloat>()->getValue();

                fval.writeIeeeBytes(valBytes, Endianness::Little);

                sec->emitBytes(valBytes.data(), valBytes.size());

            }

            else

            {

                std::vector<uint8_t> zeros(gvSize, 0);

                sec->emitBytes(zeros.data(), zeros.size());

            }

        }



        symbols.push_back({ .m_name = gvar->getName(),

                            .m_section = targetSecType,

                            .m_offset = gvOffset,

                            .m_size = gvSize,

                            .m_isGlobal = (gvar->getLinkage() != MirLinkage::Internal),

                            .m_isWeak = (gvar->getLinkage() == MirLinkage::Weak),

                            .m_isFunction = false });

        if (emitDebug)
        {
            std::string secName = ".data";
            if (targetSecType == SectionType::ReadOnly)
            {
                secName = ".rodata";
            }
            else if (targetSecType == SectionType::NonInitialized)
            {
                secName = ".bss";
            }
            emittedGlobals[gvar->getId()] = { .sectionName = std::move(secName),
                                              .sectionOffset = gvOffset,
                                              .sizeInBytes = gvSize };
        }

    }



    // 2. Emit Functions (.text)

    for (MirFunction *func : mirCtx.getFunctions())

    {

        if (!func)

        {

            continue;

        }



        funcById[func->getId()] = func;



        if (func->isDeclaration())

        {

            // Do not emit unreferenced function declarations into the object symbol table;

            // only declarations that are actually called or referenced in relocations will be emitted.

            continue;

        }



        // Honor the target's function alignment so consecutive functions do not share padding.

        textSection->alignTo(binDesc->getFunctionAlignment());

        uint64_t fnOffset = textSection->getCurrentOffset();

        emitter->beginFunction(&emitterCtx, func);

        bool seenPrologueInst = false;
        bool emittedPrologueEnd = false;
        uint32_t prologueSize = 0;
        std::vector<EzCore::Debug::DebugLineEntry> fnLineEntries;

        for (MirBlock *block : func->getBlocks())

        {

            if (!block)

            {

                continue;

            }

            emitter->bindLabel(block->getId());



            for (MirInstruction *inst : block->getInstructions())

            {

                if (!inst)

                {

                    continue;

                }

                if (inst->getTargetDesc())

                {

                    const uint64_t instStartOffset = textSection->getCurrentOffset();

                    // getOperands() exposes const pointers; emitInst accepts that const span directly.

                    const auto &ops = inst->getOperands();

                    emitter->emitInst(inst->getTargetDesc(), std::span<MirOperand *const>(ops.data(), ops.size()));

                    if (emitDebug && debugMeta)
                    {
                        bool isPrologueEnd = false;
                        if (inst->isPrologue())
                        {
                            prologueSize = static_cast<uint32_t>(textSection->getCurrentOffset() - fnOffset);
                            seenPrologueInst = true;
                        }
                        else if (seenPrologueInst && !emittedPrologueEnd)
                        {
                            isPrologueEnd = true;
                            emittedPrologueEnd = true;
                        }

                        SourceReference *srcRef = inst->getSourceRef();
                        if (srcRef && srcRef->m_sourceFileId != 0 && m_ctx.getSourceManager())
                        {
                            std::string_view fname = m_ctx.getSourceManager()->getSourceName(srcRef->m_sourceFileId);
                            const uint32_t dbgFileId = debugMeta->compileUnit.addOrGetFile(fname);
                            const SourceLineRange *lr = m_ctx.getSourceManager()->getReferenceLine(srcRef);
                            if (dbgFileId != 0 && lr && lr->m_lineNumber > 0)
                            {
                                const uint32_t line = static_cast<uint32_t>(lr->m_lineNumber);
                                const uint32_t col = (srcRef->m_beginOffset >= lr->m_beginOffset)
                                                             ? static_cast<uint32_t>(srcRef->m_beginOffset - lr->m_beginOffset + 1)
                                                             : 1;
                                const uint64_t relOffset = instStartOffset - fnOffset;

                                if (fnLineEntries.empty())
                                {
                                    fnLineEntries.push_back({ .codeOffset = relOffset,
                                                              .fileId = dbgFileId,
                                                              .line = line,
                                                              .column = col,
                                                              .isStmt = true,
                                                              .prologueEnd = isPrologueEnd,
                                                              .epilogueBegin = false });
                                }
                                else
                                {
                                    auto &prev = fnLineEntries.back();
                                    if (prev.fileId != dbgFileId || prev.line != line || prev.column != col ||
                                        (isPrologueEnd && !prev.prologueEnd))
                                    {
                                        fnLineEntries.push_back({ .codeOffset = relOffset,
                                                                  .fileId = dbgFileId,
                                                                  .line = line,
                                                                  .column = col,
                                                                  .isStmt = true,
                                                                  .prologueEnd = isPrologueEnd,
                                                                  .epilogueBegin = false });
                                    }
                                }
                            }
                        }
                    }

                }

            }

        }



        emitter->endFunction(&emitterCtx, func);

        uint64_t fnSize = textSection->getCurrentOffset() - fnOffset;

        if (emitDebug)
        {
            emittedFuncs[func->getId()] = { .sectionOffset = fnOffset,
                                            .codeSize = fnSize,
                                            .prologueSize = prologueSize,
                                            .lineEntries = std::move(fnLineEntries) };
        }



        symbols.push_back({ .m_name = func->getName(),

                            .m_section = SectionType::Text,

                            .m_offset = fnOffset,

                            .m_size = fnSize,

                            .m_isGlobal = (func->getLinkage() != MirLinkage::Internal),

                            .m_isWeak = (func->getLinkage() == MirLinkage::Weak),

                            .m_isFunction = true });

    }



    // Flatten every section so label offsets are final before branch patching. Several section

    // keys may alias one CodeSection (e.g. .rodata), so finalize each unique section once.

    std::unordered_set<CodeSection *> finalizedSections;

    for (auto &[type, sec] : sections)

    {

        if (sec && finalizedSections.insert(sec).second)

        {

            sec->finalize();

        }

    }



    // Patch intra-function branches and build object relocations

    std::vector<EzCodeEmitter::ObjectFormat::ObjectRelocEntry> objectRelocs;

    std::span<uint8_t> textBytes = textSection->getMutableData();

    // Target-owned hook that owns the opcode/displacement knowledge for in-place branch patching.

    TargetRelocationResolver *relocResolver = targetDesc->getRelocationResolver();

    const auto &allRelocs = emitterCtx.getRelocations();



    // O(1) lookup of already-defined symbol names while discovering undefined callees. Keys are

    // views into MIR-context-owned names, which outlive this emission.

    std::unordered_set<std::string_view> definedSymbolNames;

    definedSymbolNames.reserve(symbols.size() * 2);

    for (const auto &sym : symbols)

    {

        definedSymbolNames.insert(sym.m_name);

    }



    auto itTextRelocs = allRelocs.find(textSection);

    if (itTextRelocs != allRelocs.end())

    {

        for (CodeRelocation *reloc : itTextRelocs->second)

        {

            if (!reloc || !reloc->m_srcRef)

            {

                continue;

            }



            MirReference *ref = reloc->m_srcRef;

            const TargetCodeRelocationType relocType = reloc->m_relocType;



            if (ref->isBlock())

            {

                CodeLabel *targetLabel = emitterCtx.findLabel(ref->getRefId());

                if (targetLabel && relocResolver)

                {

                    // Delegate the opcode sniff and displacement computation to the target.

                    relocResolver->patch(textBytes, *reloc, targetLabel->getAddress(), relocType);

                }

            }

            else if (ref->isFunction())

            {

                // View into the resolved function's own (context-owned) name; empty when unresolved.

                std::string_view calleeName;

                auto itFunc = funcById.find(ref->getRefId());

                if (itFunc != funcById.end() && itFunc->second)

                {

                    calleeName = itFunc->second->getName();

                }



                if (!calleeName.empty())

                {

                    if (definedSymbolNames.insert(calleeName).second)

                    {

                        bool isWeak = (itFunc != funcById.end() && itFunc->second)

                                ? (itFunc->second->getLinkage() == MirLinkage::Weak)

                                : false;

                        symbols.push_back(makeUndefinedFunctionSymbol(calleeName, isWeak));

                    }



                    // Ask the resolver for the exact displacement-field offset (opcode dependent

                    // for near branches). Falls back to the relocation address for unknown targets.

                    uint64_t fieldOffset = relocResolver

                            ? relocResolver->getRelocationFieldOffset(textBytes, *reloc, relocType)

                            : reloc->m_address;



                    // ELF PC-relative fixups subtract the 4-byte displacement field length.

                    int64_t addend = (binDesc->getObjectFormat() == TargetObjectFormat::ELF) ? -4 : 0;

                    objectRelocs.push_back({ .m_section = reloc->m_definingSection

                                                             ? reloc->m_definingSection->getType()

                                                             : SectionType::Text,

                                             .m_offset = fieldOffset,

                                             .m_symbolName = calleeName,

                                             .m_type = relocType,

                                             .m_addend = addend });

                }

            }

            else if (ref->isGlobalVar())

            {

                // Same non-owning contract as the function case above.

                std::string_view gvName;

                auto itGv = gvarById.find(ref->getRefId());

                if (itGv != gvarById.end() && itGv->second)

                {

                    gvName = itGv->second->getName();

                }



                if (!gvName.empty())

                {

                    uint64_t fieldOffset = relocResolver

                            ? relocResolver->getRelocationFieldOffset(textBytes, *reloc, relocType)

                            : reloc->m_address;



                    // Same -4 addend adjustment for ELF PC-relative global references.

                    int64_t addend = (binDesc->getObjectFormat() == TargetObjectFormat::ELF) ? -4 : 0;

                    objectRelocs.push_back({ .m_section = reloc->m_definingSection

                                                             ? reloc->m_definingSection->getType()

                                                             : SectionType::Text,

                                             .m_offset = fieldOffset,

                                             .m_symbolName = gvName,

                                             .m_type = relocType,

                                             .m_addend = addend });

                }

            }

        }

    }



    // Feed the selected object writer once; the format-specific setup lives behind IObjectWriter.

    std::unique_ptr<EzCodeEmitter::ObjectFormat::IObjectWriter> writer;

    switch (binDesc->getObjectFormat())

    {

        case TargetObjectFormat::ELF:

        {

            auto elfWriter = std::make_unique<EzCodeEmitter::ObjectFormat::Elf64Writer>();

            if (m_ctx.getOptions().target.isRiscV() ||

                (targetDesc && std::string_view(targetDesc->getName()).find("riscv") != std::string_view::npos))

            {

                elfWriter->setMachine(243); // EM_RISCV

            }

            writer = std::move(elfWriter);

            break;

        }

        case TargetObjectFormat::COFF:

            writer = std::make_unique<EzCodeEmitter::ObjectFormat::CoffWriter>();

            break;

        default:

            m_ctx.getDiagCollector()->error("EzCompiler", "Unsupported object format for emission");

            return false;

    }



    for (const auto &sym : symbols)

    {

        writer->addSymbol(sym);

    }

    for (const auto &reloc : objectRelocs)

    {

        writer->addRelocation(reloc);

    }

    if (emitDebug && debugMeta)
    {
        auto &cu = debugMeta->compileUnit;
        cu.level = m_ctx.getOptions().debugLevel;
        cu.arch = m_ctx.getOptions().target.isRiscV()
                          ? EzCore::Debug::DebugCompileUnit::TargetArch::RiscV64
                          : EzCore::Debug::DebugCompileUnit::TargetArch::X86_64;
        cu.pointerSize = 8;
        cu.isLittleEndian = true;
        if (cu.compDir.empty())
        {
            std::error_code cwdEc;
            cu.compDir = std::filesystem::current_path(cwdEc).string();
        }
        if (cu.mainFile.empty())
        {
            cu.mainFile = m_ctx.getOptions().inputFilePath;
        }
        if (!cu.mainFile.empty())
        {
            cu.addOrGetFile(cu.mainFile);
        }

        cu.functions.clear();
        for (MirFunction *func : mirCtx.getFunctions())
        {
            if (!func || func->isDeclaration())
            {
                continue;
            }
            auto itFn = emittedFuncs.find(func->getId());
            if (itFn == emittedFuncs.end())
            {
                continue;
            }

            const MirDebugFuncBinding *fBind = nullptr;
            for (const auto &b : debugMeta->funcBindings)
            {
                if (b.funcId == func->getId() || b.name == std::string_view(func->getName()))
                {
                    fBind = &b;
                    break;
                }
            }

            EzCore::Debug::DebugFunction df;
            df.name = std::string(func->getName());
            df.returnTypeId = fBind ? fBind->returnTypeId : 0;
            df.declLoc = fBind ? fBind->declLoc : EzCore::Debug::DebugSourceLoc{};
            if (df.declLoc.fileId == 0 && !cu.files.empty())
            {
                df.declLoc.fileId = cu.files.front().fileId;
            }
            df.sectionOffset = itFn->second.sectionOffset;
            df.codeSize = itFn->second.codeSize;
            df.prologueSize = itFn->second.prologueSize;
            df.isExternal = (func->getLinkage() != MirLinkage::Internal);
            CallingConvDesc *cc = func->getCallingConv();
            df.hasFramePointer = cc ? cc->hasFramePointer(func) : true;
            auto *ad = func->getAnalysisData();
            df.totalFrameSize = ad ? static_cast<int64_t>(ad->m_totalFrameSize) : 0;

            // Record callee-saved register spill offsets for CFI/unwind generation.
            if (cu.arch == EzCore::Debug::DebugCompileUnit::TargetArch::RiscV64 && df.hasFramePointer)
            {
                df.savedRegs.push_back({ .regId = 1, .isFpr = false, .cfaOffset = -8 });  // ra
                df.savedRegs.push_back({ .regId = 8, .isFpr = false, .cfaOffset = -16 }); // s0/fp
                int64_t cfaOff = -16 - df.totalFrameSize;
                for (const MirRegisterRef &regRef : func->getUsedCalleeSavedRegs())
                {
                    if (regRef.getId() == 8)
                    {
                        continue;
                    }
                    cfaOff -= 16;
                    bool isFpr = (regRef.getClass() && regRef.getClass()->getBank() &&
                                  std::string_view(regRef.getClass()->getBank()->getName()).find("FPR") !=
                                          std::string_view::npos);
                    df.savedRegs.push_back({ .regId = static_cast<uint32_t>(regRef.getId()),
                                             .isFpr = isFpr,
                                             .cfaOffset = cfaOff });
                }
            }
            else if (cu.arch == EzCore::Debug::DebugCompileUnit::TargetArch::X86_64 && df.hasFramePointer)
            {
                df.savedRegs.push_back({ .regId = 5, .isFpr = false, .cfaOffset = -16 }); // rbp
                int64_t cfaOff = -16 - df.totalFrameSize;
                for (const MirRegisterRef &regRef : func->getUsedCalleeSavedRegs())
                {
                    if (regRef.getId() == 5)
                    {
                        continue;
                    }
                    cfaOff -= 8;
                    bool isFpr = (regRef.getClass() && regRef.getClass()->getBank() &&
                                  std::string_view(regRef.getClass()->getBank()->getName()).find("FPR") !=
                                          std::string_view::npos);
                    df.savedRegs.push_back({ .regId = static_cast<uint32_t>(regRef.getId()),
                                             .isFpr = isFpr,
                                             .cfaOffset = cfaOff });
                }
            }

            // Resolve parameter & local variable locations.
            for (const auto &vb : debugMeta->varBindings)
            {
                if (vb.funcName != df.name)
                {
                    continue;
                }
                EzCore::Debug::DebugVariable dv;
                dv.name = vb.varName;
                dv.typeId = vb.typeId;
                dv.isParameter = vb.isParameter;
                dv.paramIndex = vb.paramIndex;
                dv.declLoc = vb.declLoc;
                if (dv.declLoc.fileId == 0 && df.declLoc.fileId != 0)
                {
                    dv.declLoc.fileId = df.declLoc.fileId;
                }
                dv.scopeStartOffset = 0;
                dv.scopeEndOffset = df.codeSize;

                if (vb.stackObjId.has_value() && func->getStackFrame())
                {
                    if (StackFrameObject *obj = func->getStackFrame()->getObjectFromId(*vb.stackObjId))
                    {
                        dv.location.kind = df.hasFramePointer ? EzCore::Debug::DebugLocationKind::FrameBaseOffset
                                                              : EzCore::Debug::DebugLocationKind::StackPointerOffset;
                        dv.location.offset = obj->m_offset;
                    }
                }
                else if (vb.vregId.has_value() && ad)
                {
                    auto itLoc = ad->m_vregToLocation.find(*vb.vregId);
                    if (itLoc == ad->m_vregToLocation.end())
                    {
                        // Follow SSA-renamed versions back to their pre-SSA root virtual register ID.
                        for (const auto &[ssaId, rootId] : ad->m_ssaToRootVreg)
                        {
                            if (rootId == *vb.vregId)
                            {
                                auto candidate = ad->m_vregToLocation.find(ssaId);
                                if (candidate != ad->m_vregToLocation.end())
                                {
                                    itLoc = candidate;
                                    break;
                                }
                            }
                        }
                    }
                    if (itLoc != ad->m_vregToLocation.end())
                    {
                        const auto &loc = itLoc->second;
                        if (loc.isSpill && func->getStackFrame())
                        {
                            if (StackFrameObject *obj = func->getStackFrame()->getObjectFromId(loc.spillSlotId))
                            {
                                dv.location.kind = df.hasFramePointer
                                                           ? EzCore::Debug::DebugLocationKind::FrameBaseOffset
                                                           : EzCore::Debug::DebugLocationKind::StackPointerOffset;
                                dv.location.offset = obj->m_offset;
                            }
                        }
                        else if (!loc.isSpill)
                        {
                            dv.location.kind = EzCore::Debug::DebugLocationKind::Register;
                            dv.location.regId = loc.physRegId;
                            dv.location.isFpr = loc.isFpr;
                        }
                    }
                }

                if (dv.isParameter)
                {
                    df.parameters.push_back(std::move(dv));
                }
                else
                {
                    df.locals.push_back(std::move(dv));
                }
            }

            df.lineEntries = itFn->second.lineEntries;
            cu.functions.push_back(std::move(df));
        }

        cu.globals.clear();
        for (const auto &gb : debugMeta->globalBindings)
        {
            auto itGv = emittedGlobals.find(gb.gvarId);
            if (itGv == emittedGlobals.end())
            {
                continue;
            }
            EzCore::Debug::DebugGlobalVar dg;
            dg.name = gb.name;
            dg.typeId = gb.typeId;
            dg.declLoc = gb.declLoc;
            dg.isExternal = gb.isExternal;
            dg.isConstant = gb.isConstant;
            dg.sizeInBytes = itGv->second.sizeInBytes;
            dg.sectionName = itGv->second.sectionName;
            dg.sectionOffset = itGv->second.sectionOffset;
            cu.globals.push_back(std::move(dg));
        }

        const bool isCoff = (binDesc->getObjectFormat() == TargetObjectFormat::COFF);
        auto dbgEmitter = EzCodeEmitter::Debug::createDebugInfoEmitter(m_ctx.getOptions().debugFormat, isCoff);
        std::vector<EzCodeEmitter::ObjectFormat::ExtraObjectSection> dbgSections = dbgEmitter->emit(cu);
        std::vector<std::string> emittedSectionNames;
        emittedSectionNames.reserve(dbgSections.size());
        for (auto &sec : dbgSections)
        {
            emittedSectionNames.push_back(sec.m_name);
            writer->addExtraSection(std::move(sec));
        }

        if (PipelineTelemetry *telemetry = m_ctx.getPipelineTelemetry())
        {
            DebugInfoTelemetry dbgTel;
            dbgTel.enabled = true;
            if (m_ctx.getOptions().debugFormat == EzCore::Debug::DebugFormat::CodeView ||
                (m_ctx.getOptions().debugFormat == EzCore::Debug::DebugFormat::Auto && isCoff))
            {
                dbgTel.format = "CodeView";
            }
            else
            {
                dbgTel.format = "Dwarf5";
            }
            dbgTel.level = (cu.level == EzCore::Debug::DebugLevel::LineTablesOnly) ? "LineTablesOnly" : "Full";
            dbgTel.fileCount = cu.files.size();
            dbgTel.typeCount = cu.types.size();
            dbgTel.functionCount = cu.functions.size();
            dbgTel.globalCount = cu.globals.size();
            for (const auto &fn : cu.functions)
            {
                dbgTel.lineEntryCount += fn.lineEntries.size();
            }
            dbgTel.emittedSections = std::move(emittedSectionNames);
            telemetry->setDebugInfo(std::move(dbgTel));
        }
    }

    std::vector<uint8_t> outputBytes = writer->write(sections);



    // Write to a sibling temporary file, then atomically rename it over the destination so a

    // failed or partially written object never replaces a good previous artifact.

    std::filesystem::path finalPath{ std::string(outputPath) };

    std::filesystem::path tempPath = finalPath;

    tempPath += ".tmp";



    {

        std::ofstream outFile(tempPath, std::ios::binary | std::ios::trunc);

        if (!outFile.is_open())

        {

            m_ctx.getDiagCollector()->error("EzCompiler", "Failed to open output file for writing: {}", outputPath);

            return false;

        }



        outFile.write(reinterpret_cast<const char *>(outputBytes.data()),

                      static_cast<std::streamsize>(outputBytes.size()));

        outFile.close();

        if (!outFile)

        {

            m_ctx.getDiagCollector()->error("EzCompiler", "Failed writing object file: {}", outputPath);

            std::error_code removeEc;

            std::filesystem::remove(tempPath, removeEc);

            return false;

        }

    }



    std::error_code ec;

    std::filesystem::remove(finalPath, ec);

    ec.clear();

    std::filesystem::rename(tempPath, finalPath, ec);

    if (ec)

    {

        m_ctx.getDiagCollector()->error("EzCompiler", "Failed to publish object file {}: {}", outputPath, ec.message());

        std::error_code removeEc;

        std::filesystem::remove(tempPath, removeEc);

        return false;

    }



    if (m_ctx.getOptions().verbose)

    {

        std::cout << "[Emission] Emitted " << outputBytes.size() << " bytes to " << outputPath << "\n";

    }



    return true;

}



} // namespace EzCompiler

