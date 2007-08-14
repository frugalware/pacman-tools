#!/usr/bin/env python

import xmlrpclib, time, os, base64, re, sys
sys.path.append("/etc/syncpkgd")
from ctlconfig import config

server = xmlrpclib.Server(config.server_url)

if len(sys.argv) > 1:
	if server.request_build(config.server_user, config.server_pass, sys.argv[1]):
		print "Okay, the daemon will build this package for you."
	else:
		print "Oops, something went wrong. Maybe this package is already in the queue?"
else:
	print """Hello, this is the syncpkgd control client.
If you want to add a new package, just pass its url as a parameter, ie.:
syncpkgdctl 'git://current/frugalwareutils-0.7.4-2-x86_64/VMiklos <vmiklos@frugalware.org>'
(The address is used to notify you in case the build fails. Please use
your address only.)
At the moment the following packages are waiting to be built:"""
	print server.get_todo(config.server_user, config.server_pass)
	print "Please not that this list does not include failed or already started builds."
