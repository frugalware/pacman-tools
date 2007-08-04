import xmlrpclib, time, os, getopt, sys, socket
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
				if not len(pkg):
					self.sleep("no package to build")
					continue
				self.build(pkg)
		except KeyboardInterrupt:
			# TODO: abort the current build properly
			self.save()
			return

	def build(self, pkg):
		self.log(pkg, "starting build")
		# FIXME
		time.sleep(5)
		# TODO: exit code
		self.log(pkg, "build finished")

	def log(self, pkg, action):
		self.logsock.write("%s\n" % "; ".join([time.ctime(), pkg, action]))
		self.logsock.flush()
	
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
