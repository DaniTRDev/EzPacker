#ifndef EZPACKER_CODEANALYZER_H
#define EZPACKER_CODEANALYZER_H

#include "Decoder/ParsedDecodedInstruction.h"
#include "EzLifterCommon.h"
#include "ICodeAnalyzerModule.h"

/**
 * This class represents a code analyzer. Each code analyzer has different modules that are used to analyze code.
 */
class CodeAnalyzer
{
  public:
    /**
     * Creates the object.
     */
    CodeAnalyzer();

    /**
     * Destroys the object and free resources.
     */
    ~CodeAnalyzer();

    /**
     * Tries to add the given module if not already added. If module was added true is returned.
     * @param name
     * @param module
     * @return bool
     */
    bool addModule(std::string_view name, std::shared_ptr<ICodeAnalyzerModule> module);

    /**
     * Analyzes current instruction with every module.
     * @param instr
     * @param operands
     */
    void analyceInstruction(const std::shared_ptr<ParsedDecodedInstruction> &instr,
                            const std::vector<std::shared_ptr<IDecodedOperand>> &operands);

  private:
    std::map<std::string_view, std::shared_ptr<ICodeAnalyzerModule>> m_modules;
};

#endif // EZPACKER_ICODEANALIZER_H
