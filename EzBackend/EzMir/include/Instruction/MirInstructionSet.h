/**
 * @file MirInstructionSet.h
 * @brief X-macro source list containing every MIR opcode definition.
 *
 * Syntax: INSTRUCTION(Name, Category, Operands, Flags)
 */
#ifdef INSTRUCTION

// This code is needed not to collide with gtest's TEST macro.
#ifdef TEST
#define BG_TEST_WAS_DEFINED
#pragma push_macro("TEST")
#undef TEST
#endif

#define OPERAND_CONSTRAINTS(...) { __VA_ARGS__ }
// Helper to keep the flags readable without polluting the global namespace
#define F(x) MirInstructionFlags::x

INSTRUCTION(INVALID, MirCat_Invalid, OPERAND_CONSTRAINTS(), F(None))

/* --- DATA MOVEMENT -------------------------------------------------------- */
INSTRUCTION(MOV,
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(None))

// LEA specifically requests an AddressSource (MirCat_Memory or FrameIndex)
INSTRUCTION(LEA,
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::AddressSource, OperandFlag::Read }),
            F(None))

INSTRUCTION(PUSH_ARG,
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read }),
            F(HasSideEffect))

INSTRUCTION(POP_ARG,
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write }),
            F(HasSideEffect))

INSTRUCTION(PUSH_RET,
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write }),
            F(HasSideEffect))

INSTRUCTION(POP_RET,
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Integer, OperandFlag::Read },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(HasSideEffect))

/* --- MEMORY ACCESS -------------------------------------------------------- */
// LOAD forces a MirCat_Memory operand as the source
INSTRUCTION(LOAD,
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Memory, OperandFlag::Read }),
            F(ReadsMemory))

// STORE forces a MirCat_Memory operand as the destination
INSTRUCTION(STORE,
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Memory, OperandFlag::Write },
                                { ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(WritesMemory) | F(HasSideEffect))

INSTRUCTION(CREATE,
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write }),
            F(HasSideEffect))

/* --- ARITHMETIC (ALU) ----------------------------------------------------- */
INSTRUCTION(ADD,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(ADC,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(ReadsCPUFlags) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(SUB,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(SBB,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(ReadsCPUFlags) | F(WritesCPUFlags))

INSTRUCTION(MUL,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(IMUL,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative) | F(TreatAsSigned))

INSTRUCTION(DIV,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(IDIV,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(TreatAsSigned))

INSTRUCTION(REM,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(NEG,
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite }),
            F(WritesCPUFlags))

/* --- BITWISE LOGIC -------------------------------------------------------- */
INSTRUCTION(AND,
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(OR,
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(XOR,
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(NOT,
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite }),
            F(WritesCPUFlags))

INSTRUCTION(SHL,
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(WritesCPUFlags))

INSTRUCTION(SHR,
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(WritesCPUFlags))

INSTRUCTION(SAR,
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(WritesCPUFlags) | F(TreatAsSigned))

/* --- CONTROL FLOW --------------------------------------------------------- */
INSTRUCTION(CMP,
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(TEST,
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(JMP,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch))

INSTRUCTION(JE,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JNE,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JG,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JGE,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JL,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JLE,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JA,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JB,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(CALL,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Reference | ExpectedOperandType::Register |
                                          ExpectedOperandType::RuntimeSymbol,
                                  OperandFlag::Read }),
            F(IsCall) | F(HasSideEffect))

INSTRUCTION(RET,
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(IsTerminator) | F(IsReturn) | F(HasSideEffect))

/* --- TYPE CASTING --------------------------------------------------------- */
INSTRUCTION(TRUNC,
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Integer, OperandFlag::Read }),
            F(DestSmaller))

INSTRUCTION(ZEXT,
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(SEXT,
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(FPEXT,
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegFloatImm, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(BITCAST,
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch))

/* --- SYSTEM & SPECIAL ----------------------------------------------------- */
INSTRUCTION(SYSCALL,
            MirCat_System,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(HasSideEffect))

INSTRUCTION(NOP, MirCat_System, OPERAND_CONSTRAINTS(), F(None))
INSTRUCTION(HALT, MirCat_System, OPERAND_CONSTRAINTS(), F(IsTerminator) | F(HasSideEffect))

#undef F
#undef OPERAND_CONSTRAINTS

#ifdef BG_TEST_WAS_DEFINED
#pragma pop_macro("TEST")
#undef BG_TEST_WAS_DEFINED
#endif

#endif // INSTRUCTION