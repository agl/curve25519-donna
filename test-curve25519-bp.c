
#include <stdio.h>

extern void curve25519_donna(unsigned char *output, const unsigned char *a,
                             const unsigned char *b);

extern void curve25519_base_donna(unsigned char *output, const unsigned char *a);

int
main()
{
  int i, loop;
  unsigned char bp[32] = {9};
  unsigned char x[32] = {3};
  unsigned char y[32];
  unsigned char y2[32];

  for (loop = 0; loop < 1000; ++loop) {
    curve25519_donna(y, x, bp);
    curve25519_base_donna(y2, x);

    for (i = 0;i < 32;++i) printf("%02x",(unsigned int) y[i]); printf(" ");
    for (i = 0;i < 32;++i) printf("%02x",(unsigned int) y2[i]); printf("\n");

    for (i=0; i<32; ++i) if (y[i] != y2[i]) {
      printf("fail\n");
      return 1;
    }
    for (i=0; i<32; ++i) x[i] ^= y[i];
  }
  return 0;
}
