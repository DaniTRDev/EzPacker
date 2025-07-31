#ifndef EZPACKER_RULE_H
#define EZPACKER_RULE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodes.h"
#include "IParsingContext.h"
#include "Tokenizer/ITokenizer.h"

/**
 * Class that defines how to build an AST node given a list of tokens. It's fully composable and allos joining multiple
 * subrules. std::enable_shared_from_this<Rule> is used so this class can return shared_ptr from this without breaking
 * or invoking UB.
 */
class Rule : public std::enable_shared_from_this<Rule>
{
  public:
    /**
     * Function used to match certain criteria (depending on the rule). Returns true
     * if succeeded and if there was any node to be pushed, it will be inside out.
     * @param parser
     * @param out
     */
    using Matcher = std::function<bool(IParsingContext &parser, const std::shared_ptr<AstNode> &out)>;

    /**
     * Function called, if set, when a rule matches. Used to apply more transformations / checks to the input. If
     * it failed it returns false. If callback is called and returns false, the rule also returns false.
     * @param parser
     * @param out
     */
    using MatchedCallback = std::function<bool(IParsingContext &parser, const std::shared_ptr<AstNode> &out)>;

    /**
     * Tries to match this rule with the tokens of the context and returns the resulting nodes. Depending on if
     * m_encapsulate is set to true or not, m_result will be pushed as a child of out or m_result's children will be
     * pushed straight into out.
     * @param ctx
     * @param out
     * @return bool
     */
    bool match(class IParsingContext &ctx, const std::shared_ptr<AstNode> &out);

    /**
     * Creates an empty ParseRule.
     * @param matcher
     * @return std::shared_ptr<ParseRule>
     */
    static std::shared_ptr<Rule> create(const Matcher &matcher);

    /**
     * Encapsulates the result of this rule into the given builder's node. Used to have the result of a rule on its
     * own self-contained node, that will be pushed as a child of out when match finishes.
     * @param builder
     * @return std::shared_ptr<Rule>
     */
    std::shared_ptr<Rule> encapsulate(AstNodeBuilder *builder);

    /**
     * Sets the callback for when the rule is matched. Will replace previous callback.
     * @param callback
     * @return std::shared_ptr<ParseRule>
     */
    std::shared_ptr<Rule> then(const MatchedCallback &callback);

    /**
     * Executes given rule ('rule'), only if this one is matched. Will replace previous callback.
     * @param rule
     * @return std::shared_ptr<Rule>
     */
    std::shared_ptr<Rule> then(const std::shared_ptr<Rule> &rule);

  private:
    /**
     * Not used, put here to enforce Singleton design pattern.
     */
    Rule();

    /**
     * Creates the rule with the given matcher. Constructor is private to enforce the use of a Singleton design pattern.
     * m_nodeBuilder is set by default to &AstNode::Null(). If a node needs to be encapsulated into its own node, the
     * use of "encapsulate" will set a new value for m_nodeBuilder.
     * @param matcher
     */
    Rule(const Matcher &matcher);

  private:
    AstNodeBuilder *m_builder; // Used when the result needs to be encapsulated on its own node that will
                               // later be pushed into its parent node.
    bool m_encapsulate;
    MatchedCallback m_callback;
    Matcher m_matcher;
};

#endif // EZPACKER_RULE_H
