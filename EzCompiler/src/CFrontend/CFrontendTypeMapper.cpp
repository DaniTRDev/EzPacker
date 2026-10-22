#include "CFrontend/CFrontendTypeMapper.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"
#include "Block/MirBlock.h"
#include "Instruction/MirInstruction.h"
#include "chibicc.h"

namespace EzCompiler::CFrontend
{

MirType *mapType(MirTypeTable *tt, struct Type *ty)
{
    if (!ty)
        return tt->i32();

    switch (ty->kind)
    {
        case TY_VOID:
            return tt->_void();
        case TY_BOOL:
            return tt->i1();
        case TY_CHAR:
            return tt->i8();
        case TY_SHORT:
            return tt->i16();
        case TY_INT:
        case TY_ENUM:
            return tt->i32();
        case TY_LONG:
            return tt->i64();
        case TY_FLOAT:
            return tt->f32();
        case TY_DOUBLE:
        case TY_LDOUBLE:
            return tt->f64();
        case TY_PTR:
            if (ty->base)
                return tt->getPtr(mapType(tt, ty->base));
            return tt->getPtr(tt->_void());
        case TY_ARRAY:
        case TY_VLA:
            if (ty->base)
            {
                size_t len = (ty->array_len > 0) ? static_cast<size_t>(ty->array_len)
                           : (ty->base->size > 0) ? static_cast<size_t>(ty->size / ty->base->size)
                           : static_cast<size_t>(ty->size);
                return tt->getArray(mapType(tt, ty->base), len > 0 ? len : 1);
            }
            return tt->getArray(tt->i8(), ty->size > 0 ? static_cast<size_t>(ty->size) : 1);
        default:
            if (ty->size <= 1)
                return tt->i8();
            if (ty->size <= 2)
                return tt->i16();
            if (ty->size <= 4)
                return tt->i32();
            if (ty->size <= 8)
                return tt->i64();
            return tt->getArray(tt->i8(), static_cast<size_t>(ty->size));
    }
}

bool isBlockTerminated(const MirBlock *block)
{
    if (!block || block->getInstrCount() == 0)
        return false;
    const MirInstruction *last = block->back();
    return last && bool(last->getFlags() & MirInstructionFlags::IsTerminator);
}

} // namespace EzCompiler::CFrontend
