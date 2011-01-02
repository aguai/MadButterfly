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
	self.group = node
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

    def notifyChildAdded(self, node, child, prev):
        if self.type == 'DOMNodeInserted':
	    self.func(node, child)
    def notifyChildRemoved(self, node, child, prev):
        if self.type == 'DOMNodeRemoved':
	    self.func(node, child)
    def notifyChildOrderChanged(self,node,child,prev):
        pass
    def notifyContentChanged(self,node,old_content,new_content):
        if self.type == 'DOMSubtreeModified':
	    self.func(node)
    def notifyAttributeChanged(self,node, name, old_value, new_value):
        if self.type == 'DOMAttrModified':
	    self.func(node, name, old_value, new_value)

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


## \brief Monitor changes of DOM-tree.
#
class MBScene_dom_monitor(object):
    def __init__(self, *args, **kws):
	super(MBScene_dom_monitor, self).__init__()

	self._id2node = {}	# map ID to the node in the DOM tree.
	self._group2scene = {}	# map ID of a group to associated scene node.
	pass
    
    def _start_monitor(self):
	self._collect_node_ids()
	
	doc = self._doc
	addEventListener(doc,'DOMNodeInserted', self._on_insert_node, None)
	addEventListener(doc,'DOMNodeRemoved', self._on_remove_node, None)
	addEventListener(doc, 'DOMAttrModified', self._on_attr_modified, None)
	pass

    def _on_insert_node(self, node, child):
	try:
	    child_id = child.getAttribute('id')
	except:
	    pass
	else:
	    if child_id not in self._id2node:
		self._id2node[child_id] = child
		pass
	    pass

	if child.name() == 'ns0:scene':
	    try:
		ref = child.getAttribute('ref')
	    except:
		pass
	    else:
		if ref not in self._group2scene:
		    self._group2scene[ref] = child
		    pass
		pass
	    pass
	pass

    def _on_remove_node(self, node, child):
	try:
	    child_id = child.getAttribute('id')
	except:
	    pass
	else:
	    if child_id not in self._id2node:
		raise ValueError, \
		    'remove a node that is never known (%s)' % (child_id)
	    del self._id2node[child_id]
	    pass
	
	if child.name() == 'ns0:scene':
	    try:
		ref = child.getAttribute('ref')
	    except:
		pass
	    else:
		del self._group2scene[ref]
		pass
	    pass
	pass

    def _on_attr_modified(self, node, name, old_value, new_value):
	if name == 'id' and old_value != new_value:
	    if old_value and (old_value not in self._id2node):
		raise ValueError, \
		    'old ID value of passed node is valid one (%s)' % \
		    (old_value)
	    if (new_value in self._id2node):
		raise ValueError, \
		    'new ID value of passed node is valid one (%s)' % \
		    (new_value)
	    
	    if old_value:
		del self._id2node[old_value]
		pass
	    self._id2node[new_value] = node
	    pass
	elif name == 'ref' and node.name() == 'ns0:scene':
	    if old_value == new_value:
		return
	    if old_value:
		node = self._group2scene[old_value] # use old node.  Binding
						    # may generate a new
						    # wrapper.
		del self._group2scene[old_value]
		pass
	    if new_value:
		self._group2scene[new_value] = node
		pass
	    return
	pass
    
    def _collect_node_ids(self):
	self._id2node = {}
	root = self._root
	for n in root.childList():
	    self._collect_node_ids_recursive(n)
	    pass
	pass
    
    def _collect_node_ids_recursive(self, node):
	try:
	    node_id = node.getAttribute('id')
	except:
	    return
	
	self._id2node[node_id] = node
	for n in node.childList():
	    self._collect_node_ids_recursive(n)
	    pass
	pass
    
    def get_node(self, node_id):
	return self._id2node[node_id]

    def get_scene(self, group_id):
	return self._group2scene[group_id]

    def new_id(self):
	while True:
	    candidate = 's%d' % int(random.random()*100000)
	    if candidate not in self._id2node:
		return candidate
	    pass
	pass
    pass


## \brief Layer of MBScene to manipulate DOM tree.
#
class MBScene_dom(MBScene_dom_monitor):
    # Declare variables, here, for keeping tracking
    _doc = None
    _root = None
    _scenes_group = None
    
    def __init__(self, *args, **kws):
	super(MBScene_dom, self).__init__()
	pass

    def handle_doc_root(self, doc, root):
	self._doc = doc
	self._root = root
	
	self._start_monitor()	# start MBScene_dom_monitor
	self._init_metadata()
	pass
   
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

    def _parse_one_scene(self, scene_node):
	assert scene_node.name() == 'ns0:scene'
	
	start = int(scene_node.getAttribute("start"))
	try:
	    end = int(scene_node.getAttribute("end"))
	except:
	    end = start
	    pass
	
	try:
	    scene_type = scene_node.getAttribute('type')
	    if scene_type == None:
		scene_type = 'normal'
		pass
	except:
	    scene_type = 'normal'
	    pass

	return start, end, scene_type

    def _parse_one_scenes(self, scenes_node):
	self.scenemap = {}
	try:
	    cur = int(n.getAttribute("current"))
	except:
	    cur = 0
	    pass
	self.current = cur
	
	for scene_node in scenes_node.childList():
	    if scene_node.name() != 'ns0:scene':
		continue

	    try:
		start, end, scene_type = self._parse_one_scene(scene_node)
	    except:
		continue
	    
	    if end > self.maxframe:
		self.maxframe = end
		pass
	    
	    link = scene_node.getAttribute("ref")
	    self.scenemap[link] = (start, end, scene_type)
	    if cur >= start and cur <= end:
		self.currentscene = link
		pass
	    pass
	pass
    
    def _init_metadata(self):
	for node in self._root.childList():
	    if node.name() == 'svg:metadata':
		break
	    pass
	else:
	    raise RuntimeError, \
		'can not find <svg:metadata> node in the document'
	
	for n in node.childList():
	    if n.name() == 'ns0:scenes':
		self._scenes_node = n
		break
	    pass
	else:
	    ns = "http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd"
	    self._root.setAttribute("xmlns:ns0", ns)
	    scenes_node = self._doc.createElement("ns0:scenes")
	    node.appendChild(scenes_node)
	    self._scenes_node = scenes_node
	    pass
	pass

    ## \brief Create and add a ns0:scene node under ns0:scenes subtree.
    #
    def _add_scene_node(self, start, end,
			frame_type=TweenObject.TWEEN_TYPE_NORMAL,
			ref=None):
	type_names = ('normal', 'scale')
	scenes_node = self._scenes_node
	doc = self._doc
	
	scene_node = doc.createElement('ns0:scene')
	scene_node.setAttribute('start', str(start))
	if start != end:
	    scene_node.setAttribute('end', str(end))
	    pass
	type_name = type_names[frame_type]
	scene_node.setAttribute('type', type_name)
	if ref:
	    scene_node.setAttribute('ref', ref)
	    pass
	
	scenes_node.appendChild(scene_node)
	
	return scene_node

    ## \brief Create and add a svg:g for a scene under a group for a layer.
    #
    def _add_scene_group(self, layer_idx):
	layer = self.layers[layer_idx]
	doc = self._doc
	
	scene_group = doc.createElement('svg:g')
	gid = self.new_id()
	scene_group.setAttribute("id", gid)
	scene_group.setAttribute("inkscape:groupmode", "layer")

	layer.group.appendChild(scene_group)
	
	return scene_group
    
    def insertKeyScene(self, line, frame):
	"""
	Insert a new key scene into the stage. If the nth is always a
	key scene, we will return without changing anything.  If the
	nth is a filled scene, we will break the original scene into
	two parts. If the nth is out of any scene, we will append a
	new scene.

	"""
	doc = self._doc
	layer_idx = self._framelines.index(line)
	
	scene_group = self._add_scene_group(layer_idx)
	scene_group_id = scene_group.getAttribute('id')
	scene_node = self._add_scene_node(frame, frame, ref=scene_group_id)
	line.add_keyframe(frame, scene_node)

	for node in self.layers[layer_idx].group.childList():
	    try:
		label = node.getAttribute('inkscape:label')
	    except:
		continue
	    if label == 'dup':
		node.setAttribute('style', 'display: none')
		pass
	    pass
	pass

    def add_scene_on_dom(self, frameline, scenes_node):
	doc = self._doc
	for start_idx, stop_idx, tween_type in frameline.get_frame_blocks():
	    scene_node = frameline.get_frame_data(start_idx)
	    tween_type_idx = self._frameline_tween_types.index(tween_type)
	    tween_type_name = self._tween_type_names[tween_type_idx]
	    
	    scene_node = doc.createElement("ns0:scene")
	    scenes_node.appendChild(scene_node)
	    scene_node.setAttribute("start", str(start_idx + 1))
	    if start_idx != stop_idx:
		scene_node.setAttribute("end", str(stop_idx + 1))
		pass
	    scene_node.setAttribute("ref", scene_node.getAttribute("ref"))
	    scene_node.setAttribute("type", tween_type_name)
	    pass
	pass

    def update_scenes_of_dom(self):
        doc = self._root
	rdoc = self._doc
	for node in doc.childList():
	    if node.name() == 'svg:metadata':
	        for t in node.childList():
		    if t.name() == "ns0:scenes":
		        node.removeChild(t)
			scenes_node = rdoc.createElement("ns0:scenes")
			node.appendChild(scenes_node)
			for layer in range(0, len(self._framelines)):
			    lobj = self._framelines[layer]
			    self.add_scene_on_dom(lobj, scenes_node)
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
	doc = self._root

	# TODO: Remove following code sicne this function is for parsing.
	#       Why do this here?
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
	    if node.name() == 'svg:g':
		oldscene = None
		lyobj = Layer(node)
		self.layers.append(lyobj)
		for scene_group in node.childList():
		    if scene_group.name() != 'svg:g':
			continue
		    
		    try:
			label = scene_group.getAttribute('inkscape:label')
			if label == 'dup':
			    # TODO: remove this since this functio is for
			    #       parsing.  Why do this here?
			    node.removeChild(scene_group)
			    continue
		    except:
			pass

		    try:
			scene_group_id = scene_group.getAttribute('id')
			scene_node = self.get_scene(scene_group_id)
			start, stop, tween_type = \
			    self._parse_one_scene(scene_node)
		    except:
			continue
		    
		    lyobj.scenes.append(Scene(scene_node, start, stop,
					      tween_type))
		    pass
		pass
	    pass
	pass

    def getLayer(self, layer):
	for l in self.layers:
	    if l.group.getAttribute('id') == layer:
		return l
	    pass
	return None
    pass

class MBScene(MBScene_dom):
    _frameline_tween_types = (frameline.TWEEN_TYPE_NONE,
			      frameline.TWEEN_TYPE_SHAPE)
    _tween_obj_tween_types = (TweenObject.TWEEN_TYPE_NORMAL,
			      TweenObject.TWEEN_TYPE_SCALE)
    _tween_type_names = ('normal', 'scale')
    
    def __init__(self, desktop, win, root=None):
	super(MBScene, self).__init__()

	self.desktop = desktop
	self.window = win
	self.layers = []
	self.layers.append(Layer(None))
	self.top = None
	self.last_update = None
	pybInkscape.inkscape.connect('change_selection', self.show_selection)
	self.last_select = None
	self._lockui=False
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

		self.update_scenes_of_dom()
		self.last_line._draw_all_frames()
	        self.last_line.update()
		return
	    i = i + 1
	    pass
	pass
    
    def extendScene(self):
	frame_idx = self.last_frame
	frameline = self.last_line

	start, end, scene_type = frameline.get_frame_block_floor(frame_idx)
	if frame_idx <= end:
	   return

	if start < end:
	    frameline.rm_keyframe(end)
	    pass
	
	scene_node = frameline.get_frame_data(start)
	scene_node.setAttribute('end', str(frame_idx))
	frameline.add_keyframe(frame_idx)
	frameline.tween(start, scene_type)
	pass
    
    def setCurrentScene(self, idx):
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
	self.current = idx
	self.tween.updateMapping()
	for frameline in self._framelines:
	    i=0

	    # Check the duplicated scene group and create it if it is not available
	    try:
		frameline.duplicateGroup.setAttribute("style","display:none")
	    except:
	        print "*"*40
	        frameline.duplicateGroup = self.document.createElement("svg:g")
	        frameline.duplicateGroup.setAttribute("inkscape:label","dup")
	        frameline.duplicateGroup.setAttribute("sodipodi:insensitive","1")
	        frameline.duplicateGroup.setAttribute("style","")
	        frameline.layer.group.appendChild(frameline.duplicateGroup)
	        pass
	    # Create a new group
	    for start_idx, stop_idx, tween_type in frameline.get_frame_blocks():
		if start_idx == stop_idx:
		    scene_node = frameline.get_frame_data(start_idx)
		    scene_group_id = scene_node.getAttribute('ref')
		    scene_group = self.get_node(scene_group_id)
		    if idx == start_idx:
			scene_group.setAttribute('style', '')
		    else:
			scene_group.setAttribute('style', 'display: none')
			pass
		elif idx == start_idx:
		    frameline.duplicateGroup.setAttribute("style","display:none")
		    scene_node = frameline.get_frame_data(start_idx)
		    scene_node.setAttribute("style","")
		elif start_idx <= idx and stop_idx >= idx:
		    scene_node = frameline.get_frame_data(start_idx)
		    scene_group_id = scene_node.getAttribute('ref')
		    scene_group = self.get_node(scene_group_id)
		    scene_group.setAttribute("style","display:none")
		    frameline.duplicateGroup.setAttribute("style","")
		    tween_type_idx = \
			self._frameline_tween_types.index(tween_type)
		    tween_obj_tween_type = \
			self._tween_obj_tween_types[tween_type_idx]
		    
		    try:
			next_idx, next_stop_idx, next_tween_type = \
			    frameline.get_frame_block(stop_idx + 1)
		    except:
			next_scene_node = scene_node
		    else:
			next_scene_node = frameline.get_frame_data(next_idx)
			pass

		    next_scene_group_id = next_scene_node.getAttribute('ref')
		    next_scene_group = self.get_node(next_scene_group_id)
		    
		    nframes = stop_idx - start_idx + 1
		    percent = float(idx - start_idx) / nframes
		    self.tween.updateTweenContent(frameline.duplicateGroup,
						  tween_obj_tween_type,
						  scene_group,
						  next_scene_node,
						  percent)
		else:
		    scene_node = frameline.get_frame_data(start_idx)
		    scene_group_id = scene_node.getAttribute('ref')
		    scene_group = self.get_node(scene_group_id)
		    scene_group.setAttribute("style","display:none")
		    pass
		pass
	    pass
	pass

    def enterGroup(self, obj):
        for l in self.layers:
	    for s in l.group.childList():
	        if s.getAttribute('id') == obj.getAttribute("id"):
		    self.desktop.setCurrentLayer(s.spitem)
		    pass
		pass
	    pass
	pass
    
    def selectSceneObject(self, frameline, frame_idx):
	try:
	    start, stop, tween_type = frameline.get_frame_block(frame_idx)
	except:
	    return

	scene_node = frameline.get_frame_data(start)
	scene_group_id = scene_node.getAttribute('ref')
	scene_group = self.get_node(scene_group_id)
	self.enterGroup(scene_group)
	self.setTweenType(tween_type)
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
    
    def onCellClick(self, line, frame, but):
	self.last_line = line
	self.last_frame = frame
	self.last_line.active_frame(frame)
	self._lockui = True
        self.doEditScene(None)
	self._lockui = False
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
	    label = gtk.Label(self.layers[i].group.getAttribute("inkscape:label"))
	    label.set_size_request(100,0)
	    hbox.pack_start(label,expand=False,fill=True)
	    hbox.pack_start(line)
	    line.set_size_request(nframes * 10, 20)
	    vbox.pack_start(hbox, False)
	    line.label = label
	    self._framelines.append(line)
	    line.connect(line.FRAME_BUT_PRESS, self.onCellClick)
	    line.nLayer = i
	    line.layer_group = self.layers[i].group
	    line.layer = self.layers[i]
	    line.connect('motion-notify-event', self._remove_active_frame)
	    pass
	vbox.show_all()
	pass

    ## \brief Update conetent of framelines according layers.
    #
    def _update_framelines(self):
	for frameline in self._framelines:
	    layer = frameline.layer
	    if frameline.layer_group.getAttribute("inkscape:label")==None:
	        frameline.label.set_text('???')
	    else:
	        frameline.label.set_text(frameline.layer_group.getAttribute("inkscape:label"))
	    last_scene = None
	    for scene in layer.scenes:
		if last_scene and last_scene.end == scene.start:
		    frameline.setRightTween(last_scene.end)
		else:
		    frameline.add_keyframe(scene.start, scene.node)
		last_scene = scene
		if scene.start != scene.end:
		    frameline.add_keyframe(scene.end, scene.node)
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
        # Search for the current scene
	i = 0
	while i < len(self.last_line._keys):
	    key = self.last_line._keys[i]
	    if key.idx == self.last_frame:
	        if i == 0:
		    # This is the first frame, we can not duplicate it
		    return
		self.last_line.add_keyframe(self.last_frame)
		last_scene_node = last_key.ref
		last_scene_group_id = last_scene_node.getAttribute('ref')
		scene_group = self.duplicateSceneGroup(last_scene_group_id)
		scene_node = self._add_scene_node(self.last_frame,
						  self.last_frame,
						  ref=last_scene_group_id)
	        key.ref = scene_node
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
	scene_group = orig.duplicate(rdoc)

	old_nodes = _travel_DOM(orig)
	new_nodes = _travel_DOM(scene_group)
	for old_node in old_nodes:
	    print old_node
	    old_node_id = old_node.getAttribute('id')
	    new_node = new_nodes.next()
	    new_node.setAttribute('ns0:duplicate-src', old_node_id)
	    pass

	gid = self.last_line.layer_group.getAttribute("inkscape:label")+self.new_id()
	scene_group.setAttribute("id",gid)
	scene_group.setAttribute("inkscape:groupmode","layer")
	self.last_line.layer_group.appendChild(scene_group)
	return scene_group
    
    def doEditScene(self, w):
	self.setCurrentScene(self.last_frame)
	self.selectSceneObject(self.last_line, self.last_frame)
	pass
    
    def doInsertKeyScene(self,w):
	self._lockui=True
	self.insertKeyScene(self.last_line, self.last_frame)
	self.selectSceneObject(self.last_line, self.last_frame)
	self._lockui=False
	# self.grid.show_all()
	return
    
    def doDuplicateKeyScene(self,w):
	self._lockui = True
        self.duplicateKeyScene()
	self._lockui = False

    def doRemoveScene(self,w):
	self._lockui = True
	self.removeKeyScene()
	self._lockui = False
	return

    
    def doExtendScene(self,w):
	self._lockui = True
	self.extendScene()
	self._lockui = False
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
	    self._lockui = True
            self.last_update = glib.timeout_add(1000/self.framerate,self.doRunNext)
	else:
	    self.btnRun.set_label("Run")
	    glib.source_remove(self.last_update)
	    self._lockui = False
	    pass

    def doRunNext(self):
	if self.current >= self.maxframe:
	    self.current = 0
	try:
	    self.setCurrentScene(self.current)
	except:
	    traceback.print_exc()
	    raise
        self.last_update = glib.timeout_add(1000/self.framerate,self.doRunNext)

    def doInsertScene(self,w):
	self._lockui=True
	self.last_line.insert_frame(self.last_frame)
	self.update_scenes_of_dom()
	self._lockui=False

    def doRemoveScene(self,w):
	self._lockui=True
	self.last_line.remove_frame(self.last_frame)
	self.update_scenes_of_dom()
	self._lockui=False
    
    def addButtons(self,hbox):
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
		self.update_scenes_of_dom()
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

    def updateUI(self, node=None, child=None, arg=None):
        if self._lockui: return
	
        if self.last_update!= None:
            glib.source_remove(self.last_update)
        self.last_update = glib.timeout_add(300,self._updateUI)
	
	pass
    
    def _updateUI(self,node=None,arg=None):
	self._lockui = True
	self.parseScene()
	self._create_framelines()
	self._update_framelines()
	self._lockui = False
	pass
    
    def show(self):
	self.OK = True
	if not self.root:
	    self.root = self.desktop.doc().root().repr
	    pass
	
	self.document = self.desktop.doc().rdoc
	self.handle_doc_root(self.document, self.root)
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
