#include "EzDslSemaTestSuite.h"
#include "Ast/CallingConvDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/CallingConvDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/CallingConvSymbols.h"
#include "SemaPasses/CallingConvPass.h"

class CallingConvPassTest : public EzDslSemaTestSuiteAsGtest
{
  protected:
    std::optional<DSL::Ast::CallingConvDef::CallingConventionDefFile> parseCallingConv(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.ezcc", m_currentTestId++), source);
        return ctx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile,
                         DSL::Ast::CallingConvDef::CallingConventionDefFile>();
    }

  private:
    size_t m_currentTestId{ 0 };
};

TEST_F(CallingConvPassTest, TestValidSysVCallingConv)
{
    std::string source = R"(
    calling_convention SysV_AMD64 {
        stack {
            align: 16,
            growth: down,
            cleanup: caller,
            shadow_space: 0,
            red_zone: 128,
            sp: rsp,
            fp: rbp
        }

        preserve callee: [rbx, rsp, rbp, r12, r13, r14, r15]
        preserve caller: [rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11]

        classify {
            types [i8, i16, i32, i64, ptr] => integer
            types [f32, f64] => sse
            aggregate {
                when non_trivial || unaligned || size > 64 => memory,
                slice: 8 bytes,
                precedence: [memory, integer, sse],
                policy: all_or_nothing
            }
        }

        arguments {
            pass integer => seq([rdi, rsi, rdx, rcx, r8, r9]), fallback: stack(8)
            pass sse => seq([xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7]), fallback: stack(8)
            pass memory => stack(8)
        }

        returns {
            sret {
                ptr: rdi,
                consumes_slot: true,
                returns: rax
            }
            pass integer => seq([rax, rdx])
            pass sse => seq([xmm0, xmm1])
        }

        varargs {
            vector_count_reg: al,
            duplicate_floats_to_gpr: false,
            stack_align: 8
        }
    }
    )";

    auto ast = parseCallingConv(source);
    ASSERT_TRUE(ast.has_value());

    bool success = CallingConvPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_TRUE(success);

    Symbol *sym = getSymbolTable()->getSymByName("SysV_AMD64");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::CallingConv);
    auto *data = sym->getIf<Symbols::CallingConvSymbol>();
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->m_name, "SysV_AMD64");
    EXPECT_NE(data->m_astNode, nullptr);
}

TEST_F(CallingConvPassTest, TestValidWin64CallingConv)
{
    std::string source = R"(
    calling_convention Win64 {
        stack {
            align: 16,
            growth: down,
            cleanup: caller,
            shadow_space: 32,
            sp: rsp,
            fp: rbp
        }

        preserve callee: [rbx, rsi, rdi, rbp, rsp, r12, r13, r14, r15]
        preserve caller: [rax, rcx, rdx, r8, r9, r10, r11]

        classify {
            types [i8, i16, i32, i64, ptr] => integer
            types [f32, f64] => float
            aggregate {
                when size in [1, 2, 4, 8] => integer,
                default => by_ref(implicit_copy: true)
            }
        }

        arguments {
            slots [
                { integer: rcx, float: xmm0 },
                { integer: rdx, float: xmm1 },
                { integer: r8, float: xmm2 },
                { integer: r9, float: xmm3 }
            ], fallback: stack(8)
            pass by_ref => integer
        }

        returns {
            sret {
                ptr: rcx,
                consumes_slot: true,
                returns: rax
            }
            pass integer => seq([rax])
            pass float => seq([xmm0])
        }

        varargs {
            duplicate_floats_to_gpr: true,
            stack_align: 8
        }
    }
    )";

    auto ast = parseCallingConv(source);
    ASSERT_TRUE(ast.has_value());

    bool success = CallingConvPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_TRUE(success);

    Symbol *sym = getSymbolTable()->getSymByName("Win64");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::CallingConv);
}

TEST_F(CallingConvPassTest, TestInvalidStackAlignment)
{
    std::string source = R"(
    calling_convention BadAlign {
        stack {
            align: 15,
            growth: down,
            cleanup: caller,
            shadow_space: 0,
            sp: rsp,
            fp: rbp
        }
        preserve callee: [rbx]
        preserve caller: [rax]
    }
    )";

    auto ast = parseCallingConv(source);
    ASSERT_TRUE(ast.has_value());

    bool success = CallingConvPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_FALSE(success);
}

TEST_F(CallingConvPassTest, TestConflictingRegisterPreservation)
{
    std::string source = R"(
    calling_convention ConflictRegs {
        stack {
            align: 16,
            growth: down,
            cleanup: caller,
            shadow_space: 0,
            sp: rsp,
            fp: rbp
        }
        preserve callee: [rbx, rax]
        preserve caller: [rax, rcx]
    }
    )";

    auto ast = parseCallingConv(source);
    ASSERT_TRUE(ast.has_value());

    bool success = CallingConvPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_FALSE(success);
}
