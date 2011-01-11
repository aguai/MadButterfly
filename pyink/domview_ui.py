import gtk
from tween import TweenObject
from frameline import frameline, frameruler
from domview import domview


## \brief Maintain a stack of frameline UI component.
#
# Every layer is assocated with a frameline.  Framelines are showed/stacked in
# virtical.  Framelines of lower layers are placed at lower position on the
# screen.  This class provide a set of API to access framelines with layer and
# frame index number.  You access/set content of frameline by specifing layer
# index and frame index.
#
class frameline_stack(object):
    _frameline_tween_types = (frameline.TWEEN_TYPE_NONE,
			      frameline.TWEEN_TYPE_SHAPE)
    _num_frames_of_line = 100
    
    _framelines = None
    
    def __init__(self, *args, **kws):
	super(frameline_stack, self).__init__(*args, **kws)
	
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
class domview_ui(object):
    _tween_type_names = ('normal', 'scale')
    
    def __init__(self):
	super(domview_ui, self).__init__()
	self._fl_stack = frameline_stack()
	self._dom = domview()
        self._doc = None
        self._root = None
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

        self._doc = doc
        self._root = root
	pass

    ## \brief Mark given frame as a key frame.
    #
    def mark_key(self, layer_idx, key_idx):
	scene_group = self._dom.add_scene_group(layer_idx)
	scene_group_id = scene_group.getAttribute('id')
	
	scene_node = self._dom.add_scene_node(layer_idx, key_idx, key_idx)
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
	self._fl_stack.rm_frames(layer_idx, frame_idx, num)
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

    ## \brief Copy content of a source key frame to a destinate.
    #
    # Copy content of the scene group of a source key frame to the
    # scene group of a destinate key frame.
    #
    def copy_key_group(self, layer_idx, src_frame_idx, dst_frame_idx):
        src_group = self.get_key_group(layer_idx, src_frame_idx)
        dst_group = self.get_key_group(layer_idx, dst_frame_idx)
        self._dom.copy_group_children(src_group, dst_group)
        pass

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

    @property
    def doc(self):
        return self._doc

    @property
    def root(self):
        return self._root

    def lock(self):
        return True

    def unlock(self):
        return True
    pass

