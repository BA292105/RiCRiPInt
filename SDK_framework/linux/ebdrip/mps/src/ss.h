/* impl.h.ss: STACK SCANNING
 *
 *  $Id: ss.h,v 1.4.31.1.1.1 2013/12/19 11:27:08 anon Exp $
 *  $HopeName: MMsrc!ss.h(EBDSDK_P.1) $
 *  Copyright (c) 2001 Ravenbrook Limited.
 *
 *  Provides a function for scanning the stack and registers
 *
 */

#ifndef ss_h
#define ss_h

#include "mpm.h"


/*  == StackScan ==
 *
 *  StackScan scans the current stack between the
 *  stackBot and the current top of stack. It also fixes
 *  any roots which may be in registers.
 *
 *  See the specific implementation for the exact registers which
 *  are scanned.
 *
 *  The word pointed to by stackBot is fixed if the stack
 *  is by convention empty, and not fixed if it is full.
 *  Where empty means sp points to first free word beyond the top of
 *  stack.  Full means sp points to the top of stack itself.
 */

extern Res StackScan(ScanState ss, Addr *stackBot);


#endif /* ss_h */
