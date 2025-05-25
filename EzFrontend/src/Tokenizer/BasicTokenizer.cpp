#include "tokenizer/BasicTokenizer.h"

BasicTokenizer::BasicTokenizer(const std::shared_ptr<SourceManager> &sourceManager,
                               const std::shared_ptr<FrontendLogger> &logger, const std::string &source)
    : m_buffer(nullptr), m_address(0), m_bufferSize(0), m_col(0), m_line(0), m_sourceManager(sourceManager),
      m_logger(logger), m_source(source)
{
}

BasicTokenizer::~BasicTokenizer()
{
    m_sourceManager.reset();
    m_tokens.clear();
}

bool BasicTokenizer::tokenize(char *buffer, size_t address, size_t bufferSize)
{
    if (!buffer || address >= bufferSize)
    {
        m_logger->logError(LogMessage("").add("Could not tokenize because buffer, address or buffer size invalid"));
        return false;
    }

    m_buffer = buffer;
    m_address = address;
    m_bufferSize = bufferSize;
    m_col = m_line = 0;
    m_tokens.clear();

    while (m_address < m_bufferSize)
    {
        IRTokenType type = IRTokenType::Invalid;
        if (!tokenize(buffer, m_bufferSize, type))
        {
            m_logger->logError(LogMessage("").add("Unexpected token"));
            return false;
        }
    }

    return true;
}

const std::vector<TokenInformation> &BasicTokenizer::getTokens() const
{
    return m_tokens;
}

bool BasicTokenizer::consume()
{
    if (m_buffer && (m_address++ < m_bufferSize))
    {
        if (TokenizerHelpers::isEndOfLine(m_buffer[m_address]))
        {
            m_col = 1; // Reset line
            m_line++;
        }
        else
        {
            m_col++;
        }
        return true;
    }

    return false;
}

char BasicTokenizer::peek()
{
    if (m_buffer && m_address < m_bufferSize)
        return m_buffer[m_address];

    return 0;
}

bool BasicTokenizer::tokenize(char *buffer, size_t bufferSize, IRTokenType &tokenType)
{
    char ch = peek();
    tokenType = IRTokenType::Invalid;
    TokenInformation information{.m_type = tokenType,
                                 .m_sourceReference = m_sourceManager->createReference(m_col, 1, m_line, m_source),
                                 .m_str = ""};

    switch (ch)
    {
    case ':': {
        information.m_type = IRTokenType::Colon;
        break;
    }
    case ',': {
        information.m_type = IRTokenType::Comma;
        break;
    }
    case '.': {
        information.m_type = IRTokenType::Dot;
        break;
    }
    case '#': {
        information.m_type = IRTokenType::Comment;
        while (consume())
        {
            ch = peek();
            information.m_sourceReference->m_length++;

            if (TokenizerHelpers::isEndOfLine(ch))
            {
                consume(); // Skip '\n'.
                break;
            }

            information.m_str += ch;
        }

        m_tokens.push_back(information);
        return true;
    }
    case '(': {
        information.m_type = IRTokenType::LeftParen;
        break;
    }
    case '\n': {
        information.m_type = IRTokenType::NewLine;
        break;
    }
    case '%': {
        information.m_type = IRTokenType::Percentage;
        break;
    }
    case ')': {
        information.m_type = IRTokenType::RightParen;
        break;
    }
    case '"': {
        bool isEscaping = false;
        bool isFinished = false;
        information.m_type = IRTokenType::String;

        while (consume())
        {
            ch = peek();
            information.m_sourceReference->m_length++;

            if (isEscaping)
            {
                switch (ch)
                {
                case 'n': {
                    information.m_str += '\n';
                    break;
                }
                case 'r': {
                    information.m_str += '\r';
                    break;
                }
                case 't': {
                    information.m_str += '\t';
                    break;
                }
                case '\\': {
                    information.m_str += '\\';
                    break;
                }
                case '\'': {
                    information.m_str += '\'';
                    break;
                }
                case '\"': {
                    information.m_str += '\"';
                    break;
                }
                case '0': {
                    information.m_str += '\0';
                    break;
                }
                default: {
                    // Unknown escape throw error.
                    m_logger->logError(LogMessage("").add("Unrecognised scape sequence"),
                                       information.m_sourceReference);
                    return false;
                }
                }

                isEscaping = false;
                continue;
            }
            else if (ch == '"')
            {
                isFinished = true;
                consume(); // Skip this.

                break;
            }
            else if (ch == '\\')
            {
                isEscaping = true;
                continue;
            }

            information.m_str += ch;
        }

        if (!isFinished)
        {
            m_logger->logError(LogMessage("").add("Expected quote to mark end of string!"),
                               information.m_sourceReference);
            return false;
        }

        m_tokens.push_back(information);
        return true;
    }
    case ' ': {
        information.m_type = IRTokenType::WhiteSpace;
        break;
    }
    default: {
        // Identifier or Number.
        if (TokenizerHelpers::isSpecial(ch) || TokenizerHelpers::isLetter(ch))
        {
            // Identifier.
            return tokenizeIdentifier(buffer, bufferSize, tokenType);
        }
        else if (TokenizerHelpers::isDigit(ch))
        {
            // Number.
            return tokenizeNumber(buffer, bufferSize, tokenType);
        }
        else
        {
            // We don't have a token for the character throw error.
            m_logger->logError(LogMessage("").add("Unrecognised token"), information.m_sourceReference);
            return false;
        }
    }
    }

    tokenType = information.m_type;
    m_tokens.push_back(information);
    consume();

    return true;
}

bool BasicTokenizer::tokenizeIdentifier(char *buffer, size_t bufferSize, IRTokenType &token)
{
    char ch = peek();
    TokenInformation information{.m_type = IRTokenType::Identifier,
                                 .m_sourceReference = m_sourceManager->createReference(m_col, 1, m_line, m_source),
                                 .m_str = ""};

    if (!TokenizerHelpers::isLetter(ch) && !TokenizerHelpers::isSpecial(ch))
    {
        m_logger->logError(LogMessage("").add("Expected -, - or letter for the start of an identifier"),
                           information.m_sourceReference);
        return false;
    }

    information.m_str += peek();
    while (consume())
    {
        ch = peek();
        if (!TokenizerHelpers::isLetter(ch) && !TokenizerHelpers::isDigit(ch) && !TokenizerHelpers::isSpecial(ch))
        {
            break;
        }
        information.m_str += ch;
        information.m_sourceReference->m_length++;
    }

    token = information.m_type;
    m_tokens.push_back(information);

    return true;
}

bool BasicTokenizer::tokenizeNumber(char *buffer, size_t bufferSize, IRTokenType &token)
{
    TokenInformation information{.m_type = IRTokenType::NumberInt,
                                 .m_sourceReference = m_sourceManager->createReference(m_col, 1, m_line, m_source),
                                 .m_str = ""};

    information.m_str += peek();
    while (consume())
    {
        char ch = peek();
        information.m_sourceReference->m_length++;

        if (TokenizerHelpers::isDot(ch))
        {
            // Number is a float value.
            if (information.m_type == IRTokenType::NumberFloat)
            {
                // If there's a double dot, it's bad formed.
                m_logger->logError(LogMessage("").add("Dot not expected here, expected digits"),
                                   information.m_sourceReference);
                return false;
            }
            information.m_type = IRTokenType::NumberFloat;
        }
        else if (!TokenizerHelpers::isDigit(ch))
        {
            information.m_sourceReference->m_length--;
            break;
        }
        information.m_str += ch;
    }

    token = information.m_type;
    m_tokens.push_back(information);

    return true;
}