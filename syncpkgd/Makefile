install:
	mkdir -p  $(DESTDIR)/etc/syncpkgcd
	install -g 2 -o 12 -m600 cconfig.py $(DESTDIR)/etc/syncpkgcd/cconfig.py
	mkdir -p  $(DESTDIR)/etc/syncpkgd
	install -g 2 -o 12 -m600 dconfig.py $(DESTDIR)/etc/syncpkgd/dconfig.py
	install -m644 ctlconfig.py $(DESTDIR)/etc/syncpkgd/ctlconfig.py
	mkdir -p  $(DESTDIR)/lib/systemd/system
	install -m644 syncpkgcd.service $(DESTDIR)/lib/systemd/system/syncpkgcd.service
	install -m644 syncpkgd.service $(DESTDIR)/lib/systemd/system/syncpkgd.service
	mkdir -p  $(DESTDIR)/usr/bin
	install syncpkgcd.py $(DESTDIR)/usr/bin/syncpkgcd
	install syncpkgdctl.py $(DESTDIR)/usr/bin/syncpkgdctl
	install syncpkgd.py $(DESTDIR)/usr/bin/syncpkgd
	install syncpkg-shell $(DESTDIR)/usr/bin/syncpkg-shell
	mkdir -p $(DESTDIR)/var/log/syncpkgd/clientlogs
	chown 12:2 $(DESTDIR)/var/log/syncpkgd/clientlogs
	mkdir -p $(DESTDIR)/var/log/syncpkgd
	touch $(DESTDIR)/var/log/syncpkgd/daemon.log
	chown 12:2 $(DESTDIR)/var/log/syncpkgd/daemon.log
	mkdir -p $(DESTDIR)/var/lib
	touch $(DESTDIR)/var/lib/syncpkgd.status
	chown 12:2 $(DESTDIR)/var/lib/syncpkgd.status
	touch $(DESTDIR)/var/log/syncpkgcd.log
	chown 12:2 $(DESTDIR)/var/log/syncpkgcd.log
