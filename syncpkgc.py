import xmlrpclib
syncd = xmlrpclib.Server("http://localhost:1873")

print syncd.request_build("user", "pass", "pkg", "i686")
print syncd.request_build("user", "pass", "pkg", "x86_64")
print syncd.get_todo("user", "pass")
