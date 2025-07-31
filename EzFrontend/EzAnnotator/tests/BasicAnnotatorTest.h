#ifndef EZPACKER_BASICANNOTATORTEST_H
#define EZPACKER_BASICANNOTATORTEST_H

#include <gtest/gtest.h>
#include "EzAnnotator.h"

class BasicAnnotatorTest : public ::testing::Test
{
  public:
    /**
     * Expects the given node to contain a ConstantAnnotation, which is of type float and whose value is the one
     * given. Returns true if all conditions are met.
     * @param value
     * @param node
     * @return bool
     */
    bool expectConstantFloatValue(float value, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to contain a ConstantAnnotation, which is of type integer and whose value is the one
     * given. This function is only limited to a maximum of 64-bit-values. Returns true if all conditions are met.
     * @param value
     * @param node
     * @return bool
     */
    bool expectConstantIntegerValue(uint64_t value, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to contain a ConstantAnnotation, which is of type string and whose value is the one
     * given. Returns true if all conditions are met.
     * @param value
     * @param node
     * @return bool
     */
    bool expectConstantStringValue(const std::string &value, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to contain an InstructionAnnotation, with an instruction id that matches the one
     * given. Returns true if all conditions are met.
     * @return bool
     */
    bool expectInstructionId(size_t instrId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given module child id to be of type Instruction. It also expects it to have an InstructionAnnotation
     * with its instruction ID equal to the id of the given instrName. Returns true if all conditions are met.
     * @param childId
     * @param instrName
     * @param node
     * @return bool
     */
    bool expectInstrInNode(size_t childId, const std::string &instrName, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to contain CHILDREN of type InstructionOperand. The number of children must match
     * numOperands. Returns true if all conditions are met.
     * @param numOperands
     * @param node
     * @return bool
     */
    bool expectInstructionOperandCount(size_t numOperands, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given given node to contain a child, with the given childId, of type InstructionOperand. It also
     * expects that the child contains a ConstantAnnotation. The annotation must be an integer with its value being
     * equal to 'value'. Returns true if all conditions are met.
     * @param value
     * @param childId
     * @param node
     * @return bool
     */
    bool expectInstructionOperandCC(size_t value, size_t childId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given given node to contain a child, with the given childId, of type InstructionOperand. It also
     * expects that the child contains a MemoryRefAnnotation. Annotation's reference type must be equal to 'refType'.
     * Returns true if all the conditions are met.
     * @param value
     * @param childId
     * @param node
     * @return bool
     */
    bool expectInstructionOperandMM(MemoryReferenceType refType, size_t childId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given given node to contain a child, with the given childId, of type InstructionOperand. It also
     * expects that the child contains a VirtualVariableAnnotation. The annotation's symbol & type IDs must match
     * the ones given. Returns true if all conditions are met.
     * @param symbolId
     * @param typeId
     * @param childId
     * @param node
     * @return bool
     */
    bool
    expectInstructionOperandVV(size_t symbolId, size_t typeId, size_t childId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to have a MemoryRefAnnotation with a Base type. It also expects the base symbol id to
     * be the one given. Returns true if all conditions are met.
     * @param baseId
     * @param typeId
     * @param node
     * @return bool
     */
    bool expectMemoryBase(size_t baseId, size_t typeId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to have a MemoryRefAnnotation with a BaseDispl type. It also expects the base symbol id
     * and displacement value to be the ones given. Returns true if all conditions are met.
     * @param baseId
     * @param displ
     * @param typeId
     * @param node
     * @return bool
     */
    bool expectMemoryBaseDispl(size_t baseId, uint64_t displ, size_t typeId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to have a MemoryRefAnnotation with a IndexScale type. It also expects the base an index
     * symbol id, and scale and displacement value to be the ones given. Returns true if all conditions are met.
     * @param baseId
     * @param indexId
     * @param scale
     * @param displ
     * @param typeId
     * @param node
     * @return bool
     */
    bool expectMemoryBaseIndexScaleDispl(size_t baseId,
                                         size_t indexId,
                                         uint64_t scale,
                                         uint64_t displ,
                                         size_t typeId,
                                         const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to have a MemoryRefAnnotation with a Direct type. It also expects the memory address
     * to be the same as "memoryAddress". Returns true if all conditions are met.
     * @param memoryAddress
     * @param typeId
     * @param node
     * @return bool
     */
    bool expectMemoryDirect(size_t memoryAddress, size_t typeId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to have a MemoryRefAnnotation with a IndexScale type. It also expects the index symbol id
     * and scale value to be the ones given.
     * @param indexId
     * @param scale
     * @param typeId
     * @param node
     * @return bool
     */
    bool expectMemoryIndexScale(size_t indexId, uint64_t scale, size_t typeId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to have a ModuleAnnotation. It also expects to have an argument at given argId with
     * its symbol and type ID matching the ones given. Returns true if all the conditions are met.
     * @param symbolId
     * @param typeId
     * @param node
     * @return bool
     */
    bool expectModuleArgument(size_t symbolId, size_t typeId, size_t argId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to have a ModuleAnnotation. It also expects to have 'numArgs' arguments.
     * @param numArgs
     * @param node
     * @return bool
     */
    bool expectModuleArgumentCount(size_t numArgs, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given module child id to be of type Label. It also expects its symbol ID to match the one given.
     * Returns true if all the conditions are met.
     * @param childId
     * @param labelName
     * @param node
     * @return bool
     */
    bool expectModuleLabelInBody(size_t childId, size_t symbolId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to have a ModuleAnnotation. It also expects module's type to match typeId. Returns true
     * if all the conditions are met.
     * @param typeId
     * @param node
     * @return bool
     */
    bool expectModuleReturnType(size_t typeId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to have a ModuleAnnotation. It also expects module's symbolID to match the one given.
     * @param symbolId
     * @param node
     * @return bool
     */
    bool expectModuleSymbolId(size_t symbolId, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to contain a SymbolAnnotation which id matches the given one. Returns true if all
     * conditions are met.
     * @param id
     * @param node
     * @return bool
     */
    bool expectSymbolId(size_t id, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to contain a SymbolAnnotation and its type to match the one given. Returns true if all
     * conditions are met.
     * @param id
     * @param node
     * @return bool
     */
    bool expectSymbolTypeId(size_t id, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to contain a TypeAnnotation and its typeId to match the one given.
     * @param id
     * @param node
     * @return bool
     */
    bool expectTypeId(size_t id, const std::shared_ptr<AstNode> &node);

    /**
     * Expects the given node to contain a VariableAnnotation. It also expects that the variable has "count"
     * initializers. Returns true if all conditions are met.
     * @param count
     * @param node
     * @return bool
     */
    bool expectVariableInitializerCount(size_t count, const std::shared_ptr<AstNode> &node);

    /**
     * * Expects the given node to contain a VariableAnnotation. It also expects that the variable has an initializer
     * of float type at the given index and that its value matches the one given. Returns true if all the conditions
     * are met.
     * @return bool
     */
    bool expectVariableFloatValue(size_t initializerId, float value, const std::shared_ptr<AstNode> &node);

    /**
     * * Expects the given node to contain a VariableAnnotation. It also expects that the variable has an initializer
     * of integer type at the given index and that its u64 value matches the one given. Returns true if all the
     * conditions are met.
     * @return bool
     */
    bool expectVariableIntValue(size_t initializerId, size_t value, const std::shared_ptr<AstNode> &node);

    /**
     * * Expects the given node to contain a VariableAnnotation. It also expects that the variable has an initializer
     * of string type at the given index and that its value matches the one given. Returns true if all the conditions
     * are met.
     * @return bool
     */
    bool
    expectVariableStringValue(size_t initializerId, const std::string &value, const std::shared_ptr<AstNode> &node);

    /**
     * Executes before a test body.
     */
    void SetUp() override;

    /**
     * Executes after a test body.
     */
    void TearDown() override;

    /**
     * Parses given content with the given rule and returns the resulting node.
     * @param parsingRule
     * @param code
     * @return std::shared_ptr<AstNode>
     */
    std::shared_ptr<AstNode> parse(const std::shared_ptr<Rule> &parsingRule, const std::string &code);

  public:
    std::shared_ptr<ScopedTable<ScopedType>> m_typeTable;
    std::shared_ptr<ScopedTable<ScopedSymbol>> m_symbolTable;
    size_t m_i8TypeId;
    size_t m_i16TypeId;
    size_t m_i32TypeId;
    size_t m_i64TypeId;
    size_t m_floatTypeId;
    size_t m_stringTypeId;
    std::shared_ptr<BasicParsingContext> m_parser;
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<SourceLoggingSink> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
};

#endif // EZPACKER_BASICANNOTATORTEST_H
