#ifndef EZPACKER_X64INSTRSELECTOR_H
#define EZPACKER_X64INSTRSELECTOR_H

#include "EzTargetEmitter.h"
#include "asmjit/asmjit.h"

class x64InstrSelector
{
  public:
    /**
     * @brief Utility function to populate the Instruction Selection Table for x86_64 using ASMJIT.
     *
     * Note: High-level instructions like `GETARR`, `SETARR`, and `CREATE` are omitted here.
     * They should be eliminated by an earlier Lowering Pass (converted into pointer
     * math, `LEA`, `LOAD`, `STORE`, or standard library calls) before reaching Instruction Selection.
     */
    std::shared_ptr<InstructionSelectionTable> getSelectionTable();

  private:
};

#endif // EZPACKER_X64INSTRSELECTOR_H
