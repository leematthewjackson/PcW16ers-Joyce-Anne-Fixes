
lib_LTLIBRARIES=libgsx.la
#
# Binary compatibility. See libtool.info for full details, but:
# If the source has changed at all, increment the middle number
# If interfaces have been added, removed or changed, increment the
# first number and set the middle one to 0.
# If interfaces have been added increment the right-hand number.
# If interfaces have been removed set the right-hand number to 0.
# 
libgsx_la_LDFLAGS = -version-info 0:1:0
libgsx_la_SOURCES=cgsximpl.h chkctrl.c \
		 gdos.c raster.c \
		 rasterpbm.c \
		 rasterppm.c \
		 pdf.c \
		 gsans08.inc gsans14.inc gsans16.inc \
		 tsans08.inc tsans14.inc tsans16.inc \
		 fonts.c

EXTRA_DIST = gsans08.psf gsans14.psf gsans16.psf \
	     tsans08.psf tsans14.psf tsans16.psf \
	     mkfonts

AM_CPPFLAGS=-I$(top_srcdir)/include
