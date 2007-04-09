#!/usr/bin/env python

"""Checks a package or directory for possible depends

Usage: chkdep [options]

Options:
  -d ..., --dir=...       name od the dir to check
  -i ..., --ignore=...    ignore the given package (optional, useful when a
                          package links to itself)
  -m ..., --method=...    method to use to detect the deps (default: elf,
                          possible values: elf, python)
  -p ..., --package=...   name of the fpm to check

You should at least use the -d or the -p option.
"""

__author__ = "Miklos Vajna <vmiklos@frugalware.org>"
__version__ = "0.8.7"
__date__ = "Tue, 10 Apr 2007 00:22:42 +0200"
__copyright__ = "Copyright (c) 2007 Miklos Vajna"
__license__ = "GPL"

import tarfile, tempfile, shutil, os, stat, re, pacman, getopt, sys

from modulefinder import ModuleFinder

def usage():
	print __doc__

def rmdupdeps(deps):
	depdeps = []
	newdeps = []
	i = pacman.db_getpkgcache(db)
	while i:
		pkg = pacman.void_to_PM_PKG(pacman.list_getdata(i))
		pkgname = pacman.void_to_char(pacman.pkg_getinfo(pkg, pacman.PKG_NAME))
		if pkgname in deps:
			j = pacman.void_to_PM_LIST(pacman.pkg_getinfo(pkg, pacman.PKG_DEPENDS))
			while j:
				dep = pacman.void_to_char(pacman.list_getdata(j)).split("<")[0].split(">")[0].split("=")[0]
				if dep not in depdeps:
					depdeps.append(dep)
				j = pacman.list_next(j)
		i = pacman.list_next(i)
	for i in deps:
		if i not in depdeps and i not in ignorepkgs:
			newdeps.append(i)
	return newdeps

def detect_owner(lib):
	if lib.find(fpmroot) == 0:
		return
	pkg = pacman.void_to_PM_PKG(pacman.list_getdata(pacman.pkg_getowners(lib)))
	owner = pacman.void_to_char(pacman.pkg_getinfo(pkg, pacman.PKG_NAME))
	if not owner:
		print "WARNING: No package found containing %s!" % lib
	elif owner not in deps:
		deps.append(owner)

class Checks:
	def python(self, file):
		mf = ModuleFinder()
		try:
			mf.run_script(file)
		except:
			return

		for key in mf.modules.keys():
			file = mf.modules[key].__file__
			if file:
				detect_owner(file)

	def elf(self, file):
		if not os.stat(file)[stat.ST_MODE] & stat.S_IXUSR:
			return
		sock = os.popen("ldd %s" % file)
		while True:
			i = sock.readline()
			if not i:
				break
			if i.find("=>") == -1:
				continue
			lib = re.sub(r".* => (.*) \(.*", r"\1", i.strip())
			if len(lib):
				detect_owner(lib)
		sock.close()

checks = Checks()

pacman.initialize("/")
db = pacman.db_register("local")
ignorepkgs = []
deps = []

dir = None
allowedmethods = ("elf")
method="elf"
pkg = None

try:
	opts, args = getopt.getopt(sys.argv[1:], "d:p:i:m:v", ["dir=", "package=", "ignore=", "method=", "version"])
except getopt.GetoptError:
	usage()
	sys.exit(1)

for opt, arg in opts:
	if opt in ("-d", "--dir"):
		dir = arg
	if opt in ("-p", "--package"):
		pkg = arg
	if opt in ("-m", "--method"):
		method = arg
	if opt in ("-i", "--ignore"):
		ignorepkgs.append(arg)
	if opt in ("-v", "--version"):
		print "chkdep.py %s" % __version__
		sys.exit(0)

if not dir and not pkg:
	usage()
	sys.exit(1)

checker = getattr(checks, method)

if "FAKEROOTKEY" in os.environ.keys():
	ignorepkgs.append("fakeroot")

if pkg:
	fpmroot = tempfile.mkdtemp()
	fpm = tarfile.TarFile.open(pkg, "r:bz2")
	fpm.extractall(fpmroot)
	fpm.close()

if dir:
	fpmroot = dir

depfiles = []
for root, dirs, files in os.walk(fpmroot):
	for file in files:
		ret = checker(os.path.join(root, file))
		if ret:
			for i in checker(os.path.join(root, file)):
				if i not in depfiles:
					depfiles.append(i)
for i in depfiles:
	detect_owner(i)

if pkg:
	shutil.rmtree(fpmroot)

deps = rmdupdeps(deps)
print "depends=('" + "' '".join(deps) + "')"
