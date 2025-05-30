/* $Header: /newbits/kernel/USRSRC/coh/RCS/poll.c,v 1.4 91/07/24 07:51:32 bin Exp Locker: bin $ */
/*
 *	The  information  contained herein  is a trade secret  of INETCO
 *	Systems, and is confidential information.   It is provided under
 *	a license agreement,  and may be copied or disclosed  only under
 *	the terms of that agreement.   Any reproduction or disclosure of
 *	this  material  without  the express  written  authorization  of
 *	INETCO Systems or persuant to the license agreement is unlawful.
 *
 *	Copyright (c) 1986
 *	An unpublished work by INETCO Systems, Ltd.
 *	All rights reserved.
 */

/*
 * [Stream] Polling.
 *
 *	void pollinit( ) -- allocate polling buffers
 *	int pollopen(qp) -- enable polling  by current process  on given queue
 *	int pollwake(qp) -- wake all processes waiting for poll on given queue
 *	int pollexit(  ) -- terminate all polls enabled by current process
 *	event_t * ep;
 *
 * $Log:	poll.c,v $
 * Revision 1.4  91/07/24  07:51:32  bin
 * update prov by hal
 * 
 * 
 * Revision 1.1	88/03/24  16:14:10	src
 * Initial revision
 * 
 * 86/11/19	Allan Cornish		/usr/src/sys/coh/poll.c
 * Ported to Coherent from RTX.
 */

#include <sys/coherent.h>
#include <sys/proc.h>
#include <sys/uproc.h>

/*
 * Patchable data.
 */
int	NPOLL  = 0;

/*
 * Private data.
 */
static	event_t	* efreep;

/**
 *
 * event_t *
 * pollinit()		-- allocate event buffers.
 */
event_t *
pollinit()
{
	register event_t * ep;
	register event_t * ap;
	static int first = 1;

	/*
	 * If dynamically growing event pool is specified [NPOLL == 0],
	 * try to allocate an additional cluster of 32 on each call.
	 */
	if ( NPOLL == 0 ) {
		if ( ep = kalloc( 32 * sizeof(event_t) ) )
			ap = ep + 32;
	}

	/*
	 * If statically sized event pool is specified [NPOLL != 0],
	 * try to allocate the pool on the first call.
	 */
	else if ( first ) {
		first = 0;
		if ( ep = kalloc( NPOLL * sizeof(event_t) ) )
			ap = ep + NPOLL;
	}

	/*
	 * If event cluster was allocated, insert into free event queue.
	 */
	if ( ep ) {
		do {
			ep->e_pnext = efreep;
			efreep = ep;
		} while ( ++ep < ap );
	}

	return efreep;
}

/**
 *
 * int
 * pollopen(qp) -- enable polling by current process on given event queue
 * event_t * qp;
 */
pollopen( qp )
register event_t * qp;
{
	register event_t * ep;

	/*
	 * Initialize device queue if required.
	 */
	if ( qp->e_dnext == 0 )
		qp->e_dnext = qp->e_dlast = qp;

	/*
	 * Obtain a free event buffer, or return.
	 */
	if ( ((ep = efreep) == 0) && ((ep = pollinit()) == 0) ) {
		printf("out of poll buffers\n");
		return;
	}

	/*
	 * Remove event buffer from free queue.
	 */
	efreep = ep->e_pnext;

	/*
	 * Record process pointer in event buffer.
	 */
	ep->e_procp = cprocp;

	/*
	 * Insert event at head of process event singularly-linked queue.
	 */
	ep->e_pnext = cprocp->p_polls;
	cprocp->p_polls = ep;

	/*
	 * Insert event at tail of circularly-linked device queue.
	 * This ensures that processes are first-in first-out.
	 */
	ep->e_dnext  = qp;
	(ep->e_dlast = qp->e_dlast)->e_dnext = ep;
	qp->e_dlast  = ep;

	/*
	 * Record last process to enable polling on device.
	 */
	qp->e_procp = cprocp;
}

/**
 *
 * int
 * pollwake( qp ) -- wake all processes waiting for poll on given queue
 * event_t * qp;
 */
pollwake( qp )
event_t * qp;
{
	register event_t * ep = qp;
	register PROC    * pp;

	/*
	 * Clear device process pointer, indicating poll completed.
	 * NOTE: interrupt handlers may have already cleared it.
	 */
	qp->e_procp = 0;

	if ( ep = qp->e_dnext ) {

		/*
		 * Service circularly-linked polls on device queue.
		 */
		while ( ep != qp ) {
			/*
			 * Wake process if it is sleeping.
			 */
			if ( (pp = ep->e_procp) && (pp->p_state == PSSLEEP) )
				wakeup( &pp->p_polls );

			ep = ep->e_dnext;
		}
	}
}

/**
 *
 * int
 * pollexit() -- terminate all polls opened by current process
 */
int
pollexit()
{
	register PROC    * pp = cprocp;
	register event_t * ep;

	/*
	 * Service all polling event buffers enabled by current process.
	 */
	while ( ep = pp->p_polls ) {

		/*
		 * Remove event buffer from circularly-linked device queue.
		 */
		(ep->e_dnext->e_dlast = ep->e_dlast)->e_dnext = ep->e_dnext;

		/*
		 * Remove event buffer from singularly-linked process queue.
		 */
		pp->p_polls = ep->e_pnext;

		/*
		 * Insert event buffer at head of free event buffer queue.
		 */
		ep->e_pnext = efreep;
		efreep = ep;
	}
}
