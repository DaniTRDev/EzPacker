#include "HighLevelMirVisitors/ModuleInstructionLowererVisitor.h"

ModuleInstructionLowererVisitor::ModuleInstructionLowererVisitor(const std::shared_ptr<HighLevelMirModule> &module)
{
    m_moduleBlock = module;
    m_currentBlock = m_moduleBlock;
}

bool ModuleInstructionLowererVisitor::visit(const std::shared_ptr<struct CodeScope> &scope)
{
    for (auto &[id, expression] : scope->getExpressions())
    {
        if (expression->getType() == AstNodeType::Instruction)
        {
            if (!AstNodeVisitor::visitBaseClass(expression))
            {
                // Add extra error for better diagnostics.
                getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                "Invalid instruction in scope body",
                                                "HLMIR_ModuleInstructionLowererVisitor::CodeScope",
                                                scope->getFirstSourceReference());
                return false;
            }
        }
        else if (expression->getType() == AstNodeType::Label)
        {
            const std::shared_ptr<SymbolAnnotation> &symbolAnnot = scope->getAnnotation<SymbolAnnotation>();
            std::shared_ptr<HighLevelMirBlock> nextBlock =
                    HighLevelMirBlock::create(symbolAnnot->getSymbol()->getId(), m_currentBlock);

            m_currentBlock->m_next = nextBlock;
            m_currentBlock = nextBlock;
            m_currentBlock->m_sourceReferences = expression->getSourceRefs();

            if (!AstNodeVisitor::visitBaseClass(expression))
            {
                getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                "Invalid label in scope body",
                                                "HLMIR_ModuleInstructionLowererVisitor::lowerScopeBody",
                                                scope->getFirstSourceReference());
                return false;
            }
        }
    }

    return true;
}

bool ModuleInstructionLowererVisitor::visit(const std::shared_ptr<struct Instruction> &instr)
{
    const std::string &name = instr->getInstructionName();

    HighLevelMirOpCode opcode = getOpCodeFromStr(instr->getInstructionName());
    if (opcode == HighLevelMirOpCode::INVALID)
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Invalid instruction name",
                                        "HLMIR_ModuleInstructionLowererVisitor::Instruction",
                                        instr->getFirstSourceReference());
        return false;
    }

    const HighLevelMirMetadata &meta = getMeta(opcode);
    if (meta.m_operandCount != instr->getOperandCount())
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        std::format("Invalid number of operands, expected {} but got {}",
                                                    meta.m_operandCount,
                                                    instr->getOperandCount()),
                                        "HLMIR_ModuleInstructionLowererVisitor::Instruction",
                                        instr->getFirstSourceReference());
        return false;
    }

    if (meta.m_operandCount == 0)
    {
        // There are no operands and we ensured this instruction has no operands, this is a small
        // fast-return-optimization.
        return true;
    }

    const auto &operands = instr->getOperands();
    HighLevelMirInstruction instruction(opcode, instr->getSourceRefs());

    for (auto &operand : operands)
    {
        if (!lowerOperand(instruction, operand))
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Fatal,
                    "Internal compiler error: Could not lower given operand because the Ast is malformed",
                    "HLMIR_ModuleInstructionLowererVisitor::Instruction",
                    operand->getFirstSourceReference());
            return false;
        }
    }

    m_currentBlock->m_instructions.push_back(std::move(instruction));
    return true;
}

bool ModuleInstructionLowererVisitor::visit(const std::shared_ptr<struct Label> &label)
{
    // If the current block is "dirty" (already has instructions from before the label),
    // we must close it and start a new one for this label.
    if (m_currentBlock && !m_currentBlock->m_instructions.empty())
    {
        // (Implicit Fallthrough)
        const std::shared_ptr<SymbolAnnotation> &symbolAnnot = label->getAnnotation<SymbolAnnotation>();
        std::shared_ptr<HighLevelMirBlock> nextBlock =
                HighLevelMirBlock::create(symbolAnnot->getSymbol()->getId(), m_currentBlock);

        m_currentBlock->m_next = nextBlock;
        m_currentBlock = nextBlock;
    }

    return visit(label->getCodeScope());
}

bool ModuleInstructionLowererVisitor::visit(const std::shared_ptr<struct Module> &module)
{
    const std::shared_ptr<SymbolAnnotation> &symbolAnnot = module->getAnnotation<SymbolAnnotation>();

    for (auto &param : module->getHeader()->getParameters())
    {
        if (param->getType() != AstNodeType::Variable)
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Fatal,
                    "Internal compiler error: Invalid virtual register parameter for module, Ast is malformed",
                    "HLMIR_ModuleInstructionLowererVisitor::Module",
                    param->getFirstSourceReference());
            return false;
        }

        const std::shared_ptr<SymbolAnnotation> &paramAnnot = param->getAnnotation<SymbolAnnotation>();
        const std::shared_ptr<Symbol> &paramSymbol = paramAnnot->getSymbol();

        HighLevelMirInstructionOperand vRegParam = HighLevelMirInstructionOperand::createReg(
                symbolAnnot->getSymbol()->getId(),
                static_cast<uint16_t>(paramSymbol->getSymbolDataType()->getUnderlyingTypeSize()));
        vRegParam.setSourceRefs(param->getSourceRefs());

        m_moduleBlock->addParam(paramSymbol->getId(), vRegParam);
    }

    m_moduleBlock->m_sourceReferences = module->getSourceRefs();
    return visit(module->getBody());
}

std::shared_ptr<HighLevelMirBlock> ModuleInstructionLowererVisitor::getCurrentBlock() { return m_currentBlock; }

std::shared_ptr<HighLevelMirBlock> ModuleInstructionLowererVisitor::getModuleBlock() const { return m_moduleBlock; }

bool ModuleInstructionLowererVisitor::lowerOperand(HighLevelMirInstruction &instruction,
                                                   const std::shared_ptr<AstNode> &node)
{
    switch (node->getType())
    {
        case AstNodeType::Immediate:
        {
            return lowerImmediateOperand(instruction, node);
        }
        case AstNodeType::MemoryOperand:
        {
            return lowerMemoryOperand(instruction, node);
        }
        case AstNodeType::Variable:
        {
            return lowerVariableOperand(instruction, node);
        }
        default:
        {
            return false;
        }
    }

    return true;
}

bool ModuleInstructionLowererVisitor::lowerImmediateOperand(HighLevelMirInstruction &instruction,
                                                            const std::shared_ptr<AstNode> &node)
{
    HighLevelMirInstructionOperand loweredOperand;
    const std::shared_ptr<ImmediateOperand> imm = std::dynamic_pointer_cast<ImmediateOperand>(node);
    std::shared_ptr<DataTypeAnnotation> dataTypeAnnot = imm->getAnnotation<DataTypeAnnotation>();

    if (dataTypeAnnot)
    {
        const std::shared_ptr<Type> &dataType = dataTypeAnnot->getDataType();
        uint16_t bitSize = static_cast<uint16_t>(dataType->getUnderlyingTypeSize());

        switch (dataType->getUnderlyingType())
        {
            case UnderlyingType::Pointer:
            case UnderlyingType::Integer:
            {
                const std::shared_ptr<IntegerImmediate> integer = std::dynamic_pointer_cast<IntegerImmediate>(node);

                if (bitSize <= 64)
                {
                    loweredOperand =
                            HighLevelMirInstructionOperand::createIntegerImm(mp_get_i64(integer->getInteger().get()));
                }
                else
                {
                    loweredOperand =
                            HighLevelMirInstructionOperand::createBigIntegerImm(integer->getInteger(), bitSize);
                }

                break;
            }
            case UnderlyingType::FloatingPoint:
            {
                const std::shared_ptr<FloatImmediate> _float = std::dynamic_pointer_cast<FloatImmediate>(node);
                loweredOperand = HighLevelMirInstructionOperand::createIntegerImm(_float->getFloatingValue());
                break;
            }
            default:
            {
                getSemanticContext()->emitError(
                        ErrorSeverity::Fatal,
                        "Internal compiler error: Given operand type is not expected here. Ast is malformed.",
                        "HLMIR_ModuleInstructionLowererVisitor::lowerImmediateOperand",
                        node->getFirstSourceReference());

                return false;
            }
        }

        loweredOperand.setSourceRefs(imm->getSourceRefs());
        instruction.addOperand(loweredOperand);
    }

    return true;
}

bool ModuleInstructionLowererVisitor::lowerMemoryOperand(HighLevelMirInstruction &instruction,
                                                         const std::shared_ptr<AstNode> &node)
{
    std::shared_ptr<MemoryOperandAstNode> operand = std::dynamic_pointer_cast<MemoryOperandAstNode>(node);
    std::shared_ptr<DataTypeAnnotation> dataTypeAnnot = node->getAnnotation<DataTypeAnnotation>();

    if (!dataTypeAnnot)
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Internal compiler error: Could not lower given memory operand because there's "
                                        "no type, the Ast is malformed",
                                        "HLMIR_ModuleInstructionLowererVisitor::lowerMemoryOperand",
                                        operand->getFirstSourceReference());
        return false;
    }

    std::shared_ptr<Type> dataType = dataTypeAnnot->getDataType();
    size_t baseRegId = 0, indexRegId = 0;
    int64_t offset = 0;
    int8_t scale = 0;

    switch (operand->getMemoryOperandType())
    {
        case MemoryOperandType::BaseDisplacement:
        {
            std::shared_ptr<BaseDisplacementMemory> casted = std::dynamic_pointer_cast<BaseDisplacementMemory>(operand);
            std::shared_ptr<SymbolAnnotation> baseSymbolAnnot = casted->getBase()->getAnnotation<SymbolAnnotation>();
            const std::shared_ptr<IntegerImmediate> &displacement = casted->getDisplacement();

            baseRegId = baseSymbolAnnot->getSymbol()->getId();
            offset = displacement ? mp_get_i64(casted->getDisplacement()->getInteger().get()) : 0;

            break;
        }
        case MemoryOperandType::BaseIndexScaleDisplacement:
        {
            std::shared_ptr<BaseIndexScaleDisplacementMemory> casted =
                    std::dynamic_pointer_cast<BaseIndexScaleDisplacementMemory>(operand);

            std::shared_ptr<SymbolAnnotation> baseSymbolAnnot = casted->getBase()->getAnnotation<SymbolAnnotation>(),
                                              indexSymbolAnnot = casted->getIndex()->getAnnotation<SymbolAnnotation>();

            baseRegId = baseSymbolAnnot->getSymbol()->getId();
            indexRegId = indexSymbolAnnot->getSymbol()->getId();
            offset = mp_get_i64(casted->getDisplacement()->getInteger().get());
            scale = mp_get_i64(casted->getScalingFactor()->getInteger().get());

            if (scale < -8 || scale > 8)
            {
                getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                "Displacement must be between [-8, 8] offset",
                                                "HLMIR_ModuleInstructionLowererVisitor::lowerMemoryOperand",
                                                operand->getFirstSourceReference());
                return false;
            }

            break;
        }
        case MemoryOperandType::Direct:
        {
            // We explicitly ban raw address usage in HLMIR for security/obfuscation reasons.
            // Programmers must use Labels or Variables, not raw pointers.
            getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                            "Usage of raw memory addresses (Direct Addressing) is forbidden in HLMIR.",
                                            "HLMIR_ModuleInstructionLowererVisitor::lowerMemoryOperand",
                                            operand->getFirstSourceReference());
            return false;
        }
        case MemoryOperandType::IndexScale:
        {
            std::shared_ptr<IndexScaleMemory> casted = std::dynamic_pointer_cast<IndexScaleMemory>(operand);
            std::shared_ptr<SymbolAnnotation> indexSymbolAnnot = casted->getIndex()->getAnnotation<SymbolAnnotation>();

            indexRegId = indexSymbolAnnot->getSymbol()->getId();
            offset = mp_get_i64(casted->getScalingFactor()->getInteger().get());
            break;
        }
        default:
        {
            return false;
        }
    }

    HighLevelMirInstructionOperand loweredOperand =
            HighLevelMirInstructionOperand::createMem(static_cast<uint16_t>(dataType->getUnderlyingTypeSize()),
                                                      baseRegId,
                                                      indexRegId,
                                                      scale,
                                                      offset);

    loweredOperand.setSourceRefs(operand->getSourceRefs());
    instruction.addOperand(loweredOperand);

    return true;
}

bool ModuleInstructionLowererVisitor::lowerVariableOperand(HighLevelMirInstruction &instruction,
                                                           const std::shared_ptr<AstNode> &node)
{
    std::shared_ptr<SymbolAnnotation> symbolAnnot = node->getAnnotation<SymbolAnnotation>();
    std::shared_ptr<Type> type;
    std::shared_ptr<TypeCastAnnotation> typeCastAnnot = node->getAnnotation<TypeCastAnnotation>();

    if (typeCastAnnot)
    {
        type = typeCastAnnot->getCastedDataType();
    }
    else if (symbolAnnot)
    {
        type = symbolAnnot->getSymbol()->getSymbolDataType();
    }
    else
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                "Internal compiler error: Could not lower given variable operand because there's "
                "no type, the Ast is malformed",
                "HLMIR_ModuleInstructionLowererVisitor::lowerVariableOperand",
                node->getFirstSourceReference());
        return false;
    }

    HighLevelMirInstructionOperand operand;
    std::shared_ptr<AstNode> definingNode = symbolAnnot->getSymbol()->getDefiningNode();
    if (definingNode->getType() == AstNodeType::Label || definingNode->getType() == AstNodeType::Module)
    {
        // If the symbol is from a label or a module, this virtual variable is a reference.
        operand = HighLevelMirInstructionOperand::createRef(symbolAnnot->getSymbol()->getId());
    }
    else
    {
        operand = HighLevelMirInstructionOperand::createReg(symbolAnnot->getSymbol()->getId(),
                                                            static_cast<uint16_t>(type->getUnderlyingTypeSize()));
    }

    operand.setSourceRefs(node->getSourceRefs());
    instruction.addOperand(operand);

    return true;
}
