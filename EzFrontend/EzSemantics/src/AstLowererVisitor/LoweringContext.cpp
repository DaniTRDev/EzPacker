#include "AstLowererVisitor/LoweringContext.h"

LoweringContext::LoweringContext(const std::shared_ptr<struct BasicSemanticContext> &semanticCtx,
                                 const std::shared_ptr<struct MirEmitter> &emitter,
                                 const std::shared_ptr<struct MirEmitterContext> &emitterContext,
                                 const std::shared_ptr<struct MirGlobalDataEmitter> &globalDataEmitter) :
    m_ownerVisitor(nullptr), m_semanticCtx(semanticCtx), m_emitter(emitter), m_emitterContext(emitterContext),
    m_globalDataEmitter(globalDataEmitter)
{
}

class AstLowererVisitor *LoweringContext::getOwnerLowererVisitor() const { return m_ownerVisitor; }

bool LoweringContext::hasBlocks() const { return !m_blockStack.empty(); }

bool LoweringContext::hasInstructions() const { return !m_instructionStack.empty(); }

bool LoweringContext::hasOperands() const { return !m_operandStack.empty(); }

bool LoweringContext::isSymbolLinkedToMir(Symbol *symbol) const { return m_symbolToMirMap.contains(symbol->getId()); }

bool LoweringContext::linkSymbolToMirId(Symbol *sym, size_t mirId)
{
    if (isSymbolLinkedToMir(sym))
    {
        return false; // Symbol is already linked to a MIR ID.
    }

    m_symbolToMirMap[sym->getId()] = mirId;
    return true;
}

bool LoweringContext::linkTypeNameToMirTypeId(Type *semanticType, size_t mirTypeId)
{
    const std::string_view &semanticTypeName = semanticType->getTypeName();
    if (m_typeNameToMirTypeIdMap.contains(semanticTypeName))
    {
        return false; // Type name is already linked to a MIR type ID.
    }

    m_typeNameToMirTypeIdMap[semanticTypeName] = mirTypeId;
    return true;
}

MirBlock *LoweringContext::popBlock()
{
    if (!hasBlocks())
    {
        throw std::runtime_error(
                "Internal Compiler Error: There are no lowered operands to pop from the operand stack");
    }

    MirBlock *block = m_blockStack.top();
    m_blockStack.pop();
    return block;
}

MirId LoweringContext::getMirIdOfSymbol(Symbol *sym) const
{
    if (!sym)
    {
        return MIRID_INVALID;
    }

    auto it = m_symbolToMirMap.find(sym->getId());
    if (it == m_symbolToMirMap.end())
    {
        return MIRID_INVALID;
    }

    return it->second;
}

MirInstruction *LoweringContext::popInstruction()
{
    if (!hasInstructions())
    {
        throw std::runtime_error(
                "Internal Compiler Error: There are no lowered instructions to pop from the instruction stack");
    }

    MirInstruction *instr = m_instructionStack.top();
    m_instructionStack.pop();
    return instr;
}

MirOperand LoweringContext::popOperand()
{
    if (!hasOperands())
    {
        throw std::runtime_error(
                "Internal Compiler Error: There are no lowered operands to pop from the operand stack");
    }

    MirOperand operand = m_operandStack.top();
    m_operandStack.pop();
    return operand;
}

MirType *LoweringContext::createMirTypeFromSemanticType(Type *semanticType)
{
    if (!semanticType)
    {
        throw std::runtime_error("Internal Compiler Error: Attempting to create a MIR type from a null semantic type");
    }

    if (m_typeNameToMirTypeIdMap.contains(semanticType->getTypeName()))
    {
        size_t mirTypeId = m_typeNameToMirTypeIdMap[semanticType->getTypeName()];
        return m_emitterContext->getMirTypeById(mirTypeId);
    }

    MirTypeKind kind = MirTypeKind::Invalid;
    switch (semanticType->getUnderlyingType())
    {
        case UnderlyingType::FloatingPoint:
        {
            kind = MirTypeKind::FloatingPoint;
            break;
        }
        case UnderlyingType::Integer:
        {
            kind = MirTypeKind::Integer;
            break;
        }
        case UnderlyingType::String:
        {
            kind = MirTypeKind::Pointer;
            break;
        }
        case UnderlyingType::Void:
        {
            kind = MirTypeKind::Void;
            break;
        }
        default:
        {
            throw std::runtime_error("Internal Compiler Error: Don't know how to create a MIR type from semantic type "
                                     "with underlying type ");
        }
    }

    MirType *mirType = m_emitterContext->createType(kind, nullptr, semanticType->getTypeName());
    if (!linkTypeNameToMirTypeId(semanticType, mirType->getId()))
    {
        throw std::runtime_error("Internal Compiler Error: Failed to link semantic type name to MIR type ID");
    }

    m_typeNameToMirTypeIdMap.insert({ semanticType->getTypeName(), mirType->getId() });
    return mirType;
}

void LoweringContext::enterLoop(const LoopContext &loopContext) { m_loopContextStack.push(loopContext); }

void LoweringContext::enterSwitch(MirBlock *breakBlock) { m_switchContextStack.push(breakBlock); }

void LoweringContext::exitLoop()
{
    if (m_loopContextStack.empty())
    {
        throw std::runtime_error(
                "Internal Compiler Error: Attempting to exit a loop context when no loop context is active");
    }
    m_loopContextStack.pop();
}

void LoweringContext::exitSwitch()
{
    if (m_switchContextStack.empty())
    {
        throw std::runtime_error("Internal Compiler Error: Attempting to exit a switch when no switch is active");
    }
    m_switchContextStack.pop();
}

void LoweringContext::pushBlock(MirBlock *block) { m_blockStack.push(std::move(block)); }

void LoweringContext::pushInstruction(MirInstruction *instruction) { m_instructionStack.push(std::move(instruction)); }

void LoweringContext::pushOperand(MirOperand operand) { m_operandStack.push(std::move(operand)); }

void LoweringContext::setOwnerVisitor(struct AstLowererVisitor *ownerVisitor) { m_ownerVisitor = ownerVisitor; }

const LoopContext &LoweringContext::getCurrentLoopContext() const
{
    if (m_loopContextStack.empty())
    {
        throw std::runtime_error(
                "Internal Compiler Error: Attempting to get current loop context when no loop context is active");
    }
    return m_loopContextStack.top();
}

const std::shared_ptr<struct BasicSemanticContext> &LoweringContext::getSemanticContext() const
{
    return m_semanticCtx;
}

const std::shared_ptr<struct MirEmitter> &LoweringContext::getEmitter() const { return m_emitter; }

const std::shared_ptr<struct MirEmitterContext> &LoweringContext::getEmitterContext() const { return m_emitterContext; }

const std::shared_ptr<struct MirGlobalDataEmitter> &LoweringContext::getGlobalDataEmitter() const
{
    return m_globalDataEmitter;
}

const std::stack<MirBlock *> &LoweringContext::getBlockStack() const { return m_blockStack; }

const std::stack<MirInstruction *> &LoweringContext::getInstructionStack() const { return m_instructionStack; }

const std::stack<MirOperand> &LoweringContext::getOperandStack() const { return m_operandStack; }
