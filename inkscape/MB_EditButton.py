#!/usr/bin/python
import inkex
import pygtk
import gtk
from copy import deepcopy
from lxml import etree
import os
import tempfile

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

	def EditNormalButton(self,event,node):
		self.EditButton(node,'_normal')

	def EditActiveButton(self,event,node):
		self.EditButton(node,'_active')

	def EditClickButton(self,event,node):
		self.EditButton(node,'_click')

	def EditFrame(self,node):
		# Generate a SVG file with node and invoke inkscape to edit it
		svg = etree.Element('svg')
		for n in node:
			svg.append(deepcopy(n))
		fd,fname = tempfile.mkstemp(suffix='.svg')
		f = os.fdopen(fd,"w")
		f.write(etree.tostring(svg))
		f.close()
		os.system("inkscape %s >/dev/null 2>/dev/null" % fname)
		svg = etree.parse(fname)
		os.unlink(fname)
		newnode=[]
		for n in svg.getroot():
			if n.tag == '{http://www.w3.org/2000/svg}g':
				newnode.append(n)
			if n.tag == '{http://www.w3.org/2000/svg}rect':
				newnode.append(n)
			if n.tag == '{http://www.w3.org/2000/svg}text':
				newnode.append(n)
		return newnode
				


		
	def duplicateAttribute(self,new,old):
		for k,v in old.attrib.items():
			new.set(k,v)


	def EditButton(self,node,mode):
		name = node.get('mbname')
		for frame in node:
			if frame.get('mbname') == name+mode:
				newnode = self.EditFrame(frame)
				oldframe = deepcopy(frame)
				frame.clear()
				self.duplicateAttribute(frame,oldframe)
				for n in newnode:
					frame.append(n)
				return
	def DisplayNormalButton(self,event,node):
		self.displayButton(node,'_normal')
	def DisplayActiveButton(self,event,node):
		self.displayButton(node,'_active')
	def DisplayClickButton(self,event,node):
		self.displayButton(node,'_click')
	def displayButton(self,node,mode):
		name = node.get('mbname')
		for n in node:
			if n.get('mbname') == name+mode:
				n.set('style','')
			else:
				n.set('style','display:none')
		gtk.main_quit()
		
		

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
			type = node.get("mbtype")
			if type != 'button':
				self.confirm('This is not a button')
				return False
			hbox = gtk.HBox()
			self.window.add(hbox)
			button = gtk.Button('Edit Normal')
			hbox.pack_start(button)
			button.connect("clicked", self.EditNormalButton,node)
			button = gtk.Button('Edit Active')
			hbox.pack_start(button)
			button.connect("clicked", self.EditActiveButton,node)
			button = gtk.Button('Edit Click')
			hbox.pack_start(button)
			button.connect("clicked", self.EditClickButton,node)
			button = gtk.Button('Display Normal')
			hbox.pack_start(button)
			button.connect("clicked", self.DisplayNormalButton,node)
			button = gtk.Button('Display Active')
			hbox.pack_start(button)
			button.connect("clicked", self.DisplayActiveButton,node)
			button = gtk.Button('Display Click')
			hbox.pack_start(button)
			button.connect("clicked", self.DisplayClickButton,node)
			return False


a=ConvertToButton()
a.affect()

# vim: set ts=4
