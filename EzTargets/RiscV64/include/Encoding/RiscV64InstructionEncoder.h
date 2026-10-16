#ifndef EZTARGETS_RISCV64_INSTRUCTION_ENCODER_H
#define EZTARGETS_RISCV64_INSTRUCTION_ENCODER_H

#include "Encoding/RiscV64EncodingDesc.h"
#include "Encoding/EncodeResult.h"

#include <cstdint>
#include <span>
#include <vector>

namespace EzTargets::RiscV64
{

/**
 * RISC-V 64 table-driven instruction encoder and bit manipulator.
 * Emits 32-bit little-endian machine instructions from EncodingDesc.
 */
class InstructionEncoder
{
  public:
    /**
     * Encodes one instruction into `out` (appending 4 bytes) and reports
     * relocation metadata via `result`.
     * Returns false on malformed descriptor or invalid operands.
     */
    static bool encode(const EncodingDesc &desc,
                       std::span<const ResolvedOperand> operands,
                       std::vector<uint8_t> &out,
                       EzCodeEmitter::EncodeResult &result);

    /**
     * Emits a register-to-register move (ADDI rd, rs, 0 for GPR or FSGNJ.D for FPR).
     */
    static void encodeRegisterMove(const ResolvedOperand &dst,
                                   const ResolvedOperand &src,
                                   std::vector<uint8_t> &out);

    /**
     * Emits a JAL rd, offset (J-type) instruction.
     */
    static void emitJal(std::vector<uint8_t> &out, uint8_t rd, int32_t offset);

    /**
     * Emits a conditional branch (B-type) instruction with given funct3.
     */
    static void emitBranch(std::vector<uint8_t> &out, uint8_t funct3, uint8_t rs1, uint8_t rs2, int32_t offset);

    /**
     * Classifies a 32-bit branch/jump instruction at bytes[offset].
     * Returns true if it is a B-type branch or JAL jump.
     */
    static bool classifyBranch(std::span<const uint8_t> bytes,
                               size_t offset,
                               bool &isJal,
                               bool &isBranch,
                               uint8_t &funct3,
                               uint8_t &rs1,
                               uint8_t &rs2,
                               uint8_t &rd);

    /**
     * Patches a B-type branch displacement in-place at bytes[offset].
     */
    static bool patchBranchDisp(std::span<uint8_t> bytes, size_t offset, int32_t disp);

    /**
     * Patches a J-type jump displacement in-place at bytes[offset].
     */
    static bool patchJalDisp(std::span<uint8_t> bytes, size_t offset, int32_t disp);

    /**
     * Writes a 32-bit little-endian value at bytes[offset].
     */
    static bool writeDisp32(std::span<uint8_t> bytes, size_t offset, int32_t value);
};

} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_INSTRUCTION_ENCODER_H
