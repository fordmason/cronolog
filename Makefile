# Makefile for cronolog
# Copyright (C) 1996, Andrew Ford <andrew@icarus.demon.co.uk>
# $Id: Makefile,v 1.7 1996/12/20 11:03:50 andrew Exp $

# Specify compiler (CC), flags (CFLAGS) and libraries (LIBS)
# CFLAGS can contain a number of options:
#
# -DFILE_MODE=octal-number	mode used for creating files (default is 0664)
# -DDIR_MODE=octal-number	mode used for creating directories (default is 0775)
# -DDONT_CREATE_SUBDIRS		don't include code to create missing directories
# -DNEED_GETOPT_DEFS		if your platform doesn't declare getopt()
# -DDEBUG			enable debugging code
# -DTESTHARNESS			build a simple test harness program

#CC = cc
CC = gcc

CFLAGS = -O9 -Wall -DDEBUG 
# CFLAGS = -g -DDEBUG 

#LIBS=

# End of customization section



# cronolog version

VERSION = 1.4

# Files in distribution

DISTFILES	= README MANIFEST LICENSE Artistic Copying ChangeLog \
		  Makefile cronolog.c cronolog.1


default: cronolog

all: cronolog testharness

cronolog.o: Makefile
	$(CC) -c $(CFLAGS) -DVERSION=\"$(VERSION)\" cronolog.c

cronolog: cronolog.o Makefile
	$(CC) -o $@ cronolog.o $(LIBS)

testharness: cronolog.c Makefile
	$(CC) -DTESTHARNESS -DDEBUG -o $@ cronolog.c $(LIBS)

dist:
	tar  cvf cronolog-$(VERSION).tar    $(DISTFILES)
	tar zcvf cronolog-$(VERSION).tar.gz $(DISTFILES)


install: cronolog
	@copy cronolog to an appropriate directory

clean:
	@rm -f *~

distclean: clean
	@rm -f cronolog testharness *.o
