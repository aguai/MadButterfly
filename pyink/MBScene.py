#!/usr/bin/python
# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; fill-column: 79 -*-
# vim: sw=4:ts=8:sts=4:textwidth=79
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
from frameline import frameline, frameruler

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
        if self.type == 'DOMSubtreeModified':
	    self.func(node)
    def notifyAttributeChanged(self,node, name, old_value, new_value):
        # print 'attr',node,name,old_value,new_value
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
	pass

class LayerAddRemoveWatcher(pybInkscape.PYNodeObserver):
    def __init__(self,ui):
        self.ui = ui
	pass

    def notifyChildAdded(self,node,child,prev):
        self.ui.updateUI()
	pass

    def notifyChildRemoved(self,node,child,prev):
        self.ui.updateUI()
	pass

    def notifyChildOrderChanged(self,node,child,prev):
        self.ui.updateUI()
	pass

    def notifyContentChanged(self,node,old_content,new_content):
        self.ui.updateUI()
	pass

    def notifyAttributeChanged(self,node, name, old_value, new_value):
        self.ui.updateUI()
	pass

def _travel_DOM(node):
    nodes = [node]
    while nodes:
	node = nodes.pop(0)
	child = node.firstChild()
	while child:
	    nodes.append(child)
	    child = child.next()
	    pass
	yield node
	pass
    pass

class MBScene_dom(object):
    def dumpattr(self, n):
	s = ""
	for a,v in n.attrib.items():
	    s = s + ("%s=%s"  % (a,v))
	    pass
	return s
	
    def dump(self, node, l=0):
	print " " * l*2,"<", node.tag, self.dumpattr(node),">"
	for n in node:
	    self.dump(n, l+1)
	    pass
	print " " * l * 2,"/>"
	pass

    def _parse_one_scenes(self, scenes):
	self.scenemap = {}
	try:
	    cur = int(n.getAttribute("current"))
	except:
	    cur = 1
	    pass
	self.current = cur
	
	for scene in scenes.childList():
	    if scene.name() != 'ns0:scene':
		continue
	    
	    try:
		start = int(scene.getAttribute("start"))
	    except:
		traceback.print_exc()
		continue
	    try:
		end = int(scene.getAttribute("end"))
	    except:
		end = start
		pass
	    
	    if end > self.maxframe:
		self.maxframe = end
		pass
	    try:
		scene_type = scene.getAttribute('type')
		if scene_type == None:
		    scene_type = 'normal'
		    pass
	    except:
		traceback.print_exc()
		scene_type = 'normal'
		pass
	    link = scene.getAttribute("ref")
	    self.scenemap[link] = [int(start), int(end), scene_type]
	    if cur >= start and cur <= end:
		self.currentscene = link
		pass
	    pass
	pass
    
    def parseMetadata(self, node):
	for n in node.childList():
	    if n.name() == 'ns0:scenes':
		self._parse_one_scenes(n)
		break
	    pass
	else:
	    ns = "http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd"
	    self.root.setAttribute("xmlns:ns0", ns)
	    scenes = self.document.createElement("ns0:scenes")
	    node.appendChild(scenes)
	    pass
	pass
    pass

class MBScene(MBScene_dom):
    _frameline_tween_types = (frameline.TWEEN_TYPE_NONE,
			      frameline.TWEEN_TYPE_SHAPE)
    _tween_obj_tween_types = (TweenObject.TWEEN_TYPE_NORMAL,
			      TweenObject.TWEEN_TYPE_SCALE)
    _tween_type_names = ('normal', 'scale')
    
    def __init__(self, desktop, win, root=None):
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
	self.root = root
	self.framerate=12
	self.maxframe=0
	self._disable_tween_type_selector = False
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
	pass

    def confirm(self,msg):
	vbox = gtk.VBox()
	vbox.pack_start(gtk.Label(msg))
	self.button = gtk.Button('OK')
	vbox.pack_start(self.button)
	self.button.connect("clicked", self.onQuit)
	self.window.add(vbox)
	pass
    
    def update(self):
        doc = self.root
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
			    self.addScenes(lobj, ns)
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
	doc = self.root

	addEventListener(doc,'DOMNodeInserted',self.updateUI,None)
	addEventListener(doc,'DOMNodeRemoved',self.updateUI,None)
	doc.childList()
	try:
	    self.width = float(doc.getAttribute("width"))
	    self.height= float(doc.getAttribute("height"))
	except:
	    self.width = 640
	    self.height=480
	    pass
	    
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
				# XXX: This would stop animation.
				# This function is called by updateUI()
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
	root = self.root
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
	found = False
	for node in self.last_line.node.childList():
	    try:
		label = node.getAttribute("inkscape:label")
	    except:
		continue
	    if label == "dup":
		#FIXME: The duplication here is not perfect. We should
		#       get the element inside the group and apply the
		#       transformation matrix to it directly.
		for n in node.childList():
		    ns.appendChild(n.duplicate(self.document))
		found = True
		node.setAttribute("style","display:none")
		break
	    pass
	pass

	if found == False:
	    txt = rdoc.createElement("svg:rect")
	    txt.setAttribute("x","0")
	    txt.setAttribute("y","0")
	    txt.setAttribute("width","100")
	    txt.setAttribute("height","100")
	    txt.setAttribute("style","fill:#ff00")
	    ns.appendChild(txt)

	gid = self.last_line.node.getAttribute('inkscape:label')+self.newID()
	self.ID[gid]=1
	ns.setAttribute("id",gid)
	ns.setAttribute("inkscape:groupmode","layer")
	self.last_line.node.appendChild(ns)
	self.last_line.add_keyframe(x,ns)
	self.update()
	pass

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
	    pass
	pass
    
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
	    Update the scene group according to the curretn scene
	    data. There are a couple of cases.
	    1. If the type of the scene is normal, we display it when 
	    it contains the current frame. Otherwise hide it.
	    2. If the type of the scene is relocate or scale, we need 
	       to duplicate the scene group and then modify its 
	       transform matrix according to the definition of the 
	       scene. Then, hide the original scenr group and display 
	       the duplciate scene group. In addition, we may need to 
	       delete the old duplicated scene group as well.

	    For each layer, we will always use the duplicated scene 
	    group whose name as dup.
	    We will put the duplicated scene group inside it. We will 
	    create this group if it is not
	    available.
	"""
	self.current = nth
	self.tween.updateMapping()
	idx = nth - 1
	for layer in self._framelines:
	    i=0

	    # Check the duplicated scene group and create it if it is not available
	    try:
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
	    for start_idx, stop_idx, tween_type in layer.get_frame_blocks():
		if start_idx == stop_idx:
		    scene_group = layer.get_frame_data(start_idx)
		    if idx == start_idx:
			scene_group.setAttribute('style', '')
		    else:
			scene_group.setAttribute('style', 'display: none')
			pass
		elif idx == start_idx:
		    layer.duplicateGroup.setAttribute("style","display:none")
		    scene_group = layer.get_frame_data(start_idx)
		    scene_group.setAttribute("style","")
		elif start_idx <= idx and stop_idx >= idx:
		    scene_group = layer.get_frame_data(start_idx)
		    scene_group.setAttribute("style","display:none")
		    layer.duplicateGroup.setAttribute("style","")
		    tween_type_idx = \
			self._frameline_tween_types.index(tween_type)
		    tween_obj_tween_type = \
			self._tween_obj_tween_types[tween_type_idx]
		    
		    next_idx, next_stop_idx, next_tween_type = \
			layer.get_frame_block(stop_idx + 1)
		    if next_idx == -1:
			next_scene_group = scene_group
		    else:
			next_scene_group = layer.get_frame_data(next_idx)
		    
		    nframes = stop_idx - start_idx + 1
		    percent = float(idx - start_idx) / nframes
		    self.tween.updateTweenContent(layer.duplicateGroup,
						  tween_obj_tween_type,
						  scene_group,
						  next_scene_group,
						  percent)
		else:
		    scene_group = layer.get_frame_data(start_idx)
		    scene_group.setAttribute("style","display:none")
		    pass
		pass
	    pass
	pass

    def enterGroup(self,obj):
        for l in self.layers:
	    for s in l.node.childList():
	        if s.getAttribute('id') == obj.getAttribute("id"):
		    self.desktop.setCurrentLayer(s.spitem)
		    pass
		pass
	    pass
	pass
    
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

    def setTweenType(self, tween_type):
	sel_type = MBScene._frameline_tween_types.index(tween_type)
	self._disable_tween_type_selector = True
	self.tweenTypeSelector.set_active(sel_type)
	self._disable_tween_type_selector = False
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
	self.lockui = True
        self.doEditScene(frame)
	self.lockui = False
        pass
        
    def _remove_active_frame(self,widget,event):
        """
	Hide all hover frames. This is a hack. We should use the lost focus event 
	instead in the future to reduce the overhead.
	"""
        for f in self._framelines:
	    if f != widget:
	        f.hide_hover()
		pass
	    pass
	pass
	    
    def _create_framelines(self):
	if not hasattr(self, 'scrollwin'):
	    self.scrollwin = gtk.ScrolledWindow()
	    self.scrollwin.set_policy(gtk.POLICY_AUTOMATIC,
				      gtk.POLICY_AUTOMATIC)
	    self.scrollwin.set_size_request(-1,150)
	    vbox = gtk.VBox()
	    vbox.show()
	    self.scrollwin.add_with_viewport(vbox)
	    self.scrollwin_vbox = vbox
	else:
	    for c in self.scrollwin_vbox.get_children():
	    	self.scrollwin_vbox.remove(c)
	    vbox = self.scrollwin_vbox
	    pass
	
	nframes = 100
	
	ruler = frameruler(nframes)
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
	    line = frameline(nframes)
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
	vbox.show_all()
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
	    last_scene = None
	    for scene in layer.scenes:
		if last_scene and last_scene.end == scene.start:
		    frameline.setRightTween(last_scene.end)
		else:
		    frameline.add_keyframe(scene.start-1,scene.node)
		last_scene = scene
		if scene.start != scene.end:
		    frameline.add_keyframe(scene.end-1,scene.node)
		    tween_type_idx = self._tween_type_names.index(scene.type)
		    tween_type = self._frameline_tween_types[tween_type_idx]
		    frameline.tween(scene.start-1, tween_type)
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
		self.updateUI()
	        self.doEditScene(None)
		return
	    last_key = key
	    i = i + 1
	    pass
	pass

    def duplicateSceneGroup(self,gid):
	# Search for the duplicated group
        doc = self.root
	rdoc = self.document
	orig = None
	for node in doc.childList():
	    if node.name() == 'svg:g':
	        for t in node.childList():
		    if t.name() == "svg:g":
			if t.getAttribute("id") == gid:
			    orig = t
			    break
			pass
		    pass
		pass
	    pass
	if orig == None:
	    return None
	ns = orig.duplicate(rdoc)

	old_nodes = _travel_DOM(orig)
	new_nodes = _travel_DOM(ns)
	for old_node in old_nodes:
	    print old_node
	    old_node_id = old_node.getAttribute('id')
	    new_node = new_nodes.next()
	    new_node.setAttribute('ns0:duplicate-src', old_node_id)
	    pass

	gid = self.last_line.node.getAttribute("inkscape:label")+self.newID()
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
	pass

    def addNameEditor(self,hbox):
	self.nameEditor = gtk.Entry(max=40)
	hbox.pack_start(self.nameEditor,expand=False,fill=False)
	self.editDone = gtk.Button('Set')
	hbox.pack_start(self.editDone,expand=False,fill=False)
	self.editDone.connect('clicked', self.changeObjectLabel)
	pass

    def doRun(self,arg):
        """
	    Execute the current animation till the last frame.
	"""
	if self.btnRun.get_label() == "Run":
	    self.btnRun.set_label("Stop")
	    self.lockui = True
            self.last_update = glib.timeout_add(1000/self.framerate,self.doRunNext)
	else:
	    self.btnRun.set_label("Run")
	    glib.source_remove(self.last_update)
	    self.lockui = False
	    pass

    def doRunNext(self):
	if self.current >= self.maxframe:
	    self.current = 0
	try:
	    self.setCurrentScene(self.current+1)
	except:
	    traceback.print_exc()
	    raise
        self.last_update = glib.timeout_add(1000/self.framerate,self.doRunNext)

    def doInsertScene(self,w):
	self.lockui=True
	self.last_line.insert_frame(self.last_frame)
	self.update()
	self.lockui=False

    def doRemoveScene(self,w):
	self.lockui=True
	self.last_line.remove_frame(self.last_frame)
	self.update()
	self.lockui=False
    
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

	btn=gtk.Button('Duplicate Key')
	btn.connect('clicked', self.doDuplicateKeyScene)
	hbox.pack_start(btn,expand=False,fill=False)

	btn=gtk.Button('Insert')
	btn.connect('clicked', self.doInsertScene)
	hbox.pack_start(btn,expand=False,fill=False)

	btn=gtk.Button('Remove')
	btn.connect('clicked', self.doRemoveScene)
	hbox.pack_start(btn,expand=False,fill=False)

	btn=gtk.Button('Run')
	btn.connect('clicked', self.doRun)
	self.btnRun = btn
	hbox.pack_start(btn,expand=False,fill=False)

	self.addNameEditor(hbox)
	self.addTweenTypeSelector(hbox)
	pass

    def onTweenTypeChange(self, w):
	if self._disable_tween_type_selector:
	    return
	
	if self.last_line == None:
	    return
	frameline = self.last_line
	idx = self.last_frame
        i = 0
	found = -1
	for start_idx, stop_idx, tween_type in frameline.get_frame_blocks():
	    if start_idx <= idx and stop_idx >= idx:
		n = self.tweenTypeSelector.get_active()
		new_tween_type = MBScene._frameline_tween_types[n]
		self.last_line.set_tween_type(start_idx, new_tween_type)
		self.update()
		break
	    pass
	pass

    def addTweenTypeSelector(self,hbox):
	tweenbox = gtk.HBox()
	label = gtk.Label('Tween Type')
	tweenbox.pack_start(label)
	
        self.tweenTypeSelector = gtk.combo_box_new_text()
	self.tweenTypeSelector.append_text('normal')
	#self.tweenTypeSelector.append_text('relocate')
	self.tweenTypeSelector.append_text('scale')
	self.tweenTypeSelector.set_active(0)
	tweenbox.pack_start(self.tweenTypeSelector, expand=False,fill=False)
	hbox.pack_start(tweenbox,expand=False,fill=False)
	self.tweenTypeSelector.connect('changed', self.onTweenTypeChange)
	pass
    
    def onQuit(self, event):
	self.OK = False
	gtk.main_quit()
	pass
    
    def onOK(self,event):
	self.OK = True
	gtk.main_quit()
	pass

    def addScenes(self, frameline, scenes_node):
	doc = self.document
	for start_idx, stop_idx, tween_type in frameline.get_frame_blocks():
	    ref = frameline.get_frame_data(start_idx)
	    tween_type_idx = self._frameline_tween_types.index(tween_type)
	    tween_type_name = self._tween_type_names[tween_type_idx]
	    
	    scene_node = doc.createElement("ns0:scene")
	    scenes_node.appendChild(scene_node)
	    scene_node.setAttribute("start", str(start_idx + 1))
	    if start_idx != stop_idx:
		scene_node.setAttribute("end", str(stop_idx + 1))
		pass
	    scene_node.setAttribute("ref", ref.attribute("id"))
	    scene_node.setAttribute("type", tween_type_name)
	    pass
	pass

    def updateUI(self,node=None,arg=None):
        if self.lockui: return
	
        if self.last_update!= None:
            glib.source_remove(self.last_update)
        self.last_update = glib.timeout_add(300,self._updateUI)
	
	pass
    
    def _updateUI(self,node=None,arg=None):
	self.parseScene()
	self._create_framelines()
	self._update_framelines()
	pass
    
    def show(self):
	self.OK = True
	if not self.root:
	    self.root = self.desktop.doc().root().repr
	    pass
	
	self.document = self.desktop.doc().rdoc
	self.tween = TweenObject(self.document, self.root)
	self._updateUI()
	if self.top == None:
	    self.top = gtk.VBox(False,0)
	    self.desktop.getToplevel().child.child.pack_end(self.top,expand=False)
	else:
	    self.top.remove(self.startWindow)
	    pass
	
	vbox = gtk.VBox(False,0)
	self.startWindow = vbox
	self.top.pack_start(vbox,expand=False)
	vbox.pack_start(self.scrollwin,expand=False)
	hbox=gtk.HBox(False,0)
	self.addButtons(hbox)
	vbox.pack_start(hbox,expand=False)

	self.top.show_all()
	self.last_update = None
	return False
    pass
