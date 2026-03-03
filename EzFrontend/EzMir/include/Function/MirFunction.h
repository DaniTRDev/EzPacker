#ifndef EZPACKER_MIRFUNCTION_H
#define EZPACKER_MIRFUNCTION_H

#include "EzMirCommon.h"
#include "MirBlock.h"

class MirFunction
{
  public:
    /**
     *
     * @param entryPoint
     * @param id
     * @param returnTypeId
     * @param blocks
     * @param parameters
     */
    MirFunction(MirBlock *entryPoint,
                size_t id,
                size_t returnTypeId,
                TypedPoolSlice<MirBlock> *blocks,
                TypedPoolSlice<MirOperand> *parameters);

    /**
     * Returns the entry point block of this function.
     * @return  MirBlock *
     */
    MirBlock *getEntryPoint();

    /**
     * Returns the ID that represents this function in the current compilation.
     * @return size_t
     */
    size_t getId();

    /**
     * Returns the return type of this function.
     * @return size_t
     */
    size_t getReturnTypeId();

    /**
     * Returns the blocks owned by this function.
     * @return TypedPoolSlice<MirBlock> *
     */
    TypedPoolSlice<MirBlock> *getBlocks();

    /**
     * Returns the linked list of parameters used by this function.
     * @return TypedPoolSlice *
     */
    TypedPoolSlice<MirOperand> *getParameters();

  private:
    MirBlock *m_entryPoint;
    size_t m_id;
    size_t m_returnTypeId;
    TypedPoolSlice<MirBlock> *m_blocks; // blocks owned by this function.
    TypedPoolSlice<MirOperand> *m_parameters;
};

#endif // EZPACKER_MIRFUNCTION_H
