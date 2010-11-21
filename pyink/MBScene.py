#!/usr/bin/python
# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; -*-
# vim: sw=4:ts=8:sts=4
import pygtk
import gtk
from copy import deepcopy
from lxml import etree
import random
import traceback
import time

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
		try:
		    cur = int(n.repr.attribute("current"))
		except:
		    cur = 1
		self.current = cur

		for s in n.childList():
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
	if self.scenemap==None:
	    self.desktop.doc().root().repr.setAttribute("xmlns:ns0","http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd",True)
	    scenes = self.desktop.doc().rdoc.createElement("ns0:scenes")
	    node.repr.appendChild(scenes)
    def update(self):
        doc = self.desktop.doc().root()
	rdoc = self.desktop.doc().rdoc
	for node in doc.childList():
	    if node.repr.name() == 'svg:metadata':
	        for t in node.childList():
		    if t.repr.name() == "ns0:scenes":
		        node.repr.removeChild(t.repr)
			ns = rdoc.createElement("ns0:scenes")
			node.repr.appendChild(ns)
			for layer in range(0,len(self._framelines)):
			    lobj = self._framelines[layer]
			    lobj.addScenes(rdoc,ns)
    
    
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
			except:
			    lyobj.current_scene.append(scene)
			    continue

			lyobj.scenes.append(Scene(scene,scmap[0],scmap[1]))
			pass
		    else:
			lyobj.current_scene.append(scene)
			pass
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
	x = self.last_frame
	y = self.last_line
	rdoc = self.desktop.doc().rdoc
	ns = rdoc.createElement("svg:g")
	txt = rdoc.createElement("svg:rect")
	txt.setAttribute("x","0",True)
	txt.setAttribute("y","0",True)
	txt.setAttribute("width","100",True)
	txt.setAttribute("height","100",True)
	txt.setAttribute("style","fill:#ff00",True)
	ns.appendChild(txt)
	gid = self.last_line.node.label()+self.newID()
	self.ID[gid]=1
	ns.setAttribute("id",gid,True)
	ns.setAttribute("inkscape:groupmode","layer",True)
	self.last_line.node.repr.appendChild(ns)
	print 'Add key ', x
	self.last_line.add_keyframe(x,ns)
	self.update()
	self.last_line.update()
    

    def removeKeyScene(self):
	nth = self.last_frame
	y = self.last_line
	rdoc = self.desktop.doc().rdoc
	i = 0
	layer = self.last_line
	while i < len(layer._keys):
	    s = layer._keys[i]
	    print "nth:%d idx %d" % (nth,s.idx)
	    if nth > s.idx:
	        if i == len(layer._keys)-1:
	            return
	    if nth == s.idx:
	        if s.left_tween:
		    # This is left tween, we move the keyframe one frame ahead
		    if s.idx == layer._keys[i-1].idx:
			layer._keys[i].ref.parent().removeChild(layer._keys[i].ref)
	                self.last_line.rm_keyframe(nth)
	                self.last_line.rm_keyframe(nth-1)
		    else:
		        s.idx = s.idx-1
		else:
		    layer._keys[i].ref.parent().removeChild(layer._keys[i].ref)
		    if s.right_tween:
		        self.last_line.rm_keyframe(layer._keys[i+1].idx)
	                self.last_line.rm_keyframe(nth)
		    else:
	                self.last_line.rm_keyframe(nth)

		self.update()
		self.last_line._draw_all_frames()
	        self.last_line.update()
		return
	    i = i + 1
    def extendScene(self):
	nth = self.last_frame
	layer = self.last_line
	i = 0
	while i < len(layer._keys):
	    s = layer._keys[i]
	    if s.right_tween:
	        if nth > s.idx:
		    if nth <= layer._keys[i+1].idx:
		        return
		    try:
		        if nth <= layer._keys[i+2].idx:
			    layer._keys[i+1].idx = nth
			    layer.draw_all_frames()
			    self.update()
			    self.setCurrentScene(nth)
			    self.last_line.update()
			    return
			else:
			    # We may in the next scene
			    i = i + 2
			    pass
		    except:
		        # This is the last keyframe, extend the keyframe by 
			# relocate the location of the keyframe
			layer._keys[i+1].idx = nth
			layer._draw_all_frames()
			self.update()
			self.last_line.update()
			self.setCurrentScene(nth)
			return
		else:
		    # We are in the front of all keyframes
		    return
	    else:
		# This is a single keyframe
		if nth < s.idx:
		    return
		if nth == s.idx:
		    return
		try:
		    if nth < layer._keys[i+1].idx:
			# We are after a single keyframe and no scene is 
			# available here. Create a new tween here
			idx = layer._keys[i].idx
			layer.add_keyframe(nth,layer._keys[i].ref)
			layer.tween(idx)
		        layer._draw_all_frames()
			self.update()
			self.setCurrentScene(nth)
			self.last_line.update()
			return
		    else:
			# We may in the next scene
			i = i + 1
			pass
		    pass
		except:
		    # This is the last scene, create a new one
		    idx = layer._keys[i].idx
		    layer.add_keyframe(nth,layer._keys[i].ref)
		    layer.tween(idx)
		    layer._draw_all_frames()
		    self.update()
		    self.setCurrentScene(nth)
		    self.last_line.update()
		    return
		pass
	    pass
	pass

    
    def setCurrentScene(self,nth):
	self.current = nth
	for layer in self._framelines:
	    i=0
	    while i < len(layer._keys):
	        s = layer._keys[i]
		print s.ref.attribute("id"),s.idx,s.left_tween,s.right_tween
		if s.right_tween is False:
		    if nth == s.idx+1:
		        s.ref.setAttribute("style","",True)
		    else:
		        s.ref.setAttribute("style","display:none",True)
		    i = i + 1
		    continue

		if nth >= (s.idx+1) and nth <= (layer._keys[i+1].idx+1):
		    s.ref.setAttribute("style","",True)
		else:
		    s.ref.setAttribute("style","display:none",True)
		i = i + 2
		pass
	    pass
	pass
	
	
    def newCell(self,file):
	img = gtk.Image()
	img.set_from_file(file)
	btn = gtk.EventBox()
	btn.add(img)
	btn.connect("button_press_event", self.cellSelect)
	btn.modify_bg(gtk.STATE_NORMAL, btn.get_colormap().alloc_color("gray"))
	return btn
    
    def onCellClick(self,line,frame,but):
	self.last_line = line
	self.last_frame = frame
	self.last_line.active_frame(frame)
        self.doEditScene(frame)
        
        
    def _remove_active_frame(self,widget,event):
        """
	Hide all hover frames. This is a hack. We should use the lost focus event 
	instead in the future to reduce the overhead.
	"""
        for f in self._framelines:
	    if f != widget:
	        f.hide_hover()
	    
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
	for i in range(len(self.layers)-1,-1,-1):
	    line = frameline.frameline(nframes)
	    hbox = gtk.HBox()
	    label = gtk.Label(self.layers[i].node.label())
	    label.set_size_request(100,0)
	    hbox.pack_start(label,expand=False,fill=True)
	    hbox.pack_start(line)
	    line.set_size_request(nframes * 10, 20)
	    vbox.pack_start(hbox, False)
	    line.label = label
	    self._framelines.append(line)
	    line.connect(line.FRAME_BUT_PRESS, self.onCellClick)
	    line.nLayer = i
	    line.node = self.layers[i].node
	    line.layer = self.layers[i]
	    line.connect('motion-notify-event', self._remove_active_frame)
	    pass
	pass

    ## \brief Update conetent of frameliens according layers.
    #
    def _update_framelines(self):
	for frameline in self._framelines:
	    layer = frameline.layer
	    frameline.label.set_text(frameline.node.label())
	    for scene in layer.scenes:
		frameline.add_keyframe(scene.start-1,scene.node.repr)
		if scene.start != scene.end:
		    frameline.add_keyframe(scene.end-1,scene.node.repr)
		    frameline.tween(scene.start-1)
		pass
	    pass
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
	self.setCurrentScene(self.last_frame+1)
	pass
    
    def doInsertKeyScene(self,w):
	self.insertKeyScene()
	# self.grid.show_all()
	return

    def doRemoveScene(self,w):
	self.removeKeyScene()
	return

    
    def doExtendScene(self,w):
	self.extendScene()
	#self.grid.show_all()
	pass
    
    def addButtons(self,hbox):
	#btn = gtk.Button('Edit')
	#btn.connect('clicked', self.doEditScene)
	#hbox.pack_start(btn,expand=False,fill=False)
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


    def show(self):
	self.OK = True
	self.parseScene()
	self._create_framelines()
	self._update_framelines()
	vbox = gtk.VBox(False,0)
	self.desktop.getToplevel().child.child.pack_end(vbox,expand=False)
	self.window = vbox
	vbox = gtk.VBox(False,0)
	self.window.pack_start(vbox,expand=False)
	vbox.pack_start(self.scrollwin,expand=False)
	self.vbox = vbox
	hbox=gtk.HBox(False,0)
	self.addButtons(hbox)
	vbox.pack_start(hbox,expand=False)

	# self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
	# self.window.connect("destroy", gtk.main_quit)
	# self.window.set_position(gtk.WIN_POS_MOUSE)

	self.window.show_all()
	pass
    pass
