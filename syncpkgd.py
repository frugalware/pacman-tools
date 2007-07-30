import sys, getopt, os, pwd, sha, time, base64, re
from SimpleXMLRPCServer import SimpleXMLRPCServer
from config import config

class Actions:
	def __init__(self):
		self.lags = {}
		self.tobuild = []

	def __login(self, login, password):
		if login in config.passes.keys() and \
			sha.sha(password).hexdigest() == config.passes[login]:
				if login not in self.lags.keys():
					self.lags[login] = time.time()
				return True
		return False

	def __request_build(self, pkg, arch):
		p = "%s-%s" % (pkg, arch)
		if p in self.tobuild:
			return False
		else:
			self.tobuild.append(p)
			return True
	
	def request_build(self, login, password, pkg, arch):
		"""add a package to build. be careful, currently no way to undo it"""
		if not self.__login(login, password):
			return
		return self.__request_build(pkg, arch)

	def __get_todo(self, arch=None):
		if not arch:
			return self.tobuild
		else:
			ret = []
			for i in self.tobuild:
				if re.match(".*-%s$" % arch, i):
					ret.append(i)
			return ret
	
	def get_todo(self, login, password, arch=None):
		"""what's to be done? query function"""
		if not self.__login(login, password):
			return
		return self.__get_todo(arch)

	def __who(self):
		ret = []
		for k, v in self.lags.items():
			if (time.time() - v) < 300:
				ret.append(k)
		return ret
	
	def who(self, login, password):
		"""who is online? query function"""
		if not self.__login(login, password):
			return
		return self.__who()

class Options:
	def __init__(self):
		self.daemon = False
		self.pidfile = "syncpkgd.pid"
		self.help = False
		self.uid = False
	def usage(self, ret):
		print """Usage: syncpkgd [OPTION]...
syncpkgd is a daemon that accepts requests from syncpkg clients.

Options:
	-d	--daemon	run as daemon in the background
	-p	--pidfile	set the pidfile (default: syncpkgd.pid)
	-u	--uid		set the daemon's user id"""
		sys.exit(ret)


class Syncpkgd:
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
		server = SimpleXMLRPCServer(('',1873))
		server.register_instance(Actions())
		# TODO: dump todo list once the api is stable
		server.serve_forever()

if __name__ == "__main__":
	options = Options()
	try:
		opts, args = getopt.getopt(sys.argv[1:], "dhp:u:", ["daemon", "help", "pidfile=", "uid="])
	except getopt.GetoptError:
		options.usage(1)
	for opt, arg in opts:
		if opt in ("-d", "--daemon"):
			options.daemon = True
		elif opt in ("-h", "--help"):
			options.help = True
		elif opt in ("-p", "--pidfile"):
			options.pidfile = arg
		elif opt in ("-u", "--uid"):
			options.uid = arg
	if options.help:
		options.usage(0)
	Syncpkgd(options)
