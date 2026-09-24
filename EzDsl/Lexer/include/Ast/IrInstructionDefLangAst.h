#ifndef EZDSLLEXER_IR_INST_DEF_LANG_AST_H
#define EZDSLLEXER_IR_INST_DEF_LANG_AST_H

#include "EzDslLexerCommon.h"
#include "CommonAstNodes.h"

namespace DSL::Ast::IrInstDef
{
/**
 * Bitmask describing the value kinds an IR operand slot accepts. Single-bit entries name one
 * kind; composite entries combine bits for convenience.
 */
enum class IrOperandType : uint16_t
{
    None = 1 << 0,          // No value accepted.
    Register = 1 << 1,      // Virtual or physical register.
    Integer = 1 << 2,       // Integer immediate.
    FloatingPoint = 1 << 3, // Floating-point immediate.
    Memory = 1 << 4,        // Memory reference.
    Reference = 1 << 5,     // Address/reference value.
    RuntimeSymbol = 1 << 6, // Symbol resolved at link/run time.
    VariadicArgs = 1 << 7,  // Variadic argument pack.

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
 * Dataflow direction of an IR operand: whether it is read, written, or both.
 */
enum class IrOperandDir : uint8_t
{
    ArgIn = 0, // Read-only input.
    ArgOut,    // Write-only output.
    ArgInOut   // Read-modify-write.
};

/**
 * One operand of a generic IR instruction signature.
 */
struct IrOperand
{
    IrOperandType m_type;                      // Accepted value kinds (bitmask).
    Common::Identifier m_name;                 // Operand name used by matchers/rules.
    IrOperandDir m_dir{ IrOperandDir::ArgIn }; // Dataflow direction.
};

/**
 * Broad behavioral classification of a generic IR instruction.
 */
enum class IrInstCategory : uint8_t
{
    Invalid = 0,  // Unset / malformed category.
    DataMovement, // Copies or moves values.
    Memory,       // Loads and stores.
    Arithmetic,   // Integer/float arithmetic.
    Bitwise,      // Bit manipulation.
    Compare,      // Comparisons producing flags/booleans.
    ControlFlow,  // Branches, calls, returns.
    Casting,      // Width/representation conversions.
    System,       // Intrinsics and runtime operations.
    Vector        // Vector/SIMD computations and data manipulation.
};

/**
 * Compilation tier at which an IR instruction is expected to exist.
 */
enum class IrInstTier : uint8_t
{
    HighLevel,    // Available in the initial IR.
    PassInternal, // Introduced by optimization/legalization passes.
    TargetLow     // Introduced during target lowering.
};

/**
 * Behavioral bitmask describing an IR instruction (side effects, memory access, control flow,
 * operand-size relationships, etc.).
 */
enum class IrInstFlag : uint32_t
{
    None = 0,
    SizeMatch = 1 << 0,       // Source and destination have the same width.
    DestLarger = 1 << 1,      // Destination is wider than the source (widening cast).
    DestSmaller = 1 << 2,     // Destination is narrower than the source (narrowing cast).
    ReadsMemory = 1 << 3,     // May read memory.
    WritesMemory = 1 << 4,    // May write memory.
    IsTerminator = 1 << 5,    // Ends a basic block.
    IsBranch = 1 << 6,        // Transfers control conditionally/unconditionally.
    IsCall = 1 << 7,          // Calls a function.
    IsReturn = 1 << 8,        // Returns from the current function.
    HasSideEffect = 1 << 9,   // Cannot be removed even if unused.
    IsCommutative = 1 << 10,  // Operands may be reordered.
    ReadsCPUFlags = 1 << 11,  // Consumes condition/status flags.
    WritesCPUFlags = 1 << 12, // Produces condition/status flags.
    TreatAsSigned = 1 << 13,  // Signed interpretation of operands.
    VariadicArgs = 1 << 14    // Accepts a variadic argument list.
};

/**
 * Behavioral body of an IR instruction declaration: its category, tier, and flag set.
 */
struct IrInstBody
{
    IrInstCategory m_category{ IrInstCategory::Invalid }; // Instruction category.
    IrInstTier m_tier{ IrInstTier::HighLevel };           // Minimum tier at which it is valid.
    std::pmr::vector<IrInstFlag> m_flags;                 // Declared behavioral flags.
};

/**
 * A generic IR instruction declaration: name, typed operand signature, and behavioral body.
 */
struct IrInstDecl
{
    Common::Identifier m_name;              // Opcode name (also its symbol).
    std::pmr::vector<IrOperand> m_operands; // Typed operand signature.
    IrInstBody m_body;                      // Category, tier, and flags.
};

/**
 * Root AST node for a parsed `.irdf` IR instruction definition file.
 */
struct IrInstDefFile
{
    std::pmr::vector<IrInstDecl> m_instructions; // All declared IR opcodes.
};
}; // namespace DSL::Ast::IrInstDef

#endif // EZDSLLEXER_IR_INST_DEF_LANG_AST_H