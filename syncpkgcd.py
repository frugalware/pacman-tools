import xmlrpclib, time, os, getopt, sys, socket, glob
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
		print """Usage: syncpkgcd [OPTION]...
syncpkgcd is a client daemon that fetches requests from a syncpkg daemon.

Options:
	-d	--daemon	run as daemon in the background
	-l	--logfile	set the logfile (default: syncpkgcd.log)
	-p	--pidfile	set the pidfile (default: syncpkgcd.pid)
	-u	--uid		set the daemon's user id"""
		sys.exit(ret)


class Syncpkgcd:
	def __init__(self, options):
		self.options = options
		if os.getuid() == 0 and options.uid:
			try:
				os.setuid(int(options.uid))
			except:
				os.setuid(pwd.getpwnam(options.uid).pw_uid)
		if options.daemon:
			pid = os.fork()
			if pid == 0:
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
		# log
		self.logsock = open(options.logfile, "a")
		self.log("", "client started")

		# main loop
		try:
			while True:
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
				self.build(pkg)
		except KeyboardInterrupt:
			# here we could abort the current build properly
			self.save()
			return

	def build(self, pkg):
		# maybe later support protocolls (the first item) other than git?
		scm = pkg.split('/')[0][:-1]
		tree = pkg.split('/')[2]
		pkgarr = pkg.split('/')[3].split('-')
		pkgname = "-".join(pkgarr[:-3])
		pkgver = "-".join(pkgarr[-3:-1])
		arch = pkgarr[-1]
		self.log(pkg, "starting build")
		sock = os.popen(". ~/.repoman.conf; echo $fst_root; echo $%s_servers" % tree)
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
				self.system("git pull")
				self.system("git checkout -f")
			elif scm == "darcs":
				self.system("darcs pull -a")
				self.system("darcs revert -a")
		except OSError:
			if scm == "git":
				self.system("git clone %s %s" % (url, tree))
			elif scm == "darcs":
				self.system("darcs get --partial" % (url, tree))
			try:
				os.chdir(tree)
			except OSError:
				self.log(pkg, "failed to get the repo")
				return
		self.go(pkgname)
		if scm == "git":
			self.system("git clean -x -d")
		elif scm == "darcs":
			junk = []
			junk.append(glob.glob("*.fpm"))
			junk.append(glob.glob("*.log"))
			junk.append(glob.glob("*.log.bz2"))
			for i in junk:
				os.unlink(junk)
		self.system("sudo makepkg -t %s -C" % tree)
		if self.system("sudo makepkg -t %s -cu" % tree):
			self.log(pkg, "makepkg failed")
			server.report_result(config.server_user, config.server_pass, pkg, 1)
			return
		self.system("repoman -t %s -k sync" % tree)
		self.log(pkg, "build finished")
		server.report_result(config.server_user, config.server_pass, pkg, 0)

	def log(self, pkg, action):
		self.logsock.write("%s\n" % "; ".join([time.ctime(), pkg, action]))
		self.logsock.flush()
	
	def system(self, cmd):
		logfile = "syncpkgcd-%s.log" % time.strftime("%Y%m%d", time.localtime())
		return os.system("%s >> %s 2>&1" % (cmd, logfile))
	
	def go(self, pkgname):
		for root, dirs, files in os.walk("."):
			for dir in dirs:
				if dir == pkgname:
					os.chdir(os.path.join(root, dir))
					return
		raise Exception("can't find package '%s'")
	def save(self):
		self.log("", "client shutting down")
		self.logsock.close()

	def sleep(self, reason):
		self.log("", "%s, sleeping for %d seconds" % (reason, config.sleep))
		time.sleep(config.sleep)

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
			options.log = arg
		elif opt in ("-p", "--pidfile"):
			options.pidfile = arg
		elif opt in ("-u", "--uid"):
			options.uid = arg
	if options.help:
		options.usage(0)
	Syncpkgcd(options)
