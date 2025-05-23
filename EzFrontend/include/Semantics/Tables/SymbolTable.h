#ifndef EZPACKER_SYMBOLTABLE_H
#define EZPACKER_SYMBOLTABLE_H

#include "EzFrontendCommon.h"
#include "SemanticTable.h"

enum class SymbolType : uint8_t
{
    Invalid = 0,
    Module,
    Variable,
    VirtualVariable
};

// TODO: Add debug data -> Make a proper Debug generator.

struct SymbolEntry
{
    SymbolType m_type;
    std::string m_name;
};

class SymbolTable : public SemanticTable<SymbolEntry>
{
};
#endif // EZPACKER_SYMBOLTABLE_H
