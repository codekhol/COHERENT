#ifndef	__COMMON__UID_H__
#define	__COMMON__UID_H__

/*
 * Certain types come in SVR4 and pre-SVR4 flavours.
 */

typedef	unsigned short	o_dev_t;
typedef	unsigned long	n_dev_t;

typedef	short		o_nlink_t;
typedef	unsigned long	n_nlink_t;

typedef	unsigned short	o_ino_t;
typedef	unsigned long	n_ino_t;

typedef	unsigned short	o_uid_t;
typedef long		n_uid_t;

typedef	o_uid_t		o_gid_t;
typedef	n_uid_t		n_gid_t;

typedef	unsigned short	o_mode_t;
typedef	unsigned long	n_mode_t;


/*
 * The major/minor device number concepts have been altered a little under
 * System V release 4 to encompass the idea of internal and external minor
 * device numbers.
 *
 * Under this concept the "bdevsw" and "cdevsw" can map down from 32-bit
 * device numbers to some smaller internal major/minor space.
 */

typedef	unsigned short	__minor_t;	/* external minor device number */
typedef	unsigned short	__major_t;	/* external major device number */


#if	(_SYSV4 && ! _SYSV3) || _DDI_DKI

typedef	n_uid_t		__uid_t;
typedef	n_gid_t		__gid_t;
typedef	n_dev_t		__dev_t;
typedef	n_nlink_t	__nlink_t;
typedef	n_ino_t		__ino_t;
typedef	n_mode_t	__mode_t;

#else

typedef	o_uid_t		__uid_t;
typedef	o_gid_t		__gid_t;
typedef	o_dev_t		__dev_t;
typedef	o_nlink_t	__nlink_t;
typedef	o_ino_t		__ino_t;
typedef	o_mode_t	__mode_t;

#endif

#endif	/* ! defined (__COMMON__UID_H__) */
