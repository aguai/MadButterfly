#!/usr/bin/python
from twisted.web import server, resource,soap
from twisted.internet import reactor,defer
import os,time




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
	def soap_SCENE(self,n):
		if self.client == None:
			os.kill(self.pid,12)
			time.sleep(1)
			self.client = Client()

		d = defer.Deferred()
		self.client.SCENE(n).addCallback(self.generic_return,d)
		return d
	def generic_return(self,result,d):
		print [result]
		d.callback(result)
	def soap_START(self):
		if self.client == None:
			os.kill(self.pid,12)
			time.sleep(1)
			self.client = Client()
		return "OK"
		



class Client(object):
	def __init__(self):
		self.proxy = soap.Proxy('http://localhost:8080')	
	def PUBLISH(self):
		return self.proxy.callRemote('PUBLISH')
	def SCENE(self,n):
		return self.proxy.callRemote('SCENE',n)
pid = os.fork()	
if pid==0:
	os.execvp("inkscape-mb",["inkscape-mb","scene.svg"])
s = Server()
s.client = None
s.pid = pid
site = server.Site(s)
reactor.listenTCP(19192,site)
reactor.run()
		

