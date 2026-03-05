/**
 * @file MirGlobalDataEmitter.h
 * @brief Builder API for emitting global/static data entries (.data, .rdata, .bss).
 *
 * MirGlobalDataEmitter creates MirGlobalDataEntry objects — each one
 * representing an initialized or uninitialized blob of bytes that lives at
 * a fixed address in the final binary.  Convenience wrappers exist for
 * common cases: 64-bit integers, doubles, and null-terminated strings.
 * The data is copied into an internal arena so the caller may free the
 * source buffer immediately after the call.
 */
#ifndef EZPACKER_MIRDATAEMITTER_H
#define EZPACKER_MIRDATAEMITTER_H

#include "EzMirCommon.h"
#include "MirEmitterContext.h"

struct MirGlobalDataEntry
{
    bool m_isReadOnly;
    bool m_uninitialized;
    size_t m_entryId;
    size_t m_dataSize;
    ConstantArray<uint8_t> m_data;
};

class MirGlobalDataEmitter
{
  public:
    /**
     * Creates the emitter and attaches it to the given context. If there was any error while attaching, an exception is
     * thrown.
     * @param ctx
     */
    MirGlobalDataEmitter(MirEmitterContext *ctx);

    /**
     * Attaches to the given context and returns true if succeeded. If there was any error, an exception is thrown.
     * @param ctx
     * @return bool
     */
    bool attachToContext(MirEmitterContext *ctx);

    /**
     * Returns the context this emitter is attached to.
     * @return MirEmitterContext *
     */
    MirEmitterContext *getContext();
    
    /**
     * Emits an initialized global variable (.data or .rdata section equivalent). Important: Data will be COPIED into
     * the entry. This makes the caller be able to free the data right after the call yet the emitter will still know
     * what the content is. If data is nullptr, the entry will be marked as "uninitialized".
     * @param data
     * @param size
     * @param isReadOnly
     * @return MirGlobalDataEntry *
     */
    MirGlobalDataEntry *
    createGlobalData(const void *data, size_t size, bool isReadOnly = true);

    /**
     * Creates a global variable whose type is double. This is a wrapper around createGlobalData.
     * @param val
     * @return MirGlobalDataEntry *
     */
    MirGlobalDataEntry *createGlobalFloatingPoint(double val);

    /**
     * Creates a global variable whose type is integer. This is a wrapper around createGlobalData.
     * @param val
     * @return MirGlobalDataEntry *
     */
    MirGlobalDataEntry *createGlobalInteger(uint64_t val);

    /**
     * Emits an initialized global string (.data or .rdata section equivalent). Important: Data will be COPIED into
     * the entry. This makes the caller be able to free the data right after the call yet the emitter will still know
     * what the content is.
     *
     * If includeNullTerminator is set, the global string will have a null terminator.
     * @param str
     * @param includeNullTerminator
     * @param isReadOnly
     * @return MirGlobalDataEntry *
     */
    MirGlobalDataEntry *
    createGlobalString(const std::string_view &str, bool includeNullTerminator = true, bool isReadOnly = true);

  private:
    MirEmitterContext *m_context;
};

#endif // EZPACKER_MIRDATAEMITTER_H
