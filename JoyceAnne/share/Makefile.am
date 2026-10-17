EXTRA_DIST = bootfile.emj  pcw.bmp  pcwkey.bmp  utils.dsk  utils16.dsk \
	  16power2.bmp 16power3.bmp pcw16.bmp pcw16_cpm.dsk cpm2.dsk cpm86.dsk \
	  beep_fm.wav

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/Joyce
	$(mkinstalldirs) $(DESTDIR)$(datadir)/Joyce/Boot
	$(mkinstalldirs) $(DESTDIR)$(datadir)/Joyce/Disks
	$(INSTALL_DATA) $(srcdir)/pcw.bmp $(DESTDIR)$(datadir)/Joyce
	$(INSTALL_DATA) $(srcdir)/pcw16.bmp $(DESTDIR)$(datadir)/Joyce
	$(INSTALL_DATA) $(srcdir)/pcwkey.bmp $(DESTDIR)$(datadir)/Joyce
	$(INSTALL_DATA) $(srcdir)/16power2.bmp $(DESTDIR)$(datadir)/Joyce
	$(INSTALL_DATA) $(srcdir)/16power3.bmp $(DESTDIR)$(datadir)/Joyce
	$(INSTALL_DATA) $(srcdir)/beep_fm.wav $(DESTDIR)$(datadir)/Joyce
	$(INSTALL_DATA) $(srcdir)/bootfile.emj $(DESTDIR)$(datadir)/Joyce/Boot
	$(INSTALL_DATA) $(srcdir)/utils.dsk $(DESTDIR)$(datadir)/Joyce/Disks
	$(INSTALL_DATA) $(srcdir)/utils16.dsk $(DESTDIR)$(datadir)/Joyce/Disks
	$(INSTALL_DATA) $(srcdir)/pcw16_cpm.dsk $(DESTDIR)$(datadir)/Joyce/Disks
	$(INSTALL_DATA) $(srcdir)/cpm2.dsk $(DESTDIR)$(datadir)/Joyce/Disks
	$(INSTALL_DATA) $(srcdir)/cpm86.dsk $(DESTDIR)$(datadir)/Joyce/Disks

uninstall-local:
	$(RM) $(DESTDIR)$(datadir)/Joyce/pcw.bmp 
	$(RM) $(DESTDIR)$(datadir)/Joyce/pcw16.bmp 
	$(RM) $(DESTDIR)$(datadir)/Joyce/pcwkey.bmp
	$(RM) $(DESTDIR)$(datadir)/Joyce/16power2.bmp 
	$(RM) $(DESTDIR)$(datadir)/Joyce/16power3.bmp
	$(RM) $(DESTDIR)$(datadir)/Joyce/beep_fm.wav
	$(RM) $(DESTDIR)$(datadir)/Joyce/Boot/bootfile.emj 
	$(RM) $(DESTDIR)$(datadir)/Joyce/Disks/utils.dsk 
	$(RM) $(DESTDIR)$(datadir)/Joyce/Disks/utils16.dsk
	$(RM) $(DESTDIR)$(datadir)/Joyce/Disks/pcw16_cpm.dsk
	$(RM) $(DESTDIR)$(datadir)/Joyce/Disks/cpm2.dsk
	$(RM) $(DESTDIR)$(datadir)/Joyce/Disks/cpm86.dsk

