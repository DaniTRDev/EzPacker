#ifndef EZPACKER_IFRONTENDLOGSINK_H
#define EZPACKER_IFRONTENDLOGSINK_H

/**
 * To implement these methods, parent classes will surely inherit from EzLogger::LogSink.
 * This interface defines common methods used by different frontend parts to ensure correct logging and information
 * output.
 */
class IFrontendLogSink
{
  public:
    virtual ~IFrontendLogSink() = default;

    /**
     * Begins a new log block.
     */
    virtual void beginLogBlock() = 0;

    /**
     * Finishes the log block, and logs it if commit is set to true.
     */
    virtual void endLogBlock(bool commit) = 0;

  protected:
    std::stack<std::queue<LogMessage>> m_logStack;
};

#endif // EZPACKER_IFRONTENDLOGSINK_H
