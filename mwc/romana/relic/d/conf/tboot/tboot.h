/* Include file for tertiary boot programs.
 *
 * This is a real hodge-podge of symbols.  If you are looking to improve
 * code readability, start by hacking this file into many tiny pieces.
 *
 * La Monte H. Yarroll <piggy@mwc.com>, September 1991
 */

#ifndef TBOOT_H	/* Rest of file. */
#define TBOOT_H

#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#include <sys/types.h>
#include <sys/buf.h>
#include <sys/ptypes.h>

#define TRUE	(1==1)
#define FALSE	(1==2)
#define WS	" \t"
#define	NULL	((char *)0)
#define	FOURK	0x1000	/* 4k page.  Needed for alignment purposes.  */
#define FOURKBOUNDRY	0xf000
#define BLOCK	512	/* 512 bytes per disk block.  */
#define LINESIZE 81	/* Size of typical line with NUL terminator.  */
#define	MAX_SEGS 8	/* Maximum number of executable file segs + 1.  */
#define RBOOTS 0x8000	/* This constant is from Startup.s.  It is the
			 * segment to which tboot relocates itself.
			 * Setting it in this file will only affect
			 * the check in seg_too_high().
			 */
#define NORMAL_MAGIC 0x10B	/* Value of optional header magic
				 * for normal executable file.
				 */

#define DISKINT	0x13		/* Disk drive interrupt.  */
#define DISK_PARAMS (8 << 8)	/* Return Disk Drive Parameters function.  */
#define HARD_DRIVE 0x80		/* Select fixed disks.  */

#define SIXBITS 0x3f		/* Lower six bits of a byte.  */

#define INODES_PER_BLOCK 8


#ifndef	NHD
#define	NHD	1			/* # of heads per drive [1 for f9d0]. */
#endif

#ifndef	NSPT
#define	NSPT	9			/* # of sectors per track on floppy. */
#define	NTRK	40			/* # of tracks on floppy. */
#endif

#define ROOTINO 2			/* Root inode # */
#define INOORG	2			/* First inode block. */
#define IBSHIFT 3			/* Shift, inode to blocks */
#define IOSHIFT 6			/* Shift, inode to bytes */
#define INOMASK 0x0007			/* Mask, inode to offset */
#define BUFSIZE 512			/* Block size. */
#define DISK	0x13			/* Disk Interrupt */
#define KEYBD	0x16			/* Keyboard Interrupt */
#define READ1	0x0201			/* read one sector */
#define FIRST	8			/* Relative start of partition. */
#define FULLSEG	0xffff			/* Size of a whole 8086 segment. */
#define PPMASK	(unsigned short) 0xfff0 /* Mask for rounding to paragraph.  */

#define COFF_SYS_BASE	0x0200		/* System load base paragraph for 386.  */
#define DEF_SYS_BASE	0x0060		/* System load base paragraph. */
#define SYS_START	0x0100		/* System entry point. */

#define THE_DEV		((dev_t)0x01)	/* The one disk device we recognize.  */
#define THE_XDEV	((dev_t)0x02)	/* The whole disk device, rather than partition.  */

/* WAIT_DELAY is how long to wait after finding autoboot before booting.  */
#define WAIT_DELAY	91	/* 5 seconds * 18.2 clicks per second.  */

/* Maximum number of tokens on one command line.  */
#define MAX_TOKENS	10

/* Useful macros.  */
#define GREATEST(a, b, c) (a > (b>c?b:c) ? a : (b>c?b:c))
#define LESSER(a, b) (a < b ? a : b)
#define HIGH(x)	(x >> 8)	/* High byte of 16 bit number.  */
#define LOW(x)	(x & 0xff)	/* Low byte of 16 bit number.  */
#define VERBOSE(c) if (verbose_flag) { c; }
/*
 * Macros for evaluating return codes from get_cpu_type().
 */
#define IS_PRE_AT(f)	((f)==0)
#define IS_I286(f)	((f)==1)
#define IS_I386(f)	((f)==2)

/* Register structure used by call_bios().  */
struct reg {
	unsigned r_ax;
	unsigned r_bx;
	unsigned r_cx;
	unsigned r_dx;
	unsigned r_si;
	unsigned r_di;
	unsigned r_ds;
	unsigned r_es;
	unsigned r_flags;
};

/* Table entry describing a generic segment in an executable file.  */
struct load_segment {
	int valid;			/* Is this a valid table entry?	*/
	char *message;			/* Message to print while loading.  */
	uint16 load_toseg;	/* Where in memory to		*/
	uint16 load_tooffset;	/* load this segment.		*/
	fsize_t load_offset;	/* Where in file to get it.	*/
	fsize_t load_length;	/* How long it is.		*/
};

extern int intcall();	/* Provide C interface to bios interrupts.  */
/* int intcall(reg *srcreg, reg *destreg, int intnum);  */
extern void puts();	/* Put a string on the screen.  */
extern char *gets();	/* Get a string from the keyboard.  */
extern void reverse();	/* Reverse a string in place.  */
extern void itoa();	/* Convert an integer to a decimal string.  */
extern void itobase();	/* Convert an integer to an arbitrary base string.  */
extern uint16 basetoi(); /* Convert an arbitrary base string to an integer.  */
extern daddr_t vmap();	/* Convert file block number to physical block number.  */
extern char *lpad();	/* Pad a string on the left.  */
extern uint16 object_nlist();	/* Look up a symbol in an object file.  */
extern uint16 object_sys_base(); /* Generate a default sys_base.  */
extern uint32 wrap_coffnlist();	/* Candy coated coff nlist().  */
extern int wait_for_keystrok();	/* Wait a time delay for a keystroke.  */
extern BUF *bread();		/* Read a disk block.  */
extern BUF *xbread();		/* Read a disk block rel. to the whole disk.  */
extern BUF *bclaim();		/* Claim a disk buffer.  */
extern BUF *bpick();		/* Pick a buffer to trash.  */
extern void bufinit();		/* Initialize disk buffers.  */
extern void brelease();		/* Free a disk buffer.  */
extern int gate_lock();		/* Attempt to lock a GATE.  */
extern int gate_locked();	/* Check to see if a GATE is locked.  */
extern void gate_unlock();	/* Unlock a GATE.  */
extern void print8();		/* Print an 8 bit integer, base 16.  */
extern void print16();		/* Print a 16 bit integer, base 16.  */
extern void print32();		/* Print a 32 bit integer, base 16.  */
extern void sanity_check();	/* Check for insane conditions.  */
extern void seg_align();	/* Align a far address.  */
extern int coff2load();		/* Convert COFF to load table.  */
extern int coffnlist();		/* Search COFF file for symbols.  */
extern void dump_bios_disk();	/* Dump a T_BIOS_DISK typed space.  */
extern void dump_fifo();	/* Dump a T_FIFO* typed space.  */
extern void dump_gift();	/* Dump the boot_gift typed space.  */
extern void dump_rootdev();	/* Dump a T_BIOS_ROOTDEV typed space.  */
extern int gift_argf();		/* Prepare a command line gift.  */
extern int gift_drive_params();	/* Prepare a drive description from the BIOS.  */
extern int gift_rootdev();	/* Indentify the boot partition to the kernel.  */
extern void seginc();		/* Add an offset to a segment.  */
extern void ffcopy();		/* Copy from one far address to another.  */
extern int read();		/* Read from a file descriptor.  */
extern int open();		/* Open a file descriptor.  */
extern long lseek();		/* Set a read/write position in a file.  */
extern int close();		/* Close a file descriptor.  */
extern int object2load();	/* Extract information to load an executable.  */
extern void monitor();		/* Mini-monitor for testing boot code.  */
extern int lout2load();		/* Convert l.out to load table.  */
extern void l_out_nlist();	/* Get entries from l.out name list.  */
extern int iopen();		/* Open an inode for a file.  */
extern ino_t namei();	/* Convert from a name to an inode.  */
extern void iread();	/* Read from a file, given an inode.  */
extern void ifread();	/* Read from a file into a far buffer, given an inode.  */
extern daddr_t indirect();	/* Follow a block indirection out.  */
extern daddr_t ind_lookup();	/* Look up a block in an indirection table.  */
extern uint16 ind_index();	/* Calculate index into an indirection table.  */
extern int get_num_of_drives();	/* Ask the BIOS how many drives are attached.  */
extern int interpret();	/* Attempt to execute a builtin command.  */
extern void dpb();	/* Display parameters from bios.  */
extern void dir();	/* List contents of /.  */
extern int fdisk();	/* Read fixed disk configuration.  */

extern int errno;	/* Error number for "system" calls.  */
EXTERN uint16 sys_base;	/* Segment into which to load the kernel.  */
EXTERN int sys_base_set;	/* Has sys_base been explicitly set?  */
EXTERN int want_monitor;	/* Should we invoke monitor before execution?  */
EXTERN int verbose_flag;	/* Be verbose?  */
#endif /* TBOOT_H */
