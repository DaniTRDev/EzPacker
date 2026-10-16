#ifndef EZTARGETS_RISCV64_DSL_H
#define EZTARGETS_RISCV64_DSL_H

/**
 * Public entry point for the RISC-V 64 EzDSL plugin.
 *
 * The plugin contributes the RISC-V encoding dialect (used during semantic analysis of
 * .idf ENCODING blocks) and the encoding code generation backend (used to emit the runtime
 * encoding table). EzDslCli and the tooling test suites call registerDsl() so the plugin is
 * pulled in and registers itself before use.
 */
namespace EzTargets::RiscV64
{
/** Registers the RISC-V encoding dialect and encoding codegen backend. Idempotent. */
void registerDsl();
} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_DSL_H
