#ifndef EZDSL_SEMA_PASSES_PASS_DRIVER_H
#define EZDSL_SEMA_PASSES_PASS_DRIVER_H

#include "EzDslSemaCommon.h"

#include "Diagnostics/DiagnosticCollector.h"

class SymbolTable;

namespace Sema
{

/**
 * Validates the inputs shared by every Sema pass before it walks its AST.
 *
 * Centralizes the null-guard preamble that all nine passes used to open-code, and gives every
 * pass the same diagnostic wording and `Sema::<Pass>` sender. Returns false when the caller must
 * not run: a null collector is silently fatal, while a null symbol table or AST is reported.
 *
 * @param collector Diagnostics sink; a null value means "run silently impossible" and aborts.
 * @param table     Populated symbol table; must be non-null.
 * @param file      Root AST node for the dialect; must be non-null.
 * @param passName  Fully-qualified sender, e.g. "Sema::TypePass".
 */
template <typename FileT>
bool preparePass(DiagnosticCollector *collector, SymbolTable *table, FileT *file, std::string_view passName)
{
    if (!collector)
    {
        return false;
    }

    if (!table || !file)
    {
        collector->error(passName, "Invalid symbol table or AST file pointer.");
        return false;
    }

    return true;
}

} // namespace Sema

#endif // EZDSL_SEMA_PASSES_PASS_DRIVER_H
