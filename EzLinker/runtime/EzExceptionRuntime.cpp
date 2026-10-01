#include "EzExceptionRuntime.h"
#include "Rtti/RttiDescriptor.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>

extern "C" {

thread_local EzExceptionFrame *g_ezTopFrame = nullptr;
thread_local void *g_ezCurrentPayload = nullptr;
thread_local const void *g_ezCurrentRtti = nullptr;

EZ_EX_API int __ez_try_enter(struct EzExceptionFrame *frame)
{
    if (!frame)
    {
        return 0;
    }
    frame->prev = g_ezTopFrame;
    frame->currentPayload = nullptr;
    frame->currentRtti = nullptr;
    frame->isCaught = 0;
    g_ezTopFrame = frame;
    return 0;
}

EZ_EX_API void __ez_try_leave(struct EzExceptionFrame *frame)
{
    if (!frame)
    {
        return;
    }
    if (g_ezTopFrame == frame)
    {
        g_ezTopFrame = frame->prev;
    }
}

EZ_EX_API void __ez_throw(void *payload, const void *rtti)
{
    if (g_ezTopFrame == nullptr)
    {
        // Uncaught exception handling
        std::cerr << "fatal error: uncaught exception";
        if (rtti)
        {
            const auto *desc = static_cast<const EzCore::RttiTypeDescriptor *>(rtti);
            if (!desc->typeName.empty())
            {
                std::cerr << " of type '" << desc->typeName << "'";
            }
            if (!desc->declarationSite.filePath.empty())
            {
                std::cerr << " (type declared at " << desc->declarationSite.filePath << ":" << desc->declarationSite.line << ")";
            }
        }
        if (payload)
        {
            const auto *rich = static_cast<const EzCore::RichExceptionPayload *>(payload);
            if (rich && !rich->throwSite.filePath.empty())
            {
                std::cerr << "\n  at " << rich->throwSite.filePath << ":" << rich->throwSite.line << ":" << rich->throwSite.column;
                if (!rich->throwSite.functionName.empty())
                {
                    std::cerr << " in " << rich->throwSite.functionName;
                }
                if (!rich->throwSite.snippet.empty())
                {
                    std::cerr << "\n  " << rich->throwSite.snippet;
                }
            }
        }
        std::cerr << std::endl;
        std::abort();
    }

    EzExceptionFrame *target = g_ezTopFrame;
    g_ezTopFrame = target->prev; // Pop target from active stack before jumping

    g_ezCurrentPayload = payload;
    g_ezCurrentRtti = rtti;

    target->currentPayload = payload;
    target->currentRtti = rtti;
    target->isCaught = 1;

    longjmp(target->jmpBuf, 1);
}

EZ_EX_API int __ez_catch_matches(const void *thrownRtti, const void *filterRtti)
{
    // A null filter matches any exception (catch-all)
    if (!filterRtti)
    {
        return 1;
    }
    // Filter is non-null, but thrown has no RTTI (--no-rtti mode)
    if (!thrownRtti)
    {
        return 0;
    }

    const auto *thrownDesc = static_cast<const EzCore::RttiTypeDescriptor *>(thrownRtti);
    const auto *filterDesc = static_cast<const EzCore::RttiTypeDescriptor *>(filterRtti);
    return thrownDesc->isA(filterDesc) ? 1 : 0;
}

EZ_EX_API void *__ez_get_current_exception(void)
{
    return g_ezCurrentPayload;
}

EZ_EX_API const void *__ez_get_current_rtti(void)
{
    return g_ezCurrentRtti;
}

EZ_EX_API struct EzExceptionFrame *__ez_get_top_frame(void)
{
    return g_ezTopFrame;
}

EZ_EX_API void __ez_runtime_reset_for_testing(void)
{
    g_ezTopFrame = nullptr;
    g_ezCurrentPayload = nullptr;
    g_ezCurrentRtti = nullptr;
}

} // extern "C"
