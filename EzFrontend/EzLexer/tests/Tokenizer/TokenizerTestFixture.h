#ifndef EZPACKER_TOKENIZERTESTFIXTURE_H
#define EZPACKER_TOKENIZERTESTFIXTURE_H

#include "EzLexer.h"
#include <gtest/gtest.h>

class TokenizerTestFixture : public ::testing::Test
{
  public:
    /**
     * Increments by one m_currentPos and returns true if it's still in the bounds of the token array.
     * @return bool
     */
    bool advanceToken();

    /**
     * Returns true if the token at m_currentPos exists and its content is equal to 'content'.
     * @param content
     * @return
     */
    bool expectTokenContent(std::string content);

    /**
     * Returns true if tokenized content count matches the given parameter.
     * @param count
     * @return bool
     */
    bool expectTokenCount(size_t count);
    
    /**
     * Calls m_tokenizer->tokenizeBuffer with the given content while adding content to source manager.
     * @param content
     * @return bool
     */
    bool expectTokenizeResult(const std::string &content);

    /**
     * Expects the token at m_currentPos to exist and its type to be the same as the given one, returns true if that's
     * the case.
     * @param type
     * @return bool
     */
    bool expectTokenType(_TokenType type);
    
    /**
     * Sets up the test by creating a logger, a logging sink and source manager.
     */
    void SetUp() override;

    /**
     * Destroys created logging sink, source manager and logger.
     */
    void TearDown() override;

    /**
     * Creates a basic tokenizer.
     * @return std::shared_ptr<ITokenizer>
     */
    std::shared_ptr<ITokenizer> createBasicTokenizer();

  private:
    size_t m_currentPos;
    std::shared_ptr<ITokenizer> m_tokenizer; // Must be created by createTokenizer.
    std::shared_ptr<SourceLoggingSink> m_loggingSink;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SyncLogger> m_logger;
};

#endif // EZPACKER_TOKENIZERTESTFIXTURE_H
