#!/usr/bin/python
from twisted.web import server, resource,soap
from twisted.internet import reactor,defer
import os,time,sys
import traceback




class Server(soap.SOAPPublisher):
    	"""
	    SOAP server for inkscape extension.
	"""
        def soap_PUBLISH(self):
		if self.client == None:
			os.kill(self.pid,12)
			time.sleep(1)
			self.client = Client()
		d = defer.Deferred()
		self.client.PUBLISH().addCallback(self.quit,d)
		return d
			
	def quit(self,result,d):
		print [result]
		d.callback(result)
		self.client = None
	def soap_INSERTKEY(self,layer,n):
		if self.client == None:
			os.kill(self.pid,12)
			time.sleep(1)
			self.client = Client()
		try:
			n = int(n)
		except:
			n = 0
		d = defer.Deferred()
		self.client.INSERTKEY(layer,n).addCallback(self.generic_return,d).addErrback(self.generic_error,d)
		return d
	def soap_EXTENDSCENE(self,layer,n):
		if self.client == None:
			os.kill(self.pid,12)
			time.sleep(1)
			self.client = Client()
		try:
			n = int(n)
		except:
			n = 0
		d = defer.Deferred()
		self.client.EXTENDSCENE(layer,n).addCallback(self.generic_return,d).addErrback(self.generic_error,d)
		return d
	def soap_DELETESCENE(self,layer,n):
		if self.client == None:
			os.kill(self.pid,12)
			time.sleep(1)
			self.client = Client()
		try:
			n = int(n)
		except:
			n = 0
		d = defer.Deferred()
		self.client.DELETESCENE(layer,n).addCallback(self.generic_return,d).addErrback(self.generic_error,d)
		return d

	def soap_SCENE(self,n):
		if self.client == None:
			os.kill(self.pid,12)
			time.sleep(1)
			self.client = Client()

		d = defer.Deferred()
		self.client.SCENE(n).addCallback(self.generic_return,d)
		return d
	def generic_return(self,result,d):
		print "Get result:"
		print [result]
		d.callback(result)
	def generic_error(self,result,d):
		print "Error:"
		print [result]
		d.errback(result)
	def soap_START(self):
		if self.client == None:
			os.kill(self.pid,12)
			time.sleep(1)
			self.client = Client()
		return "OK"
	def getdoc_error(self,result,d):
		try:
			print "reconnect"
			time.sleep(1)
			os.kill(self.pid,12)
			op = self.client.GETDOC()
			op.addCallback(self.generic_return,d)
			op.addErrback(self.getdoc_error,d)
		except:
			traceback.print_exc()
	def soap_GETDOC(self):
		try:
			print "xxxxx"
			if self.client == None:
				while os.kill(self.pid,12)<0:
					time.sleep(1)
			
				self.client = Client()
			d = defer.Deferred()
			op = self.client.GETDOC()
			op.addCallback(self.generic_return,d)
			op.addErrback(self.getdoc_error,d)
			print "yyy"
			return d
		except:
			traceback.print_exc()
		



class Client(object):
	def __init__(self):
		self.proxy = soap.Proxy('http://localhost:8080')	
	def PUBLISH(self):
		return self.proxy.callRemote('PUBLISH')
	def SCENE(self,n):
		return self.proxy.callRemote('SCENE',n)
	def INSERTKEY(self,layer,n):
		return self.proxy.callRemote('INSERTKEY',layer,n)
	def GETDOC(self):
		doc = self.proxy.callRemote('GETDOC')
		return doc
	def EXTENDSCENE(self,layer,n):
		return self.proxy.callRemote('EXTENDSCENE',layer,n)
	def DELETESCENE(self,layer,n):
		return self.proxy.callRemote('DELETESCENE',layer,n)

os.system("killall -9 inkscape-mb")
try:
	f = open("/tmp/madbuilder.pid","r")
	pid = int(f.read())
	f.close()
	os.system("kill -9 %d" % pid)
	f = open("/tmp/madbuilder.pid","w")
	f.write("%d\n" % os.getpid())
	f.close()
except:
	traceback.print_exc()

pid = os.fork()	
if pid==0:
	os.execvp("inkscape-mb",["inkscape-mb",sys.argv[1]])
s = Server()
s.client = None
s.pid = pid
site = server.Site(s)
# Sleep for two seconds to wait for the inkscape become ready. It seems that our inkscape modification has
# semaphore issue so that it will be blocked if we connect to it in early stage. We need to check it and 
# remove the following wait in the future.
reactor.listenTCP(19192,site)
time.sleep(5)
reactor.run()
		

