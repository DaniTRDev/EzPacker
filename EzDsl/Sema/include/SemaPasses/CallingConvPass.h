#ifndef EZDSL_CALLING_CONV_PASS_H
#define EZDSL_CALLING_CONV_PASS_H

#include "Ast/CallingConvDefLangAst.h"
#include "EzDslSemaCommon.h"

class DiagnosticCollector;
class SymbolTable;

/**
 * Semantic analysis pass validating calling convention definitions (.ezcc / .ccd).
 * Registers CallingConvSymbol in the symbol table, validates stack alignments,
 * register pools, argument and return rules, and structural consistency.
 */
class CallingConvPass
{
  public:
    static bool
    run(DiagnosticCollector *collector, SymbolTable *table, DSL::Ast::CallingConvDef::CallingConventionDefFile *file);

  private:
    /**
     * Validates one calling convention and registers its symbol on success.
     */
    static bool validateSingleCallingConv(DiagnosticCollector *collector,
                                          SymbolTable *table,
                                          const DSL::Ast::CallingConvDef::CallingConventionDecl &decl);

    /**
     * Validates stack alignment, shadow space, red zone, and stack-pointer presence.
     */
    static bool validateStack(DiagnosticCollector *collector, const DSL::Ast::CallingConvDef::StackDef &stack);

    /**
     * Validates callee/caller-saved register lists for duplicates and overlap.
     */
    static bool validateRegisters(DiagnosticCollector *collector,
                                  const DSL::Ast::CallingConvDef::CallingConventionDecl &file);

    /**
     * Validates unified slots and per-class argument pass rules, including stack fallbacks.
     */
    static bool validateArguments(DiagnosticCollector *collector,
                                  const DSL::Ast::CallingConvDef::ArgumentPassingDef &args);

    /**
     * Validates the struct-return pointer register and per-class return rules.
     */
    static bool validateReturns(DiagnosticCollector *collector, const DSL::Ast::CallingConvDef::ReturnDef &rets);
};

#endif // EZDSL_CALLING_CONV_PASS_H
