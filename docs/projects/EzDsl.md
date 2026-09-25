# EzDsl Subproject Documentation

[EzPacker Documentation Index](../index.md) > **EzDsl**

---

## 1. Overview & Architectural Role

`EzDsl` is the domain-specific meta-compiler toolkit powering EzPacker. Instead of manually writing thousands of error-prone lines of C++ boilerplate to describe CPU register sets, instruction encodings, calling conventions, legalization matrices, and instruction selection patterns, developers express these declaratively in high-level DSL files.

During compilation of EzPacker (via CMake custom commands), `ezdsl_cli` parses these DSL files, validates their semantics, and synthesizes optimized, type-safe C++ headers and static lookup tables.

```
+-----------------------------------------------------------------------------------+
|                                  EzDsl Pipeline                                   |
|                                                                                   |
|  DSL Source Files                                                                 |
|   - types.tyf             (Types)                                                 |
|   - instructions.irdf     (Generic IR Opcodes)                                    |
|   - x86_64.tdesc          (Target Descriptors & Register Banks)                   |
|   - x86_64_calling_conv   (Calling Conventions)                                   |
|   - x86_64_legalize.lad   (Legalization Actions)                                  |
|   - x86_64_rules.lrd      (Legalization Rewrite Rules)                            |
|   - x86_64_instructions   (Target Instruction Definitions & Encodings)            |
|   - x86_64_patterns.isf   (Instruction Selection Patterns)                        |
|                            |                                                      |
|                            v                                                      |
|   +--------------------------------------------------+                            |
|   |                  EzDsl/Lexer                     |                            |
|   |   - Uses foonathan/lexy parser combinators       |                            |
|   |   - Builds typed AST nodes (CommonAstNodes.h)    |                            |
|   +--------------------------------------------------+                            |
|                            |                                                      |
|                            v                                                      |
|   +--------------------------------------------------+                            |
|   |                  EzDsl/Sema                      |                            |
|   |   - Scopes, Symbol Tables, and Resolvers         |                            |
|   |   - Semantic Passes (TypePass, TargetInstPass..) |                            |
|   |   - Target Encoding Dialects                     |                            |
|   +--------------------------------------------------+                            |
|                            |                                                      |
|                            v                                                      |
|   +--------------------------------------------------+                            |
|   |             EzDsl/CodeGenerators                 |                            |
|   |   - Synthesizes C++ Source Code & Tables         |                            |
|   |   - Dense 2D matrices, maximal munch matchers    |                            |
|   +--------------------------------------------------+                            |
|                            |                                                      |
|                            v                                                      |
|  Generated C++ Headers & Source Files (included by EzMir, EzTriple, EzTargets)    |
+-----------------------------------------------------------------------------------+
```

---

## 2. Subproject Structure

EzDsl is divided into four cleanly decoupled submodules:

| Submodule | Directory | Responsibility |
| :--- | :--- | :--- |
| **`EzDsl::Lexer`** | `EzDsl/Lexer/` | Lexical analysis and parsing using `lexy`. Emits ASTs for all 8 DSL dialects. |
| **`EzDsl::Sema`** | `EzDsl/Sema/` | Semantic analysis, symbol resolution, scoping, validation passes, and target encoding dialects. |
| **`EzDsl::CodeGenerators`** | `EzDsl/CodeGenerators/` | Backend emitters synthesizing C++ headers, static lookup tables, and decision trees. |
| **`EzDsl::Cli`** | `EzDsl/Cli/` | Standalone CLI executable `ezdsl_cli` used during build time and development. |

---

## 3. The 8 DSL Languages

### 3.1 Type Definition Language (`.tyf`)
Defines the primitive and vector types available across the compiler.
- **File**: `EzMir/types.tyf`
- **Example**:
  ```tyf
  integer i32(32);
  integer i64(64);
  float f32(32);
  float f64(64);
  vector v4f32(128, 128); // 128-bit vector with 128-bit alignment
  vector v8f32(256, 256); // 256-bit AVX vector
  ```
- **Generated Code**: Type enumeration, size tables, and runtime type descriptors in `MirTypeTable.h`.

### 3.2 IR Instruction Definition Language (`.irdf`)
Defines generic machine-independent MIR opcodes, their operand arities, directions, categories, and execution tiers.
- **File**: `EzMir/instructions.irdf`
- **Example**:
  ```irdf
  ir_inst ADD(Register:dst OUT, AnyValue:lhs IN, AnyValue:rhs IN) {
      CATEGORY(Arithmetic);
      TIER(HighLevel);
      FLAGS(IsCommutative);
  }
  ```
- **Generated Code**: `MirInstructionOpCode` enum and metadata lookup tables.

### 3.3 Target Descriptor Language (`.tdesc`)
Describes a hardware architecture: pointer size, stack slot size, object formats, register banks, register classes, sub-register hierarchies, hardware encodings, and CPU extensions.
- **File**: `EzTargets/X86_64/targets/x86_64/x86_64.tdesc`
- **Example**:
  ```tdesc
  target X86_64 {
      pointer_size: 8;
      stack_slot:   8;
      instruction_pointer: rip;
      object_formats: [ELF, COFF];
      default_calling_conv: SysV_AMD64;

      register_bank GPR {
          classes { GPR8: 8, GPR16: 16, GPR32: 32, GPR64: 64 }
          sub_register { GPR16 <: GPR8, GPR32 <: GPR16, GPR64 <: GPR32 }
          registers {
              rax enc 0  names { rax: GPR64, eax: GPR32, ax: GPR16, al: GPR8 }
              rcx enc 1  names { rcx: GPR64, ecx: GPR32, cx: GPR16, cl: GPR8 }
          }
      }

      extensions {
          sse  { default: true; };
          sse2 { implies: [sse]; };
          avx  { implies: [sse4_2]; };
      }
  }
  ```
- **Generated Code**: Target descriptor initialization, register tables, sub-register masks, and extension graphs.

### 3.4 Calling Convention Language (`.ezcc`, `.ccd`)
Specifies stack alignment, growth direction, shadow space, red zone, callee/caller saved registers, argument classification, and return registers.
- **File**: `EzTargets/X86_64/targets/x86_64/x86_64_calling_conv.ezcc`
- **Example**:
  ```ezcc
  calling_convention SysV_AMD64 {
      stack {
          align: 16,
          growth: down,
          cleanup: caller,
          shadow_space: 0,
          red_zone: 128,
          sp: rsp,
          fp: rbp
      }
      preserve callee: [rbx, rsp, rbp, r12, r13, r14, r15]
      preserve caller: [rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11]

      classify {
          types [i8, i16, i32, i64, ptr] => integer
          types [f32, f64] => sse
      }

      arguments {
          pass integer => seq([rdi, rsi, rdx, rcx, r8, r9]), fallback: stack(8)
          pass sse => seq([xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7]), fallback: stack(8)
      }

      returns {
          pass integer => seq([rax, rdx])
          pass sse => seq([xmm0, xmm1])
      }
  }
  ```
- **Generated Code**: Calling convention tables consumed by `MirAbiLowerer`.

### 3.5 Legalization Action Language (`.lad`)
Defines the primary legality matrix and action responses for generic opcodes on a given target.
- **File**: `EzTargets/X86_64/targets/x86_64/x86_64_legalize.lad`
- **Example**:
  ```lad
  target AMD64;
  type_set GPR_SCALARS = (i8, i16, i32, i64);

  action MOV {
      LEGAL(GPR_SCALARS, ptr, f32, f64);
      WIDENS(i1) >> i32;
  };

  action SDIV {
      LEGAL(i32, i64);
      CUSTOM(i8, i16);
      LIBCALL(i128);
  };
  ```
- **Generated Code**: Dense 2D primary legality matrix (`LegalizerInfo`).

### 3.6 Legalization Rule Language (`.lrd`)
Expresses declarative rewrite rules for strength reduction and algebraic simplification.
- **File**: `EzTargets/X86_64/targets/x86_64/x86_64_rules.lrd`
- **Example**:
  ```lrd
  rule SDivPow2_32 {
      match {
          SDIV i32:$dst, i32:$lhs, imm(i32):$c;
      };
      when {
          isPowTwo($c);
          isPositiveConst($c);
      };
      emit {
          SAR i32:$dst, i32:$lhs, log2Pow2($c);
      };
  };
  ```
- **Generated Code**: Pattern rewrite dispatchers invoked by the legalizer.

### 3.7 Target Instruction Definition Language (`.idf`)
Declares hardware instructions, mnemonics, operands, flags, and binary encoding rules.
- **File**: `EzTargets/X86_64/targets/x86_64/x86_64_instructions.idf`
- **Example**:
  ```idf
  target_inst ADD64rr(GPR64:dst OUT, GPR64:src1 IN, GPR64:src2 IN) {
      MNEMONIC("addq");
      FLAGS(IsCommutative);
      ENCODING {
          form: rr;
          opcode: [0x01];
          rex_w: true;
          operands { src2 => reg; dst => rm_reg; };
          coalesce: src1;
          size: dst;
      };
  };
  ```
- **Generated Code**: Target instruction metadata array and encoding lookup tables.

### 3.8 Instruction Selection Pattern Language (`.isf`)
Defines tree-matching rewrite patterns and addressing modes for Bottom-Up Maximal Munch.
- **File**: `EzTargets/X86_64/targets/x86_64/x86_64_patterns.isf`
- **Example**:
  ```isf
  addrmode AddrModeRegImm(GPR64:base, simm32:disp = 0) {
      variant BaseDisp { match { ADD ptr:$base, imm(i32):$disp; }; };
      variant BaseOnly { match { ptr:$base; }; };
  };

  pattern Select_ADD64rm [cost = 2] {
      match {
          ADD i64:$dst, i64:$src1, (LOAD i64:$tmp, AddrModeRegImm($base, $disp));
      };
      when {
          hasOneUse($tmp);
          noInterveningStore($tmp);
      };
      select {
          ADD64rm GPR64:$dst, GPR64:$src1, [$base, $disp];
      };
  };
  ```
- **Generated Code**: Instruction selector decision tree matching expressions to target instructions.

---

## 4. Code Generators

Located in `EzDsl/CodeGenerators/`, each generator converts semantic symbols and ASTs into high-performance C++ code:
- **`CppMirTypeTableGenerator`**: Synthesizes the runtime type table.
- **`CppMirInstructionGenerator`**: Synthesizes generic instruction enums and flags.
- **`CppRegisterInfoGenerator`**: Synthesizes register tables, sub-register hierarchies, and register masks.
- **`CppCallingConvGenerator`**: Synthesizes calling convention descriptor tables.
- **`CppLegalizerGenerator`**: Synthesizes the dense 2D primary matrix for $O(1)$ legality lookup.
- **`CppLegalizeRuleGenerator`**: Synthesizes rule matcher callbacks for strength reductions.
- **`CppTargetInstructionGenerator`**: Synthesizes target instruction metadata arrays.
- **`CppEncodingTableGenerator`**: Synthesizes binary encoding lookup tables.
- **`CppInstructionSelectorGenerator`**: Synthesizes the Bottom-Up Maximal Munch matcher.
- **`CppTargetDescGenerator`**: Synthesizes the target descriptor factory and setup logic.

---

## 5. CLI Tool: `ezdsl_cli`

The `ezdsl_cli` executable provides command-line inspection and manual code generation:

### Command-Line Arguments
```bash
ezdsl_cli [options]
```
- `--input <path>`: Primary input DSL file.
- `--output <path>`: Destination path for generated C++ source/header.
- `--generator <kind>`: Generator to invoke (`Auto`, `TypeTable`, `Instructions`, `Legalizer`, `Rules`, `TargetInstructions`, `TargetEncodings`, `InstructionSelector`, `CallingConv`, `RegisterInfo`, `TargetDesc`).
- `--dump-ast`: Prints the parsed AST to stdout.
- `--dump-symbols`: Prints the resolved symbol table.
- `--format <text|json>`: Output format for dumps (default: `text`).

### Example Usage
```bash
# Validate and dump AST of a target descriptor
ezdsl_cli --input targets/x86_64/x86_64.tdesc --dump-ast

# Manually generate calling convention C++ tables
ezdsl_cli --input targets/x86_64/x86_64_calling_conv.ezcc --output X86_64CallingConv.h --generator CallingConv
```

---

## 6. API Reference & Further Reading

- Generated Doxygen API documentation: [Doxygen Documentation Index](../doxygen/index.html)
- Next subproject: [EzCodeEmitter Subproject Documentation](EzCodeEmitter.md)
- Return to [EzPacker Landing Page](../index.md)
