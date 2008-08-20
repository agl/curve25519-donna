.text
.globl tsc_read

tsc_read:
rdtsc
shl $32,%rdx
or %rdx,%rax
ret
