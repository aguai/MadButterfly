import dom_event
from data_monitor import data_monitor
import traceback

## \brief Check consistency of a DOM-tree associated with a domview_ui.
#
# This is a co-worker of \ref domview_ui to check DOM-tree and make
# sure consistency of scenes, scene groups in the DOM-tree.  It also
# updates layer list of \ref domview_ui to reflect last changes of the
# DOM-tree.
#
# Since it is a consistency checker, it must well understanding
# structure that expected by domview_ui.  So, this class is highly
# closed to the internal of \ref domview_ui.  It is separated from
# domview_ui for collecting related logic and functions in a place to
# set up a clear boundary from the functions provided by \ref
# domview_ui.
#
# This class is expected to access private variables of \ref
# domview_ui.  But, use public interface of domview_ui if possible.
#
# This class is also monitored by \ref data_monitor to monitor the
# accessing to domview_ui.
#
class consistency_checker(object):
    __metaclass__ = data_monitor
    
    def __init__(self, domview_ui):
        self._domviewui = domview_ui
	self._locker = self._domviewui
        self._doc = None
        self._root = None
        pass

    def _start_check(self):
        doc = self._doc
	dom_event.addEventListener(doc, 'DOMNodeInserted',
                                   self.do_insert_node, None)
	dom_event.addEventListener(doc, 'DOMNodeRemoved',
                                   self.do_remove_node, None)
	dom_event.addEventListener(doc, 'DOMAttrModified',
                                   self.do_attr_modified, None)
        pass
    
    ## \brief Handle a new document.
    #
    # This method is called by domview_ui.handle_doc_root().
    #
    def handle_doc_root(self, doc, root):
        self._doc = doc
        self._root = root

        self._start_check()
        pass

    def _insert_node_recursive(self, node, child):
        child_name = child.name()
        
        if child_name == 'ns0:scene':
            scene_group_id = child.getAttribute('ref')
            try:
                scene_group = self._domviewui.get_node(scene_group_id)
            except KeyError:    # can not find associated scene group.
                pass
            else:
                self._domviewui.manage_scene_node(child, scene_group)
                pass
        elif child_name == 'svg:g':
            if node.name() == 'svg:svg':
                self._domviewui.manage_layer_group(child)
                pass
            pass
        
        for cchild in child.childList():
            self._remove_node_recursive(child, cchild)
            pass
        pass

    def _remove_node_recursive(self, node, child):
    	return
        for cchild in child.childList():
            self._remove_node_recursive(child, cchild)
            pass
        
        child_name = child.name()
        if child_name not in ('ns0:scene', 'svg:g'):
            return

        #
        # Remove the key frame assocated with a removed scene node or
        # scene group if we can find the key frame.
        #
        if child_name == 'ns0:scene':
            try:
                group_id = child.getAttribute('ref')
            except:
                return
        elif child_name == 'svg:g':
            try:
                group_id = child.getAttribute('id')
            except:
                return
            pass
            
        try:
            layer_idx, (start, end, tween_type) = \
                self._domviewui.find_key_from_group(group_id)
        except ValueError:
            pass
        else:               # We have found the key frame.
            self._domviewui.unmark_key(layer_idx, start)
            return

        #
        # Remove layers
        #
        if child_name == 'svg:g':
            try:
                layer_idx = self._domviewui.find_layer_from_group(group_id)
            except ValueError:
                pass
            else:               # It is a layer group
                self._domviewui.rm_layer(layer_idx)
                pass
            pass
        pass

    def do_insert_node(self, node, child):
        self._insert_node_recursive(node, child)
        pass

    def do_remove_node(self, node, child):
        self._remove_node_recursive(node, child)
        pass

    def do_attr_modified(self, node, name, old_value, new_value):
    	if node.name() == 'ns0:scene':
	    if name == 'ref':
	        try:
	            if new_value:
		        raise ValueError('The new_value is not empty')
	            self._domviewui.reset()
	        except:
	            traceback.print_exc()
	            pass
	        pass
	    elif name == 'end' or name == 'start':
                self._domviewui.reset()
		pass
	        
        pass
    pass
