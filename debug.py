import xmlrpclib, time, os

server_url = "http://frugalware.org/~vmiklos/syncd2/"
server_user = "debug"
server_pass = "$e89tmuCA"
server = xmlrpclib.Server(server_url)

print server.request_build(server_user, server_pass, "git://current/frugalware-0.7pre2-1-i686")
#print server.get_todo(server_user, server_pass)
