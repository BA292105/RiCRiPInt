/* impl.c.walk: OBJECT WALKER
 *
 * $Id: walk.c,v 1.16.1.1.1.1 2013/12/19 11:27:09 anon Exp $
 * $HopeName: MMsrc!walk.c(EBDSDK_P.1) $
 * Copyright (c) 2001 Ravenbrook Limited.
 * Copyright (C) 2002-2011 Global Graphics Software Ltd. All rights reserved.
 */

#include "walk.h"
#include "mpm.h"
#include "mps.h"

SRCID(walk, "$Id: walk.c,v 1.16.1.1.1.1 2013/12/19 11:27:09 anon Exp $");


/* Heap Walking
 */


Bool FormattedObjectsStepClosureCheck(FormattedObjectsStepClosure c)
{
  CHECKS(FormattedObjectsStepClosure, c);
  CHECKL(FUNCHECK(c->f));
  /* p and s fields are arbitrary and cannot be checked */
  return TRUE;
}


/* ArenaFormattedObjectsWalk -- iterate over all objects
 *
 * So called because it walks all formatted objects in an arena.  */

void ArenaFormattedObjectsWalk(Arena arena, FormattedObjectsStepMethod f,
                               void *p)
{
  Seg seg;
  FormattedObjectsStepClosure c;

  AVERT(Arena, arena);
  AVER(FUNCHECK(f));
  AVER(p != NULL);
  c = p;
  AVERT(FormattedObjectsStepClosure, c);

  if (SegFirst(&seg, arena)) {
    Addr base;
    do {
      Pool pool;
      base = SegBase(seg);
      pool = SegPool(seg);
      if (pool->class->attr & AttrFMT) {
        ShieldExpose(arena, seg);
        PoolWalk(pool, seg, f, p);
        ShieldCover(arena, seg);
      }
    } while(SegNext(&seg, arena, base));
  }
}


/* Root Walking
 *
 * This involves more code than it should. The roots are walked by
 * scanning them. But there's no direct support for invoking the scanner
 * without there being a trace, and there's no direct support for
 * creating a trace without also condemning part of the heap. (@@@@ This
 * looks like a useful canditate for inclusion in the future). For now,
 * the root walker contains its own code for creating a minimal trace
 * and scan state.
 *
 * ASSUMPTIONS
 *
 * .assume.parked: The root walker must be invoked with a parked
 * arena. It's only strictly necessary for there to be no current trace,
 * but the client has no way to ensure this apart from parking the
 * arena.
 *
 * .assume.rootaddr: The client closure is called with a parameter which
 * is the address of a reference to an object referenced from a
 * root. The client may desire this address to be the address of the
 * actual reference in the root (so that the debugger can be used to
 * determine details about the root). This is not always possible, since
 * the root might actually be a register, or the format scan method
 * might not pass this address directly to the fix method. If the format
 * code does pass on the address, the client can be sure to be passed
 * the address of any root other than a register or stack.  */


/* rootsStepClosure -- closure environment for root walker
 *
 * Defined as a subclass of ScanState.  */

/* SIGnature Roots Step CLOsure */
#define rootsStepClosureSig ((Sig)0x51965C10)  

typedef struct rootsStepClosureStruct *rootsStepClosure;
typedef struct rootsStepClosureStruct {
  ScanStateStruct ssStruct;          /* generic scan state object */
  mps_roots_stepper_t f;             /* client closure function */
  void *p;                           /* client closure data */
  size_t s;                          /* client closure data */
  Root root;                         /* current root, or NULL */
  Sig sig;                           /* impl.h.misc.sig */
} rootsStepClosureStruct;

#define rootsStepClosure2ScanState(rsc) (&(rsc)->ssStruct)
#define ScanState2rootsStepClosure(ss) \
  PARENT(rootsStepClosureStruct, ssStruct, ss)


/* rootsStepClosureCheck -- check a rootsStepClosure */

static Bool rootsStepClosureCheck(rootsStepClosure rsc)
{
  CHECKS(rootsStepClosure, rsc);
  CHECKD(ScanState, &rsc->ssStruct);
  CHECKL(FUNCHECK(rsc->f));
  /* p and s fields are arbitrary closures which cannot be checked */
  if (rsc->root != NULL) {
    CHECKL(RootCheck(rsc->root));
  }
  return TRUE;
}


/* rootsStepClosureInit -- Initialize a rootsStepClosure
 *
 * Initialize the parent ScanState too.  */

static void rootsStepClosureInit(rootsStepClosure rsc, 
                                 Globals arena, Trace trace,
                                 TraceFixMethod rootFix,
                                 mps_roots_stepper_t f, void *p, Size s)
{
  ScanState ss;

  /* First initialize the ScanState superclass */
  ss = &rsc->ssStruct;
  ScanStateInit(ss, TraceSetSingle(trace), GlobalsArena(arena), RankAMBIG,
                NULL, trace->white);
  /* Initialize the fix method in the ScanState */
  ss->fix = rootFix;

  /* Initialize subclass specific data */
  rsc->f = f;
  rsc->p = p;
  rsc->s = s;
  rsc->root = NULL;

  rsc->sig = rootsStepClosureSig;

  AVERT(rootsStepClosure, rsc);
}


/* rootsStepClosureFinish -- Finish a rootsStepClosure
 *
 * Finish the parent ScanState too.  */

static void rootsStepClosureFinish(rootsStepClosure rsc)
{
  ScanState ss;

  ss = rootsStepClosure2ScanState(rsc);
  rsc->sig = SigInvalid;
  ScanStateFinish(ss);
}


/* RootsWalkFix -- the fix method used during root walking
 *
 * This doesn't cause further scanning of transitive references, it just
 * calls the client closure.  */

static void RootsWalkFix(ScanState ss, Ref *refIO)
{
  rootsStepClosure rsc;
  Ref ref;
  Seg seg;
  Arena arena;

  AVERT(ScanState, ss);
  AVER(refIO != NULL);
  rsc = ScanState2rootsStepClosure(ss);
  AVERT(rootsStepClosure, rsc);

  arena = ss->arena;
  ref = *refIO;

  /* Check that the reference is to a valid segment */
  if (SegOfAddr(&seg, arena, ref)) {
    /* Test if the segment belongs to a GCable pool */
    /* If it isn't then it's not in the heap, and the reference */
    /* shouldn't be passed to the client */
    if ((SegPool(seg)->class->attr & AttrGC) != 0) {
      /* Call the client closure - .assume.rootaddr */
      rsc->f((mps_addr_t*)refIO, (mps_root_t)rsc->root, rsc->p, rsc->s);
    }
  } else {
    /* See design.mps.trace.exact.legal */
    AVER(ss->rank < RankEXACT || !ArenaIsReservedAddr(arena, ref));
  }

  /* See design.mps.trace.fix.fixed.all */
  ss->fixedSummary = RefSetAdd(ss->arena, ss->fixedSummary, *refIO);

  AVER(ref == *refIO);  /* can walk object graph - but not modify it */
}


/* rootWalk -- the step function for ArenaRootsWalk */

static Res rootWalk(Root root, void *p)
{
  ScanState ss = (ScanState)p;

  AVERT(ScanState, ss);

  if (RootRank(root) == ss->rank) {
    /* set the root for the benefit of the fix method */
    ScanState2rootsStepClosure(ss)->root = root;
    /* Scan it */
    ScanStateSetSummary(ss, RefSetEMPTY);
    return RootScan(ss, root);
  } else
    return ResOK;
}


/* ArenaRootsWalk -- walks all the root in the arena */

Res ArenaRootsWalk(Globals arenaGlobals, mps_roots_stepper_t f,
                   void *p, size_t s)
{
  Arena arena;
  rootsStepClosureStruct rscStruct;
  rootsStepClosure rsc = &rscStruct;
  Trace trace;
  ScanState ss;
  Rank rank;
  Res res;

  AVERT(Globals, arenaGlobals);
  AVER(FUNCHECK(f));
  /* p and s are arbitrary client-provided closure data. */
  arena = GlobalsArena(arenaGlobals);

  /* Scan all the roots with a minimal trace.  Invoke the scanner with a */
  /* rootsStepClosure, which is a subclass of ScanState and contains the */
  /* client-provided closure.  Supply a special fix method in order to */
  /* call the client closure.  This fix method must perform no tracing */
  /* operations of its own. */

  res = TraceCreate(&trace, arena);
  /* Have to fail if no trace available.  Unlikely due to .assume.parked. */
  if (res != ResOK)
    return res;
  /* Set the white set to universal so that the scanner */
  /* doesn't filter out any references from roots into the arena. */
  trace->white = ZoneSetUNIV; 
  /* Make the roots grey so that they are scanned */
  res = RootsIterate(arenaGlobals, (RootIterateFn)RootGreyAdd, (void *)trace);
  /* Make this trace look like any other trace. */
  arena->flippedTraces = TraceSetAdd(arena->flippedTraces, trace);

  rootsStepClosureInit(rsc, arenaGlobals, trace, RootsWalkFix, f, p, s);
  ss = rootsStepClosure2ScanState(rsc);

  for(rank = RankAMBIG; rank < RankLIMIT; ++rank) {
    ss->rank = rank;
    AVERT(ScanState, ss);
    res = RootsIterate(arenaGlobals, rootWalk, (void *)ss);
    if (res != ResOK)
      break;
  }

  rootsStepClosureFinish(rsc);
  /* Make this trace look like any other finished trace. */
  trace->state = TraceFINISHED;
  TraceDestroy(trace);

  return res;
}
