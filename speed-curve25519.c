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
  gettimeofday(&tv, NULL);
  uint64_t ret = tv.tv_sec;
  ret *= 1000000;
  ret += tv.tv_usec;

  return ret;
}

int
main() {
  static const unsigned char basepoint[32] = {9};
  unsigned char mysecret[32], mypublic[32];
  unsigned i;

  memset(mysecret, 42, 32);
  mysecret[0] &= 248;
  mysecret[31] &= 127;
  mysecret[31] |= 64;

  // Load the caches
  for (i = 0; i < 5; ++i) {
    curve25519_donna(mypublic, mysecret, basepoint);
  }

  uint64_t start = time_now();
  for (i = 0; i < 100; ++i) {
    curve25519_donna(mypublic, mysecret, basepoint);
  }
  uint64_t end = time_now();

  printf("%luus\n", (end - start) / 100);

  return 0;
}
