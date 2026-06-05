#ifndef EZPACKER_MIRTESTSUITE_H
#define EZPACKER_MIRTESTSUITE_H

#include "EzMir.h"
#include "gtest/gtest.h"

/**
 * Class used to defined a generic verifier.
 * @tparam T
 */
template <typename TestedObjType> class MirVerifier
{
  public:
    /**
     * Craetes the verifier with the given obj.
     * @param testedObj
     */
    MirVerifier(TestedObjType *testedObj) : m_testedObj(testedObj) {}

    /**
     * Returns the object that is going to be tested by this verifier.
     * @return
     */
    TestedObjType *getTestedObj() { return m_testedObj; }

    /**
     * Sets the object to be tested.
     * @param testedObj
     */
    void setTestedObj(TestedObjType *testedObj) { m_testedObj = testedObj; }

  protected:
    TestedObjType *m_testedObj;
};

/**
 * Class used to verify the properties of a MirType.
 */
class MirTypeVerifier : public MirVerifier<MirType>
{
  public:
    /**
     * Creates the verifier attached to the given type.
     * @param type
     */
    MirTypeVerifier(MirType *type);

    /**
     * Checks if the type has the given ID.
     * @param id
     * @return
     */
    MirTypeVerifier &id(size_t id);

    /**
     * Checks if type's name matches the one given.
     * @param name
     * @return
     */
    MirTypeVerifier &name(const std::string_view &name);

    /**
     * Checks if type's kind matches the one given.
     * @param kind
     * @return
     */
    MirTypeVerifier &typeKind(MirTypeKind kind);

    /**
     * Checks if the type's kind is an array and if it's of given type.
     * @param type
     * @return
     */
    MirTypeVerifier &arrayType(MirType *type);

    /**
     * Iterates over this type' subtypes and check they match the given list. If there's a nullptr in the list, type
     * will be skipped.
     * @param types
     * @return
     */
    MirTypeVerifier &subTypes(const std::vector<MirType *> &types);

  private:
};

/**
 * Class used to verify the content/properties of MirOperand.
 */
class MirOperandVerifier : public MirVerifier<MirOperand>
{
  public:
    /**
     * Creates the verifier with the given operand.
     * @param testedOperand
     */
    MirOperandVerifier(MirOperand *testedOperand);

    /**
     * Returns a MirType verifier.
     * @return
     */
    MirTypeVerifier mirTypeVerifier();

    /**
     * Asserts if the tested operand type is not equal to expectedType.
     * @param expectedType
     * @return
     */
    MirOperandVerifier &type(MirOperandType expectedType);

    /**
     * Verifies that the operand is a double, that its value matches val and its type matches type (if type is not
     * nullptr).
     * @param doubleType
     * @param val
     * @return
     */
    MirOperandVerifier &verifyDouble(MirType *doubleType, double val);

    /**
     * Verifies that the operand is an integer, that its value matches val and its type matches type(if type is not
     * nullptr).
     * @param floatType
     * @param val
     * @return
     */
    MirOperandVerifier &verifyInteger(MirType *intType, int64_t val);

    /**
     * Verifies that the operand is a reference, with a particular id and type. If refId == MIRID_INVALID,
     * the ID will not be checked. If expectedRefType == MirReferenceType::Invalid, the type won't be checked.
     * @param refId
     * @param expectedRefType
     * @return
     */
    MirOperandVerifier &verifyReference(size_t refId, MirReferenceType expectedRefType);

    /**
     * Verifies that the operand is a register, with a particular ID, mirType and virtuality. If mirType is nullptr it
     * won't be checked. If ID is MIRID_INVALID, the ID won't be checked.
     * @param mirType
     * @param isVirtual
     * @param id
     * @return
     */
    MirOperandVerifier &verifyRegister(MirType *mirType, bool isVirtual, size_t id);

    /**
     * Verifies that the operand is a frame index, with a particular mirType and id. If mirType is nullptr it won't
     * be checked. If ID is MIRID_INVALID, the ID won't be checked.
     * @param mirType
     * @param frameId
     * @return
     */
    MirOperandVerifier &verifyFrameIndex(MirType *mirType, size_t frameId);

    /**
     * Verifies that the operand is a memory operand and that its base and displ matches the ones given. If mirType is
     * nullptr it won't be checked. Same happens with base and displacement.
     * @param mirType
     * @param base
     * @param displ
     * @return
     */
    MirOperandVerifier &verifyMemory(MirType *mirType, MirRegister *base, MirInteger *displ);
};

/**
 * Class used to verify the content/properties of a MirInstruction and its MirOperands.
 */
class MirInstructionVerifier : public MirVerifier<MirInstruction>
{
  public:
    /**
     * Creates the verifier linked to the given instruction.
     * @param instr
     */
    MirInstructionVerifier(MirInstruction *instr);

    /**
     * Verify that the instruction's opcode matches the one given.
     * @param opcode
     * @return
     */
    MirInstructionVerifier &opcode(MirInstructionOpCode opcode);

    /**
     * Verify that instruction has exactly operandCount operands.
     * @param operandCount
     * @return
     */
    MirInstructionVerifier &operandCount(size_t operandCount);

    /**
     * Verify that this instruction has been set a target ID.
     * @param id
     * @return
     */
    MirInstructionVerifier &targetId(MirTargetInstructionId id);

    /**
     * Creates an operand verifier for the given operand at the given ID. Will also check that the instruction has
     * at least 'id' + 1 operands.
     * @param operandIndex
     * @return
     */
    MirOperandVerifier operandVerifier(size_t operandIndex);

  private:
};

/**
 * Class used to verify the content/properties of a MirBlock and its MirInstructions.
 */
class MirBlockVerifier : public MirVerifier<MirBlock>
{
  public:
    /**
     *  Creates the verifier and attaches it to the given block.
     * @param block
     */
    MirBlockVerifier(MirBlock *block);

    /**
     * Expects the block to have the given ID.
     * @param id
     * @return
     */
    MirBlockVerifier &id(size_t id);

    /**
     * Verifies that this block has exactly count instructions.
     * @return
     */
    MirBlockVerifier &instrCount(size_t count);

    /**
     * Verifies that there are exactly count instructions that matches the given opcode.
     * @param count
     * @param instr
     * @return
     */
    MirBlockVerifier &instrCountOfType(size_t count, MirInstructionOpCode opcode);

  private:
};

/**
 * Class used to verify the content/properties of a MirFunction and its MirBlocks.
 */
class MirFunctionVerifier : public MirVerifier<MirFunction>
{
  public:
    /**
     * Creates the verifier attached to the given function.
     * @param func
     */
    MirFunctionVerifier(MirFunction *func);

    /**
     * Expects the function's id to match the given one.
     * @return
     */
    MirFunctionVerifier &id(size_t id);

    /**
     * Expects the function's name to match the given one.
     * @return
     */
    MirFunctionVerifier &name(const std::string_view &name);

    /**
     * Expects the function's block count match the given one.
     * @return
     */
    MirFunctionVerifier &blockCount(size_t count);

    /**
     * Creates a verifier for the given block index. This is the MIRID of the block, this function will also check that
     * the given block is present before creating the block verifier.
     * @param id
     * @return
     */
    MirBlockVerifier blockVerifier(size_t id);
};

/**
 * Class used to contain helper methods related to creation/destruction of needed objects in common test scenarios.
 */
class MirTestSuite
{
  public:
    /**
     * Returns the builder context used by this test.
     * @return
     */
    MirBuilderContext *getBuilderCtx();

    /**
     * Returns the TEST function.
     * @return
     */
    MirFunction *getTestFunc();

    /**
     * Returns the insertion point of the first block of the TEST function.
     * @return
     */
    MirInstructionInsertionPoint *getTestInsertionPoint();

    /**
     * Returns the type table.
     * @return
     */
    MirTypeTable *getTypeTable();

    /**
     * Creates all the needed context pointers in a basic state for a test. It also creates 1 void "TEST" function,
     * without parameters.
     * @param workingPath
     */
    void create(const std::filesystem::path &workingPath);

    /**
     * Frees everything of this test suite.
     */
    void destroy();

  private:
    MirFunction *m_testFunction; // Pre-created function used to be able to create quick tests easily.
    MirInstructionInsertionPoint m_insertPoint;
    std::pmr::monotonic_buffer_resource m_arena;
    std::shared_ptr<DiagnosticCollector> m_diagCollector;
    std::shared_ptr<DiagnosticLogger> m_diagLogger;
    std::shared_ptr<MirBuilderContext> m_builderCtx;
    std::shared_ptr<MirTypeTable> m_typeTable;
    std::shared_ptr<SourceManager> m_sourceManager;
};

class MirTestSuiteAsGtest : public MirTestSuite, public ::testing::Test
{
  public:
    /**
     * Calls MirTestSuite::create.
     */
    void SetUp() override;

    /**
     * Calls MirTestSuite::destroy.
     */
    void TearDown() override;

  private:
};

#endif // EZPACKER_MIRTESTSUITE_H
