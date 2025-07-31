#include "Parser/ComplexParsers.h"

namespace NodeParsers
{
ParsingRule &InstructionOperand()
{
    static auto p = Rules::Sequence(Type(), Rules::AnyOf(Number(), MemoryReference(), String(), VirtualVariable()))
                            ->encapsulate(&AstNodes::InstructionOperand());
    return p;
}

ParsingRule &InstructionNoOperand()
{
    static auto p = Rules::Sequence(Dot(), Identifier(), SemiColon())->encapsulate(&AstNodes::Instruction());
    return p;
}

ParsingRule &Instruction1OrMoreOperands()
{
    static auto p = Rules::Sequence(Dot(),
                                    Identifier(),
                                    InstructionOperand(),
                                    Rules::StrictManyOf(Comma(), Rules::Sequence(Comma(), InstructionOperand())),
                                    SemiColon())
                            ->encapsulate(&AstNodes::Instruction());
    return p;
}

ParsingRule &Instruction()
{
    // Order is important because if InstructionNoOperand is put first, there won't be any operands on the parsed
    // instruction.
    static auto p = Rules::AnyOf(Instruction1OrMoreOperands(), InstructionNoOperand());
    return p;
}

ParsingRule &Label()
{
    static auto p = Rules::Sequence(Identifier(), Colon(), Rules::StrictManyOf(Dot(), Instruction()))
                            ->encapsulate(&AstNodes::Label());
    return p;
}

ParsingRule &MultiArgument() // Not in ComplexParsers.h
{
    static auto p = Rules::Sequence(Comma(), Type(), VirtualVariable());
    return p;
}

ParsingRule &ModuleHeader()
{
    static auto p = Rules::Sequence(Type(),
                                    Identifier(),
                                    LeftParen(),
                                    Rules::Optional(Rules::Sequence(Type(), VirtualVariable()))
                                            ->then(Rules::StrictManyOf(Comma(), MultiArgument())),
                                    RightParen())
                            ->encapsulate(&AstNodes::ModuleHeader());
    return p;
}

ParsingRule &Module()
{
    static auto p = Rules::Sequence(ModuleHeader(),
                                    LeftBrace(),
                                    Rules::ManyOf(Rules::AnyOf(Label(), Instruction())),
                                    RightBrace())
                            ->encapsulate(&AstNodes::Module());
    return p;
}

} // namespace NodeParsers
