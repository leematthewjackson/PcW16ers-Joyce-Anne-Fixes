
EXTRA_DIST = cpm86bios.asm cpm86bios.bin cpm86.sys COPYING.DRI

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/Joyce
	$(mkinstalldirs) $(DESTDIR)$(datadir)/Joyce/guest
	 $(INSTALL_DATA) $(srcdir)/cpm86.sys $(DESTDIR)$(datadir)/Joyce/guest
	 $(INSTALL_DATA) $(srcdir)/cpm86bios.bin $(DESTDIR)$(datadir)/Joyce/guest

uninstall-local:
	$(RM) $(DESTDIR)$(datadir)/Joyce/guest/cpm86.sys
	$(RM) $(DESTDIR)$(datadir)/Joyce/guest/cpm86bios.bin

cpm86bios.bin:	cpm86bios.asm
	nasm -o$@ $<	
