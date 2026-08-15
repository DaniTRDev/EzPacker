#include "Legalizer/EzTestTripleExpansionRegistry.h"

namespace EzTestTriple
{
void CreateExpansionRegistry(MirExpansionRuleRegistry *registry)
{
    using O = ExpansionOperand;
    using Op = MirInstructionOpCode;
    using Type = ExpansionOperandType;

    // =========================================================================
    // 1. DATA MOVEMENT & ABI TOKENS
    // =========================================================================

    // Split 128-bit MOV into two 64-bit MOVs
    ExpansionRuleBuilder(registry, Op::MOV)
            .emit(Op::MOV, { O::dstLo(), O::srcLo() })
            .emit(Op::MOV, { O::dstHi(), O::scrHi() });

    // ABI argument & return bindings: token binding operand stays in dstLo
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

    // =========================================================================
    // 2. MEMORY ACCESS (LOAD / STORE)
    // =========================================================================

    // Low 64-bit chunk at base, High 64-bit chunk at base + 8
    ExpansionRuleBuilder(registry, Op::LOAD)
            .emit(Op::LOAD, { O::dstLo(), O::MemHalf(Type::SrcLow, 0, 0) })
            .emit(Op::LOAD, { O::dstHi(), O::MemHalf(Type::SrcLow, 1, 0) });

    ExpansionRuleBuilder(registry, Op::STORE)
            .emit(Op::STORE, { O::MemHalf(Type::DestLow, 0, 0), O::srcLo() })
            .emit(Op::STORE, { O::MemHalf(Type::DestLow, 1, 0), O::scrHi() });

    // =========================================================================
    // 3. ARITHMETIC & LOGIC (ALU)
    // =========================================================================

    // Multi-precision addition: ADD sets carry, ADC consumes it
    ExpansionRuleBuilder(registry, Op::ADD)
            .emit(Op::ADD, { O::dstLo(), O::srcLo() })
            .emit(Op::ADC, { O::dstHi(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::ADC)
            .emit(Op::ADC, { O::dstLo(), O::srcLo() })
            .emit(Op::ADC, { O::dstHi(), O::scrHi() });

    // Multi-precision subtraction: SUB sets borrow, SBB consumes it
    ExpansionRuleBuilder(registry, Op::SUB)
            .emit(Op::SUB, { O::dstLo(), O::srcLo() })
            .emit(Op::SBB, { O::dstHi(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::SBB)
            .emit(Op::SBB, { O::dstLo(), O::srcLo() })
            .emit(Op::SBB, { O::dstHi(), O::scrHi() });

    // Wide Multiplications and Divisions (Lower to runtime helpers)
    ExpansionRuleBuilder(registry, Op::MUL).emit("__multi3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::IMUL).emit("__multi3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::DIV).emit("__udivti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::IDIV).emit("__divti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::REM).emit("__umodti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    // 128-bit Negation: ~val + 1
    ExpansionRuleBuilder(registry, Op::NEG)
            .emit(Op::NOT, { O::dstLo() })
            .emit(Op::NOT, { O::dstHi() })
            .emit(Op::ADD, { O::dstLo(), O::Imm(FlexInt(1, 64)) })
            .emit(Op::ADC, { O::dstHi(), O::Imm(FlexInt(0, 64)) });

    // Bitwise Logic
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

    // Shifts
    ExpansionRuleBuilder(registry, Op::SHL).emit("__ashlti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::SHR).emit("__lshrti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::SAR).emit("__ashrti3", { O::dstLo(), O::dstHi(), O::srcLo(), O::scrHi() });

    // =========================================================================
    // 4. COMPARISONS & CASTS
    // =========================================================================
    ExpansionRuleBuilder(registry, Op::CMP)
            .emit(Op::MOV, { O::TempLo(0), O::dstLo() })
            .emit(Op::SUB, { O::TempLo(0), O::srcLo() })
            .emit(Op::MOV, { O::TempHi(0), O::dstHi() })
            .emit(Op::SBB, { O::TempHi(0), O::scrHi() });

    ExpansionRuleBuilder(registry, Op::TRUNC).emit(Op::MOV, { O::dstLo(), O::srcLo() });

    ExpansionRuleBuilder(registry, Op::ZEXT)
            .emit(Op::MOV, { O::dstLo(), O::srcLo() })
            .emit(Op::MOV, { O::dstHi(), O::Imm(FlexInt(0, 64)) });

    ExpansionRuleBuilder(registry, Op::SEXT)
            .emit(Op::MOV, { O::dstLo(), O::srcLo() })
            .emit(Op::MOV, { O::dstHi(), O::srcLo() })
            .emit(Op::SAR, { O::dstHi(), O::Imm(FlexInt(63, 64)) });

    ExpansionRuleBuilder(registry, Op::BITCAST)
            .emit(Op::MOV, { O::dstLo(), O::srcLo() })
            .emit(Op::MOV, { O::dstHi(), O::scrHi() });
}
}; // namespace EzTestTriple