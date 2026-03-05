/**
 * @file AstNode.h
 * @brief Core definitions for the Abstract Syntax Tree: the base node class,
 *        the annotation interface, node-type enumeration, and display modes.
 *
 * Every parsed construct in the language (instructions, variables, labels,
 * control-flow blocks, …) is represented as a subclass of AstNode.  Nodes
 * can be enriched with semantic metadata through the IAstNodeAnnotation
 * interface — this is how later compiler passes (symbol definition, type
 * resolution, type checking) attach meaning without modifying node classes.
 */
#ifndef EZPACKER_AST_H
#define EZPACKER_AST_H

#include "EzLexerCommon.h"
#include "SourceManager/SourceManager.h"

/**
 * @brief Base interface for annotations that can be attached to any AstNode.
 *
 * Annotations are the mechanism through which semantic passes (symbol
 * definition, type resolution, type checking, lowering) decorate the AST
 * with extra information — symbol links, resolved types, scope ownership,
 * etc. — without changing the node classes themselves.  Every annotation
 * must be trivially destructible so it can live inside a TypedPool.
 */
class IAstNodeAnnotation
{
  public:
    ~IAstNodeAnnotation() = default;

    /**
     * Returns the name of the Annotation.
     * @return const char*
     */
    virtual const char *getAnnotationName() const = 0;
};

enum class AstNodeType
{
    Invalid = 0,
    Break,
    CodeScope,
    Condition,
    Continue,
    If,
    Include,
    Instruction,
    Immediate,
    Label,
    MemoryOperand,
    Module, // Contains the header and the code scope.
    ModuleHeader,
    Variable,
    While
};

enum class AstNodeStringMode : uint8_t
{
    Default = 0, // Shows a little piece of information about this node.
    Debug        // Shows as much information as possible about this node.
};

/**
 * @brief The base class for every node in the Abstract Syntax Tree.
 *
 * AstNode provides the common interface shared by all parsed constructs:
 *   - A type tag (AstNodeType) so callers can identify the concrete kind.
 *   - A visitor accept() method that dispatches to the correct
 *     AstNodeVisitor::visit() overload (double-dispatch / Visitor pattern).
 *   - An optional annotation list where semantic passes can attach metadata
 *     (symbol links, data types, scope ownership, cast info, …).
 *   - A source reference that ties the node back to its position in the
 *     original source text (used for error messages and diagnostics).
 *   - A human-readable string representation for debugging.
 *
 * Concrete node types (Instruction, Variable, Label, Module, …) inherit
 * from AstNode and are allocated inside an AstNodeTypedPool so they remain
 * cache-friendly and trivially destructible.
 */
class AstNode
{
  public:
    /**
     * Creates the given annotation type with the given arguments and adds it to the node.
     * @tparam Args
     * @param annotPool
     * @param args
     */
    template <typename AnnotType, typename... Args>
        requires(std::is_base_of<IAstNodeAnnotation, AnnotType>::value)
    AnnotType *createAnnotation(TypedPool *annotPool, Args &&...args)
    {
        if (!annotPool)
        {
            throw std::runtime_error(
                    "Internal compiler error: Tries to annotate something invalid or pool is not valid");
        }

        if (!m_annotations)
        {
            m_annotations = annotPool->createSlice<IAstNodeAnnotation>();
        }

        return annotPool->createAndAppendToSliceInFront<AnnotType, IAstNodeAnnotation>(m_annotations,
                                                                                       std::forward<Args>(args)...);
    }

    /**
     * Returns the type of the node.
     * @return AstNodeType
     */
    virtual AstNodeType getType() const = 0;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type.Returns
     * the result of visit.
     * @param visitor
     * @return
     */
    virtual bool accept(class AstNodeVisitor *visitor) = 0;

    /**
     * Returns true if this node has annotations.
     * @return bool
     */
    bool hasAnnotations() const;

    /**
     * Returns the name of this AstNode.
     * @return const char*
     */
    virtual const char *getAstNodeName() const = 0;

    /**
     * Returns the source references of this node. May or may not return an empty array.
     * @return const SourceReference &
     */
    const SourceReference &getSourceRef() const;

    /**
     * Returns the annotation of this node. If set, result != nullptr; other ways result = nullptr.
     * @return TypedPoolSlice<IAstNodeAnnotation> *
     */
    TypedPoolSlice<IAstNodeAnnotation> *getAnnotations();

    /**
     * Sets the source references of this node.
     * @param ref
     */
    void setSourceRefs(const SourceReference &ref);

    /**
     * Adds an annotation to the node. If no previous annotation was made, a new slice is created from the annot pool
     * and the element is appended.
     * @param annot
     * @param annotPool
     */
    void addAnnotation(IAstNodeAnnotation *annot, TypedPool *annotPool);

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information.
     * @param mode
     * @return std::string
     */
    virtual std::string getAsStr(AstNodeStringMode mode) const = 0;

    /**
     * Returns the first annotation in the list, cast to the requested type T. By design, a node should not have
     * two annotations of the same type. If no annotations exist, nullptr is returned.
     * @tparam T Annotation type (must derive from IAstNodeAnnotation).
     * @return T * Pointer to the annotation, or nullptr if none exist.
     */
    template <typename T>
        requires(std::is_base_of<IAstNodeAnnotation, T>::value)
    T *getAnnotation() const
    {
        if (!m_annotations)
            return nullptr;

        for (IAstNodeAnnotation *annot : *m_annotations)
        {
            return dynamic_cast<T *>(annot);
        }

        return nullptr;
    }

  private:
    SourceReference m_sourceRef;                                  // A node might or might not have a source reference.
    TypedPoolSlice<IAstNodeAnnotation> *m_annotations{ nullptr }; // A node might or might not have an annotation.
};

#endif // EZPACKER_AST_H
