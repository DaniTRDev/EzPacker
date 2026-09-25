#include "EzDslSemaTestSuite.h"
#include "Ast/TargetDescDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDescDefLang.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/RegisterSymbols.h"
#include "Sema/Symbols/TargetDescSymbols.h"
#include "SemaPasses/TargetDescPass.h"

using namespace DSL;

/**
 * Test fixture for the .tdesc target descriptor semantic pass.
 */
class TargetDescPassTest : public EzDslSemaTestSuiteAsGtest
{
  protected:
    // Parses target descriptor source and runs the target-descriptor semantic pass.
    bool parseAndRun(const std::string &sourceContent)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.tdesc", m_currentTestId++), sourceContent);
        m_ast = ctx.parse<Parser::TargetDesc::TargetDescFileParser, Ast::TargetDesc::TargetDescFile>();
        if (!m_ast.has_value())
        {
            return false;
        }

        TargetDescPass pass;
        return pass.run(getDiagCollector(), getSymbolTable(), &m_ast.value());
    }

    std::optional<Ast::TargetDesc::TargetDescFile> m_ast;

  private:
    size_t m_currentTestId{ 0 };
};

// Verifies a valid manifest passes and declares a TargetDesc symbol.
TEST_F(TargetDescPassTest, AcceptsValidManifestAndDeclaresSymbol)
{
    std::string source = R"(
target X86_64 {
    registers: "x86_64_registers.reg";
    pointer_size: 8;
    stack_slot: 8;
    instruction_pointer: rip;
    object_formats: [ELF, COFF];
    default_calling_conv: SysV_AMD64;
    libcalls {
        __returnNothing: "__returnNothing";
        __divdi3: "__divdi3"
    }
    components {
        frame_lowerer: X86_64FrameLowerer;
        register_allocator: X86_64RegisterAllocator
    }
}
)";

    EXPECT_TRUE(parseAndRun(source));
    auto *sym = getSymbolTable()->getSymByName("X86_64", SymbolType::TargetDesc);
    ASSERT_NE(sym, nullptr);
    EXPECT_TRUE(sym->hasData<Symbols::TargetDescSymbol>());
    EXPECT_EQ(sym->getIf<Symbols::TargetDescSymbol>()->m_name, "X86_64");
}

// Verifies a zero pointer size is rejected.
TEST_F(TargetDescPassTest, RejectsNonPositivePointerSize)
{
    std::string source = R"(
target Bad {
    pointer_size: 0;
    object_formats: [ELF];
    default_calling_conv: C
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies a stack slot size that is not a power of two is rejected.
TEST_F(TargetDescPassTest, RejectsNonPowerOfTwoStackSlot)
{
    std::string source = R"(
target Bad {
    pointer_size: 8;
    stack_slot: 12;
    object_formats: [ELF];
    default_calling_conv: C
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies declaring the same component slot twice is rejected.
TEST_F(TargetDescPassTest, RejectsDuplicateComponentSlot)
{
    std::string source = R"(
target Bad {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;
    components {
        frame_lowerer: A;
        frame_lowerer: B
    }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies declaring the same libcall id twice is rejected.
TEST_F(TargetDescPassTest, RejectsDuplicateLibcallId)
{
    std::string source = R"(
target Bad {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;
    libcalls {
        __divdi3: "__divdi3";
        __divdi3: "__udivdi3"
    }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies a second target descriptor with the same name is rejected.
TEST_F(TargetDescPassTest, RejectsDuplicateTargetSymbol)
{
    std::string source = R"(
target Dup {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C
}
)";

    EXPECT_TRUE(parseAndRun(source));

    ParseContext ctx = createParseContextFromBuff("dup_second.tdesc", source);
    auto ast = ctx.parse<Parser::TargetDesc::TargetDescFileParser, Ast::TargetDesc::TargetDescFile>();
    ASSERT_TRUE(ast.has_value());

    TargetDescPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

// Verifies valid extensions pass semantic analysis.
TEST_F(TargetDescPassTest, AcceptsValidExtensions)
{
    std::string source = R"(
target ValidExt {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    extensions {
        sse {
            default: true;
        };
        sse2 {
            default: true;
            implies: [sse];
        };
        avx {
            implies: [sse2];
        };
    }
}
)";

    EXPECT_TRUE(parseAndRun(source));
    auto *sym = getSymbolTable()->getSymByName("ValidExt", SymbolType::TargetDesc);
    ASSERT_NE(sym, nullptr);
    const auto *descSym = sym->getIf<Symbols::TargetDescSymbol>();
    ASSERT_NE(descSym, nullptr);
    ASSERT_NE(descSym->m_astNode, nullptr);
    EXPECT_EQ(descSym->m_astNode->m_extensions.size(), 3u);
}

// Verifies duplicate extension definitions are rejected.
TEST_F(TargetDescPassTest, RejectsDuplicateExtension)
{
    std::string source = R"(
target BadExt {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    extensions {
        avx { default: true; };
        avx { default: false; }
    }
}
)";

    EXPECT_FALSE(parseAndRun(source));
}

// Verifies self-implication is rejected.
TEST_F(TargetDescPassTest, RejectsSelfImpliedExtension)
{
    std::string source = R"(
target BadExt {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    extensions {
        avx { implies: [avx]; }
    }
}
)";

    EXPECT_FALSE(parseAndRun(source));
}

// Verifies implying an undeclared extension is rejected.
TEST_F(TargetDescPassTest, RejectsUnknownImpliedExtension)
{
    std::string source = R"(
target BadExt {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    extensions {
        avx { implies: [non_existent_feature]; }
    }
}
)";

    EXPECT_FALSE(parseAndRun(source));
}

// Verifies valid inline register banks and special registers are accepted and symbols declared.
TEST_F(TargetDescPassTest, AcceptsValidRegisterBanksAndDeclaresRegisterSymbols)
{
    std::string source = R"(
target X86_64 {
    pointer_size: 8;
    stack_slot: 8;
    instruction_pointer: rip;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank GPR {
        classes { GPR8: 8, GPR16: 16, GPR32: 32, GPR64: 64 }
        sub_register { GPR16 <: GPR8, GPR32 <: GPR16, GPR64 <: GPR32 }
        registers {
            rax enc 0 names { rax: GPR64, eax: GPR32, ax: GPR16, al: GPR8 }
            rcx enc 1 names { rcx: GPR64, ecx: GPR32, cx: GPR16, cl: GPR8 }
        }
    }

    register_bank FPR {
        classes { FPR32: 32, FPR64: 64 }
        sub_register { FPR64 <: FPR32 }
        registers {
            xmm0 enc 0 names { xmm0: FPR32, xmm0: FPR64 }
        }
    }

    special {
        rip: 16
    }
}
)";

    EXPECT_TRUE(parseAndRun(source));

    auto *targetSym = getSymbolTable()->getSymByName("X86_64", SymbolType::TargetDesc);
    ASSERT_NE(targetSym, nullptr);
    EXPECT_TRUE(targetSym->hasData<Symbols::TargetDescSymbol>());

    auto *bankGpr = getSymbolTable()->getSymByName("GPR", SymbolType::RegisterBank);
    ASSERT_NE(bankGpr, nullptr);
    EXPECT_TRUE(bankGpr->hasData<Symbols::RegisterBankSymbol>());
    EXPECT_EQ(bankGpr->getIf<Symbols::RegisterBankSymbol>()->m_target, "X86_64");

    auto *bankFpr = getSymbolTable()->getSymByName("FPR", SymbolType::RegisterBank);
    ASSERT_NE(bankFpr, nullptr);
    EXPECT_TRUE(bankFpr->hasData<Symbols::RegisterBankSymbol>());

    auto *classSym = getSymbolTable()->getSymByName("GPR64", SymbolType::RegisterClass);
    ASSERT_NE(classSym, nullptr);
    const auto *classData = classSym->getIf<Symbols::RegisterClassSymbol>();
    ASSERT_NE(classData, nullptr);
    EXPECT_EQ(classData->m_bitSize, 64u);
    EXPECT_EQ(classData->m_bankName, "GPR");

    auto *regSym = getSymbolTable()->getSymByName("rax", SymbolType::Register);
    ASSERT_NE(regSym, nullptr);
    const auto *regData = regSym->getIf<Symbols::RegisterSymbol>();
    ASSERT_NE(regData, nullptr);
    EXPECT_EQ(regData->m_hwEncoding, 0u);
    EXPECT_EQ(regData->m_bankName, "GPR");

    auto *specialSym = getSymbolTable()->getSymByName("rip", SymbolType::SpecialRegister);
    ASSERT_NE(specialSym, nullptr);
    const auto *specialData = specialSym->getIf<Symbols::SpecialRegisterSymbol>();
    ASSERT_NE(specialData, nullptr);
    EXPECT_EQ(specialData->m_id, 16u);
    EXPECT_EQ(specialData->m_target, "X86_64");
}

// Verifies grouped registers { ... } block declares symbols properly.
TEST_F(TargetDescPassTest, AcceptsRegistersGroupedSyntax)
{
    std::string source = R"(
target X86_64 {
    pointer_size: 8;
    stack_slot: 8;
    instruction_pointer: rip;
    object_formats: [ELF];
    default_calling_conv: C;

    registers {
        register_bank GPR {
            classes { GPR64: 64 }
            registers {
                rax enc 0 names { rax: GPR64 }
            }
        }
        special {
            rip: 16
        }
    }
}
)";

    EXPECT_TRUE(parseAndRun(source));

    auto *bankSym = getSymbolTable()->getSymByName("GPR", SymbolType::RegisterBank);
    ASSERT_NE(bankSym, nullptr);
    EXPECT_TRUE(bankSym->hasData<Symbols::RegisterBankSymbol>());

    auto *regSym = getSymbolTable()->getSymByName("rax", SymbolType::Register);
    ASSERT_NE(regSym, nullptr);
    EXPECT_EQ(regSym->getIf<Symbols::RegisterSymbol>()->m_hwEncoding, 0u);

    auto *specialSym = getSymbolTable()->getSymByName("rip", SymbolType::SpecialRegister);
    ASSERT_NE(specialSym, nullptr);
    EXPECT_EQ(specialSym->getIf<Symbols::SpecialRegisterSymbol>()->m_id, 16u);
}

// Verifies duplicate register bank names are rejected.
TEST_F(TargetDescPassTest, RejectsDuplicateBankNames)
{
    std::string source = R"(
target Foo {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank GPR { classes { G8: 8 } registers { r0 enc 0 names { r0: G8 } } }
    register_bank GPR { classes { G8: 8 } registers { r1 enc 1 names { r1: G8 } } }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies two registers sharing an encoding within a bank are rejected.
TEST_F(TargetDescPassTest, RejectsDuplicateEncodingWithinBank)
{
    std::string source = R"(
target Foo {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank GPR {
        classes { G8: 8 }
        registers {
            r0 enc 0 names { r0: G8 }
            r1 enc 0 names { r1: G8 }
        }
    }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies a sub-register edge referencing an undeclared class is rejected.
TEST_F(TargetDescPassTest, RejectsSubRegisterEdgeToUndeclaredClass)
{
    std::string source = R"(
target Foo {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank GPR {
        classes { G8: 8 }
        sub_register { G16 <: G8 }
        registers { r0 enc 0 names { r0: G8 } }
    }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies a non-narrowing sub-register edge is rejected.
TEST_F(TargetDescPassTest, RejectsNonNarrowingSubRegisterEdge)
{
    std::string source = R"(
target Foo {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank GPR {
        classes { G8: 8, G16: 16 }
        sub_register { G8 <: G16 }
        registers { r0 enc 0 names { r0: G8 } }
    }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies a name binding referencing an undeclared class is rejected.
TEST_F(TargetDescPassTest, RejectsNameBindingToUndeclaredClass)
{
    std::string source = R"(
target Foo {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank GPR {
        classes { G8: 8 }
        registers { r0 enc 0 names { r0: G64 } }
    }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies a class name declared in multiple banks is rejected.
TEST_F(TargetDescPassTest, RejectsDuplicateClassAcrossBanks)
{
    std::string source = R"(
target Foo {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank A { classes { G8: 8 } registers { a0 enc 0 names { a0: G8 } } }
    register_bank B { classes { G8: 8 } registers { b0 enc 0 names { b0: G8 } } }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies special register id colliding with allocatable encodings is rejected.
TEST_F(TargetDescPassTest, RejectsSpecialRegisterIdCollision)
{
    std::string source = R"(
target Foo {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank GPR {
        classes { G8: 8 }
        registers { r0 enc 0 names { r0: G8 } }
    }
    special { rip: 0 }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies instruction_pointer pointing to unknown register is rejected when registers are present.
TEST_F(TargetDescPassTest, RejectsInstructionPointerToUnknownRegister)
{
    std::string source = R"(
target Foo {
    pointer_size: 8;
    stack_slot: 8;
    instruction_pointer: non_existent_reg;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank GPR {
        classes { G8: 8 }
        registers { r0 enc 0 names { r0: G8 } }
    }
    special { rip: 16 }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

// Verifies instruction_pointer matching a declared physical register is accepted.
TEST_F(TargetDescPassTest, AcceptsInstructionPointerMatchingPhysicalRegister)
{
    std::string source = R"(
target Foo {
    pointer_size: 8;
    stack_slot: 8;
    instruction_pointer: pc;
    object_formats: [ELF];
    default_calling_conv: C;

    register_bank GPR {
        classes { G32: 32 }
        registers { pc enc 15 names { pc: G32 } }
    }
}
)";
    EXPECT_TRUE(parseAndRun(source));
}

