#include "x64Emitter.h"

x64Emitter::x64Emitter(MirEmitter *emitter) : m_emitter(emitter) {}

bool x64Emitter::emitFunc(MirFunction *func, CodeBuffer *buff)
{
    // First, create labels for function blocks.
    if (!emitFuncLabels(func))
    {
        throw std::runtime_error("Internal Compiler Error: Could not emit function label");
        return false;
    }

    for (auto block : *func->getBlocks())
    {
        if (!emitBlock(block))
        {
            throw std::runtime_error("Internal Compiler Error: Could not emit function block");
            return false;
        }
    }

    LOG_DEBUG(std::format("--- ASM FOR FUNCTION {} ---\n", func->getName()), "x64Emitter");
    LOG_DEBUG(std::format("{}\n", m_logger.data()), "x64Emitter");

    return true;
}

bool x64Emitter::emitFuncLabels(MirFunction *func)
{
    for (auto block : *func->getBlocks())
    {
        if (m_asmjitLabels.contains(block->getId()))
        {
            throw std::runtime_error(
                    "Internal Compiler Error: Could not emit function label because it was already emitted");
            return false;
        }

        asmjit::Label label = m_assembler.new_label();
        m_asmjitLabels[block->getId()] = label;
    }

    return true;
}

bool x64Emitter::emitBlock(MirBlock *block)
{
    asmjit::Label label = m_asmjitLabels[block->getId()];
    m_assembler.bind(label);

    for (auto instr : *block->getInstructions())
    {
        if (!emitInstr(instr))
        {
            throw std::runtime_error("Internal Compiler Error: Could not emit block instructions");
            return false;
        }
    }

    return true;
}

bool x64Emitter::emitInstr(MirInstruction *instr)
{
    MirTargetInstructionId id = instr->getTargetId();
    if (id == MIRID_INVALID)
    {
        throw std::runtime_error("Internal Compiler Error: MirInstruction was not assigned a target ID");
        return false;
    }

    auto operands = instr->getOperands();
    size_t numOps = operands->m_numElems;

    // AsmJit can take up to 4 operands per instruction. We need to parse them out.
    asmjit::Operand ops[4];
    size_t i = 0;

    for (auto opIt = operands->begin(); opIt != operands->end() && i < 4; ++opIt, ++i)
    {
        ops[i] = getAsmjitOperandFromMirOperand(*opIt, static_cast<asmjit::x86::Inst::Id>(id));
    }

    // Call the correct emit overload based on the number of operands
    asmjit::Error err = asmjit::Error::kOk;
    switch (numOps)
    {
        case 0:
            err = m_assembler.emit(id);
            break;
        case 1:
            err = m_assembler.emit(id, ops[0]);
            break;
        case 2:
            err = m_assembler.emit(id, ops[0], ops[1]);
            break;
        case 3:
            err = m_assembler.emit(id, ops[0], ops[1], ops[2]);
            break;
        case 4:
            err = m_assembler.emit(id, ops[0], ops[1], ops[2], ops[3]);
            break;
        default:
            throw std::runtime_error("Internal Compiler Error: Instruction has too many operands for AsmJit");
    }

    if (err != asmjit::Error::kOk)
    {
        throw std::runtime_error(std::format("Internal Compiler Error: Could not emit instruction. AsmJit Error: {}",
                                             asmjit::DebugUtils::error_as_string(err)));
        return false;
    }

    return true;
}

bool x64Emitter::load()
{
    m_env.set_arch(asmjit::Arch::kX64);

    m_codeHolder.reset();

    if (m_codeHolder.init(m_env, 0) != asmjit::Error::kOk || m_codeHolder.attach(&m_assembler) != asmjit::Error::kOk)
    {
        throw std::runtime_error("Internal Compiler Error: Could not initialize asmjit");
    }

    m_logger.add_flags(asmjit::FormatFlags::kMachineCode);
    m_codeHolder.set_logger(&m_logger);

    LOG_DEBUG("Initialized x64 emitter", "x64Emitter");
    return true;
}

asmjit::Operand x64Emitter::getAsmjitOperandFromMirOperand(MirOperand *operand, asmjit::x86::Inst::Id instr)
{
    if (!operand)
        return asmjit::Operand(); // Empty/None operand

    switch (operand->getType())
    {
        case MirOperandType::Register:
        {
            MirRegister *reg = operand->get<MirRegister>();

            if (reg->isVirtual())
            {
                throw std::runtime_error("Internal Compiler Error: Virtual register reached emission phase");
            }

            // Map physical register sizes to correct AsmJit objects
            size_t size = reg->getSizeInBytes();
            uint32_t id = static_cast<uint32_t>(reg->getRegId());

            if (reg->getMirType()->getKind() == MirTypeKind::FloatingPoint)
            {
                // Vector/Float registers (XMM/YMM)
                if (size <= 16)
                    return asmjit::x86::xmm(id);
                if (size == 32)
                    return asmjit::x86::ymm(id);
            }
            else
            {
                // General Purpose Registers (GPRs)
                if (size == 1)
                    return asmjit::x86::gp8(id);
                if (size == 2)
                    return asmjit::x86::gp16(id);
                if (size == 4)
                    return asmjit::x86::gp32(id);
                if (size == 8)
                    return asmjit::x86::gp64(id);
            }

            throw std::runtime_error("Internal Compiler Error: Invalid register size for x64");
        }

        case MirOperandType::Integer:
        {
            MirInteger *imm = operand->get<MirInteger>();
            return asmjit::imm(imm->getValue());
        }

        case MirOperandType::Double:
        {
            MirDouble *imm = operand->get<MirDouble>();
            return asmjit::imm(imm->getValue());
        }

        case MirOperandType::Memory:
        {
            MirMemory *mem = operand->get<MirMemory>();
            asmjit::x86::Mem asmjitMem;

            // Process Base Register
            if (mem->getBase() && mem->getBase()->isOfType<MirRegister>())
            {
                MirRegister *baseReg = mem->getBase()->get<MirRegister>();
                // x64 memory addresses always use 64-bit base registers
                asmjitMem = asmjit::x86::ptr(asmjit::x86::gp64(baseReg->getRegId()));
            }
            else
            {
                // Absolute addressing (no base register)
                asmjitMem = asmjit::x86::ptr(0);
            }

            // Process Displacement / Offset
            if (mem->getDisplacement())
            {
                // [Base + ImmediateOffset]
                asmjitMem.set_offset(mem->getDisplacement()->get<MirInteger>()->getValue());
            }

            // Set Memory Access Size (Critical for instructions like MOV [RAX], 0)
            asmjitMem.set_size(static_cast<uint32_t>(mem->getSizeInBytes()));

            return asmjitMem;
        }

        case MirOperandType::Reference:
        {
            MirReference *ref = operand->get<MirReference>();

            if (ref->isBlock())
            {
                // Branch to a basic block
                return m_asmjitLabels.at(ref->getRefId());
            }
            else if (ref->isFunction() || ref->isDataEntry())
            {
                // External function or global data reference.
                // We create a named label so AsmJit generates a relocation entry.
                std::string symbolName = std::format(".ref_{}", ref->getRefId());
                asmjit::Label extLabel = m_assembler.new_named_label(symbolName.c_str());
                return extLabel;
            }

            throw std::runtime_error("Internal Compiler Error: Unhandled MirReference type");
        }

        case MirOperandType::FrameIndex:
        {
            // By the time we reach the Emitter, all FrameIndices should have been
            // converted into MirMemory operands (e.g., [RSP + offset]) by the StackFrameLowerer.
            throw std::runtime_error(
                    "Internal Compiler Error: FrameIndex survived to emission phase. Run StackFrameLowerer first.");
        }

        default:
            throw std::runtime_error("Internal Compiler Error: Unknown MirOperandType");
    }
}