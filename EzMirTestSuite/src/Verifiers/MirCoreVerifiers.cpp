#include "Verifiers/MirCoreVerifiers.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h
#include "Instruction/MirInstruction.h"
#include "Operand/MirOperands.h"

MirTypeVerifier::MirTypeVerifier(MirType *type) : MirVerifier(type) {}

MirTypeVerifier &MirTypeVerifier::id(size_t id)
{
    EXPECT_EQ(getTestedObj()->getId(), id);
    return *this;
}

MirTypeVerifier &MirTypeVerifier::name(const std::string_view &name)
{
    EXPECT_EQ(getTestedObj()->getName(), name);
    return *this;
}

MirTypeVerifier &MirTypeVerifier::typeKind(MirTypeKind kind)
{
    EXPECT_EQ(getTestedObj()->getKind(), kind);
    return *this;
}

MirTypeVerifier &MirTypeVerifier::arrayType(MirType *type)
{
    EXPECT_NE(getTestedObj()->getArrayElementType(), nullptr);
    EXPECT_EQ(getTestedObj()->getArrayElementType()->getId(), type->getId());
    return *this;
}

MirTypeVerifier &MirTypeVerifier::subTypes(const std::vector<MirType *> &types)
{
    const auto &typeList = getTestedObj()->getSubTypes();
    EXPECT_EQ(types.size(), typeList.size());

    for (size_t i = 0; i < types.size(); i++)
    {
        if (types[i] != nullptr)
        {
            // Tracks which specific subtype element failed within the array match
            SCOPED_TRACE("MirTypeVerifier::subTypes - Comparing subtype index " + std::to_string(i));
            EXPECT_EQ(types[i]->getId(), typeList[i]->getId());
        }
    }

    return *this;
}

MirOperandVerifier::MirOperandVerifier(MirOperand *testedOperand) : MirVerifier(testedOperand) {}

MirTypeVerifier MirOperandVerifier::mirTypeVerifier()
{
    EXPECT_NE(getTestedObj()->getMirType(), nullptr);
    return MirTypeVerifier(getTestedObj()->getMirType());
}

MirOperandVerifier &MirOperandVerifier::type(MirOperandType expectedType)
{
    EXPECT_EQ(getTestedObj()->getType(), expectedType);
    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyDouble(double val)
{
    type(MirOperandType::FloatingPoint);
    EXPECT_TRUE(m_testedObj->get<MirFloat>()->getValue() == FlexFloat(val));
    EXPECT_EQ(m_testedObj->get<MirFloat>()->getMirType()->getName(), "f64");

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyFloat(float val)
{
    type(MirOperandType::FloatingPoint);
    EXPECT_TRUE(m_testedObj->get<MirFloat>()->getValue() == FlexFloat(val));
    EXPECT_EQ(m_testedObj->get<MirFloat>()->getMirType()->getName(), "f32");

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyFloatAnySize(MirType *floatType, const std::string &val)
{
    type(MirOperandType::FloatingPoint);
    EXPECT_EQ(m_testedObj->get<MirFloat>()->getMirType()->getId(), floatType->getId());
    EXPECT_TRUE(m_testedObj->get<MirFloat>()->getValue() == FlexFloat(val, floatType->getTotalSizeInBits()));

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyInteger(MirType *intType, int64_t val)
{
    type(MirOperandType::Integer);
    EXPECT_EQ(m_testedObj->get<MirInteger>()->getValue(), FlexInt(val));

    if (intType)
    {
        EXPECT_EQ(m_testedObj->get<MirInteger>()->getMirType()->getId(), intType->getId());
    }

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyInteger(MirType *intType, const FlexInt &val)
{
    type(MirOperandType::Integer);
    EXPECT_EQ(m_testedObj->get<MirInteger>()->getValue(), val);

    if (intType)
    {
        EXPECT_EQ(m_testedObj->get<MirInteger>()->getMirType()->getId(), intType->getId());
    }

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyReference(size_t refId, MirReferenceType expectedRefType)
{
    type(MirOperandType::Reference);
    MirReference *ref = m_testedObj->get<MirReference>();

    if (refId != MIRID_INVALID)
    {
        EXPECT_EQ(ref->getRefId(), refId);
    }

    if (expectedRefType != MirReferenceType::Invalid)
    {
        EXPECT_EQ(ref->getRefType(), expectedRefType);
    }

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyRegister(MirType *mirType, bool isVirtual, size_t id)
{
    type(MirOperandType::Register);
    MirRegister *reg = getTestedObj()->get<MirRegister>();

    if (mirType)
    {
        EXPECT_NE(reg->getMirType(), nullptr);
        EXPECT_EQ(mirType->getId(), reg->getMirType()->getId());
    }

    EXPECT_EQ(reg->isVirtual(), isVirtual);

    if (id != MIRID_INVALID)
    {
        EXPECT_EQ(reg->getRegId(), id);
    }

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyRuntimeSymbol(const std::string &symbolName)
{
    type(MirOperandType::RuntimeSymbol);
    MirRuntimeSymbol *sym = getTestedObj()->get<MirRuntimeSymbol>();

    if (!symbolName.empty())
        EXPECT_STREQ(symbolName.c_str(), sym->getSymbolName().c_str());

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyFrameIndex(MirType *mirType, size_t frameId)
{
    type(MirOperandType::FrameIndex);
    MirFrameIndex *frameIdx = getTestedObj()->get<MirFrameIndex>();

    if (mirType)
    {
        EXPECT_NE(frameIdx->getMirType(), nullptr);
        EXPECT_EQ(mirType->getId(), frameIdx->getMirType()->getId());
    }

    if (frameId != MIRID_INVALID)
    {
        EXPECT_EQ(frameIdx->getFrameId(), frameId);
    }

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyMemory(MirType *mirType, MirRegister *base, MirInteger *displ)
{
    type(MirOperandType::Memory);
    MirMemory *mem = getTestedObj()->get<MirMemory>();

    if (mirType)
    {
        EXPECT_NE(mem->getMirType(), nullptr);
        EXPECT_EQ(mirType->getId(), mem->getMirType()->getId());
    }

    if (base)
    {
        SCOPED_TRACE("MirOperandVerifier::verifyMemory - Verifying Base Register");
        EXPECT_NE(mem->getBase(), nullptr);
        if (mem->getBase() != nullptr)
        {
            MirOperandVerifier baseVerifier(mem->getBase());
            baseVerifier.verifyRegister(base->getMirType(), base->isVirtual(), base->getRegId());
        }
    }

    if (displ)
    {
        SCOPED_TRACE("MirOperandVerifier::verifyMemory - Verifying Displacement Value");
        EXPECT_NE(mem->getDisplacement(), nullptr);
        if (mem->getDisplacement() != nullptr)
        {
            MirOperandVerifier baseVerifier(mem->getDisplacement());
            baseVerifier.verifyInteger(nullptr, displ->getValue());
        }
    }

    return *this;
}

MirInstructionVerifier::MirInstructionVerifier(MirInstruction *instr) : MirVerifier(instr) {}

MirInstructionVerifier &MirInstructionVerifier::opcode(MirInstructionOpCode opcode)
{
    EXPECT_EQ(getTestedObj()->getOpCode(), opcode);
    return *this;
}

MirInstructionVerifier &MirInstructionVerifier::operandCount(size_t operandCount)
{
    EXPECT_EQ(getTestedObj()->getOperands().size(), operandCount);
    return *this;
}

MirInstructionVerifier &MirInstructionVerifier::targetDesc(MirTargetInstructionDesc *desc)
{
    EXPECT_EQ(getTestedObj()->getTargetDesc()->getId(), desc->getId());
    return *this;
}

MirOperandVerifier MirInstructionVerifier::operandVerifier(size_t operandIndex)
{
    MirInstruction *instr = getTestedObj();
    // Tells you which exact index went out of bounds or failed within the sequence
    SCOPED_TRACE("MirInstructionVerifier::operandVerifier - Index: " + std::to_string(operandIndex));

    EXPECT_FALSE(instr->getOperands().size() <= operandIndex);
    return MirOperandVerifier(instr->getOperands()[operandIndex]);
}

MirBlockVerifier::MirBlockVerifier(MirBlock *block) : MirVerifier(block) {}

MirBlockVerifier &MirBlockVerifier::id(size_t id)
{
    EXPECT_EQ(getTestedObj()->getId(), id);
    return *this;
}

MirBlockVerifier &MirBlockVerifier::instrCount(size_t count)
{
    EXPECT_EQ(getTestedObj()->getInstructions().size(), count);
    return *this;
}

MirBlockVerifier &MirBlockVerifier::instrCountOfType(size_t count, MirInstructionOpCode opcode)
{
    size_t counted = 0;
    for (auto instr : getTestedObj()->getInstructions())
    {
        if (instr->getOpCode() == opcode)
            counted++;
    }

    // Identifies the instruction type context during a counting failure
    SCOPED_TRACE("MirBlockVerifier::instrCountOfType - Matching OpCode: " + std::to_string(static_cast<int>(opcode)));
    EXPECT_EQ(counted, count);
    return *this;
}

MirFunctionStackFrameVerifier::MirFunctionStackFrameVerifier(MirFunctionStackFrame *stackFrame) :
    MirVerifier(stackFrame)
{
}

MirFunctionStackFrameVerifier &MirFunctionStackFrameVerifier::checkStackFrameObj(size_t id,
                                                                                 int64_t offset,
                                                                                 MirType *type,
                                                                                 StackFrameObjectSource source)
{
    StackFrameObject *stackFrame = getTestedObj()->getObjectFromId(id);
    EXPECT_NE(stackFrame, nullptr);

    if (offset != -1)
    {
        EXPECT_EQ(stackFrame->m_offset, offset);
    }

    if (type != nullptr)
    {
        EXPECT_EQ(stackFrame->m_type->getId(), type->getId());
    }

    if (source != StackFrameObjectSource::Invalid)
    {
        EXPECT_EQ(stackFrame->m_source, source);
    }

    return *this;
}
MirFunctionStackFrameVerifier &MirFunctionStackFrameVerifier::stackFrameObjCount(size_t count)
{
    EXPECT_EQ(getTestedObj()->getAllocatedObjectCount(), count);
    return *this;
}

MirFunctionVerifier::MirFunctionVerifier(MirFunction *func) : MirVerifier(func) {}

MirFunctionVerifier &MirFunctionVerifier::id(size_t id)
{
    EXPECT_EQ(getTestedObj()->getId(), id);
    return *this;
}

MirFunctionVerifier &MirFunctionVerifier::name(const std::string_view &name)
{
    EXPECT_EQ(getTestedObj()->getName(), name);
    return *this;
}

MirFunctionVerifier &MirFunctionVerifier::blockCount(size_t count)
{
    EXPECT_EQ(getTestedObj()->getBlocks().size(), count);
    return *this;
}

MirBlockVerifier MirFunctionVerifier::blockVerifier(size_t id)
{
    // Highlights which basic block lookup caused the crash or validation mismatch
    SCOPED_TRACE("MirFunctionVerifier::blockVerifier - Looking up Block MIR ID: " + std::to_string(id));

    MirBlock *block = getTestedObj()->getBlock(id);
    EXPECT_NE(block, nullptr);

    return MirBlockVerifier(block);
}

MirFunctionVerifier &MirFunctionVerifier::paramCount(size_t count)
{
    EXPECT_EQ(getTestedObj()->getParameters().size(), count);
    return *this;
}

MirFunctionVerifier &MirFunctionVerifier::paramType(const std::vector<size_t> &expectedParamTypeList)
{
    const auto &params = getTestedObj()->getParameters();
    paramCount(expectedParamTypeList.size());

    auto it = params.begin();
    for (size_t i = 0; i < expectedParamTypeList.size(); i++, it++)
    {
        size_t expectedTypeId = expectedParamTypeList[i];
        if (expectedTypeId != MIRID_INVALID)
        {
            MirRegister *reg = (*it);
            EXPECT_NE(reg, nullptr);
            EXPECT_EQ(reg->getMirType()->getId(), expectedTypeId);
        }
    }

    return *this;
}
MirFunctionStackFrameVerifier MirFunctionVerifier::stackFrameVerifier()
{
    return MirFunctionStackFrameVerifier(getTestedObj()->getStackFrame());
}

MirGlobalVarVerifier::MirGlobalVarVerifier(MirGlobalVar *globalVar) : MirVerifier<MirGlobalVar>(globalVar) {}

MirGlobalVarVerifier &MirGlobalVarVerifier::id(MirId expectedId)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->getId(), expectedId);
    }
    return *this;
}

MirGlobalVarVerifier &MirGlobalVarVerifier::name(const std::string_view &expectedName)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->getName(), expectedName);
    }
    return *this;
}

MirGlobalVarVerifier &MirGlobalVarVerifier::type(MirType *expectedType)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->getType(), expectedType);
    }
    return *this;
}

MirGlobalVarVerifier &MirGlobalVarVerifier::constant(bool expectedConstant)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->isConstant(), expectedConstant);
    }
    return *this;
}

MirGlobalVarVerifier &MirGlobalVarVerifier::linkage(MirGlobalVarLinkage expectedLinkage)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->getLinkage(), expectedLinkage);
    }
    return *this;
}

MirGlobalVarVerifier &MirGlobalVarVerifier::initializer(MirOperand *initializer)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(initializer->getType(), m_testedObj->getInitializer()->getType())
                << "Initializer type mismatch for global variable " << m_testedObj->getName();

        EXPECT_EQ(initializer->getMirType(), m_testedObj->getInitializer()->getMirType())
                << "Initializer mir type mismatch for global variable " << m_testedObj->getName();
    }
    return *this;
}

MirGlobalVarVerifier &MirGlobalVarVerifier::zeroInitialized()
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_TRUE(m_testedObj->getInitializer() == nullptr)
                << "Expected global variable " << m_testedObj->getName() << " to be zero-initialized (empty initData).";
    }
    return *this;
}

MirClassVerifier::MirClassVerifier(MirClass *_class) : MirVerifier<MirClass>(_class) {}

MirClassVerifier &MirClassVerifier::className(const std::string_view &expectedName)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->getName(), expectedName);
    }
    return *this;
}

MirClassVerifier &MirClassVerifier::classType(MirType *expectedType)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->getType(), expectedType);
    }
    return *this;
}

MirClassVerifier &MirClassVerifier::parentClass(MirClass *expectedParent)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->getParentClass(), expectedParent);
    }
    return *this;
}

MirClassVerifier &MirClassVerifier::fieldCount(size_t count)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->getFields().size(), count);
    }
    return *this;
}

MirClassVerifier &MirClassVerifier::verifyField(size_t fieldIdx,
                                                const std::string_view &expectedName,
                                                MirType *expectedType,
                                                int64_t expectedOffset)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        const auto &fields = m_testedObj->getFields();
        EXPECT_LT(fieldIdx, fields.size()) << "Field index out of bounds on class " << m_testedObj->getName();

        const auto &field = fields[fieldIdx];
        EXPECT_EQ(field->m_name, expectedName);
        if (expectedType)
        {
            EXPECT_EQ(field->m_type, expectedType);
        }
        if (expectedOffset != -1)
        {
            EXPECT_EQ(field->m_offset, static_cast<uint64_t>(expectedOffset));
        }
    }
    return *this;
}

MirClassVerifier &MirClassVerifier::vTableSize(size_t expectedSlotCount)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        EXPECT_EQ(m_testedObj->getVTable().size(), expectedSlotCount);
    }
    return *this;
}

MirClassVerifier &MirClassVerifier::vTableSlot(size_t slotIndex, MirFunction *expectedFunc)
{
    EXPECT_NE(m_testedObj, nullptr);
    if (m_testedObj)
    {
        const auto &vTable = m_testedObj->getVTable();
        EXPECT_LT(slotIndex, vTable.size()) << "VTable slot index out of bounds on class " << m_testedObj->getName();
        EXPECT_EQ(vTable[slotIndex]->m_func->getId(), expectedFunc->getId());
    }
    return *this;
}