# Introduction

This subproject is part of EzPacker, and it's responsible for lexing an input buffer (string). Let's break down how it
works.

# Tokenizer

This part of the code takes the initial input buffer and splits it into small information pieces called tokens. These
tokens are given a source reference (where they come from the original input), a type (to identify which token is which)
and, sometimes, content. Here's an example of a character, and what token would it be equivalent to:

- ``.`` -> ``_TokenType::Dot``
- ``(`` -> ``_TokenType::LeftParen``
- ``12313123`` -> ``_TokenType::NumberInt``
- ``12.32`` -> ``_TokenType::NumberFloat``
- ``"Hello"`` -> ``_TokenType::String``

As mentioned previously, there are some cases in which multiple input characters are mapped into a single token, this is
the case of:

- ``_TokenType::Identifier``
    - Represents a combination of digits, letters or ``_`` with the only restriction of not starting with a digit.
- ``_TokenType::NumberInt``
    - This token is made up of digits ``[0-9]``
- ``_TokenType::NumberFloat``
    - This token is made up of digits a dot and more digits.
- ``_TokenType::String``
    - This token contains a set of characters of the input delimited by double quotation marks, which are not included
      in the content of the token.

After tokenizing the input, it's the turn of the Parser.

# Parser

This module is a crucial part of the frontend, as it establishes the grammar of the language (how it must be written).
It takes a list of tokens provided by the tokenizer and, generally speaking, groups them in structures called AstNodes.

First of all, what's an AstNode? Well, it's a node from an AST or Abstract-Syntax-Tree, in other words, a tree that
represents the entities of our language (variables, modules, instructions, ...). There's a whole rule-based engine whose
only purpose is to walk the token list and match a group of tokens with a node in our tree.

The parser starts from any the top-most entities

- Variables
- Modules

and starts descending the tree until it gets in a leaf (which would be a Token, or rather, the content of a token).
Here's a general explanation of how each top node is expected to be matched. For simplicity’s sake, simple nodes and
nodes that result of direct token matching are ignored. If you're curious, you can read everything at
[TokenParsers](src/AstNodeParser/TokenParsers.cpp), [PrimitiveParsers](src/AstNodeParser/PrimitiveParsers.cpp)
and [ComplexParsers](src/AstNodeParser/ComplexParsers.cpp)

Before starting with AstNodes, you must know some symbols used in the representation of the parsing string:

- ``@``: Token content is preserved on its own AstNode that will be present, as a child, in the final node.
- ``Token``: Expects a token
- ``{..}``: Wraps a statement for easier reading.
- ``[]:N``: Wraps a list of, at the very least, N items.
- If a character is repeated (`{{`) it means the character must be present as a token in the input.

Again, these are REPRESENTATIONS of what the parser expects. Internally, it uses a set of composable rules that do not
use this or any notation at all.

---

## Variable

The title is pretty self-explanatory of what this node represents in our language. A variable can be defined as follows:

```
..variable {@Identifier=Name} :: ..{@Identifier=Type} [{ {@NumberInt} OR {@NumberFloat} OR {@String} }] : 1
```

Examples:

```
.variable myVar: .i8 1

.variable myVar: .i8 1, 3, 4, 5

.variable myVar: .float 3.0, 4.0, 5.0

.variable myVar: .string "FirstStr", "SecondStr"
```

## Module

The title also explains what it represents in our language. It can be defined like:

```
..{@Identifier=Type} {@Identifier=Name} (( [{ {@Identifier=ArgumentType}, %%{@Identifier=ArgumentName} }] : 0))
{{
    [
        { 
            { {@Identifier=LabelName}:: [Instruction]} OR {Instruction} } 
        }
    ] : 0
}}
```

Examples:

```
.i32 myModule(.i16 %arg1, .i32 %arg2)
{
    Instruction...
    label1:
        Instruction...
    ...
}

.i32 myModule()
{
    Instruction...
    label1:
        Instruction...
    ...
}
```

In the examples provided, ``%arg1``, ``%arg2``, ...; are what's known as virtual variables in our language.

---

After describing how our AST is built, the next phase implies another
project: [EzAnnotator](../EzAnnotator/Information.md)