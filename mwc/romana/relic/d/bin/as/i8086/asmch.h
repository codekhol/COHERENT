/*
 * Machine specific header file.
 * Intel iAPX-86.
 * Small addressing model.
 */
#define	HEX	1
#define	LOHI	1
#define	CLIST	BLIST
#define	CPU	"Intel iAPX-86"
#define MINIT   1
#define	NB	128

/*
 * Formats and things for
 * the part of the assembler that
 * makes the listings.
 */
#define	NBOL	8
#define	ADRFMT	"   %04x"
#define	BFMT	" %02x"
#define	WFMT	"  %04x"
#define	SKIP	"   "

typedef	unsigned address;

#define	fbyte(x)	(((int) x)&0xFF)
#define	sbyte(x)	((((int) x)>>8)&0xFF)
#define locrup(x)       (((unsigned)(x)+01)&~01)

/*
 * Symbol flags.
 */
#define	S_OBL	0200		/* w:s literal usable */
#define	S_NW	0201		/* Floating point no wait ops. */
#define	S_FIX	0202		/* Kludge for fdiv, fdivr, fsub, fsubr. */

/*
 * Symbol kinds.
 * Basicly one per type of
 * operation.
 */
#define	S_INH	50		/* One byte */
#define	S_INT	51		/* Interrupt */
#define	S_JMP	52		/* Jumps */
#define	S_CALL	53		/* Call */
#define	S_XOP	54		/* Xcall and xjmp */
#define	S_RET	55		/* Return */
#define	S_MUL	56		/* Mul, div */
#define	S_MULB	57		/* Ditto, but byte op. */
#define	S_REL	58		/* Relative, no adjust (jcxz) */
#define	S_DOP	59		/* Doubles */
#define	S_IN	60		/* in */
#define	S_PUSH	61		/* Push/pop */
#define	S_SHL	62		/* Shift */
#define	S_MOV	63		/* Moves */
#define	S_XCHG	64		/* Exchanges */
#define	S_IJMP	65		/* Icall and ijmp */
#define	S_LEA	67		/* Load address */
#define	S_SHLB	68		/* Shifts, byte */
#define	S_OVER	69		/* Overide prefix */
#define	S_SOP	70		/* Single op. */
#define	S_SOPB	71		/* Ditto, but bytes */
#define	S_ESC	73		/* Escape */
#define	S_EVEN	74		/* .even */
#define	S_ODD	75		/* .odd */
#define	S_SYS	76		/* sys */
#define S_OUT	77		/* out */
#define S_FP_F	78		/* Floating fixed format */
#define S_FP_S	79		/* Floating stack format */
#define S_FP_SP	80		/* Floating stack pop */
#define S_FP_S1	81		/* Floating stack single operand format */
#define S_FP_M	82		/* Floating memory format */
#define	S_PROT0	83		/* Protection Control 0 */
#define	S_PROT1	84		/* Protection Control 1 */
#define	S_PROTR 85		/* Protection Control to register */
#define	S_ENTER 86		/* Enter procedure */

/*
 * Register names.
 * The low 3 bit hold the register
 * number, as coded in the type.
 * The upper bits hold the register's
 * type (0 => word, 1 => byte, 2 => seg
 * and 3 => flags).
 * This tracks the BR, WR and SR mode
 * definitions.
 */
#define	AX	0
#define	CX	1
#define	DX	2
#define	BX	3
#define	SP	4
#define	BP	5
#define	SI	6
#define	DI	7

#define	AL	010
#define	CL	011
#define	DL	012
#define	BL	013
#define	AH	014
#define	CH	015
#define	DH	016
#define	BH	017

#define	ES	020
#define	CS	021
#define	SS	022
#define	DS	023

#define ST	040
#define ST1	041
#define ST2	042
#define ST3	043
#define ST4	044
#define ST5	045
#define ST6	046
#define ST7	047

/*
 * Address modes.
 * These go in the e_mode field of
 * and expression. Basicly the bottom 3
 * bits go in the instruction, and the
 * rest express the mode.
 */
#define	WR	(0<<3)		/* Word register */
#define	BR	(1<<3)		/* Byte register */
#define	SEGR	(2<<3)		/* Segment register */
#define	IM	(4<<3)		/* Immediate */
#define	DIR	(5<<3)		/* Direct */
#define	IDX	(6<<3)		/* (dx) */
#define	ICL	(7<<3)		/* (cl) */
#define	X	(8<<3)		/* Index */

#define	NONE	(-1)		/* No legal mode */
#define	MMASK	0170		/* Mode mask */
#define	RMASK	07		/* Register mask */

#define	reg(x)	(x&RMASK)
#define	mof(x)	((x).e_mode&MMASK)
#define	rof(x)	((x).e_mode&RMASK)

/*
 * Other magic.
 */
#define	MULDIV	0366
#define	INCDEC	0376
#define	NOTNEG	0366
#define	IDREG	0100
#define	AAM	0324
#define	AAD	0325
#define	APB	0012
#define FWAIT	0233
#define FNOP	0220
#define	S	02
#define	SEGPFX	0046
#define	SHL	0320
#define	SHLI	0300
#define	TESTB	0204
#define	TESTAB	0250
#define	TESTMB	0366
#define	TESTW	0205
#define	TESTAW	0251
#define	TESTMW	0367
#define	XCHGR	0220
#define	W	01
#define	D	02
#define	V	02
#define	CMPB	0246
#define	CMPW	0247
#define	DXOP	0202
#define	XOP	0377
#define	DCALL	0350
#define	IJORC	0377
#define	LJMP	0351
#define	JMP	0353
#define	DJMP	0351
#define	INT3	0314
#define	INT	0315
#define	PUSHSR	0006
#define	PUSHR	0120
#define	PUSHIW	0150
#define	PUSHIB	0152
#define	PUSHF	0234
#define	PUSH	0377
#define	POPSR	0007
#define	POPR	0130
#define	POPF	0235
#define	POP	0217
#define	ESC	0330
#define	MOVB	0244
#define	MOVSEG	0214
#define	MVIB	0260
#define	MVIW	0270
#define	MVI	0306
#define	MOVAM	0242
#define	MOVMA	0240
#define	ARPL	0143
#define	CLTS	0006
#define	ENTER	0310

/* Floating point two-byte opcodes. */
#define	FOP(x,y)	(((x)<<8)|(y))
#define	BYTE1(x)	((x)>>8)
#define	BYTE2(x)	((x)&0xFF)
