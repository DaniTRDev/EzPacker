/**
 * @file MirGlobalDataEmitter.h
 * @brief Builder API for emitting global/static data entries (.data, .rdata, .bss).
 *
 * `MirGlobalDataEmitter` creates `MirGlobalDataEntry` records backed by memory
 * owned by `MirEmitterContext`. Each entry receives a fresh MIR ID and stores
 * a copied byte buffer inside the context's byte-array pool, so callers may
 * discard the original source data immediately after emission.
 *
 * Passing `nullptr` to `createGlobalData()` produces an entry marked as
 * uninitialized; the backing buffer is still allocated and zero-filled.
 */
#ifndef EZPACKER_MIRDATAEMITTER_H
#define EZPACKER_MIRDATAEMITTER_H

#include "EzMirCommon.h"
#include "MirEmitterContext.h"

struct MirGlobalDataEntry
{
    bool m_isReadOnly;      // True for read-only constants / literal data.
    bool m_uninitialized;   // True when the original payload pointer was null.
    size_t m_entryId;       // Unique MIR ID of this global-data entry.
    size_t m_dataSize;      // Size of the stored byte payload.
    ConstantArray<uint8_t> m_data; // Arena-managed byte storage for the payload.
};

class MirGlobalDataEmitter
{
  public:
    /**
     * Creates an emitter attached to `ctx`.
     *
     * Construction throws if attachment fails.
     */
    MirGlobalDataEmitter(MirEmitterContext *ctx);

    /**
     * Attaches the emitter to a context.
     *
     * @return `false` when `ctx` is null; otherwise `true`.
     */
    bool attachToContext(MirEmitterContext *ctx);

    /**
     * Returns the attached context.
     */
    MirEmitterContext *getContext();

    /**
     * Creates a raw global-data entry.
     *
     * The byte range `[data, data + size)` is copied into context-owned
     * storage. If `data` is `nullptr`, the entry is flagged as uninitialized
     * and the allocated storage is zero-filled.
     *
     * @param data       Source bytes to copy, or `nullptr` for an
     *                   uninitialized/zero-filled entry.
     * @param size       Number of bytes to allocate and store.
     * @param isReadOnly Whether the entry should be treated as read-only.
     */
    MirGlobalDataEntry *
    createGlobalData(const void *data, size_t size, bool isReadOnly = true);

    /**
     * Convenience wrapper that emits a read-only `double` constant.
     */
    MirGlobalDataEntry *createGlobalFloatingPoint(double val);

    /**
     * Convenience wrapper that emits a read-only `uint64_t` constant.
     */
    MirGlobalDataEntry *createGlobalInteger(uint64_t val);

    /**
     * Emits a string as a global byte array.
     *
     * The string contents are copied into context-owned storage. When
     * `includeNullTerminator` is true, one extra `\0` byte is appended after
     * the copied characters.
     */
    MirGlobalDataEntry *
    createGlobalString(const std::string_view &str, bool includeNullTerminator = true, bool isReadOnly = true);

  private:
    MirEmitterContext *m_context;
};

#endif // EZPACKER_MIRDATAEMITTER_H
