#ifndef EZPACKER_IPARSINGCONTEXT_H
#define EZPACKER_IPARSINGCONTEXT_H

#include "EzLexerCommon.h"
#include "Tokenizer/ITokenizer.h"
#include "ErrorCollector/ErrorCollector.h"
#include "SourceManager/SourceManager.h"

/**
 * Interface that modelates how a parsing context should operate. This represents an important abstraction layer to
 * be able to add different types of contexts (for example, a context for multi threaded compilation).
 */
class IParsingContext
{
  public:
    ~IParsingContext() = default;

    /**
     * Returns true if the current token stream can be peeked.
     * @return bool
     */
    virtual bool canPeek() const = 0;

    /**
     * Returns the current position in the token stream.
     * @return size_t
     */
    virtual size_t getCurrentPosition() const = 0;

    /**
     * Returns the number of tokens left to parse.
     * @return size_t
     */
    virtual size_t getRemainingTokenCount() const = 0;

    /**
     * Peeks the current context without consuming the token. canPeek must have returned true.
     * @return const TokenInformation &
     */
    virtual const TokenInformation &peek() const = 0;

    /**
     * Begins a new multiple source reference that will be filled with "consumed nodes".
     */
    virtual void beginMultiSourceRef() = 0;

    /**
     * Advances the stream position by 1 if canPeek didn't return false. Will push the consumed node into the
     * current "merged" source reference for the caller AstNode. If no multi source ref was pushed, an access violation
     * is thrown.
     */
    virtual void consume() = 0;

    /**
     * Ends the current multiple source reference.
     */
    virtual void endMultiSourceRef() = 0;

    /**
     * Sets the position of the stream to the one given, if pos is invalid an exception is thrown.
     */
    virtual void setPosition(size_t pos) = 0;

    /**
     * Returns the error collector this context is linked to.
     * @return const std::shared_ptr<ErrorCollector> &
     */
    virtual const std::shared_ptr<ErrorCollector> &getErrorCollector() const = 0;

    /**
     * Returns the source manager this context is linked to.
     * @return const std::shared_ptr<SourceManager> &
     */
    virtual const std::shared_ptr<SourceManager> &getSourceManager() const = 0;

    /**
     * Returns the current multiple reference.
     * @return std::vector<std::shared_ptr<SourceReference>>
     */
    virtual std::vector<std::shared_ptr<SourceReference>> getCurrentMultiReference() const = 0;
};

#endif // EZPACKER_IPARSINGCONTEXT_H
