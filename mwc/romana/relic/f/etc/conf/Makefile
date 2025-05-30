# Master makefile
# Use of GCC for compilations of DDI/DKI drivers caused by this makefile is
# strongly recommended. Comment out the following lines if you cannot use gcc
# for some reason. The default compiler is fine for the Space.c and conf.c
# files, though.

CC=gcc
CFLAGS=-O2 -Wall -ansi -pedantic
CONFDIR=.

all: drivers $(CONFDIR)/drvbld.mak configure

# Update binaries of drivers and the 'devadm' program

drivers:
	j=$$CWD; for i in */src/[Mm]akefile; do \
		if [ -f $$i ]; then \
			DEVDIR=`expr $$i : '\(.*\)/.*'`; \
			echo "$$DEVDIR"; \
			cd $$DEVDIR; \
			make CC="$(CC)" CFLAGS="$(CFLAGS)" || exit 1; \
			cd $$j; \
		fi; \
	done

# As a detail, we check to see that a device for major number 4 is installed,
# otherwise the kernel will be built without a console and will fail to be
# very useful.

$(CONFDIR)/drvbld.mak: $(CONFDIR)/mdevice $(CONFDIR)/sdevice \
			$(CONFDIR)/template.mak
	bin/idcheck -i $(CONFDIR) -B -l 4; if [ $$? -gt 0 -a $$? -lt 100 ]; \
		then exit 0; \
		else echo "No console device configured"; exit 1;\
		fi
	CONFPATH=$$CWD bin/devadm -d -I $(CONFDIR)

configure:
	export CONFPATH=$$CWD; cd $(CONFDIR); \
		make K386LIB=$(K386LIB) TARGET=$(TARGET) -f drvbld.mak

clean:	newconf
	j=$$CWD; for i in */src/[Mm]akefile; do \
		cd `expr $$i : '\(.*\)/.*'`; \
		make clean || exit 1; \
		cd $$j; \
	done

newconf:
	rm -f $(CONFDIR)/drvbld.mak $(CONFDIR)/conf.c $(CONFDIR)/conf.h \
		$(CONFDIR)/obj/*
