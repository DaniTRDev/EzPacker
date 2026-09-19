#include "EmissionEngine.h"
#include "DriverContext.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Block/MirBlock.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "CodeEmitterContext.h"
#include "CodeSection.h"
#include "X86_64/X86_64CodeEmitter.h"
#include "ObjectFormat/Elf64Writer.h"
#include "ObjectFormat/CoffWriter.h"
#include "Operand/MirOperands.h"
#include "GlobalVar/MirGlobalVar.h"
#include "Type/MirType.h"
#include "FlexNumber/FlexFloat.h"
#include <fstream>
#include <unordered_map>

namespace EzCompiler
{

EmissionEngine::EmissionEngine(DriverContext &ctx) :
    m_ctx(ctx)
{
}

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

    ::CodeEmitterContext emitterCtx(m_ctx.getDiagCollector(), sections, m_ctx.getSessionAllocator());
    EzCodeEmitter::X86_64::X86_64CodeEmitter emitter;

    std::vector<EzCodeEmitter::ObjectFormat::ObjectSymbol> symbols;
    std::unordered_map<size_t, MirFunction *> funcById;
    std::unordered_map<size_t, MirGlobalVar *> gvarById;

    // 1. Emit Global Variables (.rodata, .data, .bss)
    for (MirGlobalVar *gvar : mirCtx.getGlobalVars())
    {
        if (!gvar)
        {
            continue;
        }

        gvarById[gvar->getId()] = gvar;

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

        EzCodeEmitter::ObjectFormat::ObjectSymbol sym{
            .m_name = std::string(gvar->getName()),
            .m_section = targetSecType,
            .m_offset = gvOffset,
            .m_size = gvSize,
            .m_isGlobal = (gvar->getLinkage() != MirGlobalVarLinkage::Internal),
            .m_isFunction = false
        };
        symbols.push_back(sym);
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
            EzCodeEmitter::ObjectFormat::ObjectSymbol sym{
                .m_name = std::string(func->getName()),
                .m_section = SectionType::Custom,
                .m_offset = 0,
                .m_size = 0,
                .m_isGlobal = true,
                .m_isFunction = true
            };
            symbols.push_back(sym);
            continue;
        }

        uint64_t fnOffset = textSection->getCurrentOffset();
        emitter.beginFunction(&emitterCtx, func);

        for (MirBlock *block : func->getBlocks())
        {
            if (!block)
            {
                continue;
            }
            emitter.bindLabel(block->getId());

            for (MirInstruction *inst : block->getInstructions())
            {
                if (!inst)
                {
                    continue;
                }
                if (inst->getTargetDesc())
                {
                    const auto &ops = inst->getOperands();
                    emitter.emitInst(inst->getTargetDesc(), std::span(const_cast<MirOperand **>(ops.data()), ops.size()));
                }
            }
        }

        emitter.endFunction(&emitterCtx, func);
        uint64_t fnSize = textSection->getCurrentOffset() - fnOffset;

        EzCodeEmitter::ObjectFormat::ObjectSymbol sym{
            .m_name = std::string(func->getName()),
            .m_section = SectionType::Text,
            .m_offset = fnOffset,
            .m_size = fnSize,
            .m_isGlobal = true,
            .m_isFunction = true
        };
        symbols.push_back(sym);
    }

    for (auto &[type, sec] : sections)
    {
        if (sec)
        {
            sec->finalize();
        }
    }

    // Patch intra-function branches and build object relocations
    std::vector<EzCodeEmitter::ObjectFormat::ObjectRelocEntry> objectRelocs;
    auto secData = textSection->getData();
    const auto &allRelocs = emitterCtx.getRelocations();
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
            uint64_t instOffset = reloc->m_address;

            if (ref->isBlock())
            {
                CodeLabel *targetLabel = emitterCtx.findLabel(ref->getRefId());
                if (targetLabel && instOffset < secData.size())
                {
                    uint64_t targetOffset = targetLabel->getAddress();
                    uint8_t op0 = secData[instOffset];
                    if (op0 == 0xE9) // JMP near: 5 bytes
                    {
                        uint64_t dispOffset = instOffset + 1;
                        uint64_t nextRip = instOffset + 5;
                        int32_t disp = static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(nextRip));
                        textSection->patch32(dispOffset, static_cast<uint32_t>(disp));
                    }
                    else if (op0 == 0x0F && instOffset + 1 < secData.size() && (secData[instOffset + 1] & 0xF0) == 0x80) // Jcc near: 6 bytes
                    {
                        uint64_t dispOffset = instOffset + 2;
                        uint64_t nextRip = instOffset + 6;
                        int32_t disp = static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(nextRip));
                        textSection->patch32(dispOffset, static_cast<uint32_t>(disp));
                    }
                    else if (op0 == 0xE8) // CALL near: 5 bytes
                    {
                        uint64_t dispOffset = instOffset + 1;
                        uint64_t nextRip = instOffset + 5;
                        int32_t disp = static_cast<int32_t>(static_cast<int64_t>(targetOffset) - static_cast<int64_t>(nextRip));
                        textSection->patch32(dispOffset, static_cast<uint32_t>(disp));
                    }
                }
            }
            else if (ref->isFunction())
            {
                std::string calleeName;
                auto itFunc = funcById.find(ref->getRefId());
                if (itFunc != funcById.end() && itFunc->second)
                {
                    calleeName = std::string(itFunc->second->getName());
                }

                if (!calleeName.empty())
                {
                    bool symExists = false;
                    for (const auto &s : symbols)
                    {
                        if (s.m_name == calleeName)
                        {
                            symExists = true;
                            break;
                        }
                    }
                    if (!symExists)
                    {
                        symbols.push_back({
                            .m_name = calleeName,
                            .m_section = SectionType::Custom,
                            .m_offset = 0,
                            .m_size = 0,
                            .m_isGlobal = true,
                            .m_isFunction = true
                        });
                    }

                    int64_t addend = (binDesc->getObjectFormat() == TargetObjectFormat::ELF) ? -4 : 0;
                    objectRelocs.push_back({
                        .m_section = SectionType::Text,
                        .m_offset = instOffset + 1,
                        .m_symbolName = calleeName,
                        .m_type = TargetCodeRelocationType::BranchRel32,
                        .m_addend = addend
                    });
                }
            }
            else if (ref->isGlobalVar())
            {
                std::string gvName;
                auto itGv = gvarById.find(ref->getRefId());
                if (itGv != gvarById.end() && itGv->second)
                {
                    gvName = std::string(itGv->second->getName());
                }

                if (!gvName.empty())
                {
                    int64_t addend = (binDesc->getObjectFormat() == TargetObjectFormat::ELF) ? -4 : 0;
                    objectRelocs.push_back({
                        .m_section = SectionType::Text,
                        .m_offset = reloc->m_address,
                        .m_symbolName = gvName,
                        .m_type = TargetCodeRelocationType::PCRel32,
                        .m_addend = addend
                    });
                }
            }
        }
    }

    std::vector<uint8_t> outputBytes;
    if (binDesc->getObjectFormat() == TargetObjectFormat::ELF)
    {
        EzCodeEmitter::ObjectFormat::Elf64Writer elfWriter;
        for (const auto &sym : symbols)
        {
            elfWriter.addSymbol(sym);
        }
        for (const auto &reloc : objectRelocs)
        {
            elfWriter.addRelocation(reloc);
        }
        outputBytes = elfWriter.write(sections);
    }
    else if (binDesc->getObjectFormat() == TargetObjectFormat::COFF)
    {
        EzCodeEmitter::ObjectFormat::CoffWriter coffWriter;
        for (const auto &sym : symbols)
        {
            coffWriter.addSymbol(sym);
        }
        for (const auto &reloc : objectRelocs)
        {
            coffWriter.addRelocation(reloc);
        }
        outputBytes = coffWriter.write(sections);
    }
    else
    {
        m_ctx.getDiagCollector()->error("EzCompiler", "Unsupported object format for emission");
        return false;
    }

    std::ofstream outFile(std::string(outputPath), std::ios::binary);
    if (!outFile.is_open())
    {
        m_ctx.getDiagCollector()->error("EzCompiler", "Failed to open output file for writing: {}", outputPath);
        return false;
    }

    outFile.write(reinterpret_cast<const char *>(outputBytes.data()), static_cast<std::streamsize>(outputBytes.size()));
    outFile.close();

    if (m_ctx.getOptions().verbose)
    {
        std::cout << "[Emission] Emitted " << outputBytes.size() << " bytes to " << outputPath << "\n";
    }

    return true;
}

} // namespace EzCompiler
