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

INSTRUCTION(INVALID, MirCat_Invalid, NO_EQUIV, OPERAND_CONSTRAINTS(), F(None))

/* --- DATA MOVEMENT -------------------------------------------------------- */
INSTRUCTION(MOV,
            MirCat_DataMovement,
            EQUIV(MOV, MOV),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(None))

// LEA specifically requests an AddressSource (MirCat_Memory or FrameIndex)
INSTRUCTION(LEA,
            MirCat_DataMovement,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::AddressSource, OperandFlag::Read }),
            F(None))

/* --- MEMORY ACCESS -------------------------------------------------------- */
// LOAD forces a MirCat_Memory operand as the source
INSTRUCTION(LOAD,
            MirCat_Memory,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Memory, OperandFlag::Read }),
            F(ReadsMemory))

// STORE forces a MirCat_Memory operand as the destination
INSTRUCTION(STORE,
            MirCat_Memory,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Memory, OperandFlag::Write },
                                { ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(WritesMemory) | F(HasSideEffect))

INSTRUCTION(CREATE,
            MirCat_Memory,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write }),
            F(HasSideEffect))

/* --- ARITHMETIC (ALU) ----------------------------------------------------- */
INSTRUCTION(ADD,
            MirCat_Arithmetic,
            EQUIV(ADC, ADD),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(ADC,
            MirCat_Arithmetic,
            EQUIV(ADC, ADC),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(ReadsCPUFlags) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(SUB,
            MirCat_Arithmetic,
            EQUIV(SBB, SUB),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(SBB,
            MirCat_Arithmetic,
            EQUIV(SBB, SBB),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(ReadsCPUFlags) | F(WritesCPUFlags))

INSTRUCTION(MUL,
            MirCat_Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(IMUL,
            MirCat_Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative) | F(TreatAsSigned))

INSTRUCTION(DIV,
            MirCat_Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(IDIV,
            MirCat_Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(TreatAsSigned))

INSTRUCTION(REM,
            MirCat_Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(NEG,
            MirCat_Arithmetic,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite }),
            F(WritesCPUFlags))

/* --- BITWISE LOGIC -------------------------------------------------------- */
INSTRUCTION(AND,
            MirCat_Bitwise,
            EQUIV(AND, AND),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(OR,
            MirCat_Bitwise,
            EQUIV(OR, OR),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(XOR,
            MirCat_Bitwise,
            EQUIV(XOR, XOR),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(NOT,
            MirCat_Bitwise,
            EQUIV(NOT, NOT),
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite }),
            F(WritesCPUFlags))

INSTRUCTION(SHL,
            MirCat_Bitwise,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(WritesCPUFlags))

INSTRUCTION(SHR,
            MirCat_Bitwise,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(WritesCPUFlags))

INSTRUCTION(SAR,
            MirCat_Bitwise,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(WritesCPUFlags) | F(TreatAsSigned))

/* --- CONTROL FLOW --------------------------------------------------------- */
INSTRUCTION(CMP,
            MirCat_Compare,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(TEST,
            MirCat_Compare,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(JMP,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch))

INSTRUCTION(JE,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JNE,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JG,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JGE,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JL,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JLE,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JA,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JB,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(CALL,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference | ExpectedOperandType::Register, OperandFlag::Read }),
            F(IsCall) | F(HasSideEffect))

INSTRUCTION(RET,
            MirCat_ControlFlow,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(IsTerminator) | F(IsReturn) | F(HasSideEffect))

/* --- TYPE CASTING --------------------------------------------------------- */
INSTRUCTION(TRUNC,
            MirCat_Casting,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Integer, OperandFlag::Read }),
            F(DestSmaller))

INSTRUCTION(ZEXT,
            MirCat_Casting,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(SEXT,
            MirCat_Casting,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(FPEXT,
            MirCat_Casting,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegFloatImm, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(BITCAST,
            MirCat_Casting,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch))

/* --- SYSTEM & SPECIAL ----------------------------------------------------- */
INSTRUCTION(SYSCALL,
            MirCat_System,
            NO_EQUIV,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(HasSideEffect))

INSTRUCTION(NOP, MirCat_System, NO_EQUIV, OPERAND_CONSTRAINTS(), F(None))
INSTRUCTION(HALT, MirCat_System, NO_EQUIV, OPERAND_CONSTRAINTS(), F(IsTerminator) | F(HasSideEffect))

#undef F
#undef OPERAND_CONSTRAINTS
#undef NO_EQUIV
#undef EQUIV

#ifdef BG_TEST_WAS_DEFINED
#pragma pop_macro("TEST")
#undef BG_TEST_WAS_DEFINED
#endif

#endif // INSTRUCTION