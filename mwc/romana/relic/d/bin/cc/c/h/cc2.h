/*
 * Header file for the 'direct-to-bits' code generator and optimizer.
 * This contains all of the machine independent data structures and macros. 
 * Definitions dealing with the object code are in another file.
 */

#include <stdio.h>
#include <setjmp.h>
#ifdef   vax
#include "INC$LIB:mch.h"
#include "INC$LIB:host.h"
#include "INC$LIB:ops.h"
#include "INC$LIB:var.h"
#include "INC$LIB:varmch.h"
#include "INC$LIB:opcode.h"
#include "INC$LIB:cc2mch.h"
#include "INC$LIB:stream.h"
#else
#include "mch.h"
#include "host.h"
#include "ops.h"
#include "var.h"
#include "varmch.h"
#include "opcode.h"
#include "cc2mch.h"
#include "stream.h"
#endif

/*
 * Table sizes.
 */
#define	NADDR	5			/* # of address fields */
#define	NSHASH	64			/* Symbol hash buckets */
#define	SHMASK	077			/* Mask for hashing into above */

/*
 * An array of these structures, indexed by compiler segment identifier,
 * keeps track of the current location in each segment.
 * The values of "dot" and "size" are not correct for the current segment.
 */
typedef	struct	seg	{
	ADDRESS	s_dot;			/* Current location */
	ADDRESS	s_mseek;		/* Base in memory */
	long	s_dseek;		/* Base in file */
}	SEG;

/*
 * Every symbol in the assembler symbol table looks like this.
 * Local and global symbols live in the same table.
 * A flag bit "S_LABNO" tells which variant is in use.
 */
typedef	struct	sym	{
	struct	sym	*s_fp;		/* Link */
	ADDRESS		s_value;	/* Offset into segment */
	char		s_seg;		/* Segment # */
	char		s_flag;		/* Flags */
	int		s_data;		/* Data, depends on variant */
	int		s_type;
	char		s_id[];		/* Name + null */
}	SYM;

#define	s_labno	s_data			/* Local label # */
#define	s_ref	s_data			/* Globl reference # */

#define	S_GBL	01			/* Global */
#define	S_LABNO	02			/* Has labno, not id */
#define	S_DEF	04			/* Defined */
#define	S_NFL	010			/* Not local to function */
#define	S_PUT	020			/* Debug item present */

/*
 * This structure is used to hold the address field of a machine instruction.
 * It assumes the rather weak linker model.
 */
typedef	struct	afield	{
	short	a_mode;			/* Address mode */
	SYM	*a_sp;			/* Symbol table pointer */
	ADDRESS	a_value;		/* Offset */
}	AFIELD;

/*
 * When a function is read into memory,
 * it is represented as a doubly linked list of these structures.
 * The 'i_type' field tells which variant is used.
 */
typedef	struct	INS	{
	struct	INS	*i_fp;		/* Link forward */
	struct	INS	*i_bp;		/* Link back */
	int		i_type;		/* Node type */
	union {
		int i_data[2];		/* Variant areas #1 and #2 */
		sizeof_t i_data3;
	} i_var;
	SYM		*i_sp;		/* Symbol */
	ADDRESS		i_pc;		/* Location */
	char		i_data4;	/* Variant area #3 */
	struct	INS	*i_ip;		/* Label link */
	short		i_rel;		/* JUMP relation code */	
	AFIELD		i_af[];		/* Addresses */
}	INS;

/* Variant data resolution */
#define i_data1	i_var.i_data[0]
#define i_data2 i_var.i_data[1]

/* Variant #1 */
#define	i_seg	i_data1			/* Segment identifier */
#define	i_len	i_var.i_data3		/* BLOCK length */
#define	i_line	i_data1			/* LINE number */
#define	i_align	i_data1			/* ALIGN information */
#define	i_labno	i_data1			/* LLABEL or JUMP label number */
#define	i_op	i_data1			/* CODE opcode */

/* Variant #2 */
#define	i_refc	i_data2			/* Ref. count for labels */
#define	i_pcseg	i_data2			/* Segment for code */

/* Variant #3 */
#define	i_naddr	i_data4			/* # of i_af entries */
#define	i_long	i_data4			/* Long jump flag */

/*
 * A table of these structures is indexed by opcode number
 * (obtained from the 'opcode.h' header file).
 * Each element holds information needed to assemble the instruction
 * into either ascii assembler or binary code.
 */
typedef	struct	opinfo	{
	char	op_naddr;		/* # of addresses */
	char	op_flag;		/* Flags */
	short	op_opcode;		/* Op code */
	short	op_style;		/* Style, used by formatter */
}	OPINFO;

#define	OP_JUMP	01			/* Some kind of jump */
#define	OP_DD	02			/* Some kind of data pseudo-op */

/*
 * Functions and externals.
 */
extern	SYM	*hash2[];
extern	INS	*getcode();
extern	SYM	*glookup();
extern	char	id[];
extern	SYM	*llookup();
extern	ADDRESS	dot;
extern	int	dotseg;
extern	SEG	seg[];
extern	FILE	*ofp;
extern	FILE	*ifp;
extern	OPINFO	opinfo[];
extern	INS	ins;
extern	INS	*newi();
extern	INS	*newn();
extern	int	line;
extern	INS	*getins();
extern	int	changes;
extern	int	nlabdel;
extern	INS	*deleteins();
extern	int	ndead;
extern	int	nbrbr;
extern	int	ncbrbr;
extern	int	nexbr;
extern	int	nbrnext;
extern	int	nxjump;
extern	int	ncomseq;
extern	int	nsimplify;
extern	int	nuseless;
#if !TINY
extern	int	vflag;
extern	int	xflag;
#endif
extern	char	file[];
extern	char	module[];
extern	int	nerr;
extern	char	*passname;
extern	int	labgen;

#if OVERLAID
extern	jmp_buf	death;
#endif
