#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

extern void curve25519_donna(uint8_t *, const uint8_t *, const uint8_t *);
extern uint64_t tsc_read();

int
main(int argc, char **argv) {
  uint8_t private_key[32], public[32], peer1[32], peer2[32], output[32];
  static const uint8_t basepoint[32] = {9};
  unsigned i;
  uint64_t sum = 0, sum_squares = 0, skipped = 0, mean;
  static const unsigned count = 200000;

  memset(private_key, 42, sizeof(private_key));

  private_key[0] &= 248;
  private_key[31] &= 127;
  private_key[31] |= 64;

  curve25519_donna(public, private_key, basepoint);
  memset(peer1, 0, sizeof(peer1));
  memset(peer2, 255, sizeof(peer2));

  for (i = 0; i < count; ++i) {
    const uint64_t start = tsc_read();
    curve25519_donna(output, peer1, public);
    const uint64_t end = tsc_read();
    const uint64_t delta = end - start;
    if (delta > 650000) {
      // something terrible happened (task switch etc)
      skipped++;
      continue;
    }
    sum += delta;
    sum_squares += (delta * delta);
  }

  mean = sum / ((uint64_t) count);
  printf("all 0: mean:%lu sd:%f skipped:%lu\n",
         mean,
         sqrt((double)(sum_squares/((uint64_t) count) - mean*mean)),
         skipped);

  sum = sum_squares = skipped = 0;

  for (i = 0; i < count; ++i) {
    const uint64_t start = tsc_read();
    curve25519_donna(output, peer2, public);
    const uint64_t end = tsc_read();
    const uint64_t delta = end - start;
    if (delta > 650000) {
      // something terrible happened (task switch etc)
      skipped++;
      continue;
    }
    sum += delta;
    sum_squares += (delta * delta);
  }

  mean = sum / ((uint64_t) count);
  printf("all 1: mean:%lu sd:%f skipped:%lu\n",
         mean,
         sqrt((double)(sum_squares/((uint64_t) count) - mean*mean)),
         skipped);

  return 0;
}
