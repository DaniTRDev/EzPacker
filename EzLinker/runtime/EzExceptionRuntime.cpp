#include "EzExceptionRuntime.h"
#include "Rtti/RttiDescriptor.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>

extern "C" {

EZ_EX_API const EzCore::RttiTypeDescriptor __ez_default_rtti = {
    .typeId = EzCore::kDefaultExceptionTypeId,
    .typeName = EzCore::kDefaultExceptionTypeName,
    .numBases = 0,
    .bases = nullptr,
    .declarationSite = { "<runtime>", "<builtin>", 0, 0, "Default exception descriptor" }
};

EZ_EX_API const EzCore::RichExceptionPayload __ez_default_payload = {
    .payload = nullptr,
    .rtti = &__ez_default_rtti,
    .throwSite = { "<unknown>", "<unknown>", 0, 0, "Default exception payload" }
};

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
    // Ensure RTTI is always attached; fall back to canonical default RTTI if none provided
    const void *effectiveRtti = rtti ? rtti : &__ez_default_rtti;

    // If no payload is provided, use canonical default payload
    void *effectivePayload = payload ? payload : const_cast<void *>(static_cast<const void *>(&__ez_default_payload));

    if (g_ezTopFrame == nullptr)
    {
        // Uncaught exception handling
        std::cerr << "fatal error: uncaught exception";
        if (effectiveRtti)
        {
            const auto *desc = static_cast<const EzCore::RttiTypeDescriptor *>(effectiveRtti);
            if (!desc->typeName.empty())
            {
                std::cerr << " of type '" << desc->typeName << "'";
            }
            if (!desc->declarationSite.filePath.empty())
            {
                std::cerr << " (type declared at " << desc->declarationSite.filePath << ":" << desc->declarationSite.line << ")";
            }
        }
        if (effectivePayload && effectivePayload != &__ez_default_payload)
        {
            const auto *rich = static_cast<const EzCore::RichExceptionPayload *>(effectivePayload);
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

    g_ezCurrentPayload = effectivePayload;
    g_ezCurrentRtti = effectiveRtti;

    target->currentPayload = effectivePayload;
    target->currentRtti = effectiveRtti;
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

EZ_EX_API const void *__ez_get_default_rtti(void)
{
    return &__ez_default_rtti;
}

EZ_EX_API const void *__ez_get_default_payload(void)
{
    return &__ez_default_payload;
}

EZ_EX_API const char *__ez_get_rtti_type_name(const void *rtti)
{
    if (!rtti)
    {
        return "";
    }
    const auto *desc = static_cast<const EzCore::RttiTypeDescriptor *>(rtti);
    return desc->typeName.data();
}

EZ_EX_API uint64_t __ez_get_rtti_type_id(const void *rtti)
{
    if (!rtti)
    {
        return 0;
    }
    const auto *desc = static_cast<const EzCore::RttiTypeDescriptor *>(rtti);
    return desc->typeId;
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

