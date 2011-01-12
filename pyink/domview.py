import random
import dom_event
from tween import TweenObject

class Layer:
    def __init__(self, node):
	self.scenes = []
	self.group = node
	pass
    pass

## \brief Monitor changes of DOM-tree.
#
# This class monitors DOM-tree to maintain _maxframe and maps for node ID to
# node and scene group ID to scene node.
class domview_monitor(object):
    def __init__(self, *args, **kws):
	super(domview_monitor, self).__init__()

	self._maxframe = 0
	self._id2node = {}	# map ID to the node in the DOM tree.
	self._group2scene = {}	# map ID of a group to associated scene node.
	pass
    
    def _start_monitor(self):
	self._collect_node_ids()
	self._collect_all_scenes()
	
	doc = self._doc
	dom_event.addEventListener(doc, 'DOMNodeInserted',
                                   self._on_insert_node, None)
	dom_event.addEventListener(doc, 'DOMNodeRemoved',
                                   self._on_remove_node, None)
	dom_event.addEventListener(doc, 'DOMAttrModified',
                                   self._on_attr_modified, None)
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
    
    def parse_one_scene(self, scene_node):
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
		start, end, scene_type = self.parse_one_scene(scene_node)
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


## \brief This layer provide a data view to the DOM-tree.
#
# This class maintains layers information, and provides functions to create,
# change and destroy scene node and scene group.  A scene node is a 'ns0:scene'
# in 'ns0:scenes' tag.  A scene group is respective 'svg:g' for a scene.
#
class domview(domview_monitor):
    # Declare variables, here, for keeping tracking
    _doc = None
    _root = None
    
    def __init__(self, *args, **kws):
	super(domview, self).__init__()
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
	
	self._start_monitor()	# start domview_monitor
	self._init_metadata()
	self._parse_all_layers()
	pass

    def reset(self):
        self.handle_doc_root( self._doc, self._root)
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
    def add_scene_node(self, layer_idx, start, end,
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
        
        self._layers[layer_idx].scenes.append(scene_node)
	
	return scene_node

    ## \brief Manage a existed scene node at given layer.
    #
    def manage_scene_node(self, layer_idx, scene_node):
        self._layers[layer_idx].scenes.append(scene_node)
        pass

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

    ## \brief Remove scene node from DOM-tree.
    #
    def rm_scene_node(self, scene_node):
        if not scene_node.parent():
            return              # without this, may crash the Inkscape.
        
	self._scenes_node.removeChild(scene_node)
        for layer in self._layers:
            try:
                layer.scenes.remove(scene_node)
            except ValueError:  # not in the list
                pass
            else:
                break
            pass
	pass

    ## \brief Remove scene node and asssociated scene group from DOM.
    #
    # It will remove as many as possible.  Does not complain about
    # error in the procedure of removing.
    #
    def rm_scene_node_n_group(self, scene_node):
	scene_group_id = scene_node.getAttribute('ref')
        try:
            scene_group_node = self.get_node(scene_group_id)
            if scene_group_node.parent(): # Check it, or crash the
                                          # Inkscape.
                scene_group_node.parent().removeChild(scene_group_node)
                pass
        except:
            pass
	
        try:
            self.rm_scene_node(scene_node)
        except:
            pass
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

    ## \brief Manage a existed layer group
    #
    # This method scan layer groups of all managed layers, and find a
    # proper place to insert it.
    #
    # \return -1 for error, or layer index.
    #
    def manage_layer_group(self, layer_group_id):
        layer_group = self.get_node(layer_group_id)
        new_layer = Layer(layer_group)

        if not self._layers:
            new_layer.idx = 0
            self._layers.append(new_layer)
            return 0
        
        #
        # Scan who is after the given group
        #
        next_group = layer_group.next()
        while next_group:
            next_group_id = next_group.getAttribute('id')
            
            for vlayer in self._layers:
                vlayer_group_id = vlayer.group.getAttribute('id')
                if vlayer_group_id == next_group_id:
                    # This layer group is after given one.
                    self._layers.insert(vlayer.idx, new_layer)
                    
                    for idx in range(vlayer.idx, len(self._layers)):
                        self._layers[idx].idx = idx
                        pass
                    return new_layer.idx
                pass
            
            next_group = next_group.next()
            pass
        
        #
        # Is the given group after last layer group?
        #
        tail_group = self._layers[-1].group.next()
        while tail_group:
            tail_group_id = tail_group.getAttribute('id')
            
            if tail_group_id == layer_group_id:
                # it is after last layer group.
                new_layer.idx = len(self._layers)
                self._layers.append(new_layer)
                return new_layer.idx
            
            tail_group = tail_group.next()
            pass

        return -1             # error, can not determinze the position
    
    ## \brief Remove layer and associated scene nodes and scene groups.
    #
    def rm_layer(self, layer_idx):
	layers = self._layers

        layer = self._layers[layer_idx]
        for scene_node in layer.scenes:
            scene_group_id = scene_node.getAttribute('ref')
            try:
                scene_group_node = self.get_node(scene_group_id)
                if scene_group_node.parent(): # keep from crashing
                    scene_group_node.parent().removeChild(scene_group_node)
                    pass
            except:
                pass
            
            if scene_node.parent(): # keep from crashing
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

    ## \brief Find layer index and scene info for a given scene node.
    #
    # \return (-1, None) for error.
    #
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

    ## \brief Return associated layer index of given layer group.
    #
    # \return -1 for error.
    #
    def find_layer_of_group(self, group_id):
        for layer_idx, layer in enumerate(self._layers):
            if layer.group.getAttribute('id') == group_id:
                return layer_idx
            pass
        return -1

    def insert_frames(self, layer_idx, frame_idx, num):
	layer = self._layers[layer_idx]
	for scene_node in layer.scenes:
	    start, end, tween_type = self.parse_one_scene(scene_node)
	    if start >= frame_idx:
		self.chg_scene_node(scene_node, start=(start + num))
		pass
	    if end >= frame_idx:
		self.chg_scene_node(scene_node, end=(end + num))
		pass
	    pass
	pass

    ## \brief add the current position to the undo buffer
    #
    def mark_undo(self, msg):
    	self._doc.done("none", msg)
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
		self.parse_one_scene(scene_node)
	    
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
    
    ## \brief Copy children of a group.
    #
    # Duplicate children of a group, and append them to another group.
    #
    def copy_group_children(self, src_group, dst_group):
	# Search for the duplicated group
	doc = self._doc

	dup_group = src_group.duplicate(doc)
        
	old_nodes = _DOM_iterator(src_group)
	new_nodes = _DOM_iterator(dup_group)
        new_gids = set()
	for old_node in old_nodes:
	    old_node_id = old_node.getAttribute('id')
	    new_node = new_nodes.next()
	    new_node.setAttribute('ns0:duplicate-src', old_node_id)
            
            #
            # Change ID here, or inkscape would insert the node with
            # the same ID, and change it later to avoid duplication.
            # But, our event handler would be called before changing
            # ID.  It would confuse our code.  We change ID of nodes
            # before inserting them into the DOM-tree.
            #
            gid = self.new_id()
            while gid in new_gids:
                gid = self.new_id()
                pass
            new_gids.add(gid)
            new_node.setAttribute('id', gid)
	    pass
	
	for child in dup_group.childList():
	    dup_group.removeChild(child) # prvent from crash
	    dst_group.appendChild(child)
	    pass
	pass
    pass

