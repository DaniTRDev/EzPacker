#ifndef EZDSL_SEMA_RISCV_ENCODING_VOCABULARY_H
#define EZDSL_SEMA_RISCV_ENCODING_VOCABULARY_H

#include <string_view>
#include <utility>

namespace Sema::Encoding::RiscVVocab
{

/**
 * Single source of truth for the RISC-V ENCODING vocabulary.
 *
 * Each entry pairs the DSL spelling used in `.idf` sources with the generated runtime enum
 * spelling consumed by the encoding table.
 */
inline constexpr std::pair<std::string_view, std::string_view> kForms[] = {
    { "r_type", "EncForm::RType" },
    { "i_type", "EncForm::IType" },
    { "s_type", "EncForm::SType" },
    { "b_type", "EncForm::BType" },
    { "u_type", "EncForm::UType" },
    { "j_type", "EncForm::JType" },
    { "pseudo", "EncForm::Pseudo" },
};

inline constexpr std::pair<std::string_view, std::string_view> kFields[] = {
    { "rd", "EncSlotKind::Rd" },
    { "rs1", "EncSlotKind::Rs1" },
    { "rs2", "EncSlotKind::Rs2" },
    { "rs3", "EncSlotKind::Rs3" },
    { "imm12_signed", "EncSlotKind::Imm12Signed" },
    { "imm12_store", "EncSlotKind::Imm12Store" },
    { "imm12_branch", "EncSlotKind::Imm12Branch" },
    { "imm20_upper", "EncSlotKind::Imm20Upper" },
    { "imm20_jump", "EncSlotKind::Imm20Jump" },
    { "shamt5", "EncSlotKind::Shamt5" },
    { "shamt6", "EncSlotKind::Shamt6" },
    { "rel", "EncSlotKind::Rel" },
    { "csr", "EncSlotKind::Csr" },
};

/** Returns true when form is a known RISC-V ENCODING form. */
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

/** Returns true when field is a known RISC-V operand field. */
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

/** Maps a RISC-V form name to its generated EncForm enumerator spelling, or "EncForm::None". */
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

/** Maps a RISC-V field name to its generated EncSlotKind enumerator spelling, or "EncSlotKind::None". */
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

} // namespace Sema::Encoding::RiscVVocab

#endif // EZDSL_SEMA_RISCV_ENCODING_VOCABULARY_H
