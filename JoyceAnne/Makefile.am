
if INTERNAL_LIBDSK
  MAYBE_LIBDSK = LibDsk
endif

if INTERNAL_LIB765
  MAYBE_LIB765 = 765
endif

if INTERNAL_LIBGSX
  MAYBE_LIBGSX = LibGSX
endif


SUBDIRS=share cpmredir $(MAYBE_LIBGSX) $(MAYBE_LIB765) SdlTerm SdSdl guest $(MAYBE_LIBDSK) LibLink Xbm Docs bin .

EXTRA_DIST=$(srcdir)/config/*  $(srcdir)/ministl/* $(srcdir)/m4/* \
	COPYING.txt \
	setup/pcw.ico setup/joyce16.bmp setup/pcw16.ico setup/doc.ico \
	setup/joyce.nsi \
	md3trans.exe pcwtrans.exe pcw.png font8x16.x16 \
	LibGSX/config.h

ACLOCAL_AMFLAGS=-I m4
