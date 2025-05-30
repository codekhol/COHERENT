/*
 * C character handling library.
 * isascii()
 * Not in ANSI 4.3 but allowed by 4.13.2.
 * Test if character is ASCII.
 */

#include <ctype.h>
#undef	isascii

int isascii(c) int c;
{
	return ((c&0x80)==0);
}
