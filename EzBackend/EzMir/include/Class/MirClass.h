#ifndef EZPACKER_MIRCLASS_H
#define EZPACKER_MIRCLASS_H

#include "EzMirCommon.h"
#include "Function/MirFunction.h"

struct MirClassField
{
    MirType *m_type{ nullptr };
    int64_t m_offset{ -1 }; // Calculated after the class has been fully created.
    std::pmr::string m_name{};
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
     * @param fields
     * @param vTable
     * @param sourceRef
     */
    MirClass(MirClass *parentClass,
             MirId id,
             MirType *type,
             const std::pmr::string &name,
             std::pmr::vector<MirClassField> fields,
             std::pmr::vector<MirFunction *> vTable,
             SourceReference *sourceRef = nullptr);

    /**
     * Returns the parent class of this class. If this class does not inherit from any class, nullptr is returned.
     * @return
     */
    MirClass *getParentClass() const;

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
     * Appends a field to the class.
     * @param type
     * @param name
     */
    void appendField(MirType *type, const std::pmr::string &name);

    /**
     * Appends a method to the class.
     * @param func
     */
    void appendMethod(MirFunction *func);

    /**
     * Returns the name of the class.
     * @return
     */
    const std::pmr::string &getName() const;

    /**
     * Returns an immutable list of fields.
     * @return
     */
    const std::pmr::vector<MirClassField> &getFields() const;

    /**
     * Returns a pointer to the MUTABLE list of fields.
     * @return
     */
    std::pmr::vector<MirClassField> *getFieldsPtr();

    /**
     * Returns the VTable for this class.
     * @return
     */
    const std::pmr::vector<MirFunction *> &getVTable() const;

  private:
    MirClass *m_parentClass; // Used for inheritance.
    MirId m_id;
    MirType *m_type;
    SourceReference *m_sourceRef;
    std::pmr::string m_name;
    std::pmr::vector<MirClassField> m_fields;
    std::pmr::vector<MirFunction *> m_vTable;
};

#endif // EZPACKER_MIRCLASS_H
