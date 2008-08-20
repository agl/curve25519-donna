/* 2008, Google Inc.
 * Code released into the public domain
 *
 * curve25519: Curve25519 elliptic curve, public key function
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
 *
 * You have to have read the curve25519 paper to understand this code:
 *   http://cr.yp.to/ecdh/curve25519-20060209.pdf
 *
 * DJB used limb sizes of ceil(25.5) bits in a 64-bit limb. Thus he has 10
 * limbs in a reduced coefficient form. Here, we use 51-bits in a 64-bit limb.
 * This means that we use the full 64x64->128 bit mult in x86-64 and are often
 * dealing with 128-bit values.
 *
 * Thus values are stored in arrays of 5 uint64_t's. Index 0 is the least
 * significant value. We maintain that the limbs are always positive. The only
 * place where this can change is in fdifference_backwards, so we fix up any
 * negative values by carrying between them.
 */

#include <string.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint64_t felem;

/* Sum two numbers: output += in */
static void
fsum(felem *output, const felem *in) {
  output[0] = output[0] + in[0];
  output[1] = output[1] + in[1];
  output[2] = output[2] + in[2];
  output[3] = output[3] + in[3];
  output[4] = output[4] + in[4];
}

// Externs from the .s file.
extern void fmul(felem *output, const felem *in1, const felem *in2);
extern void fsquare(felem *output, const felem *in1);
extern void fexpand(felem *ouptut, const u8 *input);
extern void fcontract(u8 *output, const felem *input);
extern void freduce_coefficients(felem *inout);
extern void fscalar(felem *output, const felem *input);
extern void fdifference_backwards(felem *output, const felem *input);

/* Input: Q, Q', Q-Q'
 * Output: 2Q, Q+Q'
 *
 *   x2 z3: long form
 *   x3 z3: long form
 *   x z: short form, destroyed
 *   xprime zprime: short form, destroyed
 *   qmqp: short form, preserved
 */
static void
fmonty(felem *x2, felem *z2,  /* output 2Q */
       felem *x3, felem *z3,  /* output Q + Q' */
       felem *x, felem *z,    /* input Q */
       felem *xprime, felem *zprime,  /* input Q' */
       const felem *qmqp /* input Q - Q' */) {
  felem origx[5], origxprime[5], zzz[5], xx[5], zz[5], xxprime[5],
        zzprime[5], zzzprime[5];

  memcpy(origx, x, 5 * sizeof(felem));
  fsum(x, z);
  fdifference_backwards(z, origx);  // does x - z

  memcpy(origxprime, xprime, sizeof(felem) * 5);
  fsum(xprime, zprime);
  fdifference_backwards(zprime, origxprime);
  fmul(xxprime, xprime, z);
  fmul(zzprime, x, zprime);
  memcpy(origxprime, xxprime, sizeof(felem) * 5);
  fsum(xxprime, zzprime);
  fdifference_backwards(zzprime, origxprime);
  fsquare(x3, xxprime);
  fmul(zzzprime, zzprime, zzprime);
  fmul(z3, zzzprime, qmqp);

  fsquare(xx, x);
  fsquare(zz, z);
  fmul(x2, xx, zz);
  fdifference_backwards(zz, xx);  // does zz = xx - zz
  fscalar(zzz, zz); // * 121665
  freduce_coefficients(zzz);
  fsum(zzz, xx);
  fmul(z2, zz, zzz);
}

/* Calculates nQ where Q is the x-coordinate of a point on the curve
 *
 *   resultx/resultz: the x coordinate of the resulting curve point (short form)
 *   n: a little endian, 32-byte number
 *   q: a point of the curve (short form)
 */
static void
cmult(felem *resultx, felem *resultz, const u8 *n, const felem *q) {
  felem a[5] = {0}, b[5] = {1}, c[5] = {1}, d[5] = {0};
  felem *nqpqx = a, *nqpqz = b, *nqx = c, *nqz = d, *t;
  felem e[5] = {0}, f[5] = {1}, g[5] = {0}, h[5] = {1};
  felem *nqpqx2 = e, *nqpqz2 = f, *nqx2 = g, *nqz2 = h;

  unsigned i, j;

  memcpy(nqpqx, q, sizeof(felem) * 5);

  for (i = 0; i < 32; ++i) {
    u8 byte = n[31 - i];
    for (j = 0; j < 8; ++j) {
      if (byte & 0x80) {
        fmonty(nqpqx2, nqpqz2,
               nqx2, nqz2,
               nqpqx, nqpqz,
               nqx, nqz,
               q);
      } else {
        fmonty(nqx2, nqz2,
               nqpqx2, nqpqz2,
               nqx, nqz,
               nqpqx, nqpqz,
               q);
      }

      t = nqx;
      nqx = nqx2;
      nqx2 = t;
      t = nqz;
      nqz = nqz2;
      nqz2 = t;
      t = nqpqx;
      nqpqx = nqpqx2;
      nqpqx2 = t;
      t = nqpqz;
      nqpqz = nqpqz2;
      nqpqz2 = t;

      byte <<= 1;
    }
  }

  memcpy(resultx, nqx, sizeof(felem) * 5);
  memcpy(resultz, nqz, sizeof(felem) * 5);
}

// -----------------------------------------------------------------------------
// Shamelessly copied from djb's code
// -----------------------------------------------------------------------------
static void
crecip(felem *out, const felem *z) {
  felem z2[5];
  felem z9[5];
  felem z11[5];
  felem z2_5_0[5];
  felem z2_10_0[5];
  felem z2_20_0[5];
  felem z2_50_0[5];
  felem z2_100_0[5];
  felem t0[5];
  felem t1[5];
  int i;

  /* 2 */ fsquare(z2,z);
  /* 4 */ fsquare(t1,z2);
  /* 8 */ fsquare(t0,t1);
  /* 9 */ fmul(z9,t0,z);
  /* 11 */ fmul(z11,z9,z2);
  /* 22 */ fsquare(t0,z11);
  /* 2^5 - 2^0 = 31 */ fmul(z2_5_0,t0,z9);

  /* 2^6 - 2^1 */ fsquare(t0,z2_5_0);
  /* 2^7 - 2^2 */ fsquare(t1,t0);
  /* 2^8 - 2^3 */ fsquare(t0,t1);
  /* 2^9 - 2^4 */ fsquare(t1,t0);
  /* 2^10 - 2^5 */ fsquare(t0,t1);
  /* 2^10 - 2^0 */ fmul(z2_10_0,t0,z2_5_0);

  /* 2^11 - 2^1 */ fsquare(t0,z2_10_0);
  /* 2^12 - 2^2 */ fsquare(t1,t0);
  /* 2^20 - 2^10 */ for (i = 2;i < 10;i += 2) { fsquare(t0,t1); fsquare(t1,t0); }
  /* 2^20 - 2^0 */ fmul(z2_20_0,t1,z2_10_0);

  /* 2^21 - 2^1 */ fsquare(t0,z2_20_0);
  /* 2^22 - 2^2 */ fsquare(t1,t0);
  /* 2^40 - 2^20 */ for (i = 2;i < 20;i += 2) { fsquare(t0,t1); fsquare(t1,t0); }
  /* 2^40 - 2^0 */ fmul(t0,t1,z2_20_0);

  /* 2^41 - 2^1 */ fsquare(t1,t0);
  /* 2^42 - 2^2 */ fsquare(t0,t1);
  /* 2^50 - 2^10 */ for (i = 2;i < 10;i += 2) { fsquare(t1,t0); fsquare(t0,t1); }
  /* 2^50 - 2^0 */ fmul(z2_50_0,t0,z2_10_0);

  /* 2^51 - 2^1 */ fsquare(t0,z2_50_0);
  /* 2^52 - 2^2 */ fsquare(t1,t0);
  /* 2^100 - 2^50 */ for (i = 2;i < 50;i += 2) { fsquare(t0,t1); fsquare(t1,t0); }
  /* 2^100 - 2^0 */ fmul(z2_100_0,t1,z2_50_0);

  /* 2^101 - 2^1 */ fsquare(t1,z2_100_0);
  /* 2^102 - 2^2 */ fsquare(t0,t1);
  /* 2^200 - 2^100 */ for (i = 2;i < 100;i += 2) { fsquare(t1,t0); fsquare(t0,t1); }
  /* 2^200 - 2^0 */ fmul(t1,t0,z2_100_0);

  /* 2^201 - 2^1 */ fsquare(t0,t1);
  /* 2^202 - 2^2 */ fsquare(t1,t0);
  /* 2^250 - 2^50 */ for (i = 2;i < 50;i += 2) { fsquare(t0,t1); fsquare(t1,t0); }
  /* 2^250 - 2^0 */ fmul(t0,t1,z2_50_0);

  /* 2^251 - 2^1 */ fsquare(t1,t0);
  /* 2^252 - 2^2 */ fsquare(t0,t1);
  /* 2^253 - 2^3 */ fsquare(t1,t0);
  /* 2^254 - 2^4 */ fsquare(t0,t1);
  /* 2^255 - 2^5 */ fsquare(t1,t0);
  /* 2^255 - 21 */ fmul(out,t1,z11);
}

void
curve25519_donna(u8 *mypublic, const u8 *secret, const u8 *basepoint) {
  felem bp[5], x[5], z[5], zmone[5];
  fexpand(bp, basepoint);
  cmult(x, z, secret, bp);
  crecip(zmone, z);
  fmul(z, x, zmone);
  fcontract(mypublic, z);
}
