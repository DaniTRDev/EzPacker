#ifndef EZPACKER_INSTRUCTIONTABLE_H
#define EZPACKER_INSTRUCTIONTABLE_H

#include "EzAnnotatorCommon.h"

/**
 * This file contains the table used to map instruction names to IDs. It also contains utils to check the validity
 * of an instruction.
 */

inline std::map<std::string, size_t> g_InstructionTable = { { "add", 1 },
                                                            { "sub", 2 },
                                                            { "mul", 3 },
                                                            { "div", 4 },
                                                            { "mod", 5 },
                                                            { "neg", 6 }, // Unary negation

                                                            // Logical
                                                            { "and", 10 },
                                                            { "or", 11 },
                                                            { "xor", 12 },
                                                            { "not", 13 }, // Unary bitwise not

                                                            // Comparison
                                                            { "cmp_eq", 20 }, // ==
                                                            { "cmp_ne", 21 }, // !=
                                                            { "cmp_lt", 22 }, // <
                                                            { "cmp_le", 23 }, // <=
                                                            { "cmp_gt", 24 }, // >
                                                            { "cmp_ge", 25 }, // >=

                                                            // Control flow
                                                            { "jmp", 30 }, // Unconditional jump
                                                            { "br", 31 },  // Conditional branch
                                                            { "ret", 32 }, // Return from function

                                                            // Memory access
                                                            { "load", 40 },  // Load from memory or another variable
                                                            { "store", 41 }, // Store to memory

                                                            // Stack-like behavior (optional depending on your IR model)
                                                            { "push", 50 },
                                                            { "pop", 51 },

                                                            // Function call support
                                                            { "call", 60 },
                                                            { "call_indirect", 61 }, // For function pointers

                                                            // Shift and rotate
                                                            { "shl", 70 },
                                                            { "shr", 71 },
                                                            { "sar", 72 },
                                                            { "rol", 73 },
                                                            { "ror", 74 },

                                                            // Floating-point operations (Not supported, yet)
                                                            { "fadd", 80 },
                                                            { "fsub", 81 },
                                                            { "fmul", 82 },
                                                            { "fdiv", 83 },
                                                            { "fsqrt", 84 },
                                                            { "fcmp_eq", 85 },
                                                            { "fcmp_lt", 86 },
                                                            { "fcmp_gt", 87 },

                                                            // Bit manipulation
                                                            { "bt", 90 },  // Bit test
                                                            { "bsf", 91 }, // Bit scan forward
                                                            { "bsr", 92 }, // Bit scan reverse

                                                            // SIMD-like (vector ops)
                                                            { "vadd", 100 },
                                                            { "vsub", 101 },
                                                            { "vmul", 102 },
                                                            { "vdiv", 103 },

                                                            // System or barriers
                                                            { "nop", 110 } };

#endif // EZPACKER_INSTRUCTIONTABLE_H
