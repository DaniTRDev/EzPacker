#ifndef EZMIR_MIR_CLASS_H
#define EZMIR_MIR_CLASS_H

#include "EzMirCommon.h"

// The offset is calculated after the class has been fully created and the resolution pass has been executed.
struct MirClassField
{
    class MirClass *m_owner{ nullptr };
    class MirType *m_type{ nullptr };
    int64_t m_offset{ -1 };
    size_t m_id{ size_t(-1) };
    std::pmr::string m_name{};
};

// The offset is calculated after the class has been fully created and the resolution pass has been executed.
struct MirClassMethod
{
    class MirClass *m_owner{ nullptr };
    class MirFunction *m_func{ nullptr };
    int64_t m_offset{ -1 };
    size_t m_id{ size_t(-1) };
};

/**
 * Inheritance follows Java's design: Single Inheritance. Meaning if a class EXTENDS another class, this makes
 * creating types easier (copy vTable + fields of super type into derived and add the methods and fields of the derived
 * later).
 *
 * The offsets are calculated in a specific pass after all the classes have been built (ClassOffsetResolverPass).
 */
class MirClass
{
  public:
    /**
     * Creates the class with the given parent class, id, type, name, fields, vTable and sourceRef.
     */
    MirClass(MirClass *parentClass,
             MirId id,
             MirType *type,
             const std::pmr::string &name,
             std::pmr::map<std::pmr::string, MirClassField *> fieldNameToField,
             std::pmr::vector<MirClassField *> fields,
             std::pmr::vector<MirClassMethod *> vTable,
             class SourceReference *sourceRef = nullptr);

    /**
     * Returns the parent class of this class. If this class does not inherit from any class, nullptr is returned.
     */
    MirClass *getParentClass() const;

    /**
     * Returns the field with the given index. If no field exists, a nullptr is returned.
     */
    MirClassField *getFieldById(size_t index) const;

    /**
     * Searches in the field list for the given name. Returns the matching field or nullptr if not found.
     */
    MirClassField *getFieldByName(const std::string_view &name) const;

    /**
     * Returns the method with the given index. If no method exists, a nullptr is returned.
     */
    MirClassMethod *getMethodById(size_t index) const;

    /**
     * Returns the method that matches the full signature: return type, arguments types and name.
     */
    MirClassMethod *
    getMethodBySignature(MirType *returnType, std::vector<MirType *> argsTypes, const std::string_view &name) const;

    /**
     * Returns the ID of this class.
     */
    MirId getId() const;

    /**
     * Returns the type that was created for this class.
     */
    class MirType *getType() const;

    /**
     * Returns the number of fields this class has.
     */
    size_t getFieldCount() const;

    /**
     * Returns the vTable size (number of methods) this class has.
     */
    size_t getVTableSize() const;

    /**
     * Returns the source ref of this class, if set.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Returns the name of the class.
     */
    const std::pmr::string &getName() const;

    /**
     * Returns an immutable list of fields.
     */
    const std::pmr::vector<MirClassField *> &getFields() const;

    /**
     * Returns the VTable for this class.
     */
    const std::pmr::vector<MirClassMethod *> &getVTable() const;

  private:
    MirClass *m_parentClass; // Used for inheritance.
    MirId m_id;
    class MirType *m_type;
    class SourceReference *m_sourceRef;
    std::pmr::string m_name;
    std::pmr::map<std::pmr::string, MirClassField *> m_fieldNameToField; // Fast search by name.
    std::pmr::vector<MirClassField *> m_fields;
    std::pmr::vector<MirClassMethod *> m_vTable;
};

#endif // EZMIR_MIR_CLASS_H
