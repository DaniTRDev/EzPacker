#ifndef EZPACKER_INSTRUCTION_H
#define EZPACKER_INSTRUCTION_H

#include "EzLexerCommon.h"
#include "Variable.h"
#include "MemoryOperand.h"
#include "ImmediateOperand.h"
#include "AstNode/AstNodeContainer.h"
#include "AstNode/AstNodeVisitor.h"

/**
 * This class represents an instruction in our language. Special instructions (that require extra logic) must inherit
 * from this class.
 */
class Instruction : public AstNode, public AstNodeContainer
{
  public:
    /**
     * Creates the instruction with the given operands and instruction name.
     * @param operands
     * @param instructionName
     */
    Instruction(TypedPoolSlice<AstNode> *operands, std::string_view instructionName);

    /**
     * Returns AstNodeType::Instruction.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type. Returns
     * the result of visit.
     * @param visitor
     * @return bool
     */
    bool accept(AstNodeVisitor *visitor) override;

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
     * @return const std::string_view &
     */
    const std::string_view &getInstructionName() const;

  private:
    std::string_view m_instructionName;
};

/**
 * A call instruction, it has its own class because it has different grammar than an "average" instruction, and I wanted
 * that to be reflected.
 */
class CallInstruction : public Instruction
{
  public:
    /**
     * Creates the call instruction with the given params, calleName and return type.
     * @param params
     * @param calleeName
     * @param returnType
     */
    CallInstruction(TypedPoolSlice<AstNode> *params, std::string_view calleeName, std::string_view returnType);

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
     * @return const std::string_view &
     */
    const std::string_view &getCalleeName() const;

    /**
     * Returns the "return type" of the function.
     * @return const std::string_view &
     */
    const std::string_view &getReturnType() const;

  private:
    std::string_view m_calleeName;
    std::string_view m_returnType;
};

#endif // EZPACKER_INSTRUCTION_H
