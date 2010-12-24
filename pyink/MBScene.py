#!/usr/bin/python
# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; -*-
# vim: sw=4:ts=8:sts=4
import pygtk
import gtk
import glib
from copy import deepcopy
from lxml import etree
import random
import traceback
import time
import pybInkscape
import math
from tween import TweenObject

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
    def __init__(self, node, start,end,typ):
	self.node = node
	self.start = int(start)
	self.end = int(end)
	self.type = typ
	pass
    pass
class DOM(pybInkscape.PYSPObject):
    def __init__(self,obj=None):
        self.proxy = obj
	pass
    def duplicate(self,doc):
	return DOM(self.repr.duplicate(doc))

class ObjectWatcher(pybInkscape.PYNodeObserver):
    def __init__(self,obj,type,func,arg):
        self.obj = obj
	self.type = type
	self.func = func
	self.arg = arg

    def notifyChildAdded(self,node,child,prev):
        if self.type == 'DOMNodeInserted':
	    self.func(node)
    def notifyChildRemoved(self,node,child,prev):
        if self.type == 'DOMNodeRemoved':
	    self.func(node)
    def notifyChildOrderChanged(self,node,child,prev):
        pass
    def notifyContentChanged(self,node,old_content,new_content):
        print 'cont'
        if self.type == 'DOMSubtreeModified':
	    self.func(node)
    def notifyAttributeChanged(self,node, name, old_value, new_value):
        print 'attr',node,name,old_value,new_value
        if self.type == 'DOMAttrModified':
	    self.func(node,name)

def addEventListener(obj, type, func,arg):
    obs = ObjectWatcher(obj,type,func,arg)
    obj.addSubtreeObserver(obs)
    

_scenes = '{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scenes'
_scene = '{http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd}scene'
class LayerAttributeWatcher(pybInkscape.PYNodeObserver):
    def __init__(self,ui):
        self.ui = ui
    def notifyChildAdded(self,node,child,prev):
        pass
    def notifyChildRemoved(self,node,child,prev):
        pass
    def notifyChildOrderChanged(self,node,child,prev):
        pass
    def notifyContentChanged(self,node,old_content,new_content):
        pass
    def notifyAttributeChanged(self,node, name, old_value, new_value):
        self.ui.updateUI()
class LayerAddRemoveWatcher(pybInkscape.PYNodeObserver):
    def __init__(self,ui):
        self.ui = ui
    def notifyChildAdded(self,node,child,prev):
        self.ui.updateUI()
    def notifyChildRemoved(self,node,child,prev):
        self.ui.updateUI()
    def notifyChildOrderChanged(self,node,child,prev):
        self.ui.updateUI()
    def notifyContentChanged(self,node,old_content,new_content):
        self.ui.updateUI()
    def notifyAttributeChanged(self,node, name, old_value, new_value):
        self.ui.updateUI()
class MBScene():
    def __init__(self,desktop,win):
	self.desktop = desktop
	self.window = win
	self.layers = []
	self.layers.append(Layer(None))
	self.scenemap = None
	self.top = None
	self.last_update = None
	pybInkscape.inkscape.connect('change_selection', self.show_selection)
	self.last_select = None
	self.lockui=False
	self.tween=None
	self.document = None
	self.dom = None
	self.framerate=12
	self.maxframe=0
	pass

    def show_selection(self,w,obj):
	objs =  self.desktop.selection.list()
	try:
	    o = objs[0]
	    print o.getCenter()
	    if o == self.last_select: 
	        return
	except:
	    self.nameEditor.set_text('')
	    self.last_select = None
	    return
	self.last_select = o
	try:
	    self.nameEditor.set_text(o.getAttribute("inkscape:label"))
	except:
	    self.nameEditor.set_text('')
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
	    if n.name() == 'ns0:scenes':
		self.scenemap={}
		try:
		    cur = int(n.getAttribute("current"))
		except:
		    cur = 1
		self.current = cur

		for s in n.childList():
		    if s.name() == 'ns0:scene':
			try:
			    start = int(s.getAttribute("start"))
			except:
			    traceback.print_exc()
			    continue
			try:
			    end = int(s.getAttribute("end"))
			    if end == None:
				end = start
				pass
			except:
			    end = start
			    pass
			if end > self.maxframe:
			    self.maxframe = end
			try:
			    typ = s.getAttribute('type')
			    if typ == None:
				typ = 'normal'
			except:
			    traceback.print_exc()
			    typ = 'normal'
			link = s.getAttribute("ref")
			self.scenemap[link] = [int(start),int(end),typ]
			if cur >= start and cur <= end:
			    self.currentscene = link
			    pass
			pass
		    pass
		pass
	    pass
	pass
	if self.scenemap==None:
	    #self.desktop.doc().root().repr.setAttribute("xmlns:ns0","http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd")
	    self.dom.setAttribute("xmlns:ns0","http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd")
	    scenes = self.document.createElement("ns0:scenes")
	    node.appendChild(scenes)
    def update(self):
        doc = self.dom
	rdoc = self.document
	for node in doc.childList():
	    if node.name() == 'svg:metadata':
	        for t in node.childList():
		    if t.name() == "ns0:scenes":
		        node.removeChild(t)
			ns = rdoc.createElement("ns0:scenes")
			node.appendChild(ns)
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
	doc = self.dom

        #obs = pybInkscape.PYNodeObserver()
        #obs = LayerAddRemoveWatcher(self)
        #doc.addObserver(obs)
	addEventListener(doc,'DOMNodeInserted',self.updateUI,None)
	addEventListener(doc,'DOMNodeRemoved',self.updateUI,None)
	doc.childList()
	try:
	    self.width = float(doc.getAttribute("width"))
	    self.height= float(doc.getAttribute("height"))
	except:
	    self.width = 640
	    self.height=480
	    
	for node in doc.childList():
	    print node.name()
	    if node.name() == 'svg:metadata':
		self.parseMetadata(node)
		pass
	    elif node.name() == 'svg:g':
		oldscene = None
	        #obs = LayerAttributeWatcher(self)
	        #addEventListener(doc,'DOMAttrModified',self.updateUI,None)
	        #node.addObserver(obs)
		lyobj = Layer(node)
		self.layers.append(lyobj)
		lyobj.current_scene = []
		for scene in node.childList():
		    print scene.getCenter()
		    if scene.name() == 'svg:g':
		        try:
			    label = scene.getAttribute('inkscape:label')
			    if label == 'dup':
			        node.removeChild(scene)
			except:
			    pass

			try:
			    scmap = self.scenemap[scene.getAttribute('id')]
			    if scmap == None:
				lyobj.current_scene.append(scene)
				continue
			except:
			    lyobj.current_scene.append(scene)
			    continue

			lyobj.scenes.append(Scene(scene,scmap[0],scmap[1],scmap[2]))
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
	root = self.dom
	for n in root.childList():
	    self.collectID_recursive(n)
	    pass
	pass
    
    def collectID_recursive(self,node):
	try:
	    self.ID[node.getAttribute('id')] = 1
	except:
	    pass
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
	    if l.node.getAttribute('id') == layer:
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
	rdoc = self.document
	ns = rdoc.createElement("svg:g")
	txt = rdoc.createElement("svg:rect")
	txt.setAttribute("x","0")
	txt.setAttribute("y","0")
	txt.setAttribute("width","100")
	txt.setAttribute("height","100")
	txt.setAttribute("style","fill:#ff00")
	ns.appendChild(txt)
	gid = self.last_line.node.label()+self.newID()
	self.ID[gid]=1
	ns.setAttribute("id",gid)
	ns.setAttribute("inkscape:groupmode","layer")
	self.last_line.node.appendChild(ns)
	print 'Add key ', x
	self.last_line.add_keyframe(x,ns)
	self.update()
	self.last_line.update()
    

    def removeKeyScene(self):
	nth = self.last_frame
	y = self.last_line
	rdoc = self.document
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
	"""
	    Update the scene group according to the curretn scene data. There are a couple of cases.
	    1. If the type of the scene is normal, we display it when it contains the current 
	       frame. Otherwise hide it.
	    2. If the type of the scene is relocate or scale, we need to duplicate the scene group
	       and then modify its transform matrix according to the definition of the scene. Then,
	       hide the original scenr group and display the duplciate scene group. In addition,
	       we may need to delete the old duplicated scene group as well.

	    For each layer, we will always use the duplicated scene group whose name as dup.
	    We will put the duplicated scene group inside it. We will create this group if it is not
	    available.
	"""
	self.current = nth
	self.tween.updateMapping()
	for layer in self._framelines:
	    i=0

	    # Check the duplicated scene group and create it if it is not available
	    try:
	        if layer.duplicateGroup:
	            layer.duplicateGroup.setAttribute("style","display:none")
	    except:
	        print "*"*40
	        layer.duplicateGroup = self.document.createElement("svg:g")
	        layer.duplicateGroup.setAttribute("inkscape:label","dup")
	        layer.duplicateGroup.setAttribute("sodipodi:insensitive","1")
	        layer.duplicateGroup.setAttribute("style","")
	        layer.layer.node.appendChild(layer.duplicateGroup)
	        pass
	    # Create a new group
	    #layer.duplicateGroup = None


	    while i < len(layer._keys):
	        s = layer._keys[i]
		print s.ref.getAttribute("id"),s.idx,s.left_tween,s.right_tween
		if s.right_tween is False:
		    if nth == s.idx+1:
		        s.ref.setAttribute("style","")
		    else:
		        s.ref.setAttribute("style","display:none")
		    i = i + 1
		    continue
		if nth == s.idx + 1:
		    s.ref.setAttribute("style","")
		else:
		    if nth > (s.idx+1) and nth <= (layer._keys[i+1].idx+1):
			if i+2 < len(layer._keys):
			    #s.ref.parent().appendChild(layer.duplicateGroup)
			    s.ref.setAttribute("style","display:none")
	                    layer.duplicateGroup.setAttribute("style","")
			    self.tween.updateTweenContent(layer.duplicateGroup, layer.get_tween_type(s.idx),s, layer._keys[i+2], nth)
			else:
	                    layer.duplicateGroup.setAttribute("style","")
			    #layer.duplicateGroup = s.ref.duplicate(self.document)
			    #layer.duplicateGroup.setAttribute("style","")
			    #layer.duplicateGroup.setAttribute("inkscape:label","dup")
			    #layer.duplicateGroup.setAttribute("sodipodi:insensitive","1")
			    s.ref.setAttribute("style","display:none")
			    #s.ref.parent().appendChild(layer.duplicateGroup)
			    pass
		    else:
		        s.ref.setAttribute("style","display:none")
		i = i + 2
		pass
	    pass
	pass

    def DOMtoItem(self,obj):
	"""
	Find the corresponding PYSPObject object for a DOM object.
	"""
	return self.DOMtoItem_recursive(self.desktop.doc().root(),obj)

    def DOMtoItem_recursive(self,tree,obj):
	nodes = tree.childList()
	for s in nodes:
	    if s.getId() == obj.getAttribute('id'):
	        return s
	    d = self.DOMtoItem_recursive(s,obj)
	    if d != None: return d
	     

    def enterGroup(self,obj):
        for l in self.layers:
	    for s in l.node.childList():
	        if s.getAttribute('id') == obj.getAttribute("id"):
		    self.desktop.setCurrentLayer(self.DOMtoItem(s))
        
    def selectSceneObject(self,frameline, nth):
        i = 0
        while i < len(frameline._keys):
	    s = frameline._keys[i]
	    if s.right_tween is False:
	        if nth == s.idx+1:
		    self.enterGroup(s.ref)
	            self.setTweenType(frameline.get_tween_type(s.idx))
		    return
		else:
		    pass
		i = i + 1
		continue

	    if nth >= (s.idx+1) and nth <= (frameline._keys[i+1].idx+1):
	        self.enterGroup(s.ref)
	        self.setTweenType(frameline.get_tween_type(s.idx))
		return
	    else:
	        pass
	    i = i + 2
	    pass
        pass
    def setTweenType(self,typ):
        if typ == 'normal':
	    self.tweenTypeSelector.set_active(0)
        elif typ == 'relocate':
	    self.tweenTypeSelector.set_active(1)
        elif typ == 'scale':
	    self.tweenTypeSelector.set_active(2)

	
	
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
	self.lockui = True
        self.doEditScene(frame)
	self.lockui = False
        
        
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
	hbox = gtk.HBox()
	label=gtk.Label('')
	label.set_size_request(100,0)
	hbox.pack_start(label,expand=False,fill=True)
	hbox.pack_start(ruler)
	vbox.pack_start(hbox, False)

	#
	# Add a frameline for each layer
	#
	self._framelines = []
	for i in range(len(self.layers)-1,-1,-1):
	    line = frameline.frameline(nframes)
	    hbox = gtk.HBox()
	    label = gtk.Label(self.layers[i].node.getAttribute("inkscape:label"))
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
	    if frameline.node.getAttribute("inkscape:label")==None:
	        frameline.label.set_text('???')
	    else:
	        frameline.label.set_text(frameline.node.getAttribute("inkscape:label"))
	    for scene in layer.scenes:
		frameline.add_keyframe(scene.start-1,scene.node)
		if scene.start != scene.end:
		    frameline.add_keyframe(scene.end-1,scene.node)
		    frameline.tween(scene.start-1,scene.type)
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
    def duplicateKeyScene(self):
        self.last_line.add_keyframe(self.last_frame)
        # Search for the current scene
	i = 0
	while i < len(self.last_line._keys):
	    key = self.last_line._keys[i]
	    if key.idx == self.last_frame:
	        if i == 0:
		    # This is the first frame, we can not duplicate it
		    self.last_line.rm_keyframe(self.last_frame)
		    return
		node = self.duplicateSceneGroup(last_key.ref.getAttribute("id"))
	        key.ref = node
		self.update()
		self.show()
	        self.doEditScene(None)
		return
	    last_key = key
	    i = i + 1
    def duplicateSceneGroup(self,gid):
	# Search for the duplicated group
        doc = self.dom
	rdoc = self.document
	orig = None
	for node in doc.childList():
	    if node.name() == 'svg:g':
	        for t in node.childList():
		    if t.name() == "svg:g":
			if t.getAttribute("id") == gid:
			    orig = t
			    break
	if orig == None:
	    return None
	ns = orig.duplicate(rdoc)
	gid = self.last_line.node.getAttribute('id')+self.newID()
	self.ID[gid]=1
	ns.setAttribute("id",gid)
	ns.setAttribute("inkscape:groupmode","layer")
	self.last_line.node.appendChild(ns)
	return ns
    
    def doEditScene(self,w):
	self.setCurrentScene(self.last_frame+1)
	self.selectSceneObject(self.last_line,self.last_frame+1)
	pass
    
    def doInsertKeyScene(self,w):
	self.lockui=True
	self.insertKeyScene()
	self.lockui=False
	# self.grid.show_all()
	return
    def doDuplicateKeyScene(self,w):
	self.lockui = True
        self.duplicateKeyScene()
	self.lockui = False

    def doRemoveScene(self,w):
	self.lockui = True
	self.removeKeyScene()
	self.lockui = False
	return

    
    def doExtendScene(self,w):
	self.lockui = True
	self.extendScene()
	self.lockui = False
	#self.grid.show_all()
	pass

    def changeObjectLabel(self,w):
	o = self.desktop.selection.list()[0]
	o.setAttribute("inkscape:label", self.nameEditor.get_text())

    def addNameEditor(self,hbox):
	self.nameEditor = gtk.Entry(max=40)
	hbox.pack_start(self.nameEditor,expand=False,fill=False)
	self.editDone = gtk.Button('Set')
	hbox.pack_start(self.editDone,expand=False,fill=False)
	self.editDone.connect('clicked', self.changeObjectLabel)

    def doRun(self,arg):
        """
	    Execute the current animation till the last frame.
	"""
	if self.btnRun.get_label() == "Run":
	    self.btnRun.set_label("Stop")
            self.last_update = glib.timeout_add(1000/self.framerate,self.doRunNext)
	else:
	    self.btnRun.set_label("Run")
	    glib.source_remove(self.last_update)

    def doRunNext(self):
	if self.current >= self.maxframe:
	    self.current = 0
	print self.current,self.maxframe
	self.setCurrentScene(self.current+1)
        self.last_update = glib.timeout_add(1000/self.framerate,self.doRunNext)
        
    
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
	btn=gtk.Button('Duplicate Key')
	btn.connect('clicked', self.doDuplicateKeyScene)
	hbox.pack_start(btn,expand=False,fill=False)
	btn=gtk.Button('Run')
	btn.connect('clicked', self.doRun)
	self.btnRun = btn
	hbox.pack_start(btn,expand=False,fill=False)
	self.addNameEditor(hbox)
	self.addTweenTypeSelector(hbox)

	pass
    def onTweenTypeChange(self,w):
	n = self.tweenTypeSelector.get_active()
	if self.last_line == None:
	    return
	frameline = self.last_line
        i = 0
	found = -1
        while i < len(frameline._keys):
	    s = frameline._keys[i]
	    if s.right_tween is False:
	        if self.last_frame == s.idx:
		    found = s.idx
		    break
		else:
		    pass
		i = i + 1
		continue

	    if self.last_frame >= s.idx and self.last_frame <= frameline._keys[i+1].idx:
		found = s.idx
		break
	    else:
	        pass
	    i = i + 2
	    pass
        pass
	if found == -1: return
	self.last_line.set_tween_type(found,self.tweenTypeSelector.get_active_text())
	self.last_line.update()
	self.update()

    def addTweenTypeSelector(self,hbox):
	tweenbox = gtk.HBox()
	label = gtk.Label('Tween Type')
	tweenbox.pack_start(label)
	
        self.tweenTypeSelector = gtk.combo_box_new_text()
	self.tweenTypeSelector.append_text('normal')
	self.tweenTypeSelector.append_text('relocate')
	self.tweenTypeSelector.append_text('scale')
	self.tweenTypeSelector.set_active(0)
	tweenbox.pack_start(self.tweenTypeSelector, expand=False,fill=False)
	hbox.pack_start(tweenbox,expand=False,fill=False)
	self.tweenTypeSelector.connect('changed', self.onTweenTypeChange)
    
    def onQuit(self, event):
	self.OK = False
	gtk.main_quit()
	pass
    
    def onOK(self,event):
	self.OK = True
	gtk.main_quit()
	pass

    def updateUI(self,node=None,arg=None):
        if self.lockui: return
        self.lockui = True
	self._updateUI()
	self.lockui = False
    def _updateUI(self,node=None,arg=None):
        if self.last_update!= None:
            glib.source_remove(self.last_update)
        self.last_update = glib.timeout_add(300,self.show)
    def show(self):
	self.OK = True
	self.dom = self.desktop.doc().root().repr
	self.document = self.desktop.doc().rdoc
	self.tween = TweenObject(self.document,self.dom)
	self.parseScene()
	self._create_framelines()
	self._update_framelines()
	if self.top == None:
	    self.top = gtk.VBox(False,0)
	    self.desktop.getToplevel().child.child.pack_end(self.top,expand=False)
	else:
	    self.top.remove(self.startWindow)
	vbox = gtk.VBox(False,0)
	self.startWindow = vbox
	self.top.pack_start(vbox,expand=False)
	vbox.pack_start(self.scrollwin,expand=False)
	hbox=gtk.HBox(False,0)
	self.addButtons(hbox)
	vbox.pack_start(hbox,expand=False)

	# self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
	# self.window.connect("destroy", gtk.main_quit)
	# self.window.set_position(gtk.WIN_POS_MOUSE)

	self.top.show_all()
	self.last_update = None
	return False
    pass
