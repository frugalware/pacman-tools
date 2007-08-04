import xmlrpclib
syncd = xmlrpclib.Server("http://localhost:1873")

#syncd.request_build("user", "pass", "pkg-1.0-1-i686")
#syncd.request_build("user", "pass", "pkg-1.0-1-x86_64")
#print syncd.get_todo("user", "pass")
#print syncd.request_pkg("user", "pass", "i686")
