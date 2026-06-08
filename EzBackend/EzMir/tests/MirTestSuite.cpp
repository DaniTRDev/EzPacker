#include "MirTestSuite.h"

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

MirOperandVerifier &MirOperandVerifier::verifyDouble(MirType *doubleType, double val)
{
    type(MirOperandType::Double);
    EXPECT_EQ(m_testedObj->get<MirDouble>()->getValue(), val);

    if (doubleType)
    {
        EXPECT_EQ(m_testedObj->get<MirDouble>()->getMirType()->getId(), doubleType->getId());
    }

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyInteger(MirType *intType, int64_t val)
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

MirInstructionVerifier &MirInstructionVerifier::targetId(MirTargetInstructionId id)
{
    EXPECT_EQ(getTestedObj()->getTargetId(), id);
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

MirFunctionStackFrameVerifier &MirFunctionStackFrameVerifier::checkStackFrameObj(
        size_t id, int64_t offset, size_t align, size_t sizeInBytes, StackFrameObjectSource source)
{
    StackFrameObject *stackFrame = getTestedObj()->getObjectFromId(id);
    if (!stackFrame)
    {
        return *this;
    }

    if (offset != -1)
    {
        EXPECT_EQ(stackFrame->m_offset, offset);
    }

    if (offset != -1)
    {
        EXPECT_EQ(stackFrame->m_align, align);
    }

    if (offset != -1)
    {
        EXPECT_EQ(stackFrame->m_sizeInBytes, sizeInBytes);
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

CodeFlowAnalysisVerifier::CodeFlowAnalysisVerifier(CodeFlowAnalysis *analysis, MirBuilderContext *ctx) :
    m_ctx(ctx), MirPassVerifier(analysis)
{
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::predecessor(size_t toId, size_t fromId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_predecessors.find(toId);

    EXPECT_TRUE(res.m_predecessors.contains(fromId));
    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::predecessorCount(size_t blockId, size_t count)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_predecessors.find(blockId);

    EXPECT_NE(it, res.m_predecessors.end());
    EXPECT_EQ(it->second.size(), count);

    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::reachable(size_t start, size_t end)
{
    const auto &res = getTestedObj()->getResult();

    // Lambda for Depth-First Search traversal
    // We use a tracking set passed by reference to handle cycles safely
    std::function<bool(size_t, size_t, std::unordered_set<size_t> &)> dfs =
            [&res, &dfs](size_t current, size_t target, std::unordered_set<size_t> &visited) -> bool
    {
        // We found our target destination block
        if (current == target)
            return true;

        // We hit a block we've already evaluated (prevents infinite loop in cycles)
        if (visited.count(current))
            return false;

        // Mark current block as processed
        visited.insert(current);

        // Look up successors for the current block
        auto it = res.m_successors.find(current);
        if (it == res.m_successors.end())
            return false; // Dead end / Sink block

        // Recursively check all outgoing control flow branches
        for (const auto &successor : it->second)
        {
            // If any path leads to the target block, cascade a success back up
            if (dfs(successor, target, visited))
                return true;
        }

        return false;
    };

    std::unordered_set<size_t> visited;
    bool isReachable = dfs(start, end, visited);

    EXPECT_TRUE(isReachable) << "Block " << end << " is expected to be reachable from Block " << start
                             << ", but no continuous path was found.";

    return *this;
}
CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::successor(size_t fromId, size_t toId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_successors.find(fromId);

    EXPECT_TRUE(it->second.contains(toId));
    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::successorCount(size_t blockId, size_t count)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_successors.find(blockId);

    EXPECT_NE(it, res.m_successors.end());
    EXPECT_EQ(it->second.size(), count);

    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::unreachable(size_t start, size_t end)
{
    const auto &res = getTestedObj()->getResult();

    // Lambda for Depth-First Search traversal
    // We use a tracking set passed by reference to handle cycles safely
    std::function<bool(size_t, size_t, std::unordered_set<size_t> &)> dfs =
            [&res, &dfs](size_t current, size_t target, std::unordered_set<size_t> &visited) -> bool
    {
        // We found our target destination block
        if (current == target)
            return true;

        // We hit a block we've already evaluated (prevents infinite loop in cycles)
        if (visited.count(current))
            return false;

        // Mark current block as processed
        visited.insert(current);

        // Look up successors for the current block
        auto it = res.m_successors.find(current);
        if (it == res.m_successors.end())
            return false; // Dead end / Sink block

        // Recursively check all outgoing control flow branches
        for (const auto &successor : it->second)
        {
            // If any path leads to the target block, cascade a success back up
            if (dfs(successor, target, visited))
                return true;
        }

        return false;
    };

    std::unordered_set<size_t> visited;
    bool isReachable = dfs(start, end, visited);

    EXPECT_FALSE(isReachable) << "Block " << end << " is expected to be unreachable from Block " << start;
    return *this;
}

CodeFlowAnalysisVerifier &CodeFlowAnalysisVerifier::exitBlock(size_t blockId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_successors.find(blockId);

    EXPECT_NE(it, res.m_successors.end());
    EXPECT_EQ(it->second.size(), 0);

    return *this;
}

MirBuilderContext *MirTestSuite::getBuilderCtx() { return m_builderCtx.get(); }

MirFunction *MirTestSuite::getTestFunc() { return m_testFunction; }

MirInstructionInsertionPoint *MirTestSuite::getTestInsertionPoint() { return &m_insertPoint; }

MirPrinter MirTestSuite::getPrinter() { return MirPrinter(); }

MirTypeTable *MirTestSuite::getTypeTable() { return m_typeTable.get(); }

void MirTestSuite::create(const std::filesystem::path &workingPath)
{
    m_diagCollector = std::make_shared<DiagnosticCollector>();
    m_typeTable = std::make_shared<MirTypeTable>(&m_arena);
    m_builderCtx = std::make_shared<MirBuilderContext>(&m_arena, m_diagCollector, m_typeTable);
    m_sourceManager = std::make_shared<SourceManager>(workingPath);
    m_diagLogger = std::make_shared<DiagnosticLogger>(m_sourceManager.get());

    m_diagCollector->addListener(m_diagLogger.get());
    m_typeTable->initialize();
    m_testFunction = MirFunctionBuilder(m_builderCtx.get()).build(m_typeTable->getVoidType(), nullptr, {}, "TEST");

    if (!m_testFunction)
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirTestSuite")
                << "The creation of the test function failed!";
    }

    MirBlock *entryPoint = m_testFunction->getEntryPoint();

    m_insertPoint = { .m_type = InsertionType::Append,
                      .m_block = entryPoint,
                      .m_iterator = entryPoint->getInstructions().begin() };
}

void MirTestSuite::destroy()
{
    m_builderCtx.reset();
    m_typeTable.reset();
    m_sourceManager.reset();
    m_diagCollector.reset();
}

void MirTestSuiteAsGtest::SetUp()
{
    MirTestSuite::create(std::filesystem::current_path());
    Test::SetUp();
}

void MirTestSuiteAsGtest::TearDown()
{
    MirTestSuite::destroy();
    Test::TearDown();
}
