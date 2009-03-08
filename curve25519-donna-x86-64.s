# 2008, Google Inc.
# All rights reserved.
#
# Code released into the public domain

################################################################################
# curve25519-donna.s - an x86-64 bit implementation of curve25519. See the
# comments at the top of curve25519-donna.c
#
# Adam Langley <agl@imperialviolet.org>
#
# Derived from public domain C code by Daniel J. Bernstein <djb@cr.yp.to>
#
# More information about curve25519 can be found here
#   http://cr.yp.to/ecdh.html
################################################################################
.text

.extern fmonty

.globl fmul
.globl fsquare
.globl fexpand
.globl freduce_coefficients
.globl fscalar
.globl fdifference_backwards
.globl cmult

################################################################################
# fmul - multiply two 256-bit numbers
# Registers: RDI (output): uint64_t[5] product
#            RSI (input): uint64_t[5] input 1
#            RDX (input): uint64_t[5] input 2
################################################################################
fmul:

# Input pointers: rdi (output), rsi (in1), rdx (in2)
# Spill: rdi, rbx, r12..15

push %rbx
push %r12
push %r13
push %r14
push %r15
push %rdi

# Load 5 64-bit values from *rsi to rsi, r8..11

mov %rsi,%rcx
mov (%rcx),%rsi
mov 8(%rcx),%r8
mov 16(%rcx),%r9
mov 24(%rcx),%r10
mov 32(%rcx),%r11

# Load 5 64-bit values from *rdx to rdi, r12..15

mov (%rdx),%rdi
mov 8(%rdx),%r12
mov 16(%rdx),%r13
mov 24(%rdx),%r14
mov 32(%rdx),%r15

# We are going to perform a polynomial multiplication of two, five element
# polynomials. I and J and the polynomials and I2 would be the coefficient of
# x^2 etc.

#define I0 %rsi
#define I1 %r8
#define I2 %r9
#define I3 %r10
#define I4 %r11

#define J0 %rdi
#define J1 %r12
#define J2 %r13
#define J3 %r14
#define J4 %r15

#define MUL(a, b) \
mov a,%rax ; \
mul b

# We accumululate results in RCX:RBX

#define FIRSTMUL(a,b) \
MUL(a,b) ; \
mov %rax,%rbx ; \
mov %rdx,%rcx

#define MAC(a,b) \
MUL (a,b) ; \
add %rax,%rbx ; \
adc %rdx,%rcx

# p[0] = i[0] * j[0]
# p[0] stored in xmm0, xmm1

MUL(I0,J0)
movq %rax,%xmm0
movq %rdx,%xmm1

# p[1] = i[0] * j[1] + i[1] * j[0]

FIRSTMUL(I0,J1)
MAC(I1, J0)

movq %rbx,%xmm2
movq %rcx,%xmm3

# p[2] = i[1] * j[1] + i[0] * j[2] + i[2] * j[0]

FIRSTMUL(I1,J1)
MAC(I0,J2)
MAC(I2,J0)

movq %rbx,%xmm4
movq %rcx,%xmm5

# p[3]

FIRSTMUL(I0,J3)
MAC(I3,J0)
MAC(I1,J2)
MAC(I2,J1)

movq %rbx,%xmm6
movq %rcx,%xmm7

# p[4]

FIRSTMUL(I0,J4)
MAC(I4,J0)
MAC(I3,J1)
MAC(I1,J3)
MAC(I2,J2)

movq %rbx,%xmm8
movq %rcx,%xmm9

# p[5]

FIRSTMUL(I4,J1)
MAC(I1,J4)
MAC(I2,J3)
MAC(I3,J2)

movq %rbx,%xmm10
movq %rcx,%xmm11

# p[6]

FIRSTMUL(I4,J2)
MAC(I2,J4)
MAC(I3,J3)

movq %rbx,%xmm12
movq %rcx,%xmm13

# p[7]

FIRSTMUL(I3,J4)
MAC(I4,J3)

movq %rbx,%xmm14
movq %rcx,%xmm15

# p[8], keeping it in RDX:RAX

MUL(I4,J4)

donna_reduce:
# We done with the original inputs now, so we start reusing them
# At this point we have a degree 8 resulting polynomial and we need to reduce
# mod 2**255-19. Since 2**255 is in our polynomial, we can multiply the
# coefficients of the higher powers and add them to the lower powers. The limb
# size (51-bits) is chosen to avoid overflows.

mov $19,%r15

# p[8] *= 19, store in R13:R12

mov %rdx,%r13
mul %r15
imul %r15,%r13
add %rdx,%r13
mov %rax,%r12

# p[3] += p[8] * 19

movq %xmm7,%rcx
movq %xmm6,%rbx
add %rbx,%r12
adc %rcx,%r13

#define MUL19ADD(srchi,srclo,desthi,destlo,src2hi,src2lo) \
movq %srclo,%rax ; \
mul %r15 ; \
movq %srchi,%desthi ; \
imul %r15,%desthi ; \
add %rdx,%desthi ; \
mov %rax,%destlo ; \
movq %src2hi,%rcx ; \
movq %src2lo,%rbx ; \
add %rbx,%destlo ; \
adc %rcx,%desthi

# p[2] += p[7] * 19, store in R11:R10

MUL19ADD(xmm15,xmm14,r11,r10,xmm5,xmm4)

# p[1] += p[6] * 19, store in R9:R8

MUL19ADD(xmm13,xmm12,r9,r8,xmm3,xmm2)

# p[0] += p[5] * 19, store in RDI:RSI

MUL19ADD(xmm11,xmm10,rdi,rsi,xmm1,xmm0)

# p[4], store in R15:R14

movq %xmm9,%r15
movq %xmm8,%r14

# Coefficient reduction

# Bottom 51-bits set
mov $0x7ffffffffffff,%rbx
mov $19,%rcx

coeffreduction:

# The carry chain takes the excess bits from a 128-bit result (excess are
# anything over 51-bits and above) and adds them to the next value. If the top
# value spills over, we reduce mod 2**255-19 again by multipling by 19 and
# adding onto the bottom.

#define CARRYCHAIN(srchi,srclo,desthi,destlo) \
mov %srclo,%rax ; \
shr $51,%srclo ; \
shl $13,%srchi ; \
or %srclo,%srchi ; \
add %srchi,%destlo ; \
adc $0,%desthi ; \
xor %srchi,%srchi ; \
mov %rax,%srclo ; \
and %rbx,%srclo

#define CARRYCHAIN19(srchi,srclo,desthi,destlo) \
mov %srclo,%rax ; \
shr $51,%srclo ; \
shl $13,%srchi ; \
or %srclo,%srchi ; \
imul $19,%srchi ; \
add %srchi,%destlo ; \
adc $0,%desthi ; \
xor %srchi,%srchi ; \
mov %rax,%srclo ; \
and %rbx,%srclo

CARRYCHAIN(rdi,rsi,r9,r8)
CARRYCHAIN(r9,r8,r11,r10)
CARRYCHAIN(r11,r10,r13,r12)
CARRYCHAIN(r13,r12,r15,r14)
CARRYCHAIN19(r15,r14,rdi,rsi)
CARRYCHAIN(rdi,rsi,r9,r8)

# write out results, which are in rsi, r8, r10, r12, rax
# output pointer is on top of the stack

pop %rdi

mov %rsi,(%rdi)
mov %r8,8(%rdi)
mov %r10,16(%rdi)
mov %r12,24(%rdi)
mov %r14,32(%rdi)

pop %r15
pop %r14
pop %r13
pop %r12
pop %rbx

ret

################################################################################
# fsquare - square a 256-bit number
# Registers: RDI (output): uint64_t[5] product
#            RSI (input): uint64_t[5] input
#
# This is very similar to fmul, above, however when squaring a number we can
# save some multiplications and replace them with doublings.
################################################################################

fsquare:

push %rbx
push %r12
push %r13
push %r14
push %r15
push %rdi

# Load 5 64-bit values from *rsi to rsi, r8..11

mov %rsi,%rcx
mov (%rcx),%rsi
mov 8(%rcx),%r8
mov 16(%rcx),%r9
mov 24(%rcx),%r10
mov 32(%rcx),%r11

# p[0] = i[0] * j[0]
# p[0] stored in xmm0, xmm1

MUL(I0,I0)
movq %rax,%xmm0
movq %rdx,%xmm1

# p[1] = i[0] * j[1] + i[1] * j[0]

MUL(I0,I1)
sal $1,%rax
rcl $1,%rdx

movq %rax,%xmm2
movq %rdx,%xmm3

# p[2] = i[1] * j[1] + i[0] * j[2] + i[2] * j[0]

#define FIRSTMULDOUBLE(x,y) \
FIRSTMUL(x,y) ; \
sal $1,%rbx ; \
rcl $1,%rcx

#define DOUBLEMAC(x,y) \
MUL(x,y) ; \
sal $1,%rax ; \
rcl $1,%rdx ; \
add %rax,%rbx ; \
adc %rdx,%rcx

FIRSTMUL(I1,I1)
DOUBLEMAC(I0,I2)

movq %rbx,%xmm4
movq %rcx,%xmm5

# p[3]

FIRSTMULDOUBLE(I0,I3)
DOUBLEMAC(I1,I2)

movq %rbx,%xmm6
movq %rcx,%xmm7

# p[4]

FIRSTMULDOUBLE(I0,I4)
DOUBLEMAC(I3,I1)
MAC(I2,I2)

movq %rbx,%xmm8
movq %rcx,%xmm9

# p[5]

FIRSTMULDOUBLE(I4,I1)
DOUBLEMAC(I2,I3)

movq %rbx,%xmm10
movq %rcx,%xmm11

# p[6]

FIRSTMULDOUBLE(I4,I2)
MAC(I3,I3)

movq %rbx,%xmm12
movq %rcx,%xmm13

# p[7]

MUL(I3,I4)
sal $1,%rax
rcl $1,%rdx

movq %rax,%xmm14
movq %rdx,%xmm15

# p[8], keeping it in RDX:RAX

MUL(I4,I4)

jmp donna_reduce

################################################################################
# fdifference_backwards - set output to in - output (note order)
#
# /*
# static const felem twotothe51 = (1ul << 51);
#
# static void fdifference_backwards(felem *ooutput, const felem *oin) {
#   const int64_t *in = (const int64_t *) oin;
#   int64_t *out = (int64_t *) ooutput;
#
#   out[0] = in[0] - out[0];
#   out[1] = in[1] - out[1];
#   out[2] = in[2] - out[2];
#   out[3] = in[3] - out[3];
#   out[4] = in[4] - out[4];
#
#   do {
#     if (out[0] < 0) {
#       out[0] += twotothe51;
#       out[1]--;
#     }
#     if (out[1] < 0) {
#       out[1] += twotothe51;
#       out[2]--;
#     }
#     if (out[2] < 0) {
#       out[2] += twotothe51;
#       out[3]--;
#     }
#     if (out[3] < 0) {
#       out[3] += twotothe51;
#       out[4]--;
#     }
#     if (out[4] < 0) {
#       out[4] += twotothe51;
#       out[0] -= 19;
#     }
#   } while (out[0] < 0);
# }
# */
################################################################################

fdifference_backwards:

mov (%rsi),%rax
mov 8(%rsi),%r8
mov 16(%rsi),%r9
mov 24(%rsi),%r10
mov 32(%rsi),%r11

sub (%rdi),%rax
sub 8(%rdi),%r8
sub 16(%rdi),%r9
sub 24(%rdi),%r10
sub 32(%rdi),%r11

# 2**51
mov $0x8000000000000,%rdx

fdifference_backwards_loop:

# In the C code, above, we have lots of branches. We replace these branches
# with a trick. An arithmetic shift right of 63-bits turns a positive number to
# 0, but a negative number turns to all ones. This gives us a bit-mask that we
# can AND against to add 2**51, conditionally.

#define NEGCHAIN(src,dest) \
mov %src,%rcx ; \
sar $63,%rcx ; \
and %rdx,%rcx ; \
add %rcx,%src ; \
shr $51,%rcx ; \
sub %rcx,%dest

#define NEGCHAIN19(src,dest) \
mov %src,%rcx ; \
sar $63,%rcx ; \
mov %rcx,%rsi ; \
and %rdx,%rcx ; \
add %rcx,%src ; \
and $19,%rsi ; \
sub %rsi,%dest

NEGCHAIN(rax,r8)
NEGCHAIN(r8,r9)
NEGCHAIN(r9,r10)
NEGCHAIN(r10,r11)
NEGCHAIN19(r11,rax)
NEGCHAIN(rax,r8)
NEGCHAIN(r8,r9)
NEGCHAIN(r9,r10)
NEGCHAIN(r10,r11)

mov %rax,(%rdi)
mov %r8,8(%rdi)
mov %r9,16(%rdi)
mov %r10,24(%rdi)
mov %r11,32(%rdi)

ret


################################################################################
# fscalar - multiply by 121665
#
# Registers: RDI: (out) pointer to uint64_t[5]
#            RSI: (in) pointer to uint64_t[5]
#
# Since we only have 13-bits of space at the top of our limbs, this is a full,
# cascading multiplication.
################################################################################
fscalar:

mov $121665,%rcx

mov (%rsi),%rax
mul %rcx
shl $13,%rdx
mov %rdx,%r8
mov %rax,%r9

mov 8(%rsi),%rax
mul %rcx
add %r8,%rax
shl $13,%rdx
mov %rdx,%r8
mov %rax,8(%rdi)

mov 16(%rsi),%rax
mul %rcx
add %r8,%rax
shl $13,%rdx
mov %rdx,%r8
mov %rax,16(%rdi)

mov 24(%rsi),%rax
mul %rcx
add %r8,%rax
shl $13,%rdx
mov %rdx,%r8
mov %rax,24(%rdi)

mov 32(%rsi),%rax
mul %rcx
add %r8,%rax
mov %rax,32(%rdi)
shl $13,%rdx
mov $19,%rcx
mov %rdx,%rax
mul %rcx
add %rax,%r9
mov %r9,0(%rdi)

ret

################################################################################
# freduce_coefficients
#
# Registers: RDI: (in/out) pointer to uint64_t[5]
################################################################################
freduce_coefficients:

push %r12

mov $0x7ffffffffffff,%rcx
mov $19,%rdx

mov (%rdi),%r8
mov 8(%rdi),%r9
mov 16(%rdi),%r10
mov 24(%rdi),%r11
mov 32(%rdi),%r12

mov %r8,%rax
shr $51,%rax
add %rax,%r9
and %rcx,%r8

mov %r9,%rax
shr $51,%rax
add %rax,%r10
and %rcx,%r9

mov %r10,%rax
shr $51,%rax
add %rax,%r11
and %rcx,%r10

mov %r11,%rax
shr $51,%rax
add %rax,%r12
and %rcx,%r11

mov %r12,%rax
shr $51,%rax
imul $19,%rax
add %rax,%r8
and %rcx,%r12

mov %r8,(%rdi)
mov %r9,8(%rdi)
mov %r10,16(%rdi)
mov %r11,24(%rdi)
mov %r12,32(%rdi)

pop %r12
ret


################################################################################
# fexpand - convert a packed (32 byte) representation to 5 uint64_t's
# Registers: RDI: (output) pointer to uint64_t[5]
#            RSI: (input) pointer to uint8_t[32]
################################################################################
fexpand:

mov $0x7ffffffffffff,%rdx

mov (%rsi),%rax
and %rdx,%rax
mov %rax,(%rdi)

mov 6(%rsi),%rax
shr $3,%rax
and %rdx,%rax
mov %rax,8(%rdi)

mov 12(%rsi),%rax
shr $6,%rax
and %rdx,%rax
mov %rax,16(%rdi)

mov 19(%rsi),%rax
shr $1,%rax
and %rdx,%rax
mov %rax,24(%rdi)

mov 25(%rsi),%rax
shr $4,%rax
and %rdx,%rax
mov %rax,32(%rdi)

ret

################################################################################
# cmult - calculates nQ wher Q is the x-coordinate of a point on the curve
#
# Registers: RDI: (output) final x
#            RSI: (output) final z
#            RDX: (input) n (big-endian)
#            RCX: (input) q (big-endian)
#
 /* Calculates nQ where Q is the x-coordinate of a point on the curve
  *
  *   resultx/resultz: the x coordinate of the resulting curve point (short form)
  *   n: a little endian, 32-byte number
  *   q: a point of the curve (short form)
  */
/*
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
*/
################################################################################
cmult:

push %rbp
push %r13
push %r14

mov %rsp,%rbp
mov $63,%r8
not %r8
and %r8,%rsp

mov %rdx,%r13
mov %rcx,%r14

sub $320,%rsp

# value nQ+Q (x)
movq (%rcx),%rax
movq %rax,(%rsp)
movq 8(%rcx),%r8
movq %r8,8(%rsp)
movq 16(%rcx),%r9
movq %r9,16(%rsp)
movq 24(%rcx),%r10
movq %r10,24(%rsp)
movq 32(%rcx),%r11
movq %r11,32(%rsp)

# value nQ+Q (z)
movq $1,40(%rsp)
movq $0,48(%rsp)
movq $0,56(%rsp)
movq $0,64(%rsp)
movq $0,72(%rsp)

# value nQ (x)
movq $1,80(%rsp)
movq $0,88(%rsp)
movq $0,96(%rsp)
movq $0,104(%rsp)
movq $0,112(%rsp)

# value nQ (z)
movq $0,120(%rsp)
movq $0,128(%rsp)
movq $0,136(%rsp)
movq $0,144(%rsp)
movq $0,152(%rsp)

push %rbx
push %r12
push %r15
push %rdi
push %rsi

# The stack looks like
# (nQ)'
# (nQ+Q)'
# nQ
# nQ+Q
# saved registers  (40-bytes)   <-- %rsp
# We switch between the two banks with an offset in %r12, starting by writing
# into the prime bank and reading from the non-prime bank.
# Based on the current MSB of the operand, we flip the two values over based
# on an offset in %r8 for the first first member and %r9 for the second

mov $160,%r12
mov $32,%rbx

cmult_loop_outer:

# On entry to the loop, the word offset is kept in %rbx. We dec 8 bytes and
# then store the outer loop counter in the top 32-bits of %rbx. The inner loop
# counter is kept in %ebx

sub $8,%rbx
movq (%r13,%rbx),%r15
shl $32,%rbx

or $64,%rbx

cmult_loop_inner:

# Register allocation:
#  r11: complement r12
# Preserved by fmonty:
#  rbx: loop counters
#  r12: bank switch offset
#  r13: (input) n
#  r14: (input) q
#  r15: the current qword, getting left shifted

# We wish to test the MSB of the qword in r15. An arithmetic shift right of 63
# places turns this either into all 1's (if MSB is set) or all zeros otherwise.

mov %r15,%r8
sar $63,%r8

# Now replicate the mask to 128-bits in xmm0

movq %r8,%xmm1
movq %xmm1,%xmm0
pslldq $8,%xmm0
por %xmm1,%xmm0

# Based on that mask, we swap the contents of several arrays in a side-channel
# free manner.

# Swap two xmm registers based on a mask in xmm0. Uses xmm11 as a temporary
#define SWAPWITHMASK(a,b) \
movdqa a,%xmm11 ; \
pxor b,%xmm11 ; \
pand %xmm0,%xmm11 ; \
pxor %xmm11,a ; \
pxor %xmm11,b

# Swap the 80 byte arrays pointed to by %rdi based on the mask in
# %xmm0
#define ARRAYSWAP \
movdqa (%rdi),%xmm1 ; \
movdqa 80(%rdi),%xmm2 ; \
movdqa 16(%rdi),%xmm3 ; \
movdqa 96(%rdi),%xmm4 ; \
movdqa 32(%rdi),%xmm5 ; \
movdqa 112(%rdi),%xmm6 ; \
movdqa 48(%rdi),%xmm7 ; \
movdqa 128(%rdi),%xmm8 ; \
movdqa 64(%rdi),%xmm9 ; \
movdqa 144(%rdi),%xmm10 ; \
\
SWAPWITHMASK(%xmm1,%xmm2) ; \
SWAPWITHMASK(%xmm3,%xmm4) ; \
SWAPWITHMASK(%xmm5,%xmm6) ; \
SWAPWITHMASK(%xmm7,%xmm8) ; \
SWAPWITHMASK(%xmm9,%xmm10) ; \
\
movdqa %xmm1,(%rdi) ; \
movdqa %xmm2,80(%rdi) ; \
movdqa %xmm3,16(%rdi) ; \
movdqa %xmm4,96(%rdi) ; \
movdqa %xmm5,32(%rdi) ; \
movdqa %xmm6,112(%rdi) ; \
movdqa %xmm7,48(%rdi) ; \
movdqa %xmm8,128(%rdi) ; \
movdqa %xmm9,64(%rdi) ; \
movdqa %xmm10,144(%rdi)

mov %r12,%r11
xor $160,%r11

lea 40(%rsp,%r11),%rdi
ARRAYSWAP

mov %rdi,%rdx
lea 40(%rsp,%r12),%rdi
mov %rdi,%rsi
add $80,%rdi
mov %rdx,%rcx
add $80,%rdx
mov %r14,%r8
call fmonty

mov %r15,%r8
sar $63,%r8
movq %r8,%xmm1
movq %xmm1,%xmm0
pslldq $8,%xmm0
por %xmm1,%xmm0

lea 40(%rsp,%r12),%rdi
ARRAYSWAP

shl $1,%r15

xor $160,%r12

dec %rbx
cmp $0,%ebx
jnz cmult_loop_inner

shr $32,%rbx
cmp $0,%rbx
jnz cmult_loop_outer

pop %rsi
pop %rdi
pop %r15
pop %r12
pop %rbx

lea 80(%rsp),%r8

movq (%r8),%rax
movq %rax,(%rdi)
movq 8(%r8),%rax
movq %rax,8(%rdi)
movq 16(%r8),%rax
movq %rax,16(%rdi)
movq 24(%r8),%rax
movq %rax,24(%rdi)
movq 32(%r8),%rax
movq %rax,32(%rdi)

movq 40(%r8),%rax
movq %rax,(%rsi)
movq 48(%r8),%rax
movq %rax,8(%rsi)
movq 56(%r8),%rax
movq %rax,16(%rsi)
movq 64(%r8),%rax
movq %rax,24(%rsi)
movq 72(%r8),%rax
movq %rax,32(%rsi)

mov %rbp,%rsp
pop %r14
pop %r13
pop %rbp

ret
