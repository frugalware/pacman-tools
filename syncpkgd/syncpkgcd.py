#!/usr/bin/env python

import xmlrpclib, time, os, getopt, sys, socket, glob, base64, pwd, signal
import traceback, shutil
sys.path.append("/etc/syncpkgcd")
from cconfig import config

server = xmlrpclib.Server(config.server_url)

class Options:
	def __init__(self):
		self.daemon = False
		self.pidfile = "syncpkgcd.pid"
		self.logfile = "syncpkgcd.log"
		self.help = False
		self.uid = False
	def usage(self, ret):
		os.system("man syncpkgcd")
		sys.exit(ret)

class Syncpkgcd:
	def __init__(self, options):
		self.options = options
		self.home = pwd.getpwnam(options.uid).pw_dir
		def on_sigterm(num, frame):
			raise KeyboardInterrupt
		signal.signal(signal.SIGTERM, on_sigterm)
		if options.daemon:
			pid = os.fork()
			if pid == 0:
				self.setuid()
				os.setpgrp()
				nullin = file('/dev/null', 'r')
				nullout = file('/dev/null', 'w')
				os.dup2(nullin.fileno(), sys.stdin.fileno())
				os.dup2(nullout.fileno(), sys.stdout.fileno())
				os.dup2(nullout.fileno(), sys.stderr.fileno())
			else:
				try:
					file(options.pidfile,'w+').write(str(pid)+'\n')
				except:
					pass
				sys.exit(0)
		else:
				self.setuid()
		# log
		self.logsock = open(options.logfile, "a")
		self.log("", "client started")

		# main loop
		try:
			while True:
				if not self.checkload():
					self.sleep("load too high")
					continue
				try:
					pkg = server.request_pkg(config.server_user, config.server_pass, os.uname()[-1])
				except socket.error:
					self.sleep("can't connect to server")
					continue
				except xmlrpclib.ProtocolError:
					self.sleep("can't connect to proxy")
					continue
				if not len(pkg):
					self.sleep("no package to build")
					continue
				# there is a pkg to build, request
				# up to date repo list first
				try:
					confs = server.request_confs()
				except socket.error:
					self.sleep("can't download repoman.conf from the server")
					continue
				try:
					os.makedirs(os.path.join(self.home, ".pacman-g2/repos"))
				except OSError:
					pass
				for k, v in confs.items():
					sock = open(os.path.join(self.home, k), "w")
					sock.write(base64.decodestring(v))
					sock.close()
				self.build(pkg)
		except KeyboardInterrupt:
			# here we could abort the current build properly
			self.save()
			return
		except Exception:
			self.log_exception()

	def setuid(self):
		if os.getuid() == 0 and self.options.uid:
			try:
				os.setuid(int(self.options.uid))
			except:
				os.setuid(pwd.getpwnam(self.options.uid).pw_uid)
	def checkload(self):
		if not hasattr(config, "throttle"):
			# no limit defined
			return True
		sock = open("/proc/loadavg")
		buf = sock.read().strip()
		sock.close()
		load = buf.split(' ')[0]
		if float(load) > config.throttle:
			return False
		else:
			return True

	def build(self, pkg):
		# maybe later support protocolls (the first item) other than git?
		scm = pkg.split('/')[0][:-1]
		tree = pkg.split('/')[2]
		pkgarr = pkg.split('/')[3].split('-')
		pkgname = "-".join(pkgarr[:-3])
		pkgver = "-".join(pkgarr[-3:-1])
		arch = pkgarr[-1]
		self.log(pkg, "starting build")
		sock = os.popen("export HOME=%s; . ~/.repoman.conf; echo $fst_root; echo $%s_servers" % (self.home, tree))
		buf = sock.readlines()
		sock.close()
		fst_root = buf[0].strip()
		url = buf[1].strip()
		try:
			os.stat(fst_root)
		except OSError:
			os.makedirs(fst_root)
		os.chdir(fst_root)
		if scm not in ["git", "darcs"]:
			self.log(pkg, "unkown scm, aborting")
			return
		try:
			os.stat(tree)
			os.chdir(tree)
			if scm == "git":
				self.system("git fetch")
				self.system("git reset --hard origin/master")
			elif scm == "darcs":
				self.system("darcs pull -a")
				self.system("darcs revert -a")
		except OSError:
			if scm == "git":
				self.system("git clone %s %s" % (url, tree))
			elif scm == "darcs":
				self.system("darcs get --partial %s %s" % (url, tree))
			try:
				os.chdir(tree)
			except OSError:
				self.log(pkg, "failed to get the repo")
				return
		if not self.go(pkgname):
			server.report_result(config.server_user, config.server_pass, pkg, 1, base64.encodestring("No such package."))
			return
		if scm == "git":
			self.system("git clean -x -d -f")
		elif scm == "darcs":
			junk = []
			junk.extend(glob.glob("*.fpm"))
			junk.extend(glob.glob("*.log"))
			junk.extend(glob.glob("*.log.bz2"))
			for i in junk:
				os.unlink(i)
		# FIXME: hardcoding this is a bit ugly, but well, this probably
		# won't change in the near future
		if tree not in ('current', 'stable'):
			makepkg_tree = "%s,current" % tree
		else:
			makepkg_tree = tree
		self.system("sudo makepkg -t %s -C" % makepkg_tree)
		# download sources from our mirror if possible
		self.system("makepkg -t %s -doeuH" % makepkg_tree)
		# clean up duplicated dirs
		for i in ["src", "pkg"]:
			if os.path.exists(i):
				shutil.rmtree(i)
		if self.system("sudo makepkg -t %s -cu" % makepkg_tree):
			self.log(pkg, "makepkg failed")
			try:
				sock = open("%s.log" % pkg.split('/')[3])
				buf = sock.read()
				sock.close()
			except IOError:
				buf = "No log available."
			try:
				server.report_result(config.server_user, config.server_pass, pkg, 1, base64.encodestring(buf))
			except socket.error:
				pass
			self.system("git clean -x -d -f")
			return
		self.system("repoman -t %s -k sync" % tree)
		self.log(pkg, "build finished")
		try:
			server.report_result(config.server_user, config.server_pass, pkg, 0)
		except socket.error:
			pass
		self.system("git clean -x -d -f")

	def log(self, pkg, action):
		self.logsock.write("%s\n" % "; ".join([time.ctime(), pkg, action]))
		self.logsock.flush()
	
	def system(self, cmd):
		logfile = "syncpkgcd-%s.log" % time.strftime("%Y%m%d", time.localtime())
		return os.system("export HOME=%s; %s >> %s 2>&1" % (self.home, cmd, logfile))
	
	def go(self, pkgname):
		for root, dirs, files in os.walk("."):
			for dir in dirs:
				if "_darcs" in root:
					continue
				if ".git" in root:
					continue
				if dir == pkgname:
					os.chdir(os.path.join(root, dir))
					return True
		return False
	def save(self):
		self.log("", "client shutting down")
		self.logsock.close()

	def sleep(self, reason):
		self.log("", "%s, sleeping for %d seconds" % (reason, config.sleep))
		time.sleep(config.sleep)
	
	def log_exception(self):
		type, value, tb = sys.exc_info()
		stype = str(type).split("'")[1]
		self.log("", "Traceback (most recent call last):")
		self.log("", "".join(traceback.format_tb(tb)).strip())
		self.log("", "%s: %s" % (stype, value))
		self.save()

if __name__ == "__main__":
	options = Options()
	try:
		opts, args = getopt.getopt(sys.argv[1:], "dhl:p:u:", ["daemon", "help", "logfile=", "pidfile=", "uid="])
	except getopt.GetoptError:
		options.usage(1)
	for opt, arg in opts:
		if opt in ("-d", "--daemon"):
			options.daemon = True
		elif opt in ("-h", "--help"):
			options.help = True
		elif opt in ("-l", "--logfile"):
			options.logfile = arg
		elif opt in ("-p", "--pidfile"):
			options.pidfile = arg
		elif opt in ("-u", "--uid"):
			options.uid = arg
	if options.help:
		options.usage(0)
	Syncpkgcd(options)
