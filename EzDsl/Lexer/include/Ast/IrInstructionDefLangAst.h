#ifndef EZDSLLEXER_IR_INST_DEF_LANG_AST_H
#define EZDSLLEXER_IR_INST_DEF_LANG_AST_H

#include "EzDslLexerCommon.h"
#include "CommonAstNodes.h"

namespace DSL::Ast::IrInstDef
{
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

enum class IrOperandDir : uint8_t
{
    ArgIn = 0,
    ArgOut,
    ArgInOut
};

struct IrOperand
{
    IrOperandType m_type;
    Common::Identifier m_name;
    IrOperandDir m_dir{ IrOperandDir::ArgIn };
};

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

enum class IrInstTier : uint8_t
{
    HighLevel,
    PassInternal,
    TargetLow
};

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

struct IrInstBody
{
    IrInstCategory m_category{ IrInstCategory::Invalid };
    IrInstTier m_tier{ IrInstTier::HighLevel };
    std::pmr::vector<IrInstFlag> m_flags;
};

struct IrInstDecl
{
    Common::Identifier m_name;
    std::pmr::vector<IrOperand> m_operands;
    IrInstBody m_body;
};

struct IrInstDefFile
{
    std::pmr::vector<IrInstDecl> m_instructions;
};
}; // namespace DSL::Ast::IrInstDef

#endif // EZDSLLEXER_IR_INST_DEF_LANG_AST_H