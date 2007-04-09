#!/usr/bin/env python

import tarfile, tempfile, shutil, os, stat, re, pacman

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
		if i not in depdeps:
			newdeps.append(i)
	return newdeps

class Checks:
	def elf(self, file):
		if not os.stat(file)[stat.ST_MODE] & stat.S_IXUSR:
			return
		sock = os.popen("ldd %s" % file)
		for i in sock.readlines():
			if i.find("=>") == -1:
				continue
			lib = re.sub(r".* => (.*) \(.*", r"\1", i.strip())
			if len(lib):
				pkg = pacman.void_to_PM_PKG(pacman.list_getdata(pacman.pkg_getowners(lib)))
				owner = pacman.void_to_char(pacman.pkg_getinfo(pkg, pacman.PKG_NAME))
				if owner not in deps:
					deps.append(owner)

checks = Checks()

pacman.initialize("/")
db = pacman.db_register("local")
ignorepkgs = []
deps = []
method="elf"

checker = getattr(checks, method)
fpmroot = tempfile.mkdtemp()

if "FAKEROOTKEY" in os.environ.keys():
	ignorepkgs.append("fakeroot")

fpm = tarfile.TarFile.open("pacman-tools-0.8.8-1-i686.fpm", "r:bz2")
fpm.extractall(fpmroot)
fpm.close()

for root, dirs, files in os.walk(fpmroot):
	for file in files:
		checker(os.path.join(root, file))

shutil.rmtree(fpmroot)

deps = rmdupdeps(deps)
print deps
