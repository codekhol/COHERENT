#ifndef	__KERNEL_DDI_BASE_H__
#define	__KERNEL_DDI_BASE_H__

/*
 * This internal header file defines structures and an access procedure for
 * DDI/DKI process-level data that will not be needed outside the process
 * context. This information corresponds to "U area" data in traditional
 * Unix implementations. Although it may in fact be implemented that way,
 * using an accessor macro guarantees source-level compatibility with systems
 * that do not or can not implement a traditional U area.
 */

/*
 *-IMPORTS:
 *	<common/ccompat.h>
 *		__EXTERN_C_BEGIN__
 *		__EXTERN_C_END__
 *		__PROTO ()
 *	<common/_cred.h>
 *		cred_t
 *	<sys/uio.h>
 *		uio_t
 *		iovec_t
 */

#include <common/ccompat.h>
#include <common/_cred.h>
#include <sys/uio.h>

#if	__COHERENT__

/*
 *-IMPORTS:
 *	<sys/io.h>
 *		IO
 *	<sys/uproc.h>
 *		UPROC
 *		u.u_uid
 *		u.u_gid
 */

#include <sys/io.h>

#define	__KERNEL__	2
#include <sys/uproc.h>
#undef	__KERNEL__

extern UPROC u;

#define	U_AREA()		(& u)

#elif	defined (__MSDOS__)

#include <kernel/dosproc.h>

#endif


/*
 * DDI/DKI per-process data needed only at base level.
 *
 * Note that it might seem like a reasonable idea to record the list of sleep
 * locks held by a process for debugging. However, this is not possible, since
 * the DDI/DKI specification defines SLEEP_TRYLOCK () and SLEEP_UNLOCK () as
 * being available at interrupt level, ie. not requiring user context, making
 * this idea impossible (it is not clear whether a process can in fact try to
 * recursively lock a sleep lock if it has arranged for some interrupt-level
 * activity to unlock it, but it seems reasonable).
 */

struct ddi_base_data {
	char		_non_empty;		/* make structure non-empty */
};

typedef struct ddi_base_data	dbdata_t;


__EXTERN_C_BEGIN__

dbdata_t      *	ddi_base_data	__PROTO ((void));
void		DESTROY_UIO	__PROTO ((uio_t * _uiop, IO * _iop));
cred_t	      *	MAKE_CRED	__PROTO ((cred_t * _cred));
uio_t	      *	MAKE_UIO	__PROTO ((uio_t * _uiop, iovec_t * _iovp,
					  int _mode, IO * _iop));

__EXTERN_C_END__


#define	ddi_base_data()		((dbdata_t *) U_AREA ()->u_nigel)


#endif	/* ! defined (__KERNEL_DDI_BASE_H__) */
