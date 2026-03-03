#ifndef EZPACKER_PARSERSTESTFIXTURE_H
#define EZPACKER_PARSERSTESTFIXTURE_H

#include "EzLexer.h"
#include <gtest/gtest.h>

class ParsersTestFixture : public ::testing::Test
{
  public:
    /**
     * Returns the resulting AstNode after parsing.
     * @return AstNode *
     */
    AstNode *getParseResult();

    /**
     * Expects the resulting node to be of the given type. Returns true if node is valid and its type matches 'type'.
     * @param type
     * @return bool
     */
    bool expectNodeType(AstNodeType type);

    /**
     * Tries to parse the tokenized input with the given parser. Parse result can be retrieved with "getParseResult".
     * @tparam ParserType
     * @return bool
     */
    template <typename ParserType> bool expectParse()
    {
        ParserBatch batch;
        batch.addParsersFromTypeList<ParserType>();

        m_parseResult = batch.parse(m_parsingContext).m_node;
        return m_parseResult != nullptr;
    }

    /**
     * Convenience: tokenize, create context, and parse in one shot. Returns true if parsing succeeded.
     * @tparam ParserType
     * @param input
     * @return bool
     */
    template <typename ParserType> bool tokenizeAndParse(const std::string &input)
    {
        tokenizeAndCreateContext(input);
        return expectParse<ParserType>();
    }

    void SetUp() override;
    void TearDown() override;

    /**
     * Tokenizes the given input and creates a BasicParsingContext.
     * @param input
     */
    void tokenizeAndCreateContext(const std::string &input);

    /**
     * Expects the parse result not to be null and to be castable to AstNodeTypeCast. Returns true if both conditions
     * are met and sets 'out'.
     * @tparam AstNodeTypeCast
     * @param out
     * @return bool
     */
    template <typename AstNodeTypeCast> bool expectNodeCast(AstNodeTypeCast &out)
    {
        AstNodeTypeCast casted = dynamic_cast<AstNodeTypeCast>(m_parseResult);
        if (!casted)
            return false;
        out = casted;
        return true;
    }

    // ──────────────────────────────────────────────────────────────
    //  Scope expression helpers  (used by label/module/codescope)
    // ──────────────────────────────────────────────────────────────

    /**
     * Returns the i-th expression from a TypedPoolSlice, cast to T.
     */
    template <typename T> static T *getExprAt(TypedPoolSlice<AstNode> *list, size_t index)
    {
        if (!list || index >= list->m_numElems)
            return nullptr;
        return list->template get<T>(index);
    }

  private:
    AstNode *m_parseResult;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<BasicParsingContext> m_parsingContext;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SourceLoggingSink> m_sourceSinkLogger;
    std::shared_ptr<SyncLogger> m_logger;
};

// =============================================================================
//  Inline helper lambdas – used by macros and directly in tests
// =============================================================================

template <typename IntegerValueT>
inline constexpr auto TEST_INTEGER_IMMEDIATE_IMPL = [](IntegerValueT value, ParsersTestFixture *fixture)
{
    ImmediateOperand *operand;
    EXPECT_TRUE(fixture->expectNodeCast<>(operand));
    EXPECT_EQ(operand->getType(), AstNodeType::Immediate);
    EXPECT_EQ(operand->getImmediateType(), ImmediateType::Integer);

    IntegerImmediate *integer;
    EXPECT_TRUE(fixture->expectNodeCast<>(integer));
    EXPECT_EQ(mp_get_mag_u64(integer->getInteger()), value);
};

inline constexpr auto TEST_BIG_INTEGER_IMMEDIATE_IMPL = [](mp_int *value, ParsersTestFixture *fixture)
{
    ImmediateOperand *operand;
    EXPECT_TRUE(fixture->expectNodeCast<>(operand));
    EXPECT_EQ(operand->getType(), AstNodeType::Immediate);
    EXPECT_EQ(operand->getImmediateType(), ImmediateType::Integer);

    IntegerImmediate *integer;
    EXPECT_TRUE(fixture->expectNodeCast<>(integer));
    EXPECT_EQ(mp_cmp(value, integer->getInteger()), MP_EQ);
};

inline constexpr auto TEST_FLOAT_IMMEDIATE_IMPL = [](double value, ParsersTestFixture *fixture)
{
    ImmediateOperand *operand;
    EXPECT_TRUE(fixture->expectNodeCast<>(operand));
    EXPECT_EQ(operand->getType(), AstNodeType::Immediate);
    EXPECT_EQ(operand->getImmediateType(), ImmediateType::FloatingPoint);

    FloatImmediate *_float;
    EXPECT_TRUE(fixture->expectNodeCast<>(_float));
    EXPECT_EQ(_float->getFloatingValue(), value);
};

inline constexpr auto TEST_STRING_IMMEDIATE_IMPL = [](std::string value, ParsersTestFixture *fixture)
{
    ImmediateOperand *operand;
    EXPECT_TRUE(fixture->expectNodeCast<>(operand));
    EXPECT_EQ(operand->getType(), AstNodeType::Immediate);
    EXPECT_EQ(operand->getImmediateType(), ImmediateType::String);

    StringImmediate *string;
    EXPECT_TRUE(fixture->expectNodeCast<>(string));
    EXPECT_EQ(string->getStr(), value);
};

inline constexpr auto TEST_VARIABLE_IMPL = [](bool isArray,
                                              std::string type,
                                              std::string name,
                                              std::vector<AstNodeType> initializersTypes,
                                              ParsersTestFixture *fixture)
{
    Variable *variable;
    EXPECT_TRUE(fixture->expectNodeCast<>(variable));
    EXPECT_EQ(variable->getIsArray(), isArray);
    EXPECT_EQ(variable->getVariableDataType(), type);
    EXPECT_EQ(variable->getVariableName(), name);

    if (!initializersTypes.empty())
    {
        auto initializers = variable->getExpressions();
        EXPECT_EQ(initializers->m_numElems, initializersTypes.size());

        size_t i = 0;
        for (const void *object : *initializers)
        {
            auto initializer = (AstNode *)object;
            EXPECT_EQ(initializer->getType(), initializersTypes[i]);
            i++;
        }
    }
};

// ─── Memory helpers ──────────────────────────────────────────────────────────

inline constexpr auto TEST_MEMORY_IMPL =
        [](MemoryOperandType type, std::string referencedMemoryType, ParsersTestFixture *fixture)
{
    MemoryOperandAstNode *memory;
    EXPECT_TRUE(fixture->expectNodeCast<>(memory));
    EXPECT_EQ(memory->getReferencedMemoryDataTypeStr(), referencedMemoryType);
    EXPECT_EQ(memory->getMemoryOperandType(), type);
};

inline constexpr auto TEST_MEMORY_BASE_DISPL_IMPL =
        [](AstNodeType expectedBaseType, AstNodeType expectedDisplType, ParsersTestFixture *fixture)
{
    BaseDisplacementMemory *memory;
    EXPECT_TRUE(fixture->expectNodeCast<>(memory));

    if (expectedBaseType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getBase(), nullptr);
        EXPECT_EQ(memory->getBase()->getType(), expectedBaseType);
    }

    if (expectedDisplType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getDisplacement(), nullptr);
        EXPECT_EQ(memory->getDisplacement()->getType(), expectedDisplType);
    }
};

inline constexpr auto TEST_MEMORY_INDEX_SCALE_IMPL =
        [](AstNodeType expectedIndexType, AstNodeType expectedScaleType, ParsersTestFixture *fixture)
{
    IndexScaleMemory *memory;
    EXPECT_TRUE(fixture->expectNodeCast<>(memory));

    if (expectedIndexType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getIndex(), nullptr);
        EXPECT_EQ(memory->getIndex()->getType(), expectedIndexType);
    }

    if (expectedScaleType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getScalingFactor(), nullptr);
        EXPECT_EQ(memory->getScalingFactor()->getType(), expectedScaleType);
    }
};

inline constexpr auto TEST_MEMORY_BASE_INDEX_SCALE_DISPL_IMPL = [](AstNodeType expectedBaseType,
                                                                   AstNodeType expectedIndexType,
                                                                   AstNodeType expectedScaleType,
                                                                   AstNodeType expectedDisplType,
                                                                   ParsersTestFixture *fixture)
{
    BaseIndexScaleDisplacementMemory *memory;
    EXPECT_TRUE(fixture->expectNodeCast<>(memory));

    if (expectedBaseType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getBase(), nullptr);
        EXPECT_EQ(memory->getBase()->getType(), expectedBaseType);
    }

    if (expectedIndexType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getIndex(), nullptr);
        EXPECT_EQ(memory->getIndex()->getType(), expectedIndexType);
    }

    if (expectedScaleType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getScalingFactor(), nullptr);
        EXPECT_EQ(memory->getScalingFactor()->getType(), expectedScaleType);
    }

    if (expectedDisplType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getDisplacement(), nullptr);
        EXPECT_EQ(memory->getDisplacement()->getType(), expectedDisplType);
    }
};

inline constexpr auto TEST_MEMORY_DIRECT_IMPL = [](ParsersTestFixture *fixture)
{
    DirectMemory *memory;
    EXPECT_TRUE(fixture->expectNodeCast<>(memory));
    EXPECT_NE(memory->getAddress(), nullptr);
    EXPECT_EQ(memory->getAddress()->getType(), AstNodeType::Immediate);
};

// ─── Instruction helpers ─────────────────────────────────────────────────────

inline constexpr auto TEST_INSTRUCTION_IMPL =
        [](std::string name, std::vector<AstNodeType> operandTypes, ParsersTestFixture *fixture)
{
    Instruction *instr;
    EXPECT_TRUE(fixture->expectNodeCast<>(instr));
    EXPECT_EQ(instr->getInstructionName(), name);

    auto operands = instr->getExpressions();
    EXPECT_EQ(operands->m_numElems, operandTypes.size());

    size_t i = 0;
    for (const void *object : *operands)
    {
        auto operand = (AstNode *)object;
        EXPECT_EQ(operand->getType(), operandTypes[i]);
        i++;
    }
};

inline constexpr auto TEST_CALL_INSTRUCTION_IMPL = [](std::string callee,
                                                      std::string returnType,
                                                      std::vector<AstNodeType> parameterTypes,
                                                      ParsersTestFixture *fixture)
{
    CallInstruction *instr;
    EXPECT_TRUE(fixture->expectNodeCast<>(instr));
    EXPECT_EQ(instr->getReturnType(), returnType);
    EXPECT_EQ(instr->getCalleeName(), callee);
    TEST_INSTRUCTION_IMPL("call", std::move(parameterTypes), fixture);
};

// ─── Label / Module / Condition / If / While helpers ─────────────────────────

inline constexpr auto TEST_LABEL_IMPL =
        [](std::string name, std::vector<AstNodeType> expectedExpressions, ParsersTestFixture *fixture)
{
    Label *label;
    EXPECT_TRUE(fixture->expectNodeCast<>(label));
    EXPECT_EQ(label->getLabelName(), name);

    auto expressions = label->getCodeScope()->getExpressions();
    EXPECT_EQ(expressions->m_numElems, expectedExpressions.size());

    size_t i = 0;
    for (const void *expression : *expressions)
    {
        AstNode *node = (AstNode *)expression;
        EXPECT_EQ(node->getType(), expectedExpressions[i]);
        i++;
    }
};

inline constexpr auto TEST_MODULE_HEADER_IMPL =
        [](std::string name, std::string type, std::vector<AstNodeType> parameterTypes, ParsersTestFixture *fixture)
{
    ModuleHeader *header;
    EXPECT_TRUE(fixture->expectNodeCast<>(header));
    EXPECT_EQ(header->getModuleName(), name);
    EXPECT_EQ(header->getReturnTypeName(), type);

    auto params = header->getExpressions();
    EXPECT_EQ(params->m_numElems, parameterTypes.size());

    size_t i = 0;
    for (const void *object : *params)
    {
        auto operand = (AstNode *)object;
        EXPECT_EQ(operand->getType(), parameterTypes[i]);
        i++;
    }
};

inline constexpr auto TEST_CONDITION_IMPL =
        [](ConditionComparisonType expectedType,
           AstNodeType expectedLeftType,
           AstNodeType expectedRightType,
           ParsersTestFixture *fixture)
{
    ConditionAstNode *condition;
    EXPECT_TRUE(fixture->expectNodeCast<>(condition));
    EXPECT_EQ(condition->getComparisonType(), expectedType);
    EXPECT_NE(condition->getLeft(), nullptr);
    EXPECT_NE(condition->getRight(), nullptr);
    EXPECT_EQ(condition->getLeft()->getType(), expectedLeftType);
    EXPECT_EQ(condition->getRight()->getType(), expectedRightType);
};

inline constexpr auto TEST_IF_IMPL =
        [](bool hasTrue, bool hasFalse, AstNodeType falseType, ParsersTestFixture *fixture)
{
    IfAstNode *ifNode;
    EXPECT_TRUE(fixture->expectNodeCast<>(ifNode));
    EXPECT_NE(ifNode->getCondition(), nullptr);

    if (hasTrue)
        EXPECT_NE(ifNode->getTrueScope(), nullptr);
    else
        EXPECT_EQ(ifNode->getTrueScope(), nullptr);

    if (hasFalse)
    {
        EXPECT_NE(ifNode->getFalseScope(), nullptr);
        EXPECT_EQ(ifNode->getFalseScope()->getType(), falseType);
    }
    else
    {
        EXPECT_EQ(ifNode->getFalseScope(), nullptr);
    }
};

inline constexpr auto TEST_WHILE_IMPL =
        [](size_t expectedBodyExprCount, ParsersTestFixture *fixture)
{
    WhileAstNode *whileNode;
    EXPECT_TRUE(fixture->expectNodeCast<>(whileNode));
    EXPECT_NE(whileNode->getCondition(), nullptr);
    EXPECT_NE(whileNode->getCodeScope(), nullptr);
    auto exprs = whileNode->getCodeScope()->getExpressions();
    EXPECT_NE(exprs, nullptr);
    EXPECT_EQ(exprs->m_numElems, expectedBodyExprCount);
};

// =============================================================================
//  Convenience macros
// =============================================================================

#define TEST_INTEGER_IMMEDIATE(value) TEST_INTEGER_IMMEDIATE_IMPL<typeof(value)>(value, this);
#define TEST_BIG_INTEGER_IMMEDIATE(value) TEST_BIG_INTEGER_IMMEDIATE_IMPL(value, this);
#define TEST_FLOAT_IMMEDIATE(value) TEST_FLOAT_IMMEDIATE_IMPL(value, this);
#define TEST_STRING_IMMEDIATE(value) TEST_STRING_IMMEDIATE_IMPL(value, this);

#define TEST_VARIABLE(isArray, type, name, ...) TEST_VARIABLE_IMPL(isArray, type, name, { __VA_ARGS__ }, this);

#define TEST_MEMORY(type, referencedMemoryType) TEST_MEMORY_IMPL(type, referencedMemoryType, this);
#define TEST_MEMORY_BASE_DISPL(baseAstNodeType, displAstNodeType)                                                      \
    TEST_MEMORY_BASE_DISPL_IMPL(baseAstNodeType, displAstNodeType, this);
#define TEST_MEMORY_INDEX_SCALE(indexAstNodeType, scaleAstNodeType)                                                    \
    TEST_MEMORY_INDEX_SCALE_IMPL(indexAstNodeType, scaleAstNodeType, this);
#define TEST_MEMORY_BASE_INDEX_SCALE_DISPL(expectedBaseType, expectedIndexType, expectedScaleType, expectedDisplType)  \
    TEST_MEMORY_BASE_INDEX_SCALE_DISPL_IMPL(expectedBaseType,                                                          \
                                            expectedIndexType,                                                         \
                                            expectedScaleType,                                                         \
                                            expectedDisplType,                                                         \
                                            this)
#define TEST_MEMORY_DIRECT() TEST_MEMORY_DIRECT_IMPL(this);

#define TEST_INSTRUCTION(name, ...) TEST_INSTRUCTION_IMPL(name, { __VA_ARGS__ }, this);
#define TEST_CALL_INSTRUCTION(calleeName, returnType, ...)                                                             \
    TEST_CALL_INSTRUCTION_IMPL(calleeName, returnType, { __VA_ARGS__ }, this);

#define TEST_LABEL(name, ...) TEST_LABEL_IMPL(name, { __VA_ARGS__ }, this);
#define TEST_MODULE_HEADER(name, type, ...) TEST_MODULE_HEADER_IMPL(name, type, { __VA_ARGS__ }, this);

#define TEST_CONDITION(compType, leftType, rightType) TEST_CONDITION_IMPL(compType, leftType, rightType, this);
#define TEST_IF(hasTrue, hasFalse, falseType) TEST_IF_IMPL(hasTrue, hasFalse, falseType, this);
#define TEST_WHILE(bodyExprCount) TEST_WHILE_IMPL(bodyExprCount, this);

#endif // EZPACKER_PARSERSTESTFIXTURE_H
