/* impl.c.testlib: TEST LIBRARY
 *
 * $Id: testlib.c,v 1.29.11.1.1.1 2013/12/19 11:27:07 anon Exp $
 * $HopeName: MMsrc!testlib.c(EBDSDK_P.1) $
 * Copyright (c) 2001 Ravenbrook Limited.
 * Copyright (C) 2002-2007 Global Graphics Software Ltd. All rights reserved.
 *Global Graphics Software Ltd. Confidential Information.
 *
 * .purpose: A library of functions that may be of use to unit tests.
 */

#include "testlib.h"
#include "mps.h"
#include "misc.h" /* for NOOP */
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#ifdef MPS_OS_IA
struct itimerspec; /* stop complaints from time.h */
#endif
#include <time.h>


/* rnd -- a random number generator
 *
 * I nabbed it from "ML for the Working Programmer", originally from:
 * Stephen K Park & Keith W Miller (1988). Random number generators:
 * good one are to find.  Communications of the ACM, 31:1192-1201.
 */

unsigned long rnd(void)
{
  static unsigned long seed = 1;
  double s;

  s = seed;
  s *= 16807.0;
  s = fmod(s, 2147483647.0);  /* 2^31 - 1 */
  seed = (unsigned long)s;
  return seed;
}


/* rnd_addr -- a random address generator
 *
 * rnd gives 31 random bits, we run it repeatedly to get enough bits.
 */

#define ADDR_BITS (sizeof(mps_addr_t) * CHAR_BIT)

mps_addr_t rnd_addr(void)
{
  mps_word_t res;
  unsigned bits;

  for (bits = 0, res = 0; bits < ADDR_BITS;
       bits += 31, res = res << 31 | (mps_word_t)rnd())
    NOOP;
  return (mps_addr_t)res;
}


/* randomize -- randomize the generator, or initialize to replay */

void randomize(int argc, char **argv)
{
  int i, k, n;

  if (argc > 1) {
    n = sscanf(argv[1], "%d", &k);
    die((n == 1) ? MPS_RES_OK : MPS_RES_FAIL, "randomize");
  } else {
    k = (int)time(NULL) % 32000;
    printf("Randomizing %d times.\n", k);
  }

  /* Randomize the random number generator a bit. */
  for (i = k; i > 0; --i)
    rnd();
}


/* verror -- die with message */

void verror(const char *format, va_list args)
{
  fflush(stdout); /* synchronize */
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  exit(1);
}


/* error -- die with message */

void error(const char *format, ...)
{
 va_list args;

 va_start(args, format);
 verror(format, args);
 va_end(args);
}


/* die -- Test a return code, and exit on error */

void die(mps_res_t res, const char *s)
{
  if (res != MPS_RES_OK) {
    error("\n%s: %d\n", s, res);
  }
}


/* die_expect -- Test a return code, and exit on unexpected result */

void die_expect(mps_res_t res, mps_res_t expected, const char *s)
{
  if (res != expected) {
    error("\n%s: %d\n", s, res);
  }
}


/* cdie -- Test a C boolean, and exit on error */

void cdie(int res, const char *s)
{
  if (!res) {
    error("\n%s: %d\n", s, res);
  }
}
