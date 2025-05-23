#ifndef EZPACKER_TYPETABLE_H
#define EZPACKER_TYPETABLE_H

#include "EzFrontendCommon.h"
#include "SemanticTable.h"

struct TypeEntry
{
    size_t m_size; // Size in bits.
    std::string m_name;
};

class TypeTable : public SemanticTable<TypeEntry>
{
};

#endif // EZPACKER_TYPETABLE_H
