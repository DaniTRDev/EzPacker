#include "CodeGenerators/CppLegalizeRuleGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/LegalizeSymbols.h"
#include "Sema/Symbols/TypeSymbols.h"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

namespace CodeGenerators
{

namespace
{

} // namespace

// Binds the generator to its diagnostics/symbols and normalizes an empty target name to "Target".
CppLegalizeRuleGenerator::CppLegalizeRuleGenerator(DiagnosticCollector *collector,
                                                   SymbolTable *table,
                                                   std::filesystem::path outPath,
                                                   std::string targetName) :
    CodeGenerator("CodeGenerators::LegalizeRule", collector, table, std::move(outPath)),
    m_targetName(SanitizeCppIdentifier(targetName, "Target"))
{
}

// Resolves the header/source destinations, emits both artifacts, and reports combined success.
bool CppLegalizeRuleGenerator::run()
{
    if (!beginGeneration())
    {
        return false;
    }

    std::string defaultBaseName = std::format("{}LegalizerRules", m_targetName);
    auto [headerPath, sourcePath] = resolveHeaderAndSourcePaths(defaultBaseName);

    std::vector<const Symbol *> ruleSymbols =
            m_table->collect<Symbols::LegalizeRuleSymbol>(SymbolType::LegalizeRule);

    CppSourceEmitter headerEmitter;
    CppSourceEmitter sourceEmitter;

    emitHeader(headerEmitter, ruleSymbols);
    emitSource(sourceEmitter, ruleSymbols);

    return writeHeaderAndSource({ headerPath, sourcePath }, headerEmitter.view(), sourceEmitter.view());
}

// Emits the rule handler declarations and the two dispatcher entry points.
void CppLegalizeRuleGenerator::emitHeader(CppSourceEmitter &emitter,
                                          const std::vector<const Symbol *> &ruleSymbols) const
{
    std::string guard = std::format("EZTRIPLE_{}_LEGALIZER_RULES_H", StrToUpper(m_targetName));
    emitter.emitIncludeGuardStart(guard);

    emitter.emitBanner("CppLegalizeRuleGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude("Legalizer/Actions/LegalizeActionCommon.h", false);
    emitter.emitInclude("Instruction/MirInstructionSet.h", false);
    emitter.emitInclude("cstdint", true);
    emitter.emitBlankLine();

    {
        auto nsScope = emitter.enterNamespace(std::format("EzTriple::{}Rules", m_targetName));

        // Collect all parsed legalize rules in symbol-table order.
        std::vector<const Symbols::LegalizeRuleSymbol *> rules;
        for (const Symbol *ruleSym : ruleSymbols)
        {
            if (const auto *r = ruleSym->getIf<Symbols::LegalizeRuleSymbol>())
            {
                rules.push_back(r);
            }
        }

        if (!rules.empty())
        {
            emitter.emitSectionComment("Individual rewrite rule handler declarations");
            for (const auto *r : rules)
            {
                emitter.emitLine("LegalizationResult Rule_{}(LegalizeCtx &ctx);", r->m_ruleName);
            }
            emitter.emitBlankLine();
        }

        emitter.emitSectionComment("Rule Dispatchers");
        emitter.emitLine("/**");
        emitter.emitLine(" * Dispatches rules matching a given opcode.");
        emitter.emitLine(" * Returns Legalized if a rule matched and transformed the instruction,");
        emitter.emitLine(" * or NotModified if no rule applied.");
        emitter.emitLine(" */");
        emitter.emitLine("LegalizationResult applyRules(LegalizeCtx &ctx, MirInstructionOpCode opcode);");
        emitter.emitBlankLine();

        emitter.emitLine("/**");
        emitter.emitLine(" * Dispatches a specific rule by compiler-assigned rule ID.");
        emitter.emitLine(" */");
        emitter.emitLine("LegalizationResult applyRuleById(LegalizeCtx &ctx, uint16_t ruleId);");

        nsScope.close();
    }

    emitter.emitBlankLine();
    emitter.emitIncludeGuardEnd(guard);
}

// Emits each rule matcher/rewriter body plus the opcode- and id-based dispatchers.
void CppLegalizeRuleGenerator::emitSource(CppSourceEmitter &emitter,
                                          const std::vector<const Symbol *> &ruleSymbols) const
{
    emitter.emitBanner("CppLegalizeRuleGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude(std::format("{}LegalizerRules.h", m_targetName), false);
    emitter.emitInclude("Builder/MirBuilderContext.h", false);
    emitter.emitInclude("Instruction/MirInstruction.h", false);
    emitter.emitInclude("Instruction/MirInstructionBuilder.h", false);
    emitter.emitInclude("Operand/MirOperands.h", false);
    emitter.emitInclude("Operand/MirOperandBuilder.h", false);
    emitter.emitInclude("Type/MirType.h", false);
    emitter.emitInclude("Type/MirTypeTable.h", false);
    emitter.emitInclude("Block/MirBlock.h", false);
    emitter.emitBlankLine();

    // Collect all parsed legalize rules in symbol-table order.
    std::vector<const Symbols::LegalizeRuleSymbol *> rules;
    for (const Symbol *ruleSym : ruleSymbols)
    {
        if (const auto *r = ruleSym->getIf<Symbols::LegalizeRuleSymbol>())
        {
            rules.push_back(r);
        }
    }

    // Collect forward declarations for predicates & transforms
    std::map<std::string, size_t> predicates;
    std::map<std::string, size_t> transforms;

    for (const auto *r : rules)
    {
        for (const auto &p : r->m_predicates)
        {
            predicates.emplace(std::string(p.m_name), p.m_args.size());
        }

        for (const auto &e : r->m_expansionSequence)
        {
            for (const auto &op : e.m_operands)
            {
                if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform)
                {
                    transforms.emplace(std::string(op.m_name), op.m_callArgs.size());
                }
            }
        }
    }

    if (!predicates.empty() || !transforms.empty())
    {
        emitter.emitSectionComment("Forward declarations of user/target predicate guards & transforms");
        for (const auto &[name, arity] : predicates)
        {
            std::string params;
            for (size_t i = 0; i < arity; ++i)
            {
                if (i > 0)
                    params += ", ";
                params += std::format("int64_t arg{}", i);
            }
            emitter.emitLine("extern bool {}({});", name, params);
        }

        for (const auto &[name, arity] : transforms)
        {
            std::string params;
            for (size_t i = 0; i < arity; ++i)
            {
                if (i > 0)
                    params += ", ";
                params += std::format("int64_t arg{}", i);
            }
            emitter.emitLine("extern int64_t {}({});", name, params);
        }
        emitter.emitBlankLine();
    }

    auto nsScope = emitter.enterNamespace(std::format("EzTriple::{}Rules", m_targetName));

    // Emit individual rules
    for (const auto *r : rules)
    {
        emitter.emitSectionComment(std::format("Rule: {}", r->m_ruleName));
        emitter.emitLine("LegalizationResult Rule_{}(LegalizeCtx &ctx)", r->m_ruleName);
        emitter.emitLine("{");
        emitter.indent();

        if (r->m_matchPatterns.empty())
        {
            emitter.emitLine("return LegalizationResult::NotModified;");
            emitter.dedent();
            emitter.emitLine("}");
            emitter.emitBlankLine();
            continue;
        }

        const auto &matchInst = r->m_matchPatterns[0];

        emitter.emitLine("MirInstruction *inst = *ctx.m_it;");
        emitter.emitLine("if (!inst || inst->getOpCode() != MirInstructionOpCode::{})", matchInst.m_opcode);
        emitter.emitLine("{");
        emitter.indent();
        emitter.emitLine("return LegalizationResult::NotModified;");
        emitter.dedent();
        emitter.emitLine("}");
        emitter.emitBlankLine();

        emitter.emitLine("const auto &operands = inst->getOperands();");
        emitter.emitLine("if (operands.size() != {})", matchInst.m_operands.size());
        emitter.emitLine("{");
        emitter.indent();
        emitter.emitLine("return LegalizationResult::NotModified;");
        emitter.dedent();
        emitter.emitLine("}");
        emitter.emitBlankLine();

        // Match Operands & Types
        emitter.emitLine("// 1. Match Operands & Types");
        std::unordered_set<std::string> boundSsaVars;
        std::unordered_set<std::string> boundImmVars;
        std::string firstImmSymbolName;

        for (size_t i = 0; i < matchInst.m_operands.size(); ++i)
        {
            const auto &op = matchInst.m_operands[i];
            std::string varName = std::format("op{}", i);
            emitter.emitLine("MirOperand *{} = operands[{}];", varName, i);

            if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister)
            {
                boundSsaVars.insert(std::string(op.m_name));
                emitter.emitLine(
                        "if (!{} || {}->getType() != MirOperandType::Register) return LegalizationResult::NotModified;",
                        varName,
                        varName);

                if (op.m_typeOrClassId.has_value())
                {
                    if (const Symbol *tsSym = m_table->getSymById(*op.m_typeOrClassId))
                    {
                        if (const auto *ts = tsSym->getIf<Symbols::TypeSymbol>())
                        {
                            if (ts->m_bitWidth > 0)
                            {
                                emitter.emitLine("if ({}->getMirType()->getTotalSizeInBits() != {}) return "
                                                 "LegalizationResult::NotModified;",
                                                 varName,
                                                 ts->m_bitWidth);
                            }
                        }
                    }
                }
            }
            else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol)
            {
                boundImmVars.insert(std::string(op.m_name));
                if (firstImmSymbolName.empty())
                {
                    firstImmSymbolName = std::string(op.m_name);
                }
                emitter.emitLine(
                        "if (!{} || {}->getType() != MirOperandType::Integer) return LegalizationResult::NotModified;",
                        varName,
                        varName);

                if (op.m_typeOrClassId.has_value())
                {
                    if (const Symbol *tsSym = m_table->getSymById(*op.m_typeOrClassId))
                    {
                        if (const auto *ts = tsSym->getIf<Symbols::TypeSymbol>())
                        {
                            if (ts->m_bitWidth > 0)
                            {
                                emitter.emitLine("if ({}->getMirType()->getTotalSizeInBits() != {}) return "
                                                 "LegalizationResult::NotModified;",
                                                 varName,
                                                 ts->m_bitWidth);
                            }
                        }
                    }
                }

                emitter.emitLine("auto *imm_{} = static_cast<MirInteger *>({});", op.m_name, varName);
                emitter.emitLine("int64_t {} = imm_{}->getValue().getI64();", op.m_name, op.m_name);
            }
            else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral)
            {
                emitter.emitLine(
                        "if (!{} || {}->getType() != MirOperandType::Integer) return LegalizationResult::NotModified;",
                        varName,
                        varName);
                int64_t litVal = op.m_immLiteral.value_or(0);
                emitter.emitLine("if (static_cast<MirInteger *>({})->getValue().getI64() != {}) return "
                                 "LegalizationResult::NotModified;",
                                 varName,
                                 litVal);
            }
        }
        emitter.emitBlankLine();

        // 2. Evaluate 'when' predicates
        if (!r->m_predicates.empty())
        {
            emitter.emitLine("// 2. Evaluate 'when' predicates");
            for (const auto &p : r->m_predicates)
            {
                std::string callArgs;
                for (size_t a = 0; a < p.m_args.size(); ++a)
                {
                    if (a > 0)
                        callArgs += ", ";
                    if (std::holds_alternative<std::string_view>(p.m_args[a]))
                    {
                        callArgs += std::string(std::get<std::string_view>(p.m_args[a]));
                    }
                    else
                    {
                        callArgs += std::to_string(std::get<int64_t>(p.m_args[a]));
                    }
                }
                emitter.emitLine("if (!{}({})) return LegalizationResult::NotModified;", p.m_name, callArgs);
            }
            emitter.emitBlankLine();
        }

        // 3. Evaluate custom transforms
        std::map<std::pair<size_t, size_t>, std::string> transformResultVars;
        bool hasTransforms = false;
        for (size_t eIdx = 0; eIdx < r->m_expansionSequence.size(); ++eIdx)
        {
            const auto &e = r->m_expansionSequence[eIdx];
            for (size_t oIdx = 0; oIdx < e.m_operands.size(); ++oIdx)
            {
                const auto &op = e.m_operands[oIdx];
                if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform)
                {
                    hasTransforms = true;
                    break;
                }
            }
        }

        if (hasTransforms)
        {
            emitter.emitLine("// 3. Evaluate custom transforms");
            for (size_t eIdx = 0; eIdx < r->m_expansionSequence.size(); ++eIdx)
            {
                const auto &e = r->m_expansionSequence[eIdx];
                for (size_t oIdx = 0; oIdx < e.m_operands.size(); ++oIdx)
                {
                    const auto &op = e.m_operands[oIdx];
                    if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform)
                    {
                        std::string resVar = std::format("trans_{}_{}_{}", op.m_name, eIdx, oIdx);
                        transformResultVars[{ eIdx, oIdx }] = resVar;

                        std::string args;
                        for (size_t a = 0; a < op.m_callArgs.size(); ++a)
                        {
                            if (a > 0)
                                args += ", ";
                            args += std::string(op.m_callArgs[a]);
                        }
                        emitter.emitLine("int64_t {} = {}({});", resVar, op.m_name, args);
                    }
                }
            }
            emitter.emitBlankLine();
        }

        // 4. Emit replacement instructions
        emitter.emitLine("// 4. Emit replacement instructions");
        emitter.emitLine("MirBlock *block = inst->getOwner();");
        emitter.emitLine("MirInstructionBuilder builder(ctx.m_ctx, block, InsertionType::InsertBefore, ctx.m_it);");
        emitter.emitLine("MirOperandBuilder opBuilder(ctx.m_ctx);");
        emitter.emitBlankLine();

        std::unordered_set<std::string> definedSsaVars = boundSsaVars;

        for (size_t eIdx = 0; eIdx < r->m_expansionSequence.size(); ++eIdx)
        {
            const auto &e = r->m_expansionSequence[eIdx];
            std::vector<std::string> emitOperandVars;

            for (size_t oIdx = 0; oIdx < e.m_operands.size(); ++oIdx)
            {
                const auto &op = e.m_operands[oIdx];
                if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister)
                {
                    std::string varName = std::string(op.m_name);
                    if (definedSsaVars.find(varName) == definedSsaVars.end())
                    {
                        // New SSA register created in emit sequence
                        definedSsaVars.insert(varName);
                        std::string typeAccessor = "i32()";
                        if (op.m_typeOrClassId.has_value())
                        {
                            if (const Symbol *tsSym = m_table->getSymById(*op.m_typeOrClassId))
                            {
                                typeAccessor = std::format("{}()", tsSym->getName());
                            }
                        }
                        emitter.emitLine(
                                "MirOperand *op_{} = opBuilder.buildVReg(ctx.m_ctx->getTypeTable()->{}, \"{}\");",
                                varName,
                                typeAccessor,
                                varName);
                        emitOperandVars.push_back(std::format("op_{}", varName));
                    }
                    else
                    {
                        // Look up corresponding operand variable from match or earlier emit
                        // In match: find index
                        bool foundInMatch = false;
                        for (size_t mIdx = 0; mIdx < matchInst.m_operands.size(); ++mIdx)
                        {
                            if (matchInst.m_operands[mIdx].m_name == op.m_name)
                            {
                                emitOperandVars.push_back(std::format("op{}", mIdx));
                                foundInMatch = true;
                                break;
                            }
                        }
                        if (!foundInMatch)
                        {
                            emitOperandVars.push_back(std::format("op_{}", varName));
                        }
                    }
                }
                else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol)
                {
                    // Re-use matched operand
                    bool found = false;
                    for (size_t mIdx = 0; mIdx < matchInst.m_operands.size(); ++mIdx)
                    {
                        if (matchInst.m_operands[mIdx].m_name == op.m_name)
                        {
                            emitOperandVars.push_back(std::format("op{}", mIdx));
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        emitOperandVars.push_back(std::string(op.m_name));
                    }
                }
                else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform)
                {
                    std::string transVar = transformResultVars[{ eIdx, oIdx }];
                    std::string opName = std::format("transOp_{}_{}", eIdx, oIdx);
                    if (!firstImmSymbolName.empty())
                    {
                        emitter.emitLine("MirOperand *{} = opBuilder.buildInt(imm_{}->getMirType(), FlexInt({}, "
                                         "imm_{}->getMirType()->getTotalSizeInBits()));",
                                         opName,
                                         firstImmSymbolName,
                                         transVar,
                                         firstImmSymbolName);
                    }
                    else
                    {
                        emitter.emitLine("MirOperand *{} = opBuilder.buildInt(ctx.m_ctx->getTypeTable()->i32(), "
                                         "FlexInt({}, 32));",
                                         opName,
                                         transVar);
                    }
                    emitOperandVars.push_back(opName);
                }
                else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral)
                {
                    std::string opName = std::format("litOp_{}_{}", eIdx, oIdx);
                    int64_t val = op.m_immLiteral.value_or(0);
                    emitter.emitLine(
                            "MirOperand *{} = opBuilder.buildInt(ctx.m_ctx->getTypeTable()->i32(), FlexInt({}, 32));",
                            opName,
                            val);
                    emitOperandVars.push_back(opName);
                }
            }

            std::string opList;
            for (size_t v = 0; v < emitOperandVars.size(); ++v)
            {
                if (v > 0)
                    opList += ", ";
                opList += emitOperandVars[v];
            }

            emitter.emitLine("builder.build(MirInstructionOpCode::{}, inst->getSourceRef(), {{ {} }});",
                             e.m_opcode,
                             opList);
        }
        emitter.emitBlankLine();

        // 5. Erase original instruction
        emitter.emitLine("// 5. Erase original matched instruction");
        emitter.emitLine("inst->eraseFromOwner();");
        emitter.emitLine("return LegalizationResult::Legalized;");

        emitter.dedent();
        emitter.emitLine("}");
        emitter.emitBlankLine();
    }

    // Emit applyRules
    emitter.emitSectionComment("Rule Dispatcher by Opcode");
    emitter.emitLine("LegalizationResult applyRules(LegalizeCtx &ctx, MirInstructionOpCode opcode)");
    emitter.emitLine("{");
    emitter.indent();

    if (!rules.empty())
    {
        std::map<std::string, std::vector<std::string>> opcodeToRules;
        for (const auto *r : rules)
        {
            if (!r->m_matchPatterns.empty())
            {
                opcodeToRules[std::string(r->m_matchPatterns[0].m_opcode)].push_back(std::string(r->m_ruleName));
            }
        }

        emitter.emitLine("switch (opcode)");
        emitter.emitLine("{");
        emitter.indent();

        for (const auto &[opcode, ruleNames] : opcodeToRules)
        {
            emitter.emitLine("case MirInstructionOpCode::{}:", opcode);
            emitter.indent();
            for (const auto &rName : ruleNames)
            {
                emitter.emitLine("if (auto res = Rule_{}(ctx); res != LegalizationResult::NotModified)", rName);
                emitter.emitLine("    return res;");
            }
            emitter.emitLine("break;");
            emitter.dedent();
        }

        emitter.emitLine("default:");
        emitter.emitLine("    break;");

        emitter.dedent();
        emitter.emitLine("}");
    }
    else
    {
        emitter.emitLine("(void)ctx;");
        emitter.emitLine("(void)opcode;");
    }

    emitter.emitLine("return LegalizationResult::NotModified;");
    emitter.dedent();
    emitter.emitLine("}");
    emitter.emitBlankLine();

    // Emit applyRuleById
    emitter.emitSectionComment("Rule Dispatcher by Rule ID");
    emitter.emitLine("LegalizationResult applyRuleById(LegalizeCtx &ctx, uint16_t ruleId)");
    emitter.emitLine("{");
    emitter.indent();

    if (!rules.empty())
    {
        emitter.emitLine("switch (ruleId)");
        emitter.emitLine("{");
        emitter.indent();

        for (size_t i = 0; i < rules.size(); ++i)
        {
            emitter.emitLine("case {}: return Rule_{}(ctx);", i, rules[i]->m_ruleName);
        }

        emitter.emitLine("default: return LegalizationResult::Failed;");
        emitter.dedent();
        emitter.emitLine("}");
    }
    else
    {
        emitter.emitLine("(void)ctx;");
        emitter.emitLine("(void)ruleId;");
        emitter.emitLine("return LegalizationResult::Failed;");
    }

    emitter.dedent();
    emitter.emitLine("}");

    nsScope.close();
}

} // namespace CodeGenerators
