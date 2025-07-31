This is a subproject of EzPacker. Its goal is to append extra information to each AstNode about its context, this is
called semantic analysis and gives _meaning_ to the AST, so it's not a plain structure anymore.

# Annotations

To give meaning to a node, we need some way of directly appending data to it. This is why there's a special interface in
the EzLexer called "IAstNodeAnnotation". This class provides a hook into the AST for higher-level modules that needs to
perform semantic analysis (and later semantic constraints, but this is not done by EzAnnotator).