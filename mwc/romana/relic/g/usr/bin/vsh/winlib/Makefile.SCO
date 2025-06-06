#	Makefile for winlib (SCO UNIX Release 3.2 Version 2.0)

CFLAGS = -c -O

#       Additional flags which may be defined in OPTIONS:
#
#       -DINTRFLUSH     - if the curses library has the function intrflush()
#                         this flag should be activated
#
#       -DNOTYPEAHEAD   - if the curses library does typeahead optimization
#                         this flag should be set, because the library
#                         does it's own optimization of typeahead
#
#	-DAIX		- this flag is mandantory if compiling under AIX
#
#	-DSCO32		- this flag is mandantory if compiling under SCO 3.2
#
#	-DSYSV4		- this flag is mandantory if compiling under SV.4
#
#	-DOLDACS	- for curses without ACS_...

OPTIONS = -DINTRFLUSH -DNOTYPEAHEAD -DSCO32

OBJ = dialbox.o buttbox.o cmdbox.o listbox.o editbox.o fielded.o selbox.o \
      pdmenu.o scrollbar.o pfkeys.o \
      initwin.o openwin.o closall.o closwin.o refrall.o sh_esc.o sh_ret.o \
      nomem.o cursor.o clrscr.o

winlib.a: $(OBJ)
	ar vru winlib.a `lorder $(OBJ) | tsort`
#	ranlib winlib.a

dialbox.o: dialbox.c winfun.h
	cc $(CFLAGS) $(OPTIONS) dialbox.c

buttbox.o: buttbox.c winfun.h
	cc $(CFLAGS) $(OPTIONS) buttbox.c

cmdbox.o: cmdbox.c
	cc $(CFLAGS) $(OPTIONS) cmdbox.c

listbox.o: listbox.c winfun.h
	cc $(CFLAGS) $(OPTIONS) listbox.c

editbox.o: editbox.c
	cc $(CFLAGS) $(OPTIONS) editbox.c

fielded.o: fielded.c winfun.h
	cc $(CFLAGS) $(OPTIONS) fielded.c

selbox.o: selbox.c winfun.h
	cc $(CFLAGS) $(OPTIONS) selbox.c

pdmenu.o: pdmenu.c winfun.h
	cc $(CFLAGS) $(OPTIONS) pdmenu.c

scrollbar.o: scrollbar.c winfun.h
	cc $(CFLAGS) $(OPTIONS) scrollbar.c

pfkeys.o: pfkeys.c winfun.h
	cc $(CFLAGS) $(OPTIONS) pfkeys.c

initwin.o: initwin.c winfun.h
	cc $(CFLAGS) $(OPTIONS) initwin.c

openwin.o: openwin.c winfun.h
	cc $(CFLAGS) $(OPTIONS) openwin.c

closall.o: closall.c winfun.h
	cc $(CFLAGS) $(OPTIONS) closall.c

closwin.o: closwin.c winfun.h
	cc $(CFLAGS) $(OPTIONS) closwin.c

refrall.o: refrall.c winfun.h
	cc $(CFLAGS) $(OPTIONS) refrall.c

sh_esc.o: sh_esc.c 
	cc $(CFLAGS) $(OPTIONS) sh_esc.c

sh_ret.o: sh_ret.c 
	cc $(CFLAGS) $(OPTIONS) sh_ret.c

nomem.o: nomem.c
	cc $(CFLAGS) $(OPTIONS) nomem.c

cursor.o: cursor.c
	cc $(CFLAGS) $(OPTIONS) cursor.c

clrscr.o: clrscr.c
	cc $(CFLAGS) $(OPTIONS) clrscr.c

clean:
	rm -f *.o winlib.a
