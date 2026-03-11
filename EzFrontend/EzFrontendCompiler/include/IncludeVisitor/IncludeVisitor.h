/**
 * @file IncludeVisitor.h
 * @brief AST visitor for resolving include directives.
 *
 * The IncludeVisitor traverses the Abstract Syntax Tree (AST) specifically looking for
 * `IncludeAstNode`s. Its primary purpose is to identify file inclusions declared in the
 * source code so that the compiler driver can load and process the included files.
 *
 * This visitor is typically run during the initial phases of compilation (e.g., after parsing)
 * to build the dependency graph of source files.
 */
#ifndef EZPACKER_INCLUDEVISITOR_H
#define EZPACKER_INCLUDEVISITOR_H

#include "EzFrontendCompilerCommon.h"

/**
 * @class IncludeVisitor
 * @brief Collects file paths from include directives in the AST.
 *
 * This visitor implements the `AstNodeVisitor` interface to traverse the AST. When it encounters
 * an `IncludeAstNode`, it extracts the included file path and stores it. The set of discovered
 * inclusions can then be retrieved by the compiler driver to schedule further compilation units.
 */
class IncludeVisitor : public AstNodeVisitor
{
  public:
    /**
     * @brief Visits an IncludeAstNode to extract the included file path.
     *
     * This method is called by the AST traversal mechanism when an `IncludeAstNode` is encountered.
     * It resolves the include path and adds it to the internal set of discovered inclusions.
     *
     * @param include Pointer to the `IncludeAstNode` being visited.
     * @return `true` if the visit was successful and traversal should continue; `false` otherwise.
     */
    bool visit(IncludeAstNode *include) override;

    /**
     * @brief Retrieves the set of files discovered by this visitor.
     *
     * Populates the provided destination set with the paths of all files found in include directives
     * during the AST traversal.
     *
     * @param[out] dest The set to be populated with the discovered file paths.
     * @return A const reference to the populated set (same as `dest`).
     */
    const std::set<std::string_view> &getInclusions(std::set<std::string_view> &dest);

  private:
    /**
     * @brief Internal storage for discovered include paths.
     *
     * Stores the paths as string views. Note that the lifetime of the string views must be managed
     * carefully, typically ensuring they point to strings in a persistent string pool.
     */
    std::set<std::string_view> m_discoveredInclusions;
};

#endif // EZPACKER_INCLUDEVISITOR_H
