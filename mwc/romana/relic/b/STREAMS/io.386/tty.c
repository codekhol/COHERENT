/*
 * File:	$USRSRC/ttydrv/tty.c
 *
 * Purpose:	COHERENT line discipline module.
 *	This is the common part of typewriter service. It handles all device-
 *	independent aspects of a typewriter, including tandem flow control,
 *	erase and kill, stop and start, and common ioctl functions.
 *
 */

/*
 * About STOP flag bits:
 *	T_ISTOP is set when the tty module's input queue is in danger of
 *		overflow.  It is up to the device driver to check this flag
 *		and do something about it.  If ttin() is called with a
 *		character from the device while T_ISTOP is set, the  character
 *		is discarded.  T_ISTOP is cleared when the input queue is
 *		sufficiently empty.  The device driver can monitor this bit for
 *		hardware flow control.
 *	T_TSTOP is the "Tandem" flow control flag for input.  If TANDEM is set
 *		and the input queue is in danger of overflow, t_stopc is sent
 *		and T_TSTOP is set.  When the input queue is empty enough,
 *		t_startc is sent and T_TSTOP is cleared.
 *	T_STOP is the flow control bit for output.  No output will be
 *		written to the output queue while this bit is true.
 *		Except for initialization of flags in the TTY struct, by
 *		ttopen(), this bit is not written by tty.c.
 *	91/09/15 - hal
 */
/*
 * About VMIN and VTIME:
 * These parameters only apply when ICANON is zero.
 * If VMIN > 0 and VTIME = 0, block until VMIN characters are received.
 * If VMIN > 0 and VTIME > 0, block until the first character is received,
 *   then return after VMIN characters are received or VTIME/10 seconds
 *   have elapsed since last character, whichever comes first.
 * If VMIN = 0, return after first character or after VTIME/10 seconds.
 *   (may return with read count of zero - but will return one character
 *   if it is available, even if VTIME is zero).
 */

/*
 * Includes.
 */
#include <kernel/v_types.h>

#include <sys/coherent.h>
#include <sys/clist.h>
#include <sys/con.h>
#include <sys/deftty.h>
#include <sys/io.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/stat.h>
#include <sys/tty.h>
#include <sys/errno.h>
#ifdef _I386
#include <termio.h>
#include <sys/file.h>
#else
#define kucopyS(k, u, n)	kucopy(k, u, n)
#define ukcopyS(u, k, n)	ukcopy(u, k, n)
#endif

#ifdef TRACER
#define DUMPSGTTY(sp)	dumpsgtty(sp)

static void dumpsgtty(sp)
struct sgttyb * sp;
{
	T_HAL(2, printf("S:%x:%x ", sp->sg_ispeed, sp->sg_flags));
}
#else
#define DUMPSGTTY(sp)
#endif

/*
 * Definitions.
 *	Constants.
 *	Macros with argument lists.
 *	Typedefs.
 *	Enums.
 */

#define	SGTTY_CPY_LEN	(sizeof (struct sgttyb))

/* NEAR_OR_FAR_CALL is for invoking t_param and t_start */
#ifdef _I386
#define	 NEAR_OR_FAR_CALL(tp_fn)  { (*tp->tp_fn)(tp); }
#define SET_HPCL { \
	if (tp->t_termio.c_cflag & HUPCL) \
		tp->t_flags |= T_HPCL; \
	else \
		tp->t_flags &= ~T_HPCL; }
#else
#define	 NEAR_OR_FAR_CALL(tp_fn)  {\
	if (tp->t_cs_sel) \
		ld_call(tp->t_cs_sel, tp->tp_fn, tp); \
	else \
		(*tp->tp_fn)(tp); }
#define SET_HPCL
#endif

/*
 * Functions.
 *	Import Functions.
 *	Export Functions.
 *	Local Functions.
 */
void ttclose();
void ttflush();
void tthup();
void ttin();
void ttinflush();
int  ttinp();
void ttioctl();
void ttopen();
int  ttout();
void ttoutflush();
int  ttoutp();
int  ttpoll();
void ttread();
void ttread0();
void ttsetgrp();
void ttsignal();
void ttstart();
void ttwrite();
void ttwrite0();

static void ttstash();
static void ttrtp();

#ifdef _I386
static void make_termio();
static void make_sg();
#else
#define make_termio(a1,a2,a3)
#define make_sg(a1,a2,a3)
#endif

/*
 * Global Data.
 *	Import Variables.
 *	Export Variables.
 *	Local Variables.
 */
extern	int	wakeup();
extern	void	pollwake();

/*
 * ttopen()
 *
 *	Called by driver on first open.
 *	Set up defaults.
 */
void
ttopen(tp)
register TTY *tp;
{
	tp->t_escape = 0;
	tp->t_sgttyb.sg_ispeed = tp->t_dispeed;
	tp->t_sgttyb.sg_ospeed = tp->t_dospeed;
	tp->t_sgttyb.sg_erase  = DEF_SG_ERASE;
	tp->t_sgttyb.sg_kill   = DEF_SG_KILL;
	tp->t_sgttyb.sg_flags  = DEF_SG_FLAGS;
	tp->t_tchars.t_intrc   = DEF_T_INTRC;
	tp->t_tchars.t_quitc   = DEF_T_QUITC;
	tp->t_tchars.t_startc  = DEF_T_STARTC;
	tp->t_tchars.t_stopc   = DEF_T_STOPC;
	tp->t_tchars.t_eofc    = DEF_T_EOFC;
	tp->t_tchars.t_brkc    = DEF_T_BRKC;

#ifdef _I386
	tp->t_termio.c_lflag |= ICANON;
	tp->t_termio.c_cc[VEOL] = '\n';
	tp->t_termio.c_cc[VEOF] = __CTRL ('D');
	make_termio(&tp->t_sgttyb, &tp->t_tchars, &tp->t_termio);
	if (tp->t_flags & T_HPCL)
		tp->t_termio.c_cflag |= HUPCL;
	else
		tp->t_termio.c_cflag &= ~HUPCL;
	tp->t_termio.c_cflag |= (CS8|CREAD);
#endif

	if (tp->t_param)
		NEAR_OR_FAR_CALL(t_param)
}

/*
 * ttsetgrp()
 *
 *	If process is a group leader without a control terminal,
 *	make its control terminal this device.
 *
 *	If process is a group leader and this device does not have
 *	a process group, give it the group of the current process.
 */
void ttsetgrp(tp, ctdev, mode)
register TTY *tp;
dev_t ctdev;
int mode;
{
	register PROC *pp;

	pp = SELF;
#ifdef _I386
	if (pp->p_group == pp->p_pid && 0 == (mode & IPNOCTTY)) {
#else
	if (pp->p_group == pp->p_pid) {
#endif
		if (pp->p_ttdev == NODEV)
			pp->p_ttdev = ctdev;
		if (tp->t_group == 0)
			tp->t_group = pp->p_pid;
	}
}

/*
 * ttyclose()
 *
 *	Called by driver on the last close.
 *	Wait for all pending output to go out.
 *	Kill input.
 */
void ttclose(tp)
register TTY *tp;
{
	register int s;

	while (tp->t_oq.cq_cc) {
		s = sphi();
		if (tp->t_oq.cq_cc) {
			tp->t_flags |= T_DRAIN;
			/* The line discipline is waiting for the tty to drain.  */
#ifdef _I386
			if (x_sleep ((char *) & tp->t_oq, pritty,
				     slpriSigCatch, "ttydrain")
			    == PROCESS_SIGNALLED) {
#else
			v_sleep((char *)&tp->t_oq, CVTTOUT, IVTTOUT, SVTTOUT,
			  "ttydrain");
			if (nondsig ()) {
#endif
				spl (s);
				break;
			}
		}
		spl(s);
	}
	ttflush(tp);
	tp->t_flags = tp->t_group = 0;
}

/*
 * ttread()
 *
 *	The read routine for a tty device driver will call this function.
 *
 *	Move data from tp->t_iq to io segment iop.
 *	Number of characters to copy is in iop->ioc.
 *
 *	In cooked mode, copy up to the first newline or break character, or
 *	until the count runs out.
 *	In CBREAK or RAW modes, return when count runs out or when input clist
 *	is empty and we're returning at least one byte.
 */

void ttread(tp, iop)
register TTY *tp;
register IO *iop;
{
	ttread0(tp, iop, 0, 0, 0, 0);
}

/* VTIME value is in 10ths of a second */
/* VTICKS is number of cpu ticks per VTIME unit */
#define VTICKS	(HZ/10)

/*
 * ttread0()
 *
 *	Move data from user (in IO struct) to clists.
 *	Do wakeups on functions supplied when read is blocked or completed.
 */
void ttread0(tp, iop, func1, arg1, func2, arg2)
register TTY *tp;
register IO *iop;
int (*func1)(), arg1, (*func2)(), arg2;
{
	register int c;
	int sioc = iop->io_ioc;  /* number of bytes to read */
#ifdef _I386
	int time0 = lbolt;
	int timing = 0;		/* a boolean flag */
	int got_ch = 0;		/* a boolean flag */
	unsigned char vtime = tp->t_termio.c_cc[VTIME];
	unsigned char vmin = tp->t_termio.c_cc[VMIN];
#endif

	while (iop->io_ioc) {
		pl_t		prev_pl;
#ifdef _I386
		/*
		 * Start VTIME timer if we got a character or vmin is zero.
		 */
		if (! _IS_CANON_MODE (tp) && vtime) {
			if (got_ch || vmin == 0) {
				timing = 1;
				time0 = lbolt;
				timeout(&tp->t_vtime, vtime*VTICKS, wakeup,
				  &tp->t_iq);
			}
		}
#endif

		prev_pl = sphi();
		while ((c = cltgetq (& tp->t_iq)) < 0) {
			if ((tp->t_flags & T_CARR) == 0) {
				u.u_error = EIO;  /* error since no carrier */
				spl (prev_pl);
				goto read_done;
			}

#ifdef _I386
			/*
			 * [T_BRD handling in COH 286 is replaced by
			 * VMIN/VTIME handling in COH 386.]
			 *
			 * If vmin is nonzero, and at least vmin characters
			 * have been received, return.
			 *
			 * If vmin is zero and vtime is zero, return
			 * whether characters have been received or not.
			 *
			 * If vmin is zero, and we got a char, return.
			 *
			 * If vtime is nonzero, and vtime 10th seconds have
			 * elapsed, return.
			 *
			 * Otherwise, go to sleep until more input arrives.
			 */
			if (! _IS_CANON_MODE (tp)) {
				if (vmin) {
					/* received vmin or more characters? */
					if ((sioc - iop->io_ioc) >= vmin) {
						spl (prev_pl);
						goto read_done;
					}
				} else {
					if (got_ch || vtime == 0) {
						spl (prev_pl);
						goto read_done;
					}
				}
			}
			if (timing && ((lbolt - time0)/VTICKS) >= vtime) {
				spl (prev_pl);
				goto read_done;
			}
#else
			/* If we're in CBREAK or RAW mode, and we don't */
			/* have the special "blocking read" bit set for */
			/* these modes, and we read at least one byte   */
		        /* of input, return immediately, since we have  */
			/* run out of characters from the clist.	*/

			if (! _IS_CANON_MODE (tp) &&
			    (tp->t_flags & T_BRD) == 0 &&
			    iop->io_ioc < sioc) {
				spl (prev_pl);
				goto read_done;
			}
#endif
			/*
			 * Non-blocking reads.
			 * Tell user process to try again later.
			 * How do we tell if we are a terminal or not? Check
			 * to see if we have a process group, for now.
			 */
			if (iop->io_flag & (IONDLY | IONONBLOCK)) {
				if (tp->t_group == 0 ||
				    (iop->io_flag & IONONBLOCK) != 0)
					u.u_error = EAGAIN;
				spl (prev_pl);
				goto read_done;
			}

			tp->t_flags |= T_INPUT;  /* wait for more data */
			if (func1)
				(*func1)(arg1);
			if (func2)
				(*func2)(arg2);
			/* The line discipline is waiting for more data.  */
#ifdef _I386
			if (x_sleep ((char *) & tp->t_iq, pritty,
				     slpriSigLjmp, "ttywait")
			    == PROCESS_SIGNALLED) {
#else
			v_sleep((char *)&tp->t_iq, CVTTIN, IVTTIN, SVTTIN,
			  "ttywait");
			if (nondsig ()) {
#endif
				if (iop->io_ioc == sioc)
					u.u_error = EINTR;
				spl (prev_pl);
				goto read_done;
			}
		}

		/* Got a character "c" from the input queue. */
		got_ch = 1;

		/*
		 * Flow control - can we turn on input from the driver yet?
		 */
		if (tp->t_iq.cq_cc <= ILOLIM) {
			if (tp->t_flags & T_ISTOP)
				tp->t_flags &= ~ T_ISTOP;
			if (_IS_TANDEM_MODE (tp) &&
			    (tp->t_flags & T_TSTOP) != 0) {
				tp->t_flags &= ~ T_TSTOP;
				while (cltputq (& tp->t_oq,
						tp->t_tchars.t_startc) < 0) {
					ttstart (tp);
					waitq ();
				}
				ttstart (tp);
			}
		}
		spl (prev_pl);
		if (_IS_CANON_MODE (tp) && _IS_EOF_CHAR (tp, c))
			goto read_done;
		if (ioputc (c, iop) < 0)
			goto read_done;
		if (_IS_CANON_MODE (tp) &&
		    (c == '\n' || _IS_BREAK_CHAR (tp, c)))
			goto read_done;
#ifdef _I386
		if (! _IS_CANON_MODE (tp) && vtime)
			timing = 1;
#endif
	}

read_done:

#ifdef _I386
	if (timing)	/* turn off the VTIME timer */
		timeout(&tp->t_vtime, 0, 0, 0);
#endif

	if (func1)
		(*func1)(arg1);
	if (func2)
		(*func2)(arg2);
	return;
}

/*
 * ttwrite()
 *
 *	Write routine.
 */
void
ttwrite(tp, iop)
register TTY *tp;
register IO *iop;
{
	ttwrite0(tp, iop, 0, 0, 0, 0);
}

/*
 * ttwrite0()
 *
 *	Move data from user (in IO struct) to clists.
 *	Do wakeups on functions supplied when write is blocked or completed.
 */
void
ttwrite0(tp, iop, func1, arg1, func2, arg2)
register TTY *tp;
register IO *iop;
int (*func1)(), arg1, (*func2)(), arg2;
{
	register int c;

	/*
	 * Non-blocking writes which can fit.
	 * NOTE: exhaustion of clists can still cause blocking writes.
	 */
	if ((iop->io_flag & (IONDLY | IONONBLOCK)) &&
	    (OHILIM >= iop->io_ioc)) {

		/*
		 * No room.
		 */
		if (tp->t_oq.cq_cc >= OHILIM-iop->io_ioc) {
			u.u_error = EAGAIN;
			return;
		}
	}

	while ((c = iogetc(iop)) >= 0) {
		pl_t		prev_pl;

		if ((tp->t_flags & T_CARR) == 0) {
			u.u_error = EIO;  /* error since no carrier */
			return;
		}

		prev_pl = sphi();

		while (tp->t_oq.cq_cc >= OHILIM) {
			ttstart (tp);
			if (tp->t_oq.cq_cc < OHILIM)
				break;
			tp->t_flags |= T_HILIM;
			if (func1)
				(*func1)(arg1);
			if (func2)
				(*func2)(arg2);
			/*
			 * The line discipline is waiting for an output
			 * queue to drain.
			 */
#ifdef _I386
			if (x_sleep ((char *) & tp->t_oq, pritty,
				     slpriSigCatch, "ttyoq")
			    == PROCESS_SIGNALLED) {
#else
			v_sleep((char *)&tp->t_oq, CVTTOUT, IVTTOUT, SVTTOUT,
				"ttyoq");
			if (nondsig ()) {
#endif
				u.u_error = EINTR;
				spl (prev_pl);
				return;
			}
		}
		while (cltputq (& tp->t_oq, c) < 0) {
			ttstart (tp);
			waitq ();
		}
		spl (prev_pl);
	}

	ttstart (tp);

	if (func1)
		(*func1)(arg1);
	if (func2)
		(*func2)(arg2);
}

/*
 * ttioctl()
 *
 *	This routine handles common typewriter ioctl functions.
 *	Note that flushing the stream now means drain the output
 *	and clear the input.
 */
void
ttioctl(tp, com, vec)
register TTY *tp;
int com;
register struct sgttyb *vec;
{
	register int	outDrain = 0;
	int rload = 0;
	int was_bbyb;
	int inFlush = 0, outFlush = 0;
	pl_t		prev_pl;

	/*
	 * Keep sgttyb, t_chars, AND termio structs for each tty device.
	 *
	 * TCSET* writes a new termio and converts so as to update
	 * sgttyb and t_chars as well.
	 *
	 * TIOCSET[NP] writes new sgttyb and converts so as to update termio.
	 *
	 * TIOCSETC writes new t_chars and converts so as to update termio.
	 */
	switch (com) {
#if	_I386
	case TCGETA:
		if (! kucopyS (& tp->t_termio, vec, sizeof (struct termio)))
			return;
		break;

	case TCSETA:
		was_bbyb = ! _IS_CANON_MODE (tp);	/* previous mode */
		if (! ukcopyS (vec, & tp->t_termio, sizeof (struct termio)))
			return;
		make_sg (vec, & tp->t_sgttyb, & tp->t_tchars);
		SET_HPCL;
		++ rload;
		if (! was_bbyb && ! _IS_CANON_MODE (tp))
			ttrtp (tp);
		break;

	case TCSETAW:
		was_bbyb = ! _IS_CANON_MODE (tp);	/* previous mode */
		if (! ukcopyS (vec, & tp->t_termio, sizeof (struct termio)))
			return;
		make_sg (vec, & tp->t_sgttyb, & tp->t_tchars);
		SET_HPCL;
		++ outDrain;	/* delay for output */
		++ rload;
		if (! was_bbyb && ! _IS_CANON_MODE (tp))
			ttrtp (tp);
		break;

	case TCSETAF:
		if (! ukcopyS (vec, & tp->t_termio, sizeof (struct termio)))
			return;
		make_sg (vec, & tp->t_sgttyb, & tp->t_tchars);
		SET_HPCL;
	        ++ inFlush;	/* flush input */
		++ outDrain;	/* delay for output */
		++ rload;
		break;
#endif

	case TIOCQUERY:
		kucopyS (& tp->t_iq.cq_cc, vec, sizeof (int));
		break;

	case TIOCGETP:
		/*
		 * The addressability checking for this is carried out at
		 * a higher level in order to support i286 Coherent binaries.
		 */
		kucopy (& tp->t_sgttyb, vec, SGTTY_CPY_LEN);
		break;

	case TIOCSETP:
		DUMPSGTTY (& tp->t_sgttyb);
		++ inFlush;	/* flush input */
		++ outDrain;	/* delay for output */
		++ rload;

		/*
		 * The addressability checking for this is carried out at
		 * a higher level in order to support i286 Coherent binaries.
		 */

		ukcopy (vec, & tp->t_sgttyb, SGTTY_CPY_LEN);
		make_termio (& tp->t_sgttyb, & tp->t_tchars, & tp->t_termio);
		break;

	case TIOCSETN:
		was_bbyb = ! _IS_CANON_MODE (tp);	/* previous mode */
		DUMPSGTTY (& tp->t_sgttyb);
		++ rload;

		/*
		 * The addressability checking for this is carried out at
		 * a higher level in order to support i286 Coherent binaries.
		 */

		ukcopy(vec, & tp->t_sgttyb, SGTTY_CPY_LEN);
		make_termio (& tp->t_sgttyb, & tp->t_tchars, & tp->t_termio);

		if (! was_bbyb && ! _IS_CANON_MODE (tp))
			ttrtp (tp);
		break;

	case TIOCGETC:
		kucopyS (& tp->t_tchars, vec, sizeof (struct tchars));
		break;

	case TIOCSETC:
		++ rload;
		++ outDrain;
		if (! ukcopyS (vec, & tp->t_tchars, sizeof (struct tchars)))
			return;
		make_termio (& tp->t_sgttyb, & tp->t_tchars, & tp->t_termio);
		break;

	case TIOCEXCL:
		prev_pl = sphi ();
		tp->t_flags |= T_EXCL;
		spl (prev_pl);
		break;

	case TIOCNXCL:
		prev_pl = sphi ();
		tp->t_flags &= ~T_EXCL;
		spl (prev_pl);
		break;

	case TIOCHPCL:		/* set hangup on last close */
		prev_pl = sphi ();
		tp->t_flags |= T_HPCL;
		spl (prev_pl);
#ifdef _I386
		tp->t_termio.c_cflag |= HUPCL;
#endif
		break;

	case TIOCCHPCL:		/* don't hangup on last close */
		if (! super ())	/* only superuser may do this */
			u.u_error = EPERM;        /* not su */
		else {
			prev_pl = sphi ();
			tp->t_flags &= ~T_HPCL;   /* turn off hangup bit */
			spl (prev_pl);
#ifdef _I386
			tp->t_termio.c_cflag &= ~HUPCL;
#endif
		}
		break;

	case TIOCGETTF:		/* get tty flag word */
		kucopyS (& tp->t_flags, (unsigned *) vec, sizeof (unsigned));
		break;

#ifdef _I386
	case TCFLSH:
		switch ((int) vec) {
		case 0:  inFlush ++;  break;
		case 1:  outFlush ++;  break;
		case 2:  inFlush ++; outFlush ++;  break;
		default: u.u_error = EINVAL;
		}
		break;

	case TCSBRK:
		++ outDrain;
		break;

#endif
	case TIOCFLUSH:
		++ inFlush;	/* flush both input and output */
		++ outFlush;
/*		++ outDrain;	Why? - hws - 91/11/22	*/
		break;

#ifndef _I386
	case TIOCBREAD:		/* blocking read for CBREAK/RAW mode */
		prev_pl = sphi ();
		tp->t_flags |= T_BRD;
		spl (prev_pl);
		break;

	case TIOCCBREAD:	/* turn off CBREAK/RAW blocking read mode */
		prev_pl = sphi ();
		tp->t_flags &= ~ T_BRD;
		spl (prev_pl);
		break;
#endif
	/*
	 * The following is a hack so that the process group for /dev/console
	 * contains the current login shell running on it.
	 * Only expect /etc/init to use this ugliness.
	 */
	case TIOCSETG:
		if (super ())
			tp->t_group = SELF->p_group;
		break;
	default:
		u.u_error = EINVAL;
	}

	/*
	 * T_STOP is set under two conditions:
	 * - a modem control device is awaiting carrier
	 * - a stopc (usually Ctrl-S) character was received.
	 *
	 * If ioctl just put device into RAWIN mode, make sure device
	 * is not still waiting for startc.
	 */
#if _I386
	/* Is XON/XOFF flow control off *and* we are waiting for startc? */
	if (! _IS_IXON_MODE (tp) && (tp->t_flags & T_XSTOP) != 0) {
		prev_pl = sphi ();
		tp->t_flags &= ~ (T_STOP | T_XSTOP);
		spl (prev_pl);

		ttstart (tp);
	}
#else
	if (! _IS_IXON_MODE (tp)) && (tp->t_flags & T_STOP) != 0 &&
	    (tp->t_flags & T_HOPEN) == 0) {
		prev_pl = sphi ();
		tp->t_flags &= ~ T_STOP;
		spl (prev_pl);

		ttstart (tp);
	}
#endif

	/*
	 * Wait for output to drain, or signal to arrive.
	 *
	 * NOTE:  This stuff is properly done within the device driver,
	 * *before* ttioctl() is called.  This paragraph should disappear
	 * from tty.c, with maybe an entry point added for the driver to
	 * drain the queue while draining the peripheral device. -hws-
	 */

	if (outDrain) {
		while (tp->t_oq.cq_cc) {
			prev_pl = sphi ();
			tp->t_flags |= T_DRAIN;
			spl (prev_pl);
			/* A TIOC has asked for tty output to drain.  */
#ifdef _I386
			if (x_sleep ((char *) & tp->t_oq, pritty,
				     slpriSigCatch, "ttyiodrn")
			    == PROCESS_SIGNALLED) {
#else
			v_sleep((char *)&tp->t_oq, CVTTOUT, IVTTOUT, SVTTOUT,
			  "ttyiodrn");
			if (nondsig ()) {
#endif
				break;
			}
		}
	}

	/*
	 * Flush.
	 */
	if (inFlush)
		ttinflush(tp);
	if (outFlush)
		ttoutflush(tp);

	/*
	 * Re-initialize hardware.
	 */
	if (rload) {
		if (tp->t_param)
			NEAR_OR_FAR_CALL(t_param)
	}
}

/*
 * ttpoll()
 *
 *	Polling routine.
 *	[System V.3 Compatible]
 */
int ttpoll(tp, ev, msec)
register TTY * tp;
int ev;
int msec;
{
	/*
	 * Priority polls not supported.
	 */
	ev &= ~POLLPRI;

	/*
	 * Input poll with no data present.
	 */
	if ((ev & POLLIN) && (tp->t_iq.cq_cc == 0)) {

		/*
		 * Blocking input poll.
		 */
		if (msec)
			pollopen(&tp->t_ipolls);

		/*
		 * Second look to avoid interrupt race.
		 */
		if (tp->t_iq.cq_cc == 0)
			ev &= ~POLLIN;
	}

	/*
	 * Output poll with no space.
	 */
	if ((ev & POLLOUT) && (tp->t_oq.cq_cc >= OLOLIM)) {

		/*
		 * Blocking output poll.
		 */
		if (msec)
			pollopen(&tp->t_opolls);

		/*
		 * Second look to avoid interrupt race.
		 */
		if (tp->t_oq.cq_cc >= OLOLIM)
			ev &= ~POLLOUT;
	}

	if (((ev & POLLIN) == 0) && ((tp->t_flags & T_CARR) == 0))
		ev |= POLLHUP;

	return ev;
}

/*
 * ttout()
 *
 *	Pull a character from the output queues of the typewriter.
 *	Doing fills, newline insert, tab expansion, etc.
 *
 *	If the stream is empty return a -1.
 *	Called at high priority.
 */
int ttout(tp)
register TTY *tp;
{
	register int c;

	if (tp->t_nfill) {
		--tp->t_nfill;
		c = tp->t_fillb;
	} else if (tp->t_flags & T_INL) {
		tp->t_flags &= ~T_INL;
		c = '\n';
	} else {
		if ((c=cltgetq(&tp->t_oq)) < 0)
			return -1;
		if (! _IS_RAW_OUT_MODE (tp)) {
			if (c == '\n' && _IS_ONLCR_MODE (tp)) {
				tp->t_flags |= T_INL;
				c = '\r';
			} else if (c == '\r' && _IS_OCRNL_MODE (tp)) {
				c = '\n';
			} else if (c == '\t' && _IS_XTABS_MODE (tp)) {
				tp->t_nfill = ~ (tp->t_hpos | ~ 7);
				tp->t_fillb = ' ';
				c = ' ';
			}
		}
	}
	if (! _IS_RAW_OUT_MODE (tp)) {
		if (c == '\b') {
			if (tp->t_hpos)
				-- tp->t_hpos;
		} else if (c == '\r')
			tp->t_hpos = 0;
		else if (c == '\t')
			tp->t_hpos = (tp->t_hpos | 7) + 1;
#if NOT_8_BIT
		else if (c >= ' ' && c <= '~')
#else
		else if ((c >= ' ' && c <= '~') || (c >= 0200 && c <= 0376))
#endif
			++ tp->t_hpos;
	}
	return c;
}

/*
 * ttin()
 *
 *	Pass a character to the device independent typewriter routines.
 *	Handle erase and kill, tandem flow control, and other magic.
 *	Often called at high priority from the driver's interrupt routine.
 */
void
ttin(tp, c)
register TTY *tp;
register int c;
{
	int dc, i, n;
	pl_t		prev_pl;

	if (_IS_ISTRIP_MODE (tp))
		c &= 0x7F;

	if (_IS_ISIG_MODE (tp) && _IS_QUIT_CHAR (tp, c)) {
		ttsignal (tp, SIGQUIT);
		goto ttin_ret;
	}

	if (_IS_ISIG_MODE (tp) && _IS_INTERRUPT_CHAR (tp, c)) {
		ttsignal (tp, SIGINT);
		goto ttin_ret;
	}

	if (tp->t_flags & T_ISTOP)
		goto ttin_ret;

	if (_IS_ICRNL_MODE (tp) && ! _IS_IGNCR_MODE (tp)) {
		if (c == '\r')
			c = '\n';
	}

	if (! _IS_RAW_INPUT_MODE (tp)) {
		if (tp->t_escape) {
			if (c == ESC)
				++ tp->t_escape;
			else {
				if (_IS_ERASE_CHAR (tp, c) ||
				    _IS_KILL_CHAR (tp, c)) {
					c |= 0x80;
					-- tp->t_escape;
				}
				while (tp->t_escape && tp->t_ibx < NCIB - 1) {
					tp->t_ib [tp->t_ibx ++] = ESC;
					-- tp->t_escape;
				}
				ttstash (tp, c);
			}
			if (_IS_ECHO_MODE (tp)) {
#if NOT_8_BIT
				cltputq (& tp->t_oq, c & 0x7F);
#else
				cltputq (& tp->t_oq, c); /* no strip for 8-bit */
#endif
				ttstart (tp);
			}
			goto ttin_ret;
		}
		if (_IS_ERASE_CHAR (tp, c) && _IS_CANON_MODE (tp)) {
			while (tp->t_escape && tp->t_ibx < NCIB - 1) {
				tp->t_ib [tp->t_ibx ++] = ESC;
				-- tp->t_escape;
			}
			if (tp->t_ibx == 0)
				goto ttin_ret;
			dc = tp->t_ib [-- tp->t_ibx];
			if (_IS_ECHO_MODE (tp)) {
				if (! _IS_CRT_MODE (tp))
					cltputq (& tp->t_oq, c);
				/* don't erase for bell, null, or rubout */
#if NOT_8_BIT
				else if ((c = dc & 0x7F) == '\a' || c == 0 ||
					 c == 0x7F)
#else
				else if ((c = dc) == '\a' || c == 0 ||
					 c == 0x7F || c == 0xFF)
#endif
				        goto ttin_ret;
				else if (c != '\b' && c != '\t') {
					cltputq (& tp->t_oq, '\b');
					cltputq (& tp->t_oq,  ' ');
					cltputq (& tp->t_oq, '\b');
				} else if (c == '\t') {
					n = tp->t_opos + tp->t_escape;
					for (i = 0 ; i < tp->t_ibx ; ++ i) {
						c = tp->t_ib [i];
#if NOT_8_BIT
						if (c & 0x80) {
							++ n;
							c &= 0x7F;
						}
#endif
						if (c == '\b')
							-- n;
						else {
							if (c == '\t')
								n |= 07;
							++ n;
						}
					}
					while (n ++ < tp->t_hpos)
						cltputq (& tp->t_oq, '\b');
				}
#if NOT_8_BIT
				if (dc & 0x80) {
					if ((dc & 0x7F) != '\b')
						cltputq (& tp->t_oq, '\b');
					cltputq (& tp->t_oq,  ' ');
					cltputq (& tp->t_oq, '\b');
				}
#endif
				ttstart (tp);
			}
			goto ttin_ret;
		}
		if (_IS_KILL_CHAR (tp, c) && _IS_CANON_MODE (tp)) {
			tp->t_ibx = 0;
			tp->t_escape = 0;
			if (_IS_ECHO_MODE (tp)) {
				if (c < 0x20) {
					cltputq (& tp->t_oq, '^');
					c += 0x40;
				}
				cltputq (& tp->t_oq, c);
				cltputq (& tp->t_oq, '\n');
				ttstart (tp);
			}
			goto ttin_ret;
		}
	}
	if (! _IS_CANON_MODE (tp)) {
		cltputq (& tp->t_iq, c);
		if (tp->t_flags & T_INPUT) {
			prev_pl = sphi ();
			tp->t_flags &= ~T_INPUT;
			spl (prev_pl);
			wakeup (& tp->t_iq);
		}
		if (tp->t_ipolls.e_procp) {
			tp->t_ipolls.e_procp = 0;
			pollwake ((char *) & tp->t_ipolls);
		}
	} else {
		if (tp->t_ibx == 0)
			tp->t_opos = tp->t_hpos;
		if (c == ESC)
			++ tp->t_escape;
		else
			ttstash(tp, c);
	}
	if (_IS_ECHO_MODE (tp)) {
		if (_IS_RAW_INPUT_MODE (tp) || ! _IS_EOF_CHAR (tp, c)) {
			cltputq (& tp->t_oq, c);
			ttstart (tp);
		}
	}
	if ((n = tp->t_iq.cq_cc) >= IHILIM) {
		prev_pl = sphi ();
		tp->t_flags |= T_ISTOP;
		spl (prev_pl);
	} else if (_IS_TANDEM_MODE (tp) && (tp->t_flags & T_TSTOP)==0 &&
		   n >= ITSLIM) {
		prev_pl = sphi ();
		tp->t_flags |= T_TSTOP;
		spl (prev_pl);
		cltputq (& tp->t_oq, tp->t_tchars.t_stopc);
		ttstart (tp);
	}

ttin_ret:
	return;
}

/*
 * ttstash()
 *
 *	Cooked mode.
 *	Put character in the buffer and check for end of line.
 *	Only a legal end of line can take the last character position.
 *
 *	Only called from ttin(), and ttin() is called at high priority.
 */
static void ttstash(tp, c)
register TTY *tp;
{
	register char *p1, *p2;

	if (c == '\n' || _IS_EOF_CHAR (tp, c) || _IS_BREAK_CHAR (tp, c)) {
		p1 = & tp->t_ib [0];
		p2 = & tp->t_ib [tp->t_ibx];
		* p2 ++ = c;			/* Always room */
		while (p1 < p2)
#if NOT_8_BIT
			cltputq (& tp->t_iq, (* p1 ++) & 0x7F);
#else
			cltputq (& tp->t_iq, (* p1 ++));
#endif
		tp->t_ibx = 0;
		tp->t_escape = 0;

		if (tp->t_flags & T_INPUT) {
			tp->t_flags &= ~ T_INPUT;
			wakeup (& tp->t_iq);
		}

		if (tp->t_ipolls.e_procp) {
			tp->t_ipolls.e_procp = 0;
			pollwake ((char *) & tp->t_ipolls);
		}

	} else if (tp->t_ibx < NCIB - 1)
		tp->t_ib [tp->t_ibx ++] = c;
}

/*
 * ttstart()
 *
 *	Start output on a tty.
 *	Duck out if stopped.  Do wakeups.
 */
void ttstart(tp)
register TTY *tp;
{
	register int n;
	pl_t		prev_pl;

	n = tp->t_flags;
	if (n & T_STOP)
		goto stdone;

	if ((n & T_DRAIN) != 0 && tp->t_oq.cq_cc == 0 && (n & T_INL) == 0 &&
	    tp->t_nfill == 0) {
		prev_pl = sphi ();
		tp->t_flags &= ~ T_DRAIN;
		spl (prev_pl);
		wakeup (& tp->t_oq);
		goto stdone;
	}

	NEAR_OR_FAR_CALL(t_start)

	if (tp->t_oq.cq_cc > OLOLIM)
		goto stdone;

	if (n & T_HILIM) {
		prev_pl = sphi ();
	   	tp->t_flags &= ~ T_HILIM;
		spl (prev_pl);
		wakeup (& tp->t_oq);
	}

	if (tp->t_opolls.e_procp) {
		tp->t_opolls.e_procp = 0;
		pollwake ((char *) & tp->t_opolls);
	}
stdone:
	return;
}

/*
 * ttflush()
 *
 *	Flush tty input and output queues.
 */
void
ttflush(tp)
register TTY *tp;
{
	ttinflush (tp);
	ttoutflush (tp);
}

/*
 * ttinflush()
 *
 *	Flush input queues.
 */
void
ttinflush(tp)
register TTY *tp;
{
	clrq (& tp->t_iq);

	if (tp->t_flags & T_INPUT) {
		wakeup (& tp->t_iq);
	}

	if (tp->t_ipolls.e_procp) {
		tp->t_ipolls.e_procp = 0;
		pollwake ((char *) & tp->t_ipolls);
	}

	tp->t_ibx = 0;
	tp->t_escape = 0;
}

/*
 * ttoutflush()
 *
 *	Flush tty output queues.
 */
void
ttoutflush(tp)
register TTY *tp;
{
	clrq(&tp->t_oq);

	if (tp->t_flags & (T_DRAIN | T_HILIM)) {
		wakeup (& tp->t_oq);
	}

	if (tp->t_opolls.e_procp) {
		tp->t_opolls.e_procp = 0;
		pollwake ((char *) & tp->t_opolls);
	}
}

/*
 * ttsignal()
 *
 *	Send a signal to every process in the given process group.
 */
void
ttsignal(tp, sig)
TTY *tp;
int sig;
{
	register int g;
	register PROC *pp;

	g = tp->t_group;
	if (g == 0)
		goto sigdone;
	ttflush(tp);
	pp = & procq;
	while ((pp = pp->p_nforw) != & procq) {
		if (pp->p_group == g)
			sendsig(sig, pp);
	}
sigdone:
	return;
}

/*
 * tthup()
 *
 *	Flag hangup internally to force errors on tty read/write, flush tty,
 *	then send hangup signal.
 */
void tthup(tp)
register TTY *tp;
{
	ttflush (tp);
	ttsignal (tp, SIGHUP);
}

#if	_I386
/*
 * Convert from sgttyb and tchars structs to termio.
 */
static void
make_termio(sgp, tcp, trp)
struct sgttyb * sgp;
struct tchars * tcp;
struct termio * trp;
{
	char	vmin = 1, vtime = 0;
	char	veof = 4, veol = 10; /* default to ^D, ^J */

	/*
	 * If VMIN/VTIME are active, save now for possible restore.
	 */
	if ((trp->c_lflag & ICANON) == 0) {
		vmin = trp->c_cc [VMIN];
		vtime = trp->c_cc [VTIME];
	} else {
		veol = trp->c_cc [VEOL];
	}

	trp->c_cc [VINTR] = tcp->t_intrc;
	trp->c_cc [VQUIT] = tcp->t_quitc;
	veof = tcp->t_eofc;
	trp->c_cc [VERASE] = sgp->sg_erase;
	trp->c_cc [VKILL ] = sgp->sg_kill;

	trp->c_iflag = BRKINT | IXON | IGNPAR | INPCK;
	trp->c_oflag = OPOST;
	trp->c_cflag &= CSIZE | HUPCL | CLOCAL | CREAD;
	trp->c_lflag = ICANON | ISIG | ECHONL | ECHOK;

	if (sgp->sg_flags & TANDEM)
		trp->c_iflag |= IXOFF;

	if (sgp->sg_flags & CRMOD) {
		trp->c_iflag |= ICRNL;
		trp->c_oflag |= ONLCR;
	}

	if (sgp->sg_flags & LCASE) {
		trp->c_lflag |= XCASE;
		trp->c_iflag |= IUCLC;
		trp->c_oflag |= OLCUC;
	}

	if (sgp->sg_flags & (RAW | RAWIN)) {
		trp->c_iflag = 0;
		trp->c_cflag &= ~ (PARODD|PARENB);
		trp->c_cflag |= CS8 | CREAD;
		trp->c_lflag &= ~ (ECHONL | ISIG | ICANON);
	}

	if (sgp->sg_flags & (RAW|RAWOUT)) {
		trp->c_oflag &= ~ OPOST;
		trp->c_lflag &= ~ XCASE;
	}

	if (sgp->sg_flags & XTABS)
		trp->c_oflag |= TAB3;

	if (sgp->sg_flags & (EVENP | ODDP)) {
		trp->c_cflag |= PARENB;
		if (sgp->sg_flags & ODDP)
			trp->c_cflag |= PARODD;
	}
	trp->c_cflag |= sgp->sg_ispeed;

	if (sgp->sg_flags & CRT)
		trp->c_lflag |= ECHOE;

	if (sgp->sg_flags & CBREAK)
		trp->c_lflag &= ~ ICANON;

	if (sgp->sg_flags & ECHO)
		trp->c_lflag |= ECHO;

	/*
	 * If not doing canonical processing, set VMIN/VTIME.
	 */

	if ((trp->c_lflag & ICANON) == 0) {
		trp->c_cc [VMIN] = vmin;
		trp->c_cc [VTIME] = vtime;
	} else {
		trp->c_cc [VEOF] = veof;
		trp->c_cc [VEOL] = veol;
	}
}

/*
 * Convert from termio struct to sgttyb and tchars.
 */
static void
make_sg(trp, sgp, tcp)
struct termio * trp;
struct sgttyb * sgp;
struct tchars * tcp;
{
	T_HAL(1, printf("T:%x:%x:%x:%x ", trp->c_iflag, trp->c_oflag,
	  trp->c_cflag, trp->c_lflag));
	tcp->t_intrc = trp->c_cc [VINTR];
	tcp->t_quitc = trp->c_cc [VQUIT];
	tcp->t_startc= __CTRL ('Q');
	tcp->t_stopc = __CTRL ('S');
	tcp->t_eofc  = trp->c_cc [VEOF];
	tcp->t_brkc  = -1;

	sgp->sg_erase  = trp->c_cc [VERASE];
	sgp->sg_kill   = trp->c_cc [VKILL];
	sgp->sg_ispeed = trp->c_cflag & CBAUD;
	sgp->sg_ospeed = sgp->sg_ispeed;
	sgp->sg_flags  = RAWIN | RAWOUT | CBREAK;

	if (trp->c_lflag & ECHO)
		sgp->sg_flags |= ECHO;

	if (trp->c_lflag & ECHOE)
		sgp->sg_flags |= CRT;

	if ((trp->c_lflag & XCASE) != 0 || (trp->c_oflag & OLCUC) != 0 ||
	    (trp->c_iflag & IUCLC) != 0)
		sgp->sg_flags |= LCASE;

	if (trp->c_iflag & IXOFF)
		sgp->sg_flags |= TANDEM;

	if (trp->c_iflag & ICRNL)
		sgp->sg_flags |= CRMOD;

	if (trp->c_oflag & ONLCR)
		sgp->sg_flags |= CRMOD;

	if (trp->c_oflag & OPOST)
		sgp->sg_flags &= ~ RAWOUT;

	if ((trp->c_oflag & TABDLY) == TAB3)
		sgp->sg_flags |= XTABS;

	if (trp->c_cflag & PARENB) {
		if (trp->c_cflag & PARODD)
			sgp->sg_flags |= ODDP;
		else
			sgp->sg_flags |= EVENP;
	}

	if (trp->c_lflag & ISIG)
		sgp->sg_flags &= ~ RAWIN;

	if (trp->c_lflag & ICANON)
		sgp->sg_flags &= ~ CBREAK;
}
#endif

/*
 * ttrtp()
 *
 * Recover contents of typeahead when changing modes.
 * Called for ioctls of TIOCSETP and TCSETA,
 * when going from not byte-by-byte input to BBYB.
 */
static void
ttrtp(tp)
TTY * tp;
{
	register char	*p1, *p2;

	if (tp->t_ibx) {
		p1 = & tp->t_ib [0];
		p2 = & tp->t_ib [tp->t_ibx];
		while (p1 < p2) {
#if NOT_8_BIT
			cltputq (& tp->t_iq, (* p1 ++) & 0x7F);
#else
			cltputq (& tp->t_iq, (* p1 ++));
#endif
		}
		tp->t_ibx = 0;
	}
}

/*
 * ttinp()
 *
 * Return nonzero if ttin() may be called to send data for pickup by ttread(),
 * or 0 if not.
 */

int
ttinp(tp)
TTY * tp;
{
	return (tp->t_flags & T_ISTOP) == 0;
}

/*
 * ttoutp()
 *
 * Return nonzero if ttout() may be called to fetch data stored by ttwrite(),
 * or 0 if not.
 */

int
ttoutp(tp)
TTY * tp;
{
	return tp->t_nfill || (tp->t_flags & T_INL) != 0 || tp->t_oq.cq_cc;
}
