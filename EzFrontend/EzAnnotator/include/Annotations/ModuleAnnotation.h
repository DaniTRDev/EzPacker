#ifndef EZPACKER_MODULEANNOTATION_H
#define EZPACKER_MODULEANNOTATION_H

#include "EzAnnotatorCommon.h"
#include "Annotations/TypeAbleAnnotation.h"
#include "Annotations/SymbolAbleAnnotation.h"
#include "ScopedTable/ScopedSymbol.h"
#include "ScopedTable/ScopedType.h"

/**
 * This annotation is quite complex and contains a lot of information that is the result of a complex-chained analysis:
 *  - Return type of the module (TypeAbleAnnotation).
 *  - Symbol of the module (SymbolAbleAnnotation).
 *  - A list with the IDs of the symbols created to reference arguments within the module. There's no need to store
 *    anything more of the arguments because with the symbol id we can also retrieve their type and other information
 *    if it was needed.
 */
class ModuleAnnotation : public TypeAbleAnnotation, public SymbolAbleAnnotation
{
  public:
    /**
     * Creates the annotation with the given symbol and type ID
     * @param symbolId
     * @param typeId
     */
    ModuleAnnotation(size_t symbolId, size_t typeId);

    /**
     * Returns "Module".
     * @return const char*
     */
    const char *getAnnotationName() override;

    /**
     * Returns the symbolId of the argument at given argId.
     * @param argId
     * @return size_t
     */
    size_t getArgumentSymbolId(size_t argId) const;

    /**
     * Push an argument with the given symbol ID.
     * @param symbolId
     */
    void pushArgument(size_t symbolId);

    /**
     * Returns the IDs of the symbols of arguments.
     * @return const std::vector<size_t> &
     */
    const std::vector<size_t> &getArgumentSymbolIds() const;

  private:
    std::vector<size_t> m_argumentSymbolIds; // SymbolIDs of arguments.
};

#endif // EZPACKER_MODULEANNOTATION_H
