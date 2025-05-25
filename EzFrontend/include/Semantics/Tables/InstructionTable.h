#ifndef EZPACKER_INSTRUCTIONTABLE_H
#define EZPACKER_INSTRUCTIONTABLE_H

#include "EzFrontendCommon.h"
#include "SemanticTable.h"

struct InstructionEntry
{
    size_t m_id;
};

/**
 * This class will be used to know which instructions does our IR support as well as some other special information.
 * Will be filled using scripting utility.
 */
class InstructionTable : public SemanticTable<InstructionEntry>
{

};

#endif // EZPACKER_INSTRUCTIONTABLE_H
