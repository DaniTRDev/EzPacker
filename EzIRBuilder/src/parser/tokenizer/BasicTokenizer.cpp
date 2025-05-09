#include "parser/tokenizer/BasicTokenizer.h"

BasicTokenizer::BasicTokenizer()
    : m_col(), m_line(0), LogSink(g_logger.get(), LogSegment("Tokenizer").colorize(Colors::yellow))
{
}

BasicTokenizer::~BasicTokenizer()
{
    m_instructions.clear();
    m_keywords.clear();
    m_tokens.clear();
    m_types.clear();
}

bool BasicTokenizer::tokenize(char *buffer, size_t address, size_t bufferSize)
{
    if (!buffer || address >= bufferSize)
    {
        LogSink::pushLog(LogMessage("").add("Could not tokenize because buffer, address or buffer size invalid"));
        return false;
    }

    m_col = 1;
    m_line = 1;

    while (address < bufferSize)
    {
        char ch = buffer[address];
        if (TokenizerHelpers::isWhiteSpace(ch))
        {
            address++;
            newCol();

            continue;
        }
        else if (TokenizerHelpers::isEndOfLine(ch))
        {
            address++;
            newLine();

            continue;
        }

        IRTokenType type = IRTokenType::Invalid;
        address += tokenize(buffer, address, bufferSize, type);

        if (type == IRTokenType::Invalid)
        {
            logTokenizerError(LogMessage("").add("Unexpected token"));
            return false;
        }
    }

    return true;
}

void BasicTokenizer::addInstructionTable(const std::set<std::string> &instructions)
{
    m_instructions = instructions;
}

void BasicTokenizer::addKeywordMap(const std::map<std::string, IRTokenType> &keywords)
{
    m_keywords = keywords;
}

void BasicTokenizer::addTypeMap(const std::map<std::string, IRTokenType> &types)
{
    m_types = types;
}

const std::vector<TokenInformation> &BasicTokenizer::getTokens() const
{
    return m_tokens;
}

size_t BasicTokenizer::tokenize(char *buffer, size_t address, size_t bufferSize, IRTokenType &tokenType)
{
    char ch = buffer[address];
    if (TokenizerHelpers::isDot(ch))
    {
        // Keyword
        size_t res = tokenizeIdentifier(buffer, address, bufferSize, tokenType);

        if (res != 0)
        {
            auto &token = m_tokens.back();
            if (auto instr = m_instructions.contains(token.m_str); instr)
                tokenType = IRTokenType::Instruction;
            else if (auto type = m_types.find(token.m_str); type != m_types.end())
                tokenType = type->second;
            else if (auto keyword = m_keywords.find(token.m_str); keyword != m_keywords.end())
                tokenType = keyword->second;
            else
            {
                logTokenizerError(LogMessage("").add("Keyword not recognized"));
                tokenType = IRTokenType::Invalid;
                m_tokens.pop_back();
                return 0;
            }
            
            token.m_type = tokenType;
        }

        return res;
    }
    else if (TokenizerHelpers::isDoubleDot(ch))
    {
        tokenType = IRTokenType::DoubleDot;
        m_tokens.push_back({.m_type = tokenType, .m_col = m_col, .m_line = m_line, .m_str = ":"});
        
        return 1;
    }
    else if (TokenizerHelpers::isPercentage(ch))
    {
        // Register
        size_t res = tokenizeIdentifier(buffer, address, bufferSize, tokenType);
        if (res != 0)
        {
            tokenType = IRTokenType::Register;
            m_tokens.back().m_type = tokenType; // Last element have been tokenized as identifier, but it's not one.
        }
        return res;
    }
    else if (TokenizerHelpers::isComma(ch))
    {
        // ,
        tokenType = IRTokenType::Comma;
        m_tokens.push_back({.m_type = tokenType, .m_col = m_col, .m_line = m_line, .m_str = ","});
        
        return 1;
    }
    else if (TokenizerHelpers::isLeftParen(ch))
    {
        // (
        tokenType = IRTokenType::LeftParen;
        m_tokens.push_back({.m_type = tokenType, .m_col = m_col, .m_line = m_line, .m_str = "("});
        
        return 1;
    }
    else if (TokenizerHelpers::isRightParen(ch))
    {
        // )
        tokenType = IRTokenType::RightParen;
        m_tokens.push_back({.m_type = tokenType, .m_col = m_col, .m_line = m_line, .m_str = ")"});
        
        return 1;
    }
    else if (TokenizerHelpers::isHashtag(ch))
    {
        // Comment.
        return tokenizeComment(buffer, address, bufferSize, tokenType);
    }
    else if (TokenizerHelpers::isQuote(ch))
    {
        return tokenizeString(buffer, address, bufferSize, tokenType);
    }
    else
    {
        // Identifier or Number.
        if (TokenizerHelpers::isSpecial(ch) || TokenizerHelpers::isLetter(ch))
        {
            // Identifier.
            return tokenizeIdentifier(buffer, address, bufferSize, tokenType);
        }
        else if (TokenizerHelpers::isDigit(ch))
        {
            // Number.
            return tokenizeNumber(buffer, address, bufferSize, tokenType);
        }
    }

    return 0;
}

size_t BasicTokenizer::tokenizeComment(char *buffer, size_t address, size_t bufferSize, IRTokenType &token)
{
    TokenInformation information{.m_type = IRTokenType::Comment, .m_col = m_col, .m_line = m_line, .m_str = ""};
    size_t size = 1;

    information.m_str += buffer[address++];
    while (address < bufferSize)
    {
        char ch = buffer[address];

        if (TokenizerHelpers::isEndOfLine(ch))
        {
            break;
        }

        information.m_str += buffer[address++];
        size++;
        newCol();
    }

    token = information.m_type;
    m_tokens.push_back(information);
    
    return size;
}

size_t BasicTokenizer::tokenizeIdentifier(char *buffer, size_t address, size_t bufferSize, IRTokenType &token)
{
    TokenInformation information{.m_type = IRTokenType::Identifier, .m_col = m_col, .m_line = m_line, .m_str = ""};
    size_t size = 1;

    information.m_str += buffer[address++];
    while (address < bufferSize)
    {
        char ch = buffer[address];
        if (!TokenizerHelpers::isLetter(ch) && !TokenizerHelpers::isDigit(ch) && !TokenizerHelpers::isSpecial(ch))
        {
            break;
        }

        information.m_str += buffer[address++];
        size++;
        newCol();
    }

    token = information.m_type;
    m_tokens.push_back(information);

    return size;
}

size_t BasicTokenizer::tokenizeNumber(char *buffer, size_t address, size_t bufferSize, IRTokenType &token)
{
    TokenInformation information{.m_type = IRTokenType::NumberInt, .m_col = m_col, .m_line = m_line, .m_str = ""};
    size_t size = 1;

    information.m_str += buffer[address++];
    while (address < bufferSize)
    {
        char ch = buffer[address];
        if (TokenizerHelpers::isWhiteSpace(ch) || TokenizerHelpers::isEndOfLine(ch))
        {
            break;
        }

        if (TokenizerHelpers::isDot(ch))
        {
            // Number is a float value.
            if (information.m_type == IRTokenType::NumberFloat)
            {
                // If there's a double dot, it's bad formed.
                logTokenizerError(LogMessage("").add("Dot not expected here, expected digits"));
                return 0;
            }
            information.m_type = IRTokenType::NumberFloat;
        }
        else if (!TokenizerHelpers::isDigit(ch))
        {
            // Not a valid character for a number, we might have finished tokenizing the number.
            break;
        }

        information.m_str += buffer[address++];
        size++;
        newCol();
    }

    token = information.m_type;
    m_tokens.push_back(information);
    
    return size;
}

size_t BasicTokenizer::tokenizeString(char *buffer, size_t address, size_t bufferSize, IRTokenType &token)
{
    bool wasFinished = false; // Is this string finished aka has a closing quotation mark?
    bool wasThereABackSlash = false;
    TokenInformation information{.m_type = IRTokenType::String, .m_col = m_col, .m_line = m_line, .m_str = ""};
    size_t size = 1;

    address++; // Don't insert the first quotation mark.
    while (address < bufferSize)
    {
        char ch = buffer[address];
        bool insert = true;

        if (TokenizerHelpers::isBackSlash(ch))
        {
            if (wasThereABackSlash)
            {
                // '\\' represent a single backslash. Used to scape format.
                information.m_str += buffer[address++]; // Add backslash from input buffer.
                wasThereABackSlash = false;
            }
            else if (wasFinished)
            {
                // String was closed and a backSlash have been found. There's a continuation of this string in the next
                // line.
                newLine();
                size_t nextLineStrSize = tokenizeString(buffer, address + 1, bufferSize, token);
                
                if (nextLineStrSize == 0)
                    return nextLineStrSize; // There was an error with the string on the next line, propagate it.

                address += nextLineStrSize + 1;
                size += nextLineStrSize;

                // Insert new line string.
                auto &token = m_tokens.back();
                information.m_str.append(token.m_str);
                m_tokens.pop_back(); // And remove the generated token from the list.
            }
            else
            {
                address++;
                wasThereABackSlash = true;
            }
            
            insert = false;
        }
        else if (TokenizerHelpers::isQuote(ch))
        {
            wasFinished = true;
            address++;
            insert = false;
        }
        else if (TokenizerHelpers::isEndOfLine(ch))
        {
            break;
        }
        else if (wasThereABackSlash)
        {
            // Previous character is a backslash. Check for special code.
            if (ch == 'n')
            {
                information.m_str += '\n';
            }
            else if (ch == 'r')
            {
                information.m_str += '\r';
            }
            else
            {
                logTokenizerError(LogMessage("").add("Invalid special character after '\\'"));
                return 0;
            }
            
            address++;
            insert = false;
            wasThereABackSlash = false;
        }
        else if (wasFinished)
        {
            // If string was finished, character is not a white space and none of the conditions before triggered, it
            // means we had 1 string and not multi-line strings.
            if (!TokenizerHelpers::isWhiteSpace(ch))
                break;
            else
            {
                // Spaces after '"', need to omit in case there's a '\'.
                address++;
                insert = false;
            }
            
        }

        if (insert)
            information.m_str += buffer[address++];

        size++;
        newCol();
    }

    token = IRTokenType::String;
    m_tokens.push_back(information);

    return size;
}

void BasicTokenizer::logTokenizerError(const LogMessage &logMessage)
{
    LogSink::pushLog(
        LogMessage("")
            .add("Tokenize error at line {}, col: {}. \t Error: {}", m_line, m_col, logMessage.getRawMessage())
            .colorize(Colors::red));
}

void BasicTokenizer::newLine()
{
    m_line++;
    m_col = 0;
}

void BasicTokenizer::newCol()
{
    m_col++;
}
