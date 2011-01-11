import pybInkscape

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

