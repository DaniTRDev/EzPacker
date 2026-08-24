#ifndef EZDSL_IR_INST_DEF_LANG_AST_H
#define EZDSL_IR_INST_DEF_LANG_AST_H

#include "EzDslCommon.h"
#include "InstructionDefLangAst.h"

namespace DSL::Ast::IrInstDef
{
/**
 * Bitmask enumeration representing expected operand types in IR instruction declarations.
 */
enum class IrOperandType : uint16_t
{
    None = 1 << 0,
    Register = 1 << 1,
    Integer = 1 << 2,
    FloatingPoint = 1 << 3,
    Memory = 1 << 4,
    Reference = 1 << 5,
    RuntimeSymbol = 1 << 6,
    VariadicArgs = 1 << 7,

    // Composite helper masks
    Immediate = Integer | FloatingPoint,
    RegIntImm = Register | Integer,
    RegFloatImm = Register | FloatingPoint,
    RegImm = RegIntImm | RegFloatImm,
    AddressSource = Memory | Reference,
    AnyValue = Register | Integer | FloatingPoint,
    Any = 0xFFFF
};

/**
 * Dataflow direction for an IR operand (In, Out, InOut).
 */
enum class IrOperandDir : uint8_t
{
    ArgIn,
    ArgOut,
    ArgInOut
};

/**
 * Single operand in an IR instruction declaration with type, identifier, and direction.
 */
struct IrOperand
{
    IrOperandType m_type;
    Common::Identifier m_name;
    IrOperandDir m_dir{ IrOperandDir::ArgIn };
};

/**
 * Functional category for classifying IR instructions.
 */
enum class IrInstCategory : uint8_t
{
    Invalid = 0,
    DataMovement,
    Memory,
    Arithmetic,
    Bitwise,
    Compare,
    ControlFlow,
    Casting,
    System
};

/**
 * Abstraction tier of the IR instruction (HighLevel, PassInternal, TargetLow).
 */
enum class IrInstTier : uint8_t
{
    HighLevel,
    PassInternal,
    TargetLow
};

/**
 * Behavioral and verification flags for IR instructions.
 */
enum class IrInstFlag : uint32_t
{
    None = 0,
    SizeMatch = 1 << 0,
    DestLarger = 1 << 1,
    DestSmaller = 1 << 2,
    ReadsMemory = 1 << 3,
    WritesMemory = 1 << 4,
    IsTerminator = 1 << 5,
    IsBranch = 1 << 6,
    IsCall = 1 << 7,
    IsReturn = 1 << 8,
    HasSideEffect = 1 << 9,
    IsCommutative = 1 << 10,
    ReadsCPUFlags = 1 << 11,
    WritesCPUFlags = 1 << 12,
    TreatAsSigned = 1 << 13,
    VariadicArgs = 1 << 14
};

/**
 * IR instruction body containing category, tier, and behavioral flags.
 */
struct IrInstBody
{
    IrInstCategory m_category{ IrInstCategory::Invalid };
    IrInstTier m_tier{ IrInstTier::HighLevel };
    std::pmr::vector<IrInstFlag> m_flags;
};

/**
 * Complete IR instruction declaration AST node (name, operand list, body attributes).
 */
struct IrInstDecl
{
    Common::Identifier m_name;
    std::pmr::vector<IrOperand> m_operands;
    IrInstBody m_body;
};

/**
 * Root AST structure representing a parsed IR instruction definition file.
 */
struct IrInstDefFile
{
    std::pmr::vector<IrInstDecl> m_instructions;
};
}; // namespace DSL::Ast::IrInstDef

#endif // EZDSL_IR_INST_DEF_LANG_AST_H