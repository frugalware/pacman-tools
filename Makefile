# Makefile for pacman-tools
#
# Copyright (C) 2004-2008 Miklos Vajna <vmiklos@frugalware.org>
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

VERSION = 1.0.9
DATE := $(shell date +%Y-%m-%d)

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
DOCS = $(wildcard *.txt) $(wildcard syncpkgd/*.txt) $(wildcard mkiso/*.txt)
MANS = $(subst .txt,.1,$(DOCS))

PROGRAMS = bumppkg chkdep chkworld emulgen fblint fpmdiff fwcpan fwmirror \
	genauthors genchangelog mkisorelease mkpkghtml pear-makefb pootle-update \
	portpkg repoman revdep-rebuild rpm2fpm syncemul

compile: $(PROGRAMS) apidocs docs
	$(MAKE) -C mkiso
	$(MAKE) -C repoman.d

docs: $(MANS)

install: compile
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
	$(INSTALL) -m755 $(PROGRAMS) $(DESTDIR)$(bindir)
	$(INSTALL) -m644 $(MANS) $(DESTDIR)$(man1dir)
	$(INSTALL) darcs-git.py $(DESTDIR)$(bindir)/darcs-git
	ln -s darcs-git $(DESTDIR)$(bindir)/dg
	$(INSTALL) -m644 repoman.conf $(DESTDIR)$(sysconfdir)
	$(INSTALL) -m644 repoman.d/current $(DESTDIR)$(sysconfdir)/repoman.d/current
	$(INSTALL) -m644 repoman.d/stable $(DESTDIR)$(sysconfdir)/repoman.d/stable
	$(INSTALL) lib/fwmakepkg $(DESTDIR)$(libdir)
	$(INSTALL) etcconfig.py $(DESTDIR)$(sbindir)/etcconfig
	$(INSTALL) mkiso/mkiso $(DESTDIR)$(bindir)/mkiso
	$(INSTALL) -m644 mkiso/volumes.xml $(DESTDIR)$(docdir)/volumes.xml
	$(INSTALL) -m644 apidocs/*.3 $(DESTDIR)$(man3dir)
	make -C syncpkgd DESTDIR=$(DESTDIR) install

clean:
	rm -rf genauthors apidocs $(MANS)
	$(MAKE) -C mkiso clean

dist:
	git-archive --format=tar --prefix=pacman-tools-$(VERSION)/ HEAD > pacman-tools-$(VERSION).tar
	mkdir -p pacman-tools-$(VERSION)
	git log --no-merges |git name-rev --tags --stdin > pacman-tools-$(VERSION)/Changelog
	tar rf pacman-tools-$(VERSION).tar pacman-tools-$(VERSION)/Changelog
	rm -rf pacman-tools-$(VERSION)
	gzip -f -9 pacman-tools-$(VERSION).tar

release:
	git tag -l |grep -q $(VERSION) || dg tag $(VERSION)
	$(MAKE) dist
	gpg --comment "See http://ftp.frugalware.org/pub/README.GPG for info" \
		-ba -u 20F55619 pacman-tools-$(VERSION).tar.gz
	mv pacman-tools-$(VERSION).tar.gz{,.asc} ../

apidocs:
	cp -a $(FINCDIR) apidocs
	make -C apidocs

%.html: %.txt
	asciidoc $^

%.1: %.txt
	a2x --asciidoc-opts="-f asciidoc.conf" -a \
		pacman_tools_version=$(VERSION) -a pacman_tools_date=$(DATE) -f manpage $^

doc: ../HEADER.html ../Changelog

../HEADER.html: README
	ln -sf pacman-tools/README ../HEADER.txt
	asciidoc -a toc -a numbered -a sectids ../HEADER.txt
	rm ../HEADER.txt

../Changelog: .git/refs/heads/master
	git log --no-merges |git name-rev --tags --stdin >../Changelog
