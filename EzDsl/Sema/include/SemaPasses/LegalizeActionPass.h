#ifndef EZDSLSEMA_LEGALIZE_ACTION_PASS_H
#define EZDSLSEMA_LEGALIZE_ACTION_PASS_H

#include "EzDslSemaCommon.h"
#include "Ast/LegalizeActionDefLangAst.h"

/**
 * Forward declarations.
 */
namespace Symbols
{
class LegalizeActionClauseSymbol;
class LegalizeActionConstraintSymbol;
} // namespace Symbols

/**
 * Semantic analysis pass validating target legalization actions (.lad).
 * Resolves opcode legality matrices, type constraints, target scalar promotion types (WIDENS/NARROWS),
 * and runtime libcall symbols against the symbol table.
 */
class LegalizeActionPass
{
  public:
    /**
     * Executes the semantic analysis and symbol resolution pass for legalization action definitions.
     * Returns true if all action rules, type constraints, and target types were successfully validated.
     */
    static bool run(class DiagnosticCollector *collector,
                    class SymbolTable *table,
                    DSL::Ast::LegalizeActionDef::LegalizeActionFile *file);

  private:
    /**
     * Processes and declares legalization action symbols for a single generic IR opcode.
     */
    static bool processInstructionDecl(class DiagnosticCollector *collector,
                                       SymbolTable *table,
                                       const DSL::Ast::LegalizeActionDef::LegalizeInstructionDecl &decl);

    /**
     * Validates a single action clause (LEGAL, WIDENS, NARROWS, LIBCALL) and resolves its target types.
     */
    static bool processClause(class DiagnosticCollector *collector,
                              class SymbolTable *table,
                              const DSL::Ast::LegalizeActionDef::LegalizeActionClause &clause,
                              std::string_view instName,
                              Symbols::LegalizeActionClauseSymbol &outClause,
                              size_t &maxOperandIndex);

    /**
     * Resolves a type constraint (e.g. i32, i8:1) against declared type symbols.
     */
    static bool resolveConstraint(class DiagnosticCollector *collector,
                                  class SymbolTable *table,
                                  const DSL::Ast::LegalizeActionDef::TypeConstraint &constraint,
                                  Symbols::LegalizeActionConstraintSymbol &outConstraint);
};

#endif // EZDSLSEMA_LEGALIZE_ACTION_PASS_H