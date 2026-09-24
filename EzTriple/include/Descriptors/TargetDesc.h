#ifndef EZTRIPLE_TARGET_DESC_H
#define EZTRIPLE_TARGET_DESC_H

#include "EzTripleCommon.h"
#include "Operand/MirRegisterReference.h"
#include "Descriptors/TargetExtensionSet.h"

#include <memory>

class GenericCodeEmitter;
class TargetRelocationResolver;

/**
 * Interface used to store target-dependent information (CPU-level).
 *
 * Ex: TargetDesc = AMD64, TargetBinaryDesc = AMD64_Windows | AMD64_Linux.
 * The calling convention is also dependant on the target binary desc (AMD64_Windows_Windows | AMD64_Linux_SysV)
 */

class TargetDesc
{
  public:
    virtual ~TargetDesc() = default;

    /**
     * Returns the name of the target.
     * @return
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the frame lowerer for this target.
     */
    virtual class MirFrameLowerer *getFrameLowerer() = 0;

    /**
     * Returns the instruction selector required for this target.
     */
    virtual class MirInstructionSelector *getInstructionSelector() = 0;

    /**
     * Returns the addressing mode matcher required for this target, or nullptr if none.
     */
    virtual class MirAddressingModeMatcher *getAddressingModeMatcher() { return nullptr; }

    /**
     * Returns the general-purpose integer register class for this target, or nullptr if unspecified.
     */
    virtual class MirRegisterClass *getGprClass() { return nullptr; }

    /**
     * Returns the legalizer needed for this target.
     */
    virtual class MirLegalizer *getLegalizer() = 0;

    /**
     * Returns the LegalizerInfo containing table-driven legality definitions for this target.
     */
    virtual class LegalizerInfo *getLegalizerInfo() = 0;

    /**
     * Returns the register allocator needed for this target.
     */
    virtual class MirRegisterAllocator *getRegisterAllocator() = 0;

    /**
     * Returns the displacement's type of a memory operand.
     */
    virtual class MirType *getMemOperandDisplacementType() = 0;

    /**
     * Returns a reference to the target's instruction pointer.
     */
    virtual MirRegisterRef getInstructionPtrReg() const = 0;

    /**
     * Returns the size in bytes of a standard stack slot (e.g., 8 for 64-bit targets, 4 for 32-bit).
     */
    virtual size_t getStackSlotSize() const = 0;

    /**
     * Initializes the target descriptor. This is the function that starts creating everything needed by the descriptor.
     */
    virtual void initialize() = 0;

    /**
     * Returns the name of the libcall symbol pointed by the given libcall symbol Id.
     */
    virtual std::string_view getLibcallStr(uint8_t symId) = 0;

    /**
     * Returns a list with the available binary descriptors.
     */
    virtual const std::pmr::vector<class TargetBinaryDesc *> &getAvailableBinaryDescriptors() = 0;

    /**
     * Returns a list with the available calling conventions defined for this target.
     */
    virtual const std::pmr::vector<class CallingConvDesc *> &getAvailableCallingConventions() = 0;

    /**
     * Returns a list with the available register banks for this target.
     */
    virtual const std::pmr::vector<class MirRegisterBank *> &getAvailableRegisterBanks() = 0;

    /**
     * Creates and registers a new register bank owned by this target.
     *
     * The default implementation returns nullptr so existing targets remain valid; generated
     * target descriptors implement it so declarative bank setup does not need to reach into
     * target internals. The returned bank is exposed through getAvailableRegisterBanks().
     */
    virtual class MirRegisterBank *createRegisterBank(const char * /*name*/) { return nullptr; }

    /**
     * Creates the target's machine code emitter.
     *
     * Returns nullptr when the target does not provide an emitter.
     */
    virtual std::unique_ptr<GenericCodeEmitter> createCodeEmitter();

    /**
     * Returns the target's relocation resolver, or nullptr if the target does not support
     * in-place relocation patching.
     */
    virtual TargetRelocationResolver *getRelocationResolver() { return nullptr; }

    /**
     * Checks if the specified extension is enabled for this target.
     */
    virtual bool hasExtension(std::string_view name) const { return m_extensions.has(name); }

    /**
     * Checks if an extension is supported and recognized by this target.
     */
    virtual bool isExtensionSupported(std::string_view name) const { return m_extensions.isSupported(name); }

    /**
     * Enables or disables an extension on this target.
     */
    virtual bool setExtension(std::string_view name, bool enabled = true) { return m_extensions.set(name, enabled); }

    /**
     * Applies a list of feature modifiers (e.g. {"+avx", "-sse"}).
     */
    virtual bool applyFeatures(const std::vector<std::string> &features, std::string *outError = nullptr)
    {
        return m_extensions.applyFeatures(features, outError);
    }

    /**
     * Applies a comma-separated feature string (e.g. "+avx2,-sse4.1").
     */
    virtual bool applyFeatureString(std::string_view featureString, std::string *outError = nullptr)
    {
        return m_extensions.applyFeatureString(featureString, outError);
    }

    /**
     * Returns the target's extension set.
     */
    const TargetExtensionSet &getExtensionSet() const { return m_extensions; }
    TargetExtensionSet &getExtensionSet() { return m_extensions; }

  protected:
    TargetExtensionSet m_extensions;
};

#endif // EZTRIPLE_TARGET_DESC_H
