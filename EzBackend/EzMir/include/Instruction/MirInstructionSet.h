#ifdef INSTRUCTION

// This code is needed not to collide with gtest's TEST macro.
#ifdef TEST
#define BG_TEST_WAS_DEFINED
#pragma push_macro("TEST")
#undef TEST
#endif

#define OPERAND_CONSTRAINTS(...)                                                                                       \
    {                                                                                                                  \
        __VA_ARGS__                                                                                                    \
    }
// Helper to keep the flags readable without polluting the global namespace
#define F(x) MirInstructionFlags::x
#define T(x) MirInstructionTier::x

INSTRUCTION(INVALID, T(HighLevel), MirCat_Invalid, OPERAND_CONSTRAINTS(), F(None))

/* --- DATA MOVEMENT -------------------------------------------------------- */
INSTRUCTION(MOV,
            T(HighLevel),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(None))

INSTRUCTION(LEA,
            T(HighLevel),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::AddressSource, OperandFlag::Read }),
            F(None))

INSTRUCTION(PUSH_ARG,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(HasSideEffect))

INSTRUCTION(PUSH_RET,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(HasSideEffect))

INSTRUCTION(POP_ARG,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::Register, OperandFlag::Write }),
            F(HasSideEffect))

INSTRUCTION(END_ARG,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read }),
            F(HasSideEffect))

INSTRUCTION(POP_RET,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::Register, OperandFlag::Write }),
            F(HasSideEffect))

/* --- MEMORY ACCESS -------------------------------------------------------- */
INSTRUCTION(LOAD,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::AddressSource, OperandFlag::Read }),
            F(ReadsMemory))

INSTRUCTION(STORE,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AddressSource, OperandFlag::Write },
                                { ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(WritesMemory) | F(HasSideEffect))

INSTRUCTION(PUSH,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read }),
            F(ReadsMemory))

INSTRUCTION(POP,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write }),
            F(WritesMemory) | F(HasSideEffect))

INSTRUCTION(ALLOC,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write }),
            F(HasSideEffect))

INSTRUCTION(DALLOC,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(HasSideEffect))

/* --- ARITHMETIC (INTEGER ALU) --------------------------------------------- */
INSTRUCTION(ADD,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(ADC,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(ReadsCPUFlags) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(SUB,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(SBB,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(ReadsCPUFlags) | F(WritesCPUFlags))

INSTRUCTION(MUL,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(IMUL,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative) | F(TreatAsSigned))

INSTRUCTION(DIV,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(IDIV,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(TreatAsSigned))

INSTRUCTION(REM,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(NEG,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite }),
            F(WritesCPUFlags))

/* --- ARITHMETIC (FLOATING-POINT ALU) --------------------------------------- */
INSTRUCTION(FADD,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

INSTRUCTION(FSUB,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch))

INSTRUCTION(FMUL,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

INSTRUCTION(FDIV,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch))

/* --- BITWISE LOGIC -------------------------------------------------------- */
INSTRUCTION(AND,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(OR,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(XOR,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(NOT,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(WritesCPUFlags))

INSTRUCTION(SHL,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(WritesCPUFlags))

INSTRUCTION(SHR,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(WritesCPUFlags))

INSTRUCTION(SAR,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::ReadWrite },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(WritesCPUFlags) | F(TreatAsSigned))

/* --- CONTROL FLOW & COMPARISONS ------------------------------------------- */
INSTRUCTION(CMP,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(FCMP,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags))

INSTRUCTION(TEST,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, OperandFlag::Read }),
            F(SizeMatch) | F(WritesCPUFlags) | F(IsCommutative))

INSTRUCTION(JMP,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch))

INSTRUCTION(JE,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JNE,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JG,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JGE,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JL,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JLE,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JA,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(JB,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, OperandFlag::Read }),
            F(IsTerminator) | F(IsBranch) | F(ReadsCPUFlags))

INSTRUCTION(CALL,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Reference | ExpectedOperandType::Register |
                                          ExpectedOperandType::RuntimeSymbol,
                                  OperandFlag::Read }),
            F(IsCall) | F(HasSideEffect))

INSTRUCTION(RET,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(IsTerminator) | F(IsReturn) | F(HasSideEffect))

/* --- TYPE CASTING & CONVERSIONS ------------------------------------------- */
INSTRUCTION(TRUNC,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(DestSmaller))

INSTRUCTION(ZEXT,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(SEXT,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(FPEXT,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(DestLarger))

INSTRUCTION(FPTRUNC,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(DestSmaller))

INSTRUCTION(SITOFP,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(None))

INSTRUCTION(FPTOSI,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(None))

INSTRUCTION(BITCAST,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, OperandFlag::Write },
                                { ExpectedOperandType::Register, OperandFlag::Read }),
            F(SizeMatch))

/* --- SYSTEM & SPECIAL ----------------------------------------------------- */
INSTRUCTION(SYSCALL,
            T(HighLevel),
            MirCat_System,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, OperandFlag::Read }),
            F(HasSideEffect))

INSTRUCTION(NOP, T(HighLevel), MirCat_System, OPERAND_CONSTRAINTS(), F(None))
INSTRUCTION(HALT, T(HighLevel), MirCat_System, OPERAND_CONSTRAINTS(), F(IsTerminator) | F(HasSideEffect))

// Used internally to mark a MirInstruction as a target instruction during ISel
INSTRUCTION(TARGET_INST, T(TargetLow), MirCat_System, OPERAND_CONSTRAINTS(), F(HasSideEffect))

#undef T
#undef F
#undef OPERAND_CONSTRAINTS

#ifdef BG_TEST_WAS_DEFINED
#pragma pop_macro("TEST")
#undef BG_TEST_WAS_DEFINED
#endif

#endif // INSTRUCTION