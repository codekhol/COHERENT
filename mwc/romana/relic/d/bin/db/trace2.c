/*
 * A debugger.
 * Runtime support.
 */
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "trace.h"
#if NOUT
#include <n.out.h>
#else
#include <l.out.h>
#endif
#include <sys/uproc.h>
#include <signal.h>

/*
 * Talk to the user and try to solve his problems.
 */
process()
{
	register BPT *bp;
	register int n;
	register int nibflag;
	register int bptflag;
	register vaddr_t pc;
	register vaddr_t fp;
	register int f;

	/*
	 * Let the user speak.
	 */
top:
	bptflag = 0;
	execute(":x\n");
	for (;;) {
		/*
		 * Initialise flags.
		 */
		bitflag = 0;
		nibflag = 0;
	
		/*
		 * If in single step mode, set up for single step.
		 */
		if (sinmode!=SNULL && sinmode!=SWAIT) {
			bitflag = 1;
			if (sinmode == SCONT)
				setcont();
		}
	
		/*
		 * Place all breakpoints.
		 */
		pc = getpc();
		for (n=NBPT, bp=&bpt[0]; n--; bp++) {
			if (bp->b_flag == 0)
				continue;
			add = bp->b_badd;
			if (getb(1, bp->b_bins, sizeof(BIN)) == 0) {
				fprintf( stderr, "cannot read\n");
				printb(bp->b_badd);
				goto err;
			}
			if (bptflag!=0 && bp->b_badd==pc) {
				bitflag = 1;
				continue;
			}
			add = bp->b_badd;
			if (putb(1, bin, sizeof(BIN)) == 0) {
				fprintf( stderr, "cannot install\n");
				printb(bp->b_badd);
				goto err;
			}
		}
		bptflag = 0;

		/*
		 * Set flags and call machine dependent restore routine.
		 */
		if (testbpt(pc) != 0)
			nibflag = 1;
		if (restret() == 0)
			goto err;
	
		/*
		 * Start up the child.
		 */
		errno = 0;
		ptrace(bitflag?9:7, pid, (int *)1, 0);
		if (errno) {
			printr("Ptrace error (%d)", errno);
			goto err;
		}
		if (waitc() == 0)
			goto err;
	
		/*
		 * Get registers and call machine dependent trap routine.
		 */
		if (trapint() == 0)
			goto err;
		if (setregs() == 0)
			goto err;
		if ((f=settrap()) == 0)
			goto err;
		/*
		 * Replace breakpoints with instructions.
		 */
		for (n=NBPT, bp=&bpt[0]; n--; bp++) {
			if (bp->b_flag == 0)
				continue;
			add = bp->b_badd;
			if (putb(1, bp->b_bins, sizeof(BIN)) == 0) {
				fprintf( stderr, "cannot replace instr\n");
				printb(bp->b_badd);
				goto err;
			}
		}
	
		/*
		 * If trap was not a breakpoint, tell the user, accept input.
		 */
		if (f != SIGTRAP) {
			execute(":f\n:x\n");
			continue;
		}

		/*
		 * Find the breakpoint we are at.
		 */
		pc = getpc() - sizeof(BIN);
		if (bitflag==0 || nibflag!=0) {
			for (n=NBPT, bp=&bpt[0]; n--; bp++) {
				if (bp->b_flag == 0)
					continue;
				if (bp->b_badd != pc)
					continue;
				setpc((vaddr_t)pc);
				bptflag = 1;
				break;
			}
		}

		/*
		 * If in single step mode, execute command.
		 */
		switch (sinmode) {
		case SSTEP:
		case SCONT:
			execute(sinp);
			if (--sindecr == 0)
				execute(":x\n");
			continue;
		case SCSET:
			for (n=0, bp=&bpt[0]; n<NBPT; n++, bp++)
				bp->b_flag &= ~BSIN;
			sinmode = SWAIT;
			intcont();
			break;
		}

		/*
		 * If we got an unexpected trace trap or unknown
		 * breakpoint, we handle it here.
		 */
		if (bp == NULL) {
			if (bitflag==0 || nibflag!=0)
				execute(":f\n:x\n");
			continue;
		}

		/*
		 * Single step breakpoints have highest priority.
		 */
		fp = getfp();
		if ((bp->b_flag&BSIN) != 0) {
			if (bp->b_sfpt==0 || bp->b_sfpt==fp) {
				bp->b_flag &= ~BSIN;
				if (sinmode == SWAIT) {
					sinmode = SCONT;
					execute(sinp);
					if (--sindecr == 0)
					       execute(":x\n");
					continue;
				}
			}
		}

		/*
		 * Return breakpoints are next.
		 */
		if ((bp->b_flag&BRET) != 0) {
			if (fp == bp->b_rfpt) {
				bp->b_flag &= ~BRET;
				execute(bp->b_rcom);
				continue;
			}
		}

		/*
		 * Your conventional everyday ordinary breakpoint.
		 */
		if ((bp->b_flag&BBPT) != 0) {
			execute(bp->b_bcom);
			continue;
		}
	}

	/*
	 * Something is terribly wrong.  Kill off our child,
	 * and generally reset everything to the start.
	 */
err:
	killc();
	reslout();
	bptinit();
	goto top;

	/*
	 * They say psychiatry is one of the hardest occupations.
	 * I think I agree.
	 */
}

/*
 * Reset segmentation for an l.out.
 */
reslout()
{
	struct ldheader ldh;

	clramap();
	objflag = 0;
	fseek(lfp, (long)0, 0);
	if (fread(&ldh, sizeof(ldh), 1, lfp) != 1) {
		printr("Can't read object file");
		return (0);
	}
	canlout(&ldh);
	if (ldh.l_magic != L_MAGIC) {
		printr("not an object file");
		return (0);
	}
	objflag = 1;
	setaseg(&ldh);
	return (1);
}

/*
 * Given a command line in `miscbuf', parse the command line, kill the
 * current child and start up a new one.  0 is returned on success, 1
 * on failure.
 */
runfile()
{
	register char *bp, *cp;
	register int c;
	char *ifn, *ofn, *argl[ARGSIZE];
	int qflag, aflag, n;

	killc();
	if (objflag == 0) {
		printe("No l.out");
		return (1);
	}
	ifn = NULL;
	ofn = NULL;
	qflag = 0;
	aflag = 0;
	n = 0;
	bp = miscbuf;
	cp = miscbuf;
	c = *bp++;
	while (c != '\n') {
		switch (c) {
		case '<':
			ifn = cp;
			c = *bp++;
			break;
		case '>':
			ofn = cp;
			if ((c=*bp++) == '>') {
				aflag = 1;
				c = *bp++;
			}
			break;
		default:
			if (n >= ARGSIZE-1) {
				printe("Too many arguments");
				return (1);
			}
			argl[n++] = cp;
		}
		while (qflag || !isascii(c) || !isspace(c)) {
			if (c == '\n')
				break;
			if (c == '"') {
				qflag ^= 1;
				c = *bp++;
				continue;
			}
			if (c == '\\') {
				if ((c=*bp++) == '\n') {
					printe("Syntax error");
					return (1);
				}
			}
			*cp++ = c;
			c = *bp++;
		}
		if (qflag) {
			printe("Missing \"");
			return (1);
		}
		*cp++ = '\0';
		if (c == '\n')
			break;
		while (isascii(c) && isspace(c))
			c = *bp++;
	}
	if (n == 0)
		argl[n++] = lfn;
	argl[n] = NULL;
	if (startup(argl, ifn, ofn, aflag) == 0)
		return (1);
	return (0);
}

/*
 * Start execution of the child.  `argv' is the argument list, `ifnp' is
 * the name of the input file, `ofnp' is the name of the output file and
 * `appf' tells us whether the output file is opened for append or write.
 */
startup(argv, ifnp, ofnp, appf)
char **argv;
char *ifnp;
char *ofnp;
{
	register int n;

	if ((pid=fork()) < 0) {
		printr("Cannot fork");
		return (0);
	}
	if (pid == 0) {
		if (ifnp != NULL) {
			if ((n=open(ifnp, 0)) < 0)
				panic("Cannot open %s", ifnp);
			dup2(n, 0);
			close(n);
		}
		if (ofnp != NULL) {
			n = -1;
			if (appf) {
				if ((n=open(ofnp, 1)) >= 0)
					lseek(n, 0L, 2);
			}
			if (n < 0) {
				if ((n=creat(ofnp, 0644)) < 0)
					panic("%s: cannot create", ofnp);
			}
			dup2(n, 1);
			close(n);
		}
		ptrace(0, 0, NULL, 0);
		execv(lfn, argv);
		exit(1);
	}
	if (waitc() == 0)
		return (0);
	clramap();
	DSPACE = setsmap(NULL, (fsize_t)0, (fsize_t)LI, (fsize_t)0,
			getp, putp, 1);
	ISPACE = setsmap(NULL, (fsize_t)0, (fsize_t)LI, (fsize_t)0,
			getp, putp, 0);
	USPACE = setsmap(NULL, (fsize_t)0, (fsize_t)UPASIZE, (fsize_t)0,
			getp, putp, 2);
	excflag = 1;
	regflag = 1;
	return (1);
}

/*
 * Given a newly started child, find out necessary information.
 */
shiftup()
{
	if (trapint() == 0)
		return (0);
	if (setregs() == 0)
		return (0);
	return (settrap());
}

/*
 * Kill off our child.
 */
killc()
{
	if (excflag) {
		ptrace(8, pid, 0, 0);
		waitc();
	}
	excflag = 0;
	regflag = 0;
	trapstr = NULL;
}

/*
 * Wait for the traced process to stop.
 */
waitc()
{
	register int p;
	int s;

	while ((p=wait(&s)) != pid) {
		if (p >= 0) {
			printr("Adopted a child %d", p);
			continue;
		}
		if (intflag == 0) {
			excflag = 0;
			printr("Nonexistant child");
			return (0);
		}
		intflag = 0;
	}
	if ((s&0377) != 0177) {
		excflag = 0;
		printr("Child process terminated (%d)", (s>>8)&0377);
		return (0);
	}
	return (1);
}
