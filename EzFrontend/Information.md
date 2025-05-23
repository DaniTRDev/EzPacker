This file will serve as "external documentation" to anyone that tries to follow the source code. This project,
in terms of "Compilers Fundamentals" is the Frontend, the unit responsible for taking human-readable code and convert
it into a compact, yet information-rich set of structures that will later be used by the compiler to produce code.
Frontend is divided in 3 big parts:

- Tokenizer, extract tokens from given text (text symbols, strings, ...)
- Parser, converts a set of tokens into a structure that is of use by the IR (a dot and an identifier -> AST
  TypeNode, ...). This is the grammar of the code: how it is written.
- Normalizer, converts an AST node into a flattened IR structure (TypeNode -> NormalizedTypeData, with a field called
  with m_id, that represents the resolved type id). This is the semantic of the code: what does code mean.