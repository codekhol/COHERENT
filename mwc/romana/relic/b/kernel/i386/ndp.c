/*
 * File:	ndp.c
 *
 * Purpose:	all ndp-related functions, except for assembler routines
 *
 * $Log:	ndp.c,v $
 * Revision 1.7  93/06/14  13:43:02  bin
 * Hal: kernel 78 update
 * 
 * Revision 1.2  93/04/14  10:27:57  root
 * r75
 * 
 * Revision 1.1  92/11/09  17:09:23  root
 * Just before adding vio segs.
 * 
 */

/*
 * ----------------------------------------------------------------------
 * Includes.
 */
#include <sys/coherent.h>
#include <errno.h>
#include <sys/seg.h>

/*
 * ----------------------------------------------------------------------
 * Definitions.
 *	Constants.
 *	Macros with argument lists.
 *	Typedefs.
 *	Enums.
 */
/* bit positions in u.u_ndpFlags */
#define NF_NDP_USER	1	/* this process has used the ndp */
#define NF_NDP_SAVED	2	/* ndp status is saved in u area */
#define NF_EM_TRAPPED	4	/* no ndp, em trap has occurred */

/* supported coprocessor types - will autosense if initially unpatched */
#define NDP_TYPE_UNPATCHED	0
#define NDP_TYPE_NONE		1
#define NDP_TYPE_287		2
#define NDP_TYPE_387		3
#define NDP_TYPE_486		4

#define NDP_IRQ		13	/* 387 uses Irq for unmasked exceptions */
#define NDP_PORT	0xF0	/* 387 uses this port to clear exception */
/*
 * ----------------------------------------------------------------------
 * Functions.
 *	Import Functions.
 *	Export Functions.
 *	Local Functions.
 */
void	emFinit();
void	emtrap();
void	fptrap();
void	ndpConRest();
void	ndpDetach();
void	ndpEmTraps();
void	ndpEndProc();
void	ndpIrq();
void	ndpMine();
void	ndpNewOwner();
void	ndpNewProc();
char *	ndpTypeName();
int	rdEmTrapped();
int	rdNdpSaved();
int	rdNdpSavedU();
int	rdNdpUser();
void	senseNdp();
void	wrEmTrapped();
void	wrNdpSaved();
void	wrNdpSavedU();
void	wrNdpUser();

/*
 * ----------------------------------------------------------------------
 * Global Data.
 *	Import Variables.
 *	Export Variables.
 *	Local Variables.
 */

/*
 * ndp control word is 16 bits:
 * 0000 RC:2 PC:2 01 PM:1 UM:1 OM:1 ZM:1 DM:1 IM:1
 * RC - rounding control
 * PC - precision control
 * PM - precision mask
 * UM - underflow mask
 * OM - overflow mask
 * ZM - zero divide mask
 * DM - denormal operand mask
 * IM - invalid operation mask
 * for masks, 1 masks the exception
 *
 * iBCS2 page 3-46 specifies the following:
 *   0000 : 00 10 : 0 1 1 1 : 0 0 1 0 = 0x0272
 */

/* Patchable ndp-related variables. */
short	ndpCW = 0x0272;	/* NDP Control Word at start of each NDP process. */
short	ndpDump = 0;	/* Patch to 1 for NDP register dump on FP exceptions. */
short	ndpType = NDP_TYPE_UNPATCHED;	/* Patch overrides NDP type sensing. */
int	ndpEmSig = SIGFPE;	/* signal sent on receiving emulator traps */

/* Patchable emulator-related function pointers. */
int	(*ndpEmFn)() = 0;
int	(*ndpKfsave)() = 0;
int	(*ndpKfrstor)() = 0;

static int	kerEm = 1;	/* RAM copy of CR0 EM bit */
static int	ndpUseg;	/* system global address of U segment */
static PROC *	ndpOwner;	/* process whose stuff is now in ndp */

/*
 * ----------------------------------------------------------------------
 * Code.
 */

/*
 * Called from trap handler the first time a process executes an ndp
 * instruction.
 */
void
ndpNewOwner()
{
	UPROC *		up;

	/* disable further emulator traps for this process */
	wrNdpUser(1);
	ndpEmTraps(0);

	/* save old ndp status, if any process was using it */
	if (ndpOwner) {
		int work = workAlloc();
		ptable1_v[work] = sysmem.u.pbase[btocrd(ndpUseg)] | SEG_RW;
		mmuupd();
		up = (UPROC *)(ctob(work) + U_OFFSET);
		ndpSave(&up->u_ndpCon);
		wrNdpSavedU(1, up);
		workFree(work);
	}

	/* Make current process NDP owner */
	ndpMine();

	/* give process a clean ndp */
	ndpInit(ndpCW);
}

/*
 * NDP initialization for a new process.
 * Called at exec time.
 * Sets defaults, before it is known whether the process uses NDP or not.
 */
void
ndpNewProc()
{
	/* default for a process is to trap on NDP instructions */
	ndpEmTraps(1);
	wrNdpUser(0);
	wrNdpSaved(0);
	wrEmTrapped(0);
}

/*
 * Restore some ndp info when doing a regular conrest().
 * Called just after conrest - u area has just been restored.
 */
void
ndpConRest()
{
	UPROC *		up;

	/* make CR0 EM bit match what this process needs */
	ndpEmTraps(rdNdpUser() ? 0 : 1);

	/*
	 * If current process uses ndp, may need to fix ndp state
	 *
	 * By the nature of NDP save op's, if the NDP owner's NDP state
	 * is saved, then it's not in the NDP.
	 *
	 * So, we have to be careful (1) not to save twice, and (2) to
	 * restore, even if we are NDP owner, if NDP state is saved.
	 */
	if (rdNdpUser()) {
		if (ndpOwner != SELF) {
			if (ndpOwner) {		/* save old ndp state */
				int work = workAlloc();
				ptable1_v[work] =
				  sysmem.u.pbase[btocrd(ndpUseg)] | SEG_RW;
				mmuupd();
				up = (UPROC *)(ctob(work) + U_OFFSET);
				if (!rdNdpSavedU(up)) {
					ndpSave(&up->u_ndpCon);
					wrNdpSavedU(1, up);
				}
				workFree(work);
			}

			/* Make current process NDP owner and reload ndp state */
			ndpMine();
			ndpRestore(&u.u_ndpCon);
			wrNdpSaved(0);
		} else if (rdNdpSaved()) {
			ndpRestore(&u.u_ndpCon);
			wrNdpSaved(0);
		}
	}
}

/*
 * When a process exits, it relinquishes the ndp.
 */
void
ndpEndProc()
{
	if (SELF == ndpOwner)
		ndpDetach();
}

/*
 * ----------------------------------------------------------------------
 * Trap handlers.
 */

/*
 * fptrap()
 *
 * Entered when NDP generates a CPU error.
 * err is either SIFP or 0x0D40
 */
#define RDUMP() { \
  printf("\neax=%x  ebx=%x  ecx=%x  edx=%x\n", eax, ebx, ecx, edx); \
  printf("esi=%x  edi=%x  ebp=%x  esp=%x\n", esi, edi, ebp, esp); \
  printf("cs=%x  ds=%x  es=%x  ss=%x  fs=%x  gs=%x\n", \
    cs&0xffff, ds&0xffff, es&0xffff, ss&0xffff, fs&0xffff, gs&0xffff); \
  printf("err #%d eip=%x  uesp=%x  cmd=%s\n", err, eip, uesp, u.u_comm); \
  printf("efl=%x  ", efl); }

void
fptrap(gs, fs, es, ds, edi, esi, ebp, esp, ebx, edx, ecx, eax, trapno, err,
  eip, cs, efl, uesp, ss)
char *eip;
{
	unsigned short	sw;		/* ndp status word */
	struct _fpstate * fsp = &u.u_ndpCon;

	if (err == SIFP)
		u.u_regl = &gs;	/* hook in register set for consave/conrest */

	/*
	 * Send user a signal.
	 */
	ndpSave(fsp);
	/* Clear exception flag in NDP to prevent runaway trap. */
	sw = fsp->status = fsp->sw;
	fsp->sw &= 0x7f00;
	wrNdpSaved(1);
	if (ndpDump) {
		RDUMP();
		printf("\nfcs=%x  fip=%x  fos=%x  foo=%x\n",
		  fsp->cssel&0xffff, fsp->ipoff,
		  fsp->datasel&0xffff, fsp->dataoff);
		printf("User Floating Point Trap: ");
		if (sw & 1)
			printf("Invalid Operation");
		else if (sw & 2)
			printf("Denormalized Operand");
		else if (sw & 4)
			printf("Divide by Zero");
		else if (sw & 8)
			printf("Overflow");
		else if (sw & 0x10)
			printf("Underflow");
		else if (sw & 0x20)
			printf("Precision");
		else
			printf("???");
	}
	sendsig(SIGFPE, SELF);
}

/*
 * emtrap()
 *
 * Entered when NDP opcode is executed and EM bit of CR0 is 1.
 * err is SIXNP (Device Not Available Fault)
 */
void
emtrap(gs, fs, es, ds, edi, esi, ebp, esp, ebx, edx, ecx, eax, trapno, err,
  eip, cs, efl, uesp, ss)
char *eip;
{
	switch (ndpType) {
	case NDP_TYPE_287:
	case NDP_TYPE_387:
	case NDP_TYPE_486:
		ndpNewOwner();
		break;
	default:
		if (ndpDump) {
			RDUMP();
		}
		if (!rdEmTrapped()) {
			wrEmTrapped(1);
			emFinit(&u.u_ndpCon);
		}
		if (ndpEmFn) {
			int looker = 1;

			/*
			 * No emulator lookahead if ptraced or
			 * single step process.
			 */
			if ((SELF->p_flags & PFTRAC) || (u.u_regl[EFL] & MFTTB))
				looker = 0;
			(*ndpEmFn)(&gs, &u.u_ndpCon, looker);
		} else
			sendsig(ndpEmSig, SELF);
	}
}

/*
 * IRQ 13 handler.  Not used with 486.
 */
void
ndpIrq()
{
	struct _fpstate * fsp = &u.u_ndpCon;
	unsigned short sw;

	outb(NDP_PORT, 0);
	/*
	 * Send user a signal.
	 */
	ndpSave(fsp);
	/* Clear exception flag in NDP to prevent runaway trap. */
	sw = fsp->status = fsp->sw;
	fsp->sw &= 0x7f00;
	wrNdpSaved(1);
	if (ndpDump) {
		printf("\nfcs=%x  fip=%x  fos=%x  foo=%x\n",
		  fsp->cssel&0xffff, fsp->ipoff,
		  fsp->datasel&0xffff, fsp->dataoff);
		printf("User 387 Trap: ");
		if (sw & 1)
			printf("Invalid Operation");
		else if (sw & 2)
			printf("Denormalized Operand");
		else if (sw & 4)
			printf("Divide by Zero");
		else if (sw & 8)
			printf("Overflow");
		else if (sw & 0x10)
			printf("Underflow");
		else if (sw & 0x20)
			printf("Precision");
		else
			printf("???");
	}
	sendsig(SIGFPE, SELF);
}

/*
 * ----------------------------------------------------------------------
 * Routines concerned with whether current process has used the ndp.
 */
int
rdNdpUser()
{
	return (u.u_ndpFlags & NF_NDP_USER) ? 1 : 0;
}

void
wrNdpUser(n)
int n;
{
	if (n)
		u.u_ndpFlags |= NF_NDP_USER;
	else
		u.u_ndpFlags &= ~NF_NDP_USER;
}

/*
 * Since saving NDP state is destructive, we need to keep track
 * of where the current NDP state is - u area, or NDP?
 */
int
rdNdpSaved()
{
	return (u.u_ndpFlags & NF_NDP_SAVED) ? 1 : 0;
}

int
rdNdpSavedU(up)
UPROC * up;
{
	return (up->u_ndpFlags & NF_NDP_SAVED) ? 1 : 0;
}

void
wrNdpSaved(n)
int n;
{
	if (n)
		u.u_ndpFlags |= NF_NDP_SAVED;
	else
		u.u_ndpFlags &= ~NF_NDP_SAVED;
}

void
wrNdpSavedU(n, up)
int n;
UPROC * up;
{
	if (n)
		up->u_ndpFlags |= NF_NDP_SAVED;
	else
		up->u_ndpFlags &= ~NF_NDP_SAVED;
}

/*
 * Enable (1) or disable (0) emulator traps.
 */
void
ndpEmTraps(n)
int n;
{
	if (kerEm != n) {
		kerEm = n;
		setEm(n);
	}
}

/*
 * Make ndp owned by no one.
 */
void
ndpDetach()
{
	ndpOwner = 0;
	ndpUseg = 0;
}

/*
 * Make ndp owned by the current process.
 */
void
ndpMine()
{
	SR *		srp = &(u.u_segl[SIUSERP]);
	SEG *		sp = srp->sr_segp;

	ndpOwner = SELF;
	ndpUseg = MAPIO(sp->s_vmem, U_OFFSET);
}

/*
 * ----------------------------------------------------------------------
 * Code concerned with identifying coprocessor type, and taking specialized
 * action depending on the type.
 */

/*
 * Using usual algorithms, determine existence and type of NDP.
 * If interrupt vector needs to be set for FP exception, do it.
 *
 * If 2's bit of int11 is on, NDP is present.
 */
void
senseNdp()
{
	if (ndpType == NDP_TYPE_UNPATCHED) {
		ndpEmTraps(0);		/* Will need to do some FP code. */
		ndpType = ndpSense();	/* Rely on assembler tricks now. */
		ndpEmTraps(1);
	}
	if (ndpType == NDP_TYPE_387 || ndpType == NDP_TYPE_287) {
		setivec(NDP_IRQ, ndpIrq);
	}
}

/*
 * Called from main().
 * Return name string for the type of coprocessor detected.
 */
char *
ndpTypeName()
{
	char * ret = "**ERROR: Bad ndp type**";

	switch(ndpType) {
	case NDP_TYPE_NONE:
		ret = "No NDP.  ";
		break;
	case NDP_TYPE_287:
		ret = "NDP=287.  ";
		break;
	case NDP_TYPE_387:
		ret = "NDP=387.  ";
		break;
	case NDP_TYPE_486:
		ret = "NDP=486.  ";
		break;
	}
	return ret;
}

/*
 * ----------------------------------------------------------------------
 * Little routines for tracking emulator state.
 */

int
rdEmTrapped()
{
	return (u.u_ndpFlags & NF_EM_TRAPPED) ? 1 : 0;
}

void
wrEmTrapped(n)
int n;
{
	if (n)
		u.u_ndpFlags |= NF_EM_TRAPPED;
	else
		u.u_ndpFlags &= ~NF_EM_TRAPPED;
}

/*
 * Provide the emulator with a fresh context.
 */
void
emFinit(fpsp)
struct _fpemstate * fpsp;
{
	register int r;

	memset(fpsp, '\0', sizeof(struct _fpemstate));	/* mostly zeroes */
	fpsp->cw = ndpCW;
	for(r = 0; r < 8; r++)
		fpsp->regs[r].tag = 7;		/* Empty */
}

/*
 * ----------------------------------------------------------------------
 * Functions to interface with the emulator.
 */
get_fs_byte(cp)
char *cp;
{
	char getubd();

	return getubd(cp);
}

get_fs_word(sp)
short *sp;
{
	short getusd();

	return getusd(sp);
}

get_fs_long(lp)
long *lp;
{
	long getuwd();

	return getuwd(lp);
}

void
put_fs_byte(data, cp)
char *cp;
char data;
{
	putubd(cp, data);
}

void
put_fs_word(data, sp)
short *sp;
short data;
{
	putusd(sp, data);
}

void
put_fs_long(data, lp)
long *lp;
long data;
{
	putuwd(lp, data);
}

/*
 * Return zero if out of bounds for write.
 */
int
verify_area(cp, len)
int * cp;
int len;
{
	int ret = useracc(cp, len, 1);

	if (!ret) {
#if 0
		printf("Bad Em write, base=%x, len=%x, r.a.=%x",
			cp, len, *(int *)((&cp) - 1));
#endif
		sendsig(SIGSEGV, SELF);
	}
	return ret;
}

/*
 * print kernel message.
 */
printk(s)
char *s;
{
	puts(s);
}

emSendsig()
{
	sendsig(SIGFPE, SELF);
}
