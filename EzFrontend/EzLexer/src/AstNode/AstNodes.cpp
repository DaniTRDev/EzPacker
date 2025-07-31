#include "AstNode/AstNodes.h"

namespace AstNodes
{
AstNodeBuilder &Null()
{
    static AstNodeBuilder builder("Null");
    return builder;
}

AstNodeBuilder &Identifier()
{
    static AstNodeBuilder builder("Identifier");
    return builder;
}

AstNodeBuilder &Instruction()
{
    static AstNodeBuilder builder("Instruction");
    return builder;
}

AstNodeBuilder &InstructionOperand()
{
    static AstNodeBuilder builder("InstructionOperand");
    return builder;
}

AstNodeBuilder &IntNumber()
{
    static AstNodeBuilder builder("IntNumber");
    return builder;
}

AstNodeBuilder &FloatNumber()
{
    static AstNodeBuilder builder("FloatNumber");
    return builder;
}

AstNodeBuilder &Label()
{
    static AstNodeBuilder builder("Label");
    return builder;
}

AstNodeBuilder &BaseMemory()
{
    static AstNodeBuilder builder("BaseMemory");
    return builder;
}

AstNodeBuilder &BaseDisplMemory()
{
    static AstNodeBuilder builder("BaseDisplMemory");
    return builder;
}

AstNodeBuilder &BaseIndexScaleDisplMemory()
{
    static AstNodeBuilder builder("BaseIndexScaleDisplMemory");
    return builder;
}

AstNodeBuilder &DirectMemory()
{
    static AstNodeBuilder builder("DirectMemory");
    return builder;
}

AstNodeBuilder &IndexScaleMemory()
{
    static AstNodeBuilder builder("IndexScaleMemory");
    return builder;
}

AstNodeBuilder &ModuleHeader()
{
    static AstNodeBuilder builder("ModuleHeader");
    return builder;
}

AstNodeBuilder &Module()
{
    static AstNodeBuilder builder("Module");
    return builder;
}

AstNodeBuilder &String()
{
    static AstNodeBuilder builder("String");
    return builder;
}

AstNodeBuilder &Type()
{
    static AstNodeBuilder builder("Type");
    return builder;
}

AstNodeBuilder &Variable()
{
    static AstNodeBuilder builder("Variable");
    return builder;
}

AstNodeBuilder &VirtualVariable()
{
    static AstNodeBuilder builder("VirtualVariable");
    return builder;
}
}; // namespace AstNodes
