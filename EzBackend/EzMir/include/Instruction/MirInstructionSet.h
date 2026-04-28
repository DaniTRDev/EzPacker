/**
 * @file MirInstructionSet.h
 * @brief X-macro source list containing every MIR opcode definition.
 *
 * Syntax: INSTRUCTION(Name, Category, Operands, Flags)
 */
#ifdef INSTRUCTION

#define OPERAND_CONSTRAINTS(...) { __VA_ARGS__ }

INSTRUCTION(INVALID, Invalid, OPERAND_CONSTRAINTS(), None)

/* --- ARRAY MANAGEMENT -------------------------------------------------------- */
INSTRUCTION(GETARR,
            Array,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Reference, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            None)

INSTRUCTION(SETARR,
            Array,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            WritesMemory)

/* --- DATA MOVEMENT -------------------------------------------------------- */
INSTRUCTION(MOV,
            DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            None)

INSTRUCTION(LEA,
            DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Reference | ExpectedOperandType::RegImm, OperandFlag::Read }),
            None)

/* --- MEMORY ACCESS -------------------------------------------------------- */
INSTRUCTION(LOAD,
            Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::Immediate, OperandFlag::Read }),
            ReadsMemory)

INSTRUCTION(STORE,
            Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::Immediate, OperandFlag::Read },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            WritesMemory | HasSideEffect)

INSTRUCTION(CREATE, Memory, OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write }), HasSideEffect)

/* --- ARITHMETIC (ALU) ----------------------------------------------------- */
INSTRUCTION(ADD,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags | IsCommutative)

/* NEW: Add with Carry (used for multi-register expansion) */
INSTRUCTION(ADC,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | ReadsCPUFlags | WritesCPUFlags | IsCommutative)

INSTRUCTION(SUB,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags)

/* NEW: Subtract with Borrow (used for multi-register expansion) */
INSTRUCTION(SBB,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | ReadsCPUFlags | WritesCPUFlags)

INSTRUCTION(MUL,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags | IsCommutative)

INSTRUCTION(IMUL,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags | IsCommutative)

INSTRUCTION(DIV,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags)

INSTRUCTION(IDIV,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags)

INSTRUCTION(REM,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags)

INSTRUCTION(NEG,
            Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite }),
            WritesCPUFlags)

/* --- BITWISE LOGIC -------------------------------------------------------- */
INSTRUCTION(AND,
            Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags | IsCommutative)

INSTRUCTION(OR,
            Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags | IsCommutative)

INSTRUCTION(XOR,
            Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags | IsCommutative)

INSTRUCTION(NOT,
            Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite }),
            WritesCPUFlags)

INSTRUCTION(SHL,
            Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            WritesCPUFlags)

INSTRUCTION(SHR,
            Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            WritesCPUFlags)

INSTRUCTION(SAR,
            Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            WritesCPUFlags)

/* --- CONTROL FLOW --------------------------------------------------------- */
INSTRUCTION(CMP,
            Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags)

INSTRUCTION(TEST,
            Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch | WritesCPUFlags | IsCommutative)

INSTRUCTION(JMP,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            IsTerminator | IsBranch)

INSTRUCTION(JE,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JNE,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JG,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JGE,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JL,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JLE,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JA,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            IsTerminator | IsBranch | ReadsCPUFlags)
INSTRUCTION(JB,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            IsTerminator | IsBranch | ReadsCPUFlags)

INSTRUCTION(CALL,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference | ExpectedOperandType::Register, OperandFlag::Read }),
            IsCall | HasSideEffect)
INSTRUCTION(RET,
            ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::RegImm, OperandFlag::Read }),
            IsTerminator | IsReturn | HasSideEffect)

/* --- TYPE CASTING --------------------------------------------------------- */
INSTRUCTION(TRUNC,
            Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            DestSmaller)

INSTRUCTION(ZEXT,
            Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            DestLarger)

INSTRUCTION(SEXT,
            Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            DestLarger)

INSTRUCTION(BITCAST,
            Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            SizeMatch)

/* --- SYSTEM & SPECIAL ----------------------------------------------------- */
INSTRUCTION(SYSCALL, System, OPERAND_CONSTRAINTS({ ExpectedOperandType::RegImm, OperandFlag::Read }), HasSideEffect)
INSTRUCTION(NOP, System, OPERAND_CONSTRAINTS(), None)
INSTRUCTION(HALT, System, OPERAND_CONSTRAINTS(), IsTerminator | HasSideEffect)

#undef OPERAND_CONSTRAINTS
#endif // INSTRUCTION