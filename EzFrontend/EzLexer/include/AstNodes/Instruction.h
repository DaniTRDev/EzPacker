#ifndef EZPACKER_INSTRUCTION_H
#define EZPACKER_INSTRUCTION_H

#include "EzLexerCommon.h"
#include "Variable.h"
#include "MemoryOperand.h"
#include "ImmediateOperand.h"

/**
 * This class represents an instruction in our language. Special instructions (that require extra logic) must inherit
 * from this class.
 */
class Instruction : public AstNode
{
  public:
    /**
     * Creates the instruction with the given name and operands.
     * @param instructionName
     * @param operands
     */
    Instruction(std::string instructionName, std::vector<std::shared_ptr<AstNode>> operands);

    /**
     * Returns AstNodeType::Instruction.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Returns "Instruction".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. If mode is set to default, only
     * instruction code and basic operand information are shown. If mode is set to debug, instruction operands are also
     * shown.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

    /**
     * Returns the name of the instruction.
     * @return const std::string &
     */
    const std::string &getInstructionName() const;

    /**
     * Returns the operands of this instruction.
     * @return const std::vector<std::shared_ptr<AstNode>> &
     */
    const std::vector<std::shared_ptr<AstNode>> &getOperands() const;

  private:
    std::string m_instructionName;
    std::vector<std::shared_ptr<AstNode>> m_operands;
};

/**
 * A call instruction, it has its own class because it has different grammar than an "average" instruction, and I wanted
 * that to be reflected.
 */
class CallInstruction : public Instruction
{
  public:
    /**
     * Creates the call instruction with the given calleName, return type and parameters.
     * @param calleeName
     * @param returnType
     * @param parameters
     */
    CallInstruction(std::string calleeName, std::string returnType, std::vector<std::shared_ptr<AstNode>> parameters);

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. If mode is set to default, only
     * callee name and type are shown. If mode is set to debug, call parameters will also be shown.
     * shown.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

    /**
     * Returns the name of the callee function.
     * @return const std::string &
     */
    const std::string &getCalleeName() const;

    /**
     * Returns the "return type" of the function.
     * @return const std::string &
     */
    const std::string &getReturnType() const;

  private:
    std::string m_calleeName;
    std::string m_returnType;
};

#endif // EZPACKER_INSTRUCTION_H
