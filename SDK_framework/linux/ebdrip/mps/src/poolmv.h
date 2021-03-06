/* impl.h.poolmv: MANUAL VARIABLE POOL
 *
 * $Id: poolmv.h,v 1.11.11.1.1.1 2013/12/19 11:27:09 anon Exp $
 * $HopeName: MMsrc!poolmv.h(EBDSDK_P.1) $
 * Copyright (c) 2001 Ravenbrook Limited.
 * Copyright (C) 2002-2007 Global Graphics Software Ltd. All rights reserved.
 *Global Graphics Software Ltd. Confidential Information.
 *
 * .purpose: This is the interface to the manual-variable pool class.
 *
 * .mv: Manual-variable pools manage variably-sized blocks of memory in a
 *  flexible manner.  They have higher overheads than a fixed-size pool.
 *
 * .init: This class adds the following arguments to PoolCreate:
 *
 *    Size extendBy
 *
 *  extendBy is the default number of bytes reserved by the pool at a time.
 *  A large size will make allocation cheaper but have a higher resource
 *  overhead.  A typical value might be 65536.  See note 2.
 *
 *    Size avgSize
 *
 *  avgSize is an estimate of the average size of an allocation, and is used
 *  to choose the size of internal tables.  An accurate estimate will
 *  improve the efficiency of the pool.  A low estimate will make the pool
 *  less space efficient.  A high estimate will make the pool less time
 *  efficient.  A typical value might be 32.  avgSize must not be less than
 *  extendBy.
 *
 *    Size maxSize
 *
 *  maxSize is an estimate of the maximum total size that the pool will
 *  reach.  Setting this parameter does not actually contrain the pool, but
 *  an accurate estimate will improve the efficiency of the pool.  maxSize
 *  must not be less than extendBy.
 *
 *  Notes
 *   2. The documentation could suggest a segment size according to the
 *      distribution of allocation size requests.  richard 1994-11-08
 */

#ifndef poolmv_h
#define poolmv_h


#include "mpmtypes.h"

typedef struct MVStruct *MV;

extern PoolClass PoolClassMV(void);

extern Bool MVCheck(MV mv);

#define MV2Pool(mv) (&(mv)->poolStruct)


#endif /* poolmv_h */
