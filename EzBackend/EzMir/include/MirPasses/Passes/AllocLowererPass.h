#ifndef EZPACKER_ALLOCLOWERER_H
#define EZPACKER_ALLOCLOWERER_H

/**
 * This pass makes 2 big things:
 *  - Consumes ALLOC instructions and create a stack frame object. Every reference to the dest register is replaced
 *  by a reference to this object.
 *  - Consumes DALLOC instructions and create a DYNAMIC stack frame object. This is an object whose size is not known
 *  at compile time -> int n; int arr[n].
 *
 *
 */
class AllocLowererPass
{
  public:
  private:
};

#endif // EZPACKER_ALLOCLOWERER_H
