#include "CodeAnalyzer/CodeAnalyzer.h"

CodeAnalyzer::CodeAnalyzer()
{
}

CodeAnalyzer::~CodeAnalyzer()
{
    m_modules.clear();
}

bool CodeAnalyzer::addModule(std::string_view name, std::shared_ptr<ICodeAnalyzerModule> module)
{
    if (m_modules.contains(name))
        return false;

    m_modules[name] = module;
}

void CodeAnalyzer::analyceInstruction(const std::shared_ptr<ParsedDecodedInstruction> &instr,
                                      const std::vector<std::shared_ptr<IDecodedOperand>> &operands)
{
    for (auto &[name, module] : m_modules)
    {
        module->analyzeInstruction(instr, operands);
    }
}