#!/usr/bin/python
import inkex
import pygtk
import gtk
from copy import deepcopy
from lxml import etree
import random

# In the inkscape, the top layer group are treated as layer. The Mad butter fly add another structure under the layer as frame. It means that each layer can has multiple frame. 
# Like the layer, the frames are represented by special groups. All groups directly under a layer will be treated as frames. However, a group may be spanned for more than one
# frame. For example
# <g id="layer1">
#    <g id="g1234" scene="1">
#    </g>
#    <g id="g1235" scene="2-7">
#    </g>
#    <g id="g1236">
#    </g>
# </g>
# This will stand for 7 scenes. Scene 1 and scene 2 are key scenes. 3,4,5,6,7 are filled scenes.In the above example, g1234 and g1236 are in the scene 1. g1235 and g1236 are in
# scene 2-7
#
# In the inkscape extention, we will provide an grid for users to select the current scene or change the scene structure. Users are allowed to
#     Insert a new key scene
#     Delete a key scene
#     Insert a filled scene
#     Delete a filled scene
#     Select a scene for edit.
#
# When user select a scene to edit, we will hide all scenes which is not in the selected scene. For example, if we select scene 4, g1234 will be hidden and g1235 and g1236 will 
# be displayed.



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

	def parseScene(self):
		self.layer = []
		for layer in self.document.getroot():
			if layer.tag == '{http://www.w3.org/2000/svg}g':
				#print layer.attrib.get("id")
				lyobj = Layer(layer)
				self.layer.append(lyobj)
				for scene in layer:
					if scene.tag == '{http://www.w3.org/2000/svg}g':
						range = scene.attrib.get("scene").split('-')
						if len(range) == 1:
							#print "    scene %d" % int(range[0])
							lyobj.scene.append(Scene(scene,range[0],range[0]))
						elif len(range) == 2:
							#print "    scene%d-%d" % (int(range[0]),int(range[1]))
							lyobj.scene.append(Scene(scene,range[0],range[1]))
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
			last = layer.scene[len(layer.scene)-1]
			for x in range(last.end+1, nth):
				btn = self.newCell('fill.png')
				btn.nScene = x
				btn.layer = layer
				btn.nLayer = y
				self.grid.attach(btn, x, x+1, y , y+1,0,0,0,0)
			last.end = nth-1
			newscene = Scene(deepcopy(s.node),nth,nth)
			newscene.node.set("id",self.newID())
			layer.scene.append(newscene)
			btn = self.newCell('start.png')
			x = self.last_cell.nScene
			y = self.last_cell.nLayer
			btn.nScene = nth
			btn.layer = layer
			btn.nLayer = y
			self.grid.attach(btn, x, x+1, y, y+1,0,0,0,0)


	def removeKeyScene(self):
		nth = self.last_cell.nScene
		layer = self.getLayer(self.last_cell.layer)
		x = self.last_cell.nScene
		y = self.last_cell.nLayer
		# We can not remove the key scene at the first scene
		if nth == 1: return
		for i in range(0,len(layer.scene)):
			s = layer.scene[i]
			if nth == s.start:
				layer.scene[i-1].end = s.end
				layer.scene.remove(s)
				btn = self.newCell('fill.png')
				btn.nScene = nth
				btn.layer = layer
				btn.nLayer = y
				self.grid.attach(btn, x,x+1,y,y+1,0,0,0,0)
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
		for layer in self.layer:
			for s in layer.scene:
				if nth >= s.start and nth <= s.end:
					s.node.set("style","")
				else:
					s.node.set("style","display:none")
	def generate(self):
		newdoc = deepcopy(self.document)
		root = newdoc.getroot()
		for n in root:
			if n.tag ==  '{http://www.w3.org/2000/svg}g':
				root.remove(n)

		for l in self.layer:
			# Duplicate all attribute of the layer
			lnode = etree.Element("{http://www.w3.org/2000/svg}g")
			for a,v in l.node.attrib.items():
				lnode.set(a,v)
			root.append(lnode)
			for s in l.scene:
				snode = etree.Element("{http://www.w3.org/2000/svg}g")
				for a,v in s.node.attrib.items():
					snode.set(a,v)
				if s.start == s.end:
					snode.set("scene", "%d" % s.start)
				else:
					snode.set("scene","%d-%d" % (s.start,s.end))
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


