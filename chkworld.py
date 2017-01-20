#!/usr/bin/env python
#
#   chkworld.py, a rewrite of the chkworld tool by Zsolt Szalai.
#
#   Copyright (c) 2014 by Paolo Cretaro <melko@frugalware.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
#   USA.
#

import os
import getopt
import sys
import subprocess
import signal
from multiprocessing import Process, Queue
from re import search as regex_search
from time import asctime, localtime

signal.signal(signal.SIGPIPE, signal.SIG_DFL)

def usage():
	os.system("man chkworld")


def html_preout():
	print "<html>\n\t<head>\n\t\t<title>\n\t\t\tChkworld status\n\t\t</title>\n\t</head>\n\t<body>\n"
	print "\t\t<i>Last updated: {0}</i>\n".format(asctime(localtime()))
	print "\t\t<table>\n"


def html_postout():
	print "\t\t</table>\n"
	print "\t\t<table>\n"
	print "\t\t\t<tr>\n\t\t\t\t<td>\n\t\t\t\t\tTotal packages checked:\n\t\t\t\t</td>"
	print "\t\t\t\t<td>\n\t\t\t\t\t{0}\n\t\t\t\t</td>\n\t\t\t</tr>".format(total)
	print "\t\t\t<tr>\n\t\t\t\t<td>\n\t\t\t\t\tPassed\n\t\t\t\t</td>"
	print "\t\t\t\t<td>\n\t\t\t\t\t{0}\n\t\t\t\t</td>\n\t\t\t</tr>".format(passed)
	print "\t\t\t<tr>\n\t\t\t\t<td>\n\t\t\t\t\tNeed to update:\n\t\t\t\t</td>"
	print "\t\t\t\t<td>\n\t\t\t\t\t{0}\n\t\t\t\t</td>\n\t\t\t</tr>".format(needupdate)
	print "\t\t\t<tr>\n\t\t\t\t<td>\n\t\t\t\t\tTimed out:\n\t\t\t\t</td>"
	print "\t\t\t\t<td>\n\t\t\t\t\t{0}\n\t\t\t\t</td>\n\t\t\t</tr>".format(timeouted)
	print "\t\t\t<tr>\n\t\t\t\t<td>\n\t\t\t\t\tMaybe broken up2date:\n\t\t\t\t</td>"
	print "\t\t\t\t<td>\n\t\t\t\t\t{0}\n\t\t\t\t</td>\n\t\t\t</tr>".format(maybebroken)
	print "\t\t</table>\n"
	print "\t</body>\n</html>\n"


def std_postout():
	print "\nTotal packages checked: {0}".format(total)
	print "Passed                : {0}".format(passed)
	print "Need to update        : {0}".format(needupdate)
	print "Timed out             : {0}".format(timeouted)
	print "Maybe broken up2date  : {0}".format(maybebroken)


def find_file(filename, path, blacklist):
	global total, passed, needupdate, timeouted, maybebroken
	matches = []

	for root, dirs, files in os.walk(path):
		for d in dirs:
			if os.path.join(root, d) in blacklist:
				dirs.remove(d)  # Remove folders in blacklist
		if filename in files and root not in blacklist:
			fb = FrugalBuild(os.path.join(root, filename))
			if fb.skip:
				continue
			total += 1
			if fb.up2date == "":
				maybebroken += 1
			elif fb.up2date == "TIMEOUT":
				timeouted += 1
			elif fb.pkgver != fb.up2date:
				needupdate += 1
			else:
				passed += 1
				continue
			matches.append(fb)
			if not sort:
				fb.print_wrapper()
	return matches


def run_command(fb, command):
	cwd = os.getcwd()
	os.chdir(fb)  # this is necessary otherwise it won't use user repo but system one (stable on genesis)
	p = subprocess.Popen("source {0};source ./FrugalBuild; {1}".format(fwmakepkg, command),
		stdout=subprocess.PIPE,
		stderr=subprocess.PIPE,
		shell=True)
	out, err = p.communicate()
	if err:
		print "@".join([fb, out, err]), "@"
	os.chdir(cwd)
	return out


class FrugalBuild:
	def eval_up2date(self):
		self.queue.put(run_command(self.path, "eval $up2date").strip())

	def __init__(self, path):
		self.path = os.path.dirname(path)
		self.pkgname, self.pkgver, self.group = run_command(
			self.path,
			"echo $pkgname;echo $pkgver;echo $groups").split()
		self.up2date = run_command(self.path, "echo -n $up2date")
		try:
			with open(path, "r") as f:
				self.m8r = regex_search(
					r"# Maintainer:\s+(.*?)\s+<",
					f.read()).groups()[0]
		except IndexError:
			self.m8r = "????????"  # probably Maintainer is empty
		except AttributeError:
			self.m8r = "????????"

		self.skip = False
		if devel and self.m8r != devel:
			self.skip = True
			return  # just ignore this package and don't eval $up2date

		if " " in self.up2date:  # hardcoded version numer in $up2date
			self.queue = Queue()
			p = Process(target=self.eval_up2date)
			p.start()
			p.join(timeout)

			if p.is_alive():
				p.terminate()
				p.join()
				self.up2date = "TIMEOUT"
			else:
				self.up2date = self.queue.get()
			del self.queue

	def print_html(self):
		print "\t\t\t<tr>\n\t\t\t\t<td>\n\t\t\t\t\t{0}/{1}-{2}\n\t\t\t\t</td>".format(self.group, self.pkgname, self.pkgver)
		if self.up2date == "":
			print "\t\t\t\t<td>\n\t\t\t\t\t<font color=\"red\">There was no output! {0}</font>\n\t\t\t\t</td>\n\t\t\t</tr>\n".format(self.m8r)
		elif self.up2date == "TIMEOUT":
			print "\t\t\t\t<td>\n\t\t\t\t\t<font color=\"red\">Timed out! {0}</font>\n\t\t\t\t</td>\n\t\t\t</tr>\n".format(self.m8r)
		else:
			print "\t\t\t\t<td>\n\t\t\t\t\t<font color=\"red\">!= {0} {1}</font>\n\t\t\t\t</td>\n\t\t\t</tr>\n".format(self.up2date, self.m8r)

	def print_term(self):
		if color:
			s_timeout = "\033[1;33mTimed out!\033[1;0m"
			s_broken = "\033[1;33mThere was no output!\033[1;0m"
			s_update = "\033[1;31m{0}\033[1;0m".format(self.up2date)
		else:
			s_timeout = "Timed out!"
			s_broken = "There was no output!"
			s_update = self.up2date

		print "{0}/{1}-{2}".format(self.group, self.pkgname, self.pkgver),
		if self.up2date == "":
			print "   {0}   {1}".format(s_broken, self.m8r)
		elif self.up2date == "TIMEOUT":
			print "   {0}   {1}".format(s_timeout, self.m8r)
		else:
			print "   != {0}   {1}".format(s_update, self.m8r)

	def print_wrapper(self):
		if html:
			self.print_html()
		else:
			self.print_term()


try:
	opts, args = getopt.gnu_getopt(
		sys.argv[1:],
		"r:t:d:mscb",
		["dir=", "time=", "devel=", "html", "sort", "color", "blacklist"])
except getopt.GetoptError:
	usage()
	sys.exit(1)

fwmakepkg = "/usr/lib/frugalware/fwmakepkg"
directory = "."
timeout = 30
devel = ""
html = False
sort = False
color = False
blacklist_paths = []

total = 0
passed = 0
needupdate = 0
timeouted = 0
maybebroken = 0

for opt, arg in opts:
	if opt in ("-r", "--dir"):
		directory = arg
	if opt in ("-t", "--time"):
		timeout = int(arg)
	if opt in ("-d", "--devel"):
		devel = arg
	if opt in ("-m", "--html"):
		html = True
	if opt in ("-s", "--sort"):
		sort = True
	if opt in ("-c", "--color"):
		color = True
	if opt in ("-b", "--blacklist"):
		blacklist_paths = [os.path.normpath(a) for a in args]

if html:
	html_preout()
frugalbuilds = find_file("FrugalBuild", directory, blacklist_paths)
if sort:
	frugalbuilds.sort(key=lambda fb: fb.m8r.lower())  # otherwise [A-Z] comes before [a-z]
	for frugalbuild in frugalbuilds:
		frugalbuild.print_wrapper()
if html:
	html_postout()
else:
	std_postout()
