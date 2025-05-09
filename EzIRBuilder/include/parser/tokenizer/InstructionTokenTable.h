#ifndef EZPACKER_INSTRUCTIONTOKENTABLE_H
#define EZPACKER_INSTRUCTIONTOKENTABLE_H

#include "EzIRBuilderCommon.h"

inline std::set<std::string> g_instructionTable = {
    // Arithmetic
    ".add", ".div", ".mul", ".sub",
    
    // Logic
    ".and", ".not", ".or",
    
    // Bit displacements
    ".rotl", ".rotr", ".shl", ".shr",
    
    // Memory
    ".freeStack", ".load", ".pop", ".push", ".reserveStack", ".store",
    
    // Code Flow
    ".ret"
};

#endif // EZPACKER_INSTRUCTIONTOKENTABLE_H
