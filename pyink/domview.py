import random
import dom_event
from tween import TweenObject


class Layer:
    def __init__(self, node):
	self.scenes = []
	self.group = node
	pass
    pass


class Timeline(object):
    def __init__(self, scenes_node):
        self.scenes_node = scenes_node
        pass

    def name(self):
        name = self.scenes_node.getAttribute('name')
        return name
    pass


class Component(object):
    #
    # \param comp_node is None for main component.
    #
    def __init__(self, comp_mgr, comp_node):
        self._comp_mgr = comp_mgr
        self.node = comp_node
        self.layers = []
        self.timelines = []

        if comp_node:
            self._initialize_comp()
            pass
        pass

    def _initialize_comp(self):
        comp_node = self.node
        for child in comp_node.childList():
            if child.name() == 'ns0:scenes':
                break
            pass
        else:                   # no any ns0:scenes
            doc = self._comp_mgr._doc
            scenes_node = doc.createElement('ns0:scenes')
            scenes_node.setAttribute('name', 'default')
            
            node_id = self._comp_mgr.new_id()
            scenes_node.setAttribute('id', node_id)
            
            comp_node.appendChild(scenes_node)
            pass
        pass

    def name(self):
        if self.node:
            name = self.node.getAttribute('name')
        else:
            name = 'main'
            pass
        return name

    def all_timeline_names(self):
        names = [tl.name() for tl in self.timelines]
        return names

    def parse_timelines(self):
        if self.node:
            assert self.node.name() == 'ns0:component'
            pass
        
        comp_node = self.node
        for child in comp_node.childList():
            if child.name() == 'ns0:scenes':
                tl = Timeline(child)
                self.timelines.append(tl)
                pass
            pass
        pass

    def get_timeline(self, name):
        for tl in self.timelines:
            if tl.name() == name:
                return tl
            pass
        raise Value, 'invalid timeline name - %s' % (name)

    def has_timeline(self, name):
        for tl in self.timelines:
            if tl.name() == name:
                return True
            pass
        return False

    def add_timeline(self, name):
        if self.has_timeline(name):
            raise ValueError, \
                'try add a timeline with duplicated name - %s' % (name)
        
        doc = self._comp_mgr._doc
        comp_node = self.node
        
        scenes_node = doc.createElement('ns0:scenes')
        scenes_node.setAttribute('name', name)
        node_id = self._comp_mgr.new_id()
        scenes_node.setAttribute('id', node_id)

        comp_node.appendChild(scenes_node)

        tl = Timeline(scenes_node)
        self.timelines.append(tl)
        pass

    ## \brief Add a timeline for an existed scenes node.
    #
    def add_timeline_scenes(self, scenes_node):
        tl = Timeline(scenes_node)
        name = tl.name()
        if self.has_timeline(name):
            raise ValueError, \
                'name of scenes node of a timeline is duplicated'

        self.timeline.append(tl)
        pass

    def rm_timeline(self, name):
        for i, tl in enumerate(self.timelines):
            if tl.name() == name:
                comp_node = self.node
                comp_node.removeChild(tl.scenes_node)
                
                del self.timelines[i]
                return
            pass
        raise ValueError, 'try to remove a non-existed timeline - %s' % (name)
    pass


## \brief A mix-in for class domview for management of components.
#
# This class is responsible for manage components and timelines.  It
# is also responsible for switching component and timeline.  Switching
# to a component is actually switching to a timeline in another
# component.  When switch to a timeline, it actuall change
# domview._scense_node, parent of all scene nodes of a component, and
# domview._layers_parent, parent of all layer group of a component.
# domview relys on these two variables to operate on right component
# and timeline.  (It should be changed to get more hint with
# meaningful names.)
#
# FIXME: The root node is always the 'main' component.  It is a
# special case with slightly different in structure.  It should be
# removed and normalized to normal components.
#
class component_manager(object):
    def __init__(self):
        self._components_node = None
        self._components = []
        self._comp_names = set()
        self._main_comp = None
        self._cur_comp = None
        self._cur_timeline = None
        self._components_group = None
        pass

    def _set_main_component(self):
        comp = Component(self, None)
        comp.layers = self._layers
        scenes_node = self._scenes_node
        timeline = Timeline(scenes_node)
        comp.timelines = [timeline]

        self._components.append(comp)
        self._comp_names.add('main')
        
        self._main_comp = comp
        pass

    def _parse_components(self):
        comp_names = self._comp_names
        components_node = self._components_node
        for child in components_node.childList():
            child_name = child.name()
            if child_name != 'ns0:component':
                continue
            if child_name in comp_names:
                raise ValueError, 'duplicate component name %s' % (child_name)

            comp = Component(self, child)
            comp.parse_timelines()
            
            self._components.append(comp)
            comp_names.add(child_name)
            pass
        pass

    ## \brief To initialize subtree of metadata.
    #
    # This method is called by domview._init_metadata().
    #
    def _component_manager_init_metadata(self):
	for n in self._metadata_node.childList():
	    if n.name() == 'ns0:components':
		self._components_node = n
		break
	    pass
	else:
	    components_node = self._doc.createElement("ns0:components")
	    node.appendChild(components_node)
	    self._components_node = components_node
	    pass
        
        for n in self._root.childList():
            if n.name() != 'svg:g':
                continue
            try:
                nlabel = n.getAttribute('inkscape:label')
            except:
                continue
            if nlabel == 'components':
                self._components_group
                break
            pass
        else:                   # no components group
            components_group = self._doc.createElement('svg:g')
            components_group.setAttribute('inkscape:label', 'components')
            gid = self.new_id()
            components_group.setAttribute('id', gid)
            
            self._root.appendChild(components_group)
            self._components_group = components_group
            pass
        pass

    def _start_component_manager(self):
        self._component_manager_init_metadata()
        self._set_main_component()
        self._parse_components()

        self._cur_comp = self._main_comp
        tl = self._main_comp.get_timeline('default')
        self._cur_timeline = tl
        self._scenes_node = tl.scenes_node
        pass

    def _create_component_group(self):
        doc = self._doc
        group = doc.createElement('svg:g')
        gid = self.new_id()
        group.setAttribute('id', gid)
        group.setAttribute('component_group', 'true')
        
        self._components_group.appendChild(group)
        return group

    ## \brief Create a ns0:component node for a given name.
    #
    # \param comp_name is the name of the created component.
    # \param comp_group_id is the component group.
    # \return a ns0:component node.
    #
    def _create_component_node(self, comp_name, comp_group_id):
        comp_node = self._doc.createElement('ns0:component')
        comp_id = self.new_id()
        comp_node.setAttribute('id', comp_id)
        comp_node.setAttribute('name', comp_name)
        comp_node.setAttribute('ref', comp_group_id)
        self._components_node.appendChild(comp_node)
        return comp_node

    ## \brief Get Component object associated with the given name.
    #
    def _get_component(self, comp_name):
        if comp_name in self._comp_names:
            for comp in self._components:
                if comp.name() == comp_name:
                    return comp
                pass
            pass
        raise ValueError, 'can not find component node - %s' % (comp_name)

    ## \brief Create a layer group for give layer of a component.
    #
    def _create_comp_layer_group(self, comp_group, layer_name):
        doc = self._doc
        gid = self.new_id()
        
        layer_group = doc.createElement('svg:g')
        layer_group.setAttribute('id', gid)
        layer_group.setAttribute('inkscape:label', layer_name)
        comp_group.appendChild(layer_group)
        pass
    
    def all_comp_names(self):
        return [comp.name() for comp in self._components]

    def has_component(self, name):
        return name in self._comp_names

    def switch_component(self, comp_name):
        comp = self._get_component(comp_name)
        self._cur_comp = comp
        self._layers = comp.layers
        comp_name = self._cur_comp.name()
        # for domview
        self._layers_parent = self.get_component_group(comp_name)
        
        first_name = comp.all_timeline_names()[0]
        self.switch_timeline(first_name)
        pass

    def add_component(self, comp_name):
        if self.has_component(comp_name):
            raise ValueError, \
                'try add a component with existed name %s' % (comp_name)

        comp_group = self._create_component_group()
        comp_group_id = comp_group.getAttribute('id')
        comp_node = self._create_component_node(comp_name, comp_group_id)

        self._create_comp_layer_group(comp_group, comp_name + ' Layer1')
        
        comp = Component(self, comp_node)
        comp.parse_timelines()
        
        self._components.append(comp)
        self._comp_names.add(comp_name)
        pass

    def add_component_node(self, comp_node):
        comp = Component(self, comp_node)
        comp_name = comp.name()
        if self.has_component(comp_name):
            raise ValueError, \
                'the name of a ns0:component is duplicated'

        self._components.append(comp)
        self._comp_names.add(comp_name)
        pass

    def rm_component(self, comp_name):
        comp = self._get_component(comp_name)
        comp_name = comp.name()
        comp_node = comp.node
        comp_group = self.get_component_group(comp_name)
        
        self._components.remove(comp)
        self._comp_names.remove(comp_name)
        self._components_node.removeChild(comp_node)
        self._components_group.removeChild(comp_group)
        pass
    
    def get_component_group(self, comp_name):
        comp = self._get_component(comp_name)
        
        comp_name = comp.name()
        if comp_name == 'main':
            return self._root
        
        comp_node = comp.node
        gid = comp_node.getAttribute('ref')
        comp_group = self.get_node(gid)
        return comp_group

    def get_current_component(self):
        return self._cur_comp.name()
    
    def switch_timeline(self, timeline_name):
        tl = self._cur_comp.get_timeline(timeline_name)
        self._cur_timeline = tl
        self._scenes_node = tl.scenes_node # of class domview

        # Make domview to rescan layers and scenes.
        self.reset()            # from domview
        pass

    def add_timeline(self, timeline_name):
        self._cur_comp.add_timeline(timeline_name)
        pass

    def rm_timeline(self, timeline_name):
        self._cur_comp.rm_timeline(timeline_name)
        pass

    def all_timeline_names(self):
        r = self._cur_comp.all_timeline_names()
        return r

    def has_timeline(self, name):
        r = self._cur_comp.has_timeline(name)
        return r

    def get_current_timeline(self):
        return self._cur_timeline.name()
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

    ## \brief Rescan the tree.
    #
    def _monitor_reparse(self):
        self._maxframe = 0
        self._id2node = {}
        self._group2scene = {}

        self._collect_node_ids()
        self._collect_all_scenes()
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
            try:
                new_value = int(new_value)
                old_value = int(old_value)
            except TypeError:
                scenes_node = node.parent()
                self._maxframe = self._find_maxframe(scenes_node)
            else:
                if old_value == self._maxframe and old_value > new_value:
                    # _maxframe may be reduced.
                    scenes_node = node.parent()
                    self._maxframe = self._find_maxframe(scenes_node)
                else:
                    self._maxframe = max(int(new_value), self._maxframe)
                    pass
                pass
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
                group_id = scene_node.getAttribute("ref")
	    except:             # the scene node is incompleted.
		continue
	    
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
class domview(domview_monitor, component_manager):
    # Declare variables, here, for keeping tracking
    _doc = None
    _root = None
    
    def __init__(self, *args, **kws):
	super(domview, self).__init__()
        self._metadata_node = None
        #
        # Following two variables would be changed by class
        # component_manager to switch components and timelines.
        #
        self._scenes_node = None
        self._layers_parent = None
	pass

    ## \brief Create a scenes node if not existed.
    #
    def _init_metadata(self):
        self._layers_parent = self._root

	for node in self._root.childList():
	    if node.name() == 'svg:metadata':
		break
	    pass
	else:
	    raise RuntimeError, \
		'can not find <svg:metadata> node in the document'

        self._metadata_node = node
	
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
	root = self._scenes_node
	layers = self._layers
        layers_parent = self._layers_parent
	
	for child in layers_parent.childList():
	    if child.name() != 'svg:g':
		continue
            
            try:
                label = child.getAttribute('inkscape:label')
            except:
                pass
            else:               # has no label
                if label == 'components':
                    continue
                pass

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
	
	self._init_metadata()
	self._start_monitor()	# from domview_monitor
        self._start_component_manager() # from component_manager
	self._parse_all_layers()
	pass

    def reset(self):
        self._monitor_reparse() # from domview_monitor
        self._layers = []
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
        scene_group.setAttribute('scene_group', 'true')

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
            dup_group.removeChild(child) # prevent from crash
            dst_group.appendChild(child)
            pass
	pass

    ## \brief Clone children of a source group to a destinate group.
    #
    # It create a 'svg:use' node for every child of the source group,
    # and append nodes to the desitnate group.
    #
    def clone_group_children(self, src_group, dst_group):
        doc = self._doc

        for src_child in src_group.childList():
            src_child_id = src_child.getAttribute('id')
            dst_child_id = self.new_id()
            
            dst_child = doc.createElement('svg:use')
            dst_child.setAttribute('id', dst_child_id)
            dst_child.setAttribute('xlink:href', '#' + src_child_id)
            dst_child.setAttribute('ns0:duplicate-src', src_child_id)
            dst_group.appendChild(dst_child)
            pass
        pass
    pass

