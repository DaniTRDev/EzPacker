#ifndef EZDSL_PARSE_CONTEXT_H
#define EZDSL_PARSE_CONTEXT_H

#include "EzDslCommon.h"
#include "SourceManager/GenericSourceManager.h"

/**
 * Per-file lexical and syntactic parsing state passed to Lexy grammar rules.
 * Holds references to the diagnostic collector, source manager, active file ID,
 * and arena allocator for PMR AST node allocation.
 */
class ParseContext
{
  public:
    /**
     * Constructs a parsing context bound to a DiagnosticCollector, GenericSourceManager, sourceId, and PMR allocator.
     */
    ParseContext(class DiagnosticCollector *diagCollector,
                 class GenericSourceManager *sourceManager,
                 size_t sourceId,
                 std::pmr::memory_resource *alloc);

    /**
     * Returns the diagnostic collector linked to this parsing context.
     */
    class DiagnosticCollector *getDiagCollector() const;

    /**
     * Returns the generic source manager linked to this context.
     */
    class GenericSourceManager *getSourceManager() const;

    /**
     * Returns the numeric source ID of the file being parsed.
     */
    size_t getSourceId() const;

    /**
     * Creates a SourceReference span corresponding to the input iterator interval [startIter, endIter).
     */
    class SourceReference *createRef(const char *startIter, const char *endIter);

    /**
     * Lexy error callback handler intercepting syntax parse failures and translating them into compiler diagnostics.
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

                // Write the main annotation based on error tag category
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
     * Parses the current source buffer using the specified top-level Lexy grammar rule.
     * Returns std::optional containing the resulting AST on success, or std::nullopt if syntax errors occurred.
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

    /**
     * Returns the arena memory resource used for PMR list and node allocations during parsing.
     */
    std::pmr::memory_resource *getAllocator() const;

  private:
    /**
     * Pushes a syntax error message with associated SourceReference to the DiagnosticCollector.
     */
    void pushToCollector(std::string_view sourceName, std::string_view message, class SourceReference *sourceRef);

  private:
    class DiagnosticCollector *m_diagCollector;
    class GenericSourceManager *m_sourceManager;
    LexyDiagnosticHandler m_handler;
    size_t m_sourceId;
    std::pmr::memory_resource *m_alloc;
};

#endif // EZDSL_PARSE_CONTEXT_H
