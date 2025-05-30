#ifndef	__SYS_V_TYPES_H__
#define	__SYS_V_TYPES_H__

#if	! _DDI_DKI
# error	You must be compiling in the DDI/DKI environment for <sys/v_types>
#endif

/*
 * Type-name definitions for compatibility with the System V manuals.
 *
 * This file contains type-names that are referenced by STREAMS structures
 * and which we should at least try and build equivalents to.
 *
 * The exact contents of these structure and type definitions have been taken
 * from the System V Release 4 STREAMS Programmer's Guide, and the System V
 * Release 4 Multi-Processor DDI/DKI Reference Manual.
 *
 * The System V definition have been used specifically and only to aid in the
 * support of binary compatibility for STREAMS drivers. This may be an
 * unattainable goal, since the values of certain constant definitions have
 * not been published. However, we include all the published data in the
 * hope of reducing the number of gratuitous differences.
 */
/*
 * The types defined in this file are available from <sys/types.h> under the
 * DDI/DKI, but that is a little problematical, in that this file has to live
 * between two worlds. We require that any file compiled under the DDI/DKI
 * environment be compiled such that <sys/types.h> knows to include these
 * definitions rather than any old definitions. A few files need to work with
 * both the new and old definitions simultaneously; these files should define
 * _TRANSITION to prefix symbols that are sources of conflict with the prefix
 * "n_" (by symmetry with the use of "o_" for old types).
 */

#include <sys/ccompat.h>

#if	defined (__MSDOS__)
#include <sys/_types.h>
#else
#include <sys/types.h>
#endif

#include <sys/__clock.h>		/* for drv_ prototypes */


typedef __VOID__	_VOID;		/* in case there's no void */


/*
 * Types used in the System V DDI/DKI
 */

typedef unsigned short	bool_t;		/* boolean variable type */


/*
 * The major/minor device number concepts have been altered a little under
 * System V release 4 to encompass the idea of internal and external minor
 * device numbers.
 *
 * Under this concept the "bdevsw" and "cdevsw" can map down from 32-bit
 * device numbers to some smaller internal major/minor space.
 */

typedef	unsigned short	minor_t;	/* external minor device number */
typedef	unsigned short	major_t;	/* external major device number */

#define	NODEV		((major_t) -1)

typedef unsigned int	toid_t;		/*
					 * timeout code, used by bufcall ()/
					 * unbufcall () and timeout ()/
					 * untimeout () to identify events.
					 */

/*
 * The maximum-value defined below can't be used without <limits.h>, but we
 * define it here because it depends critically on the type of toid_t.
 */

#define	TOID_MAX	UINT_MAX


/*
 * Processor priorities under the System V DDI/DKI are specified abstractly
 * via the following types and names. The abstract priorities defined below
 * must conform to the following partial order:
 *	plbase < pltimeout <= pldisk, plstr <= plhi
 *
 * The concrete values given here should correspond to values that could be
 * legally passed to one of the spl... () functions. For this reason the
 * actual values assigned are highly machine-specific.
 *
 * THIS SHOULD REALLY BE CHANGED. We deal in machine-specific values for now
 * because of the problems involved in mapping the current interrupt-priority
 * level back to one of these abstract values; while this could be done via
 * a CPU-private global variable, that will not work without the cooperation
 * of other kernel systems that manipulate the interrupt priority.
 *
 * Essentially, it's all abstract or all machine-specific. Abstract is to
 * be preferred if at all possible, since the 80x86 interrupt-flag mechanism
 * used here is very poorly designed. Worse, the hardwired priority levels in
 * the IBM PC interrupt controller were seemingly chosen at random. In order
 * to support a more concurrent kernel, it is desirable to change over to
 * using interrupt masks anyway; doing this provides an opportunity to move
 * to abstract, configurable, and more fine-grained specification of priority
 * level that is otherwise possible, to say nothing of the opportunities to
 * be gained with the help of the locking system.
 */

#if	! defined (__MSDOS__) && ! defined (__COHERENT__)

#error	You need to define values for the "pl_t" type and pl... constants

#else

enum {  _pc_level_0 = 0x200, _pc_level_7 = 0 };

#endif

typedef int		pl_t;		/* processor interrupt level */

enum {
	invpl = -1,			/* invalid processor priority */
	plbase = _pc_level_0,		/* block no interrupts */
	pltimeout = _pc_level_0,	/*
					 * block functions scheduled by
					 * itimeout and dtimeout
					 */
	pldisk = _pc_level_7,		/* block disk device interrupts */
	plstr = _pc_level_7,		/* block STREAMS interrupts */
	plhi = _pc_level_7		/* block all interrupts */
};


/*
 * The following abstract values are used in the DDI/DKI for specifying the
 * priority to be given to processes after awakening from kernel sleep.
 * Clients are permitted to specify a relative bias of up to +/- 3 from the
 * values specified below.
 *
 * The actual values chosen below are such that given the bias, abstract
 * priorities can be mapped via a table into whatever concrete form is
 * desired by the scheduling algorithm. In particular, it is not possible for
 * clients to infer anything about the relative priorities of different levels
 * given the information below.
 */

enum {
	prilo	= 3,			/* low priority */
	pritape	= 10,			/* appropriate for tape driver */
	primed	= 17,			/* medium priority */
	pritty	= 24,			/* appropriate for terminal driver */
	pridisk = 31,			/* appropriate for disk driver */
	prinet	= 38,			/* appropriate for network driver */
	prihi	= 45			/* high priority */
};


/*
 * Credentials structure, containing all the credentials-related
 * information about the user.
 */

typedef	struct {
	unsigned short	cr_ref;		/* reference count */

	unsigned short	cr_grp;		/* number of groups in cr_grps */
	uid_t		cr_uid;		/* real user ID */
	gid_t		cr_gid;		/* real current group ID */
	uid_t		cr_euid;	/* effective user ID */
	gid_t		cr_egid;	/* effective current group ID */
	gid_t	      * cr_grps;	/* array of group membership */

	unsigned long	cr_pad [2];	/* reserved for future use */
} cred_t;


/*
 * The following constants are used to request values from the drv_getparm ()
 * function and to set kernel variables via the drv_setparm () function.
 */

enum {
	LBOLT,			/* Clock ticks since last kernel reboot */
	UPROCP,			/* "proc_t *" for use with vtop () */
	UCRED,			/* "cred_t *" for use with drv_priv () */
	TIME,			/* POSIX-format time, seconds since 1970 */

	SYSCANC,		/* count of cooked terminal characters */
	SYSMINT,		/* count of modem interrupts */
	SYSOUTC,		/* count of characters output to terminal */
	SYSRAWC,		/* count of raw terminal characters */
	SYSRINT,		/* count of terminal receive interrupts */
	SYSXINT			/* count of terminal transmit interrupts */
};


/*
 * We need a TRUE and FALSE along the lines of the old-style <bool.h>
 */

enum { FALSE = 0, TRUE = 1 };


/*
 * Type of function argument to itimeout ().
 */

typedef	void	     (* __tfuncp_t)	__PROTO ((_VOID * _arg));


/*
 * The System V DDI/DKI for Intel processors wants to see us make the
 * I/O functions visible. We've kept that information in another file...
 */

#include <sys/x86io.h>

__EXTERN_C_BEGIN__

int		drv_getparm	__PROTO ((ulong_t _parm, ulong_t * _value_p));
__clock_t	drv_hztousec	__PROTO ((__clock_t _ticks));
int		drv_priv	__PROTO ((cred_t * _credp));
int		drv_setparm	__PROTO ((ulong_t _parm, ulong_t _value));
__clock_t	drv_usectohz	__PROTO ((__clock_t _microsecs));

toid_t		itimeout	__PROTO ((__tfuncp_t _fn, _VOID * _arg,
					  __clock_t _ticks, pl_t _pl));
void		untimeout	__PROTO ((toid_t));

minor_t		etoimajor	__PROTO ((major_t _emaj));
major_t		getemajor	__PROTO ((n_dev_t _dev));
minor_t		geteminor	__PROTO ((n_dev_t _dev));
major_t		getmajor	__PROTO ((n_dev_t _dev));
minor_t		getminor	__PROTO ((n_dev_t _dev));
major_t		itoemajor	__PROTO ((major_t _imaj, major_t _prevemaj));
n_dev_t		makedevice	__PROTO ((major_t _majnum, minor_t _minnum));

int		copyin		__PROTO ((caddr_t _userbuf, caddr_t _driverbuf,
					  size_t _count));
int		copyout		__PROTO ((caddr_t _driverbuf, caddr_t _userbuf,
					  size_t _count));

void		bcopy		__PROTO ((caddr_t _from, caddr_t _to,
					  size_t _bcount));
void		bzero		__PROTO ((caddr_t _addr, size_t _bytes));

ulong_t		btop		__PROTO ((ulong_t _numbytes));
ulong_t		btopr		__PROTO ((ulong_t _numbytes));
ulong_t		ptob		__PROTO ((ulong_t _numpages));

__EXTERN_C_END__


/*
 * 4Kbyte pages for the 386 mean a shift count of 12.
 */

#if	defined (GNUDOS) || defined (COHERENT) || defined (__BORLANDC__)
# define	__PAGE_SHIFT	12
#endif

#define	__PAGE_MASK		((1UL << __PAGE_SHIFT) - 1)

#define	btop(n)			((ulong_t) (n) >> __PAGE_SHIFT)
#define	btopr(n)		(((ulong_t) (n) + __PAGE_MASK) >> __PAGE_SHIFT)
#define	ptob(n)			((ulong_t) (n) << __PAGE_SHIFT)

#endif	/* ! defined (__SYS_V_TYPES_H__) */
