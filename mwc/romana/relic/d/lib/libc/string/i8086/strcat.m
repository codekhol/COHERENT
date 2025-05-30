//////////
/ i8086 C string library.
/ strcat()
/ ANSI 4.11.3.1.
//////////

//////////
/ char *
/ strcat(To, From)
/ char *To, *From;
/
/ Append From to To.
//////////

#include <larges.h>

To	=	LEFTARG
From	=	To+DPL

	Enter(strcat_)
	Lds	si, From(bp)	/ From address to DS:SI
	Les	di, To(bp)	/ To address to ES:DI
	mov	cx, $-1		/ Max count to CX
	sub	ax, ax
	cld
	repne
	scasb			/ Find end of To
	dec	di		/ and back up to NUL

1:	lodsb			/ Fetch From char to AL
	stosb			/ and store through To
	orb	al, al
	jne	1b		/ Not done
	mov	ax, To(bp)	/ Return the destination
#if	LARGEDATA
	mov	dx, es
#endif
	Leave
