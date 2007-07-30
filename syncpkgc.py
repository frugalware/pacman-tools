import xmlrpclib
testsvr = xmlrpclib.Server("http://localhost:1873")

print testsvr.who("user", "pass")
