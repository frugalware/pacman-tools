# Makefile for pacman-tools
#
# Copyright (C) 2004 Miklos Vajna <mamajom@axelero.hu>
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

INSTALL = /usr/bin/install -c
DESTDIR =
bindir = /usr/bin
sbindir = /usr/sbin
libdir = /usr/lib/frugalware

compile:

install:
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(sbindir)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) checkworld $(DESTDIR)$(bindir)/checkworld
	$(INSTALL) chkdep.pl $(DESTDIR)$(bindir)/chkdep
	$(INSTALL) fwmakepkg $(DESTDIR)$(libdir)
	$(INSTALL) movepkg $(DESTDIR)$(bindir)
	$(INSTALL) pacman-source $(DESTDIR)$(bindir)
	$(INSTALL) etcconfig.py $(DESTDIR)$(sbindir)/etcconfig
