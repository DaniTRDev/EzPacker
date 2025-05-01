#ifndef EZPACKER_INSTRUCTIONLIFTERS_H
#define EZPACKER_INSTRUCTIONLIFTERS_H

#include "EzLifterCommon.h"
#include "InstructionLifterDefines.h"

inline static std::map<DecodedInstructionType, std::shared_ptr<IInstructionLifter>> g_lifters;

MAKE_INSTRUCTION_LIFTER(_Mov, [&](const InstructionLiftContext &context) -> bool {
     return false;
});

MAKE_INSTRUCTION_LIFTER(Add, [&](const InstructionLiftContext &context) -> bool {
    
    llvm::Value *lhs = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context.m_context), 10);
    llvm::Value *rhs = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context.m_context), 20);
    
    // Emit the add instruction: %result = add i32 10, 20
    llvm::Value *result = context.m_builder.CreateAdd(lhs, rhs);
    
    return result != nullptr;
});

MAKE_INSTRUCTION_LIFTER(And, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Branch, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Call, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Compare, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Divide, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Exchange, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Jump, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(LoadEffectiveAddress, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Multiply, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Nop, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Not, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Or, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Pop, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Prefetch, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Push, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Return, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(RotateLeft, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(RotateRight, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(SetFlags, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(ShiftLeft, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(ShiftRight, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(SignExtend, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Subtract, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(SysCall, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Test, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(Xor, [&](const InstructionLiftContext &context) -> bool { return false; });

MAKE_INSTRUCTION_LIFTER(ZeroExtend, [&](const InstructionLiftContext &context) -> bool { return false; });

#endif // EZPACKER_INSTRUCTIONLIFTERS_H
