#include "EzTripleTestSelector.h"

void EzTripleTestSelector::create(MirBuilderContext *ctx, MirInstructionSelector *selector)
{
    using namespace EzTripleTestInstructionSet;
    const auto &t = ctx->getTypeTable();

    // Cache legal primitive MIR types matching legalizer definitions
    MirType *i8 = t->i8();
    MirType *i16 = t->i16();
    MirType *i32 = t->i32();
    MirType *i64 = t->i64();
    MirType *f32 = t->f32();
    MirType *f64 = t->f64();

    InstructionSelectionRuleBuilder builder(selector);

// Helper macro to reduce rule definition boilerplate
#define ADD_RULE(name, mirOpcode, predicate, targetId)                                                                 \
    builder.begin(name, mirOpcode)                                                                                     \
            .pred(ISelPreds::_and(ISelPreds::opcode(mirOpcode), predicate))                                            \
            .act(SelectorActions::ManualAction(targetId))                                                              \
            .dump();

    // =========================================================================
    // 1. DATA MOVEMENT & MEMORY ACCESS
    // =========================================================================
    // MOV
    ADD_RULE("mov_i8", MirInstructionOpCode::MOV, ISelPreds::operandMirType(0, i8), TargetInst::MOV8rr);
    ADD_RULE("mov_i16", MirInstructionOpCode::MOV, ISelPreds::operandMirType(0, i16), TargetInst::MOV16rr);
    ADD_RULE("mov_i32", MirInstructionOpCode::MOV, ISelPreds::operandMirType(0, i32), TargetInst::MOV32rr);
    ADD_RULE("mov_i64", MirInstructionOpCode::MOV, ISelPreds::operandMirType(0, i64), TargetInst::MOV64rr);
    ADD_RULE("mov_f32", MirInstructionOpCode::MOV, ISelPreds::operandMirType(0, f32), TargetInst::MOVSSrr);
    ADD_RULE("mov_f64", MirInstructionOpCode::MOV, ISelPreds::operandMirType(0, f64), TargetInst::MOVSDrr);

    // LOAD
    ADD_RULE("load_i8", MirInstructionOpCode::LOAD, ISelPreds::operandMirType(0, i8), TargetInst::MOV8rm);
    ADD_RULE("load_i16", MirInstructionOpCode::LOAD, ISelPreds::operandMirType(0, i16), TargetInst::MOV16rm);
    ADD_RULE("load_i32", MirInstructionOpCode::LOAD, ISelPreds::operandMirType(0, i32), TargetInst::MOV32rm);
    ADD_RULE("load_i64", MirInstructionOpCode::LOAD, ISelPreds::operandMirType(0, i64), TargetInst::MOV64rm);
    ADD_RULE("load_f32", MirInstructionOpCode::LOAD, ISelPreds::operandMirType(0, f32), TargetInst::MOVSSrm);
    ADD_RULE("load_f64", MirInstructionOpCode::LOAD, ISelPreds::operandMirType(0, f64), TargetInst::MOVSDrm);

    // STORE
    ADD_RULE("store_i8", MirInstructionOpCode::STORE, ISelPreds::operandMirType(1, i8), TargetInst::MOV8mr);
    ADD_RULE("store_i16", MirInstructionOpCode::STORE, ISelPreds::operandMirType(1, i16), TargetInst::MOV16mr);
    ADD_RULE("store_i32", MirInstructionOpCode::STORE, ISelPreds::operandMirType(1, i32), TargetInst::MOV32mr);
    ADD_RULE("store_i64", MirInstructionOpCode::STORE, ISelPreds::operandMirType(1, i64), TargetInst::MOV64mr);
    ADD_RULE("store_f32", MirInstructionOpCode::STORE, ISelPreds::operandMirType(1, f32), TargetInst::MOVSSmr);
    ADD_RULE("store_f64", MirInstructionOpCode::STORE, ISelPreds::operandMirType(1, f64), TargetInst::MOVSDmr);

    // ALLOC
    // Alloc WILL BE lowered during MirFrameLowererPass.

    // =========================================================================
    // 2. ARITHMETIC & LOGIC (ALU)
    // =========================================================================
    // ADD
    ADD_RULE("add_i8", MirInstructionOpCode::ADD, ISelPreds::operandMirType(0, i8), TargetInst::ADD8rr);
    ADD_RULE("add_i16", MirInstructionOpCode::ADD, ISelPreds::operandMirType(0, i16), TargetInst::ADD16rr);
    ADD_RULE("add_i32", MirInstructionOpCode::ADD, ISelPreds::operandMirType(0, i32), TargetInst::ADD32rr);
    ADD_RULE("add_i64", MirInstructionOpCode::ADD, ISelPreds::operandMirType(0, i64), TargetInst::ADD64rr);

    // SUB
    ADD_RULE("sub_i8", MirInstructionOpCode::SUB, ISelPreds::operandMirType(0, i8), TargetInst::SUB8rr);
    ADD_RULE("sub_i16", MirInstructionOpCode::SUB, ISelPreds::operandMirType(0, i16), TargetInst::SUB16rr);
    ADD_RULE("sub_i32", MirInstructionOpCode::SUB, ISelPreds::operandMirType(0, i32), TargetInst::SUB32rr);
    ADD_RULE("sub_i64", MirInstructionOpCode::SUB, ISelPreds::operandMirType(0, i64), TargetInst::SUB64rr);

    // AND, OR, XOR
    ADD_RULE("and_i32", MirInstructionOpCode::AND, ISelPreds::operandMirType(0, i32), TargetInst::AND32rr);
    ADD_RULE("and_i64", MirInstructionOpCode::AND, ISelPreds::operandMirType(0, i64), TargetInst::AND64rr);
    ADD_RULE("or_i32", MirInstructionOpCode::OR, ISelPreds::operandMirType(0, i32), TargetInst::OR32rr);
    ADD_RULE("or_i64", MirInstructionOpCode::OR, ISelPreds::operandMirType(0, i64), TargetInst::OR64rr);
    ADD_RULE("xor_i32", MirInstructionOpCode::XOR, ISelPreds::operandMirType(0, i32), TargetInst::XOR32rr);
    ADD_RULE("xor_i64", MirInstructionOpCode::XOR, ISelPreds::operandMirType(0, i64), TargetInst::XOR64rr);

    // CMP & TEST
    ADD_RULE("cmp_i32", MirInstructionOpCode::CMP, ISelPreds::operandMirType(0, i32), TargetInst::CMP32rr);
    ADD_RULE("cmp_i64", MirInstructionOpCode::CMP, ISelPreds::operandMirType(0, i64), TargetInst::CMP64rr);
    ADD_RULE("test_i32", MirInstructionOpCode::TEST, ISelPreds::operandMirType(0, i32), TargetInst::TEST32rr);
    ADD_RULE("test_i64", MirInstructionOpCode::TEST, ISelPreds::operandMirType(0, i64), TargetInst::TEST64rr);

    // ADC & SBB
    ADD_RULE("adc_i32", MirInstructionOpCode::ADC, ISelPreds::operandMirType(0, i32), TargetInst::ADC32rr);
    ADD_RULE("adc_i64", MirInstructionOpCode::ADC, ISelPreds::operandMirType(0, i64), TargetInst::ADC64rr);
    ADD_RULE("sbb_i32", MirInstructionOpCode::SBB, ISelPreds::operandMirType(0, i32), TargetInst::SBB32rr);
    ADD_RULE("sbb_i64", MirInstructionOpCode::SBB, ISelPreds::operandMirType(0, i64), TargetInst::SBB64rr);

    // WIDE ALU (Legalizer promotes sub-i16 wide ops up to i16, i32, i64)
    ADD_RULE("imul_i16", MirInstructionOpCode::IMUL, ISelPreds::operandMirType(0, i16), TargetInst::IMUL16rr);
    ADD_RULE("imul_i32", MirInstructionOpCode::IMUL, ISelPreds::operandMirType(0, i32), TargetInst::IMUL32rr);
    ADD_RULE("imul_i64", MirInstructionOpCode::IMUL, ISelPreds::operandMirType(0, i64), TargetInst::IMUL64rr);

    ADD_RULE("idiv_i32", MirInstructionOpCode::IDIV, ISelPreds::operandMirType(0, i32), TargetInst::IDIV32r);
    ADD_RULE("idiv_i64", MirInstructionOpCode::IDIV, ISelPreds::operandMirType(0, i64), TargetInst::IDIV64r);
    ADD_RULE("div_i32", MirInstructionOpCode::DIV, ISelPreds::operandMirType(0, i32), TargetInst::DIV32r);
    ADD_RULE("div_i64", MirInstructionOpCode::DIV, ISelPreds::operandMirType(0, i64), TargetInst::DIV64r);

    // UNARY & SHIFTS
    ADD_RULE("neg_i32", MirInstructionOpCode::NEG, ISelPreds::operandMirType(0, i32), TargetInst::NEG32r);
    ADD_RULE("neg_i64", MirInstructionOpCode::NEG, ISelPreds::operandMirType(0, i64), TargetInst::NEG64r);
    ADD_RULE("not_i32", MirInstructionOpCode::NOT, ISelPreds::operandMirType(0, i32), TargetInst::NOT32r);
    ADD_RULE("not_i64", MirInstructionOpCode::NOT, ISelPreds::operandMirType(0, i64), TargetInst::NOT64r);

    ADD_RULE("shl_i32", MirInstructionOpCode::SHL, ISelPreds::operandMirType(0, i32), TargetInst::SHL32rCL);
    ADD_RULE("shr_i32", MirInstructionOpCode::SHR, ISelPreds::operandMirType(0, i32), TargetInst::SHR32rCL);
    ADD_RULE("sar_i32", MirInstructionOpCode::SAR, ISelPreds::operandMirType(0, i32), TargetInst::SAR32rCL);

    // =========================================================================
    // 3. CASTING & EXTENSIONS
    // =========================================================================
    ADD_RULE("zext_i32", MirInstructionOpCode::ZEXT, ISelPreds::operandMirType(0, i32), TargetInst::MOVZX32rr8);
    ADD_RULE("sext_i32", MirInstructionOpCode::SEXT, ISelPreds::operandMirType(0, i32), TargetInst::MOVSX32rr8);
    ADD_RULE("trunc_i8", MirInstructionOpCode::TRUNC, ISelPreds::operandMirType(0, i8), TargetInst::TRUNC8rr);
    ADD_RULE("fpext_f64", MirInstructionOpCode::FPEXT, ISelPreds::operandMirType(0, f64), TargetInst::CVTSS2SDrr);
    ADD_RULE("bitcast", MirInstructionOpCode::BITCAST, ISelPreds::operandMirType(0, i64), TargetInst::MOVDQUrr);

    // =========================================================================
    // 4. CONTROL FLOW & SYSTEM RULES
    // =========================================================================
    auto catchAllPred = [](const SelectionContext &) -> bool { return true; };

    ADD_RULE("jmp", MirInstructionOpCode::JMP, catchAllPred, TargetInst::JMP);
    ADD_RULE("je", MirInstructionOpCode::JE, catchAllPred, TargetInst::JE);
    ADD_RULE("jne", MirInstructionOpCode::JNE, catchAllPred, TargetInst::JNE);
    ADD_RULE("jg", MirInstructionOpCode::JG, catchAllPred, TargetInst::JG);
    ADD_RULE("jge", MirInstructionOpCode::JGE, catchAllPred, TargetInst::JGE);
    ADD_RULE("jl", MirInstructionOpCode::JL, catchAllPred, TargetInst::JL);
    ADD_RULE("jle", MirInstructionOpCode::JLE, catchAllPred, TargetInst::JLE);
    ADD_RULE("ja", MirInstructionOpCode::JA, catchAllPred, TargetInst::JA);
    ADD_RULE("jb", MirInstructionOpCode::JB, catchAllPred, TargetInst::JB);

    ADD_RULE("push64", MirInstructionOpCode::PUSH, ISelPreds::operandMirType(1, i64), TargetInst::PUSH64r);
    ADD_RULE("pop64", MirInstructionOpCode::POP, ISelPreds::operandMirType(1, i64), TargetInst::POP64r);

    ADD_RULE("call", MirInstructionOpCode::CALL, catchAllPred, TargetInst::CALL);
    ADD_RULE("ret", MirInstructionOpCode::RET, catchAllPred, TargetInst::RET);
    ADD_RULE("nop", MirInstructionOpCode::NOP, catchAllPred, TargetInst::NOP);
    ADD_RULE("halt", MirInstructionOpCode::HALT, catchAllPred, TargetInst::HLT);
    ADD_RULE("syscall", MirInstructionOpCode::SYSCALL, catchAllPred, TargetInst::SYSCALL);

#undef ADD_RULE
}