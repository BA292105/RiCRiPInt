/* impl.h.boot: BOOTSTRAP ALLOCATOR INTERFACE
 *
 * $Id: boot.h,v 1.3.31.1.1.1 2013/12/19 11:27:06 anon Exp $
 * $HopeName: MMsrc!boot.h(EBDSDK_P.1) $
 * Copyright (c) 2001 Ravenbrook Limited.
 *
 * .overview:  A protocol for allocating memory from a given block.
 */

#ifndef boot_h
#define boot_h

#include "mpmtypes.h"


/* BootBlockStruct -- descriptor of the block to allocate from */

typedef struct BootBlockStruct
{
  Sig sig;
  void *base;
  void *alloc;
  void *limit;
} BootBlockStruct;


extern Res BootBlockInit(BootBlockStruct *boot, void *base, void *limit);
extern void BootBlockFinish(BootBlock boot);
extern Res BootAlloc(void **pReturn, BootBlock boot, size_t size,
                     size_t align);
extern size_t BootAllocated(BootBlock boot);
extern Bool BootBlockCheck(BootBlock boot);


#endif /* boot_h */
