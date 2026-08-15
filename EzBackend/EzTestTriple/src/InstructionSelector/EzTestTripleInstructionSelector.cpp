#include "InstructionSelector/EzTestTripleInstructionSelector.h"

namespace EzTestTriple
{
/**
 * Custom ISel Action to lower `MOV FPR, FloatImm` via integer register bitcasting:
 *   1. Bitcast float/double value to uint32_t / uint64_t.
 *   2. Insert `MOV gpr, int_imm` before the current iterator.
 *   3. Convert the current instruction to `MOVDtoFPR` / `MOVQtoFPR` (dst: FPR, src: GPR).
 */
inline InstructionSelAction LowerFloatImmediateAction(size_t bitWidth,
                                                      MirTargetInstructionDesc *movIntImmTarget,
                                                      MirTargetInstructionDesc *toFprTarget,
                                                      MirRegisterClass *gprClass,
                                                      MirRegisterClass *fprClass)
{
    return [bitWidth, movIntImmTarget, toFprTarget, gprClass, fprClass](SelectionContext &ctx) -> SelectionResult
    {
        MirInstruction *instr = *ctx.m_it;

        if (!instr)
            return SelectionResult::SelectionError;

        auto &operands = instr->getOperands();

        MirOperand *dstOp = operands[0];
        MirFloat *srcOp = operands[1]->get<MirFloat>();
        const FlexFloat &scrVal = srcOp->getValue();

        // 1. Extract and bitcast raw float/double bits
        uint64_t rawBits = 0;
        if (bitWidth == 32)
        {
            float fVal = scrVal.getFloat();
            uint32_t u32Val = 0;

            std::memcpy(&u32Val, &fVal, sizeof(float));
            rawBits = static_cast<uint64_t>(u32Val);
        }
        else if (bitWidth == 64)
        {
            double dVal = scrVal.getDouble();
            std::memcpy(&rawBits, &dVal, sizeof(double));
        }
        else
        {
            ctx.m_ctx->getDiagCollector()->builder(Diag_Error, "EzTestTriple::LowerFloatImmediateAction")
                    << "Can't convert mov fp, fpImm because imm is bigger than 64bits" << srcOp->getSourceRef();

            return SelectionResult::SelectionError;
        }

        MirOperandBuilder oBuilder(ctx.m_ctx);
        // 2. Allocate a temporary virtual GPR for the integer bits
        MirOperand *tempGpr =
                oBuilder.buildVReg(bitWidth == 32 ? ctx.m_ctx->getTypeTable()->i32() : ctx.m_ctx->getTypeTable()->i64(),
                                   "",
                                   nullptr,
                                   gprClass);

        // 3. Create integer immediate operand
        MirOperand *intImmOp =
                oBuilder.buildInt(bitWidth == 32 ? ctx.m_ctx->getTypeTable()->i32() : ctx.m_ctx->getTypeTable()->i64(),
                                  FlexInt(rawBits, bitWidth));

        // 4. Build and insert the integer immediate load: `MOV tempGpr, rawBits`
        MirInstructionBuilder iBuilder(ctx.m_ctx,
                                       instr->getOwner(),
                                       InsertionType::InsertBefore,
                                       instr->getOwner()->getInstructions().begin());

        MirInstruction *loadIntInst =
                iBuilder.buildTarget(movIntImmTarget, instr->getSourceRef(), { tempGpr, intImmOp });

        // 5. Mutate the current MOV instruction to MOVDtoFPR / MOVQtoFPR (tempGpr -> dstFpr)
        instr->setOpcode(MirInstructionOpCode::TARGET_INST);
        instr->setTargetDesc(toFprTarget);

        // Switch the operand so instead an immediate is a temporal GPR.
        operands[0]->get<MirRegister>()->setClass(fprClass);
        operands[1] = tempGpr;

        return SelectionResult::Selected;
    };
}

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

    // Register Banks & Classes
    MirRegisterBank *gprBank = Banks::GPR;
    MirRegisterBank *fprBank = Banks::FPR;

    MirRegisterClass *gpr8 = gprBank->getClass("GPR8");
    MirRegisterClass *gpr16 = gprBank->getClass("GPR16");
    MirRegisterClass *gpr32 = gprBank->getClass("GPR32");
    MirRegisterClass *gpr64 = gprBank->getClass("GPR64");
    MirRegisterClass *fpr32 = fprBank->getClass("FPR32");
    MirRegisterClass *fpr64 = fprBank->getClass("FPR64");

    InstructionSelectionRuleBuilder builder(selector);

#define ADD_RULE(name, mirOpcode, predicate, targetId, ...)                                                            \
    builder.begin(name, mirOpcode)                                                                                     \
            .pred(ISelPreds::_and(ISelPreds::opcode(mirOpcode), predicate))                                            \
            .act(SelectorActions::ManualAction(targetId))                                                              \
            .act(SelectorActions::SelectRegisterClass({ __VA_ARGS__ }))                                                \
            .dump();

#define ADD_RULE_NOREG(name, mirOpcode, predicate, targetId)                                                           \
    builder.begin(name, mirOpcode)                                                                                     \
            .pred(ISelPreds::_and(ISelPreds::opcode(mirOpcode), predicate))                                            \
            .act(SelectorActions::ManualAction(targetId))                                                              \
            .dump();

    // Predicate Aliases using strictly ISelPreds API
    auto isReg = [](size_t opIdx) { return ISelPreds::operandType(opIdx, MirOperandType::Register); };
    auto isIntImm = [](size_t opIdx) { return ISelPreds::operandType(opIdx, MirOperandType::Integer); };
    auto isMem = [](size_t opIdx) { return ISelPreds::operandType(opIdx, MirOperandType::Memory); };
    auto isGlobalRef = [](size_t opIdx) { return ISelPreds::operandIsGlobalRef(opIdx); };
    auto isPtr = [](size_t opIdx) { return ISelPreds::operandMirTypeKind(opIdx, MirTypeKind::Pointer); };
    auto isImmS32 = [](size_t opIdx) { return ISelPreds::operandIntFitsInSigned(opIdx, 32); };
    auto isFloatImm = [](size_t opIdx) { return ISelPreds::operandType(opIdx, MirOperandType::FloatingPoint); };

    auto isSmallModel = ISelPreds::codeModel(CodeModel::Small);
    auto isLargeModel = ISelPreds::codeModel(CodeModel::Large);

    auto catchAll = [](const SelectionContext &) -> bool { return true; };

    // =========================================================================
    // 1. DATA MOVEMENT (MOV, LEA, BITCAST)
    // =========================================================================
    // Register-to-Register
    ADD_RULE("mov_i8_rr",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::MOV8rr,
             gpr8,
             gpr8);
    ADD_RULE("mov_i16_rr",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::MOV16rr,
             gpr16,
             gpr16);
    ADD_RULE("mov_i32_rr",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::MOV32rr,
             gpr32,
             gpr32);
    ADD_RULE("mov_i64_rr",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::MOV64rr,
             gpr64,
             gpr64);
    ADD_RULE("mov_ptr_rr",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(isPtr(0), isReg(1)),
             TargetInst::MOV64rr,
             gpr64,
             gpr64);
    ADD_RULE("mov_f32_rr",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, f32), isReg(1)),
             TargetInst::MOVSSrr,
             fpr32,
             fpr32);
    ADD_RULE("mov_f64_rr",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, f64), isReg(1)),
             TargetInst::MOVSDrr,
             fpr64,
             fpr64);

    // Immediate-to-Register
    ADD_RULE("mov_i8_ri",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::MOV8ri,
             gpr8);
    ADD_RULE("mov_i16_ri",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::MOV16ri,
             gpr16);
    ADD_RULE("mov_i32_ri",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::MOV32ri,
             gpr32);
    ADD_RULE("mov_i64_ri32",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::MOV64ri32,
             gpr64);
    ADD_RULE("mov_i64_ri64",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1)),
             TargetInst::MOVABS64ri,
             gpr64);

    builder.begin("mov_f32_imm", MirInstructionOpCode::MOV)
            .pred(ISelPreds::_and(ISelPreds::operandMirType(0, f32), isFloatImm(1)))
            .act(LowerFloatImmediateAction(32, TargetInst::MOV32ri, TargetInst::MOVDtoFPR, gpr32, fpr32))
            .dump();

    // 64-bit Float Immediate: MOVABS64ri -> MOVQtoFPR
    builder.begin("mov_f64_imm", MirInstructionOpCode::MOV)
            .pred(ISelPreds::_and(ISelPreds::operandMirType(0, f64), isFloatImm(1)))
            .act(LowerFloatImmediateAction(64, TargetInst::MOVABS64ri, TargetInst::MOVQtoFPR, gpr64, fpr64))
            .dump();

    // Address Materialization
    ADD_RULE("lea_addr", MirInstructionOpCode::LEA, isMem(1), TargetInst::LEA64rm, gpr64, gpr64);
    ADD_RULE("lea_global_small",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(isSmallModel, isGlobalRef(1)),
             TargetInst::LEA64rm_rip,
             gpr64);
    ADD_RULE("movabs_large",
             MirInstructionOpCode::MOV,
             ISelPreds::_and(isLargeModel, isGlobalRef(1)),
             TargetInst::MOVABS64ri,
             gpr64);

    // Bitcasts
    ADD_RULE("bitcast_f32_i32",
             MirInstructionOpCode::BITCAST,
             ISelPreds::_and(ISelPreds::operandMirType(0, f32), ISelPreds::operandMirType(1, i32)),
             TargetInst::MOVDtoFPR,
             fpr32,
             gpr32);
    ADD_RULE("bitcast_i32_f32",
             MirInstructionOpCode::BITCAST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), ISelPreds::operandMirType(1, f32)),
             TargetInst::MOVDtoGPR,
             gpr32,
             fpr32);
    ADD_RULE("bitcast_f64_i64",
             MirInstructionOpCode::BITCAST,
             ISelPreds::_and(ISelPreds::operandMirType(0, f64), ISelPreds::operandMirType(1, i64)),
             TargetInst::MOVQtoFPR,
             fpr64,
             gpr64);
    ADD_RULE("bitcast_i64_f64",
             MirInstructionOpCode::BITCAST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), ISelPreds::operandMirType(1, f64)),
             TargetInst::MOVQtoGPR,
             gpr64,
             fpr64);
    ADD_RULE("bitcast_ptr_i64",
             MirInstructionOpCode::BITCAST,
             ISelPreds::_and(isPtr(0), ISelPreds::operandMirType(1, i64)),
             TargetInst::MOV64rr,
             gpr64,
             gpr64);
    ADD_RULE("bitcast_i64_ptr",
             MirInstructionOpCode::BITCAST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isPtr(1)),
             TargetInst::MOV64rr,
             gpr64,
             gpr64);

    // =========================================================================
    // 2. MEMORY OPERATIONS (LOAD, STORE, PUSH, POP, ALLOC)
    // =========================================================================
    // Base-Indirect Loads
    ADD_RULE("load_i8",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isMem(1)),
             TargetInst::MOV8rm,
             gpr8,
             gpr64);
    ADD_RULE("load_i16",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isMem(1)),
             TargetInst::MOV16rm,
             gpr16,
             gpr64);
    ADD_RULE("load_i32",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isMem(1)),
             TargetInst::MOV32rm,
             gpr32,
             gpr64);
    ADD_RULE("load_i64",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isMem(1)),
             TargetInst::MOV64rm,
             gpr64,
             gpr64);
    ADD_RULE("load_ptr",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(isPtr(0), isMem(1)),
             TargetInst::MOV64rm,
             gpr64,
             gpr64);
    ADD_RULE("load_f32",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(ISelPreds::operandMirType(0, f32), isMem(1)),
             TargetInst::MOVSSrm,
             fpr32,
             gpr64);
    ADD_RULE("load_f64",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(ISelPreds::operandMirType(0, f64), isMem(1)),
             TargetInst::MOVSDrm,
             fpr64,
             gpr64);

    // RIP-Relative Loads (Small Model)
    ADD_RULE("load_global_i8_rip",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(isSmallModel, ISelPreds::operandMirType(0, i8), isGlobalRef(1)),
             TargetInst::MOV8rm_rip,
             gpr8);
    ADD_RULE("load_global_i16_rip",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(isSmallModel, ISelPreds::operandMirType(0, i16), isGlobalRef(1)),
             TargetInst::MOV16rm_rip,
             gpr16);
    ADD_RULE("load_global_i32_rip",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(isSmallModel, ISelPreds::operandMirType(0, i32), isGlobalRef(1)),
             TargetInst::MOV32rm_rip,
             gpr32);
    ADD_RULE("load_global_i64_rip",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(isSmallModel, ISelPreds::operandMirType(0, i64), isGlobalRef(1)),
             TargetInst::MOV64rm_rip,
             gpr64);
    ADD_RULE("load_global_ptr_rip",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(isSmallModel, isPtr(0), isGlobalRef(1)),
             TargetInst::MOV64rm_rip,
             gpr64);
    ADD_RULE("load_global_f32_rip",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(isSmallModel, ISelPreds::operandMirType(0, f32), isGlobalRef(1)),
             TargetInst::MOVSSrm_rip,
             fpr32);
    ADD_RULE("load_global_f64_rip",
             MirInstructionOpCode::LOAD,
             ISelPreds::_and(isSmallModel, ISelPreds::operandMirType(0, f64), isGlobalRef(1)),
             TargetInst::MOVSDrm_rip,
             fpr64);

    // Base-Indirect Stores (Register Source)
    ADD_RULE("store_i8",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, i8), isReg(1)),
             TargetInst::MOV8mr,
             gpr64,
             gpr8);
    ADD_RULE("store_i16",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, i16), isReg(1)),
             TargetInst::MOV16mr,
             gpr64,
             gpr16);
    ADD_RULE("store_i32",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, i32), isReg(1)),
             TargetInst::MOV32mr,
             gpr64,
             gpr32);
    ADD_RULE("store_i64",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, i64), isReg(1)),
             TargetInst::MOV64mr,
             gpr64,
             gpr64);
    ADD_RULE("store_ptr",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), isPtr(1), isReg(1)),
             TargetInst::MOV64mr,
             gpr64,
             gpr64);
    ADD_RULE("store_f32",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, f32), isReg(1)),
             TargetInst::MOVSSmr,
             gpr64,
             fpr32);
    ADD_RULE("store_f64",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, f64), isReg(1)),
             TargetInst::MOVSDmr,
             gpr64,
             fpr64);

    // Base-Indirect Stores (Immediate Source)
    ADD_RULE("store_i8_imm",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, i8), isIntImm(1)),
             TargetInst::MOV8mi,
             gpr64);
    ADD_RULE("store_i16_imm",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, i16), isIntImm(1)),
             TargetInst::MOV16mi,
             gpr64);
    ADD_RULE("store_i32_imm",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, i32), isIntImm(1)),
             TargetInst::MOV32mi,
             gpr64);
    ADD_RULE("store_i64_imm32",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isMem(0), ISelPreds::operandMirType(1, i64), isIntImm(1), isImmS32(1)),
             TargetInst::MOV64mi32,
             gpr64);

    // RIP-Relative Stores (Small Model)
    ADD_RULE("store_global_i8_rip",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isSmallModel, isGlobalRef(0), ISelPreds::operandMirType(1, i8)),
             TargetInst::MOV8mr_rip,
             gpr8);
    ADD_RULE("store_global_i16_rip",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isSmallModel, isGlobalRef(0), ISelPreds::operandMirType(1, i16)),
             TargetInst::MOV16mr_rip,
             gpr16);
    ADD_RULE("store_global_i32_rip",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isSmallModel, isGlobalRef(0), ISelPreds::operandMirType(1, i32)),
             TargetInst::MOV32mr_rip,
             gpr32);
    ADD_RULE("store_global_i64_rip",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isSmallModel, isGlobalRef(0), ISelPreds::operandMirType(1, i64)),
             TargetInst::MOV64mr_rip,
             gpr64);
    ADD_RULE("store_global_f32_rip",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isSmallModel, isGlobalRef(0), ISelPreds::operandMirType(1, f32)),
             TargetInst::MOVSSmr_rip,
             fpr32);
    ADD_RULE("store_global_f64_rip",
             MirInstructionOpCode::STORE,
             ISelPreds::_and(isSmallModel, isGlobalRef(0), ISelPreds::operandMirType(1, f64)),
             TargetInst::MOVSDmr_rip,
             fpr64);

    // Stack Operations
    ADD_RULE("push_reg", MirInstructionOpCode::PUSH, isReg(0), TargetInst::PUSH64r, gpr64);
    ADD_RULE("push_imm32",
             MirInstructionOpCode::PUSH,
             ISelPreds::_and(isIntImm(0), isImmS32(0)),
             TargetInst::PUSH64i32);
    ADD_RULE("pop_reg", MirInstructionOpCode::POP, isReg(0), TargetInst::POP64r, gpr64);

    // Stack Allocations
    ADD_RULE("alloc_static", MirInstructionOpCode::ALLOC, isPtr(0), TargetInst::ALLOC, gpr64);
    ADD_RULE("alloc_dynamic", MirInstructionOpCode::DALLOC, isPtr(0), TargetInst::DYNAMIC_ALLOC, gpr64, gpr64);

    // =========================================================================
    // 3. INTEGER ALU (ADD, ADC, SUB, SBB, MUL, IMUL, DIV, IDIV, REM, NEG)
    // =========================================================================
    // ADD (Reg-Reg)
    ADD_RULE("add_i8_rr",
             MirInstructionOpCode::ADD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::ADD8rr,
             gpr8,
             gpr8);
    ADD_RULE("add_i16_rr",
             MirInstructionOpCode::ADD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::ADD16rr,
             gpr16,
             gpr16);
    ADD_RULE("add_i32_rr",
             MirInstructionOpCode::ADD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::ADD32rr,
             gpr32,
             gpr32);
    ADD_RULE("add_i64_rr",
             MirInstructionOpCode::ADD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::ADD64rr,
             gpr64,
             gpr64);
    ADD_RULE("add_ptr_rr",
             MirInstructionOpCode::ADD,
             ISelPreds::_and(isPtr(0), isReg(1)),
             TargetInst::ADD64rr,
             gpr64,
             gpr64);

    // ADD (Reg-Imm)
    ADD_RULE("add_i8_ri",
             MirInstructionOpCode::ADD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::ADD8ri,
             gpr8);
    ADD_RULE("add_i16_ri",
             MirInstructionOpCode::ADD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::ADD16ri,
             gpr16);
    ADD_RULE("add_i32_ri",
             MirInstructionOpCode::ADD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::ADD32ri,
             gpr32);
    ADD_RULE("add_i64_ri32",
             MirInstructionOpCode::ADD,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::ADD64ri32,
             gpr64);

    // ADC (Reg-Reg & Reg-Imm)
    ADD_RULE("adc_i8_rr",
             MirInstructionOpCode::ADC,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::ADC8rr,
             gpr8,
             gpr8);
    ADD_RULE("adc_i16_rr",
             MirInstructionOpCode::ADC,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::ADC16rr,
             gpr16,
             gpr16);
    ADD_RULE("adc_i32_rr",
             MirInstructionOpCode::ADC,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::ADC32rr,
             gpr32,
             gpr32);
    ADD_RULE("adc_i64_rr",
             MirInstructionOpCode::ADC,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::ADC64rr,
             gpr64,
             gpr64);
    ADD_RULE("adc_i8_ri",
             MirInstructionOpCode::ADC,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::ADC8ri,
             gpr8);
    ADD_RULE("adc_i16_ri",
             MirInstructionOpCode::ADC,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::ADC16ri,
             gpr16);
    ADD_RULE("adc_i32_ri",
             MirInstructionOpCode::ADC,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::ADC32ri,
             gpr32);
    ADD_RULE("adc_i64_ri32",
             MirInstructionOpCode::ADC,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::ADC64ri32,
             gpr64);

    // SUB (Reg-Reg)
    ADD_RULE("sub_i8_rr",
             MirInstructionOpCode::SUB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::SUB8rr,
             gpr8,
             gpr8);
    ADD_RULE("sub_i16_rr",
             MirInstructionOpCode::SUB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::SUB16rr,
             gpr16,
             gpr16);
    ADD_RULE("sub_i32_rr",
             MirInstructionOpCode::SUB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::SUB32rr,
             gpr32,
             gpr32);
    ADD_RULE("sub_i64_rr",
             MirInstructionOpCode::SUB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::SUB64rr,
             gpr64,
             gpr64);
    ADD_RULE("sub_ptr_rr",
             MirInstructionOpCode::SUB,
             ISelPreds::_and(isPtr(0), isReg(1)),
             TargetInst::SUB64rr,
             gpr64,
             gpr64);

    // SUB (Reg-Imm)
    ADD_RULE("sub_i8_ri",
             MirInstructionOpCode::SUB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::SUB8ri,
             gpr8);
    ADD_RULE("sub_i16_ri",
             MirInstructionOpCode::SUB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::SUB16ri,
             gpr16);
    ADD_RULE("sub_i32_ri",
             MirInstructionOpCode::SUB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::SUB32ri,
             gpr32);
    ADD_RULE("sub_i64_ri32",
             MirInstructionOpCode::SUB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::SUB64ri32,
             gpr64);

    // SBB (Reg-Reg & Reg-Imm)
    ADD_RULE("sbb_i8_rr",
             MirInstructionOpCode::SBB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::SBB8rr,
             gpr8,
             gpr8);
    ADD_RULE("sbb_i16_rr",
             MirInstructionOpCode::SBB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::SBB16rr,
             gpr16,
             gpr16);
    ADD_RULE("sbb_i32_rr",
             MirInstructionOpCode::SBB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::SBB32rr,
             gpr32,
             gpr32);
    ADD_RULE("sbb_i64_rr",
             MirInstructionOpCode::SBB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::SBB64rr,
             gpr64,
             gpr64);
    ADD_RULE("sbb_i8_ri",
             MirInstructionOpCode::SBB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::SBB8ri,
             gpr8);
    ADD_RULE("sbb_i16_ri",
             MirInstructionOpCode::SBB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::SBB16ri,
             gpr16);
    ADD_RULE("sbb_i32_ri",
             MirInstructionOpCode::SBB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::SBB32ri,
             gpr32);
    ADD_RULE("sbb_i64_ri32",
             MirInstructionOpCode::SBB,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::SBB64ri32,
             gpr64);

    // MUL (Unsigned)
    ADD_RULE("mul_i8", MirInstructionOpCode::MUL, ISelPreds::operandMirType(0, i8), TargetInst::MUL8r, gpr8, gpr8);
    ADD_RULE("mul_i16", MirInstructionOpCode::MUL, ISelPreds::operandMirType(0, i16), TargetInst::MUL16r, gpr16, gpr16);
    ADD_RULE("mul_i32", MirInstructionOpCode::MUL, ISelPreds::operandMirType(0, i32), TargetInst::MUL32r, gpr32, gpr32);
    ADD_RULE("mul_i64", MirInstructionOpCode::MUL, ISelPreds::operandMirType(0, i64), TargetInst::MUL64r, gpr64, gpr64);

    // IMUL (Signed: Reg-Reg & Reg-Imm)
    ADD_RULE("imul_i8", MirInstructionOpCode::IMUL, ISelPreds::operandMirType(0, i8), TargetInst::IMUL8r, gpr8, gpr8);
    ADD_RULE("imul_i16_rr",
             MirInstructionOpCode::IMUL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::IMUL16rr,
             gpr16,
             gpr16);
    ADD_RULE("imul_i32_rr",
             MirInstructionOpCode::IMUL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::IMUL32rr,
             gpr32,
             gpr32);
    ADD_RULE("imul_i64_rr",
             MirInstructionOpCode::IMUL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::IMUL64rr,
             gpr64,
             gpr64);
    ADD_RULE("imul_i16_ri",
             MirInstructionOpCode::IMUL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::IMUL16rri,
             gpr16,
             gpr16);
    ADD_RULE("imul_i32_ri",
             MirInstructionOpCode::IMUL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::IMUL32rri,
             gpr32,
             gpr32);
    ADD_RULE("imul_i64_ri32",
             MirInstructionOpCode::IMUL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::IMUL64rri32,
             gpr64,
             gpr64);

    // DIV, IDIV, REM
    ADD_RULE("div_i8", MirInstructionOpCode::DIV, ISelPreds::operandMirType(0, i8), TargetInst::DIV8r, gpr8, gpr8);
    ADD_RULE("div_i16", MirInstructionOpCode::DIV, ISelPreds::operandMirType(0, i16), TargetInst::DIV16r, gpr16, gpr16);
    ADD_RULE("div_i32", MirInstructionOpCode::DIV, ISelPreds::operandMirType(0, i32), TargetInst::DIV32r, gpr32, gpr32);
    ADD_RULE("div_i64", MirInstructionOpCode::DIV, ISelPreds::operandMirType(0, i64), TargetInst::DIV64r, gpr64, gpr64);
    ADD_RULE("idiv_i8", MirInstructionOpCode::IDIV, ISelPreds::operandMirType(0, i8), TargetInst::IDIV8r, gpr8, gpr8);
    ADD_RULE("idiv_i16",
             MirInstructionOpCode::IDIV,
             ISelPreds::operandMirType(0, i16),
             TargetInst::IDIV16r,
             gpr16,
             gpr16);
    ADD_RULE("idiv_i32",
             MirInstructionOpCode::IDIV,
             ISelPreds::operandMirType(0, i32),
             TargetInst::IDIV32r,
             gpr32,
             gpr32);
    ADD_RULE("idiv_i64",
             MirInstructionOpCode::IDIV,
             ISelPreds::operandMirType(0, i64),
             TargetInst::IDIV64r,
             gpr64,
             gpr64);
    ADD_RULE("rem_i8", MirInstructionOpCode::REM, ISelPreds::operandMirType(0, i8), TargetInst::IDIV8r, gpr8, gpr8);
    ADD_RULE("rem_i16",
             MirInstructionOpCode::REM,
             ISelPreds::operandMirType(0, i16),
             TargetInst::IDIV16r,
             gpr16,
             gpr16);
    ADD_RULE("rem_i32",
             MirInstructionOpCode::REM,
             ISelPreds::operandMirType(0, i32),
             TargetInst::IDIV32r,
             gpr32,
             gpr32);
    ADD_RULE("rem_i64",
             MirInstructionOpCode::REM,
             ISelPreds::operandMirType(0, i64),
             TargetInst::IDIV64r,
             gpr64,
             gpr64);

    // NEG
    ADD_RULE("neg_i8", MirInstructionOpCode::NEG, ISelPreds::operandMirType(0, i8), TargetInst::NEG8r, gpr8);
    ADD_RULE("neg_i16", MirInstructionOpCode::NEG, ISelPreds::operandMirType(0, i16), TargetInst::NEG16r, gpr16);
    ADD_RULE("neg_i32", MirInstructionOpCode::NEG, ISelPreds::operandMirType(0, i32), TargetInst::NEG32r, gpr32);
    ADD_RULE("neg_i64", MirInstructionOpCode::NEG, ISelPreds::operandMirType(0, i64), TargetInst::NEG64r, gpr64);

    // =========================================================================
    // 4. FLOATING POINT ARITHMETIC (FADD, FSUB, FMUL, FDIV, FCMP)
    // =========================================================================
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

    // =========================================================================
    // 5. BITWISE OPERATIONS & SHIFTS
    // =========================================================================
    // AND
    ADD_RULE("and_i8_rr",
             MirInstructionOpCode::AND,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::AND8rr,
             gpr8,
             gpr8);
    ADD_RULE("and_i16_rr",
             MirInstructionOpCode::AND,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::AND16rr,
             gpr16,
             gpr16);
    ADD_RULE("and_i32_rr",
             MirInstructionOpCode::AND,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::AND32rr,
             gpr32,
             gpr32);
    ADD_RULE("and_i64_rr",
             MirInstructionOpCode::AND,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::AND64rr,
             gpr64,
             gpr64);
    ADD_RULE("and_i8_ri",
             MirInstructionOpCode::AND,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::AND8ri,
             gpr8);
    ADD_RULE("and_i16_ri",
             MirInstructionOpCode::AND,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::AND16ri,
             gpr16);
    ADD_RULE("and_i32_ri",
             MirInstructionOpCode::AND,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::AND32ri,
             gpr32);
    ADD_RULE("and_i64_ri32",
             MirInstructionOpCode::AND,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::AND64ri32,
             gpr64);

    // OR
    ADD_RULE("or_i8_rr",
             MirInstructionOpCode::OR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::OR8rr,
             gpr8,
             gpr8);
    ADD_RULE("or_i16_rr",
             MirInstructionOpCode::OR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::OR16rr,
             gpr16,
             gpr16);
    ADD_RULE("or_i32_rr",
             MirInstructionOpCode::OR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::OR32rr,
             gpr32,
             gpr32);
    ADD_RULE("or_i64_rr",
             MirInstructionOpCode::OR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::OR64rr,
             gpr64,
             gpr64);
    ADD_RULE("or_i8_ri",
             MirInstructionOpCode::OR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::OR8ri,
             gpr8);
    ADD_RULE("or_i16_ri",
             MirInstructionOpCode::OR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::OR16ri,
             gpr16);
    ADD_RULE("or_i32_ri",
             MirInstructionOpCode::OR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::OR32ri,
             gpr32);
    ADD_RULE("or_i64_ri32",
             MirInstructionOpCode::OR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::OR64ri32,
             gpr64);

    // XOR
    ADD_RULE("xor_i8_rr",
             MirInstructionOpCode::XOR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::XOR8rr,
             gpr8,
             gpr8);
    ADD_RULE("xor_i16_rr",
             MirInstructionOpCode::XOR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::XOR16rr,
             gpr16,
             gpr16);
    ADD_RULE("xor_i32_rr",
             MirInstructionOpCode::XOR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::XOR32rr,
             gpr32,
             gpr32);
    ADD_RULE("xor_i64_rr",
             MirInstructionOpCode::XOR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::XOR64rr,
             gpr64,
             gpr64);
    ADD_RULE("xor_i8_ri",
             MirInstructionOpCode::XOR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::XOR8ri,
             gpr8);
    ADD_RULE("xor_i16_ri",
             MirInstructionOpCode::XOR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::XOR16ri,
             gpr16);
    ADD_RULE("xor_i32_ri",
             MirInstructionOpCode::XOR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::XOR32ri,
             gpr32);
    ADD_RULE("xor_i64_ri32",
             MirInstructionOpCode::XOR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::XOR64ri32,
             gpr64);

    // NOT
    ADD_RULE("not_i8", MirInstructionOpCode::NOT, ISelPreds::operandMirType(0, i8), TargetInst::NOT8r, gpr8);
    ADD_RULE("not_i16", MirInstructionOpCode::NOT, ISelPreds::operandMirType(0, i16), TargetInst::NOT16r, gpr16);
    ADD_RULE("not_i32", MirInstructionOpCode::NOT, ISelPreds::operandMirType(0, i32), TargetInst::NOT32r, gpr32);
    ADD_RULE("not_i64", MirInstructionOpCode::NOT, ISelPreds::operandMirType(0, i64), TargetInst::NOT64r, gpr64);

    // SHL
    ADD_RULE("shl_i8_rr",
             MirInstructionOpCode::SHL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::SHL8rr,
             gpr8,
             gpr8);
    ADD_RULE("shl_i16_rr",
             MirInstructionOpCode::SHL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::SHL16rr,
             gpr16,
             gpr8);
    ADD_RULE("shl_i32_rr",
             MirInstructionOpCode::SHL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::SHL32rr,
             gpr32,
             gpr8);
    ADD_RULE("shl_i64_rr",
             MirInstructionOpCode::SHL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::SHL64rr,
             gpr64,
             gpr8);
    ADD_RULE("shl_i8_ri",
             MirInstructionOpCode::SHL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::SHL8ri,
             gpr8);
    ADD_RULE("shl_i16_ri",
             MirInstructionOpCode::SHL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::SHL16ri,
             gpr16);
    ADD_RULE("shl_i32_ri",
             MirInstructionOpCode::SHL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::SHL32ri,
             gpr32);
    ADD_RULE("shl_i64_ri",
             MirInstructionOpCode::SHL,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1)),
             TargetInst::SHL64ri,
             gpr64);

    // SHR
    ADD_RULE("shr_i8_rr",
             MirInstructionOpCode::SHR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::SHR8rr,
             gpr8,
             gpr8);
    ADD_RULE("shr_i16_rr",
             MirInstructionOpCode::SHR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::SHR16rr,
             gpr16,
             gpr8);
    ADD_RULE("shr_i32_rr",
             MirInstructionOpCode::SHR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::SHR32rr,
             gpr32,
             gpr8);
    ADD_RULE("shr_i64_rr",
             MirInstructionOpCode::SHR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::SHR64rr,
             gpr64,
             gpr8);
    ADD_RULE("shr_i8_ri",
             MirInstructionOpCode::SHR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::SHR8ri,
             gpr8);
    ADD_RULE("shr_i16_ri",
             MirInstructionOpCode::SHR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::SHR16ri,
             gpr16);
    ADD_RULE("shr_i32_ri",
             MirInstructionOpCode::SHR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::SHR32ri,
             gpr32);
    ADD_RULE("shr_i64_ri",
             MirInstructionOpCode::SHR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1)),
             TargetInst::SHR64ri,
             gpr64);

    // SAR
    ADD_RULE("sar_i8_rr",
             MirInstructionOpCode::SAR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::SAR8rr,
             gpr8,
             gpr8);
    ADD_RULE("sar_i16_rr",
             MirInstructionOpCode::SAR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::SAR16rr,
             gpr16,
             gpr8);
    ADD_RULE("sar_i32_rr",
             MirInstructionOpCode::SAR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::SAR32rr,
             gpr32,
             gpr8);
    ADD_RULE("sar_i64_rr",
             MirInstructionOpCode::SAR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::SAR64rr,
             gpr64,
             gpr8);
    ADD_RULE("sar_i8_ri",
             MirInstructionOpCode::SAR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::SAR8ri,
             gpr8);
    ADD_RULE("sar_i16_ri",
             MirInstructionOpCode::SAR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::SAR16ri,
             gpr16);
    ADD_RULE("sar_i32_ri",
             MirInstructionOpCode::SAR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::SAR32ri,
             gpr32);
    ADD_RULE("sar_i64_ri",
             MirInstructionOpCode::SAR,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1)),
             TargetInst::SAR64ri,
             gpr64);

    // =========================================================================
    // 6. COMPARISONS & CONTROL FLOW
    // =========================================================================
    // CMP (Reg-Reg)
    ADD_RULE("cmp_i8_rr",
             MirInstructionOpCode::CMP,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::CMP8rr,
             gpr8,
             gpr8);
    ADD_RULE("cmp_i16_rr",
             MirInstructionOpCode::CMP,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::CMP16rr,
             gpr16,
             gpr16);
    ADD_RULE("cmp_i32_rr",
             MirInstructionOpCode::CMP,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::CMP32rr,
             gpr32,
             gpr32);
    ADD_RULE("cmp_i64_rr",
             MirInstructionOpCode::CMP,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::CMP64rr,
             gpr64,
             gpr64);
    ADD_RULE("cmp_ptr_rr",
             MirInstructionOpCode::CMP,
             ISelPreds::_and(isPtr(0), isReg(1)),
             TargetInst::CMP64rr,
             gpr64,
             gpr64);

    // CMP (Reg-Imm)
    ADD_RULE("cmp_i8_ri",
             MirInstructionOpCode::CMP,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::CMP8ri,
             gpr8);
    ADD_RULE("cmp_i16_ri",
             MirInstructionOpCode::CMP,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::CMP16ri,
             gpr16);
    ADD_RULE("cmp_i32_ri",
             MirInstructionOpCode::CMP,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::CMP32ri,
             gpr32);
    ADD_RULE("cmp_i64_ri32",
             MirInstructionOpCode::CMP,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::CMP64ri32,
             gpr64);

    // TEST (Reg-Reg & Reg-Imm)
    ADD_RULE("test_i8_rr",
             MirInstructionOpCode::TEST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isReg(1)),
             TargetInst::TEST8rr,
             gpr8,
             gpr8);
    ADD_RULE("test_i16_rr",
             MirInstructionOpCode::TEST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isReg(1)),
             TargetInst::TEST16rr,
             gpr16,
             gpr16);
    ADD_RULE("test_i32_rr",
             MirInstructionOpCode::TEST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isReg(1)),
             TargetInst::TEST32rr,
             gpr32,
             gpr32);
    ADD_RULE("test_i64_rr",
             MirInstructionOpCode::TEST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isReg(1)),
             TargetInst::TEST64rr,
             gpr64,
             gpr64);
    ADD_RULE("test_i8_ri",
             MirInstructionOpCode::TEST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i8), isIntImm(1)),
             TargetInst::TEST8ri,
             gpr8);
    ADD_RULE("test_i16_ri",
             MirInstructionOpCode::TEST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), isIntImm(1)),
             TargetInst::TEST16ri,
             gpr16);
    ADD_RULE("test_i32_ri",
             MirInstructionOpCode::TEST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), isIntImm(1)),
             TargetInst::TEST32ri,
             gpr32);
    ADD_RULE("test_i64_ri32",
             MirInstructionOpCode::TEST,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), isIntImm(1), isImmS32(1)),
             TargetInst::TEST64ri32,
             gpr64);

    // Branches
    ADD_RULE_NOREG("jmp_direct", MirInstructionOpCode::JMP, isSmallModel, TargetInst::JMP);
    ADD_RULE("jmp_indirect", MirInstructionOpCode::JMP, isReg(0), TargetInst::JMP64r, gpr64);
    ADD_RULE_NOREG("je", MirInstructionOpCode::JE, catchAll, TargetInst::JE);
    ADD_RULE_NOREG("jne", MirInstructionOpCode::JNE, catchAll, TargetInst::JNE);
    ADD_RULE_NOREG("jg", MirInstructionOpCode::JG, catchAll, TargetInst::JG);
    ADD_RULE_NOREG("jge", MirInstructionOpCode::JGE, catchAll, TargetInst::JGE);
    ADD_RULE_NOREG("jl", MirInstructionOpCode::JL, catchAll, TargetInst::JL);
    ADD_RULE_NOREG("jle", MirInstructionOpCode::JLE, catchAll, TargetInst::JLE);
    ADD_RULE_NOREG("ja", MirInstructionOpCode::JA, catchAll, TargetInst::JA);
    ADD_RULE_NOREG("jb", MirInstructionOpCode::JB, catchAll, TargetInst::JB);

    // Subroutine Calls
    ADD_RULE_NOREG("call_direct",
                   MirInstructionOpCode::CALL,
                   ISelPreds::_and(isSmallModel, ISelPreds::_or(isGlobalRef(0), isGlobalRef(1))),
                   TargetInst::CALL);
    ADD_RULE("call_indirect",
             MirInstructionOpCode::CALL,
             ISelPreds::_or(isReg(0), isReg(1)),
             TargetInst::CALL64r,
             gpr64);

    ADD_RULE_NOREG("ret", MirInstructionOpCode::RET, catchAll, TargetInst::RET);

    // =========================================================================
    // 7. CASTS & CONVERSIONS
    // =========================================================================
    // Zero Extensions
    ADD_RULE("zext_i16_i8",
             MirInstructionOpCode::ZEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), ISelPreds::operandMirType(1, i8)),
             TargetInst::MOVZX16rr8,
             gpr16,
             gpr8);
    ADD_RULE("zext_i32_i8",
             MirInstructionOpCode::ZEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), ISelPreds::operandMirType(1, i8)),
             TargetInst::MOVZX32rr8,
             gpr32,
             gpr8);
    ADD_RULE("zext_i32_i16",
             MirInstructionOpCode::ZEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), ISelPreds::operandMirType(1, i16)),
             TargetInst::MOVZX32rr16,
             gpr32,
             gpr16);
    ADD_RULE("zext_i64_i8",
             MirInstructionOpCode::ZEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), ISelPreds::operandMirType(1, i8)),
             TargetInst::MOVZX64rr8,
             gpr64,
             gpr8);
    ADD_RULE("zext_i64_i16",
             MirInstructionOpCode::ZEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), ISelPreds::operandMirType(1, i16)),
             TargetInst::MOVZX64rr16,
             gpr64,
             gpr16);
    ADD_RULE("zext_i64_i32",
             MirInstructionOpCode::ZEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), ISelPreds::operandMirType(1, i32)),
             TargetInst::MOVZX64rr32,
             gpr64,
             gpr32);

    // Sign Extensions
    ADD_RULE("sext_i16_i8",
             MirInstructionOpCode::SEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i16), ISelPreds::operandMirType(1, i8)),
             TargetInst::MOVSX16rr8,
             gpr16,
             gpr8);
    ADD_RULE("sext_i32_i8",
             MirInstructionOpCode::SEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), ISelPreds::operandMirType(1, i8)),
             TargetInst::MOVSX32rr8,
             gpr32,
             gpr8);
    ADD_RULE("sext_i32_i16",
             MirInstructionOpCode::SEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), ISelPreds::operandMirType(1, i16)),
             TargetInst::MOVSX32rr16,
             gpr32,
             gpr16);
    ADD_RULE("sext_i64_i8",
             MirInstructionOpCode::SEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), ISelPreds::operandMirType(1, i8)),
             TargetInst::MOVSX64rr8,
             gpr64,
             gpr8);
    ADD_RULE("sext_i64_i16",
             MirInstructionOpCode::SEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), ISelPreds::operandMirType(1, i16)),
             TargetInst::MOVSX64rr16,
             gpr64,
             gpr16);
    ADD_RULE("sext_i64_i32",
             MirInstructionOpCode::SEXT,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), ISelPreds::operandMirType(1, i32)),
             TargetInst::MOVSX64rr32,
             gpr64,
             gpr32);

    // Truncations
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

    // Floating Point Extensions & Truncations
    ADD_RULE("fpext_f64_f32",
             MirInstructionOpCode::FPEXT,
             ISelPreds::operandMirType(0, f64),
             TargetInst::CVTSS2SDrr,
             fpr64,
             fpr32);
    ADD_RULE("fptrunc_f32_f64",
             MirInstructionOpCode::FPTRUNC,
             ISelPreds::operandMirType(0, f32),
             TargetInst::CVTSD2SSrr,
             fpr32,
             fpr64);

    // Integer to Float Conversions
    ADD_RULE("sitofp_f32_i32",
             MirInstructionOpCode::SITOFP,
             ISelPreds::_and(ISelPreds::operandMirType(0, f32), ISelPreds::operandMirType(1, i32)),
             TargetInst::CVTSI2SSrr32,
             fpr32,
             gpr32);
    ADD_RULE("sitofp_f32_i64",
             MirInstructionOpCode::SITOFP,
             ISelPreds::_and(ISelPreds::operandMirType(0, f32), ISelPreds::operandMirType(1, i64)),
             TargetInst::CVTSI2SSrr64,
             fpr32,
             gpr64);
    ADD_RULE("sitofp_f64_i32",
             MirInstructionOpCode::SITOFP,
             ISelPreds::_and(ISelPreds::operandMirType(0, f64), ISelPreds::operandMirType(1, i32)),
             TargetInst::CVTSI2SDrr32,
             fpr64,
             gpr32);
    ADD_RULE("sitofp_f64_i64",
             MirInstructionOpCode::SITOFP,
             ISelPreds::_and(ISelPreds::operandMirType(0, f64), ISelPreds::operandMirType(1, i64)),
             TargetInst::CVTSI2SDrr64,
             fpr64,
             gpr64);

    // Float to Integer Conversions
    ADD_RULE("fptosi_i32_f32",
             MirInstructionOpCode::FPTOSI,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), ISelPreds::operandMirType(1, f32)),
             TargetInst::CVTTSS2SIrr32,
             gpr32,
             fpr32);
    ADD_RULE("fptosi_i64_f32",
             MirInstructionOpCode::FPTOSI,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), ISelPreds::operandMirType(1, f32)),
             TargetInst::CVTTSS2SIrr64,
             gpr64,
             fpr32);
    ADD_RULE("fptosi_i32_f64",
             MirInstructionOpCode::FPTOSI,
             ISelPreds::_and(ISelPreds::operandMirType(0, i32), ISelPreds::operandMirType(1, f64)),
             TargetInst::CVTTSD2SIrr32,
             gpr32,
             fpr64);
    ADD_RULE("fptosi_i64_f64",
             MirInstructionOpCode::FPTOSI,
             ISelPreds::_and(ISelPreds::operandMirType(0, i64), ISelPreds::operandMirType(1, f64)),
             TargetInst::CVTTSD2SIrr64,
             gpr64,
             fpr64);

    // =========================================================================
    // 8. SYSTEM INSTRUCTIONS & INTERNAL ABI PASSES
    // =========================================================================
    ADD_RULE_NOREG("nop", MirInstructionOpCode::NOP, catchAll, TargetInst::NOP);
    ADD_RULE_NOREG("halt", MirInstructionOpCode::HALT, catchAll, TargetInst::HLT);
    ADD_RULE_NOREG("syscall", MirInstructionOpCode::SYSCALL, catchAll, TargetInst::SYSCALL);
#undef ADD_RULE_NOREG
#undef ADD_RULE
}
} // namespace EzTestTriple