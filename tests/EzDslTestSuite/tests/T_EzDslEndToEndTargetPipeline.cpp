#include "EzDslTestSuite.h"

// DSL AST & Parsers
#include "Ast/CallingConvDefLangAst.h"
#include "Ast/InstructionDefLangAst.h"
#include "Ast/InstructionSelDefLangAst.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Ast/TargetDefLangAst.h"
#include "Ast/TypeDefLangAst.h"

#include "Parser/CallingConvDefLang.h"
#include "Parser/InstructionDefLang.h"
#include "Parser/InstructionSelDefLang.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Parser/TypeDefLang.h"

// Sema Passes
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/CallingConvPass.h"
#include "SemaPasses/InstSelPass.h"
#include "SemaPasses/LegalizeActionPass.h"
#include "SemaPasses/LegalizeRulePass.h"
#include "SemaPasses/RegisterBankPass.h"
#include "SemaPasses/TargetDefPass.h"
#include "SemaPasses/TargetInstPass.h"
#include "SemaPasses/TypePass.h"

// CodeGenerators
#include "CodeGenerators/CppCallingConvGenerator.h"
#include "CodeGenerators/CppISelTableGenerator.h"
#include "CodeGenerators/CppLegalizerGenerator.h"
#include "CodeGenerators/CppLegalizerRuleGenerator.h"
#include "CodeGenerators/CppTargetBankGenerator.h"
#include "CodeGenerators/CppTargetDescGenerator.h"
#include "CodeGenerators/CppTargetInstGenerator.h"
#include "CodeGenerators/CppTargetTypeLayoutGenerator.h"

// EzTriple & EzMir Backend Pipeline
#include "AbiLowerer/MirAbiLowererPass.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "FrameLowerer/MirFrameLowerer.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "Function/CallingConvDesc.h"
#include "Function/CallLoweringState.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionStackFrame.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "InstructionSelector/MirInstructionSelectorPass.h"
#include "Legalizer/Actions/LegalizeActionCommon.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Legalizer/Actions/LegalizeWidenScalarAction.h"
#include "Legalizer/MirLegalizer.h"
#include "Legalizer/MirLegalizerPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"
#include "Type/IMirTargetTypeLayout.h"
#include "Type/MirTypeTable.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

namespace
{

class ExampleTargetCallingConv : public CallingConvDesc
{
  public:
    explicit ExampleTargetCallingConv(MirRegisterClass *gprClass, std::pmr::memory_resource *alloc) :
        m_calleeSaved(alloc),
        m_callerSaved(alloc)
    {
        m_rax = MirRegisterRef(gprClass, 0);
        m_rdi = MirRegisterRef(gprClass, 1);
        m_rsi = MirRegisterRef(gprClass, 2);
        m_rbp = MirRegisterRef(gprClass, 3);
        m_rsp = MirRegisterRef(gprClass, 4);

        m_callerSaved.push_back(m_rdi);
        m_callerSaved.push_back(m_rsi);
        m_callerSaved.push_back(m_rax);

        m_calleeSaved.push_back(m_rbp);
        m_calleeSaved.push_back(m_rsp);
    }

    ArgumentLocationDesc getArgLoc(MirType *type, CallLoweringState *callState) override
    {
        size_t sizeInBytes = (type ? type->getTotalSizeInBits() + 7 : 64) / 8;
        MirRegisterRef reg;
        if (callState && callState->allocate(m_rdi.getClass(), reg))
        {
            return ArgumentLocationDesc::Reg(reg, sizeInBytes);
        }
        return ArgumentLocationDesc::Stack(sizeInBytes, callState ? callState->allocateStack(type) : nullptr);
    }

    ArgumentLocationDesc getReturnLoc(MirType *type, CallLoweringState *callState) override
    {
        (void)callState;
        size_t sizeInBytes = (type ? type->getTotalSizeInBits() + 7 : 64) / 8;
        return ArgumentLocationDesc::Reg(m_rax, sizeInBytes);
    }

    bool canReturnInRegs(MirType *type) const override
    {
        if (!type) return true;
        return type->getTotalSizeInBits() <= 128;
    }

    bool isCalleeCleanup() const override { return false; }
    bool doesStackGrowsDownwards() const override { return true; }
    const char *getName() const override { return "ExampleSystemV"; }
    MirRegisterRef getFramePointerReg() const override { return m_rbp; }
    MirRegisterRef getStackPointerReg() const override { return m_rsp; }
    size_t getStackAlignment() const override { return 16; }
    size_t getShadowSpaceSize() const override { return 0; }
    bool hasFramePointer(MirFunction *func) const override { (void)func; return true; }
    void classify(MirType *type, std::pmr::vector<CallingConvTypeClass> &out) const override
    {
        (void)type;
        out.push_back(CallingConvTypeClass::Integer);
    }

    const std::pmr::vector<MirRegisterRef> &getAllCalleeSavedRegs() override { return m_calleeSaved; }
    const std::pmr::vector<MirRegisterRef> &getCalleeSavedRegs(MirRegisterClass *_class) override { (void)_class; return m_calleeSaved; }
    const std::pmr::vector<MirRegisterRef> &getAllCallerSavedRegs() override { return m_callerSaved; }
    const std::pmr::vector<MirRegisterRef> &getCallerSavedRegs(MirRegisterClass *_class) override { (void)_class; return m_callerSaved; }

  private:
    MirRegisterRef m_rax;
    MirRegisterRef m_rdi;
    MirRegisterRef m_rsi;
    MirRegisterRef m_rbp;
    MirRegisterRef m_rsp;
    std::pmr::vector<MirRegisterRef> m_calleeSaved;
    std::pmr::vector<MirRegisterRef> m_callerSaved;
};

class ExampleTargetFrameLowerer : public MirFrameLowerer
{
  public:
    void insertPrologue(FrameLowererCtx &ctx) override
    {
        (void)ctx;
    }

    void insertEpilogue(FrameLowererCtx &ctx) override
    {
        (void)ctx;
    }

    bool lowerAlloc(FrameLowererCtx &ctx) override
    {
        (void)ctx;
        return false;
    }

    bool lowerDAlloc(FrameLowererCtx &ctx) override
    {
        (void)ctx;
        return false;
    }
};

class ExampleTargetTypeLayout : public IMirTargetTypeLayout
{
  public:
    size_t getPointerSizeInBytes() const override { return 8; }
    size_t getTypeAlignmentInBytes(const MirType *type) const override { return type ? (type->getTotalSizeInBits() + 7) / 8 : 8; }
    size_t getTypeSizeInBytes(const MirType *type) const override { return type ? (type->getTotalSizeInBits() + 7) / 8 : 8; }
};

class ExampleTargetISel : public MirInstructionSelector
{
  public:
    explicit ExampleTargetISel(MirTargetInstructionDesc *addDesc, MirTargetInstructionDesc *retDesc) :
        m_addDesc(addDesc), m_retDesc(retDesc)
    {
    }

    bool select(MirBuilderContext *ctx, MirInstruction *inst) override
    {
        if (!ctx || !inst) return false;

        // If instruction is already a target instruction, nothing to do
        if (inst->getOpCode() == MirInstructionOpCode::TARGET_INST && inst->getTargetDesc() != nullptr)
        {
            return true;
        }

        if (inst->getOpCode() == MirInstructionOpCode::ADD)
        {
            auto &operands = inst->getOperands();
            auto it = std::find(inst->getOwner()->getInstructions().begin(), inst->getOwner()->getInstructions().end(), inst);
            MirInstructionBuilder ib(ctx, inst->getOwner(), InsertionType::InsertBefore, it);
            ib.buildTarget(m_addDesc, inst->getSourceRef(), { operands[0], operands[1], operands[2] });
            inst->getOwner()->getInstructions().erase(it);
            return true;
        }
        else if (inst->getOpCode() == MirInstructionOpCode::RET)
        {
            auto &operands = inst->getOperands();
            auto it = std::find(inst->getOwner()->getInstructions().begin(), inst->getOwner()->getInstructions().end(), inst);
            MirInstructionBuilder ib(ctx, inst->getOwner(), InsertionType::InsertBefore, it);
            if (!operands.empty())
            {
                ib.buildTarget(m_retDesc, inst->getSourceRef(), { operands[0] });
            }
            else
            {
                ib.buildTarget(m_retDesc, inst->getSourceRef(), {});
            }
            inst->getOwner()->getInstructions().erase(it);
            return true;
        }

        return false;
    }

  private:
    MirTargetInstructionDesc *m_addDesc;
    MirTargetInstructionDesc *m_retDesc;
};

class ExampleTargetDesc : public TargetDesc
{
  public:
    explicit ExampleTargetDesc(MirBuilderContext *ctx) :
        m_ctx(ctx),
        m_banks(ctx->getGlobalAllocator()),
        m_callingConvs(ctx->getGlobalAllocator()),
        m_binDescs(ctx->getGlobalAllocator())
    {
        std::pmr::polymorphic_allocator<> alloc(ctx->getGlobalAllocator());
        m_gprBank = alloc.new_object<MirRegisterBank>("GPR", ctx->getGlobalAllocator());
        m_gprClass = alloc.new_object<MirRegisterClass>("GPR64", m_gprBank, ctx->getGlobalAllocator());
        m_gprBank->addClass("GPR64", m_gprClass);

        m_cc = std::make_unique<ExampleTargetCallingConv>(m_gprClass, ctx->getGlobalAllocator());
        m_layout = std::make_unique<ExampleTargetTypeLayout>();
        m_frameLowerer = std::make_unique<ExampleTargetFrameLowerer>();

        m_addDesc = std::make_unique<MirTargetInstructionDesc>(
            "ADD_r64_r64",
            1001,
            std::initializer_list<MirOperandFlag>{ MirOperandFlag::Write, MirOperandFlag::Read, MirOperandFlag::Read }
        );
        m_retDesc = std::make_unique<MirTargetInstructionDesc>(
            "RET_r64",
            1002,
            std::initializer_list<MirOperandFlag>{ MirOperandFlag::Read }
        );

        m_isel = std::make_unique<ExampleTargetISel>(m_addDesc.get(), m_retDesc.get());
        m_legalizer = std::make_unique<MirLegalizer>(ctx, this);

        m_banks.push_back(m_gprBank);
        m_callingConvs.push_back(m_cc.get());
    }

    const char *getName() const override { return "ExampleTarget"; }
    IMirTargetTypeLayout *getTypeLayout() override { return m_layout.get(); }
    MirExpansionRuleRegistry *getExpansionRegistry() override { return nullptr; }
    MirFrameLowerer *getFrameLowerer() override { return m_frameLowerer.get(); }
    MirInstructionSelector *getInstructionSelector() override { return m_isel.get(); }
    MirLegalizer *getLegalizer() override { return m_legalizer.get(); }
    MirRegisterAllocator *getRegisterAllocator() override { return nullptr; }
    MirType *getMemOperandDisplacementType() override { return m_ctx ? m_ctx->getTypeTable()->i64() : nullptr; }
    MirType *getNearestLegalType(MirType *type) override { return type; }
    MirRegisterRef getInstructionPtrReg() const override { return MirRegisterRef{}; }
    size_t getStackSlotSize() const override { return 8; }
    void initialize() override {}
    std::pmr::vector<TargetBinaryDesc *> getAvailableBinaryDescriptors() override { return m_binDescs; }
    std::pmr::vector<CallingConvDesc *> getAvailableCallingConventions() override { return m_callingConvs; }
    std::pmr::vector<MirRegisterBank *> getAvailableRegisterBanks() override { return m_banks; }

    CallingConvDesc *getCallingConv() const { return m_cc.get(); }
    MirRegisterClass *getGprClass() const { return m_gprClass; }

  private:
    MirBuilderContext *m_ctx;
    MirRegisterBank *m_gprBank;
    MirRegisterClass *m_gprClass;
    std::unique_ptr<ExampleTargetCallingConv> m_cc;
    std::unique_ptr<ExampleTargetTypeLayout> m_layout;
    std::unique_ptr<ExampleTargetFrameLowerer> m_frameLowerer;
    std::unique_ptr<MirTargetInstructionDesc> m_addDesc;
    std::unique_ptr<MirTargetInstructionDesc> m_retDesc;
    std::unique_ptr<ExampleTargetISel> m_isel;
    std::unique_ptr<MirLegalizer> m_legalizer;
    std::pmr::vector<MirRegisterBank *> m_banks;
    std::pmr::vector<CallingConvDesc *> m_callingConvs;
    std::pmr::vector<TargetBinaryDesc *> m_binDescs;
};

} // anonymous namespace

class EzDslEndToEndTargetPipelineTest : public DslTestSuiteAsGtest
{
  protected:
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_tempDir = std::filesystem::temp_directory_path() / ("ezdsl_e2e_test_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(m_tempDir);
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(m_tempDir, ec);
        DslTestSuiteAsGtest::TearDown();
    }

    std::filesystem::path m_tempDir;
};

TEST_F(EzDslEndToEndTargetPipelineTest, CompleteTargetSynthesisAndPipelineExecution)
{
    // 1. Define complete DSL specifications for ExampleTarget
    std::string typesDsl = R"dsl(
integer i1(1);
integer i8(8);
integer i16(16);
integer i32(32);
integer i64(64);
)dsl";

    std::string targetDsl = R"dsl(
target ExampleCpu {
    bank GPR {
        CLASS(GPR64,
            rax(, 64, 0),
            rbx(, 64, 1),
            rcx(, 64, 2),
            rdx(, 64, 3),
            rsi(, 64, 4),
            rdi(, 64, 5),
            rbp(, 64, 6),
            rsp(, 64, 7)
        );
    };
};
)dsl";

    std::string instsDsl = R"dsl(

format I_TYPE(32) {
    f[0:31];
};

inst ADD_r64_r64(GPR64:dst OUT, GPR64:lhs IN, GPR64:rhs IN) format I_TYPE {
}

inst MOV_r64_r64(GPR64:dst OUT, GPR64:src IN) format I_TYPE {
}
inst RET_r64(GPR64:val IN) format I_TYPE {
    FLAGS(isReturn);
};
)dsl";

    std::string ladDsl = R"dsl(
action ADD {
    LEGAL(i64);
    WIDENS(i32) >> i64;
};

action MOV {
    LEGAL(i64);
};

action RET {
    LEGAL(i64);
};
)dsl";

    std::string isfDsl = R"dsl(
pattern SelectAdd {
    match {
        ADD GPR64:$dst, GPR64:$lhs, GPR64:$rhs;
    };
    emit {
        ADD_r64_r64 GPR64:$dst, GPR64:$lhs, GPR64:$rhs;
    };
    cost(1);
};

pattern SelectMov {
    match {
        MOV GPR64:$dst, GPR64:$src;
    };
    emit {
        MOV_r64_r64 GPR64:$dst, GPR64:$src;
    };
    cost(1);
};

pattern SelectRet {
    match {
        RET GPR64:$val;
    };
    emit {
        RET_r64 GPR64:$val;
    };
    cost(1);
};
)dsl";

    std::string ccdfDsl = R"dsl(
calling_conv ExampleSystemV {
    STACK_ALIGN 16;
    STACK_DIRECTION DOWN;
    STACK_CLEANUP CALLER;
    SHADOW_SPACE 0;

    STACK_POINTER GPR64:rsp;
    FRAME_POINTER GPR64:rbp;

    CALLEE_SAVED (GPR64:rbp, GPR64:rsp, GPR64:rbx);
    CALLER_SAVED (GPR64:rdi, GPR64:rsi, GPR64:rax, GPR64:rdx, GPR64:rcx);

    CLASSIFY {
        TYPE(i64) -> INTEGER;
    };

    PASS {
        ASSIGN(INTEGER) >> REG_SEQ(GPR64:rdi, GPR64:rsi, GPR64:rdx, GPR64:rcx) >> STACK;
    };

    RETURN {
        ASSIGN(INTEGER) >> REG_SEQ(GPR64:rax);
    };
};
)dsl";

    SymbolTable symTable(getAllocator());

    // Register IR instructions in global symbol table
    Sema::Symbols::IrInstructionSymbol irAdd{ .m_name = "ADD" };
    symTable.declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::IrInstruction, irAdd, "ADD");
    Sema::Symbols::IrInstructionSymbol irMov{ .m_name = "MOV" };
    symTable.declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::IrInstruction, irMov, "MOV");
    Sema::Symbols::IrInstructionSymbol irRet{ .m_name = "RET" };
    symTable.declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::IrInstruction, irRet, "RET");

    // Run Sema Passes
    ParseContext typeCtx = createParseContextFromBuff("types.tyf", typesDsl);
    auto typeAst = typeCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    ASSERT_TRUE(typeAst.has_value());
    TypePass typePass;
    ASSERT_TRUE(typePass.run(getDiagCollector(), &symTable, &*typeAst));

    ParseContext targetCtx = createParseContextFromBuff("target.tdf", targetDsl);
    auto targetAst = targetCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(targetAst.has_value());
    ASSERT_TRUE(TargetDefPass::run(getDiagCollector(), &symTable, &*targetAst));
    ASSERT_TRUE(RegisterBankPass::run(getDiagCollector(), &symTable, &*targetAst));

    ParseContext instCtx = createParseContextFromBuff("insts.idf", instsDsl);
    auto instAst = instCtx.parse<DSL::Parser::InstDef::InstDefFile, DSL::Ast::InstDef::InstDefFile>();
    ASSERT_TRUE(instAst.has_value());
    ASSERT_TRUE(InstructionDefPass::run(getDiagCollector(), &symTable, &*instAst));

    symTable.enterScope("ExampleCpu");

    ParseContext ladCtx = createParseContextFromBuff("legal.lad", ladDsl);
    auto ladAst = ladCtx.parse<DSL::Parser::LegalizeActionDef::TargetLegalizeDef, DSL::Ast::LegalizeActionDef::TargetLegalizeDef>();
    ASSERT_TRUE(ladAst.has_value());
    ASSERT_TRUE(LegalizeActionPass::run(getDiagCollector(), &symTable, &*ladAst));

    ParseContext isfCtx = createParseContextFromBuff("rules.isf", isfDsl);
    auto isfAst = isfCtx.parse<DSL::Parser::InstSelDef::ISelDefFileParser, DSL::Ast::InstSelDef::ISelDefFile>();
    ASSERT_TRUE(isfAst.has_value());
    ASSERT_TRUE(InstSelPass::run(getDiagCollector(), &symTable, &*isfAst));

    ParseContext ccdfCtx = createParseContextFromBuff("convs.ccdf", ccdfDsl);
    auto ccdfAst = ccdfCtx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile, DSL::Ast::CallingConvDef::CallingConvDefFile>();
    ASSERT_TRUE(ccdfAst.has_value());
    ASSERT_TRUE(CallingConvPass::run(getDiagCollector(), &symTable, &*ccdfAst));

    // 2. Synthesize all target code generators into m_tempDir
    ASSERT_TRUE(CodeGenerators::GenerateTargetRegisterBanks(getDiagCollector(), &symTable, m_tempDir, "ExampleCpu"));
    ASSERT_TRUE(CodeGenerators::GenerateTargetInstructionDefs(getDiagCollector(), &symTable, m_tempDir, "ExampleCpu"));
    ASSERT_TRUE(CodeGenerators::GenerateTargetTypeLayout(getDiagCollector(), &symTable, m_tempDir, "ExampleCpu"));
    ASSERT_TRUE(CodeGenerators::GenerateTargetLegalizerTable(getDiagCollector(), &symTable, m_tempDir, "ExampleCpu"));
    ASSERT_TRUE(CodeGenerators::GenerateTargetLegalizerRules(getDiagCollector(), &symTable, m_tempDir, "ExampleCpu"));
    ASSERT_TRUE(CodeGenerators::GenerateTargetISelTable(getDiagCollector(), &symTable, m_tempDir, "ExampleCpu"));
    ASSERT_TRUE(CodeGenerators::GenerateTargetCallingConventions(getDiagCollector(), &symTable, m_tempDir, "ExampleCpu"));
    ASSERT_TRUE(CodeGenerators::GenerateTargetDescriptor(getDiagCollector(), &symTable, m_tempDir, "ExampleCpu"));

    // Verify generated files exist
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "ExampleCpuRegisterBanks.h"));
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "ExampleCpuInstructionDefs.h"));
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "ExampleCpuTypeLayout.h"));
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "ExampleCpuLegalizerActionTable.h"));
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "ExampleCpuLegalizeRules.h"));
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "ExampleCpuISelTable.h"));
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "ExampleCpuCallingConventions.h"));
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "ExampleCpuTargetDesc.h"));

    // 3. Build MIR program and execute full compiler pipeline on ExampleTarget
    std::pmr::monotonic_buffer_resource arena(1024 * 1024);
    DiagnosticCollector diagCollector;
    MirTypeTable typeTable(&arena);
    MirBuilderContext builderCtx(nullptr, &diagCollector, &typeTable, &arena);

    ExampleTargetDesc targetDesc(&builderCtx);
    typeTable.initialize(targetDesc.getTypeLayout());
    builderCtx.setDefaultCallingConvention(targetDesc.getCallingConv());

    // Create test function: i64 example_func(i64 %arg0)
    MirFunctionBuilder funcBuilder(&builderCtx);
    funcBuilder.setCallingConvention(targetDesc.getCallingConv());
    std::pmr::string funcName("example_func", &arena);
    MirFunction *func = funcBuilder.build(typeTable.i64(), funcName);

    MirBlockBuilder blockBuilder(&builderCtx, func);
    std::pmr::string entryName("entry", &arena);
    MirBlock *entryBlock = blockBuilder.build(nullptr, entryName);

    // Create a stack frame object
    StackFrameObject *stackObj = func->getStackFrame()->createStaticStackObj(typeTable.i64());
    ASSERT_NE(stackObj, nullptr);

    MirInstructionBuilder ib(&builderCtx, entryBlock, InsertionType::Append);
    MirOperandBuilder ob(&builderCtx);

    MirRegister *v0 = ob.buildVReg(typeTable.i64(), "v0", nullptr, targetDesc.getGprClass());
    MirRegister *v1 = ob.buildVReg(typeTable.i64(), "v1", nullptr, targetDesc.getGprClass());
    MirRegister *vSum = ob.buildVReg(typeTable.i64(), "vSum", nullptr, targetDesc.getGprClass());

    // Load from stack object reference: LOAD %v0, %stack[0]
    MirReference *stackRef = ob.buildRef(stackObj);
    ib.LOAD(v0, stackRef);

    // Arithmetic operation: ADD %vSum, %v0, %v1
    ib.ADD(vSum, v0, v1);

    // Return value: RET %vSum
    ib.RET(vSum);

    // --- PIPELINE EXECUTION ---
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    // Step 1: Legalization Pass (MirLegalizerPass)
    MirLegalizerPass legPass(&builderCtx, &targetDesc);
    auto legResult = legPass.run(funcList, funcList.begin(), nullptr);
    EXPECT_TRUE(legResult.m_succeeded);

    // Step 2: Instruction Selection Pass (MirInstructionSelectorPass)
    MirInstructionSelectorPass iselPass(&builderCtx, &targetDesc);
    auto iselResult = iselPass.run(funcList, funcList.begin(), nullptr);
    EXPECT_TRUE(iselResult.m_succeeded);

    // Verify all arithmetic & return instructions are selected into TARGET_INST
    for (MirInstruction *inst : entryBlock->getInstructions())
    {
        if (inst->getOpCode() != MirInstructionOpCode::LOAD &&
            inst->getOpCode() != MirInstructionOpCode::PUSH_RET)
        {
            EXPECT_TRUE(inst->getOpCode() == MirInstructionOpCode::TARGET_INST || inst->getTargetDesc() != nullptr);
        }
    }

    // Step 3: ABI Lowering Pass (MirAbiLowererPass)
    MirAbiLowererPass abiPass(&builderCtx);
    auto abiResult = abiPass.run(funcList, funcList.begin(), nullptr);
    EXPECT_TRUE(abiResult.m_succeeded);

    // Step 4: Register Allocator Pass (MirRegisterAllocatorPass)
    MirRegisterAllocatorPass regAllocPass(&builderCtx, &targetDesc);
    auto regResult = regAllocPass.run(funcList, funcList.begin(), nullptr);
    EXPECT_TRUE(regResult.m_succeeded);

    // Step 5: Frame Lowerer Pass (MirFrameLowererPass)
    MirFrameLowererPass framePass(&builderCtx, &targetDesc);
    auto frameResult = framePass.run(funcList, funcList.begin(), nullptr);
    EXPECT_TRUE(frameResult.m_succeeded);
    EXPECT_TRUE(frameResult.m_modifiedMir);

    // Verification:
    // Frame layout must have been calculated correctly
    EXPECT_GT(func->getAnalysisData()->m_totalFrameSize, 0);
    EXPECT_EQ(func->getAnalysisData()->m_totalFrameSize % 16, 0);

    // Abstract stack object reference %stack[0] must have been lowered to a concrete MirMemory operand
    bool foundConcreteMemOp = false;
    for (MirInstruction *inst : entryBlock->getInstructions())
    {
        for (MirOperand *op : inst->getOperands())
        {
            if (op && op->isOfType<MirMemory>())
            {
                foundConcreteMemOp = true;
                MirMemory *mem = op->get<MirMemory>();
                EXPECT_NE(mem->getBase(), nullptr);
            }
        }
    }
    EXPECT_TRUE(foundConcreteMemOp);
}
