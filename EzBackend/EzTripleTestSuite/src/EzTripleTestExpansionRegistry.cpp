#include "EzTripleTestExpansionRegistry.h"

namespace EzTripleTestExpansionRegistry
{
void create(MirExpansionRuleRegistry *registry)
{
    using O = ExpansionOperand;
    using Op = MirInstructionOpCode;
    using Type = ExpansionOperandType;

    /* --- DATA MOVEMENT -------------------------------------------------------- */

    // Split 128-bit or wide movement into independent lower and upper slots
    ExpansionRuleBuilder(registry, Op::MOV)
            .emit(Op::MOV, { O::dstLo(), O::srcLo() })
            .emit(Op::MOV, { O::dstHi(), O::scrHi() });

    // In this case, dstLo represents the binding token, which is needed.
    ExpansionRuleBuilder(registry, Op::PUSH_ARG)
            .emit(Op::PUSH_ARG, { O::dstLo(), O::srcLo() })
            .emit(Op::PUSH_ARG, { O::dstLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::POP_ARG)
            .emit(Op::POP_ARG, { O::dstLo(), O::srcLo() })
            .emit(Op::POP_ARG, { O::dstLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::PUSH_RET)
            .emit(Op::PUSH_RET, { O::dstLo(), O::srcLo() })
            .emit(Op::PUSH_RET, { O::dstLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::POP_RET)
            .emit(Op::POP_RET, { O::dstLo(), O::srcLo() })
            .emit(Op::POP_RET, { O::dstLo(), O::scrHi() });

    /* --- MEMORY ACCESS -------------------------------------------------------- */

    // Sequentially load low chunk from base address, then high chunk with a stride offset
    ExpansionRuleBuilder(registry, Op::LOAD)
            .emit(Op::LOAD, { O::dstLo(), O::MemHalf(Type::SrcLow, 0) })
            .emit(Op::LOAD, { O::dstHi(), O::MemHalf(Type::SrcLow, 1) });

    // Sequentially store low and high pieces into memory
    ExpansionRuleBuilder(registry, Op::STORE)
            .emit(Op::STORE, { O::MemHalf(Type::DestLow, 0), O::srcLo() })
            .emit(Op::STORE, { O::MemHalf(Type::DestLow, 1), O::scrHi() });

    /* --- ARITHMETIC (ALU) ----------------------------------------------------- */

    // Multi-precision addition: ADD sets EFLAGS.CF, ADC consumes it
    ExpansionRuleBuilder(registry, Op::ADD)
            .emit(Op::ADD, { O::dstLo(), O::srcLo() })
            .emit(Op::ADC, { O::dstHi(), O::scrHi() });

    // Carry-aware addition continuation chain
    ExpansionRuleBuilder(registry, Op::ADC)
            .emit(Op::ADC, { O::dstLo(), O::srcLo() })
            .emit(Op::ADC, { O::dstHi(), O::scrHi() });

    // Multi-precision subtraction: SUB sets EFLAGS.CF (borrow), SBB consumes it
    ExpansionRuleBuilder(registry, Op::SUB)
            .emit(Op::SUB, { O::dstLo(), O::srcLo() })
            .emit(Op::SBB, { O::dstHi(), O::scrHi() });

    // Borrow-aware subtraction continuation chain
    ExpansionRuleBuilder(registry, Op::SBB)
            .emit(Op::SBB, { O::dstLo(), O::srcLo() })
            .emit(Op::SBB, { O::dstHi(), O::scrHi() });

    // Wide Multiplications: Safely routed to the runtime math library
    // Note: Emitting runtime library call + parameter list
    ExpansionRuleBuilder(registry, Op::MUL).emit("__multi3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::IMUL).emit("__multi3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    // Unsigned Division: D = D / S0
    ExpansionRuleBuilder(registry, Op::DIV).emit("__udivti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    // Signed Division: D = D / S0
    ExpansionRuleBuilder(registry, Op::IDIV).emit("__divti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    // Remainder/Modulo: D = D % S0
    ExpansionRuleBuilder(registry, Op::REM).emit("__umodti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    // Two's complement bitwise negation + 1 bit injection to simulate wide negation
    ExpansionRuleBuilder(registry, Op::NEG)
            .emit(Op::NOT, { O::dstLo() })
            .emit(Op::NOT, { O::dstHi() })
            .emit(Op::ADD, { O::dstLo(), O::Imm(FlexInt(1, 32)) })
            .emit(Op::ADC, { O::dstHi(), O::Imm(FlexInt(0, 32)) });

    /* --- BITWISE LOGIC -------------------------------------------------------- */

    ExpansionRuleBuilder(registry, Op::AND)
            .emit(Op::AND, { O::dstLo(), O::srcLo() })
            .emit(Op::AND, { O::dstHi(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::OR)
            .emit(Op::OR, { O::dstLo(), O::srcLo() })
            .emit(Op::OR, { O::dstHi(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::XOR)
            .emit(Op::XOR, { O::dstLo(), O::srcLo() })
            .emit(Op::XOR, { O::dstHi(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::NOT).emit(Op::NOT, { O::dstLo() }).emit(Op::NOT, { O::dstHi() });

    /* --- SHIFTS --- */

    ExpansionRuleBuilder(registry, Op::SHL).emit("__ashlti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::SHR).emit("__lshrti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::SAR).emit("__ashrti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    /* --- COMPARISONS ---------------------------------------------------------- */

    // Uses unbounded temporal registers: TempLo(0) and TempHi(0)
    ExpansionRuleBuilder(registry, Op::CMP)
            .emit(Op::MOV, { O::TempLo(0), O::dstLo() })
            .emit(Op::SUB, { O::TempLo(0), O::srcLo() })
            .emit(Op::MOV, { O::TempHi(0), O::dstHi() })
            .emit(Op::SBB, { O::TempHi(0), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::TEST)
            .emit(Op::MOV, { O::TempLo(0), O::dstLo() })
            .emit(Op::AND, { O::TempLo(0), O::srcLo() })
            .emit(Op::MOV, { O::TempHi(0), O::dstHi() })
            .emit(Op::AND, { O::TempHi(0), O::scrHi() })
            .emit(Op::OR, { O::TempLo(0), O::TempHi(0) });

    /* --- TYPE CASTING --------------------------------------------------------- */

    ExpansionRuleBuilder(registry, Op::TRUNC).emit(Op::MOV, { O::dstLo(), O::srcLo() });

    ExpansionRuleBuilder(registry, Op::ZEXT)
            .emit(Op::MOV, { O::dstLo(), O::srcLo() })
            .emit(Op::MOV, { O::dstHi(), O::Imm(FlexInt(0, 32)) });

    ExpansionRuleBuilder(registry, Op::SEXT)
            .emit(Op::MOV, { O::dstLo(), O::srcLo() })
            .emit(Op::MOV, { O::dstHi(), O::srcLo() })
            .emit(Op::SAR, { O::dstHi(), O::Imm(FlexInt(63, 32)) });

    ExpansionRuleBuilder(registry, Op::BITCAST)
            .emit(Op::MOV, { O::dstLo(), O::srcLo() })
            .emit(Op::MOV, { O::dstHi(), O::scrHi() });
}
}; // namespace EzTripleTestExpansionRegistry