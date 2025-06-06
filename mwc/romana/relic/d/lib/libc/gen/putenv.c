/*
 * Change or add value to environment.
 *
 * $Log:	/newbits/lib/libc/gen/RCS/putenv.c,v $
 * Revision 1.1	91/04/22  13:14:00 	bin
 * Initial revision
 * 
 * 87/02/05	Allan Cornish
 * Initial revision.
 */
#include <stdio.h>
#include <errno.h>

/**
 *
 * int
 * putenv( s )		- change or add value to environment
 * char * s;
 *
 *	Input:	s = pointer to string of the form 'NAME=value'
 *
 *	Action:	The function putenv makes the value of the environment
 *		variable 'name' equal to 'value' by altering an existing
 *		variable or creating a new one.  In either case, the string
 *		created by 'string' becomes part of the environment,
 *		so altering the string will change the environment.
 *		The space used by string is no longer used once a new
 *		string-defining name is passed to the function putenv.
 *
 *	Return:	0 = environment updated.
 *		* = insufficient memory, or invalid argument.
 *
 *	Notes:	The third argument to main [envp] is not changed.
 */

int
putenv( string )
char * string;
{
	register char **epp;
	register int	len;
	static char ** lastenv;
	extern char ** environ;

	/*
	 * Paranoia.
	 */
	if ( string == NULL ) {
		errno = EFAULT;
		return -1;
	}

	/*
	 * Validate string, which must be of form NAME=value.
	 */
	for ( len = 0; string[len] != '='; len++ ) {
		if ( string[len] == '\0' ) {
			errno = EINVAL;
			return -1;
		}
	}

	/*
	 * Update len to include the '='.
	 */
	len++;

	/*
	 * Search for existing value.
	 */
	for ( epp = environ; *epp != NULL; epp++ ) {

		/*
		 * Variable already in environment.
		 */
		if ( strncmp( string, *epp, len ) == 0 ) {

			/*
			 * Update environment.
			 * NOTE: should release previous value if malloc'ed.
			 */
			*epp = string;
			return 0;
		}
	}

	/*
	 * Allocate new environment array.
	 */
	len = (epp - environ + 2) * sizeof(*epp);
	if ( (epp = malloc(len)) == NULL ) {
		errno = ENOMEM;
		return -1;
	}

	/*
	 * Copy old environment to new environment.
	 */
	len = 0;
	while ( epp[len] = environ[len] )
		len++;

	/*
	 * Append new variable and NULL terminator.
	 */
	epp[len++] = string;
	epp[len++] = NULL;

	/*
	 * Release last malloc'ed environment array.
	 */
	if ( lastenv != NULL )
		free( lastenv );

	/*
	 * Install new environment.
	 */
	environ = epp;
	lastenv = epp;

	return 0;
}
