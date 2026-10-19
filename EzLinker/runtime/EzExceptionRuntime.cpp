#include "EzExceptionRuntime.h"
#include "Rtti/RttiDescriptor.h"

extern "C" {

#if defined(__x86_64__) || defined(_M_X64)
#if defined(_WIN32)
__attribute__((naked)) int __ez_setjmp(struct EzJmpBuf *buf)
{
    __asm__ volatile (
        "movq %rbx, 0(%rcx)\n\t"
        "movq %rbp, 8(%rcx)\n\t"
        "movq %rdi, 16(%rcx)\n\t"
        "movq %rsi, 24(%rcx)\n\t"
        "movq %r12, 32(%rcx)\n\t"
        "movq %r13, 40(%rcx)\n\t"
        "movq %r14, 48(%rcx)\n\t"
        "movq %r15, 56(%rcx)\n\t"
        "leaq 8(%rsp), %rax\n\t" // RSP before call
        "movq %rax, 64(%rcx)\n\t"
        "movq (%rsp), %rax\n\t"  // return address
        "movq %rax, 72(%rcx)\n\t"
        "xorl %eax, %eax\n\t"
        "ret\n\t"
    );
}

__attribute__((naked)) void __ez_longjmp(struct EzJmpBuf *buf, int val)
{
    __asm__ volatile (
        "movl %edx, %eax\n\t"
        "testl %eax, %eax\n\t"
        "jnz 1f\n\t"
        "movl $1, %eax\n\t"
    "1:\n\t"
        "movq 0(%rcx), %rbx\n\t"
        "movq 8(%rcx), %rbp\n\t"
        "movq 16(%rcx), %rdi\n\t"
        "movq 24(%rcx), %rsi\n\t"
        "movq 32(%rcx), %r12\n\t"
        "movq 40(%rcx), %r13\n\t"
        "movq 48(%rcx), %r14\n\t"
        "movq 56(%rcx), %r15\n\t"
        "movq 64(%rcx), %rsp\n\t"
        "jmp *72(%rcx)\n\t"
    );
}
#else
__attribute__((naked)) int __ez_setjmp(struct EzJmpBuf *buf)
{
    __asm__ volatile (
        "movq %rbx, 0(%rdi)\n\t"
        "movq %rbp, 8(%rdi)\n\t"
        "movq %r12, 16(%rdi)\n\t"
        "movq %r13, 24(%rdi)\n\t"
        "movq %r14, 32(%rdi)\n\t"
        "movq %r15, 40(%rdi)\n\t"
        "leaq 8(%rsp), %rax\n\t"
        "movq %rax, 48(%rdi)\n\t"
        "movq (%rsp), %rax\n\t"
        "movq %rax, 56(%rdi)\n\t"
        "xorl %eax, %eax\n\t"
        "ret\n\t"
    );
}

__attribute__((naked)) void __ez_longjmp(struct EzJmpBuf *buf, int val)
{
    __asm__ volatile (
        "movl %esi, %eax\n\t"
        "testl %eax, %eax\n\t"
        "jnz 1f\n\t"
        "movl $1, %eax\n\t"
    "1:\n\t"
        "movq 0(%rdi), %rbx\n\t"
        "movq 8(%rdi), %rbp\n\t"
        "movq 16(%rdi), %r12\n\t"
        "movq 24(%rdi), %r13\n\t"
        "movq 32(%rdi), %r14\n\t"
        "movq 40(%rdi), %r15\n\t"
        "movq 48(%rdi), %rsp\n\t"
        "jmp *56(%rdi)\n\t"
    );
}
#endif
#elif defined(__riscv) && (__riscv_xlen == 64)
__attribute__((naked)) int __ez_setjmp(struct EzJmpBuf *buf)
{
    __asm__ volatile (
        "sd ra, 0(a0)\n\t"
        "sd sp, 8(a0)\n\t"
        "sd s0, 16(a0)\n\t"
        "sd s1, 24(a0)\n\t"
        "sd s2, 32(a0)\n\t"
        "sd s3, 40(a0)\n\t"
        "sd s4, 48(a0)\n\t"
        "sd s5, 56(a0)\n\t"
        "sd s6, 64(a0)\n\t"
        "sd s7, 72(a0)\n\t"
        "sd s8, 80(a0)\n\t"
        "sd s9, 88(a0)\n\t"
        "sd s10, 96(a0)\n\t"
        "sd s11, 104(a0)\n\t"
        "li a0, 0\n\t"
        "ret\n\t"
    );
}

__attribute__((naked)) void __ez_longjmp(struct EzJmpBuf *buf, int val)
{
    __asm__ volatile (
        "ld ra, 0(a0)\n\t"
        "ld sp, 8(a0)\n\t"
        "ld s0, 16(a0)\n\t"
        "ld s1, 24(a0)\n\t"
        "ld s2, 32(a0)\n\t"
        "ld s3, 40(a0)\n\t"
        "ld s4, 48(a0)\n\t"
        "ld s5, 56(a0)\n\t"
        "ld s6, 64(a0)\n\t"
        "ld s7, 72(a0)\n\t"
        "ld s8, 80(a0)\n\t"
        "ld s9, 88(a0)\n\t"
        "ld s10, 96(a0)\n\t"
        "ld s11, 104(a0)\n\t"
        "seqz a0, a1\n\t"
        "add a0, a0, a1\n\t"
        "ret\n\t"
    );
}
#else
int __ez_setjmp(struct EzJmpBuf *buf)
{
    return __builtin_setjmp((void **)buf->regs);
}

void __ez_longjmp(struct EzJmpBuf *buf, int val)
{
    __builtin_longjmp((void **)buf->regs, val ? val : 1);
}
#endif

extern EZ_EX_API const EzCore::RttiTypeDescriptor __ez_default_rtti;
extern EZ_EX_API const EzCore::RichExceptionPayload __ez_default_payload;

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
        // Uncaught exception handling: trigger hardware fault trap
        __builtin_trap();
    }

    EzExceptionFrame *target = g_ezTopFrame;
    g_ezTopFrame = target->prev; // Pop target from active stack before jumping

    g_ezCurrentPayload = effectivePayload;
    g_ezCurrentRtti = effectiveRtti;

    target->currentPayload = effectivePayload;
    target->currentRtti = effectiveRtti;
    target->isCaught = 1;

    __ez_longjmp(&target->jmpBuf, 1);
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

