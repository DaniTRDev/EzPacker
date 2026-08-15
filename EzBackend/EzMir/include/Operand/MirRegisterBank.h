#ifndef EZPACKER_MIRREGISTERBANK_H
#define EZPACKER_MIRREGISTERBANK_H

#include "EzMirCommon.h"
#include "MirRegisterClass.h"

/**
 * This class represents a bank of register the HW has. For example: GPR, FPR, SSE, AVX, ... Each bank might contain 1
 * or more register classes.
 *
 * Important note, IDs are unique within each bank and shared across classes. This means that register ID 1 from bank 1
 * is different to register ID 1 from bank 2; AND register ID 1, with class 1 is different of register ID 1 with
 * class 2.
 */
class MirRegisterBank
{
  public:
    /**
     * Creates the bank with the given name and allocator.
     */
    MirRegisterBank(const char *name, std::pmr::memory_resource *alloc);

    /**
     * Tries to add a class to the bank. If the class already existed, it returns false and it won't be inserted.
     * Returns true other ways.
     */
    bool addClass(const std::string_view &name, MirRegisterClass *_class);

    /**
     * Returns the name of the bank.
     */
    const char *getName() const;

    /**
     * Returns the class that has the same name as the one given. If no class matches, nullptr is returned.
     */
    MirRegisterClass *getClass(const std::string_view &name) const;

    /**
     * Returns the map which contains all the register classes of this bank.
     */
    const std::pmr::unordered_map<std::string_view, MirRegisterClass *> &getClasses() const;

  private:
    const char *m_name;
    std::pmr::unordered_map<std::string_view, MirRegisterClass *> m_classes;
};

#endif // EZPACKER_MIRREGISTERBANK_H