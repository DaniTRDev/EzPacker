#include "AstLowererVisitor/AstLowererVisitor.h"
#include "AstLowererVisitor/CodeScopeLowerer.h"
#include "AstLowererVisitor/ConditionLowerer.h"
#include "AstLowererVisitor/ForLowerer.h"
#include "AstLowererVisitor/IfLowerer.h"
#include "AstLowererVisitor/ImmediateLowerer.h"
#include "AstLowererVisitor/InstructionLowerer.h"
#include "AstLowererVisitor/LabelLowerer.h"
#include "AstLowererVisitor/MemoryLowerer.h"
#include "AstLowererVisitor/ModuleLowerer.h"
#include "AstLowererVisitor/SwitchLowerer.h"
#include "AstLowererVisitor/VariableLowerer.h"
#include "AstLowererVisitor/WhileLowerer.h"

AstLowererVisitor::AstLowererVisitor(const std::shared_ptr<LoweringContext> &loweringCtx) : m_loweringCtx(loweringCtx)
{
}

bool AstLowererVisitor::visit(CodeScope *scope) { return CodeScopeLowerer().lower(scope, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(ConditionAstNode *cond) { return ConditionLowerer().lower(cond, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(ForAstNode *_for) { return ForLowerer().lower(_for, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(IfAstNode *ifNode) { return IfLowerer().lower(ifNode, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(ImmediateOperand *imm) { return ImmediateLowerer().lower(imm, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(Instruction *instr) { return InstructionLowerer().lower(instr, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(Label *label) { return LabelLowerer().lower(label, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(Module *module) { return ModuleLowerer().lower(module, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(ModuleHeader *header) { return ModuleHeaderLowerer().lower(header, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(MemoryOperandAstNode *operand)
{
    return MemoryLowerer().lower(operand, m_loweringCtx.get());
}

bool AstLowererVisitor::visit(SwitchAstNode *_switch) { return SwitchLowerer().lower(_switch, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(Variable *var) { return VariableLowerer().lower(var, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(WhileAstNode *whileNode) { return WhileLowerer().lower(whileNode, m_loweringCtx.get()); }
