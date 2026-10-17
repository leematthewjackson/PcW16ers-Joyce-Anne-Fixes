AM_CPPFLAGS=-I$(top_srcdir)/include
JAVAC=@JAVAC@
JAVAH=@JAVAH@
JAVA=@JAVA@
JAVADOC=@JAVADOC@
JAR=@JAR@
CLASSCPRE=uk.co.demon.seasip.liblink
CLASSDPRE=uk/co/demon/seasip/liblink
HEADERPRE=uk_co_demon_seasip_liblink
lib_LTLIBRARIES=liblink.la
noinst_PROGRAMS=@LLINKJAR@
EXTRA_PROGRAMS=
EXTRA_DIST=$(CLASSDPRE)/LibLink.java \
	   $(CLASSDPRE)/LibLinkException.java 

#
# Binary compatibility. See libtool.info for full details, but:
# If the source has changed at all, increment the middle number
# If interfaces have been added, removed or changed, increment the
# first number and set the middle one to 0.
# If interfaces have been added increment the right-hand number.
# If interfaces have been removed set the right-hand number to 0.
# 
liblink_la_LDFLAGS=-version-info 2:0:1
liblink_la_SOURCES=ll.h lldrivers.inc lldrivers.h \
		   llopen.c llerror.c llflush.c \
                   llisend.c llirecv.c	\
                   llsend.c llrecv.c	\
		   llcallback.c \
		   lllaplink.c lllocolink.c \
		   lllinkit.c \
		   llpacket.c \
		   llparfile.c llparfile.h \
		   llparpipe.c llparpipe.h \
		   llparport.c llparport.h \
		   llparlink.c llparlink.h \
		   llparserv.c llparserv.h \
		   lldosfs.c lldosfs.h \
		   lllocoserv.c lllocoserv.h \
		   llrpc.c lltypes.h \
		   lldiag.c \
		   lljni.c \
		   crc16.c crc16.h \
		   llparsocket.c llparsocket.h llspacket.h \
		   llparshm.c llparshm.h



JARCLASSES=$(CLASSDPRE)/LibLink.class \
	   $(CLASSDPRE)/LibLinkException.class 
JARHEADERS=$(HEADERPRE)_LibLink.h
CLEANFILES=$(JARCLASSES) $(JARHEADERS)

liblink.jar:     $(JARCLASSES)
	$(JAR) c0f $@ uk/*
	$(JAVADOC) -d $(top_srcdir)/doc $(CLASSCPRE)

$(CLASSDPRE)/%.class:   $(srcdir)/$(CLASSDPRE)/%.java
	here=`pwd` && cd $(srcdir) && $(JAVAC) -d $$here $<

%.class:        $(srcdir)/%.java
	here=`pwd` && cd $(srcdir) && $(JAVAC) -d $$here $<


$(HEADERPRE)_%.h:         $(CLASSDPRE)/%.class
	$(JAVAH) -jni $(CLASSCPRE).`basename $< .class`
	touch $@

$(srcdir)/lljni.c:     $(JARHEADERS)

install-exec-local:
	$(mkinstalldirs) $(libdir)
	if test x@LLINKJAR@ = x; then true; else $(INSTALL_DATA) @LLINKJAR@ $(libdir); fi

