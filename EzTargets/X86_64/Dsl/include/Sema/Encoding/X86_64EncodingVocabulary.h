#ifndef EZDSL_SEMA_X86_64_ENCODING_VOCABULARY_H
#define EZDSL_SEMA_X86_64_ENCODING_VOCABULARY_H

#include <string_view>
#include <utility>

namespace Sema::Encoding::X86Vocab
{

/**
 * Single source of truth for the x86-64 ENCODING vocabulary.
 *
 * Each entry pairs the DSL spelling used in `.idf` sources with the generated runtime enum
 * spelling consumed by the encoding table. The semantic dialect validates names against these
 * tables and the code generation backend maps them to enumerators, so a form/field can never
 * pass semantic analysis yet silently emit `None` during code generation.
 */
inline constexpr std::pair<std::string_view, std::string_view> kForms[] = {
    { "rr", "EncForm::Rr" },         { "rm", "EncForm::Rm" },       { "mr", "EncForm::Mr" },
    { "ri", "EncForm::Ri" },         { "movri", "EncForm::MovRI" }, { "movzx", "EncForm::Movzx" },
    { "movsx", "EncForm::Movsx" },   { "lea", "EncForm::Lea" },     { "unary", "EncForm::Unary" },
    { "test", "EncForm::Test" },     { "shift", "EncForm::Shift" }, { "imul_rr", "EncForm::ImulRR" },
    { "imul_ri", "EncForm::ImulRI" }, { "div", "EncForm::Div" },    { "jcc", "EncForm::Jcc" },
    { "jmp", "EncForm::Jmp" },       { "call", "EncForm::Call" },   { "ret", "EncForm::Ret" },
    { "push", "EncForm::Push" },     { "pop", "EncForm::Pop" },     { "nop", "EncForm::Nop" },
    { "syscall", "EncForm::Syscall" }, { "setcc", "EncForm::Setcc" }, { "sse", "EncForm::Sse" },
    { "cvt", "EncForm::Cvt" }
};

inline constexpr std::pair<std::string_view, std::string_view> kFields[] = {
    { "reg", "EncSlotKind::Reg" },       { "rm_reg", "EncSlotKind::RmReg" }, { "rm_mem", "EncSlotKind::RmMem" },
    { "imm8", "EncSlotKind::Imm8" },     { "imm16", "EncSlotKind::Imm16" },  { "imm32", "EncSlotKind::Imm32" },
    { "imm64", "EncSlotKind::Imm64" },   { "imm8_signed", "EncSlotKind::Imm8Signed" },
    { "rel8", "EncSlotKind::Rel8" },     { "rel32", "EncSlotKind::Rel32" },  { "cc", "EncSlotKind::CondCode" }
};

/** Returns true when form is a known x86-64 ENCODING form. */
inline bool isKnownForm(std::string_view form)
{
    for (const auto &[name, _] : kForms)
    {
        if (name == form)
        {
            return true;
        }
    }
    return false;
}

/** Returns true when field is a known x86-64 operand field. */
inline bool isKnownField(std::string_view field)
{
    for (const auto &[name, _] : kFields)
    {
        if (name == field)
        {
            return true;
        }
    }
    return false;
}

/** Maps an x86-64 form name to its generated EncForm enumerator spelling, or "EncForm::None". */
inline std::string_view formEnum(std::string_view form)
{
    for (const auto &[name, enumerator] : kForms)
    {
        if (name == form)
        {
            return enumerator;
        }
    }
    return "EncForm::None";
}

/** Maps an x86-64 field name to its generated EncSlotKind enumerator spelling, or "EncSlotKind::None". */
inline std::string_view slotEnum(std::string_view field)
{
    for (const auto &[name, enumerator] : kFields)
    {
        if (name == field)
        {
            return enumerator;
        }
    }
    return "EncSlotKind::None";
}

} // namespace Sema::Encoding::X86Vocab

#endif // EZDSL_SEMA_X86_64_ENCODING_VOCABULARY_H
