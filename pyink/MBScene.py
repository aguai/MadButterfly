#!/usr/bin/python
# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; fill-column: 79 -*-
# vim: sw=4:ts=8:sts=4:textwidth=79
import pygtk
import gtk
import glib
import traceback
import pybInkscape
from tween import scenes_director
from domview_ui import create_domview_ui
from data_monitor import data_monitor

## \page design_scribboo Designs of Scribboo
#
# \image html scribboo_arch.png
#
# The idea of Scribboo is that domview_ui is responsible for synchronizing
# domview and framelines.  domview is responsible for managing data model
# provided by Inkscape for SVG documents.  All access to data model use APIs
# provided by domview.  domview_ui is a decorator of domview.  It does not only
# delegate calls to domview, but also make sure that framelines are always up
# to date and keep consistent with data model provided by domview.
#
# MBScene implements most feature about editing scenes.  It uses domview_ui to
# manage and control scenes.  With MBScene, you can create, delete, and change
# scenes.  It also provide the capability of tweening animation.
#
# comp_dock is responsible for managing components and actions.  You can see a
# list of components and a list of actions.  These two list boxes are
# implemented by comp_dock.
#
# FSM_window is responsible for implementation of FSM editor.  All features
# provided by FSM editor is a part of FSM_window.  It also use domview_ui to
# access data model.
#

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

## \brief MBScene connect GUI and DOM-tree
#
# This method accepts user actions and involves domview_ui to update
# data on the document.
#
# This class is protected by \ref data_monitor, meta-class.
#
class MBScene(object):
    __metaclass__ = data_monitor

    _tween_type_names = ('normal', 'scale')
    
    _num_frames_of_line = 100
    
    def __init__(self, desktop, win, root=None):
	super(MBScene, self).__init__()

	self.desktop = desktop
	self.window = win
	self.top = None
	self.last_update = None
	pybInkscape.inkscape.connect('change_selection', self.do_selection)
	self.last_select = None
	self._director = None
	self.document = None
	self._root = root
	self.framerate = 12
	self._disable_tween_type_selector = False
	self.current = 0

	self._domviewui = create_domview_ui()
	self._locker = self._domviewui
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
		    self._domviewui.find_key_from_group(node_id)
	    except:
		pass
	    else:
		self._domviewui.set_active_layer_frame(layer_idx, start)
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
	    self._domviewui.mark_key(layer_idx, frame_idx)
	except ValueError:	# existed key frame
	    pass
	pass

    def removeKeyScene(self, layer_idx, frame_idx):
	self._domviewui.unmark_key(layer_idx, frame_idx)
	self._director.show_scene(frame_idx)
	pass
    
    def extendScene(self):
	# Create a tween
	layer_idx, frame_idx = self._domviewui.get_active_layer_frame()
	start, end, tween_type = \
	    self._domviewui.get_left_key(layer_idx, frame_idx)
	tween_len = frame_idx - start
	self._domviewui.tween(layer_idx, start, tween_len, tween_type)

	# Create a key frame which link to the previous key frame
	self._domviewui.mark_key(layer_idx, frame_idx)
	self._domviewui.clone_key_group(layer_idx, start, frame_idx)
	self._director.show_scene(frame_idx)
	self.selectSceneObject(layer_idx, frame_idx)
	pass
    
    def _enterGroup(self, scene_group):
	self.desktop.selection.clear()
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
		self._domviewui.get_key(layer_idx, frame_idx)
	except:
	    dup_group = self._domviewui.get_layer_dup_group(layer_idx)
	    self._enterGroup(dup_group)
	    return

	scene_group = self._domviewui.get_key_group(layer_idx, start)
	self._enterGroup(scene_group)
	self.setTweenType(tween_type)
	pass

    def duplicateKeyScene(self):
        # Search for the current scene
	layer_idx, frame_idx = self._domviewui.get_active_layer_frame()
	try:
	    self.removeKeyScene(layer_idx, frame_idx)
	except:			# no key and tween
	    pass

	try:
	    left_start, left_end, left_tween_type = \
		self._domviewui.get_left_key(layer_idx, frame_idx)
	except:
	    return
	if left_end >= frame_idx:
	    return

	self._domviewui.mark_key(layer_idx, frame_idx)
	self._domviewui.copy_key_group(layer_idx, left_start, frame_idx)

	self._director.show_scene(frame_idx)
	pass

    def _drop_undo(self):
	self.document.commit()	# commit the transation and drop change log.
	self.document.beginTransaction()
    
    def addNameEditor(self,hbox):
	self.nameEditor = gtk.Entry(max=40)
	hbox.pack_start(self.nameEditor,expand=False,fill=False)
	self.editDone = gtk.Button('Set')
	hbox.pack_start(self.editDone,expand=False,fill=False)
	self.editDone.connect('clicked', self.do_changeObjectLabel)
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
	self.tweenTypeSelector.connect('changed', self.do_TweenTypeChange)
	pass
    
    def do_changeObjectLabel(self,w):
	o = self.desktop.selection.list()[0]
	o.setAttribute("inkscape:label", self.nameEditor.get_text())
	pass

    def do_selection(self,w,obj):
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

    def do_CellClick(self, layer_idx, frame_idx):
	self._director.show_scene(frame_idx)
	self.selectSceneObject(layer_idx, frame_idx)
	self._domviewui.remember_current_frame(layer_idx, frame_idx)
        pass

    def doAddLayer(self, w):
	domview = self._domviewui
	layer_num = domview.get_layer_num()
	domview.insert_layer(layer_num)
	pass

    def doRemoveLayer(self, w):
	domview = self._domviewui
	layer_idx, frame_idx = domview.get_current_frame()
	if layer_idx == 0:	# never remove first layer (default)
	    return
	
	# We must switch current layer to another one before removing a layer
	# group node.  It avoids a crash since inkscape does not know the group
	# node of the layer is removed, and it would emit a signal on this node
	# later.
	self.selectSceneObject(layer_idx - 1, frame_idx)
	
	self._domviewui.remember_current_frame(layer_idx - 1, frame_idx)
	domview.rm_layer(layer_idx)
	pass

    def doInsertKeyScene(self,w):
	layer_idx, frame_idx = self._domviewui.get_active_layer_frame()
	self.insertKeyScene(layer_idx, frame_idx)
	self.selectSceneObject(layer_idx, frame_idx)
	self._drop_undo()
	return
    
    def doDuplicateKeyScene(self,w):
        self.duplicateKeyScene()
	self._drop_undo()
	pass

    def doRemoveScene(self,w):
	layer_idx, frame_idx = self._domviewui.get_active_layer_frame()
	self.removeKeyScene(layer_idx, frame_idx)
	self._drop_undo()
	return

    def doExtendScene(self,w):
	self.extendScene()
	self._drop_undo()
	pass

    def doRun(self,arg):
        """
	    Execute the current animation till the last frame.
	"""
	if self.btnRun.get_label() == "Run": # Go running
	    self.desktop.selection.clear()
	    
	    #
	    # Make dup groups empty.
	    # It forces TweenObject to re-generate content from scratch.
	    #
	    nlayers = self._domviewui.get_layer_num()
	    for layer_idx in range(nlayers):
		layer_dup = self._domviewui.get_layer_dup_group(layer_idx)
		for child in layer_dup.childList():
		    layer_dup.removeChild(child)
		    pass
		pass
	    
	    self.btnRun.set_label("Stop")
	    tmout = 1000 / self.framerate
            self.last_update = glib.timeout_add(tmout, self.doRunNext)
	else:			# Stop running
	    self.btnRun.set_label("Run")
	    glib.source_remove(self.last_update)
	    pass
	pass

    def doRunNext(self):
	if self.current > self._domviewui.get_max_frame():
	    self.current = 0
	    pass
	try:
	    self._director.show_scene(self.current)
	except:
	    traceback.print_exc()
	    raise
	self.current = self.current + 1
	tmout = 1000 / self.framerate
        self.last_update = glib.timeout_add(tmout, self.doRunNext)
	pass

    def doInsertFrame(self, w):
	layer_idx, frame_idx = self._domviewui.get_active_layer_frame()
	self._domviewui.insert_frames(layer_idx, frame_idx, 1)
	self._drop_undo()

    def doRemoveFrame(self, w):
	layer_idx, frame_idx = self._domviewui.get_active_layer_frame()
	self._domviewui.rm_frames(layer_idx, frame_idx, 1)
	self._drop_undo()

    def do_TweenTypeChange(self, w):
	if self._disable_tween_type_selector:
	    return

	layer_idx, frame_idx = self._domviewui.get_active_layer_frame()
	tween_type = self.tweenTypeSelector.get_active()
	
	start, end, old_tween_type = \
	    self._domviewui.get_left_key(layer_idx, frame_idx)
	if end >= frame_idx and start != end:
	    # Length of tween > 1 and cover this frame
	    self._domviewui.chg_tween(layer_idx, start, tween_type=tween_type)
	    pass
	self._drop_undo()
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
	btn = gtk.Button('Add a Layer')
	btn.connect('clicked', self.doAddLayer)
	hbox.pack_start(btn, expand=False, fill=False)

	btn = gtk.Button('Remove the Layer')
	btn.connect('clicked', self.doRemoveLayer)
	hbox.pack_start(btn, expand=False, fill=False)

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

    def do_show(self):
	self.OK = True
	if not self._root:
	    self._root = self.desktop.doc().root().repr
	    pass
	
	self.document = self.desktop.doc().rdoc
	
	self._domviewui.set_desktop(self.desktop)
	self._domviewui.handle_doc_root(self.document, self._root)
	self._domviewui.register_active_frame_callback(self.do_CellClick)
	self._director = scenes_director(self._domviewui)

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
	frame_ui = self._domviewui.get_frame_ui_widget()
	vbox.pack_start(frame_ui, expand=False)
	hbox=gtk.HBox(False, 0)
	self._add_buttons(hbox)
	vbox.pack_start(hbox, expand=False)

	self.top.show_all()
	self.last_update = None
	
	self._drop_undo()

	self.desktop.connectCurrentLayerChanged(self.handle_change_layer)
	
	return False

    ## \brief To handle context menu event.
    #
    def do_make_component_from_group(self, node):
	node_parent_group = node.parent()
	
	comp_name = 'Component ' + node.getAttribute('id')
	i = 0
	while comp_name in self._domviewui.all_comp_names():
	    comp_name = 'Component %s - %d' % (comp_name, i)
	    i = i + 1
	    pass
	self._domviewui.add_component_from_group(comp_name, node)
	
	self._domviewui.link_to_component(comp_name, node_parent_group)
	pass

    ## \brief Handle event that user change layers.
    #
    # This method is always being called for chaning layer event.
    # So, we should do some check at beginning for re-entry condition.
    #
    def handle_change_layer(self, node):
	layer = self.desktop.currentLayer()
	node = layer.repr

	#
	# Only scene group and dup group are allowed.
	#
	try:
	    scene_group_attr = node.getAttribute('scene_group')
	except KeyError:
	    pass
	else:
	    if scene_group_attr == 'true':
		return
	    pass

	try:
	    label = node.getAttribute('inkscape:label')
	except KeyError:
	    pass
	else:
	    if label == 'dup':
		return
	    pass
	# It is not a scene or dup group.

	layer_idx, frame_idx = self._domviewui.get_current_frame()
	self.selectSceneObject(layer_idx, frame_idx)
	pass

    ## \brief Add menu item to context menu.
    #
    # This method is called by pyink.pyink_context_menu() to notify the
    # creation of context menu for a node.
    #
    def context_menu(self, spitem, menu_factory):
	node = spitem.repr
	if node.name() != 'svg:g':
	    return		# not a group

	if self._domviewui.is_graph_node(node):
	    menu_item_handler = \
		lambda *args: self.do_make_component_from_group(node)
	    menu_factory.add_item_label('Make a component',
					menu_item_handler)
	    pass
	pass

    def show(self):
	self.do_show()
	pass
    pass
