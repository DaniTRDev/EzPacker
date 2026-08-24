#ifndef EZDSL_CPP_MIR_IR_INSTRUCTION_GENERATOR_H
#define EZDSL_CPP_MIR_IR_INSTRUCTION_GENERATOR_H

#include "EzDslCommon.h"
#include <filesystem>

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Synthesizes the EzMir IR instruction definition file (MirInstructionSetDefs.h)
 * from the parsed IR instruction symbols in the SymbolTable.
 */
extern bool
GenerateMirIrInstructionDefs(DiagnosticCollector *collector, SymbolTable *table, std::filesystem::path outPath);

} // namespace CodeGenerators

#endif // EZDSL_CPP_MIR_IR_INSTRUCTION_GENERATOR_H