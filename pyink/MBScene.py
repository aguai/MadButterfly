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
# This class monitors DOM-tree to maintain _maxframe and maps for node ID to
# node and scene group ID to scene node.
class MBScene_dom_monitor(object):
    def __init__(self, *args, **kws):
	super(MBScene_dom_monitor, self).__init__()

	self._maxframe = 0
	self._id2node = {}	# map ID to the node in the DOM tree.
	self._group2scene = {}	# map ID of a group to associated scene node.
	pass
    
    def _start_monitor(self):
	self._collect_node_ids()
	self._collect_all_scenes()
	
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

	    try:
		start = child.getAttribute('start')
		self._maxframe = max(int(start), self._maxframe)
	    except:
		pass
	    try:
		start = child.getAttribute('end')
		self._maxframe = max(int(start), self._maxframe)
	    except:
		pass
	    pass
	pass

    def _find_maxframe(self, scenes_node):
	maxframe = 0
	for child in scenes_node.childList():
	    if child.name() != 'ns0:scene':
		continue
	    
	    try:
		start = child.getAttribute('start')
		maxframe = max(int(start), maxframe)
	    except:
		pass
	    try:
		end = child.getAttribute('end')
		maxframe = max(int(end), maxframe)
	    except:
		pass
	    pass
	return maxframe

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

	    try:
		if node.name() == 'ns0:scenes' and \
			(int(child.getAttribute('start')) == self._maxframe or
			 int(child.getAttribute('end')) == self._maxframe):
		    self._maxframe = self._find_maxframe(node)
		    pass
	    except:
		pass
	    pass
	pass

    def _on_attr_modified(self, node, name, old_value, new_value):
	if name == 'id' and old_value != new_value:
	    if old_value and (old_value not in self._id2node):
		raise ValueError, \
		    'old ID value of passed node is invalid one (%s)' % \
		    (old_value)
	    if (new_value in self._id2node):
		raise ValueError, \
		    'new ID value of passed node is invalid one (%s)' % \
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
	    pass
	elif (name in ('start', 'end')) and node.name() == 'ns0:scene':
	    self._maxframe = max(int(new_value), self._maxframe)
	    pass
	pass
    
    ## \brief Collect ID of nodes in the document.
    #
    # It is used to implement a fast mapping from an ID to the respective node.
    #
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
	    
	    group_id = scene_node.getAttribute("ref")
	    self._group2scene[group_id] = (start, end, scene_type)
	    pass
	pass

    ## \brief Parse all scenes node in svg:metadata subtree.
    #
    def _collect_all_scenes(self):
	root = self._root
	for child in root.childList():
	    if child.name() != 'svg:metadata':
		continue

	    metadata_node = child
	    for metachild in metadata_node.childList():
		if metachild.name() == 'ns0:scenes':
		    self._parse_one_scenes(metachild)
		    self._maxframe = self._find_maxframe(metachild)
		    pass
		pass
	    pass
	pass
    
    ## \brief Return the node with given ID.
    #
    def get_node(self, node_id):
	return self._id2node[node_id]

    ## \brief Return a scene node corresponding to a scene group of given ID.
    #
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
# This class maintains layers information, and provides functions to create,
# change and destroy scene node and scene group.  A scene node is a 'ns0:scene'
# in 'ns0:scenes' tag.  A scene group is respective 'svg:g' for a scene.
#
class MBScene_dom(MBScene_dom_monitor):
    # Declare variables, here, for keeping tracking
    _doc = None
    _root = None
    
    def __init__(self, *args, **kws):
	super(MBScene_dom, self).__init__()
	pass

    ## \brief Create a scenes node if not existed.
    #
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

    def _parse_all_layers(self):
	root = self._root
	layers = self._layers
	
	for child in root.childList():
	    if child.name() != 'svg:g':
		continue

	    layer_group = child
	    layer = Layer(layer_group)
	    layer.idx = len(layers)
	    layers.append(layer)
	    self.parse_layer(layer.idx)
	    pass
	pass

    def handle_doc_root(self, doc, root):
	self._doc = doc
	self._root = root
	self._layers = []
	
	self._start_monitor()	# start MBScene_dom_monitor
	self._init_metadata()
	self._parse_all_layers()
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

    ## \brief Create and add a ns0:scene node under ns0:scenes subtree.
    #
    def add_scene_node(self, start, end,
		       frame_type=TweenObject.TWEEN_TYPE_NORMAL,
		       ref=None):
	type_names = ('normal', 'scale')
	scenes_node = self._scenes_node
	doc = self._doc
	
	scene_node = doc.createElement('ns0:scene')
	self.chg_scene_node(scene_node, start=start)
	if start != end:
	    self.chg_scene_node(scene_node, end=end)
	    pass
	type_name = type_names[frame_type]
	self.chg_scene_node(scene_node, tween_type=type_name)
	if ref:
	    self.chg_scene_node(scene_node, ref=ref)
	    pass
	
	scenes_node.appendChild(scene_node)
	
	return scene_node

    ## \brief Change attributes of a scene node.
    #
    # This is here to monitor changes of scene node.
    def chg_scene_node(self, scene_node, start=None, end=None,
			tween_type=None, ref=None):
	if start:
	    scene_node.setAttribute('start', str(start))
	    pass
	if end:
	    scene_node.setAttribute('end', str(end))
	    pass
	if tween_type:
	    scene_node.setAttribute('type', tween_type)
	    pass
	if ref:
	    scene_node.setAttribute('ref', ref)
	    pass
	pass

    def rm_scene_node(self, scene_node):
	self._scenes_node.removeChild(scene_node)
	pass

    ## \brief Create and add a svg:g for a scene under a group for a layer.
    #
    def add_scene_group(self, layer_idx):
	layer = self._layers[layer_idx]
	doc = self._doc
	
	scene_group = doc.createElement('svg:g')
	gid = self.new_id()
	scene_group.setAttribute("id", gid)
	scene_group.setAttribute("inkscape:groupmode", "layer")

	layer.group.appendChild(scene_group)
	
	return scene_group
    
    def parse_layer(self, layer_idx):
	layer = self._layers[layer_idx]
	layer_group = layer.group
	
	for child in layer_group.childList():
	    if child.name() != 'svg:g':
		continue
	    try:
		child_id = child.getAttribute('id')
		scene_node = self.get_scene(child_id)
	    except:
		continue
	    
	    layer.scenes.append(scene_node)
	    pass
	pass

    ## \brief Add/insert a layer at given position.
    #
    # \param layer_idx is the position in the layer list.
    #
    def add_layer(self, layer_idx, layer_group):
	layers = self._layers
	
	layer = Layer(layer_group)
	if layer_idx >= len(layers):
	    layers.append(layer)
	else:
	    layers.insert(layer_idx, layer)
	    for idx in range(layer_idx, len(layers)):
		layers[idx].idx = idx
		pass
	    pass
	pass

    def rm_layer(self, layer_idx):
	layers = self._layers

	del layers[layer_idx]

	for idx in range(layer_idx, len(layers)):
	    layers[idx].idx = idx
	    pass
	pass
    pass

## \brief Maintain frameline list for MBScene.
#
class MBScene_framelines(object):
    _frameline_tween_types = (frameline.TWEEN_TYPE_NONE,
			      frameline.TWEEN_TYPE_SHAPE)

    _framelines = None
    
    def __init__(self, *args, **kws):
	super(MBScene_framelines, self).__init__(*args, **kws)
	pass
    
    def _remove_active_frame(self,widget,event):
        """
	Hide all hover frames. This is a hack. We should use the lost focus
	event instead in the future to reduce the overhead.
	"""
        for f in self._framelines:
	    if f != widget:
	        f.hide_hover()
		pass
	    pass
	pass

    ## \brief Add a frameline into the frameline box for the given layer.
    #
    def _add_frameline(self, layer_idx):
	if layer_idx > len(self._framelines):
	    raise ValueError, 'layer number should be a consequence'

	vbox = self._frameline_vbox
	
	line = frameline(self._num_frames_of_line)
	line.set_size_request(self._num_frames_of_line * 10, 20)
	
	hbox = gtk.HBox()
	label = gtk.Label('')
	label.set_size_request(100,0)
	hbox.pack_start(label,expand=False, fill=True)
	hbox.pack_start(line)
	vbox.pack_start(hbox, False)
	
	if layer_idx != len(self._framelines):
	    vbox.reorder_child(hbox, layer_idx + 1) # there is ruler at pos 0
	    pass
	
	self._framelines[layer_idx: layer_idx] = [line]
	
	line.label = label
	line.layer_idx = layer_idx
	line.connect(line.FRAME_BUT_PRESS, self.onCellClick)
	line.connect('motion-notify-event', self._remove_active_frame)
	pass
    
    ## \brief Remove the given frameline from the frameline box.
    #
    def _remove_frameline(self, layer_idx):
	vbox = self._frameline_vbox
	line = self._framelines[layer_idx]
	
	hbox = line.parent
	vbox.remove(hbox)
	del self._framelines[layer_idx]
	pass

    def _init_framelines(self):
	self._framelines = []
	
	box = gtk.ScrolledWindow()
	self._frameline_box = box
	box.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	box.set_size_request(-1, 150)
	vbox = gtk.VBox()
	self._frameline_vbox = vbox
	box.add_with_viewport(vbox)
	
	nframes = self._num_frames_of_line
	
	#
	# Set up a ruler
	#
	ruler = frameruler(nframes)
	ruler.set_size_request(nframes * 10, 20)
	ruler.show()
	hbox = gtk.HBox()
	label=gtk.Label('')
	label.set_size_request(100,0)
	hbox.pack_start(label,expand=False,fill=True)
	hbox.pack_start(ruler)
	vbox.pack_start(hbox, False)
	pass

    ## \brief Show framelines on the screen.
    #
    # When a frameline was inserted or removed, it would not be showed
    # immediately.  This function is used to notify toolkit to update the
    # screen and drawing framelines.
    def _show_framelines(self):
	self._frameline_vbox.show_all()
	pass
    pass

## \brief MBScene connect GUI and DOM-tree
#
# This class connect behavior of GUI to the DOM-tree.  All about GUI is
# implemented by this class.  It use API provided by MBScene_dom to reflect
# actions to the DOM-tree.
#
class MBScene(MBScene_dom, MBScene_framelines):
    _tween_obj_tween_types = (TweenObject.TWEEN_TYPE_NORMAL,
			      TweenObject.TWEEN_TYPE_SCALE)
    _tween_type_names = ('normal', 'scale')
    
    _num_frames_of_line = 100
    
    def __init__(self, desktop, win, root=None):
	super(MBScene, self).__init__()

	self.desktop = desktop
	self.window = win
	self.top = None
	self.last_update = None
	pybInkscape.inkscape.connect('change_selection', self.show_selection)
	self.last_select = None
	self._lockui = False
	self.tween = None
	self.document = None
	self.root = root
	self.framerate = 12
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
	
	scene_group = self.add_scene_group(layer_idx)
	scene_group_id = scene_group.getAttribute('id')
	scene_node = self.add_scene_node(frame, frame, ref=scene_group_id)
	line.add_keyframe(frame, scene_node)

	for node in self._layers[layer_idx].group.childList():
	    try:
		label = node.getAttribute('inkscape:label')
	    except:
		continue
	    if label == 'dup':
		node.setAttribute('style', 'display: none')
		pass
	    pass
	pass

    def removeKeyScene(self, frameline, frame_idx):
	start, end, scene_type = frameline.get_frame_block(frame_idx)
	scene_node = frameline.get_frame_data(start)
	
	frameline.rm_keyframe(start)
	if start != end:
	    frameline.rm_keyframe(end)
	    pass
	
	scene_group_id = scene_node.getAttribute('ref')
	scene_group = self.get_node(scene_group_id)
	scene_group.parent().removeChild(scene_group)
	scene_node.parent().removeChild(scene_node)

	try:
	    frameline.duplicateGroup.setAttribute('style', 'display: none')
	except AttributeError:
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
	self.chg_scene_node(scene_node, end=frame_idx)
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

	    # Check the duplicated scene group and create it if it is not
	    # available
	    try:
		frameline.duplicateGroup.setAttribute("style","display:none")
	    except:
	        print "*" * 40
		layer_idx = frameline.layer_idx
		layer = self._layers[layer_idx]
		for child in layer.group.childList():
		    label = child.getAttribute('inkscape:label')
		    if label == 'dup':
			frameline.duplicateGroup = child
			break
		    pass
		else:
		    duplicateGroup = self.document.createElement("svg:g")
		    duplicateGroup.setAttribute("inkscape:label","dup")
		    duplicateGroup.setAttribute("sodipodi:insensitive","1")
		    duplicateGroup.setAttribute("style","")
		    
		    layer.group.appendChild(duplicateGroup)
		    frameline.duplicateGroup = duplicateGroup
		    pass
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
		    scene_group_id = scene_node.getAttribute('ref')
		    scene_group = self.get_node(scene_group_id)
		    scene_group.setAttribute("style","")
		elif start_idx < idx and stop_idx >= idx:
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
		    print tween_obj_tween_type
		    self.tween.updateTweenContent(frameline.duplicateGroup,
						  tween_obj_tween_type,
						  scene_group,
						  next_scene_group,
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
        for l in self._layers:
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
	sel_type = self._frameline_tween_types.index(tween_type)
	self._disable_tween_type_selector = True
	self.tweenTypeSelector.set_active(sel_type)
	self._disable_tween_type_selector = False
	pass
	
    ## \brief Remove the layer that lost the layer group.
    #
    # This function is called when a layer group being removed from the
    # DOM-tree.
    def _remove_lost_group_layer(self, layer_idx):
	layer = self._layers[layer_idx]
	frameline = self._framelines[layer_idx]
	for start, end, tween_type in frameline.get_frame_blocks():
	    scene_node = frameline.get_frame_data(start)
	    self.rm_scene_node(scene_node)
	    pass

	self._remove_frameline(layer_idx) # TODO
	self._show_framelines()
	del self._layers[layer_idx]
	pass

    ## \brief Make status of layers is updated when DOM is changed.
    #
    # When DOM-tree is changed, this function make sure layer information is up
    # to date.
    def _make_layers_integral(self):
	root = self._root
	root_id = root.getAttribute('id')
	
	# Remove group of removed layers
	layer_idx = 0
	while layer_idx < len(self._layers):
	    layer = self._layers[layer_idx]
	    
	    if layer.group.name() != 'svg:g':
		self._remove_lost_group_layer(layer_idx) # TODO
		continue

	    parent = layer.group.parent()

	    if not parent:	# parent is None when a node being removed from
				# DOM-tree.
		self._remove_lost_group_layer(layer_idx)
		continue

	    if parent.name() != 'svg:svg':
		self._remove_lost_group_layer(layer_idx)
		continue

	    if parent.getAttribute('id') != root_id:
		self._remove_lost_group_layer(layer_idx)
		continue

	    layer_idx = layer_idx + 1
	    pass

	# Add new layers
	layer_idx = 0
	for child in root.childList():
	    if child.name() != 'svg:g':
		continue
	    
	    try:
		layer = self._layers[layer_idx]
	    except IndexError:
		layer = None
	    else:
		layer_group_id = layer.group.getAttribute('id')
		pass
	    
	    child_id = child.getAttribute('id')
	    if (not layer) or layer_group_id != child_id:
		self.add_layer(layer_idx, child)
		self.parse_layer(layer_idx)
		self._add_frameline(layer_idx)
		
		layer = self._layers[layer_idx]
		frameline = self._framelines[layer_idx]
		try:
		    label = layer.group.getAttribute('inkscape:label')
		except:
		    label = layer.group.getAttribute('id')
		    pass
		frameline.label.set_text(label)
		
		self._show_framelines()
		pass
	    
	    layer_idx = layer_idx + 1
	    pass
	pass

    def _add_frameline_for_layers(self):
	for layer_idx in range(len(self._layers)):
	    self._add_frameline(layer_idx)
	    line = self._framelines[layer_idx]
	    layer = self._layers[layer_idx]
	    try:
		label = layer.group.getAttribute('inkscape:label')
	    except:
		label = layer.group.getAttribute('id')
		pass
	    line.label.set_text(label)
	    pass
	pass
    
    def duplicateKeyScene(self):
        # Search for the current scene
	frameline = self.last_line
	frame_idx = self.last_frame
	
	try:
	    start, end, scene_type = frameline.get_frame_block_floor(frame_idx)
	except:
	    return
	if end >= frame_idx:
	    return

	prev_scene_node = frameline.get_frame_data(start)
	prev_scene_group_id = prev_scene_node.getAttribute('ref')
	
	scene_group = self.duplicateSceneGroup(prev_scene_group_id)
	scene_group_id = scene_group.getAttribute('id')
	scene_node = self.add_scene_node(frame_idx, frame_idx,
					 ref=scene_group_id)

	frameline.add_keyframe(frame_idx, scene_node)

	self.setCurrentScene(frame_idx)
	pass

    def duplicateSceneGroup(self,gid):
	# Search for the duplicated group
        root = self._root
	doc = self._doc
	orig = self.get_node(gid)
	scene_group = orig.duplicate(doc)

	old_nodes = _travel_DOM(orig)
	new_nodes = _travel_DOM(scene_group)
	for old_node in old_nodes:
	    print old_node
	    old_node_id = old_node.getAttribute('id')
	    new_node = new_nodes.next()
	    new_node.setAttribute('ns0:duplicate-src', old_node_id)
	    pass

	layer = self._layers[self.last_line.layer_idx]
	gid = layer.group.getAttribute("inkscape:label")+self.new_id()
	scene_group.setAttribute("id",gid)
	scene_group.setAttribute("inkscape:groupmode","layer")
	layer.group.appendChild(scene_group)
	return scene_group
    
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

    def updateUI(self, *args):
        if self._lockui: return
	
        if self.last_update!= None:
            glib.source_remove(self.last_update)
        self.last_update = glib.timeout_add(300,self._updateUI)
	
	pass
    
    def _updateUI(self,node=None,arg=None):
	self._lockui = True
	self._make_layers_integral()
	self._lockui = False
	pass
    
    def onCellClick(self, line, frame, but):
	self.last_line = line
	self.last_frame = frame
	self.last_line.active_frame(frame)
	self._lockui = True
        self.doEditScene(None)
	self._lockui = False
        pass
        
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
	self.removeKeyScene(self.last_line, self.last_frame)
	self._lockui = False
	return

    
    def doExtendScene(self,w):
	self._lockui = True
	self.extendScene()
	self._lockui = False
	pass

    def doRun(self,arg):
        """
	    Execute the current animation till the last frame.
	"""
	if self.btnRun.get_label() == "Run":
	    self.btnRun.set_label("Stop")
	    self._lockui = True
	    tmout = 1000 / self.framerate
            self.last_update = glib.timeout_add(tmout, self.doRunNext)
	else:
	    self.btnRun.set_label("Run")
	    glib.source_remove(self.last_update)
	    self._lockui = False
	    pass
	pass

    def doRunNext(self):
	if self.current > self._maxframe:
	    self.current = 0
	    pass
	try:
	    self.setCurrentScene(self.current)
	except:
	    traceback.print_exc()
	    raise
	self.current = self.current + 1
	tmout = 1000 / self.framerate
        self.last_update = glib.timeout_add(tmout, self.doRunNext)
	pass

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
	start, end, old_tween_type = frameline.get_frame_block(self.last_frame)

	type_idx = self.tweenTypeSelector.get_active()
	tween_type = self._frameline_tween_types[type_idx]
	type_name = self._tween_type_names[type_idx]
	
	frameline.tween(start, tween_type)
	
	scene_node = frameline.get_frame_data(start)
	self.chg_scene_node(scene_node, tween_type=type_name)
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

    def show(self):
	self.OK = True
	if not self.root:
	    self.root = self.desktop.doc().root().repr
	    pass
	
	self.document = self.desktop.doc().rdoc
	self.handle_doc_root(self.document, self.root)
	self.tween = TweenObject(self.document, self.root)
	self._init_framelines()
	self._add_frameline_for_layers()
	self._show_framelines()
	
	if self.top == None:
	    self.top = gtk.VBox(False,0)
	    toplevel = self.desktop.getToplevel()
	    toplevel.child.child.pack_end(self.top,expand=False)
	else:
	    self.top.remove(self.startWindow)
	    pass
	
	vbox = gtk.VBox(False,0)
	self.startWindow = vbox
	self.top.pack_start(vbox,expand=False)
	vbox.pack_start(self._frameline_box,expand=False)
	hbox=gtk.HBox(False,0)
	self.addButtons(hbox)
	vbox.pack_start(hbox,expand=False)

	doc = self.document
	addEventListener(doc,'DOMNodeInserted', self.updateUI, None)
	addEventListener(doc,'DOMNodeRemoved', self.updateUI, None)
	
	self.top.show_all()
	self.last_update = None
	return False
    pass
