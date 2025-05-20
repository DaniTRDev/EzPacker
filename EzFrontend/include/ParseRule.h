#ifndef EZPACKER_PARSERULE_H
#define EZPACKER_PARSERULE_H

#include "EzFrontendCommon.h"
#include "IParser.h"
#include "ast/TokenTypeNode.h"
#include "tokenizer/ITokenizer.h"

/**
 * Class that defines the behaviour of the parser when certain conditions are fulfilled. It uses enable_shared_from_this
 * to ensure that this class is able to create shared_ptr<ParseRules> of this object and all of them point to the same
 * instance.
 */
class ParseRule : public std::enable_shared_from_this<ParseRule>
{
  public:
    /**
     * Function used to match certain criteria (depending on the rule) that returns an array of nodes. Returns true
     * if succeeded and if there was any node to be pushed, it will be inside out.
     * @param parser
     * @param out
     */
    using Matcher = std::function<bool(IParser &parser, const std::shared_ptr<Ast> &out)>;

    /**
     * Function called, if set, when a rule matches. Used to transform primitive tokens into expressions.
     * If false is returned, rule also fails.
     * @param parser
     * @param out
     */
    using MatchedCallback = std::function<bool(IParser &parser, const std::shared_ptr<Ast> &out)>;

    /**
     * Destroys the object.
     */
    ~ParseRule();

    /**
     * Tries to match this rule with the tokens of the parser and returns the resulting nodes, moving the result to
     * out (not preserving state).
     * @param parser
     * @param out
     * @return bool
     */
    bool matchRet(class IParser &parser, const std::shared_ptr<Ast> &out);

    /**
     * Creates an empty ParseRule.
     * @return std::shared_ptr<ParseRule>
     */
    static std::shared_ptr<ParseRule> create(const Matcher &matcher, std::string_view name);

    /**
     * Maps the returning node into a specific type.
     * @tparam T
     * @tparam ret
     * @tparam enable_if
     * @return
     */
    template <typename T, typename... Args, typename ret = std::shared_ptr<ParseRule>,
              typename enable_if = std::enable_if<std::is_base_of_v<Ast, T>, ret>>
    enable_if::type map(Args &&...args)
    {
        m_mapped = true;
        m_result = std::make_shared<T>(args...);
        
        return shared_from_this();
    }

    /**
     * Sets the then for when the rule is matched.
     * @param callback
     * @return std::shared_ptr<ParseRule>
     */
    std::shared_ptr<ParseRule> then(const MatchedCallback &callback);

  private:
    /**
     * Not used, put here to make sure this class's entry point is create.
     */
    ParseRule();

    /**
     * Creates the rule with the given matcher. Constructor is private to enforce the use of a factory design pattern.
     * @param errorMessage
     * @param matcher
     * @param name
     */
    ParseRule(const Matcher &matcher, std::string_view name);

  private:
    bool m_mapped; // Tells if this rule's result should be mapped into their own node or just pushed into out.
    MatchedCallback m_callback;
    Matcher m_matcher;
    std::string_view m_name; // Used for debugging.
    std::shared_ptr<Ast> m_result;
};

#endif // EZPACKER_PARSERULE_H
