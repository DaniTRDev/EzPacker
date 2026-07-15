#ifndef EZPACKER_MIRCOREVERIFIERS_H
#define EZPACKER_MIRCOREVERIFIERS_H

#include "gtest/gtest.h"
#include "EzMir.h"

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

template <typename TestedPassObjType, typename OwnerObj>
    requires(std::is_base_of<MirPass, TestedPassObjType>::value)
class MirPassVerifier : public MirVerifier<TestedPassObjType>
{
  public:
    /**
     * Creates the verifier and links it to a pass object. At the time of performing verify actions, the object should
     * have been executed.
     * @param obj
     */
    MirPassVerifier(TestedPassObjType *obj) : MirVerifier<TestedPassObjType>(obj) {}

    /**
     * Checks if the given pass has been executed.
     * @return
     */
    OwnerObj &executed()
    {
        MirPass *pass = this->getTestedObj();
        EXPECT_NE(pass->getResult(), nullptr);
        EXPECT_TRUE(pass->getResult()->m_executed);

        return *static_cast<OwnerObj *>(this);
    }

    /**
     * Checks if the given pass has modified the IR.
     * @return
     */
    OwnerObj &mirModified()
    {
        MirPass *pass = this->getTestedObj();
        EXPECT_NE(pass->getResult(), nullptr);
        EXPECT_TRUE(pass->getResult()->m_modifiedMir);

        return *static_cast<OwnerObj *>(this);
    }

    /**
     * Checks if the given pass threw any error during execution.
     * @return
     */
    OwnerObj &succeeded()
    {
        MirPass *pass = this->getTestedObj();
        EXPECT_NE(pass->getResult(), nullptr);
        EXPECT_TRUE(pass->getResult()->m_succeeded);

        return *static_cast<OwnerObj *>(this);
    }
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
    MirOperandVerifier &verifyDouble(double val);

    /**
     * Verifies that the operand is a float, that its value matches val and its type matches type (if type is not
     * nullptr).
     * @param doubleType
     * @param val
     * @return
     */
    MirOperandVerifier &verifyFloat(float val);

    /**
     * Verifies that the operand is a float, that its value matches val and its type matches type. This method is
     * exposed to test any given-length float.
     * @param doubleType
     * @param val
     * @return
     */
    MirOperandVerifier &verifyFloatAnySize(MirType *floatType, const std::string &val);

    /**
     * Verifies that the operand is an integer, that its value matches val and its type matches type(if type is not
     * nullptr).
     * @param floatType
     * @param val
     * @return
     */
    MirOperandVerifier &verifyInteger(MirType *intType, int64_t val);

    /**
     * Verifies that the operand is an integer, that its value matches val and its type matches type.
     * @param floatType
     * @param val
     * @return
     */
    MirOperandVerifier &verifyInteger(MirType *intType, const FlexInt &val);

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
     * Verifies that the operand is a MirRuntimeSymbol and that if symbolName is not empty, it will also check if it's
     * symbol name matches the one given.
     * @param symbolName
     * @return
     */
    MirOperandVerifier &verifyRuntimeSymbol(const std::string &symbolName);

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
     * nullptr it won't be checked. Same happens with base and m_displ.
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

class MirFunctionStackFrameVerifier : public MirVerifier<MirFunctionStackFrame>
{
  public:
    /**
     * Creates the verifier linked to the given stack frame.
     * @param stackFrame
     */
    MirFunctionStackFrameVerifier(MirFunctionStackFrame *stackFrame);

    /**
     * Checks the given stack frame object to match the given parameters. If offset == -1, align ==
     * -1, sizeInBytes == 0, or source is Invalid, the specific parameter won't be checked. If id == MIRID_INVALID, then
     * this function WON'T check anything.
     * @param offset
     * @param align
     * @param sizeInBytes
     * @param source
     * @return
     */
    MirFunctionStackFrameVerifier &
    checkStackFrameObj(size_t id, int64_t offset, size_t align, size_t sizeInBytes, StackFrameObjectSource source);

    /**
     * Checks if there are exactly "count" stack objects in the current frame.
     * @param count
     * @return
     */
    MirFunctionStackFrameVerifier &stackFrameObjCount(size_t count);

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
     * Creates a verifier for the given block index. This is the MIRID of the block, this function will also check that
     * the given block is present before creating the block verifier.
     * @param id
     * @return
     */
    MirBlockVerifier blockVerifier(size_t id);

    /**
     * Expects the function's block count match the given one.
     * @return
     */
    MirFunctionVerifier &blockCount(size_t count);

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
     * Checks if the function has count parameters.
     * @param count
     * @return
     */
    MirFunctionVerifier &paramCount(size_t count);

    /**
     * Checks if the given type list matches the parameter types. If an element from the input list is set to
     * MIRID_INVALID, that item will be skipped.
     * @param expectedParamTypeList
     * @return
     */
    MirFunctionVerifier &paramType(const std::vector<size_t> &expectedParamTypeList);

    /**
     * Returns a verifier for the function's stack frame.
     * @return
     */
    MirFunctionStackFrameVerifier stackFrameVerifier();
};

/**
 * Class used to verify the properties of a MirClass (including inheritance, fields, and VTable layout).
 */
class MirClassVerifier : public MirVerifier<MirClass>
{
  public:
    /**
     * Creates the verifier attached to the given class.
     * @param _class
     */
    MirClassVerifier(MirClass *_class);

    /**
     * Verifies that the class name matches the expected name.
     * @param expectedName
     * @return
     */
    MirClassVerifier &className(const std::string_view &expectedName);

    /**
     * Verifies that the class's underlying MirType matches the expected type.
     * @param expectedType
     * @return
     */
    MirClassVerifier &classType(MirType *expectedType);

    /**
     * Verifies that the class has an expected parent class.
     * Pass nullptr to verify that the class is a root base class.
     * @param expectedParent
     * @return
     */
    MirClassVerifier &parentClass(MirClass *expectedParent);

    /**
     * Verifies that the class contains exactly the expected number of fields.
     * @param count
     * @return
     */
    MirClassVerifier &fieldCount(size_t count);

    /**
     * Verifies the details of a specific class field by index.
     * @param fieldIdx Index of the field in layout order
     * @param expectedName Expected string name of the field
     * @param expectedType Expected type pointer
     * @param expectedOffset Expected byte offset (if -1, offset check is ignored)
     * @return
     */
    MirClassVerifier &verifyField(size_t fieldIdx,
                                  const std::string_view &expectedName,
                                  MirType *expectedType,
                                  int64_t expectedOffset = -1);

    /**
     * Verifies that the VTable has exactly the expected size (virtual function slot count).
     * @param expectedSlotCount
     * @return
     */
    MirClassVerifier &vTableSize(size_t expectedSlotCount);

    /**
     * Verifies that a specific slot in the class's VTable points to the expected function.
     * @param slotIndex
     * @param expectedFunc
     * @return
     */
    MirClassVerifier &vTableSlot(size_t slotIndex, MirFunction *expectedFunc);
};

#endif // EZPACKER_MIRCOREVERIFIERS_H
