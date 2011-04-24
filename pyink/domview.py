import random
import dom_event
from tween import TweenObject
from trait import trait, require, composite


## \brief Compare two nodes with ID.
#
# \return True if node1 ID equivalent to ndoe2 ID.
#
def _id_eq(node1, node2):
    try:
        node1_id = node1.getAttribute('id')
    except:
        return False

    try:
        node2_id = node2.getAttribute('id')
    except:
        return False
    return node1_id == node2_id


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
    	try:
            name = self.scenes_node.getAttribute('name')
	except:
	    name='default'
        return name

    def rename(self, new_name):
        scenes_node = self.scenes_node
        scenes_node.setAttribute('name', new_name)
        pass
    pass


class Transition(object):
    node = None
    condition = None
    target = None
    action = None
    path = None
    
    def __init__(self, node=None):
        self.node = node
        pass

    def set_condition(self, cond):
        node = self.node

        node.setAttribute('condition', cond)
        self.condition = cond
        pass

    def set_action(self, action):
        self.action = action
        node = self.node
        node.setAttribute('action', action)
        pass

    def set_path(self, path):
        self.paht = path
        node = self.node
        path_txt = ' '.join([str(c) for c in path.strip().split()])
        node.setAttribute('path', path_txt)
        pass

    def reparse(self):
        condition = node.getAttribute('condition')
        target = node.getAttribute('target')
        try:
            action = node.getAttribute('action')
        except:
            action = None
            pass
        try:
            path = node.getAttribute('path').strip().split()
            path = [float(c) for c in path]
        except:
            path = None
            pass
        
        self.condition = condition
        self.target = target
        self.action = action
        self.path = path
        pass

    def update_node(self):
        node = self.node
        node.setAttribute('condition', self.condition)
        node.setAttribute('target', self.target)
        if self.action:
            node.setAttribute('action', self.action)
            pass
        if self.path:
            node.setAttribute('path', self.path)
            pass
        pass

    ## \brief Create a node according status of the object.
    #
    # A new node is created for the object, and attributes of the node
    # is initialized with information from the object.
    #
    def create_node(self, doc):
        node = doc.createElement('ns0:transition')
        self.node = node
        self.update_node()
        
        return node
    
    @staticmethod
    def parse_transition(node):
        trn = Transition(node)
        trn.reparse()
        return trn
    pass


## \brief A state in a FSM
#
# Every component can have a FSM to describe its behavior.  Every
# state has an entry action and transitions to other states.  Every
# transition can have a transition action.
#
# The FSM of a component is defined as a child of ns0:component node.
# \verbatim
# <ns0:component>
#     <ns0:states start_state="UP">
#         <ns0:state name="UP" entry_action="entry action">
#             <ns0:transition target="DOWN"
#                             condition="button_down" action="..."/>
#         </ns0:state>
#     </ns0:states>
# </ns0:/component>
# \endverbatim
#
class State(object):
    name = None
    entry_action = None
    transitions = None
    r = None
    x = 0
    y = 0
    
    def __init__(self, node=None):
        self.node = node
        self.transitions = {}
        pass

    def rename(self, name):
        self.name = name
        self.node.setAttribute('name', name)
        pass

    def set_entry_action(self, action):
        self.entry_action = action
        self.node.setAttribute('entry_action', action)
        pass

    def set_r(self, r):
        self.r = r
        self.node.setAttribute('r', str(r))
        pass

    def set_xy(self, x, y):
        self.x = x
        self.y = y
        self.node.setAttribute('x', str(x))
        self.node.setAttribute('y', str(y))
        pass

    def reparse(self):
        node = self.node
        
        name = node.getAttribute('name')
        try:
            entry_action = node.getAttribute('entry_action')
        except:
            entry_action = None
            pass
        try:
            r = float(node.getAttribute('r'))
        except:
            r = None
            pass
        try:
            x = float(node.getAttribute('x'))
        except:
            x = 0
            pass
        try:
            y = float(node.getAttribute('y'))
        except:
            y = 0
            pass
        
        all_transitions = [Transition.parse_transition(child)
                           for child in node.childList()
                           if child.name() == 'ns0:transition']
        transitions = dict([(trn.condition, trn)
                            for trn in all_transitions])
        
        self.name = name
        self.transitions = transitions
        self.entry_action = entry_action
        self.r = r
        self.x = x
        self.y = y
        pass

    def update_node(self):
        node = self.node
        
        node.setAttribute('name', self.name)
        if self.entry_action:
            node.setAttribute('entry_action', self.entry_action)
            pass
        if self.r:
            node.setAttribute('r', self.r)
            pass
        if self.x:
            node.setAttribute('x', self.x)
            pass
        if self.y:
            node.setAttribute('y', self.y)
            pass

        transitions = self.transitions
        for trn in transitions:
            trn.update_node()
            pass
        pass

    def create_node(self, doc):
        node = doc.createElement('ns0:state')
        self.node = node

        self.update_node()
        
        return node

    @staticmethod
    def parse_state(node):
        state = State(node)
        state.parse()
        
        return state

    def change_transition_cond(self, old_cond, new_cond):
        transitions = self.transitions
        trn = transitions[old_cond]
        del transitions[old_cond]
        transitions[new_cond] = trn

        trn.set_condition(new_cond)
        pass

    def add_transition(self, cond, target):
        transitions = self.transitions
        assert cond not in transitions
        
        node = self.node
        
        trn = Transition()
        trn.condition = cond
        trn.target = target
        trn_node = trn.create_node(node.document())

        node.appendChild(trn_node)
        transitions[cond] = trn
        pass

    def rm_transition(self, cond):
        transitions = self.transitions
        if cond not in transitions:
            raise ValueError, \
                'There is no transition defined for this condition (%s)' % \
                (cond)
        
        trn = transitions[cond]
        trn_node = trn.node
        node = self.node
        
        node.removeChild(trn_node)
        del transitions[cond]
        pass

    def all_transitions(self):
        return self.transitions.keys()

    def get_transition(self, cond):
        transitions = self.transitions
        transition = transitions[cond]
        return transition
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
        self.fsm_states = {}
        self.fsm_states_node = None
        self.fsm_start_state = None

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

    def _get_comp_node(self):
        return self.node or self._comp_mgr._metadata_node

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
        self.timelines[:] = []
        
        if self.node:           # main component has None value for self.node
            assert self.node.name() == 'ns0:component'
            pass
        
        comp_node = self._get_comp_node()
        for child in comp_node.childList():
            if child.name() == 'ns0:scenes':
                tl = Timeline(child)
                self.timelines.append(tl)
                print '    ' + tl.name()
                pass
            pass
        pass

    ## \brief Parse FSM from a ns0:states node.
    #
    def parse_states(self):
        comp_node = self._get_comp_node()
        assert (not comp_node) or comp_node.name() == 'ns0:component'
        
        states_nodes = [node
                        for node in comp_node.childList()
                        if node.name() == 'ns0:states']
        if not states_nodes:
            self.fsm_states = {}
            return
        states_node = states_nodes[0]
        
        try:
            self.fsm_start_state = states_node.getAttribute('start_state')
        except:
            pass
        
        state_nodes = [child
                       for child in states_node.childList
                       if child.name() == 'ns0:state']
        states = [State.parse_state(node) for state_node in state_nodes]
        self.fsm_states = dict([(state.name, state) for state in states])

        self.fsm_states_node = states_node
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
        comp_node = self._get_comp_node()
        
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
                comp_node = self._get_comp_node()
                comp_node.removeChild(tl.scenes_node)
                
                del self.timelines[i]
                return
            pass
        raise ValueError, 'try to remove a non-existed timeline - %s' % (name)

    def rename_timeline(self, timeline_name, new_name):
        for i, tl in enumerate(self.timelines):
            if tl.name() == timeline_name:
                tl.rename(new_name)
                return
            pass
        raise ValueError, 'try to remove a non-existed timeline - %s' % (name)

    def rename(self, new_name):
        self.node.setAttribute('name', new_name)
        pass

    def _create_states_node(self):
        node = self._get_comp_node()
        doc = self._comp_mgr._doc

        states_node = doc.createElement('ns0:states')
        node.appendChild(states_node)
        self.fsm_states_node = states_node
        pass

    def get_start_state_name(self):
        return self.fsm_start_state

    def set_start_state(self, state_name):
        assert state_name in self.fsm_states
        self.fsm_start_state = state_name
        pass

    def all_state_names(self):
        return self.fsm_states.keys()

    def get_state(self, name):
        return self.fsm_states[name]

    def add_state(self, name):
        if not self.fsm_states_node:
            self._create_states_node()
            pass
        
        doc = self._comp_mgr._doc
        
        if name in self.fsm_states:
            raise KeyError, \
                'Add a state with a name (%s) been used' % (name)
        
        state = State()
        state.name = name
        self.fsm_states[name] = state

        state_node = state.create_node(doc)
        states_node = self.fsm_states_node
        states_node.appendChild(state_node)
        pass

    def rename_state(self, state_name, new_name):
        state = self.fsm_states[state_name]
        del self.fsm_states[state_name]
        self.fsm_states[new_name] = state
        
        state.rename(new_name)
        pass

    def rm_state(self, name):
        fsm_states = self.fsm_states
        state = fsm_states[name]
        del self.fsm_states[name]

        state_node = state.node
        states_node = self.fsm_states_node
        states_node.removeChild(state_node)
        pass
    pass


## \brief A mix-in for class component_manager for UI updating.
#
# This class collects all methods for supporting UI updating.
#
class component_manager_ui_update(object):
    ## \brief Update the list of components.
    #
    def reparse_components(self):
        saved_cur_comp = self._cur_comp
        self._components[:] = [self._main_comp]
        self._comp_names.clear()
        self._parse_components()

        for comp in self._components:
            if comp.name() == saved_cur_comp.name():
                self._cur_comp = comp
                break
            pass
        else:
            self._cur_comp = self._main_comp
            pass
        pass

    ## \brief Update the list of timelines of current component.
    #
    def reparse_timelines(self):
        comp = self._cur_comp
        saved_cur_timeline = self._cur_timeline
        comp.parse_timelines()
        
        for timeline in comp.timelines:
            if timeline.name() == saved_cur_timeline.name():
                self._cur_timeline = timeline
                break
            pass
        else:
            self._cur_timeline = comp.timelines[0]
            pass
        pass
    pass


## \brief A trait for class domview for managing components.
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
@trait
class component_manager(component_manager_ui_update):
    _scenes_node = require
    _metadata_node = require
    _doc = require
    _root = require
    _layers = require
    _layers_parent = require
    _cur_comp = require
    new_id = require
    get_node = require
    reset = require

    def __init__(self):
        super(component_manager, self).__init__()
        self._components_node = None
        self._components = []
        self._comp_names = set()
        self._main_comp = None
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
            child_node_name = child.name()
            if child_node_name != 'ns0:component':
                continue
            
            child_name = child.getAttribute('name')
            if child_name in comp_names:
                raise ValueError, 'duplicate component name %s' % (child_name)

            comp = Component(self, child)
            comp.parse_timelines()
            comp.parse_states()
            
            self._components.append(comp)
            
            comp_names.add(child_name)
            pass
        pass

    ## \brief To initialize subtree of metadata.
    #
    # This method is called by domview._init_metadata().
    #
    def _component_manager_init_metadata(self):
        metadata_node = self._metadata_node

        # Make sure ns0:components in metadata
	for n in metadata_node.childList():
	    if n.name() == 'ns0:components':
		self._components_node = n
		break
	    pass
	else:
	    components_node = \
                self._doc.createElement("ns0:components")
	    metadata_node.appendChild(components_node)
	    self._components_node = components_node
	    pass
        
        # Make sure the group for containing components.
        for n in self._root.childList():
            if n.name() != 'svg:g':
                continue
            try:
                nlabel = n.getAttribute('inkscape:label')
            except:
                continue
            if nlabel == 'components':
                self._components_group = n
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

    ## \brief Create component group
    #
    # A component group is a group with a layers group as child.
    # The layers group is where layer groups is in.
    #
    def _create_component_group(self):
        doc = self._doc
        group = doc.createElement('svg:g')
        gid = self.new_id()
        group.setAttribute('id', gid)
        
        self._components_group.appendChild(group)

        # Create layers group
        layers_group = doc.createElement('svg:g')
        gid = self.new_id()
        layers_group.setAttribute('id', gid)
        layers_group.setAttribute('inkscape:label', 'layers')
        group.appendChild(layers_group)
        
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
    def _create_comp_layer_group(self, layers_group, layer_name):
        doc = self._doc
        gid = self.new_id()
        
        layer_group = doc.createElement('svg:g')
        layer_group.setAttribute('id', gid)
        layer_group.setAttribute('inkscape:label', layer_name)
        layer_group.setAttribute('inkscape:groupmode', 'layer')
        layers_group.appendChild(layer_group)
        
        return layer_group
    
    ## \brief Return group of specified layer in a component.
    #
    # This is here for getting layer group without switch current
    # component.
    #
    def _get_group_of_component_layer(self, comp_name, layer_idx):
        comp = self._get_component(comp_name)
        layer = comp.layers[layer_idx]
        return layer.group

    def _get_layers_group_of_component(self, comp_name):
        if comp_name == 'main':
            return self._root
        
        comp_group = self.get_component_group(comp_name)
        layers_group = comp_group.firstChild()
        assert layers_group.getAttribute('inkscape:label') == 'layers'
        
        return layers_group
    
    def all_comp_names(self):
        return [comp.name() for comp in self._components]

    def has_component(self, name):
        return name in self._comp_names

    def hide_component(self, comp_name):
        if comp_name == 'main':
            comp = self._get_component(comp_name)
            for layer in comp.layers:
                group = layer.group
                group.setAttribute('style', 'display: none')
                pass
            return

        comp_group = self.get_component_group(comp_name)
        comp_group.setAttribute('style', 'display: none')
        pass

    def show_component(self, comp_name):
        if comp_name == 'main':
            comp = self._get_component(comp_name)
            for layer in comp.layers:
                group = layer.group
                group.setAttribute('style', '')
                pass
            return

        comp_group = self.get_component_group(comp_name)
        comp_group.setAttribute('style', '')
        pass

    def switch_component(self, comp_name):
        old_comp = self._cur_comp
        
        comp = self._get_component(comp_name)
        self._cur_comp = comp
        self._layers = comp.layers
        comp_name = self._cur_comp.name()
        # for domview
        self._layers_parent = \
            self._get_layers_group_of_component(comp_name)
        
        self.make_sure_timeline()

        try:
            comp_grp = self.get_component_group(old_comp.name())
            old_comp_existed = True
        except ValueError:
            old_comp_existed = False
            pass
        
        if old_comp_existed:
            self.hide_component(old_comp.name())
            pass
        
        self.show_component(comp.name())
        pass

    def add_component(self, comp_name):
        if self.has_component(comp_name):
            raise ValueError, \
                'try add a component with existed name %s' % (comp_name)
        
        comp_group = self._create_component_group()
        comp_group_id = comp_group.getAttribute('id')
        comp_node = self._create_component_node(comp_name, comp_group_id)
        
        comp = Component(self, comp_node)
        comp.parse_timelines()

        self._components.append(comp)
        self._comp_names.add(comp_name)

        # Create Layer1 (at least one layer for a component)
        layers_group = self._get_layers_group_of_component(comp_name)
        layer_group = self._create_comp_layer_group(layers_group, 'Layer1')
        layer = Layer(layer_group)
        comp.layers.append(layer)
        
        self.hide_component(comp_name)
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

    def rename_component(self, comp_name, new_name):
        comp = self._get_component(comp_name)
        comp.rename(new_name)
        self._comp_names.remove(comp_name)
        self._comp_names.add(new_name)
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

    ## \brief Hide scene groups of current timeline.
    #
    # This method all scene groups of current timeline invisible.
    #
    def _hide_current_timeline(self):
        tl = self._cur_timeline
        scenes_node = tl.scenes_node
        for child in scenes_node.childList():
            if child.name() != 'ns0:scene':
                continue
            gid = child.getAttribute('ref')
            group = self.get_node(gid)
            group.setAttribute('style', 'display: none')
            pass
        pass
    
    def switch_timeline(self, timeline_name):
        if self._cur_timeline:
            self._hide_current_timeline()
            pass
        
        tl = self._cur_comp.get_timeline(timeline_name)
        self._cur_timeline = tl
        self._scenes_node = tl.scenes_node # of class domview

        # Make domview to rescan layers and scenes.
        self.reset()            # from domview

        cur_comp_name = self.get_current_component()
        cur_comp_node = self.get_component_group(cur_comp_name)
        cur_comp_node.setAttribute("cur_timeline", timeline_name)
        pass

    def make_sure_timeline(self):
        cur_comp_name = self.get_current_component()
        cur_comp_node = self.get_component_group(cur_comp_name)
        try:
            timeline_name = cur_comp_node.getAttribute("cur_timeline")
        except KeyError:
            timeline_name = self.all_timeline_names()[0]
            pass
        self._cur_timeline = None
        self.switch_timeline(timeline_name)
        pass

    def add_timeline(self, timeline_name):
        self._cur_comp.add_timeline(timeline_name)
        pass

    def rm_timeline(self, timeline_name):
        self._cur_comp.rm_timeline(timeline_name)
        pass

    def rename_timeline_of_component(self, timeline_name, new_name, comp_name):
        comp = self._get_component(comp_name)
        comp.rename_timeline(timeline_name, new_name)
        pass

    def rename_timeline(self, timeline_name, new_name):
        comp_name = self._cur_comp.name()
        self.rename_timeline_of_component(timeline_name, new_name, comp_name)
        pass

    def all_timeline_names(self):
        r = self._cur_comp.all_timeline_names()
        return r

    def has_timeline(self, name):
        r = self._cur_comp.has_timeline(name)
        return r

    def get_current_timeline(self):
        return self._cur_timeline.name()

    ## \brief Add a new component from a group node.
    #
    # The group node is reparented to the group of first layer of
    # specified component.
    #
    def mv_group_to_component(self, group, comp_name):
        group_parent = group.parent()
        if group_parent:
            group_parent.removeChild(group)
            pass
        
        layer_group = self._get_group_of_component_layer(comp_name, 0)
        layer_group.appendChild(group)
        pass

    ## \brief Create a link to a component.
    #
    # \param parent_group is where the link will be pliaced in.
    # \return link node.
    #
    def link_to_component(self, comp_name, parent_group):
        layers_group = self._get_layers_group_of_component(comp_name)
        
        use_node = self._doc.createElement('svg:use')
        layers_group_id = layers_group.getAttribute('id')
        use_node.setAttribute('xlink:href', '#' + layers_group_id)
        use_node_id = self.new_id()
        use_node.setAttribute('id', use_node_id)
        use_node.setAttribute('use_component', 'true')

        parent_group.appendChild(use_node)
        
        return use_node

    ## \brief Remember current frame and layer on the scenes node.
    #
    def remember_current_frame(self, layer_idx, frame_idx):
        if not isinstance(layer_idx, int):
            raise TypeError, 'layer index should be a integer'
        if not isinstance(frame_idx, int):
            raise TypeError, 'frame index should be a integer'
        
        timeline_name = self.get_current_timeline()
        timeline = self._cur_comp.get_timeline(timeline_name)
        timeline_scenes = timeline.scenes_node
        timeline_scenes.setAttribute('cur_layer', str(layer_idx))
        timeline_scenes.setAttribute('cur_frame', str(frame_idx))
        pass

    ## \brief Get current frame and layer from the scenes node.
    #
    def get_current_frame(self):
        timeline_name = self.get_current_timeline()
        timeline = self._cur_comp.get_timeline(timeline_name)
        timeline_scenes = timeline.scenes_node
        try:
            cur_layer = timeline_scenes.getAttribute('cur_layer')
        except KeyError:
            cur_layer_idx = 0
        else:
            cur_layer_idx = int(cur_layer)
            pass
        try:
            cur_frame = timeline_scenes.getAttribute('cur_frame')
        except KeyError:
            cur_frame_idx = 0
        else:
            cur_frame_idx = int(cur_frame)
            pass

        return cur_layer_idx, cur_frame_idx
    pass


## \brief A trait for management FSM associated with current component.
#
@trait
class FSM_manager(object):
    _cur_comp = require

    def __init__(self):
        super(FSM_manager, self).__init__()
        pass
    
    def all_state_names(self):
        return self._cur_comp.all_state_names()

    def get_start_state_name(self):
        return self._cur_comp.get_start_state_name()

    ## \brief To return state object for the given name.
    #
    # This method should only be used by component_manager internally.
    #
    def _get_state(self, state_name):
        return self._cur_comp.get_state(state_name)

    def rm_state(self, state_name):
        self._cur_comp.rm_state(state_name)
        pass

    def add_state(self, state_name):
        self._cur_comp.add_state(state_name)
        pass

    def rename_state(self, state_name, new_name):
        self._cur_comp.rename_state(state_name, new_name)
        pass

    def set_start_state(self, state_name):
        self._cur_comp.set_start_state(state_name)
        pass

    def set_state_entry_action(self, state_name, entry_action):
        state = self._get_state(state_name)
        state.set_entry_action(entry_action)
        pass

    def set_state_r(self, state_name, r):
        state = self._get_state(state_name)
        state.set_r(r)
        pass

    def set_state_xy(self, state_name, x, y):
        state = self._get_state(state_name)
        state.set_xy(x, y)
        pass

    def get_state_entry_action(self, state_name):
        state = self._get_state(state_name)
        action = state.get_entry_action()
        return action

    def get_state_r(self, state_name):
        state = self._get_state(state_name)
        r = state.r
        return r

    def get_state_xy(self, state_name):
        state = self._get_state(state_name)
        xy = state.x, state.y
        return xy

    def all_transitions(self, state_name):
        state = self._get_state(state_name)
        trn_names = state.all_transitions()
        return trn_names

    def add_transition(self, state_name, cond, target):
        state = self._get_state(state_name)
        state.add_transition(cond, target)
        pass

    def rm_transition(self, state_name, cond):
        state = self._get_state(state_name)
        state.rm_transition(cond)
        pass

    def change_transition_cond(self, state_name, old_cond, new_cond):
        state = self._get_state(state_name)
        state.change_transition_cond(old_cond, new_cond)
        pass

    def get_transition(self, state_name, cond):
        state = self._get_state(state_name)
        trn = state.get_transition(cond)
        
        cond = trn.condition
        target = trn.target
        action = trn.action
        path = trn.path
        
        return cond, target, action, path

    def set_transition_action(self, state_name, cond, action):
        trn = state.get_transition(state_name, cond)
        trn.set_action(action)
        pass

    def set_transition_path(self, state_name, cond, path):
        trn = state.get_transition(state_name, cond)
        trn.set_path(path)
        pass
    pass


## \brief Parser for scenes nodes.
#
# This class parses scenes nodes and collect ID of all nodes.
#
@trait
class scenes_parser(object):
    _root = require
    _scenes_node = require
    _id2node = require
    _group2scene = require
    current = require
    _maxframe = require
    
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
            pass
        else:
            self._id2node[node_id] = node
            pass
        
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
        scenes_node = self._scenes_node
        self._parse_one_scenes(scenes_node)
        self._maxframe = self._find_maxframe(scenes_node)
        pass
	pass
    
    ## \brief Return the node with given ID.
    #
    def get_node(self, node_id):
	value = self._id2node[node_id]
        if isinstance(value, list):
            return value[-1]
        return value

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

## \brief Monitor changes of DOM-tree.
#
# This class monitors DOM-tree to maintain _maxframe and maps for node ID to
# node and scene group ID to scene node.
@composite
class domview_monitor(object):
    use_traits = (scenes_parser,)

    method_map_traits = {
        scenes_parser._find_maxframe: '_find_maxframe',
        scenes_parser._collect_all_scenes: '_collect_all_scenes',
        scenes_parser._collect_node_ids: '_collect_node_ids'}
    
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

    ## \brief Add a node to id2node mapping.
    #
    # domview_monitor._id2node is a multiple mapping to map a key to
    # multiple node.  The reason that it is not a single mapping is
    # Inkscape would insert a node with the ID from the node been
    # copied, and change its ID to a unique one later.  So, we must
    # provide the capability to handle two or more nodes with the same
    # ID.
    def _map_id2node(self, node, node_id):
        if node_id in self._id2node:
            old_value = self._id2node[node_id]
            if isinstance(old_value, list):
                old_value.append(node)
            else:
                self._id2node[node_id] = [old_value, node]
                pass
        else:
            self._id2node[node_id] = node
            pass
        pass

    def _unmap_id2node(self, node, node_id):
        if node_id not in self._id2node:
            raise ValueError, 'invalide node ID (%s)' % (node_id)

        value = self._id2node[node_id]
        if isinstance(value, list):
            value.remove(node)
            if not value:
                del self._id2node[node_id]
                pass
            pass
        else:
            del self._id2node[node_id]
            pass
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
            self._map_id2node(child, child_id)
	    pass

	if child.name() == 'ns0:scene' and _id_eq(node, self._scenes_node):
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

    def _on_remove_node(self, node, child):
	for cchild in child.childList():
	    self._on_remove_node(child, cchild)
	    pass
	
	try:
	    child_id = child.getAttribute('id')
	except:
	    pass
	else:
            self._unmap_id2node(child, child_id)
	    pass
	
	if child.name() == 'ns0:scene' and _id_eq(node, self._scenes_node):
	    try:
		ref = child.getAttribute('ref')
	    except:
		pass
	    else:
		del self._group2scene[ref]
		pass

	    try:
		if int(child.getAttribute('start')) == self._maxframe or \
                        int(child.getAttribute('end')) == self._maxframe:
		    self._maxframe = self._find_maxframe(node)
		    pass
	    except:
		pass
	    pass
	pass

    def _on_attr_modified(self, node, name, old_value, new_value):
        if old_value == new_value:
            return
        
	if name == 'id':
	    if old_value and old_value in self._id2node:
                self._unmap_id2node(node, old_value)
		pass
            if new_value:
                self._map_id2node(node, new_value)
                pass
	    pass
	elif name == 'ref' and node.name() == 'ns0:scene':
            parent_node = node.parent()
            scenes_node = self._scenes_node
            if not _id_eq(parent_node, scenes_node):
                return          # not in current timeline
            
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
            parent_node = node.parent()
            scenes_node = self._scenes_node
            if not _id_eq(parent_node, scenes_node):
                return          # not in current timeline
            
            try:
                new_value = int(new_value)
                old_value = int(old_value)
            except TypeError:
                self._maxframe = self._find_maxframe(scenes_node)
            else:
                if old_value == self._maxframe and old_value > new_value:
                    # _maxframe may be reduced.
                    self._maxframe = self._find_maxframe(scenes_node)
                else:
                    self._maxframe = max(int(new_value), self._maxframe)
                    pass
                pass
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


@trait
class layers_parser(object):
    _doc = require
    _layers = require
    _layers_parent = require
    get_scene = require
    get_node = require
    new_id = require
    
    def parse_all_layers(self):
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
    
    def get_layer_num(self):
	return len(self._layers)

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

    def reset_layers(self):
        self._layers[:] = []
        self.parse_all_layers()
        pass
    pass

## \brief This layer provide a data view to the DOM-tree.
#
# This class maintains layers information, and provides functions to create,
# change and destroy scene node and scene group.  A scene node is a 'ns0:scene'
# in 'ns0:scenes' tag.  A scene group is respective 'svg:g' for a scene.
#
@composite
class domview(domview_monitor):
    use_traits = (component_manager, layers_parser, FSM_manager)
    
    method_map_traits = {component_manager._start_component_manager:
                             '_start_component_manager'}

    # Declare variables, here, for keeping tracking
    _doc = None
    _root = None
    # Required by component_manager and FSM_manager
    _cur_comp = None
    
    def __init__(self, *args, **kws):
	super(domview, self).__init__()
        self._metadata_node = None
        #
        # Following two variables would be changed by class
        # component_manager to switch components and timelines.
        #
        self._scenes_node = None
        self._layers_parent = None
        self._layers = []
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
            scenes_node_id = 'main_default_scenes'
            scenes_node.setAttribute('id', scenes_node_id)
            scenes_node.setAttribute('name', 'default')
	    node.appendChild(scenes_node)
	    self._scenes_node = scenes_node
	    pass
	pass

    def handle_doc_root(self, doc, root):
	self._doc = doc
	self._root = root
	
	self._init_metadata()
	self._start_monitor()	# from domview_monitor
        self._start_component_manager()
        self.reset_layers()
	pass

    def reset(self):
        self._monitor_reparse() # from domview_monitor
        self.reset_layers()
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

    ## \brief To test a graphic node.
    #
    # A graphic node is a SVG node that is not layer group, scene
    # group, ... etc.  It is only a normal node in a layer group or a
    # scene group.
    def is_graph_node(self, node):
        try:
            mode = node.getAttribute('inkscape:groupmode')
        except:
            pass
        else:
            if mode == 'layer':
                return False
            pass

        try:
            label = node.geteAttribute('inkscape:label')
        except:
            pass
        else:
            return False
        
        try:
            scene_group = node.geteAttribute('scene_group')
        except:
            pass
        else:
            if scene_group == 'true':
                return False
            pass
        
        return True
    pass

