/*
 * Virtual memory system for Coherent.
 */
#include "misc.h"
#include <sys/stat.h>

#define FILES 20
#define VBLKB 9		/* bytes in virtual block as a power of 2 */
#define VBLK (1 << VBLKB) /* bytes in a virtual block */

struct mapper {
	unsigned dirty:1;
	unsigned what_in:15;
};
static unsigned long limit[20];
static unsigned long initp;
static unsigned topf;
static struct mapper *map;
static char *data;
static unsigned blocks;
static int tmp, ramsw;
static struct stat st;

/*
 * init virtual system.
 * Called with work file name, may be /dev/ram1, and the
 * approximate amount of storage to be used for buffers.
 * The more you give it the better it works.
 */
void
vinit(fname, ram)
char *fname;	/* file name of virtual device e.g. /dev/rram1 */
unsigned ram;	/* amount of storage to devote to ram */
{
	if (!(blocks = ram / (VBLK + sizeof(*map))))
		fatal("Virtual; workspace too small");
	data = alloc(blocks << VBLKB);
	map  = (struct mapper *)alloc(blocks * sizeof(*map));
	ramsw = topf = initp = 0;	/* make code reusable */
	
	if ((0 == stat(fname, &st)) && (S_IFCHR == (st.st_mode & S_IFCHR))) {
		switch (is_fs(fname)) {
		case -1:
			fatal("Virtual; cannot seek %s", fname);
		case 1:
			fatal("Virtual; %s has a file system", fname);
		case 0:
			if (!strcmp(fname, "/dev/ram1"))
				ramsw = 1;
			if (-1 == (tmp = open(fname, 2)))
				fatal("Virtual; cannot open(%s, 2)", fname);
		}
		return;
	}

	if (-1 == (tmp = creat(fname, 0600)))
		fatal("Virtual; cannot creat(%s, 0600)", fname);
	close(tmp);
	if (-1 == (tmp = open(fname, 2)))
		fatal("Virtual; cannot open(%s, 2)", fname);
	unlink(fname);
	fstat(tmp, &st);
}

/*
 * Close down virtual system and free it's space
 */
void
vshutDown()
{
	close(tmp);
	if (ramsw)
		close(open("/dev/ram1close", 0));
	free(map);
	free(data);
}

/*
 * Set up a virtual file. Say you want to emulate having a
 * 100000 byte array and a 50000 byte array with virtual.
 * vid1 = vopen(100000L); vid2 = vopen(50000L);
 */
unsigned
vopen(vamt)
unsigned long vamt;
{
	if (topf == FILES)
		fatal("Too many vopen calls");
	vamt = limit[topf + 1] = limit[topf] + vamt;
	topf++;
	vamt >>= VBLKB;
	vamt /= blocks;		/* must fit in 15 bits */
	if (vamt > 0x7fff)
		fatal("Virtual; too much virtual space for buffers");

	if (st.st_mode & S_IFCHR) {	/* character special file */
		char work[VBLK];

		memset(work, 0, VBLK);
		for (; initp < limit[topf]; initp += VBLK)
			if (VBLK != write(tmp, work, VBLK))
				fatal("Virtual; error writing to tmp file");
	}
	return (topf - 1);
}

/*
 * Find a character on the virtual system
 * mark the blocks dirty bit if Find is to write.
 * Given the example in the vopen() comment. If you want to
 * find the 1000 th byte in vid1 
 * c = vfind(vid1, 1000L, 0);
 * To change the 2000 th byte in vid2
 * *(vfind(vid2, 2000L, 1)) = d;
 */
char *
vfind(vid, disp, dirty)
unsigned vid;		/* virtual file no */
unsigned long disp;	/* where */
int dirty;		/* 1 if character gotten to write */
{
	unsigned  which, what_in, byte_no;
	unsigned long diskad;
	char *where;

	if (vid >= topf)
		fatal("Virtual; invalid vfile");
	if ((disp += limit[vid]) >= limit[vid + 1])
		fatal("Virtual; request out of limits");
		
	byte_no = disp & (VBLK - 1);
	disp >>= VBLKB;
	which = disp % blocks;
	what_in = disp / blocks;
	where = data + (which << VBLKB);

	if ((diskad = map[which].what_in) != what_in)	{
		if (map[which].dirty) {
			diskad *= blocks;
			diskad += which;
			diskad <<= VBLKB;
			if (-1 == lseek(tmp, diskad, 0))
				fatal("Virtual; error in seek");
			if (VBLK != write(tmp, where, VBLK))
				fatal("Virtual; error in write");
		}
		map[which].dirty = 0;	/* clean */
		map[which].what_in = diskad = what_in;
		diskad *= blocks;
		diskad += which;
		diskad <<= VBLKB;
		if (-1 == lseek(tmp, diskad, 0))
			fatal("Virtual; error in seek");
		if (VBLK != read(tmp, where, VBLK))
			if (st.st_mode & S_IFCHR) /* char special device */
				fatal("Virtual; error in read");
			else
				memset(where, 0, VBLK);
	}
	map[which].dirty |= dirty; /* maybe dirty */
	return (where + byte_no);
}

#ifdef TEST
static int vid[FILES];
static long lim[FILES];
static char buf[80];

main()
{
	char c, d;
	int i, f;
	long atol();
	long l, t, tests;

	vinit("/dev/ram1", atoi(ask(buf, "How much ram to reserve")));
	f = atol(ask(buf, "How many virtual files?"));
	for (i = 0; i < f; i++)
	   vid[i] = vopen(lim[i] = atol(ask(buf, "How big is file %d", i)));
	tests = atol(ask(buf, "How many tests?"));
	srandl(100L, 100L);
	for (t = 0; t < tests; t++) {	/* set random locs */
		i = randl() % f; /* pick virtual file */
		l = randl() % lim[i]; /* pick location */
		c = i ^ l;
		*(vfind(vid[i], l, 1)) = c;
	}
	srandl(100L, 100L);
	for (t = 0; t < tests; t++) {	/* test same random locs */
		i = randl() % f; /* pick virtual file */
		l = randl() % lim[i]; /* pick location */
		c = i ^ l;
		if ((d = *(vfind(vid[i], l, 0))) != c)
			fatal("Expected %c got %c", c, d);
	}
	vshutDown();
}
#endif
