/*
 * malloc.c
 * Memory allocation routines.
 * The memory arena is a circular linked list rooted at __a_first.
 * Each arena is subdivided into two or more blocks.
 * The first word of each block gives its length,
 * with the low order bit set if the block is free;
 * block lengths are always multiples of 2 to allow free marking.
 * A length word containing 0 marks the end of an arena;
 * in this case the following pointer points to the next arena.
 */

#include <stdio.h>
#include <sys/malloc.h>

MBLOCK	*__a_scanp = NULL;		/* start search here */
MBLOCK	*__a_first = NULL;		/* first arena */
unsigned __a_count = 0;			/* number of blocks */

/*
 * Get a new arena from sbrk() and hook it to the old list.
 * Return 1 if there is any chance of malloc succeeding, else 0.
 * Note that this assumes the argument to sbrk() is unsigned;
 * bad news if sbrk() expects int and shrinks the break if negative.
 */
static
int
newarena(size) unsigned size;
{
	register MBLOCK *mp, *pmp, *linkmp;
	register unsigned len;
	static MBLOCK *__a_top = NULL;
	static char failed = 0;

	if (failed)			/* no more room */
		return 0;

	/* Add space for end mblock, round up to 2^ARENASIZE */
	len = roundup(size + sizeof(MBLOCK), ARENASIZE);
	if (len < size)
		len = size;

	__a_scanp = __a_first;		/* rescan from the begining */

	/*
	 * If there isn't enough space get what we can it may be enough.
	 * This means further calls to newarena must fail.
	 */
	while ((mp = (MBLOCK *)sbrk(len)) == BADSBRK) {
		failed = 1;
		if (len <= DECRSIZE)
			return 1;		/* even zero may be ok */
		len -= DECRSIZE;
		if (sizeof(MBLOCK) > len)
			len = sizeof(MBLOCK);
	}

	if (__a_top == NULL) {			/* first time through */
		__a_count = 2;
		__a_first = __a_scanp = linkmp = mp;
	}
	else if (__a_top == mp) {		/* new arena follows old */
		/*
		 * The following assumes that len + sizeof(MBLOCK)
		 * will not be greater than the maximum unsigned value,
		 * which will be true if 2^ARENASIZE > sizeof(MBLOCK).
		 */
		--mp;
		len += sizeof(MBLOCK);
		linkmp = mp->uval.next;
		__a_count++;
	}
	else {					/* discontigous arenas */
		pmp = __a_top - 1;
		linkmp = pmp->uval.next;	/* save old pointer */
		pmp->uval.next = mp;		/* old points to new */
		__a_count += 2;
	}
	mp->blksize = (len - sizeof(MBLOCK)) | FREE;
	__a_top = bumpp(mp, len);
	pmp = __a_top - 1;
	pmp->blksize = 0;
	pmp->uval.next = linkmp;
	return 1;
}

/*
 * Allocate memory.
 * Successive free blocks are consolidated when found.
 */
char *
malloc(size) unsigned size;
{
    register MBLOCK *mp, *prevmp;
    register unsigned len, needed, counter;
    static char msg[] = "Bad pointer in malloc.\r\n";

    if (size == 0)
	    return NULL;
    needed = roundup(size + sizeof(unsigned), BLOCKSIZE);
    if (needed < size)
	    return NULL;

    do { /* until we find enough or newarena fails */
	prevmp = NULL;
	mp = __a_scanp;
	for(counter = __a_count; counter--; ) {
		if (!isfree(len = mp->blksize))	/* used block or pointer */
			prevmp = NULL;
		else {
			if (prevmp != NULL) {		/* consolidate free */
#if	0
/*
 * The following assumes adjacent free blocks can be consolidated without
 * overflow of the size.  The overflow test is conditionalized out here,
 * but it may be required on some machines.  In i8086 LARGE model, the code
 * works without the test but only barely.  When the memory arena gets larger
 * than 64K, sbrk()  returns mp pointing to the same memory location as
 * newarena()/__a_top, but with a different segment:offset representation;
 * thus the "if (__a_top == mp)" test in newarena fails (even though they
 * point to the same memory location) and newarena() leaves a 0 end marker
 * between the arenas.
 */
				if (prevmp->blksize + realsize(len) > len) {
					mp = prevmp;
					len = (mp->blksize += realsize(len));
					__a_count--;
				}
#else
				mp = prevmp;
				len = (mp->blksize += realsize(len));
				__a_count--;
#endif
			}
			if (len < needed)
				prevmp = mp;
			else {		/* got one big enough */
				if ((len -= needed) < LEASTFREE) {
					/* grab the entire block */
					mp->blksize=needed=realsize(mp->blksize);
					__a_scanp = bumpp(mp, needed);
				} else {
					/* split into used and free portions */
					mp->blksize = needed;
					__a_scanp = bumpp(mp, needed);
					__a_scanp->blksize = len;
					__a_count++;
				}
				return mp->uval.usera;
			}
		}
		mp = (len) ? bumpp(mp, realsize(len)) : mp->uval.next;
	}

	/*
	 * There should have been __a_count blocks bringing us full circle.
	 */
	if (mp != __a_scanp) {
		write(2, msg, sizeof(msg) - 1);
		abort();
	}

    /* Not enough room in the current arena, allocate a new one. */
    } while (newarena(needed));
    return NULL;
}

/*
 * Free a block.
 * Some sanity checking.
 * Adjacent free block consolidation happens in malloc(), not here.
 */
void
free(cp) char *cp;
{
	register MBLOCK *mp;
	register unsigned len;

	if (NULL == cp)		/* ansi 4.10.3.2: free(NULL) has no effect */
		return;

	mp = mblockp(cp);
	len = mp->blksize;
	if (len < 2) {		/* length of 0 or 1 is wrong */
		static char msg[] = "Bad pointer in free.\r\n";

		write(2, msg, sizeof(msg) - 1);
		abort();
	}
	mp->blksize |= FREE;			/* mark free */

	/*
	 * If freed block precedes scan pointer or scan pointer is not free,
	 * reset the scan pointer.
	 */
	if (bumpp(mp, realsize(len)) == __a_scanp
	 || !isfree(__a_scanp->blksize))
		__a_scanp = mp;
}
