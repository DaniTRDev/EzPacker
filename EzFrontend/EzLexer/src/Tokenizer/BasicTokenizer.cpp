#include "Tokenizer/BasicTokenizer.h"

BasicTokenizer::BasicTokenizer(const std::shared_ptr<ErrorCollector> &errorCollector,
                               const std::shared_ptr<SourceManager> &sourceManager) :
    m_buffer(nullptr), m_address(0), m_bufferSize(0), m_col(0), m_line(0), m_sourceId(0),
    m_errorCollector(errorCollector), m_sourceManager(sourceManager)
{
}

bool BasicTokenizer::tokenizeBuffer(size_t address, size_t sourceId)
{
    std::string_view buffer = m_sourceManager->getSourceContent(sourceId);
    m_errorCollector->beginScope();

    size_t buffSize = buffer.size();
    if (address >= buffSize)
    {
        m_errorCollector->onError(ErrorSeverity::Fatal,
                                  "Could not tokenizeBuffer because buffer, address or buffer size is invalid",
                                  "Tokenizer");
        m_errorCollector->endScope(ErrorAction::Commit); // If there was any error, commit it.
        return false;
    }

    m_buffer = (char *)buffer.data();
    m_address = address;
    m_bufferSize = buffSize - address;
    m_col = m_line = 0;
    m_tokens.clear();
    m_sourceId = sourceId;

    while (m_address < m_bufferSize)
    {
        _TokenType type = _TokenType::Invalid;
        if (!tokenizeSingle(m_buffer, m_bufferSize, type))
        {
            m_errorCollector->onError(ErrorSeverity::Fatal, "Unexpected token", "Tokenizer");
            m_errorCollector->endScope(ErrorAction::Commit); // If there was any error, commit it.
            return false;
        }
    }

    m_errorCollector->endScope(ErrorAction::Discard);
    return true;
}

const std::vector<TokenInformation> &BasicTokenizer::getTokens() const { return m_tokens; }

bool BasicTokenizer::consume()
{
    if (m_buffer && (++m_address < m_bufferSize))
    {
        m_col++;
        return true;
    }

    return false;
}

char BasicTokenizer::peek() const
{
    if (m_buffer && m_address < m_bufferSize)
        return m_buffer[m_address];

    return 0;
}

bool BasicTokenizer::tokenizeSingle(char *buffer, size_t bufferSize, _TokenType &tokenType)
{
    char ch = peek();

    tokenType = _TokenType::Invalid;
    TokenInformation information{ .m_type = tokenType,
                                  .m_sourceReference = m_sourceManager->createReference(m_col, 1, m_line, m_sourceId),
                                  .m_str = "" };

    switch (ch)
    {
        case '\r':
        {
            if ((m_address + 1) < m_bufferSize && buffer[m_address + 1] == '\n')
            {
                ++m_address; // consume the '\n' too
            }
        }
        case '\n':
        {
            consume();
            ++m_line;  // count a new line
            m_col = 0; // reset column

            return true;
        }
        case ' ':
        {
            consume();
            return true;
        }
        case ':':
        {
            information.m_type = _TokenType::Colon;
            information.m_str += ch;

            break;
        }
        case ',':
        {
            information.m_type = _TokenType::Comma;
            information.m_str += ch;

            break;
        }
        case '.':
        {
            information.m_type = _TokenType::Dot;
            information.m_str += ch;

            break;
        }
        case '>':
        {
            information.m_type = _TokenType::GreaterThan;
            information.m_str += ch;

            break;
        }
        case '#':
        {
            information.m_type = _TokenType::Comment;
            while (consume())
            {
                ch = peek();
                information.m_sourceReference.m_length++;

                if (TokenizerHelpers::isEndOfLine(ch))
                {
                    break; // Let the next iteration detect \n and update lines and the rest of things.
                }

                information.m_str += ch;
            }

            // m_tokens.push_back(information);
            return true;
        }
        case '{':
        {
            information.m_type = _TokenType::LeftBrace;
            information.m_str += ch;

            break;
        }
        case '(':
        {
            information.m_type = _TokenType::LeftParen;
            information.m_str += ch;

            break;
        }
        case '<':
        {
            information.m_type = _TokenType::LowerThan;
            information.m_str += ch;

            break;
        }
        case '-':
        {
            information.m_type = _TokenType::Minus;
            information.m_str += ch;

            break;
        }
        case '%':
        {
            information.m_type = _TokenType::Percentage;
            information.m_str += ch;

            break;
        }
        case '+':
        {
            information.m_type = _TokenType::Plus;
            information.m_str += ch;

            break;
        }
        case '}':
        {
            information.m_type = _TokenType::RightBrace;
            information.m_str += ch;

            break;
        }
        case ')':
        {
            information.m_type = _TokenType::RightParen;
            information.m_str += ch;

            break;
        }
        case ';':
        {
            information.m_type = _TokenType::SemiColon;
            information.m_str += ch;

            break;
        }
        case '\t':
        {
            information.m_type = _TokenType::Tab;
            information.m_str += ch;

            break;
        }
        case '"':
        {
            bool isEscaping = false;
            bool isFinished = false;
            information.m_type = _TokenType::String;

            while (consume())
            {
                ch = peek();
                information.m_sourceReference.m_length++;

                if (isEscaping)
                {
                    switch (ch)
                    {
                        case 'n':
                        {
                            information.m_str += '\n';
                            break;
                        }
                        case 'r':
                        {
                            information.m_str += '\r';
                            break;
                        }
                        case 't':
                        {
                            information.m_str += '\t';
                            break;
                        }
                        case '\\':
                        {
                            information.m_str += '\\';
                            break;
                        }
                        case '\'':
                        {
                            information.m_str += '\'';
                            break;
                        }
                        case '\"':
                        {
                            information.m_str += '\"';
                            break;
                        }
                        case '0':
                        {
                            information.m_str += '\0';
                            break;
                        }
                        default:
                        {
                            // Unknown escape throw error.
                            m_errorCollector->onError(ErrorSeverity::Fatal,
                                                      "Unknown escape sequence",
                                                      "Tokenizer",
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
                    consume(); // Skip " character.

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
                m_errorCollector->onError(ErrorSeverity::Fatal,
                                          "Expected quote to mark end of string!",
                                          "Tokenizer",
                                          information.m_sourceReference);
                return false;
            }

            m_tokens.push_back(information);
            return true;
        }
        default:
        {
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
                // We don't have a token for the character, throw error.
                m_errorCollector->onError(ErrorSeverity::Fatal,
                                          std::format("Unrecognised token: {}", ch),
                                          "Tokenizer",
                                          information.m_sourceReference);
                return false;
            }
        }
    }

    tokenType = information.m_type;
    m_tokens.push_back(information);
    consume();

    return true;
}

bool BasicTokenizer::tokenizeIdentifier(char *buffer, size_t bufferSize, _TokenType &token)
{
    char ch = peek();
    TokenInformation information{ .m_type = _TokenType::Identifier,
                                  .m_sourceReference = m_sourceManager->createReference(m_col, 1, m_line, m_sourceId),
                                  .m_str = "" };

    if (!TokenizerHelpers::isLetter(ch) && !TokenizerHelpers::isSpecial(ch))
    {
        m_errorCollector->onError(ErrorSeverity::Fatal,
                                  "Expected -, - or letter for the start of an identifier",
                                  "Tokenizer",
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
        information.m_sourceReference.m_length++;
    }

    // TODO: Ensure proper handling of reserver keywords.
    std::string identifierToLower = StrToLower(information.m_str);
    if (identifierToLower == "case")
    {
        information.m_type = _TokenType::Case;
    }
    else if (identifierToLower == "break")
    {
        information.m_type = _TokenType::Break;
    }
    else if (identifierToLower == "continue")
    {
        information.m_type = _TokenType::Continue;
    }
    else if (identifierToLower == "default")
    {
        information.m_type = _TokenType::Default;
    }
    else if (identifierToLower == "for")
    {
        information.m_type = _TokenType::For;
    }
    else if (identifierToLower == "if")
    {
        information.m_type = _TokenType::If;
    }
    else if (identifierToLower == "include")
    {
        information.m_type = _TokenType::Include;
    }
    else if (identifierToLower == "else")
    {
        information.m_type = _TokenType::Else;
    }
    if (identifierToLower == "switch")
    {
        information.m_type = _TokenType::Switch;
    }
    else if (identifierToLower == "while")
    {
        information.m_type = _TokenType::While;
    }

    token = information.m_type;
    m_tokens.push_back(information);

    return true;
}

bool BasicTokenizer::tokenizeNumber(char *buffer, size_t bufferSize, _TokenType &token)
{
    TokenInformation information{ .m_type = _TokenType::NumberInt,
                                  .m_sourceReference = m_sourceManager->createReference(m_col, 1, m_line, m_sourceId),
                                  .m_str = "" };

    bool canBeHex = peek() == '0';
    bool isHex = false;

    information.m_str += peek();
    information.m_sourceReference.m_length++;

    while (consume())
    {
        char ch = peek();

        if (ch == '.')
        {
            // Number is a float value.
            if (information.m_type == _TokenType::NumberFloat)
            {
                // If there's a double dot, it's bad formed.
                m_errorCollector->onError(ErrorSeverity::Fatal,
                                          "Dot not expected here, expected digits",
                                          "Tokenizer",
                                          information.m_sourceReference);
                return false;
            }
            information.m_type = _TokenType::NumberFloat;
        }
        else if (ch == 'x' || ch == 'X')
        {
            if (canBeHex)
            {
                isHex = true;
            }
            else
                break; // Not a digit.
        }
        else if (!TokenizerHelpers::isDigit(ch))
        {
            if (!isHex)
                break;

            if (std::tolower(ch) < 'a' || std::tolower(ch) > 'f')
                break; // Not a hex digit.
        }

        information.m_sourceReference.m_length++;
        information.m_str += ch;
        canBeHex = false;
    }

    if (isHex && information.m_str.size() <= 2)
    {
        // User input 0xZZPP..., which is not a valid hex number.
        m_errorCollector->onError(ErrorSeverity::Fatal,
                                  "Malformed hex number",
                                  "Tokenizer",
                                  information.m_sourceReference);

        return false;
    }

    token = information.m_type;
    m_tokens.push_back(information);

    return true;
}