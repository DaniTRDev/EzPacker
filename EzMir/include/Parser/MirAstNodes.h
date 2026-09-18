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

enum class TypeKind : uint8_t
{
    Primitive,
    Pointer,
    Array,
    Token,
    Void
};

struct MirAstType
{
    TypeKind m_kind{ TypeKind::Primitive };
    std::pmr::string m_name;
    size_t m_bitWidth{ 0 };
    MirAstType *m_subType{ nullptr };
    size_t m_arraySize{ 0 };
    SourceReference *m_ref{ nullptr };

    explicit MirAstType(std::pmr::memory_resource *mr) : m_name(mr) {}
};

enum class Linkage : uint8_t
{
    External,
    Internal,
    Weak
};

enum class ConstantKind : uint8_t
{
    Integer,
    Float,
    String,
    Array,
    ZeroInit
};

struct MirAstConstantInit
{
    ConstantKind m_kind{ ConstantKind::ZeroInit };
    int64_t m_intVal{ 0 };
    double m_floatVal{ 0.0 };
    std::pmr::string m_strVal;
    std::pmr::vector<MirAstConstantInit> m_elements;
    SourceReference *m_ref{ nullptr };

    explicit MirAstConstantInit(std::pmr::memory_resource *mr) : m_strVal(mr), m_elements(mr) {}
};

struct MirAstGlobalVar
{
    std::pmr::string m_name;
    Linkage m_linkage{ Linkage::Internal };
    bool m_isConst{ true };
    MirAstType *m_type{ nullptr };
    std::optional<MirAstConstantInit> m_init;
    SourceReference *m_ref{ nullptr };

    explicit MirAstGlobalVar(std::pmr::memory_resource *mr) : m_name(mr) {}
};

struct MirAstMemExpr
{
    std::pmr::string m_baseReg;
    std::pmr::string m_indexReg;
    uint8_t m_scale{ 1 };
    int64_t m_disp{ 0 };
    bool m_hasPtrPrefix{ false };
    SourceReference *m_ref{ nullptr };

    explicit MirAstMemExpr(std::pmr::memory_resource *mr) : m_baseReg(mr), m_indexReg(mr) {}
};

enum class OperandKind : uint8_t
{
    Register,
    ImmediateInt,
    ImmediateFloat,
    LabelRef,
    MemoryRef,
    GlobalRef,
    StackRef,
    RuntimeSymbol,
    PhiPair
};

struct MirAstOperand
{
    OperandKind m_kind{ OperandKind::Register };
    MirAstType *m_type{ nullptr };
    std::pmr::string m_name;
    std::pmr::string m_classBinding;
    int64_t m_intVal{ 0 };
    double m_floatVal{ 0.0 };
    int64_t m_offset{ 0 };
    MirAstMemExpr m_mem;
    std::pmr::string m_phiValue;
    std::pmr::string m_phiBlock;
    SourceReference *m_ref{ nullptr };

    explicit MirAstOperand(std::pmr::memory_resource *mr) :
        m_name(mr), m_classBinding(mr), m_mem(mr), m_phiValue(mr), m_phiBlock(mr)
    {
    }
};

struct MirAstInstruction
{
    std::optional<std::pmr::string> m_assignedDst;
    std::pmr::string m_opcode;
    MirAstType *m_type{ nullptr };
    std::pmr::vector<MirAstOperand> m_operands;
    SourceReference *m_ref{ nullptr };

    explicit MirAstInstruction(std::pmr::memory_resource *mr) : m_opcode(mr), m_operands(mr) {}
};

struct MirAstBasicBlock
{
    std::pmr::string m_name;
    std::pmr::vector<MirAstInstruction> m_instructions;
    SourceReference *m_ref{ nullptr };

    explicit MirAstBasicBlock(std::pmr::memory_resource *mr) : m_name(mr), m_instructions(mr) {}
};

struct MirAstParam
{
    MirAstType *m_type{ nullptr };
    std::pmr::string m_name;
    SourceReference *m_ref{ nullptr };

    explicit MirAstParam(std::pmr::memory_resource *mr) : m_name(mr) {}
};

struct MirAstAttribute
{
    std::pmr::string m_name;
    std::pmr::string m_value;
    SourceReference *m_ref{ nullptr };

    explicit MirAstAttribute(std::pmr::memory_resource *mr) : m_name(mr), m_value(mr) {}
};

struct MirAstFunctionDecl
{
    std::pmr::string m_name;
    std::pmr::vector<MirAstType *> m_paramTypes;
    MirAstType *m_returnType{ nullptr };
    bool m_isVariadic{ false };
    SourceReference *m_ref{ nullptr };

    explicit MirAstFunctionDecl(std::pmr::memory_resource *mr) : m_name(mr), m_paramTypes(mr) {}
};

struct MirAstFunctionDef
{
    std::pmr::string m_name;
    std::pmr::vector<MirAstParam> m_params;
    MirAstType *m_returnType{ nullptr };
    std::pmr::vector<MirAstAttribute> m_attributes;
    std::pmr::vector<MirAstBasicBlock> m_blocks;
    SourceReference *m_ref{ nullptr };

    explicit MirAstFunctionDef(std::pmr::memory_resource *mr) :
        m_name(mr), m_params(mr), m_attributes(mr), m_blocks(mr)
    {
    }
};

struct MirAstTargetDirective
{
    std::pmr::string m_target;
    SourceReference *m_ref{ nullptr };

    explicit MirAstTargetDirective(std::pmr::memory_resource *mr) : m_target(mr) {}
};

struct MirAstModule
{
    std::optional<MirAstTargetDirective> m_target;
    std::pmr::vector<MirAstGlobalVar> m_globals;
    std::pmr::vector<MirAstFunctionDecl> m_functionDecls;
    std::pmr::vector<MirAstFunctionDef> m_functionDefs;

    explicit MirAstModule(std::pmr::memory_resource *mr) :
        m_globals(mr), m_functionDecls(mr), m_functionDefs(mr)
    {
    }
};

} // namespace EzMir::Ast

#endif // EZMIR_MIR_AST_NODES_H
