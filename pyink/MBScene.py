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
    def __init__(self, node):
	self.scenes = []
	self.group = node
	pass
    pass

class ObjectWatcher(pybInkscape.PYNodeObserver):
    def __init__(self, obj, type, func, arg):
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

def addEventListener(obj, type, func, arg):
    obs = ObjectWatcher(obj, type, func, arg)
    obj.addSubtreeObserver(obs)
    pass

## \brief Iterator to travel a sub-tree of DOM.
#
def _DOM_iterator(node):
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
class MBScene_domview_monitor(object):
    def __init__(self, *args, **kws):
	super(MBScene_domview_monitor, self).__init__()

	self._maxframe = 0
	self._id2node = {}	# map ID to the node in the DOM tree.
	self._group2scene = {}	# map ID of a group to associated scene node.
	pass
    
    def _start_monitor(self):
	self._collect_node_ids()
	self._collect_all_scenes()
	
	doc = self._doc
	addEventListener(doc, 'DOMNodeInserted', self._on_insert_node, None)
	addEventListener(doc, 'DOMNodeRemoved', self._on_remove_node, None)
	addEventListener(doc, 'DOMAttrModified', self._on_attr_modified, None)
	pass

    def _on_insert_node(self, node, child):
	for cchild in child.childList():
	    self._on_insert_node(child, cchild)
	    pass
	
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
	for cchild in child.childList():
	    self._on_remove_node(child, cchild)
	    pass
	
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
	    self._group2scene[group_id] = scene_node
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


## \brief This layer provide a data view to the DOM-tree.
#
# This class maintains layers information, and provides functions to create,
# change and destroy scene node and scene group.  A scene node is a 'ns0:scene'
# in 'ns0:scenes' tag.  A scene group is respective 'svg:g' for a scene.
#
class MBScene_domview(MBScene_domview_monitor):
    # Declare variables, here, for keeping tracking
    _doc = None
    _root = None
    
    def __init__(self, *args, **kws):
	super(MBScene_domview, self).__init__()
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
	
	self._start_monitor()	# start MBScene_domview_monitor
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
	if start is not None:
	    scene_node.setAttribute('start', str(start))
	    pass
	if end is not None:
	    scene_node.setAttribute('end', str(end))
	    pass
	if tween_type is not None:
	    scene_node.setAttribute('type', tween_type)
	    pass
	if ref is not None:
	    scene_node.setAttribute('ref', ref)
	    pass
	pass

    def rm_scene_node(self, scene_node):
	self._scenes_node.removeChild(scene_node)
	pass

    def rm_scene_node_n_group(self, scene_node):
	scene_group_id = scene_node.getAttribute('ref')
	scene_group_node = self.get_node(scene_group_id)
	scene_group_node.parent().removeChild(scene_group_node)
	
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
    def insert_layer(self, layer_idx, layer_group):
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

    ## \brief Remove layer and associated scene nodes and scene groups.
    #
    def rm_layer(self, layer_idx):
	layers = self._layers

	for layer in layers:
	    for scene_node in layer.scenes:
		scene_group_id = scene_node.getAttribute('ref')
		scene_group_node = self.get_node(scene_group_id)
		scene_group_node.parent().removeChild(scene_group_node)
		
		scene_node.parent().removeChild(scene_node)
		pass
	    pass
	
	del layers[layer_idx]

	for idx in range(layer_idx, len(layers)):
	    layers[idx].idx = idx
	    pass
	pass

    def get_layer_num(self):
	return len(self._layers)

    def find_layer_n_scene_of_node(self, node_id):
	for layer_idx, layer in enumerate(self._layers):
	    for scene_node in layer.scenes:
		scene_group_id = scene_node.getAttribute('ref')
		if scene_group_id == node_id:
		    return layer_idx, scene_node
		pass
	    pass
	return -1, None

    def get_layer_group(self, layer_idx):
	layer = self._layers[layer_idx]
	return layer.group

    def get_all_scene_node_of_layer(self, layer_idx):
	layer = self._layers[layer_idx]
	return layer.scenes

    def get_layer_data(self, layer_idx):
	layer = self._layers[layer_idx]
	try:
	    data = layer.data
	except:
	    return None
	return data

    def set_layer_data(self, layer_idx, data):
	layer = self._layers[layer_idx]
	layer.data = data
	pass

    def create_layer_dup_group(self, layer_idx):
	layer = self._layers[layer_idx]
	
	dup_group = self._doc.createElement('svg:g')
	gid = self.new_id()
	dup_group.setAttribute('id', gid)
	dup_group.setAttribute('inkscape:label', 'dup')
	dup_group.setAttribute('sodipodi:insensitive', '1')
	dup_group.setAttribute('style', '')

	layer.group.appendChild(dup_group)
	
	return dup_group

    def insert_frames(self, layer_idx, frame_idx, num):
	layer = self._layers[layer_idx]
	for scene_node in layer.scenes:
	    start, end, tween_type = self._parse_one_scene(scene_node)
	    if start >= frame_idx:
		self.chg_scene_node(scene_node, start=(start + num))
		pass
	    if end >= frame_idx:
		self.chg_scene_node(scene_node, end=(end + num))
		pass
	    pass
	pass

    ## \brief Remove frames
    #
    # - Scenes covered by removing range were removed.
    # - Scenes after removing range were shifted left.
    #
    def rm_frames(self, layer_idx, frame_idx, num):
	layer = self._layers[layer_idx]
	
	last_rm = frame_idx + num - 1 # last removed frame
	for scene_node in layer.scenes:
	    start, end, tween_type = \
		self._dom._parse_one_scene(scene_node)
	    
	    if end < frame_idx:
		continue
	    
	    if start > last_rm:	# this scene is at right side
		self.chg_scene_node(scene_node,
				    start=(start - num),
				    end=(end - num))
	    else:	 # this scene is covered by removing range
		self.rm_scene_node_n_group(scene_node)
		pass
	    pass
	pass

    def get_max_frame(self):
	return self._maxframe
    pass

## \brief Maintain a stack of frameline UI component.
#
# Every layer is assocated with a frameline.  Framelines are showed/stacked in
# virtical.  Framelines of lower layers are placed at lower position on the
# screen.  This class provide a set of API to access framelines with layer and
# frame index number.  You access/set content of frameline by specifing layer
# index and frame index.
#
class MBScene_frameline_stack(object):
    _frameline_tween_types = (frameline.TWEEN_TYPE_NONE,
			      frameline.TWEEN_TYPE_SHAPE)
    _num_frames_of_line = 100
    
    _framelines = None
    
    def __init__(self, *args, **kws):
	super(MBScene_frameline_stack, self).__init__(*args, **kws)
	
	self._last_mouse_over_frameline = None
	self._last_active_frameline = None
	self._active_frame_callback = None
	pass

    def _change_hover_frameline(self, widget, event):
        """
	Hide all hover frames. This is a hack. We should use the lost focus
	event instead in the future to reduce the overhead.
	"""
	if self._last_mouse_over_frameline and \
		widget != self._last_mouse_over_frameline:
	    self._last_mouse_over_frameline.mouse_leave()
	    pass
	self._last_mouse_over_frameline = widget
	pass

    ## \brief Switch to new active frameline.
    #
    # Hide active frame mark for the active frame of old active frameline.  It
    # always shows at most one active frame mark.  When a frame is activated,
    # all active frame mark of other frameline should be hidden.
    #
    def _active_frameline(self, frameline):
	last = self._last_active_frameline
	
	if last and last != frameline:
	    last.deactive()
	    pass
	
	self._last_active_frameline = frameline
	pass

    ## \brief Called for changing of active frame.
    #
    # This handle deactive previous frameline that owns an active frame when a
    # frame in another frameline is activated.
    #
    def _change_active_frame(self, frameline, frame_idx, button):
	frameline.active_frame(frame_idx)
	self._active_frameline(frameline)
	
	if self._active_frame_callback:
	    layer_idx = frameline.layer_idx
	    self._active_frame_callback(layer_idx, frame_idx)
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

	# Put later one on the top of earier one, but after the ruler.
	position = len(self._framelines) - layer_idx + 1
	vbox.reorder_child(hbox, position)
	
	self._framelines[layer_idx: layer_idx] = [line]
	
	for idx in range(layer_idx, len(self._framelines)):
	    self._framelines[idx].layer_idx = idx
	    pass
	
	line.label = label
	line.connect('motion-notify-event', self._change_hover_frameline)
	line.connect(frameline.FRAME_BUT_PRESS, self._change_active_frame)
	pass
    
    ## \brief Remove the given frameline from the frameline box.
    #
    def _remove_frameline(self, layer_idx):
	vbox = self._frameline_vbox
	line = self._framelines[layer_idx]
	
	hbox = line.parent
	vbox.remove(hbox)
	del self._framelines[layer_idx]
	
	for idx in range(layer_idx, len(self._framelines)):
	    self._framelines[idx].layer_idx = idx
	    pass
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

    ## \brief Make given frame as current active frame.
    #
    def active_frame(self, layer_idx, frame_idx):
	frameline = self._framelines[layer_idx]
	self._active_frameline(frameline)
	frameline.active_frame(frame_idx)
	pass

    ## \brief Get layer and frame index of current active frame.
    #
    # \return (-1, -1) for no active, (layer_idx, frame_idx) for current
    #		active.
    def get_active_layer_frame(self):
	if self._last_active_frameline:
	    layer_idx = self._last_active_frameline.layer_idx
	    frame_idx = self._last_active_frameline.get_active_frame()
	    if frame_idx != -1:
		return layer_idx, frame_idx
	    pass
	return -1, -1

    ## \brief Get information of the key frame at left-side.
    #
    # The key frame, returned, is at the place of the give frame or its
    # left-side.
    def get_left_key_tween(self, layer_idx, frame_idx):
	frameline = self._framelines[layer_idx]
	start, end, fl_tween_type = frameline.get_frame_block_floor(frame_idx)
	tween_type = self._frameline_tween_types.index(fl_tween_type)
	return start, end, tween_type

    ## \brief Return information of a key frame and its tweening.
    #
    # This method return the key frame that the given frame is, or is in its
    # tween.
    #
    # \return (start, end, tween_type)
    def get_key_tween(self, layer_idx, frame_idx):
	frameline = self._framelines[layer_idx]
	start, end, fl_tween_type = frameline.get_frame_block(frame_idx)

	tween_type = self._frameline_tween_types.index(fl_tween_type)
	return start, end, tween_type

    def get_all_key_tween_of_layer(self, layer_idx):
	frameline = self._framelines[layer_idx]
	info = frameline.get_frame_blocks()
	tweens = [(tween[0], tween[1],
		   self._frameline_tween_types.index(tween[2]))
		  for tween in info]
	return tweens
    
    ## \brief Tweening key frame to a give size
    #
    # The tween can be changed by tweening it again.
    def tween(self, layer_idx, key_frame_idx, tween_len,
	      tween_type=TweenObject.TWEEN_TYPE_NORMAL):
	assert tween_len > 0
	frameline = self._framelines[layer_idx]
	right_frame_idx = key_frame_idx + tween_len - 1
	fl_tween_type = self._frameline_tween_types[tween_type]

	start, end, old_fl_tween_type = \
	    frameline.get_frame_block(key_frame_idx)
	if start != key_frame_idx:
	    ValueError, 'invalid key frame (%d)' % (key_frame_idx)
	if start < end:
	    frameline.unmark_keyframe(end)
	    pass
	frameline.mark_keyframe(right_frame_idx)
	frameline.tween(start, fl_tween_type)
	pass

    ## \brief Unmark a key frame.
    #
    # Once a key frame was unmark, the associated tween was also removed
    # totally.
    #
    def unmark_keyframe(self, layer_idx, frame_idx):
	frameline = self._framelines[layer_idx]
	start, end, fl_tween_type = frameline.get_frame_block(frame_idx)
	if start != frame_idx:
	    raise ValueError, 'no such key (%d, %d)' % (layer_idx, frame_idx)

	frameline.unmark_keyframe(frame_idx)
	if start < end:
	    frameline.unmark_keyframe(end)
	    pass
	pass

    ## \brief Makr a key frame.
    #
    # Make a frame as a key frame.
    #
    def mark_keyframe(self, layer_idx, frame_idx):
	frameline = self._framelines[layer_idx]
	frameline.mark_keyframe(frame_idx)
	pass

    ## \brief Get data associated with the given key frame.
    #
    # The given frame index must be exactly a key frame.
    #
    def get_keyframe_data(self, layer_idx, frame_idx):
	frameline = self._framelines[layer_idx]
	data = frameline.get_frame_data(frame_idx)
	return data

    ## \brief Set/associate data with the given key frame.
    #
    def set_keyframe_data(self, layer_idx, frame_idx, data):
	frameline = self._framelines[layer_idx]
	frameline.set_frame_data(frame_idx, data)
	pass

    ## \brief Insert frames before specified frame.
    #
    # Specified frame and frames after it are shift right for \ref num
    # positions to make a space for new frames.
    #
    def insert_frames(self, layer_idx, frame_idx, num):
	assert num > 0
	assert frame_idx >= 0
	frameline = self._framelines[layer_idx]
	for i in range(num):
	    frameline.add_frame(frame_idx)
	    pass
	pass

    ## \brief Remove a number of frames from the frameline.
    #
    # All key frames and associated tween info covered by removing range would
    # be removed.
    #
    def rm_frames(self, layer_idx, frame_idx, num):
	assert num > 0
	assert frame_idx >= 0
	
	frameline = self._framelines[layer_idx]
	
	#
	# Remove right key frame of last tween which left one will be removed.
	#
	last_rm = frame_idx + num - 1 # last removed frame
	try:
	    start, end, tween_type = frameline.get_frame_block(last_rm)
	except ValueError:	# last removed frame is not in any tween
	    pass
	else:
	    if start >= frame_idx and end > last_rm:
		# Left key frame of the tween was removed, but not right one.
		frameline.untween(start)
		frameline.unmark_keyframe(end)
		pass
	    pass

	#
	# Remove left key of the tween that right key frame is in removing
	# range.
	#
	try:
	    start, end, tween_type = frameline.get_frame_block(frame_idx)
	except ValueError:
	    pass
	else:
	    if start < frame_idx and end <= last_rm:
		# right key frame is in removing range but left one.
		frameline.untween(start)
		frameline.unmark_keyframe(start)
		frameline.unmark_keyframe(end)
		pass
	    pass
	
	for i in range(num):
	    frameline.rm_frame(frame_idx)
	    pass
	pass

    ## \brief Set label for a layer.
    #
    def set_layer_label(self, layer_idx, txt):
	frameline = self._framelines[layer_idx]
	frameline.label.set_text(txt)
	pass

    ## \brief Register a callback for active frame event.
    #
    # The callback would be called when a frame is activated.
    #
    def register_active_frame_callback(self, cb):
	self._active_frame_callback = cb
	pass
    pass

## \brief Bridge of DOM-tree to syncrhonize data-model and UI.
#
# This class is a wrapper
class MBScene_domview_ui(object):
    _tween_type_names = ('normal', 'scale')
    
    def __init__(self):
	super(MBScene_domview_ui, self).__init__()
	self._fl_stack = MBScene_frameline_stack()
	self._dom = MBScene_domview()
	pass

    ## \brief Update content of a frameline from scenes of respective layer.
    #
    def _update_frameline_content(self, layer_idx):
	fl_stack = self._fl_stack
	scene_nodes = self._dom.get_all_scene_node_of_layer(layer_idx)
	for scene_node in scene_nodes:
	    start, end, tween_name = self._dom._parse_one_scene(scene_node)

	    fl_stack.mark_keyframe(layer_idx, start)
	    fl_stack.set_keyframe_data(layer_idx, start, scene_node)
	    if start != end:
		tween_type = self._tween_type_names.index(tween_name)
		tween_len = end - start + 1
		fl_stack.tween(layer_idx, start, tween_len, tween_type)
		pass
	    pass
	pass
    
    ## \brief Add a frameline for every found layer.
    #
    # This method is called to create a frameline for every layer found when
    # loading a document.
    #
    def _add_frameline_for_every_layer(self):
	for layer_idx in range(self._dom.get_layer_num()):
	    layer_group_node = self._dom.get_layer_group(layer_idx)
	    label = layer_group_node.getAttribute('inkscape:label')
	    
	    self._fl_stack._add_frameline(layer_idx)
	    self._fl_stack.set_layer_label(layer_idx, label)

	    self._update_frameline_content(layer_idx)
	    pass
	pass
    
    ## \brief This method is called to handle a new document.
    #
    def handle_doc_root(self, doc, root):
	self._dom.handle_doc_root(doc, root)
	self._fl_stack._init_framelines()
	self._add_frameline_for_every_layer()
	self._fl_stack._show_framelines()
	pass

    ## \brief Mark given frame as a key frame.
    #
    def mark_key(self, layer_idx, key_idx):
	scene_group = self._dom.add_scene_group(layer_idx)
	scene_group_id = scene_group.getAttribute('id')
	
	scene_node = self._dom.add_scene_node(key_idx, key_idx)
	self._dom.chg_scene_node(scene_node, ref=scene_group_id)
	
	self._fl_stack.mark_keyframe(layer_idx, key_idx)
	self._fl_stack.set_keyframe_data(layer_idx, key_idx, scene_node)
	pass

    ## \brief Tweening a key frame.
    #
    # To tween a key spanning several frames at right-side.
    # The tween of a key frame can be changed by tweening it again.
    #
    def tween(self, layer_idx, key_frame_idx, tween_len,
	      tween_type=TweenObject.TWEEN_TYPE_NORMAL):
	self._fl_stack.tween(layer_idx, key_frame_idx, tween_len, tween_type)
	
	end_frame_idx = key_frame_idx + tween_len - 1
	scene_node = self._fl_stack.get_keyframe_data(layer_idx, key_frame_idx)
	tween_name = self._tween_type_names[tween_type]
	self._dom.chg_scene_node(scene_node, end=end_frame_idx,
				 tween_type=tween_name)
	pass

    ## \brief Change tween info of a key frame
    #
    def chg_tween(self, layer_idx, key_frame_idx,
		  tween_len=None, tween_type=None):
	scene_node = self._fl_stack.get_keyframe_data(layer_idx, key_frame_idx)
	start, end, old_tween_type = \
	    self._fl_stack.get_key_tween(layer_idx, key_frame_idx)
	
	if tween_len is not None:
	    end = start + tween_len - 1	    
	    self._dom.chg_scene_node(scene_node, end=end)
	    pass
	if tween_type is not None:
	    tween_name = self._tween_type_names[tween_type]
	    self._dom.chg_scene_node(scene_node, tween_type=tween_name)
	    pass

	if tween_type is None:
	    tween_type = old_tween_type
	    pass

	tween_len = end - start + 1
	self._fl_stack.tween(layer_idx, start, tween_len, tween_type)
	pass

    ## \brief Unmark a frame from a key frame.
    #
    def unmark_key(self, layer_idx, key_frame_idx):
	scene_node = self._fl_stack.get_keyframe_data(layer_idx, key_frame_idx)
	self._dom.rm_scene_node_n_group(scene_node)
	
	self._fl_stack.unmark_keyframe(layer_idx, key_frame_idx)
	pass

    ## \brief Insert frames at specified position.
    #
    # All frame at and after given position will shift right.
    #
    def insert_frames(self, layer_idx, frame_idx, num):
	self._fl_stack.insert_frames(layer_idx, frame_idx, num)
	self._dom.insert_frames(layer_idx, frame_idx, num)
	pass

    ## \brief Insert frames at specified position.
    #
    # All frame at and after given position will shift left, except nearest
    # \ref num frames are removed.
    #
    def rm_frames(self, layer_idx, frame_idx, num):
	self._fl_stack.insert_frames(layer_idx, frame_idx, num)
	self._dom.rm_frames(layer_idx, frame_idx, num)
	pass

    ## \brief Insert a layer at given position.
    #
    # Original layer \ref layer_idx and later ones would be shifted to make a
    # space for the new layer.
    #
    def insert_layer(self, layer_idx):
	self._dom.insert_layer(layer_idx)
	self._fl_stack._add_frameline(layer_idx)
	self._fl_stack._show_framelines()
	pass

    ## \brief Remove given layer.
    #
    def rm_layer(self, layer_idx):
	self._dom.rm_layer(layer_idx)
	self._fl_stack._remove_frameline(layer_idx)
	self._fl_stack._show_framelines()
	pass
    
    def set_active_layer_frame(self, layer_idx, frame_idx):
	self._fl_stack.active_frame(layer_idx, frame_idx)
	pass
    
    ## \bref Return current active frame and its layer.
    #
    # \return (layer_idx, frame_idx) of active frame, or (-1, -1) when no
    #	      active one.
    def get_active_layer_frame(self):
	layer_idx, frame_idx = self._fl_stack.get_active_layer_frame()
	return layer_idx, frame_idx

    def get_layer_num(self):
	return self._dom.get_layer_num()

    ## \brief Return associated group node for a key frame.
    #
    # The given frame index must be exactly a key frame.
    #
    def get_key_group(self, layer_idx, frame_idx):
	scene_node = self._fl_stack.get_keyframe_data(layer_idx, frame_idx)
	scene_group_id = scene_node.getAttribute('ref')
	scene_group_node = self._dom.get_node(scene_group_id)
	return scene_group_node

    ## \brief Find an associated key frame and tween info for a group ID.
    #
    def find_key_from_group(self, scene_group_id):
	layer_idx, scene_node = \
	    self._dom.find_layer_n_scene_of_node(scene_group_id)
	start, end, tween_name = self._dom._parse_one_scene(scene_node)
	tween_type = self._tween_type_names.index(tween_name)
	return layer_idx, (start, end, tween_type)
    
    ## \brief Return key and tween info for given frame index.
    #
    # The return key is at given frame, or its tween covers given frame.
    #
    def get_key(self, layer_idx, frame_idx):
	start, end, tween_type = \
	    self._fl_stack.get_key_tween(layer_idx, frame_idx)
	return start, end, tween_type

    def get_left_key(self, layer_idx, frame_idx):
	start, end, tween_type = \
	    self._fl_stack.get_left_key_tween(layer_idx, frame_idx)
	return start, end, tween_type

    ## \brief Return information of key frames in the given layer.
    #
    def get_layer_keys(self, layer_idx):
	key_tweens = self._fl_stack.get_all_key_tween_of_layer(layer_idx)
	return key_tweens

    ## \brief Return widget showing frames and layers.
    #
    def get_frame_ui_widget(self):
	return self._fl_stack._frameline_box

    ## \brief Register a callback for activating a frame event.
    #
    # The callback function is called when a frame is activated.
    #
    def register_active_frame_callback(self, cb):
	self._fl_stack.register_active_frame_callback(cb)
	pass

    ## \brief Get duplicate group of a layer.
    #
    def get_layer_dup_group(self, layer_idx):
	data = self._dom.get_layer_data(layer_idx)
	if not data:
	    data = dict()
	    self._dom.set_layer_data(layer_idx, data)
	    pass

	dup_group = None
	if data.has_key('dup_group_id'):
	    try:
		dup_group = self._dom.get_node(data['dup_group_id'])
	    except KeyError:
		pass
	    pass
	
	if not dup_group:
	    # Search dup group from children of the layer group
	    layer_group = self._dom.get_layer_group(layer_idx)
	    for child in layer_group.childList():
		try:
		    label = child.getAttribute('inkscape:label')
		except:
		    pass
		else:
		    if label == 'dup':
			data['dup_group_id'] = child
			return child
		    pass
		pass
	    
	    # Or create a new dup group for the layer
	    dup_group = self._dom.create_layer_dup_group(layer_idx)
	    data['dup_group_id'] = dup_group.getAttribute('id')
	    pass

	return dup_group

    def get_max_frame(self):
	max_frame = self._dom.get_max_frame()
	return max_frame
    pass

## \brief MBScene connect GUI and DOM-tree
#
# This method accepts user actions and involves MBScene_domview_ui to update
# data on the document.
#
class MBScene(object):
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
	pybInkscape.inkscape.connect('change_selection', self.on_selection)
	self.last_select = None
	self._lockui = False
	self.tween = None
	self.document = None
	self._root = root
	self.framerate = 12
	self._disable_tween_type_selector = False
	self.current = 0

	self._domview = MBScene_domview_ui()
	pass

    def change_active_frame(self, node):
	"""
	    Change the active frame to the current selected node. This will
	    tell users where the current node is.
	"""
	while node:
	    try:
		node_id = node.getAttribute('id')
	    except:
		node = node.parent()
		continue
	    
	    try:
		layer_idx, (start, end, tween_type) = \
		    self._domview.find_key_from_group(node_id)
	    except:
		pass
	    else:
		self._domview.set_active_layer_frame(layer_idx, start)
		break
	    
	    node = node.parent()
	    pass
	pass

    def insertKeyScene(self, layer_idx, frame_idx):
	"""
	Insert a new key scene into the stage. If the nth is always a
	key scene, we will return without changing anything.  If the
	nth is a filled scene, we will break the original scene into
	two parts. If the nth is out of any scene, we will append a
	new scene.

	"""
	try:
	    self._domview.mark_key(layer_idx, frame_idx)
	except ValueError:	# existed key frame
	    pass
	pass

    def removeKeyScene(self, layer_idx, frame_idx):
	self._domview.unmark_key(layer_idx, frame_idx)
	self.setCurrentScene(frame_idx)
	pass
    
    def extendScene(self):
	layer_idx, frame_idx = self._domview.get_active_layer_frame()
	start, end, tween_type = \
	    self._domview.get_left_key(layer_idx, frame_idx)
	tween_len = frame_idx - start + 1
	self._domview.tween(layer_idx, start, tween_len, tween_type)
	
	scene_group = self._domview.get_key_group(layer_idx, start)
	self._enterGroup(scene_group)
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
	for layer_idx in range(self._domview.get_layer_num()):
	    dup_group = self._domview.get_layer_dup_group(layer_idx)
	    dup_group.setAttribute('style', 'display: none')

	    all_key_tweens = self._domview.get_layer_keys(layer_idx)
	    for start, end, tween_type in all_key_tweens:
		if start == idx: # at key frame
		    scene_group = \
			self._domview.get_key_group(layer_idx, start)
		    scene_group.setAttribute('style', '')
		elif start < idx and end >= idx: # in Tween
		    dup_group.setAttribute('style', '')
		    scene_group = \
			self._domview.get_key_group(layer_idx, start)
		    scene_group.setAttribute('style', 'display: none')
		    
		    try:
			next_scene_group = \
			    self._domview.get_key_group(layer_idx, end + 1)
		    except:	# no next key frame
			next_scene_group = scene_group
			pass

		    tween_obj_type = self._tween_obj_tween_types[tween_type]
		    nframes = end - start + 1
		    percent = float(idx - start) / nframes
		    self.tween.updateTweenContent(dup_group,
						  tween_obj_type,
						  scene_group,
						  next_scene_group,
						  percent)
		    pass
		else:		# this scene should not be showed.
		    scene_group = \
			self._domview.get_key_group(layer_idx, start)
		    scene_group.setAttribute('style', 'display: none')
		    pass
		pass
	    pass
	pass
    
    def _enterGroup(self, scene_group):
	self.desktop.setCurrentLayer(scene_group.spitem)
	pass
    
    def setTweenType(self, tween_type):
	self._disable_tween_type_selector = True
	self.tweenTypeSelector.set_active(tween_type)
	self._disable_tween_type_selector = False
	pass
	
    def selectSceneObject(self, layer_idx, frame_idx):
	try:
	    start, stop, tween_type = \
		self._domview.get_key(layer_idx, frame_idx)
	except:
	    return

	scene_group = self._domview.get_key_group(layer_idx, start)
	self._enterGroup(scene_group)
	self.setTweenType(tween_type)
	pass

    def duplicateKeyScene(self):
        # Search for the current scene
	layer_idx, frame_idx = self._domview.get_active_layer_frame()

	try:
	    left_start, left_end, left_tween_type = \
		self._domview.get_left_key(layer_idx, frame_idx)
	except:
	    return
	if left_end >= frame_idx:
	    return

	self._domview.mark_key(layer_idx, frame_idx)
	
	scene_group = self._domview.get_key_group(layer_idx, frame_idx)
	left_scene_group = \
	    self._domview.get_key_group(layer_idx, left_start)
	
	dup_group = self._duplicate_group(left_scene_group, scene_group)

	self.setCurrentScene(frame_idx)
	pass

    ## \brief Duplicate children of a group.
    #
    # Duplicate children of a group, and append them to another group.
    #
    def _duplicate_group(self, src_group, dst_group):
	# Search for the duplicated group
        root = self._root
	doc = self.document
	
	dup_group = src_group.duplicate(doc)
	for child in dup_group.childList():
	    dup_group.removeChild(child) # prvent from crash
	    dst_group.appendChild(child)
	    pass

	old_nodes = _DOM_iterator(src_group)
	new_nodes = _DOM_iterator(dst_group)
	for old_node in old_nodes:
	    old_node_id = old_node.getAttribute('id')
	    new_node = new_nodes.next()
	    new_node.setAttribute('ns0:duplicate-src', old_node_id)
	    pass
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

    def addTweenTypeSelector(self, hbox):
	tweenbox = gtk.HBox()
	label = gtk.Label('Tween Type')
	tweenbox.pack_start(label)
	
        self.tweenTypeSelector = gtk.combo_box_new_text()
	self.tweenTypeSelector.append_text('normal')
	self.tweenTypeSelector.append_text('scale')
	self.tweenTypeSelector.set_active(0)
	tweenbox.pack_start(self.tweenTypeSelector, expand=False, fill=False)
	hbox.pack_start(tweenbox, expand=False, fill=False)
	self.tweenTypeSelector.connect('changed', self.onTweenTypeChange)
	pass
    
    def on_selection(self,w,obj):
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

	# The selection is a PYSPObject. Convert it to be PYNode
	self.change_active_frame(self.last_select.repr.parent())
	pass

    def onCellClick(self, layer_idx, frame_idx):
	self._lockui = True
	
	self.setCurrentScene(frame_idx)
	self.selectSceneObject(layer_idx, frame_idx)
	
	self._lockui = False
        pass
    
    def doInsertKeyScene(self,w):
	self._lockui=True
	layer_idx, frame_idx = self._domview.get_active_layer_frame()
	self.insertKeyScene(layer_idx, frame_idx)
	self.selectSceneObject(layer_idx, frame_idx)
	self._lockui=False
	return
    
    def doDuplicateKeyScene(self,w):
	self._lockui = True
        self.duplicateKeyScene()
	self._lockui = False

    def doRemoveScene(self,w):
	self._lockui = True
	layer_idx, frame_idx = self._domview.get_active_layer_frame()
	self.removeKeyScene(layer_idx, frame_idx)
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
	if self.current > self._domview.get_max_frame():
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

    def doInsertFrame(self, w):
	self.lockui=True
	layer_idx, frame_idx = self._domview.get_active_layer_frame()
	self._domview.insert_frames(layer_idx, frame_idx, 1)
	self.lockui=False

    def doRemoveFrame(self, w):
        self.lockui=True
	layer_idx, frame_idx = self._domview.get_active_layer_frame()
	self._domview.rm_frames(layer_idx, frame_idx, 1)
	self.lockui=False

    def onTweenTypeChange(self, w):
	if self._disable_tween_type_selector:
	    return

	layer_idx, frame_idx = self._domview.get_active_layer_frame()
	tween_type = self.tweenTypeSelector.get_active()
	
	start, end, old_tween_type = \
	    self._domview.get_left_key(layer_idx, frame_idx)
	if end >= frame_idx and start != end:
	    # Length of tween > 1 and cover this frame
	    self._domview.chg_tween(layer_idx, start, tween_type=tween_type)
	    pass
	pass
    
    def onQuit(self, event):
	self.OK = False
	gtk.main_quit()
	pass
    
    def onOK(self, event):
	self.OK = True
	gtk.main_quit()
	pass

    def _add_buttons(self, hbox):
	btn = gtk.Button('Insert Key')
	btn.connect('clicked', self.doInsertKeyScene)
	hbox.pack_start(btn, expand=False, fill=False)

	btn=gtk.Button('Remove Key')
	btn.connect('clicked', self.doRemoveScene)
	hbox.pack_start(btn, expand=False, fill=False)

	btn=gtk.Button('Extend scene')
	btn.connect('clicked', self.doExtendScene)
	hbox.pack_start(btn, expand=False, fill=False)

	btn=gtk.Button('Duplicate Key')
	btn.connect('clicked', self.doDuplicateKeyScene)
	hbox.pack_start(btn, expand=False, fill=False)

	btn=gtk.Button('insert')
	btn.connect('clicked', self.doInsertFrame)
	hbox.pack_start(btn, expand=False, fill=False)
	
	btn=gtk.Button('remove')
	btn.connect('clicked', self.doRemoveFrame)
	hbox.pack_start(btn, expand=False, fill=False)
	
	btn=gtk.Button('Run')
	btn.connect('clicked', self.doRun)
	self.btnRun = btn
	hbox.pack_start(btn, expand=False, fill=False)

	self.addNameEditor(hbox)
	self.addTweenTypeSelector(hbox)
	pass

    def show(self):
	self.OK = True
	if not self._root:
	    self._root = self.desktop.doc().root().repr
	    pass
	
	self.document = self.desktop.doc().rdoc
	
	self.tween = TweenObject(self.document, self._root)
	self._domview.handle_doc_root(self.document, self._root)
	self._domview.register_active_frame_callback(self.onCellClick)

	if self.top == None:
	    self.top = gtk.VBox(False, 0)
	    toplevel = self.desktop.getToplevel()
	    toplevel.child.child.pack_end(self.top, expand=False)
	else:
	    self.top.remove(self.startWindow)
	    pass
	
	vbox = gtk.VBox(False, 0)
	self.startWindow = vbox
	self.top.pack_start(vbox, expand=False)
	frame_ui = self._domview.get_frame_ui_widget()
	vbox.pack_start(frame_ui, expand=False)
	hbox=gtk.HBox(False, 0)
	self._add_buttons(hbox)
	vbox.pack_start(hbox, expand=False)

	self.top.show_all()
	self.last_update = None
	return False
    pass
