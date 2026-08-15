#include "InstructionSelector/EzTestTripleInstructionSelector.h"

namespace EzTestTriple
{
void CreateInstructionSelector(MirBuilderContext *ctx, MirInstructionSelector *selector)
{
    const auto &t = ctx->getTypeTable();

    // Cache Types
    MirType *i8 = t->i8();
    MirType *i16 = t->i16();
    MirType *i32 = t->i32();
    MirType *i64 = t->i64();
    MirType *f32 = t->f32();
    MirType *f64 = t->f64();

    MirRegisterBank *gprBank = Banks::GPR;
    MirRegisterBank *fprBank = Banks::FPR;

    MirRegisterClass *gpr8 = gprBank->getClass("GPR8");
    MirRegisterClass *gpr16 = gprBank->getClass("GPR16");
    MirRegisterClass *gpr32 = gprBank->getClass("GPR32");
    MirRegisterClass *gpr64 = gprBank->getClass("GPR64");
    MirRegisterClass *fpr32 = fprBank->getClass("FPR32");
    MirRegisterClass *fpr64 = fprBank->getClass("FPR64");

    InstructionSelectionRuleBuilder builder(selector);

// Macro chaining .act(ManualAction) and .act(SelectRegisterClass)
#define ADD_RULE(name, mirOpcode, predicate, targetId, ...)                                                            \
    builder.begin(name, mirOpcode)                                                                                     \
            .pred(ISelPreds::_and(ISelPreds::opcode(mirOpcode), predicate))                                            \
            .act(SelectorActions::ManualAction(targetId))                                                              \
            .act(SelectorActions::SelectRegisterClass({ __VA_ARGS__ }))                                                \
            .dump();

// Macro for instructions without register operands (branches, system ops, etc.)
#define ADD_RULE_NOREG(name, mirOpcode, predicate, targetId)                                                           \
    builder.begin(name, mirOpcode)                                                                                     \
            .pred(ISelPreds::_and(ISelPreds::opcode(mirOpcode), predicate))                                            \
            .act(SelectorActions::ManualAction(targetId))                                                              \
            .dump();

    // =========================================================================
    // 1. DATA MOVEMENT & MEMORY ACCESS
    // =========================================================================
    // MOV reg, reg
    ADD_RULE("mov_i8", MirInstructionOpCode::MOV, ISelPreds::operandMirType(0, i8), TargetInst::MOV8rr, gpr8, gpr8);
    ADD_RULE("mov_i16",
             MirInstructionOpCode::MOV,
             ISelPreds::operandMirType(0, i16),
             TargetInst::MOV16rr,
             gpr16,
             gpr16);
    ADD_RULE("mov_i32",
             MirInstructionOpCode::MOV,
             ISelPreds::operandMirType(0, i32),
             TargetInst::MOV32rr,
             gpr32,
             gpr32);
    ADD_RULE("mov_i64",
             MirInstructionOpCode::MOV,
             ISelPreds::operandMirType(0, i64),
             TargetInst::MOV64rr,
             gpr64,
             gpr64);
    ADD_RULE("mov_f32",
             MirInstructionOpCode::MOV,
             ISelPreds::operandMirType(0, f32),
             TargetInst::MOVSSrr,
             fpr32,
             fpr32);
    ADD_RULE("mov_f64",
             MirInstructionOpCode::MOV,
             ISelPreds::operandMirType(0, f64),
             TargetInst::MOVSDrr,
             fpr64,
             fpr64);
    ADD_RULE("mov_ptr",
             MirInstructionOpCode::MOV,
             ISelPreds::operandMirTypeKind(0, MirTypeKind::Pointer),
             TargetInst::MOV64rr,
             gpr64,
             gpr64);

    // LOAD reg, [mem] (Op 0: Reg, Op 1: Memory Address Base -> GPR64)
    ADD_RULE("load_i8", MirInstructionOpCode::LOAD, ISelPreds::operandMirType(0, i8), TargetInst::MOV8rm, gpr8, gpr64);
    ADD_RULE("load_i16",
             MirInstructionOpCode::LOAD,
             ISelPreds::operandMirType(0, i16),
             TargetInst::MOV16rm,
             gpr16,
             gpr64);
    ADD_RULE("load_i32",
             MirInstructionOpCode::LOAD,
             ISelPreds::operandMirType(0, i32),
             TargetInst::MOV32rm,
             gpr32,
             gpr64);
    ADD_RULE("load_i64",
             MirInstructionOpCode::LOAD,
             ISelPreds::operandMirType(0, i64),
             TargetInst::MOV64rm,
             gpr64,
             gpr64);
    ADD_RULE("load_f32",
             MirInstructionOpCode::LOAD,
             ISelPreds::operandMirType(0, f32),
             TargetInst::MOVSSrm,
             fpr32,
             gpr64);
    ADD_RULE("load_f64",
             MirInstructionOpCode::LOAD,
             ISelPreds::operandMirType(0, f64),
             TargetInst::MOVSDrm,
             fpr64,
             gpr64);
    ADD_RULE("load_ptr",
             MirInstructionOpCode::LOAD,
             ISelPreds::operandMirTypeKind(0, MirTypeKind::Pointer),
             TargetInst::MOV64rm,
             gpr64,
             gpr64);

    // STORE [mem], reg (Op 0: Memory Address Base -> GPR64, Op 1: Reg Value being stored)
    ADD_RULE("store_i8",
             MirInstructionOpCode::STORE,
             ISelPreds::operandMirType(1, i8),
             TargetInst::MOV8mr,
             gpr64,
             gpr8);
    ADD_RULE("store_i16",
             MirInstructionOpCode::STORE,
             ISelPreds::operandMirType(1, i16),
             TargetInst::MOV16mr,
             gpr64,
             gpr16);
    ADD_RULE("store_i32",
             MirInstructionOpCode::STORE,
             ISelPreds::operandMirType(1, i32),
             TargetInst::MOV32mr,
             gpr64,
             gpr32);
    ADD_RULE("store_i64",
             MirInstructionOpCode::STORE,
             ISelPreds::operandMirType(1, i64),
             TargetInst::MOV64mr,
             gpr64,
             gpr64);
    ADD_RULE("store_f32",
             MirInstructionOpCode::STORE,
             ISelPreds::operandMirType(1, f32),
             TargetInst::MOVSSmr,
             gpr64,
             fpr32);
    ADD_RULE("store_f64",
             MirInstructionOpCode::STORE,
             ISelPreds::operandMirType(1, f64),
             TargetInst::MOVSDmr,
             gpr64,
             fpr64);
    ADD_RULE("store_ptr",
             MirInstructionOpCode::STORE,
             ISelPreds::operandMirTypeKind(1, MirTypeKind::Pointer),
             TargetInst::MOV64mr,
             gpr64,
             gpr64);

    // =========================================================================
    // 2. INTEGER & FLOATING-POINT ALU
    // =========================================================================
    ADD_RULE("add_i8", MirInstructionOpCode::ADD, ISelPreds::operandMirType(0, i8), TargetInst::ADD8rr, gpr8, gpr8);
    ADD_RULE("add_i16",
             MirInstructionOpCode::ADD,
             ISelPreds::operandMirType(0, i16),
             TargetInst::ADD16rr,
             gpr16,
             gpr16);
    ADD_RULE("add_i32",
             MirInstructionOpCode::ADD,
             ISelPreds::operandMirType(0, i32),
             TargetInst::ADD32rr,
             gpr32,
             gpr32);
    ADD_RULE("add_i64",
             MirInstructionOpCode::ADD,
             ISelPreds::operandMirType(0, i64),
             TargetInst::ADD64rr,
             gpr64,
             gpr64);

    ADD_RULE("sub_i8", MirInstructionOpCode::SUB, ISelPreds::operandMirType(0, i8), TargetInst::SUB8rr, gpr8, gpr8);
    ADD_RULE("sub_i16",
             MirInstructionOpCode::SUB,
             ISelPreds::operandMirType(0, i16),
             TargetInst::SUB16rr,
             gpr16,
             gpr16);
    ADD_RULE("sub_i32",
             MirInstructionOpCode::SUB,
             ISelPreds::operandMirType(0, i32),
             TargetInst::SUB32rr,
             gpr32,
             gpr32);
    ADD_RULE("sub_i64",
             MirInstructionOpCode::SUB,
             ISelPreds::operandMirType(0, i64),
             TargetInst::SUB64rr,
             gpr64,
             gpr64);

    ADD_RULE("and_i32",
             MirInstructionOpCode::AND,
             ISelPreds::operandMirType(0, i32),
             TargetInst::AND32rr,
             gpr32,
             gpr32);
    ADD_RULE("and_i64",
             MirInstructionOpCode::AND,
             ISelPreds::operandMirType(0, i64),
             TargetInst::AND64rr,
             gpr64,
             gpr64);
    ADD_RULE("or_i32", MirInstructionOpCode::OR, ISelPreds::operandMirType(0, i32), TargetInst::OR32rr, gpr32, gpr32);
    ADD_RULE("or_i64", MirInstructionOpCode::OR, ISelPreds::operandMirType(0, i64), TargetInst::OR64rr, gpr64, gpr64);
    ADD_RULE("xor_i32",
             MirInstructionOpCode::XOR,
             ISelPreds::operandMirType(0, i32),
             TargetInst::XOR32rr,
             gpr32,
             gpr32);
    ADD_RULE("xor_i64",
             MirInstructionOpCode::XOR,
             ISelPreds::operandMirType(0, i64),
             TargetInst::XOR64rr,
             gpr64,
             gpr64);

    ADD_RULE("cmp_i32",
             MirInstructionOpCode::CMP,
             ISelPreds::operandMirType(0, i32),
             TargetInst::CMP32rr,
             gpr32,
             gpr32);
    ADD_RULE("cmp_i64",
             MirInstructionOpCode::CMP,
             ISelPreds::operandMirType(0, i64),
             TargetInst::CMP64rr,
             gpr64,
             gpr64);
    ADD_RULE("test_i32",
             MirInstructionOpCode::TEST,
             ISelPreds::operandMirType(0, i32),
             TargetInst::TEST32rr,
             gpr32,
             gpr32);
    ADD_RULE("test_i64",
             MirInstructionOpCode::TEST,
             ISelPreds::operandMirType(0, i64),
             TargetInst::TEST64rr,
             gpr64,
             gpr64);

    ADD_RULE("adc_i32",
             MirInstructionOpCode::ADC,
             ISelPreds::operandMirType(0, i32),
             TargetInst::ADC32rr,
             gpr32,
             gpr32);
    ADD_RULE("adc_i64",
             MirInstructionOpCode::ADC,
             ISelPreds::operandMirType(0, i64),
             TargetInst::ADC64rr,
             gpr64,
             gpr64);
    ADD_RULE("sbb_i32",
             MirInstructionOpCode::SBB,
             ISelPreds::operandMirType(0, i32),
             TargetInst::SBB32rr,
             gpr32,
             gpr32);
    ADD_RULE("sbb_i64",
             MirInstructionOpCode::SBB,
             ISelPreds::operandMirType(0, i64),
             TargetInst::SBB64rr,
             gpr64,
             gpr64);

    // Multiplications & Divisions
    ADD_RULE("imul_i16",
             MirInstructionOpCode::IMUL,
             ISelPreds::operandMirType(0, i16),
             TargetInst::IMUL16rr,
             gpr16,
             gpr16);
    ADD_RULE("imul_i32",
             MirInstructionOpCode::IMUL,
             ISelPreds::operandMirType(0, i32),
             TargetInst::IMUL32rr,
             gpr32,
             gpr32);
    ADD_RULE("imul_i64",
             MirInstructionOpCode::IMUL,
             ISelPreds::operandMirType(0, i64),
             TargetInst::IMUL64rr,
             gpr64,
             gpr64);

    ADD_RULE("idiv_i32", MirInstructionOpCode::IDIV, ISelPreds::operandMirType(0, i32), TargetInst::IDIV32r, gpr32);
    ADD_RULE("idiv_i64", MirInstructionOpCode::IDIV, ISelPreds::operandMirType(0, i64), TargetInst::IDIV64r, gpr64);
    ADD_RULE("div_i32", MirInstructionOpCode::DIV, ISelPreds::operandMirType(0, i32), TargetInst::DIV32r, gpr32);
    ADD_RULE("div_i64", MirInstructionOpCode::DIV, ISelPreds::operandMirType(0, i64), TargetInst::DIV64r, gpr64);

    // Floating Point Arithmetic
    ADD_RULE("fadd_f32",
             MirInstructionOpCode::FADD,
             ISelPreds::operandMirType(0, f32),
             TargetInst::FADD32rr,
             fpr32,
             fpr32);
    ADD_RULE("fadd_f64",
             MirInstructionOpCode::FADD,
             ISelPreds::operandMirType(0, f64),
             TargetInst::FADD64rr,
             fpr64,
             fpr64);
    ADD_RULE("fsub_f32",
             MirInstructionOpCode::FSUB,
             ISelPreds::operandMirType(0, f32),
             TargetInst::FSUB32rr,
             fpr32,
             fpr32);
    ADD_RULE("fsub_f64",
             MirInstructionOpCode::FSUB,
             ISelPreds::operandMirType(0, f64),
             TargetInst::FSUB64rr,
             fpr64,
             fpr64);
    ADD_RULE("fmul_f32",
             MirInstructionOpCode::FMUL,
             ISelPreds::operandMirType(0, f32),
             TargetInst::FMUL32rr,
             fpr32,
             fpr32);
    ADD_RULE("fmul_f64",
             MirInstructionOpCode::FMUL,
             ISelPreds::operandMirType(0, f64),
             TargetInst::FMUL64rr,
             fpr64,
             fpr64);
    ADD_RULE("fdiv_f32",
             MirInstructionOpCode::FDIV,
             ISelPreds::operandMirType(0, f32),
             TargetInst::FDIV32rr,
             fpr32,
             fpr32);
    ADD_RULE("fdiv_f64",
             MirInstructionOpCode::FDIV,
             ISelPreds::operandMirType(0, f64),
             TargetInst::FDIV64rr,
             fpr64,
             fpr64);
    ADD_RULE("fcmp_f32",
             MirInstructionOpCode::FCMP,
             ISelPreds::operandMirType(0, f32),
             TargetInst::FCMP32rr,
             fpr32,
             fpr32);
    ADD_RULE("fcmp_f64",
             MirInstructionOpCode::FCMP,
             ISelPreds::operandMirType(0, f64),
             TargetInst::FCMP64rr,
             fpr64,
             fpr64);

    // Unary & Shifts
    ADD_RULE("neg_i32", MirInstructionOpCode::NEG, ISelPreds::operandMirType(0, i32), TargetInst::NEG32r, gpr32);
    ADD_RULE("neg_i64", MirInstructionOpCode::NEG, ISelPreds::operandMirType(0, i64), TargetInst::NEG64r, gpr64);
    ADD_RULE("not_i32", MirInstructionOpCode::NOT, ISelPreds::operandMirType(0, i32), TargetInst::NOT32r, gpr32);
    ADD_RULE("not_i64", MirInstructionOpCode::NOT, ISelPreds::operandMirType(0, i64), TargetInst::NOT64r, gpr64);

    ADD_RULE("shl_i32",
             MirInstructionOpCode::SHL,
             ISelPreds::operandMirType(0, i32),
             TargetInst::SHL32rr,
             gpr32,
             gpr32);
    ADD_RULE("shl_i64",
             MirInstructionOpCode::SHL,
             ISelPreds::operandMirType(0, i64),
             TargetInst::SHL64rr,
             gpr64,
             gpr64);
    ADD_RULE("shr_i32",
             MirInstructionOpCode::SHR,
             ISelPreds::operandMirType(0, i32),
             TargetInst::SHR32rr,
             gpr32,
             gpr32);
    ADD_RULE("shr_i64",
             MirInstructionOpCode::SHR,
             ISelPreds::operandMirType(0, i64),
             TargetInst::SHR64rr,
             gpr64,
             gpr64);
    ADD_RULE("sar_i32",
             MirInstructionOpCode::SAR,
             ISelPreds::operandMirType(0, i32),
             TargetInst::SAR32rr,
             gpr32,
             gpr32);
    ADD_RULE("sar_i64",
             MirInstructionOpCode::SAR,
             ISelPreds::operandMirType(0, i64),
             TargetInst::SAR64rr,
             gpr64,
             gpr64);

    // =========================================================================
    // 3. CASTS & CONVERSIONS
    // =========================================================================
    ADD_RULE("zext_i32_i8",
             MirInstructionOpCode::ZEXT,
             ISelPreds::operandMirType(0, i32),
             TargetInst::MOVZX32rr8,
             gpr32,
             gpr8);
    ADD_RULE("sext_i32_i8",
             MirInstructionOpCode::SEXT,
             ISelPreds::operandMirType(0, i32),
             TargetInst::MOVSX32rr8,
             gpr32,
             gpr8);
    ADD_RULE("trunc_i8",
             MirInstructionOpCode::TRUNC,
             ISelPreds::operandMirType(0, i8),
             TargetInst::TRUNC8rr,
             gpr8,
             gpr64);
    ADD_RULE("trunc_i16",
             MirInstructionOpCode::TRUNC,
             ISelPreds::operandMirType(0, i16),
             TargetInst::TRUNC16rr,
             gpr16,
             gpr64);
    ADD_RULE("trunc_i32",
             MirInstructionOpCode::TRUNC,
             ISelPreds::operandMirType(0, i32),
             TargetInst::TRUNC32rr,
             gpr32,
             gpr64);
    ADD_RULE("fpext_f64",
             MirInstructionOpCode::FPEXT,
             ISelPreds::operandMirType(0, f64),
             TargetInst::CVTSS2SDrr,
             fpr64,
             fpr32);
    ADD_RULE("fptrunc_f32",
             MirInstructionOpCode::FPTRUNC,
             ISelPreds::operandMirType(0, f32),
             TargetInst::CVTSD2SSrr,
             fpr32,
             fpr64);
    ADD_RULE("sitofp_f32",
             MirInstructionOpCode::SITOFP,
             ISelPreds::operandMirType(0, f32),
             TargetInst::CVTSI2SSrr,
             fpr32,
             gpr32);
    ADD_RULE("sitofp_f64",
             MirInstructionOpCode::SITOFP,
             ISelPreds::operandMirType(0, f64),
             TargetInst::CVTSI2SDrr,
             fpr64,
             gpr64);
    ADD_RULE("fptosi_i32",
             MirInstructionOpCode::FPTOSI,
             ISelPreds::operandMirType(0, i32),
             TargetInst::CVTSS2SIrr,
             gpr32,
             fpr32);
    ADD_RULE("fptosi_i64",
             MirInstructionOpCode::FPTOSI,
             ISelPreds::operandMirType(0, i64),
             TargetInst::CVTSD2SIrr,
             gpr64,
             fpr64);

    // =========================================================================
    // 4. CONTROL FLOW, STACK & SYSTEM OPS
    // =========================================================================
    auto catchAllPred = [](const SelectionContext &) -> bool { return true; };

    ADD_RULE_NOREG("jmp", MirInstructionOpCode::JMP, catchAllPred, TargetInst::JMP);
    ADD_RULE_NOREG("je", MirInstructionOpCode::JE, catchAllPred, TargetInst::JE);
    ADD_RULE_NOREG("jne", MirInstructionOpCode::JNE, catchAllPred, TargetInst::JNE);
    ADD_RULE_NOREG("jg", MirInstructionOpCode::JG, catchAllPred, TargetInst::JG);
    ADD_RULE_NOREG("jge", MirInstructionOpCode::JGE, catchAllPred, TargetInst::JGE);
    ADD_RULE_NOREG("jl", MirInstructionOpCode::JL, catchAllPred, TargetInst::JL);
    ADD_RULE_NOREG("jle", MirInstructionOpCode::JLE, catchAllPred, TargetInst::JLE);
    ADD_RULE_NOREG("ja", MirInstructionOpCode::JA, catchAllPred, TargetInst::JA);
    ADD_RULE_NOREG("jb", MirInstructionOpCode::JB, catchAllPred, TargetInst::JB);

    ADD_RULE("push64", MirInstructionOpCode::PUSH, ISelPreds::operandMirType(0, i64), TargetInst::PUSH64r, gpr64);
    ADD_RULE("pop64", MirInstructionOpCode::POP, ISelPreds::operandMirType(0, i64), TargetInst::POP64r, gpr64);
    ADD_RULE("call", MirInstructionOpCode::CALL, catchAllPred, TargetInst::CALL, gpr64);

    ADD_RULE_NOREG("ret", MirInstructionOpCode::RET, catchAllPred, TargetInst::RET);
    ADD_RULE_NOREG("nop", MirInstructionOpCode::NOP, catchAllPred, TargetInst::NOP);
    ADD_RULE_NOREG("halt", MirInstructionOpCode::HALT, catchAllPred, TargetInst::HLT);
    ADD_RULE_NOREG("syscall", MirInstructionOpCode::SYSCALL, catchAllPred, TargetInst::SYSCALL);

    // =========================================================================
    // 4. PSEUDO-INSTRUCTIONS
    // =========================================================================
    ADD_RULE("pseudo_dalloc",
             MirInstructionOpCode::DALLOC,
             ISelPreds::operandMirTypeKind(0, MirTypeKind::Pointer),
             TargetInst::DYNAMIC_ALLOC,
             gpr64);

    ADD_RULE("pseudo_alloc",
             MirInstructionOpCode::ALLOC,
             ISelPreds::operandMirTypeKind(0, MirTypeKind::Pointer),
             TargetInst::ALLOC,
             gpr64);

#undef ADD_RULE_NOREG
#undef ADD_RULE
}
} // namespace EzTestTriple