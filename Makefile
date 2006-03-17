# Makefile for pacman-tools
#
# Copyright (C) 2004-2006 Miklos Vajna <vmiklos@frugalware.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

VERSION = 0.6.9

CFLAGS ?= -Wall -g -O2 -pipe
CFLAGS += -Wall $(shell pkg-config --cflags libxml-2.0)
LDFLAGS += $(shell pkg-config --libs libxml-2.0)

INSTALL = /usr/bin/install -c
DESTDIR =
bindir = /usr/bin
sbindir = /usr/sbin
libdir = /usr/lib/frugalware
man1dir = /usr/share/man/man1
sysconfdir = /etc

compile: chkperm

install:
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -d $(DESTDIR)$(man1dir)
	$(INSTALL) -d $(DESTDIR)$(sysconfdir)
	$(INSTALL) -d $(DESTDIR)/home/syncpkgd
	$(INSTALL) -d $(DESTDIR)/etc/rc.d
	$(INSTALL) -d $(DESTDIR)/lib/initscripts/messages/hu_HU/LC_MESSAGES/
	chown syncpkgd:daemon $(DESTDIR)/home/syncpkgd
	$(INSTALL) -d $(DESTDIR)/var/log/syncpkg
	chown syncpkgd:daemon $(DESTDIR)/var/log/syncpkg
	$(INSTALL) chkworld $(DESTDIR)$(bindir)/chkworld
	$(INSTALL) -m644 chkworld.1 $(DESTDIR)$(man1dir)
	$(INSTALL) chkdep.pl $(DESTDIR)$(bindir)/chkdep
	$(INSTALL) -m644 chkdep.1 $(DESTDIR)$(man1dir)
	$(INSTALL) pud $(DESTDIR)$(bindir)
	$(INSTALL) rf $(DESTDIR)$(bindir)
	$(INSTALL) -m644 rf.1 $(DESTDIR)$(man1dir)
	$(INSTALL) genchangelog $(DESTDIR)$(bindir)
	$(INSTALL) -m644 genchangelog.conf $(DESTDIR)$(sysconfdir)
	$(INSTALL) repoman $(DESTDIR)$(bindir)
	$(INSTALL) -m644 repoman.1 $(DESTDIR)$(man1dir)
	$(INSTALL) -m644 repoman.conf $(DESTDIR)$(sysconfdir)
	$(INSTALL) syncpkg $(DESTDIR)$(bindir)
	$(INSTALL) -m644 syncpkg.conf $(DESTDIR)$(sysconfdir)
	$(INSTALL) syncpkgd $(DESTDIR)$(sbindir)
	$(INSTALL) rc.syncpkgd $(DESTDIR)/etc/rc.d
	msgfmt -o $(DESTDIR)/lib/initscripts/messages/hu_HU/LC_MESSAGES/syncpkgd.mo rc.syncpkgd-hu.po
	$(INSTALL) fwmakepkg $(DESTDIR)$(libdir)
	$(INSTALL) movepkg $(DESTDIR)$(bindir)
	$(INSTALL) pacman-source $(DESTDIR)$(bindir)
	$(INSTALL) etcconfig.py $(DESTDIR)$(sbindir)/etcconfig
	$(INSTALL) rpm2fpm $(DESTDIR)$(bindir)/rpm2fpm
	$(INSTALL) fwcpan $(DESTDIR)$(bindir)/fwcpan
	$(INSTALL) chkperm $(DESTDIR)$(bindir)/chkperm
	$(INSTALL) fblint $(DESTDIR)$(bindir)/fblint

clean:
	rm chkperm

dist:
	darcs changes >_darcs/current/Changelog
	darcs dist -d pacman-tools-$(VERSION)
	rm _darcs/current/Changelog
