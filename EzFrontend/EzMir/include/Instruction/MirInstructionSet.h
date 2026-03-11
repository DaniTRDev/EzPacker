/**
 * @file MirInstructionSet.h
 * @brief X-macro source list containing every MIR opcode definition.
 *
 * Each row defines the public contract of one opcode: its symbolic name, the
 * exact operand count expected by `MirEmitter`, and the semantic flags exposed
 * through `MirInstructionMetadata`. Every constraint is fully explicit.
 */
#ifdef INSTRUCTION

INSTRUCTION(INVALID, 0, None)

/* --- DATA MOVEMENT -------------------------------------------------------- */
INSTRUCTION(MOV, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm)
INSTRUCTION(LEA, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeMem)

INSTRUCTION(CLOAD, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeMem | ReadsMemory | SizeMatch | ReadsCPUFlags)
INSTRUCTION(CSTORE,
            2,
            Op1_Read | Op1_MustBeMem | Op2_Read | Op2_MustBeReg | WritesMemory | HasSideEffect | ReadsCPUFlags)

/* --- MEMORY ACCESS -------------------------------------------------------- */
INSTRUCTION(LOAD, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeMem | IsLoad | ReadsMemory)
INSTRUCTION(STORE, 2, Op1_Read | Op1_MustBeMem | Op2_Read | Op2_MustBeRegOrImm | IsStore | WritesMemory | HasSideEffect)
INSTRUCTION(CREATE, 1, Op1_Write | Op1_MustBeReg | HasSideEffect)

/* --- ARITHMETIC (ALU) ----------------------------------------------------- */
/* Explicitly defined: Dest is Read/Write Reg, Src is Read Reg/Imm, Sets Flags, Sizes Match */
INSTRUCTION(ADD, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)
INSTRUCTION(SUB, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)
INSTRUCTION(MUL, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)
INSTRUCTION(IMUL, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)
INSTRUCTION(DIV, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)
INSTRUCTION(IDIV, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)
INSTRUCTION(REM, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)

INSTRUCTION(NEG, 1, Op1_Read | Op1_Write | Op1_MustBeReg | WritesCPUFlags)

/* --- BITWISE LOGIC -------------------------------------------------------- */
INSTRUCTION(AND, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)
INSTRUCTION(OR, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)
INSTRUCTION(XOR, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)

INSTRUCTION(NOT, 1, Op1_Read | Op1_Write | Op1_MustBeReg | WritesCPUFlags)

/* Note: Shift instructions rarely require the sizes to match (e.g., shifting an i64 by an i8 amount is valid) */
INSTRUCTION(SHL, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | WritesCPUFlags)
INSTRUCTION(SHR, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | WritesCPUFlags)
INSTRUCTION(SAR, 2, Op1_Read | Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | WritesCPUFlags)

/* --- CONTROL FLOW --------------------------------------------------------- */
INSTRUCTION(CMP, 2, Op1_Read | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)
INSTRUCTION(TEST, 2, Op1_Read | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch | WritesCPUFlags)

INSTRUCTION(JMP, 1, Op1_Read | Op1_MustBeRef | IsTerminator | IsBranch)

INSTRUCTION(JE, 1, Op1_Read | Op1_MustBeRef | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JNE, 1, Op1_Read | Op1_MustBeRef | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JG, 1, Op1_Read | Op1_MustBeRef | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JGE, 1, Op1_Read | Op1_MustBeRef | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JL, 1, Op1_Read | Op1_MustBeRef | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JLE, 1, Op1_Read | Op1_MustBeRef | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JA, 1, Op1_Read | Op1_MustBeRef | IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JB, 1, Op1_Read | Op1_MustBeRef | IsTerminator | IsBranch | ReadsCPUFlags)

INSTRUCTION(CALL, 1, Op1_Read | Op1_MustBeRef | IsCall | HasSideEffect)
INSTRUCTION(RET, 1, Op1_Read | Op1_MustBeRegOrImm | IsTerminator | IsReturn | HasSideEffect)

/* --- TYPE CASTING --------------------------------------------------------- */
INSTRUCTION(TRUNC, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | DestSmaller)
INSTRUCTION(ZEXT, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | DestLarger)
INSTRUCTION(SEXT, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | DestLarger)
INSTRUCTION(BITCAST, 2, Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeRegOrImm | SizeMatch)

/* --- SYSTEM & SPECIAL ----------------------------------------------------- */
INSTRUCTION(SYSCALL, 1, Op1_Read | Op1_MustBeRegOrImm | HasSideEffect)
INSTRUCTION(NOP, 0, None)
INSTRUCTION(HALT, 0, IsTerminator | HasSideEffect)

#endif // INSTRUCTION