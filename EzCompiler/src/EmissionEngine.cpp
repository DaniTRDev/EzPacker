#include "EmissionEngine.h"
#include "DriverContext.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
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
#include "GlobalVar/MirGlobalVar.h"
#include "Type/MirType.h"
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
EzCodeEmitter::ObjectFormat::ObjectSymbol makeUndefinedFunctionSymbol(std::string_view name)
{
    return { .m_name = name,
             .m_section = SectionType::Undefined,
             .m_offset = 0,
             .m_size = 0,
             .m_isGlobal = true,
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

    // 1. Emit Global Variables (.rodata, .data, .bss)
    for (MirGlobalVar *gvar : mirCtx.getGlobalVars())
    {
        if (!gvar)
        {
            continue;
        }

        gvarById[gvar->getId()] = gvar;

        // Size the object from its pointee type, rounding bits up to whole bytes.
        MirType *pointeeType = gvar->getType() ? gvar->getType()->getPointedType() : nullptr;
        size_t gvSize = pointeeType ? (pointeeType->getTotalSizeInBits() + 7) / 8 : 8;
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
        MirOperand *init = gvar->getInitializer();
        bool hasInit = (init != nullptr);
        bool isZero = false;

        if (init && init->getType() == MirOperandType::Integer)
        {
            if (init->get<MirInteger>()->getValue().getI64() == 0)
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
                int64_t val = init->get<MirInteger>()->getValue().getI64();
                std::vector<uint8_t> valBytes(gvSize, 0);
                for (size_t b = 0; b < gvSize && b < 8; ++b)
                {
                    valBytes[b] = static_cast<uint8_t>((val >> (b * 8)) & 0xFF);
                }
                sec->emitBytes(valBytes.data(), valBytes.size());
            }
            else if (init && init->getType() == MirOperandType::FloatingPoint)
            {
                std::vector<uint8_t> valBytes(gvSize, 0);
                if (gvSize == 4)
                {
                    float fval = init->get<MirFloat>()->getValue().getFloat();
                    std::memcpy(valBytes.data(), &fval, 4);
                }
                else
                {
                    double dval = init->get<MirFloat>()->getValue().getDouble();
                    std::memcpy(valBytes.data(), &dval, std::min<size_t>(gvSize, 8));
                }
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
                            .m_isGlobal = (gvar->getLinkage() != MirGlobalVarLinkage::Internal),
                            .m_isFunction = false });
    }

    // 2. Emit Functions (.text)
    for (MirFunction *func : mirCtx.getFunctions())
    {
        if (!func)
        {
            continue;
        }

        funcById[func->getId()] = func;

        if (func->getBlockCount() == 0)
        {
            // External declaration
            symbols.push_back(makeUndefinedFunctionSymbol(func->getName()));
            continue;
        }

        // Honor the target's function alignment so consecutive functions do not share padding.
        textSection->alignTo(binDesc->getFunctionAlignment());
        uint64_t fnOffset = textSection->getCurrentOffset();
        emitter->beginFunction(&emitterCtx, func);

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
                    // getOperands() exposes const pointers; emitInst accepts that const span directly.
                    const auto &ops = inst->getOperands();
                    emitter->emitInst(inst->getTargetDesc(), std::span<MirOperand *const>(ops.data(), ops.size()));
                }
            }
        }

        emitter->endFunction(&emitterCtx, func);
        uint64_t fnSize = textSection->getCurrentOffset() - fnOffset;

        symbols.push_back({ .m_name = func->getName(),
                            .m_section = SectionType::Text,
                            .m_offset = fnOffset,
                            .m_size = fnSize,
                            .m_isGlobal = true,
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
                        symbols.push_back(makeUndefinedFunctionSymbol(calleeName));
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
            writer = std::make_unique<EzCodeEmitter::ObjectFormat::Elf64Writer>();
            break;
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
