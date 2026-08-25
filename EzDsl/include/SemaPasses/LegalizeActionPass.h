#ifndef EZDSL_LEGALIZE_ACTION_PASS_H
#define EZDSL_LEGALIZE_ACTION_PASS_H

#include "EzDslCommon.h"
#include "Ast/LegalizeActionDefLangAst.h"

namespace Sema::Symbols
{
class LegalizeClauseSymbol;
}

class LegalizeActionPass
{
  public:
    /**
     * Executes the semantic analysis and symbol resolution pass for legalization action definitions.
     */
    static bool run(class DiagnosticCollector *collector,
                    class SymbolTable *table,
                    DSL::Ast::LegalizeActionDef::TargetLegalizeDef *file);

  private:
    static bool processInstructionDecl(class DiagnosticCollector *collector,
                                       class SymbolTable *table,
                                       const DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl &decl);

    static bool processClause(class DiagnosticCollector *collector,
                              class SymbolTable *table,
                              const DSL::Ast::LegalizeActionDef::LegalizeActionClause &clause,
                              std::string_view instName,
                              Sema::Symbols::LegalizeClauseSymbol &outClause);

    static bool resolveConstraint(class DiagnosticCollector *collector,
                                  class SymbolTable *table,
                                  const DSL::Ast::LegalizeActionDef::TypeConstraint &constraint,
                                  Sema::Symbols::LegalizeConstraintSymbol &outConstraint);
};

#endif // EZDSL_LEGALIZE_ACTION_PASS_H