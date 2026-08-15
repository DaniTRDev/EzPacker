#include "RegisterBanks/EzTestTripleRegisterBanks.h"

namespace EzTestTriple
{
namespace Banks
{
MirRegisterBank *GPR = nullptr;
MirRegisterBank *FPR = nullptr;
}; // namespace Banks

std::pmr::vector<MirRegisterBank *> CreateRegisterBanks(std::pmr::memory_resource *alloc)
{
    std::pmr::polymorphic_allocator<> pAlloc(alloc);
    std::pmr::vector<MirRegisterBank *> banks(alloc);

    // ==========================================
    // 1. GPR BANK SETUP
    // ==========================================
    auto *gprBank = pAlloc.new_object<MirRegisterBank>("GPR", alloc);
    auto *gpr64 = pAlloc.new_object<MirRegisterClass>("GPR64", gprBank, alloc);
    auto *gpr32 = pAlloc.new_object<MirRegisterClass>("GPR32", gprBank, alloc);
    auto *gpr16 = pAlloc.new_object<MirRegisterClass>("GPR16", gprBank, alloc);
    auto *gpr8 = pAlloc.new_object<MirRegisterClass>("GPR8", gprBank, alloc);

    Banks::GPR = gprBank;

    struct GprDef
    {
        size_t id;
        const char *r64;
        const char *r32;
        const char *r16;
        const char *r8;
    };

    GprDef gprs[] = {
        { RegisterIds::R0, "r0", "r0d", "r0w", "r0b" },      { RegisterIds::R1, "r1", "r1d", "r1w", "r1b" },
        { RegisterIds::R2, "r2", "r2d", "r2w", "r2b" },      { RegisterIds::R3, "r3", "r3d", "r3w", "r3b" },
        { RegisterIds::RSP, "rsp", "esp", "sp", "spl" },     { RegisterIds::RFP, "rfp", "ebp", "fp", "bpl" },
        { RegisterIds::R6, "r6", "r6d", "r6w", "r6b" },      { RegisterIds::R7, "r7", "r7d", "r7w", "r7b" },
        { RegisterIds::R8, "r8", "r8d", "r8w", "r8b" },      { RegisterIds::R9, "r9", "r9d", "r9w", "r9b" },
        { RegisterIds::R10, "r10", "r10d", "r10w", "r10b" }, { RegisterIds::R11, "r11", "r11d", "r11w", "r11b" },
        { RegisterIds::R12, "r12", "r12d", "r12w", "r12b" }, { RegisterIds::R13, "r13", "r13d", "r13w", "r13b" },
        { RegisterIds::R14, "r14", "r14d", "r14w", "r14b" }, { RegisterIds::R15, "r15", "r15d", "r15w", "r15b" },
    };

    for (const auto &reg : gprs)
    {
        gpr8->addRegister(reg.r8, 8, 0, {});
        auto *d8 = gpr8->getReg(reg.r8);

        gpr16->addRegister(reg.r16, 16, 0, { d8 });
        auto *d16 = gpr16->getReg(reg.r16);

        gpr32->addRegister(reg.r32, 32, 0, { d8, d16 });
        auto *d32 = gpr32->getReg(reg.r32);

        gpr64->addRegister(reg.r64, 64, 0, { d8, d16, d32 });
    }

    gprBank->addClass("GPR64", gpr64);
    gprBank->addClass("GPR32", gpr32);
    gprBank->addClass("GPR16", gpr16);
    gprBank->addClass("GPR8", gpr8);
    banks.push_back(gprBank);

    // ==========================================
    // 2. FPR BANK SETUP
    // ==========================================
    auto *fprBank = pAlloc.new_object<MirRegisterBank>("FPR", alloc);
    auto *fpr64 = pAlloc.new_object<MirRegisterClass>("FPR64", fprBank, alloc);
    auto *fpr32 = pAlloc.new_object<MirRegisterClass>("FPR32", fprBank, alloc);

    Banks::FPR = fprBank;

    struct FprDef
    {
        size_t id;
        const char *r64;
        const char *r32;
    };
    FprDef fprs[] = {
        { RegisterIds::XMM0, "xmm0", "xmm0_s" }, { RegisterIds::XMM1, "xmm1", "xmm1_s" },
        { RegisterIds::XMM2, "xmm2", "xmm2_s" }, { RegisterIds::XMM3, "xmm3", "xmm3_s" },
        { RegisterIds::XMM4, "xmm4", "xmm4_s" }, { RegisterIds::XMM5, "xmm5", "xmm5_s" },
        { RegisterIds::XMM6, "xmm6", "xmm6_s" }, { RegisterIds::XMM7, "xmm7", "xmm7_s" },
    };

    for (const auto &reg : fprs)
    {
        fpr32->addRegister(reg.r32, 32, 0, {});
        auto *d32 = fpr32->getReg(reg.r32);
        fpr64->addRegister(reg.r64, 64, 0, { d32 });
    }

    fprBank->addClass("FPR64", fpr64);
    fprBank->addClass("FPR32", fpr32);
    banks.push_back(fprBank);

    return banks;
}

}; // namespace EzTestTriple