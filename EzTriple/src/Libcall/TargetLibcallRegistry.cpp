#include "Libcall/TargetLibcallRegistry.h"
#include "Type/MirType.h"

TargetLibcallRegistry::TargetLibcallRegistry()
{
    m_availability.set();
}

void TargetLibcallRegistry::initDefaults(std::string_view arch, std::string_view os, CrtFlavor crtFlavor)
{
    m_crtFlavor = crtFlavor;

    if (os == "windows" || os == "win32" || os == "msvc")
    {
        m_crtFlavor = CrtFlavor::Msvc;
    }
    else if (os == "darwin" || os == "macos" || os == "ios")
    {
        m_crtFlavor = CrtFlavor::Darwin;
    }
    else if (os == "none" || os == "baremetal")
    {
        m_crtFlavor = CrtFlavor::BareMetal;
        setHosted(false);
    }

    if (m_crtFlavor == CrtFlavor::Msvc)
    {
        // On 32-bit x86 MSVC, standard 64-bit integer ops map to _alldiv, etc.
        if (arch == "i386" || arch == "i686" || arch == "x86")
        {
            setLibcallName(LibcallKind::DivI64, "_alldiv");
            setLibcallName(LibcallKind::UDivI64, "_aulldiv");
            setLibcallName(LibcallKind::RemI64, "_allrem");
            setLibcallName(LibcallKind::URemI64, "_aullrem");
            setLibcallName(LibcallKind::MulI64, "_allmul");
            setLibcallName(LibcallKind::ShlI64, "_allshl");
            setLibcallName(LibcallKind::LShrI64, "_aullshr");
            setLibcallName(LibcallKind::AShrI64, "_allshr");
        }
    }
}

std::string_view TargetLibcallRegistry::getLibcallName(LibcallKind kind) const
{
    auto it = m_symbolOverrides.find(kind);
    if (it != m_symbolOverrides.end())
    {
        return it->second;
    }
    return getDefaultLibcallName(kind, m_crtFlavor);
}

void TargetLibcallRegistry::setLibcallName(LibcallKind kind, std::string_view name)
{
    m_symbolOverrides[kind] = std::string(name);
}

CallingConvDesc *TargetLibcallRegistry::getCallingConvention(LibcallKind kind) const
{
    auto it = m_callingConvs.find(kind);
    if (it != m_callingConvs.end())
    {
        return it->second;
    }
    return nullptr;
}

void TargetLibcallRegistry::setCallingConvention(LibcallKind kind, CallingConvDesc *cc)
{
    m_callingConvs[kind] = cc;
}

bool TargetLibcallRegistry::isAvailable(LibcallKind kind) const
{
    auto idx = static_cast<size_t>(kind);
    if (idx < m_availability.size())
    {
        return m_availability.test(idx);
    }
    return false;
}

void TargetLibcallRegistry::setAvailable(LibcallKind kind, bool available)
{
    auto idx = static_cast<size_t>(kind);
    if (idx < m_availability.size())
    {
        m_availability.set(idx, available);
    }
}

void TargetLibcallRegistry::setHosted(bool isHosted)
{
    m_isHosted = isHosted;
    if (!m_isHosted)
    {
        // In freestanding mode, disable hosted C-rt math and process functions
        // while preserving freestanding memory intrinsics (memcpy, memmove, memset, memcmp).
        for (size_t i = 0; i < static_cast<size_t>(LibcallKind::COUNT); ++i)
        {
            auto kind = static_cast<LibcallKind>(i);
            if (isCRuntime(kind))
            {
                if (kind != LibcallKind::Memcpy && kind != LibcallKind::Memmove &&
                    kind != LibcallKind::Memset && kind != LibcallKind::Memcmp &&
                    kind != LibcallKind::Bzero)
                {
                    setAvailable(kind, false);
                }
            }
        }
    }
}

void TargetLibcallRegistry::setCrtFlavor(CrtFlavor flavor)
{
    m_crtFlavor = flavor;
}

uint16_t TargetLibcallRegistry::registerCustomLibcall(std::string_view name, LibcallSignature sig)
{
    for (size_t i = 0; i < m_customLibcalls.size(); ++i)
    {
        if (m_customLibcalls[i].first == name)
        {
            return static_cast<uint16_t>(i);
        }
    }
    m_customLibcalls.emplace_back(std::string(name), std::move(sig));
    return static_cast<uint16_t>(m_customLibcalls.size() - 1);
}

std::string_view TargetLibcallRegistry::getCustomLibcallName(uint16_t id) const
{
    if (id < m_customLibcalls.size())
    {
        return m_customLibcalls[id].first;
    }
    return {};
}

std::optional<LibcallKind> TargetLibcallRegistry::findKindByName(std::string_view name) const
{
    // First, check explicit overrides
    for (const auto &[kind, sym] : m_symbolOverrides)
    {
        if (sym == name)
        {
            return kind;
        }
    }

    // Next, check standard names under current flavor
    for (size_t i = 0; i < static_cast<size_t>(LibcallKind::COUNT); ++i)
    {
        auto kind = static_cast<LibcallKind>(i);
        if (getDefaultLibcallName(kind, m_crtFlavor) == name)
        {
            return kind;
        }
    }

    // Fallback: check GNU defaults if flavor is different
    if (m_crtFlavor != CrtFlavor::Gnu)
    {
        for (size_t i = 0; i < static_cast<size_t>(LibcallKind::COUNT); ++i)
        {
            auto kind = static_cast<LibcallKind>(i);
            if (getDefaultLibcallName(kind, CrtFlavor::Gnu) == name)
            {
                return kind;
            }
        }
    }

    return std::nullopt;
}

std::optional<LibcallKind> TargetLibcallRegistry::findKindForOpcode(MirInstructionOpCode op, MirType *type) const
{
    if (!type)
    {
        return std::nullopt;
    }

    size_t bitWidth = type->getTotalSizeInBits();
    auto kind = type->getKind();

    if (kind == MirTypeKind::Integer)
    {
        switch (op)
        {
            case MirInstructionOpCode::SDIV:
            case MirInstructionOpCode::IDIV:
                if (bitWidth == 32) return LibcallKind::DivI32;
                if (bitWidth == 64) return LibcallKind::DivI64;
                if (bitWidth == 128) return LibcallKind::DivI128;
                break;
            case MirInstructionOpCode::UDIV:
            case MirInstructionOpCode::DIV:
                if (bitWidth == 32) return LibcallKind::UDivI32;
                if (bitWidth == 64) return LibcallKind::UDivI64;
                if (bitWidth == 128) return LibcallKind::UDivI128;
                break;
            case MirInstructionOpCode::REM:
                if (bitWidth == 32) return LibcallKind::RemI32;
                if (bitWidth == 64) return LibcallKind::RemI64;
                if (bitWidth == 128) return LibcallKind::RemI128;
                break;
            case MirInstructionOpCode::MUL:
            case MirInstructionOpCode::IMUL:
                if (bitWidth == 64) return LibcallKind::MulI64;
                if (bitWidth == 128) return LibcallKind::MulI128;
                break;
            case MirInstructionOpCode::SHL:
                if (bitWidth == 64) return LibcallKind::ShlI64;
                if (bitWidth == 128) return LibcallKind::ShlI128;
                break;
            case MirInstructionOpCode::SHR:
                if (bitWidth == 64) return LibcallKind::LShrI64;
                if (bitWidth == 128) return LibcallKind::LShrI128;
                break;
            case MirInstructionOpCode::SAR:
                if (bitWidth == 64) return LibcallKind::AShrI64;
                if (bitWidth == 128) return LibcallKind::AShrI128;
                break;
            default:
                break;
        }
    }
    else if (kind == MirTypeKind::FloatingPoint)
    {
        switch (op)
        {
            case MirInstructionOpCode::FADD:
                if (bitWidth == 32) return LibcallKind::AddF32;
                if (bitWidth == 64) return LibcallKind::AddF64;
                if (bitWidth == 128) return LibcallKind::AddF128;
                break;
            case MirInstructionOpCode::FSUB:
                if (bitWidth == 32) return LibcallKind::SubF32;
                if (bitWidth == 64) return LibcallKind::SubF64;
                if (bitWidth == 128) return LibcallKind::SubF128;
                break;
            case MirInstructionOpCode::FMUL:
                if (bitWidth == 32) return LibcallKind::MulF32;
                if (bitWidth == 64) return LibcallKind::MulF64;
                if (bitWidth == 128) return LibcallKind::MulF128;
                break;
            case MirInstructionOpCode::FDIV:
                if (bitWidth == 32) return LibcallKind::DivF32;
                if (bitWidth == 64) return LibcallKind::DivF64;
                if (bitWidth == 128) return LibcallKind::DivF128;
                break;
            default:
                break;
        }
    }

    return std::nullopt;
}
