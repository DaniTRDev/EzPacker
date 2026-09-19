#ifndef EZDSLSEMA_TARGET_DESC_PASS_H
#define EZDSLSEMA_TARGET_DESC_PASS_H

#include "Ast/TargetDescDefLangAst.h"
#include "EzDslSemaCommon.h"

/**
 * Semantic analysis pass processing parsed .tdesc target descriptor manifests.
 * Validates target constants, referenced config files, object formats, libcall ids,
 * component slots, and default calling convention selection.
 */
class TargetDescPass
{
  public:
    /**
     * Executes the target descriptor semantic pass over the provided AST root.
     * Returns true if the manifest was successfully validated and declared.
     */
    bool
    run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::TargetDesc::TargetDescFile *file);
};

#endif // EZDSLSEMA_TARGET_DESC_PASS_H
