install:
	mkdir -p  $(DESTDIR)/etc/syncpkgcd
	install -g 2 -u 12 -m600 cconfig.py $(DESTDIR)/etc/syncpkgcd/cconfig.py
	mkdir -p  $(DESTDIR)/etc/syncpkgd
	install -g 2 -u 12 -m600 dconfig.py $(DESTDIR)/etc/syncpkgd/dconfig.py
	mkdir -p  $(DESTDIR)/etc/rc.d
	install rc.syncpkgcd $(DESTDIR)/etc/rc.d/rc.syncpkgcd
	install rc.syncpkgd $(DESTDIR)/etc/rc.d/rc.syncpkgd
	mkdir -p  $(DESTDIR)/usr/bin
	install syncpkgcd.py $(DESTDIR)/usr/bin/syncpkgcd
	install syncpkgd.py $(DESTDIR)/usr/bin/syncpkgd
