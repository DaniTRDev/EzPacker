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
    virtual ~IAstNodeAnnotation() = default;

    /**
     * Returns the name of the Annotation.
     * @return const char*
     */
    virtual const char *getAnnotationName() const = 0;
};

enum class AstNodeType
{
    Invalid = 0,
    CodeScope,
    Condition,
    Elif, //"elif"
    Else,
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
 * annotation that will be used to give a meaning to the AST.
 */
class AstNode
{
  public:
    /**
     * Returns the type of the node.
     * @return AstNodeType
     */
    virtual AstNodeType getType() const = 0;

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
     * Adds an annotation to this node. It sets it as the first-top-most annotation.
     * @param annotation
     */
    void addAnnotation(const std::shared_ptr<IAstNodeAnnotation> &annotation);

    /**
     * Adds 1 source reference for this node.
     * @param ref
     */
    void setSourceRef(const std::shared_ptr<SourceReference> &ref);

    /**
     * Adds given source references to this node.
     * @param ref
     */
    void setSourceRef(const std::vector<std::shared_ptr<SourceReference>> &refs);

    /**
     * Returns the annotation of this node. If set, result != nullptr; other ways result = nullptr.
     * @return const std::shared_ptr<IAstNodeAnnotation> &
     */
    const std::list<std::shared_ptr<IAstNodeAnnotation>> &getAnnotations() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information.
     * @param mode
     * @return std::string
     */
    virtual std::string getAsStr(AstNodeStringMode mode) const = 0;

    /**
     * Returns the first source reference out of the reference array. If no references are set, nullptr is returned.
     * @return std::shared_ptr<SourceReference>
     */
    std::shared_ptr<SourceReference> getFirstSourceReference() const;

    /**
     * Returns an annotation based on its type. By design, a node can't have 2 annotations with the same type. This
     * module will return the FIRST one, traversing the list in DESCENDING order.
     * @tparam T
     * @return const std::shared_ptr<T> &
     */
    template <typename T>
        requires(std::is_base_of<IAstNodeAnnotation, T>::value)
    std::shared_ptr<T> getAnnotation() const
    {
        for (auto &annot : m_annotations)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(annot); casted)
                return casted;
        }

        return nullptr;
    }

    /**
     * Returns the source references of this node. May or may not return an empty array.
     * @return const std::vector<std::shared_ptr<SourceReference>> &
     */
    const std::vector<std::shared_ptr<SourceReference>> &getSourceRefs() const;

  private:
    std::list<std::shared_ptr<IAstNodeAnnotation>> m_annotations;
    std::vector<std::shared_ptr<SourceReference>> m_sourceRefs;
};

#endif // EZPACKER_AST_H
