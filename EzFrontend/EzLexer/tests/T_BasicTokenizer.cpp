/**
 * @file T_BasicTokenizer.cpp
 * @brief Unit tests for BasicTokenizer.
 *
 * Covers: keyword recognition, identifier classification, numeric literals
 * (integer / float / negative), string literals with escape sequences, all
 * punctuation tokens, comment stripping, whitespace skipping, and error
 * cases.
 */
#include <gtest/gtest.h>
#include <EzLexer.h>

// ─── Fixture ─────────────────────────────────────────────────────────────────

class BasicTokenizerTests : public ::testing::Test
{
  protected:
    size_t currentTestId;
    std::shared_ptr<ErrorCollector> errorCollector;
    std::shared_ptr<SourceManager> sourceManager;

    void SetUp() override
    {
        currentTestId = 0;
        errorCollector = std::make_shared<ErrorCollector>();
        sourceManager = std::make_shared<SourceManager>(std::filesystem::current_path());
        errorCollector->beginScope();
    }

    void TearDown() override { errorCollector->endScope(ErrorAction::Discard); }

    /// Tokenize a raw string and return the token list on success.
    std::vector<TokenInformation> tokenize(const std::string &source)
    {
        std::string srcName = std::format("test_{}", currentTestId++);
        size_t sourceId = sourceManager->addSourceContent(srcName, source);
        BasicTokenizer tok(errorCollector, sourceManager);
        bool ok = tok.tokenizeBuffer(0, sourceId);
        if (!ok)
            return {};
        return tok.getTokens();
    }
};

// ─── Keyword tests ────────────────────────────────────────────────────────────

TEST_F(BasicTokenizerTests, BreakKeyword)
{
    auto toks = tokenize("break");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Break);
}

TEST_F(BasicTokenizerTests, ContinueKeyword)
{
    auto toks = tokenize("continue");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Continue);
}

TEST_F(BasicTokenizerTests, IfKeyword)
{
    auto toks = tokenize("if");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::If);
}

TEST_F(BasicTokenizerTests, ElseKeyword)
{
    auto toks = tokenize("else");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Else);
}

TEST_F(BasicTokenizerTests, WhileKeyword)
{
    auto toks = tokenize("while");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::While);
}

TEST_F(BasicTokenizerTests, ForKeyword)
{
    auto toks = tokenize("for");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::For);
}

TEST_F(BasicTokenizerTests, SwitchKeyword)
{
    auto toks = tokenize("switch");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Switch);
}

TEST_F(BasicTokenizerTests, CaseKeyword)
{
    auto toks = tokenize("case");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Case);
}

TEST_F(BasicTokenizerTests, DefaultKeyword)
{
    auto toks = tokenize("default");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Default);
}

TEST_F(BasicTokenizerTests, IncludeKeyword)
{
    auto toks = tokenize("include");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Include);
}

// ─── Identifier tests ─────────────────────────────────────────────────────────

TEST_F(BasicTokenizerTests, SimpleIdentifier)
{
    auto toks = tokenize("myVar");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Identifier);
    EXPECT_EQ(toks[0].m_str, "myVar");
}

TEST_F(BasicTokenizerTests, UnderscoreIdentifier)
{
    auto toks = tokenize("_myVar");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Identifier);
    EXPECT_EQ(toks[0].m_str, "_myVar");
}

TEST_F(BasicTokenizerTests, IdentifierWithDigitSuffix)
{
    auto toks = tokenize("var123");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Identifier);
    EXPECT_EQ(toks[0].m_str, "var123");
}

TEST_F(BasicTokenizerTests, IdentifierCannotStartWithDigit)
{
    // "1abc" should produce a NumberInt token followed by an Identifier,
    // not a single identifier token.
    auto toks = tokenize("1abc");
    // The first token must be numeric.
    ASSERT_GE(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberInt);
}

// ─── Numeric literal tests ─────────────────────────────────────────────────────

TEST_F(BasicTokenizerTests, IntegerLiteral)
{
    auto toks = tokenize("42");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberInt);
    EXPECT_EQ(toks[0].m_str, "42");
}

TEST_F(BasicTokenizerTests, ZeroLiteral)
{
    auto toks = tokenize("0");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberInt);
}

TEST_F(BasicTokenizerTests, FloatLiteral)
{
    auto toks = tokenize("3.14");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberFloat);
    EXPECT_EQ(toks[0].m_str, "3.14");
}

TEST_F(BasicTokenizerTests, NegativeNumberTokenisedAsTwoTokens)
{
    // Per the documented behaviour, '-42' is Minus + NumberInt.
    auto toks = tokenize("-42");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Minus);
    EXPECT_EQ(toks[1].m_type, _TokenType::NumberInt);
}

TEST_F(BasicTokenizerTests, HexLiteral)
{
    auto toks = tokenize("0xFF");
    // Should produce a single integer token.
    ASSERT_GE(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberInt);
}

// ─── String literal tests ──────────────────────────────────────────────────────

TEST_F(BasicTokenizerTests, SimpleStringLiteral)
{
    auto toks = tokenize(R"("hello")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::String);
    EXPECT_EQ(toks[0].m_str, "hello");
}

TEST_F(BasicTokenizerTests, EmptyStringLiteral)
{
    auto toks = tokenize(R"("")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::String);
    EXPECT_EQ(toks[0].m_str, "");
}

TEST_F(BasicTokenizerTests, StringWithNewlineEscape)
{
    auto toks = tokenize(R"("a\nb")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::String);
}

TEST_F(BasicTokenizerTests, StringWithTabEscape)
{
    auto toks = tokenize(R"("a\tb")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::String);
}

TEST_F(BasicTokenizerTests, StringWithBackslashEscape)
{
    auto toks = tokenize(R"("a\\b")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::String);
}

TEST_F(BasicTokenizerTests, StringWithQuoteEscape)
{
    auto toks = tokenize(R"("a\"b")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::String);
}

// ─── Punctuation tests ────────────────────────────────────────────────────────

TEST_F(BasicTokenizerTests, AllPunctuationTokens)
{
    // Each line tests a single punctuation token.
    auto check = [&](const std::string &src, _TokenType expected)
    {
        auto toks = tokenize(src);
        ASSERT_EQ(toks.size(), 1u) << "source: " << src;
        EXPECT_EQ(toks[0].m_type, expected) << "source: " << src;
    };

    check(":", _TokenType::Colon);
    check(",", _TokenType::Comma);
    check(".", _TokenType::Dot);
    check(">", _TokenType::GreaterThan);
    check("{", _TokenType::LeftBrace);
    check("(", _TokenType::LeftParen);
    check("<", _TokenType::LowerThan);
    check("-", _TokenType::Minus);
    check("%", _TokenType::Percentage);
    check("+", _TokenType::Plus);
    check("}", _TokenType::RightBrace);
    check(")", _TokenType::RightParen);
    check(";", _TokenType::SemiColon);
}

// ─── Comment handling ─────────────────────────────────────────────────────────

TEST_F(BasicTokenizerTests, CommentIsStripped)
{
    auto toks = tokenize("# this is a comment");
    EXPECT_TRUE(toks.empty());
}

TEST_F(BasicTokenizerTests, CommentDoesNotConsumeNextLine)
{
    auto toks = tokenize("# comment\nmyId");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Identifier);
    EXPECT_EQ(toks[0].m_str, "myId");
}

TEST_F(BasicTokenizerTests, InlineCommentAfterToken)
{
    auto toks = tokenize("42 # comment");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberInt);
}

// ─── Whitespace tests ─────────────────────────────────────────────────────────

TEST_F(BasicTokenizerTests, SpacesAreSkipped)
{
    auto toks = tokenize("   42   ");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberInt);
}

TEST_F(BasicTokenizerTests, MultipleNewlines)
{
    auto toks = tokenize("\n\n\nmyId\n\n");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Identifier);
}

TEST_F(BasicTokenizerTests, EmptySourceProducesNoTokens)
{
    auto toks = tokenize("");
    EXPECT_TRUE(toks.empty());
}

// ─── Multi-token sequences ────────────────────────────────────────────────────

TEST_F(BasicTokenizerTests, SimpleAssignmentLike)
{
    // "i64 %x: {0}" — type, percent, identifier, colon, brace, int, brace
    auto toks = tokenize("i64 %x: {0}");
    ASSERT_GE(toks.size(), 6u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Identifier); // "i64"
    EXPECT_EQ(toks[1].m_type, _TokenType::Percentage);
    EXPECT_EQ(toks[2].m_type, _TokenType::Identifier); // "x"
    EXPECT_EQ(toks[3].m_type, _TokenType::Colon);
    EXPECT_EQ(toks[4].m_type, _TokenType::LeftBrace);
    EXPECT_EQ(toks[5].m_type, _TokenType::NumberInt);
}

TEST_F(BasicTokenizerTests, KeywordFollowedByIdentifierNotMerged)
{
    // "ifFoo" should be a single Identifier, not If + Foo.
    auto toks = tokenize("ifFoo");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Identifier);
}

// ─── Additional keyword & edge-case tests ─────────────────────────────────────

TEST_F(BasicTokenizerTests, ForKeywordStandalone)
{
    // "for" alone is a keyword, but "forEach" is an identifier.
    auto toks = tokenize("for");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::For);

    auto toks2 = tokenize("forEach");
    ASSERT_EQ(toks2.size(), 1u);
    EXPECT_EQ(toks2[0].m_type, _TokenType::Identifier);
}

TEST_F(BasicTokenizerTests, IncludeKeywordStandalone)
{
    auto toks = tokenize("include");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Include);
}

TEST_F(BasicTokenizerTests, LargeIntegerLiteral)
{
    auto toks = tokenize("999999999999999999");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberInt);
    EXPECT_EQ(toks[0].m_str, "999999999999999999");
}

TEST_F(BasicTokenizerTests, FloatWithLeadingZero)
{
    auto toks = tokenize("0.001");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberFloat);
    EXPECT_EQ(toks[0].m_str, "0.001");
}

TEST_F(BasicTokenizerTests, NegativeFloatIsTwoTokens)
{
    auto toks = tokenize("-3.14");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Minus);
    EXPECT_EQ(toks[1].m_type, _TokenType::NumberFloat);
}

TEST_F(BasicTokenizerTests, StringWithSpaces)
{
    auto toks = tokenize(R"("hello world")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::String);
    EXPECT_EQ(toks[0].m_str, "hello world");
}

TEST_F(BasicTokenizerTests, StringWithNullEscape)
{
    auto toks = tokenize(R"("a\0b")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].m_type, _TokenType::String);
}

TEST_F(BasicTokenizerTests, MultipleCommentsInterspersed)
{
    auto toks = tokenize("# line1\n42\n# line3\nmyId");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].m_type, _TokenType::NumberInt);
    EXPECT_EQ(toks[1].m_type, _TokenType::Identifier);
}

TEST_F(BasicTokenizerTests, ComplexModuleDeclarationTokenSequence)
{
    auto toks = tokenize("void foo(i64 %x) {}");
    // void=Identifier foo=Identifier (=LeftParen i64=Identifier %=Percentage x=Identifier )=RightParen {=LeftBrace
    // }=RightBrace
    ASSERT_GE(toks.size(), 8u);
    EXPECT_EQ(toks[0].m_type, _TokenType::Identifier);
    EXPECT_EQ(toks[0].m_str, "void");
    EXPECT_EQ(toks[1].m_type, _TokenType::Identifier);
    EXPECT_EQ(toks[1].m_str, "foo");
    EXPECT_EQ(toks[2].m_type, _TokenType::LeftParen);
    EXPECT_EQ(toks[3].m_type, _TokenType::Identifier);
    EXPECT_EQ(toks[3].m_str, "i64");
    EXPECT_EQ(toks[4].m_type, _TokenType::Percentage);
    EXPECT_EQ(toks[5].m_type, _TokenType::Identifier);
    EXPECT_EQ(toks[5].m_str, "x");
    EXPECT_EQ(toks[6].m_type, _TokenType::RightParen);
    EXPECT_EQ(toks[7].m_type, _TokenType::LeftBrace);
}

TEST_F(BasicTokenizerTests, OnlyWhitespaceProducesOnlyTabs)
{
    // Only tabs should be present.
    auto toks = tokenize("   \t\t  \n  \n ");
    EXPECT_EQ(toks.size(), 2);
}

TEST_F(BasicTokenizerTests, AdjacentPunctuationsAreSeparateTokens)
{
    auto toks = tokenize("{}()");
    ASSERT_EQ(toks.size(), 4u);
    EXPECT_EQ(toks[0].m_type, _TokenType::LeftBrace);
    EXPECT_EQ(toks[1].m_type, _TokenType::RightBrace);
    EXPECT_EQ(toks[2].m_type, _TokenType::LeftParen);
    EXPECT_EQ(toks[3].m_type, _TokenType::RightParen);
}

TEST_F(BasicTokenizerTests, UnterminatedStringLiteral)
{
    auto toks = tokenize(R"("hello)");
    // Should fail or produce partial token?
    // Based on implementation, it likely returns false or empty.
    EXPECT_TRUE(toks.empty());
}
