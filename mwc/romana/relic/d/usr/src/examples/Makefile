Example: Example.o
	cc Example.o
abs: abs.o
	cc abs.o
access: access.o
	cc access.o
acos: acos.o
	cc -f acos.o -lm
argv: argv.o
	cc argv.o
array: array.o
	cc array.o
asctime: asctime.o
	cc asctime.o
atoi: atoi.o
	cc atoi.o
atol: atol.o
	cc atol.o
buffer: buffer.o
	cc buffer.o
byteordering: byteordering.o
	cc byteordering.o
calloc: calloc.o
	cc calloc.o
ceil: ceil.o
	cc -f ceil.o -lm
cosh: cosh.o
	cc -f cosh.o -lm
ctime: ctime.o
	cc ctime.o
ctype: ctype.o
	cc ctype.o
curses: curses.o
	cc curses.o -lcurses -lterm
__DATE__: __DATE__.o
	cc __DATE__.o
envp: envp.o
	cc envp.o
execve: execve.o
	cc execve.o
exp: exp.o
	cc -f exp.o -lm
fdopen: fdopen.o
	cc fdopen.o
ferror: ferror.o
	cc ferror.o
fgetc: fgetc.o
	cc fgetc.o
fgets: fgets.o
	cc fgets.o
fileno: fileno.o
	cc fileno.o
fopen: fopen.o
	cc fopen.o
fputc: fputc.o
	cc fputc.o
freopen: freopen.o
	cc freopen.o
frexp: frexp.o
	cc -f frexp.o -lm
fscanf: fscanf.o
	cc fscanf.o
fseek: fseek.o
 	cc fseek.o
getc: getc.o 
	cc getc.o
getenv: getenv.o
 	cc getenv.o
gets: gets.o
	cc gets.o
goto: goto.o
	cc goto.o
j0: j0.o
	cc -f j0.o -lm
localtime: localtime.o
	cc localtime.o
malloc: malloc.o
	cc malloc.o
modf: modf.o
	cc -f modf.o
open: open.o
 	cc open.o
preproc: preproc.o
	cc -f preproc.o -lm
modulus: modulus.o
	cc modulus.o
printf: printf.o
	cc -f printf.o
putc: putc.o
	cc putc.o
rand: rand.o
	cc rand.o
rindex: rindex.o
 	cc rindex.o
scanf: scanf.o
	cc scanf.o
sizeof: sizeof.o
	cc sizeof.o
sscanf: sscanf.o
	cc sscanf.o
stringfun: stringfun.o
	cc stringfun.o
strncpy: strncpy.o
	cc strncpy.o
swab: swab.o
	cc swab.o
system: system.o
	cc system.o
tolower: tolower.o
	cc tolower.o
unlink: unlink.o
	cc unlink.o

all:	curses       buffer	      exp	     getc	    putc\
	Example      byteordering     fdopen	     getenv	    rand\
	__DATE__     calloc	      ferror	     gets	    rindex\
	abs	     ceil	      fgetc	     goto	    scanf\
	access       cosh	      fgets	     j0	            sizeof\
	acos	     ctime	      fileno	     localtime      sscanf\
	all	     ctype	      fopen	     malloc	    stringfun\
	argv	     		      fputc	     modf	    strncpy\
	array	     freopen	      modulus	     swab	    asctime\
        frexp	     open	      system	     atoi	    envp\
        fscanf	     preproc	      tolower	     atol	    execve\
        fseek	     printf	      unlink 	     
				>all_done

clean:	
	rm   -f      buffer	      exp	     getc	    putc\
	Example      byteordering     fdopen	     getenv	    rand\
	__DATE__     calloc	      ferror	     gets	    rindex\
	abs	     ceil	      fgetc	     goto	    scanf\
	access       cosh	      fgets	     j0	            sizeof\
	acos	     ctime	      fileno	     localtime      sscanf\
	all	     ctype	      fopen	     malloc	    stringfun\
	argv	     		      fputc	     modf	    strncpy\
	array	     freopen	      modulus	     swab	    asctime\
        frexp	     open	      system	     atoi	    envp\
        fscanf	     preproc	      tolower	     atol	    execve\
        fseek	     printf	      unlink 	     *.o	    tmpfile\
	all_done     curses
			echo all clean
