#include "CodeGenerators/CppInstructionSelectorGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/InstructionSelectSymbols.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>

namespace CodeGenerators
{

namespace
{

} // namespace

// Binds the generator to its diagnostics/symbols and normalizes an empty target name to "Target".
CppInstructionSelectorGenerator::CppInstructionSelectorGenerator(DiagnosticCollector *collector,
                                                                 SymbolTable *table,
                                                                 std::filesystem::path outPath,
                                                                 std::string targetName,
                                                                 std::string namespaceRoot) :
    CodeGenerator("CodeGenerators::InstructionSelector", collector, table, std::move(outPath)),
    m_targetName(SanitizeCppIdentifier(targetName, "Target")), m_namespaceRoot(std::move(namespaceRoot))
{
}

// Collects the parsed selection patterns produced by Sema, preserving declaration order.
std::vector<const Symbol *> CppInstructionSelectorGenerator::collectPatternSymbols() const
{
    if (!m_table)
    {
        return {};
    }

    return m_table->collect<Symbols::SelectionPatternSymbol>(SymbolType::SelectionPattern);
}

// Resolves the header/source destinations, emits both artifacts, and reports combined success.
bool CppInstructionSelectorGenerator::run()
{
    if (!beginGeneration())
    {
        return false;
    }

    std::string defaultBaseName = std::format("{}InstructionSelector", m_targetName);
    auto [headerPath, sourcePath] = resolveHeaderAndSourcePaths(defaultBaseName);

    CppSourceEmitter headerEmitter;
    CppSourceEmitter sourceEmitter;

    auto patternSymbols = collectPatternSymbols();
    emitHeader(headerEmitter, patternSymbols);
    emitSource(sourceEmitter, patternSymbols);

    return writeHeaderAndSource({ headerPath, sourcePath }, headerEmitter.view(), sourceEmitter.view());
}

// Emits the selector class declaration with one private select<Opcode> entry point per opcode.
void CppInstructionSelectorGenerator::emitHeader(CppSourceEmitter &emitter,
                                                 const std::vector<const Symbol *> &patternSymbols) const
{
    std::string guard = std::format("EZTARGETS_{}_INSTRUCTION_SELECTOR_H", StrToUpper(m_targetName));
    emitter.emitIncludeGuardStart(guard);
    emitter.emitBlankLine();
    emitter.emitBanner("CppInstructionSelectorGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude("InstructionSelector/MirInstructionSelector.h");
    emitter.emitInclude("Descriptors/TargetDesc.h");
    emitter.emitBlankLine();

    // Group unique generic root opcodes
    std::set<std::string> uniqueOpcodes;
    for (const auto *sym : patternSymbols)
    {
        const auto *data = sym->getIf<Symbols::SelectionPatternSymbol>();
        if (data && data->m_astNode)
        {
            uniqueOpcodes.insert(std::string(data->m_astNode->m_matchTree.m_opcode.m_node));
        }
    }

    {
        auto nsScope = emitter.enterNamespace(m_namespaceRoot);

        std::string className = std::format("{}InstructionSelector", m_targetName);
        {
            auto classScope = emitter.enterClass(className, "public MirInstructionSelector");
            emitter.emitLine("public:");
            emitter.indent();
            emitter.emitLine("explicit {}(TargetDesc *targetDesc);", className);
            emitter.emitLine("bool select(MirBuilderContext *ctx, MirInstruction *inst) override;");
            emitter.dedent();
            emitter.emitBlankLine();

            emitter.emitLine("private:");
            emitter.indent();
            for (const auto &opc : uniqueOpcodes)
            {
                emitter.emitLine("bool select{}(MirBuilderContext *ctx, MirInstruction *inst);", opc);
            }
            emitter.emitBlankLine();
            emitter.emitLine("TargetDesc *m_targetDesc{ nullptr };");
            emitter.dedent();
        }
    }

    emitter.emitBlankLine();
    emitter.emitIncludeGuardEnd(guard);
}

// Emits the selector dispatch, per-opcode matchers and the lowering bodies they perform.
void CppInstructionSelectorGenerator::emitSource(CppSourceEmitter &emitter,
                                                 const std::vector<const Symbol *> &patternSymbols) const
{
    emitter.emitBanner("CppInstructionSelectorGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude(std::format("{}InstructionSelector.h", m_targetName));
    emitter.emitInclude(std::format("{}TargetInstructionTable.h", m_targetName));
    emitter.emitInclude("Instruction/MirInstruction.h");
    emitter.emitInclude("Instruction/MirInstructionBuilder.h");
    emitter.emitInclude("Operand/MirOperands.h");
    emitter.emitInclude("Operand/MirOperandBuilder.h");
    emitter.emitInclude("Operand/MirRegisterClass.h");
    emitter.emitInclude("Operand/MirRegisterBank.h");
    emitter.emitInclude("Builder/MirBuilderContext.h");
    emitter.emitInclude("Type/MirTypeTable.h");
    emitter.emitInclude("Function/MirFunction.h");
    emitter.emitInclude("Function/MirFunctionRegisterInfo.h");
    emitter.emitInclude("FlexNumber/FlexInt.h");
    emitter.emitLine("#include <cstdint>");
    emitter.emitLine("#include <string_view>");
    emitter.emitBlankLine();

    {
        auto nsScope = emitter.enterNamespace(m_namespaceRoot);

        // Static helper predicates
        emitter.emitLine("namespace");
        {
            auto anonScope = emitter.enterBlock();
            emitter.emitLine("[[maybe_unused]] static bool isSimm32(int64_t val) { return val >= -2147483648LL && val "
                             "<= 2147483647LL; }");
            emitter.emitLine("[[maybe_unused]] static bool isSimm8(int64_t val) { return val >= -128 && val <= 127; }");
            emitter.emitLine("[[maybe_unused]] static bool isValidScale(int64_t val) { return val == 1 || val == 2 || "
                             "val == 4 || val == 8; }");
        }
        emitter.emitBlankLine();

        std::string className = std::format("{}InstructionSelector", m_targetName);

        // Constructor
        emitter.emitLine("{}::{}(TargetDesc *targetDesc) :", className, className);
        emitter.indent();
        emitter.emitLine("MirInstructionSelector(targetDesc), m_targetDesc(targetDesc)");
        emitter.dedent();
        emitter.emitLine("{}");
        emitter.emitBlankLine();

        // Group patterns by opcode, with higher-cost (more specific) patterns first
        std::map<std::string, std::vector<const DSL::Ast::InstructionSelectDef::SelectionPattern *>> opcodePatterns;
        for (const auto *sym : patternSymbols)
        {
            const auto *data = sym->getIf<Symbols::SelectionPatternSymbol>();
            if (data && data->m_astNode)
            {
                opcodePatterns[std::string(data->m_astNode->m_matchTree.m_opcode.m_node)].push_back(data->m_astNode);
            }
        }

        // Sort each group descending by cost / complexity
        for (auto &[opc, patterns] : opcodePatterns)
        {
            std::sort(patterns.begin(),
                      patterns.end(),
                      [](const auto *a, const auto *b)
                      {
                          bool aHasNested = false;
                          for (const auto &op : a->m_matchTree.m_operands)
                              if (op.m_kind == DSL::Ast::InstructionSelectDef::PatternOperand::Kind::NestedTree)
                                  aHasNested = true;
                          bool bHasNested = false;
                          for (const auto &op : b->m_matchTree.m_operands)
                              if (op.m_kind == DSL::Ast::InstructionSelectDef::PatternOperand::Kind::NestedTree)
                                  bHasNested = true;

                          if (aHasNested != bHasNested)
                              return aHasNested;
                          return a->m_cost > b->m_cost;
                      });
        }

        // select method
        emitter.emitLine("bool {}::select(MirBuilderContext *ctx, MirInstruction *inst)", className);
        {
            auto fnScope = emitter.enterBlock();
            emitter.emitLine("if (!ctx || !inst) return false;");
            emitter.emitBlankLine();
            emitter.emitLine("switch (inst->getOpCode())");
            {
                auto swScope = emitter.enterBlock();
                for (const auto &[opc, _] : opcodePatterns)
                {
                    emitter.emitLine("case MirInstructionOpCode::{}:", opc);
                    emitter.indent();
                    emitter.emitLine("return select{}(ctx, inst);", opc);
                    emitter.dedent();
                }
                emitter.emitLine("default:");
                emitter.indent();
                emitter.emitLine("return false;");
                emitter.dedent();
            }
            emitter.emitLine("return false;");
        }
        emitter.emitBlankLine();

        // Specific select<Opcode> methods
        for (const auto &[opc, patterns] : opcodePatterns)
        {
            emitter.emitLine("bool {}::select{}(MirBuilderContext *ctx, MirInstruction *inst)", className, opc);
            {
                auto fnScope = emitter.enterBlock();

                for (const auto *pat : patterns)
                {
                    emitter.emitLine("// Pattern: {}", pat->m_name.m_node);
                    {
                        auto patScope = emitter.enterBlock();

                        // Check target extension / feature when clauses
                        std::string extCheck;
                        for (const auto &when : pat->m_whenClauses)
                        {
                            if (when.m_predicate.m_node == "hasExtension" || when.m_predicate.m_node == "hasFeature")
                            {
                                if (!when.m_args.empty())
                                {
                                    if (!extCheck.empty())
                                    {
                                        extCheck += " && ";
                                    }
                                    extCheck += std::format("m_targetDesc && m_targetDesc->hasExtension(\"{}\")",
                                                            EscapeString(when.m_args[0].m_node, EscapeMode::CppStringLiteral));
                                }
                            }
                        }

                        std::optional<CppSourceEmitter::Scope> extScope;
                        if (!extCheck.empty())
                        {
                            extScope.emplace(emitter.enterBlock(std::format("if ({})", extCheck)));
                        }

                        // Check if any operand is a nested tree
                        int nestedOpIdx = -1;
                        for (size_t i = 0; i < pat->m_matchTree.m_operands.size(); ++i)
                        {
                            if (pat->m_matchTree.m_operands[i].m_kind ==
                                DSL::Ast::InstructionSelectDef::PatternOperand::Kind::NestedTree)
                            {
                                nestedOpIdx = static_cast<int>(i);
                                break;
                            }
                        }

                        // Nested-tree patterns fold a single-use defining instruction into this one.
                        if (nestedOpIdx >= 0)
                        {
                            const auto &nestedOp = pat->m_matchTree.m_operands[nestedOpIdx];
                            const auto &nestedTree = *nestedOp.m_nestedTree;

                            emitter.emitLine("if (inst->getOperandCount() >= {})", pat->m_matchTree.m_operands.size());
                            {
                                auto ifScope = emitter.enterBlock();
                                emitter.emitLine("auto *vregOp = inst->getOperand({})->get<MirRegister>();",
                                                 nestedOpIdx);
                                std::string vregCheck = "vregOp";
                                for (size_t opIdx = 0; opIdx < pat->m_matchTree.m_operands.size(); ++opIdx)
                                {
                                    if (static_cast<int>(opIdx) == nestedOpIdx)
                                        continue;
                                    const auto &op = pat->m_matchTree.m_operands[opIdx];
                                    if (op.m_kind == DSL::Ast::InstructionSelectDef::PatternOperand::Kind::SsaRegister)
                                    {
                                        vregCheck += std::format(" && inst->getOperand({})->isOfType<MirRegister>()", opIdx);
                                        if (op.m_type.has_value())
                                        {
                                            vregCheck += std::format(" && inst->getOperand({})->getMirType() && "
                                                                     "inst->getOperand({})->getMirType()->getName() == \"{}\"",
                                                                     opIdx,
                                                                     opIdx,
                                                                     EscapeString(op.m_type->m_node, EscapeMode::CppStringLiteral));
                                        }
                                    }
                                }
                                emitter.emitLine("if ({})", vregCheck);
                                {
                                    auto vregScope = emitter.enterBlock();
                                    emitter.emitLine("MirInstruction *defInst = getDefiningInstruction(ctx, vregOp);");
                                    std::string defCheck = std::format("defInst && defInst->getOpCode() == MirInstructionOpCode::{} "
                                                                       "&& hasOneUse(vregOp) && noInterveningStore(defInst, inst)",
                                                                       nestedTree.m_opcode.m_node);
                                    if (!nestedTree.m_operands.empty() && nestedTree.m_operands[0].m_type.has_value())
                                    {
                                        defCheck += std::format(" && defInst->getOperand(0)->getMirType() && "
                                                                "defInst->getOperand(0)->getMirType()->getName() == \"{}\"",
                                                                EscapeString(nestedTree.m_operands[0].m_type->m_node, EscapeMode::CppStringLiteral));
                                    }
                                    emitter.emitLine("if ({})", defCheck);
                                    {
                                        auto defScope = emitter.enterBlock();
                                        emitter.emitLine(
                                                "MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);");
                                        emitter.emitLine("MirOperandBuilder ob(ctx);");

                                        // Check target emit
                                        for (const auto &selInst : pat->m_selectClauses)
                                        {
                                            // Assign register classes
                                            for (const auto &op : selInst.m_operands)
                                            {
                                                if (op.m_kind ==
                                                            DSL::Ast::InstructionSelectDef::TargetEmitOperand::Kind::
                                                                    ClassBoundVar &&
                                                    op.m_regClass)
                                                {
                                                    // Find which operand in inst or defInst matches
                                                    if (nestedOpIdx != 0)
                                                    {
                                                        emitter.emitLine("if (auto *r = "
                                                                         "inst->getOperand(0)->get<MirRegister>()) "
                                                                         "r->setClass(findClass(\"{}\"));",
                                                                         op.m_regClass->m_node);
                                                    }
                                                }
                                            }

                                            // Build target emit instruction operands
                                            emitter.emitLine("std::vector<MirOperand *> emittedOps;");
                                            for (size_t opIdx = 0; opIdx < selInst.m_operands.size(); ++opIdx)
                                            {
                                                const auto &op = selInst.m_operands[opIdx];
                                                if (op.m_kind ==
                                                    DSL::Ast::InstructionSelectDef::TargetEmitOperand::Kind::
                                                            AddrModeMem)
                                                {
                                                    // Memory operand from defInst
                                                    emitter.emitLine("MirOperand *memTarget = defInst->getOperand(1);");
                                                    emitter.emitLine("if (auto *mem = memTarget->get<MirMemory>())");
                                                    emitter.emitLine("{");
                                                    emitter.indent();
                                                    emitter.emitLine("if (mem->getBase()) "
                                                                     "mem->getBase()->setClass(findClass(\"GPR64\"));");
                                                    emitter.emitLine("emittedOps.push_back(mem);");
                                                    emitter.dedent();
                                                    emitter.emitLine("}");
                                                    emitter.emitLine(
                                                            "else if (auto *reg = memTarget->get<MirRegister>())");
                                                    emitter.emitLine("{");
                                                    emitter.indent();
                                                    emitter.emitLine("reg->setClass(findClass(\"GPR64\"));");
                                                    emitter.emitLine(
                                                            "auto *disp0 = ob.buildInt(ctx->getTypeTable()->i32(), "
                                                            "FlexInt(static_cast<int32_t>(0)));");
                                                    emitter.emitLine("emittedOps.push_back(ob.buildMem(ctx->"
                                                                     "getTypeTable()->i32(), reg, disp0));");
                                                    emitter.dedent();
                                                    emitter.emitLine("}");
                                                    emitter.emitLine("else");
                                                    emitter.emitLine("{");
                                                    emitter.indent();
                                                    emitter.emitLine("emittedOps.push_back(memTarget);");
                                                    emitter.dedent();
                                                    emitter.emitLine("}");
                                                }
                                                else
                                                {
                                                    // Ssa register or other
                                                    if (opIdx == 0 && nestedOpIdx != 0)
                                                    {
                                                        emitter.emitLine("emittedOps.push_back(inst->getOperand(0));");
                                                    }
                                                    else if (opIdx == 1 && nestedOpIdx != 1)
                                                    {
                                                        emitter.emitLine("emittedOps.push_back(inst->getOperand(1));");
                                                    }
                                                    else
                                                    {
                                                        emitter.emitLine("emittedOps.push_back(inst->getOperand({}));",
                                                                         opIdx);
                                                    }
                                                }
                                            }

                                            std::string targetDescCall =
                                                    std::format("{}TargetInst::getTargetDesc({}TargetInst::{})",
                                                                m_targetName,
                                                                m_targetName,
                                                                selInst.m_targetOpcode.m_node);
                                            emitter.emitLine("ib.buildTarget({}, inst->getSourceRef(), emittedOps);",
                                                              targetDescCall);
                                        }

                                        emitter.emitLine("defInst->eraseFromOwner();");
                                        emitter.emitLine("inst->eraseFromOwner();");
                                        emitter.emitLine("return true;");
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Standard pattern without nested instructions
                            if (pat->m_matchTree.m_operands.empty())
                            {
                                emitter.emitLine("if (inst->getOperandCount() == 0)");
                            }
                            else
                            {
                                emitter.emitLine("if (inst->getOperandCount() >= {})",
                                                 pat->m_matchTree.m_operands.size());
                            }
                            {
                                auto ifScope = emitter.enterBlock();

                                // Check predicates / when clauses
                                bool hasImmWhen = false;
                                for (const auto &when : pat->m_whenClauses)
                                {
                                    if (when.m_predicate.m_node == "isSimm32" || when.m_predicate.m_node == "isSimm8")
                                    {
                                        hasImmWhen = true;
                                    }
                                }

                                if (hasImmWhen)
                                {
                                    // Match immediate
                                    std::string immCond;
                                    for (size_t opIdx = 0; opIdx < pat->m_matchTree.m_operands.size(); ++opIdx)
                                    {
                                        const auto &op = pat->m_matchTree.m_operands[opIdx];
                                        if (op.m_kind == DSL::Ast::InstructionSelectDef::PatternOperand::Kind::SsaRegister)
                                        {
                                            if (!immCond.empty())
                                                immCond += " && ";
                                            immCond += std::format("inst->getOperand({})->isOfType<MirRegister>()", opIdx);
                                            if (op.m_type.has_value())
                                            {
                                                immCond += std::format(" && inst->getOperand({})->getMirType() && "
                                                                       "inst->getOperand({})->getMirType()->getName() == \"{}\"",
                                                                       opIdx,
                                                                       opIdx,
                                                                       EscapeString(op.m_type->m_node, EscapeMode::CppStringLiteral));
                                            }
                                        }
                                        else if (op.m_kind == DSL::Ast::InstructionSelectDef::PatternOperand::Kind::ImmediateSymbol)
                                        {
                                            if (!immCond.empty())
                                                immCond += " && ";
                                            immCond += std::format("inst->getOperand({})->isOfType<MirInteger>()", opIdx);
                                            if (op.m_type.has_value())
                                            {
                                                immCond += std::format(" && inst->getOperand({})->getMirType() && "
                                                                       "inst->getOperand({})->getMirType()->getName() == \"{}\"",
                                                                       opIdx,
                                                                       opIdx,
                                                                       EscapeString(op.m_type->m_node, EscapeMode::CppStringLiteral));
                                            }
                                        }
                                    }

                                    size_t lastIdx = pat->m_matchTree.m_operands.size() - 1;
                                    emitter.emitLine("auto *imm = inst->getOperand({})->get<MirInteger>();", lastIdx);
                                    std::string predCheck = "imm && isSimm32(imm->getValue().getI64())";
                                    if (!immCond.empty())
                                    {
                                        predCheck = std::format("{} && {}", immCond, predCheck);
                                    }
                                    emitter.emitLine("if ({})", predCheck);
                                    {
                                        auto immScope = emitter.enterBlock();
                                        emitter.emitLine(
                                                "MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);");
                                        // Assign classes
                                        for (const auto &selInst : pat->m_selectClauses)
                                        {
                                            for (size_t sIdx = 0; sIdx < selInst.m_operands.size(); ++sIdx)
                                            {
                                                const auto &op = selInst.m_operands[sIdx];
                                                if (op.m_kind ==
                                                            DSL::Ast::InstructionSelectDef::TargetEmitOperand::Kind::
                                                                    ClassBoundVar &&
                                                    op.m_regClass)
                                                {
                                                    emitter.emitLine(
                                                            "if (auto *r = inst->getOperand({})->get<MirRegister>()) "
                                                            "r->setClass(findClass(\"{}\"));",
                                                            sIdx,
                                                            op.m_regClass->m_node);
                                                }
                                            }

                                            emitter.emitLine("std::vector<MirOperand *> emittedOps;");
                                            for (size_t sIdx = 0; sIdx < selInst.m_operands.size(); ++sIdx)
                                            {
                                                emitter.emitLine("emittedOps.push_back(inst->getOperand({}));", sIdx);
                                            }
                                            std::string targetDescCall =
                                                    std::format("{}TargetInst::getTargetDesc({}TargetInst::{})",
                                                                m_targetName,
                                                                m_targetName,
                                                                selInst.m_targetOpcode.m_node);
                                            emitter.emitLine("ib.buildTarget({}, inst->getSourceRef(), emittedOps);",
                                                              targetDescCall);
                                        }
                                        emitter.emitLine("inst->eraseFromOwner();");
                                        emitter.emitLine("return true;");
                                    }
                                }
                                else
                                {
                                    // Register-register pattern
                                    std::string regCond;
                                    for (size_t opIdx = 0; opIdx < pat->m_matchTree.m_operands.size(); ++opIdx)
                                    {
                                        const auto &op = pat->m_matchTree.m_operands[opIdx];
                                        if (op.m_kind ==
                                            DSL::Ast::InstructionSelectDef::PatternOperand::Kind::SsaRegister)
                                        {
                                            if (!regCond.empty())
                                                regCond += " && ";
                                            regCond +=
                                                    std::format("inst->getOperand({})->isOfType<MirRegister>()", opIdx);
                                            if (op.m_type.has_value())
                                            {
                                                regCond += std::format(" && inst->getOperand({})->getMirType() && "
                                                                       "inst->getOperand({})->getMirType()->getName() == \"{}\"",
                                                                       opIdx,
                                                                       opIdx,
                                                                       EscapeString(op.m_type->m_node, EscapeMode::CppStringLiteral));
                                            }
                                        }
                                    }

                                    auto emitPatternBody = [&]()
                                    {
                                        emitter.emitLine(
                                                "MirInstructionBuilder ib(ctx, inst, InsertionType::InsertBefore);");
                                        for (const auto &selInst : pat->m_selectClauses)
                                        {
                                            for (size_t sIdx = 0; sIdx < selInst.m_operands.size(); ++sIdx)
                                            {
                                                const auto &op = selInst.m_operands[sIdx];
                                                if (op.m_kind ==
                                                            DSL::Ast::InstructionSelectDef::TargetEmitOperand::Kind::
                                                                    ClassBoundVar &&
                                                    op.m_regClass)
                                                {
                                                    emitter.emitLine(
                                                            "if (auto *r = inst->getOperand({})->get<MirRegister>()) "
                                                            "r->setClass(findClass(\"{}\"));",
                                                            sIdx,
                                                            op.m_regClass->m_node);
                                                }
                                            }

                                            emitter.emitLine("std::vector<MirOperand *> emittedOps;");
                                            for (size_t sIdx = 0; sIdx < selInst.m_operands.size(); ++sIdx)
                                            {
                                                emitter.emitLine("emittedOps.push_back(inst->getOperand({}));", sIdx);
                                            }
                                            std::string targetDescCall =
                                                    std::format("{}TargetInst::getTargetDesc({}TargetInst::{})",
                                                                m_targetName,
                                                                m_targetName,
                                                                selInst.m_targetOpcode.m_node);
                                            emitter.emitLine("ib.buildTarget({}, inst->getSourceRef(), emittedOps);",
                                                             targetDescCall);
                                        }
                                        emitter.emitLine("inst->eraseFromOwner();");
                                        emitter.emitLine("return true;");
                                    };

                                    if (!regCond.empty())
                                    {
                                        emitter.emitLine("if ({})", regCond);
                                        {
                                            auto regScope = emitter.enterBlock();
                                            emitPatternBody();
                                        }
                                    }
                                    else
                                    {
                                        emitPatternBody();
                                    }
                                }
                            }
                        }
                    }
                    emitter.emitBlankLine();
                }

                emitter.emitLine("return false;");
            }
            emitter.emitBlankLine();
        }
    }
}

} // namespace CodeGenerators
