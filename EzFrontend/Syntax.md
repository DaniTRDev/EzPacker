This file describes the syntax of the IR.

# Syntax Format

The general syntax for the IR depends on the keyword. Generally, for instruction keywords this syntax is followed:

- Single arguments: ``.instr source`` / ``.instr destination``. Depends on the instruction (push, pop, ...).
- Multiple arguments: ``.instr destination, source``.

Sources and destinations can be of type:

- Virtual Variable: In the form of %my_Var.
- Immediate: In the form of 12312, 4919, ....
- Memory: In the form discussed in [Memory Addressing](#memory-adressing)

# Data types

This chapter includes how to types are defined within the IR. Take in my mind that many types are architecture-specific:
32-bit architectures like AARCH32 don't support 64-bit values. This will be handled by performing analysis on source and
target architecture by the IR builder.

> [!WARNING]
> Strings are handled internally as 8-bits characters, meaning that they won't appear directly in the IR file, they will
> just be vectors of decimal / hexadecimal characters.

Supported data types are:

- ``.i8``: 8-bits integer.
- ``.i16``: 16-bits integer.
- ``.i32``: 32-bits integer.
- ``.i64``: 64-bits integer.
- ``.ptr``: Represents a pointer. It only indicates that memory is going to be used,
  e.g: ``.ptr .i8``. A type can only contain a single .ptr declaration. If pointers to pointers are needed,
  first pointer should be accessed to get the second one and access it.

(More data types will be added in a future to support vectored operations).

# Global variables

This chapter includes how to define Global variables within the IR. Global variables must be defined outside any
module.

Here are a few examples to illustrate:

- Create a simple 64-bit integer variable initialized with 0.

```
    .variable MyVariable: .i64 0
```

- Create a vector of 64-bit integers, all of them initialized to a single value:

```
    .variable MyVariable: .i64 INITIAL_VALUE
```

- Create a vector of 64-bit integers, all of them initialized to a value

```
    .variable MyVariable: .i64 VALUE_ELEMENT_1, VALUE_ELEMENT_2, VALUE_ELEMENT_3  
```

> [!CAUTION]
> If no initial value is given, an error will be thrown.

# Virtual Variables

Virtual variables are slots in a virtual memory used by the Compiler to know where should it write / read each time a
variable is used. They can be a register (given by the source architecture) or a stack reference. Deep, in the
AstNormalizer there is a table that contains defined virtual variable names that later will be converted to physical
registers in the compiling stage. 

> [!CAUTION]
> Only 'load' instruction will define new variables: if you want to reference a variable you must have declared it
> somewhere in the module with a load. There, it will acquire a type that will be reused when normalizing types.

To reference a virtual variable, you must use "%identifier":

```
.module myMod:
  .load .i64 %myVar, .i64 (1231)
  .add .i32 %myVar, .i32 314
.end
```

In this example, myVar acquires the type of a 64-bit integer that get its content by reading 64bits of given direct
memory reference. In the next line, the first 32 bits of myVar (low part) are set from a 32-bit constant.

# Module syntaxis

This chapter introduces how a module is defined in the IR. Modules need information so they can be correctly translated
and analysed. Here's a breakdown of how to define a simple module:

```
.module MyModule
    # Comments are supported!
.end
```

As you might have noticed, modules don't have a return type. That's because it's left to the lifter to correctly set
return values accordingly from the source code. Arguments are also left to the lifter.

Here's an example that shows what we've discussed earlier:

```C++
    // C++
    int MyModule(int a)
    {
        a += 1;
        return a;
    }
```

It will be compiled into something like:

```asm
MyModule:
    push rbp ; Save rbp's content.
    sub rsp, 4 ; Reserve space for 'a'.
    mov rbp, rsp ; Set RBP to use current frame.

    inc [rbp] ; Increment local variable
    mov rax, [rbp] ; Save return value

    add rsp, 4 ; Restore reserved stack for variables.
    pop rbp ; Restore RBP's content.
    ret ; Pops return address from stack and returns to it
```

And within our IR, it will be lifted into (without metadata such as .markUse or .markUnuse):

```
.module MyModule
    .reserveStack .i32 1 # Reserves 1 element of 32-bits -> 4 bytes.
    .push .i64 %stackFrame  # %stackFrame represents the stackFrame register.
    .lea .i64 %stackFrame, %stackPtr
    
    .load .i64 %myVar, .ptr (%stackFrame) # Moves the 64-bit-portion of memory at stackFrame into myVar.
    
    .pop .i64 %stackFrame
    .freeStack .i32 1
    .ret
.end
```

An important point is the reserved register ``%stackFrame``, which is defined by the IR and represents the register
used to access function's stack frame. If it's not available in an architecture, it should be defined as the stack
pointer or any other volatile memory indicator.

Lastly, ``.push`` and ``.pop`` instructions might not be present in the target architecture, they will be replaced by
``.stackReserve .... && .store [stackAddr], value`` and ``.load value, [stackAddr] && .stackFree ...``.

# Memory Adressing

This chapter exposes how memory address can be used in the IR. It's a crucial part when it's recompiled back.
A memory address can be formed following one of these formats:

- ``(Address)``: Address is a value that represents an absolute offset, destination won't surely be inside this IR
  module, so lifter must ensure that these addresses are properly handled.
- ``(Base)``: Address is inside a register.
- ``(Base, displacement)``: Address is formed with the content of the register and the displacement. Translates to:
  ``Base + displacement``.
- ``(, Index, Scale)``: Address is formed with the content of the index register multiplied by a signed number that can
  be 1, 2, 4, 8, ... in general multiples of 2 (limited to 1 byte). Translates to: ``(index*scale)``.
- ``(Base, Index, Scale)``: Address is formed with the content of the base register, added to the product of index
  register and scale value. Translates to: ``(base + index*scale)``.
- ``(Base, Index, Scale, Displacement)``: Address is formed with the content of the base register, added to the product
  of index register and scale value and with a displacement. Translates to: ``(base + index*scale + displacement)``.