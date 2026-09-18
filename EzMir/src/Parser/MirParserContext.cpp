#include "Parser/MirParserContext.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "GlobalVar/MirGlobalVar.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "SourceManager/GenericSourceManager.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

namespace EzMir
{

MirParserContext::MirParserContext(MirBuilderContext *bCtx,
                                   DiagnosticCollector *diagCollector,
                                   std::pmr::memory_resource *arena,
                                   GenericSourceManager *sourceMgr,
                                   size_t sourceId) :
    m_bCtx(bCtx),
    m_diag(diagCollector),
    m_arena(arena ? arena : std::pmr::get_default_resource()),
    m_sourceMgr(sourceMgr),
    m_sourceId(sourceId),
    m_registers(m_arena),
    m_blocks(m_arena),
    m_globals(m_arena),
    m_functions(m_arena),
    m_pendingFixups(m_arena)
{
}

void MirParserContext::enterFunction(MirFunction *func)
{
    m_currentFunction = func;
    m_registers.clear();
    m_blocks.clear();

    if (!func)
    {
        return;
    }

    // Register existing function parameters into local scope
    for (MirRegister *param : func->getParameters())
    {
        if (param && !param->getName().empty())
        {
            m_registers[param->getName()] = param;
        }
    }

    // If function already has entry point block, register it
    if (MirBlock *entry = func->getEntryPoint())
    {
        std::pmr::string entryName = entry->getName();
        if (entryName.empty())
        {
            entryName = "entry";
        }
        m_blocks[entryName] = entry;
    }
}

void MirParserContext::exitFunction()
{
    resolvePendingFunctionFixups();
    m_registers.clear();
    m_blocks.clear();
    m_currentFunction = nullptr;
}

SourceReference *MirParserContext::createRef(size_t offset, size_t length)
{
    if (!m_sourceMgr)
    {
        return nullptr;
    }
    return m_sourceMgr->createReference(offset, length, m_sourceId);
}

MirType *MirParserContext::resolveType(const Ast::MirAstType *astType)
{
    if (!astType || !m_bCtx)
    {
        return nullptr;
    }

    MirTypeTable *tt = m_bCtx->getTypeTable();
    if (!tt)
    {
        return nullptr;
    }

    switch (astType->m_kind)
    {
        case Ast::TypeKind::Void:
            return tt->_void();
        case Ast::TypeKind::Token:
            return tt->__bindToken();
        case Ast::TypeKind::Primitive:
        {
            std::string_view name = astType->m_name;
            if (name == "i1") return tt->i1();
            if (name == "i8") return tt->i8();
            if (name == "i16") return tt->i16();
            if (name == "i32") return tt->i32();
            if (name == "i64") return tt->i64();
            if (name == "i128") return tt->i128();
            if (name == "i256") return tt->i256();
            if (name == "f32") return tt->f32();
            if (name == "f64") return tt->f64();
            if (name == "f128") return tt->f128();
            if (name == "void" || name == "_void") return tt->_void();
            if (name == "token" || name == "__bindToken") return tt->__bindToken();
            if (name == "ptr") return tt->getPtr(tt->i8());

            if (m_diag)
            {
                m_diag->error("MirParser", "Unknown primitive type '{}'", name) << astType->m_ref;
            }
            recordError();
            return nullptr;
        }
        case Ast::TypeKind::Pointer:
        {
            if (astType->m_subType)
            {
                MirType *sub = resolveType(astType->m_subType);
                if (!sub)
                {
                    return nullptr;
                }
                return tt->getPtr(sub);
            }
            return tt->getPtr(tt->i8());
        }
        case Ast::TypeKind::Array:
        {
            if (!astType->m_subType)
            {
                if (m_diag)
                {
                    m_diag->error("MirParser", "Array type missing element type") << astType->m_ref;
                }
                recordError();
                return nullptr;
            }
            MirType *elem = resolveType(astType->m_subType);
            if (!elem)
            {
                recordError();
                return nullptr;
            }
            return tt->getArray(elem, astType->m_arraySize);
        }
    }

    return nullptr;
}

MirRegister *MirParserContext::declareRegister(std::string_view name,
                                               MirType *type,
                                               SourceReference *ref,
                                               MirRegisterClass *regClass)
{
    std::pmr::string key(name, m_arena);
    auto it = m_registers.find(key);
    if (it != m_registers.end())
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "Redefinition of register '%{}'", name) << ref;
        }
        recordError();
        return it->second;
    }

    if (!type && m_bCtx)
    {
        type = m_bCtx->getTypeTable()->i64();
    }

    MirOperandBuilder opBuilder(m_bCtx);
    MirRegister *reg = nullptr;

    // Check if physical register: %p...
    if (name.starts_with("%p") || name.starts_with("p"))
    {
        std::string_view numPart = name.starts_with("%p") ? name.substr(2) : name.substr(1);
        size_t physId = 0;
        std::from_chars(numPart.data(), numPart.data() + numPart.size(), physId);
        reg = opBuilder.buildPhysReg(type, physId, name, regClass, ref);
    }
    else
    {
        reg = opBuilder.buildVReg(type, name, ref, regClass);
    }

    m_registers[key] = reg;
    return reg;
}

MirRegister *MirParserContext::resolveRegister(std::string_view name, SourceReference * /*ref*/)
{
    std::pmr::string key(name, m_arena);
    auto it = m_registers.find(key);
    if (it != m_registers.end())
    {
        return it->second;
    }
    return nullptr;
}

MirRegister *MirParserContext::getOrCreateRegister(std::string_view name,
                                                   MirType *type,
                                                   SourceReference *ref,
                                                   MirRegisterClass *regClass)
{
    std::pmr::string key(name, m_arena);
    auto it = m_registers.find(key);
    if (it != m_registers.end())
    {
        return it->second;
    }

    if (!type && m_bCtx)
    {
        type = m_bCtx->getTypeTable()->i64();
    }

    MirOperandBuilder opBuilder(m_bCtx);
    MirRegister *reg = nullptr;

    if (name.starts_with("%p") || name.starts_with("p"))
    {
        std::string_view numPart = name.starts_with("%p") ? name.substr(2) : name.substr(1);
        size_t physId = 0;
        std::from_chars(numPart.data(), numPart.data() + numPart.size(), physId);
        reg = opBuilder.buildPhysReg(type, physId, name, regClass, ref);
    }
    else
    {
        reg = opBuilder.buildVReg(type, name, ref, regClass);
    }

    m_registers[key] = reg;
    return reg;
}

MirBlock *MirParserContext::declareBlock(std::string_view name, SourceReference *ref)
{
    if (!m_currentFunction)
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "Cannot declare basic block '{}' outside of function definition", name) << ref;
        }
        return nullptr;
    }

    std::pmr::string key(name, m_arena);
    auto it = m_blocks.find(key);
    if (it != m_blocks.end())
    {
        // Check if this was already declared and populated
        return it->second;
    }

    // If function has an empty skeleton entry point block, reuse it for the first declared block
    MirBlock *entry = m_currentFunction->getEntryPoint();
    if (entry && entry->getInstructions().empty() &&
        (m_blocks.empty() || (m_blocks.size() == 1 && m_blocks.find("entryPoint") != m_blocks.end())))
    {
        m_blocks.erase("entryPoint");
        entry->setName(key);
        m_blocks[key] = entry;
        return entry;
    }

    MirBlockBuilder blockBuilder(m_bCtx, m_currentFunction);
    MirBlock *blk = blockBuilder.build(ref, name);
    m_blocks[key] = blk;
    return blk;
}

MirBlock *MirParserContext::getOrCreateBlock(std::string_view name, SourceReference *ref)
{
    if (!m_currentFunction)
    {
        return nullptr;
    }

    std::pmr::string key(name, m_arena);
    auto it = m_blocks.find(key);
    if (it != m_blocks.end())
    {
        return it->second;
    }

    MirBlockBuilder blockBuilder(m_bCtx, m_currentFunction);
    MirBlock *blk = blockBuilder.build(ref, name);
    m_blocks[key] = blk;
    return blk;
}

MirBlock *MirParserContext::resolveBlock(std::string_view name, SourceReference * /*ref*/)
{
    std::pmr::string key(name, m_arena);
    auto it = m_blocks.find(key);
    if (it != m_blocks.end())
    {
        return it->second;
    }
    return nullptr;
}

MirGlobalVar *MirParserContext::declareGlobal(std::string_view name, MirGlobalVar *var)
{
    std::pmr::string key(name, m_arena);
    m_globals[key] = var;
    return var;
}

MirGlobalVar *MirParserContext::resolveGlobal(std::string_view name, SourceReference * /*ref*/)
{
    std::pmr::string key(name, m_arena);
    auto it = m_globals.find(key);
    if (it != m_globals.end())
    {
        return it->second;
    }
    return nullptr;
}

MirFunction *MirParserContext::declareFunction(std::string_view name, MirFunction *func)
{
    std::pmr::string key(name, m_arena);
    m_functions[key] = func;
    return func;
}

MirFunction *MirParserContext::resolveFunction(std::string_view name, SourceReference * /*ref*/)
{
    std::pmr::string key(name, m_arena);
    auto it = m_functions.find(key);
    if (it != m_functions.end())
    {
        return it->second;
    }
    return nullptr;
}

void MirParserContext::recordForwardReference(std::string_view name,
                                              MirInstruction *inst,
                                              size_t operandIdx,
                                              SymbolKind kind,
                                              SourceReference *ref)
{
    m_pendingFixups.emplace_back(name, inst, operandIdx, kind, ref, m_arena);
}

bool MirParserContext::resolvePendingFunctionFixups()
{
    bool success = true;
    MirOperandBuilder opBuilder(m_bCtx);

    auto it = m_pendingFixups.begin();
    while (it != m_pendingFixups.end())
    {
        if (it->m_kind == SymbolKind::BasicBlock)
        {
            MirBlock *blk = resolveBlock(it->m_symbolName, it->m_ref);
            if (!blk)
            {
                if (m_diag)
                {
                    m_diag->error("MirParser", "Undefined basic block label '%{}'", it->m_symbolName) << it->m_ref;
                }
                recordError();
                success = false;
            }
            else if (it->m_targetInstruction)
            {
                MirReference *blockRef = opBuilder.buildRef(blk, it->m_ref);
                MirInstructionBuilder instBuilder(m_bCtx, it->m_targetInstruction);
                instBuilder.swapOperand(it->m_targetInstruction, blockRef, it->m_operandIndex);
            }
            it = m_pendingFixups.erase(it);
        }
        else if (it->m_kind == SymbolKind::Register)
        {
            MirRegister *reg = resolveRegister(it->m_symbolName, it->m_ref);
            if (!reg)
            {
                if (m_diag)
                {
                    m_diag->error("MirParser", "Undefined register '%{}'", it->m_symbolName) << it->m_ref;
                }
                recordError();
                success = false;
            }
            else if (it->m_targetInstruction)
            {
                MirInstructionBuilder instBuilder(m_bCtx, it->m_targetInstruction);
                instBuilder.swapOperand(it->m_targetInstruction, reg, it->m_operandIndex);
            }
            it = m_pendingFixups.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return success;
}

bool MirParserContext::resolveAllPendingFixups()
{
    bool success = resolvePendingFunctionFixups();
    MirOperandBuilder opBuilder(m_bCtx);

    for (const auto &fixup : m_pendingFixups)
    {
        if (fixup.m_kind == SymbolKind::Function)
        {
            MirFunction *fn = resolveFunction(fixup.m_symbolName, fixup.m_ref);
            if (fn && fixup.m_targetInstruction)
            {
                MirReference *fnRef = opBuilder.buildRef(fn, fixup.m_ref);
                MirInstructionBuilder instBuilder(m_bCtx, fixup.m_targetInstruction);
                instBuilder.swapOperand(fixup.m_targetInstruction, fnRef, fixup.m_operandIndex);
            }
            else if (!fn)
            {
                // Also check runtime symbols or emit error
                if (m_diag)
                {
                    m_diag->error("MirParser", "Undefined function '@{}'", fixup.m_symbolName) << fixup.m_ref;
                }
                recordError();
                success = false;
            }
        }
        else if (fixup.m_kind == SymbolKind::GlobalVar)
        {
            MirGlobalVar *gv = resolveGlobal(fixup.m_symbolName, fixup.m_ref);
            if (gv && fixup.m_targetInstruction)
            {
                MirReference *gvRef = opBuilder.buildRef(gv, 0, fixup.m_ref);
                MirInstructionBuilder instBuilder(m_bCtx, fixup.m_targetInstruction);
                instBuilder.swapOperand(fixup.m_targetInstruction, gvRef, fixup.m_operandIndex);
            }
            else if (!gv)
            {
                if (m_diag)
                {
                    m_diag->error("MirParser", "Undefined global variable '@{}'", fixup.m_symbolName) << fixup.m_ref;
                }
                recordError();
                success = false;
            }
        }
    }

    m_pendingFixups.clear();
    return success;
}

} // namespace EzMir
