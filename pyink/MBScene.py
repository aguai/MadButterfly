#!/usr/bin/python
# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; fill-column: 79 -*-
# vim: sw=4:ts=8:sts=4:textwidth=79
import pygtk
import gtk
import glib
import traceback
import pybInkscape
from tween import TweenObject
from domview_ui import MBScene_domview_ui

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
