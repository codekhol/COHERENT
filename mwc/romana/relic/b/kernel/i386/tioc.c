/*
 * i386/tioc.c
 *
 * Convert COH286 tty ioctl's to Sys 5 compatible calls.
 *
 * Revised: Wed May 26 16:49:20 1993 CDT
 */

/*
 * ----------------------------------------------------------------------
 * Includes.
 */
#include <sys/coherent.h>
#include <sgtty.h>
#include <errno.h>

/*
 * ----------------------------------------------------------------------
 * Definitions.
 *	Constants.
 *	Macros with argument lists.
 *	Typedefs.
 *	Enums.
 */

#define	OIOC_LOW	0100
#define OIOC_HIGH	0110

/*
 * Bits from COH286 sgttyb sg_flags field.
 */
#define	O_EVENP		0x0001	/* Allow even parity */
#define	O_ODDP		0x0002	/* Allow odd parity */
#define	O_CRMOD		0x0004	/* Map '\r' to '\n' */
#define	O_ECHO		0x0008	/* Echo input characters */
#define	O_LCASE		0x0010	/* Lowercase mapping on input */
#define	O_CBREAK	0x0020	/* Each input character causes wakeup */
#define	O_RAWIN		0x0040	/* 8-bit input raw */
#define	O_RAWOUT	0x0080	/* 8-bit output raw */
#define	O_TANDEM	0x0100	/* flow control protocol */
#define	O_XTABS		0x0200	/* Expand tabs to spaces */
#define	O_CRT		0x0400	/* CRT character erase */

#define	O_RAW	(O_RAWIN|O_RAWOUT)	/* Raw mode */

/*
 * Names for terminal speeds.
 */
#define	O_B0	0		/* Hangup if modem control enabled */
#define	O_B50	1		/* 50 bps */
#define	O_B75	2		/* 75 bps */
#define	O_B110	3		/* 110 bps */
#define	O_B134	4		/* 134.5 bps (IBM 2741) */
#define	O_B150	5		/* 150 bps */
#define	O_B200	6		/* 200 bps */
#define	O_B300	7		/* 300 bps */
#define	O_B600	8		/* 600 bps */
#define	O_B1200	9		/* 1200 bps */
#define	O_B1800	10		/* 1800 bps */
#define	O_B2000	11		/* 2000 bps */
#define	O_B2400	12		/* 2400 bps */
#define	O_B3600	13		/* 3600 bps */
#define	O_B4800	14		/* 4800 bps */
#define	O_B7200	15		/* 7200 bps */
#define	O_B9600	16		/* 9600 bps */
#define	O_B19200	17	/* 19200 bps */
#define	O_EXTA	18		/* External A (DH-11) */
#define	O_EXTB	19		/* External B (DH-11) */

/*
 * ----------------------------------------------------------------------
 * Functions.
 *	Import Functions.
 *	Export Functions.
 *	Local Functions.
 */

static void to_s5_sgfld();
static void to_s5speed();
static void to_coh_sgfld();
static void to_cohspeed();

/*
 * ----------------------------------------------------------------------
 * Global Data.
 *	Import Variables.
 *	Export Variables.
 *	Local Variables.
 */
/*
 * Here are the COH286 values for tty ioctl's.
 * In cvtsgtty[] below, subtract 0100 from the 286 COH ioctl value to
 * index to the equivalent Sys V ioctl.
 *
 *	OIOCSETP	0100		 Terminal set modes (old stty) 
 *	OIOCGETP	0101		 Terminal get modes (old gtty) 
 *	OIOCSETC	0102		 Set characters 
 *	OIOCGETC	0103		 Get characters 
 *	OIOCSETN	0104		 Set modes w/o delay or out flush 
 *	OIOCEXCL	0105		 Set exclusive use 
 *	OIOCNXCL	0106		 Set non-exclusive use 
 *	OIOCHPCL	0107		 Hang up on last close 
 *	OIOCFLUSH	0110		 Flush characters in I/O queues 
 */
static unsigned short cvtsgtty[] = {
	TIOCSETP,
	TIOCGETP,
	TIOCSETC,
	TIOCGETC,
	TIOCSETN,
	TIOCEXCL,
	TIOCNXCL,
	TIOCHPCL,
	TIOCFLUSH
};

/*
 * ----------------------------------------------------------------------
 * Code.
 */

/*
 * tioc()
 *
 * This function is called by dioctl() whenever a 286 binary does an ioctl().
 * Its arguments are the arguments for to the ioctl, plus the local driver's
 * ioctl function, which should support S5 sgtty and termio commands.
 *
 * 1.  If com is COH 286 TIOC, translate it to S5 TIOC.
 * 2.  If translated com is TIOCSET[NP], convert sgttyb struct *vec to S5.
 * 3.  If translated com is TIOCGETP, point vec to a full S5 struct (COH 286
 *     sgttyb struct is 2 bytes shorter than S5 format).
 * 4.  Call driver's ioctl().
 * 5.  If just finished a converted TIOCGETP, convert back to COH 286 sgttyb.
 */
void tioc(dev, com, vec, iocfn)
int dev, com, vec, (*iocfn)();
{
	struct sgttyb sg;
	int my_com = com, my_vec = vec, old_getp = 0;

	if (com >= OIOC_LOW && com <= OIOC_HIGH && u.u_error==0) {
		my_com = cvtsgtty[com - OIOC_LOW];
		if (my_com==TIOCSETP || my_com==TIOCSETN) {
			ukcopy(vec, &sg, sizeof (struct sgttyb));
			sg.sg_flags &= 0xffff;
			to_s5_sgfld(&sg);
			my_vec = &sg;
		}
		if (my_com==TIOCGETP) {
			old_getp = 1;
			my_vec = &sg;
		}
	}
	(*iocfn)(dev, my_com, my_vec);
	if (old_getp && u.u_error==0) {
		to_coh_sgfld(my_vec);
		kucopy(my_vec, vec, sizeof(struct sgttyb)-2);
	}
}

/*
 * to_s5_sgfld()
 *
 * Convert fields in a sgttyb struct from COH 286 format to Sys 5.
 */
static void to_s5_sgfld(sgp)
struct sgttyb * sgp;
{
	unsigned int f = sgp->sg_flags, g = 0;

	/*
	 * Convert sg_ispeed and sg_ospeed.
	 */
	to_s5speed(&(sgp->sg_ispeed));
	to_s5speed(&(sgp->sg_ospeed));

	/*
	 * Convert sg_flags.
	 *   f is old COH 286 flags.
	 *   g is new Sys V flags.
	 */
	if (f & O_EVENP)
		g |= EVENP;
	if (f & O_ODDP)
		g |= ODDP;
	if (f & O_CRMOD)
		g |= CRMOD;
	if (f & O_ECHO)
		g |= ECHO;
	if (f & O_LCASE)
		g |= LCASE;
	if (f & O_CBREAK)
		g |= CBREAK;	/* No CBREAK in Sys 5 sgtty. */
				/* Only one RAW bit in Sys 5 sgtty. */
	if ((f & O_RAWIN)&&(f & O_RAWOUT))
		g |= RAW;
	if (f & O_RAWIN)
		g |= RAWIN;
	if (f & O_RAWOUT)
		g |= RAWOUT;
	if (f & O_TANDEM)
		g |= TANDEM;	/* No TANDEM in Sys 5 sgtty. */
	if (f & O_XTABS)
		g |= XTABS;
	if (f & O_CRT)
		g |= CRT;	/* No CRT in Sys 5 sgtty. */
	sgp->sg_flags = g;
}

/*
 * to_s5speed()
 *
 * Convert speed from COH286 to Sys5.
 * Here are the numbers:
 *	const.	COH286	Sys5
 *	B0	0	0
 *	B50	1	1
 *	B75	2	2
 *	B110	3	3
 *	B134	4	4
 *	B150	5	5
 *	B200	6	6
 *	B300	7	7
 *	B600	8	8
 *	B1200	9	9
 *	B1800	10	10
 *	B2000	11	-
 *	B2400	12	11
 *	B3600	13	-
 *	B4800	14	12
 *	B7200	15	-
 *	B9600	16	13
 *	B19200	17	14
 *	EXTA	18	14
 *	EXTB	19	15
 */
#define BADSPD	99

static void to_s5speed(speed)
unsigned char *speed;
{
	static char s5sp[]={B0,B50,B75,B110,B134,B150,B200,B300,B600,B1200,
		B1800,BADSPD,B2400,BADSPD,B4800,BADSPD,B9600,EXTA,EXTA,EXTB};

	if (*speed >= sizeof(s5sp))
		u.u_error = EINVAL;
	else if (s5sp[*speed] == BADSPD)
		u.u_error = EINVAL;
	else *speed = s5sp[*speed];
}

/*
 * to_coh_sgfld()
 *
 * Convert fields in a sgttyb struct from Sys V format to COH 286.
 */
static void to_coh_sgfld(sgp)
struct sgttyb * sgp;
{
	unsigned int f = sgp->sg_flags, g = 0;

	/*
	 * Convert sg_ispeed and sg_ospeed.
	 */
	to_cohspeed(&(sgp->sg_ispeed));
	to_cohspeed(&(sgp->sg_ospeed));

	/*
	 * Convert sg_flags.
	 *   f is old Sys V flags.
	 *   g is new COH 286 flags.
	 */
	if (f & EVENP)
		g |= O_EVENP;
	if (f & ODDP)
		g |= O_ODDP;
	if (f & CRMOD)
		g |= O_CRMOD;
	if (f & ECHO)
		g |= O_ECHO;
	if (f & LCASE)
		g |= O_LCASE;
	if (f & CBREAK)
		g |= O_CBREAK;	/* No CBREAK in Sys 5 sgtty. */
	if (f & RAWIN)		/* Only one RAW bit in Sys 5 sgtty. */
		g |= O_RAWIN;
	if (f & RAWOUT)
		g |= O_RAWOUT;
	if (f & TANDEM)
		g |= O_TANDEM;	/* No TANDEM in Sys 5 sgtty. */
	if (f & XTABS)
		g |= O_XTABS;
	if (f & CRT)
		g |= O_CRT;	/* No CRT in Sys 5 sgtty. */
	sgp->sg_flags = g;
}

static void to_cohspeed(speed)
unsigned char *speed;
{
	static char cohsp[]={O_B0,O_B50,O_B75,O_B110,O_B134,O_B150,O_B200,
		O_B300,O_B600,O_B1200,O_B1800,O_B2400,O_B4800,O_B9600,O_EXTA,
		O_EXTB};

	if (*speed >= sizeof(cohsp))
		u.u_error = EINVAL;
	else *speed = cohsp[*speed];
}
