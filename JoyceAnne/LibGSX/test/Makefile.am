## Process this file with automake to produce Makefile.in

AM_CPPFLAGS=-I$(top_srcdir)/include
LDADD=../lib/libgsx.la -lm

noinst_PROGRAMS=gsxtest rastest
gsxtest_SOURCES=gsxtest.c z80.c z80.h
rastest_SOURCES=rastest.c


EXTRA_DIST=Z80_README.md gsxtest.zip 

clean-local:
	-rm -f *.ppm *.pbm *.pdf *.png
