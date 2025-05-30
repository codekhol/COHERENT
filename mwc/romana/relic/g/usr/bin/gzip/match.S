/* match.s -- optional optimized asm version of longest match in deflate.c
 * Copyright (C) 1992-1993 Jean-loup Gailly
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License, see the file COPYING.
 */
 
/* $Id: match.S,v 0.9 1993/02/24 18:23:13 jloup Exp $ */

/* Preprocess with -DNO_UNDERLINE if your C compiler does not prefix
 * external symbols with an underline character '_'.
 */
#ifdef NO_UNDERLINE
#  define _prev             prev
#  define _window           window
#  define _match_start	    match_start
#  define _prev_length	    prev_length
#  define _good_match	    good_match
#  define _nice_match	    nice_match
#  define _strstart	    strstart
#  define _max_chain_length max_chain_length

#  define _match_init       match_init
#  define _longest_match    longest_match
#endif

#ifdef DYN_ALLOC
  error: DYN_ALLOC not yet supported in match.s
#endif

#if defined(i386) || defined(_I386)

/* This version is for 386 Unix or OS/2 in 32 bit mode.
 * Warning: it uses the AT&T syntax: mov source,dest
 * This file is only optional. If you want to force the C version,
 * add -DNO_ASM to CFLAGS in Makefile and set OBJA to an empty string.
 * If you have reduced WSIZE in gzip.h, then change its value below.
 * This version assumes static allocation of the arrays (-DDYN_ALLOC not used).
 */

	.file   "match.S"

#define MAX_MATCH 	258
#define MAX_MATCH2      128     /* MAX_MATCH/2-1 */
#define MIN_MATCH	3
#define WSIZE 		32768
#define MAX_DIST 	WSIZE - MAX_MATCH - MIN_MATCH - 1

	.globl	_match_init
	.globl  _longest_match

	.text

_match_init:
	ret

/*-----------------------------------------------------------------------
 * Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to prev_length are discarded,
 * in which case the result is equal to prev_length and match_start is
 * garbage.
 * IN assertions: cur_match is the head of the hash chain for the current
 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
 */

_longest_match:	/* int longest_match(cur_match) */

#define cur_match   20(%esp)
     /* return address */               /* esp+16 */
        push    %ebp                    /* esp+12 */
        push    %edi                    /* esp+8  */
	push	%esi                    /* esp+4  */
	push    %ebx			/* esp    */

/*
 *      match        equ esi
 *      scan         equ edi
 *      chain_length equ ebp
 *      best_len     equ ebx
 *      limit        equ edx
 */
	mov 	cur_match,%esi
        mov 	_max_chain_length,%ebp /* chain_length = max_chain_length */
	mov 	_strstart,%edi
	mov 	%edi,%edx
	sub	$ MAX_DIST,%edx        /* limit = strstart-MAX_DIST */
	jae	limit_ok
	sub	%edx,%edx              /* limit = NIL */
limit_ok:
        add	$ _window+2,%edi       /* edi = offset(window+strstart+2) */
        mov	_prev_length,%ebx      /* best_len = prev_length */
        movw 	-3(%ebx,%edi),%ax      /* ax = scan[best_len-1..best_len] */
        movw 	-2(%edi),%cx           /* cx = scan[0..1] */
	cmp	_good_match,%ebx       /* do we have a good match already? */
        jb      do_scan
	shr 	$2,%ebp                /* chain_length >>= 2 */
        jmp     do_scan

        .align  4
long_loop:
/* at this point, edi == scan+2, esi == cur_match */
        movw	-3(%ebx,%edi),%ax       /* ax = scan[best_len-1..best_len] */
        movw     -2(%edi),%cx           /* cx = scan[0..1] */
short_loop:
/*
 * at this point, di == scan+2, si == cur_match,
 * ax = scan[best_len-1..best_len] and cx = scan[0..1]
 */
        and     $ WSIZE-1, %esi
        movw    _prev(%esi,%esi),%si    /* cur_match = prev[cur_match] */
                                        /* top word of esi is still 0 */
        cmp     %edx,%esi		/* cur_match <= limit ? */
        jbe     the_end
        dec     %ebp                    /* --chain_length */
        jz      the_end
do_scan:
        cmpw    _window-1(%ebx,%esi),%ax/* check match at best_len-1 */
        jne     short_loop
        cmpw    _window(%esi),%cx       /* check min_match_length match */
        jne     short_loop

        lea     _window+2(%esi),%esi    /* si = match */
        mov     %edi,%eax               /* ax = scan+2 */
        mov 	$ MAX_MATCH2,%ecx       /* scan for at most MAX_MATCH bytes */
        rep;    cmpsw                   /* loop until mismatch */
        je      maxmatch                /* match of length MAX_MATCH? */
mismatch:
        movb    -2(%edi),%cl        /* mismatch on first or second byte? */
        subb    -2(%esi),%cl        /* cl = 0 if first bytes equal */
        xchg    %edi,%eax           /* edi = scan+2, eax = end of scan */
        sub     %edi,%eax           /* eax = len */
	sub	%eax,%esi           /* esi = cur_match + 2 + offset(window) */
	sub	$ _window+2,%esi    /* esi = cur_match */
        subb    $1,%cl              /* set carry if cl == 0 (cannot use DEC) */
        adc     $0,%eax             /* eax = carry ? len+1 : len */
        cmp     %ebx,%eax           /* len > best_len ? */
        jle     long_loop
        mov     %esi,_match_start       /* match_start = cur_match */
        mov     %eax,%ebx               /* ebx = best_len = len */
        cmp     _nice_match,%eax        /* len >= nice_match ? */
        jl      long_loop
the_end:
        mov     %ebx,%eax               /* result = eax = best_len */
	pop     %ebx
        pop     %esi
        pop     %edi
        pop     %ebp
        ret
maxmatch:
        cmpsb
        jmp     mismatch

#else
#if defined(m68k) || defined(mc68020)

/* 68020 version, written by Francesco Potorti` <pot@cnuce.cnr.it> */

#ifdef m68k  /* Try 'delta' style */

#  define GLOBAL(symbol)	global	symbol
#  define TEXT			text
#  define FILE(filename)	file	filename
#  define invert_maybe(src,dst)	dst,src
#  define imm(data)		&data
#  define reg(register)		%register

#  define addl			add.l
#  define addql			addq.l
#  define blos			blo.b
#  define bhis			bhi.b
#  define bras			bra.b
#  define clrl			clr.l
#  define cmpmb			cmpm.b
#  define cmpw			cmp.w
#  define cmpl			cmp.l
#  define lslw			lsl.w
#  define lsrl			lsr.l
#  define movel			move.l
#  define movew			move.w
#  define moveml		movem.l
#  define subl			sub.l
#  define subw			sub.w
#  define subql			subq.l

#  define IndBase(bd,An)	(bd,An)
#  define IndBaseNdxl(bd,An,Xn)	(bd,An,Xn.l)
#  define IndBaseNdxw(bd,An,Xn)	(bd,An,Xn.w)
#  define predec(An)		-(An)
#  define postinc(An)		(An)+

#else /* Try Sun 3 or Amiga style */

#  define GLOBAL(symbol)	.globl	symbol
#  define TEXT			.text
#  define FILE(filename)	.even
#  define invert_maybe(src,dst)	src,dst
#  ifdef sun
#    define imm(data)		#data
#  else
#    define imm(data)		\#data
#  endif
#  define reg(register)		register

#  define blos			bcss
#  ifdef sun
#    define movel		movl
#    define movew		movw
#  endif
#  define IndBase(bd,An)	An@(bd)
#  define IndBaseNdxl(bd,An,Xn)	An@(bd,Xn:l)
#  define IndBaseNdxw(bd,An,Xn)	An@(bd,Xn:w)
#  define predec(An)		An@-
#  define postinc(An)		An@+

#endif	/* styles */

#define Best_Len	reg(d0)		/* unsigned */
#define Cur_Match	reg(d1)		/* Ipos */
#define Loop_Counter	reg(d2)		/* int */
#define Scan_Start	reg(d3)		/* unsigned short */
#define Scan_End	reg(d4)		/* unsigned short */
#define Limit		reg(d5)		/* IPos */
#define Chain_Length	reg(d6)		/* unsigned */
#define Scan		reg(a0)		/* *uch */
#define Match		reg(a1)		/* *uch */
#define Prev_Address	reg(a2)		/* *Pos */
#define Scan_Ini	reg(a3)		/* *uch */
#define Match_Ini	reg(a4)		/* *uch */
#define Stack_Pointer	reg(sp)

#define MAX_MATCH 	258
#define MIN_MATCH	3
#define WSIZE		32768
#define MAX_DIST 	(WSIZE - MAX_MATCH - MIN_MATCH - 1)

	GLOBAL	(_match_init)
	GLOBAL	(_longest_match)

	TEXT

	FILE	("match.S")

_match_init:
	rts

/*-----------------------------------------------------------------------
 * Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to prev_length are discarded,
 * in which case the result is equal to prev_length and match_start is
 * garbage.
 * IN assertions: cur_match is the head of the hash chain for the current
 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
 */

/* int longest_match (cur_match) */
#define pushreg	15928		/* d2-d6/a2-a4 */
#define popreg	7292

_longest_match:
	movel	IndBase(4,Stack_Pointer),Cur_Match
	moveml	imm(pushreg),predec(Stack_Pointer)
	movel	_max_chain_length,Chain_Length
	movel	_prev_length,Best_Len
	movel	imm(_prev),Prev_Address
	movel	imm(_window+MIN_MATCH),Match_Ini
	movel	_strstart,Limit
	movel	Match_Ini,Scan_Ini
	addl	Limit,Scan_Ini
	subw	imm(MAX_DIST),Limit
	bhis	L__limit_ok
	clrl	Limit
L__limit_ok:
	cmpl	invert_maybe(_good_match,Best_Len)
	blos	L__length_ok
	lsrl	imm(2),Chain_Length
L__length_ok:
	subql	imm(1),Chain_Length
	movew	IndBase(-MIN_MATCH,Scan_Ini),Scan_Start
	movew	IndBaseNdxw(-MIN_MATCH-1,Scan_Ini,Best_Len),Scan_End
	bras	L__do_scan

L__long_loop:
	movew	IndBaseNdxw(-MIN_MATCH-1,Scan_Ini,Best_Len),Scan_End

L__short_loop:
	lslw	imm(1),Cur_Match
	movew	IndBaseNdxl(0,Prev_Address,Cur_Match),Cur_Match
	cmpw	invert_maybe(Limit,Cur_Match)
	dbls	Chain_Length,L__do_scan
	bras	L__return

L__do_scan:
	movel	Match_Ini,Match
	addl	Cur_Match,Match
	cmpw	invert_maybe(IndBaseNdxw(-MIN_MATCH-1,Match,Best_Len),Scan_End)
	bne	L__short_loop
	cmpw	invert_maybe(IndBase(-MIN_MATCH,Match),Scan_Start)
	bne	L__short_loop

	movew	imm((MAX_MATCH-MIN_MATCH+1)-1),Loop_Counter
	movel	Scan_Ini,Scan
L__scan_loop:
	cmpmb	postinc(Match),postinc(Scan)
	dbne	Loop_Counter,L__scan_loop

	subl	Scan_Ini,Scan
	addql	imm(MIN_MATCH-1),Scan
	cmpl	invert_maybe(Best_Len,Scan)
	bls	L__short_loop
	movel	Scan,Best_Len
	movel	Cur_Match,_match_start
	cmpl	invert_maybe(_nice_match,Best_Len)
	blos	L__long_loop
L__return:
	moveml	postinc(Stack_Pointer),imm(popreg)
	rts

#else
 error: this asm version is for 386 or 68020 only
#endif /* m68k || mc68020 */
#endif /* i386 || _I386   */
