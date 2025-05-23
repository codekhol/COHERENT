/* globals.c: these are our global variable, structs, file pointers... */

#include <stdio.h>
#include <curses.h>
#include <fcntl.h>

#define PORTFILE	"/usr/lib/uucp/port"
#define SYSFILE		"/usr/lib/uucp/sys"
#define DIALFILE	"/usr/lib/uucp/dial"
#define UUEDIT 		"/usr/lib/uucp/editfile"
#define UUTMP 		"/usr/lib/uucp/tmpfile"

#define MAXENTRIES 100

/* screen positions, opening screen */
#define UNAMELABEL	20
#define SYSLINE		7
#define PORTLINE	8
#define DIALLINE	9

extern int  startpos[];		/* corresponding start line in file */
extern int  total_entries_found;/* total number opf ports read from file */
extern short dialflag;		/* flag to indicate that we're working w/dial */
extern short sysflag;		/* flag to indicate that we're working w/sys */
extern short portflag;		/* flag to indicate that we're working w/port */
extern char action;		/* action to be performed on a file entry. */
extern char litestring[];	/* used to highlight and select a name. */

/* selection window coordinates used to highlight and select entries */
extern int newrow, prevrow, newcol, prevcol;

extern WINDOW *selwin;		/* window of existing entries */
extern WINDOW *promptwin;	/* window to print prompts in */
extern WINDOW *menwin;		/* window of menu options */
extern WINDOW *portwin;		/* view/create/edit port file entries */

/* character arrays to hold port info when enterin port data */

extern char portname[];
extern char porttype[];
extern char portdev[];
extern char portspeed[];
extern char portdial[];


/* character arrays to hold dial info when entring dialer data */

extern char dialname[];
extern char dialchat[];
extern char dialtout[];
extern char dialfail1[];
extern char dialfail2[];
extern char dialfail3[];
extern char dialplete[];
extern char dialabort[];

/* character arrays to hold sys info when entering data */

extern char sysname[];
extern char systime[];
extern char sysspeed[];
extern char sysport[];
extern char sysphone[];
extern char syschat[];
extern char sysmyname[];
extern char sysprot[];
extern char syscmds[];
extern char sysread[];
extern char syswrite[];

extern char * get_data();


/* color related stuff */

extern short use_colors;		/* flag to use colors */

extern int mainfore;			/* main menu colors */
extern int mainback;
extern int actfore;			/* action menu colors */
extern int actback;
extern int selfore;			/* select screen colors */
extern int selback;
extern int helpfore;			/* help message colors */
extern int helpback;
