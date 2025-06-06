/* $Header: /y/coh.386/RCS/alloc.c,v 1.4 93/04/14 10:06:13 root Exp $ */
/* (lgl-
 *	The information contained herein is a trade secret of Mark Williams
 *	Company, and  is confidential information.  It is provided  under a
 *	license agreement,  and may be  copied or disclosed  only under the
 *	terms of  that agreement.  Any  reproduction or disclosure  of this
 *	material without the express written authorization of Mark Williams
 *	Company or persuant to the license agreement is unlawful.
 *
 *	COHERENT Version 2.3.37
 *	Copyright (c) 1982, 1983, 1984.
 *	An unpublished work by Mark Williams Company, Chicago.
 *	All rights reserved.
 -lgl) */
/*
 * Coherent.
 * Storage allocator.
 *
 * $Log:	alloc.c,v $
 * Revision 1.4  93/04/14  10:06:13  root
 * r75
 * 
 * Revision 1.2  92/01/06  11:58:31  hal
 * Compile with cc.mwc.
 * 
 * Revision 1.1	88/03/24  16:13:25	src
 * Initial revision
 * 
 */
#include <sys/coherent.h>
#include <sys/alloc.h>
#include <errno.h>
#include <sys/proc.h>
#include <sys/param.h>

#ifndef TEST	/* Do not test setarena() or alloc() or free().  */
/*
 * Create an arena.
 */
ALL *
setarena(cp, n)
register char *cp;
{
	register ALL *ap1;
	register ALL *ap2;

	if ((char *)(ap1=align(cp)) < (char *)cp)
		ap1++;
	if ((ap2=align(&cp[n])-1) < ap1)
		panic("Arena %x too small", (int) cp);
	ap1->a_link = (char *)ap2;
	ap2->a_link = (char *)ap1;
	setused(ap2);
	return (ap1);
}

/*
 * Allocate `l' bytes of memory.
 */
char *
alloc(apq, l)
ALL *apq;
unsigned l;
{
	register ALL *ap;
	register ALL *ap1;
	register ALL *ap2;
	register unsigned i;
	register unsigned n;
	register unsigned s;
	char * ret = NULL;

	n = 1 + (l + sizeof(ALL) - 1) / sizeof(ALL);
	for (i=0; i<2; i++) {
		for (ap1=apq; link(ap1)!=apq; ap1=link(ap1)) {
			if (vtop(ap1) == 0) {
				panic("Corrupt arena");
				goto alloc_done;
			}
			if (tstfree(ap1)) {
			       for (ap2=link(ap1); tstfree(ap2); ap2=link(ap2))
					if (ap2 == apq)
						break;
				ap1->a_link = (char *)ap2;
				if ((s=ap2-ap1) >= n) {
					if (s > n) {
						if (i == 0)
							continue;
						ap = &ap1[n];
						ap->a_link = (char *)ap2;
						ap1->a_link = (char *)ap;
					}
					setused(ap1);
					kclear((char *)ap1->a_data, l);
					ret = ap1->a_data;
					goto alloc_done;
				}
			}
		}
	}
	u.u_error = ENOSPC;
	goto alloc_done;
alloc_done:
	return ret;
}

/*
 * Free memory.
 */
free(cp)
char *cp;
{
	register ALL *ap;
	extern char __end;

#if 0
	ap = ((ALL *)cp) - 1;
	if (ap<(ALL *)&__end || tstfree(ap))
		panic("Bad free %x\n", (unsigned)cp);
#else
	ap = ((ALL *)cp) - 1;
	if (ap<(ALL *)&__end) {
		int *r = (int *)(&cp);	/* return address */
		printf("cp=%x ap=%x &__end=%x\n", cp, ap, &__end);
		panic("Bad free() from eip=%x\n", *(r-1));
	}
	if (tstfree(ap)) {
		int *r = (int *)(&cp);	/* return address */
		printf("cp=%x tstfree(%x)=%x\n", cp, ap, tstfree(ap));
		panic("Bad free() from eip=%x\n", *(r-1));
	}
#endif
	setfree(ap);
}

#endif /* TEST */

#ifdef _I386
/*
 * unsigned char *palloc(int size);
 *
 * Allocate 'size' bytes of kernel space, which does not cross a click
 * boundary.  Returns a pointer to the space allocated on success,
 * NULL on failure.
 *
 * Allocate twice as much memory as we need, and then return a chunk that
 * does not cross a click boundary.  Immediately before the chunk that
 * we return, we store the true address of the chunk that was kalloc()'d.
 *
 * Since this routine is for relatively small short-lived objects,
 * which we expect to allocate frequently, speed is more important than
 * space overhead.
 *
 * We assume that kalloc() returns word aligned addresses.
 *
 * There are two cases:
 * There is enough room before the click boundary (or there is no click
 * 	boundary) for the pointer and the memory we need.
 * Otherwise, return the chunk starting at the click boundary, storing
 *	the pointer right before the click boundary.  This trick allows
 *	us to allocate up to 1 full click.
 *
 * If kalloc() did NOT return word aligned chunks, then there would be
 * a third case, where there might not be enough space for the pointer
 * before the click boundary.
 */

#define c_boundry(x)	ctob(btoc((x)+1)) /* Next click boundary above x.  */
#define VOID	unsigned char

#ifdef TEST
#undef kalloc
#undef kfree
VOID *kalloc();
void kfree();
#endif /* TEST */

VOID *
palloc(size)
	int size;	/* Size in bytes of area to allocate.  */
{
	VOID *local_arena;	/* Value returned by kalloc().  */
	VOID *boundry;		/* Next click boundry above local_arena.  */
	VOID *retval;		/* What we give back to our caller.  */

	if (size > NBPC) {
		panic("palloc(%x): can not palloc more than 1 click.", size);
	}

	/* Fetch twice as much space as requested, plus a pointer.  */
	if ( NULL == (local_arena =
		(VOID *)kalloc(sizeof(VOID *) + (2 * size)))) {
		return NULL;
	}

	
	boundry = (VOID *) c_boundry(local_arena);

	T_PIGGY(0x2000, printf("b: %x ", boundry); );

	/* First case:  enough space before the boundry.  */
	if ( (boundry - local_arena) >= (size + sizeof(VOID *)) ) {

		T_PIGGY(0x2000, printf("c1 "); );

		* (VOID **)local_arena = local_arena;
		retval = local_arena + sizeof(VOID *);
	} else if ((boundry - local_arena) < sizeof(VOID *)) {
		/*
		 * Second case: There is not enough space before the
		 * boundry for the whole pointer.
		 */
		T_PIGGY(0x2000, printf("c2 "); );

		* (VOID **)local_arena = local_arena;
		retval = local_arena + sizeof(VOID *);
	} else {

		T_PIGGY(0x2000, printf("c3: %x ", (boundry - local_arena)); );

		* (VOID **)(boundry - sizeof(VOID *)) = local_arena;
		retval = boundry;
	}

	T_PIGGY( 0x2000, {
		printf("palloc(%x) = %x:%x (was %x:%x), ",
			size, retval, (retval+size)-1,
			local_arena, (local_arena+(2*size)+sizeof(VOID *))-1);
	} );

	T_PIGGY( 0x2000, {
		if ((retval+size)-1 > (local_arena+(2*size)+sizeof(VOID *))-1) {
			printf("\npalloc() overrun\n");
		}
		if (retval < local_arena) {
			printf("\npalloc() underrun\n");
		}
	} );
	return((VOID *)retval);
} /* palloc() */

/*
 * void pfree(VOID *ptr);
 * Free the chunk of memory 'ptr' allocated by palloc().
 *
 * Note that 'ptr' is really a VOID *, but we call it VOID **
 * to simplify arithmetic.
 *
 * The address returned by kalloc() is stored immediately
 * before the chunk returned by palloc().
 */
void
pfree(ptr)
	VOID *ptr[];
{
	T_PIGGY(0x2000, { printf("pfree(%x):kfree(%x), ", ptr, *(ptr-1)); } );
	kfree(*(ptr-1));
} /* pfree() */


#ifdef TEST

#include <stdio.h>
#define FOURK	4096	/* How many bytes in 4K?  */
#define NUM_TESTS 40	/* How many tests do we run?  */
#define SMALL_NUMBER 6	/* A small number whose exact value we don't care about.  */
#define HUGE	(100*FOURK)	/* Allocate from this pool.  */
#define IGNORE(v)	(v==v)	/* Lint food.  */

unsigned t_piggy = 0x2000;	/* Turn on TRACER bits.  */

main()
{
	int i;
	VOID *chunk;

	for (i = 0; i < NUM_TESTS; ++i) {
		if (NULL == (chunk = palloc(SMALL_NUMBER))) {
			printf("No more fake memory to eat.\n");
			printf("This is probably a bug.\n");
			exit(1);
		}

		printf("chunk: %x\n", chunk);
	}
} /* main() for TEST */

/*
 * Print a message and die.
 */
panic(args)
{
	printf("%r\n", &args);
	exit(1);
}

/*
 * Fake kalloc() for use by palloc().
 * Allocate a chunk of some non-existant memory space.
 */
VOID *
kalloc(size)
	int size;
{
	static VOID *base = NULL;
	static VOID *top_free = NULL;
	VOID *retval;


	/*
	 * First time through, allocate a nice big chunk of memory
	 * to carve up.
	 */
	if (NULL == base) {
		if (NULL == (base = malloc(HUGE))) {
			printf("Can not malloc %d bytes.\n", HUGE);
			exit(1);
		}
		/* Make sure we start close to a click boundry.  */
		top_free = c_boundry(base) + SMALL_NUMBER;
	}

	retval = top_free;
	/*
	 * We want to encourage test addresses to migrate accross
	 * click boundries.
	 */
	if (size < (FOURK - 1)) {
		top_free += (FOURK - 1);
	} else {
		top_free += size;
	}

	return(retval);
} /* kalloc() */

/*
 * Fake kfree for pfree() to use.
 */
void
kfree(addr)
	VOID *addr;
{
	IGNORE(addr);
	/* Do nothing!  */
} /* kfree() */

#endif /* TEST */

#endif /* _I386 */
