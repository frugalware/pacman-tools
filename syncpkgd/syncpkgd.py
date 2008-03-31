#!/usr/bin/env python

import sys, getopt, os, pwd, sha, time, base64, re, pickle, signal
sys.path.append("/etc/syncpkgd")
from SimpleXMLRPCServer import SimpleXMLRPCServer
from dconfig import config

class Actions:
	def __init__(self, options):
		self.lags = {}
		self.tobuild = []
		self.options = options
		self.logsock = open(self.options.logfile, "a")
		self.__log("server", "", "server started")
		try:
			sock = open(self.options.statusfile)
			self.tobuild = pickle.load(sock)
			sock.close()
		except IOError:
			pass
		except EOFError:
			pass

	def __log(self, user, pkg, action):
		self.logsock.write("%s\n" % "; ".join([time.ctime(), user, pkg, action]))
		self.logsock.flush()

	def __login(self, login, password):
		if login in config.passes.keys() and \
			sha.sha(password).hexdigest() == config.passes[login]:
				if login not in self.lags.keys():
					self.lags[login] = time.time()
				return True
		return False

	def save(self):
		self.__log("server", "", "server shutting down")
		self.logsock.close()
		sock = open(self.options.statusfile, "w")
		pickle.dump(self.tobuild, sock)
		sock.close()

	def __request_build(self, pkg):
		# this regex is not too nice, but at least works
		if not re.search("^(git|darcs)://[A-Za-z][A-Za-z0-9_]+/.+-[^-]+-[^-]+-[^-]+/.+ <.+>$", pkg):
			raise Exception("invalid uri")
		if pkg in self.tobuild:
			return False
		else:
			self.tobuild.append(pkg)
			return True
	
	def request_build(self, login, password, pkg):
		"""add a package to build. be careful, currently no way to undo it"""
		if not self.__login(login, password):
			return
		if self.__request_build(pkg):
			self.__log(login, pkg, "package accepted by the server")
			return True
		else:
			self.__log(login, pkg, "package rejected by the server")
			return False

	def request_conf(self):
		"""request the up to date repo list"""
		try:
			sock = open(os.path.join(pwd.getpwnam(options.uid).pw_dir, ".repoman.conf"))
			buf = sock.read()
			sock.close()
		except IOError:
			buf = ""
		return base64.encodestring(buf)

	def report_result(self, login, password, pkg, exitcode, log=None):
		"""report the build result of a package"""
		if not self.__login(login, password):
			return
		self.__log(login, pkg, "package build finished with exit code %s" % exitcode)
		path = os.path.join(self.options.clientlogs, login)
		if int(exitcode) == 0:
			try:
				os.unlink(os.path.join(path, "%s.log" % pkg.split('/')[3]))
			except OSError:
				pass
		else:
			# mail the devel about this
			import smtplib
			fro = "Syncpkgd <noreply@frugalware.org>"
			to = pkg.split('/')[4]
			title = "%s failed to build %s" % (login, pkg.split('/')[3])
			msg = "From: %s \nTo: %s\nSubject: %s\n\n" \
					% (fro, to, title)
			msg += """Hello,

The syncpkg client daemon running at '%s' failed to build '%s' for you.
The build log is available at:

http://frugalware.org/buildlogs/%s/%s.log

If you think syncpkgd should try to build again (ie. you know that a missing
dependency is now available), then issue the following command:

ssh genesis.frugalware.org "syncpkgdctl '%s'"

- Syncpkgd""" % (login, pkg.split('/')[3], login, pkg.split('/')[3], pkg)
			s = smtplib.SMTP('localhost')
			s.sendmail(fro, to, msg)
			s.quit()
		if log:
			try:
				os.stat(path)
			except OSError:
				os.makedirs(path)
			sock = open(os.path.join(path, "%s.log" % pkg.split('/')[3]), "a")
			sock.write(base64.decodestring(log))
			sock.close()
		return True

	def __request_pkg(self, arch):
		for i in self.tobuild:
			if i.split("/")[3].split("-")[-1] == arch:
				self.tobuild.remove(i)
				return i
		return ""
	
	def request_pkg(self, login, password, arch):
		"""request a package to build. it's primitive, if the client
		does not request a build after a build failure, the original
		request will be lost"""
		if not self.__login(login, password):
			return
		pkg = self.__request_pkg(arch)
		if len(pkg):
			self.__log(login, pkg, "package accepted by a client")
		return pkg

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
		self.clientlogs = "clientlogs"
		self.daemon = False
		self.pidfile = "syncpkgd.pid"
		self.statusfile = "syncpkgd.status"
		self.logfile = "syncpkgd.log"
		self.help = False
		self.uid = False
	def usage(self, ret):
		os.system("man syncpkgd")
		sys.exit(ret)


class Syncpkgd:
	def __init__(self, options):
		def on_sigterm(num, frame):
			raise KeyboardInterrupt
		signal.signal(signal.SIGTERM, on_sigterm)
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
		actions = Actions(options)
		server.register_instance(actions)
		try:
			server.serve_forever()
		except KeyboardInterrupt:
			actions.save()
			return

if __name__ == "__main__":
	options = Options()
	try:
		opts, args = getopt.getopt(sys.argv[1:], "c:dhl:p:s:u:",
			["clientlogs=", "daemon", "help", "logfile=",
				"pidfile=", "statusfile=", "uid="])
	except getopt.GetoptError:
		options.usage(1)
	for opt, arg in opts:
		if opt in ("-c", "--clientlogs"):
			options.clientlogs = arg
		elif opt in ("-d", "--daemon"):
			options.daemon = True
		elif opt in ("-h", "--help"):
			options.help = True
		elif opt in ("-l", "--logfile"):
			options.logfile = arg
		elif opt in ("-p", "--pidfile"):
			options.pidfile = arg
		elif opt in ("-s", "--statusfile"):
			options.statusfile = arg
		elif opt in ("-u", "--uid"):
			options.uid = arg
	if options.help:
		options.usage(0)
	Syncpkgd(options)
