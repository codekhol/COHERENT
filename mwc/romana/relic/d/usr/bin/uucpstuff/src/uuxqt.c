/*
 *  uuxqt.c
 *
 *  Execute local commands spooled by remote sites.
 *
 *  copyright (x) richard h. lamb 1985, 1986, 1987
 *  changes (massive) copyright (c) 1989-1991 by Mark Williams Company
 */

#include <stdio.h>
#include <signal.h>
#include <access.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "dirent.h"
#include "dcp.h"
#include "perm.h"

/*
 *  Global Variables Definitions
 */

char	directory[CTLFLEN];		/* directory for control files	*/
char	xfile[CTLFLEN];			/* "X.*" control file name	*/
FILE	*xfp = NULL;			/* Opened "X.*" FILE pointer	*/
int	processid;			/* process id of this uuxqt	*/
char	line[BUFSIZ];			/* Reading a text line		*/

static	char	command[BUFSIZ], input[60], output[60];
static	char	orig_user[128];
static	char	orig_system[SITELEN+1];
static	char	notifywho[BUFSIZ];
static	char	*allowed;

static	int	failstatus_req;
static	int	succstatus_req;
static	char	errbuf1[BUFSIZ];
static	char	errbuf2[BUFSIZ];
static	char	errbuf3[BUFSIZ];
static	char	reason[80];

static	char	*sep = " \t\n";

char	*rmtname = NULL;
char	**zenvp;			/* Globally Available envp	*/

extern	int optind;
extern	int optopt;
extern	char *optarg;

/*
 *  Extern Function Declarations
 */

extern	char	*strtok();
extern	char	*index();

catchsegv()
{
	fatal("Segmentation violation -- uuxqt aborted");
}

catchterm()
{
	fatal("Local signal -- uuxqt aborted");
}

main(argc, argv, envp)
int argc;
char *argv[], *envp[];
{
	char xqtdir[LOGFLEN];
	char ch;

	umask(077);
	while ( (ch=getopt(argc, argv, "x:vV")) != EOF ) {
		switch (ch) {
		case 'x':
			debuglevel = atoi(optarg);
			break;
		case 'v':
		case 'V':
			fatal("uuxqt: Version %s", VERSION);
		case '?':
		default:
			fatal("Improper option usage: %c", optopt);
		}
	}

	bedaemon();	/* detach from controlling terminal */

	zenvp = envp;
	processid = getpid();
	signal(SIGINT,  SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, catchterm);
	signal(SIGSEGV, catchsegv);
	open_debug("uuxqt", 0);
	sprintf(xqtdir, "%s/.Xqtdir", SPOOLDIR);
	if (chdir(xqtdir) != 0)
		fatal("can't chdir to: %s", xqtdir);
	dcxqt();
	close_debug();
	exit(0);
}

dcxqt()
{
	if ( lockexist("uuxqt") )
		return;

	if ( lockit("uuxqt") < 0 )
		fatal("Can't lock uuxqt");

	if ( dscan_start() ) {
		while ( dscan() ) {
			if ( xscan_start() ) {
				while ( xscan() )
					dcxqt_work();
				xscan_done();
			}
		}
		dscan_done();
	}
	if ( lockrm("uuxqt") < 0 )
		printmsg(M_LOG, "error unlocking uuxqt");
}

/*
 * Perform the work specified in the "X.*" control file: "xfile"
 */

dcxqt_work()
{
	static	char lastsite[SITELEN] = "\0";
	int	filewait = 0;
	int	removethis = 0;
	int	did_work = 0;
	int	fnd;
	int	execval;
	char	*sp, *osp;

	if ( (xfp=fopen(xfile, "r")) == NULL )
		return;

	reason[0] = input[0] = output[0] = command[0] =
	notifywho[0] = orig_user[0] = orig_system[0] =
	errbuf1[0] = errbuf2[0] = errbuf3[0] = '\0';
	failstatus_req = succstatus_req = fnd = 0;

	while ( fgets(line, BUFSIZ, xfp) != NULL ) {
		sp = strtok(line, sep);
		switch(line[0]) {
		case 'C':
			sp = strtok(NULL, "#\n");
			strcpy(command, sp);
			break;
		case 'F':
			sp = strtok(NULL, sep);
			osp = strtok(NULL, sep);
			sprintf(input,"%s/%s", directory, sp);
			filewait |= isfileabsent(input);
			
			if (filewait == 0 && osp != NULL) {
				if (link(input, osp) == -1) {
					sprintf(errbuf1,
						"Cannot link %s to %s.\n",
						input, osp);
					removethis = 1;
				}
				ul(input);
				input[0] = '\0';
			}
			break;
		case 'I':
			sp = strtok(NULL, sep);
			sprintf(input, "%s/%s", directory, sp);
			break;
		case 'M':
			strcpy(errbuf2,
				"Execute M record not supported");
			break;
		case 'O':
			sp = strtok(NULL, sep);
			sprintf(output, "%s/%s", directory, sp);
			break;
		case 'R':
			strcpy (notifywho, sp = strtok(NULL, sep));
			break;
		case 'U':
			strcpy(orig_user, sp = strtok(NULL, sep));
			strncpy(orig_system,sp=strtok(NULL, sep), SITELEN);
			if ( strncmp(orig_system, lastsite, SITELEN) != 0) {
				strncpy(lastsite, orig_system, SITELEN);
				rmtname = &lastsite[0];
				open_the_logfile("uuxqt");
				plog(M_INFO, "Starting Xqt {%d} (V%s)",
							 processid, VERSION);
				perm_get(orig_system, NULL);
				allowed = perm_value(commands_e);
			}
			break;
		case 'Z':
			failstatus_req = 1;
			break;
		case 'n':
			succstatus_req = 1;
			break;
		case '#':
			break;
		default:
			sprintf(errbuf3, "Unknown command %s", sp);
			break;
		}
	}

	if (strlen(notifywho) < 1)
		strcpy (notifywho, orig_user);
	if (strlen(errbuf1) > 0)
		plog(M_INFO, errbuf1);
	if (strlen(errbuf2) > 0)
		plog(M_INFO, errbuf2);
	if (strlen(errbuf3) > 0)
		plog(M_INFO, errbuf3);

	plog(M_INFO, "%s (<%s >%s)", command, input, output);
	if (removethis) {
		unlinkfiles();
	} else if (filewait)
		plog(M_INFO, "Waiting for files");
	else {
		did_work ++;
		if ( (execval=shell2(command, input, output)) != 0 ) {
			plog(M_INFO, "Command failed, status %d (0x%04x)",
						execval, execval);
			strcpy(reason, "Exit status not zero");
		}
		if (succstatus_req || (failstatus_req && (execval != 0))) 
			remote_status(execval);
		unlinkfiles();
	}
	if (did_work > 0) {
		plog(M_INFO, "Finished {%d}", processid);
	}
	fclose(xfp);
}

unlinkfiles()
{
	char	*osp, *sp;

	rewind(xfp);
	while(fgets(line, BUFSIZ, xfp) != NULL) {
		sp = strtok(line, sep);
		switch(line[0]) {
		case 'C':
			break;
		case 'F':
			sp = strtok(NULL, sep);
			osp = strtok(NULL, sep);
			ul(sp);
			if (osp != NULL) 
				ul(osp);
			break;
		case 'I':case 'M':case 'O':
		case 'U':case 'Z':case 'n':
		default:
			break;
		
		}
	}
	ul(xfile);
	ul(input);
	ul(output);
}

static
ul(fn)
char	*fn;
{
	int	status;
	if (strlen(fn) < 1)
		return;
	status = unlink(fn);	
}

remote_status(val)
int val;
{
	static char pbuf[BUFSIZ];

	FILE	*fmp;
	(void) signal(SIGPIPE, SIG_IGN);
	sprintf(pbuf, "mail -auucp %s!%s ", orig_system, notifywho);
	if ((fmp = popen(pbuf, "w")) == NULL)
		plog(M_INFO, "Cannot send remote status mail");
	else {
		fprintf(fmp, "From: UUXQT V%s\n", VERSION);
		fprintf(fmp, "Subject: UUXQT remote execution status\n\n");
		fprintf(fmp,
			"Command \"%s\" %s.\n\tStatus %d (0x%04x)",
			command, val ? "failed" : "succeeded", val, val);
		if (strlen(reason) > 0)
			fprintf(fmp, "\nReason: %s", reason);
		fprintf(fmp, "\n");
		if (pclose(fmp) != 0)
			plog(M_INFO, "Remote status mail failed");
		plog(M_INFO, "Remote status mail posted to %s!%s",
			orig_system, notifywho);
	}
}

isfileabsent(fn)
char	*fn;
{
	return access(fn, AREAD);
}

#define	MAXENVS	200
static	char *uuenvp[MAXENVS];
static	char uu_user[BUFSIZ];
static	char uu_mach[BUFSIZ];

shell2(command, inname, outname)
char *command;
char *inname;
char *outname;
{
	int	waitstat;
	int	fd, i;
	int	waitpid, cpid;

	if ( !permission(command) ) {
		strcpy(reason, "No permission to execute command");
		plog(M_INFO, "No permission to execute: %s", command);
		if (failstatus_req)
			remote_status(-1);
		return 0;
	}

	sprintf(uu_user, "UU_USER=%s", notifywho);
	sprintf(uu_mach, "UU_MACHINE=%s", orig_system);
	uuenvp[0] = uu_user;
	uuenvp[1] = uu_mach;
	for (i=2; (zenvp[i]!=NULL) && (i<MAXENVS); i++)
		uuenvp[i] = zenvp[i-2];
		if ((cpid = fork()) < 0) {
		plog(M_INFO, "couldn't fork");
		return -1;
	}
	if (cpid == 0) {
		if (strlen(inname) != 0) {
			fd = open(inname, 0);
			dup2(fd, 0);
			if (fd > 0)
				close(fd);
		}
		if (strlen(outname) != 0) {
			fd = creat(outname, 0644);
			dup2(fd, 1);
			if (fd > 1)
				close(fd);
		}
		execle("/bin/sh", "sh", "-c", command, (char *)0, uuenvp);
		plog(M_INFO, "Could not exec /bin/sh");
		exit(0177);
	}
	while (((waitpid = wait(&waitstat)) != cpid) && (waitpid > 0))
		;
	return waitstat;
}

permission(command)
char *command;
{
	char *sp, *cp, *spcp;
	int ok;

	if ( (spcp=index(command, ' ')) != NULL ) 
		*spcp = '\0';

	sp = allowed;
	ok = 0;
	do {
		if ( (cp=index(sp, ':')) != NULL )
			*cp = '\0';
		if ( strcmp(command, sp) == 0 )
			ok = 1;
		if ( cp != NULL )
			*cp++ = ':';
	} while ( !ok && ((sp=cp) != NULL) );

	if ( spcp != NULL )
		*spcp = ' ';
	return(ok);
}

/*
 * Directory Scan Functions:
 *
 * dscan_start()  :  Opens SPOOLDIR to scan for sitename subdirectories
 *		  :  Returns: (1) opened ok, (0) error
 * dscan()	  :  Sets variable "directory" to next directory
 *		  :  Returns: (1) found directory, (0) no directory
 * dscan_done()	  :  Done scanning SPOOLDIR for directories
 *
 * xscan_start()  :  Opens "directory" to scan for "X.*" control files
 *		  :  Returns: (1) opened ok, (0) error
 * xscan()	  :  Sets variable "xfile" to next such "X.*" control file
 *		  :  Returns: (1) found file, (0) no file
 * xscan_done()	  :  Done scanning "directory" for "X.*" control files
 */

static	DIR *sdirp, *xdirp;
extern	DIR *opendir();
extern	struct direct *readdir();

dscan_start()
{
	if ( (sdirp=opendir(SPOOLDIR)) == NULL ) {
		printmsg(M_LOG, "Cannot open spool directory: %s", SPOOLDIR);
		return(0);
	}
	return(1);
}

dscan()
{
	struct stat statbuf;
	struct dirent *mdp;

	while( (mdp=readdir(sdirp)) != NULL ) {
		if ( mdp->d_name[0] == '.' )
			continue;
		sprintf(directory, "%s/%s", SPOOLDIR, mdp->d_name);
		stat(directory, &statbuf);
		if ( statbuf.st_mode & S_IFDIR )
			return(1);
	}
	return(0);
}

dscan_done()
{
	closedir(sdirp);
}

xscan_start()
{
	if ( (xdirp=opendir(directory)) == NULL ) {
		printmsg(M_LOG, "Cannot open spool directory: %s", directory);
		return(0);
	}
	return(1);
}

xscan()
{
	struct stat statbuf;
	struct dirent *xdp;

	while( (xdp=readdir(xdirp)) != NULL ) {
		if ( strncmp(xdp->d_name, "X.", 2) == 0 ) {
			sprintf(xfile, "%s/%s", directory, xdp->d_name);
			stat(xfile, &statbuf);
			if ( statbuf.st_mode & S_IFDIR )
				continue;
			return(1);
		}
	}
	return(0);
}

xscan_done()
{
	closedir(xdirp);
}

fatal(x)
{
	printmsg(M_FATAL, "%r", &x);
	if ( lockexist("uuxqt") )
		lockrm("uuxqt");
	close_logfile();
	close_debug();
	exit(1);
}	
