#ifndef EZPACKER_X64TYPELEGALIZER_H
#define EZPACKER_X64TYPELEGALIZER_H

#include "EzTargetEmitter.h"

class x64TypeLegalizer
{
  public:
    /**
     * Returns the action list used to legalize a type in x64.
     * @return
     */
    std::shared_ptr<LegalizerActionList> getActionList(MirTypes *types);

    /**
     * Returns the handler list used to legalize a type in x64.
     * @return
     */
    std::shared_ptr<LegalizerHandlerList> getHandlerList(MirTypes *types);
};

#endif // EZPACKER_X64TYPELEGALIZER_H
