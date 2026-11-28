#ifndef EZCORE_DEBUG_INFO_H
#define EZCORE_DEBUG_INFO_H

#include "Rtti/RttiDescriptor.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace EzCore::Debug
{

/**
 * Granularity of debug information to emit.
 */
enum class DebugLevel : uint8_t
{
    None = 0,           ///< Emit no debug information.
    LineTablesOnly = 1, ///< Emit only file/line mappings for stack traces.
    Full = 2            ///< Emit full type, variable, location, and line information.
};

/**
 * Target debug information container format.
 */
enum class DebugFormat : uint8_t
{
    Auto = 0, ///< Select format automatically from the target triple (DWARF5 for ELF, CodeView for COFF/PE).
    Dwarf5 = 1,
    CodeView = 2
};

/**
 * Source language identifier (aligned with DWARF DW_LANG constants).
 */
enum class DebugLanguage : uint16_t
{
    C11 = 0x001D, ///< ISO C11 (DW_LANG_C11).
    Mir = 0x8001  ///< EzPacker MIR (vendor-defined language code).
};

/**
 * Base-type encoding classification (aligned with DWARF DW_ATE_* encodings).
 */
enum class DebugBaseEncoding : uint8_t
{
    None = 0,
    Address,
    Boolean,
    Float,
    Signed,
    SignedChar,
    Unsigned,
    UnsignedChar
};

/**
 * Represents a source file registered within a compile unit's file table.
 */
struct DebugSourceFile
{
    uint32_t fileId{ 0 };    ///< 1-based unique file identifier within the compile unit.
    std::string directory;   ///< Directory component (without trailing separator, except root).
    std::string filename;    ///< Base filename component.
    std::string fullPath;    ///< Normalized full path used for deduplication.
};

/**
 * Source coordinate referencing a registered file, line, and column.
 */
struct DebugSourceLoc
{
    uint32_t fileId{ 0 }; ///< 1-based file ID (0 indicates unknown/synthetic location).
    uint32_t line{ 0 };   ///< 1-based source line number (0 indicates unknown).
    uint32_t column{ 0 }; ///< 1-based source column number (0 indicates unknown).
};

/**
 * Describes a field/member of a composite type (struct or union).
 */
struct DebugMember
{
    std::string name;         ///< Member identifier.
    uint64_t typeId{ 0 };     ///< Type ID of the member's type.
    uint32_t byteOffset{ 0 }; ///< Byte offset from the start of the enclosing type.
    uint16_t bitOffset{ 0 };  ///< Bit offset within the storage unit for bitfields.
    uint16_t bitWidth{ 0 };   ///< Bit width for bitfields (0 if not a bitfield).
    bool isBitfield{ false }; ///< True if this member is a bitfield.
};

/**
 * Describes a single enumerator constant within an enum type.
 */
struct DebugEnumerator
{
    std::string name;  ///< Enumerator identifier.
    int64_t value{ 0 }; ///< Signed integral value of the enumerator.
};

/**
 * Target-independent representation of a source-level type.
 */
struct DebugType
{
    uint64_t typeId{ 0 };                                       ///< Unique 64-bit type identifier (e.g. FNV-1a).
    std::string name;                                           ///< Type name (empty for anonymous/derived types if unnamed).
    EzCore::RttiTypeKind kind{ EzCore::RttiTypeKind::Unknown }; ///< Structural kind of the type.
    DebugBaseEncoding encoding{ DebugBaseEncoding::None };      ///< Base-type encoding for primitive types.
    uint64_t sizeInBytes{ 0 };                                  ///< Size in bytes.
    uint32_t alignment{ 0 };                                    ///< Alignment in bytes.
    uint64_t referencedTypeId{ 0 };                             ///< Pointee, element, underlying, or aliased type ID.
    uint64_t arrayLength{ 0 };                                  ///< Element count for array or vector types.
    std::vector<DebugMember> members;                           ///< Fields for struct/union types.
    std::vector<DebugEnumerator> enumerators;                   ///< Enumerators for enum types.
    uint64_t returnTypeId{ 0 };                                 ///< Return type ID for function types (0 for void).
    std::vector<uint64_t> paramTypeIds;                         ///< Parameter type IDs for function types.
    bool isVariadic{ false };                                   ///< True if function type accepts variadic arguments.
    DebugSourceLoc declLoc;                                     ///< Declaration site in source.
};

/**
 * Storage location kind for a debug variable or parameter.
 */
enum class DebugLocationKind : uint8_t
{
    None = 0,           ///< Location unavailable or optimized out.
    FrameBaseOffset,    ///< Relative to frame base (e.g. RBP/FP + offset).
    StackPointerOffset, ///< Relative to stack pointer (e.g. RSP/SP + offset).
    Register,           ///< Held directly in a hardware register.
    GlobalSymbol        ///< Located at a named global/static symbol.
};

/**
 * Target-independent location expression for a variable.
 */
struct DebugVarLocation
{
    DebugLocationKind kind{ DebugLocationKind::None }; ///< How to interpret the location fields.
    int64_t offset{ 0 };                               ///< Signed byte offset for frame/stack-relative locations.
    uint32_t regId{ 0 };                               ///< Target hardware register index.
    bool isFpr{ false };                               ///< True if regId refers to a floating-point/vector register.
    std::string symbolName;                            ///< Symbol name for GlobalSymbol locations.
};

/**
 * Describes a function parameter or local variable.
 */
struct DebugVariable
{
    std::string name;               ///< Variable identifier.
    uint64_t typeId{ 0 };           ///< Type ID of the variable.
    bool isParameter{ false };      ///< True if this variable is a formal parameter.
    uint32_t paramIndex{ 0 };       ///< 0-based parameter index (when isParameter is true).
    DebugSourceLoc declLoc;         ///< Declaration site in source.
    DebugVarLocation location;      ///< Runtime storage location.
    uint64_t scopeStartOffset{ 0 }; ///< Function-relative start offset of lexical scope.
    uint64_t scopeEndOffset{ 0 };   ///< Function-relative end offset of lexical scope (0 = entire function).
};

/**
 * Maps a machine-code offset within a function to a source location.
 */
struct DebugLineEntry
{
    uint64_t codeOffset{ 0 };   ///< Byte offset relative to the start of the enclosing function.
    uint32_t fileId{ 0 };       ///< 1-based file ID.
    uint32_t line{ 0 };         ///< 1-based line number.
    uint32_t column{ 0 };       ///< 1-based column number.
    bool isStmt{ true };        ///< Recommended breakpoint location (DWARF is_stmt).
    bool prologueEnd{ false };  ///< Marks the end of the function prologue.
    bool epilogueBegin{ false }; ///< Marks the start of the function epilogue.
};

/**
 * Describes a callee-saved register spilled in a function prologue for CFI/unwind generation.
 */
struct DebugSavedReg
{
    uint32_t regId{ 0 };   ///< Target hardware register index.
    bool isFpr{ false };   ///< True if regId refers to a floating-point/vector register.
    int64_t cfaOffset{ 0 }; ///< Offset from the Canonical Frame Address (CFA) where the register is saved.
};

/**
 * Target-independent debug description of a compiled function.
 */
struct DebugFunction
{
    std::string name;                      ///< Source-level function name.
    std::string linkageName;               ///< Mangled/assembler symbol name (empty if same as name).
    uint64_t returnTypeId{ 0 };            ///< Return type ID (0 for void).
    DebugSourceLoc declLoc;                ///< Declaration site in source.
    uint64_t sectionOffset{ 0 };           ///< Byte offset within the code section.
    uint64_t codeSize{ 0 };                ///< Total machine code size in bytes.
    uint32_t prologueSize{ 0 };            ///< Size of the prologue in bytes.
    bool isExternal{ true };               ///< True if externally visible.
    bool hasFramePointer{ true };          ///< True if the function establishes a frame pointer.
    int64_t totalFrameSize{ 0 };           ///< Total stack frame size in bytes.
    std::vector<DebugSavedReg> savedRegs;  ///< Callee-saved register spill locations.
    std::vector<DebugVariable> parameters; ///< Formal parameters.
    std::vector<DebugVariable> locals;     ///< Local variables.
    std::vector<DebugLineEntry> lineEntries; ///< Line table entries ordered by codeOffset.
};

/**
 * Target-independent debug description of a global or static variable.
 */
struct DebugGlobalVar
{
    std::string name;              ///< Variable/symbol name.
    uint64_t typeId{ 0 };          ///< Type ID of the global variable.
    DebugSourceLoc declLoc;        ///< Declaration site in source.
    bool isExternal{ true };       ///< True if externally visible.
    bool isConstant{ false };      ///< True if read-only / constant.
    uint64_t sizeInBytes{ 0 };     ///< Storage size in bytes.
    std::string sectionName;       ///< Target section name (e.g. ".data", ".rodata", ".bss").
    uint64_t sectionOffset{ 0 };   ///< Byte offset within the section.
};

/**
 * Root container for all target-independent debug information of a translation/compile unit.
 */
struct DebugCompileUnit
{
    /**
     * Target CPU architecture for register mapping and pointer/endianness metadata.
     */
    enum class TargetArch : uint8_t
    {
        X86_64,
        RiscV64
    };

    std::string producer{ "EzPacker ezc 1.0.0" };   ///< Producer identification string.
    DebugLanguage language{ DebugLanguage::C11 };   ///< Primary source language.
    DebugLevel level{ DebugLevel::Full };           ///< Debug emission level.
    std::string compDir;                            ///< Compilation working directory.
    std::string mainFile;                           ///< Primary source file path.
    uint8_t pointerSize{ 8 };                       ///< Target pointer size in bytes (4 or 8).
    bool isLittleEndian{ true };                    ///< Target byte order.
    TargetArch arch{ TargetArch::X86_64 };          ///< Target architecture.

    std::vector<DebugSourceFile> files;             ///< Registered source files (1-based IDs).
    std::vector<DebugType> types;                   ///< Type table.
    std::vector<DebugFunction> functions;           ///< Compiled functions.
    std::vector<DebugGlobalVar> globals;            ///< Global and static variables.

    /**
     * Registers a source file path if not already present and returns its 1-based file ID.
     * Normalizes path separators and splits directory/filename for both Linux ('/') and Windows ('\').
     * Returns 0 if fullPath is empty.
     */
    uint32_t addOrGetFile(std::string_view fullPath);

    /**
     * Adds a type to the compile unit, or updates the existing entry if a type with the same typeId exists.
     */
    void addType(DebugType type);

    /**
     * Looks up a registered type by its 64-bit typeId.
     * Returns nullptr if not found.
     */
    const DebugType *findType(uint64_t typeId) const;

    /**
     * Validates structural invariants of the compile unit (pointer size, unique non-zero IDs,
     * valid file references, non-empty symbol names).
     * Returns true if valid; otherwise writes a diagnostic message to outError and returns false.
     */
    bool validate(std::string &outError) const;
};

/**
 * Maps an x86-64 hardware register index to its System V AMD64 DWARF register number.
 * Returns 0xFFFF if regId is out of range.
 */
uint16_t mapX86_64ToDwarfReg(uint32_t regId, bool isFpr);

/**
 * Maps a RISC-V 64 hardware register index (x0..x31 or f0..f31) to its DWARF register number.
 * Returns 0xFFFF if regId is out of range.
 */
uint16_t mapRiscV64ToDwarfReg(uint32_t regId, bool isFpr);

/**
 * Maps an x86-64 hardware register index and operand size to its Microsoft CodeView CV_HREG_e ID.
 * Returns 0 (CV_AMD64_NOREG) if regId is out of range.
 */
uint16_t mapX86_64ToCodeViewReg(uint32_t regId, bool isFpr, uint32_t byteSize);

/**
 * Splits a file path into directory and filename components, supporting both '/' and '\' separators.
 */
void splitDirectoryAndFilename(std::string_view path, std::string &outDir, std::string &outFile);

} // namespace EzCore::Debug

#endif // EZCORE_DEBUG_INFO_H
