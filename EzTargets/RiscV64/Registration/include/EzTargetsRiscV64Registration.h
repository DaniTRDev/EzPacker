#ifndef EZTARGETS_RISCV64_REGISTRATION_H
#define EZTARGETS_RISCV64_REGISTRATION_H

/**
 * Public entry point for RISC-V 64 target factory registration.
 */
namespace EzTargets::RiscV64
{
/** Registers the RISC-V 64 target factory with EzCompiler::TargetResolver. Idempotent. */
void registerTarget();
} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_REGISTRATION_H
