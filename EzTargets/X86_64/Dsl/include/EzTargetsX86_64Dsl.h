#ifndef EZTARGETS_X86_64_DSL_H
#define EZTARGETS_X86_64_DSL_H

/**
 * Public entry point for the x86-64 EzDSL plugin.
 *
 * The plugin contributes the x86-64 encoding dialect (used during semantic analysis of
 * .idf ENCODING blocks) and the encoding code generation backend (used to emit the runtime
 * encoding table). EzDslCli and the tooling test suites call registerDsl() so the plugin is
 * pulled in from its shared library and registers itself before use.
 */
namespace EzTargets::X86_64
{
/** Registers the x86-64 encoding dialect and encoding codegen backend. Idempotent. */
void registerDsl();
} // namespace EzTargets::X86_64

#endif // EZTARGETS_X86_64_DSL_H
