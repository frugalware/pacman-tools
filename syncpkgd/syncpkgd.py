#!/usr/bin/env python

import sys, getopt, os, pwd, hashlib, time, base64, re, pickle, signal, glob
sys.path.append("/etc/syncpkgd")
from xmlrpc.server import SimpleXMLRPCServer
import xmlrpc.client
from dconfig import config

class Actions:
    def __init__(self, options):
        self.lags = {}
        self.tobuild = []
        self.options = options
        self.logsock = open(self.options.logfile, "a")
        self.__log("server", "", "server started")
        try:
            sock = open(self.options.statusfile, "rb")
            self.tobuild = pickle.load(sock)
            sock.close()
        except IOError:
            pass
        except EOFError:
            pass

    def __log(self, user, pkg, action):
        self.logsock.write("%s\n" % "; ".join([time.ctime(), user, pkg, action]))
        self.logsock.flush()

    def log_exception(self):
        type, value, tb = sys.exc_info()
        stype = str(type).split("'")[1]
        self.__log("server", "", "Traceback (most recent call last):")
        self.__log("server", "", "".join(traceback.format_tb(tb)).strip())
        self.__log("server", "", "%s: %s" % (stype, value))
        self.save()

    def __login(self, login, password):
        if login in list(config.passes.keys()) and \
            hashlib.sha1(password.encode()).hexdigest() == config.passes[login]:
                if login not in list(self.lags.keys()):
                    self.lags[login] = time.time()
                return True
        return False

    def save(self):
        self.__log("server", "", "server shutting down")
        self.logsock.close()
        sock = open(self.options.statusfile, "wb")
        pickle.dump(self.tobuild, sock)
        sock.close()

    def __request_build(self, pkg):
        # this regex is not too nice, but at least works
        if not re.search("^(git|darcs)://[^/]+/[^/]+-[^-]+-[^-]+-[^-]+/[^/]+ <[^/]+>$", pkg):
            raise Exception("invalid uri")
        if pkg in self.tobuild:
            self.__log(pkg, "already building package")
            return False
        else:
            self.tobuild.append(pkg)
            return True
    
    def request_build(self, login, password, pkg):
        """add a package to build"""
        if not self.__login(login, password):
            self.__log(login, "", "unable to login")
            return False
        if self.__request_build(pkg):
            self.__log(login, pkg, "package accepted by the server")
            return True
        else:
            self.__log(login, pkg, "package rejected by the server")
            return False

    def __cancel_build(self, pkg):
        if pkg not in self.tobuild:
            return False
        else:
            self.tobuild.remove(pkg)
            return True
    
    def cancel_build(self, login, password, pkg):
        """delete a package from the build queue, if it's there yet (ie the build is not yet started)"""
        if not self.__login(login, password):
            return False
        if self.__cancel_build(pkg):
            self.__log(login, pkg, "package deletion request was accepted by the server")
            return True
        else:
            self.__log(login, pkg, "package deletion request was rejected by the server")
            return False

    def __get_conf_list(self):
        """build a list of conf files to transfer"""
        confs = ['.repoman.conf', '.pacman-g2/repos/*']
        home = pwd.getpwnam(options.uid).pw_dir

        l = []
        for i in confs:
            res = glob.glob(os.path.join(home, i))
            for j in res:
                l.append(j.replace(home + os.path.sep, ''))
        return l

    def __get_base64(self, path):
        """return a path as a base64-encoded string"""
        try:
            sock = open(os.path.join(pwd.getpwnam(options.uid).pw_dir, path), "rb")
            buf = sock.read()
            sock.close()
        except IOError:
            buf = ""
        return xmlrpc.client.Binary(buf)

    def request_confs(self):
        """request the up to date config files"""
        ret = {}
        for i in self.__get_conf_list():
            ret[i] = self.__get_base64(i)
        return ret

    def request_conf(self):
        """request the up to date repo list"""
        try:
            sock = open(os.path.join(pwd.getpwnam(options.uid).pw_dir, ".repoman.conf"))
            buf = sock.read()
            sock.close()
        except IOError:
            buf = ""
        return xmlrpc.client.Binary(buf)

    def report_result(self, login, password, pkg, exitcode, log=None):
        """report the build result of a package"""
        if not self.__login(login, password):
            return False
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

The syncpkg client service running at '%s' failed to build '%s' for you.
The build log is available at:

http://frugalware.org/buildlogs/%s/%s.log

If you think syncpkgd should try to build again (ie. you know that a missing
dependency is now available), then issue the following command:

ssh frugalware.org "syncpkgdctl '%s'"

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
            sock.write(log)
            sock.close()
        return True

    def __request_pkg(self, arch):
        for i in self.tobuild:
            if i.split("/")[3].split("-")[-1] == arch:
                self.tobuild.remove(i)
                return i
        return []
    
    def request_pkg(self, login, password, arch):
        """request a package to build. it's primitive, if the client
        does not request a build after a build failure, the original
        request will be lost"""
        if not self.__login(login, password):
            return False
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
            return []
        return self.__get_todo(arch)

    def __who(self):
        ret = []
        for k, v in list(self.lags.items()):
            if (time.time() - v) < 300:
                ret.append(k)
        return ret
    
    def who(self, login, password):
        """who is online? query function"""
        if not self.__login(login, password):
            return False
        return self.__who()

class Options:
    def __init__(self):
        self.clientlogs = "clientlogs"
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
        self.options = options
        def on_sigterm(num, frame):
            raise KeyboardInterrupt
        signal.signal(signal.SIGTERM, on_sigterm)
        self.setuid()
        server = SimpleXMLRPCServer(('',1873))
        actions = Actions(self.options)
        server.register_instance(actions)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            actions.save()
            return None
        except Exception as ex:
            actions.log_exception()
            return None
    
    def setuid(self):
        if os.getuid() == 0 and self.options.uid:
            try:
                os.setuid(int(self.options.uid))
            except:
                os.setuid(pwd.getpwnam(self.options.uid).pw_uid)

if __name__ == "__main__":
    options = Options()
    try:
        opts, args = getopt.getopt(sys.argv[1:], "c:hl:p:s:u:",
            ["clientlogs=", "help", "logfile=",
                "pidfile=", "statusfile=", "uid="])
    except getopt.GetoptError:
        options.usage(1)
    for opt, arg in opts:
        if opt in ("-c", "--clientlogs"):
            options.clientlogs = arg
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
