OBJECTS = main.o port.o globals.o lite.o get_entry.o get_data.o add_port.o \
	  add_dial.o add_sys.o display_rec.o delete.o

CFLAGS = -v -c

LIBS = -lcurses -lmisc

uuinstall:	$(OBJECTS)
	cc -v $(OBJECTS) -o $@ $(LIBS)

globals.o:	globals.c
	cc $(CFLAGS) $<

main.o:	main.c
	cc $(CFLAGS) $<

port.o:	port.c
	cc $(CFLAGS) $<

lite.o:	lite.c
	cc $(CFLAGS) $<

get_entry.o:	get_entry.c
	cc $(CFLAGS) $<

get_data.o:	get_data.c
	cc $(CFLAGS) $<

add_port.o:	add_port.c
	cc $(CFLAGS) $<

add_dial.o:	add_dial.c
	cc $(CFLAGS) $<

add_sys.o:	add_sys.c
	cc $(CFLAGS) $<

display_rec.o:	display_rec.c
	cc $(CFLAGS) $<

delete.o:	delete.c
	cc $(CFLAGS) $<
