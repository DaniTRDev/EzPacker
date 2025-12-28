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

class IAstNodeVisitor
{
  public:
    virtual ~IAstNodeVisitor() = default;
    
    /**
     * Visits given Immediate operand node. Should return true visitor wants to keep traversing the tree.
     * @param operand
     * @return bool
     */
    virtual bool visit(struct ImmediateOperand *operand)
    {
        return true;
    }
    
    /**
     * Visits given instruction node. Should return true visitor wants to keep traversing the tree.
     * @param instr
     * @return bool
     */
    virtual bool visit(struct Instruction *instr)
    {
        return true;
    }
    
    /**
     * Visits given Label operand node. Should return true visitor wants to keep traversing the tree.
     * @param label
     * @return bool
     */
    virtual bool visit(struct Label *label)
    {
        return true;
    }
    
    /**
     * Visits given Memory operand node. Should return true visitor wants to keep traversing the tree.
     * @param operand
     * @return bool
     */
    virtual bool visit(struct MemoryOperandAstNode *operand)
    {
        return true;
    }
    
    /**
     * Visits given Module node. Should return true visitor wants to keep traversing the tree.
     * @param module
     * @return bool
     */
    virtual bool visit(struct Module *module)
    {
        return true;
    }
    
    /**
     * Visits given Variable node. Should return true visitor wants to keep traversing the tree.
     * @param var
     * @return bool
     */
    virtual bool visit(struct Variable *var)
    {
        return true;
    }
};

enum class AstNodeType
{
    Invalid = 0,
    Instruction,
    Immediate,
    Label,
    MemoryOperand,
    Module, // Contains the header and the body.
    ModuleHeader,
    ModuleBody,
    Variable
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
     * Returns the name of this AstNode.
     * @return const char*
     */
    virtual const char *getAstNodeName() const = 0;

    /**
     * Sets the annotation of this node. Will be filled by EzAnnotator during the semantic analysis.
     * @param annotation
     */
    void setAnnotation(const std::shared_ptr<IAstNodeAnnotation> &annotation);

    /**
     * Sets the source reference for this node.
     * @param ref
     */
    void setSourceRef(const std::shared_ptr<SourceReference> &ref);

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information.
     * @param mode
     * @return std::string
     */
    virtual std::string getAsStr(AstNodeStringMode mode) const = 0;

    /**
     * Returns the annotation of this node. If set, result != nullptr; other ways result = nullptr.
     * @return const std::shared_ptr<IAstNodeAnnotation> &
     */
    const std::shared_ptr<IAstNodeAnnotation> &getAnnotation() const;

    /**
     * Returns the source reference of this node. If set, return != nullptr; other ways result = nullptr.
     * @return const std::shared_ptr<SourceReference> &
     */
    const std::shared_ptr<SourceReference> &getSourceRef() const;

  private:
    std::shared_ptr<IAstNodeAnnotation> m_annotation;
    std::shared_ptr<SourceReference> m_sourceRef;
};

#endif // EZPACKER_AST_H
