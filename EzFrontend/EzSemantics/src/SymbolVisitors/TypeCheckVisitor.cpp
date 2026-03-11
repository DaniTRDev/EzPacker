#include "SymbolVisitors/TypeCheckVisitor.h"

bool TypeCheckVisitor::visit(BreakAstNode *_break)
{
    if (!m_ctx->isContextInsideLoop() && !m_ctx->isContextInsideSwitch())
    {
        m_ctx->emitError(ErrorSeverity::Fatal,
                         "Break statement is not inside a loop or switch",
                         "TypeCheckVisitor::BreakAstNode",
                         _break->getSourceRef());
        return false;
    }

    return true;
}

bool TypeCheckVisitor::visit(CodeScope *scope) { return AstNodeVisitor::visitAll(scope->getExpressions()); }

bool TypeCheckVisitor::visit(ContinueAstNode *_continue)
{
    if (!m_ctx->isContextInsideLoop())
    {
        m_ctx->emitError(ErrorSeverity::Fatal,
                         "Continue statement is not inside a loop",
                         "TypeCheckVisitor::ContinueAstNode",
                         _continue->getSourceRef());
        return false;
    }

    return true;
}

bool TypeCheckVisitor::visit(ForAstNode *_for)
{
    if (!_for->getInitialization()->accept(this) || !_for->getCondition()->accept(this) ||
        (_for->getNextItClause() && !_for->getNextItClause()->accept(this)))
    {
        return false;
    }

    LoopGuard loopGuard(m_ctx); // Allow break/continue inside the for-loop body
    return _for->getBody()->accept(this);
}

bool TypeCheckVisitor::visit(IfAstNode *ifNode)
{
    return ifNode->getCondition()->accept(this) && ifNode->getTrueScope()->accept(this) &&
            (!ifNode->getFalseScope() || ifNode->getFalseScope()->accept(this));
}

bool TypeCheckVisitor::visit(Instruction *instr)
{
    CallInstruction *callInstr = dynamic_cast<CallInstruction *>(instr);
    if (callInstr)
    {
        Variable *callee = callInstr->getExpressions()->get<Variable>(0);
        if (!callee)
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             "Invalid callee for call instruction",
                             "TypeCheckVisitor",
                             instr->getSourceRef());
            return false;
        }

        Symbol *calleeSym = callee->getAnnotation<SymbolAnnotation>()->getSymbol();
        Type *calleeType = calleeSym->getSymbolDataType();
        std::vector<Type *> calleeSubTypes = calleeType->getSubTypes();

        size_t expectedCalleeSubTypeCount = calleeSubTypes.size() - 1;
        size_t currentCalleeSubTypeCount = callInstr->getExpressionCount() - 1;

        if (currentCalleeSubTypeCount != expectedCalleeSubTypeCount)
        {
            m_ctx->emitError(ErrorSeverity::Fatal,
                             std::format("Given {} parameters and expected {}",
                                         currentCalleeSubTypeCount,
                                         expectedCalleeSubTypeCount),
                             "TypeCheckVisitor",
                             instr->getSourceRef());
            return false;
        }

        TypedPoolSlice<AstNode>::Iterator paramIt = callInstr->getExpressions()->begin();
        ++paramIt;

        for (size_t i = 1; i < currentCalleeSubTypeCount + 1; i++)
        {
            if (!paramIt)
            {
                m_ctx->emitError(ErrorSeverity::Fatal,
                                 std::format("Expected parameter {} (pos {})", calleeSubTypes[i]->getTypeName(), i),
                                 "TypeCheckVisitor",
                                 instr->getSourceRef());
                return false;
            }

            if (!(*paramIt)->accept(this))
            {
                return false;
            }

            Type *parameterType = (*paramIt)->getAnnotation<DataTypeAnnotation>()->getDataType();
            if (!checkCastSafety(instr, calleeSubTypes[i], parameterType))
            {
                m_ctx->emitError(ErrorSeverity::Fatal, "Invalid parameter", "TypeCheckVisitor");
                return false;
            }

            ++paramIt;
        }

        return true;
    }

    // We need to manually traverse this container to infer the types of immediates.
    Type *targetInstructionType = nullptr;

    // First pass: Find the target type (usually from the first Variable or Memory operand)
    for (AstNode *node : *instr->getExpressions())
    {
        if (node->getType() == AstNodeType::Variable)
        {
            Variable *var = dynamic_cast<Variable *>(node);
            Symbol *sym = var->getAnnotation<SymbolAnnotation>()->getSymbol();
            targetInstructionType = sym->getSymbolDataType();
            break;
        }
        else if (node->getType() == AstNodeType::MemoryOperand)
        {
            MemoryOperandAstNode *mem = dynamic_cast<MemoryOperandAstNode *>(node);
            targetInstructionType =
                    getSemanticContext()->getTypeTable()->getType(mem->getReferencedMemoryDataTypeStr()).get();
            break;
        }
    }

    // Second pass: Validate all operands against this context
    for (AstNode *node : *instr->getExpressions())
    {
        if (node->getType() == AstNodeType::Immediate)
        {
            ImmediateOperand *imm = dynamic_cast<ImmediateOperand *>(node);
            Type *immType = imm->getAnnotation<DataTypeAnnotation>()->getDataType();

            if (targetInstructionType)
            {
                if (!checkImmediateSafety(imm, targetInstructionType))
                {
                    m_ctx->emitError(ErrorSeverity::Fatal,
                                     "Invalid immediate found",
                                     "TypeCheckVisitor",
                                     node->getSourceRef());
                    return false;
                }

                // Attach an annotation so the Lowerer knows exactly what size this immediate should be emitted as.
                node->createAnnotation<TypeCastAnnotation>(getSemanticContext()->getAnnotPool(),
                                                           nullptr, // Immediates don't have symbols
                                                           targetInstructionType);
            }
        }
        else if (!node->accept(this))
        {
            return false;
        }
    }

    return true;
}

bool TypeCheckVisitor::visit(Label *label) { return label->getCodeScope()->accept(this); }

bool TypeCheckVisitor::visit(Module *module) { return module->getBody()->accept(this); }

bool TypeCheckVisitor::visit(MemoryOperandAstNode *operand)
{
    const std::string_view &dataTypeStr = operand->getReferencedMemoryDataTypeStr();
    if (dataTypeStr.empty())
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Unknown memory operand data-type",
                                        "TypeCheckVisitor::MemoryOperandAstNode",
                                        operand->getSourceRef());
        return false;
    }

    AstNode *base = nullptr, *index = nullptr;
    switch (operand->getMemoryOperandType())
    {
        case MemoryOperandType::BaseDisplacement:
        {
            base = dynamic_cast<BaseDisplacementMemory *>(operand)->getBase();
            return base->accept(this);
        }
        case MemoryOperandType::BaseIndexScaleDisplacement:
        {
            base = dynamic_cast<BaseIndexScaleDisplacementMemory *>(operand)->getBase();
            index = dynamic_cast<BaseIndexScaleDisplacementMemory *>(operand)->getIndex();
            return base->accept(this) && index->accept(this);
        }
        case MemoryOperandType::IndexScale:
        {
            index = dynamic_cast<IndexScaleMemory *>(operand)->getIndex();
            return index->accept(this);
        }
        default:
        {
            // If the memory operand does not have a base / scale (the case of direct), just return true.
            return true;
        }
    }

    // This can't happen.
    return false;
}

bool TypeCheckVisitor::visit(Variable *var)
{
    SymbolAnnotation *annotation = var->getAnnotation<SymbolAnnotation>();
    if (!annotation)
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                "Internal Compiler Error: Variable has no associated symbol, Definition pass failed?",
                "TypeCheckVisitor::Variable",
                var->getSourceRef());
        return false;
    }

    Symbol *symbol = annotation->getSymbol();
    if (symbol->getType() == SymbolType::Label)
    {
        // Labels do not need casting.
        return true;
    }

    DataTypeAnnotation *annot = var->getAnnotation<DataTypeAnnotation>();
    TypedPool *annotPool = getSemanticContext()->getAnnotPool();

    if (!annot)
    {
        // Variable didn't have attached a type, it will use symbol's type.
        annot = var->createAnnotation<DataTypeAnnotation>(annotPool, symbol->getSymbolDataType());
    }

    Type *usedType = annot->getDataType();
    Type *symbolDataType = symbol->getSymbolDataType();

    if (!checkCastSafety(var, symbolDataType, usedType))
    {
        return false;
    }

    if (symbol->getType() == SymbolType::GlobalVariable || symbol->getType() == SymbolType::LocalVariable)
    {
        if (symbolDataType->getUnderlyingType() == UnderlyingType::Void)
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Fatal,
                    std::format("Variables can't be declared with {} type", symbolDataType->getTypeName()),
                    "TypeCheckVisitor::Variable",
                    var->getSourceRef());
            return false;
        }
    }

    var->createAnnotation<TypeCastAnnotation>(annotPool, symbol, usedType);
    return true;
}

bool TypeCheckVisitor::visit(SwitchAstNode *_switch)
{
    if (!_switch->getSwitchVariable()->accept(this))
        return false;

    // Get the type of the switch variable to check all cases against
    SymbolAnnotation *symAnnot = _switch->getSwitchVariable()->getAnnotation<SymbolAnnotation>();
    Type *switchVarType = symAnnot->getSymbol()->getSymbolDataType();

    getSemanticContext()->enterSwitch();
    {
        for (AstNode *ptr : *_switch->getCases())
        {
            SwitchCaseAstNode *_case = dynamic_cast<SwitchCaseAstNode *>(ptr);

            // Check the immediate value against the switch variable's type
            if (!checkImmediateSafety(_case->getCaseValue(), switchVarType))
            {
                return false;
            }

            // Attach the cast annotation so the Lowerer emits the correct size Immediate
            _case->getCaseValue()->createAnnotation<TypeCastAnnotation>(getSemanticContext()->getAnnotPool(),
                                                                        nullptr,
                                                                        switchVarType);

            if (!_case->accept(this))
                return false; // Visit the body
        }
    }
    getSemanticContext()->exitSwitch();

    return true;
}

bool TypeCheckVisitor::visit(SwitchCaseAstNode *_switchCase) { return _switchCase->getBody()->accept(this); }

bool TypeCheckVisitor::visit(WhileAstNode *whileNode)
{
    if (!whileNode->getCondition()->accept(this))
    {
        return false;
    }

    LoopGuard loopGuard(m_ctx); // This will set the context to be inside a loop for the duration of this scope,
                                // allowing break and continue statements.
    return whileNode->getCodeScope()->accept(this);
}

bool TypeCheckVisitor::checkCastSafety(AstNode *node, Type *originalType, Type *usedType)
{
    if (usedType->getTypeName() == originalType->getTypeName())
    {
        // Used type matches the type of the symbol, no cast needed.
        return true;
    }

    // Check if casting is safe.
    if (usedType->getUnderlyingType() == originalType->getUnderlyingType())
    {
        if (usedType->getUnderlyingTypeSize() > originalType->getUnderlyingTypeSize())
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Warning,
                    "Used type is bigger than the original symbol size, this might result in more "
                    "instructions in the final code to expand the value",
                    "TypeCheckVisitor::Variable",
                    node->getSourceRef());
        }
        else if (usedType->getUnderlyingTypeSize() < originalType->getUnderlyingTypeSize())
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Warning,
                    "Used type is smaller than the original symbol size, this might result in a data loss",
                    "TypeCheckVisitor::Variable",
                    node->getSourceRef());
        }
    }
    else
    {
        if (originalType->getUnderlyingType() == UnderlyingType::Void ||
            usedType->getUnderlyingType() == UnderlyingType::Void)
        {
            getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                            std::format("Can't perform a cast from '{}' to '{}'",
                                                        usedType->getTypeName(),
                                                        originalType->getTypeName()),
                                            "TypeCheckVisitor::Variable",
                                            node->getSourceRef());
            return false;
        }

        if (originalType->getUnderlyingType() == UnderlyingType::String)
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Fatal,
                    std::format("Can't perform a cast from string to '{}'", usedType->getTypeName()),
                    "TypeCheckVisitor::Variable",
                    node->getSourceRef());
            return false;
        }

        if (usedType->getUnderlyingType() == UnderlyingType::String)
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Fatal,
                    std::format("Can't perform a cast from '{}' to string", originalType->getTypeName()),
                    "TypeCheckVisitor::Variable",
                    node->getSourceRef());
            return false;
        }

        // Types are different, we need to be cautious.
        getSemanticContext()->emitError(ErrorSeverity::Warning,
                                        std::format("Explicit cast from '{}' to '{}' might cause a data loss",
                                                    originalType->getTypeName(),
                                                    usedType->getTypeName()),
                                        "TypeCheckVisitor::Variable",
                                        node->getSourceRef());
    }
    return true;
}

bool TypeCheckVisitor::checkImmediateSafety(ImmediateOperand *operand, Type *usedType)
{
    size_t targetSize = static_cast<size_t>(usedType->getUnderlyingTypeSize());

    if (auto *strImm = dynamic_cast<StringImmediate *>(operand); strImm)
    {
        if (usedType->getUnderlyingType() != UnderlyingType::String)
        {
            getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                            "Cannot assign a string literal to a non-string target.",
                                            "TypeCheckVisitor::Immediate",
                                            operand->getSourceRef());
            return false;
        }
        return true;
    }

    if (auto *floatImm = dynamic_cast<FloatImmediate *>(operand); floatImm)
    {
        if (usedType->getUnderlyingType() != UnderlyingType::FloatingPoint)
        {
            getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                            "Cannot assign a floating-point literal to an integer target.",
                                            "TypeCheckVisitor::Immediate",
                                            operand->getSourceRef());
            return false;
        }
        return true;
    }

    if (auto *smallInt = dynamic_cast<IntegerImmediate *>(operand); smallInt && targetSize <= 64)
    {
        if (usedType->getUnderlyingType() != UnderlyingType::Integer)
        {
            getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                            "Cannot assign an integer literal to a non-integer target.",
                                            "TypeCheckVisitor::Immediate",
                                            operand->getSourceRef());
            return false;
        }

        if (smallInt->isSigned())
        {
            // Calculate Signed Bounds
            int64_t maxAllowed = (targetSize == 64) ? INT64_MAX : (1ULL << (targetSize - 1)) - 1;
            int64_t minAllowed = (targetSize == 64) ? INT64_MIN : -(1ULL << (targetSize - 1));

            int64_t val = static_cast<int64_t>(mp_get_i64(smallInt->getInteger()));

            if (val > maxAllowed || val < minAllowed)
            {
                getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                std::format("Signed literal '{}' is out of bounds for {}-bit type '{}'",
                                                            val,
                                                            targetSize,
                                                            usedType->getTypeName()),
                                                "TypeCheckVisitor::Immediate",
                                                smallInt->getSourceRef());
                return false;
            }
        }
        else
        {
            uint64_t maxAllowed = (targetSize == 64) ? UINT64_MAX : ((1ULL << targetSize) - 1);
            uint64_t val = mp_get_i64(smallInt->getInteger());

            if (smallInt->isSigned() || val > maxAllowed)
            {
                getSemanticContext()->emitError(
                        ErrorSeverity::Fatal,
                        std::format("Literal is out of bounds or invalid for unsigned {}-bit type '{}'",
                                    targetSize,
                                    usedType->getTypeName()),
                        "TypeCheckVisitor::Immediate",
                        smallInt->getSourceRef());
                return false;
            }
        }
    }
    else if (auto *bigInt = dynamic_cast<IntegerImmediate *>(operand); bigInt)
    {
        mp_int max_val, min_val;
        mp_err err = MP_OKAY; // We need this to avoid warnings about return not being used.

        err = mp_init(&max_val);
        err = mp_init(&min_val);

        if (bigInt->isSigned())
        {
            // Max = 2^(targetSize - 1) - 1
            err = mp_2expt(&max_val, targetSize - 1);
            err = mp_sub_d(&max_val, 1, &max_val);

            // Min = -(2^(targetSize - 1))
            err = mp_2expt(&min_val, targetSize - 1);
            err = mp_neg(&min_val, &min_val);
        }
        else
        {
            // Max = 2^targetSize - 1
            err = mp_2expt(&max_val, targetSize);
            err = mp_sub_d(&max_val, 1, &max_val);

            // Min = 0
            mp_zero(&min_val);
        }

        mp_int *val = bigInt->getInteger();
        bool isOutOfBounds = false;

        // Compare: val > max_val OR val < min_val
        if (mp_cmp(val, &max_val) == MP_GT || mp_cmp(val, &min_val) == MP_LT)
        {
            isOutOfBounds = true;
        }

        mp_clear(&max_val);
        mp_clear(&min_val);

        if (isOutOfBounds)
        {
            getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                            std::format("BigInteger literal is too large to fit in {}-bit type '{}'",
                                                        targetSize,
                                                        usedType->getTypeName()),
                                            "TypeCheckVisitor::Immediate",
                                            bigInt->getSourceRef());
            return false;
        }
    }
    return true;
}
