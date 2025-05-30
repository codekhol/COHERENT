/* $Header: /newbits/usr/lib/libcurses/RCS/getkey.c,v 1.2 91/09/30 13:05:46 bin Exp Locker: bin $
 *
 *	The  information  contained herein  is a trade secret  of INETCO
 *	Systems, and is confidential information.   It is provided under
 *	a license agreement,  and may be copied or disclosed  only under
 *	the terms of that agreement.   Any reproduction or disclosure of
 *	this  material  without  the express  written  authorization  of
 *	INETCO Systems or persuant to the license agreement is unlawful.
 *
 *	Copyright (c) 1989
 *	An unpublished work by INETCO Systems, Ltd.
 *	All rights reserved.
 */

/*
 * Keyboard Escape Sequence Mapping Routine.
 *
 *	int getkey();
 *
 *	FUTURE: Support escape sequences split across multiple reads.
 */

#include <stdio.h>
#include "curses.ext"

/*
 * Get character from terminal, recognizing escape sequences.
 */
int
getkey()
{
	register tkeyent_t * kp;
	register int len;
	tkeyent_t * sav_kp;
	int sav_len;
	uchar * str;
	static unsigned char buf[64];
	static int bufoff;
	static int buflen;

#if DEBUG > 0
	fprintf( outf, "getkey: bufoff=%d buflen=%d\n", bufoff, buflen );
#endif

	while ( 1 ) {

		while ( bufoff >= buflen ) {

			buflen = read( 0, buf, sizeof(buf) - 1 );
			bufoff = 0;

#if DEBUG > 0
			fprintf( outf, "getkey: %d bytes read\n", buflen );
			if ( buflen > 0 ) {
				fprintf( outf, "\t'" );
				for ( bufoff = 0; bufoff < buflen; bufoff++ )
					fprintf(outf, "%s",
						unctrl(buf[bufoff]) );
				fprintf( outf, "'\n" );
				bufoff = 0;
			}
#endif

			if ( buflen <= 0 )
				return (0);

			buf[buflen] = '\0';
		}

		/*
		 * Search for matching escape sequences.
		 * 'kp'    	is a pointer into the keymap array.
		 * 'kp->cpp'	is a pointer to the capability variable.
		 * '*(kp->cpp)'	is a pointer to the capability string.
		 * '**(kp->cpp)'is the first character in the capability string.
		 * Remember the longest capability string found.
		 */
		for ( sav_kp = NULL, kp = tkeymap; kp->id != 0; kp++ ) {

			/*
			 * Capability is not defined.
			 */
			if ( kp->cpp == NULL )
				continue;

			/*
			 * Capability is not supported on current terminal.
			 */
			if ( (str = *(kp->cpp)) == NULL )
				continue;

			/*
			 * Compute length of string.
			 */
			len = strlen(str);

			/*
			 * Ignore zero-length strings.
			 */
			if ( len == 0 )
				continue;

#if DEBUG > 0
			{
			uchar * cp;
			fprintf( outf, "getkey:0%03o = '", kp->id );
			for ( cp = str; *cp; cp++ )
				fprintf( outf, "%s", unctrl(*cp) );
			fprintf( outf, "'\n" );
			}
#endif
			/*
			 * Ignore strings which are longer than the input.
			 */
			if ( bufoff + len > buflen )
				continue;

			/*
			 * Match not found.
			 */
			if ( memcmp( str, &buf[bufoff], len ) != 0 )
				continue;

#if DEBUG > 0
			fprintf(outf, "getkey:0%03o match of len %d found!\n",
				kp->id, len );
#endif
			/*
			 * Match is longer than previous one - remember it.
			 */
			if ( (sav_kp == NULL) || (sav_len < len) ) {
				sav_kp  = kp;
				sav_len = len;
			}
#if DEBUG > 0
			else {
				fprintf(outf, "getkey:0%03o match discarded\n",
					kp->id );
			}
#endif
		}

		/*
		 * Match was found.
		 */
		if ( sav_kp != NULL ) {
#if DEBUG > 0
			fprintf(outf,"getkey: match token = 0%o\n",sav_kp->id);
#endif
			bufoff += sav_len;

			return (sav_kp->id);
		}

#if DEBUG > 0
		fprintf( outf, "getkey: char = %s\n", unctrl(buf[bufoff]) );
#endif
		/*
		 * Return next char.
		 */
		return (buf[ bufoff++ ]);
	}
}
