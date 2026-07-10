#include "Amd64.h"

DEFINE_EXPANSION_RECIPES(AMD64)

/* --- DATA MOVEMENT -------------------------------------------------------- */

// Split 128-bit or wide movement into independent lower and upper slots
RECIPE_FOR(MOV)
EMIT_INST(MOV, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(MOV, Expansion::D_HI, Expansion::S0_HI)
END_RECIPE

/* --- MEMORY ACCESS -------------------------------------------------------- */

// Sequentially load low chunk from base address, then high chunk with a stride offset
RECIPE_FOR(LOAD)
EMIT_INST(LOAD, Expansion::D_LO, MEM_HALF(S0_LO, 0))
EMIT_INST(LOAD, Expansion::D_HI, MEM_HALF(S0_LO, 1))
END_RECIPE

// Sequentially store low and high pieces into memory
RECIPE_FOR(STORE)
EMIT_INST(STORE, MEM_HALF(D_LO, 0), Expansion::S0_LO)
EMIT_INST(STORE, MEM_HALF(D_LO, 1), Expansion::S0_HI)
END_RECIPE

/* --- ARITHMETIC (ALU) ----------------------------------------------------- */

// Multi-precision addition: ADD sets EFLAGS.CF, ADC consumes it
RECIPE_FOR(ADD)
EMIT_INST(ADD, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(ADC, Expansion::D_HI, Expansion::S0_HI)
END_RECIPE

// Carry-aware addition continuation chain
RECIPE_FOR(ADC)
EMIT_INST(ADC, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(ADC, Expansion::D_HI, Expansion::S0_HI)
END_RECIPE

// Multi-precision subtraction: SUB sets EFLAGS.CF (borrow), SBB consumes it
RECIPE_FOR(SUB)
EMIT_INST(SUB, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(SBB, Expansion::D_HI, Expansion::S0_HI)
END_RECIPE

// Borrow-aware subtraction continuation chain
RECIPE_FOR(SBB)
EMIT_INST(SBB, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(SBB, Expansion::D_HI, Expansion::S0_HI)
END_RECIPE

// Wide Multiplications: Safely routed to the runtime math library
RECIPE_FOR(MUL)
EMIT_RT_CALL("__multi3",
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::S0_LO,
             Expansion::S0_HI)
END_RECIPE

RECIPE_FOR(IMUL)
EMIT_RT_CALL("__multi3",
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::S0_LO,
             Expansion::S0_HI)
END_RECIPE

// Unsigned Division: D = D / S0
RECIPE_FOR(DIV)
EMIT_RT_CALL("__udivti3",
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::S0_LO,
             Expansion::S0_HI)
END_RECIPE

// Signed Division: D = D / S0
RECIPE_FOR(IDIV)
EMIT_RT_CALL("__divti3",
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::S0_LO,
             Expansion::S0_HI)
END_RECIPE

// Remainder/Modulo: D = D % S0
RECIPE_FOR(REM)
EMIT_RT_CALL("__umodti3",
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::S0_LO,
             Expansion::S0_HI)
END_RECIPE

// Two's complement bitwise negation + 1 bit injection to simulate wide negation
RECIPE_FOR(NEG)
EMIT_INST(NOT, Expansion::D_LO)
EMIT_INST(NOT, Expansion::D_HI)
EMIT_INST(ADD, Expansion::D_LO, INT_IMM(1))
EMIT_INST(ADC, Expansion::D_HI, INT_IMM(0))
END_RECIPE

/* --- BITWISE LOGIC -------------------------------------------------------- */

RECIPE_FOR(AND)
EMIT_INST(AND, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(AND, Expansion::D_HI, Expansion::S0_HI)
END_RECIPE

RECIPE_FOR(OR)
EMIT_INST(OR, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(OR, Expansion::D_HI, Expansion::S0_HI)
END_RECIPE

RECIPE_FOR(XOR)
EMIT_INST(XOR, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(XOR, Expansion::D_HI, Expansion::S0_HI)
END_RECIPE

RECIPE_FOR(NOT)
EMIT_INST(NOT, Expansion::D_LO)
EMIT_INST(NOT, Expansion::D_HI)
END_RECIPE

/* --- SHIFTS --- */

RECIPE_FOR(SHL)
EMIT_RT_CALL("__ashlti3",
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::S0_LO,
             Expansion::S0_HI)
END_RECIPE

RECIPE_FOR(SHR)
EMIT_RT_CALL("__lshrti3",
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::S0_LO,
             Expansion::S0_HI)
END_RECIPE

RECIPE_FOR(SAR)
EMIT_RT_CALL("__ashrti3",
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::D_LO,
             Expansion::D_HI,
             Expansion::S0_LO,
             Expansion::S0_HI)
END_RECIPE

/* --- COMPARISONS ---------------------------------------------------------- */

RECIPE_FOR(CMP)
EMIT_INST(MOV, Expansion::T0_LO, Expansion::D_LO)
EMIT_INST(SUB, Expansion::T0_LO, Expansion::S0_LO)
EMIT_INST(MOV, Expansion::T0_HI, Expansion::D_HI)
EMIT_INST(SBB, Expansion::T0_HI, Expansion::S0_HI)
END_RECIPE

RECIPE_FOR(TEST)
EMIT_INST(MOV, Expansion::T0_LO, Expansion::D_LO)
EMIT_INST(AND, Expansion::T0_LO, Expansion::S0_LO)
EMIT_INST(MOV, Expansion::T0_HI, Expansion::D_HI)
EMIT_INST(AND, Expansion::T0_HI, Expansion::S0_HI)
EMIT_INST(OR, Expansion::T0_LO, Expansion::T0_HI)
END_RECIPE

/* --- TYPE CASTING --------------------------------------------------------- */

RECIPE_FOR(TRUNC)
EMIT_INST(MOV, Expansion::D_LO, Expansion::S0_LO)
END_RECIPE

RECIPE_FOR(ZEXT)
EMIT_INST(MOV, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(MOV, Expansion::D_HI, INT_IMM(0))
END_RECIPE

RECIPE_FOR(SEXT)
EMIT_INST(MOV, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(MOV, Expansion::D_HI, Expansion::S0_LO)
EMIT_INST(SAR, Expansion::D_HI, INT_IMM(63))
END_RECIPE

RECIPE_FOR(BITCAST)
EMIT_INST(MOV, Expansion::D_LO, Expansion::S0_LO)
EMIT_INST(MOV, Expansion::D_HI, Expansion::S0_HI)
END_RECIPE

END_EXPANSION_RECIPES