# EzPacker: Examples & Use Cases

[EzPacker Documentation Index](index.md) > **Examples & Use Cases**

---

## 1. Overview

EzPacker provides 12 bundled MIR test and demonstration modules in the `examples/` directory. These modules exercise every layer of the compiler:
- High-level arithmetic and bitwise logic.
- Complex multi-block control flow graphs with conditional branching and phi-nodes.
- ABI lowering differences between System V AMD64 (Linux) and Microsoft Win64 (Windows).
- Instruction selector load-folding and addressing mode synthesis.
- Multi-exit functions with distinct epilogues.
- Deep recursion with caller-saved register clobbering, callee-saved preservation, and object relocations.
- Structured Exception Handling (SjLj) with `TRY`, `THROW`, and `CATCH` instructions lowered to runtime libcalls.
- IEEE-754 precision conversions (`FPEXT`, `FPTRUNC`) and signed integer-to-float conversions (`SITOFP`, `FPTOSI`).
- 128-bit SSE and 256-bit AVX/AVX2 SIMD vector operations (`v4f32`, `v8f32`, `v4i32`, `v8i32`, `v4f64`) with non-destructive 3-operand VEX prefix encoding.

---

## 2. Walkthrough of Bundled Examples

### 2.1 `arithmetic_32bit.mir`: 32-Bit Arithmetic & Logic
Demonstrates 32-bit scalar operations, sign-extended immediate values, and 32-bit register allocation:

```mir
fn @hash_step32(i32 %val, i32 %seed) -> i32 {
entry:
    %c_const = MOV i32 40503;
    %sum = ADD i32 %val, %c_const;
    %prod = IMUL i32 %sum, %seed;
    %diff = SUB i32 %prod, %val;
    RET i32 %diff;
}

fn @bit_blend32(i32 %a, i32 %b) -> i32 {
entry:
    %and_val = AND i32 %a, %b;
    %or_val = OR i32 %a, %b;
    %xor_val = XOR i32 %and_val, %or_val;
    RET i32 %xor_val;
}
```

#### Compilation & Output (x86_64 Linux):
```bash
EzCompiler examples/arithmetic_32bit.mir --emit-asm
```
Generated Assembly:
```nasm
hash_step32:
    push    rbp
    mov     rbp, rsp
    mov     eax, edi            ; Copy %val to accumulator
    add     eax, 40503          ; ADD32ri (immediate sign-extended)
    imul    eax, esi            ; IMUL32rr (multiply with %seed)
    sub     eax, edi            ; SUB32rr (subtract original %val)
    mov     rsp, rbp
    pop     rbp
    ret                         ; Return in eax (upper 32-bits of rax zeroed)
```
*Key Architectural Insight*: In x86-64, writing to a 32-bit register (like `eax`) automatically zero-extends into the upper 32 bits of `rax`. EzPacker's register allocator recognizes this sub-register relationship without requiring redundant `MOVZX` or sign-extension instructions.

---

### 2.2 `branch_control_flow.mir`: Conditional Branching & Phi Nodes
Computes absolute difference $|a - b|$ and clamps values within $[min, max]$:

```mir
fn @abs_diff(i64 %a, i64 %b) -> i64 {
entry:
    %is_gt = CMP_SGT i1 %a, %b;
    BR_COND %is_gt, label %a_greater, label %b_greater;

a_greater:
    %diff1 = SUB i64 %a, %b;
    BR label %exit;

b_greater:
    %diff2 = SUB i64 %b, %a;
    BR label %exit;

exit:
    %res = PHI i64 [%diff1, %a_greater], [%diff2, %b_greater];
    RET i64 %res;
}
```

#### How EzPacker Lowers This:
1. `CodeFlowAnalysisPass` computes the dominator tree: `entry` dominates both `a_greater` and `b_greater`, which join at `exit`.
2. `MirInstructionSelectorPass` maps `CMP_SGT` + `BR_COND` to `cmpq` + `jg a_greater`.
3. In `NonSsaToSsaPass` and target lowering, the `PHI` node is eliminated by inserting register moves along the incoming edges, converging on `rax` at `exit`.
4. `BranchRelaxer` verifies that the short jump displacement fits in 8 bits.

Generated Assembly:
```nasm
abs_diff:
    push    rbp
    mov     rbp, rsp
    cmp     rdi, rsi
    jg      .L_a_greater
    mov     rax, rsi
    sub     rax, rdi
    jmp     .L_exit
.L_a_greater:
    mov     rax, rdi
    sub     rax, rsi
.L_exit:
    mov     rsp, rbp
    pop     rbp
    ret
```

---

### 2.3 `calling_conventions.mir`: System V vs Microsoft Win64 ABI
Demonstrates function argument lowering for unary, binary, ternary, and quaternary functions:

```mir
fn @pass_quaternary(i64 %a, i64 %b, i64 %c, i64 %d) -> i64 {
entry:
    %ab = ADD i64 %a, %b;
    %cd = ADD i64 %c, %d;
    %res = IMUL i64 %ab, %cd;
    RET i64 %res;
}
```

#### Side-by-Side ABI Comparison:

```bash
# Compile for Linux ELF
EzCompiler examples/calling_conventions.mir --target x86_64-unknown-linux-gnu --emit-asm

# Compile for Windows COFF
EzCompiler examples/calling_conventions.mir --target x86_64-pc-windows-coff --emit-asm
```

| Linux System V AMD64 | Windows Microsoft Win64 |
| :--- | :--- |
| `rdi` holds `%a` | `rcx` holds `%a` |
| `rsi` holds `%b` | `rdx` holds `%b` |
| `rdx` holds `%c` | `r8` holds `%c` |
| `rcx` holds `%d` | `r9` holds `%d` |
| Stack aligned to 16 bytes, no shadow space | Stack frame includes mandatory 32-byte shadow store |

---

### 2.4 `crypto_primitives.mir`: Feistel Rounds & 64-Bit Mixers
Demonstrates heavy dataflow mixing arithmetic multiplications with bitwise XOR and AND operations:

```mir
fn @feistel_round(i64 %left, i64 %right, i64 %round_key) -> i64 {
entry:
    %c_mult = MOV i64 14781283;
    %f1 = IMUL i64 %right, %c_mult;
    %f2 = XOR i64 %f1, %round_key;
    %new_right = XOR i64 %left, %f2;
    RET i64 %new_right;
}

fn @murmur_mix64(i64 %key) -> i64 {
entry:
    %c1 = MOV i64 104729;
    %m1 = IMUL i64 %key, %c1;
    %mask = MOV i64 16777215;
    %masked = AND i64 %m1, %mask;
    %c2 = MOV i64 734567;
    %m2 = ADD i64 %masked, %c2;
    RET i64 %m2;
}
```

The register allocator colors these chains using hardware registers without spilling, and the instruction selector folds large immediates into 32-bit sign-extended immediate slots where possible.

---

### 2.5 `math_ops.mir`: Multi-Function & Strength Reduction
Demonstrates multiple functions within one module and power-of-two division strength reductions:

```mir
fn @arithmetic_chain(i64 %a, i64 %b) -> i64 {
entry:
    %c100 = MOV i64 100;
    %sum = ADD i64 %a, %c100;
    %c32 = MOV i64 32;
    %diff = SUB i64 %b, %c32;
    %prod = IMUL i64 %sum, %diff;
    %mask = AND i64 %a, %b;
    %result = XOR i64 %prod, %mask;
    RET i64 %result;
}
```

When integer divisions or modulo operations with power-of-two constants are processed, EzTriple's legalizer matches the rules defined in `x86_64_rules.lrd`:
- `SDivPow2_64`: `SDIV %x, 8` -> `SAR %x, 3`
- `UDivPow2_64`: `UDIV %x, 16` -> `SHR %x, 4`
- `URemPow2_64`: `UREM %x, 32` -> `AND %x, 31`

---

### 2.6 `memory_fold.mir`: Instruction Selector Load-Folding
Demonstrates pointer arithmetic, SIB displacement addressing, and instruction selector load-folding:

```mir
fn @accumulate_offset(i64 %seed, ptr %buf) -> i64 {
entry:
    %val0 = LOAD i64 [ptr %buf + 0];
    %sum0 = ADD i64 %seed, %val0;
    %val1 = LOAD i64 [ptr %buf + 8];
    %sum1 = ADD i64 %sum0, %val1;
    RET i64 %sum1;
}
```

#### How Load-Folding Works:
Instead of emitting two distinct machine instructions:
```nasm
mov  rax, [rsi + 0]    ; Load into temporary register
add  rdi, rax          ; Add
```
`MirInstructionSelectorPass` matches the pattern `Select_ADD64rm` from `x86_64_patterns.isf`:
```nasm
accumulate_offset:
    push    rbp
    mov     rbp, rsp
    mov     rax, rdi
    add     rax, [rsi]          ; Load folded directly into add!
    add     rax, [rsi + 8]      ; Load folded directly into add!
    mov     rsp, rbp
    pop     rbp
    ret
```
This reduces register pressure and eliminates separate memory load instructions.

---

### 2.7 `multiple_returns.mir`: Functions with Multiple Epilogues
Demonstrates functions containing multiple exit blocks:

```mir
fn @calculate_dual_path(i64 %base, i64 %modifier) -> i64 {
path_alpha:
    %sum = ADD i64 %base, %modifier;
    %scaled = ADD i64 %sum, %sum;
    RET i64 %scaled;

path_beta:
    %diff = SUB i64 %base, %modifier;
    %offset = SUB i64 %diff, 10;
    RET i64 %offset;
}
```
`MirFrameLowerer` visits every basic block ending in `RET` and inserts a tailored function epilogue before each return, correctly popping callee-saved registers and restoring `rsp`/`rbp`.

---

### 2.8 `multi_arguments.mir`: Stack-Passed Arguments
Tests function parameter sequences requiring stack storage:

```mir
fn @linear_combination_4arg(i64 %a, i64 %b, i64 %c, i64 %d) -> i64 {
entry:
    %ab = ADD i64 %a, %b;
    %abc = ADD i64 %ab, %c;
    %abcd = ADD i64 %abc, %d;
    RET i64 %abcd;
}

fn @dot_product_4d_32bit(i32 %x1, i32 %y1, i32 %x2, i32 %y2) -> i32 {
entry:
    %p1 = IMUL i32 %x1, %y1;
    %p2 = IMUL i32 %x2, %y2;
    %dot = ADD i32 %p1, %p2;
    RET i32 %dot;
}
```

When functions have more arguments than available ABI registers (e.g. $>6$ on Linux, $>4$ on Windows), `MirAbiLowerer` emits stack loads referencing offsets from `rbp` (e.g. `[rbp + 16]`).

---

### 2.9 `recursive_factorial.mir`: Recursion & Relocations
Demonstrates single and double recursion, callee-saved preservation across calls, and object relocations:

```mir
fn @factorial(i64 %n) -> i64 {
entry:
    %one = MOV i64 1;
    %is_base = CMP_SLE i1 %n, %one;
    BR_COND %is_base, label %base_case, label %recursive_case;

base_case:
    RET i64 %one;

recursive_case:
    %n_minus_one = SUB i64 %n, %one;
    %rec = CALL i64 @factorial, %n_minus_one;
    %result = IMUL i64 %n, %rec;
    RET i64 %result;
}

fn @fibonacci(i64 %n) -> i64 {
entry:
    %one = MOV i64 1;
    %is_base = CMP_SLE i1 %n, %one;
    BR_COND %is_base, label %fib_base, label %fib_rec;

fib_base:
    RET i64 %n;

fib_rec:
    %n1 = SUB i64 %n, 1;
    %fib1 = CALL i64 @fibonacci, %n1;
    %n2 = SUB i64 %n, 2;
    %fib2 = CALL i64 @fibonacci, %n2;
    %total = ADD i64 %fib1, %fib2;
    RET i64 %total;
}
```

#### What Happens Under the Hood:
1. In `@fibonacci`, `%n` and `%fib1` must survive across the second call `@fibonacci(%n - 2)`. Because `CALL` clobbers all caller-saved registers, `MirRegisterAllocator` assigns them to **callee-saved registers** (`rbx`, `r12`).
2. `MirFrameLowerer` detects the use of `rbx` and `r12` and inserts `push rbx; push r12` in the prologue and `pop r12; pop rbx` in the epilogue.
3. `Elf64Writer` creates an `Elf64_Rela` entry with relocation type `R_X86_64_PLT32` pointing to symbol `@factorial` / `@fibonacci`.
4. `CoffWriter` creates an `IMAGE_RELOCATION` record with relocation type `IMAGE_REL_AMD64_REL32`.

---

### 2.10 `exception_handling.mir`: Structured Exception Handling (SjLj)
Demonstrates setjmp/longjmp (SjLj) based exception handling using EzPacker's native `TRY`, `THROW`, and `CATCH` intermediate representation instructions:

```mir
fn @safe_divide(i64 %a, i64 %b) -> i64 {
entry:
    TRY label %try_body, label %catch_zero;

try_body:
    %zero_const = MOV i64 0;
    %is_zero = CMP_EQ i1 %b, %zero_const;
    BR_COND %is_zero, label %do_throw, label %do_compute;

do_throw:
    %err = MOV i64 500;
    THROW i64 %err;

do_compute:
    %res = SUB i64 %a, %b;
    RET i64 %res;

catch_zero:
    %caught = CATCH i64;
    %zero = MOV i64 0;
    %fallback = SUB i64 %zero, %caught;
    RET i64 %fallback;
}

fn @process_account(i64 %balance, i64 %debit) -> i64 {
entry:
    TRY label %verify_funds, label %on_overdraft;

verify_funds:
    %insufficient = CMP_SLT i1 %balance, %debit;
    BR_COND %insufficient, label %raise_overdraft, label %apply_debit;

raise_overdraft:
    %err_code = MOV i64 402;
    THROW i64 %err_code;

apply_debit:
    %remaining = SUB i64 %balance, %debit;
    RET i64 %remaining;

on_overdraft:
    %ex_code = CATCH i64;
    %penalty = MOV i64 35;
    %zero = MOV i64 0;
    %neg_fee = SUB i64 %zero, %penalty;
    RET i64 %neg_fee;
}
```

#### Under the Hood:
1. `TRY label %try_body, label %catch_block` registers the protected region and handler landing pad.
2. `THROW %payload` moves the exception payload into the platform ABI's first argument register (`rdi` on System V AMD64, `rcx` on Win64), automatically attaches `@__ez_default_rtti` as the second argument (`rsi` on System V AMD64, `rdx` on Win64), and emits a direct call to `__ez_throw`.
3. `THROW %payload, @CustomRtti` passes the specified type descriptor symbol to `__ez_throw`, enabling rich RTTI-based inspection and hierarchical type matching.
4. `THROW` (0-operand) automatically attaches both `@__ez_default_payload` and `@__ez_default_rtti`.
5. `CATCH %dst` invokes runtime libcall `__ez_get_current_exception` and moves the caught exception payload into `%dst`.
- Consult [How to Build a Target Architecture](how_to_build_a_target.md) to understand how instructions, encodings, and calling conventions are defined.
- Return to the [EzPacker Documentation Index](index.md).

---

### 2.11 `float_conversions.mir`: Precision & Type Conversions

Demonstrates IEEE-754 precision extension (`FPEXT`), truncation (`FPTRUNC`), and signed integer conversions (`SITOFP`, `FPTOSI`):

```mir
fn @single_to_double(f32 %val) -> f64 {
entry:
    %res = FPEXT f64 %val;
    RET f64 %res;
}

fn @double_to_single(f64 %val) -> f32 {
entry:
    %res = FPTRUNC f32 %val;
    RET f32 %res;
}

fn @int_to_float(i32 %val) -> f32 {
entry:
    %res = SITOFP f32 %val;
    RET f32 %res;
}

fn @float_to_int(f32 %val) -> i32 {
entry:
    %res = FPTOSI i32 %val;
    RET i32 %res;
}
```

#### Generated x86-64 Machine Code:
```nasm
single_to_double:
    cvtss2sd xmm0, xmm0
    ret

double_to_single:
    cvtsd2ss xmm0, xmm0
    ret

int_to_float:
    cvtsi2ss xmm0, edi
    ret

float_to_int:
    cvttss2si eax, xmm0
    ret
```

---

### 2.12 `vector_simd_avx.mir`: 128-Bit SSE & 256-Bit AVX/AVX2 SIMD Operations

Demonstrates high-performance data-parallel SIMD operations across single-precision (`v4f32`, `v8f32`), double-precision (`v4f64`), and 32-bit integer (`v4i32`, `v8i32`) vector registers:

```mir
fn @sse_vec4_add(v4f32 %a, v4f32 %b) -> v4f32 {
entry:
    %res = VADD v4f32 %a, %b;
    RET v4f32 %res;
}

fn @avx_vec8_fma_like(v8f32 %a, v8f32 %b, v8f32 %c) -> v8f32 {
entry:
    %prod = VMUL v8f32 %a, %b;
    %sum = VADD v8f32 %prod, %c;
    RET v8f32 %sum;
}

fn @avx2_int_vec8_add(v8i32 %a, v8i32 %b) -> v8i32 {
entry:
    %res = VADD v8i32 %a, %b;
    RET v8i32 %res;
}
```

#### Compilation & VEX Prefix Output:
```bash
EzCompiler examples/vector_simd_avx.mir -mattr +avx2 -S
```

```nasm
sse_vec4_add:
    vaddps  xmm0, xmm0, xmm1    ; VEX-encoded non-destructive 128-bit vector add
    ret

avx_vec8_fma_like:
    vmulps  ymm0, ymm0, ymm1    ; VEX-encoded 256-bit vector multiply
    vaddps  ymm0, ymm0, ymm2    ; VEX-encoded 256-bit vector addition
    ret

avx2_int_vec8_add:
    vpaddd  ymm0, ymm0, ymm1    ; AVX2 256-bit packed 32-bit integer add
    ret
```

*Key Architectural Insight*: EzTargets' instruction encoder automatically selects compact 2-byte (`0xC5`) or 3-byte (`0xC4`) VEX prefixes, binds the second operand to the inverted `VEX.vvvv` field, and uses `VR256` (`ymm0`..`ymm15`) registers when 256-bit vector types are encountered.

---

## 3. End-to-End Verification Harness

Create `test_examples.c`:

```c
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

// External function signatures matching the example MIR modules
extern int32_t hash_step32(int32_t val, int32_t seed);
extern int64_t abs_diff(int64_t a, int64_t b);
extern int64_t pass_quaternary(int64_t a, int64_t b, int64_t c, int64_t d);
extern int64_t accumulate_offset(int64_t seed, const int64_t *buf);
extern int64_t factorial(int64_t n);
extern int64_t fibonacci(int64_t n);
extern int64_t safe_divide(int64_t a, int64_t b);
extern int64_t process_account(int64_t balance, int64_t debit);
extern double float_to_double(float x);
extern int64_t float_to_int64(float x);

int main(void) {
    printf("Running EzPacker End-to-End Verification...\n");

    // 1. Test 32-bit arithmetic
    int32_t h = hash_step32(100, 5);
    printf("hash_step32(100, 5) = %d\n", h);

    // 2. Test control flow abs_diff
    assert(abs_diff(50, 20) == 30);
    assert(abs_diff(20, 50) == 30);
    printf("abs_diff verified!\n");

    // 3. Test quaternary calling convention
    int64_t q = pass_quaternary(1, 2, 3, 4);
    assert(q == (1 + 2) * (3 + 4)); // 3 * 7 = 21
    printf("pass_quaternary verified!\n");

    // 4. Test load-folding memory access
    int64_t buffer[2] = { 100, 250 };
    int64_t acc = accumulate_offset(50, buffer);
    assert(acc == 50 + 100 + 250); // 400
    printf("accumulate_offset verified!\n");

    // 5. Test recursion
    assert(factorial(5) == 120);
    assert(factorial(7) == 5040);
    assert(fibonacci(7) == 13);
    printf("factorial and fibonacci verified!\n");

    // 6. Test exception handling
    assert(safe_divide(100, 20) == 80);
    assert(process_account(500, 200) == 300);
    printf("exceptions verified!\n");

    // 7. Test float conversions
    assert(float_to_double(3.14159f) > 3.14158 && float_to_double(3.14159f) < 3.14160);
    assert(float_to_int64(42.85f) == 42);
    printf("float conversions verified!\n");

    printf("\nAll EzPacker example modules executed successfully!\n");
    return 0;
}
```

### Build & Run Commands
```bash
# 1. Compile each MIR file to an object
EzCompiler examples/arithmetic_32bit.mir -o arithmetic.o
EzCompiler examples/branch_control_flow.mir -o branch.o
EzCompiler examples/calling_conventions.mir -o calling.o
EzCompiler examples/memory_fold.mir -o memory.o
EzCompiler examples/recursive_factorial.mir -o factorial.o
EzCompiler examples/exception_handling.mir -o exceptions.o
EzCompiler examples/float_conversions.mir -o float_conv.o

# 2. Link with GCC or Clang (linking EzExceptionRuntime)
gcc test_examples.c arithmetic.o branch.o calling.o memory.o factorial.o exceptions.o float_conv.o -lEzExceptionRuntime -o verify_suite

# 3. Execute
./verify_suite
```

---

## 4. Next Steps

- Consult [How to Build a Target Architecture](how_to_build_a_target.md) to understand how instructions, encodings, and calling conventions are defined.
- Return to the [EzPacker Documentation Index](index.md).
