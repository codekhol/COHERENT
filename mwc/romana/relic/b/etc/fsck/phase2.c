/*
 *	phase 2 of fsck - Check Pathnames
 */

#include "fsck.h"
#include <pwd.h>

#define MAXDEPTH 50
struct	direct *path[MAXDEPTH];
int 	depth;
int 	fixflag;			/* flags when must rewrite databuf  */
daddr_t blocknum;			/* Current block num read in        */

typedef struct list {
	struct direct d_entry;
	struct list *next;
} list;

char	buf2[BSIZE];			/* buffer for blocks		*/
daddr_t	cdbn;				/* current logical block number */

/*	Format errors, args to blkerr() */

#define	NULLNAME	5
#define	NULLPAD		6
#define SLASHES		7
#define DOT		8
#define DOTDOT		9

phase2()
{
	if (!qflag)
		printf("Phase 2 : Check Pathnames\n");
	checkroot();
	depth = 0;
	checkpath(ROOTIN);
}


checkroot()
{
	if ( (flags(ROOTIN)&MODEMASK) == UNALLOC )
		fatal("Root i-node is unallocated.  Terminating");

	if ( (flags(ROOTIN)&MODEMASK) != IDIR ) {
		switch ( query("Root i-node is not a directory (FIX)") ){
		case YES:
			fixroot();
			break;
		case NO:
			abort();
		}
	}
	
	if ( badblks(ROOTIN) ){
		switch ( query("Dup/Bad blocks in root i-node (Continue)") ) {
		case YES:
			break;
		case NO:
			abort();
		}
	}
	linkincr(ROOTIN);		/* (?) Feature (?) of mkfs is  */
					/* that root inode of a file   */
					/* system has a link count one */
					/* too large.		       */
}

fixroot()
{
	register struct dinode *dip;

	if (!writeflg) 
		fatal("File System Read-Only (NO WRITE)\n");

	setflags(ROOTIN, (flags(ROOTIN) & ~MODEMASK) | IDIR);

	/* Save change to disk */
	
	dip = ptrino(ROOTIN, databuf);
	dip->di_mode &= (~IFMT);
	dip->di_mode |= IFDIR;
	writeino(ROOTIN, databuf);
}

#define	myinum	( (depth>0) ? path[depth-1]->d_ino : ROOTIN )
#define popinum	( (depth>1) ? path[depth-2]->d_ino : ROOTIN )

char *memory = "Can't malloc memory, phase 2";

list *
procfiles(addrs, length)
daddr_t *addrs;
fsize_t length;
{
	register struct direct *elemnt;
	register list *first, *ptr;

	if ((first=ptr=(list *)malloc(sizeof(list)))==NULL)
		fatal(memory);
	first->d_entry.d_ino = 0;
	first->next = NULL;
	if ( length < DSIZE )
		return( first );
	cdbn = 0;
	nextblock(addrs);
	fixflag = FALSE;
	elemnt = (struct direct *) databuf;
	cdots(elemnt++, ".", myinum, DOT);
	++numfiles;
	if ( (length-=DSIZE) < DSIZE )
		return( first );
	cdots(elemnt++, "..", popinum, DOTDOT);
	++numfiles;
	length -= DSIZE;

	while ( length >= DSIZE ) {
		while ( (elemnt<&databuf[BSIZE]) && (length>=DSIZE) ) {
			if (chck(elemnt)==IDIR) {
				copy(elemnt, &ptr->d_entry, 
						sizeof(struct direct));
				if ((ptr->next=(list *)malloc(sizeof(list))) ==
					NULL)
					fatal(memory);
				ptr = ptr->next;
				ptr->d_entry.d_ino = 0;
				ptr->next = NULL;
			}
			elemnt++;
			length -= DSIZE;
		}
		if (fixflag && (fixblock(addrs) == NUL) )
			fixblkerr();
		nextblock(addrs);
		fixflag = FALSE;
		elemnt = (struct direct *) databuf;
	}
	return(first);	 
}

candblock(dptr)
register struct direct *dptr;
{
	register int num, i;

	num = BSIZE/sizeof(struct direct);
	for (i=0; i<num; i++)
		canino(dptr[i].d_ino);
}

nextblock(addrs)
register daddr_t *addrs;
{
	register daddr_t bn;

	if ( (bn=imap(addrs, cdbn++)) == 0 ) {
		bclear(databuf, BSIZE);
		return;
	}
	bread(bn, databuf);
	candblock(databuf);
	blocknum = bn;
}

fixblock(addrs)
daddr_t *addrs;
{
	daddr_t bn;

	if ( (bn=imap(addrs, cdbn-1)) == 0 ) 
		return(NUL);
	candblock(databuf);
	bwrite(bn, databuf);
	return(TRUE);
}

fixblkerr()
{
	fatal("Fixblock error.");
}

copy(from, to, size)
register char *from, *to;
register int size;
{
	while (size--)
		*to++=*from++;
}

cdots(elemnt, dots, ino, type)
register struct direct *elemnt;
char *dots;
ino_t ino;	/* what the inode number should be */
int type;
{
	register char *name = elemnt->d_name;
	register ino_t inum;

	inum = elemnt->d_ino;
	if ( (strcmp(name, dots) != 0) || (inum != ino) ||
		(format(name) != GOOD) ) {
		blkerr(type);
		return;
	}
	linkincr(inum);
}

format(name)
register char *name;
{
	register char *ptr=name;
	register char *end=&name[DIRSIZ];

	while ( (ptr<end) && (*ptr++ != '\0') ) ;

	if ( (ptr-1) == name )
		return(NULLNAME);
	
	if (ptr<end) {
		while ( (ptr<end) && (*ptr++ == '\0') ) ;
		if (ptr!=end)
			return(NULLPAD);
	}

	ptr = name;
	while (ptr < end) 
		if (*ptr++ == '/')
			return(SLASHES);

	return(GOOD);
}

	
blkerr(type)
int type;
{	
	char *errname;

	switch(type) {
	case NULLNAME:
		errname = "Null name";
		break;
	case NULLPAD:
		errname = "Non null padded";
		break;
	case SLASHES:
		errname = "Embedded slashes in";
		break;
	case DOT:
		errname = "Inconsistent .";
		break;
	case DOTDOT:
		errname = "Inconsistent ..";
		break;
	default:
		errname = "Bad";
		break;
	}

	printf("%s entry in block %lu in directory\n", errname, blocknum);
	pinfo(myinum);
	pname(path[--depth]->d_name);
	depth++;
}

chck(elemnt)
register struct direct *elemnt;
{
	register ino_t inum;
	register char *name;
	int type;

	inum = elemnt->d_ino;
	if ( inum == 0 )
		return(UNALLOC);

	name = elemnt->d_name;
	if ( (type=format(name)) != GOOD )
		blkerr(type);

	if (inum>ninodes) {
		if ( irange(inum, name) ) 
			zeroent(elemnt);
		return(UNALLOC);
	}

	if ( (flags(inum)&ALLOCMASK)==UNALLOC ) {
		if ( unalloc(inum, name) )
			zeroent(elemnt);
		return(UNALLOC);
	}

	if ( badblks(inum) ) {
		if ( baddup(inum, name) ) {
			zeroent(elemnt);
			return(UNALLOC);
		}
		if ( (flags(inum)&MODEMASK)==IDIR ) {
			linkincr(inum);
			numfiles++;
			return(UNALLOC);	/* Don't traverse this baby */
		}
	}

	linkincr(inum);
	numfiles++;
	return( flags(inum)&MODEMASK );
}

zeroent(elemnt)
struct direct *elemnt;
{
	elemnt->d_ino = 0;
	fixflag = TRUE;
}

pname(name)
char *name;
{
	int i=0;

	while (i<depth) 
		prdirsize(path[i++]->d_name);
	prdirsize(name);
	putchar('\n');
}

prdirsize(name)
char *name;
{
	if (name[DIRSIZ-1] == '\0')
		printf("/%s", name);
	else
		printf("/%*s", DIRSIZ, name);
}

fsize_t
pinfo(inum)
ino_t inum;
{
	register struct dinode *dip;
	register struct passwd *pwd;

	if ( (dip=ptrino(inum, buf2)) == NULL ) {
		printf("i-number = %u is in a bad inode block.\n", inum);
		return(0);
	}

	printf("i-number = %u, ", inum);
	if ( (pwd=getpwuid(dip->di_uid)) != NULL )
		printf("Owner=%s, ", pwd->pw_name);
	else
		printf("Owner=%u, ", dip->di_uid);
	printf(" Mode=0%o\n", dip->di_mode);
	printf("Size=%lu, Mtime=%s", dip->di_size, ctime(&dip->di_mtime));
	return(dip->di_size);
}

char *remove = "(Remove)";

irange(inum, name)
ino_t inum;
char *name;
{
	printf("I-number is out of range  I=%u\n", inum);
	pname(name);
	return(action(remove));
}

unalloc(inum, name)
ino_t inum;
char *name;
{	
	fsize_t size;

	printf("Unallocated\n");
	size = pinfo(inum);
	pname(name);
	if ( (daction != NO) && (size == 0) && (mounted == FALSE) ) {
		printf("%s [Forced - Yes]\n", remove);
		return(YES);
	}
	return(action(remove));
}

baddup(inum, name)
ino_t inum;
char *name;
{
	register int mode = flags(inum)&MODEMASK;

	printf("Bad or Dup blocks in %s\n", typename(inum));
	pinfo(inum);
	pname(name);
	return(action(remove));
}

checkpath(ino)
register ino_t ino;
{
	static daddr_t	addrs[NADDR];
	static struct dinode *dip;
	static list *temp;
	register list *ptr;

	if ( (flags(ino)&VISITED)==VISITED ) {
		circle(ino);
		return;
	} else
		orflags(ino, VISITED);

	dip = ptrino(ino, databuf);
	l3tol(addrs, dip->di_addr, NADDR);
	
	if ( (dip->di_mode&IFMT) != IFDIR )
		fatal("Tried to checkpath i-node %u which is not dir.\n", ino);

	ptr = procfiles(addrs, dip->di_size);

	while (ptr->d_entry.d_ino) {
		if (depth >= MAXDEPTH)
			toolong();
		path[depth++] = &ptr->d_entry;
		checkpath(ptr->d_entry.d_ino);
		depth--;
		temp = ptr;
		ptr = ptr->next;
		free(temp);
	}
	free(ptr);
}

circle(ino)
ino_t ino;
{
	printf("I-node %u is a multiply referenced directory i-node.\n", ino);
}

toolong()
{
	printf("Name too long.\n");
	pname("\0");
	abort();
}

	
