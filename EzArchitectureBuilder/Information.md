This file will clarify the functionality of EzArchitectureBuilder. It's a library that will be used to create semantic
rules for instructions, operands, ... This information is crucial to the parser because it gives to the parser the
ability to know if an expression is well-formed. It also contains information of the architecture itself, like registers
word size, ...

# Builder an architecture

Building an architecture is a long process because it requires a lot of information that in most of the times, it must
be added by the developer itself. EzArchitectureBuild follows a JSON-oriented approach so everything cane be coded in
JSON. This approach assures:

- No recompilation of the builder when something of 1 architecture changes.
- Having architectures well-contained on their own .json file.
- Using JSON instead of C++.

In this part of the documentation, a compact, yet extensive example of how an architecture like x64_x86(amd64) can be
built.

## Initial Step

Here's an example of an incomplete approximation for x86_64.

```json
{
  "name": "x64_x86(AMD64)",
  "wordSize": 64,
  "endianness": "little",
  "registerFamilies": [
    {
      "name": "GeneralPurpose",
      "description": "64-bit general-purpose registers (GPRs) with sub-registers.",
      "registers": [
        {
          "name": "RAX",
          "size": 64,
          "subRegisters": [
            "EAX",
            "AX",
            "AL",
            "AH"
          ],
          "role": "accumulator"
        },
        {
          "name": "RBX",
          "size": 64,
          "subRegisters": [
            "EBX",
            "BX",
            "BL",
            "BH"
          ],
          "role": "base"
        },
        {
          "name": "RCX",
          "size": 64,
          "subRegisters": [
            "ECX",
            "CX",
            "CL",
            "CH"
          ],
          "role": "counter"
        },
        {
          "name": "RDX",
          "size": 64,
          "subRegisters": [
            "EDX",
            "DX",
            "DL",
            "DH"
          ],
          "role": "data"
        },
        {
          "name": "RSI",
          "size": 64,
          "subRegisters": [
            "ESI",
            "SI",
            "SIL"
          ],
          "role": "source_index"
        },
        {
          "name": "RDI",
          "size": 64,
          "subRegisters": [
            "EDI",
            "DI",
            "DIL"
          ],
          "role": "destination_index"
        },
        {
          "name": "RBP",
          "size": 64,
          "subRegisters": [
            "EBP",
            "BP",
            "BPL"
          ],
          "role": "base_pointer"
        },
        {
          "name": "RSP",
          "size": 64,
          "subRegisters": [
            "ESP",
            "SP",
            "SPL"
          ],
          "role": "stack_pointer"
        },
        {
          "name": "R8",
          "size": 64,
          "subRegisters": [
            "R8D",
            "R8W",
            "R8B"
          ],
          "role": "general"
        },
        {
          "name": "R9",
          "size": 64,
          "subRegisters": [
            "R9D",
            "R9W",
            "R9B"
          ],
          "role": "general"
        },
        {
          "name": "R10",
          "size": 64,
          "subRegisters": [
            "R10D",
            "R10W",
            "R10B"
          ],
          "role": "general"
        },
        {
          "name": "R11",
          "size": 64,
          "subRegisters": [
            "R11D",
            "R11W",
            "R11B"
          ],
          "role": "general"
        },
        {
          "name": "R12",
          "size": 64,
          "subRegisters": [
            "R12D",
            "R12W",
            "R12B"
          ],
          "role": "general"
        },
        {
          "name": "R13",
          "size": 64,
          "subRegisters": [
            "R13D",
            "R13W",
            "R13B"
          ],
          "role": "general"
        },
        {
          "name": "R14",
          "size": 64,
          "subRegisters": [
            "R14D",
            "R14W",
            "R14B"
          ],
          "role": "general"
        },
        {
          "name": "R15",
          "size": 64,
          "subRegisters": [
            "R15D",
            "R15W",
            "R15B"
          ],
          "role": "general"
        },
        {
          "name": "RIP",
          "size": 64,
          "subRegisters": [
            "eip"
          ],
          "role": "instruction_pointer"
        },
        {
          "name": "rflags",
          "size": 64,
          "subRegisters": [
            "rwflags"
          ],
          "role": "flags"
        }
      ]
    },
    {
      "name": "SIMD",
      "description": "XMM/YMM/ZMM registers for SSE/AVX instructions.",
      "registers": []
    },
    {
      "name": "Segment",
      "description": "Segment registers (used in legacy modes).",
      "registers": []
    },
    {
      "name": "Control",
      "description": "Special-purpose control registers.",
      "registers": []
    }
  ]
}
```

In this case we have defined a very basic starting point, which consists on defining supported registers of the
architecture as well as their size, their sub-registers, ... Each sub-register will be of size (n / 2) where n is the
size in bits. If size is 1 and there are still registers in the list, they will be "low" part and "high" part, if even
more are given, they won't be used at all.

There must be, at least, 3 registers with the role set to "stack_pointer", "instruction_pointer" and "flags". This can't
be
omitted and will cause errors if done.

## Memory Addressing

The next step is to configure the allowed addressing types (taking in mind limitations of the IR):

```json
{
  "allowedMemoryAddressing": [
    "Direct",
    "Base",
    "BaseDisplacement",
    "IndexScale",
    "BaseIndexScaleDisplacement",
    "IPRelative"
  ]
}
```

## Instruction Semantics

After declaring the information of previous parts of this guide, we can start by modeling the most tedious and critical
part, the semantics of an instruction. Here's an example code that demonstrates how an instruction can be fully modeled
into its semantic expression. This should be included inside a section called "semanticsDefinitions" in the main
section.

```json
{
  "semanticsDefinitions": [
    {
      "name": "TipeR",
      "description": "Defines the semantic of a Type R instruction (instructions that only use registers)",
      "allowedSourceOperands": [
        "register"
      ],
      "allowedDestinationOperands": [
        "register"
      ],
      "restrictedRegisterRoles": [
        "instruction_pointer"
      ]
    },
    {
      "name": "TipeI",
      "description": "Defines the semantic of a Type I instruction (instructions that use registers as destination operands and immediates as source operands)",
      "allowedSourceOperands": [
        "register"
      ],
      "allowedDestinationOperands": [
        "immediate"
      ],
      "restrictedRegisterRoles": [
        "instruction_pointer"
      ]
    },
    {
      "name": "TipeMem",
      "description": "Defines the semantic of a Type M instruction (instruction in which any of the operands can be a memory operand)",
      "allowedSourceOperands": [
        "memory",
        "register"
      ],
      "allowedDestinationOperands": [
        "memory",
        "register"
      ],
      "restrictedRegisterRoles": [
      ]
    },
    {
      "name": "TipeCF",
      "description": "Defines the semantic of a Type Code Flow instruction (it modifies the IP)",
      "allowedSourceOperands": [],
      "allowedDestinationOperands": [
        "memory"
      ],
      "restrictedRegisterRoles": [
      ]
    },
    {
      "name": "TipeBr",
      "description": "Defines the semantic of a Type Branch instruction (it modifies the IP if a condition is met)",
      "allowedSourceOperands": [
        ""
      ],
      "allowedDestinationOperands": [
        "memory"
      ],
      "restrictedRegisterRoles": [
      ]
    }
  ]
}
```

After defining the semantics, we need to annotate which ones does each instruction support. This piece of code shall be
inside in a new section called "instructionSemantics"

```json
{
  "instructionSemantics": [
    {
      "instruction": "Add",
      "semanticType": "TipeR"
    },
    {
      "instruction": "And",
      "semanticType": "TipeR"
    },
    {
      "instruction": "Branch",
      "semanticType": "TipeBr"
    },
    {
      "instruction": "Call",
      "semanticType": "TipeCF"
    },
    {
      "instruction": "Compare",
      "semanticType": "TipeR"
    },
    {
      "instruction": "Divide",
      "semanticType": "TipeR"
    },
    {
      "instruction": "Exchange",
      "semanticType": "TipeR"
    },
    {
      "instruction": "FreeStack",
      "semanticType": "TipeI"
    },
    {
      "instruction": "Jump",
      "semanticType": "TipeCF"
    },
    {
      "instruction": "Load",
      "semanticType": "TipeMem"
    },
    {
      "instruction": "LoadEffectiveAddress",
      "semanticType": "TipeMem"
    },
    {
      "instruction": "Multiply",
      "semanticType": "TipeR"
    },
    {
      "instruction": "Nop",
      "semanticType": null
    },
    {
      "instruction": "Not",
      "semanticType": "TipeR"
    },
    {
      "instruction": "Or",
      "semanticType": "TipeR"
    },
    {
      "instruction": "Pop",
      "semanticType": "TipeMem"
    },
    {
      "instruction": "Push",
      "semanticType": "TipeMem"
    },
    {
      "instruction": "ReserveStack",
      "semanticType": "TipeI"
    },
    {
      "instruction": "Return",
      "semanticType": "TipeCF"
    },
    {
      "instruction": "RotateLeft",
      "semanticType": "TipeR"
    },
    {
      "instruction": "RotateRight",
      "semanticType": "TipeR"
    },
    {
      "instruction": "SetFlags",
      "semanticType": "TipeR"
    },
    {
      "instruction": "ShiftLeft",
      "semanticType": "TipeR"
    },
    {
      "instruction": "ShiftRight",
      "semanticType": "TipeR"
    },
    {
      "instruction": "SignExtend",
      "semanticType": "TipeR"
    },
    {
      "instruction": "Store",
      "semanticType": "TipeMem"
    },
    {
      "instruction": "Subtract",
      "semanticType": "TipeR"
    },
    {
      "instruction": "Test",
      "semanticType": "TipeR"
    },
    {
      "instruction": "Xor",
      "semanticType": "TipeR"
    }
  ]
}
```

After all of this have been included in the final "archName.json" file, it is ready to be given to the architecture's
json parser to create the architecture within our application context.