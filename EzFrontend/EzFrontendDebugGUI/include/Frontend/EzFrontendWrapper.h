#ifndef EZPACKER_EZFRONTENDWRAPPER_H
#define EZPACKER_EZFRONTENDWRAPPER_H

#include "EzFrontendDebugGUICommon.h"

class EzFrontendWrapper
{
  public:
    /**
     * Creates the wrapper with the given source logging sink.
     * @param sink
     */
    EzFrontendWrapper(std::shared_ptr<SourceLoggingSink> sink);

    /**
     * Tries to tokenize given file and returns true if succeeded. If there was an error, they will be pushed
     * to the logger and false will be returned. If success, file content will be returned inside outFileData and
     * outFileDataSize will also have the size of the buffer.
     * @param filePath
     * @param outFileDataSize
     * @param outFileData
     * @return bool
     */
    bool
    openAndTokenize(std::filesystem::path filePath, size_t &outFileDataSize, std::unique_ptr<uint8_t[]> &outFileData);

    /**
     * Tries to parse the opened file. If file was not tokenized or if there was any error, false is returned and an
     * error is pushed into the error collector.
     * @return bool
     */
    bool parse();

    /**
     * Returns the error collector linked to this frontend instance.
     * @return const std::shared_ptr<ErrorCollector> &
     */
    const std::shared_ptr<ErrorCollector> &getErrorCollector() const;

    /**
     * Returns the tokens resulting from openAndTokenize, if any. If there are no tokens, an emty array is returned.
     * @return const std::vector<TokenInformation> &
     */
    const std::vector<TokenInformation> &getTokens() const;

    /**
     * Returns the logging sink used by this frontend instance.
     * @return const std::shared_ptr<SourceLoggingSink> &
     */
    const std::shared_ptr<SourceLoggingSink> &getLoggingSink() const;

    /**
     * Returns the source manager linked to this frontend instance.
     * @return const std::shared_ptr<SourceManager> &
     */
    const std::shared_ptr<SourceManager> &getSourceManager() const;

    /**
     * Returns the result of parsing, if there was any error this will return nullptr.
     * @return const std::shared_ptr<AstNode> &
     */
    const std::vector<std::shared_ptr<AstNode>> &getParseResult() const;

  private:
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<IParsingContext> m_parsingContext;
    std::shared_ptr<SourceLoggingSink> m_sink;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::vector<std::shared_ptr<AstNode>> m_parseResult;
};

#endif // EZPACKER_EZFRONTENDWRAPPER_H
