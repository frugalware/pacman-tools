from xml.dom import minidom
import sys

class Mirror (object):
	def __init__(self):
		self.types = []
	def dumpattr(self, what, dest):
		dest.write("# ")
		for i in range(len(what)+4):
			dest.write("-")
		dest.write("\n")
		dest.write("# - %s -\n" % what.upper())
		dest.write("# ")
		for i in range(len(self.country)+4):
			dest.write("-")
		dest.write("\n")

mirrors = {}
try:
	ver = sys.argv[1]
	repo = sys.argv[2]
	input = sys.argv[3]
	out = sys.argv[4]
except IndexError:
	raise Exception("usage example: python %s frugalware-stable frugalware input.xml output" % sys.argv[0])

xmldoc = minidom.parse(input)
for i in xmldoc.getElementsByTagName('mirror'):
	if not i.getElementsByTagName('id'):
		continue
	m = Mirror()
	for j in i.getElementsByTagName('type'):
		m.types.append(j.firstChild.toxml())
	for j in ['id', 'path', 'rsync_path', 'country', 'supplier', 'bandwidth']:
		if i.getElementsByTagName(j) and i.getElementsByTagName(j)[0].firstChild:
			m.__setattr__(j, i.getElementsByTagName(j)[0].firstChild.toxml())
	try:
		mirrors[m.country]
	except KeyError:
		mirrors[m.country] = []
	mirrors[m.country].append(m)
countries = list(mirrors.keys())
countries.sort()
sock = open(out, "w")
sock.write("""#
# %s repository
#

repos=(${repos[@]} '%s')

%s_servers=(
""" % (ver, out, out))
for i in countries:
	dumped = False
	for j in mirrors[i]:
		if "rsync" in j.types:
			try:
				j.path = j.rsync_path
			except AttributeError:
				pass
		else:
			continue
		if not dumped:
			mirrors[i][0].dumpattr(mirrors[i][0].country, sock)
			dumped = True
		sock.write("# %s (%s)\n" % (j.supplier, j.bandwidth))
		sock.write('"rsync://rsync%s.frugalware.org/%s/%s"\n' %
				(j.id, j.path, ver))
sock.write(""")

# the rest is only for developers who upload packages
# if the server requires sudo usage, the name of the user to execute the commands as
%s_sudo="repo"
# name of the package database
%s_fdb="%s.fdb"
""" % (out, out, repo))
sock.close()
