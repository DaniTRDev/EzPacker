#ifndef EZCORE_CLI_EXIT_CODE_H
#define EZCORE_CLI_EXIT_CODE_H

/**
 * Process exit codes shared by the EzPacker command-line entry points.
 *
 * Both EzDslCli and ezc use the same contract: success, a user/compilation error, and an
 * unexpected fatal exception. Defining them once keeps the two drivers aligned.
 */
namespace EzCli
{

inline constexpr int kSuccess = 0;       ///< Requested work completed (or help/version was printed).
inline constexpr int kError = 1;         ///< Invalid usage or a reported compilation/generation failure.
inline constexpr int kFatalException = 2; ///< An unhandled exception escaped the driver.

} // namespace EzCli

#endif // EZCORE_CLI_EXIT_CODE_H
