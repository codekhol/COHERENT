#include <stdio.h>
#include <ctype.h>
#include "tsort.h"


void
getcon(fp)
FILE *fp;
{
	register char *wrd;
	struct word *wordp1, *wordp2;
	struct wordlist *rel;
	char *getwrd();

	for (;;) {
		wrd = getwrd(fp);
		if (wrd == NULL)
			break;
		wordp1 = insert(wrd);
		wrd = getwrd(fp);
		if (wrd == NULL)
			die("odd number of fields input");
		wordp2 = insert(wrd);
		if (wordp1 != wordp2) {
			rel = newwordl(wordp1);
			rel->next = wordp2->ancestors;
			wordp2->ancestors = rel;
		}
	}
	words = cmphash();
}


/*
 *	Getwrd is used to get the next word from the FILE pointed to
 *	by "fp".  Word is defined as non-white-space.  If the end of
 *	file is reached, then getwrd returns an empty string.  Note
 *	that since getwrd returns a pointer to a static area, the result
 *	must be copyied before the next call.
 */

static char *
getwrd(fp)
register FILE *fp;
{
	register int ch;
	static char word[MAXWORD];
	register char *wrdp;

	while ((ch = getc(fp)) != EOF && isascii(ch) && isspace(ch))
		;
	if (ch == EOF)
		return (NULL);
	for (wrdp = word; !(isascii(ch) && isspace(ch)); ch = getc(fp)) {
		if (ch == EOF)
			break;
		if (wrdp >= &word[MAXWORD - 1])
			die("word too long");
		*wrdp++ = ch;
	}
	*wrdp = '\0';
	return (word);
}
