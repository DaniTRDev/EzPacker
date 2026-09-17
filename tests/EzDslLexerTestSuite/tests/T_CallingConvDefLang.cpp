#include "EzDslLexerTestSuite.h"
#include "Ast/CallingConvDefLangAst.h"
#include "Parser/CallingConvDefLang.h"
#include "Parser/ParseContext.h"

/**
 * Test fixture for Calling Convention Definition Language (.ezcc) parser.
 */
class CallingConvDefLangTest : public DslLexerTestSuiteAsGtest
{
};

// ============================================================================
// 1. System V AMD64 Calling Convention
// ============================================================================

TEST_F(CallingConvDefLangTest, TestSysVCallingConvParsing)
{
    std::string source = R"(
calling_convention x86_64_sysv {
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
        types [i1, i8, i16, i32, i64, ptr] => integer
        types [f32, f64]                  => float

        aggregate {
            when non_trivial => memory,
            when unaligned   => memory,
            when size > 64   => memory,
            slice: 8 bytes,
            precedence: [memory, integer, float],
            policy: all_or_nothing
        }
    }

    arguments {
        pass integer => seq([rdi, rsi, rdx, rcx, r8, r9]), fallback: stack(8, 8)
        pass float   => seq([xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7]), fallback: stack(8, 8)
        pass memory  => stack(8, 8)
    }

    returns {
        sret {
            ptr: rdi,
            consumes_slot: true,
            returns: rax
        }
        pass integer => seq([rax, rdx])
        pass float   => seq([xmm0, xmm1])
        pass memory  => stack(8, 8)
    }

    varargs {
        vector_count_reg: rax
    }
}
)";

    ParseContext ctx = createParseContextFromBuff("sysv.ezcc", source);
    auto res = ctx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile,
                         DSL::Ast::CallingConvDef::CallingConventionDefFile>();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "x86_64_sysv");

    // Stack assertions
    EXPECT_EQ(res->m_stack.m_alignment.m_node, 16);
    EXPECT_EQ(res->m_stack.m_growth, DSL::Ast::CallingConvDef::StackGrowth::Down);
    EXPECT_EQ(res->m_stack.m_cleanup, DSL::Ast::CallingConvDef::StackCleanup::Caller);
    EXPECT_EQ(res->m_stack.m_shadowSpace.m_node, 0);
    ASSERT_TRUE(res->m_stack.m_redZone.has_value());
    EXPECT_EQ(res->m_stack.m_redZone->m_node, 128);
    EXPECT_EQ(res->m_stack.m_stackPointer.m_node, "rsp");
    EXPECT_EQ(res->m_stack.m_framePointer.m_node, "rbp");

    // Preserved registers
    EXPECT_EQ(res->m_calleeSaved.size(), 7);
    EXPECT_EQ(res->m_callerSaved.size(), 9);

    // Classification assertions
    EXPECT_EQ(res->m_classification.m_primitives.size(), 2);
    EXPECT_EQ(res->m_classification.m_primitives[0].m_types.size(), 6);
    EXPECT_EQ(res->m_classification.m_primitives[0].m_targetClass.m_node, "integer");
    EXPECT_EQ(res->m_classification.m_primitives[1].m_targetClass.m_node, "float");

    ASSERT_TRUE(res->m_classification.m_aggregate.has_value());
    EXPECT_EQ(res->m_classification.m_aggregate->m_conditions.size(), 3);
    EXPECT_EQ(res->m_classification.m_aggregate->m_conditions[0].m_kind,
              DSL::Ast::CallingConvDef::AggregateCondition::Kind::NonTrivial);
    EXPECT_EQ(res->m_classification.m_aggregate->m_conditions[1].m_kind,
              DSL::Ast::CallingConvDef::AggregateCondition::Kind::Unaligned);
    EXPECT_EQ(res->m_classification.m_aggregate->m_conditions[2].m_kind,
              DSL::Ast::CallingConvDef::AggregateCondition::Kind::SizeGreaterThan);
    ASSERT_TRUE(res->m_classification.m_aggregate->m_sliceChunkSize.has_value());
    EXPECT_EQ(res->m_classification.m_aggregate->m_sliceChunkSize->m_node, 8);
    EXPECT_EQ(res->m_classification.m_aggregate->m_policy,
              DSL::Ast::CallingConvDef::AllocPolicy::AllOrNothing);

    // Arguments assertions
    EXPECT_EQ(res->m_arguments.m_rules.size(), 3);
    EXPECT_EQ(res->m_arguments.m_rules[0].m_abiClass.m_node, "integer");
    auto *intSeq = std::get_if<DSL::Ast::CallingConvDef::RegisterSequence>(&res->m_arguments.m_rules[0].m_source);
    ASSERT_NE(intSeq, nullptr);
    EXPECT_EQ(intSeq->m_registers.size(), 6);
    EXPECT_EQ(intSeq->m_registers[0].m_node, "rdi");
    ASSERT_TRUE(res->m_arguments.m_rules[0].m_fallback.has_value());
    EXPECT_EQ(res->m_arguments.m_rules[0].m_fallback->m_slotSize.m_node, 8);

    // Returns assertions
    ASSERT_TRUE(res->m_returns.m_sret.has_value());
    EXPECT_EQ(res->m_returns.m_sret->m_pointerRegister.m_node, "rdi");
    EXPECT_TRUE(res->m_returns.m_sret->m_consumesArgSlot);
    ASSERT_TRUE(res->m_returns.m_sret->m_returnRegister.has_value());
    EXPECT_EQ(res->m_returns.m_sret->m_returnRegister->m_node, "rax");

    // Varargs assertions
    ASSERT_TRUE(res->m_varargs.has_value());
    ASSERT_TRUE(res->m_varargs->m_vectorCountReg.has_value());
    EXPECT_EQ(res->m_varargs->m_vectorCountReg->m_node, "rax");
}

// ============================================================================
// 2. Microsoft Windows x64 Calling Convention
// ============================================================================

TEST_F(CallingConvDefLangTest, TestWin64CallingConvParsing)
{
    std::string source = R"(
calling_convention x86_64_windows {
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
        types [i1, i8, i16, i32, i64, ptr] => integer
        types [f32, f64]                  => float

        aggregate {
            when size in [1, 2, 4, 8] => integer,
            default                   => by_ref(implicit_copy: true)
        }
    }

    arguments {
        slots [
            { integer: rcx, float: xmm0, by_ref: rcx },
            { integer: rdx, float: xmm1, by_ref: rdx },
            { integer: r8,  float: xmm2, by_ref: r8  },
            { integer: r9,  float: xmm3, by_ref: r9  }
        ], fallback: stack(8, 8)
    }

    returns {
        sret {
            ptr: rcx,
            consumes_slot: true,
            returns: rax
        }
        pass integer => seq([rax])
        pass float   => seq([xmm0])
        pass by_ref  => rax
    }

    varargs {
        duplicate_floats_to_gpr: true
    }
}
)";

    ParseContext ctx = createParseContextFromBuff("win64.ezcc", source);
    auto res = ctx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile,
                         DSL::Ast::CallingConvDef::CallingConventionDefFile>();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "x86_64_windows");
    EXPECT_EQ(res->m_stack.m_shadowSpace.m_node, 32);

    // Aggregate condition: size in [1, 2, 4, 8]
    ASSERT_TRUE(res->m_classification.m_aggregate.has_value());
    const auto &conds = res->m_classification.m_aggregate->m_conditions;
    ASSERT_GE(conds.size(), 2);
    EXPECT_EQ(conds[0].m_kind, DSL::Ast::CallingConvDef::AggregateCondition::Kind::SizeIn);
    EXPECT_EQ(conds[0].m_sizeSet.size(), 4);
    EXPECT_EQ(conds[1].m_kind, DSL::Ast::CallingConvDef::AggregateCondition::Kind::Default);
    EXPECT_EQ(conds[1].m_resultClass.m_node, "by_ref");
    EXPECT_TRUE(conds[1].m_implicitCopy);

    // Unified slots
    EXPECT_EQ(res->m_arguments.m_unifiedSlots.size(), 4);
    EXPECT_EQ(res->m_arguments.m_unifiedSlots[0].m_bindings.size(), 3);
    EXPECT_EQ(res->m_arguments.m_unifiedSlots[0].m_bindings[0].m_abiClass.m_node, "integer");
    EXPECT_EQ(res->m_arguments.m_unifiedSlots[0].m_bindings[0].m_register.m_node, "rcx");
    EXPECT_EQ(res->m_arguments.m_unifiedSlots[0].m_bindings[1].m_abiClass.m_node, "float");
    EXPECT_EQ(res->m_arguments.m_unifiedSlots[0].m_bindings[1].m_register.m_node, "xmm0");

    // Varargs duplicate floats
    ASSERT_TRUE(res->m_varargs.has_value());
    EXPECT_TRUE(res->m_varargs->m_duplicateFloatsToGpr);
}

// ============================================================================
// 3. ARM AArch64 AAPCS64 Calling Convention
// ============================================================================

TEST_F(CallingConvDefLangTest, TestAArch64CallingConvParsing)
{
    std::string source = R"(
calling_convention aarch64_aapcs {
    stack {
        align: 16,
        growth: down,
        cleanup: caller,
        shadow_space: 0,
        sp: sp,
        fp: x29,
        lr: x30
    }

    preserve callee: [x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30]
    preserve caller: [x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15]

    classify {
        types [i1, i8, i16, i32, i64, ptr] => integer
        types [f16, f32, f64, f128]        => float

        aggregate {
            when hfa(float, max_elements: 4) => hfa,
            when size <= 16                  => integer,
            default                          => by_ref(implicit_copy: true)
        }
    }

    arguments {
        pass integer => seq([x0, x1, x2, x3, x4, x5, x6, x7]), fallback: stack(8, 8)
        pass float   => seq([v0, v1, v2, v3, v4, v5, v6, v7]), fallback: stack(8, 8)
        pass hfa     => consecutive([v0, v1, v2, v3, v4, v5, v6, v7]), fallback: stack(8, 8)
        pass by_ref  => integer
    }

    returns {
        sret {
            ptr: x8,
            consumes_slot: false
        }
        pass integer => seq([x0, x1, x2, x3, x4, x5, x6, x7])
        pass float   => seq([v0, v1, v2, v3])
        pass hfa     => consecutive([v0, v1, v2, v3])
    }
}
)";

    ParseContext ctx = createParseContextFromBuff("aarch64.ezcc", source);
    auto res = ctx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile,
                         DSL::Ast::CallingConvDef::CallingConventionDefFile>();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "aarch64_aapcs");

    // Link register
    ASSERT_TRUE(res->m_stack.m_linkRegister.has_value());
    EXPECT_EQ(res->m_stack.m_linkRegister->m_node, "x30");

    // HFA aggregate condition
    ASSERT_TRUE(res->m_classification.m_aggregate.has_value());
    const auto &conds = res->m_classification.m_aggregate->m_conditions;
    ASSERT_GE(conds.size(), 3);
    EXPECT_EQ(conds[0].m_kind, DSL::Ast::CallingConvDef::AggregateCondition::Kind::HomogeneousAggregate);
    ASSERT_TRUE(conds[0].m_homoBaseType.has_value());
    EXPECT_EQ(conds[0].m_homoBaseType->m_node, "float");
    ASSERT_TRUE(conds[0].m_homoMaxCount.has_value());
    EXPECT_EQ(conds[0].m_homoMaxCount->m_node, 4);

    // Consecutive register sequence
    EXPECT_EQ(res->m_arguments.m_rules.size(), 4);
    auto *hfaSeq = std::get_if<DSL::Ast::CallingConvDef::RegisterSequence>(&res->m_arguments.m_rules[2].m_source);
    ASSERT_NE(hfaSeq, nullptr);
    EXPECT_EQ(hfaSeq->m_kind, DSL::Ast::CallingConvDef::RegisterSequence::Kind::ConsecutiveBlock);
    EXPECT_EQ(hfaSeq->m_registers.size(), 8);

    // SRET does not consume argument slot
    ASSERT_TRUE(res->m_returns.m_sret.has_value());
    EXPECT_EQ(res->m_returns.m_sret->m_pointerRegister.m_node, "x8");
    EXPECT_FALSE(res->m_returns.m_sret->m_consumesArgSlot);
    EXPECT_FALSE(res->m_returns.m_sret->m_returnRegister.has_value());
}

// ============================================================================
// 4. x86 32-Bit stdcall Calling Convention
// ============================================================================

TEST_F(CallingConvDefLangTest, TestStdcallCallingConvParsing)
{
    std::string source = R"(
calling_convention x86_stdcall {
    stack {
        align: 4,
        growth: down,
        cleanup: callee,
        shadow_space: 0,
        sp: esp,
        fp: ebp
    }

    preserve callee: [ebx, esi, edi, ebp, esp]
    preserve caller: [eax, ecx, edx]

    classify {
        types [i1, i8, i16, i32, ptr] => integer
        types [f32, f64]              => float
        aggregate {
            default => memory
        }
    }

    arguments {
        pass integer => stack(4, 4)
        pass float   => stack(4, 4)
        pass memory  => stack(4, 4)
    }

    returns {
        sret {
            ptr: eax,
            consumes_slot: false,
            returns: eax
        }
        pass integer => seq([eax, edx])
    }
}
)";

    ParseContext ctx = createParseContextFromBuff("stdcall.ezcc", source);
    auto res = ctx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile,
                         DSL::Ast::CallingConvDef::CallingConventionDefFile>();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "x86_stdcall");
    EXPECT_EQ(res->m_stack.m_alignment.m_node, 4);
    EXPECT_EQ(res->m_stack.m_cleanup, DSL::Ast::CallingConvDef::StackCleanup::Callee);
}
