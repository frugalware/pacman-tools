import xmlrpclib, time, os
from cconfig import config

server = xmlrpclib.Server(config.server_url)

def build(pkg):
	print "simulating pkg build..."
	time.sleep(5)

while True:
	pkg = server.request_pkg(config.server_user, config.server_pass, os.uname()[-1])
	if not len(pkg):
		print "no pkg to build, sleeping"
		time.sleep(config.sleep)
	else:
		build(pkg)
