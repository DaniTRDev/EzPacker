#ifndef EZPACKER_MIRTESTSUITE_H
#define EZPACKER_MIRTESTSUITE_H

#include "EzMir.h"
#include "gtest/gtest.h"

/**
 * Class used to defined a generic verifier.
 * @tparam T
 */
template <typename TestedObjType, typename ParentClass> class MirVerifier
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

class MirOperandVerifier : public MirVerifier<MirOperand, MirOperandVerifier>
{
  public:
    /**
     * Creates the verifier with the given operand.
     * @param testedOperand
     */
    MirOperandVerifier(MirOperand *testedOperand);

    /**
     * Asserts if the tested operand's mir type is not equal to expectedType.
     * @param expectedType
     * @return
     */
    MirOperandVerifier &mirType(MirType *expectedType);

    /**
     * Asserts if the tested operand type is not equal to expectedType.
     * @param expectedType
     * @return
     */
    MirOperandVerifier &type(MirOperandType expectedType);

    /**
     * Verifies that the operand is a double and that its value matches val.
     * @param val
     * @return
     */
    MirOperandVerifier &verifyDouble(double val);

    /**
     * Verifies that the operand is an integer and that its value matches val.
     * @param val
     * @return
     */
    MirOperandVerifier &verifyInteger(int64_t val);

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

class MirInstructionVerifier : public MirVerifier<MirInstruction, MirInstructionVerifier>
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

#endif // EZPACKER_MIRTESTSUITE_H
