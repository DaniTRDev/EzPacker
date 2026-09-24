#ifndef EZMIR_MIR_REGISTERBANK_H
#define EZMIR_MIR_REGISTERBANK_H

#include "EzMirCommon.h"

/**
 * Hardware register bank container (e.g. GPR, FPR, SSE/AVX vector banks).
 *
 * Groups related register classes that share a common underlying physical register set.
 * Register IDs are unique within each bank and shared across classes within the same bank.
 */
class MirRegisterBank
{
  public:
    /**
     * Constructs a register bank with the specified name and allocator.
     */
    MirRegisterBank(const char *name, std::pmr::memory_resource *alloc);

    /**
     * Registers a new class into the bank under the specified name.
     * Returns true if successfully inserted, false if a class with the same name already exists.
     */
    bool addClass(const std::string_view &name, class MirRegisterClass *_class);

    /**
     * Returns the name of the register bank.
     */
    const char *getName() const;

    /**
     * Retrieves a register class by name, or nullptr if no matching class exists.
     */
    class MirRegisterClass *getClass(const std::string_view &name) const;

    /**
     * Returns the map of all register classes contained within this bank.
     */
    const std::pmr::unordered_map<std::string_view, class MirRegisterClass *> &getClasses() const;

  private:
    /**
     * Identifier name of the register bank (e.g., "GPR", "FPR").
     */
    const char *m_name;

    /**
     * Collection of register classes registered in this bank.
     */
    std::pmr::unordered_map<std::string_view, class MirRegisterClass *> m_classes;
};

#endif // EZMIR_MIR_REGISTERBANK_H