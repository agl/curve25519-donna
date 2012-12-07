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

int
main() {
  static const unsigned char basepoint[32] = {9};
  unsigned char mysecret[32], mypublic[32];
  unsigned i;
  uint64_t start, end;

  memset(mysecret, 42, 32);
  mysecret[0] &= 248;
  mysecret[31] &= 127;
  mysecret[31] |= 64;

  // Load the caches
  for (i = 0; i < 1000; ++i) {
    curve25519_donna(mypublic, mysecret, basepoint);
  }

  start = time_now();
  for (i = 0; i < 30000; ++i) {
    curve25519_donna(mypublic, mysecret, basepoint);
  }
  end = time_now();

  printf("%luus\n", (unsigned long) ((end - start) / 30000));

  return 0;
}
