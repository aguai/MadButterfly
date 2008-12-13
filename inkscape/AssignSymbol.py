#!/usr/bin/python
import inkex
import pygtk
import gtk

class AssignSymbol(inkex.Effect):
	def effect(self):
		self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
		self.window.set_position(gtk.WIN_POS_MOUSE)
		self.defaultname = 'input symbol name here'
		self.fillcontent()
		self.window.show_all()
		self.window.connect("delete_event", gtk.main_quit)
		gtk.main()
	def onQuit(self,data):
		gtk.main_quit()
	def onAssign(self,data):
		text = self.text.get_text()
		if text != self.defaultname:
			self.node.set("mbname",text)
			self.node.set("id",text)
		gtk.main_quit()
		
	def confirm(self,msg):
		vbox = gtk.VBox()
		vbox.pack_start(gtk.Label(msg))
		self.button = gtk.Button('OK')
		vbox.pack_start(self.button)
		self.button.connect("clicked", self.onQuit)
		self.window.add(vbox)
	def dumpattr(self,n):
		s = ""
		for a,v in n.attrib.items():
			s = s + ("%s=%s"  % (a,v))
		return s
			
	def dump(self,node,l=0):
		print " " * l*2,"<", node.tag, self.dumpattr(node),">"
		for n in node:
			self.dump(n,l+1)
		print " " * l * 2,"/>"

	def fillcontent(self):
		if len(self.selected) != 1:
			self.confirm('Please select on group only')
			return
		for id,node in self.selected.iteritems():
			#self.dump(node)
			self.node = node
			vbox = gtk.VBox()
			vbox.pack_start(gtk.Label('Please input the symbol name'))
			self.text = gtk.Entry()
			try:
				self.text.set_text(node.get("mbname"))
			except:
				self.text.set_text(self.defaultname)
			vbox.pack_start(self.text)
			self.button = gtk.Button('OK')
			self.button.connect("clicked", self.onAssign)
			vbox.pack_start(self.button)
			self.window.add(vbox)
			self.window.show_all()
			
		
		

a=AssignSymbol()
a.affect()

# vim: set ts=4
