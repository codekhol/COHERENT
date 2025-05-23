#include "asm.h"

/*
 * Copy a block of source and code to
 * the listing file. If no listing file or
 * the current line is not to be listed
 * just return. Multiple code bytes get
 * put out on extra lines after the source
 * line.
 */
list()
{
	register char *wp;
	register nb;

	if (lflag==0 || lmode==NLIST)
		return;
	slew();
	while (ep < &eb[NERR])
		*ep++ = ' ';
	printf("%.10s", eb);
	if (lmode == SLIST) {
		printf("%31s %5d %s\n", "", line, ib);
		return;
	}
	printf(ADRFMT, laddr);
	if (lmode == ALIST) {
		printf("%24s %5d %s\n", "", line, ib);
		return;
	}
	wp = cb;
	nb = cp - cb;
	list1(wp, nb, 1);
	printf(" %5d %s\n", line, ib);
	while ((nb -= NBOL) > 0) {
		wp += NBOL;
		slew();
		printf("%17s", "");
		list1(wp, nb, 0);
		putchar('\n');
	}
}

list1(wp, nb, f)
register char *wp;
register nb;
{
	register d, i;

	if (nb > NBOL)
		nb = NBOL;
	for (i=0; i<nb; ++i) {
		d = (*wp++)&0377;
		if (lmode == BLIST)
			printf(BFMT, d);
		else {
#if LOHI
			d |= ((*wp++) << 8);
#else
			d = (d<<8) | ((*wp++)&0377);
#endif
			printf(WFMT, d);
			++i;
		}
	}
	if (f) {
		while (i < NBOL) {
			printf(SKIP);
			++i;
		}
	}
}

slew()
{
	if (lop++ >= NLPP) {
		if (page)
			putchar('\f');
		printf("Coherent assembler (%s), page %d\n", CPU, ++page);
		printf("%s\n\n", tb);
		lop = 4;
	}
}
