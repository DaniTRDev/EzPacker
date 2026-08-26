#ifndef EZDSL_CALLING_CONV_PASS_H
#define EZDSL_CALLING_CONV_PASS_H

#include "EzDslCommon.h"
#include "Ast/CallingConvDefLangAst.h"

// Forward declarations.
namespace Sema::Symbols
{
class RegisterRefSymbol;
class AggregateClassifySymbol;
class CallingConvSymbol;
class DispatchRuleSymbol;
class LoweringActionSymbol;
class SretConfigSymbol;
}; // namespace Sema::Symbols

/**
 * Executes the semantic analysis and symbol resolution pass for calling convention definitions (.ccdf).
 */
class CallingConvPass
{
  public:
    static bool run(class DiagnosticCollector *collector,
                    class SymbolTable *table,
                    DSL::Ast::CallingConvDef::CallingConvDefFile *file);

  private:
    static bool resolveRegisterRef(class DiagnosticCollector *collector,
                                   class SymbolTable *table,
                                   const DSL::Ast::CallingConvDef::RegisterRef &astRef,
                                   std::string_view context,
                                   class Sema::Symbols::RegisterRefSymbol &outRef);

    static bool processClassification(class DiagnosticCollector *collector,
                                      class SymbolTable *table,
                                      const DSL::Ast::CallingConvDef::ClassifyBlock &classifyBlock,
                                      std::string_view ccName,
                                      class Sema::Symbols::CallingConvSymbol &ccSym);

    static bool processAggregateDef(class DiagnosticCollector *collector,
                                    class SymbolTable *table,
                                    const DSL::Ast::CallingConvDef::AggregateClassifyDef &aggDef,
                                    std::string_view ccName,
                                    class Sema::Symbols::AggregateClassifySymbol &outAggSym);

    static bool processDispatchRules(class DiagnosticCollector *collector,
                                     class SymbolTable *table,
                                     const std::pmr::vector<DSL::Ast::CallingConvDef::DispatchRule> &rules,
                                     std::string_view context,
                                     std::string_view ccName,
                                     std::pmr::vector<class Sema::Symbols::DispatchRuleSymbol> &outRules);

    static bool processLoweringAction(class DiagnosticCollector *collector,
                                      class SymbolTable *table,
                                      const DSL::Ast::CallingConvDef::LoweringAction &action,
                                      std::string_view context,
                                      std::string_view ccName,
                                      class Sema::Symbols::LoweringActionSymbol &outAction);

    static bool processSretConfig(class DiagnosticCollector *collector,
                                  class SymbolTable *table,
                                  const DSL::Ast::CallingConvDef::SretConfig &config,
                                  std::string_view ccName,
                                  class Sema::Symbols::SretConfigSymbol &outSret);
};

#endif // EZDSL_CALLING_CONV_PASS_H