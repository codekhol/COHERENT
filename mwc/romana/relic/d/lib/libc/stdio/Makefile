# /usr/src/libc/stdio/Makefile 2/14/91

# The order of stdio objects in libc.a is significant and a bit complicated.
# To reduce the size of linked objects when possible,
# libc includes both real and fake versions of _dtefg() (floating point output)
# and of _finish() (close open FILEs).
# If a program requires floating point output, the "-f" flag to /bin/cc
# produces an "-u _dtefg_" argument to /bin/ld, forcing the link to include
# the real floating point output routine from _dtefg.o.
# Otherwise, /bin/ld resolves the reference to _dtefg() in printf.o with
# the much smaller fake floating point output routine from _fdtefg.o.
# To avoid linking in extra stdio overhead when not required,
# libc.a should contain _fp.o, finit.o, exit.o and _finish.o, in that order,
# after stdio objects from $(OBJ) and from the machine-dependent subdirectory,
# If a program uses stdio, /bin/ld will link in finit.o;
# finit.o defines the real _finish() which closes all open files for exit().
# If a program does not use stdio, /bin/ld will link in exit.o but not finit.o;
# then it resolves the _finish() reference in exit() with the fake _finish.o,
# which does nothing.

OBJ=\
	_fgetb.o\
	_fgetc.o\
	_fgeteof.o\
	_fopen.o\
	_fpseek.o\
	_fputb.o\
	_fputc.o\
	_fputt.o\
	_stropen.o\
	fclose.o\
	fdopen.o\
	fflush.o\
	fgetc.o\
	fgets.o\
	fopen.o\
	fputc.o\
	fputs.o\
	fread.o\
	freopen.o\
	fseek.o\
	ftell.o\
	fwrite.o\
	getc.o\
	getchar.o\
	gets.o\
	popen.o\
	putc.o\
	putchar.o\
	puts.o\
	rewind.o\
	scanf.o\
	setbuf.o\
	ungetc.o

# N.B. The order of objects in $(OBJ2) is significant,
# see the lengthy comment above and be extremely careful if you change it.
OBJ2=\
	_dtefg.o\
	printf.o\
	_fdtefg.o\
	_fp.o\
	finit.o\
	exit.o\
	_finish.o

# Primary target.
all:	stdioi8086 STDIO
	: libc/stdio done.

# Pseudotargets.
stdioi8086:
	make SRCPATH=$(SRCPATH)/i8086
# This assumes that either libc.a is being made from scratch
# or the names of the objects in $(OBJ2) are unchanged.
# Otherwise, "ar rkus lib.c $?" may not preserve the required object order.
# It could be done more correctly with ar positional specifications.
STDIO:	$(OBJ) $(OBJ2)
	ar rkus libc.a $?
	touch $@

# Secondary targets.
_dtefg.o:	_dtefg.c
_fdtefg.o:	_fdtefg.c
_fgetb.o:	_fgetb.c
_fgetc.o:	_fgetc.c
_fgeteof.o:	_fgeteof.c
_finish.o:	_finish.c
_fopen.o:	_fopen.c
_fp.o:		_fp.c
_fpseek.o:	_fpseek.c
_fputb.o:	_fputb.c
_fputc.o:	_fputc.c
_fputt.o:	_fputt.c
_stropen.o:	_stropen.c
exit.o:		exit.c
fclose.o:	fclose.c
fdopen.o:	fdopen.c
fflush.o:	fflush.c
fgetc.o:	fgetc.c
fgets.o:	fgets.c
finit.o:	finit.c
fopen.o:	fopen.c
fputc.o:	fputc.c
fputs.o:	fputs.c
fread.o:	fread.c
freopen.o:	freopen.c
fseek.o:	fseek.c
ftell.o:	ftell.c
fwrite.o:	fwrite.c
getc.o:		getc.c
getchar.o:	getchar.c
gets.o:		gets.c
popen.o:	popen.c
printf.o:	printf.c
putc.o:		putc.c
putchar.o:	putchar.c
puts.o:		puts.c
rewind.o:	rewind.c
scanf.o:	scanf.c
setbuf.o:	setbuf.c
ungetc.o:	ungetc.c

# end of /usr/src/libc/stdio/Makefile
