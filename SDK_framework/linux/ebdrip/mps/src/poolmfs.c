/* impl.c.poolmfs: MANUAL FIXED SMALL UNIT POOL
 *
 * $Id: poolmfs.c,v 1.38.8.1.1.1 2013/12/19 11:27:04 anon Exp $
 * $HopeName: MMsrc!poolmfs.c(EBDSDK_P.1) $
 * Copyright (c) 2001 Ravenbrook Limited.
 * Copyright (C) 2014 Global Graphics Software Ltd. All rights reserved.
 *
 * This is the implementation of the MFS pool class.
 *
 * DESIGN
 *
 * .design.misplaced: This design is misplaced, it should be in a
 * separate document.
 *
 * MFS operates in a very simple manner: each region allocated from 
 * the arena is divided into units.  Free units are kept on a linked 
 * list using a header stored in the unit itself.  The linked list is 
 * not ordered; allocation and deallocation simply pop and push from
 * the head of the list.  This is fast, but successive allocations might 
 * have poor locality if previous successive frees did.
 *
 * .restriction: This pool cannot allocate from the arena control
 * pool (as the control pool is an instance of PoolClassMV and MV uses
 * MFS in its implementation), nor can it allocate sub-pools, as that
 * causes allocation in the control pool.
 *
 * Notes
 *
 * .freelist.fragments: The simple freelist policy might lead to poor
 * locality of allocation if the list gets fragmented.
 * 
 * .buffer.not: This pool doesn't support fast cache allocation, which
 * is a shame.
 */


#include "poolmfs.h"
#include "mpm.h"

SRCID(poolmfs, "$Id: poolmfs.c,v 1.38.8.1.1.1 2013/12/19 11:27:04 anon Exp $");


/* ROUND -- Round up
 *
 *  Rounds n up to the nearest multiple of unit.
 */

#define ROUND(unit, n)  ((n)+(unit)-1 - ((n)+(unit)-1)%(unit))


#define PoolPoolMFS(pool)       PARENT(MFSStruct, poolStruct, pool)


/* HeaderStruct -- Freelist structure */

typedef struct MFSHeaderStruct {
  struct MFSHeaderStruct *next;
} HeaderStruct, *Header;



#define UNIT_MIN        sizeof(HeaderStruct)

MFSInfo MFSGetInfo(void)
{
  static const struct MFSInfoStruct info =
  {
    /* unitSizeMin */   UNIT_MIN
  };
  return &info;
}


Pool (MFSPool)(MFS mfs)
{
  AVERT(MFS, mfs);
  return &mfs->poolStruct;
}


static Res MFSInit(Pool pool, va_list arg)
{
  Size extendBy, unitSize;
  MFS mfs;
  Arena arena;

  AVER(pool != NULL);

  extendBy = va_arg(arg, Size);
  unitSize = va_arg(arg, Size);

  AVER(unitSize >= UNIT_MIN);
  AVER(extendBy >= unitSize);
  
  mfs = PoolPoolMFS(pool);
  arena = PoolArena(pool);

  mfs->unroundedUnitSize = unitSize;

  unitSize = SizeAlignUp(unitSize, MPS_PF_ALIGN);
  extendBy = SizeAlignUp(extendBy, ArenaAlign(arena));

  mfs->extendBy = extendBy;
  mfs->unitSize = unitSize;
  mfs->unitsPerExtent = extendBy/unitSize;
  mfs->freeList = NULL;
  mfs->tractList = NULL;
  mfs->sig = MFSSig;

  AVERT(MFS, mfs);
  EVENT_PPP(PoolInit, pool, arena, ClassOfPool(pool));
  return ResOK;
}


static void MFSFinish(Pool pool)
{
  Tract tract;
  MFS mfs;

  AVERT(Pool, pool);
  mfs = PoolPoolMFS(pool);
  AVERT(MFS, mfs);

  tract = mfs->tractList;
  while (tract != NULL) {
    Tract nextTract = (Tract)TractP(tract);   /* .tract.chain */
    ArenaFree(TractBase(tract), mfs->extendBy, pool);
    tract = nextTract;
  }

  mfs->sig = SigInvalid;
}


static void MFSClear(Pool pool)
{
  Tract tract;
  MFS mfs;

  AVERT(Pool, pool);
  mfs = PoolPoolMFS(pool);
  AVERT(MFS, mfs);

  tract = mfs->tractList;
  while (tract != NULL) {
    Tract nextTract = (Tract)TractP(tract);   /* .tract.chain */
    ArenaFree(TractBase(tract), mfs->extendBy, pool);
    tract = nextTract;
  }
  mfs->freeList = NULL;
  mfs->tractList = NULL;
}


/*  == Allocate ==
 *
 *  Allocation simply involves taking a unit from the front of the freelist
 *  and returning it.  If there are none, a new region is allocated from the 
 *  arena.
 */

static Res MFSAlloc(Addr *pReturn, Pool pool, Size size,
                    Bool withReservoirPermit, DebugInfo info)
{
  Header f;
  Res res;
  MFS mfs;

  AVERT(Pool, pool);
  mfs = PoolPoolMFS(pool);
  AVERT(MFS, mfs);

  AVER(pReturn != NULL);
  AVER(size == mfs->unroundedUnitSize);
  AVER(BoolCheck(withReservoirPermit));
  UNUSED(info);

  f = mfs->freeList;

  /* If the free list is empty then extend the pool with a new region. */

  if(f == NULL)
  {
    Tract tract;
    Word i, unitsPerExtent;
    Size unitSize;
    Addr base;
    Header header = NULL, next;

    /* Create a new region and attach it to the pool. */
    res = ArenaAlloc(&base, SegPrefDefault(), mfs->extendBy, pool,
                     withReservoirPermit);
    if(res != ResOK)
      return res;

    /* .tract.chain: chain first tracts through TractP(tract) */
    tract = TractOfBaseAddr(PoolArena(pool), base);
    TractSetP(tract, (void *)mfs->tractList);
    mfs->tractList = tract;

    /* Sew together all the new empty units in the region, working down */
    /* from the top so that they are in ascending order of address on the */
    /* free list. */

    unitsPerExtent = mfs->unitsPerExtent;
    unitSize = mfs->unitSize;
    next = NULL;

#define SUB(b, s, i)    ((Header)AddrAdd(b, (s)*(i)))

    for(i=0; i<unitsPerExtent; ++i)
    {
      header = SUB(base, unitSize, unitsPerExtent-i - 1);
      AVER(AddrIsAligned(header, pool->alignment));
      AVER(AddrAdd((Addr)header, unitSize) <= AddrAdd(base, mfs->extendBy));
      header->next = next;
      next = header;
    }

#undef SUB

    /* The first unit in the region is now the head of the new free list. */
    f = header;
  }

  AVER(f != NULL);

  /* Detach the first free unit from the free list and return its address. */

  mfs->freeList = f->next;

  *pReturn = (Addr)f;
  return ResOK;
}


/*  == Free ==
 *
 *  Freeing a unit simply involves pushing it onto the front of the
 *  freelist.
 */

static void MFSFree(Pool pool, Addr old, Size size)
{
  Header h;
  MFS mfs;

  AVERT(Pool, pool);
  mfs = PoolPoolMFS(pool);
  AVERT(MFS, mfs);
  AVER(PoolHasAddr(pool, old));
  AVER(size == mfs->unroundedUnitSize);

  /* .freelist.fragments */
  h = (Header)old;
  h->next = mfs->freeList;
  mfs->freeList = h;
}


Size MFSFreeSize(Pool pool)
{
  Count free = 0;
  Header p;
  MFS mfs;

  AVERT(Pool, pool);
  mfs = PoolPoolMFS(pool);
  AVERT(MFS, mfs);

  for ( p = mfs->freeList; p != NULL; p = p->next )
    ++free;
  return free * mfs->unroundedUnitSize;
}


static Res MFSDescribe(Pool pool, mps_lib_FILE *stream)
{
  MFS mfs;
  Res res;

  AVERT(Pool, pool);
  mfs = PoolPoolMFS(pool);
  AVERT(MFS, mfs);

  AVER(stream != NULL);

  res = WriteF(stream,
               "  unrounded unit size $W\n", (WriteFW)mfs->unroundedUnitSize,
               "  unit size $W\n",           (WriteFW)mfs->unitSize,
               "  extent size $W\n",         (WriteFW)mfs->extendBy,
               "  units per extent $U\n",    (WriteFU)mfs->unitsPerExtent,
               "  free list begins at $P\n", (WriteFP)mfs->freeList,
               "  tract list begin at $P\n", (WriteFP)mfs->tractList,
               NULL);
  if(res != ResOK) return res;

  return ResOK;
}


DEFINE_POOL_CLASS(MFSPoolClass, this)
{
  INHERIT_CLASS(this, AbstractAllocFreePoolClass);
  this->name = "MFS";
  this->size = sizeof(MFSStruct);
  this->offset = offsetof(MFSStruct, poolStruct);
  this->init = MFSInit;
  this->finish = MFSFinish;
  this->clear = MFSClear;
  this->alloc = MFSAlloc;
  this->free = MFSFree;
  this->describe = MFSDescribe;
}


PoolClass PoolClassMFS(void)
{
  return EnsureMFSPoolClass();
}


Bool MFSCheck(MFS mfs)
{
  Arena arena;

  CHECKS(MFS, mfs);
  CHECKD(Pool, &mfs->poolStruct);
  CHECKL(mfs->poolStruct.class == EnsureMFSPoolClass());
  CHECKL(mfs->unroundedUnitSize >= UNIT_MIN);
  CHECKL(mfs->extendBy >= UNIT_MIN);
  arena = PoolArena(&mfs->poolStruct);
  CHECKL(SizeIsAligned(mfs->extendBy, ArenaAlign(arena)));
  CHECKL(SizeAlignUp(mfs->unroundedUnitSize, mfs->poolStruct.alignment) ==
         mfs->unitSize);
  CHECKL(mfs->unitsPerExtent == mfs->extendBy/mfs->unitSize);
  if(mfs->tractList != NULL) {
    CHECKL(TractCheck(mfs->tractList));
  }
  return TRUE;
}
