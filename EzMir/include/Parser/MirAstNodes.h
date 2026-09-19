#ifndef EZMIR_MIR_AST_NODES_H
#define EZMIR_MIR_AST_NODES_H

#include "EzMirCommon.h"
#include <memory_resource>
#include <optional>
#include <string_view>
#include <vector>

class SourceReference;

namespace EzMir::Ast
{

/**
 * Discriminates the shape of a parsed MIR type reference used by the AST.
 */
enum class TypeKind : uint8_t
{
    Primitive, ///< A built-in scalar/named type described by m_name and m_bitWidth.
    Pointer,   ///< A pointer type whose pointee is stored in m_subType.
    Array,     ///< A fixed-size array type; element type in m_subType and element count in m_arraySize.
    Token,     ///< A single token/opaque type used to represent unresolved text.
    Void       ///< The void/empty type carrying no storage.
};

/**
 * Parsed representation of a MIR type expression, holding the resolved kind and any
 * nested subtype/array metadata needed to materialize a MirType later.
 */
struct MirAstType
{
    TypeKind m_kind{ TypeKind::Primitive }; ///< Shape of this type node.
    std::pmr::string m_name;                ///< Base name of the type as written in the source (e.g. "i32").
    size_t m_bitWidth{ 0 };                 ///< Width in bits for primitive numeric types; 0 when not applicable.
    MirAstType *m_subType{ nullptr };       ///< Pointee/element type for pointer and array kinds.
    size_t m_arraySize{ 0 };                ///< Number of elements for array kinds.
    SourceReference *m_ref{ nullptr };      ///< Source location this type was parsed from.

    /**
     * Allocates the name string from the supplied PMR arena.
     */
    explicit MirAstType(std::pmr::memory_resource *mr) : m_name(mr) {}
};

/**
 * Symbol visibility/linkage of a declared global entity.
 */
enum class Linkage : uint8_t
{
    External, ///< Visible outside the current module and resolved at link time.
    Internal, ///< Private to the module; may be optimized/removed freely.
    Weak      ///< Overridable definition emitted as a weak symbol.
};

/**
 * Discriminates the payload stored inside a MirAstConstantInit node.
 */
enum class ConstantKind : uint8_t
{
    Integer, ///< Scalar integer literal stored in m_intVal.
    Float,   ///< Scalar floating-point literal stored in m_floatVal.
    String,  ///< String literal stored in m_strVal.
    Array,   ///< Aggregate of nested initializers stored in m_elements.
    ZeroInit ///< Implicit zero-initialization with no explicit payload.
};

/**
 * One node in a (possibly nested) constant initializer tree for a global variable.
 */
struct MirAstConstantInit
{
    ConstantKind m_kind{ ConstantKind::ZeroInit };   ///< Selects which payload field below is meaningful.
    int64_t m_intVal{ 0 };                           ///< Integer value for ConstantKind::Integer.
    double m_floatVal{ 0.0 };                        ///< Floating-point value for ConstantKind::Float.
    std::pmr::string m_strVal;                       ///< Text payload for ConstantKind::String.
    std::pmr::vector<MirAstConstantInit> m_elements; ///< Element list for ConstantKind::Array.
    SourceReference *m_ref{ nullptr };               ///< Source location of this initializer expression.

    /**
     * Allocates the string and element containers from the supplied PMR arena.
     */
    explicit MirAstConstantInit(std::pmr::memory_resource *mr) : m_strVal(mr), m_elements(mr) {}
};

/**
 * Parsed declaration of a module-level global variable, including its type and optional initializer.
 */
struct MirAstGlobalVar
{
    std::pmr::string m_name;                  ///< Symbol name of the global.
    Linkage m_linkage{ Linkage::Internal };   ///< Visibility/linkage of the symbol.
    bool m_isConst{ true };                   ///< Whether the global is declared read-only.
    MirAstType *m_type{ nullptr };            ///< Declared type of the global.
    std::optional<MirAstConstantInit> m_init; ///< Present when the declaration carries an initializer.
    SourceReference *m_ref{ nullptr };        ///< Source location of the declaration.

    /**
     * Allocates the name string from the supplied PMR arena.
     */
    explicit MirAstGlobalVar(std::pmr::memory_resource *mr) : m_name(mr) {}
};

/**
 * Parsed memory addressing expression of the form base + index*scale + disp, mirroring
 * the addressing-mode components accepted by the backend.
 */
struct MirAstMemExpr
{
    std::pmr::string m_baseReg;        ///< Base register name; empty when absent.
    std::pmr::string m_indexReg;       ///< Index register name; empty when absent.
    uint8_t m_scale{ 1 };              ///< Multiplier applied to the index register.
    int64_t m_disp{ 0 };               ///< Constant displacement added to the address.
    bool m_hasPtrPrefix{ false };      ///< True when the expression was written with an explicit pointer dereference.
    SourceReference *m_ref{ nullptr }; ///< Source location of the memory expression.

    /**
     * Allocates the base and index strings from the supplied PMR arena.
     */
    explicit MirAstMemExpr(std::pmr::memory_resource *mr) : m_baseReg(mr), m_indexReg(mr) {}
};

/**
 * Discriminates the payload carried by a MirAstOperand.
 */
enum class OperandKind : uint8_t
{
    Register,       ///< Virtual/physical register reference named by m_name.
    ImmediateInt,   ///< Integer immediate stored in m_intVal.
    ImmediateFloat, ///< Floating-point immediate stored in m_floatVal.
    LabelRef,       ///< Reference to a basic block label named by m_name.
    MemoryRef,      ///< Memory operand described by m_mem.
    GlobalRef,      ///< Reference to a module-level global named by m_name.
    StackRef,       ///< Stack slot reference with a byte offset in m_offset.
    RuntimeSymbol,  ///< Reference to an externally-resolved runtime symbol.
    PhiPair         ///< One (value, block) edge of a phi node.
};

/**
 * Parsed operand of a MIR instruction; which payload fields are valid depends on m_kind.
 */
struct MirAstOperand
{
    OperandKind m_kind{ OperandKind::Register }; ///< Selects the valid payload fields.
    MirAstType *m_type{ nullptr };               ///< Type of the operand as written/to be inferred.
    std::pmr::string m_name;                     ///< Primary name (register, label, global or symbol).
    std::pmr::string m_classBinding;             ///< Optional register-class binding annotation.
    int64_t m_intVal{ 0 };                       ///< Integer immediate payload.
    double m_floatVal{ 0.0 };                    ///< Floating-point immediate payload.
    int64_t m_offset{ 0 };                       ///< Byte offset for stack references.
    MirAstMemExpr m_mem;                         ///< Addressing details for memory operands.
    std::pmr::string m_phiValue;                 ///< Incoming value name for a phi pair.
    std::pmr::string m_phiBlock;                 ///< Predecessor block name for a phi pair.
    SourceReference *m_ref{ nullptr };           ///< Source location of the operand.

    /**
     * Allocates all owned strings/containers from the supplied PMR arena.
     */
    explicit MirAstOperand(std::pmr::memory_resource *mr) :
        m_name(mr), m_classBinding(mr), m_mem(mr), m_phiValue(mr), m_phiBlock(mr)
    {
    }
};

/**
 * A single parsed MIR instruction with an optional destination, opcode, result type and operands.
 */
struct MirAstInstruction
{
    std::optional<std::pmr::string>
            m_assignedDst;         ///< Assigned destination register, absent for store/terminator instructions.
    std::pmr::string m_opcode;     ///< Instruction opcode text.
    MirAstType *m_type{ nullptr }; ///< Declared/derived result type of the instruction.
    std::pmr::vector<MirAstOperand> m_operands; ///< Ordered operands consumed by the instruction.
    SourceReference *m_ref{ nullptr };          ///< Source location of the instruction.

    /**
     * Allocates the opcode and operand list from the supplied PMR arena.
     */
    explicit MirAstInstruction(std::pmr::memory_resource *mr) : m_opcode(mr), m_operands(mr) {}
};

/**
 * A labeled basic block holding an ordered list of instructions terminated by a branch instruction.
 */
struct MirAstBasicBlock
{
    std::pmr::string m_name;                            ///< Block label used by branch targets and phi edges.
    std::pmr::vector<MirAstInstruction> m_instructions; ///< Instructions belonging to this block.
    SourceReference *m_ref{ nullptr };                  ///< Source location of the block definition.

    /**
     * Allocates the label and instruction list from the supplied PMR arena.
     */
    explicit MirAstBasicBlock(std::pmr::memory_resource *mr) : m_name(mr), m_instructions(mr) {}
};

/**
 * A single formal parameter of a parsed function definition.
 */
struct MirAstParam
{
    MirAstType *m_type{ nullptr };     ///< Declared type of the parameter.
    std::pmr::string m_name;           ///< Parameter name.
    SourceReference *m_ref{ nullptr }; ///< Source location of the parameter.

    /**
     * Allocates the name string from the supplied PMR arena.
     */
    explicit MirAstParam(std::pmr::memory_resource *mr) : m_name(mr) {}
};

/**
 * A key/value attribute attached to a parsed function definition.
 */
struct MirAstAttribute
{
    std::pmr::string m_name;           ///< Attribute key.
    std::pmr::string m_value;          ///< Attribute value.
    SourceReference *m_ref{ nullptr }; ///< Source location of the attribute.

    /**
     * Allocates the key and value strings from the supplied PMR arena.
     */
    explicit MirAstAttribute(std::pmr::memory_resource *mr) : m_name(mr), m_value(mr) {}
};

/**
 * A parsed function prototype: name, parameter types, return type and variadic flag,
 * with no body attached.
 */
struct MirAstFunctionDecl
{
    std::pmr::string m_name;                     ///< Function symbol name.
    std::pmr::vector<MirAstType *> m_paramTypes; ///< Declared parameter types, in order.
    MirAstType *m_returnType{ nullptr };         ///< Declared return type.
    bool m_isVariadic{ false };                  ///< True for C-style variadic prototypes.
    SourceReference *m_ref{ nullptr };           ///< Source location of the declaration.

    /**
     * Allocates the name and parameter list from the supplied PMR arena.
     */
    explicit MirAstFunctionDecl(std::pmr::memory_resource *mr) : m_name(mr), m_paramTypes(mr) {}
};

/**
 * A parsed function definition: signature, attributes and body composed of basic blocks.
 */
struct MirAstFunctionDef
{
    std::pmr::string m_name;                        ///< Function symbol name.
    std::pmr::vector<MirAstParam> m_params;         ///< Formal parameters with names and types.
    MirAstType *m_returnType{ nullptr };            ///< Declared return type.
    std::pmr::vector<MirAstAttribute> m_attributes; ///< Attributes attached to the definition.
    std::pmr::vector<MirAstBasicBlock> m_blocks;    ///< Basic blocks forming the function body.
    SourceReference *m_ref{ nullptr };              ///< Source location of the definition.

    /**
     * Allocates the name and all owned containers from the supplied PMR arena.
     */
    explicit MirAstFunctionDef(std::pmr::memory_resource *mr) : m_name(mr), m_params(mr), m_attributes(mr), m_blocks(mr)
    {
    }
};

/**
 * A target directive selecting the backend triple for the parsed module.
 */
struct MirAstTargetDirective
{
    std::pmr::string m_target;         ///< Target triple/name string.
    SourceReference *m_ref{ nullptr }; ///< Source location of the directive.

    /**
     * Allocates the target string from the supplied PMR arena.
     */
    explicit MirAstTargetDirective(std::pmr::memory_resource *mr) : m_target(mr) {}
};

/**
 * Root AST node for a parsed MIR source file: optional target directive plus all
 * global variable, function prototype and function definition nodes.
 */
struct MirAstModule
{
    std::optional<MirAstTargetDirective> m_target;        ///< Target directive when present in the module.
    std::pmr::vector<MirAstGlobalVar> m_globals;          ///< Global variable declarations.
    std::pmr::vector<MirAstFunctionDecl> m_functionDecls; ///< Function prototypes.
    std::pmr::vector<MirAstFunctionDef> m_functionDefs;   ///< Function definitions.

    /**
     * Allocates all owned containers from the supplied PMR arena.
     */
    explicit MirAstModule(std::pmr::memory_resource *mr) : m_globals(mr), m_functionDecls(mr), m_functionDefs(mr) {}
};

} // namespace EzMir::Ast

#endif // EZMIR_MIR_AST_NODES_H
