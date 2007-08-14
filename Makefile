# Makefile for pacman-tools
#
# Copyright (C) 2004-2007 Miklos Vajna <vmiklos@frugalware.org>
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

VERSION = 0.9.8

LANGS = hu pl

CFLAGS ?= -Wall -Werror -g -O2 -pipe
CFLAGS += $(shell pkg-config --cflags libxml-2.0)
LDFLAGS += $(shell pkg-config --libs libxml-2.0)

INSTALL = /usr/bin/install -c
DESTDIR =
bindir = /usr/bin
sbindir = /usr/sbin
libdir = /usr/lib/frugalware
man1dir = /usr/share/man/man1
man3dir = /usr/share/man/man3
man8dir = /usr/share/man/man8
sysconfdir = /etc
docdir = /usr/share/doc/pacman-tools-$(VERSION)
FINCDIR = $(shell source /usr/lib/frugalware/fwmakepkg; echo $$Fincdir)

compile: genauthors apidocs fwmakepkg.3
	$(MAKE) -C mkiso
	$(MAKE) -C repoman.d
	chmod +x fwmirror pear-makefb chkdep darcs-git.py
	help2man -n "mirrors Frugalware archives" -S Frugalware -N ./fwmirror |sed 's/\\(co/(c)/' >fwmirror.1
	help2man -n "Writes FrugalBuild scripts for PHP PEAR/PECL packages" -S Frugalware -N ./pear-makefb \
		|sed 's/\\(co/(c)/' >pear-makefb.1
	chmod +x fblint
	help2man -n "searches for common FrugalBuild problems" -S Frugalware -N ./fblint |sed 's/\\(co/(c)/' >fblint.1
	help2man -n "Checks a package or directory for possible depends" -S Frugalware -N ./chkdep |sed 's/\\(co/(c)/' >chkdep.1
	help2man -n "Converts a README.Frugalware to HTML" -S Frugalware -N ./mkpkghtml |sed 's/\\(co/(c)/' >mkpkghtml.1
	help2man -n "A darcs-like interface for git" -S Frugalware -N ./darcs-git.py |sed 's/\\(co/(c)/' >darcs-git.1

install:
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -d $(DESTDIR)$(man1dir)
	$(INSTALL) -d $(DESTDIR)$(man3dir)
	$(INSTALL) -d $(DESTDIR)$(man8dir)
	$(INSTALL) -d $(DESTDIR)$(sysconfdir)
	$(INSTALL) -d $(DESTDIR)$(sysconfdir)/repoman.d
	$(INSTALL) -d $(DESTDIR)$(docdir)
	$(INSTALL) -d $(DESTDIR)/home/syncpkgd
	chown syncpkgd:daemon $(DESTDIR)/home/syncpkgd
	$(INSTALL) chkworld $(DESTDIR)$(bindir)/chkworld
	$(INSTALL) -m644 chkworld.1 $(DESTDIR)$(man1dir)
	$(INSTALL) chkdep $(DESTDIR)$(bindir)/chkdep
	$(INSTALL) -m644 chkdep.1 $(DESTDIR)$(man1dir)
	$(INSTALL) mkpkghtml $(DESTDIR)$(bindir)/mkpkghtml
	$(INSTALL) -m644 mkpkghtml.1 $(DESTDIR)$(man1dir)
	$(INSTALL) pud $(DESTDIR)$(bindir)
	$(INSTALL) genchangelog $(DESTDIR)$(bindir)
	$(INSTALL) darcs-git.py $(DESTDIR)$(bindir)/darcs-git
	ln -s darcs-git $(DESTDIR)$(bindir)/dg
	$(INSTALL) repoman $(DESTDIR)$(bindir)
	$(INSTALL) -m644 repoman.1 $(DESTDIR)$(man1dir)
	$(INSTALL) -m644 repoman.conf $(DESTDIR)$(sysconfdir)
	$(INSTALL) -m644 repoman.d/current $(DESTDIR)$(sysconfdir)/repoman.d/current
	$(INSTALL) -m644 repoman.d/stable $(DESTDIR)$(sysconfdir)/repoman.d/stable
	$(INSTALL) fwmakepkg $(DESTDIR)$(libdir)
	$(INSTALL) movepkg $(DESTDIR)$(bindir)
	$(INSTALL) pacman-source $(DESTDIR)$(bindir)
	$(INSTALL) etcconfig.py $(DESTDIR)$(sbindir)/etcconfig
	$(INSTALL) rpm2fpm $(DESTDIR)$(bindir)/rpm2fpm
	$(INSTALL) fwcpan $(DESTDIR)$(bindir)/fwcpan
	$(INSTALL) genauthors $(DESTDIR)$(bindir)/genauthors
	$(INSTALL) fblint $(DESTDIR)$(bindir)/fblint
	$(INSTALL) pootle-update $(DESTDIR)$(bindir)/pootle-update
	$(INSTALL) mkiso/mkiso $(DESTDIR)$(bindir)/mkiso
	$(INSTALL) fwmirror $(DESTDIR)$(bindir)/fwmirror
	$(INSTALL) -m644 fwmirror.1 $(DESTDIR)$(man1dir)
	$(INSTALL) pear-makefb $(DESTDIR)$(bindir)/pear-makefb
	$(INSTALL) -m644 pear-makefb.1 $(DESTDIR)$(man1dir)
	$(INSTALL) -m644 fblint.1 $(DESTDIR)$(man1dir)
	$(INSTALL) -m644 mkiso/mkiso.8 $(DESTDIR)$(man8dir)
	$(INSTALL) -m644 mkiso/volumes.xml $(DESTDIR)$(docdir)/volumes.xml
	$(INSTALL) -m644 apidocs/*.3 $(DESTDIR)$(man3dir)
	$(INSTALL) -m644 fwmakepkg.3 $(DESTDIR)$(man3dir)
	make -C syncpkgd DESTDIR=$(DESTDIR) install

clean:
	rm -rf genauthors apidocs fwmakepkg.3
	$(MAKE) -C mkiso clean

dist:
	git-archive --format=tar --prefix=pacman-tools-$(VERSION)/ HEAD > pacman-tools-$(VERSION).tar
	mkdir -p pacman-tools-$(VERSION)
	git log --no-merges |git name-rev --tags --stdin > pacman-tools-$(VERSION)/Changelog
	tar rf pacman-tools-$(VERSION).tar pacman-tools-$(VERSION)/Changelog
	rm -rf pacman-tools-$(VERSION)
	gzip -f -9 pacman-tools-$(VERSION).tar
	gpg --comment "See http://ftp.frugalware.org/pub/README.GPG for info" \
		-ba -u 20F55619 pacman-tools-$(VERSION).tar.gz

release:
	git tag $(VERSION)
	$(MAKE) dist
	mv pacman-tools-$(VERSION).tar.gz{,.asc} ../

apidocs:
	cp -a $(FINCDIR) apidocs
	make -C apidocs

fwmakepkg.3: apidocs fwmakepkg.3.in
	cat fwmakepkg.3.in > fwmakepkg.3
	ls apidocs/*.sh|sed 's|apidocs/||;$$!s/\(.*\)$$/.BR \1 (3),/;$$s/\(.*\)$$/.BR \1 (3)/' >> fwmakepkg.3
