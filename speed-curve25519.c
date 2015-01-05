#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

typedef uint8_t u8;

extern void curve25519_donna(u8 *output, const u8 *secret, const u8 *bp);

static uint64_t
time_now() {
  struct timeval tv;
  uint64_t ret;

  gettimeofday(&tv, NULL);
  ret = tv.tv_sec;
  ret *= 1000000;
  ret += tv.tv_usec;

  return ret;
}

/* ticks - not tested on anything other than x86 */
static uint64_t
cycles_now(void) {
        #if defined(__GNUC__)
                uint32_t lo, hi;
                __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
                return ((uint64_t)lo | ((uint64_t)hi << 32));
	#else
		return 0;	/* Undefined for now; should be obvious in the output */
        #endif
}


int
main() {
  static const unsigned char basepoint[32] = {9};
  unsigned char mysecret[32], mypublic[32];
  unsigned i;
  uint64_t start, end;
  const unsigned iterations = 100000;
  uint64_t start_c, end_c;

  memset(mysecret, 42, 32);
  mysecret[0] &= 248;
  mysecret[31] &= 127;
  mysecret[31] |= 64;

  // Load the caches
  for (i = 0; i < 1000; ++i) {
    curve25519_donna(mypublic, mysecret, basepoint);
  }

  start = time_now();
  start_c = cycles_now();
  for (i = 0; i < iterations; ++i) {
    curve25519_donna(mypublic, mysecret, basepoint);
  }
  end = time_now();
  end_c = cycles_now();

  printf("%lu us, %g op/s, %lu cycles/op\n", 
	(unsigned long) ((end - start) / iterations), 
	iterations*1000000. / (end - start), 
	(unsigned long)((end_c-start_c)/iterations) );

  return 0;
}
