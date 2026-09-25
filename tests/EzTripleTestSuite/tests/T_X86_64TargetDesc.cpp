#include "EzTripleTestSuite.h"
#include "X86_64TargetDesc.h"
#include "X86_64ElfBinaryDesc.h"
#include "X86_64CoffBinaryDesc.h"
#include "Descriptors/TargetRelocationResolver.h"
#include "GenericCodeEmitter.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "Operand/MirOperandBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Function/CallingConvDesc.h"
#include "CodeSection.h"
#include "Legalizer/LegalityQuery.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "x86_64TargetInstructionTable.h"

using namespace EzTargets::X86_64;

// Verifies basic descriptor metadata, displacement type, RIP register id, and GPR class.
TEST_F(EzTripleTestSuite, TestX86_64TargetDescInitialization)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    EXPECT_STREQ(target.getName(), "x86_64");
    EXPECT_EQ(target.getStackSlotSize(), 8u);

    // Displacement type is i64
    EXPECT_EQ(target.getMemOperandDisplacementType(), getBuilderCtx()->getTypeTable()->i64());

    // RIP instruction pointer register
    MirRegisterRef rip = target.getInstructionPtrReg();
    EXPECT_EQ(rip.getId(), 16u);

    // GPR class
    MirRegisterClass *gpr = target.getGprClass();
    ASSERT_NE(gpr, nullptr);
    EXPECT_STREQ(gpr->getName(), "GPR64");
}

// Verifies the GPR and FPR banks expose their classes, register counts, and aliases.
TEST_F(EzTripleTestSuite, TestX86_64RegisterBanksAndClasses)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    auto banks = target.getAvailableRegisterBanks();
    ASSERT_EQ(banks.size(), 2u);

    // GPR Bank
    MirRegisterBank *gprBank = banks[0];
    ASSERT_NE(gprBank, nullptr);
    EXPECT_STREQ(gprBank->getName(), "GPR");

    MirRegisterClass *gpr64 = gprBank->getClass("GPR64");
    MirRegisterClass *gpr32 = gprBank->getClass("GPR32");
    MirRegisterClass *gpr16 = gprBank->getClass("GPR16");
    MirRegisterClass *gpr8 = gprBank->getClass("GPR8");

    ASSERT_NE(gpr64, nullptr);
    ASSERT_NE(gpr32, nullptr);
    ASSERT_NE(gpr16, nullptr);
    ASSERT_NE(gpr8, nullptr);

    // Check register count in GPR bank (16 registers)
    EXPECT_EQ(gpr64->getRegs().size(), 16u);
    EXPECT_EQ(gpr32->getRegs().size(), 16u);
    EXPECT_EQ(gpr16->getRegs().size(), 16u);
    EXPECT_EQ(gpr8->getRegs().size(), 16u);

    // Check register lookups
    EXPECT_NE(gpr64->getReg("rax"), nullptr);
    EXPECT_NE(gpr32->getReg("eax"), nullptr);
    EXPECT_NE(gpr16->getReg("ax"), nullptr);
    EXPECT_NE(gpr8->getReg("al"), nullptr);

    EXPECT_NE(gpr64->getReg("rsp"), nullptr);
    EXPECT_NE(gpr64->getReg("rbp"), nullptr);

    // FPR Bank
    MirRegisterBank *fprBank = banks[1];
    ASSERT_NE(fprBank, nullptr);
    EXPECT_STREQ(fprBank->getName(), "FPR");

    MirRegisterClass *fpr64 = fprBank->getClass("FPR64");
    MirRegisterClass *fpr32 = fprBank->getClass("FPR32");
    MirRegisterClass *vr128 = fprBank->getClass("VR128");
    ASSERT_NE(fpr64, nullptr);
    ASSERT_NE(fpr32, nullptr);
    ASSERT_NE(vr128, nullptr);
    EXPECT_EQ(target.getVr128Class(), vr128);

    EXPECT_EQ(fpr64->getRegs().size(), 16u);
    EXPECT_EQ(fpr32->getRegs().size(), 16u);
    EXPECT_EQ(vr128->getRegs().size(), 16u);
    EXPECT_NE(fpr64->getReg("xmm0"), nullptr);
    EXPECT_NE(fpr64->getReg("xmm15"), nullptr);
    EXPECT_NE(vr128->getReg("xmm0"), nullptr);
    EXPECT_NE(vr128->getReg("xmm15"), nullptr);
    EXPECT_EQ(vr128->getReg("xmm0")->m_bitSize, 128u);
}

// Verifies the SysV and Win64 conventions with alignment, shadow space, and stack direction.
TEST_F(EzTripleTestSuite, TestX86_64CallingConventions)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    auto convs = target.getAvailableCallingConventions();
    ASSERT_EQ(convs.size(), 2u);

    CallingConvDesc *sysV = target.getSysVCallingConv();
    ASSERT_NE(sysV, nullptr);
    EXPECT_STREQ(sysV->getName(), "SysV_AMD64");
    EXPECT_EQ(sysV->getStackAlignment(), 16u);
    EXPECT_EQ(sysV->getShadowSpaceSize(), 0u);
    EXPECT_TRUE(sysV->doesStackGrowsDownwards());

    CallingConvDesc *win64 = target.getWin64CallingConv();
    ASSERT_NE(win64, nullptr);
    EXPECT_STREQ(win64->getName(), "Win64");
    EXPECT_EQ(win64->getStackAlignment(), 16u);
    EXPECT_EQ(win64->getShadowSpaceSize(), 32u);
    EXPECT_TRUE(win64->doesStackGrowsDownwards());
}

// Verifies all target subsystems, target instruction descriptors, and libcall mappings.
TEST_F(EzTripleTestSuite, TestX86_64Subsystems)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    EXPECT_NE(target.getLegalizer(), nullptr);
    EXPECT_NE(target.getLegalizerInfo(), nullptr);
    EXPECT_NE(target.getInstructionSelector(), nullptr);
    // Addressing-mode folding is not wired into production selection, so the target exposes none.
    EXPECT_EQ(target.getAddressingModeMatcher(), nullptr);
    EXPECT_NE(target.getRegisterAllocator(), nullptr);
    EXPECT_NE(target.getFrameLowerer(), nullptr);

    // Target instruction retrieval
    auto *descADD64rr = x86_64TargetInst::getTargetDesc(x86_64TargetInst::ADD64rr);
    ASSERT_NE(descADD64rr, nullptr);
    EXPECT_STREQ(descADD64rr->getName(), "ADD64rr");

    auto *descMOV64rr = x86_64TargetInst::getTargetDesc(x86_64TargetInst::MOV64rr);
    ASSERT_NE(descMOV64rr, nullptr);
    EXPECT_STREQ(descMOV64rr->getName(), "MOV64rr");

    // Libcall resolution mirrors the generated legality table's symbol pool (i128 divide).
    EXPECT_EQ(target.getLibcallStr(0), "__divti3");
    EXPECT_EQ(target.getLibcallStr(1), "__udivti3");
    EXPECT_TRUE(target.getLibcallStr(2).empty());
}

// Verifies the ELF and COFF binary descriptors and their standard sections.
TEST_F(EzTripleTestSuite, TestX86_64BinaryDescriptors)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    auto binaries = target.getAvailableBinaryDescriptors();
    ASSERT_EQ(binaries.size(), 2u);

    TargetBinaryDesc *elf = target.getElfBinaryDesc();
    ASSERT_NE(elf, nullptr);
    EXPECT_STREQ(elf->getName(), "x86_64-elf");
    EXPECT_TRUE(elf->isLittleEndian());
    EXPECT_NE(elf->getSection(SectionType::Text), nullptr);
    EXPECT_NE(elf->getSection(SectionType::Data), nullptr);
    EXPECT_NE(elf->getSection(SectionType::ReadOnly), nullptr);
    EXPECT_NE(elf->getSection(SectionType::NonInitialized), nullptr);

    TargetBinaryDesc *coff = target.getCoffBinaryDesc();
    ASSERT_NE(coff, nullptr);
    EXPECT_STREQ(coff->getName(), "x86_64-coff");
    EXPECT_TRUE(coff->isLittleEndian());
    EXPECT_NE(coff->getSection(SectionType::Text), nullptr);
    EXPECT_NE(coff->getSection(SectionType::Data), nullptr);
    EXPECT_NE(coff->getSection(SectionType::ReadOnly), nullptr);
    EXPECT_NE(coff->getSection(SectionType::NonInitialized), nullptr);
}

// Verifies the emitter factory, register bank factory, and stable relocation resolver.
TEST_F(EzTripleTestSuite, TestX86_64EmitterBankFactoryAndResolverSurface)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    // createCodeEmitter returns a fresh target emitter owned by the caller.
    std::unique_ptr<GenericCodeEmitter> emitter = target.createCodeEmitter();
    ASSERT_NE(emitter, nullptr);

    // createRegisterBank registers a new bank accessible through the descriptor.
    const size_t initialBankCount = target.getAvailableRegisterBanks().size();
    MirRegisterBank *bank = target.createRegisterBank("TMP");
    ASSERT_NE(bank, nullptr);
    EXPECT_STREQ(bank->getName(), "TMP");
    EXPECT_EQ(target.getAvailableRegisterBanks().size(), initialBankCount + 1);

    // The relocation resolver is stable and non-null.
    TargetRelocationResolver *resolver = target.getRelocationResolver();
    ASSERT_NE(resolver, nullptr);
    EXPECT_EQ(target.getRelocationResolver(), resolver);
}

// WEI-08: selectPHI maps PHI incoming operands to predecessors using the CFG's explicit
// predecessor metadata (ascending MirId order) and skips undefined incoming values based on SSA
// def metadata rather than a register-name heuristic.
TEST_F(EzTripleTestSuite, TestSelectPhiUsesPredecessorMetadata)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *tt = ctx->getTypeTable();
    X86_64TargetDesc target(ctx);
    target.initialize();

    MirFunction *func = createTestFunction("phi_select", tt->i64());
    MirBlock *entry = func->getEntryPoint();
    MirBlock *left = createBlock(func, "left");
    MirBlock *right = createBlock(func, "right");
    MirBlock *join = createBlock(func, "join");
    ASSERT_LT(left->getId(), right->getId());

    MirInstructionBuilder entryIb(ctx, entry, InsertionType::Append);
    MirInstructionBuilder leftIb(ctx, left, InsertionType::Append);
    MirInstructionBuilder rightIb(ctx, right, InsertionType::Append);
    MirInstructionBuilder joinIb(ctx, join, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *cond = ob.buildVReg(tt->i1(), "cond");
    entryIb.BR_COND(cond, ob.buildRef(left), ob.buildRef(right));

    MirRegister *lv = ob.buildVReg(tt->i64(), "lv");
    MirRegister *rv = ob.buildVReg(tt->i64(), "rv");
    // Define both incoming values so neither is treated as an undefined phantom.
    leftIb.MOV(lv, ob.buildInt(tt->i64(), FlexInt(1)));
    rightIb.MOV(rv, ob.buildInt(tt->i64(), FlexInt(2)));
    leftIb.JMP(ob.buildRef(join));
    rightIb.JMP(ob.buildRef(join));

    MirRegister *dst = ob.buildVReg(tt->i64(), "phi");
    MirInstruction *phi = joinIb.PHI(dst);
    joinIb.addOperand(phi, lv);
    joinIb.addOperand(phi, rv);
    joinIb.RET(dst);

    // Populate the explicit predecessor metadata exactly as the pipeline does.
    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    ASSERT_NE(passManager.getAnalysis<CodeFlowAnalysisPass>(ctx), nullptr);

    ASSERT_EQ(join->getPredecessors().size(), 2u);
    EXPECT_EQ(join->getPredecessors()[0], left);
    EXPECT_EQ(join->getPredecessors()[1], right);

    MirInstructionSelector *isel = target.getInstructionSelector();
    ASSERT_NE(isel, nullptr);
    ASSERT_TRUE(isel->select(ctx, phi));

    // The PHI is erased and a copy of the destination is emitted on each incoming edge.
    EXPECT_TRUE(phi->isErased());

    auto hasCopyOfDst = [&](MirBlock *block)
    {
        for (MirInstruction *inst : block->getInstructions())
        {
            if (inst->getOpCode() == MirInstructionOpCode::MOV && inst->getOperandCount() >= 2 &&
                inst->getOpAs<MirRegister>(0) == dst)
            {
                return true;
            }
        }
        return false;
    };
    EXPECT_TRUE(hasCopyOfDst(left));
    EXPECT_TRUE(hasCopyOfDst(right));
    ASSERT_NE(dst->getRegClass(), nullptr);
}

// Verifies vector ALU, horizontal, and MOV instructions select to SSE/SSE2/SSE3/SSSE3/SSE4.1 opcodes with VR128.
TEST_F(EzTripleTestSuite, TestX86_64VectorInstructionSelection)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    auto *ctx = getBuilderCtx();
    auto *tt = ctx->getTypeTable();
    auto *func = createTestFunction("test_vector_isel", tt->_void());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    // 1. v4f32 VADD with SSE (enabled by default) -> ADDPSrr
    auto *v4f32Type = tt->v4f32();
    auto *dst1 = ob.buildVReg(v4f32Type, "dst1");
    auto *src1a = ob.buildVReg(v4f32Type, "src1a");
    auto *src1b = ob.buildVReg(v4f32Type, "src1b");
    auto *inst1 = ib.VADD(dst1, src1a, src1b);

    MirInstructionSelector *isel = target.getInstructionSelector();
    ASSERT_NE(isel, nullptr);
    ASSERT_TRUE(isel->select(ctx, inst1));

    EXPECT_TRUE(inst1->isErased());
    auto &instrs = entry->getInstructions();
    ASSERT_FALSE(instrs.empty());
    MirInstruction *selected1 = instrs.back();
    ASSERT_NE(selected1->getTargetDesc(), nullptr);
    EXPECT_EQ(selected1->getTargetDesc()->getId(), static_cast<size_t>(x86_64TargetInst::ADDPSrr));
    EXPECT_STREQ(selected1->getTargetDesc()->getName(), "ADDPSrr");
    EXPECT_EQ(dst1->getRegClass(), target.getVr128Class());
    EXPECT_EQ(src1a->getRegClass(), target.getVr128Class());
    EXPECT_EQ(src1b->getRegClass(), target.getVr128Class());

    // 2. v2f64 VADD with SSE2 (enabled by default) -> ADDPDrr
    auto *v2f64Type = tt->v2f64();
    auto *dst2 = ob.buildVReg(v2f64Type, "dst2");
    auto *src2a = ob.buildVReg(v2f64Type, "src2a");
    auto *src2b = ob.buildVReg(v2f64Type, "src2b");
    auto *inst2 = ib.VADD(dst2, src2a, src2b);

    ASSERT_TRUE(isel->select(ctx, inst2));
    EXPECT_TRUE(inst2->isErased());
    MirInstruction *selected2 = instrs.back();
    ASSERT_NE(selected2->getTargetDesc(), nullptr);
    EXPECT_EQ(selected2->getTargetDesc()->getId(), static_cast<size_t>(x86_64TargetInst::ADDPDrr));

    // 3. v4i32 VADD with SSE2 (enabled by default) -> PADDDrr
    auto *v4i32Type = tt->v4i32();
    auto *dst3 = ob.buildVReg(v4i32Type, "dst3");
    auto *src3a = ob.buildVReg(v4i32Type, "src3a");
    auto *src3b = ob.buildVReg(v4i32Type, "src3b");
    auto *inst3 = ib.VADD(dst3, src3a, src3b);

    ASSERT_TRUE(isel->select(ctx, inst3));
    EXPECT_TRUE(inst3->isErased());
    MirInstruction *selected3 = instrs.back();
    ASSERT_NE(selected3->getTargetDesc(), nullptr);
    EXPECT_EQ(selected3->getTargetDesc()->getId(), static_cast<size_t>(x86_64TargetInst::PADDDrr));

    // 4. v4f32 VHADD with SSE3 (requires sse3) -> HADDPSrr
    EXPECT_FALSE(target.hasExtension("sse3"));
    auto *inst4 = ib.VHADD(dst1, src1a, src1b);
    EXPECT_FALSE(isel->select(ctx, inst4)); // rejected without sse3

    EXPECT_TRUE(target.getExtensionSet().enable("sse3"));
    EXPECT_TRUE(target.hasExtension("sse3"));
    ASSERT_TRUE(isel->select(ctx, inst4));
    EXPECT_TRUE(inst4->isErased());
    MirInstruction *selected4 = instrs.back();
    ASSERT_NE(selected4->getTargetDesc(), nullptr);
    EXPECT_EQ(selected4->getTargetDesc()->getId(), static_cast<size_t>(x86_64TargetInst::HADDPSrr));

    // 5. v4i32 VHADD with SSSE3 (requires ssse3) -> PHADDDrr
    EXPECT_FALSE(target.hasExtension("ssse3"));
    auto *inst5 = ib.VHADD(dst3, src3a, src3b);
    EXPECT_FALSE(isel->select(ctx, inst5)); // rejected without ssse3

    EXPECT_TRUE(target.getExtensionSet().enable("ssse3"));
    EXPECT_TRUE(target.hasExtension("ssse3"));
    ASSERT_TRUE(isel->select(ctx, inst5));
    EXPECT_TRUE(inst5->isErased());
    MirInstruction *selected5 = instrs.back();
    ASSERT_NE(selected5->getTargetDesc(), nullptr);
    EXPECT_EQ(selected5->getTargetDesc()->getId(), static_cast<size_t>(x86_64TargetInst::PHADDDrr));

    // 6. v4i32 VMUL with SSE4.1 (requires sse4_1) -> PMULLDrr
    EXPECT_FALSE(target.hasExtension("sse4_1"));
    auto *inst6 = ib.VMUL(dst3, src3a, src3b);
    EXPECT_FALSE(isel->select(ctx, inst6)); // rejected without sse4_1

    EXPECT_TRUE(target.getExtensionSet().enable("sse4_1"));
    EXPECT_TRUE(target.hasExtension("sse4_1"));
    ASSERT_TRUE(isel->select(ctx, inst6));
    EXPECT_TRUE(inst6->isErased());
    MirInstruction *selected6 = instrs.back();
    ASSERT_NE(selected6->getTargetDesc(), nullptr);
    EXPECT_EQ(selected6->getTargetDesc()->getId(), static_cast<size_t>(x86_64TargetInst::PMULLDrr));

    // 7. Vector MOV on v4f32 -> MOVAPSrr
    auto *dstMove = ob.buildVReg(v4f32Type, "dstMove");
    auto *inst7 = ib.MOV(dstMove, src1a);
    ASSERT_TRUE(isel->select(ctx, inst7));
    EXPECT_TRUE(inst7->isErased());
    MirInstruction *selected7 = instrs.back();
    ASSERT_NE(selected7->getTargetDesc(), nullptr);
    EXPECT_EQ(selected7->getTargetDesc()->getId(), static_cast<size_t>(x86_64TargetInst::MOVAPSrr));
    EXPECT_EQ(dstMove->getRegClass(), target.getVr128Class());
}

// Verifies that disabling an extension prevents selection of instructions guarded by that extension.
TEST_F(EzTripleTestSuite, TestX86_64VectorInstructionSelectionExtensionGuards)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    auto *ctx = getBuilderCtx();
    auto *tt = ctx->getTypeTable();
    auto *func = createTestFunction("test_vector_guards", tt->_void());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    // Disable sse4_1
    EXPECT_TRUE(target.getExtensionSet().disable("sse4_1"));
    EXPECT_FALSE(target.hasExtension("sse4_1"));

    auto *v4i32Type = tt->v4i32();
    auto *dst = ob.buildVReg(v4i32Type, "dst");
    auto *src1 = ob.buildVReg(v4i32Type, "src1");
    auto *src2 = ob.buildVReg(v4i32Type, "src2");

    // VMUL on v4i32 requires sse4_1, should fail selection
    auto *inst = ib.VMUL(dst, src1, src2);
    MirInstructionSelector *isel = target.getInstructionSelector();
    ASSERT_NE(isel, nullptr);
    EXPECT_FALSE(isel->select(ctx, inst));
}

