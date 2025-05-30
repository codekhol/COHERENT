//////////
/ libc/crt/i386/frexp.s
/ i386 C runtime library.
/ IEEE software floating point support.
/ i386 IEEE software floating point library.
/ frexp()
//////////

//////////
/ double
/ frexp(real, ep) double real; int *ep;
/
/ Store exponent e through ep and return mantissa m,
/ where real = m * 2^e, 1/2 <= |m| < 1.
//////////

	.globl	frexp

real	.equ	4
ep	.equ	real+8
EXPMASK	.equ	0x7FF00000
DEFEXP	.equ	1022

frexp:
	movl	%edx, real+4(%esp)	/ real sign/exp to EDX
	movl	%ecx, $EXPMASK
	andl	%ecx, %edx		/ extract exponent bits in ECX
	jnz	?L0			/ watch for -0.0
	movl	%edx, %ecx		/ exponent 0, zap sign bit in case -0.0

?L0:
	xorl	%edx, %ecx		/ mask off exponent in EDX
	orl	%edx, $DEFEXP<<20	/ and replace with new exponent
	shrl	%ecx, $20		/ unshifted exponent in ECX
	subl	%ecx, $DEFEXP		/ unbiased
	movl	%eax, ep(%esp)		/ ep to EAX
	movl	(%eax), %ecx		/ store exponent
	movl	%eax, real(%esp)	/ fetch low mantissa dword to EAX
	ret

/ end of libc/crt/i386/frexp.s
