#! /usr/bin/python

# (c) 2005 Shrift <shrift@frugalware.org>
# etcconfig for Frugalware
# distributed under GPL License

# version 0.1

import os,sys
paclist=[]
editorvars=['EDITOR','ETCONFIG_EDITOR']
editors=['vi','emacs','nano']
a_editors=[]
def checkroot():
	uid=os.geteuid()	
	if uid == 0:
		print
		print"-- EtcUpdater --"   
		print
	else:
		print"Sorry, you must be root to run this script."
		sys.exit()
def checkedit():
	p=0
	for i in editorvars[:2]:
		act=os.getenv(i,1)
		if(act==1): 
		 	print "- $%s is not accessible!" % i
		else:
			print "- $%s is accessibble!" % i
			a_editors.append('$'+i)
	
        for i in os.walk('/usr/bin'):
         files=(i[2])
         for s in files:
		if s in editors:
			a_editors.append(s)
			print "- Found %s as an alternative editor" % s
	        else:
			continue


checkroot()
checkedit()
if '$EDITOR' in a_editors:
     if '$ETCONFIG_EDITOR' in a_editors:
	editor='$ETCONFIG_EDITOR'
     else: editor='$EDITOR'


elif '$ETCONFIG_EDITOR' in a_editors:
	editor='$ETCONFIG_EDITOR'
else:
        editor=a_editors.pop()
        
	
print "- Using <%s> as an editor -" % editor
print '-'*17
print '  Searching...'
print '-'*17
for i in os.walk('/etc'):
 files=(i[2])
 for s in files:
      	if s.endswith(".pacnew"):
		s=i[0]+'/'+s
		paclist.append(s)
		print "Found %s adding to list" % s
	else:
		continue
print '-'*17
print 'Searching done...'
print '-'*17
print
choices=['y','n','d','e','i']
m=len(paclist)
x=0
if len(paclist)==0:
	print('--> There are no new config files available for updateing!')
else:	
	print('-->[(y=update) (n=delete .pacnew) (d=diff old) (e=edit) (i=ignore)]<--')
while x<m:
       k=paclist[x]
       choice=raw_input('-- %s (y,n,d,e,i): ' % k)
       korig=k.split('.pacnew')[0]
       if choice in choices:
	       if(choice == choices[0]):
		       print(k,korig)
		       command='mv --reply=yes %s %s' % (k,korig)
		       os.system("%s" % command)
		       print "Updated!"
	       	       x=x+1
	       elif(choice == choices[1]):
		       command='rm -f %s' % k
		       os.system("%s" % command)
		       print "Deleted .pacnew!"
	       	       x=x+1
	       elif(choice == choices[2]):
		       command='diff -u %s %s|less' % (korig,k)
		       os.system("%s" % command)
		       print "'Diff'-ed!"
	       elif(choice == choices[3]):
		       print "Editing..."
		       command='%s %s' % (editor,k)
		       os.system("%s" % command)
	       	       x=x+1
	       elif(choice == choices[4]):
		       print "Ignored!"
	       	       x=x+1
	       else:
		       continue
