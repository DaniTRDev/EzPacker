#ifndef EZPACKER_NORMALIZEDMODULE_H
#define EZPACKER_NORMALIZEDMODULE_H

#include "EzFrontendCommon.h"
#include "InstructionNormalizer.h"
#include "OperandNormalizer.h"

struct NormalizedModule
{
    std::shared_ptr<NormalizedSymbol> m_symbol;
    std::shared_ptr<NormalizedType> m_returnType; // TODO
    std::vector<std::shared_ptr<NormalizedVirtualVariable>> m_parameters;
    std::vector<std::shared_ptr<NormalizedInstruction>> m_instructions;
};

class ModuleNormalizer : public INormalizer<NormalizedModule>
{
  public:
    /**
     * Tries to normalize the given module. It assumes that given node is valid and its type is AstType::Module.
     * @param node
     * @param context
     * @return std::shared_ptr<NormalizedModule>
     */
    std::shared_ptr<NormalizedModule> normalizeNode(std::shared_ptr<Ast> node,
                                                    const std::shared_ptr<NormalizerContext> &context) override;

  private:
    /**
     * Normalizes given module's header (name and parameters) and returns true if succeeded. It will also return in
     * bodyStart the id of the first node of the body.
     * @param bodyStart
     * @param node
     * @param module
     * @param context
     * @return bool
     */
    bool normalizeHeader(size_t &bodyStart, std::shared_ptr<Ast> node, std::shared_ptr<NormalizedModule> module,
                         const std::shared_ptr<NormalizerContext> &context);

    /**
     * Normalizes given module's body (instructions) and returns true if succeeded. It will also return the first node
     * that is not part of the body (.end).
     * @param bodyStart The id of the first child of the body.
     * @param node
     * @param module
     * @param context
     * @return bool
     */
    bool normalizeBody(size_t &bodyEnd, size_t bodyStart, std::shared_ptr<Ast> node,
                       std::shared_ptr<NormalizedModule> module, const std::shared_ptr<NormalizerContext> &context);
};

#endif // EZPACKER_NORMALIZEDMODULE_H
