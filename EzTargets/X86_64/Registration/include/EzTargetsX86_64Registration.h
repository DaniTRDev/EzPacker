#ifndef EZTARGETS_X86_64_REGISTRATION_H
#define EZTARGETS_X86_64_REGISTRATION_H

/**
 * Public entry point for the x86-64 target factory registration.
 *
 * The compiler executable and tests call registerTarget() so the registration library is
 * pulled in and its registration runs, regardless of whether the linker would otherwise
 * drop the unreferenced shared library.
 */
namespace EzTargets::X86_64
{
/** Registers the x86-64 target factory with EzCompiler::TargetResolver. Idempotent. */
void registerTarget();
} // namespace EzTargets::X86_64

#endif // EZTARGETS_X86_64_REGISTRATION_H
