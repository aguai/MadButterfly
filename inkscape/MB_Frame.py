#!/usr/bin/python
import inkex
import pygtk
import gtk
from copy import deepcopy
from lxml import etree
import random

# Please refer to http://www.assembla.com/wiki/show/MadButterfly/Inkscape_extention for the designed document.


# Algorithm:
# 
# We will parse the first two level of the SVG DOM. collect a table of layer and scene.
# 1. Collect the layer table which will be displayed as the first column of the grid.
# 2. Get the maximum scene number. This will decide the size of the grid. 
# 3. When F6 is pressed, we will check if this scene has been defined. This can be done by scan all second level group and check if the current scene number is within the 
#    range specified by scene field. The function IsSceneDefined(scene) can be used for this purpose.
# 4. If this is a new scene, we will append a new group which duplication the content of the last scene in the same group. The scene field will contain the number from the 
#    last scene number of the last scene to the current scenen number. For example, if the last scene is from 4-7 and the new scene is 10, we will set the scene field as
#    "8-10".
# 5. If this scene are filled screne, we will split the existing scene into two scenes with the same content.

class Layer:
	def __init__(self,node):
		self.scene = []
		self.node = node
		self.nodes=[]
class Scene:
	def __init__(self, node, start,end):
		self.node = node
		self.start = int(start)
		self.end = int(end)


class MBScene(inkex.Effect):
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
	def parseMetadata(self,node):
		for n in node:
			if n.tag == '{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scenes':
				self.scenemap={}
				cur = int(n.get("current"))
				self.current = cur

				for s in n:
					if s.tag == '{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scene':
						try:
							start = int(s.get("start"))
						except:
							continue
						try:
							end = s.get("end")
							if end == None:
								end = start
						except:
							end = start
						link = s.get("ref")
						self.scenemap[link] = [int(start),int(end)]
						if cur >= start and cur <= end:
							self.currentscene = link

					pass
				pass
			pass
		pass
						
						

	def parseScene(self):
		"""
		In this function, we will collect all items for the current scene and then relocate them back to the appropriate scene object.
		"""
		self.layer = []
		for node in self.document.getroot():
			if node.tag == '{http://www.w3.org/2000/svg}metadata':
				self.parseMetadata(node)
			elif node.tag == '{http://www.w3.org/2000/svg}g':
				oldscene = None
				#print layer.attrib.get("id")
				lyobj = Layer(node)
				self.layer.append(lyobj)
				lyobj.current_scene = []
				for scene in node:
					if scene.tag == '{http://www.w3.org/2000/svg}g':
						try:
							scmap = self.scenemap[scene.get("id")]
							if scmap == None:
								lyobj.current_scene.append(scene)
								continue
							if self.current <= scmap[1] and self.current >= scmap[0]:
								oldscene = scene
						except:
							lyobj.current_scene.append(scene)
							continue

						lyobj.scene.append(Scene(scene,scmap[0],scmap[1]))
					else:
						lyobj.current_scene.append(scene)
					pass
				pass

				if oldscene != None:
					# Put the objects back to the current scene
					for o in lyobj.current_scene:
						#print o.tag
						oldscene.append(o)
					pass
				pass
			pass
		pass

		self.collectID()
		#self.dumpID()
	def collectID(self):
		self.ID = {}
		root = self.document.getroot()
		for n in root:
			self.collectID_recursive(n)
	def collectID_recursive(self,node):
		self.ID[node.get("id")] = 1
		for n in node:
			self.collectID_recursive(n)
	def newID(self):
		while True:
			n = 's%d' % int(random.random()*10000)
			#print "try %s" % n
			if self.ID.has_key(n) == False:
				return n
	def dumpID(self):
		for a,v in self.ID.items():
			print a

		
	def getLayer(self, layer):
		for l in self.layer:
			if l.node.attrib.get("id") == layer:
				return l
		return None
		
	
	def insertKeyScene(self):
		"""
		Insert a new key scene into the stage. If the nth is always a key scene, we will return without changing anything. 
		If the nth is a filled scene, we will break the original scene into two parts. If the nth is out of any scene, we will
		append a new scene.

		"""
		nth = self.last_cell.nScene
		layer = self.getLayer(self.last_cell.layer)
		x = self.last_cell.nScene
		y = self.last_cell.nLayer
		if layer == None: return
		for i in range(0,len(layer.scene)):
			s = layer.scene[i]
			if nth >= s.start and nth <= s.end:
				if nth == s.start: return
				newscene = Scene(deepcopy(s.node),nth,s.end)
				newscene.node.set("id", self.newID())
				layer.scene.insert(i+1,newscene)
				layer.scene[i].end = nth-1
				btn = self.newCell('start.png')
				btn.nScene = nth
				btn.layer = layer
				btn.nLayer = y
				self.grid.remove(self.last_cell)
				self.grid.attach(btn, x,x+1,y,y+1,0,0,0,0)
				return
		if len(layer.scene) > 0:
			last = nth
			lastscene = None
			for s in layer.scene:
				if s.end < nth and last < s.end:
					last = s.end
					lastscene = s
			for x in range(last+1, nth):
				btn = self.newCell('fill.png')
				btn.nScene = x
				btn.layer = layer.node.get('id')
				btn.nLayer = y
				self.grid.attach(btn, x, x+1, y , y+1,0,0,0,0)
			if lastscene == None:
				node = etree.Element('{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scene')
				node.set("id", self.newID())
				newscene = Scene(node,nth,nth)
			else:
				lastscene.end = nth-1
				newscene = Scene(deepcopy(lastscene.node),nth,nth)
				newscene.node.set("id",self.newID())
			layer.scene.append(newscene)
			btn = self.newCell('start.png')
			x = self.last_cell.nScene
			y = self.last_cell.nLayer
			btn.nScene = nth
			btn.layer = layer.node.get('id')
			btn.nLayer = y
			self.grid.attach(btn, x, x+1, y, y+1,0,0,0,0)
		else:
			# This is the first scene in the layer
			node = etree.Element('{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scene')
			node.set("id", self.newID())
			newscene = Scene(node,nth,nth)
			layer.scene.append(newscene)
			btn = self.newCell('start.png')
			btn.nScene = nth
			btn.layer = layer
			btn.nLayer = y
			self.grid.attach(btn, x, x+1, y, y+1,0,0,0,0)




	def removeKeyScene(self):
		nth = self.last_cell.nScene
		try:
			layer = self.getLayer(self.last_cell.layer.node.get('id'))
		except:
			return
		x = self.last_cell.nScene
		y = self.last_cell.nLayer
		for i in range(0,len(layer.scene)):
			s = layer.scene[i]
			if nth == s.start:
				if i == 0:
					for j in range(s.start,s.end+1):
						btn = self.newCell('empty.png')
						btn.nScene = nth
						btn.layer = layer
						btn.nLayer = y
						self.grid.attach(btn, j,j+1,y,y+1,0,0,0,0)
					layer.scene.remove(s)
				else:
					if s.start == layer.scene[i-1].end+1:
						# If the start of the delete scene segment is the end of the last scene segmenet, convert all scenes in the deleted
						# scene segmenet to the last one
						layer.scene[i-1].end = s.end
						layer.scene.remove(s)
						btn = self.newCell('fill.png')

						btn.nScene = nth
						btn.layer = layer
						btn.nLayer = y
						self.grid.attach(btn, x,x+1,y,y+1,0,0,0,0)
					else:
						# Convert all scenes into empty cell
						layer.scene.remove(s)
						for j in range(s.start,s.end+1):
							btn = self.newCell('empty.png')
							btn.nScene = nth
							btn.layer = layer
							btn.nLayer = y
							self.grid.attach(btn, j,j+1,y,y+1,0,0,0,0)

						
				return

	def extendScene(self,layer,nth):
		layer = self.getLayer(layer)
		if layer == None: return
		for i in range(0,len(layer.scene)-1):
			s = layer.scene[i]
			if nth >= layer.scene[i].start and nth < layer.scene[i+1].start:
				layer.scene[i].end = nth
		if len(layer.scene) > 0:
			layer.scene[len(layer.scene)-1].end = nth
	def setCurrentScene(self,nth):
		self.current = nth
		for layer in self.layer:
			for s in layer.scene:
				if nth >= s.start and nth <= s.end:
					s.node.set("style","")
					#print "Put the elemenets out"
					layer.nodes = []

					for o in s.node:
						#print "    ",o.tag
						layer.nodes.append(o)
					for o in s.node:
						s.node.remove(o)
				else:
					s.node.set("style","display:none")
	def generate(self):
		newdoc = deepcopy(self.document)
		root = newdoc.getroot()
		has_scene = False
		for n in root:
			if n.tag == '{http://www.w3.org/2000/svg}metadata':
				for nn in n:
					if nn.tag == '{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scenes':
						nn.clear()
						nn.set("current", "%d" % self.current)
						scenes = []
						for l in self.layer:
							for s in l.scene:
								id = s.node.get("id")
								scene = etree.Element('{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scene')
								scene.set("ref", id)
								if s.start == s.end:
									scene.set("start", "%d" % s.start)
								else:
									scene.set("start", "%d" % s.start)
									scene.set("end", "%d" % s.end)

								scenes.append(scene)
						for s in scenes:
							nn.append(s)
						has_scene = True
				if has_scene == False:
					scenes = etree.Element('{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scenes')
					scenes.set("current","%d" % self.current)
					for l in self.layer:
						for s in l.scene:
							scene = etree.Element('{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scene')
							scene.set("ref", s.node.get("id"))
							if s.start == s.end:
								scene.set("start", "%d" % s.start)
							else:
								scene.set("start", "%d" % s.start)
								scene.set("end", "%d" % s.end)
							scenes.append(scene)
					n.append(scenes)
			if n.tag ==  '{http://www.w3.org/2000/svg}g':
				root.remove(n)

		for l in self.layer:
			# Duplicate all attribute of the layer
			lnode = etree.Element("{http://www.w3.org/2000/svg}g")
			for a,v in l.node.attrib.items():
				lnode.set(a,v)
			for n in l.nodes:
				lnode.append(n)
			root.append(lnode)
			for s in l.scene:
				snode = etree.Element("{http://www.w3.org/2000/svg}g")
				for a,v in s.node.attrib.items():
					snode.set(a,v)
				for n in s.node:
					snode.append(deepcopy(n))
				lnode.append(snode)
		self.document = newdoc
	def newCell(self,file):
		img = gtk.Image()
		img.set_from_file(file)
		btn = gtk.EventBox()
		btn.add(img)
		btn.connect("button_press_event", self.cellSelect)
		btn.modify_bg(gtk.STATE_NORMAL, btn.get_colormap().alloc_color("gray"))
		return btn
	def showGrid(self):
		max = 0
		for layer in self.layer:
			for s in layer.scene:
				if s.end > max:
					max = s.end
		max = 50

		self.grid = gtk.Table(len(self.layer)+1, 50)
		self.scrollwin = gtk.ScrolledWindow()
		self.scrollwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		self.scrollwin.add_with_viewport(self.grid)
		for i in range(1,max):
			self.grid.attach(gtk.Label('%d'% i), i,i+1,0,1,0,0,0,0)
		for i in range(1,len(self.layer)+1):
			l = self.layer[i-1]
			self.grid.attach(gtk.Label(l.node.get('id')), 0, 1, i, i+1,0,0,10,0)
			for s in l.scene:
				btn = self.newCell('start.png')
				btn.nScene = s.start
				btn.layer = l.node.get('id')
				btn.nLayer = i

				self.grid.attach(btn, s.start, s.start+1, i, i+1,0,0,0,0)
				for j in range(s.start+1,s.end+1):
					btn = self.newCell('fill.png')
					self.grid.attach(btn, j, j+1, i , i+1,0,0,0,0)
					btn.modify_bg(gtk.STATE_NORMAL, btn.get_colormap().alloc_color("gray"))
					btn.nScene = j
					btn.layer = l.node.get('id')
					btn.nLayer = i
			if len(l.scene) == 0:
				start = 0
			else:
				start = l.scene[len(l.scene)-1].end
			for j in range(start,max):
				btn = self.newCell('empty.png')
				self.grid.attach(btn, j+1, j+2,i,i+1,0,0,0,0)
				btn.modify_bg(gtk.STATE_NORMAL, btn.get_colormap().alloc_color("gray"))
				btn.nScene = j+1
				btn.layer = l.node.get('id')
				btn.nLayer = i
		self.last_cell = None
	def cellSelect(self, cell, data):
		if self.last_cell:
			self.last_cell.modify_bg(gtk.STATE_NORMAL, self.last_cell.get_colormap().alloc_color("gray"))
			
		self.last_cell = cell
		cell.modify_bg(gtk.STATE_NORMAL, cell.get_colormap().alloc_color("green"))
		
	def doEditScene(self,w):
		self.setCurrentScene(self.last_cell.nScene)
		self.generate()
		gtk.main_quit()
	def doInsertKeyScene(self,w):
		self.insertKeyScene()
		self.grid.show_all()
		self.generate()

	def doRemoveScene(self,w):
		self.removeKeyScene()
		self.grid.show_all()
		self.generate()
	def addButtons(self,hbox):
		btn = gtk.Button('Edit')
		btn.connect('clicked', self.doEditScene)
		hbox.pack_start(btn)
		btn = gtk.Button('Insert Key')
		btn.connect('clicked',self.doInsertKeyScene)
		hbox.pack_start(btn)
		btn=gtk.Button('Remove Key')
		btn.connect('clicked', self.doRemoveScene)
		hbox.pack_start(btn)
	def effect(self):
		self.parseScene()
		self.showGrid()
		self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
		self.window.connect("destroy", gtk.main_quit)
		self.window.set_position(gtk.WIN_POS_MOUSE)
		vbox = gtk.VBox()
		self.window.add(vbox)
		vbox.add(self.scrollwin)
		self.vbox = vbox
		hbox=gtk.HBox()
		self.addButtons(hbox)
		vbox.add(hbox)
		self.window.set_size_request(600,200)
		self.window.show_all()
		gtk.main()

		



A = MBScene()
A.affect()


