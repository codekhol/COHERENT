//////////
/ i8086 C string library.
/ strstr()
/ ANSI 4.11.5.7.
//////////

//////////
/ char *
/ strstr(s1, s2)
/ char *s1, *s2;
/
/ Return the first occurance in s1 of s2.
/ If s2 is empty, return s1.
//////////

#include <larges.h>

s1	=	LEFTARG
s2	=	s1+DPL

	Enter(strstr_)
	Lds	bx, s1(bp)	/ s1 address to DS:BX
	Les	di, s2(bp)	/ s2 address to ES:DI
	mov	dx, di		/ Save s2 start in DX
	mov	cx, $-1		/ Max count to CX
	sub	ax, ax
	cld
	repne
	scasb			/ Scan to NUL
	dec	di		/ Back up to match
	mov	ax, di
	sub	ax, dx		/ &NUL - &start = length of s2 to AX
	je	3f		/ s2 is empty, just return s1

1:	mov	cx, ax		/ Count to CX
	mov	di, dx		/ s2 address to ES:DI
	mov	si, bx		/ Current location within s1 to DS:SI
	repe
	cmpsb			/ Compare entire s2 to current s1
	je	3f		/ Success, return current location
	inc	bx		/ Mismatched, try next s1 location
	cmpb	-1(si), $0	/ Check if end of s1 reached
	jne	1b		/ No, keep trying

2:	sub	ax, ax		/ Failure, return NULL
#ifdef	LARGEDATA
	mov	dx, ax
#endif
	jmp	4f

3:	mov	ax, bx		/ Success, return pointer on s1
#ifdef	LARGEDATA
	mov	dx, ds
#endif

4:	Leave
