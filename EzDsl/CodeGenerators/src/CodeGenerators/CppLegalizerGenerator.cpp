#include "CodeGenerators/CppLegalizerGenerator.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>

namespace CodeGenerators
{

namespace
{

std::string ActionKindToCpp(DSL::Ast::LegalizeActionDef::LegalizeActionKind kind)
{
    switch (kind)
    {
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Legal:
            return "LegalizeActionKind::Legal";
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar:
            return "LegalizeActionKind::WidenScalar";
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar:
            return "LegalizeActionKind::NarrowScalar";
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Bitcast:
            return "LegalizeActionKind::Bitcast";
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Libcall:
            return "LegalizeActionKind::Libcall";
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Lower:
            return "LegalizeActionKind::Lower";
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom:
            return "LegalizeActionKind::Custom";
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Unsupported:
            return "LegalizeActionKind::Unsupported";
    }
    return "LegalizeActionKind::Unsupported";
}

std::string ToUpper(std::string_view s)
{
    std::string res(s);
    for (char &c : res)
    {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return res;
}

} // namespace

CppLegalizerGenerator::CppLegalizerGenerator(DiagnosticCollector *collector,
                                             SymbolTable *table,
                                             std::filesystem::path outPath,
                                             std::string targetName) :
    CodeGenerator("CodeGenerators::Legalizer", collector, table, std::move(outPath)),
    m_targetName(std::move(targetName))
{
    if (m_targetName.empty())
    {
        m_targetName = "Target";
    }
}

bool CppLegalizerGenerator::run()
{
    if (!validate())
    {
        return false;
    }

    std::string defaultBaseName = std::format("{}LegalizerActionTable", m_targetName);
    auto [headerPath, sourcePath] = resolveHeaderAndSourcePaths(defaultBaseName);

    CppSourceEmitter headerEmitter;
    CppSourceEmitter sourceEmitter;

    emitHeader(headerEmitter);
    emitSource(sourceEmitter);

    bool headerOk = writeOutput(headerPath, headerEmitter.str());
    bool sourceOk = writeOutput(sourcePath, sourceEmitter.str());

    return headerOk && sourceOk;
}

void CppLegalizerGenerator::emitHeader(CppSourceEmitter &emitter) const
{
    std::string guardName = std::format("EZTRIPLE_{}_LEGALIZER_ACTION_TABLE_H", ToUpper(m_targetName));
    emitter.emitIncludeGuardStart(guardName);
    emitter.emitBlankLine();
    emitter.emitBanner("CppLegalizerGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude("Legalizer/LegalizerInfo.h");
    emitter.emitBlankLine();

    std::string className = std::format("{}LegalizerInfo", m_targetName);
    {
        auto classScope = emitter.enterClass(className, "public LegalizerInfo");
        emitter.emitLine("public:");
        emitter.indent();
        emitter.emitLine("{}();", className);
        emitter.emitLine("~{}() override = default;", className);
        emitter.emitBlankLine();
        emitter.emitLine("LegalityResponse query(const LegalityQuery &q) const override;");
        emitter.emitLine("LegalizationResult executeCustom(LegalizeCtx &ctx, uint16_t handlerId) override;");
        emitter.dedent();
    }
    emitter.emitLine(";");
    emitter.emitBlankLine();
    emitter.emitIncludeGuardEnd(guardName);
}

void CppLegalizerGenerator::emitSource(CppSourceEmitter &emitter) const
{
    std::string defaultBaseName = std::format("{}LegalizerActionTable", m_targetName);
    emitter.emitBanner("CppLegalizerGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude(std::format("{}.h", defaultBaseName));
    emitter.emitInclude("Actions/LegalizeActionCommon.h");
    emitter.emitInclude("Actions/LegalizeCallAction.h");
    emitter.emitInclude("Actions/LegalizeReturnAction.h");
    emitter.emitInclude("Instruction/MirInstructionSet.h");
    emitter.emitInclude("Legalizer/LegalityQuery.h");
    emitter.emitInclude("array", true);
    emitter.emitInclude("cstdint", true);

    std::vector<const Symbols::LegalizeRuleSymbol *> rules;
    for (const Symbol *sym : m_table->getSymbols())
    {
        if (sym && sym->getType() == SymbolType::LegalizeRule)
        {
            if (const auto *r = sym->getIf<Symbols::LegalizeRuleSymbol>())
            {
                rules.push_back(r);
            }
        }
    }

    if (!rules.empty())
    {
        emitter.emitInclude(std::format("{}LegalizerRules.h", m_targetName));
    }
    emitter.emitBlankLine();

    // 1. Build Type Compact ID Map from SymbolTable
    struct TypeEntry
    {
        std::string name;
        uint8_t compactId{ 0 };
    };
    std::unordered_map<SymbolId, TypeEntry> typeMap;
    for (const Symbol *sym : m_table->getSymbols())
    {
        if (sym && sym->getType() == SymbolType::Type)
        {
            if (const auto *tdata = sym->getIf<Symbols::TypeSymbol>())
            {
                typeMap[sym->getId()] = TypeEntry{ std::string(sym->getName()), tdata->m_compactId };
            }
        }
    }

    auto getCompactId = [&](SymbolId tid) -> uint8_t {
        auto it = typeMap.find(tid);
        if (it != typeMap.end())
        {
            return it->second.compactId;
        }
        return 0;
    };

    // 2. Collect Actions, Libcalls, and Handlers
    std::vector<std::string> libcalls;
    auto getOrAddLibcall = [&](std::string_view sym) -> uint16_t {
        for (size_t i = 0; i < libcalls.size(); ++i)
        {
            if (libcalls[i] == sym) return static_cast<uint16_t>(i);
        }
        libcalls.emplace_back(sym);
        return static_cast<uint16_t>(libcalls.size() - 1);
    };

    std::vector<std::string> handlers;
    auto getOrAddHandler = [&](std::string_view h) -> uint16_t {
        for (size_t i = 0; i < handlers.size(); ++i)
        {
            if (handlers[i] == h) return static_cast<uint16_t>(i);
        }
        handlers.emplace_back(h);
        return static_cast<uint16_t>(handlers.size() - 1);
    };

    struct OpcodeActionInfo
    {
        std::string opcode;
        const Symbols::LegalizeActionSymbol *data{ nullptr };
        bool isHeterogeneous{ false };
        bool isWildcard{ false };
    };
    std::vector<OpcodeActionInfo> actionInfos;

    for (const Symbol *sym : m_table->getSymbols())
    {
        if (sym && sym->getType() == SymbolType::LegalizeAction)
        {
            const auto *data = sym->getIf<Symbols::LegalizeActionSymbol>();
            if (!data) continue;

            bool isHet = false;
            bool hasNonEmpty = false;
            for (const auto &clause : data->m_clauses)
            {
                if (!clause.m_types.empty())
                {
                    hasNonEmpty = true;
                }
                if (clause.m_types.size() > 1)
                {
                    isHet = true;
                }
                for (const auto &tc : clause.m_types)
                {
                    if (tc.m_operandIndex.has_value() && *tc.m_operandIndex > 0)
                    {
                        isHet = true;
                    }
                }
                if (clause.m_libcallSymbol.has_value())
                {
                    getOrAddLibcall(*clause.m_libcallSymbol);
                }
                if (clause.m_lowerHandler.has_value())
                {
                    getOrAddHandler(*clause.m_lowerHandler);
                }
            }

            bool isWild = !data->m_clauses.empty() && !hasNonEmpty;
            actionInfos.push_back({ std::string(sym->getName()), data, isHet, isWild });
        }
    }

    uint16_t ruleBaseId = static_cast<uint16_t>(handlers.size());
    auto getRuleIndex = [&](SymbolId ruleSymId) -> uint16_t {
        const Symbol *ruleSym = m_table->getSymById(ruleSymId);
        if (ruleSym)
        {
            for (size_t r = 0; r < rules.size(); ++r)
            {
                if (rules[r]->m_ruleName == ruleSym->getName())
                {
                    return static_cast<uint16_t>(r);
                }
            }
        }
        return 0;
    };

    // 3. Emit Forward Declarations for Custom Lowering Handlers
    if (!handlers.empty())
    {
        emitter.emitSectionComment("Target Lowering Handler Forward Declarations");
        for (const auto &h : handlers)
        {
            if (h.find("::") == std::string::npos)
            {
                emitter.emitLine("extern LegalizationResult {}(LegalizeCtx &ctx);", h);
            }
        }
        emitter.emitBlankLine();
    }

    // 4. Emit Libcall Pool
    emitter.emitSectionComment("Libcall String Pool");
    emitter.emitLine("static constexpr const char * const g_{}_Libcalls[] = {{", m_targetName);
    emitter.indent();
    for (const auto &lib : libcalls)
    {
        emitter.emitLine("\"{}\",", lib);
    }
    emitter.dedent();
    emitter.emitLine("};");
    emitter.emitBlankLine();

    // 5. Emit Heterogeneous Tier 2 Matchers
    emitter.emitSectionComment("Heterogeneous Signature Matchers (Tier 2)");
    std::vector<std::string> heterogeneousOpcodes;
    for (const auto &info : actionInfos)
    {
        if (!info.isHeterogeneous) continue;
        heterogeneousOpcodes.push_back(info.opcode);

        emitter.emitLine("static LegalityResponse match_{}(const LegalityQuery &q)", info.opcode);
        emitter.emitLine("{");
        emitter.indent();

        for (const auto &clause : info.data->m_clauses)
        {
            uint8_t targetCid = clause.m_targetTypeId.has_value() ? getCompactId(*clause.m_targetTypeId) : 0;
            uint16_t handlerOrStrId = 0;
            if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::Libcall && clause.m_libcallSymbol.has_value())
            {
                handlerOrStrId = getOrAddLibcall(*clause.m_libcallSymbol);
            }
            else if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::Lower &&
                     clause.m_lowerHandler.has_value())
            {
                handlerOrStrId = getOrAddHandler(*clause.m_lowerHandler);
            }
            else if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom)
            {
                if (clause.m_lowerHandler.has_value())
                {
                    handlerOrStrId = getOrAddHandler(*clause.m_lowerHandler);
                }
                else if (clause.m_customRules.has_value() && !clause.m_customRules->empty())
                {
                    handlerOrStrId = ruleBaseId + getRuleIndex(clause.m_customRules->front());
                }
            }

            uint8_t targetSlot = 0;
            if (!clause.m_types.empty() && clause.m_types[0].m_operandIndex.has_value())
            {
                targetSlot = static_cast<uint8_t>(*clause.m_types[0].m_operandIndex);
            }

            std::string cond;
            if (clause.m_types.empty())
            {
                cond = "true";
            }
            else
            {
                for (size_t i = 0; i < clause.m_types.size(); ++i)
                {
                    const auto &tc = clause.m_types[i];
                    uint8_t cid = getCompactId(tc.m_typeId);
                    uint8_t slot = tc.m_operandIndex.value_or(0);
                    if (i > 0) cond += " && ";
                    cond += std::format("q.m_compactIds[{}] == {}", slot, cid);
                }
            }

            emitter.emitLine("if ({})", cond);
            emitter.emitLine("{{");
            emitter.indent();
            emitter.emitLine("return LegalityResponse{{ {}, {}, {}, {} }};",
                             ActionKindToCpp(clause.m_kind),
                             targetSlot,
                             targetCid,
                             handlerOrStrId);
            emitter.dedent();
            emitter.emitLine("}}");
        }

        emitter.emitLine("return LegalityResponse{{ LegalizeActionKind::Unsupported, 0, 0, 0 }};");
        emitter.dedent();
        emitter.emitLine("}");
        emitter.emitBlankLine();
    }

    // 6. Emit Tier 3 Wildcard Array
    emitter.emitSectionComment("Tier 3 Wildcard Actions Table");
    emitter.emitLine("static constexpr auto g_{}_WildcardActions = []() {{", m_targetName);
    emitter.indent();
    emitter.emitLine("std::array<LegalityResponse, LegalizerInfo::OPCODE_COUNT> wildcards{{}};");
    for (const auto &info : actionInfos)
    {
        if (info.isWildcard && !info.data->m_clauses.empty())
        {
            const auto &clause = info.data->m_clauses[0];
            uint16_t handlerId = 0;
            if (clause.m_lowerHandler.has_value())
            {
                handlerId = getOrAddHandler(*clause.m_lowerHandler);
            }
            else if (clause.m_customRules.has_value() && !clause.m_customRules->empty())
            {
                handlerId = ruleBaseId + getRuleIndex(clause.m_customRules->front());
            }
            emitter.emitLine("wildcards[static_cast<size_t>(MirInstructionOpCode::{})] = LegalityResponse{{ {}, 0, 0, {} }};",
                             info.opcode,
                             ActionKindToCpp(clause.m_kind),
                             handlerId);
        }
    }
    emitter.emitLine("return wildcards;");
    emitter.dedent();
    emitter.emitLine("}}();");
    emitter.emitBlankLine();

    // 7. Emit Tier 1 Dense Primary Matrix
    emitter.emitSectionComment("Tier 1 Dense Primary Matrix (O(1))");
    emitter.emitLine("static constexpr auto g_{}_PrimaryMatrix = []() {{", m_targetName);
    emitter.indent();
    emitter.emitLine("std::array<std::array<LegalityResponse, LegalizerInfo::MAX_COMPACT_TYPES>, LegalizerInfo::OPCODE_COUNT> mat{{}};");

    for (const auto &info : actionInfos)
    {
        if (info.isHeterogeneous || info.isWildcard) continue;

        for (const auto &clause : info.data->m_clauses)
        {
            uint8_t targetCid = clause.m_targetTypeId.has_value() ? getCompactId(*clause.m_targetTypeId) : 0;
            uint16_t handlerOrStrId = 0;
            if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::Libcall && clause.m_libcallSymbol.has_value())
            {
                handlerOrStrId = getOrAddLibcall(*clause.m_libcallSymbol);
            }
            else if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::Lower &&
                     clause.m_lowerHandler.has_value())
            {
                handlerOrStrId = getOrAddHandler(*clause.m_lowerHandler);
            }
            else if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom)
            {
                if (clause.m_lowerHandler.has_value())
                {
                    handlerOrStrId = getOrAddHandler(*clause.m_lowerHandler);
                }
                else if (clause.m_customRules.has_value() && !clause.m_customRules->empty())
                {
                    handlerOrStrId = ruleBaseId + getRuleIndex(clause.m_customRules->front());
                }
            }

            for (const auto &tc : clause.m_types)
            {
                uint8_t cid = getCompactId(tc.m_typeId);
                uint8_t slot = tc.m_operandIndex.value_or(0);
                if (cid < 32)
                {
                    emitter.emitLine("mat[static_cast<size_t>(MirInstructionOpCode::{})][{}] = LegalityResponse{{ {}, {}, {}, {} }};",
                                     info.opcode,
                                     cid,
                                     ActionKindToCpp(clause.m_kind),
                                     slot,
                                     targetCid,
                                     handlerOrStrId);
                }
            }
        }
    }

    emitter.emitLine("return mat;");
    emitter.dedent();
    emitter.emitLine("}}();");
    emitter.emitBlankLine();

    // 8. Emit Class Constructor
    std::string className = std::format("{}LegalizerInfo", m_targetName);
    emitter.emitSectionComment("Constructor & Table Initialization");
    emitter.emitLine("{}::{}()", className, className);
    emitter.emitLine("{");
    emitter.indent();

    emitter.emitLine("for (size_t op = 0; op < OPCODE_COUNT; ++op)");
    emitter.emitLine("{");
    emitter.indent();
    emitter.emitLine("m_wildcardActions[op] = g_{}_WildcardActions[op];", m_targetName);
    emitter.emitLine("for (size_t t = 0; t < MAX_COMPACT_TYPES; ++t)");
    emitter.emitLine("{");
    emitter.indent();
    emitter.emitLine("m_primaryMatrix[op][t] = g_{}_PrimaryMatrix[op][t];", m_targetName);
    emitter.dedent();
    emitter.emitLine("}");
    emitter.dedent();
    emitter.emitLine("}");
    emitter.emitBlankLine();

    if (!libcalls.empty())
    {
        emitter.emitLine("for (const char *libcall : g_{}_Libcalls)", m_targetName);
        emitter.emitLine("{");
        emitter.indent();
        emitter.emitLine("registerLibcallSymbol(libcall);");
        emitter.dedent();
        emitter.emitLine("}");
        emitter.emitBlankLine();
    }

    for (const auto &op : heterogeneousOpcodes)
    {
        emitter.emitLine("m_ruleMatchers[MirInstructionOpCode::{}].push_back(&match_{});", op, op);
    }

    emitter.dedent();
    emitter.emitLine("}");
    emitter.emitBlankLine();

    // 9. Emit query(q)
    emitter.emitSectionComment("Query Dispatcher");
    emitter.emitLine("LegalityResponse {}::query(const LegalityQuery &q) const", className);
    emitter.emitLine("{");
    emitter.indent();
    emitter.emitLine("size_t opIdx = static_cast<size_t>(q.m_opcode);");
    emitter.emitLine("if (opIdx >= OPCODE_COUNT)");
    emitter.emitLine("{");
    emitter.indent();
    emitter.emitLine("return LegalityResponse{{ .m_action = LegalizeActionKind::Unsupported }};");
    emitter.dedent();
    emitter.emitLine("}");
    emitter.emitBlankLine();

    emitter.emitLine("// Tier 3: Wildcard Check");
    emitter.emitLine("if (g_{}_WildcardActions[opIdx].m_action != LegalizeActionKind::Unsupported)", m_targetName);
    emitter.emitLine("{");
    emitter.indent();
    emitter.emitLine("return g_{}_WildcardActions[opIdx];", m_targetName);
    emitter.dedent();
    emitter.emitLine("}");
    emitter.emitBlankLine();

    if (!heterogeneousOpcodes.empty())
    {
        emitter.emitLine("// Tier 2: Heterogeneous Rule Matchers");
        emitter.emitLine("switch (q.m_opcode)");
        emitter.emitLine("{");
        emitter.indent();
        for (const auto &op : heterogeneousOpcodes)
        {
            emitter.emitLine("case MirInstructionOpCode::{}:", op);
            emitter.emitLine("{");
            emitter.indent();
            emitter.emitLine("auto resp = match_{}(q);", op);
            emitter.emitLine("if (resp.m_action != LegalizeActionKind::Unsupported)");
            emitter.emitLine("    return resp;");
            emitter.emitLine("break;");
            emitter.dedent();
            emitter.emitLine("}");
        }
        emitter.emitLine("default: break;");
        emitter.dedent();
        emitter.emitLine("}");
        emitter.emitBlankLine();
    }

    emitter.emitLine("// Tier 1: Dense 2D Primary Matrix (O(1))");
    emitter.emitLine("if (q.m_operandCount > 0 && q.m_compactIds[0] < MAX_COMPACT_TYPES)");
    emitter.emitLine("{");
    emitter.indent();
    emitter.emitLine("LegalityResponse fastResp = g_{}_PrimaryMatrix[opIdx][q.m_compactIds[0]];", m_targetName);
    emitter.emitLine("if (fastResp.m_action != LegalizeActionKind::Unsupported)");
    emitter.emitLine("{");
    emitter.indent();
    emitter.emitLine("return fastResp;");
    emitter.dedent();
    emitter.emitLine("}");
    emitter.dedent();
    emitter.emitLine("}");
    emitter.emitBlankLine();

    emitter.emitLine("return LegalityResponse{{ .m_action = LegalizeActionKind::Unsupported }};");
    emitter.dedent();
    emitter.emitLine("}");
    emitter.emitBlankLine();

    // 10. Emit executeCustom
    emitter.emitSectionComment("Target Lowering Dispatcher");
    emitter.emitLine("LegalizationResult {}::executeCustom(LegalizeCtx &ctx, uint16_t handlerId)", className);
    emitter.emitLine("{");
    emitter.indent();
    if (!handlers.empty())
    {
        emitter.emitLine("switch (handlerId)");
        emitter.emitLine("{");
        emitter.indent();
        for (size_t i = 0; i < handlers.size(); ++i)
        {
            emitter.emitLine("case {}: return {}(ctx);", i, handlers[i]);
        }
        emitter.emitLine("default: break;");
        emitter.dedent();
        emitter.emitLine("}");
        emitter.emitBlankLine();
    }

    if (!rules.empty())
    {
        if (!handlers.empty())
        {
            emitter.emitLine("uint16_t ruleIndex = handlerId - {};", handlers.size());
            emitter.emitLine("return EzTriple::{}Rules::applyRuleById(ctx, ruleIndex);", m_targetName);
        }
        else
        {
            emitter.emitLine("return EzTriple::{}Rules::applyRuleById(ctx, handlerId);", m_targetName);
        }
    }
    else
    {
        if (handlers.empty())
        {
            emitter.emitLine("(void)ctx;");
            emitter.emitLine("(void)handlerId;");
        }
        emitter.emitLine("return LegalizationResult::Failed;");
    }
    emitter.dedent();
    emitter.emitLine("}");
}

bool GenerateLegalizerActionTable(DiagnosticCollector *collector,
                                  SymbolTable *table,
                                  std::filesystem::path outPath,
                                  std::string targetName)
{
    CppLegalizerGenerator generator(collector, table, std::move(outPath), std::move(targetName));
    return generator.run();
}

} // namespace CodeGenerators
