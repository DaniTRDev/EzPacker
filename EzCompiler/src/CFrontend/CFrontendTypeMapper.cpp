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
        case TY_ARRAY:
        case TY_VLA:
            if (ty->base)
                return tt->getPtr(mapType(tt, ty->base));
            return tt->getPtr(tt->_void());
        default:
            if (ty->size <= 1)
                return tt->i8();
            if (ty->size <= 2)
                return tt->i16();
            if (ty->size <= 4)
                return tt->i32();
            return tt->i64();
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

