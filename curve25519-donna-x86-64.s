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

.globl fmul
.globl fsquare
.globl fexpand
.globl fcontract
.globl freduce_coefficients
.globl fscalar
.globl fdifference_backwards

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

# Bottom 50-bits set
mov $0x7ffffffffffff,%rbx

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

CARRYCHAIN(rdi,rsi,r9,r8)
CARRYCHAIN(r9,r8,r11,r10)
CARRYCHAIN(r11,r10,r13,r12)
CARRYCHAIN(r13,r12,r15,r14)

# CARRYCHAIN(r15,r14,rdi,rsi)

mov %r14,%rax
shr $51,%r14
shl $13,%r15
or %r14,%r15

jz carrydone

mov $19,%r14
imul %r14,%r15
add %r15,%rsi
adc $0,%rdi
xor %r15,%r15
mov %rax,%r14
and %rbx,%r14

jmp coeffreduction

carrydone:

# write out results, which are in rsi, r8, r10, r12, rax
# output pointer is on top of the stack

pop %rdi

mov %rsi,(%rdi)
mov %r8,8(%rdi)
mov %r10,16(%rdi)
mov %r12,24(%rdi)
mov %rax,32(%rdi)

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

NEGCHAIN(rax,r8)
NEGCHAIN(r8,r9)
NEGCHAIN(r9,r10)
NEGCHAIN(r10,r11)

mov %r11,%rcx
sar $63,%rcx
mov %rcx,%rsi
and %rdx,%rcx
add %rcx,%r11
and $19,%rsi
sub %rsi,%rax

js fdifference_backwards_loop

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

carrychain_:

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
jz carrydone_
mov $19,%rdx
mul %rdx
add %rax,%r8
and %rcx,%r12

jmp carrychain_

carrydone_:

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
# fcontract - convert 5 uint64_t's to a packed (32 byte) representation
#
# Registers: RDI: (output) pointer to uint8_t[32]
#            RSI: (input) pointer to uint64_t[5]
################################################################################
fcontract:

mov (%rsi),%rax
mov 8(%rsi),%rdx
mov 16(%rsi),%r8
mov 24(%rsi),%r9
mov 32(%rsi),%r10

mov %rdx,%rcx
shl $51,%rcx
or %rcx,%rax
mov %rax,(%rdi)

shr $13,%rdx
mov %r8,%rcx
shl $38,%rcx
or %rcx,%rdx
mov %rdx,8(%rdi)

shr $26,%r8
mov %r9,%rcx
shl $25,%rcx
or %rcx,%r8
mov %r8,16(%rdi)

shr $39,%r9
shl $12,%r10
or %r10,%r9
mov %r9,24(%rdi)

ret
