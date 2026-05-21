#include "x64InstrSelector.h"

std::shared_ptr<InstructionSelectionTable> x64InstrSelector::getSelectionTable()
{
    using namespace asmjit::x86;
    auto res = std::make_shared<InstructionSelectionTable>();

    // A wildcard size vector. 0 means "Match any valid size for this operand".
    // ASMJIT natively infers the operation width based on the virtual/physical registers passed to it.
    const std::vector<size_t> ANY_SIZE = {};

    // ========================================================================
    // DATA MOVEMENT & MEMORY ACCESS
    // ========================================================================

    res->addRule(MirInstructionOpCode::MOV,
                 { static_cast<MirTargetInstructionId>(Inst::kIdMov),
                   { ExpectedOperandType::Register, ExpectedOperandType::AnyValue },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::LEA,
                 { static_cast<MirTargetInstructionId>(Inst::kIdLea),
                   { ExpectedOperandType::Register, ExpectedOperandType::AddressSource },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::LOAD,
                 { static_cast<MirTargetInstructionId>(Inst::kIdMov),
                   { ExpectedOperandType::Register, ExpectedOperandType::Memory },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::STORE,
                 { static_cast<MirTargetInstructionId>(Inst::kIdMov),
                   { ExpectedOperandType::Memory, ExpectedOperandType::AnyValue },
                   ANY_SIZE });

    // ========================================================================
    // ARITHMETIC (ALU)
    // ========================================================================

    res->addRule(MirInstructionOpCode::ADD,
                 { static_cast<MirTargetInstructionId>(Inst::kIdAdd),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::ADC,
                 { static_cast<MirTargetInstructionId>(Inst::kIdAdc),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::SUB,
                 { static_cast<MirTargetInstructionId>(Inst::kIdSub),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::SBB,
                 { static_cast<MirTargetInstructionId>(Inst::kIdSbb),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::MUL,
                 { static_cast<MirTargetInstructionId>(Inst::kIdMul),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::IMUL,
                 { static_cast<MirTargetInstructionId>(Inst::kIdImul),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::DIV,
                 { static_cast<MirTargetInstructionId>(Inst::kIdDiv),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::IDIV,
                 { static_cast<MirTargetInstructionId>(Inst::kIdIdiv),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    // TODO: x86_64 doesn't have a REM instruction. The modulo is just the RDX remainder
    // after a DIV/IDIV instruction. The backend emitter must handle pulling the result from RDX.
    res->addRule(MirInstructionOpCode::REM,
                 { static_cast<MirTargetInstructionId>(Inst::kIdIdiv),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::NEG,
                 { static_cast<MirTargetInstructionId>(Inst::kIdNeg), { ExpectedOperandType::Register }, ANY_SIZE });

    // ========================================================================
    // BITWISE LOGIC
    // ========================================================================

    res->addRule(MirInstructionOpCode::AND,
                 { static_cast<MirTargetInstructionId>(Inst::kIdAnd),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::OR,
                 { static_cast<MirTargetInstructionId>(Inst::kIdOr),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::XOR,
                 { static_cast<MirTargetInstructionId>(Inst::kIdXor),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::NOT,
                 { static_cast<MirTargetInstructionId>(Inst::kIdNot), { ExpectedOperandType::Register }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::SHL,
                 { static_cast<MirTargetInstructionId>(Inst::kIdShl),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::SHR,
                 { static_cast<MirTargetInstructionId>(Inst::kIdShr),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::SAR,
                 { static_cast<MirTargetInstructionId>(Inst::kIdSar),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    // ========================================================================
    // COMPARE & CONTROL FLOW
    // ========================================================================

    res->addRule(MirInstructionOpCode::CMP,
                 { static_cast<MirTargetInstructionId>(Inst::kIdCmp),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::TEST,
                 { static_cast<MirTargetInstructionId>(Inst::kIdTest),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::JMP,
                 { static_cast<MirTargetInstructionId>(Inst::kIdJmp), { ExpectedOperandType::Reference }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::JE,
                 { static_cast<MirTargetInstructionId>(Inst::kIdJe), { ExpectedOperandType::Reference }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::JNE,
                 { static_cast<MirTargetInstructionId>(Inst::kIdJne), { ExpectedOperandType::Reference }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::JG,
                 { static_cast<MirTargetInstructionId>(Inst::kIdJg), { ExpectedOperandType::Reference }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::JGE,
                 { static_cast<MirTargetInstructionId>(Inst::kIdJge), { ExpectedOperandType::Reference }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::JL,
                 { static_cast<MirTargetInstructionId>(Inst::kIdJl), { ExpectedOperandType::Reference }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::JLE,
                 { static_cast<MirTargetInstructionId>(Inst::kIdJle), { ExpectedOperandType::Reference }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::JA,
                 { static_cast<MirTargetInstructionId>(Inst::kIdJa), { ExpectedOperandType::Reference }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::JB,
                 { static_cast<MirTargetInstructionId>(Inst::kIdJb), { ExpectedOperandType::Reference }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::CALL,
                 { static_cast<MirTargetInstructionId>(Inst::kIdCall),
                   { ExpectedOperandType::Reference | ExpectedOperandType::Register },
                   ANY_SIZE });

    res->addRule(MirInstructionOpCode::RET, { static_cast<MirTargetInstructionId>(Inst::kIdRet), {}, ANY_SIZE });

    // ========================================================================
    // TYPE CASTING
    // ========================================================================

    // Standard down-cast. Typically resolved as a sub-register access (e.g. EAX instead of RAX) via MOV
    res->addRule(MirInstructionOpCode::TRUNC,
                 { static_cast<MirTargetInstructionId>(Inst::kIdMov),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    // Zero Extension (e.g., i8 -> i32)
    res->addRule(MirInstructionOpCode::ZEXT,
                 { static_cast<MirTargetInstructionId>(Inst::kIdMovzx),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    // Sign Extension (e.g., i8 -> i32 signed)
    res->addRule(MirInstructionOpCode::SEXT,
                 { static_cast<MirTargetInstructionId>(Inst::kIdMovsx),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    // Pure bit interpretation cast. Always a simple move.
    res->addRule(MirInstructionOpCode::BITCAST,
                 { static_cast<MirTargetInstructionId>(Inst::kIdMov),
                   { ExpectedOperandType::Register, ExpectedOperandType::RegImm },
                   ANY_SIZE });

    // ========================================================================
    // SYSTEM & SPECIAL
    // ========================================================================

    res->addRule(
            MirInstructionOpCode::SYSCALL,
            { static_cast<MirTargetInstructionId>(Inst::kIdSyscall), { ExpectedOperandType::AnyValue }, ANY_SIZE });

    res->addRule(MirInstructionOpCode::NOP, { static_cast<MirTargetInstructionId>(Inst::kIdNop), {}, ANY_SIZE });

    res->addRule(MirInstructionOpCode::HALT, { static_cast<MirTargetInstructionId>(Inst::kIdHlt), {}, ANY_SIZE });

    return res;
}
