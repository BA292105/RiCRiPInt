/* impl.h.config: MPS CONFIGURATION
 *
 * $Id: config.h,v 1.60.11.1.1.1 2013/12/19 11:27:08 anon Exp $
 * $HopeName: MMsrc!config.h(EBDSDK_P.1) $
 * Copyright (c) 2001 Ravenbrook Limited.
 * Copyright (C) 2007 Global Graphics Software Ltd. All rights reserved.
 *Global Graphics Software Ltd. Confidential Information.
 *
 * PURPOSE
 *
 * This module translates from high-level symbols defined by the
 * external build system (gnumake, nmake, etc.) into specific sets
 * of features used by MPS modules.
 *
 * DESIGN
 *
 * See design.mps.config.
 */

#ifndef config_h
#define config_h


/* Variety Configuration */


#if defined(CONFIG_ASSERT)
#define CHECK
#define MPS_ASSERT_STRING "asserted"
#else
#define CHECK_NONE
#define MPS_ASSERT_STRING "nonasserted"
#endif


#if defined(CONFIG_LOG)
#define EVENT
#define MPS_LOG_STRING "logging"
#else
#define EVENT_NONE
#define MPS_LOG_STRING "nonlogging"
#endif


#if defined(CONFIG_DEBUG)
#define DIAGNOSTICS
#define MPS_DEBUG_STRING "debug"
#else
#define DIAGNOSTICS_NONE
#define MPS_DEBUG_STRING "nondebug"
#endif

#define MPS_VARIETY_STRING \
  MPS_ASSERT_STRING "." MPS_LOG_STRING "." MPS_DEBUG_STRING


/* Platform Configuration */

#include "mpstd.h"

/* Suppress Visual C warnings at warning level 4, */
/* see mail.richard.1997-09-25.13-26. */
/* Essentially the same settings are done in testlib.h. */

#ifdef MPS_BUILD_MV

/* "unreferenced inline function has been removed" (windows.h) */
#pragma warning(disable: 4514)

/* "constant conditional" (MPS_END) */
#pragma warning(disable: 4127)

/* "unreachable code" (ASSERT, if cond is constantly true). */
#pragma warning(disable: 4702)

/* "function selected for automatic inline expansion" */
#pragma warning(disable: 4711)

/* MSVC 2.0 generates a warning when using NOCHECK or UNUSED */
#ifdef _MSC_VER
#if _MSC_VER < 1000
#pragma warning(disable: 4705)
#endif
#else /* _MSC_VER */
#error "Expected _MSC_VER to be defined for builder.mv"
#endif /* _MSC_VER */

/* MSVC 10.00 on PowerPC generates erroneous warnings about */
/* uninitialized local variables, if you take their address. */
#ifdef MPS_ARCH_PP
#pragma warning(disable: 4701)
#endif /* MPS_ARCH_PP */


/* Non-checking varieties give many spurious warnings because parameters
 * are suddenly unused, etc.  We aren't interested in these
 */

#if defined(CHECK_NONE)

/* "unreferenced formal parameter" */
#pragma warning(disable: 4100)

/* "unreferenced local function has been removed" */
#pragma warning(disable: 4505)

#endif /* CHECK_NONE */

#endif /* MPS_BUILD_MV */


/* EPVMDefaultSubsequentSegSIZE is a default for the alignment of
 * subsequent segments (non-initial at each save level) in EPVM.  See
 * design.mps.poolepvm.arch.segment.size.
 */

#define EPVMDefaultSubsequentSegSIZE (64ul * 1024)


/* Arena Configuration -- see impl.c.arena* */

#define ARENA_CONTROL_EXTENDBY  ((Size)4096)
#define ARENA_CONTROL_AVGSIZE   ((Size)32)
#define ARENA_CONTROL_MAXSIZE   ((Size)65536)

#define ArenaPollALLOCTIME (65536.0)

#define ARENA_ZONESHIFT         ((Shift)20)

/* ARENA_CLIENT_PAGE_SIZE is the size in bytes of a "page" in the client
 * arena.  It's set at 8192 with no particular justification.  */
#define ARENA_CLIENT_PAGE_SIZE          ((Size)8192)


/* sharedArenaChunkMAX limits the number of chunks in a shared arena.
 * The later chunks are smaller and slow down allocation.  Also, some
 * OSs have limits on the number of shared memory segments.  This value
 * allows ~4 GB of 32 MB segments. */
#define sharedArenaChunkMAX ((Count)127)


/* Arbitrary calculation for the maximum number of distinct
 * object sets for generations. */
#define VMArenaGenCount ((Count)(MPS_WORD_WIDTH/2))

/* VMArenaTryCOUNT regulates how many times the VM arena is prepared to
 * try to reserve the right size of block. */
#define VMArenaTryCOUNT (128)


/* Segment placement configuration -- see impl.c.locus */

#define ArenaDefaultZONESET (ZoneSetUNIV << (MPS_WORD_WIDTH / 2))
/* @@@@ knows the implementation of ZoneSets */

/* .segpref.default: For EPcore, non-DL segments should be placed high */
/* to reduce fragmentation of DL pools (see request.epcore.170193). */
#define SegPrefDEFAULT { \
  SegPrefSig,          /* sig */ \
  TRUE,                /* high */ \
  ArenaDefaultZONESET, /* zoneSet */ \
  FALSE,               /* isCollected */ \
  FALSE,               /* isGen */ \
  (Serial)0,           /* gen */ \
}


/*  Location Dependency configuration -- see impl.c.ld */

#define LDHistoryLENGTH ((Size)4)


/* Stack configuration */

/* Currently StackProbe has a useful implementation only on
 * Intel platforms and only when using Microsoft build tools (builder.mv)
 */
#if defined(MPS_ARCH_I3) && defined(MPS_BUILD_MV)
#define StackProbeDEPTH ((Size)500)
#else
#define StackProbeDEPTH ((Size)0)
#endif /* MPS_ARCH_I3 */


/* Shield Configuration -- see impl.c.shield */

#define ShieldCacheSIZE ((size_t)16)
#define ShieldDepthWIDTH (4)


/* VM Configuration -- see impl.c.vm* */

#define VMANPageALIGNMENT ((Align)4096)
#define VMJunkBYTE ((unsigned char)0xA9)


/* Tracer Configuration -- see impl.c.trace */

#define TraceLIMIT ((size_t)1)
/* I count 4 function calls to scan, 10 to copy. */
#define TraceCopyScanRATIO (1.5)



/* Events
 *
 * EventBufferSIZE is the number of words in the global event buffer.
 */

#define EventBufferSIZE ((size_t)4096)
#define EventStringLengthMAX ((size_t)255) /* Not including NUL */


/* memory operator configuration
 *
 * We need efficient operators similar to memcpy, memset, and memcmp.
 * In general, we cannot use the C library mem functions directly as
 * that would not be freestanding.  However, on some platforms we can do
 * this, because they are inlined by the compiler and so do not actually
 * create a dependence on an external library.
 */

#if defined(MPS_HOSTED)
/* MSVC & mingw on Intel inline mem*() when optimizing */
#define mps_lib_memset memset
#define mps_lib_memcpy memcpy
#define mps_lib_memcmp memcmp
/* get prototypes for ANSI mem* */
#include <string.h>
#endif


/* Product Configuration
 *
 * Convert CONFIG_PROD_* defined on compiler command line into
 * internal configuration parameters.  See design.mps.config.prod.
 */

#if defined(CONFIG_PROD_EPCORE)
#define MPS_PROD_STRING         "epcore"
#define MPS_PROD_EPCORE
#define ARENA_INIT_SPARE_COMMIT_LIMIT   ((Size)0)
#if defined(MPS_OS_VX)
#define THREAD_SINGLE
#else
#define THREAD_MULTI
#endif
#define PROTECTION_NONE
#define DONGLE_NONE
#define CHECK_DEFAULT CheckNONE /* CheckSHALLOW is too slow for SW */
#define STACKPROBE_NONE

#elif defined(CONFIG_PROD_DYLAN)
#define MPS_PROD_STRING         "dylan"
#define MPS_PROD_DYLAN
/* .prod.arena-size: ARENA_SIZE is currently set larger for the
 * MM/Dylan product as an interim solution.
 * See request.dylan.170170.sol.patch and change.dylan.buffalo.170170.
 */
#define ARENA_SIZE              ((Size)1<<30)
#define ARENA_INIT_SPARE_COMMIT_LIMIT   ((Size)10uL*1024uL*1024uL)
#define THREAD_MULTI
#define PROTECTION
#define DONGLE_NONE
#define CHECK_DEFAULT CheckSHALLOW
#define STACKPROBE

#elif defined(CONFIG_PROD_MPS)
#define MPS_PROD_STRING         "mps"
#define MPS_PROD_MPS
#define ARENA_INIT_SPARE_COMMIT_LIMIT   ((Size)10uL*1024uL*1024uL)
#if defined(MPS_OS_VX)
#define THREAD_SINGLE
#else
#define THREAD_MULTI
#endif
#define PROTECTION
#define DONGLE_NONE
#define CHECK_DEFAULT CheckSHALLOW
#define STACKPROBE_NONE

#else
#error "No target product configured."
#endif


/* Dongle configuration */

#if defined(DONGLE)

#define DONGLE_TEST_FREQUENCY ((unsigned int)4000)

#elif defined(DONGLE_NONE)

/* nothing to do */

#else
#error "No dongle configured."
#endif


#endif /* config_h */
