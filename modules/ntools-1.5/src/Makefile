##########################################################################
#
# Makefile for ntools
# (C) 2010, Norbert Vegh, ntools@norvegh.com
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
##########################################################################

BINDIR ?= /usr/local/bin
MANDIR ?= /usr/local/man
CC = gcc

# set DEBUG to YES to enable debug mode
#DEBUG = YES

#---------- DO NOT CHANGE ANYTHING BELOW THIS LINE ----------#

TARGETS = ngen nrecv nserver nclient nemu nemud
MANPAGES = ntools.1 ngen.1 nrecv.1 nserver.1 nclient.1 nemu.1 nemud.1

ifeq ( $(DEBUG), YES )
	DEBUGDEF = -DDEBUG
endif

CFLAGS = -Wall -O3 $(DEBUGDEF)
LDFLAGS = $(CFLAGS) -lstdc++ -lpthread -lm

.cpp.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGETS)

ngen:  ngen-main.o ngen.o utils.o distrib.o range.o linkedlist.h statistics.o defs.h
	$(CC) -o ngen ngen-main.o ngen.o utils.o distrib.o range.o statistics.o $(LDFLAGS)

nrecv:  nrecv-main.o nrecv.o utils.o range.o statistics.o linkedlist.h defs.h
	$(CC) -o nrecv nrecv-main.o nrecv.o utils.o range.o statistics.o $(LDFLAGS)

nserver:  nserver.o utils.o defs.h
	$(CC) -o nserver nserver.o utils.o $(LDFLAGS)

nclient:  nclient.o utils.o distrib.o defs.h
	$(CC) -o nclient nclient.o utils.o distrib.o $(LDFLAGS)

nemu:  nemu.o impairs.o utils.o defs.h
	$(CC) -o nemu impairs.o nemu.o utils.o $(LDFLAGS)

nemud:  nemud.o impairs.o utils.o linkedlist.h defs.h
	$(CC) -o nemud impairs.o nemud.o utils.o $(LDFLAGS)

install: all
	if [ ! -d $(BINDIR) ]; then echo "$(BINDIR) does not exist"; mkdir -vp $(BINDIR); fi
	if [ ! -d $(MANDIR) ]; then echo "$(MANDIR) does not exist"; mkdir -vp $(MANDIR); fi
	if [ ! -d $(MANDIR)/man1 ]; then mkdir -v $(MANDIR)/man1; fi
	install -m 755 $(TARGETS) $(BINDIR)
	cd ../man/man1; install -m 644 $(MANPAGES) $(MANDIR)/man1
	
uninstall:
	cd $(BINDIR); rm -vf $(TARGETS)
	cd $(MANDIR)/man1; rm -vf $(MANPAGES)
	
clean:
	-rm -f *.o *.a *~ core $(TARGETS)

