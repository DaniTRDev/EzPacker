This file describes, in a general vision, the Frontend of the compiler. There are 4 main projects at the moment, each
one used in a particular step of IR generation (this is what will be really compiled).

# EzLexer

This project covers the first step of the compilation: take input files and transform them into correct grammar
structures (AstNodes).

# EzAnnotator

After creating correctly-written grammatical structures, we need to know what they mean. This is what EzAnnotator does,
takes a set of AstNodes and gives them a meaning.

# EzAnnotationValidator
