from twisted.web import soap
from twisted.internet import reactor
import sys,os
class Inkscape(object):
	def __init__(self):
		self.server = soap.Proxy('http://localhost:8080')
	def PUBLISH(self):
		return self.server.callRemote('PUBLISH')
	def SCENE(self,n):
		return self.server.callRemote('SCENE',n)
	def START(self):
		return self.server.callRemote('START')
	def INSERTKEY(self,layer,n):
		return self.server.callRemote('INSERTKEY',layer,n)
	def EXTENDSCENE(self,layer,n):
		return self.server.callRemote('EXTENDSCENE',layer,n)
	def GETDOC(self):
		return self.server.callRemote('GETDOC')


def quitSession(result):
	print [result]
	reactor.stop()
def quitError(result):
	print "Error"
	print[result]
	reactor.stop()


ink = Inkscape()

if sys.argv[1] == 'PUBLISH':
	d = ink.PUBLISH()
elif sys.argv[1] == 'SCENE':
	d = ink.SCENE(sys.argv[2])
elif sys.argv[1] == 'START':
	d = ink.START()
elif sys.argv[1] == 'INSERTKEY':
	d = ink.INSERTKEY(sys.argv[2], sys.argv[3])
elif sys.argv[1] == 'GETDOC':
	d = ink.GETDOC()
elif sys.argv[1] == 'EXTENDSCENE':
	d = ink.EXTENDSCENE(sys.argv[2],sys.argv[3])
else:
	print 'Unknown command %s' % sys.argv[1]
	sys.exit(-1)
d.addCallback(quitSession)
d.addErrback(quitError)


reactor.run()
