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

    const auto &types = file->m_types;
    size_t registeredTypes = 0;
    bool hasErrors = false;

    for (const auto &type : types)
    {
        const auto &typeName = type.m_name.m_node;
        uint32_t resolvedBitWidth = 0;

        // 1. Validate bit sizes based on TypeKind
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
            {
                if (type.m_bitSize.has_value())
                {
                    if (type.m_bitSize->m_node != 0)
                    {
                        collector->error(passName, "Void type '{}' cannot have a non-zero bit size.", typeName)
                                << type.m_bitSize->m_sourceRef;
                        hasErrors = true;
                        continue;
                    }
                }

                resolvedBitWidth = 0;
                break;
            }

            case DSL::Ast::TypeDef::TypeKind::BindingToken:
            {
                if (type.m_bitSize.has_value())
                {
                    if (type.m_bitSize->m_node != 0)
                    {
                        collector->error(passName, "Binding token type '{}' cannot have a non-zero bit size.", typeName)
                                << type.m_bitSize->m_sourceRef;
                        hasErrors = true;
                        continue;
                    }
                }

                resolvedBitWidth = 0;
                break;
            }
            case DSL::Ast::TypeDef::TypeKind::Pointer:
            {
                if (type.m_bitSize.has_value())
                {
                    if (type.m_bitSize->m_node != 0)
                    {
                        collector->error(passName, "Pointer type '{}' cannot have a non-zero bit size.", typeName)
                                << type.m_bitSize->m_sourceRef;
                        hasErrors = true;
                        continue;
                    }
                }

                resolvedBitWidth = 0;
                break;
            }
        }

        // 2. Declare and register the type symbol
        Sema::Symbols::TypeSymbol data{ .m_kind = type.m_kind, .m_bitWidth = resolvedBitWidth, .m_name = typeName };

        SymbolId id = table->declareSym(type.m_name.m_sourceRef,
                                        SymbolFlags::IsDefined,
                                        SymbolType::Type,
                                        std::move(data),
                                        typeName);

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
                         "Registered type '{}' (Kind: {}, BitWidth: {})",
                         typeName,
                         static_cast<int>(type.m_kind),
                         resolvedBitWidth);
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