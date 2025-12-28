#ifndef EZPACKER_SINGLETHREADPARSER_H
#define EZPACKER_SINGLETHREADPARSER_H

#include "IParsingContext.h"
#include "ErrorCollector/ErrorCollector.h"

class SingleThreadParsingContext : public IParsingContext
{
  public:
    /**
     * Creates the parsing context with the given error collector, source manager and token array.
     * @param errorCollector
     * @param sourceManager
     * @param tokens
     */
    SingleThreadParsingContext(std::shared_ptr<ErrorCollector> errorCollector,
                               std::shared_ptr<SourceManager> sourceManager,
                               std::vector<TokenInformation> tokens);

    /**
     * Returns true if the current token stream can be peeked.
     * @return bool
     */
    bool canPeek() const override;

    /**
     * Returns the current position in the token stream.
     * @return size_t
     */
    size_t getCurrentPosition() const override;

    /**
     * Returns the number of tokens left to parse.
     * @return size_t
     */
    size_t getRemainingTokenCount() const override;

    /**
     * Peeks the current context without consuming the token. canPeek must habe returned true.
     * @return const std::shared_ptr<TokenInformation> &
     */
    const TokenInformation &peek() const;

    /**
     * Begins a new multiple source reference that will be filled with "consumed nodes".
     */
    void beginMultiSourceRef() override;

    /**
     * Advances the stream position by 1 if canPeek didn't return false. Will push the consumed node into the
     * current "merged" source reference for the caller AstNode. If no multi source ref was pushed, an access violation
     * is thrown.
     */
    void consume() override;

    /**
     * Ends the current multiple source reference.
     */
    void endMultiSourceRef() override;

    /**
     * Sets the position of the stream to the one given, if pos is invalid an exception is thrown.
     */
    void setPosition(size_t pos) override;

    /**
     * Returns the error collector this context is linked to.
     * @return const std::shared_ptr<ErrorCollector> &
     */
    const std::shared_ptr<ErrorCollector> &getErrorCollector() const override;

    /**
     * Returns the source manager this context is linked to.
     * @return const std::shared_ptr<SourceManager> &
     */
    const std::shared_ptr<SourceManager> &getSourceManager() const override;

    /**
     * Returns the current multiple reference.
     * @return std::vector<std::shared_ptr<SourceReference>>
     */
    std::vector<std::shared_ptr<SourceReference>> getCurrentMultiReference() const override;

  private:
    size_t m_currentPos;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::stack<std::vector<std::shared_ptr<SourceReference>>> m_multiSourceRefs;
    std::vector<TokenInformation> m_tokens;
};

#endif // EZPACKER_SINGLETHREADPARSER_H
