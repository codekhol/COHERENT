/*
 * A debugger.
 * Intel 8086.
 */
#include <stdio.h>
#include <sys/const.h>
#include <l.out.h>
#include <signal.h>
#include <sys/uproc.h>
#include "trace.h"
#include "i8086.h"

/*
 * Given an l.out header, set up segmentation for an l.out.
 */
setaseg(ldhp)
register struct ldheader *ldhp;
{
	register long fbase;
	register long si;
	register long pi;
	register long bi;
	register long sd;
	register long pd;
	register long rs;

	fbase = sizeof(*ldhp);
	si = ldhp->l_ssize[L_SHRI];
	pi = ldhp->l_ssize[L_PRVI];
	bi = ldhp->l_ssize[L_BSSI];
	sd = ldhp->l_ssize[L_SHRD];
	pd = ldhp->l_ssize[L_PRVD];
	switch (ldhp->l_flag & (LF_SEP|LF_SHR)) {
	case 0:
		endpure = NULL;
		DSPACE = setsmap(NULL, (long)0, si+pi, fbase,
				getf, putf, 0);
		DSPACE = setsmap(DSPACE, si+pi+bi, sd+pd, fbase+si+pi,
				getf, putf, 0);
		ISPACE = DSPACE;
		break;

	case LF_SHR:
		endpure = NULL;
		rs = (si+sd+0xF) & ~0xF;
		DSPACE = setsmap(NULL, (long)0, si, fbase, getf, putf, 0);
		DSPACE = setsmap(DSPACE, si, sd, fbase+si+pi, getf, putf, 0);
		DSPACE = setsmap(DSPACE, rs, pi, fbase+si, getf, putf, 0);
		DSPACE = setsmap(DSPACE, rs+pi, pd, fbase+si+pi+sd,
				getf, putf, 0);
		ISPACE = DSPACE;
		break;

	case LF_SEP:
		endpure = NULL;
		ISPACE = setsmap(NULL, (long)0, si+pi, fbase, getf, putf, 0);
		DSPACE = setsmap(NULL, (long)0, sd+pd, fbase+si+pi,
				getf, putf, 0);
		break;

	case LF_SEP|LF_SHR:
		endpure = NULL;	/* No pure data */
		rs  = (sd+0xF) & ~0xF;
		ISPACE = setsmap(NULL, (long)0, si+pi, fbase, getf, putf, 0);
		DSPACE = setsmap(NULL, (long)0, sd, fbase+si+pi, getf, putf, 0);
		DSPACE = setsmap(DSPACE, rs, pd, fbase+si+pi+sd, getf, putf, 0);
		break;
	}
}

/*
 * Set up segmentation for kernel memory.
 */
setkmem(np)
char *np;
{
	register fsize_t l;
	unsigned n;
	struct ldsym lds;

	cfp = openfil(np, rflag);
	USPACE = setsmap(NULL, (fsize_t)0, (fsize_t)LI, (fsize_t)0,
		getf, putf, 1);
	add = 2;
	if (getb(2, &n, sizeof(n)) == 0)
		panic("Bad memory file");
	l = (fsize_t)n << 4;
	ISPACE = setsmap(NULL, (fsize_t)0, (fsize_t)LI, (fsize_t)l,
		getf, putf, 1);
	strncpy(lds.ls_id, "etext_", NCPLN);
	if (nameval(&lds) == 0)
		DSPACE = ISPACE;
	else {
		n = (lds.ls_addr+0xF) & ~0xF;
		DSPACE = setsmap(NULL, (fsize_t)0, (fsize_t)LI, (fsize_t)l+n,
			getf, putf, 1);
	}
}

/*
 * Set up segmentation for a system dump.
 */
setdump(np)
char *np;
{
	setkmem(np);
}

/*
 * Update register structure in memory.
 */
setregs()
{
	struct ureg ureg;

	regflag = 0;
	add = UREGOFF;
	if (getb(2, (char *)&ureg, sizeof(ureg)) == 0) {
		printr("Cannot read registers");
		return (0);
	}
	copyreg(&ureg);
	regflag = 1;
	return (1);
}

/*
 * Copy registers from the User area structure to the register
 * structure.
 */
copyreg(up)
register struct ureg *up;
{
	reg.r_ax = up->ur_ax;
	reg.r_bx = up->ur_bx;
	reg.r_cx = up->ur_cx;
	reg.r_dx = up->ur_dx;
	reg.r_sp = up->ur_sp;
	reg.r_bp = up->ur_bp;
	reg.r_si = up->ur_si;
	reg.r_di = up->ur_di;
	reg.r_cs = up->ur_cs;
	reg.r_ds = up->ur_ds;
	reg.r_ss = up->ur_ss;
	reg.r_es = up->ur_es;
	reg.r_ip = up->ur_ip;
	reg.r_fw = up->ur_fw;
}

/*
 * If the given name matches a register, set up `ldp' with the address
 * of a registers in the user area.
 */
regaddr(ldp)
struct ldsym *ldp;
{
	register char *cp;
	register int c0;
	register int c1;
	register int a;

	a = UPASIZE - sizeof(struct ureg);
	cp = ldp->ls_id;
	if ((c0=*cp++) == '\0')
		return (0);
	if ((c1=*cp++) == '\0')
		return (0);
	if (*cp != '\0')
		return (0);
	switch (c1) {
	case 'h':
		a++;
	case 'l':
		c1 = 'x';
	}
	cp = regitab;
	do {
		if (cp[0]==c0 && cp[1]==c1) {
			ldp->ls_addr = a;
			ldp->ls_type = L_REG;
			return (1);
		}
		a += 2;
		cp += 2;
	} while (*cp != '\0');
	return (0);
}

/*
 * Return the programme counter.
 */
vaddr_t
getpc()
{
	return (reg.r_ip);
}

/*
 * Set the programme counter.
 */
setpc(ip)
vaddr_t ip;
{
	reg.r_ip = ip;
	add = UREGOFF + offset(ureg, ur_ip);
	putb(2, (char *)&reg.r_ip, sizeof(reg.r_ip));
}

/*
 * Return the frame pointer.
 */
vaddr_t
getfp()
{
	return (reg.r_bp);
}

/*
 * Display registers.
 */
dispreg()
{
	if (testint())
		return;
	printf("ip=%04x fw=%04x\n", reg.r_ip, reg.r_fw);

	if (testint())
		return;
	printf("ax=%04x bx=%04x cx=%04x dx=%04x\n",
		reg.r_ax, reg.r_bx, reg.r_cx, reg.r_dx);

	if (testint())
		return;
	printf("sp=%04x bp=%04x si=%04x di=%04x\n",
		reg.r_sp, reg.r_bp, reg.r_si, reg.r_di);

	if (testint())
		return;
	printf("cs=%04x ds=%04x ss=%04x es=%04x\n",
		reg.r_cs, reg.r_ds, reg.r_ss, reg.r_es);
}

/*
 * Stack traceback.
 */
dispsbt(l)
{
	int	sdbase = dbase;
	register int argc;
	register int iflag;
	register int sflag;
	register int qflag;
	unsigned amain;
	unsigned rpc;
	unsigned pc;
	unsigned fp;
	unsigned n;
	unsigned char e[1];
	unsigned char b[3];
	struct ldsym ldsym;

	dbase = 16;
	amain = 1;
	strcpy(ldsym.ls_id, "main_");
	if (nameval(&ldsym) != 0)
		amain = ldsym.ls_addr;
	add = reg.r_ip;
	if (getb(1, b, 1) == 0) {
		printe("Illegal ip");
		goto done;
	}
	sflag = 0;
	fp = reg.r_bp;
	e[0] = 0x56;				/* push si */
	if (valname(1, (vaddr_t)reg.r_ip, &ldsym) != 0) {
		add = (long)ldsym.ls_addr;
		getb(1, (char *)e, 1);
	}
	if (b[0]==0x56 || e[0]!=0x56) {		/* push si */
		sflag = 1;
		fp = reg.r_sp - 6;
	}
	pc = reg.r_ip;
	qflag = 0;
	for (;;) {
		add = fp + 6;
		if (getb(0, &rpc, 2) == 0)
			break;
		argc = 0;
		add = rpc;
		if (getb(1, b, 3) != 0) {
			if (b[0]==0x83 && b[1]==0xC4)	/* add sp,$? */
				argc = b[2]/2;
		}
		add = rpc - 3;
		if (getb(1, b, 1) == 0)
			break;
		if (getb(1, &n, sizeof(n)) == 0)
			break;
		printx(DAFMT, (long)fp);
		printx("  ");
		printx(DAFMT, (long)pc);
		printx("  ");
		if (b[0] == 0xE8) {			/* call ? */
			n += rpc;
			putaddr(1, (long)n, I);
			if (n == amain) {
				qflag = 1;
				argc = 3;
			}
		} else {
			if (valname(1, (vaddr_t)pc, &ldsym) == 0)
				printx("*");
			else {
				printx("%.*s", NCPLN, ldsym.ls_id);
				if (strcmp(ldsym.ls_id, "main_") == 0) {
					qflag = 1;
					argc = 3;
				}
			}
		}
		putx('(');
		iflag = 0;
		add = fp + 8;
		while (argc--) {
			if (getb(0, &n, 2) == 0)
				break;
			if (iflag++ != 0)
				printx(", ");
			putaddr(1, (long)n, -1);
		}
		printx(")\n");
		if (testint())
			goto done;
		if (qflag)
			break;
		if (sflag) {
			sflag = 0;
			fp = reg.r_bp;
		} else {
			add = fp;
			if (getb(0, &fp, sizeof(fp)) == 0)
				break;
		}
		if (--l == 0)
			goto done;
		pc = rpc;
	}
done:	dbase = sdbase;
}

/*
 * Get the return pc and frame pointer to set a return breakpoint.
 */
setretf(pcp, fpp)
register vaddr_t *pcp;
register vaddr_t *fpp;
{
	char b[1];

	add = reg.r_ip;
	if (getb(0, b, sizeof(b)) == 0)
		return (0);
	if (b[0] == 0x56) {			/* push si */
		*fpp = reg.r_bp;
		add = reg.r_sp;
	} else {
		add = reg.r_bp;
		if (getb(0, (char *)fpp, sizeof(*fpp)) == 0)
			return (0);
		add = reg.r_bp + 6;
	}
	if (getb(0, (char *)pcp, sizeof(*pcp)) == 0)
		return (0);
}

/*
 * Initialise after getting a trap.
 */
trapint()
{
	vaddr_t pc;
	vaddr_t fw;

	cacsegn = -1;
	add = UREGOFF + offset(ureg, ur_fw);
	if (getb(2, (char *)&fw, sizeof(fw)) == 0) {
		printr("Cannot get fw");
		return (0);
	}
	fw &= ~TBIT;
	add = UREGOFF + offset(ureg, ur_fw);
	if (putb(2, (char *)&fw, sizeof(fw)) == 0) {
		printr("Cannot set fw");
		return (0);
	}
	if (sysflag) {
		add = UREGOFF + offset(ureg, ur_ip);
		if (getb(2, (char *)&pc, sizeof(pc)) == 0) {
			printr("Cannot get pc");
			return (0);
		}
		pc -= 2;
		add = UREGOFF + offset(ureg, ur_ip);
		if (putb(2, (char *)&pc, sizeof(pc)) == 0) {
			printr("Cannot set pc");
			return (0);
		}
		add = (long)pc;
		if (putb(1, (char *)sin, sizeof(sin)) == 0) {
			printr("Cannot set sin");
			return (0);
		}
		bitflag = 1;
	}
	return (1);
}

/*
 * Set up before returning from a trap.  Single stepping over a system
 * call is handled here.
 */
restret()
{
	char w[1];

	sysflag = 0;
	if (bitflag == 0)
		return (1);
	add = reg.r_ip;
	if (getb(1, (char *)w, sizeof(w)) == 0)
		return (1);
	if (w[0] != 0xCD)		/* Interrupt (system call) */
		return (1);
	add = reg.r_ip + 2;
	if (getb(1, (char *)sin, sizeof(sin)) == 0) {
		printr("Cannot get breakpoint");
		return (0);
	}
	add -= sizeof(sin);
	if (putb(1, (char *)bin, sizeof(bin)) == 0) {
		printr("Cannot set breakpoint");
		return (0);
	}
	bitflag = 0;
	sysflag = 1;
	return (1);
}

/*
 * Set up breakpoint for single step continue.
 */
setcont()
{
	char w[2];

	add = reg.r_ip;
	if (getb(1, (char *)w, sizeof(w)) == 0)
		return;
	if (w[0] == 0xE8)			/* Direct call */
		sinmode = SCSET;
	if (w[0]==0xFF && (w[1]&0x38)==0x10)	/* Indirect call */
		sinmode = SCSET;
}

/*
 * Continue after initial break in single step continue.
 */
intcont()
{
	unsigned a;

	add = reg.r_sp;
	if (getb(0, (char *)&a, sizeof(a)))
		setibpt(BSIN, a, reg.r_bp, NULL);
}

/*
 * See if the given instruction is a breakpoint.
 */
testbpt(pc)
{
	char w[1];

	add = pc;
	return (getb(1, (char *)w, sizeof(w))!=0 && w==0xCC);
}

/*
 * Read `n' characters into the buffer `bp' starting at address `a'
 * from the traced process.
 */
getp(f, la, bp, n)
long la;
register char *bp;
register int n;
{
	register unsigned a;

	a = la;
	if (a != la)
		return (0);
	while (n--) {
		if (getq(f, a++, bp++) == 0)
			return (0);
	}
	return (1);
}

/*
 * Write `n' characters from the buffer `bp' starting at address `a' in
 * the traced process.
 */
putp(f, la, bp, n)
long la;
register char *bp;
register int n;
{
	extern int errno;
	int d;
	register unsigned a;

	a = la;
	if (a != la)
		return (0);
	cacsegn = -1;
	if (n!=0 && (a&1)!=0) {
		if (getq(f, --a, (char *)&d) == 0)
			return (0);
		((char *)&d)[1] = *bp++;
		ptrace(4+f, pid, (int *) a, d);
		if (errno)
			return (0);
		a += 2;
		--n;
	}
	while (n > 1) {
		((char *)&d)[0] = *bp++;
		((char *)&d)[1] = *bp++;
		ptrace(4+f, pid, (int *) a, d);
		if (errno)
			return (0);
		a += 2;
		n -= 2;
	}
	if (n) {
		((char *)&d)[0] = *bp++;
		if (getq(f, a+1, &((char *)&d)[1]) == 0)
			return (0);
		ptrace(4+f, pid, (int *)a, d);
		if (errno)
			return (0);
	}
	return (1);
}

/*
 * Return the byte at address `a' in segment `f' indirectly through
 * the byte pointer `bp'.
 */
getq(f, a, bp)
register int a;
register char *bp;
{
	extern int errno;
	register int w;

	if ((w=(a&~1))!=cacaddr || f!=cacsegn) {
		errno = 0;
		cacdata = ptrace(1+f, pid, (int *) w, 0);
		if (errno) {
			cacsegn = -1;
			return (0);
		}
		cacsegn = f;
		cacaddr = w;
	}
	*bp = ((char *)&cacdata)[a&1];
	return (1);
}
