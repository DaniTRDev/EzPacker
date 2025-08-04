This is a subproject of EzPacker. Its goal is to append extra information to each AstNode about its context, this is
called semantic analysis and gives _meaning_ to the AST, so it's not a plain structure anymore.

# Annotations

To give meaning to a node, we need some way of directly appending data to it. This is why there's a special interface
inside EzLexer called "IAstNodeAnnotation". This class provides a hook into the AST for higher-level modules that need
to perform semantic analysis (and later semantic constraints, but this is not done by EzAnnotator).

## Utilities

There are some classes that inherit from "IAstNodeAnnotation" that are not annotations themselves, but they are a used
to give annotations a set of abilities or data that are required in more than one place.

An example of this behavior is "SymbolAbleAnnotation", which gives an annotation common methods and variables to handle
symbols within the semantic analysis.

Another example is "TypeAbleAnnotation". It works like the utility class above, but in this case with the type.

## Annotation Types

In this subsection, we will talk about a detailed explanation of each annotation and why it's used for.

### [Constant Annotation](./include/Annotations/ConstantAnnotation.h)

As its name suggests, this annotation is used for values known at compile time (this is the reason they are called
_constants_). A constant can be of any primitive type such as:

- String
- Integer
- Float

Constants are present in many, many scenarios like in function parameters, regular instruction operands, ...

### [TypeAnnotation](./include/Annotations/TypeAnnotation.h)

This annotation holds an ID to the type (that can be referenced with the corresponding type table). Internally, it just
inherits from "TypeAbleAnnotation".

### [SymbolAnnotaiton](./include/Annotations/SymbolAnnotation.h)

A SymbolAnnotation saves the ID of the symbol being referenced or created. Symbol's information is stored in a symbol
table. Internally, it just inherits from "SymbolAbleAnnotation".

### [MemoryRefAnnotation](./include/Annotations/MemoryRefAnnotation.h)

A "memory reference annotation" is responsible for resolving the type of memory reference (Base, Direct,
IndexScale, ...), its operands, and the type of the referenced memory region.

### [InstructionAnnotation](./include/Annotations/InstructionAnnotation.h)

This annotation is quite important and complex, its complete functionality can be read in the documentation of the code,
but generally speaking, this annotation contains the ID of the instruction that has been parsed.

### [LabelAnnotation](./include/Annotations/LabelAnnotation.h)

At the moment of writing, this annotation only holds a symbol used to identify the label within its parent module's
scope.

### [ModuleAnnotation](./include/Annotations/ModuleAnnotation.h)

As it happens with InstructionAnnotation, this one is quite complex and should be checked following code's
documentation. This annotation keeps the skeleton of the module: the header. By header, I mean:

- Returning type
- Symbol (created with module's name).
- Parameters, defined locally within module's scope.

### [VariableAnnotation]

Like a ModuleAnnotation, it stores the skeleton of a variable:

- Type
- Symbol, defined in the global scope of the compiled file.

# Annotators

We now have AstNodes and Annotations, but how do we annotate a node with a particular annotation while ensuring it's
everything correct? This is the function of an annotator, it takes a node and a logger as an input and tries to resolve
its meaning. All of this, while preserving the resolved information in a structure (annotation), which will later be
appended into the node.

There's almost one annotator per annotation, and since their functionality depends heavily on the code and other
factors, it's recommended to see how they work directly in the code: [Annotators](./include/NodeAnnotators).