/**
 * @file MirType.h
 * @brief Lightweight, self-contained type representation for the MIR layer.
 *
 * `MirType` is intentionally decoupled from semantic-layer type objects. It
 * captures just enough information for MIR construction and later lowering:
 * a kind (`MirTypeKind`), a unique MIR ID, an optional slice of child types,
 * and a debug-friendly name.
 *
 * The currently supported kinds are:
 *   - `Integer`
 *   - `FloatingPoint`
 *   - `Pointer`
 *   - `Array`
 *   - `Void`
 *
 * `subTypes` is used only when a kind needs extra type structure. In the
 * current implementation this is primarily intended for compound/container
 * forms such as arrays or pointer targets; primitive and `Void` types usually
 * leave it null.
 *
 * This class should remain trivially destructible so it can live in arena
 * storage without custom lifetime management.
 */
#ifndef EZPACKER_MIRTYPE_H
#define EZPACKER_MIRTYPE_H

#include "EzMirCommon.h"

enum class MirTypeKind
{
    Invalid = 0,
    Integer,
    FloatingPoint,
    Pointer,
    Array, // An array of other types.
    Void
};

/**
 * Represents one MIR type record stored in the emitter context's type pool.
 *
 * The object is a plain descriptor. It performs no semantic validation on its
 * own and does not resolve names; callers are expected to build valid type
 * graphs before referencing them from functions or instructions.
 */
class MirType
{
  public:
    /**
     * Constructs a MIR type descriptor.
     *
     * @param kind     High-level classification of the type.
     * @param id       Unique MIR ID assigned by the context.
     * @param subTypes Optional child-type slice used by compound kinds.
     * @param name     Human-readable type name kept for diagnostics/debugging.
     */
    MirType(MirTypeKind kind,
            size_t id,
            size_t totalSize,
            TypedPoolLinkedList<MirType> *subTypes,
            const std::string_view &name);

    /**
     * Returns the high-level kind of this type.
     */
    MirTypeKind getKind() const;

    /**
     * Returns the unique MIR ID of this type.
     */
    size_t getId() const;

    /**
     * Returns the total size in bytes of this type.
     * @return
     */
    size_t getTotalSizeInBytes() const;

    /**
     * Returns the child-type slice for compound kinds, or `nullptr` when this
     * type has no subordinate types.
     */
    TypedPoolLinkedList<MirType> *getSubTypes() const;

    /**
     * Returns the human-readable name associated with this type.
     *
     * This name is informational; identity is determined by `getId()`.
     */
    const std::string_view &getName() const;

  private:
    MirTypeKind m_kind;                       // High-level classification of the type.
    size_t m_id;                              // Unique MIR identifier for this type.
    size_t m_totalSizeInBytes;                // Total size in bytes of this type.
    TypedPoolLinkedList<MirType> *m_subTypes; // Optional child types for compound kinds.
    std::string_view m_name;                  // Debug/diagnostic name.
};

#endif // EZPACKER_MIRTYPE_H
