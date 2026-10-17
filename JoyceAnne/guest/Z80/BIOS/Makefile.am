
EXTRA_DIST = cpm2bios.z80 cpm2bios.bin \
	     os2ccp.z80 os2ccp.prl os3bdos.z80 os3bdos.prl \
	     COPYING.DRI

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/Joyce
	$(mkinstalldirs) $(DESTDIR)$(datadir)/Joyce/guest
	 $(INSTALL_DATA) $(srcdir)/os2ccp.prl $(DESTDIR)$(datadir)/Joyce/guest
	 $(INSTALL_DATA) $(srcdir)/os3bdos.prl $(DESTDIR)$(datadir)/Joyce/guest
	 $(INSTALL_DATA) $(srcdir)/cpm2bios.bin $(DESTDIR)$(datadir)/Joyce/guest

uninstall-local:
	$(RM) $(DESTDIR)$(datadir)/Joyce/guest/os2ccp.prl
	$(RM) $(DESTDIR)$(datadir)/Joyce/guest/os3bdos.prl
	$(RM) $(DESTDIR)$(datadir)/Joyce/guest/cpm2bios.bin

cpm2bios.bin:	cpm2bios.z80
	pasmo $< $@
