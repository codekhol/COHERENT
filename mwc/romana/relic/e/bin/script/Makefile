# Makefile for Berkeley type script command
DESTDIR=/bin386/usr/bin/
script:		recorder.o server.o tty_mode.o tty_raw.o getpseudotty.o
	cc -o $(DESTDIR)script $<
	strip $(DESTDIR)script

getpseudotty.o:	getpseudotty.c

recorder.o:	recorder.c

server.o:	server.c

tty_mode:	tty_mode.c

tty_raw.o:	tty_raw.c
