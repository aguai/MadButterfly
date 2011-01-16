import dom_event
from data_monitor import data_monitor

class unlink_clone_checker(object):
    __metaclass__ = data_monitor
    
    _no_change_attrs = ('ns0:duplicate-src', 'saved_id')
    
    def __init__(self, domview_ui):
        self._domviewui = domview_ui
        self._locker = domview_ui
        self._doc = None
        self._root = None
        pass

    def _start_check(self):
        doc = self._doc
	dom_event.addEventListener(doc, 'DOMNodeInserted',
                                   self.do_insert_node, None)
	dom_event.addEventListener(doc, 'DOMAttrModified',
                                   self.do_attr_modified, None)
        pass

    def handle_doc_root(self, doc, root):
        self._doc = doc
        self._root = root

        self._start_check()
        pass

    def _handle_unlinked_or_copied_nodes(self, node, child):
        try:
            saved_id = child.getAttribute('saved_id')
        except:                 # Skip it for losting saved_id
            pass
        else:
            child.setAttribute('ns0:duplicate-src', saved_id)
            pass
        
        try:
            child_id = child.getAttribute('id')
        except:                 # still have no ID.
            pass                # Assign saved_id later with attr
                                # modified event.
        else:
            child.setAttribute('saved_id', child_id)
            pass
        pass

    def _handle_new_nodes(self, node, child):
        if child.name() == 'svg:use':
            return

        try:
            child_id = child.getAttribute('id')
        except KeyError:
            return
        child.setAttribute('saved_id', child_id)
        pass

    ## \brief Check inserted node recurisvely.
    #
    # Travel the tree in post-order.
    #
    def _handle_insert_node_recursive(self, node, child):
        #
        # Traveling forest of children.
        #
        for cchild in child.childList():
            self._handle_insert_node_recursive(child, cchild)
            pass
        
        #
        # Visit the node
        #
        try:
            child_id = child.getAttribute('saved_id')
        except KeyError:                 # have no saved_id
            pass
        else:
            self._handle_unlinked_or_copied_nodes(node, child)
            return

        # have no saved_id
        self._handle_new_nodes(node, child)
        pass

    def do_insert_node(self, node, child):
        self._handle_insert_node_recursive(node, child)
        pass

    def do_attr_modified(self, node, name, old_value, new_value):
        if name == 'id' and node.name() != 'svg:use':
            #
            # The ID of a node may not be assigned when it being
            # inserted, and be assigned later.  So, we checking
            # attribute modification event to assign value of
            # saved_id.
            #
            node.setAttribute('saved_id', new_value)
        elif old_value and (name in self._no_change_attrs):
            #
            # Restore to old value for attributes that is not allowed
            # to be changed by the user.
            #
            node.setAttribute(name, old_value)
            pass
        pass
    pass
