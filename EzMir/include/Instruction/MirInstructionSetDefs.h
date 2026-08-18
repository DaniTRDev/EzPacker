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

/**
 * Sentinel value indicating an uninitialized or illegal instruction.
 */
INSTRUCTION(INVALID, T(HighLevel), MirCat_Invalid, OPERAND_CONSTRAINTS(), F(None))

/* ========================================================================= */
/* --- DATA MOVEMENT ------------------------------------------------------- */
/* ========================================================================= */

/**
 * Copies a value into a destination register: dst = src.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  Source value (Register or Immediate)
 */
INSTRUCTION(MOV,
            T(HighLevel),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::AnyValue, MirOperandFlag::Read }),
            F(None))

/**
 * Loads an effective memory address into a register without dereferencing (like x86 LEA): dst = &src.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  Source Address/Symbol
 */
INSTRUCTION(MOV_ADDR,
            T(HighLevel),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::AddressSource, MirOperandFlag::Read }),
            F(None))

/**
 * SSA Phi function. Selects an incoming register value based on the predecessor basic block.
 * Operands:
 *   [0] Write: Destination Register (new SSA version)
 *   [1..N] Read: Variable list of incoming register values corresponding to CFG predecessors
 */
INSTRUCTION(PHI,
            T(HighLevel),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write }),
            F(VariadicArgs))

/**
 * Internal pass instruction: Places an argument register into a call frame buffer or target slot.
 * Operands:
 *   [0] Read: Binding token
 *   [1] Read: Source Value Register
 */
INSTRUCTION(PUSH_ARG,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(HasSideEffect))

/**
 * Internal pass instruction: Places a return value into the caller's return slot.
 * Operands:
 *   [0] Read: Binding token
 *   [1] Read: Return Value Register
 */
INSTRUCTION(PUSH_RET,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(HasSideEffect))

/**
 * Internal pass instruction: Extracts a passed parameter from the function frame into a register.
 * Operands:
 *   [0] Read:  Binding token
 *   [1] Write: Destination Register
 */
INSTRUCTION(POP_ARG,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::Register, MirOperandFlag::Write }),
            F(HasSideEffect))

/**
 * Internal pass instruction: Marks the end of argument setup sequence before a function call.
 * Operands:
 *   [0] Read: Binding token
 */
INSTRUCTION(END_ARG,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(HasSideEffect))

/**
 * Internal pass instruction: Retrieves a function call's returned value into a local register.
 * Operands:
 *   [0] Read:  Binding token
 *   [1] Write: Destination Register
 */
INSTRUCTION(POP_RET,
            T(PassInternal),
            MirCat_DataMovement,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::Register, MirOperandFlag::Write }),
            F(HasSideEffect))

/* ========================================================================= */
/* --- MEMORY ACCESS ------------------------------------------------------- */
/* ========================================================================= */

/**
 * Loads a value from memory at address [src] into register [dst]: dst = *src.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  Source Memory Address
 */
INSTRUCTION(LOAD,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::AddressSource, MirOperandFlag::Read }),
            F(ReadsMemory))

/**
 * Stores a value into memory at target address [dst]: *dst = src.
 * Operands:
 *   [0] Read: Destination Memory Address
 *   [1] Read: Value to Store (Register or Immediate)
 */
INSTRUCTION(STORE,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AddressSource, MirOperandFlag::Read },
                                { ExpectedOperandType::AnyValue, MirOperandFlag::Read }),
            F(WritesMemory) | F(HasSideEffect))

/**
 * Pushes a register onto the hardware stack.
 * Operands:
 *   [0] Read: Source Register
 */
INSTRUCTION(PUSH,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(ReadsMemory) | F(HasSideEffect))

/**
 * Pops the top value off the hardware stack into a register.
 * Operands:
 *   [0] Write: Destination Register
 */
INSTRUCTION(POP,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write }),
            F(WritesMemory) | F(HasSideEffect))

/**
 * Allocates stack frame space (alloca) and returns the base address pointer.
 * Operands:
 *   [0] Write: Destination Register (Address of allocated stack memory)
 */
INSTRUCTION(ALLOC,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write }),
            F(HasSideEffect))

/**
 * Deallocates or frees dynamically allocated memory.
 * Operands:
 *   [0] Write: Destination Status/Result Register
 *   [1] Read:  Base Address Register
 */
INSTRUCTION(DALLOC,
            T(HighLevel),
            MirCat_Memory,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(HasSideEffect))

/* ========================================================================= */
/* --- ARITHMETIC (INTEGER ALU: 3-OPERAND) ---------------------------------- */
/* ========================================================================= */

/**
 * Integer Addition: dst = src1 + src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(ADD,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Integer Addition with Carry: dst = src1 + src2 + Carry.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(ADC,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Integer Subtraction: dst = src1 - src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register (Minuend)
 *   [2] Read:  RHS (Register or Immediate Subtrahend)
 */
INSTRUCTION(SUB,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Integer Subtraction with Borrow: dst = src1 - src2 - Borrow.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register (Minuend)
 *   [2] Read:  RHS (Register or Immediate Subtrahend)
 */
INSTRUCTION(SBB,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Unsigned Integer Multiplication: dst = src1 * src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(MUL,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Signed Integer Multiplication: dst = src1 * src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(IMUL,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative) | F(TreatAsSigned))

/**
 * Unsigned Integer Division: dst = src1 / src2.
 * Operands:
 *   [0] Write: Destination Register (Quotient)
 *   [1] Read:  LHS Register (Dividend)
 *   [2] Read:  RHS (Register or Immediate Divisor)
 */
INSTRUCTION(DIV,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Signed Integer Division: dst = src1 / src2.
 * Operands:
 *   [0] Write: Destination Register (Quotient)
 *   [1] Read:  LHS Register (Dividend)
 *   [2] Read:  RHS (Register or Immediate Divisor)
 */
INSTRUCTION(IDIV,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(TreatAsSigned))

/**
 * Integer Remainder / Modulo: dst = src1 % src2.
 * Operands:
 *   [0] Write: Destination Register (Remainder)
 *   [1] Read:  LHS Register (Dividend)
 *   [2] Read:  RHS (Register or Immediate Divisor)
 */
INSTRUCTION(REM,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Unary Arithmetic Negation (Two's complement): dst = -src.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  Source Register
 */
INSTRUCTION(NEG,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(None))

/* ========================================================================= */
/* --- ARITHMETIC (FLOATING-POINT ALU: 3-OPERAND) -------------------------- */
/* ========================================================================= */

/**
 * Floating-Point Addition: dst = src1 + src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Float Immediate)
 */
INSTRUCTION(FADD,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Floating-Point Subtraction: dst = src1 - src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Float Immediate)
 */
INSTRUCTION(FSUB,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Floating-Point Multiplication: dst = src1 * src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Float Immediate)
 */
INSTRUCTION(FMUL,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Floating-Point Division: dst = src1 / src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Float Immediate)
 */
INSTRUCTION(FDIV,
            T(HighLevel),
            MirCat_Arithmetic,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch))

/* ========================================================================= */
/* --- BITWISE LOGIC (3-OPERAND) ------------------------------------------- */
/* ========================================================================= */

/**
 * Bitwise AND: dst = src1 & src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(AND,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Bitwise OR: dst = src1 | src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(OR,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Bitwise XOR (Exclusive OR): dst = src1 ^ src2.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(XOR,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Unary Bitwise NOT (One's complement): dst = ~src.
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  Source Register
 */
INSTRUCTION(NOT,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(None))

/**
 * Logical Shift Left: dst = src1 << src2 (zero-fill).
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  Value Register
 *   [2] Read:  Shift Amount (Register or Integer Immediate)
 */
INSTRUCTION(SHL,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, MirOperandFlag::Read }),
            F(None))

/**
 * Logical Shift Right: dst = src1 >> src2 (zero-fill).
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  Value Register
 *   [2] Read:  Shift Amount (Register or Integer Immediate)
 */
INSTRUCTION(SHR,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, MirOperandFlag::Read }),
            F(None))

/**
 * Arithmetic Shift Right: dst = src1 >> src2 (preserves sign bit).
 * Operands:
 *   [0] Write: Destination Register
 *   [1] Read:  Value Register
 *   [2] Read:  Shift Amount (Register or Integer Immediate)
 */
INSTRUCTION(SAR,
            T(HighLevel),
            MirCat_Bitwise,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegIntImm, MirOperandFlag::Read }),
            F(TreatAsSigned))

/* ========================================================================= */
/* --- COMPARISONS & CONTROL FLOW (VALUE-BASED) ---------------------------- */
/* ========================================================================= */

/**
 * Integer Equality Comparison: dst = (src1 == src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_EQ,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Integer Inequality Comparison: dst = (src1 != src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_NE,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(IsCommutative))

/**
 * Signed Integer Less Than: dst = (src1 < src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_SLT,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(TreatAsSigned))

/**
 * Signed Integer Less Than or Equal: dst = (src1 <= src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_SLE,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(TreatAsSigned))

/**
 * Signed Integer Greater Than: dst = (src1 > src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_SGT,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(TreatAsSigned))

/**
 * Signed Integer Greater Than or Equal: dst = (src1 >= src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_SGE,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch) | F(TreatAsSigned))

/**
 * Unsigned Integer Less Than: dst = (src1 < src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_ULT,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Unsigned Integer Less Than or Equal: dst = (src1 <= src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_ULE,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Unsigned Integer Greater Than: dst = (src1 > src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_UGT,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Unsigned Integer Greater Than or Equal: dst = (src1 >= src2). Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Immediate)
 */
INSTRUCTION(CMP_UGE,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Floating-Point Comparison. Returns boolean (i1).
 * Operands:
 *   [0] Write: Destination Boolean Register
 *   [1] Read:  LHS Register
 *   [2] Read:  RHS (Register or Float Immediate)
 */
INSTRUCTION(FCMP,
            T(HighLevel),
            MirCat_Compare,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),
            F(SizeMatch))

/**
 * Unconditional Jump / Branch. Jumps unconditionally to target basic block.
 * Operands:
 *   [0] Read: Target Basic Block Reference / Label
 */
INSTRUCTION(JMP,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, MirOperandFlag::Read }),
            F(IsTerminator) | F(IsBranch))

/**
 * Conditional Branch: if (cond) goto true_block else goto false_block.
 * Operands:
 *   [0] Read: Condition Register (Boolean i1)
 *   [1] Read: True Target Basic Block Reference
 *   [2] Read: False Target Basic Block Reference
 */
INSTRUCTION(BR_COND,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Read },
                                { ExpectedOperandType::Reference, MirOperandFlag::Read },
                                { ExpectedOperandType::Reference, MirOperandFlag::Read }),
            F(IsTerminator) | F(IsBranch))

/**
 * Function Call: dst = call target(args...).
 * Operands:
 *   [0] Write: Return Value Register
 *   [1] Read:  Function Target (Reference, Register, or Symbol)
 *   [2..N] Read: Passed Arguments (Variadic)
 */
INSTRUCTION(CALL,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Reference | ExpectedOperandType::Register |
                                          ExpectedOperandType::RuntimeSymbol,
                                  MirOperandFlag::Read }),
            F(IsCall) | F(HasSideEffect) | F(VariadicArgs))

/**
 * Return from Function: returns execution and an optional value to the caller.
 * Operands:
 *   [0] Read: Return Value (Register, Immediate, or Void/None)
 */
INSTRUCTION(RET,
            T(HighLevel),
            MirCat_ControlFlow,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, MirOperandFlag::Read }),
            F(IsTerminator) | F(IsReturn) | F(HasSideEffect))

/* ========================================================================= */
/* --- TYPE CASTING & CONVERSIONS ------------------------------------------ */
/* ========================================================================= */

/**
 * Integer Truncation: Reduces bit-width of an integer value (e.g., i64 -> i32).
 * Operands:
 *   [0] Write: Destination Register (Narrower Type)
 *   [1] Read:  Source Register (Wider Type)
 */
INSTRUCTION(TRUNC,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(DestSmaller))

/**
 * Zero Extension: Extends unsigned integer with leading zeros (e.g., u8 -> u32).
 * Operands:
 *   [0] Write: Destination Register (Wider Type)
 *   [1] Read:  Source Register (Narrower Type)
 */
INSTRUCTION(ZEXT,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(DestLarger))

/**
 * Sign Extension: Extends signed integer preserving sign bit (e.g., i8 -> i32).
 * Operands:
 *   [0] Write: Destination Register (Wider Type)
 *   [1] Read:  Source Register (Narrower Type)
 */
INSTRUCTION(SEXT,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(DestLarger))

/**
 * Floating-Point Extension: Widens float precision (e.g., f32 -> f64).
 * Operands:
 *   [0] Write: Destination Register (Wider Float)
 *   [1] Read:  Source Register (Narrower Float)
 */
INSTRUCTION(FPEXT,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(DestLarger))

/**
 * Floating-Point Truncation: Narrows float precision (e.g., f64 -> f32).
 * Operands:
 *   [0] Write: Destination Register (Narrower Float)
 *   [1] Read:  Source Register (Wider Float)
 */
INSTRUCTION(FPTRUNC,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(DestSmaller))

/**
 * Signed Integer to Floating-Point Conversion (e.g., i32 -> f32).
 * Operands:
 *   [0] Write: Destination Float Register
 *   [1] Read:  Source Signed Integer Register
 */
INSTRUCTION(SITOFP,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(None))

/**
 * Floating-Point to Signed Integer Conversion (e.g., f32 -> i32).
 * Operands:
 *   [0] Write: Destination Signed Integer Register
 *   [1] Read:  Source Float Register
 */
INSTRUCTION(FPTOSI,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(None))

/**
 * Bitcast / Reinterpretation: Converts type without altering underlying bits (e.g., f32 <-> i32).
 * Operands:
 *   [0] Write: Destination Register (New Type)
 *   [1] Read:  Source Register (Original Type)
 */
INSTRUCTION(BITCAST,
            T(HighLevel),
            MirCat_Casting,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },
                                { ExpectedOperandType::Register, MirOperandFlag::Read }),
            F(SizeMatch))

/* ========================================================================= */
/* --- SYSTEM & SPECIAL ---------------------------------------------------- */
/* ========================================================================= */

/**
 * Invokes an OS-level system call interrupt.
 * Operands:
 *   [0] Read: Syscall Number / Argument Payload
 */
INSTRUCTION(SYSCALL,
            T(HighLevel),
            MirCat_System,
            OPERAND_CONSTRAINTS({ ExpectedOperandType::AnyValue, MirOperandFlag::Read }),
            F(HasSideEffect))

/**
 * No Operation: Emits a placeholder instruction that performs no action.
 */
INSTRUCTION(NOP, T(HighLevel), MirCat_System, OPERAND_CONSTRAINTS(), F(None))

/**
 * Halts CPU execution / traps program state.
 */
INSTRUCTION(HALT, T(HighLevel), MirCat_System, OPERAND_CONSTRAINTS(), F(IsTerminator) | F(HasSideEffect))

/**
 * Target instruction placeholder used during instruction selection (ISel) to emit backend-specific opcodes.
 */
INSTRUCTION(TARGET_INST, T(TargetLow), MirCat_System, OPERAND_CONSTRAINTS(), F(HasSideEffect))

#undef T
#undef F
#undef OPERAND_CONSTRAINTS

#ifdef BG_TEST_WAS_DEFINED
#pragma pop_macro("TEST")
#undef BG_TEST_WAS_DEFINED
#endif

#endif // INSTRUCTION