#include "NodeAnnotators/ConstantAnnotator.h"

bool ConstantAnnotator::annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger)
{
    if (!logger)
        return false; // If no logger, exit.

    if (!node)
    {
        logger->logError(LogMessage("Invalid constant node"));
        return false;
    }

    std::shared_ptr<ConstantAnnotation> annot;
    const std::shared_ptr<SourceReference> &sourceRef = node->getSourceRef();
    std::string nodeContent = node->getContent(); // Force copy, we are going to need to modify this.

    if (node->getId() == AstNodes::FloatNumber().getId())
    {
        /*
         * Since speed matters, using std::from_chars is not a good option. From what I've seen on certain forums,
         * it can be 40% slower than regular std::sto... functions because of localing and interop layers of C++.
         */
        float value = std::stof(node->getContent());
        if (value == 0.f)
        {
            // From official documentation, stof can return 0 if it failed parsing the input, so we need to check
            // if the input was a real 0 by traversing each one of the digits.

            for (size_t i = 0; i < nodeContent.size(); i++)
            {
                // We have guaranteed that the node is well-formed: it's of the form: 'X' + '.' + 'X'.....
                if (nodeContent[i] != '0' && nodeContent[i] != '.')
                {
                    logger->logSourceError(LogMessage("Invalid floating point value"), sourceRef);
                    return false;
                }
            }
        }

        annot = std::make_shared<ConstantAnnotation>(m_typeId, value);
    }
    else if (node->getId() == AstNodes::IntNumber().getId())
    {
        if (nodeContent.empty())
        {
            logger->logSourceError(LogMessage("Constant integer is empty"), sourceRef);
            return false;
        }

        std::shared_ptr<mp_int> integer = std::make_shared<mp_int>();
        if (mp_init(integer.get()) != MP_OKAY || mp_read_radix(integer.get(), nodeContent.c_str(), 10) != MP_OKAY)
        {
            logger->logSourceError(LogMessage("Invalid constant integer"), sourceRef);
            mp_clear(integer.get());

            return false;
        }

        annot = std::make_shared<ConstantAnnotation>(m_typeId, integer);
    }
    else if (node->getId() == AstNodes::String().getId())
    {
        annot = std::make_shared<ConstantAnnotation>(m_typeId, nodeContent);
    }

    node->setAnnotation(annot);
    node->setContent(""); // Safety measure.

    return true;
}

bool ConstantAnnotator::canAnnotate(const std::shared_ptr<AstNode> &node)
{
    return node->getId() == AstNodes::FloatNumber().getId() || node->getId() == AstNodes::IntNumber().getId() ||
            node->getId() == AstNodes::String().getId();
}

void ConstantAnnotator::setTypeId(size_t id) { m_typeId = id; }
