install:
	mkdir -p  $(DESTDIR)/etc/syncpkgcd
	install -g 2 -o 12 -m600 cconfig.py $(DESTDIR)/etc/syncpkgcd/cconfig.py
	mkdir -p  $(DESTDIR)/etc/syncpkgd
	install -g 2 -o 12 -m600 dconfig.py $(DESTDIR)/etc/syncpkgd/dconfig.py
	mkdir -p  $(DESTDIR)/etc/rc.d
	install rc.syncpkgcd $(DESTDIR)/etc/rc.d/rc.syncpkgcd
	install rc.syncpkgd $(DESTDIR)/etc/rc.d/rc.syncpkgd
	mkdir -p  $(DESTDIR)/usr/bin
	install syncpkgcd.py $(DESTDIR)/usr/bin/syncpkgcd
	install syncpkgd.py $(DESTDIR)/usr/bin/syncpkgd
	mkdir -p $(DESTDIR)/var/log/syncpkgd/clientlogs
	chown 12:2 $(DESTDIR)/var/log/syncpkgd/clientlogs
	mkdir -p $(DESTDIR)/var/log/syncpkgd
	touch $(DESTDIR)/var/log/syncpkgd/daemon.log
	chown 12:2 $(DESTDIR)/var/log/syncpkgd/daemon.log
	mkdir -p $(DESTDIR)/var/lock
	touch $(DESTDIR)/var/lock/syncpkgd.pid
	chown 12:2 $(DESTDIR)/var/lock/syncpkgd.pid
	mkdir -p $(DESTDIR)/var/run
	touch $(DESTDIR)/var/run/syncpkgd.status
	chown 12:2 $(DESTDIR)/var/run/syncpkgd.status
	touch $(DESTDIR)/var/log/syncpkgcd.log
	chown 12:2 $(DESTDIR)/var/log/syncpkgcd.log
	touch $(DESTDIR)/var/lock/syncpkgcd.pid
	chown 12:2 $(DESTDIR)/var/lock/syncpkgcd.pid
