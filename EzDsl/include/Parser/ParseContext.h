#ifndef EZDSL_PARSE_CONTEXT_H
#define EZDSL_PARSE_CONTEXT_H

#include "EzDslCommon.h"
#include "SourceManager/GenericSourceManager.h"

/**
 * This class represents a cheap-to-create context used PER-FILE.
 */
class ParseContext
{
  public:
    /**
     * Creates the parsing context with the given diagnostic collector, source manager and sourceId.
     */
    ParseContext(class DiagnosticCollector *diagCollector, class GenericSourceManager *sourceManager, size_t sourceId);

    /**
     * Returns the diagnostic collector linked to this context.
     */
    class DiagnosticCollector *getDiagCollector() const;

    /**
     * Returns the source manager linked to this context.
     */
    class GenericSourceManager *getSourceManager() const;

    /**
     * Returns the source ID linked to this context.
     */
    size_t getSourceId() const;

    /**
     * Creates a source reference out of the given starting and ending iter (given by lexy).
     */
    class SourceReference *createRef(const char *startIter, const char *endIter);

    /**
     * Simple diagnostic collector that will get lexy's errors and will push them into our collector.
     */
    struct LexyDiagnosticHandler
    {
        ParseContext &ctx;

        struct ErrorSink
        {
            ParseContext &ctx;
            std::size_t _count;
            using return_type = std::size_t;

            template <typename Input, typename Reader, typename Tag>
            void operator()(const lexy::error_context<Input> &context, const lexy::error<Reader, Tag> &error)
            {
                GenericSourceManager *sm = ctx.getSourceManager();
                const size_t sourceId = ctx.getSourceId();
                const char *basePtr = sm->getSourceContent(sourceId).data();
                std::string_view sourceName = sm->getSourceName(sourceId);
                std::string errorMsg = "";

                // Calculate source offsets from the error's iterator range
                auto errBegin = error.position();
                auto errSize = 0;

                // Write the main annotation.
                if constexpr (std::is_same_v<Tag, lexy::expected_literal>)
                {
                    errSize = error.index() + 1;
                    errorMsg = std::format("Expected '{}'", error.string());
                }
                else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>)
                {
                    errSize = std::distance(error.position(), error.end());
                    errorMsg = std::format("Expected keyword '{}'", error.string());
                }
                else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>)
                {
                    errSize = 1;
                    errorMsg = std::format("Expected '{}'", error.name());
                }
                else
                {
                    errSize = std::distance(error.position(), error.end());
                    errorMsg = error.message();
                }

                const size_t startOffset = static_cast<size_t>(errBegin - basePtr);

                SourceReference *ref = sm->createReference(startOffset, errSize, sourceId);
                ctx.pushToCollector(sourceName, errorMsg, ref);

                ++_count;
            }

            std::size_t finish() && { return _count; }
        };

        constexpr auto sink() const { return ErrorSink{ ctx }; }
    };

    /**
     * Tries to parse the source file linked to this context using the given rule. If an error is thrown, false is
     * returned and the diagnostic collector will have the information.
     */
    template <typename Rule, typename Ret> std::optional<Ret> parse()
    {
        std::string_view content = m_sourceManager->getSourceContent(m_sourceId);
        lexy::string_input input(content);

        auto result = lexy::parse<Rule>(input, *this, m_handler);
        if (!result.is_success())
        {
            // Errors have already been reported to diag via LexyDiagnosticHandler
            return std::nullopt;
        }

        return std::move(result.value());
    }

  private:
    /**
     * Pushes the given error into the linked collector.
     */
    void pushToCollector(std::string_view sourceName, std::string_view message, class SourceReference *sourceRef);

  private:
    class DiagnosticCollector *m_diagCollector;
    class GenericSourceManager *m_sourceManager;
    LexyDiagnosticHandler m_handler;
    size_t m_sourceId;
};

#endif // EZDSL_PARSE_CONTEXT_H
