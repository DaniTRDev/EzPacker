#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/TypePass.h"

bool TypePass::run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::TypeDef::TypeDefFile *file)
{
    constexpr auto passName = "TypePass";

    if (!collector)
    {
        return false;
    }

    if (!table || !file)
    {
        collector->error(passName, "Invalid symbol table or AST file pointer.");
        return false;
    }

    bool hasErrors = false;
    size_t registeredTypes = 0;
    uint8_t compactId = 1; // 0 is reserved as invalid/error.

    for (const auto &type : file->m_types)
    {
        const auto &typeName = type.m_name.m_node;
        uint32_t resolvedBitWidth = 0;

        // Resolve the effective bit width according to the declared type kind.
        switch (type.m_kind)
        {
            case DSL::Ast::TypeDef::TypeKind::Integer:
            {
                if (!type.m_bitSize.has_value())
                {
                    collector->error(passName,
                                     "Integer type '{}' requires an explicit bit size (e.g., '{}(32)').",
                                     typeName,
                                     typeName)
                            << type.m_name.m_sourceRef;
                    hasErrors = true;
                    continue;
                }

                auto bitSize = type.m_bitSize->m_node;
                if (bitSize <= 0 || bitSize > 1024)
                {
                    collector->error(passName,
                                     "Invalid integer bit width {} for type '{}'. Must be between 1 and 1024.",
                                     bitSize,
                                     typeName)
                            << type.m_bitSize->m_sourceRef;
                    hasErrors = true;
                    continue;
                }

                resolvedBitWidth = static_cast<uint32_t>(bitSize);
                break;
            }

            case DSL::Ast::TypeDef::TypeKind::FloatingPoint:
            {
                if (!type.m_bitSize.has_value())
                {
                    collector->error(passName,
                                     "Floating-point type '{}' requires an explicit bit size (e.g., '{}(32)').",
                                     typeName,
                                     typeName)
                            << type.m_name.m_sourceRef;
                    hasErrors = true;
                    continue;
                }

                auto bitSize = type.m_bitSize->m_node;
                if (bitSize != 16 && bitSize != 32 && bitSize != 64 && bitSize != 128)
                {
                    collector->error(
                            passName,
                            "Unsupported floating-point width {} for type '{}'. Supported widths: 16, 32, 64, 128.",
                            bitSize,
                            typeName)
                            << type.m_bitSize->m_sourceRef;
                    hasErrors = true;
                    continue;
                }

                resolvedBitWidth = static_cast<uint32_t>(bitSize);
                break;
            }

            case DSL::Ast::TypeDef::TypeKind::Void:
            case DSL::Ast::TypeDef::TypeKind::BindingToken:
            case DSL::Ast::TypeDef::TypeKind::Pointer:
            {
                // Non-scalar kinds carry no width and must not declare one.
                if (type.m_bitSize.has_value() && type.m_bitSize->m_node != 0)
                {
                    collector->error(passName, "Type '{}' cannot have a non-zero bit size.", typeName)
                            << type.m_bitSize->m_sourceRef;
                    hasErrors = true;
                    continue;
                }

                resolvedBitWidth = 0;
                break;
            }
        }

        // Resolve alignment, defaulting to the bit width when the declaration omits it.
        uint32_t resolvedAlignment = 0;
        if (type.m_alignment.has_value())
        {
            if (resolvedBitWidth == 0 && type.m_alignment->m_node != 0)
            {
                collector->error(passName, "Type '{}' cannot have a non-zero alignment.", typeName)
                        << type.m_alignment->m_sourceRef;
                hasErrors = true;
                continue;
            }

            resolvedAlignment = type.m_alignment->m_node;
        }
        else
        {
            resolvedAlignment = resolvedBitWidth;
            collector->trace(passName, "Using default alignment ('{}') for type '{}'", resolvedAlignment, typeName)
                    << type.m_name.m_sourceRef;
        }

        // Check compact ID limits (1-254)
        if (compactId >= 255)
        {
            collector->error(passName,
                             "Reached end of compact IDs for machine types. For optimization purposes this number is "
                             "limited to 254. "
                             "Move out of the .tyf file any type that is not STRICTLY used in legalization and "
                             "instruction selection.")
                    << type.m_name.m_sourceRef;
            return false;
        }

        // Declare and register the type symbol
        Symbols::TypeSymbol data{ .m_name = typeName,
                                  .m_kind = type.m_kind,
                                  .m_bitWidth = resolvedBitWidth,
                                  .m_alignment = resolvedAlignment,
                                  .m_compactId = compactId++ };

        SymbolId id = table->declareSym(type.m_name.m_sourceRef, SymbolType::Type, std::move(data), typeName);

        if (id == InvalidSymbolId)
        {
            collector->error(
                    passName,
                    "Can't register type because a symbol with the name '{}' already exists in the current scope.",
                    typeName)
                    << type.m_name.m_sourceRef;
            hasErrors = true;
            continue;
        }

        collector->trace(passName,
                         "Registered type '{}' (Kind: {}, BitWidth: {}, Alignment: {})",
                         typeName,
                         static_cast<int>(type.m_kind),
                         resolvedBitWidth,
                         resolvedAlignment);

        registeredTypes++;
    }

    if (hasErrors)
    {
        collector->error(passName, "TypePass completed with errors.");
        return false;
    }

    collector->trace(passName, "Registered {} types successfully", registeredTypes);
    return true;
}