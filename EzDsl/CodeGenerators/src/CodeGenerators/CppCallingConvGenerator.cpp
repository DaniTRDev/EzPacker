#include "CodeGenerators/CppCallingConvGenerator.h"
#include "Ast/CallingConvDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/CallingConvSymbols.h"

#include <algorithm>
#include <cctype>
#include <format>

namespace CodeGenerators
{

namespace
{

} // namespace

// Binds the generator to its diagnostics/symbols and normalizes an empty target name to "Target".
CppCallingConvGenerator::CppCallingConvGenerator(DiagnosticCollector *collector,
                                                 SymbolTable *table,
                                                 std::filesystem::path outPath,
                                                 std::string targetName) :
    CodeGenerator("CodeGenerators::CallingConv", collector, table, std::move(outPath)),
    m_targetName(SanitizeCppIdentifier(targetName, "Target"))
{
}

// Requires at least one calling-convention symbol, then emits and writes the header/source pair.
bool CppCallingConvGenerator::run()
{
    if (!beginGeneration())
    {
        return false;
    }

    // Bail out early when the input declared no calling conventions at all.
    const auto convs = getSymbolTable()->collect<Symbols::CallingConvSymbol>(SymbolType::CallingConv);
    if (convs.empty())
    {
        error("No calling convention symbols found in symbol table.");
        return false;
    }

    std::string defaultBaseName = std::format("{}CallingConvDesc", m_targetName);
    auto [headerPath, sourcePath] = resolveHeaderAndSourcePaths(defaultBaseName);

    CppSourceEmitter headerEmitter;
    CppSourceEmitter sourceEmitter;

    emitHeader(headerEmitter, convs);
    emitSource(sourceEmitter, convs);

    return writeHeaderAndSource({ headerPath, sourcePath }, headerEmitter.view(), sourceEmitter.view());
}

// Emits one CallingConvDesc subclass declaration per parsed calling convention.
void CppCallingConvGenerator::emitHeader(CppSourceEmitter &emitter,
                                         const std::vector<const Symbol *> &convs) const
{
    std::string guardName = std::format("EZMIR_{}_CALLING_CONV_DESC_H", StrToUpper(m_targetName));
    emitter.emitIncludeGuardStart(guardName);
    emitter.emitBlankLine();
    emitter.emitBanner("CppCallingConvGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude("Function/CallingConvDesc.h");
    emitter.emitInclude("Operand/MirRegisterReference.h");
    emitter.emitBlankLine();

    // Forward declarations keep the generated header free of heavy Mir includes.
    emitter.emitLine("class MirBuilderContext;");
    emitter.emitLine("class MirRegisterClass;");
    emitter.emitLine("class MirFunction;");
    emitter.emitLine("class MirType;");
    emitter.emitLine("class CallLoweringState;");
    emitter.emitBlankLine();

    for (const Symbol *sym : convs)
    {
        const auto *ccData = sym->getIf<Symbols::CallingConvSymbol>();
        if (!ccData || !ccData->m_astNode)
            continue;

        const auto &file = *ccData->m_astNode;
        std::string className = std::format("{}CallingConvDesc", file.m_name.m_node);

        {
            auto classScope = emitter.enterClass(className, "public CallingConvDesc");
            // Public ABI surface: argument/return placement, stack policy and saved-register sets.
            emitter.emitLine("public:");
            emitter.indent();
            emitter.emitLine(
                    "explicit {}(class MirBuilderContext *ctx, class MirRegisterClass *defaultRegClass = nullptr);",
                    className);
            emitter.emitLine("~{}() override = default;", className);
            emitter.emitBlankLine();
            emitter.emitLine("ArgumentLocationDesc getArgLoc(class MirType *type, class CallLoweringState *callState) "
                             "override;");
            emitter.emitLine("ArgumentLocationDesc getReturnLoc(class MirType *type, class CallLoweringState "
                             "*callState) override;");
            emitter.emitLine("bool canReturnInRegs(class MirType *type) const override;");
            emitter.emitLine("bool isCalleeCleanup() const override;");
            emitter.emitLine("bool doesStackGrowsDownwards() const override;");
            emitter.emitLine("const char *getName() const override;");
            emitter.emitLine("MirRegisterRef getFramePointerReg() const override;");
            emitter.emitLine("MirRegisterRef getStackPointerReg() const override;");
            emitter.emitLine("size_t getStackAlignment() const override;");
            emitter.emitLine("size_t getShadowSpaceSize() const override;");
            emitter.emitLine("size_t getRedZoneSize() const override;");
            emitter.emitLine("std::optional<MirRegisterRef> getLinkRegister() const override;");
            emitter.emitLine("bool consumesSretSlot() const override;");
            emitter.emitLine("bool hasFramePointer(class MirFunction *func) const override;");
            emitter.emitLine(
                    "void classify(MirType *type, std::pmr::vector<CallingConvTypeClass> &out) const override;");
            emitter.emitLine("const std::pmr::vector<MirRegisterRef> &getAllCalleeSavedRegs() override;");
            emitter.emitLine("const std::pmr::vector<MirRegisterRef> &getCalleeSavedRegs(class MirRegisterClass "
                             "*_class) override;");
            emitter.emitLine("const std::pmr::vector<MirRegisterRef> &getAllCallerSavedRegs() override;");
            emitter.emitLine("const std::pmr::vector<MirRegisterRef> &getCallerSavedRegs(class MirRegisterClass "
                             "*_class) override;");
            emitter.dedent();
            emitter.emitBlankLine();
            // Private state: allocator context, default class and cached saved-register sets.
            emitter.emitLine("private:");
            emitter.indent();
            emitter.emitLine("class MirBuilderContext *m_ctx;");
            emitter.emitLine("class MirRegisterClass *m_defaultClass;");
            emitter.emitLine("std::pmr::vector<MirRegisterRef> m_allCalleeSaved;");
            emitter.emitLine("std::pmr::vector<MirRegisterRef> m_allCallerSaved;");
            emitter.emitLine("std::pmr::vector<MirRegisterRef> m_empty;");
            emitter.emitBlankLine();
            emitter.emitLine("size_t resolveRegId(std::string_view name, size_t fallback) const;");
            emitter.dedent();
        }
        emitter.emitBlankLine();
    }

    emitter.emitIncludeGuardEnd(guardName);
}

// Emits the out-of-line definitions for every CallingConvDesc declared in the header.
void CppCallingConvGenerator::emitSource(CppSourceEmitter &emitter,
                                         const std::vector<const Symbol *> &convs) const
{
    std::string defaultBaseName = std::format("{}CallingConvDesc", m_targetName);
    emitter.emitBanner("CppCallingConvGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude(std::format("{}.h", defaultBaseName));
    emitter.emitInclude("Function/CallLoweringState.h");
    emitter.emitInclude("Function/MirFunction.h");
    emitter.emitInclude("Function/MirFunctionStackFrame.h");
    emitter.emitInclude("Type/MirType.h");
    emitter.emitInclude("Builder/MirBuilderContext.h");
    emitter.emitInclude("Operand/MirRegisterClass.h");
    emitter.emitBlankLine();

    for (const Symbol *sym : convs)
    {
        const auto *ccData = sym->getIf<Symbols::CallingConvSymbol>();
        if (!ccData || !ccData->m_astNode)
            continue;

        const auto &file = *ccData->m_astNode;
        std::string className = std::format("{}CallingConvDesc", file.m_name.m_node);

        // Assigns a dense id to each saved register: caller-saved first, then callee-saved.
        auto findRegId = [&](std::string_view name) -> std::optional<size_t>
        {
            for (size_t i = 0; i < file.m_callerSaved.size(); ++i)
            {
                if (file.m_callerSaved[i].m_node == name)
                    return i;
            }
            for (size_t i = 0; i < file.m_calleeSaved.size(); ++i)
            {
                if (file.m_calleeSaved[i].m_node == name)
                    return i + file.m_callerSaved.size();
            }
            return std::nullopt;
        };
        emitter.emitBlankLine();

        // Resolves a register name through the default class, falling back to the dense id when absent.
        emitter.emitLine("size_t {}::resolveRegId(std::string_view name, size_t fallback) const", className);
        {
            auto body = emitter.enterBlock();
            emitter.emitLine("if (m_defaultClass)");
            {
                auto ifClass = emitter.enterBlock();
                emitter.emitLine("if (auto *desc = m_defaultClass->getReg(name)) return desc->m_id;");
            }
            emitter.emitLine("return fallback;");
        }
        emitter.emitBlankLine();

        // Constructor
        emitter.emitLine("{}::{}(MirBuilderContext *ctx, MirRegisterClass *defaultRegClass) :", className, className);
        emitter.indent();
        emitter.emitLine("m_ctx(ctx),");
        emitter.emitLine("m_defaultClass(defaultRegClass),");
        emitter.emitLine("m_allCalleeSaved(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),");
        emitter.emitLine("m_allCallerSaved(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),");
        emitter.emitLine("m_empty(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource())");
        emitter.dedent();
        {
            auto ctorBody = emitter.enterBlock();
            for (size_t i = 0; i < file.m_calleeSaved.size(); ++i)
            {
                emitter.emitLine(
                        "m_allCalleeSaved.push_back(MirRegisterRef(m_defaultClass, resolveRegId(\"{}\", {}))); // {}",
                        file.m_calleeSaved[i].m_node,
                        i + file.m_callerSaved.size(),
                        file.m_calleeSaved[i].m_node);
            }
            for (size_t i = 0; i < file.m_callerSaved.size(); ++i)
            {
                emitter.emitLine(
                        "m_allCallerSaved.push_back(MirRegisterRef(m_defaultClass, resolveRegId(\"{}\", {}))); // {}",
                        file.m_callerSaved[i].m_node,
                        i,
                        file.m_callerSaved[i].m_node);
            }
        }
        emitter.emitBlankLine();

        // Stack & ABI methods
        bool isCallee = (file.m_stack.m_cleanup == DSL::Ast::CallingConvDef::StackCleanup::Callee);
        emitter.emitLine("bool {}::isCalleeCleanup() const {{ return {}; }}", className, isCallee ? "true" : "false");

        bool growthDown = (file.m_stack.m_growth == DSL::Ast::CallingConvDef::StackGrowth::Down);
        emitter.emitLine("bool {}::doesStackGrowsDownwards() const {{ return {}; }}",
                         className,
                         growthDown ? "true" : "false");

        emitter.emitLine("const char *{}::getName() const {{ return \"{}\"; }}", className, file.m_name.m_node);
        emitter.emitLine("size_t {}::getStackAlignment() const {{ return {}; }}",
                         className,
                         file.m_stack.m_alignment.m_node);
        emitter.emitLine("size_t {}::getShadowSpaceSize() const {{ return {}; }}",
                         className,
                         file.m_stack.m_shadowSpace.m_node);

        int64_t redZone = file.m_stack.m_redZone.has_value() ? file.m_stack.m_redZone->m_node : 0;
        emitter.emitLine("size_t {}::getRedZoneSize() const {{ return {}; }}", className, redZone);

        bool consumesSret = file.m_returns.m_sret.has_value() && file.m_returns.m_sret->m_consumesArgSlot;
        emitter.emitLine("bool {}::consumesSretSlot() const {{ return {}; }}",
                         className,
                         consumesSret ? "true" : "false");

        if (file.m_stack.m_linkRegister.has_value())
        {
            size_t lrId = findRegId(file.m_stack.m_linkRegister->m_node).value_or(0);
            emitter.emitLine("std::optional<MirRegisterRef> {}::getLinkRegister() const {{ return "
                             "MirRegisterRef(m_defaultClass, resolveRegId(\"{}\", {})); }}",
                             className,
                             file.m_stack.m_linkRegister->m_node,
                             lrId);
        }
        else
        {
            emitter.emitLine("std::optional<MirRegisterRef> {}::getLinkRegister() const {{ return std::nullopt; }}",
                             className);
        }

        size_t fpId = findRegId(file.m_stack.m_framePointer.m_node).value_or(0);
        size_t spId = findRegId(file.m_stack.m_stackPointer.m_node).value_or(0);
        emitter.emitLine("MirRegisterRef {}::getFramePointerReg() const {{ return MirRegisterRef(m_defaultClass, "
                         "resolveRegId(\"{}\", {})); }}",
                         className,
                         file.m_stack.m_framePointer.m_node,
                         fpId);
        emitter.emitLine("MirRegisterRef {}::getStackPointerReg() const {{ return MirRegisterRef(m_defaultClass, "
                         "resolveRegId(\"{}\", {})); }}",
                         className,
                         file.m_stack.m_stackPointer.m_node,
                         spId);

        emitter.emitLine("bool {}::hasFramePointer(MirFunction *func) const {{ return func && func->getStackFrame() && "
                         "func->getStackFrame()->getAllocatedObjectCount() > 0; }}",
                         className);
        emitter.emitBlankLine();

        // canReturnInRegs
        emitter.emitLine("bool {}::canReturnInRegs(MirType *type) const", className);
        {
            auto body = emitter.enterBlock();
            emitter.emitLine("if (!type) return true;");
            if (file.m_classification.m_aggregate.has_value())
            {
                for (const auto &cond : file.m_classification.m_aggregate->m_conditions)
                {
                    if (cond.m_resultClass.m_node == "memory" || cond.m_resultClass.m_node == "by_ref")
                    {
                        if (cond.m_kind == DSL::Ast::CallingConvDef::AggregateCondition::Kind::SizeGreaterThan &&
                            cond.m_sizeLimit.has_value())
                        {
                            emitter.emitLine("if (type->getTotalSizeInBits() > {}) return false;",
                                             cond.m_sizeLimit->m_node * 8);
                        }
                    }
                }
            }
            emitter.emitLine("return true;");
        }
        emitter.emitBlankLine();

        // classify
        emitter.emitLine("void {}::classify(MirType *type, std::pmr::vector<CallingConvTypeClass> &out) const",
                         className);
        {
            auto body = emitter.enterBlock();
            emitter.emitLine("if (!type) {{ out.push_back(CallingConvTypeClass::Integer); return; }}");
            emitter.emitLine("if (type->getKind() == MirTypeKind::FloatingPoint) {{ "
                             "out.push_back(CallingConvTypeClass::Float); return; }}");
            emitter.emitLine("if (type->getKind() == MirTypeKind::Pointer || type->getKind() == MirTypeKind::Integer) "
                             "{{ out.push_back(CallingConvTypeClass::Integer); return; }}");
            emitter.emitLine("if (!canReturnInRegs(type)) {{ out.push_back(CallingConvTypeClass::Memory); return; }}");
            emitter.emitLine("out.push_back(CallingConvTypeClass::Integer);");
        }
        emitter.emitBlankLine();

        // getArgLoc
        emitter.emitLine("ArgumentLocationDesc {}::getArgLoc(MirType *type, CallLoweringState *callState)", className);
        {
            auto body = emitter.enterBlock();
            emitter.emitLine("size_t sizeInBytes = (type ? type->getTotalSizeInBits() + 7 : 64) / 8;");
            emitter.emitLine("if (callState)");
            {
                auto ifState = emitter.enterBlock();
                if (!file.m_arguments.m_unifiedSlots.empty())
                {
                    // Positional slot model: each argument index maps to a fixed register or stack slot.
                    size_t numSlots = file.m_arguments.m_unifiedSlots.size();
                    emitter.emitLine("size_t slot = callState->getSlotIndex();");
                    emitter.emitLine("callState->advanceSlot();");
                    emitter.emitLine("callState->advanceArg();");
                    emitter.emitLine("switch (slot)");
                    {
                        auto switchSlot = emitter.enterBlock();
                        for (size_t s = 0; s < numSlots; ++s)
                        {
                            const auto &slot = file.m_arguments.m_unifiedSlots[s];
                            emitter.emitLine("case {}:", s);
                            emitter.indent();
                            const DSL::Ast::CallingConvDef::SlotBinding *floatBinding = nullptr;
                            const DSL::Ast::CallingConvDef::SlotBinding *intBinding = nullptr;
                            for (const auto &b : slot.m_bindings)
                            {
                                if (b.m_abiClass.m_node == "sse" || b.m_abiClass.m_node == "float")
                                    floatBinding = &b;
                                else if (b.m_abiClass.m_node == "integer")
                                    intBinding = &b;
                            }
                            if (floatBinding && intBinding)
                            {
                                emitter.emitLine("if (type && type->getKind() == MirTypeKind::FloatingPoint) return "
                                                 "ArgumentLocationDesc::Reg(MirRegisterRef(m_defaultClass, "
                                                 "resolveRegId(\"{}\", {})), sizeInBytes);",
                                                 floatBinding->m_register.m_node,
                                                 findRegId(floatBinding->m_register.m_node).value_or(0));
                                emitter.emitLine("return ArgumentLocationDesc::Reg(MirRegisterRef(m_defaultClass, "
                                                 "resolveRegId(\"{}\", {})), sizeInBytes);",
                                                 intBinding->m_register.m_node,
                                                 findRegId(intBinding->m_register.m_node).value_or(0));
                            }
                            else if (floatBinding)
                            {
                                emitter.emitLine("return ArgumentLocationDesc::Reg(MirRegisterRef(m_defaultClass, "
                                                 "resolveRegId(\"{}\", {})), sizeInBytes);",
                                                 floatBinding->m_register.m_node,
                                                 findRegId(floatBinding->m_register.m_node).value_or(0));
                            }
                            else if (intBinding)
                            {
                                emitter.emitLine("return ArgumentLocationDesc::Reg(MirRegisterRef(m_defaultClass, "
                                                 "resolveRegId(\"{}\", {})), sizeInBytes);",
                                                 intBinding->m_register.m_node,
                                                 findRegId(intBinding->m_register.m_node).value_or(0));
                            }
                            else if (!slot.m_bindings.empty())
                            {
                                emitter.emitLine("return ArgumentLocationDesc::Reg(MirRegisterRef(m_defaultClass, "
                                                 "resolveRegId(\"{}\", {})), sizeInBytes);",
                                                 slot.m_bindings[0].m_register.m_node,
                                                 findRegId(slot.m_bindings[0].m_register.m_node).value_or(0));
                            }
                            else
                            {
                                emitter.emitLine("break;");
                            }
                            emitter.dedent();
                        }
                    }
                    emitter.emitLine(
                            "return ArgumentLocationDesc::Stack(sizeInBytes, callState->allocateStack(type));");
                }
                else if (!file.m_arguments.m_rules.empty())
                {
                    // Rule model: walk the ABI-class register sequences in order, spilling once exhausted.
                    emitter.emitLine("callState->advanceArg();");
                    const DSL::Ast::CallingConvDef::PassRule *floatRule = nullptr;
                    const DSL::Ast::CallingConvDef::PassRule *intRule = nullptr;
                    for (const auto &rule : file.m_arguments.m_rules)
                    {
                        if (rule.m_abiClass.m_node == "sse" || rule.m_abiClass.m_node == "float")
                            floatRule = &rule;
                        else if (rule.m_abiClass.m_node == "integer")
                            intRule = &rule;
                    }

                    if (floatRule &&
                        std::holds_alternative<DSL::Ast::CallingConvDef::RegisterSequence>(floatRule->m_source))
                    {
                        const auto &seq = std::get<DSL::Ast::CallingConvDef::RegisterSequence>(floatRule->m_source);
                        emitter.emitLine("if (type && type->getKind() == MirTypeKind::FloatingPoint)");
                        {
                            auto floatBlock = emitter.enterBlock();
                            emitter.emitLine("size_t cursor = callState->getBankCursor(\"{}\");",
                                             floatRule->m_abiClass.m_node);
                            emitter.emitLine("callState->advanceBankCursor(\"{}\");", floatRule->m_abiClass.m_node);
                            std::string sseRegsStr;
                            for (size_t r = 0; r < seq.m_registers.size(); ++r)
                            {
                                if (r > 0)
                                    sseRegsStr += ", ";
                                sseRegsStr += std::format("MirRegisterRef(m_defaultClass, resolveRegId(\"{}\", {}))",
                                                          seq.m_registers[r].m_node,
                                                          findRegId(seq.m_registers[r].m_node).value_or(0));
                            }
                            emitter.emitLine("const MirRegisterRef s_sseRegs[] = {{ {} }};", sseRegsStr);
                            emitter.emitLine("if (cursor < sizeof(s_sseRegs)/sizeof(s_sseRegs[0]))");
                            {
                                auto ifSse = emitter.enterBlock();
                                emitter.emitLine("return ArgumentLocationDesc::Reg(s_sseRegs[cursor], sizeInBytes);");
                            }
                            emitter.emitLine(
                                    "return ArgumentLocationDesc::Stack(sizeInBytes, callState->allocateStack(type));");
                        }
                    }

                    if (intRule &&
                        std::holds_alternative<DSL::Ast::CallingConvDef::RegisterSequence>(intRule->m_source))
                    {
                        const auto &seq = std::get<DSL::Ast::CallingConvDef::RegisterSequence>(intRule->m_source);
                        emitter.emitLine("if (!type || type->getKind() == MirTypeKind::Integer || type->getKind() == "
                                         "MirTypeKind::Pointer)");
                        {
                            auto intBlock = emitter.enterBlock();
                            emitter.emitLine("size_t cursor = callState->getBankCursor(\"{}\");",
                                             intRule->m_abiClass.m_node);
                            emitter.emitLine("callState->advanceBankCursor(\"{}\");", intRule->m_abiClass.m_node);
                            std::string intRegsStr;
                            for (size_t r = 0; r < seq.m_registers.size(); ++r)
                            {
                                if (r > 0)
                                    intRegsStr += ", ";
                                intRegsStr += std::format("MirRegisterRef(m_defaultClass, resolveRegId(\"{}\", {}))",
                                                          seq.m_registers[r].m_node,
                                                          findRegId(seq.m_registers[r].m_node).value_or(0));
                            }
                            emitter.emitLine("const MirRegisterRef s_intRegs[] = {{ {} }};", intRegsStr);
                            emitter.emitLine("if (cursor < sizeof(s_intRegs)/sizeof(s_intRegs[0]))");
                            {
                                auto ifInt = emitter.enterBlock();
                                emitter.emitLine("return ArgumentLocationDesc::Reg(s_intRegs[cursor], sizeInBytes);");
                            }
                            emitter.emitLine(
                                    "return ArgumentLocationDesc::Stack(sizeInBytes, callState->allocateStack(type));");
                        }
                    }

                    emitter.emitLine("MirRegisterRef reg;");
                    emitter.emitLine("if (callState->allocate(m_defaultClass, reg))");
                    {
                        auto ifAlloc = emitter.enterBlock();
                        emitter.emitLine("return ArgumentLocationDesc::Reg(reg, sizeInBytes);");
                    }
                    emitter.emitLine(
                            "return ArgumentLocationDesc::Stack(sizeInBytes, callState->allocateStack(type));");
                }
                else
                {
                    // No explicit placement rules: hand the whole decision to the lowering state.
                    emitter.emitLine("callState->advanceArg();");
                    emitter.emitLine("MirRegisterRef reg;");
                    emitter.emitLine("if (callState->allocate(m_defaultClass, reg))");
                    {
                        auto ifAlloc = emitter.enterBlock();
                        emitter.emitLine("return ArgumentLocationDesc::Reg(reg, sizeInBytes);");
                    }
                    emitter.emitLine(
                            "return ArgumentLocationDesc::Stack(sizeInBytes, callState->allocateStack(type));");
                }
            }
            emitter.emitLine("return ArgumentLocationDesc::Stack(sizeInBytes, nullptr);");
        }
        emitter.emitBlankLine();

        // getReturnLoc
        emitter.emitLine("ArgumentLocationDesc {}::getReturnLoc(MirType *type, CallLoweringState *callState)",
                         className);
        {
            auto body = emitter.enterBlock();
            emitter.emitLine("size_t sizeInBytes = (type ? type->getTotalSizeInBits() + 7 : 64) / 8;");
            size_t sretRegId = 0;
            std::string sretRegName;
            if (file.m_returns.m_sret.has_value())
            {
                sretRegName = file.m_returns.m_sret->m_pointerRegister.m_node;
                sretRegId = findRegId(sretRegName).value_or(0);
            }
            // Returns that cannot fit in registers use an indirect sret pointer.
            emitter.emitLine("if (!canReturnInRegs(type))");
            {
                auto ifSret = emitter.enterBlock();
                emitter.emitLine("return ArgumentLocationDesc::Indirect(false, true, sizeInBytes, "
                                 "MirRegisterRef(m_defaultClass, resolveRegId(\"{}\", {})));",
                                 sretRegName,
                                 sretRegId);
            }

            size_t intRetRegId = 0;
            size_t floatRetRegId = 0;
            std::string intRetName;
            std::string floatRetName;
            for (const auto &rule : file.m_returns.m_rules)
            {
                if (std::holds_alternative<DSL::Ast::CallingConvDef::RegisterSequence>(rule.m_source))
                {
                    const auto &seq = std::get<DSL::Ast::CallingConvDef::RegisterSequence>(rule.m_source);
                    if (!seq.m_registers.empty())
                    {
                        if (rule.m_abiClass.m_node == "sse" || rule.m_abiClass.m_node == "float")
                        {
                            floatRetName = seq.m_registers.front().m_node;
                            floatRetRegId = findRegId(floatRetName).value_or(0);
                        }
                        else if (rule.m_abiClass.m_node == "integer")
                        {
                            intRetName = seq.m_registers.front().m_node;
                            intRetRegId = findRegId(intRetName).value_or(0);
                        }
                    }
                }
            }

            // Prefer the floating-point return register when one is declared and distinct.
            if (floatRetRegId != intRetRegId || !floatRetName.empty())
            {
                emitter.emitLine("if (type && type->getKind() == MirTypeKind::FloatingPoint) return "
                                 "ArgumentLocationDesc::Reg(MirRegisterRef(m_defaultClass, resolveRegId(\"{}\", {})), "
                                 "sizeInBytes);",
                                 floatRetName,
                                 floatRetRegId);
            }
            emitter.emitLine("return ArgumentLocationDesc::Reg(MirRegisterRef(m_defaultClass, resolveRegId(\"{}\", "
                             "{})), sizeInBytes);",
                             intRetName,
                             intRetRegId);
        }
        emitter.emitBlankLine();

        // Preserved register getters
        emitter.emitLine(
                "const std::pmr::vector<MirRegisterRef> &{}::getAllCalleeSavedRegs() {{ return m_allCalleeSaved; }}",
                className);
        emitter.emitLine("const std::pmr::vector<MirRegisterRef> &{}::getCalleeSavedRegs(MirRegisterClass *) {{ return "
                         "m_allCalleeSaved; }}",
                         className);
        emitter.emitLine(
                "const std::pmr::vector<MirRegisterRef> &{}::getAllCallerSavedRegs() {{ return m_allCallerSaved; }}",
                className);
        emitter.emitLine("const std::pmr::vector<MirRegisterRef> &{}::getCallerSavedRegs(MirRegisterClass *) {{ return "
                         "m_allCallerSaved; }}",
                         className);
        emitter.emitBlankLine();
    }
}

} // namespace CodeGenerators
