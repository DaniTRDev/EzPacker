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
#include <fstream>

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

    for (MirFunction *func : mirCtx.getFunctions())
    {
        if (!func)
        {
            continue;
        }

        uint64_t fnOffset = textSection->getCurrentOffset();
        emitter.beginFunction(&emitterCtx, func->getName());

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

        emitter.endFunction(&emitterCtx);
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

    textSection->finalize();

    std::vector<uint8_t> outputBytes;
    if (binDesc->getObjectFormat() == TargetObjectFormat::ELF)
    {
        EzCodeEmitter::ObjectFormat::Elf64Writer elfWriter;
        for (const auto &sym : symbols)
        {
            elfWriter.addSymbol(sym);
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
