#ifndef EZDSL_TARGET_INST_PASS_H
#define EZDSL_TARGET_INST_PASS_H

#include "EzDslCommon.h"
#include "Ast/InstructionDefLangAst.h"
#include "Sema/Symbols/Symbols.h"

/**
 * Semantic analysis pass processing target instruction definitions and binary encoding formats (.idf).
 * Validates format bitfield overlaps, bit-slice bounds, operand class resolutions, assembly template placeholders,
 * and FORMAT assignment bit expressions.
 */
class InstructionDefPass
{
  public:
    /**
     * Executes the instruction definition semantic analysis pass.
     * Returns true if all formats, instructions, operands, assignments, and flags were successfully resolved.
     */
    static bool
    run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::InstDef::InstDefFile *file);

  private:
    /**
     * Declares instruction format symbols in the symbol table.
     */
    static bool declareFormats(class DiagnosticCollector *collector,
                               class SymbolTable *table,
                               DSL::Ast::InstDef::InstDefFile *file);

    /**
     * Validates bitfield boundaries, slice ranges, and detects overlapping bitfields within a format.
     */
    static bool validateFormat(class DiagnosticCollector *collector,
                               class SymbolTable *table,
                               const DSL::Ast::InstDef::InstFormatDecl &formatDecl,
                               SymbolId formatSymId);

    /**
     * Declares instruction opcode symbols in the symbol table.
     */
    static bool declareInstructions(class DiagnosticCollector *collector,
                                    class SymbolTable *table,
                                    DSL::Ast::InstDef::InstDefFile *file);

    /**
     * Validates a single instruction declaration against its referenced encoding format.
     */
    static bool validateInstruction(class DiagnosticCollector *collector,
                                    class SymbolTable *table,
                                    const DSL::Ast::InstDef::InstDecl &instDecl);

    /**
     * Resolves operand types/classes (e.g. GPR, simm(i12)) against declared register banks and primitive types.
     */
    static bool resolveOperands(class DiagnosticCollector *collector,
                                class SymbolTable *table,
                                const DSL::Ast::InstDef::InstDecl &instDecl,
                                std::pmr::vector<Sema::Symbols::TargetOperandSymbol> &outArgs,
                                std::pmr::vector<Sema::Symbols::TargetOperandSymbol> &outImplicitArgs);

    /**
     * Validates that assembly string placeholders (e.g. ${rd}, ${rs1}) refer to declared instruction operands.
     */
    static bool validateAsmTemplate(DiagnosticCollector *collector,
                                    const DSL::Ast::InstDef::InstDecl &instDecl,
                                    const std::unordered_set<std::string_view> &validOperands);

    /**
     * Validates FORMAT(...) assignments against the bitfields of the instruction's format.
     */
    static bool validateFieldAssignments(class DiagnosticCollector *collector,
                                         class SymbolTable *table,
                                         const DSL::Ast::InstDef::InstDecl &instDecl,
                                         const Sema::Symbols::InstructionFormatSymbol &formatSym);

    /**
     * Resolves FORMAT(...) assignments into evaluated FieldAssignmentSymbol entries.
     */
    static bool validateFieldAssignments(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         const DSL::Ast::InstDef::InstDecl &instDecl,
                                         const Sema::Symbols::InstructionFormatSymbol &formatSym,
                                         std::pmr::vector<Sema::Symbols::FieldAssignmentSymbol> &outAssignments);

    /**
     * Validates and combines behavioral instruction flags into a bitmask.
     */
    static bool aggregateAndValidateFlags(class DiagnosticCollector *collector,
                                          uint32_t &out,
                                          const DSL::Ast::InstDef::InstDecl &instDecl);
};

#endif // EZDSL_TARGET_INST_PASS_H