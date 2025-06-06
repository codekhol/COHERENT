/* (-lgl
 * 	COHERENT Driver Kit Version 1.1.0
 * 	Copyright (c) 1982, 1990 by Mark Williams Company.
 * 	All rights reserved. May not be copied without permission.
 -lgl) */
/*
 * Memory Mapped Video
 * High level output routines.
 */

/*
 *	virtual terminal additions Copyright 1991, 1992 Todd Fleming
 *	6/91, 1/92
 *
 *	Todd Fleming
 *	1991 Mountainside Drive
 *	Blacksburg, VA 24060
 */

#include <sys/coherent.h>
#include <sys/sched.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <sys/tty.h>
#include <sys/timeout.h>
#include <sys/seg.h>

/* For beeper */
#define	TIMCTL	0x43			/* Timer control port */
#define	TIMCNT	0x42			/* Counter timer port */
#define	PORTB	0x61			/* Port containing speaker enable */
#define	FREQ	((int)(1193181L/440))	/* Counter for 440 Hz. tone */

int mmtime();

char mmbeeps;		/* number of ticks remaining on bell */
char mmesc;		/* last unserviced escape character */
char mmescterm;		/* terminal for -^    (TBF) */
int  mmcrtsav = 1;	/* crt saver enabled */
int  mmvcnt   = 900;	/* seconds remaining before crt saver is activated */

/*
 * For virtual terminals
 * (TBF)
 */

#define NVIRTERMS 4		/* # of virtual terminals */
				/* must also be set in mmas.s and kb.c */

struct mmspecspacestruct	/* C translation of mmas.s data structure */
{
 unsigned *FUNC,PORT,BASE;
 unsigned char ROW,COL;
 unsigned POS;
 unsigned char ATTR,N1,N2,BROW,EROW,LROW,SROW,SCOL,IBROW,IEROW;
 unsigned INVIS;
 unsigned char SLOW,WRAP,CURTERM,UNUSED;
} extern mmspecspace[NVIRTERMS];

unsigned mmcurvirterm=1;	/* current virtual terminal */
SEG *mmswapsegs[NVIRTERMS];	/* holders for swap buffers */
unsigned mmvideobase;		/* base address of video screen */
unsigned mmtermbufs[NVIRTERMS];	/* selectors for swap buffers */
int mmvirtermsinitialized=0;
extern mmblankbuf();		/* buffer copy function */
extern mmbufcopy();		/* buffer blanking function */
extern unsigned mmcursor;	/* update cursor */
mminitvirterms();		/* initialize virtual terminals */
mmchangeterm();			/* change current terminal */

extern TTY istty[NVIRTERMS+1];

/*
 * Start the output stream.
 * Called from `ttwrite' and `isrint' routines.
 */
TIM mmtim;

mmstart(tp)
register TTY *tp;
{
	int c;
	IO iob;
	static int mmbegun;

	while ((tp->t_flags&T_STOP) == 0) {
		if ((c = ttout(tp)) < 0)
			break;
		iob.io_seg  = IOSYS;
		iob.io_ioc  = 1;
		iob.io_base = &c;
		iob.io_flag = 0;
		mmwrite( makedev(2,0), &iob );
	}

	if (mmbegun == 0) {
		++mmbegun;
		timeout(&mmtim, HZ/10, mmtime, (char *)tp);
	}
}

/*
 * Initialize virtual terminals (TBF)
 * called from kb.c
 */

mminitvirterms()
{
 if(!mmvirtermsinitialized)
 {
  int i;
  mmvirtermsinitialized=1;
  mmvideobase=mmspecspace[0].BASE;	/* save video address */
  mmcurvirterm=1;			/* set current terminal */
  for(i=0;i<NVIRTERMS;i++)
  {
   mmswapsegs[i]=salloc((fsize_t)80*25*2,SFSYST|SFHIGH|SFNSWP);	/* allocate virtual */
   mmtermbufs[i]=mmswapsegs[i]->s_faddr>>16;		/* terminal space   */
   if(i)
   {
    mmspecspace[i]=mmspecspace[0];	/* copy data structures */
    mmspecspace[i].BASE=mmtermbufs[i];	/* assign buffers to terminals */
    mmblankbuf(mmtermbufs[i]);		/* blank out buf i */
    mmspecspace[i].CURTERM=0;		/* terms 2-NVIRTERMS switched off */
   }
   else
    mmspecspace[0].CURTERM=1;		/* term 1 switched on */
  }
 }
}

/*
 * Change virtual terminals (TBF)
 * mmcurvirterm = virtual terminal from 1 to NVIRTERMS
 * mmtermbufs[from 0 to NVIRTERMS-1]
 * mmspecspace[from 0 to NVIRTERMS-1]
 */

mmchangeterm(term)
unsigned term;
{
 if(term!=mmcurvirterm)
 {
  unsigned *funchold;

  mmbufcopy(mmtermbufs[mmcurvirterm-1],mmvideobase); /* dest, source */
  mmbufcopy(mmvideobase,mmtermbufs[term-1]);

  mmspecspace[mmcurvirterm-1].BASE=mmtermbufs[mmcurvirterm-1];
  mmspecspace[mmcurvirterm-1].CURTERM=0;/* disable prev. term */
  mmspecspace[term-1].BASE=mmvideobase;		
  mmspecspace[term-1].CURTERM=1; 	/* enable new term */
  mmcurvirterm=term;			/* note the change */

  funchold=mmspecspace[term-1].FUNC;	/* preserve state */
  mmspecspace[term-1].FUNC=&mmcursor;	/* set mmgo to update cursor */
  mmgo(NULL,makedev(2,term));		/* update cursor */
  mmspecspace[term-1].FUNC=funchold;	/* restore state */
 }
}

mmtime(xp)
char *xp;
{
	register int s;

	s = sphi();
	if (mmbeeps < 0) {
		mmbeeps = 2;
		outb(TIMCTL, 0xB6);	/* Timer 2, lsb, msb, binary */
		outb(TIMCNT, FREQ&0xFF);
	        outb(TIMCNT, FREQ>>8);
		outb(PORTB, inb(PORTB) | 03);	/* Turn speaker on */
	}
	else if ((mmbeeps > 0) && (--mmbeeps == 0))
		outb( PORTB, inb(PORTB) & ~03 );

	if (mmesc) {
		ismmfunc(mmesc,mmescterm);
		mmesc = 0;
	}
	spl(s);

	ttstart( (TTY *) xp );

	timeout(&mmtim, HZ/10, mmtime, xp);
}

/**
 *
 * void
 * mmwatch()	-- turn video display off after 15 minutes inactivity.
 */
void
mmwatch()
{
	if ( (mmcrtsav > 0) && (mmvcnt > 0) && (--mmvcnt == 0) )
		mm_voff();
}

mmwrite( dev, iop )
dev_t dev;
register IO *iop;
{
	int ioc;
	int s;

	ioc = iop->io_ioc;

	/*
	 * Kernel writes.
	 */
	if (iop->io_seg == IOSYS) {
                mmescterm=mmcurvirterm; /* for kb esc processing */
		while (mmgo(iop,makedev(2,mmcurvirterm)))
			;
		if(!mmvirtermsinitialized)
		 mminitvirterms(); /* TBF */
	}

	/*
	 * Blocking user writes.
	 */
	else 
	{     
	     if(minor(dev)==0)
	      dev=makedev(2,mmcurvirterm); /*TBF */

	     if ( (iop->io_flag & IONDLY) == 0 ) {
		do {
			while (istty[minor(dev)].t_flags & T_STOP) {
				s = sphi();
				istty[minor(dev)].t_flags |= T_HILIM;
				sleep((char*) &istty[minor(dev)].t_oq,
					CVTTOUT, IVTTOUT, SVTTOUT);
				spl( s );
			}
			/*
			 * Signal received.
			 */
			if ( SELF->p_ssig && nondsig() ) {
				kbunscroll();	/* update kbd LEDS */
				/*
				 * No data transferred yet.
				 */
				if ( ioc == iop->io_ioc )
					u.u_error = EINTR;
				/*
				 * Transfer remaining data
				 * without pausing after scrolling.
				 */
				else 
				{
				 mmescterm=minor(dev);
				 while ( mmgo(iop,dev) )
					;
				}
				return;
			}
			mmescterm=minor(dev);
			mmgo(iop,dev);
		} while ( iop->io_ioc );
	}

	/*
	 * Non-blocking user writes with output stopped.
	 */
	else if ( istty[minor(dev)].t_flags & T_STOP ) {
		u.u_error = EAGAIN;
		return;
	}

	/*
	 * Non-blocking user writes do not pause after scrolling.
	 */
	else {
		mmescterm=minor(dev);
		while ( mmgo(iop,dev) )
			;
	}
 }
}
