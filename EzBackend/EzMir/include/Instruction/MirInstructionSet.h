/**
 * @file MirInstructionSet.h
 * @brief X-macro source list containing every MIR opcode definition.
 *
 * Syntax: INSTRUCTION(Name, Category, LinearEquivalent, Operands, Flags)
 */
#ifdef INSTRUCTION

// This code is needed not to collide with gtest's TEST macro.
#ifdef TEST
#define BG_TEST_WAS_DEFINED
#pragma push_macro("TEST")
#undef TEST
#endif

#define OPERAND_CONSTRAINTS(...) { __VA_ARGS__ }
#define NO_EQUIV { MirInstructionOpCode::INVALID, MirInstructionOpCode::INVALID }
#define EQUIV(high, low) { MirInstructionOpCode::high, MirInstructionOpCode::low }

// Helper to keep the flags readable without polluting the global namespace
#define F(x) MirInstructionFlags::x

INSTRUCTION(INVALID, Invalid, NO_EQUIV, OPERAND_CONSTRAINTS(), F(None))

/* --- DATA MOVEMENT -------------------------------------------------------- */
INSTRUCTION(MOV,
            DataMovement,
            EQUIV(MOV, MOV),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(None))

// LEA specifically requests an AddressSource (Memory or FrameIndex)
INSTRUCTION(LEA,
            DataMovement,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::AddressSource, OperandFlag::Read }),
            F(None))

/* --- MEMORY ACCESS -------------------------------------------------------- */
// LOAD forces a Memory operand as the source
INSTRUCTION(LOAD,
            Memory,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Memory, OperandFlag::Read }),
            F(ReadsMemory))

// STORE forces a Memory operand as the destination
INSTRUCTION(STORE,
            Memory,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Memory, OperandFlag::Write },
                                { ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(WritesMemory) | F(HasSideEffect))

INSTRUCTION(CREATE,
            Memory,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write }),
            F(HasSideEffect))

/* --- ARITHMETIC (ALU) ----------------------------------------------------- */
INSTRUCTION(ADD,
            Arithmetic,
            EQUIV(ADC, ADD),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(ADC,
            Arithmetic,
            EQUIV(ADC, ADC),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(ReadsCPUFlags) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(SUB,
            Arithmetic,
            EQUIV(SBB, SUB),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(SBB,
            Arithmetic,
            EQUIV(SBB, SBB),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(ReadsCPUFlags) | F(WritesCPUFlags))

INSTRUCTION(MUL,
            Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(IMUL,
            Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative) | F(TreatAsSigned))

INSTRUCTION(DIV,
            Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(IDIV,
            Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(TreatAsSigned))

INSTRUCTION(REM,
            Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(NEG,
            Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite }),
            F(WritesCPUFlags))

/* --- BITWISE LOGIC -------------------------------------------------------- */
INSTRUCTION(AND,
            Bitwise,
            EQUIV(AND, AND),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(OR,
            Bitwise,
            EQUIV(OR, OR),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(XOR,
            Bitwise,
            EQUIV(XOR, XOR),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(NOT,
            Bitwise,
            EQUIV(NOT, NOT),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite }),
            F(WritesCPUFlags))

INSTRUCTION(SHL,
            Bitwise,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(WritesCPUFlags))

INSTRUCTION(SHR,
            Bitwise,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(WritesCPUFlags))

INSTRUCTION(SAR,
            Bitwise,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(WritesCPUFlags) | F(TreatAsSigned))

/* --- CONTROL FLOW --------------------------------------------------------- */
INSTRUCTION(CMP,
            Compare,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(TEST,
            Compare,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(JMP,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch))

INSTRUCTION(JE,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JNE,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JG,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JGE,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JL,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JLE,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JA,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JB,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(CALL,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference | ExpectedOperandType::Register, OperandFlag::Read }),
            F(IsCall) | F(HasSideEffect))

INSTRUCTION(RET,
            ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(IsTerminator) | F(IsReturn) | F(HasSideEffect))

/* --- TYPE CASTING --------------------------------------------------------- */
INSTRUCTION(TRUNC,
            Casting,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(DestSmaller))

INSTRUCTION(ZEXT,
            Casting,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(SEXT,
            Casting,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(BITCAST,
            Casting,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch))

/* --- SYSTEM & SPECIAL ----------------------------------------------------- */
INSTRUCTION(SYSCALL,
            System,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(HasSideEffect))

INSTRUCTION(NOP, System, NO_EQUIV, OPERAND_CONSTRAINTS(), F(None))
INSTRUCTION(HALT, System, NO_EQUIV, OPERAND_CONSTRAINTS(), F(IsTerminator) | F(HasSideEffect))

#undef F
#undef OPERAND_CONSTRAINTS
#undef NO_EQUIV
#undef EQUIV

#ifdef BG_TEST_WAS_DEFINED
#pragma pop_macro("TEST")
#undef BG_TEST_WAS_DEFINED
#endif

#endif // INSTRUCTION