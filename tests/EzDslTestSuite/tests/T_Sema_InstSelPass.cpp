#include "EzDslTestSuite.h"
#include "Ast/InstructionDefLangAst.h"
#include "Ast/InstructionSelDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/InstructionSelDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/InstSelPass.h"

class InstSelPassTest : public DslTestSuiteAsGtest
{
  protected:
    std::unique_ptr<SymbolTable> m_table;

    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_table = std::make_unique<SymbolTable>(getAllocator());

        setupTargetEnvironment();
        m_table->enterScope("TargetScope");
    }

    void registerType(std::string_view name, uint32_t bitWidth)
    {
        Sema::Symbols::TypeSymbol typeSym{ .m_name = name,
                                           .m_kind = static_cast<DSL::Ast::TypeDef::TypeKind>(0),
                                           .m_bitWidth = bitWidth };
        m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Type, typeSym, name);
    }

    void registerRegisterClass(std::string_view name)
    {
        Sema::Symbols::RegisterClassSymbol classSym{ .m_name = name,
                                                     .m_bankId = InvalidSymbolId,
                                                     .m_registers = std::pmr::vector<SymbolId>{ getAllocator() } };
        m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::RegisterClass, classSym, name);
    }

    void registerIrInstruction(std::string_view name, size_t numOperands)
    {
        std::pmr::vector<Sema::Symbols::IrOperandSymbol> operands{ getAllocator() };
        for (size_t i = 0; i < numOperands; ++i)
        {
            operands.push_back(
                    Sema::Symbols::IrOperandSymbol{ .m_typeMask = static_cast<DSL::Ast::IrInstDef::IrOperandType>(0),
                                                    .m_name = "op",
                                                    .m_dir = static_cast<DSL::Ast::IrInstDef::IrOperandDir>(0) });
        }

        Sema::Symbols::IrInstructionSymbol irSym{ .m_name = name,
                                                  .m_category = static_cast<DSL::Ast::IrInstDef::IrInstCategory>(0),
                                                  .m_tier = static_cast<DSL::Ast::IrInstDef::IrInstTier>(0),
                                                  .m_flagsMask = static_cast<DSL::Ast::IrInstDef::IrInstFlag>(0),
                                                  .m_operands = std::move(operands) };
        m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::IrInstruction, irSym, name);
    }

    void registerTargetInstruction(std::string_view name, std::pmr::vector<Sema::Symbols::TargetOperandSymbol> args)
    {
        Sema::Symbols::TargetInstructionSymbol instSym{
            .m_name = name,
            .m_formatId = InvalidSymbolId,
            .m_fieldAssignments = std::pmr::vector<Sema::Symbols::FieldAssignmentSymbol>{ getAllocator() },
            .m_args = std::move(args),
            .m_implicitArgs = std::pmr::vector<Sema::Symbols::TargetOperandSymbol>{ getAllocator() },
            .m_asmTemplate = "",
            .m_latency = 1,
            .m_flagsMask = 0
        };
        m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Instruction, instSym, name);
    }

    void setupTargetEnvironment()
    {
        // Primitive scalar types
        registerType("i1", 1);
        registerType("i8", 8);
        registerType("i12", 12);
        registerType("i16", 16);
        registerType("i32", 32);
        registerType("i64", 64);
        registerType("f32", 32);
        registerType("f64", 64);

        // Target Hardware Register Classes
        registerRegisterClass("GPR");
        registerRegisterClass("FPR");

        Symbol *gprSym = m_table->getSymByName("GPR");
        Symbol *fprSym = m_table->getSymByName("FPR");
        Symbol *i12Sym = m_table->getSymByName("i12");
        SymbolId gprId = gprSym ? gprSym->getId() : InvalidSymbolId;
        SymbolId fprId = fprSym ? fprSym->getId() : InvalidSymbolId;
        SymbolId i12Id = i12Sym ? i12Sym->getId() : InvalidSymbolId;

        // IR Instructions with strict arity definitions
        registerIrInstruction("ADD", 3);
        registerIrInstruction("SUB", 3);
        registerIrInstruction("MUL", 3);
        registerIrInstruction("SHL", 3);
        registerIrInstruction("LOAD", 2);
        registerIrInstruction("STORE", 2);
        registerIrInstruction("FADD", 3);

        // Target Instructions with formal parameter metadata
        registerTargetInstruction("ADD_R",
                                  { { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rd",
                                      DSL::Ast::InstDef::InstOperandDir::ArgOut },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rs1",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rs2",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn } });

        registerTargetInstruction("SUB_R",
                                  { { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rd",
                                      DSL::Ast::InstDef::InstOperandDir::ArgOut },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rs1",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rs2",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn } });

        registerTargetInstruction("ADDI",
                                  { { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rd",
                                      DSL::Ast::InstDef::InstOperandDir::ArgOut },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rs1",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn },
                                    { DSL::Ast::InstDef::InstOperandKind::Immediate,
                                      i12Id,
                                      "imm",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn } });

        registerTargetInstruction("SLLI",
                                  { { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rd",
                                      DSL::Ast::InstDef::InstOperandDir::ArgOut },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rs1",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn },
                                    { DSL::Ast::InstDef::InstOperandKind::Immediate,
                                      i12Id,
                                      "shamt",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn } });

        registerTargetInstruction("LW",
                                  { { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rd",
                                      DSL::Ast::InstDef::InstOperandDir::ArgOut },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "addr",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn } });

        registerTargetInstruction("SW",
                                  { { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "rs2",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      gprId,
                                      "addr",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn } });

        registerTargetInstruction("FADD_S",
                                  { { DSL::Ast::InstDef::InstOperandKind::Register,
                                      fprId,
                                      "rd",
                                      DSL::Ast::InstDef::InstOperandDir::ArgOut },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      fprId,
                                      "rs1",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn },
                                    { DSL::Ast::InstDef::InstOperandKind::Register,
                                      fprId,
                                      "rs2",
                                      DSL::Ast::InstDef::InstOperandDir::ArgIn } });
    }

    std::optional<DSL::Ast::InstSelDef::ISelDefFile> parseFile(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("InstSelPassTest", source);
        return ctx.parse<DSL::Parser::InstSelDef::ISelDefFileParser, DSL::Ast::InstSelDef::ISelDefFile>();
    }

    bool runPass(const std::string &source)
    {
        auto ast = parseFile(source);
        if (!ast.has_value())
        {
            return false;
        }
        return InstSelPass::run(getDiagCollector(), m_table.get(), &ast.value());
    }
};

// ============================================================================
// 1. Addressing Mode Validation Tests
// ============================================================================

TEST_F(InstSelPassTest, TestValidAddressingModeWithMultipleVariantsAndPredicates)
{
    std::string code = R"(
addrmode BaseOffset(GPR:base, simm(i12):offset = 0) {
    variant RegOffset {
        match {
            ADD $addr, GPR:$base, simm(i12):$offset;
        };
        when {
            isAligned($offset);
        };
    };
    variant BaseOnly {
        match {
            GPR:$base;
        };
    };
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("BaseOffset");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::AddrMode);

    const auto *addrData = sym->getIf<Sema::Symbols::AddrModeSymbol>();
    ASSERT_NE(addrData, nullptr);
    EXPECT_EQ(addrData->m_name, "BaseOffset");
    ASSERT_EQ(addrData->m_parameters.size(), 2);
    EXPECT_EQ(addrData->m_parameters[0].m_name, "base");
    EXPECT_EQ(addrData->m_parameters[0].m_kind, DSL::Ast::InstDef::InstOperandKind::Register);
    EXPECT_EQ(addrData->m_parameters[1].m_name, "offset");
    EXPECT_EQ(addrData->m_parameters[1].m_kind, DSL::Ast::InstDef::InstOperandKind::Immediate);
    ASSERT_EQ(addrData->m_variants.size(), 2);
    EXPECT_EQ(addrData->m_variants[0].m_name, "RegOffset");
    EXPECT_EQ(addrData->m_variants[1].m_name, "BaseOnly");
}

TEST_F(InstSelPassTest, TestErrorAddrModeNoVariants)
{
    std::string code = R"(
addrmode EmptyMode(GPR:base) {
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorDuplicateAddrModeParameterNames)
{
    std::string code = R"(
addrmode DuplicateParams(GPR:base, simm(i12):base) {
    variant Default {
        match { GPR:$base; };
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorUndefinedClassInAddrModeParameter)
{
    std::string code = R"(
addrmode UnknownClass(UNKNOWN_CLASS:base) {
    variant Default {
        match { GPR:$base; };
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorUndefinedTypeInAddrModeImmediateParameter)
{
    std::string code = R"(
addrmode UnknownType(simm(unknown_i24):offset) {
    variant Default {
        match { GPR:$base; };
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorDuplicateVariantNameWithinSameAddrMode)
{
    std::string code = R"(
addrmode DuplicateVariant(GPR:base) {
    variant SameName {
        match { GPR:$base; };
    };
    variant SameName {
        match { GPR:$base; };
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorUndefinedVariableInVariantPredicate)
{
    std::string code = R"(
addrmode InvalidPredVar(GPR:base, simm(i12):offset) {
    variant RegOffset {
        match {
            ADD $addr, GPR:$base, simm(i12):$offset;
        };
        when {
            isPowerOfTwo($unbound_variable);
        };
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorDuplicateAddrModeRedefinition)
{
    std::string code = R"(
addrmode MyMode(GPR:base) {
    variant V1 { match { GPR:$base; }; };
};

addrmode MyMode(GPR:base) {
    variant V2 { match { GPR:$base; }; };
};
)";

    EXPECT_FALSE(runPass(code));
}

// ============================================================================
// 2. ISel Pattern Matching & Argument Validation Tests
// ============================================================================

TEST_F(InstSelPassTest, TestValidPatternWithRegisterMatching)
{
    std::string code = R"(
pattern SelectAdd {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    cost(1);
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("SelectAdd");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::ISelPattern);

    const auto *patData = sym->getIf<Sema::Symbols::ISelPatternSymbol>();
    ASSERT_NE(patData, nullptr);
    EXPECT_EQ(patData->m_cost, 1u);
    ASSERT_EQ(patData->m_matchPatterns.size(), 1);
    ASSERT_EQ(patData->m_emitSequence.size(), 1);
}

TEST_F(InstSelPassTest, TestValidPatternDefaultCost)
{
    std::string code = R"(
pattern SelectAddNoCost {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("SelectAddNoCost");
    ASSERT_NE(sym, nullptr);
    const auto *patData = sym->getIf<Sema::Symbols::ISelPatternSymbol>();
    ASSERT_NE(patData, nullptr);
    EXPECT_EQ(patData->m_cost, 1u); // Default cost
}

TEST_F(InstSelPassTest, TestValidMultiInstructionExpansion)
{
    std::string code = R"(
pattern MultiInstExpansion {
    match {
        SUB GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD_R GPR:$tmp, GPR:$lhs, GPR:$rhs;
        SUB_R GPR:$dst, GPR:$tmp, GPR:$rhs;
    };
    cost(2);
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("MultiInstExpansion");
    ASSERT_NE(sym, nullptr);
    const auto *patData = sym->getIf<Sema::Symbols::ISelPatternSymbol>();
    ASSERT_NE(patData, nullptr);
    EXPECT_EQ(patData->m_emitSequence.size(), 2);
}

TEST_F(InstSelPassTest, TestValidPatternWithAddrModeAndTransform)
{
    std::string code = R"(
addrmode BaseOffset(GPR:base, simm(i12):offset = 0) {
    variant RegOffset {
        match {
            ADD $addr, GPR:$base, simm(i12):$offset;
        };
    };
};

pattern SelectLoad {
    match {
        LOAD GPR:$dst, BaseOffset:$addr;
    };
    when {
        isAligned($addr);
    };
    emit {
        LW GPR:$dst, $addr;
    };
    cost(2);
};
)";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("SelectLoad");
    ASSERT_NE(sym, nullptr);
    const auto *patData = sym->getIf<Sema::Symbols::ISelPatternSymbol>();
    ASSERT_NE(patData, nullptr);
    EXPECT_EQ(patData->m_cost, 2u);
}

TEST_F(InstSelPassTest, TestValidCustomTransformInEmit)
{
    std::string code = R"(
pattern SelectMulPowerOf2 {
    match {
        MUL GPR:$dst, GPR:$src, simm(i12):$imm;
    };
    when {
        isPowerOfTwo($imm);
    };
    emit {
        SLLI GPR:$dst, GPR:$src, log2($imm);
    };
    cost(1);
};
)";

    ASSERT_TRUE(runPass(code));
}

// ============================================================================
// 3. ISel Pattern Error & Semantic Constraint Invariants
// ============================================================================

TEST_F(InstSelPassTest, TestErrorPatternMissingMatch)
{
    std::string code = R"(
pattern MissingMatch {
    emit {
        ADD_R GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    cost(1);
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorPatternMissingEmit)
{
    std::string code = R"(
pattern MissingEmit {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    cost(1);
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorPatternNegativeCost)
{
    std::string code = R"(
pattern NegativeCost {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    cost(-5);
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorPatternDuplicateName)
{
    std::string code = R"(
pattern DuplicateName {
    match { ADD GPR:$dst, GPR:$lhs, GPR:$rhs; };
    emit { ADD_R GPR:$dst, GPR:$lhs, GPR:$rhs; };
};

pattern DuplicateName {
    match { SUB GPR:$dst, GPR:$lhs, GPR:$rhs; };
    emit { SUB_R GPR:$dst, GPR:$lhs, GPR:$rhs; };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorMatchUsesTargetInstructionInsteadOfIr)
{
    // ADD_R is a target instruction and must not be used in the match block
    std::string code = R"(
pattern TargetInMatch {
    match {
        ADD_R GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorEmitUsesIrInstructionInsteadOfTarget)
{
    // ADD is an IR instruction and must not be used in the emit sequence
    std::string code = R"(
pattern IrInEmit {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorMatchArityMismatchTooFewOperands)
{
    // ADD requires 3 operands, but only 2 are given
    std::string code = R"(
pattern TooFewMatchOps {
    match {
        ADD GPR:$dst, GPR:$lhs;
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs, GPR:$lhs;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorMatchArityMismatchTooManyOperands)
{
    // ADD requires 3 operands, but 4 are given
    std::string code = R"(
pattern TooManyMatchOps {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs, GPR:$extra;
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorEmitArityMismatch)
{
    // ADD_R expects 3 arguments, but 2 are emitted
    std::string code = R"(
pattern EmitArityMismatch {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorEmitOperandKindMismatchLiteralPassedToRegisterSlot)
{
    // ADD_R expects a register in slot 3, but integer literal 42 is passed
    std::string code = R"(
pattern LiteralToRegisterSlot {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs, 42;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorEmitOperandKindMismatchRegisterPassedToImmediateSlot)
{
    // ADDI expects an immediate in slot 3, but GPR register is passed
    std::string code = R"(
pattern RegisterToImmediateSlot {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADDI GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorEmitRegisterClassMismatch)
{
    // ADD_R destination expects GPR, but FPR is passed
    std::string code = R"(
pattern FPRToGPRMismatch {
    match {
        FADD FPR:$dst, FPR:$lhs, FPR:$rhs;
    };
    emit {
        ADD_R FPR:$dst, GPR:$lhs, GPR:$rhs;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorEmitImmediateTypeMismatch)
{
    // ADDI expects i12 immediate, but i16 immediate is passed
    std::string code = R"(
pattern ImmediateTypeMismatch {
    match {
        ADD GPR:$dst, GPR:$lhs, simm(i16):$imm;
    };
    emit {
        ADDI GPR:$dst, GPR:$lhs, simm(i16):$imm;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorCustomTransformInMatchBlock)
{
    // Custom transforms are not allowed in match patterns
    std::string code = R"(
pattern TransformInMatch {
    match {
        ADD GPR:$dst, GPR:$lhs, log2($rhs);
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs, $rhs;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorCustomTransformWithUndefinedVariable)
{
    // Transform argument '$unbound' is not defined in the match sequence
    std::string code = R"(
pattern UnboundTransformArg {
    match {
        MUL GPR:$dst, GPR:$src, simm(i12):$imm;
    };
    emit {
        SLLI GPR:$dst, GPR:$src, log2($unbound);
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorUndefinedVariableInWhenPredicate)
{
    // '$non_existent_var' is not bound in the match block
    std::string code = R"(
pattern UnboundWhenVar {
    match {
        ADD GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
    when {
        isPositive($non_existent_var);
    };
    emit {
        ADD_R GPR:$dst, GPR:$lhs, GPR:$rhs;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}

TEST_F(InstSelPassTest, TestErrorScopeIsolationBetweenPatterns)
{
    // Variable '$secret_var' declared in Pattern1 must not leak into Pattern2
    std::string code = R"(
pattern Pattern1 {
    match {
        ADD GPR:$secret_var, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD_R GPR:$secret_var, GPR:$lhs, GPR:$rhs;
    };
};

pattern Pattern2 {
    match {
        SUB GPR:$dst, GPR:$a, GPR:$b;
    };
    when {
        isPositive($secret_var); // $secret_var is not declared in Pattern2!
    };
    emit {
        SUB_R GPR:$dst, GPR:$a, GPR:$b;
    };
};
)";

    EXPECT_FALSE(runPass(code));
}