#include "tokenizer/BasicTokenizer.h"

BasicTokenizer::BasicTokenizer()
    : m_buffer(nullptr), m_address(0), m_bufferSize(0), m_col(1), m_lastTokenCol(1), m_lastTokenLine(1), m_line(1),
      LogSink(g_logger.get(), LogSegment("Tokenizer").colorize(Colors::yellow))
{
}

BasicTokenizer::~BasicTokenizer()
{
    m_tokens.clear();
}

bool BasicTokenizer::tokenize(char *buffer, size_t address, size_t bufferSize)
{
    if (!buffer || address >= bufferSize)
    {
        LogSink::pushLog(LogMessage("").add("Could not tokenize because buffer, address or buffer size invalid"));
        return false;
    }

    m_buffer = buffer;
    m_address = address;
    m_bufferSize = bufferSize;
    m_col = m_lastTokenCol = m_lastTokenLine = m_line = 1;
    m_tokens.clear();

    while (m_address < m_bufferSize)
    {
        IRTokenType type = IRTokenType::Invalid;
        if (!tokenize(buffer, m_bufferSize, type))
        {
            logTokenizerError(LogMessage("").add("Unexpected token"));
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
    tokenLogCheckPoint();
    
    char ch = peek();
    tokenType = IRTokenType::Invalid;
    TokenInformation information{.m_type = tokenType, .m_col = m_col, .m_line = m_line, .m_str = ""};

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
                    // Unknown escape, keep as-is (e.g., \x stays as \x)
                    information.m_str += '\\';
                    information.m_str += ch;
                    break;
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
        
        tokenLogCheckPoint(); // We want to know where this character was expected.
        if (!isFinished)
        {
            logTokenizerError(LogMessage("").add("Expected quote to mark end of string!"));
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
            logTokenizerError(LogMessage("").add("Unrecognised token"));
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
    TokenInformation information{.m_type = IRTokenType::Identifier, .m_col = m_col, .m_line = m_line, .m_str = ""};

    tokenLogCheckPoint();
    if (!TokenizerHelpers::isLetter(ch) && !TokenizerHelpers::isSpecial(ch))
    {
        logTokenizerError(LogMessage("").add("Expected -, - or letter for the start of an identifier"));
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
    }

    token = information.m_type;
    m_tokens.push_back(information);

    return true;
}

bool BasicTokenizer::tokenizeNumber(char *buffer, size_t bufferSize, IRTokenType &token)
{
    TokenInformation information{.m_type = IRTokenType::NumberInt, .m_str = ""};

    information.m_str += peek();
    while (consume())
    {
        char ch = peek();
        if (TokenizerHelpers::isDot(ch))
        {
            // Number is a float value.
            tokenLogCheckPoint();
            if (information.m_type == IRTokenType::NumberFloat)
            {
                // If there's a double dot, it's bad formed.
                logTokenizerError(LogMessage("").add("Dot not expected here, expected digits"));
                return false;
            }
            information.m_type = IRTokenType::NumberFloat;
        }
        else if (!TokenizerHelpers::isDigit(ch))
        {
            break;
        }
        information.m_str += ch;
    }

    token = information.m_type;
    m_tokens.push_back(information);

    return true;
}

void BasicTokenizer::logTokenizerError(const LogMessage &logMessage)
{
    LogSink::pushLog(LogMessage("")
                         .add("Tokenize error at line: {}, column: {} -> ", m_lastTokenLine, m_lastTokenCol)
                         .add(logMessage.getRawMessage())
                         .colorize(Colors::red));
}

void BasicTokenizer::tokenLogCheckPoint()
{
    m_lastTokenCol = m_col;
    m_lastTokenLine = m_line;
}