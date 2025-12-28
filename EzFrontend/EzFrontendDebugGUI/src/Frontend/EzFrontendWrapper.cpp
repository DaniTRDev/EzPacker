#include "Frontend/EzFrontendWrapper.h"

EzFrontendWrapper::EzFrontendWrapper(std::shared_ptr<SourceLoggingSink> sink) : m_sink(std::move(sink))
{
    m_sourceManager = std::make_shared<SourceManager>();
    m_errorCollector = std::make_shared<ErrorCollector>(m_sink, m_sourceManager);
}

bool EzFrontendWrapper::openAndTokenize(std::filesystem::path filePath,
                                        size_t &outFileDataSize,
                                        std::unique_ptr<uint8_t[]> &outFileData)
{
    if (!std::filesystem::exists(filePath))
    {
        m_errorCollector->error(LogMessage("Input file does not exist"), nullptr);
        return false;
    }

    std::ifstream fileStream(filePath, std::ios::app | std::ios::in | std::ios::binary);
    std::string fileName = filePath.filename().string();
    if (!fileStream.is_open())
    {
        m_errorCollector->error(LogMessage("Could not open input file: {}", fileName), nullptr);
        return false;
    }

    fileStream.seekg(0, std::ios::end);
    outFileDataSize = fileStream.tellg();
    fileStream.seekg(std::ios::beg);
    m_errorCollector->information(LogMessage("Opening file: {} of size: 0x{:X}b", fileName, outFileDataSize));

    outFileData = std::make_unique<uint8_t[]>(outFileDataSize);
    if (fileStream.read((char *)outFileData.get(), outFileDataSize).tellg() < outFileDataSize)
    {
        m_errorCollector->error(LogMessage("Could not read the file correctly, is it too big?"), nullptr);
        return false;
    }

    m_sourceManager->addSourceContent(fileName, std::string((char *)outFileData.get(), outFileDataSize));
    m_tokenizer = std::make_shared<BasicTokenizer>(m_sink, m_sourceManager, fileName);

    if (!m_tokenizer->tokenizeBuffer((char *)outFileData.get(), 0, outFileDataSize))
    {
        m_errorCollector->error(LogMessage("Could not tokenize the file"), nullptr);
        return false;
    }

    m_errorCollector->information(
            LogMessage("Tokenized file: {} (token count: {})", fileName, m_tokenizer->getTokens().size()));
    return true;
}

bool EzFrontendWrapper::parse()
{
    auto &tokens = m_tokenizer->getTokens();
    m_parseResult.clear();

    if (tokens.empty())
    {
        m_errorCollector->error(LogMessage("Invalid tokens from file"), nullptr);
        return false;
    }

    m_parsingContext = std::make_shared<SingleThreadParsingContext>(m_errorCollector, m_sourceManager, tokens);
    while (m_parsingContext->getRemainingTokenCount() != 0)
    {
        std::shared_ptr<AstNode> node = AstNodeParsingUtils::tryParsers<ModuleParser, VariableParser>(m_parsingContext);
        if (!node)
        {
            m_errorCollector->error(LogMessage("Could not parse node"), nullptr);
            return false;
        }

        m_parseResult.push_back(std::move(node));
    }

    if (m_errorCollector->areThereErrors())
    {
        m_errorCollector->exitScope(ErrorHandleType::Commit); // Once parsed finished, commit any possible error.
        return false;
    }

    return true;
}

const std::shared_ptr<ErrorCollector> &EzFrontendWrapper::getErrorCollector() const { return m_errorCollector; }

const std::shared_ptr<SourceLoggingSink> &EzFrontendWrapper::getLoggingSink() const { return m_sink; }

const std::shared_ptr<SourceManager> &EzFrontendWrapper::getSourceManager() const { return m_sourceManager; }

const std::vector<TokenInformation> &EzFrontendWrapper::getTokens() const { return m_tokenizer->getTokens(); }

const std::vector<std::shared_ptr<AstNode>> &EzFrontendWrapper::getParseResult() const { return m_parseResult; }
