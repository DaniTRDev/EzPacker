#ifndef EZPACKER_AST_H
#define EZPACKER_AST_H

#include "EzLexerCommon.h"
#include "SourceManager/SourceManager.h"

/**
 * Interface used so this project can be fully self-contained. Annotator will work on top of this class, making the code
 * much more modular and maintainable.
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
 * Very crucial class for the entire frontend. This class holds information about a node in the abstract syntax tree
 * (AST). It's also indispensable for the Annotator (during semantic analysis) because it keeps a pointer to an
 * annotation list that will be used to give a meaning to the AST.
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
