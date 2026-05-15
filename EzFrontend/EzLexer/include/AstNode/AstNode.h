/**
 * @file AstNode.h
 * @brief Core AST contracts: base node type, node tags, and semantic
 *        annotation support.
 *
 * Every concrete syntax element produced by EzLexer inherits from AstNode.
 * Consumers are expected to inspect nodes through the combination of:
 * - `getType()` for quick kind checks,
 * - `accept()` for structured traversal,
 * - node-specific getters for payload,
 * - annotations added later by EzSemantics.
 */
#ifndef EZPACKER_AST_H
#define EZPACKER_AST_H

#include "EzLexerCommon.h"
#include "SourceManager/SourceManager.h"

/**
 * @brief Base interface for annotations that can be attached to any AstNode.
 *
 * EzLexer itself only creates the syntactic tree. Later phases attach semantic
 * meaning through annotations instead of mutating node layouts. Typical uses
 * include resolved symbols, resolved data types, scope ownership, and cast
 * information.
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
    For,
    If,
    Include,
    Instruction,
    Immediate,
    Label,
    Module, // Contains the header and the code scope.
    ModuleHeader,
    Switch,
    SwitchCase,
    Variable,
    While
};

enum class AstNodeStringMode : uint8_t
{
    Default = 0, // Shows a little piece of information about this node.
    Debug        // Shows as much information as possible about this node.
};

/**
 * Base class for every AST node emitted by EzLexer.
 *
 * Ownership and lifetime:
 * - Concrete nodes are expected to be allocated from AstNodeTypedPool.
 * - The node object itself does not own child nodes or annotation pools.
 * - Any pointers returned by getters remain valid while the owning parse or
 *   semantic context remains alive.
 *
 * Source references:
 * - Parsers should set a SourceReference when they can identify the exact
 *   source span for the construct.
 * - ParserBatch assigns a fallback reference when a parser returns a node
 *   without one.
 */
class AstNode
{
  public:
    /**
     * Creates and attaches a new annotation of the requested type.
     *
     * New annotations are prepended to the annotation slice. Callers typically
     * use this from semantic passes, not from parsers.
     *
     * @throws std::runtime_error if annotPool is null.
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
            m_annotations = annotPool->createLinkedList<IAstNodeAnnotation>();
        }

        return annotPool->createAndAppendToListFront<AnnotType, IAstNodeAnnotation>(m_annotations,
                                                                                       std::forward<Args>(args)...);
    }

    /**
     * Returns the concrete node kind.
     */
    virtual AstNodeType getType() const = 0;

    /**
     * Dispatches this node to the corresponding AstNodeVisitor overload.
     *
     * Returns whatever the visitor returns for this node type.
     */
    virtual bool accept(class AstNodeVisitor *visitor) = 0;

    /**
     * Returns true when at least one annotation has been attached.
     */
    bool hasAnnotations() const;

    /**
     * Returns a short stable name for the node class, mainly for debugging,
     * logging, and diagnostics.
     */
    virtual const char *getAstNodeName() const = 0;

    /**
     * Returns the source span associated with this node.
     *
     * The reference may be empty when the node has not been assigned a source
     * span yet.
     */
    const SourceReference &getSourceRef() const;

    /**
     * Returns the raw annotation slice, or nullptr when the node has not been
     * annotated.
     */
    TypedPoolLinkedList<IAstNodeAnnotation> *getAnnotations();

    /**
     * Assigns the source span associated with this node.
     */
    void setSourceRefs(const SourceReference &ref);

    /**
     * Attaches an already-created annotation to the node.
     *
     * This is the low-level companion to createAnnotation().
     */
    void addAnnotation(IAstNodeAnnotation *annot, TypedPool *annotPool);

    /**
     * Returns the first annotation in the slice that can be dynamically cast to
     * the requested type.
     *
     * By convention a node should not carry multiple annotations of the same
     * concrete type.
     */
    template <typename T>
        requires(std::is_base_of<IAstNodeAnnotation, T>::value)
    T *getAnnotation() const
    {
        if (!m_annotations)
            return nullptr;

        for (IAstNodeAnnotation *annot : *m_annotations)
        {
            T *ptr = dynamic_cast<T *>(annot);
            if (ptr)
            {
                return ptr;
            }
        }

        return nullptr;
    }

  private:
    SourceReference m_sourceRef;                                  // A node might or might not have a source reference.
    TypedPoolLinkedList<IAstNodeAnnotation> *m_annotations{ nullptr }; // A node might or might not have an annotation.
};

#endif // EZPACKER_AST_H
