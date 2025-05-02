This file describes the syntax of the IR.

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
- ``.ptr``: Represents a pointer. It will also need information about the underlying type, e.g: ``.ptr .i8``.

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
    .variable MyVariable: .vector .i64 SIZE INITIAL_VALUE
```

- Create a vector of 64-bit integers, all of them initialized to a value

```
    .variable MyVariable: .vector .i64 SIZE VALUE_ELEMENT_1 VALUE_ELEMENT_2 VALUE_ELEMENT_3  
```

> [!CAUTION]
> If no initial value is given, 0 will be default. If there are more or less ``VALUE_ELEMENT_...`` than ``SIZE`` in the
> vector, an error will be thrown.

# Module syntaxis

This chapter introduces how a module is defined in the IR. Modules need information so they can be correctly translated
and analysed. Here's a breakdown of how to define a simple module:

```
.module MyModule: argument1 TYPE argument2 TYPE
    # Comments are supported!
.end
```

As you might have noticed, modules don't have a return type. That's because it's left to the lifter to correctly set
return values accordingly from the source code. Arguments are also left to the lifter, a function might use arguments
in the source architecture but the resulting lifting function might not contain any, since this IR is intended to be
used
by a packer, we don't really need this information, but it might be of use to apply certain types of obfuscations.

Here's an example that shows what we've discussed earlier:
Imagine this piece of code in C++

```C++
    int MyModule(int a)
    {
        a += 1;
        return 1;
    }
```

It will be compiled into something like:

```asm
MyModule:
    sub rsp, 8 ; Reserve space for return address and 'a'. Order of the phrase represents order in stack.
    push rbp ; Save rbp's content.
    mov rbp, rsp ; Set RBP to use current frame.

    inc [rbp+4] ; Increment local variable
    mov rax, [rbp+4] ; Save return value

    pop rbp ; Restore RBP's content.
    add rsp, 4 ; Restore reserved stack for variables.
    ret ; Pops return address from stack and returns to it
```

And within our IR, it will be lifted into (without metadata such as .markUse or .markUnuse):

```
.module MyModule
    .reserveStack .i8 8 # Reserves 8 elements of 8-bits -> 8 bytes.
    .push %stackFrame  # %stackFrame represents the stackFrame register.
    
    .mov %vReg1, .ptr(i64) (%stackFrame, 4) # Moves the portion of memory at stackFrame+4 into vReg1.
    
    .pop %stackFrame
    .freeStack .i8 8
    .ret
.end
```

As you can see, the IR is blind about most of the registers. In the compiling stage, IRCompiler will be fed with a
virtual-register-translate table that will match ``vReg1`` with ``rax``. Since this IR is not intended for
recompilation on other architectures, is safe to assume that lifted IR will always be compatible with target
architecture. It's also remarkable that, as a safety measure, each IR file defines what architecture was it lift from.

Another important point is the reserved register ``%stackFrame``, which is defined by the IR and represents the register
used to access function's stack frame. If it's not available in an architecture, it should be defined as the stack
pointer.

Lastly, ``.push`` and ``.pop`` instructions might not be present in the target architecture, they will be replaced by
``.stackReserve .... && .mov [stackAddr], value`` and ``.mov place, [stackAddr] && .stackFree ...``.

# Memory Adressing

This chapter exposes how memory address can be used in the IR. It's a crucial part when it's recompiled back.
A memory address can be formed following one of these formats:

- ``(Address)``: Address is a value that represents an absolute offset, destination won't surely be inside this IR
  module, so lifter must ensure that these addresses are properly handled.
- ``(Base)``: Address is inside a register.
- ``(Base, displacement)``: Address is formed with the content of the register and the displacement. Translates to:
  ``Base + displacement``.
- ``(, Index, Scale)``: Address is formed with the content of the index register multiplied by a signed number that can
  be 1, 2, 4, 8, ... in general multiples of 2 (limited to maximum word size). Translates to: ``(index*scale)``.
- ``(Base, Index, Scale)``: Address is formed with the content of the base register, added to the product of index
  register and scale value. Translates to: ``(base + index*scale)``.
- ``(Base, Index, Scale, Displacement)``: Address is formed with the content of the base register, added to the product
  of index register and scale value and with a displacement. Translates to: ``(base + index*scale + displacement)``.