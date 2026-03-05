/**
 * @file MirInstructionSet.h
 * @brief X-macro catalogue of every MIR instruction.
 *
 * This file is included multiple times with different definitions of
 * the INSTRUCTION(NAME, OPERAND_COUNT, FLAGS) macro to generate:
 *   - The MirInstructionOpCode enum values.
 *   - The g_MirInstructionSet[] metadata table.
 *   - Per-opcode emit helpers in MirEmitter (emitMOV, emitADD, …).
 *
 * To add a new instruction, append a single INSTRUCTION() line in the
 * appropriate section below — everything else is generated automatically.
 */
#ifdef INSTRUCTION

INSTRUCTION(INVALID, 0, None)

/* --- DATA MOVEMENT -------------------------------------------------------- */
/* Basic Copy: dstReg = src. Handles Register to Register or Register to Immediate. */
INSTRUCTION(MOV, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm)
INSTRUCTION(LEA, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeMem)

/* Conditional operations read the CPU flags left by a previous CMP/TEST       */
INSTRUCTION(CLOAD, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeMem | ReadsMemory | SizeMatch | ReadsCPUFlags)
INSTRUCTION(CSTORE,
            2,
            Op1_Read | Op1_MustBeMem | Op2_Read | Op2_MustBeReg | WritesMemory | HasSideEffect | ReadsCPUFlags)

/* --- MEMORY ACCESS -------------------------------------------------------- */
INSTRUCTION(LOAD, 2, IsLoad)
INSTRUCTION(STORE, 2, IsStore)
INSTRUCTION(CREATE, 1, Op1_Write | Op1_MustBeReg | HasSideEffect)

/* --- ARITHMETIC (ALU) ----------------------------------------------------- */
/* ReadWrite shortcut automatically includes WritesCPUFlags */
INSTRUCTION(ADD, 2, ReadWrite | SizeMatch)
INSTRUCTION(SUB, 2, ReadWrite | SizeMatch)
INSTRUCTION(MUL, 2, ReadWrite | SizeMatch)
INSTRUCTION(IMUL, 2, ReadWrite | SizeMatch)
INSTRUCTION(DIV, 2, ReadWrite | SizeMatch)
INSTRUCTION(IDIV, 2, ReadWrite | SizeMatch)
INSTRUCTION(REM, 2, ReadWrite | SizeMatch)
INSTRUCTION(NEG, 1, Op1_Read | Op1_Write | Op1_MustBeReg | WritesCPUFlags)

/* --- BITWISE LOGIC -------------------------------------------------------- */
INSTRUCTION(AND, 2, ReadWrite | SizeMatch)
INSTRUCTION(OR, 2, ReadWrite | SizeMatch)
INSTRUCTION(XOR, 2, ReadWrite | SizeMatch)
INSTRUCTION(NOT, 1, Op1_Read | Op1_Write | Op1_MustBeReg | WritesCPUFlags)
INSTRUCTION(SHL, 2, ReadWrite)
INSTRUCTION(SHR, 2, ReadWrite)
INSTRUCTION(SAR, 2, ReadWrite)

/* --- CONTROL FLOW --------------------------------------------------------- */
/* Compare & Test ONLY write flags; they do not modify their operands.        */
INSTRUCTION(CMP, 2, Op1_Read | Op2_Read | SizeMatch | WritesCPUFlags)
INSTRUCTION(TEST, 2, Op1_Read | Op2_Read | SizeMatch | WritesCPUFlags)

/* Unconditional Jump. Touches no flags.                                      */
INSTRUCTION(JMP, 1, Op1_Read | IsTerminator)

/* Conditional Jumps read the CPU flags.                                      */
INSTRUCTION(JE, 1, Op1_Read | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JNE, 1, Op1_Read | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JG, 1, Op1_Read | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JGE, 1, Op1_Read | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JL, 1, Op1_Read | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JLE, 1, Op1_Read | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JA, 1, Op1_Read | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JB, 1, Op1_Read | IsTerminator | IsBranch | ReadsCPUFlags)

/* Function Call/Return.                                                      */
INSTRUCTION(CALL, 1, Op1_Read | IsCall | HasSideEffect)
INSTRUCTION(RET, 1, Op1_Read | IsTerminator | IsReturn | HasSideEffect)

/* --- TYPE CASTING --------------------------------------------------------- */
INSTRUCTION(TRUNC, 2, Op1_Write | Op1_MustBeReg | Op2_Read | DestSmaller)
INSTRUCTION(ZEXT, 2, Op1_Write | Op1_MustBeReg | Op2_Read | DestLarger)
INSTRUCTION(SEXT, 2, Op1_Write | Op1_MustBeReg | Op2_Read | DestLarger)
INSTRUCTION(BITCAST, 2, Op1_Write | Op1_MustBeReg | Op2_Read | SizeMatch)

/* --- SYSTEM & SPECIAL ----------------------------------------------------- */
INSTRUCTION(SYSCALL, 1, Op1_Read | HasSideEffect)
INSTRUCTION(NOP, 0, None)
INSTRUCTION(HALT, 0, IsTerminator | HasSideEffect)

#endif // INSTRUCTION
