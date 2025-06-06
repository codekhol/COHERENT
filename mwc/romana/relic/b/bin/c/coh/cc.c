/*
 * coh/cc.c
 * CC command.
 * Compile, assemble and link edit C programs.
 * A lot of grunge.
 * Revised by rec 5/87 to incorporate all
 * COHERENT and GEMDOS revisions to date.
 * Revised by steve 3/92 to produce monolithic COHERENT compiler.
 *
 * Compliing with -DVeryVflag produces very verbose output under -V option.
 *
 * C compiler/loader switch map.
 *	*7	marks a version seven documented option.
 *	*7d	marks a defunct version seven option.
 *	*7c	marks a changed version seven option.
 *	*u	marks an option unrecognized by cc, i.e. passed to loader
 *		with no interpretation or processing.
 *
 * Options still up for grabs:
 * [  C  FGH J     P R    W Y ]
 * [ b     h j  m           yz]
 * Change the verbose usage() message below when options change!
 *
 *	?		list available options, cf. usage(1) below
 *	A		auto edit mode
 *7c	Bstring		use string to find compiler passes
 *7	Dname[=value]	preprocessor: #define
 *7	E		run preprocessor to stdout
 *7	Ipathname	preprocessor: #include search directory
 *	K		keep intermediate files
 *	Lpathname	loader: library directory specification
 *	Mstring		use string as cross-compiler prefix
 *	N[01ab2sdlrt]string	rename pass with string
 *7	O		run object code optimiser
 *7d	P		put preprocessor output into name.i; use -Kqp
 *	Q		be quiet, make no messages
 *7	S		make assembly language output
 *	T[value]	use in-memory tempfiles of size value (default: 64K)
 *7	Uname		preprocessor: #undef
 *	V		be verbose, report everything
 *	Vvariant	enable variant
 *	X		loader: remove C-generated local symbols
 *	Z		(GEMDOS) floppy change prompts for phases
 *	a		do not implicit output file name to loader
 *7	c		compile but do not load
 *	d		loader: define common space
 *7	e name		loader: entry point specification
 *7c	f		load floating point output conversion routines
 *	g		generate debug info, same as -VDB
 *7u	i		loader: separate i and d spaces
 *	k system	loader: bind as kernel process [but ld doesn't grok it]
 *7c	lname		loader: library specification
 *7u	n		loader: shared instruction space
 *7	o name		loader: output file name
 *7	p		generate code to profile function calls
 *	q[p012s]	quit after specified pass
 *	r		loader: retain relocation in output
 *	s		loader: strip symbol table
 *7c	t[p012adlrt]	take specified passes from -Bdirectory
 *7	u name		loader: undefine name
 *	v		verbose, same as V
 *	w		loader: watch
 *	x		loader: remove local symbols from symbol table
 */

#if	GEMDOS
#ifndef VERS
#define VERS	"2.1"
#endif
#endif

#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <path.h>
#include <errno.h>
#include "mch.h"
#include "host.h"
#include "ops.h"
#include "stream.h"
#undef NONE
#include "var.h"
#include "varmch.h"

#ifndef VeryVflag
#define VeryVflag 0
#endif

#ifndef PREFIX
#define PREFIX	""
#endif

#if	_I386
#define	DTEFG	"_dtefg"
#else
#define	DTEFG	"_dtefg_"
#endif

/*
** Pass information.
*/
#define NONE	-1
#define CPP	0	/* Pass index numbers */
#define CC0	1
#define CC1	2
#define CC2	3
#define CC3	4
#define CC4	5	/* Output writer postprocessor */
#define AS	6
#define LD	7
#define LD2	8	/* Loader postprocessor */
#define ED	9
#define LIB	10
#define CRT	11
#define TMP	12
#define ALL	13
#define	CC2B	14	/* for monolithic compiler */

#define P_TAKE	1	/* Take pass from backup directory */
#define P_BACK	2	/* Backup directory specified */
#define P_LIB	4	/* Take pass from LIBPATH */
#define P_BIN	8	/* Take pass from BIN */

#define PTMP	32	/* Pass name buffer size */

char	dnul[] = "";		/* Global null string */
char	dmch[PTMP] = PREFIX;	/* Cross compiler prefix */

struct pass {
	char p_flag;		/* Flags */
	char p_psn;		/* Pass short name */
	char p_pln[PTMP];	/* Pass long name */
	char p_pfs[4];		/* Pass output file suffix */
	char *p_ifn;		/* Input file name */
	char *p_ofn;		/* Output file name */
	char *p_sfn;		/* Scratch file name */
	char *p_dir;		/* Path lookup or backup string */
	char *p_mch;		/* Machine prefix name */
} pass[] = {
	{ P_LIB, 'p', "cpp",  "i",	NULL, NULL, NULL, NULL,	dmch },
	{ P_LIB, '0', "cc0",  "0",	NULL, NULL, NULL, NULL,	dmch },
	{ P_LIB, '1', "cc1",  "1",	NULL, NULL, NULL, NULL,	dmch },
	{ P_LIB, '2', "cc2",  "o",	NULL, NULL, NULL, NULL,	dmch },
	{ P_LIB, '3', "cc3",  "s",	NULL, NULL, NULL, NULL,	dmch },
	{ P_LIB, '4', "cc4",  "o",	NULL, NULL, NULL, NULL,	dmch },
	{ P_BIN, 's', "as",   "o",	NULL, NULL, NULL, NULL,	dmch },
	{ P_BIN, 'd', "ld",   "",	NULL, NULL, NULL, NULL,	dnul },
	{ P_BIN, 'x', "ld2",  "",	NULL, NULL, NULL, NULL,	dnul },
	{ P_BIN, 'e', "me",   "",	NULL, NULL, NULL, NULL,	dnul },
	{ P_LIB, 'l', "lib",  "a",	NULL, NULL, NULL, NULL,	dmch },
	{ P_LIB, 'r', "crts0.o", "",	NULL, NULL, NULL, NULL,	dmch },
	{     0, 't', "cc",    "",	NULL, NULL, NULL, NULL,	dnul }
};

/*
** Option and argument information.
*/
#define CCOPT	0x001		/* Argument flags in argf[] */
#define PPOPT	0x002		/* Also in option table, at least */
#define LDOPT	0x004		/* those that fit in a byte */
#define LD2OPT	0x008
#define CCLIB	0x010
#define LDLIB	0x020
#define CCARG	0x040
#define ASARG	0x080
#define MARG	0x100
#define LDARG	0x200

/* Flag bits for ccvariant. */
#define FLAG_c	0x001		/* -c flag */
#define FLAG_O	0x002		/* -O flag */
#define FLAG_f	0x004		/* -f flag */
#define FLAG_K	0x008		/* -K or -VKEEP flag */
#define VS	0x010		/* Turn on strict messages */
#define VDB	0x020		/* Turn on debugging */
#define FLAG_A	0x040		/* Auto edit mode */
#define FLAG_Z	0x080		/* Floppy change prompts */
#define FLAG_V	0x100		/* Verbose flag */
#if	GEMDOS
#define FLAG_GEMAPP	0x200	/* Gem application compile */
#define FLAG_GEMACC	0x400	/* Gem accessory compile */
#endif
#define	FLAG_a	0x800		/* Suppress output file name to ld */

struct option {			/* option table */
	char o_kind;
	char o_flag;
	char *o_name;
	int o_bits;
} option[] = {
/* Strict */
	{ 0,	CCOPT,	"VSUREG",	VSUREG	},
	{ 0,	CCOPT,	"VSUVAR",	VSUVAR	},
	{ 0,	CCOPT,	"VSNREG",	VSNREG	},
	{ 0,	CCOPT,	"VSRTVC",	VSRTVC	},
	{ 0,	CCOPT,	"VSMEMB",	VSMEMB	},
	{ 0,	CCOPT,	"VSBOOK",	VSBOOK	},
	{ 0,	CCOPT,	"VSLCON",	VSLCON	},
	{ 0,	CCOPT,	"VSPVAL",	VSPVAL	},
	{ 0,	CCOPT,	"VSCCON",	VSCCON	},
/*
 * Debug.
 * The -VDB option turns on all of these;
 * no earlier option may start with "VD",
 * and the next non-debug option must not start with "VD".
 */
	{ 0,	CCOPT,	"VDEBUG",	VDEBUG  },	
	{ 0,	CCOPT,	"VDLINE",	VLINES	},
	{ 0,	CCOPT,	"VDTYPE",	VTYPES	},
	{ 0,	CCOPT,	"VDSYMB",	VDSYMB	},

/* Miscellaneous */
	{ 0,	CCOPT,	"VSTAT",	VSTAT	},
	{ 0,	CCOPT,	"VWIDEN",	VWIDEN	},
	{ 0,	CCOPT,	"VPEEP",	VPEEP	},
	{ 0,	CCOPT,	"VCOMM",	VCOMM	},
	{ 0,	CCOPT,	"VQUIET",	VQUIET	},
	{ 0,	CCOPT,	"VPSTR",	VPSTR	},
	{ 0,	CCOPT,	"VROM",		VROM	},
	{ 0,	CCOPT,	"VASM",		VASM	},

	{ 0,	CCOPT,	"VLIB",		VLIB	},
	{ 0,	CCOPT,	"VNOWARN",	VNOWARN	},
	{ 0,	CCOPT,	"VPROF",	VPROF	},
	{ 0,	CCOPT,	"VALIEN",	VALIEN	},
	{ 0,	CCOPT,	"VREADONLY",	VREADONLY },
	{ 0,	CCOPT,	"VSINU",	VSINU	},
	{ 0,	CCOPT,	"VNOOPT",	VNOOPT	},
	{ 0,	CCOPT,	"VCPLUS",	VCPLUS	},
	{ 0,	CCOPT,	"VCPPE",	VCPPE	},
	{ 0,	CCOPT,	"VCPPC",	VCPPC	},
	{ 0,	CCOPT,	"VCPP",		VCPP	},
	{ 0,	CCOPT,	"VTPROF",	VTPROF	},
	{ 0,	CCOPT,	"V3GRAPH",	V3GRAPH	},
#if 1
/* Intel flags */
	{ 0,	CCOPT,	"VSMALL",	VSMALL	},
	{ 0,	CCOPT,	"VLARGE",	VLARGE	},
	{ 0,	CCOPT,	"V8087",	V8087	},
	{ 0,	CCOPT,	"VNDP",		VNDP	},
	{ 0,	CCOPT,	"VRAM",		VRAM	},
	{ 0,	CCOPT,	"VOMF",		VOMF	},
	{ 0,	CCOPT,	"V80186",	V80186	},
	{ 0,	CCOPT,	"V80287",	V80287	},
	{ 0,	CCOPT,	"VALIGN",	VALIGN	},
	{ 0,	CCOPT,	"VEMU87",	VEMU87	},
	{ 0,	CCOPT,	"VXSTAT",	VXSTAT	},
#endif
#if	GEMDOS
/* Motorola and Gemdos flags */
	{ 12,	CCOPT,	"VGEMACC",	FLAG_GEMACC	},
	{ 12,	CCOPT,	"VGEMAPP",	FLAG_GEMAPP	},
	{ 12,	CCOPT,	"VGEM",		FLAG_GEMAPP	},
	{ 10,	CCOPT,	"VSPLIM",	VSPLIM	},
	{ 10,	CCOPT,	"VNOTRAPS",	VNOTRAPS	},
#endif
/* More miscellaneous flags */
	{ 2,	CCOPT,	"VDB",		VDB	},
	{ 2,	CCOPT,	"g",		VDB	},
	{ 2,	CCOPT,	"VS",		VS	},
	{ 12,	CCOPT,	"A",		FLAG_A	},
	{ 12,	CCOPT,	"a",		FLAG_a	},
	{ 12,	CCOPT,	"Z",		FLAG_Z	},
	{ 12,	CCOPT,	"c",		FLAG_c	},
	{ 10,	CCOPT,	"S",		VASM	},
	{ 2,	CCOPT,	"O",		FLAG_O	},
	{ 0,	CCOPT,	"E",		VCPPE	},
	{ 12,	CCOPT,	"f",		FLAG_f	},
	{ 12,	CCOPT,	"VFLOAT",	FLAG_f	},
	{ 0,	CCOPT,	"p",		VPROF	},
	{ 12,	CCOPT,	"VKEEP",	FLAG_K	},
	{ 12,	CCOPT,	"K",		FLAG_K	},
	{ 0,	CCOPT,	"Q",		VQUIET	},
	{ 11,	CCOPT,	"VVERSION"		},
	{ 12,	CCOPT,	"V",		FLAG_V	},	/* all -V* must precede this */
	{ 12,	CCOPT,	"v",		FLAG_V	},
/* Preprocessor options */
	{ 3,	PPOPT,	"D"	},
	{ 3,	PPOPT,	"I"	},
	{ 3,	PPOPT,	"U"	},
/* Loader options */
	{ 4,	LDOPT,	"d"	},
	{ 4,	LDOPT,	"i"	},
	{ 4,	LDOPT,	"n"	},
	{ 5,	LDOPT,	"o"	},
	{ 4,	LDOPT,	"r"	},
	{ 4,	LDOPT,	"s"	},
	{ 4,	LDOPT,	"w"	},
	{ 4,	LDOPT,	"x"	},
	{ 4,	LDOPT,	"X"	},
#if	1
	{ 8,	LDOPT,	"L"	},
#endif
	{ 5,	LDOPT,	"e"	},
	{ 5,	LDOPT,	"k"	},
	{ 5,	LDOPT,	"u"	},
/* C dispatcher options */
	{ 6,	CCOPT,	"q"	},
	{ 7,	CCOPT,	"t"	},
	{ 7,	CCOPT,	"B"	},
	{ 7,	CCOPT,	"M"	},
	{ 7,	CCOPT,	"N"	},
#if	TEMPBUF
	{ 9,	CCOPT,	"T"	},
#endif
	{ 8,	CCLIB,	"l"	},
	{ 13,	CCOPT,	"?"	},
	{ -1,	-1,	""	}
};

int	ccvariant = 0;			/* Variant template */
VARIANT	variant;			/* Binary variant template */
char	vstr[2*VMAXIM/VGRANU + 1];	/* Ascii variant string */
char	vstr2[2*VMAXIM/VGRANU + 1];	/* Ascii variant string for .m */

/*
** Compiler parameters and flags
*/
#define	NCMDA	128		/* Size of argument pointer buffer */
#define	NCMDB	1024		/* Size of argument buffer */
#define	NTMP	L_tmpnam	/* Temp file buffer size, from stdio.h */
#define	MAGIC	127		/* Exit status mask */
#define	NSLEEP	10		/* # of sleeps */
#define	DELAY	5		/* Sleep delay, seconds */

int	nldob;			/* Number of objects created */
int	nfile;			/* Number of files to compile or assemble */
int	nname;			/* Number of file name arguments */
int	ndota;
int	ndotc;
int	ndotm;
int 	ndoto;
int	ndots;
int	partial;			/* Partial link specified */

#define	aflag	((ccvariant&FLAG_a)!=0)
#define	cflag	((ccvariant&FLAG_c)!=0)
#define pflag	isvariant(VPROF)
#define	fflag	((ccvariant&FLAG_f)!=0)
#define	Kflag	((ccvariant&FLAG_K)!=0)
#define	Oflag	((ccvariant&FLAG_O)!=0)
#define Sflag	isvariant(VASM)
#define	Eflag	isvariant(VCPPE)
#define Vflag	((ccvariant&FLAG_V)!=0)
#define	Qflag	isvariant(VQUIET)
#define	Aflag	((ccvariant&FLAG_A)!=0)
#define	Zflag	((ccvariant&FLAG_Z)!=0)
#if	GEMDOS
#define GEMAPPflag	((ccvariant&FLAG_GEMAPP)!=0)
#define GEMACCflag	((ccvariant&FLAG_GEMACC)!=0)
#endif

int	qpass = NONE;		/* Quit after this pass */
int	nload;			/* No load phase */
char	*outf;			/* Output file */
char	*doutf;			/* Default output file stem */
char	*dpath = DEFPATH;
char	*dlibpath = DEFLIBPATH;
int	xstat;			/* Exit status */

char	*cmda[NCMDA];		/* Argument pointer buffer */
char	cmdb[NCMDB];		/* Argument buffer */
char	newo[NTMP];		/* New object to be removed */
char	tmp[7][NTMP];		/* Intermediate filenames */
char	istmp[7];		/* Truth about their temporariness */
int	argf[NCMDA];		/* Argument flags */
char	optb[NCMDB];		/* Option rewrite buffer */
char	*optp = &optb[0];	/* Option rewrite position */

#if	TEMPBUF
unsigned char	*inbuf;			/* memory input buffer	*/
unsigned char	*inbufp;		/* input buffer pointer	*/
unsigned char	*inbufmax;		/* input buffer end	*/
unsigned char	*outbuf;		/* memory output buffer	*/
unsigned char	*outbufp;		/* output buffer pointer */
unsigned char	*outbufmax;		/* output buffer end	*/
unsigned int	tempsize = 65536;	/* memory buffer size	*/
#endif

#if	MONOLITHIC
int		monolithic = 1;		/* set to 0 if phases needed	*/

/* Information passed to phases through globals in monolithic compiler */
jmp_buf		death;			/* for fatal error handling	*/
int		dotseg;			/* current segment		*/
char		file[NFNAME];		/* current input file name	*/
char		basefile[NFNAME];	/* original input file name	*/
char		id[NCSYMB];		/* global identifier buffer	*/
FILE		*ifp;			/* input FILE			*/
int		line;			/* line number			*/
int		mflag;			/* debug flag			*/
char		module[NMNAME];		/* module name buffer		*/
int		oflag;			/* debug flag			*/
FILE		*ofp;			/* output FILE			*/
int		sflag;			/* debug flag			*/
#endif

/* Forward. */
FILE	*ccopen();
char	*makepass();
char	*makelib();
int	cleanup();

/* External. */
char	*getenv();
char	*malloc();
char	*tempnam();
char	*path();

main(argc, argv) int argc; char *argv[];
{
	register char *p;
	register struct option *op;
	int i, narg, c, n;

#if COHERENT
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, cleanup);
#endif
#if	_I386
	/* Conditionalized only because _addargs() not in COH286 libc.a yet. */
	_addargs("CC", &argc, &argv);
#endif
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	setvariant(VSUVAR);
	setvariant(VSMEMB);
	setvariant(VSLCON);
	setvariant(VSPVAL);
	setvariant(VPEEP);
	setvariant(VCOMM);
	setvariant(VPSTR);
	setvariant(V80186);

#if	0
	printmem("initially");
#endif
	if (p = getenv("PATH")) dpath = p;
	if (p = getenv("LIBPATH")) dlibpath = p;
	if (p = getenv("EDITOR")) strcpy(pass[ED].p_pln, p);

	for (i=1; i<argc; i+=narg) {
		narg = 1;
		p = argv[i];
		if (*p++ == '-') {
			/*
			 * Process an option argument.
			 * This is particularly squirrly because it allows
			 * several options to be combined in a single arg.
			 */
			char *ppopt = "";
			static char tldopt[64] = "-";
			char *tlp = &tldopt[1];

			if (*p == '\0') {
		badopt:
				whatopt(argv[i]);
				continue;
			}
			while (*p != '\0') {
				for (op = &option[0]; ; op += 1)
					if (op->o_name[0] == '\0')
						goto badopt;
					else if (opeq(op->o_name, p))
						break;
				if (VeryVflag && Vflag)
					printf("Option: -%s\n", op->o_name);
				argf[i] |= op->o_flag;
				switch (op->o_kind) {
				case 0:
					/* Toggle variant. */
					chgvariant(op->o_bits);
					p += strlen(op->o_name);
					continue;
				case 10:
					/* Set variant. */
					setvariant(op->o_bits);
					p += strlen(op->o_name);
					continue;
				case 2:
					/* Toggle ccvariant. */
					ccvariant ^= op->o_bits;
					p += strlen(op->o_name);
					/* Set strict */
					if ((ccvariant&VS) != 0) {
						for (op = &option[0];; op += 1)
							if (op->o_name[1]=='S')
								break;
						while (op->o_name[1]=='S') {
							setvariant(op->o_bits);
							op += 1;
						}
					}
					/* Set debug */
					if ((ccvariant&VDB) != 0) {
						for (op = &option[0];; op += 1)
							if (op->o_name[1]=='D')
								break;
						while (op->o_name[1]=='D') {
							setvariant(op->o_bits);
							op += 1;
						}
					}
					continue;
				case 12:
					/* Set ccvariant. */
					ccvariant |= op->o_bits;
					p += strlen(op->o_name);
					continue;
				case 3:
					/* Preprocessor options. */
					ppopt = p;
					p += strlen(p);
					continue;
				case 5:
					/* ld options with file args */
					if (*p == 'o') {
						outf = argv[i+narg];
					}
					if (*p == 'u')
						ndoto += 1;
					if (i+narg >= argc)
						missingname();
					argf[i+narg++] = LDOPT;
					/* Fall through */
				case 4:
					/* ld options without args */
					if (*p == 'r')
						++partial;
					n = strlen(op->o_name);
					if (tlp + n >= &tldopt[64])
						cquit("ld args too long");
					strcpy(tlp, op->o_name);
					tlp += n;
					p += n;
					continue;
				case 6:
					/* -q */
					if ((c = p[1]) == '\0'
					 || (c = getpsn(c)) < CPP)
						goto badopt;
					qpass = c;
					p += 2;
					continue;
				case 7:
					/* Pass renaming options -tBMN */
#if	MONOLITHIC
					monolithic = 0;		/* need phases */
#if	TEMPBUF
					tempsize = 0;		/* use disk temps */
#endif
#endif
					c = p[0];
					getpass(c, p+1);
					p = dnul;
					continue;
				case 8:
					/* -l */
					p = dnul;
					continue;
#if	TEMPBUF
				case 9:
					/* -T */
					tempsize = atoi(++p);
					tempsize = (tempsize + 3) & ~3;
					p = dnul;
					continue;
#endif
				case 11:
					/* -VVERSION */
					p += strlen(op->o_name);
					fprintf(stderr, "cc: "
#if	COHERENT
						"COHERENT "
#endif
#if	_I386
						"i386 "
#endif
						"cc " VERSMWC "\n");
					break;

				case 13:
					/* -? */
					usage(1);
					break;
				default:
					cquit("options");
				}
			}
			if (((unsigned)argf[i]-1)&((unsigned)argf[i])) {
				/*
				 * Handle multiple arg types in a single arg
				 * by writing the arg,
				 * then the cpp form of the arg,
				 * then the ld form of the arg.
				 * runpp and runld understand how to grovel
				 * for their args when this gets used,
				 * undoing this braindamage.
				 */
				if (optp + strlen(argv[i]) + 1
					 + 1 + strlen(ppopt) + 1
					 + strlen(tldopt) + 1 >= &optb[NCMDB])
					cquit("option buffer overflow");
				strcpy(optp, argv[i]);
				argv[i] = optp;
				optp += strlen(optp)+1;
				*optp++ = '-';
				strcpy(optp, ppopt);
				optp += strlen(optp)+1;
				strcpy(optp, tldopt);
				optp += strlen(optp)+1;
			}
		} else {
			while (*p) p+=1;
			c = *--p;
			if (*--p != '.') c = 0;
			if (c >= 'A' && c <= 'Z')
				c |= 'a'-'A';
			switch (c) {
			case 'h':
			case 'c':
				if (doutf == NULL) doutf = argv[i];
				ndotc += 1;
				argf[i] = CCARG;
				if (VeryVflag && Vflag)
					printf("cc file: %s\n", argv[i]);
				break;
			case 's':
				if (doutf == NULL) doutf = argv[i];
				ndots += 1;
				argf[i] = ASARG;
				if (VeryVflag && Vflag)
					printf("as file: %s\n", argv[i]);
				break;
			case 'm':
				if (doutf == NULL) doutf = argv[i];
				ndotm += 1;
				argf[i] = MARG;
				if (VeryVflag && Vflag)
					printf("m file: %s\n", argv[i]);
				break;
			case 'o':
				if (doutf == NULL) doutf = argv[i];
				ndoto += 1;
				argf[i] = LDARG;
				if (VeryVflag && Vflag)
					printf("ld file: %s\n", argv[i]);
				break;
			case 'a':
			default:
				ndota += 1;
				argf[i] = LDLIB;
				if (VeryVflag && Vflag)
					printf("library: %s\n", argv[i]);
				break;
			}
		}
	}
#if	GEMDOS
	if (Vflag) {
	 fprintf(stderr, "Mark Williams C for the Atari ST, %s\n", VERS);
	 fprintf(stderr, "Copyright 1984-1987, Mark Williams Co., Chicago\n");
	}
#endif
	resolve();
	compile(argc, argv);
	if (nload==0 && runld(argc, argv)==0) {
		if (Kflag==0 && nldob==1 && newo[0]!=0)
			rm(newo);
	}
#if	TEMPBUF
	if (inbuf != NULL)
		free(inbuf);
	if (outbuf != NULL)
		free(outbuf);
#if	0
	printmem("before exit");
#endif
#endif
	exit(xstat);
}

/*
 * Resolve remaining ambiguities.
 * Initialize variant arguments and files.
 */
resolve()
{
	register int i;

#if	TEMPBUF
	register char *p;

	/*
	 * The following malloc is a temporary hack for COH386 efficiency.
	 * The rationale is:
	 * (1) reduce the number of required sbrk() calls, and
	 * (2) reduce the risk of running out of memory,
	 * because COH386 without paging requires old+new bytes free
	 * to grow from old to new.
	 * This will doubtless be changed in the future.
	 */
#define	NARENA	131072
	if ((p = malloc(tempsize + tempsize + NARENA)) != NULL)
		free(p);
	if (tempsize != 0 && Eflag == 0 && Kflag == 0) {
		inbuf = malloc(tempsize);
		outbuf = malloc(tempsize);
		outbufp = outbuf;
		outbufmax = outbuf + tempsize;
		if (inbuf == NULL || outbuf == NULL) {
			fprintf(stderr, "cc: in-memory tempfile buffer allocation failed\n");
			if (inbuf != NULL)
				free(inbuf);
			inbuf = outbuf = NULL;
		}
	}
#if	0
	printmem("after buffer allocation");
#endif
#endif
	for (i = CPP; i < ALL; i += 1) {
		if (pass[i].p_flag & P_BACK)
			;
		else if (pass[i].p_flag & P_LIB)
			pass[i].p_dir = dlibpath;
		else if (pass[i].p_flag & P_BIN)
			pass[i].p_dir = dpath;
		strcpy(cmdb, pass[i].p_mch);
		strcat(cmdb, pass[i].p_pln);
#if	GEMDOS
		if (i <= ED && strchr(pass[i].p_pln, '.') == NULL)
			strcat(cmdb, i==ED ? ".tos" : ".prg");
#endif
		if (strlen(cmdb) > PTMP-1)
			cquit("pass name \"%s\" is too long", cmdb);
		strcpy(pass[i].p_pln, cmdb);
	}
#if 1
	if (isvariant(VOMF)) {
		clrvariant(VCOMM);
		if (isvariant(VLARGE) && isvariant(VSMALL))
			cquit("invalid model specification");
		if (notvariant(VLARGE) && notvariant(VSMALL))
			setvariant(VSMALL);
	} else {
		clrvariant(VLARGE);
		setvariant(VSMALL);
	}
#endif
#if	GEMDOS
	if (GEMAPPflag && GEMACCflag)
		cquit("specify VGEMAPP or VGEMACC, not both");
	if (GEMAPPflag) getpass('N', "rcrtsg.o");
	if (GEMACCflag) getpass('N', "rcrtsd.o");
#endif
	if (pflag) getpass('N', "rmcrts0.o");
	if (Eflag) setvariant(VCPP);
	if (qpass == NONE) {
		if (isvariant(VCPP))
			qpass = CPP;
		else if (Sflag)
			qpass = CC3;
		else if (cflag)
			qpass = AS;
		else
			qpass = LD;
	} else if (qpass == CPP)
		setvariant(VCPP);
	if (VeryVflag && Vflag)
		printf("quit pass is %s\n", pass[qpass].p_pln);
	makvariant(vstr);
	if (ndotm != 0 && notvariant(VCPP) && notvariant(VCPPE)) {
		setvariant(VCPP);
		setvariant(VCPPE);
		makvariant(vstr2);
		clrvariant(VCPP);
		clrvariant(VCPPE);
	}
	nfile = ndotc + ndots + ndotm;
	nname = nfile + ndoto + ndota;
	if (nname == 0)
		usage(0);
	if (qpass < LD)
		++nload;
	if (Aflag)
		maketempfile(6);
	if ( ! Eflag) {
		chkofile();
		if (Kflag) {
			pass[CPP].p_ofn = pass[CC0].p_ifn = tmp[0];
			pass[CC0].p_ofn = pass[CC1].p_ifn = tmp[1];
			pass[CC1].p_ofn = pass[CC2].p_ifn = tmp[2];
			if ( ! Sflag) {
				pass[CC2].p_sfn = tmp[3];
				pass[CC2].p_ofn = tmp[5];
			} else {
				pass[CC3].p_ifn = pass[CC2].p_ofn = tmp[3];
				pass[CC3].p_ofn = tmp[5];
			}
		} else {
			pass[CPP].p_ofn = pass[CC0].p_ifn = tmp[0];
			pass[CC0].p_ofn = pass[CC1].p_ifn = tmp[1];
			pass[CC1].p_ofn = pass[CC2].p_ifn = tmp[0];
			if ( ! Sflag) {
				pass[CC2].p_sfn = tmp[1];
				pass[CC2].p_ofn = tmp[5];
			} else {
				pass[CC3].p_ifn = pass[CC2].p_ofn = tmp[1];
				pass[CC3].p_ofn = tmp[5];
			}
#if	TEMPBUF
			if (inbuf != NULL && outbuf != NULL)
				tmp[0][0] = tmp[1][0] = '\0';
			else
#endif
			{
				maketempfile(0);
				maketempfile(1);
			}
		}
		pass[AS].p_ofn = tmp[5];
	}
}

maketempfile(i) register int i;
{
	register char *p;

	strcpy(tmp[i], p = tempnam(pass[TMP].p_dir, pass[TMP].p_pln));
	free(p);
	++istmp[i];
}

/*
** Compilation.
*/
compile(argc, argv) int argc; char *argv[];
{
	register char *p1;
	register c, i, err;

	err = 0;
	for (i=1; i<argc; ++i) {
		if ((c = (argf[i] & (CCARG | ASARG | MARG))) == 0)
			continue;
		p1 = argv[i];
		if (err) {
			err = 0;
			if (runme(p1)) {
				++nload;
			} else {
				--i;
				xstat = 0;
			}
			continue;
		}
		if (nfile > 1 && Qflag == 0)
			printf("%s:\n", p1);
		setfiles(c, p1);
		if (c == CCARG) {
			if (runpp(argc, argv, vstr)) {
				if (Aflag) {
					++err; --i;
				} else
					++nload;
				continue;
			}
			if (qpass <= CC0)
				continue;
			if (runcc(CC1)) {
				++nload;
				continue;
			}
			if (qpass <= CC1)
				continue;
			if (runcc(CC2)) {
				++nload;
				continue;
			}
			if (qpass <= CC2)
				continue;
			if (Sflag) {
				if (runcc(CC3)) {
					++nload;
					continue;
				}
				if (qpass <= CC3)
					continue;
			}
		} else if (c == ASARG && qpass>=AS) {
			if (runas()) {
				if (Aflag) {
					++err; --i;
				} else
					++nload;
				continue;
			}
			if (qpass <= AS)
				continue;
		} else if (c == MARG) {
			if (runpp(argc, argv, vstr2)) {
				if (Aflag) {
					++err; --i;
				} else
					++nload;
				continue;
			}
			if (qpass < AS)
				continue;
			if (runas()) {
				if (Aflag) {
					++err; --i;
				} else
					++nload;
				continue;
			}
			if (qpass == AS)
				continue;
		}
		makeft(p1, p1, "o");
	}
	cleanup(0);
}

runpp(argc, argv, var) int argc; register char *argv[]; char var[];
{
	register char **cp;
	register int i;
	char *p1;
#if	MONOLITHIC
	int status;
	extern int nerr;			/* in common/diag.c */
#endif

	cp = cmda;
#if	MONOLITHIC
	if (monolithic) {
		nerr = 0;
		*cp++ = "cc0";
	} else {
#endif
		p1 = cmdb;
		p1 = makepass(CC0, *cp++ = p1, AEXEC);
#if	MONOLITHIC
	}
#endif
	*cp++ = var;
	*cp++ = pass[CPP].p_ifn;
	if (qpass < CC0)
		p1 = *cp++ = Kflag ? pass[CPP].p_ofn : "-";
	else
		p1 = *cp++ = pass[CC0].p_ofn;
	for (i=1; i<argc; ++i) {
		if ((argf[i]&PPOPT) != 0) {
			*cp++ = argv[i];
			if ((argf[i]&~PPOPT) != 0)	/* see comment in main */
				cp[-1] += strlen(cp[-1])+1;
		}
	}
	*cp = 0;
#if	MONOLITHIC
	if (monolithic) {
		if ((ifp = fopen(pass[CPP].p_ifn, "r")) == NULL) {
			fprintf(stderr, "cc: cannot open %s\n", pass[CPP].p_ifn);
			return 1;
		}
		if (Eflag)
			ofp = (strcmp(p1, "-") == 0) ? stdout : ccopen(p1, "w");
		else
			ofp = (*p1 == '\0') ? NULL : ccopen(p1, SWMODE);
		argc = cp - cmda;
		if (Vflag) {
			for (i = 0; i < argc; i++)
				printf("%s ", cmda[i]);
			if (ofp == NULL)
				printf("0x%x", outbuf);
			printf("\n");
		}
		if (Aflag)
			err_open();
		status = cc0(argc, cmda);
		if (Aflag)
			err_close(status);
		closeinout();
#if	0
		printmem("after cc0");
#endif
		if (status)
			xstat = 1;
		return status;
	}
#endif
	return run(Aflag, 0);
}

#if	MONOLITHIC

#if	0
/* This is temporary stuff, here for malloc arena debugging. */
/* Knows format of malloc arena. */
#include <sys/malloc.h>
printmem(s) char *s;
{
	register int i, j;
	unsigned int u;
	register MBLOCK *p;
	register unsigned char *cp;
	int nfree, nused, bfree, bused, freef;

	printf("%s:\n", s);
	printf("__a_count=%d\n", __a_count);
	printf("__a_scanp=%x\n", __a_scanp);
	printf("__a_first=%x\n", __a_first);
	nfree = nused = bfree = bused = 0;
	for (p = __a_first, i = 0; i < __a_count; i++) {
		u = p->blksize;
		freef = isfree(u);
		u = realsize(u);
		printf("%x: size %d ", p, u-4);
		if (freef) {
			printf("free\n");
			++nfree;
			bfree += u - sizeof(unsigned);
		} else {
			++nused;
			if (u != 0) {
				printf("%x: size %d ", p, u-4);
				printf("used\n");
				bused += u - sizeof(unsigned);
			}
			if (u != tempsize + 4) {
				for (j = 4, cp = (char *)p + 4; j < u; j++)
					printf("%02x ", *cp++);
				printf("\n");
			}
		}
		p = bumpp(p, u);
	}
	printf("nfree=%d nused=%d\n", nfree, nused-1);
	printf("bfree=%d bused=%d overhead=%d total=%d\n",
		bfree, bused, 4*(nfree+nused+1), bfree+bused+4*(nfree+nused+1));
}
#endif

/*
 * Close input and output files as appropriate.
 */
closeinout()
{
	if (ifp != NULL) {
		fclose(ifp);
		ifp = NULL;
	}
	if (ofp != NULL) {
		fclose(ofp);
		ofp = NULL;
	}
}

/*
 * Set input and output file pointers for monolithic compiler.
 * Print verbose phase message if Vflag.
 */
setinout(pn) register int pn;
{
	register char *p, *in, *out, *ln;

	if (pn == CC2B) {
		ln = "cc2b";
		in = pass[CC2].p_sfn;
		out = pass[CC2].p_ofn;
	} else {
		in = pass[pn].p_ifn;
		if (pn == CC2) {
			ln = "cc2a";
			out = (Sflag) ? pass[pn].p_ofn : pass[pn].p_sfn;
		} else {
			ln = pass[pn].p_pln;
			out = pass[pn].p_ofn;
		}
	}
	if (Vflag)
		printf("%s %s ", ln, vstr);
	if (*in == '\0') {
		ifp = NULL;
		p = inbuf;
		inbuf = inbufp = outbuf;
		inbufmax = outbufp;
		outbuf = outbufp = p;
		outbufmax = outbuf + tempsize;
		if (Vflag)
			printf("0x%x ", inbuf);
	} else {
		ifp = ccopen(in, SRMODE);
		if (Vflag)
			printf("%s ", in);
	}
	if (*out == '\0') {
		ofp = NULL;
		if (Vflag)
			printf("0x%x\n", outbuf);
	} else {
		ofp = ccopen(out, SWMODE);
		if (Vflag)
			printf("%s\n", out);
	}
}
#endif

runcc(pn) register int pn;
{
	register char **cp;
#if	MONOLITHIC
	register int status;

	if (monolithic) {
		setinout(pn);
		switch(pn) {
		case CC1:
				status = cc1();
				break;
		case CC2:
				if (status = cc2a())
					 break;
				if (!Sflag) {
					closeinout();
					setinout(CC2B);
					status = cc2b();
				}
				break;
		case CC3:
				status = cc3();
				break;
		default:
				cquit("bad pass number %d", pn);
				break;
		}
		closeinout();
#if	0
		printmem((pn == CC1) ? "after cc1": (pn == CC2) ? "after cc2" : "after cc3");
#endif
		return status;
	}
#endif
	cp = cmda;
	makepass(pn, *cp++ = cmdb, AEXEC);
	*cp++ = vstr;
	*cp++ = pass[pn].p_ifn;
	*cp++ = pass[pn].p_ofn;
	if (pn == CC2)
		*cp++ = pass[CC2].p_sfn;
	*cp = 0;
	return run(0, 0);
}

runas()
{
	register char **cp;

	cp = cmda;
	makepass(AS, *cp++ = cmdb, AEXEC);
#if	_I386
	*cp++ = "-fgx";
	if (Qflag)
		*cp++ = "-w";
#else
	*cp++ = "-gx";
#endif
	*cp++ = "-o";
	*cp++ = pass[AS].p_ofn;
	*cp++ = pass[AS].p_ifn;
	*cp = 0;
	return run(Aflag, 0);
}

runme(fn)
char *fn;
{
	register char **cp;

	cp = cmda;
	makepass(ED, *cp++ = cmdb, AEXEC);
	*cp++ = fn;
	*cp++ = "-e";
	*cp++ = tmp[6];
	*cp = 0;
	return run(0, 0);
}
runld(argc, argv)
char *argv[];
{
	char *p1;
	register char *p2;
	register char **cp;
	char **cp1;
	register char **cp2;
	int i;
	static char buff[32];

	cp = cmda;
	p1 = makepass(LD, *cp++ = cmdb, AEXEC);
	*cp++ = ((ccvariant&VDB) != 0) ? "-g" : "-X";
#if	_I386
	if (Qflag)
		*cp++ = "-Q";
#endif
	if (isvariant(VNDP)) {
		/*
		 * This gets libraries and rts from /lib/ndp on system
		 * with both software fp and NDP libraries, hack hack...
		 * It also puts /usr/lib/ndp in the ld library search path.
		 */
		pass[CRT].p_flag |= P_BACK;
		pass[CRT].p_dir = "/lib/ndp";
		*cp++ = "-L/lib/ndp";
		*cp++ = "-L/usr/lib/ndp";
	}
	if (outf == NULL && !aflag) {
		*cp++ = "-o";
		if (partial || doutf == NULL) {
#if	COHERENT
#if	_I386
			*cp++ = "a.out";
#else
			*cp++ = "l.out";
#endif
#endif
#if	GEMDOS
			*cp++ = "l.prg";
#endif
		} else {
			basename(doutf, buff);
#if	GEMDOS
			strcat(buff, ".prg");
#endif
			*cp++ = buff;
		}
	}
	for (i=1; i<argc; ++i) {
		if ((argf[i]&LDOPT)!=0) {
			*cp++ = argv[i];
			if ((argf[i]&~LDOPT)!=0) {	/* see comment in main */
				cp[-1] += strlen(cp[-1])+1;
				cp[-1] += strlen(cp[-1])+1;
			}
		}
	}
	cp1 = cp;
	p1 = makepass(CRT, *cp++ = p1, AREAD);
	if (fflag) {
		*cp++ = "-u";
		*cp++ = DTEFG;
	}
	for (i=1; i<argc; ++i) {
		p2 = argv[i];
		if ((argf[i]&CCLIB)!=0) {
			while (*p2++ != 'l');
			p1 = makelib(LIB, *cp++ = p1, p2);
		} else if ((argf[i]&(CCARG|ASARG|MARG|LDARG))!=0) {
			for (cp2=cp1; cp2<cp; ++cp2)
				if (strcmp(*cp2, p2) == 0)
					break;
			if (cp2 == cp) {
				*cp++ = p2;
				++nldob;
			}
		} else if ((argf[i]&LDLIB)!=0) {
			*cp++ = p2;
		}
	}
#if	GEMDOS
	if ( ! partial && (GEMAPPflag || GEMACCflag)) {
		p1 = makelib(LIB, *cp++ = p1, "aes");
		p1 = makelib(LIB, *cp++ = p1, "vdi");
	}
#endif
	p1 = makelib(LIB, *cp++ = p1, "c");
#if	_I386
	if (Kflag == 0 && nldob == 1 && newo[0] != 0) {
		*cp++ = "-Z";
		*cp++ = newo;
	}
#endif
	*cp = 0;
	return run(0, 1);
}

basename(in, out)
char *in, *out;
{
	register char *s;
	register int n;

	if ((s = strrchr(in, PATHSEP)) != NULL)
		in = ++s;		/* skip past last PATHSEP */
	if ((s = strrchr(in, '.')) != NULL)
		n = s - in;		/* copy to last '.' */
	else
		n = strlen(in);		/* copy all */
	strncpy(out, in, n);		/* copy as appropriate */
	out[n] = '\0';			/* and NUL-terminate */
}

run(catch_errors, nofork) int catch_errors; int nofork;
{
	int s;
	extern char **environ;

	if (Vflag) {
		char *cp, **cpp;
		cpp = cmda;
		while ((cp = *cpp) != NULL) {
			if (cpp != cmda)
				putchar(' ');
			printf("%s", cp);
			++cpp;
		}
		putchar('\n');
		fflush(stdout);
	}
	if (catch_errors)
		err_open();
#if	COHERENT
#if	_I386
	if (nofork) {
		execve(cmda[0], cmda, environ);
		cquit("cannot execute %s", cmda[0]);
	}
#endif
	{
		int nsleep;
		int status;
		int pid;
		nsleep = NSLEEP;
		while ((pid=fork())<0 && nsleep--)
			sleep(DELAY);
		if (pid < 0)
			cquit("can't fork");
		if (pid == 0) {
			execve(cmda[0], cmda, environ);
			exit(MAGIC);
		}
		while (wait(&status) != pid)
			;
		if ((s = status&0177)!=0 && s!=SIGINT) {
			extern char *signame[];

			fprintf(stderr, "cc: %s ", cmda[0]);
			if (s==SIGSYS && (status&0200)==0)
				fprintf(stderr, "exec failed");
			else
				fprintf(stderr, signame[s]);
			if ((status&0200) != 0)
				fprintf(stderr, " -- core dumped");
			fprintf(stderr, "\n");
		} else if ((s = (status>>8)&0377) == MAGIC)
			cquit("cannot execute %s", cmda[0]);
	}
#else
	if (s = execve(cmda[0], cmda, environ))
		if (s < 0)
			perror(cmda[0]);
#endif
	if (catch_errors)
		err_close(s);
	if (s != 0)
		xstat = 1;
	return s;
}

/*
 * Routines to catch and release stderr for -A.
 */
int new_fd;
int saved_fd;

err_open()
{
	fflush(stderr);
	saved_fd = dup(2);
	if ((new_fd = creat(tmp[6], 0644)) < 0)
		cquit("%s: %s", tmp[6], sys_errlist[errno]);
	if (dup2(new_fd, 2) < 0)
		cquit("dup2 failed");
}

err_close(status) int status;
{
	register int c;
	register FILE *fp;

	fflush(stderr);
	if (dup2(saved_fd, 2) < 0)
		cquit("dup2 failed");
	if (close(new_fd) < 0)
		/* cquit("error on close") */ ;
	if (status == 0 && (fp = fopen(tmp[6], "r")) != NULL) {
		/* Copy warnings, otherwise they go down the rathole. */
		while ((c = getc(fp)) != EOF)
			fputc(c, stderr);
		fclose(fp);
	}
}

/*
** Option processing.
*/

/*
 * Compare option name to argument.
 */
opeq(op, ap)
register char *op, *ap;
{
	while (*op!=0)
		if (*op++ != *ap++)
			return 0;
	return 1;
}


/*
 * Get short pass name.
 *	translate single character code into pass index number.
 */
getpsn(c)
register int c;
{
	register int pn;

	for (pn = CPP; pn < ALL; pn += 1)
		if (pass[pn].p_psn == c)
			return pn;
	return NONE;
}

/*
 * Process -t*, -B*, -M* options.
 *	-t*	sets take flag for specified passes.
 *	-B*	copies prefix string for flagged passes
 *		or for all machine dependent passes if none flagged.
 *	-M*	copies machine string for machine dependent passes.
 *	-N*	renames a pass, such as crts0.o.
 */
getpass(c, cp)
register int c;
register char *cp;
{
	if (c == 't') {		/* Take passes from backup */
		if (*cp != 0)
			while ((c = *cp++) != 0)
				if ((c = getpsn(c)) >= CPP) {
					pass[c].p_flag |= P_TAKE;
					pass[c].p_dir = dnul;
					if (Vflag)
						passrpt(c);
				} else {
					badpsn(cp[-1]);
				}
		else
			getpass('t', "01234");
	} else if (c == 'B') {	/* Get backup string */
		for (c = CPP; c < ALL; c += 1) {
			if ((pass[c].p_flag&P_TAKE) != 0) {
				pass[c].p_flag ^= P_TAKE;
				pass[c].p_flag |= P_BACK;
				pass[c].p_dir = cp;
				if (Vflag)
					passrpt(c);
			}
		}
	} else if (c == 'M') {	/* Machine prefix string */
		if (strlen(cp) > PTMP-1)
			toolong(cp);
		strcpy(dmch, cp);
		if (Vflag)
			printf("Prefix: %s\n", dmch);
	} else if (c == 'N') {	/* New pass name */
		if ((c = getpsn(*cp++)) >= CPP) {
			if (strlen(cp) > PTMP-1)
				toolong(cp);
			if (Vflag)
				printf("Rename: %s to %s\n", pass[c].p_pln, cp);
			strcpy(pass[c].p_pln, cp);
		} else
			badpsn(cp[-1]);
	}
}

passrpt(c)
register int c;
{
	printf("Use: {%s}%s%s\n", pass[c].p_dir, pass[c].p_mch, pass[c].p_pln);
}

missingname()
{
	fprintf(stderr, "cc: missing name in -o, -e or -u\n");
	exit(1);
}

badoutput()
{
	fprintf(stderr, "cc: improbable name in -o: %s\n", outf);
	exit(1);
}

toomany()
{
	fprintf(stderr, "cc: too many files for -o option\n");
	exit(1);
}

badpsn(c)
{
	fprintf(stderr, "cc: unknown short path name: %c\n", c);
	exit(1);
}

toolong(cp)
char *cp;
{
	fprintf(stderr, "cc: name, prefix, or directory too long: %s\n", cp);
	exit(1);
}

/*
 * Check specified output file name,
 * or construct name from first file
 * name seen.
 */
chkofile()
{
	register char *fn;
	register char c;

	if ((fn = outf) == NULL)
		return;
	if (fn[0] == '-')
		badoutput();
	if (nname > 1 && qpass < LD)
		toomany();
	while (*fn != 0)
		++fn;
	while (*fn != '.' && fn > outf)
		--fn;
	if (*fn++ == '.' && (c = *fn++) &&  *fn == '\0') {
		if (c == 'c'
		|| c == 'h'
		|| c == 'm'
		|| (c != 'o' && cflag)
		|| (c == 'o' && !cflag)
		|| (c != 's' && Sflag)
		|| (c == 's' && !Sflag))
			badoutput();
	} else {
		if (Sflag || cflag)
			badoutput();
	}
}

whatopt(s)
char *s;
{
	fprintf(stderr, "cc: unrecognized option: %s\n", s);
	usage(0);
}

cquit(s)
char *s;
{
	fprintf(stderr, "cc: fatal error: %r\n", &s);
	cleanup(0);
	exit(1);
}

/*
** Intermediate file processing.
*/
/*
 * Given cp -> `name.[chms]' rewrite ultimate destination and intermediates
 * if necessary.
 */
setfiles(c, cp)
int c;
register char *cp;
{
	register char *ip;

	ip = cp;
	if (qpass < LD && outf != NULL)
		cp = outf;
	if (c == CCARG) {
		pass[CPP].p_ifn = ip;
		if (Eflag)
			return;
		if (Kflag) {
			makeft(pass[CPP].p_ofn, cp, "i");
			makeft(pass[CC0].p_ofn, cp, "0");
			makeft(pass[CC1].p_ofn, cp, "1");
			if ( ! Sflag)
				makeft(pass[CC2].p_sfn, cp, "2");
			else
				makeft(pass[CC2].p_ofn, cp, "2");
		}
	} else if (c == MARG) {
		pass[CPP].p_ifn = ip;
		if (Kflag)
			makeft(pass[CC0].p_ofn, cp, "s");
		pass[AS].p_ifn = pass[CC0].p_ofn;
	} else if (c == ASARG)
		pass[AS].p_ifn = ip;
	makeft(tmp[5], cp, Sflag ? "s" : "o");
	if (!Sflag && newo[0] == 0 && access(tmp[5], 0) < 0)
		strcpy(newo, tmp[5]);
}

#if	MONOLITHIC
/*
 * Open a file, die on failure.
 */
FILE *
ccopen(name, mode) char *name; char *mode;
{
	register FILE *fp;

	if ((fp = fopen(name, mode)) != NULL)
		return fp;
	cquit("cannot open file \"%s\"", name);
}
#endif

/*
 * Unlink a file.
 * If `-V' echo the rm.
 */
rm(s)
register char *s;
{
	if (Vflag)
		printf("rm %s\n", s);
	unlink(s);
}

/*
 * Cleanup after yourself if
 * the user hits interrupt during a compile (sig == SIGINT),
 * or when all compiles have completed (sig == 0).
 */
cleanup(sig)
{
	register int i;

#if COHERENT
	if (sig == SIGINT)
		signal(SIGINT, SIG_IGN);
#endif
	for (i = 0; i < 7; i += 1)
		if (istmp[i])
			rm(tmp[i]);
#if COHERENT
	if (sig == SIGINT && nldob==1 && newo[0]!=0)
		rm(newo);
	if (sig == SIGINT)
		exit(1);
#endif
}

/*
** Name construction.
*/
char *
ccpath(cp, p, n, mode) register char *cp; char *p, *n; int mode;
{
	register char *pn;
	if (VeryVflag && Vflag)
		printf("search %s for %s\n", p, n);
	while ((pn = path(p, n, mode)) == NULL) {
		if (Zflag) {	/* Disk swap dialog flag */
fprintf(stderr, "Insert %s's disk in a free drive and press <Return>\n", n);
			gets(cp);
			continue;
		}
		pn = n;
		break;
	}
	while (*cp++ = *pn++) ;
	return cp;
}

char *
makepass(pn, cp, mode)
int pn;
register char *cp;
int mode;
{
	return ccpath(cp, pass[pn].p_dir, pass[pn].p_pln, mode);
}

char *
makelib(pn, cp, lp)
int pn;
char *cp;
char *lp;
{
#if	_I386
	/* Let ld do the work, just pass -lwhatever. */
	strcpy(cp, "-l");
	strcat(cp, lp);
	return cp + strlen(cp) + 1;
#else
	strcpy(cp, pass[pn].p_pln);
	strcat(cp, lp);
	strcat(cp, ".a");
	return ccpath(cp, pass[pn].p_dir, cp, AREAD);
#endif
}

makeft(op, ip, ft)
char *op, *ip, *ft;
{
	register char *ep, *tp;
	register c;
	char *dp;

	ep = ip;
	tp = op;
	dp = NULL;
	while ((c = *ep++) != '\0') {
		if (c == PATHSEP && ip != outf) {
			tp = op;
			dp = NULL;
		} else {
			if (c == '.')
				dp = tp;
			*tp++ = c;
		}
	}
	if (ip == outf)
		return;
	if (dp != NULL)
		tp = dp;
	*tp++ = '.';
	ep = ft;
	while (*tp++ = *ep++)
		;
}

/*
 * Print a terse or verbose usage message.
 */
usage(flag) register int flag;
{
	register FILE *ofp;

	ofp = (flag) ? stdout : stderr;
	fprintf(ofp, "Usage: cc [ option ... ] file ...\n");
	if (flag == 0)
		fprintf(ofp, "Type \"cc -?\" to list the available options.\n");
	else
		fprintf(ofp,	
		"Options:\n"
		"\t-A\t\tAuto edit mode\n"
		"\t-Bpathname\tUse pathname to find substitute compiler passes\n"
		"\t-Dname[=value]\tcpp: #define\n"
		"\t-E\t\tExpand: run preprocessor to stdout\n"
		"\t-Ipathname\tcpp: #include search directory\n"
		"\t-K\t\tKeep intermediate files\n"
		"\t-Lpathname\tld: library directory specification\n"
		"\t-Mstring\t\tUse string as cross-compiler prefix\n"
		"\t-N[01ab2sdlrt]string\tRename pass with string\n"
		"\t-O\t\tOptimize: run object code optimizer\n"
		"\t-Q\t\tQuiet: no error or warning messages\n"
		"\t-S\t\tOutput assembly language\n"
		"\t-T[value]\tTemp size for in-memory tempfiles (default: 64K)\n"
		"\t-Uname\t\tcpp: #undef\n"
		"\t-V\t\tVerbose: report everything\n"
		"\t-Vvariant\tVariant enable/disable, see list below\n"
		"\t-X\t\tld: remove C-generated local symbols\n"
#if	GEMDOS
		"\t-Z\t\t(GEMDOS) floppy change prompts for phases\n"
#endif
		"\t-a\t\tDo not use implicit output file name for loader\n"
		"\t-c\t\tCompile only, do not load\n"
		"\t-d\t\tld: define common space\n"
		"\t-e name\t\tld: entry point specification\n"
		"\t-f\t\tld: use floating point output conversion routines\n"
		"\t-g\t\tGenerate debug information, same as -VDB\n"
		"\t-i\t\tld: separate i and d spaces\n"
		"\t-lname\t\tld: library specification\n"
		"\t-n\t\tld: shared instruction space\n"
		"\t-o name\t\tld: output file name\n"
		"\t-p\t\tProfile: generate code to profile function calls\n"
		"\t-q[p012s]\tQuit after specified pass\n"
		"\t-r\t\tld: retain relocation information in output\n"
		"\t-s\t\tld: strip symbol table\n"
		"\t-t[p012adlrt]\ttake specified passes from -Bdirectory\n"
		"\t-u name\t\tld: undefine name\n"
		"\t-v\t\tVerbose: report everything, same as -V\n"
		"\t-w\t\tld: watch\n"
		"\t-x\t\tld: remove all local symbols from symbol table\n"
		"Variant options:\n"
		"\t-VASM\t\tAssembly language output, same as -S\n"
		"\t-VCOMM\t\tCommon data items (default)\n"
		"\t-VCPLUS\t\tIgnore C++ style online //-delimited comments\n"
		"\t-VCPPE\t\tRun cpp in -E mode, same as -E\n"
		"\t-VDB\t\tDebug: generate debug output, same as -g\n"
		"\t-VFLOAT\t\tFloating point output conversion, same as -f\n"
		"\t-VKEEP\t\tKeep intermediate files, same as -K\n"
		"\t-VNDP\t\tUse hardware (80x87) floating point\n"
		"\t-VNOOPT\t\tNo optimization\n"
		"\t-VNOWARN\tNo warning messages\n"
		"\t-VPEEP\t\tPeephole optimization (default)\n"
		"\t-VPROF\t\tProfile: generate code to profile function calls\n"
		"\t-VPSTR\t\tPure strings (default)\n"
		"\t-VQUIET\t\tNo messages, same as -Q\n"
		"\t-VREADONLY\tRecognize \"readonly\" keyword\n"
		"\t-VS\t\tStrict: turn on all strict messages\n"
		"\t-VSBOOK\t\tStrict: only constructs in K&R\n"
		"\t-VSCCON\t\tStrict: constant conditional\n"
		"\t-VSINU\t\tAllow struct-in-union constructs\n"
		"\t-VSLCON\t\tStrict: long constant promotion (default)\n"
		"\t-VSMEMB\t\tStrict: strict member checking (default)\n"
		"\t-VSNREG\t\tStrict: declared register but allocated auto\n"
		"\t-VSPVAL\t\tStrict: pointer value truncation (default)\n"
		"\t-VSRTVC\t\tStrict: risky constructs in truth contexts\n"
		"\t-VSTAT\t\tStatistics on optimization\n"
		"\t-VSUREG\t\tStrict: unused registers\n"
		"\t-VSUVAR\t\tStrict: unused variables (default)\n"
		"\t-VVERSION\tPrint compiler version number\n"
		"\t-VWIDEN\t\tWidening of parameter or function value warning\n"
		"\t-VXSTAT\t\tWrite static external symbols\n"
		"\t-V3GRAPH\tRecognize ANSI trigraphs\n"
		);
		exit(1);
#if	0
/*
 * The following variants are not included in the usage(1) output above
 * because they are useful primarily for MWC internal purposes
 * or they are for machine models which are currently irrelevant
 * or because I'm not sure what they do.
 */
/* Miscellaneous */
/* ld does not grok -k, dunno why -k exists. */
"\tk system\tld: bind as kernel process\n"
VALIEN\t\tAlien (PL/M) calling conventions
VALIGN\t\tAlign the stack (i8086)
VTPROF\t\tCode table profiling
VROM\t\tProduce ROMable code
VLIB
VCPPC\t
VCPP\t
/* Debug */
VDEBUG\tDebug: 
VDLINE\tDebug: 
VDTYPE\tDebug: 
VDSYMB\tDebug: 
/* Intel */
VSMALL
VLARGE
V8087
VRAM
VOMF
V80186 (default)
V80287
VEMU87
#if	GEMDOS
/* Motorola and Gemdos */
VGEMACC
VGEMAPP
VGEM
VSPLIM
VNOTRAPS
#endif
#endif
}

/* end of coh/cc.c */
