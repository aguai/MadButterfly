#!/usr/bin/python
import inkex
import pygtk
import gtk
from copy import deepcopy

class ConvertToButton(inkex.Effect):
	def effect(self):
		self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
		self.window.set_position(gtk.WIN_POS_MOUSE)
		self.defaultname = 'input symbol name here'
		if self.fillcontent() == False:
			self.window.show_all()
			self.window.connect("delete_event", gtk.main_quit)
			gtk.main()
	def onQuit(self,data):
		gtk.main_quit()
	def onAssign(self,data):
		text = self.text.get_text()
		if text != self.defaultname:
			self.node.set("mbname",text)
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

	def hide_frame(self,frame):
		frame.set('style','display:none')
	def show_frame(self,frame):
		frame.set('style','')
		

	def fillcontent(self):
		if len(self.selected) != 1:
			self.confirm('Please select one group only')
			return False
		for id,node in self.selected.iteritems():
			#self.dump(node)
			name = node.get("mbname")
			if name == None:
				self.confirm("The MadButterFly symbol is not defined yet. Please convert it to the symbol before convert it to button.")
				return False
			for frame in node:
				if frame.get('mbname') == name+'_click':
					self.show_frame(frame)
				else:
					self.hide_frame(frame)
			return True


a=ConvertToButton()
a.affect()

# vim: set ts=4
