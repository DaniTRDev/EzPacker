#ifndef EZDSL_SEMA_ENUM_NAMES_H
#define EZDSL_SEMA_ENUM_NAMES_H

#include "EzDslSemaCommon.h"

#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/TypeDefLangAst.h"

namespace Sema::EnumNames
{

/**
 * Shared canonical spellings for the EzDSL AST enums that both the CLI info dumper and the
 * C++ code generators must render. Centralizing them keeps the two mappings in sync; a
 * consumer that needs a decorated form (e.g. "MirTypeKind::Integer") prefixes the returned
 * name rather than re-declaring the switch.
 */

/** Returns the canonical display name of a .tyf type kind. */
constexpr std::string_view typeKindName(DSL::Ast::TypeDef::TypeKind kind) noexcept
{
    using DSL::Ast::TypeDef::TypeKind;
    switch (kind)
    {
        case TypeKind::Integer:
            return "Integer";
        case TypeKind::FloatingPoint:
            return "FloatingPoint";
        case TypeKind::Void:
            return "Void";
        case TypeKind::BindingToken:
            return "BindingToken";
        case TypeKind::Pointer:
            return "Pointer";
        case TypeKind::Vector:
            return "Vector";
    }
    return "Unknown";
}

/** Returns the canonical display name of an IR instruction category. */
constexpr std::string_view irCategoryName(DSL::Ast::IrInstDef::IrInstCategory category) noexcept
{
    using DSL::Ast::IrInstDef::IrInstCategory;
    switch (category)
    {
        case IrInstCategory::Invalid:
            return "Invalid";
        case IrInstCategory::DataMovement:
            return "DataMovement";
        case IrInstCategory::Memory:
            return "Memory";
        case IrInstCategory::Arithmetic:
            return "Arithmetic";
        case IrInstCategory::Bitwise:
            return "Bitwise";
        case IrInstCategory::Compare:
            return "Compare";
        case IrInstCategory::ControlFlow:
            return "ControlFlow";
        case IrInstCategory::Casting:
            return "Casting";
        case IrInstCategory::System:
            return "System";
        case IrInstCategory::Vector:
            return "Vector";
    }
    return "Unknown";
}

/** Returns the canonical display name of an IR instruction tier. */
constexpr std::string_view irTierName(DSL::Ast::IrInstDef::IrInstTier tier) noexcept
{
    using DSL::Ast::IrInstDef::IrInstTier;
    switch (tier)
    {
        case IrInstTier::HighLevel:
            return "HighLevel";
        case IrInstTier::PassInternal:
            return "PassInternal";
        case IrInstTier::TargetLow:
            return "TargetLow";
    }
    return "Unknown";
}

/** Returns the canonical display name of an IR operand direction. */
constexpr std::string_view irOperandDirName(DSL::Ast::IrInstDef::IrOperandDir dir) noexcept
{
    using DSL::Ast::IrInstDef::IrOperandDir;
    switch (dir)
    {
        case IrOperandDir::ArgIn:
            return "IN";
        case IrOperandDir::ArgOut:
            return "OUT";
        case IrOperandDir::ArgInOut:
            return "INOUT";
    }
    return "IN";
}

} // namespace Sema::EnumNames

#endif // EZDSL_SEMA_ENUM_NAMES_H
