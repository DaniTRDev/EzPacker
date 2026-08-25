#ifndef EZDSL_TARGET_INST_PASS_H
#define EZDSL_TARGET_INST_PASS_H

#include "EzDslCommon.h"
#include "Ast/InstructionDefLangAst.h"
#include "Sema/Symbols/Symbols.h"

class InstructionDefPass
{
  public:
    static bool
    run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::InstDef::InstDefFile *file);

  private:
    static bool declareFormats(class DiagnosticCollector *collector,
                               class SymbolTable *table,
                               DSL::Ast::InstDef::InstDefFile *file);

    static bool validateFormat(class DiagnosticCollector *collector,
                               class SymbolTable *table,
                               const DSL::Ast::InstDef::InstFormatDecl &formatDecl,
                               SymbolId formatSymId);

    static bool declareInstructions(class DiagnosticCollector *collector,
                                    class SymbolTable *table,
                                    DSL::Ast::InstDef::InstDefFile *file);

    static bool validateInstruction(class DiagnosticCollector *collector,
                                    class SymbolTable *table,
                                    const DSL::Ast::InstDef::InstDecl &instDecl);

    static bool resolveOperands(class DiagnosticCollector *collector,
                                class SymbolTable *table,
                                const DSL::Ast::InstDef::InstDecl &instDecl,
                                std::pmr::vector<Sema::Symbols::TargetOperandSymbol> &outArgs,
                                std::pmr::vector<Sema::Symbols::TargetOperandSymbol> &outImplicitArgs);

    static bool validateAsmTemplate(DiagnosticCollector *collector,
                                    const DSL::Ast::InstDef::InstDecl &instDecl,
                                    const std::unordered_set<std::string_view> &validOperands);

    static bool validateFieldAssignments(class DiagnosticCollector *collector,
                                         class SymbolTable *table,
                                         const DSL::Ast::InstDef::InstDecl &instDecl,
                                         const Sema::Symbols::InstructionFormatSymbol &formatSym);

    static bool validateFieldAssignments(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         const DSL::Ast::InstDef::InstDecl &instDecl,
                                         const Sema::Symbols::InstructionFormatSymbol &formatSym,
                                         std::pmr::vector<Sema::Symbols::FieldAssignmentSymbol> &outAssignments);

    static bool aggregateAndValidateFlags(class DiagnosticCollector *collector,
                                          uint32_t &out,
                                          const DSL::Ast::InstDef::InstDecl &instDecl);
};

#endif // EZDSL_TARGET_INST_PASS_H