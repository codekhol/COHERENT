/* $Header: /kernel/kersrc/io.286/msg.c,v 1.1 92/07/17 15:24:33 bin Exp Locker: bin $
 *
 *	The  information  contained herein  is a trade secret  of INETCO
 *	Systems, and is confidential information.   It is provided under
 *	a license agreement,  and may be copied or disclosed  only under
 *	the terms of that agreement.   Any reproduction or disclosure of
 *	this  material  without  the express  written  authorization  of
 *	INETCO Systems or persuant to the license agreement is unlawful.
 *
 *	Copyright (c) 1987, 1986, 1985, 1984.
 *	An unpublished work by INETCO Systems, Ltd.
 *	All rights reserved.
 */

/*
 * System V Compatible Messaging
 *
 *	This module provides System V compatible messaging operations.
 *
 *			Author: Allan Cornish, INETCO Systems Ltd., Oct 1984
 *
 * $Log:	msg.c,v $
 * Revision 1.1  92/07/17  15:24:33  bin
 * Initial revision
 * 
 * Revision 2.1	88/09/03  13:09:07	src
 * *** empty log message ***
 * 
 * Revision 1.2	88/08/02  16:49:52	src
 * Bug:	msgget with IPC_CREAT could fail if message queue already
 * 	existed, and queue permissions were not an exact match
 * 	for requested permissions.
 * Fix:	Permission checking made more rigorous.
 * 
 * Revision 1.1	88/03/24  17:05:44	src
 * Initial revision
 * 
 * 87/04/24	Allan Cornish		/usr/src/sys/i8086/drv/msg.c
 * Msgctl() IPC_STAT check of polled devices modified.
 *
 * 87/04/01	Allan Cornish		/usr/src/sys/i8086/drv/msg.c
 * Msgctl() now correctly reports polling events on an IPC_STAT operation.
 *
 * 87/03/20	Allan Cornish		/usr/src/sys/i8086/drv/msg.c
 * Msgpoll() now correctly reports POLLOUT events.
 *
 * 87/01/20	Allan Cornish		/usr/src/sys/i8086/drv/msg.c
 * msginit() is now more paranoid about validating NMSC, NMSG, NMSQID.
 *
 * 87/01/05	Allan Cornish		/usr/src/sys/i8086/drv/msg.c
 * msginit() now allocates message buffers in high memory.
 *
 * 86/12/12	Allan Cornish		/usr/src/sys/i8086/drv/msg.c
 * Added 3rd argument to msgpoll() to support non-blocking polls.
 *
 * 86/11/21	Allan Cornish		/usr/src/sys/i8086/drv/msg.c
 * Added support for System V.3 compatible polls.
 * Added msgpoll() routine and xmsqid_ds structure.
 *
 * 85/08/06	Allan Cornish		/usr/src/sys/i8086/drv/msg.c
 * Msg.c split into configuration (msgcon.c) and implementation (msg.c).
 *
 * 85/07/22	Allan Cornish		/usr/src/sys/i8086/drv/msg.c
 * Msgget, msgctl, msgsnd, msgrcv now return immediately if u.u_error is set.
 *
 * 85/07/19	Allan Cornish
 * Msgrcv() now reports E2BIG if message is larger than size requested.
 * Msgfree() integrated into msginit() and msgrcv() functions.
 * Msgsnd() now checks for queue removal after waking from sleep.
 * Msgsnd() and msgrcv() now detect address faults and report EFAULT.
 *
 * 85/07/03	Allan Cornish
 * Replaced use of EDOM with EIDRM.
 * Replaced msgaccess() by calls to ipcaccess(), increasing shared ipc code.
 * Eliminated many calls to sphi() and spl().  They are not really required,
 * since system calls are synchronous unless they sleep or are interrupted.
 * Sleeps cause no problem, and interrupts do not affect messaging state.
 *
 * 85/06/19	Allan Cornish
 * Added code to msgctl to allow the owner of a queue to reduce msg_qbytes.
 * Previously only the super-user could modify msg_qbytes.
 *
 * 85/06/18	Allan Cornish
 * Added code to msgsnd to check for full queue (msg_cbytes >= msg_qbytes).
 * Integrated msgalloc function into msgsnd, since only called from there.
 * Fixed bug in msgrcv when msgtyp < 0, to treat msg_type as mesg priority.
 *
 * 85/05/10	Allan Cornish
 * C compiler bug in msginit caused 'mp->msg_spot = (wanted -= NMSC)' to fail.
 * For NMSG=10, NMSC=640, msg_spot always set to 0x1900 or 0xE980.
 * Code changed to 'wanted -= NMSC; mp->msg_spot = wanted'.
 *
 * 85/04/01	Allan Cornish
 * fixed qp->msg_last bug in msgrcv().
 */

#include <sys/coherent.h>
#include <sched.h>
#include <types.h>
#include <errno.h>
#include <stat.h>
#include <sys/con.h>
#include <seg.h>
#include <msg.h>

#ifndef	EIDRM
#define	EIDRM	EDOM
#endif

/*
 * Extended message queue id data structure.
 *	- extended to support System V.3 compatible polls.
 */
struct xmsqid_ds {
	struct msqid_ds	msq;
	struct event	ipolls;
	struct event	opolls;
};

/*
 *	Message Information
 */

struct xmsqid_ds *msqs = 0;	/* Pointer to array of message queues */
				/* (first queue contains free message list) */

struct msg * msgs = 0;		/* Pointer to array of message headers */

static SEG * msgsp;		/* Segment containing messages */
#define	msgsel FP_SEL(msgsp->s_faddr)

/*
 *	Global Message Parameters
 */

unsigned NMSQID = 9;	/* allocated number of message queues */
unsigned NMSQB  = 2048;	/* default maximum queue size in bytes */
unsigned NMSG = 10;	/* allocated messages: (NMSG * NMSC) < 2^16 */
unsigned NMSC = 640;	/* maximum message text size */

/*
 * Message Device Initialization.
 *
 *	Initialize message ids.
 */

msginit()
{
	register struct xmsqid_ds *qp;
	register struct msg *mp;
	long wanted;
	int i;

	if ( NMSG == 0 )
		NMSQID = 0;
	if ( NMSC == 0 )
		NMSQID = 0;
	if ( NMSQID == 0 )
		return 0;

	if ( NMSQID > 128 )
		NMSQID = 128;

	/*
	 * Allocate message queues and message headers.
	 */
	wanted = (NMSQID * (long) sizeof(struct xmsqid_ds)) +
		(NMSG * (long) sizeof(struct msg));
	if (wanted > 16384) {
		printf("invalid NMSQID or NMSG kernel variable\n");
		NMSQID=0;
		return;
	}

	if ( msqs = kalloc( (unsigned) wanted) ) {

		/*
		 * Ensure allocated space is cleared.
		 */
		memset( msqs, 0, (unsigned) wanted );

		/*
		 * Message headers are contiguous to message queues.
		 */
		msgs = (struct msg *) (&msqs[NMSQID]);

		/*
		 * Allocate message buffers.
		 */
		wanted = (long) NMSG * NMSC;
		if ( wanted > 0xFFFFL ) {
			printf("invalid NMSG or NMSC kernel variable\n");
			kfree( msqs );
			NMSQID = 0;
			msqs   = 0;
			return;
		}

		msgsp = salloc( (size_t) wanted, SFHIGH|SFNSWP|SFNCLR );

		if ( msgsp == 0 ) {
			printf("could not salloc %u messages\n", NMSG);
			kfree( msqs );
			NMSQID = 0;
			msqs   = 0;
			return;
		}

		/*
		 * Initialize message queues.
		 */
		for ( qp = msqs, i = 0; i < NMSQID; i++, qp++ ) {

			qp->msq.msg_perm.seq  = i * 256;

			qp->ipolls.e_dnext =
			qp->ipolls.e_dlast = &qp->ipolls;

			qp->opolls.e_dnext =
			qp->opolls.e_dlast = &qp->opolls;
		}

		/*
		 * Initialize message headers, place on free queue.
		 */
		for ( qp = msqs, mp = &msgs[NMSG]; --mp >= msgs; ) {
			wanted -= NMSC;
			mp->msg_spot  = wanted;
			mp->msg_next  = qp->msq.msg_first;
			qp->msq.msg_first = mp;
		}
	}
	else {
		printf("could not kalloc %u message ids\n", NMSQID);
		NMSQID = 0;
	}
}


/*
 * Msgctl - Message Control Operations.
 */

umsgctl( qid, cmd, buf )

int qid;
int cmd;
struct msqid_ds *buf;

{
	register struct xmsqid_ds *qp;
	register struct msg *mp;
	unsigned n;

	/*
	 * Validate queue identifier.
	 */
	if ( (qid <= 0) || (qid/256 >= NMSQID) || (msqs == 0) ) {
		u.u_error = EINVAL;
		return -1;
	}

	qp = &msqs[ qid / 256 ];

	/*
	 * Validate queue existence.
	 */
	if ( (qp == 0) || ((qp->msq.msg_perm.mode & IPC_ALLOC) == 0) ) {
		u.u_error = EINVAL;
		return -1;
	}

	/*
	 * Validate qid for all commands except for IPC_STAT.
	 */
	if ( (cmd != IPC_STAT) && (qp->msq.msg_perm.seq != qid) ) {
		u.u_error = EINVAL;
		return -1;
	}

	switch ( cmd ) {

	case IPC_STAT:
		/*
		 * Validate access authority.
		 */
		if ( (ipcaccess(&qp->msq.msg_perm) & MSG_R) == 0 ) {
			u.u_error = EACCES;
			break;
		}

		/*
		 * Copy queue info to user.
		 */
		kucopy( qp, buf, sizeof(struct msqid_ds) );

		/*
		 * Include input polls in receive waiting.
		 */
		if ( (qp->ipolls.e_dnext != NULL)
		  && (qp->ipolls.e_dnext != &qp->ipolls) ) {
			putuwd( &buf->msg_perm.mode,
				getuwd( &buf->msg_perm.mode ) | MSG_RWAIT );
		}

		/*
		 * Include output polls in write waiting.
		 */
		if ( (qp->opolls.e_dnext != NULL)
		  && (qp->opolls.e_dnext != &qp->opolls) ) {
			putuwd( &buf->msg_perm.mode,
				getuwd( &buf->msg_perm.mode ) | MSG_WWAIT );
		}

		/*
		 * Validate qid.
		 * Doing it now lets user get info on message queue.
		 */
		if ( qp->msq.msg_perm.seq != qid )
			u.u_error = EINVAL;
		break;

	case IPC_SET:
		/*
		 * Validate modify authority.
		 */
		if ( (u.u_uid != 0) && (u.u_uid != qp->msq.msg_perm.uid) ) {
			u.u_error = EPERM;
			break;
		}

		/*
		 * Get desired queue size.
		 */
		n = getuwd( &(buf->msg_qbytes) );
		if (u.u_error)
			break;

		/*
		 * Only super-user can increase queue size.
		 */
		if ( (u.u_uid != 0) && (n > qp->msq.msg_qbytes) ) {
			u.u_error = EPERM;
			break;
		}

		/*
		 * Set queue parameters.
		 */
		qp->msq.msg_perm.uid   = getuwd( &(buf->msg_perm.uid ) );
		qp->msq.msg_perm.gid   = getuwd( &(buf->msg_perm.gid ) );
		qp->msq.msg_perm.mode &= ~0777;
		qp->msq.msg_perm.mode |= getuwd( &(buf->msg_perm.mode) ) & 0777;
		qp->msq.msg_qbytes     = n;
		break;

	case IPC_RMID:
		/*
		 * Validate removal authority.
		 */
		if ( (u.u_uid != 0) && (u.u_uid != qp->msq.msg_perm.uid) ) {
			u.u_error = EPERM;
			break;
		}

		/*
		 * Free all messages on the queue being removed.
		 */
		while ( mp = qp->msq.msg_first ) {
			qp->msq.msg_first   = mp->msg_next;
			mp->msg_next        = msqs->msq.msg_first;
			msqs->msq.msg_first = mp;
		}

		/*
		 * Wakeup processes waiting for free message buffers.
		 */
		if ( msqs->msq.msg_perm.mode & MSG_RWAIT ) {
			msqs->msq.msg_perm.mode &= ~MSG_RWAIT;
			wakeup( msqs );
		}
		if ( msqs->ipolls.e_procp )
			pollwake( &msqs->ipolls );

		/*
		 * Initialize queue parameters.
		 */
		qp->msq.msg_last   = 0;
		qp->msq.msg_qnum   = 0;
		qp->msq.msg_cbytes = 0;
		if ( (qp->msq.msg_perm.seq & 0x00FF) == 0x00FF )
			qp->msq.msg_perm.seq &= 0x7F00;
		qp->msq.msg_perm.seq++;

		/*
		 * Wakeup processes sleeping on the removed message queue.
		 */
		if ( qp->msq.msg_perm.mode & (MSG_RWAIT|MSG_WWAIT) )
			wakeup( qp );
		if ( qp->ipolls.e_procp )
			pollwake( &qp->ipolls );
		if ( qp->opolls.e_procp )
			pollwake( &qp->opolls );

		qp->msq.msg_perm.mode = 0;
		break;

	default:
		u.u_error = EINVAL;
	}

	if ( u.u_error )
		return -1;

	return 0;
}

/*
 * Msgget - Get set of messages
 */

umsgget( mykey, msgflg )

key_t mykey;
int msgflg;

{
	register struct xmsqid_ds *qp;
	register struct xmsqid_ds *freeidp = 0;
	int rwmode;
	
	if ( msqs == 0 ) {

		msginit();

		if ( msqs == 0 ) {
			u.u_error = ENOMEM;
			return;
		}
	}

	/*
	 * Extract desired access mode from flags.
	 */
	rwmode = msgflg & 0777;

	/*
	 * Search for desired message queue [also for first free queue].
	 */
	for ( qp = &msqs[NMSQID]; --qp > msqs; ) {

		if ( (qp->msq.msg_perm.mode & IPC_ALLOC) == 0 ) {

			if ((freeidp == 0) ||
			    (freeidp->msq.msg_ctime > qp->msq.msg_ctime))
				freeidp = qp;
			continue;
		}

		if (mykey == IPC_PRIVATE)
			continue;

		if ( mykey == qp->msq.msg_perm.key ) {		/* found! */

			if ( (msgflg & IPC_CREAT) && (msgflg & IPC_EXCL) ) {
				u.u_error = EEXIST;
				return -1;
			}

			if ( (qp->msq.msg_perm.mode & rwmode) != rwmode ) {
				u.u_error = EACCES;
				return -1;
			}

			return qp->msq.msg_perm.seq;
		}
	}

	if ( ! (msgflg & IPC_CREAT) ) {
		u.u_error = ENOENT;
		return -1;
	}

	if ( (qp = freeidp) == 0 ) {
		u.u_error = ENOSPC;
		return -1;
	}

	qp->msq.msg_first  = 0;
	qp->msq.msg_last   = 0;
	qp->msq.msg_cbytes = 0;
	qp->msq.msg_qnum   = 0;
	qp->msq.msg_qbytes = NMSQB;
	qp->msq.msg_lspid  = 0;
	qp->msq.msg_lrpid  = 0;
	qp->msq.msg_stime  = 0;
	qp->msq.msg_rtime  = 0;
	qp->msq.msg_ctime  = timer.t_time;

	qp->msq.msg_perm.cuid = qp->msq.msg_perm.uid = u.u_uid;
	qp->msq.msg_perm.cgid = qp->msq.msg_perm.gid = u.u_gid;
	qp->msq.msg_perm.mode = rwmode | IPC_ALLOC;
	qp->msq.msg_perm.key  = mykey;

	return qp->msq.msg_perm.seq;
}

/*
 * Send a Message
 */

umsgsnd( qid, bufp, msgsz, msgflg )

int qid;
struct msgbuf *bufp;
unsigned msgsz, msgflg;

{
	register struct xmsqid_ds * qp;
	register struct msg      * mp;

	/*
	 * Validate queue identifier.
	 */
	if ((qid <= 0) || (qid/256 >= NMSQID) || (msgsz > NMSC) || (msqs==0)) {
		u.u_error = EINVAL;
		return -1;
	}

	qp = &msqs[ qid / 256 ];

	/*
	 * Validate queue existence.
	 */
	if ( (qp->msq.msg_perm.seq != qid)
	  || ((qp->msq.msg_perm.mode & IPC_ALLOC) == 0) ) {
		u.u_error = EINVAL;
		return -1;
	}

	if ((ipcaccess(&qp->msq.msg_perm) & MSG_W) == 0){ /* can't send mesg */
		u.u_error = EACCES;
		return -1;
	}

	/*
	 * Wait for a free message buffer
	 */
	while ( (msqs->msq.msg_first == 0)
	     || (qp->msq.msg_qbytes <= qp->msq.msg_cbytes)) {

		if ( msgflg & IPC_NOWAIT ) {
			u.u_error = EAGAIN;
			return -1;
		}

		if (qp->msq.msg_qbytes <= qp->msq.msg_cbytes) {
			qp->msq.msg_perm.mode |= MSG_WWAIT;
			sleep( qp, CVTTOUT, IVTTOUT, SVTTOUT );
		}
		else {
			msqs->msq.msg_perm.mode |= MSG_RWAIT;
			sleep( msqs, CVTTOUT, IVTTOUT, SVTTOUT );
		}

		/*
		 * Abort if a signal was received
		 */
		if (SELF->p_ssig && nondsig() ) {
			u.u_error = EINTR;
			return -1;
		}

		/*
		 * Abort if the message queue was removed.
		 */
		if ( qid != qp->msq.msg_perm.seq ) {
			u.u_error = EIDRM;
			return -1;
		}
	}

	/*
	 * Use first message on free message queue
	 */
	mp = msqs->msq.msg_first;
	mp->msg_ts = msgsz;

	/*
	 * Transfer the message type and text.
	 */
	ukcopy( &(bufp->mtype), &(mp->msg_type), sizeof(mp->msg_type) );
	if ( ufcopy( &bufp->mtext[0], mp->msg_spot, msgsel, msgsz ) != msgsz )
			u.u_error = EFAULT;

	/*
	 * Abort if address fault occured during transfer.
	 */
	if ( u.u_error )
		return -1;

	/*
	 * Move the message to the desired queue.
	 */
	msqs->msq.msg_first = mp->msg_next;
	mp->msg_next = 0;

	if ( qp->msq.msg_last )
		qp->msq.msg_last->msg_next = mp;
	else
		qp->msq.msg_first = mp;
	qp->msq.msg_last = mp;

	/*
	 * Update queue statistics.
	 */
	qp->msq.msg_cbytes += msgsz;
	qp->msq.msg_qnum++;
	qp->msq.msg_lspid = SELF->p_pid;
	qp->msq.msg_stime = timer.t_time;

	/*
	 * Wake processes waiting to receive.
	 */
	if ( qp->msq.msg_perm.mode & MSG_RWAIT ) {
		qp->msq.msg_perm.mode &= ~MSG_RWAIT;
		wakeup( qp );
	}
	if ( qp->ipolls.e_procp )
		pollwake( &qp->ipolls );

	return 0;
}

/*
 * Receive A Message
 */

umsgrcv( qid, bufp, msgsz, msgtyp, msgflg )

int qid;
struct msgbuf *bufp;
unsigned msgsz;
long msgtyp;
unsigned msgflg;

{
	register struct xmsqid_ds *qp;
	register struct msg *mp;
	register struct msg *prev;

	/*
	 * Validate queue identifier.
	 */
	if ( (qid <= 0) || (qid/256 >= NMSQID) || (msqs == 0) ) {
		u.u_error = EINVAL;
		return -1;
	}

	qp = &msqs[ qid / 256 ];

	/*
	 * Validate queue existence.
	 */
	if ( (qp->msq.msg_perm.seq != qid)
	  || ((qp->msq.msg_perm.mode & IPC_ALLOC) == 0) ) {
		u.u_error = EINVAL;
		return -1;
	}

	/*
	 * Permission denied
	 */
	if ( (ipcaccess(&qp->msq.msg_perm) & MSG_R) == 0 ) {
		u.u_error = EACCES;
		return -1;
	}

	/*
	 * Wait for message
	 */
	for (;;) {

		mp   = qp->msq.msg_first;
		prev = 0;

		/*
		 * Find mesg of type <= abs(msgtyp)
		 */
		if ( msgtyp < 0 ) {

			struct msg *xp, *xprev;
			
			xp     = 0;
			xprev  = 0;
			msgtyp = -msgtyp;

			for ( ; mp != 0 ; prev = mp, mp = mp->msg_next ) {

				if (mp->msg_type > msgtyp)
					continue;

				if ((xp==0) || (mp->msg_type < xp->msg_type)) {
					xp    = mp;
					xprev = prev;
				}
			}
			mp     = xp;
			prev   = xprev;
			msgtyp = -msgtyp;
		}

		/*
		 * Find message of type == msgtyp
		 */
		else if ( msgtyp > 0 ) {

			while ( (mp != 0) && (mp->msg_type != msgtyp) ) {
				prev = mp;
				mp = mp->msg_next;
			}
		}

		/*
		 * Else take first message
		 */

		if ( mp )
			break;

		/*
		 * Can't wait to receive mesg
		 */
		if ( msgflg & IPC_NOWAIT ) {
			u.u_error = EAGAIN;
			return -1;
		}

		qp->msq.msg_perm.mode |= MSG_RWAIT;
		sleep( qp, CVTTOUT, IVTTOUT, SVTTOUT );

		/*
		 * Signal received
		 */
		if ( SELF->p_ssig && nondsig() ) {
			u.u_error = EINTR;
			return -1;
		}

		/*
		 * Not same q anymore
		 */
		if ( qid != qp->msq.msg_perm.seq ) {
			u.u_error = EIDRM;
			return -1;
		}
	}

	/*
	 * Ensure entire message can be transferred, or MSG_NOERROR asserted.
	 */
	if ( (msgsz < mp->msg_ts) && ((msgflg & MSG_NOERROR) == 0) ) {
		u.u_error = E2BIG;
		return -1;
	}

	/*
	 * Transfer message data
	 */
	if ( msgsz > mp->msg_ts )
		msgsz = mp->msg_ts;

	kucopy( &(mp->msg_type), &(bufp->mtype), sizeof(mp->msg_type) );
	if (fucopy( mp->msg_spot, msgsel, &(bufp->mtext[0]), msgsz ) != msgsz)
		u.u_error = EFAULT;

	/*
	 * Abort if address fault occurred during transfer.
	 */
	if ( u.u_error )
		return -1;

	/*
	 * Remove message from queue
	 */
	if ( prev )
		prev->msg_next = mp->msg_next;
	else
		qp->msq.msg_first = mp->msg_next;

	if ( qp->msq.msg_last == mp )
		qp->msq.msg_last = prev;

	/*
	 * Update queue statistics
	 */
	qp->msq.msg_cbytes -= mp->msg_ts;
	qp->msq.msg_qnum--;
	qp->msq.msg_lrpid = SELF->p_pid;
	qp->msq.msg_rtime = timer.t_time;

	/*
	 * Wakeup processes waiting to send.
	 */
	if (qp->msq.msg_perm.mode & MSG_WWAIT) {
		qp->msq.msg_perm.mode &= ~MSG_WWAIT;
		wakeup( qp );
	}
	if ( qp->opolls.e_procp )
		pollwake( &qp->opolls );

	/*
	 * Place message buffer on free message queue
	 */
	qp = msqs;
	mp->msg_next = qp->msq.msg_first;
	qp->msq.msg_first = mp;

	/*
	 * Wakeup processes waiting for empty message buffer
	 */
	if ( qp->msq.msg_perm.mode & MSG_RWAIT ) {
		qp->msq.msg_perm.mode &= ~MSG_RWAIT;
		wakeup( qp );
	}
	if ( msqs->ipolls.e_procp )
		pollwake( &msqs->ipolls );

	return msgsz;
}

/*
 * Msgpoll - Message Queue Polling.
 */
msgpoll( qid, ev, msec )
int qid;
int ev;
int msec;
{
	register struct xmsqid_ds * qp;

	/*
	 * Validate queue identifier.
	 */
	if ( (qid <= 0) || (qid/256 >= NMSQID) || (msqs == 0) )
		return POLLNVAL;

	qp = &msqs[ qid / 256 ];

	/*
	 * Validate queue existence.
	 */
	if ( ((qp->msq.msg_perm.mode & IPC_ALLOC) == 0)
	  || (qp->msq.msg_perm.seq != qid) )
		return POLLHUP;

	/*
	 * Priority polls not supported.
	 */
	ev &= ~POLLPRI;

	/*
	 * Input poll.
	 */
	if ( ev & POLLIN ) {

		/*
		 * No messages on queue.
		 */
		if ( qp->msq.msg_qnum == 0 ) {
			/*
			 * Enable input monitor.
			 */
			if ( msec != 0 )
				pollopen( &qp->ipolls );
			ev &= ~POLLIN;
		}

		/*
		 * Prevent output monitor.
		 */
		else
			msec = 0;
	}

	/*
	 * Output poll.
	 */
	if ( ev & POLLOUT ) {

		/*
		 * Queue full.
		 */
		if ( qp->msq.msg_cbytes >= qp->msq.msg_qbytes ) {
			if ( msec != 0 )
				pollopen( &qp->opolls );
			ev &= ~POLLOUT;
		}

		/*
		 * No free message buffers.
		 */
		else if ( msqs->msq.msg_first == NULL ) {
			if ( msec != 0 )
				pollopen( &msqs->ipolls );
			ev &= ~POLLOUT;
		}
	}

	return ev;
}
