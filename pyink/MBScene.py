#!/usr/bin/python
# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; -*-
# vim: sw=4:ts=8:sts=4
import pygtk
import gtk
from copy import deepcopy
from lxml import etree
import random
import traceback

# Please refer to
# http://www.assembla.com/wiki/show/MadButterfly/Inkscape_extention
# for the designed document.


# Algorithm:
# 
# We will parse the first two level of the SVG DOM. collect a table of
# layer and scene.
# - 1. Collect the layer table which will be displayed as the first
#      column of the grid.
# - 2. Get the maximum scene number. This will decide the size of the
#      grid.
# - 3. When F6 is pressed, we will check if this scene has been
#      defined. This can be done by scan all second level group and
#      check if the current scene number is within the range specified
#      by scene field. The function IsSceneDefined(scene) can be used
#      for this purpose.
# - 4. If this is a new scene, we will append a new group which
#      duplication the content of the last scene in the same
#      group. The scene field will contain the number from the last
#      scene number of the last scene to the current scenen
#      number. For example, if the last scene is from 4-7 and the new
#      scene is 10, we will set the scene field as "8-10".
# - 5. If this scene are filled screne, we will split the existing
#       scene into two scenes with the same content.
#

class Layer:
    def __init__(self,node):
	self.scenes = []
	self.node = node
	self.nodes=[]
	pass
    pass

class Scene:
    def __init__(self, node, start,end):
	self.node = node
	self.start = int(start)
	self.end = int(end)
	pass
    pass

_scenes = '{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scenes'
_scene = '{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scene'

class MBScene():
    def __init__(self,desktop,win):
	self.desktop = desktop
	self.window = win
	self.layers = []
	self.layers.append(Layer(None))
	self.scenemap = None
	pass

    def confirm(self,msg):
	vbox = gtk.VBox()
	vbox.pack_start(gtk.Label(msg))
	self.button = gtk.Button('OK')
	vbox.pack_start(self.button)
	self.button.connect("clicked", self.onQuit)
	self.window.add(vbox)
	pass
    
    def dumpattr(self,n):
	s = ""
	for a,v in n.attrib.items():
	    s = s + ("%s=%s"  % (a,v))
	    pass
	return s
	
    def dump(self,node,l=0):
	print " " * l*2,"<", node.tag, self.dumpattr(node),">"
	for n in node:
	    self.dump(n,l+1)
	    pass
	print " " * l * 2,"/>"
	pass
    
    def parseMetadata(self,node):
	self.current = 1
	for n in node.childList():
	    if n.repr.name() == 'ns0:scenes':
		self.scenemap={}
		cur = int(n.repr.attribute("current"))
		self.current = cur

		for s in n.childList():
		    print '--->',s.repr.name()
		    if s.repr.name() == 'ns0:scene':
			try:
			    start = int(s.repr.attribute("start"))
			except:
			    traceback.print_exc()
			    continue
			try:
			    end = s.repr.attribute("end")
			    if end == None:
				end = start
				pass
			except:
			    end = start
			    pass
			link = s.repr.attribute("ref")
			self.scenemap[link] = [int(start),int(end)]
			print "scene %d to %d" % (self.scenemap[link][0],
						  self.scenemap[link][1])
			if cur >= start and cur <= end:
			    self.currentscene = link
			    pass
			pass
		    pass
		pass
	    pass
	pass
    
    
    def parseScene(self):
	"""
	In this function, we will collect all items for the current
	scene and then relocate them back to the appropriate scene
	object.
	"""
	self.layers = []
	self.scenemap = None
	doc = self.desktop.doc().root()

	for node in doc.childList():
	    if node.repr.name() == 'svg:metadata':
		self.parseMetadata(node)
		pass
	    elif node.repr.name() == 'svg:g':
		oldscene = None
				#print layer.attrib.get("id")
		lyobj = Layer(node)
		self.layers.append(lyobj)
		lyobj.current_scene = []
		for scene in node.childList():
		    if scene.repr.name() == 'svg:g':
			try:
			    scmap = self.scenemap[scene.getId()]
			    if scmap == None:
				lyobj.current_scene.append(scene)
				continue
			    if self.current <= scmap[1] and \
				    self.current >= scmap[0]:
				oldscene = scene
				pass
			except:
			    lyobj.current_scene.append(scene)
			    continue

			lyobj.scenes.append(Scene(scene,scmap[0],scmap[1]))
			pass
		    else:
			lyobj.current_scene.append(scene)
			pass
		    pass

		if oldscene != None:
		    # Put the objects back to the current scene
		    # for o in lyobj.current_scene:
		    #     print o.tag
		    #     oldscene.append(o)
		    pass
		pass
	    pass

	self.collectID()
	self.dumpID()
	pass

    def collectID(self):
	self.ID = {}
	root = self.desktop.doc().root()
	for n in root.childList():
	    self.collectID_recursive(n)
	    pass
	pass
    
    def collectID_recursive(self,node):
	self.ID[node.getId()] = 1
	for n in node.childList():
	    self.collectID_recursive(n)
	    pass
	pass
    
    def newID(self):
	while True:
	    n = 's%d' % int(random.random()*10000)
			#print "try %s" % n
	    if self.ID.has_key(n) == False:
		return n
	    pass
	pass
    
    def dumpID(self):
	for a,v in self.ID.items():
	    print a
	    pass
	pass
    
    def getLayer(self, layer):
	for l in self.layers:
	    if l.node.getId() == layer:
		return l
	    pass
	return None
    
    
    def insertKeyScene(self):
	"""
	Insert a new key scene into the stage. If the nth is always a
	key scene, we will return without changing anything.  If the
	nth is a filled scene, we will break the original scene into
	two parts. If the nth is out of any scene, we will append a
	new scene.

	"""
	nth = self.last_cell.nScene
	layer = self.getLayer(self.last_cell.layer)
	x = self.last_cell.nScene
	y = self.last_cell.nLayer
	if layer == None: return
	for i in range(0,len(layer.scenes)):
	    s = layer.scenes[i]
	    if nth >= s.start and nth <= s.end:
		if nth == s.start: return
		newscene = Scene(DuplicateNode(s.node),nth,s.end)
		newscene.node.setId(self.newID())
		layer.scenes.insert(i+1,newscene)
		layer.scenes[i].end = nth-1
		btn = self.newCell('start.png')
		btn.nScene = nth
		btn.layer = layer
		btn.nLayer = y
		self.grid.remove(self.last_cell)
		self.grid.attach(btn, x,x+1,y,y+1,0,0,0,0)
		return
	    pass
	
	if len(layer.scenes) > 0:
	    last = nth
	    lastscene = None
	    for s in layer.scenes:
		if s.end < nth and last < s.end:
		    last = s.end
		    lastscene = s
		    pass
		pass
	    
	    for x in range(last+1, nth):
		btn = self.newCell('fill.png')
		btn.nScene = x
		btn.layer = layer.node.getId()
		btn.nLayer = y
		self.grid.attach(btn, x, x+1, y , y+1,0,0,0,0)
		pass
	    
	    if lastscene == None:
		node = etree.Element(_scene)
		node.setId(self.newID())
		newscene = Scene(node,nth,nth)
	    else:
		lastscene.end = nth-1
		newscene = Scene(DuplicateNode(lastscene.node),nth,nth)
		newscene.node.setId(self.newID())
		pass
	    
	    layer.scenes.append(newscene)
	    btn = self.newCell('start.png')
	    x = self.last_cell.nScene
	    y = self.last_cell.nLayer
	    btn.nScene = nth
	    btn.layer = layer.node.getId()
	    btn.nLayer = y
	    self.grid.attach(btn, x, x+1, y, y+1,0,0,0,0)
	else:
	    # This is the first scene in the layer
	    node = etree.Element(_scene)
	    node.repr.setId(self.newID())
	    newscene = Scene(node,nth,nth)
	    layer.scenes.append(newscene)
	    btn = self.newCell('start.png')
	    btn.nScene = nth
	    btn.layer = layer.node.getId()
	    btn.nLayer = y
	    self.grid.attach(btn, x, x+1, y, y+1,0,0,0,0)
	    pass
	pass
    

    def removeKeyScene(self):
	nth = self.last_cell.nScene
	try:
	    layer = self.getLayer(self.last_cell.layer)
	except:
	    return
	x = self.last_cell.nScene
	y = self.last_cell.nLayer
	for i in range(0,len(layer.scenes)):
	    s = layer.scenes[i]
	    if nth == s.start:
		if i == 0:
		    for j in range(s.start,s.end+1):
			btn = self.newCell('empty.png')
			btn.nScene = nth
			btn.layer = layer
			btn.nLayer = y
			self.grid.attach(btn, j,j+1,y,y+1,0,0,0,0)
			pass
		    layer.scenes.remove(s)
		else:
		    if s.start == layer.scenes[i-1].end+1:
			# If the start of the delete scene segment is
			# the end of the last scene segmenet, convert
			# all scenes in the deleted scene segmenet to
			# the last one
			layer.scenes[i-1].end = s.end
			layer.scenes.remove(s)
			btn = self.newCell('fill.png')

			btn.nScene = nth
			btn.layer = layer
			btn.nLayer = y
			self.grid.attach(btn, x,x+1,y,y+1,0,0,0,0)
		    else:
			# Convert all scenes into empty cell
			layer.scenes.remove(s)
			for j in range(s.start,s.end+1):
			    btn = self.newCell('empty.png')
			    btn.nScene = nth
			    btn.layer = layer
			    btn.nLayer = y
			    self.grid.attach(btn, j,j+1,y,y+1,0,0,0,0)
			    pass
			pass
		    pass
		return
	    pass
	pass

    def extendScene(self):
	nth = self.last_cell.nScene
	try:
	    layer = self.getLayer(self.last_cell.layer)
	except:
	    traceback.print_exc()
	    return
	x = self.last_cell.nScene
	y = self.last_cell.nLayer
	if layer == None:
	    return

	for i in range(0,len(layer.scenes)-1):
	    s = layer.scenes[i]
	    if nth >= layer.scenes[i].start and nth <= layer.scenes[i].end:
		return
	    pass

	for i in range(0,len(layer.scenes)-1):
	    s = layer.scenes[i]
	    if nth >= layer.scenes[i].start and nth < layer.scenes[i+1].start:
		for j in range(layer.scenes[i].end+1, nth+1):
		    btn = self.newCell('fill.png')
		    btn.nScene = nth
		    btn.nLayer = y
		    btn.layer = self.last_cell.layer
		    self.grid.attach(btn, j,j+1,y,y+1,0,0,0,0)
		    layer.scenes[i].end = nth
		    return
		pass
	    if len(layer.scenes) > 0 and \
		    nth > layer.scenes[len(layer.scenes)-1].end:
		for j in range(layer.scenes[len(layer.scenes)-1].end+1, nth+1):
		    btn = self.newCell('fill.png')
		    btn.nScene = nth
		    btn.nLayer = y
		    btn.layer = self.last_cell.layer
		    self.grid.attach(btn, j,j+1,y,y+1,0,0,0,0)
		    pass
		layer.scenes[len(layer.scenes)-1].end = nth
		pass
	    pass
	pass
    
    def setCurrentScene(self,nth):
	self.current = nth
	for layer in self.layers:
	    for s in layer.scenes:
		if nth >= s.start and nth <= s.end:
		    s.node.repr.setAttribute("style","",True)
		    # print "Put the elemenets out"
		    layer.nodes = []
		    
		    # for o in s.node:
		    #        print "    ",o.tag
		    #	layer.nodes.append(o)
		    # for o in s.node:
		    #	s.node.remove(o)
		else:
		    s.node.repr.setAttribute("style","display:none",True)
		    pass
		pass
	    pass
	pass
	
    def generate(self):
	newdoc = deepcopy(self.document)
	root = newdoc.getroot()
	has_scene = False
	for n in root:
	    if n.tag == '{http://www.w3.org/2000/svg}metadata':
		for nn in n:
		    if nn.tag == _scenes:
			nn.clear()
			nn.set("current", "%d" % self.current)
			scenes = []
			for l in self.layers:
			    for s in l.scenes:
				id = s.node.get("id")
				scene = etree.Element(_scene)
				scene.set("ref", id)
				if s.start == s.end:
				    scene.set("start", "%d" % s.start)
				else:
				    scene.set("start", "%d" % s.start)
				    scene.set("end", "%d" % s.end)
				    pass

				scenes.append(scene)
				pass
			    pass
			for s in scenes:
			    nn.append(s)
			    pass
			has_scene = True
			pass
		    pass
		if has_scene == False:
		    scenes = etree.Element(_scenes)
		    scenes.set("current","%d" % self.current)
		    for l in self.layers:
			for s in l.scenes:
			    scene = etree.Element(_scene)
			    scene.set("ref", s.node.get("id"))
			    if s.start == s.end:
				scene.set("start", "%d" % s.start)
			    else:
				scene.set("start", "%d" % s.start)
				scene.set("end", "%d" % s.end)
				pass
			    scenes.append(scene)
			    pass
			pass
		    n.append(scenes)
		    pass
		pass
	    if n.tag ==  '{http://www.w3.org/2000/svg}g':
		root.remove(n)
		pass
	    pass
	
	for l in self.layers:
	    # Duplicate all attribute of the layer
	    lnode = etree.Element("{http://www.w3.org/2000/svg}g")
	    for a,v in l.node.attrib.items():
		lnode.set(a,v)
		pass
	    for n in l.nodes:
		lnode.append(n)
		pass
	    root.append(lnode)
	    for s in l.scenes:
		snode = etree.Element("{http://www.w3.org/2000/svg}g")
		for a,v in s.node.attrib.items():
		    snode.set(a,v)
		    pass
		for n in s.node:
		    snode.append(deepcopy(n))
		    pass
		lnode.append(snode)
		pass
	    pass
	self.document = newdoc
	pass
	
    def newCell(self,file):
	img = gtk.Image()
	img.set_from_file(file)
	btn = gtk.EventBox()
	btn.add(img)
	btn.connect("button_press_event", self.cellSelect)
	btn.modify_bg(gtk.STATE_NORMAL, btn.get_colormap().alloc_color("gray"))
	return btn
    
    def _create_framelines(self):
	import frameline
	self.scrollwin = gtk.ScrolledWindow()
	self.scrollwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	self.scrollwin.set_size_request(-1,150)
	
	nframes = 100
	
	vbox = gtk.VBox()
	vbox.show()
	self.scrollwin.add_with_viewport(vbox)
	
	ruler = frameline.frameruler(nframes)
	ruler.set_size_request(nframes * 10, 20)
	ruler.show()
	vbox.pack_start(ruler, False)

	#
	# Add a frameline for each layer
	#
	self._framelines = []
	for i in range(len(self.layers)):
	    line = frameline.frameline(nframes)
	    line.set_size_request(nframes * 10, 20)
	    vbox.pack_start(line, False)
	    self._framelines.append(line)
	    pass
	pass

    ## \brief Update conetent of frameliens according layers.
    #
    def _update_framelines(self):
	for layer_i, layer in enumerate(self.layers):
	    for scene in layer.scenes:
		frameline = self._framelines[layer_i]
		for scene_i in range(scene.start, scene.stop + 1):
		    frameline.add_keyframe(scene_i)
		    pass
		pass
	    pass
	pass

    def showGrid(self):
	max = 0
	for layer in self.layers:
	    for s in layer.scenes:
		if s.end > max:
		    max = s.end
		    pass
		pass
	    pass
	max = 50

	self.grid = gtk.Table(len(self.layers)+1, 50)
	self.scrollwin = gtk.ScrolledWindow()
	self.scrollwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	self.scrollwin.add_with_viewport(self.grid)
	self.scrollwin.set_size_request(-1,150)
	for i in range(1,max):
	    self.grid.attach(gtk.Label('%d'% i), i,i+1,0,1,0,0,0,0)
	    pass
	for i in range(1,len(self.layers)+1):
	    print "Layer", i
	    l = self.layers[i-1]
	    for s in l.scenes:
		btn = self.newCell('start.png')
		btn.nScene = s.start
		btn.layer = l.node.getId()
		btn.nLayer = i

		self.grid.attach(btn, s.start, s.start+1, i, i+1,0,0,0,0)
		for j in range(s.start+1,s.end+1):
		    btn = self.newCell('fill.png')
		    self.grid.attach(btn, j, j+1, i , i+1,0,0,0,0)
		    color = btn.get_colormap().alloc_color("gray")
		    btn.modify_bg(gtk.STATE_NORMAL, color)
		    btn.nScene = j
		    btn.layer = l.node.getId()
		    btn.nLayer = i
		    pass
		pass
	    if len(l.scenes) == 0:
		start = 0
	    else:
		start = l.scenes[len(l.scenes)-1].end
		pass
	    
	    for j in range(start,max):
		btn = self.newCell('empty.png')
		self.grid.attach(btn, j+1, j+2,i,i+1,0,0,0,0)
		color = btn.get_colormap().alloc_color("gray")
		btn.modify_bg(gtk.STATE_NORMAL, color)
		btn.nScene = j+1
		btn.layer = l.node.getId()
		btn.nLayer = i
		pass
	    pass
	self.last_cell = None
	pass
    
    def cellSelect(self, cell, data):
	if self.last_cell:
	    color = self.last_cell.get_colormap().alloc_color("gray")
	    self.last_cell.modify_bg(gtk.STATE_NORMAL, color)
	    pass
	
	self.last_cell = cell
	color = cell.get_colormap().alloc_color("green")
	cell.modify_bg(gtk.STATE_NORMAL, color)
	pass
    
    def doEditScene(self,w):
	self.setCurrentScene(self.last_cell.nScene)
	pass
    
    def doInsertKeyScene(self,w):
	# self.insertKeyScene()
	# self.grid.show_all()
	return

    def doRemoveScene(self,w):
	# self.removeKeyScene()
	# self.grid.show_all()
	# self.generate()
	return

    
    def doExtendScene(self,w):
	self.extendScene()
	self.grid.show_all()
	pass
    
    def addButtons(self,hbox):
	btn = gtk.Button('Edit')
	btn.connect('clicked', self.doEditScene)
	hbox.pack_start(btn,expand=False,fill=False)
	btn = gtk.Button('Insert Key')
	btn.connect('clicked',self.doInsertKeyScene)
	hbox.pack_start(btn,expand=False,fill=False)
	btn=gtk.Button('Remove Key')
	btn.connect('clicked', self.doRemoveScene)
	hbox.pack_start(btn,expand=False,fill=False)
	btn=gtk.Button('Extend scene')
	btn.connect('clicked', self.doExtendScene)
	hbox.pack_start(btn,expand=False,fill=False)
	pass
    
    def onQuit(self, event):
	self.OK = False
	gtk.main_quit()
	pass
    
    def onOK(self,event):
	self.OK = True
	gtk.main_quit()
	pass

    def onConfirmDelete(self):
	if self.scenemap == None:
	    vbox = gtk.VBox(False,0)
	    vbox.pack_start(gtk.Label('Convert the SVG into a MadButterfly'
				      ' SVG file. All current element will'
				      ' be delted'))
	    hbox = gtk.HBox(False,0)
	    self.button = gtk.Button('OK')
	    hbox.pack_start(self.button,expand=False,fill=False)
	    self.button.connect('clicked', self.onOK)
	    self.button = gtk.Button('Cancel')
	    hbox.pack_start(self.button,expand=False,fill=False)
	    self.button.connect("clicked", self.onQuit)
	    vbox.pack_start(hbox,expand=False,fill=False)
	    self.window.add(vbox)
	    self.window.show_all()
	    gtk.main()
	    self.window.remove(vbox)
	    pass
	pass

    def show(self):
	self.OK = True
	self.parseScene()
	self._create_framelines()
	self._update_framelines()
	vbox = gtk.VBox(False,0)
	self.desktop.getToplevel().child.child.pack_end(vbox,expand=False)
	self.window = vbox
	# self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
	# self.window.connect("destroy", gtk.main_quit)
	# self.window.set_position(gtk.WIN_POS_MOUSE)
	if self.scenemap == None:
	    self.onConfirmDelete()
	    pass
	if self.OK:
	    vbox = gtk.VBox(False,0)
	    self.window.pack_start(vbox,expand=False)
	    vbox.pack_start(self.scrollwin,expand=False)
	    self.vbox = vbox
	    hbox=gtk.HBox(False,0)
	    self.addButtons(hbox)
	    vbox.pack_start(hbox,expand=False)
	else:
	    return

	# self.window.set_size_request(600,200)

	self.window.show_all()
	pass
    pass
