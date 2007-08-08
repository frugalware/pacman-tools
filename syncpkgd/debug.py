import xmlrpclib, time, os, base64

server_url = "http://frugalware.org/~vmiklos/syncd2/"
server_user = "user"
server_pass = "pass"
server = xmlrpclib.Server(server_url)

print server.request_build(server_user, server_pass, "git://current/frugalware-0.7pre2-1-i686")
