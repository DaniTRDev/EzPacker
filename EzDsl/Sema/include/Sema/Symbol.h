#ifndef EZDSLSEMA_SYMBOL_H
#define EZDSLSEMA_SYMBOL_H

#include "EzDslSemaCommon.h"
#include "Symbols/IrSymbols.h"
#include "Symbols/TypeSymbols.h"
#include "Symbols/LegalizeSymbols.h"
#include "Symbols/TargetSymbols.h"
#include "Symbols/InstructionSelectSymbols.h"
#include "Symbols/CallingConvSymbols.h"
#include "Symbols/RegisterSymbols.h"
#include "Symbols/TargetDescSymbols.h"

/**
 * Discriminator enum identifying the syntactic/semantic category of a Symbol.
 */
enum class SymbolType : uint8_t
{
    // .tyf Type File
    Type,

    // .irdf IR instruction entities.
    IrInstruction,
    IrInstructionOperand,

    // .lad files.
    LegalizeAction,
    TypeSet,

    // .lrd files.
    LegalizeRule,

    // .idf Target Instruction files.
    TargetInstruction,
    TargetOperand,

    // .isf Instruction Selection files.
    AddressingMode,
    SelectionPattern,

    // .ezcc / .ccd Calling Convention files.
    CallingConv,

    // .reg Register Definition files.
    RegisterFile,
    RegisterBank,
    RegisterClass,
    Register,
    SpecialRegister,

    // .tdesc Target Descriptor files.
    TargetDesc,

    // Reusable symbol types.
    SsaVariable,
    ImmediateVariable
};

/**
 * Universal symbol table entry representing named definitions across all EzDsl sub-languages.
 * Holds typed semantic payload in a std::variant.
 */
class Symbol
{
  public:
    /**
     * Variant holding the typed semantic payload for any supported declaration kind; the active
     * alternative is determined by the symbol's SymbolType.
     */
    using SymbolData = std::variant<std::monostate,
                                    Symbols::TypeSymbol,
                                    Symbols::IrInstructionSymbol,
                                    Symbols::IrOperandSymbol,
                                    Symbols::LegalizeActionConstraintSymbol,
                                    Symbols::LegalizeActionClauseSymbol,
                                    Symbols::LegalizeActionSymbol,
                                    Symbols::TypeSetSymbol,
                                    Symbols::LegalizeRuleOperandSymbol,
                                    Symbols::LegalizeRuleInstructionSymbol,
                                    Symbols::LegalizeRuleSymbol,
                                    Symbols::TargetOperandSymbol,
                                    Symbols::TargetInstructionSymbol,
                                    Symbols::AddrModeSymbol,
                                    Symbols::SelectionPatternSymbol,
                                    Symbols::CallingConvSymbol,
                                    Symbols::RegisterFileSymbol,
                                    Symbols::RegisterBankSymbol,
                                    Symbols::RegisterClassSymbol,
                                    Symbols::RegisterSymbol,
                                    Symbols::SpecialRegisterSymbol,
                                    Symbols::TargetDescSymbol>;

    /**
     * Constructs a symbol with source location span, flags, defining scope ID, unique symbol ID, type, and name.
     */
    Symbol(class SourceReference *sourceRef,
           SymbolId definingScope,
           SymbolId id,
           SymbolType type,
           std::string_view name);

    /**
     * Checks if the symbol holds semantic data of type T in its variant payload.
     */
    template <typename T> bool hasData() const { return std::holds_alternative<T>(m_data); }

    /**
     * Returns the SourceReference span representing the symbol's declaration location.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Returns the unique ID of the lexical/semantic scope that contains this symbol.
     */
    SymbolId getDefiningScopeId() const;

    /**
     * Returns the unique numeric ID assigned to this symbol.
     */
    SymbolId getId() const;

    /**
     * Returns the symbol category type.
     */
    SymbolType getType() const;

    /**
     * Returns a mutable pointer to the payload if it matches type T, or nullptr otherwise.
     */
    template <typename T> T *getIf() { return std::get_if<T>(&m_data); }

    /**
     * Const overload returning a read-only pointer to the payload of type T, or nullptr if unmatched.
     */
    template <typename T> const T *getIf() const { return std::get_if<T>(&m_data); }

    /**
     * Sets or overwrites the symbol's semantic data payload.
     */
    void setData(SymbolData data);

    /**
     * Returns the textual identifier name of the symbol.
     */
    const std::string_view &getName() const;

  private:
    class SourceReference *m_sourceRef; // Source span of the declaration.
    SymbolData m_data;                  // Typed semantic payload.
    SymbolId m_definingScopeId;         // Same datatype as ScopeId but we can't use here...
    SymbolId m_id;                      // Unique ID within the symbol table.
    SymbolType m_type;                  // Classification of the symbol.
    std::string_view m_name;            // Declared identifier name.
};

#endif // EZDSLSEMA_SYMBOL_H