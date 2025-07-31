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
    virtual const char *getAnnotationName() = 0;
};

/**
 * Very crucial class for the entire frontend. This class holds information about a node in the abstract syntax tree
 * (AST). It's also indispensable for the Annotator (during semantic analysis) because it keeps a pointer to an
 * annotation that will be used to give a meaning to the AST.
 */
class AstNode
{
  public:
    friend class AstNodeBuilder; // Allows AstNodeBuilder to access the private constructor.

    // Enforce factory design pattern.
    AstNode() = delete;
    AstNode(const AstNode &copy) = delete;

    /**
     * Returns true if this node has children nodes.
     * @return bool
     */
    bool hasChildren() const;

    /**
     * Removes the child at index 0, if there's any. Returns true ONLY if the child was removed.
     * @return bool
     */
    bool removeChild();

    /**
     * Removes the child at specified ID, if exists. If succeeded returns true, other-ways it returns false.
     * @param childID
     * @return bool
     */
    bool removeChild(size_t childID);

    /**
     * Returns the id of the node.
     * @return size_t
     */
    size_t getId() const;

    /**
     * Adds a child. If this node doesn't have a valid source reference, it will save a copy of child's ptr.
     * @param node
     */
    void addChild(const std::shared_ptr<AstNode> &node);

    /**
     * Clears the vector of children. And resets the source reference.
     */
    void clear();

    /**
     * Copies (not deep, only pointers) this' children into other.
     * @param other
     */
    void copyChildrenTo(const std::shared_ptr<AstNode> &other) const;

    /**
     * Moves this' children into 'other'.
     * @param other
     */
    void moveChildrenTo(std::shared_ptr<AstNode> &other);

    /**
     * Sets the annotation of this node. Will be filled by EzAnnotator during the semantic analysis.
     * @param annotation
     */
    void setAnnotation(const std::shared_ptr<IAstNodeAnnotation> &annotation);

    /**
     * Sets the content of this node.
     * @param content
     */
    void setContent(const std::string &content);

    /**
     * Sets the source reference for this node.
     * @param ref
     */
    void setSourceRef(const std::shared_ptr<SourceReference> &ref);

    /**
     * Returns the child. If id is not valid for this node, nullptr will be returned.
     * @param id
     * @return const std::shared_ptr<AstNode>
     */
    std::shared_ptr<AstNode> getChild(size_t id) const;

    /**
     * Returns the annotation of this node. If set, return != nullptr; other ways result = nullptr.
     * @return const std::shared_ptr<IAstNodeAnnotation> &
     */
    const std::shared_ptr<IAstNodeAnnotation> &getAnnotation() const;

    /**
     * Returns the source reference of this node. If set, return != nullptr; other ways result = nullptr.
     * @return const std::shared_ptr<SourceReference> &
     */
    const std::shared_ptr<SourceReference> &getSourceRef() const;

    /**
     * Returns the content of the node.
     * @return const std::string &
     */
    const std::string &getContent() const;

    /**
     * Returns the name of the node.
     * @return const std::string &
     */
    const std::string &getName() const;

    /**
     * Returns a reference to the vector of children.
     * @return const std::vector<std::shared_ptr<AstNode>> &
     */
    const std::vector<std::shared_ptr<AstNode>> &getChildren() const;

  private:
    /**
     * Creates the object with the given id and name.
     * @param id
     * @param name
     */
    AstNode(size_t id, std::string name);

  private:
    size_t m_id;
    std::shared_ptr<IAstNodeAnnotation> m_annotation;
    std::shared_ptr<SourceReference> m_sourceRef;
    std::string m_content; // Might be empty on certain nodes.
    std::string m_name;
    std::vector<std::shared_ptr<AstNode>> m_children;
};

/**
 * Class used to wrap the creation of AstNodes. It follows a "factory" design pattern.
 */
class AstNodeBuilder
{
  public:
    /**
     * Creates the builder with the given node name.
     * @param name
     */
    explicit AstNodeBuilder(std::string name);

    /**
     * Returns the ID of the node built by this builder.
     * @return size_t
     */
    size_t getId() const;

    /**
     * Builds an empty node with current information.
     * @return std::shared_ptr<AstNode>
     */
    std::shared_ptr<AstNode> build() const;

    /**
     * Returns the name of the node built by this builder.
     * @return const std::string &
     */
    const std::string &getName() const;

  private:
    inline static size_t m_currentId{ 0 };
    size_t m_id;
    std::string m_name;
};

#endif // EZPACKER_AST_H
