#include "AstNodes/CodeScope.h"

AstNodeType CodeScope::getType() const { return AstNodeType::CodeScope; }

const char *CodeScope::getAstNodeName() const { return nullptr; }

std::string CodeScope::getAsStr(AstNodeStringMode mode) const
{
    const std::map<size_t, std::shared_ptr<AstNode>> &expressions = getExpressions();
    if (mode == AstNodeStringMode::Debug)
    {
        std::string str = "CodeScope\n{\n";

        for (auto &expr : expressions)
        {
            str += expr.second->getAsStr(mode);
        }

        return str + "\n}\n";
    }

    return std::format("CodeScope (size: {})", expressions.size());
}
