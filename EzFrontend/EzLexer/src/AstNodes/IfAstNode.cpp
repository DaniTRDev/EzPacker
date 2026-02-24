#include "AstNodes/IfAstNode.h"

AstNodeType IfAstNode::getType() const { return AstNodeType::If; }

const char *IfAstNode::getAstNodeName() const { return "IfAstNode"; }