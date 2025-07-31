#include "AstNode/AstNode.h"

AstNode::AstNode(size_t id, std::string name) : m_id(id), m_name(std::move(name)) {}

bool AstNode::hasChildren() const { return m_children.empty(); }

bool AstNode::removeChild() { return removeChild(0); }

bool AstNode::removeChild(size_t childID)
{
    if (childID >= m_children.size())
        return false;

    m_children.erase(m_children.begin() + childID);
    return true;
}

size_t AstNode::getId() const { return m_id; }

void AstNode::addChild(const std::shared_ptr<AstNode> &node)
{
    if (!m_sourceRef)
        m_sourceRef = node->getSourceRef();

    m_children.push_back(node);
}

void AstNode::clear()
{
    m_sourceRef.reset();
    m_children.clear();
}

void AstNode::copyChildrenTo(const std::shared_ptr<AstNode> &other) const
{
    auto &otherChildren = other->m_children;
    otherChildren.insert(otherChildren.end(), m_children.begin(), m_children.end());
}

void AstNode::moveChildrenTo(std::shared_ptr<AstNode> &other) { other->m_children = std::move(m_children); }

void AstNode::setAnnotation(const std::shared_ptr<IAstNodeAnnotation> &annotation) { m_annotation = annotation; }

void AstNode::setContent(const std::string &content) { m_content = content; }

void AstNode::setSourceRef(const std::shared_ptr<SourceReference> &ref) { m_sourceRef = ref; }

std::shared_ptr<AstNode> AstNode::getChild(size_t id) const
{
    if (id >= m_children.size())
        return nullptr;

    return m_children[id];
}

const std::shared_ptr<IAstNodeAnnotation> &AstNode::getAnnotation() const { return m_annotation; }

const std::shared_ptr<SourceReference> &AstNode::getSourceRef() const
{
    // If a node doesn't have a source reference, it should be extracted from its children.
    if (!m_sourceRef)
    {
        for (auto &child : getChildren())
        {
            if (child->getSourceRef())
                return child->getSourceRef();
        }
    }

    return m_sourceRef;
}

const std::string &AstNode::getContent() const { return m_content; }

const std::string &AstNode::getName() const { return m_name; }

const std::vector<std::shared_ptr<AstNode>> &AstNode::getChildren() const { return m_children; }

AstNodeBuilder::AstNodeBuilder(std::string name) : m_id(++m_currentId), m_name(std::move(name)) {}

size_t AstNodeBuilder::getId() const { return m_id; }

std::shared_ptr<AstNode> AstNodeBuilder::build() const
{
    /**
     * The constructor of AstNode is private. Hence we can't directly use make_shared, because it internally calls
     * the constructor and, as we set, for any other class; apart from AstNodeBuilder, it's private.
     */
    AstNode *node = new AstNode(m_id, m_name);
    return std::shared_ptr<AstNode>(node);
}

const std::string &AstNodeBuilder::getName() const { return m_name; }
