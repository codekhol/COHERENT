/* (-lgl
 * 	COHERENT 386 Device Driver Kit release 2.0
 * 	Copyright (c) 1982, 1992 by Mark Williams Company.
 * 	All rights reserved. May not be copied without permission.
 -lgl) */
/*
 * File:	sbp.c
 *
 * Purpose:	Sound Blaster Pro device driver
 *
 *	Master devices are named /dev/sbp[pqrs][0-f].
 *	Corresponding slaves are /dev/tty[pqrs][0-f].
 *
 */

/*
 * -----------------------------------------------------------------
 * Includes.
 */
#include <sys/coherent.h>
#include <sys/stat.h>
#include <sys/proc.h>
#include <sys/tty.h>		/* indirectly includes sgtty.h */
#include <sys/con.h>
#include <sys/devices.h>
#include <errno.h>
#include <poll.h>
#include <sys/sched.h>		/* CVTTOUT, IVTTOUT, SVTTOUT */

/*
 * -----------------------------------------------------------------
 * Definitions.
 *	Constants.
 *	Macros with argument lists.
 *	Typedefs.
 *	Enums.
 */

#define channel(dev)	(dev & 0x7F)
#define master(dev)	(dev & 0x80)
#ifdef _I386
#define	EEBUSY	EBUSY
#else
#define	EEBUSY	EDBUSY
#endif

/*
 * -----------------------------------------------------------------
 * Functions.
 *	Import Functions.
 *	Export Functions.
 *	Local Functions.
 */
int nulldev();

/*
 * Configuration functions (local functions).
 */
static void sbpclose();
static void sbpioctl();
static void sbpload();
static void sbpopen();
static void sbpread();
static void sbpunload();
static void sbpwrite();
static void sbpstart();
static int sbppoll();

/*
 * Support functions (local functions).
 */

/*
 * -----------------------------------------------------------------
 * Global Data.
 *	Import Variables.
 *	Export Variables.
 *	Local Variables.
 */
int	SBP_BASE	= 0x220;
int	SBP_IRQ		= 7;
int	SBP_DMA_CHAN	= 1;

/*
 * Configuration table (export data).
 */
CON sbpcon ={
	DFCHR|DFPOL,			/* Flags */
	SBP_MAJOR,			/* Major index */
	sbpopen,			/* Open */
	sbpclose,			/* Close */
	nulldev,			/* Block */
	sbpread,			/* Read */
	sbpwrite,			/* Write */
	sbpioctl,			/* Ioctl */
	nulldev,			/* Powerfail */
	nulldev,			/* Timeout */
	sbpload,			/* Load */
	sbpunload,			/* Unload */
	sbppoll				/* Poll */
};

/*
 * -----------------------------------------------------------------
 * Code.
 */

/*
 * sbpload()
 */
static void
sbpload()
{
	printf("Loaded sbp driver at major number %d\n", SBP_MAJOR);
}

/*
 * sbpunload()
 */
static void
sbpunload()
{
}

/*
 * sbpopen()
 */
static void
sbpopen(dev, mode)
dev_t dev;
int mode;
{
}

/*
 * sbpclose()
 */
static void
sbpclose(dev, mode)
dev_t dev;
int mode;
{
}

/*
 * sbpread()
 */
static void
sbpread(dev, iop)
dev_t dev;
register IO * iop;
{
}

/*
 * sbpwrite()
 */
static void
sbpwrite(dev, iop)
dev_t dev;
register IO * iop;
{
}

/*
 * sbpioctl()
 */
static void
sbpioctl(dev, com, vec)
dev_t	dev;
int	com;
struct sgttyb *vec;
{
}

/*
 * sbppoll()
 */
static int
sbppoll(dev, ev, msec)
dev_t dev;
int ev;
int msec;
{
}

/* end of sbp.c */
