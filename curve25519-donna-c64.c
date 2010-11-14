/* Copyright 2008, Google Inc.
 * All rights reserved.
 *
 * Code released into the public domain.
 *
 * curve25519-donna: Curve25519 elliptic curve, public key function
 *
 * http://code.google.com/p/curve25519-donna/
 *
 * Adam Langley <agl@imperialviolet.org>
 *
 * Derived from public domain C code by Daniel J. Bernstein <djb@cr.yp.to>
 *
 * More information about curve25519 can be found here
 *   http://cr.yp.to/ecdh.html
 *
 * djb's sample implementation of curve25519 is written in a special assembly
 * language called qhasm and uses the floating point registers.
 *
 * This is, almost, a clean room reimplementation from the curve25519 paper. It
 * uses many of the tricks described therein. Only the crecip function is taken
 * from the sample implementation.
 */

#include <string.h>
#include <stdint.h>

#define DONNA_INLINE
#if defined(DONNA_INLINE)
	#undef DONNA_INLINE
	#define DONNA_INLINE __attribute__((always_inline))
#else
	#define DONNA_INLINE
#endif

typedef uint8_t u8;
typedef uint64_t felem;
typedef int64_t felemsigned;
typedef felem bignum[5];
// This is a special gcc mode for 128-bit integers. It's implemented on 64-bit
// platforms only as far as I know.
typedef unsigned uint128_t __attribute__((mode(TI)));
typedef uint128_t felemx2;


/* Sum two numbers: output += in */
static void DONNA_INLINE
fsum(bignum output, const bignum in) {
  output[0] += in[0];
  output[1] += in[1];
  output[2] += in[2];
  output[3] += in[3];
  output[4] += in[4];
}

/* Find the difference of two numbers: output = in - output
 * (note the order of the arguments!)
 */
static void DONNA_INLINE
fdifference_backwards(bignum output, const bignum in) {
  static const felemsigned twotothe51 = (1ll << 51);
  felemsigned r0,r1,r2,r3,r4;

  // An arithmetic shift right of 63 places turns a positive number to 0 and a
  // negative number to all 1's. This gives us a bitmask that lets us avoid
  // side-channel prone branches.
  felemsigned t;

  r0 = (felemsigned)in[0] - (felemsigned)output[0];
  r1 = (felemsigned)in[1] - (felemsigned)output[1];
  r2 = (felemsigned)in[2] - (felemsigned)output[2];
  r3 = (felemsigned)in[3] - (felemsigned)output[3];
  r4 = (felemsigned)in[4] - (felemsigned)output[4];

  #define negchain(a,b) \
    t = r##a >> 63; \
    r##b -= (felem)t >> 63; \
    r##a += twotothe51 & t;

  #define negchain19(a,b) \
    t = r##a >> 63; \
    r##b -= 19 & t; \
    r##a += twotothe51 & t;

  negchain(0, 1);
  negchain(1, 2);
  negchain(2, 3);
  negchain(3, 4);
  negchain19(4, 0);
  negchain(0, 1);
  negchain(1, 2);
  negchain(2, 3);
  negchain(3, 4);

  output[0] = r0;
  output[1] = r1;
  output[2] = r2;
  output[3] = r3;
  output[4] = r4;
}

/* Multiply a number by a scalar and add: output = (in * scalar) + add */
static void DONNA_INLINE
fscalar_product_sum(bignum output, const bignum in, const felem scalar, const bignum add) {
  felemx2 a;
  felem r0,r1,r2,r3,r4,c;

  a = ((felemx2) in[0]) * scalar;     r0 = (felem)a & 0x7ffffffffffff; c = (felem)(a >> 51);
  a = ((felemx2) in[1]) * scalar + c; r1 = (felem)a & 0x7ffffffffffff; c = (felem)(a >> 51);
  a = ((felemx2) in[2]) * scalar + c; r2 = (felem)a & 0x7ffffffffffff; c = (felem)(a >> 51);
  a = ((felemx2) in[3]) * scalar + c; r3 = (felem)a & 0x7ffffffffffff; c = (felem)(a >> 51);
  a = ((felemx2) in[4]) * scalar + c; r4 = (felem)a & 0x7ffffffffffff; c = (felem)(a >> 51);
                                      r0 += c * 19;
  output[0] = r0 + add[0];
  output[1] = r1 + add[1];
  output[2] = r2 + add[2];
  output[3] = r3 + add[3];
  output[4] = r4 + add[4];
}

/* Multiply two numbers: output = in2 * in
 *
 * The inputs are reduced coefficient form, the output is not.
 */
static void DONNA_INLINE
fmul(bignum output, const bignum in2, const bignum in) {
  felemx2 t[5];
  felem r0,r1,r2,r3,r4,s0,s1,s2,s3,s4,c;

  r0 = in[0];
  r1 = in[1];
  r2 = in[2];
  r3 = in[3];
  r4 = in[4];

  s0 = in2[0];
  s1 = in2[1];
  s2 = in2[2];
  s3 = in2[3];
  s4 = in2[4];

  t[0]  =  ((felemx2) r0) * s0;
  t[1]  =  ((felemx2) r0) * s1 + ((felemx2) r1) * s0;
  t[2]  =  ((felemx2) r0) * s2 + ((felemx2) r2) * s0 + ((felemx2) r1) * s1;
  t[3]  =  ((felemx2) r0) * s3 + ((felemx2) r3) * s0 + ((felemx2) r1) * s2 + ((felemx2) r2) * s1;
  t[4]  =  ((felemx2) r0) * s4 + ((felemx2) r4) * s0 + ((felemx2) r3) * s1 + ((felemx2) r1) * s3 + ((felemx2) r2) * s2;

  r4 *= 19;
  r1 *= 19;
  r2 *= 19;
  r3 *= 19;

  t[0] += ((felemx2) r4) * s1 + ((felemx2) r1) * s4 + ((felemx2) r2) * s3 + ((felemx2) r3) * s2;
  t[1] += ((felemx2) r4) * s2 + ((felemx2) r2) * s4 + ((felemx2) r3) * s3;
  t[2] += ((felemx2) r4) * s3 + ((felemx2) r3) * s4;
  t[3] += ((felemx2) r4) * s4;

                  r0 = (felem)t[0] & 0x7ffffffffffff; c = (felem)(t[0] >> 51);
  t[1] += c;      r1 = (felem)t[1] & 0x7ffffffffffff; c = (felem)(t[1] >> 51);
  t[2] += c;      r2 = (felem)t[2] & 0x7ffffffffffff; c = (felem)(t[2] >> 51);
  t[3] += c;      r3 = (felem)t[3] & 0x7ffffffffffff; c = (felem)(t[3] >> 51);
  t[4] += c;      r4 = (felem)t[4] & 0x7ffffffffffff; c = (felem)(t[4] >> 51);
  r0 +=   c * 19; c = r0 >> 51; r0 = r0 & 0x7ffffffffffff;
  r1 +=   c;      c = r1 >> 51; r1 = r1 & 0x7ffffffffffff;
  r2 +=   c;

  output[0] = r0;
  output[1] = r1;
  output[2] = r2;
  output[3] = r3;
  output[4] = r4;
}

static void DONNA_INLINE
fsquare_times(bignum output, const bignum in, felem count) {
  felemx2 t[5];
  felem r0,r1,r2,r3,r4,c;
  felem d0,d1,d2,d4,d419;

  r0 = in[0];
  r1 = in[1];
  r2 = in[2];
  r3 = in[3];
  r4 = in[4];

  do {
    d0 = r0 * 2;
    d1 = r1 * 2;
    d2 = r2 * 2 * 19;
    d419 = r4 * 19;
    d4 = d419 * 2;

	t[0] = ((felemx2) r0) * r0 + ((felemx2) d4) * r1 + (((felemx2) d2) * (r3     ));
	t[1] = ((felemx2) d0) * r1 + ((felemx2) d4) * r2 + (((felemx2) r3) * (r3 * 19));
	t[2] = ((felemx2) d0) * r2 + ((felemx2) r1) * r1 + (((felemx2) d4) * (r3     ));
	t[3] = ((felemx2) d0) * r3 + ((felemx2) d1) * r2 + (((felemx2) r4) * (d419   ));
    t[4] = ((felemx2) d0) * r4 + ((felemx2) d1) * r3 + (((felemx2) r2) * (r2     ));

                    r0 = (felem)t[0] & 0x7ffffffffffff; c = (felem)(t[0] >> 51);
    t[1] += c;      r1 = (felem)t[1] & 0x7ffffffffffff; c = (felem)(t[1] >> 51);
    t[2] += c;      r2 = (felem)t[2] & 0x7ffffffffffff; c = (felem)(t[2] >> 51);
    t[3] += c;      r3 = (felem)t[3] & 0x7ffffffffffff; c = (felem)(t[3] >> 51);
    t[4] += c;      r4 = (felem)t[4] & 0x7ffffffffffff; c = (felem)(t[4] >> 51);
    r0 +=   c * 19; c = r0 >> 51; r0 = r0 & 0x7ffffffffffff;
    r1 +=   c;      c = r1 >> 51; r1 = r1 & 0x7ffffffffffff;
    r2 +=   c;
  } while(--count);

  output[0] = r0;
  output[1] = r1;
  output[2] = r2;
  output[3] = r3;
  output[4] = r4;
}

/* Take a little-endian, 32-byte number and expand it into polynomial form */
static void DONNA_INLINE
fexpand(bignum output, const u8 *in) {
  felem t, i;

  #define read51full(n,start,shift) \
    for (t = in[(start)] >> (shift), i = 0; i < (6 + ((shift)/6)); i++) \
      t |= ((felem)in[i+(start)+1] << ((i * 8) + (8 - (shift)))); \
    output[n] = t & 0x7ffffffffffff;
  #define read51(n) read51full(n,(n*51)/8,(n*3)&7)

  read51(0)
  read51(1)
  read51(2)
  read51(3)
  read51(4)
}

/* Take a fully reduced polynomial form number and contract it into a
 * little-endian, 32-byte array
 */
static void DONNA_INLINE
fcontract(u8 *output, const bignum input) {
  felemx2 t[5];
  felem f, i;

  t[0] = input[0];
  t[1] = input[1];
  t[2] = input[2];
  t[3] = input[3];
  t[4] = input[4];

  #define fcontract_carry() \
    t[1] += t[0] >> 51; t[0] &= 0x7ffffffffffff; \
    t[2] += t[1] >> 51; t[1] &= 0x7ffffffffffff; \
    t[3] += t[2] >> 51; t[2] &= 0x7ffffffffffff; \
    t[4] += t[3] >> 51; t[3] &= 0x7ffffffffffff;

  #define fcontract_carry_full() fcontract_carry() \
    t[0] += 19 * (t[4] >> 51); t[4] &= 0x7ffffffffffff;

  #define fcontract_carry_final() fcontract_carry() \
    t[4] &= 0x7ffffffffffff;

  fcontract_carry_full()
  fcontract_carry_full()

  /* now t is between 0 and 2^255-1, properly carried. */
  /* case 1: between 0 and 2^255-20. case 2: between 2^255-19 and 2^255-1. */
  t[0] += 19;
  fcontract_carry_full()

  /* now between 19 and 2^255-1 in both cases, and offset by 19. */
  t[0] += 0x8000000000000 - 19;
  t[1] += 0x8000000000000 - 1;
  t[2] += 0x8000000000000 - 1;
  t[3] += 0x8000000000000 - 1;
  t[4] += 0x8000000000000 - 1;

  /* now between 2^255 and 2^256-20, and offset by 2^255. */
  fcontract_carry_final()

  #define write51full(n,shift) \
    f = ((t[n] >> shift) | (t[n+1] << (51 - shift))); \
    for (i = 0; i < 8; i++, f >>= 8) *output++ = (u8)f;
  #define write51(n) write51full(n,13*n)
  write51(0)
  write51(1)
  write51(2)
  write51(3)
}

static void DONNA_INLINE
fcopy(bignum out, const bignum in) {
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
  out[3] = in[3];
  out[4] = in[4];
}

// -----------------------------------------------------------------------------
// Shamelessly copied from djb's code, tightened a little
// -----------------------------------------------------------------------------
static void
crecip(bignum out, const bignum z) {
  bignum a,t0,b,c;

  /* 2 */ fsquare_times(a, z, 1); // a = 2
  /* 8 */ fsquare_times(t0, a, 2);
  /* 9 */ fmul(b, t0, z); // b = 9
  /* 11 */ fmul(a, b, a); // a = 11
  /* 22 */ fsquare_times(t0, a, 1);
  /* 2^5 - 2^0 = 31 */ fmul(b, t0, b);
  /* 2^10 - 2^5 */ fsquare_times(t0, b, 5);
  /* 2^10 - 2^0 */ fmul(b, t0, b);
  /* 2^20 - 2^10 */ fsquare_times(t0, b, 10);
  /* 2^20 - 2^0 */ fmul(c, t0, b);
  /* 2^40 - 2^20 */ fsquare_times(t0, c, 20);
  /* 2^40 - 2^0 */ fmul(t0, t0, c);
  /* 2^50 - 2^10 */ fsquare_times(t0, t0, 10);
  /* 2^50 - 2^0 */ fmul(b, t0, b);
  /* 2^100 - 2^50 */ fsquare_times(t0, b, 50);
  /* 2^100 - 2^0 */ fmul(c, t0, b);
  /* 2^200 - 2^100 */ fsquare_times(t0, c, 100);
  /* 2^200 - 2^0 */ fmul(t0, t0, c);
  /* 2^250 - 2^50 */ fsquare_times(t0, t0, 50);
  /* 2^250 - 2^0 */ fmul(t0, t0, b);
  /* 2^255 - 2^5 */ fsquare_times(t0, t0, 5);
  /* 2^255 - 21 */ fmul(out, t0, a);
}

// -----------------------------------------------------------------------------
// Maybe swap the contents of two felem arrays (@a and @b), each 5 elements
// long. Perform the swap iff @swap is non-zero.
//
// This function performs the swap without leaking any side-channel
// information.
// -----------------------------------------------------------------------------
static void DONNA_INLINE
swap_conditional(bignum a, bignum b, felem iswap) {
  const felem swap = -iswap;
  felem x0,x1,x2,x3,x4;

  x0 = swap & (a[0] ^ b[0]); a[0] ^= x0; b[0] ^= x0;
  x1 = swap & (a[1] ^ b[1]); a[1] ^= x1; b[1] ^= x1;
  x2 = swap & (a[2] ^ b[2]); a[2] ^= x2; b[2] ^= x2;
  x3 = swap & (a[3] ^ b[3]); a[3] ^= x3; b[3] ^= x3;
  x4 = swap & (a[4] ^ b[4]); a[4] ^= x4; b[4] ^= x4;
}

/* Calculates nQ where Q is the x-coordinate of a point on the curve
 *
 *   mypublic: the packed little endian x coordinate of the resulting curve point
 *   n: a little endian, 32-byte number
 *   basepoint: a packed little endian point of the curve
 */

static void
curve25519_scalarmult(u8 *mypublic, const u8 n[32], const u8 basepoint[32]) {
  bignum nqpqx, nqpqz = {1}, nqx = {1}, nqz = {0};
  bignum q, origx, origxprime, zzz, xx, zz, xxprime, zzprime, zzzprime, zmone;
  felem bit, lastbit, i;

  fexpand(q, basepoint);
  fcopy(nqpqx, q);

  i = 255;
  lastbit = 0;

  do {
    bit = (n[i/8] >> (i & 7)) & 1;
    swap_conditional(nqx, nqpqx, bit ^ lastbit);
    swap_conditional(nqz, nqpqz, bit ^ lastbit);
    lastbit = bit;

    fcopy(origx, nqx);
    fsum(nqx, nqz);
    fdifference_backwards(nqz, origx); // x - z
    fcopy(origxprime, nqpqx);
    fsum(nqpqx, nqpqz);
    fdifference_backwards(nqpqz, origxprime); // nqpqx - nqpqz
    fmul(xxprime, nqpqx, nqz);
    fmul(zzprime, nqx, nqpqz);
    fcopy(origxprime, xxprime);
    fsum(xxprime, zzprime);
    fdifference_backwards(zzprime, origxprime); // xxprime - zzprime
    fsquare_times(zzzprime, zzprime, 1);
    fsquare_times(nqpqx, xxprime, 1);
    fmul(nqpqz, zzzprime, q);
    fsquare_times(xx, nqx, 1);
    fsquare_times(zz, nqz, 1);
    fmul(nqx, xx, zz);
    fdifference_backwards(zz, xx);  // does zz = xx - zz
    fscalar_product_sum(zzz, zz, 121665, xx); // zzz = (zz * 121665) + xx
    fmul(nqz, zz, zzz);
  } while (i--);

  swap_conditional(nqx, nqpqx, bit);
  swap_conditional(nqz, nqpqz, bit);

  crecip(zmone, nqz);
  fmul(nqz, nqx, zmone);
  fcontract(mypublic, nqz);
}


int
curve25519_donna(u8 *mypublic, const u8 *secret, const u8 *basepoint) {
  u8 e[32];
  felem i;

  for (i = 0;i < 32;++i) e[i] = secret[i];
  e[0] &= 248;
  e[31] &= 127;
  e[31] |= 64;
  curve25519_scalarmult(mypublic, e, basepoint);
  return 0;
}
