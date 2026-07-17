#ifndef EZPACKER_MIRCLASS_H
#define EZPACKER_MIRCLASS_H

#include "EzMirCommon.h"
#include "Function/MirFunction.h"
#include "Type/MirType.h"
#include <vector>

// The offset is calculated after the class has been fully created and the resolution pass has been executed.
struct MirClassField
{
    class MirClass *m_owner{ nullptr };
    MirType *m_type{ nullptr };
    int64_t m_offset{ -1 };
    size_t m_id{ size_t(-1) };
    std::pmr::string m_name{};
};

// The offset is calculated after the class has been fully created and the resolution pass has been executed.
struct MirClassMethod
{
    class MirClass *m_owner{ nullptr };
    MirFunction *m_func{ nullptr };
    int64_t m_offset{ -1 };
    size_t m_id{ size_t(-1) };
};

/**
 * Inheritance follows Java's design: Single Inheritance. Meaning if a class EXTENDS another class, this makes
 * creating types easier (copy vTable + fields of super type into derived and add the methods of the derived later).
 *
 * The offset is calculated in the early stages of the MIR using a target layout.
 */
class MirClass
{
  public:
    /**
     * Creates the class with the given parent class, id, type, name, fields, vTable and sourceRef.
     * @param parentClass
     * @param id
     * @param type
     * @param name
     * @param fieldNameToField
     * @param fields
     * @param vTable
     * @param sourceRef
     */
    MirClass(MirClass *parentClass,
             MirId id,
             MirType *type,
             const std::pmr::string &name,
             std::pmr::map<std::pmr::string, MirClassField *> fieldNameToField,
             std::pmr::vector<MirClassField *> fields,
             std::pmr::vector<MirClassMethod *> vTable,
             SourceReference *sourceRef = nullptr);

    /**
     * Returns the parent class of this class. If this class does not inherit from any class, nullptr is returned.
     * @return
     */
    MirClass *getParentClass() const;

    /**
     * Returns the field with the given index. If no field exists, a nullptr is returned.
     * @param index
     * @return MirClassField*
     */
    MirClassField *getFieldById(size_t index) const;

    /**
     * Searches in the field list for the given name. Returns the matching field or nullptr if not found.
     * @param name
     * @return
     */
    MirClassField *getFieldByName(const std::string_view &name) const;

    /**
     * Returns the method with the given index. If no method exists, a nullptr is returned.
     * @param index
     * @return
     */
    MirClassMethod *getMethodById(size_t index) const;

    /**
     * Returns the method that matches the full signature: return type, arguments types and name.
     * @param returnType
     * @return
     */
    MirClassMethod *
    getMethodBySignature(MirType *returnType, std::vector<MirType *> argsTypes, const std::string_view &name) const;

    /**
     * Returns the ID of this class.
     * @return
     */
    MirId getId() const;

    /**
     * Returns the type that was created for this class.
     * @return
     */
    MirType *getType() const;

    /**
     * Returns the source ref of this class, if set.
     * @return
     */
    SourceReference *getSourceRef() const;

    /**
     * Returns the name of the class.
     * @return
     */
    const std::pmr::string &getName() const;

    /**
     * Returns an immutable list of fields.
     * @return
     */
    const std::pmr::vector<MirClassField *> &getFields() const;

    /**
     * Returns the VTable for this class.
     * @return
     */
    const std::pmr::vector<MirClassMethod *> &getVTable() const;

  private:
    MirClass *m_parentClass; // Used for inheritance.
    MirId m_id;
    MirType *m_type;
    SourceReference *m_sourceRef;
    std::pmr::string m_name;
    std::pmr::map<std::pmr::string, MirClassField *> m_fieldNameToField; // Fast search by name.
    std::pmr::vector<MirClassField *> m_fields;
    std::pmr::vector<MirClassMethod *> m_vTable;
};

#endif // EZPACKER_MIRCLASS_H
