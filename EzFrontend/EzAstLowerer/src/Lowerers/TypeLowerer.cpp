#include "Lowerers/TypeLowerer.h"

bool TypeLowerer::lower(TypeTable *table, AstLoweringContext *ctx)
{
    if (!table)
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal, "Invalid type table to lower", "TypeLowerer");
        return false;
    }

    const auto &typeMap = table->getTypeMap();
    for (auto &typePair : typeMap)
    {
        Type *type = typePair.second.get();
        if (!ctx->createMirTypeFromSemanticType(type))
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 std::format("Failed to lower type {}", type->getTypeName()),
                                                 "TypeLowerer");
            return false;
        }
    }

    return true;
}
