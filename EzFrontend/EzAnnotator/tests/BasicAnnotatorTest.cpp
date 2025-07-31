/*
 * This helps certain IDEs like CLion to identify all targets that compile this file as a GTest target, which is useful
 * to be able to run the tests directly with the target selector.
 */
#include <gtest/gtest.h>
#include "BasicAnnotatorTest.h"

bool BasicAnnotatorTest::expectConstantFloatValue(float value, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<ConstantAnnotation> annot = std::dynamic_pointer_cast<ConstantAnnotation>(node->getAnnotation());
    if (!annot || !annot->isFloat() || annot->isInteger() || annot->isString())
        return false;

    return annot->getFloat() == value;
}

bool BasicAnnotatorTest::expectConstantIntegerValue(uint64_t value, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<ConstantAnnotation> annot = std::dynamic_pointer_cast<ConstantAnnotation>(node->getAnnotation());
    if (!annot || annot->isFloat() || !annot->isInteger() || annot->isString())
        return false;

    return mp_get_u64(annot->getInt().get()) == value;
}

bool BasicAnnotatorTest::expectConstantStringValue(const std::string &value, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<ConstantAnnotation> annot = std::dynamic_pointer_cast<ConstantAnnotation>(node->getAnnotation());
    if (!annot || annot->isFloat() || annot->isInteger() || !annot->isString())
        return false;

    return annot->getString() == value;
}

bool BasicAnnotatorTest::expectInstructionId(size_t instrId, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<InstructionAnnotation> annot =
            std::dynamic_pointer_cast<InstructionAnnotation>(node->getAnnotation());
    if (!annot)
        return false;

    return annot->getInstrId() == instrId;
}

bool BasicAnnotatorTest::expectInstrInNode(size_t childId,
                                           const std::string &instrName,
                                           const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    const std::shared_ptr<AstNode> &child = node->getChild(childId);
    if (!child || child->getId() != AstNodes::Instruction().getId())
        return false;

    auto annot = std::dynamic_pointer_cast<InstructionAnnotation>(child->getAnnotation());

    return annot->getInstrId() == g_InstructionTable.at(instrName);
}

bool BasicAnnotatorTest::expectInstructionOperandCount(size_t numOperands, const std::shared_ptr<AstNode> &node)
{
    if (!node || !std::dynamic_pointer_cast<InstructionAnnotation>(node->getAnnotation()))
        return false;

    return node->getChildren().size() == numOperands;
}

bool BasicAnnotatorTest::expectInstructionOperandCC(size_t value, size_t childId, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<AstNode> child = node->getChild(childId);

    if (!child)
        return false;

    return expectConstantIntegerValue(value, child);
}

bool BasicAnnotatorTest::expectInstructionOperandMM(MemoryReferenceType refType,
                                                    size_t childId,
                                                    const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<AstNode> child = node->getChild(childId);

    std::shared_ptr<MemoryRefAnnotation> annot = std::dynamic_pointer_cast<MemoryRefAnnotation>(child->getAnnotation());
    if (!annot)
        return false;

    return annot->getRefType() == refType;
}

bool BasicAnnotatorTest::expectInstructionOperandVV(size_t symbolId,
                                                    size_t typeId,
                                                    size_t childId,
                                                    const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<AstNode> child = node->getChild(childId);
    return expectSymbolId(symbolId, child) && expectSymbolTypeId(typeId, child);
}

bool BasicAnnotatorTest::expectMemoryBase(size_t baseId, size_t typeId, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<MemoryRefAnnotation> annot = std::dynamic_pointer_cast<MemoryRefAnnotation>(node->getAnnotation());
    if (!annot || annot->getRefType() != MemoryReferenceType::Base || annot->getTypeId() != typeId)
        return false;

    return annot->getBaseId() == baseId;
}

bool BasicAnnotatorTest::expectMemoryBaseDispl(size_t baseId,
                                               uint64_t displ,
                                               size_t typeId,
                                               const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<MemoryRefAnnotation> annot = std::dynamic_pointer_cast<MemoryRefAnnotation>(node->getAnnotation());
    if (!annot || annot->getRefType() != MemoryReferenceType::BaseDispl || annot->getTypeId() != typeId)
        return false;

    return annot->getBaseId() == baseId && mp_get_u64(annot->getDispl()->getInt().get()) == displ;
}

bool BasicAnnotatorTest::expectMemoryBaseIndexScaleDispl(size_t baseId,
                                                         size_t indexId,
                                                         uint64_t scale,
                                                         uint64_t displ,
                                                         size_t typeId,
                                                         const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<MemoryRefAnnotation> annot = std::dynamic_pointer_cast<MemoryRefAnnotation>(node->getAnnotation());
    if (!annot || annot->getRefType() != MemoryReferenceType::BaseIndexScaleDispl || annot->getTypeId() != typeId)
        return false;

    return annot->getBaseId() == baseId && annot->getIndexId() == indexId &&
            mp_get_u64(annot->getDispl()->getInt().get()) == displ &&
            mp_get_u64(annot->getScale()->getInt().get()) == scale;
}

bool BasicAnnotatorTest::expectMemoryDirect(size_t memoryAddress, size_t typeId, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<MemoryRefAnnotation> annot = std::dynamic_pointer_cast<MemoryRefAnnotation>(node->getAnnotation());
    if (!annot || annot->getRefType() != MemoryReferenceType::Direct || annot->getTypeId() != typeId)
        return false;

    return mp_get_u64(annot->getDispl()->getInt().get()) == memoryAddress;
}

bool BasicAnnotatorTest::expectMemoryIndexScale(size_t indexId,
                                                uint64_t scale,
                                                size_t typeId,
                                                const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<MemoryRefAnnotation> annot = std::dynamic_pointer_cast<MemoryRefAnnotation>(node->getAnnotation());
    if (!annot || annot->getRefType() != MemoryReferenceType::IndexScale || annot->getTypeId() != typeId)
        return false;

    return annot->getIndexId() == indexId && mp_get_u64(annot->getScale()->getInt().get()) == scale;
}

bool BasicAnnotatorTest::expectModuleArgument(size_t symbolId,
                                              size_t typeId,
                                              size_t argId,
                                              const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<ModuleAnnotation> annot = std::dynamic_pointer_cast<ModuleAnnotation>(node->getAnnotation());
    if (!annot)
        return false;

    size_t argSymbolId = annot->getArgumentSymbolId(argId);
    if (argSymbolId == 0)
        return false;

    std::shared_ptr<ScopedSymbol> symbol = m_symbolTable->getItemAtAnyScope<ScopedSymbol>(argSymbolId);
    return symbol && symbol->getTypeId() == typeId;
}

bool BasicAnnotatorTest::expectModuleArgumentCount(size_t numArgs, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<ModuleAnnotation> annot = std::dynamic_pointer_cast<ModuleAnnotation>(node->getAnnotation());
    return annot && annot->getArgumentSymbolIds().size() == numArgs;
}

bool BasicAnnotatorTest::expectModuleLabelInBody(size_t childId, size_t symbolId, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    const std::shared_ptr<AstNode> &child = node->getChild(childId);
    if (!child || child->getId() != AstNodes::Label().getId())
        return false;

    auto annot = std::dynamic_pointer_cast<LabelAnnotation>(child->getAnnotation());
    return annot && annot->getSymbolId() == symbolId;
}

bool BasicAnnotatorTest::expectModuleReturnType(size_t typeId, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<ModuleAnnotation> annot = std::dynamic_pointer_cast<ModuleAnnotation>(node->getAnnotation());
    return annot && annot->getTypeId() == typeId;
}

bool BasicAnnotatorTest::expectModuleSymbolId(size_t symbolId, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<ModuleAnnotation> annot = std::dynamic_pointer_cast<ModuleAnnotation>(node->getAnnotation());
    return annot && annot->getSymbolId() == symbolId;
}

bool BasicAnnotatorTest::expectSymbolId(size_t id, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<SymbolAnnotation> annot = std::dynamic_pointer_cast<SymbolAnnotation>(node->getAnnotation());
    return annot && annot->getSymbolId() == id;
}

bool BasicAnnotatorTest::expectSymbolTypeId(size_t id, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<SymbolAnnotation> annot = std::dynamic_pointer_cast<SymbolAnnotation>(node->getAnnotation());
    return annot && annot->getTypeId() == id;
}

bool BasicAnnotatorTest::expectTypeId(size_t id, const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    std::shared_ptr<TypeAnnotation> annot = std::dynamic_pointer_cast<TypeAnnotation>(node->getAnnotation());
    return annot && annot->getTypeId() == id;
}

bool BasicAnnotatorTest::expectVariableInitializerCount(size_t count, const std::shared_ptr<AstNode> &node)
{
    return node && node->getChildren().size() == count;
}

bool BasicAnnotatorTest::expectVariableFloatValue(size_t initializerId,
                                                  float value,
                                                  const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    const std::shared_ptr<AstNode> &initializer = node->getChild(initializerId);
    if (!initializer || initializer->getId() != AstNodes::FloatNumber().getId())
        return false;

    const auto &annot = std::dynamic_pointer_cast<ConstantAnnotation>(initializer->getAnnotation());

    return annot && annot->isFloat() && annot->getFloat() == value;
}

bool BasicAnnotatorTest::expectVariableIntValue(size_t initializerId,
                                                size_t value,
                                                const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    const std::shared_ptr<AstNode> &initializer = node->getChild(initializerId);
    if (!initializer || initializer->getId() != AstNodes::IntNumber().getId())
        return false;

    const auto &annot = std::dynamic_pointer_cast<ConstantAnnotation>(initializer->getAnnotation());

    return annot && annot->isInteger() && mp_get_u64(annot->getInt().get()) == value;
}

bool BasicAnnotatorTest::expectVariableStringValue(size_t initializerId,
                                                   const std::string &value,
                                                   const std::shared_ptr<AstNode> &node)
{
    if (!node)
        return false;

    const std::shared_ptr<AstNode> &initializer = node->getChild(initializerId);
    if (!initializer || initializer->getId() != AstNodes::String().getId())
        return false;

    const auto &annot = std::dynamic_pointer_cast<ConstantAnnotation>(initializer->getAnnotation());
    return annot && annot->isString() && annot->getString() == value;
}

void BasicAnnotatorTest::SetUp()
{
    m_typeTable = std::make_shared<ScopedTable<ScopedType>>();
    m_symbolTable = std::make_shared<ScopedTable<ScopedSymbol>>();

    m_symbolTable->beginScope(); // Global scope.
    m_typeTable->beginScope();   // Global scope.

    // Add defined types, in the real application should be added using IR's internal types.
    std::shared_ptr<ScopedType> i8 = std::make_shared<ScopedType>("i8");
    std::shared_ptr<ScopedType> i16 = std::make_shared<ScopedType>("i16");
    std::shared_ptr<ScopedType> i32 = std::make_shared<ScopedType>("i32");
    std::shared_ptr<ScopedType> i64 = std::make_shared<ScopedType>("i64");
    std::shared_ptr<ScopedType> _float = std::make_shared<ScopedType>("float");
    std::shared_ptr<ScopedType> string = std::make_shared<ScopedType>("string");

    m_i8TypeId = m_typeTable->addItem(i8);
    m_i16TypeId = m_typeTable->addItem(i16);
    m_i32TypeId = m_typeTable->addItem(i32);
    m_i64TypeId = m_typeTable->addItem(i64);
    m_floatTypeId = m_typeTable->addItem(_float);
    m_stringTypeId = m_typeTable->addItem(string);

    // Let's add random symbols.
    m_symbolTable->addItem(std::make_shared<ScopedSymbol>(m_i8TypeId, "TEST_RandomSymbol1"));
    m_symbolTable->addItem(std::make_shared<ScopedSymbol>(m_i8TypeId, "TEST_RandomSymbol2"));
    m_symbolTable->addItem(std::make_shared<ScopedSymbol>(m_i8TypeId, "TEST_RandomSymbol3"));
    m_symbolTable->addItem(std::make_shared<ScopedSymbol>(m_i8TypeId, "TEST_RandomSymbol4"));
    m_symbolTable->addItem(std::make_shared<ScopedSymbol>(m_i8TypeId, "TEST_RandomSymbol5"));

    m_sourceManager = std::make_shared<SourceManager>();
    m_logger = std::make_shared<SourceLoggingSink>(g_logger.get(), m_sourceManager);
    m_parser = std::make_shared<BasicParsingContext>(m_logger, m_sourceManager);
    m_tokenizer = std::make_shared<BasicTokenizer>(m_logger, m_sourceManager, "TEST");
}

void BasicAnnotatorTest::TearDown()
{
    m_symbolTable.reset();
    m_typeTable.reset();

    m_i8TypeId = 0;
    m_i16TypeId = 0;
    m_i32TypeId = 0;
    m_i64TypeId = 0;
    m_floatTypeId = 0;

    m_sourceManager.reset();
    m_logger.reset();
    m_parser.reset();
    m_tokenizer.reset();
}

std::shared_ptr<AstNode> BasicAnnotatorTest::parse(const std::shared_ptr<Rule> &parsingRule, const std::string &code)
{
    std::shared_ptr<AstNode> result = AstNodes::Null().build();
    m_sourceManager->addSourceContent("TEST", code);
    m_tokenizer->tokenizeBuffer((char *)code.data(), 0, code.size());
    m_parser->setTokens(m_tokenizer->getTokens());

    if (!parsingRule->match(*m_parser, result))
    {
        m_parser->getErrorCollector()->exitScope(ErrorHandleType::Commit);
        return nullptr;
    }

    return std::move(result);
}
