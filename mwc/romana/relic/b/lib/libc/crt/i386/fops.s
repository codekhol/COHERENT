//////////
/ libc/crt/i386/fops.s
/ i386 C runtime library.
/ IEEE software floating point support.
//////////

//////////
/ double _fadd(float f)
/ Return f + %edx:eax in %edx:%eax.
/
/ double _fdiv(float f)
/ Return f / %edx:eax in %edx:%eax.
/
/ double _fmul(float f)
/ Return f * %edx:eax in %edx:%eax.
/
/ double _fsub(float f)
/ Return f - %edx:eax in %edx:%eax.
//////////

	.globl	_fadd
	.globl	_fdiv
	.globl	_fmul
	.globl	_fsub
	.globl	_dadd
	.globl	_drdiv
	.globl	_dmul
	.globl	_drsub
	.globl	_dsub
	.globl	_dfcvt

_fadd:					/ Stack: f ra
	push	%ecx			/ f ra ECX
	mov	%ecx, $_dadd		/ function to execute fn to ECX
	jmp	.L0

_fdiv:
	push	%ecx
	mov	%ecx, $_drdiv
	jmp	.L0

_fmul:
	push	%ecx
	mov	%ecx, $_dmul
	jmp	.L0

_fsub:
	push	%ecx
	mov	%ecx, $_drsub

.L0:					/ Stack: f ra ECX; fn in ECX
	push	%edx			/ f ra ECX hi(d)
	push	%eax			/ f ra ECX hi(d) lo(d)
	movl	%eax, 16(%esp)		/ f to EAX
	call	_dfcvt			/ (double)f to EDX:EAX
	icall	%ecx			/ execute fn(d), result to EDX:EAX
	addl	%esp, $8		/ f ra ECX
	pop	%ecx			/ restore ECX
	ret				/ and return
	
/ end of libc/crt/i386/fops.s
