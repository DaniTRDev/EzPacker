#ifndef EZPACKER_PARSERSTESTFIXTURE_H
#define EZPACKER_PARSERSTESTFIXTURE_H

#include "EzLexer.h"
#include <gtest/gtest.h>

class ParsersTestFixture : public ::testing::Test
{
  public:
    /**
     * Expects the resulting node to be of the given type. Returns true if node is valid and its type matches 'type'.
     * @param type
     * @return bool
     */
    bool expectNodeType(AstNodeType type);

    /**
     * Tries to parse the tokenized input (tokenized when called tokenizeAndCreateContext with the given parser. Parse
     * result can be retrieved with "getParseResult".
     * @tparam ParserType
     * @param input
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
     * Creates the test fixture by instantiating the tokenizer, source logger, source manager, logger and parsing
     * context.
     */
    void SetUp() override;

    /**
     * Destroys the instances of tokenizer, source sink logger, source manager, logger and parsing context.
     */
    void TearDown() override;

    /**
     * Tokenizes the given input and creates a SingleThreadedParsingContext. Tokenize won't be checked since it
     * must have been checked previously.
     * @param input
     */
    void tokenizeAndCreateContext(const std::string &input);

    /**
     * Returns the resulting AstNode after parsing.
     * @return const std::shared_ptr<AstNode> &
     */
    const std::shared_ptr<AstNode> &getParseResult();

    /**
     * Expects the parse result not to be null and to be able to be casted into AstNodeType. Will return true if
     * both conditions fulfill and out will be set to the casted node.
     * @tparam AstNodeType
     * @param out
     * @return bool
     */
    template <typename AstNodeType> bool expectNodeCast(std::shared_ptr<AstNodeType> &out)
    {
        std::shared_ptr<AstNodeType> casted = std::dynamic_pointer_cast<AstNodeType>(m_parseResult);

        if (!casted)
            return false;

        out = std::move(casted);
        return true;
    }

  private:
    std::shared_ptr<AstNode> m_parseResult;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<BasicParsingContext> m_parsingContext;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SourceLoggingSink> m_sourceSinkLogger;
    std::shared_ptr<SyncLogger> m_logger;
};

template <typename IntegerValueT>
inline constexpr auto TEST_INTEGER_IMMEDIATE_IMPL = [](IntegerValueT value, ParsersTestFixture *fixture)
{
    std::shared_ptr<ImmediateOperand> operand;
    EXPECT_TRUE(fixture->expectNodeCast<>(operand));

    EXPECT_EQ(operand->getType(), AstNodeType::Immediate);
    EXPECT_EQ(operand->getImmediateType(), ImmediateType::Integer);

    std::shared_ptr<IntegerImmediate> integer;
    EXPECT_TRUE(fixture->expectNodeCast<>(integer));
    EXPECT_EQ(mp_get_mag_u64(integer->getInteger().get()), value);
};

inline constexpr auto TEST_BIG_INTEGER_IMMEDIATE_IMPL = [](mp_int *value, ParsersTestFixture *fixture)
{
    std::shared_ptr<ImmediateOperand> operand;
    EXPECT_TRUE(fixture->expectNodeCast<>(operand));

    EXPECT_EQ(operand->getType(), AstNodeType::Immediate);
    EXPECT_EQ(operand->getImmediateType(), ImmediateType::Integer);

    std::shared_ptr<IntegerImmediate> integer;
    EXPECT_TRUE(fixture->expectNodeCast<>(integer));
    EXPECT_EQ(mp_cmp(value, integer->getInteger().get()), MP_EQ);
};

inline constexpr auto TEST_FLOAT_IMMEDIATE_IMPL = [](double value, ParsersTestFixture *fixture)
{
    std::shared_ptr<ImmediateOperand> operand;
    EXPECT_TRUE(fixture->expectNodeCast<>(operand));

    EXPECT_EQ(operand->getType(), AstNodeType::Immediate);
    EXPECT_EQ(operand->getImmediateType(), ImmediateType::FloatingPoint);

    std::shared_ptr<FloatImmediate> _float;
    EXPECT_TRUE(fixture->expectNodeCast<>(_float));
    EXPECT_EQ(_float->getFloatingValue(), value);
};

inline constexpr auto TEST_STRING_IMMEDIATE_IMPL = [](std::string value, ParsersTestFixture *fixture)
{
    std::shared_ptr<ImmediateOperand> operand;
    EXPECT_TRUE(fixture->expectNodeCast<>(operand));

    EXPECT_EQ(operand->getType(), AstNodeType::Immediate);
    EXPECT_EQ(operand->getImmediateType(), ImmediateType::String);

    std::shared_ptr<StringImmediate> string;
    EXPECT_TRUE(fixture->expectNodeCast<>(string));
    EXPECT_EQ(string->getStr(), value);
};

inline constexpr auto TEST_VARIABLE_IMPL = [](bool isArray,
                                              std::string type,
                                              std::string name,
                                              std::vector<AstNodeType> initializersTypes,
                                              ParsersTestFixture *fixture)
{
    std::shared_ptr<Variable> variable;
    EXPECT_TRUE(fixture->expectNodeCast<>(variable));

    EXPECT_EQ(variable->getIsArray(), isArray);
    EXPECT_EQ(variable->getVariableDataType(), type);
    EXPECT_EQ(variable->getVariableName(), name);

    auto &initializers = variable->getInitializers();
    EXPECT_EQ(initializers.size(), initializersTypes.size());

    for (size_t i = 0; i < initializers.size(); i++)
    {
        auto &initializer = initializers[i];
        auto &expectedInitializerType = initializersTypes[i];

        EXPECT_EQ(initializer->getType(), expectedInitializerType);
    }
};

inline constexpr auto TEST_MEMORY_BASE_DISPL_IMPL =
        [](AstNodeType expectedBaseType, AstNodeType expectedDisplType, ParsersTestFixture *fixture)
{
    std::shared_ptr<BaseDisplacementMemory> memory;
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
        [](AstNodeType expectedBaseType, AstNodeType expectedDisplType, ParsersTestFixture *fixture)
{
    std::shared_ptr<IndexScaleMemory> memory;
    EXPECT_TRUE(fixture->expectNodeCast<>(memory));

    if (expectedBaseType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getIndex(), nullptr);
        EXPECT_EQ(memory->getIndex()->getType(), expectedBaseType);
    }

    if (expectedDisplType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getScalingFactor(), nullptr);
        EXPECT_EQ(memory->getScalingFactor()->getType(), expectedDisplType);
    }
};

inline constexpr auto TEST_MEMORY_BASE_INDEX_SCALE_DISPL_IMPL = [](AstNodeType expectedBaseType,
                                                                   AstNodeType expectedIndexType,
                                                                   AstNodeType expectedScaleType,
                                                                   AstNodeType expectedDisplType,
                                                                   ParsersTestFixture *fixture)
{
    std::shared_ptr<BaseIndexScaleDisplacementMemory> memory;
    EXPECT_TRUE(fixture->expectNodeCast<>(memory));

    if (expectedBaseType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getIndex(), nullptr);
        EXPECT_EQ(memory->getIndex()->getType(), expectedBaseType);
    }

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

    if (expectedDisplType != AstNodeType::Invalid)
    {
        EXPECT_NE(memory->getScalingFactor(), nullptr);
        EXPECT_EQ(memory->getScalingFactor()->getType(), expectedDisplType);
    }
};

inline constexpr auto TEST_MEMORY_DIRECT_IMPL = [](ParsersTestFixture *fixture)
{
    std::shared_ptr<DirectMemory> memory;
    EXPECT_TRUE(fixture->expectNodeCast<>(memory));

    EXPECT_NE(memory->getAddress(), nullptr);
    EXPECT_EQ(memory->getAddress()->getType(), AstNodeType::Immediate);
};

inline constexpr auto TEST_MEMORY_IMPL =
        [](MemoryOperandType type, std::string referencedMemoryType, ParsersTestFixture *fixture)
{
    std::shared_ptr<MemoryOperandAstNode> memory;
    EXPECT_TRUE(fixture->expectNodeCast<>(memory));
    EXPECT_EQ(memory->getReferencedMemoryDataTypeStr(), referencedMemoryType);
    EXPECT_EQ(memory->getMemoryOperandType(), type);
};

inline constexpr auto TEST_INSTRUCTION_IMPL =
        [](std::string name, std::vector<AstNodeType> operandTypes, ParsersTestFixture *fixture)
{
    std::shared_ptr<Instruction> instr;
    EXPECT_TRUE(fixture->expectNodeCast<>(instr));
    EXPECT_EQ(instr->getInstructionName(), name);

    auto &operands = instr->getOperands();
    EXPECT_EQ(operands.size(), operandTypes.size());

    for (size_t i = 0; i < operands.size(); i++)
    {
        EXPECT_EQ(operands[i]->getType(), operandTypes[i]);
    }
};

inline constexpr auto TEST_CALL_INSTRUCTION_IMPL = [](std::string callee,
                                                      std::string returnType,
                                                      std::vector<AstNodeType> parameterTypes,
                                                      ParsersTestFixture *fixture)
{
    std::shared_ptr<CallInstruction> instr;
    EXPECT_TRUE(fixture->expectNodeCast<>(instr));
    EXPECT_EQ(instr->getReturnType(), returnType);
    EXPECT_EQ(instr->getCalleeName(), callee);
    TEST_INSTRUCTION_IMPL("call", std::move(parameterTypes), fixture);
};

inline constexpr auto TEST_LABEL_IMPL =
        [](std::string name, std::vector<AstNodeType> expectedExpressions, ParsersTestFixture *fixture)
{
    std::shared_ptr<Label> label;
    EXPECT_TRUE(fixture->expectNodeCast<>(label));
    EXPECT_EQ(label->getLabelName(), name);

    auto &expressions = label->getCodeScope()->getExpressions();
    EXPECT_EQ(expressions.size(), expectedExpressions.size());

    size_t i = 0;
    for (auto &[id, expression] : expressions)
    {
        EXPECT_EQ(expression->getType(), expectedExpressions[i]);
        i++;
    }
};

inline constexpr auto TEST_MODULE_HEADER_IMPL =
        [](std::string name, std::string type, std::vector<AstNodeType> parameterTypes, ParsersTestFixture *fixture)
{
    std::shared_ptr<ModuleHeader> header;
    EXPECT_TRUE(fixture->expectNodeCast<>(header));
    EXPECT_EQ(header->getModuleName(), name);
    EXPECT_EQ(header->getReturnTypeName(), type);

    auto &params = header->getParameters();
    EXPECT_EQ(params.size(), parameterTypes.size());

    for (size_t i = 0; i < params.size(); i++)
    {
        auto paramType = params[i]->getType();
        EXPECT_EQ(paramType, parameterTypes[i]);
    }
};

#define TEST_INTEGER_IMMEDIATE(value) TEST_INTEGER_IMMEDIATE_IMPL<typeof(value)>(value, this);
#define TEST_BIG_INTEGER_IMMEDIATE(value) TEST_BIG_INTEGER_IMMEDIATE_IMPL(value, this);
#define TEST_FLOAT_IMMEDIATE(value) TEST_FLOAT_IMMEDIATE_IMPL(value, this);
#define TEST_STRING_IMMEDIATE(value) TEST_STRING_IMMEDIATE_IMPL(value, this);

#define TEST_VARIABLE(isArray, type, name, ...) TEST_VARIABLE_IMPL(isArray, type, name, { __VA_ARGS__ }, this);

#define TEST_MEMORY(type, referencedMemoryType) TEST_MEMORY_IMPL(type, referencedMemoryType, this);
#define TEST_MEMORY_BASE_DISPL(baseAstNodeType, indexAstNodeType)                                                      \
    TEST_MEMORY_BASE_DISPL_IMPL(baseAstNodeType, indexAstNodeType, this);
#define TEST_MEMORY_INDEX_SCALE(baseAstNodeType, indexAstNodeType)                                                     \
    TEST_MEMORY_INDEX_SCALE_IMPL(baseAstNodeType, indexAstNodeType, this);
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
#endif // EZPACKER_PARSERSTESTFIXTURE_H
