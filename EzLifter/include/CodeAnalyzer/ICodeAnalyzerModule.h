#ifndef EZPACKER_ICODEANALYZERMODULE_H
#define EZPACKER_ICODEANALYZERMODULE_H

#include "EzLifterCommon.h"
#include "Decoder/ParsedDecodedInstruction.h"
#include "Decoder/IDecodedOperand.h"

/**
 * This interface represents the basic functionality for a module of the code analizer. Code analizer modules work
 * with our IL (ParsedDecodedInstruction).
 */
class ICodeAnalyzerModule
{
  public:
    virtual ~ICodeAnalyzerModule() = default;
    
    virtual void analyzeInstruction(const std::shared_ptr<ParsedDecodedInstruction> &instr,
                                    const std::vector<std::shared_ptr<IDecodedOperand>> &operands) = 0;
};

#endif // EZPACKER_ICODEANALYZERMODULE_H
