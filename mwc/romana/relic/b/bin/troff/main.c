/*
 * main.c
 * Nroff/Troff.
 * Main program and initialization.
 */

#include <ctype.h>
#if	MSDOS
#include <types.h>
#include <stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <time.h>
#include <path.h>
#include "roff.h"

extern	char	*getenv();
extern	char	*path();
extern	time_t	time();
#ifdef	GEMDOS
extern	char	*tempnam();
#else
extern	char	*mktemp();
#endif

#ifdef	GEMDOS
unsigned long _stksize = 0x8000L;
#endif

static	int	kflag;		/* keep tmp file for debug purposes	*/
static	char	template[sizeof(TMPLATE)+1] = TMPLATE;
static	char	*tempname;	/* temp file name			*/

main(argc, argv) int argc; char *argv[];
{
	register int i, fileflag, iflag;
	register char *libpath, *cp;
	register REG *rp;
	char c, name[2];

	argv0 = (ntroff == NROFF) ? "nroff" : "troff";
	cp = getenv((ntroff == NROFF) ? "NROFF" : "TROFF");
	if (cp != NULL && *cp != '\0')
		addargs(cp, &argc, &argv);
	initialize(argc, argv);

	/*
	 * Process specified input files.
	 * initialize() already handled most options.
	 */
	fileflag = iflag = 0;
	for (i = 1; i < argc; i++) {
		cp = argv[i];
		if (*cp != '-') {
			/* Process non-option argument. */
			fileflag = 1;
			if (adsfile(cp) != 0)
				process();
			continue;
		}

		/* Process '-'-option argument. */
		c = *++cp;			/* argv[i][1] */
		cp++;				/* &argv[i][2] */
		if (c == 'i')
			iflag = 1;		/* process stdin when done */
		else if (c == 'f')
			++i;			/* ignore tempfile arg */
		else if (c == 'm') {
			/* Process "-m" macro package argument. */
			sprintf(miscbuf, TMACFMT, cp);
			libpath = DEFLIBPATH;
			if ((libpath = path(libpath, miscbuf, R_OK)) != NULL)
				strcpy(miscbuf, libpath);
#if	(DDEBUG & DBGFILE)
			printd(DBGFILE, "tmac file = %s\n", miscbuf);
#endif
			adsfile(miscbuf);
			process();
		} else if (c == 'n') {
			/* Reset page number. */
			pno = atoi(cp);
			npn = pno + 1;
		} else if (c == 'r' && *cp != '\0') {
			/* Reset register value. */
			name[0] = *cp++;
			if (isdigit(*cp))
				name[1] = '\0';
			else
				name[1] = *cp++;
			rp = getnreg(name);
			rp->n_reg.r_nval = atoi(cp);
			if (rp == nrpnreg)		/* Page # register */
				npn = pno + 1;		/* Set next page # */
		}
	}
	if (fileflag == 0 || iflag != 0) {
		/* Process standard input. */
		adsunit(stdin);
		process();
	}
	if (iestackx != -1)
		printe(".ie without matching .el");
	leave(0);
}

/*
 * Open temp file, set up registers and general initialization.
 */
initialize(argc, argv) int argc; char *argv[];
{
	register REG *rp;
	register REQ *qp;
	register int i;
	int Dflag, tmparg;
	char *s;

	A_reg = ntroff==NROFF;

	/*
	 * Pass over args, process those dealing with global initialization.
	 * main() makes another pass over the arg list to process input files.
	 */
	Dflag = tmparg = 0;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			continue;
		switch (argv[i][1]) {
		case 'a':
			A_reg = 1;
			continue;
		case 'd':
#if	DDEBUG
			if (argv[i][2] != '\0') {
				dbglvl = atoi(&argv[i][2]);
				dbginit();
			} else
#endif
				dflag++;
			continue;
		case 'D':
			Dflag = 1;
			continue;
		case 'f':
			if (i < (argc-1))
				tmparg = ++i;
			else
				panic("-f option requires file argument");
			continue;
		case 'i':
			continue;	/* handled in main() */
		case 'K':
		case 'k':
			kflag++;
			continue;
		case 'l':
			lflag = 1;
			continue;
		case 'm':
			continue;	/* handled in main() */
		case 'n':
			continue;	/* handled in main() */
		case 'p':
			pflag = 1;
			continue;
		case 'r':
			continue;	/* handled in main() */
#ifndef GEMDOS
		case 'T':
			if (ntroff == NROFF)
				T_reg = 1;
			continue;
#endif
		case 'x':
			xflag++;
			continue;
		case 'V':
		case 'v':
			fprintf(stderr, "%s: V%s\n", argv0, VERSION);
			continue;
#if	ZKLUDGE
		case 'Z':
			if (argv[i][2] != '\0')
				Zflag = atoi(&argv[i][2]);
			else
				Zflag = ZPAGES;
			continue;
#endif
		default:
			fprintf(stderr, "%s: illegal option: %s\n", argv0, argv[i]);
			/* fall through... */
		case '?':
			usage();
		}
	}

	/* Initialize tempfile. */
#ifdef	GEMDOS
	tempname = (tmparg) ? argv[tmparg] : tempnam(0L, "nroff");
#else
	tempname = (tmparg) ? argv[tmparg] : mktemp(template);
#endif
	dprint2(DBGFILE, "temp file name = %s\n", tempname);
	if ((tmp=fopen(tempname, "wb")) == NULL)
		panic("cannot create temp file");
	else if (freopen(tempname, "rwb", tmp) == NULL)
		panic("cannot reopen temp file");
	tmpseek = ENVSIZE * sizeof (ENV);
	tmpseek = (tmpseek+DBFSIZE+DBFSIZE-1) & ~(DBFSIZE-1);

#ifdef	GEMDOS
	/*
	 * GEMDOS lseek does not produce sparse files;
	 * this makes it necessary to write the beginning of nroff's work file.
	 */
	memset(diskbuf, '\0', DBFSIZE);
	for (i = 0; i < tmpseek; i += DBFSIZE) {
		dprintd(DBGFILE, "initializing tempfile\n");
		if (write(fileno(tmp), diskbuf, DBFSIZE) != DBFSIZE)
			panic("temp file write error");
	}
#endif
#ifdef	COHERENT
	/*
	 * Unlinking temp file immediately under COHERENT makes it
	 * go away if program is interrupted with <Ctrl-C>;
	 * under GEMDOS it would destroy the file immediately,
	 * so it is done under leave() below.
	 */
	if (kflag == 0)
		unlink(tempname);
#endif

	/* Copy .pre-file if it exists. */
	if (!Dflag) {
		s = (lflag) ? PRE_L : PRE_P;
		if (lib_file(s, 0) == 0 & ntroff == TROFF)
			printe("file \"%s\" not found", s);
	}
	
	/* Initialize globals. */
	dev_init();		/* output writer-specific initialization */
	for (i = 0; i < NWIDTH; i++)
		trantab[i] = i;				/* translation table */
	for (i = 0; i < RHTSIZE; i++)
		regt[i] = NULL;				/* request hash table */
	for (qp = reqtab; qp->q_name[0]; qp++) {	/* built-in requests */
		rp = makereg(qp->q_name, RTEXT);
		rp->t_reg.r_macd.r_div.m_next = NULL;
		rp->t_reg.r_macd.r_div.m_type = MREQS;
		rp->t_reg.r_macd.r_div.m_func = qp->q_func;
	}

	/* Create built in registers. */
	/* UNDONE: "c.", same as ".c" */
	nrpnreg = getnreg("%");
	nrctreg = getnreg("ct");
	nrdlreg = getnreg("dl");
	nrdnreg = getnreg("dn");
	nrdwreg = getnreg("dw");
	nrdyreg = getnreg("dy");
	nrhpreg = getnreg("hp");
	nrlnreg = getnreg("ln");
	nrmoreg = getnreg("mo");
	nrnlreg = getnreg("nl");
	nrsbreg = getnreg("sb");
	nrstreg = getnreg("st");
	nryrreg = getnreg("yr");
	setnreg();

	/* Environment initialization. */
	envset();
	envinit[0] = 1;

	/* Etc. */
	iestackx = -1;
	cdivp = NULL;
	newdivn("\0\0");
	mdivp = cdivp;
#ifdef	GEMDOS
	if (((long)mdivp) & 1L)
		panic("diversion buffer odd alignment");
#endif
	endtrap[0] = '\0';
	strp = NULL;
	pgl = (lflag) ? unit(17*SMINCH, 2*SDINCH) : unit(11*SMINCH, SDINCH);
	pno = 1;
	npn = 2;
	esc = '\\';

	/* Load default fonts for troff, initialize font numbers. */
	i = lib_file("fonts.r", 1);
	if (ntroff == TROFF && i == 0)
		panic("fonts.r not found");
	if (Dflag) {
		font_display();
		exit(0);
	}
	if (setfont("R", 1) == -1)
		leave(1);			/* font R is mandatory */
	tfn = curfont;				/* tab character font */
	if ((ufn = font_num("I")) == -1)	/* underline font number */
		ufn = curfont;

	/* Process special character definitions. */
	lib_file("specials.r", 1);		/* special characters */
#if	(DDEBUG & DBGCHEK)
	printd(DBGFUNC, "initialized...\n");
#endif
}

/*
 * Initialize pre-defined number registers.
 */
setnreg()
{
	time_t curtime;
	register struct tm *tmp;

	curtime = time((time_t *)0);
	tmp = localtime(&curtime);
	nryrreg->n_reg.r_nval = tmp->tm_year % 100;
	nrmoreg->n_reg.r_nval = tmp->tm_mon + 1;
	nrdyreg->n_reg.r_nval = tmp->tm_mday;
	nrdwreg->n_reg.r_nval = tmp->tm_wday + 1;
}

/*
 * Leave.
 * The passed exit status is:
 *	0	normal
 *	1	fatal error
 *	2	usage error
 */
leave(status) register int status;
{
	char name[2];
	static int depth = 0;

	if (status == 0 && depth++ == 0) {
		if (endtrap[0] != '\0') {
			name[0] = endtrap[0];
			name[1] = endtrap[1];
			endtrap[0] = '\0';
			execute(name);
		}
		setbreak();
		if (xflag == 0) {
			byeflag = 1;
			pspace();
		}
	} else if (status == 1 && depth++ == 0) {
		/* Space to bottom of page if status==1, useful for PS. */
		if (xflag == 0) {
			byeflag = 1;
			pspace();
		}
	}

#ifndef COHERENT
#if	(DDEBUG & DBGFILE)
	{
		struct stat statblk;

		fclose(tmp);			/* Close the temp file.	*/
		stat(tempname, &statblk);	/* so we can stat it.	*/
		printd(DBGFILE, "deleting temporary file %s, size = %ld\n",
			tempname, statblk.st_size);
	}
#endif
	/* Unlink temp file if not COHERENT. */
	if (kflag == 0)
		unlink(tempname);
#endif
	exit(status);
}

/*
 * Print a fatal usage message and die.
 */
usage()
{
	fprintf(stderr, "Usage: %s [ option ... ] [ file ... ]\n",
		ntroff == NROFF ? "nroff" : "troff");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\t-d\tDebug: print each request before executing\n");
	if (ntroff == TROFF)
		fprintf(stderr, "\t-D\tDisplay available fonts\n");
	fprintf(stderr, "\t-f name\tWrite temporary file in file name\n");
	fprintf(stderr, "\t-i\tRead stdin after each file has been read\n");
	fprintf(stderr, "\t-k\tKeep temporary file\n");
	if (ntroff == TROFF)
		fprintf(stderr, "\t-l\tLandscape mode\n");
	fprintf(stderr, "\t-mname\tRead macro package /usr/lib/tmac.name\n");
	fprintf(stderr, "\t-nN\tNumber first page of output N (default, 1)\n");
	if (ntroff == TROFF)
		fprintf(stderr, "\t-p\tProduce PostScript output\n");
	fprintf(stderr, "\t-raN\tSet number register a to value N\n");
	fprintf(stderr, "\t-x\tDo not eject to bottom of final page\n");
	leave(2);
}

/*
 * cp contains space-separated environmental args to be added to argv.
 * Change argc/argv accordingly.
 */
addargs(cp, argcp, argvp) char *cp; int *argcp; char ***argvp;
{
	register int n;
	register char *s, **nargv, **np;

	for (s = cp, n = 1; *s != '\0'; s++)
		if (*s == ' ')
			++n;		/* number of added args */
	*argcp += n;			/* bump argc */
	np = nargv = (char **)nalloc((*argcp + 1) * sizeof (char *)); /* allocate */
	*np++ = *(*argvp)++;		/* copy old argv0 */
	for (s = cp; *s != '\0'; ) {
		*np++ = s;		/* store pointer to new arg */
		while (*s != '\0' && *s != ' ')
			s++;		/* scan to NUL or space */
		if (*s == ' ')
			*s++ = '\0';	/* NUL-terminate space-separated args */
	}
	while (**argvp != NULL)
		*np++ = *(*argvp)++;	/* copy old argv */
	*np = NULL;			/* NULL-terminate new argv */
	*argvp = nargv;			/* pass back new argv */
}

/* end of main.c */
