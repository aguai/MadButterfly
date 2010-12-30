# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; fill-column: 79 -*-
# vim: sw=4:ts=8:sts=4:textwidth=79
import pygtk
pygtk.require("2.0")
import gtk
import gtk.gdk
import pango
import gobject

def color_to_rgb(v):
    return (((v >> 16) & 0xff) * 65535 / 0xff,
            ((v >> 8) & 0xff) * 65535 / 0xff,
            (v & 0xff) * 65535 / 0xff)

class keyframe(object):
    def __init__(self, frame_idx):
        self.idx = frame_idx
        self.left_tween = False
        self.right_tween = False
        self.right_tween_type = 0
	self.ref = ''
        pass
    pass

class frameruler(gtk.DrawingArea):
    _type = 0
    _frame_width = 10           # Width for each frame is 10 pixels
    _mark_color = 0x808080      # color of mark lines
    _number_color = 0x000000    # color of frame number
    _number_sz = 8             # font size of frame number
    
    def __new__(clz, *args):
        if not frameruler._type:
            frameruler._type = gobject.type_register(frameruler)
            pass
        fr = gobject.new(frameruler._type)
        return fr

    def __init__(self, num_frames=20):
        self.connect('expose_event', self._fr_expose)
        self._num_frames = num_frames
        pass

    def _fr_expose(self, widget, event):
        self.update()
        pass

    def queue_draw(self):
        print 'queue_draw'
        self.update()
        pass

    def queue_draw_area(self, x, y, w, h):
        print 'queue_draw_area'
        pass

    def update(self):
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()

        gc = gtk.gdk.GC(win)

        #
        # Set color of mark lines
        #
        color_rgb = color_to_rgb(self._mark_color)
        color = gtk.gdk.Color(*color_rgb)
        gc.set_rgb_fg_color(color)
        
        #
        # Mark mark lines
        #
        mark_h = w_h / 10
        for i in range(self._num_frames):
            mark_x = (i + 1) * self._frame_width
            win.draw_line(gc, mark_x, 0, mark_x, mark_h)
            win.draw_line(gc, mark_x, w_h - mark_h - 1, mark_x, w_h - 1)
            pass

        win.draw_line(gc, 0, w_h - 1, w_w, w_h -1)

        #
        # Set color of frame number
        #
        color_rgb = color_to_rgb(self._number_color)
        color = gtk.gdk.Color(*color_rgb)
        gc.set_rgb_fg_color(color)
        
        font_desc = pango.FontDescription()
        font_desc.set_size(self._number_sz * pango.SCALE)

        number_y = (w_h - self._number_sz) / 2
        
        #
        # Draw frame number
        #
        layout = self.create_pango_layout('1')
        layout.set_font_description(font_desc)
        win.draw_layout(gc, 0, number_y, layout)
        for i in range(4, self._num_frames, 5):
            mark_x = i * self._frame_width
            layout.set_text(str(i + 1))
            win.draw_layout(gc, mark_x, number_y, layout)
            pass
        pass
    pass

## \brief Drawing on the screen for a frameline.
#
# This class contain all functions that drawing thing on the screen for a
# frameline.  It is used by descendants to drawing a frameline.  This class is
# only responsible to draw screen, the logical part is the responsible of
# deriving classes.
#
# This class should only include functions about drawing, no any control, logic
# function, and state should be implemented here.  This class should change/or
# read states of instances.  It is stateless.
#
class frameline_draw(gtk.DrawingArea):
    _type = 0
    _frame_width = 10           # Width for each frame is 10 pixels
    _select_color = 0xee2222    # color of border of selected frame
    _key_mark_color = 0x000000  # color of marks for key frames.
    _key_mark_sz = 4            # width and height of a key frame mark
    _tween_color = 0x808080     # color of tween line
    # bg colors of tween frames
    _tween_bgcolors = [0x80ff80, 0xff8080, 0xffff80]
    # Colors for normal frames
    _normal_bgcolors = [0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xcccccc]
    _normal_border = 0xaaaaaa   # border color of normal frames.
    _active_border = 0xff3030   # border color of an active frame
    _hover_border_color = 0xa0a0a0 # border when the pointer over a frame
    
    def __new__(clz, *args):
        if not clz._type:
            clz._type = gobject.type_register(clz)
            pass
        fl_obj = gobject.new(clz._type)
        return fl_obj
    
    def _draw_tween(self, first_idx, last_idx, tween_type):
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        
        #
        # Get background color of a tween
        #
        bg_idx = tween_type
        bg_color_v = self._tween_bgcolors[bg_idx]
        bg_color_rgb = color_to_rgb(bg_color_v)
        bg_color = gtk.gdk.Color(*bg_color_rgb)
        
        gc = self._gc
	gc.set_rgb_fg_color(bg_color)
	
        draw_x = first_idx * self._frame_width + 1
        draw_w = (last_idx -  first_idx + 1) * self._frame_width - 1

        win.draw_rectangle(gc, True, draw_x, 0, draw_w, w_h)

        #
        # Set color of tween line
        #
        line_v = self._tween_color
        line_rgb = color_to_rgb(line_v)
        line_color = gtk.gdk.Color(*line_rgb)
        gc.set_rgb_fg_color(line_color)
        
        #
        # Draw tween line
        #
        line_x1 = int((first_idx + 0.5) * self._frame_width)
        line_x2 = line_x1 + (last_idx - first_idx) * self._frame_width
        line_y = int(w_h * 2 / 3)
        win.draw_line(gc, line_x1, line_y, line_x2, line_y)
        pass

    def _draw_normal_frame(self, idx):
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        
        gc = self._gc
        bg_idx = idx % len(self._normal_bgcolors)
        rgb = color_to_rgb(self._normal_bgcolors[bg_idx])
        color = gtk.gdk.Color(*rgb)
        gc.set_rgb_fg_color(color)
        
        f_x = self._frame_width * idx
        win.draw_rectangle(gc, True, f_x + 1, 0, self._frame_width - 1, w_h)
        next_f_x = f_x + self._frame_width
        
        border_rgb = color_to_rgb(self._normal_border)
        border_color = gtk.gdk.Color(*border_rgb)
        gc.set_rgb_fg_color(border_color)
        gc.set_line_attributes(1, gtk.gdk.LINE_SOLID,
                               gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER)
        win.draw_line(gc, next_f_x, 0, next_f_x, w_h)
        pass

    ## \brief Draw a bottom line from start to the point before stop frame.
    #
    def _draw_bottom_line(self, start_idx, stop_idx):
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        gc = self._gc
        
        border_rgb = color_to_rgb(self._normal_border)
        border_color = gtk.gdk.Color(*border_rgb)
        gc.set_rgb_fg_color(border_color)
        start_x = start_idx * self._frame_width
        stop_x = stop_idx * self._frame_width
        win.draw_line(gc, start_x, w_h - 1, stop_x, w_h - 1)
        pass
    
    def _draw_active(self, idx):
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()

        color_v = self._active_border
        color_rgb = color_to_rgb(color_v)
        color = gtk.gdk.Color(*color_rgb)

        gc = self._gc
        gc.set_rgb_fg_color(color)
        
        line_x1 = idx * self._frame_width
        line_x2 = line_x1 + self._frame_width

        win.draw_line(gc, line_x1, 0, line_x1, w_h)
        win.draw_line(gc, line_x2, 0, line_x2, w_h)
        win.draw_line(gc, line_x1, w_h - 1, line_x2, w_h - 1)
        win.draw_line(gc, line_x1, 0, line_x2, 0)
	pass

    def _draw_keyframe_(self, frame_idx):
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        
        color_v = self._key_mark_color
        color_rgb = color_to_rgb(color_v)
        color = gtk.gdk.Color(*color_rgb)
        
        gc = self._gc
        gc.set_rgb_fg_color(color)
        
        mark_sz = self._key_mark_sz
        mark_x = int((frame_idx + 0.5) * self._frame_width - mark_sz / 2)
        mark_y = w_h * 2 / 3 - mark_sz / 2
        
        win.draw_rectangle(gc, True, mark_x, mark_y, mark_sz, mark_sz)
        pass

    ## \brief Show a mark for the pointer for a frame.
    #
    def _draw_hover(self, frame_idx):
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        gc = self._gc
        
        color_rgb = color_to_rgb(self._hover_border_color)
        color = gtk.gdk.Color(*color_rgb)
        gc.set_rgb_fg_color(color)

        line_x1 = frame_idx * self._frame_width + 1
        line_x2 = line_x1 + self._frame_width - 2
        
        win.draw_line(gc, line_x1, 1, line_x1, w_h - 2)
        win.draw_line(gc, line_x2, 1, line_x2, w_h - 2)
        win.draw_line(gc, line_x1, 1, line_x2, 1)
        win.draw_line(gc, line_x1, w_h - 2, line_x2, w_h - 2)
        pass
    pass


## \brief Drawing frameline according state of a frameline.
#
# Thsi class calls methods of class frameline_draw to drawing frameline.  But,
# this class is state awared.  It according states of an instance to determines
# calling methods and arguments.  This class decorates methods of
# frameline_draw class according states of an instance.
#
# This classs reading state of a frameline, but it does not change and control
# states.  The deriving classes are responsible to change and control states.
#
class frameline_draw_state(frameline_draw):
    # tween types
    TWEEN_TYPE_NONE = 0
    TWEEN_TYPE_MOVE = 1
    TWEEN_TYPE_SHAPE = 2    

    def __init__(self, num_frames):
	frameline_draw.__init__(self)
	
        self._num_frames = num_frames
        self._keys = []
        self._active_frame = -1
        self._drawing = False
	pass

    def _draw_keyframe(self, frame_idx):
	# Only keyframes that is not right-side of NONE type tween should be
	# draw.
	pos = self._find_keyframe(frame_idx)
	key = self._keys[pos]
	if key.left_tween and not key.right_tween:
	    return
	
	self._draw_keyframe_(frame_idx)
	pass

    def _draw_keyframes(self):
        for key in self._keys:
	    self._draw_keyframe(key.idx)
            pass
        pass

    ## \brief Redraw a frame specified by an index.
    #
    def _draw_frame(self, frame_idx):
        if not self._drawing:
            return

	pos = self._find_keyframe_floor(frame_idx)
	try:
	    key = self._keys[pos]
	except IndexError:
	    key = None
	    pass
	
	if key and (key.right_tween or
		    (key.left_tween and key.idx == frame_idx)):
	    #
            # in tween
            #
            first_pos, last_pos = self._find_tween_range(pos)
            first_key = self._keys[first_pos]
            last_key = self._keys[last_pos]
            
            self._draw_tween_of_key(first_pos)
        else:                   # not in tween
            self._draw_normal_frame(frame_idx)
            self._draw_bottom_line(frame_idx, frame_idx + 1)
            if key and (key.idx == frame_idx):
                self._draw_keyframe(frame_idx)
                pass
            pass
        pass
    
    def _draw_all_frames(self):
        if not self._drawing:
            return
        
        i = 0
        key_pos = 0
        try:
            key = self._keys[key_pos]
        except IndexError:
            key = keyframe(self._num_frames)
            pass
        num_frames = self._num_frames
        while i < num_frames:
            if key.idx == i and key.right_tween:
                #
                # Skip tween keys
                #
		first_tween_pos, last_tween_pos = \
		    self._find_tween_range(key_pos)
		first_tween_key = self._keys[first_tween_pos]
		last_tween_key = self._keys[last_tween_pos]
		self._draw_tween(first_tween_key.idx, last_tween_key.idx,
				 first_tween_key.right_tween_type)
		last_tween_key = self._keys[last_tween_pos]
		key_pos = last_tween_pos + 1
		try:
		    key = self._keys[key_pos]
		except:
		    key = keyframe(self._num_frames)
		    pass
                i = last_tween_key.idx + 1
	    else:
                self._draw_normal_frame(i)
	        if key.idx == i:
		    key_pos = key_pos+1
		    try:
		        key = self._keys[key_pos]
		    except:
		        key = keyframe(self._num_frames)
			pass
		    pass
                i = i + 1
                pass
            pass

        self._draw_bottom_line(0, num_frames)

	self._draw_keyframes()
        pass

    def _draw_tween_of_key(self, key_pos):
        if not self._drawing:
            return

	first_pos, last_pos = self._find_tween_range(key_pos)
	first_key = self._keys[first_pos]
	last_key = self._keys[last_pos]
	
	self._draw_tween(first_key.idx, last_key.idx,
			 first_key.right_tween_type)
	self._draw_bottom_line(first_key.idx, last_key.idx + 1)

	for i in range(first_pos, last_pos + 1):
	    key = self._keys[i]
	    self._draw_keyframe(key.idx)
            pass
	pass
    
    def _draw_active_frame(self):
        if not self._drawing:
            return
        
        if self._active_frame == -1:
            return

	self._draw_active(self._active_frame)
        pass

    def _draw_hover_frame(self, frame_idx):
	if not self._drawing:
	    return
	self._draw_hover(frame_idx)
	pass
    
    ## \brief Start future drawing actions
    #
    def start_drawing(self):
        if not hasattr(self, '_gc'):
	    win = self.window
            self._gc = gtk.gdk.GC(win)
            #
            # register for button press event
            #
            emask = win.get_events()
            emask = emask | gtk.gdk.BUTTON_PRESS_MASK | \
                gtk.gdk.POINTER_MOTION_MASK
            win.set_events(emask)
            pass
        self._drawing = True
        pass
    
    ## \brief Stop any future drawing actions
    #
    # When doing massive udpate, to stop drawing the screen make
    # application more effecient.  The screen is updated by calling
    # update() method after massive update and calliing start_drawing().
    #
    def stop_drawing(self):
        self._drawing = False
        pass
    pass


## Show frame status of a layer
#
# \section frameline_sigs Signals
# - 'frame-button-pree' for user press on a frame.
#   - callback(widget, frame_idx, button)
#
# All methos that change state of the frameline, must call methods to update
# the screen.
#
class frameline(frameline_draw_state):
    _sig_frame_but_press = None
    
    FRAME_BUT_PRESS = 'frame-button-press'
    
    def __new__(clz, *args):
	fl_obj = frameline_draw_state.__new__(clz, *args)
	
        if not clz._sig_frame_but_press:
            but_press = gobject.signal_new(frameline.FRAME_BUT_PRESS,
                                           frameline._type,
                                           gobject.SIGNAL_RUN_FIRST,
                                           gobject.TYPE_NONE,
                                           (gobject.TYPE_INT,
                                            gobject.TYPE_INT))
            clz._sig_frame_but_press = but_press
            pass
        return fl_obj
    
    def __init__(self, num_frames=20):
	frameline_draw_state.__init__(self, num_frames)
	
        self.connect('button-press-event', self._press_hdl)
        self.connect('expose-event', self._fl_expose)
        self.connect('motion-notify-event', self._motion_hdl)
        self._last_hover = -1   # frame index of last hover
        pass

    def __len__(self):
        return self._num_frames
    
    def _find_keyframe(self, idx):
	key_indic = [key.idx for key in self._keys]
	key_pos = key_indic.index(idx)
	return key_pos

    def _find_keyframe_floor(self, frame_idx):
	pos = 0
        keys = [key.idx for key in self._keys]
	keys.append(frame_idx)
	keys.sort()
	keys.reverse()
	pos = (len(keys) - 1) - keys.index(frame_idx) - 1
	return pos

    ## \brief Find the range a continous tween.
    #
    def _find_tween_range(self, key_pos):
	key = self._keys[key_pos]
	if not (key.left_tween or key.right_tween):
	    raise ValueError, 'the keyframe is not in a tween'
	
	#
	# Initialize tween type and first_pos
	#
	if key.right_tween:
	    tween_type = key.right_tween_type
	    first_pos = key_pos
	else:
	    # key.left_tween is True since the key is in a tween.
	    first_pos = key_pos -1
	    key = self._keys[first_pos]
	    tween_type = key.right_tween_type
	    pass
	
	#
	# Find first_pos
	#
        while first_pos and key.left_tween:
	    right_pos = first_pos - 1
	    right_key = self._keys[right_pos]
	    if right_key.right_tween_type != tween_type:
		break
            first_pos = right_pos
	    key = right_key
            pass
        
	#
	# Find last_pos
	#
        max_pos = len(self._keys) - 1
        last_pos = key_pos
	key = self._keys[last_pos]
        while last_pos < max_pos and self._keys[last_pos].right_tween:
	    if key.right_tween_type != tween_type:
		break
            last_pos = last_pos + 1
	    key = self._keys[last_pos]
            pass
        
        return first_pos, last_pos

    def _press_hdl(self, widget, event):
        frame = event.x / self._frame_width
        but = event.button
        self.emit(frameline.FRAME_BUT_PRESS, frame, but)
        pass
    
    def hide_hover(self):
        if self._active_frame != self._last_hover:
            self._draw_normal_frame(self._last_hover)
	    pass
	pass
    
    def _motion_hdl(self, widget, event):
        frame_idx = int(event.x / self._frame_width)
        if self._last_hover != -1:
            self._draw_frame(self._last_hover)
	    if self._last_hover == self._active_frame:
		self._draw_active_frame()
		pass
            pass
	
        if frame_idx < self._num_frames and frame_idx >= 0:
            self._draw_hover_frame(frame_idx)
	    if self._last_hover == self._active_frame:
		self._draw_active_frame()
		pass
	    self._last_hover = frame_idx
	else:
	    self._last_hover = -1
	    pass
        pass

    def _fl_expose(self, widget, event):
        win = self.window
	self.start_drawing()
        self.update()
        pass

    def set_tween_type(self, frame_idx, tween_type):
	pos = self._find_keyframe(frame_idx)
	key = self._keys[pos]
	assert key.right_tween

	key.right_tween_type = tween_type
	self._draw_tween_of_key(pos)

	self._draw_active_frame()
	pass

    def update(self):
        self._draw_all_frames()
	self._draw_active_frame()
        pass

    ## Add a key frame
    #
    # A key frame is the frame that user specify actions.  For
    # example, move a object or add new objects at the frame.
    def add_keyframe(self, idx, ref=None):
	try:
	    pos = self._find_keyframe(idx) # it is not already a keyframe.
	except ValueError:
	    pass
	else:
	    raise ValueError, 'the frame is already a key frame'
	
	key_indic = [key.idx for key in self._keys]
        key_indic.append(idx)
        key_indic.sort()
        insert_pos = key_indic.index(idx)
        
        key = keyframe(idx)
	key.ref = ref
        if insert_pos > 0 and self._keys[insert_pos - 1].right_tween:
	    if self._keys[insert_pos-1].idx == idx-1:
		self._keys[insert_pos-1].right_tween = False
                self._keys[insert_pos:insert_pos] = [key]
		return
	    else:
		key2 = keyframe(idx-1)
		key2.ref = self._keys[insert_pos-1].ref
		key2.left_tween = True
		self._keys[insert_pos:insert_pos] = [key2,key]
		key.left_tween = False
		key.right_tween = True
		key.right_tween_type = self._keys[insert_pos - 1].right_tween_type
            pass
	else:
            self._keys[insert_pos:insert_pos] = [key]

	if self._drawing:
	    self._draw_keyframe(idx)
	    pass
        pass

    ## Set the frame @idx as the right of a tween
    def set_right_tween(self,idx):
	pos = self._find_keyframe(idx)
	self._keys[pos].right_tween = TRue

    def remove_frame(self, idx):
	pos = self._find_keyframe_floor(idx)
	if pos != -1:
	    key = self._keys[pos]
	    if key.idx == idx:
		if key.left_tween:
		    self._keys[pos-1].right_tween = False
    		    del self._keys[pos]
		    while pos < len(self._keys):
			self._keys[pos].idx = self._keys[pos].idx - 1
			pos = pos+1
		    self.update()
		# Use remove key frame to remove the key frame
		return
	    elif key.right_tween:
		pos = pos + 1
		while pos < len(self._keys):
		    self._keys[pos].idx = self._keys[pos].idx - 1
		    pos = pos + 1
		if self._drawing:
		    self.update()
	    else:
		return 
	    pass
	pass

    def insert_frame(self,idx):
	pos = self._find_keyframe_floor(idx)
	if pos != -1:
	    key = self._keys[pos]
	    if key.idx == idx:
		while pos < len(self._keys):
		    self._keys[pos].idx = self._keys[pos].idx + 1
		    pos = pos + 1
		if self._drawing:
		    self.update()
		return
	    elif key.right_tween:
		pos = pos + 1
		while pos < len(self._keys):
		    self._keys[pos].idx = self._keys[pos].idx + 1
		    pos = pos + 1
		if self._drawing:
		    self.update()
	    else:
		return 
	    pass
	pass
        pass
    def rm_keyframe(self, idx):
	key_pos = self._find_keyframe(idx)
        key = self._keys[key_pos]
	del self._keys[key_pos]
        
        if key.right_tween ^ key.left_tween:
            #
            # tween in one side
            #
            if key.right_tween:
		right_key = self._keys[key_pos]
                right_key.left_tween = False
                redraw_range = (right_key.idx, idx + 1)
            else:
                left_key = self._keys[key_pos - 1]
                left_key.right_tween = False
                redraw_range = (idx, left_key.idx + 1)
                pass
                self._draw_frame(i)
                pass
        else:
            self._draw_frame(idx)
            pass

        self._draw_active_frame()
        pass

    ## Tween the key frame specified by an index and the key frame at right.
    #
    # \see http://www.entheosweb.com/Flash/shape_tween.asp
    def tween(self, idx, tween_type=frameline_draw_state.TWEEN_TYPE_NONE):
	pos = self._find_keyframe(idx)
        key = self._keys[pos]
        
        try:
            right_key = self._keys[pos + 1]
        except IndexError:
            raise ValueError, 'no right key frame'

        key.right_tween = True
        right_key.left_tween = True
	key.right_tween_type = tween_type
	
	self._draw_tween_of_key(pos)
	self._draw_active_frame()
        pass
    
    def get_tween_type(self, idx):
        for i in range(0,len(self._keys)):
	    key = self._keys[i]
	    if key.left_tween is True: continue
	    if key.idx == idx:
		return key.right_tween_type
	    pass
	pass

    def get_frame_blocks(self):
	blocks = []
	for pos, key in enumerate(self._keys):
	    if key.right_tween:
		next_key = self._keys[pos + 1]
		block = (key.idx, next_key.idx, key.right_tween_type)
	    elif not key.left_tween:
		block = (key.idx, key.idx, 0)
	    else:
		continue
	    blocks.append(block)
	    pass
	return blocks

    def get_frame_block(self, idx):
	pos = self._find_keyframe_floor(idx)
	if pos != -1:
	    key = self._keys[pos]
	    print key.right_tween, key.left_tween
	    if key.idx == idx:
		return key.idx, key.idx, 0
	    elif key.right_tween:
		next_key = self._keys[pos + 1]
		return key.idx, next_key.idx, key.right_tween_type
	    else:
		return -1,-1,-1
	    pass
	raise ValueError, \
	    'the frame specified by idx is not in any tween or a key frame'
    
    def get_frame_data(self, idx):
	pos = self._find_keyframe(idx)
	key = self._keys[pos]
	return key.ref
    
    ## Set active frame
    #
    # The active frame is the frame that is working on.
    #
    def active_frame(self, idx):
        if idx < 0 or idx >= self._num_frames:
            raise IndexError, 'value of index (%d) is out of range' % (idx)

        if self._active_frame != -1:
            self._draw_frame(self._active_frame)
            pass
        self._active_frame = idx
        self._draw_active_frame()
        pass

    def deactive(self):
        self._draw_frame(self._active_frame)
        self._active_frame = -1
        pass

    def set_num_frames(self, num):
        self._num_frames = num
        pass

    def reset(self):
        self._keys = []
	self._active_frame = -1
	self._draw_all_frames()
        pass
    pass

if __name__ == '__main__':
    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    fr = frameruler(40)
    fr.set_size_request(300, 20)
    
    fl = frameline(40)
    fl.set_size_request(300, 20)
    fl.add_keyframe(15)
    fl.add_keyframe(3)
    fl.tween(3)
    fl.add_keyframe(9)
    fl.add_keyframe(20)
    fl.tween(9)
    fl.active_frame(1)
    fl.rm_keyframe(15)
    print 'num of frames: %d' % (len(fl))

    def press_sig(fl, frame, but):
        print 'press_sig button %d for frame %d' % (but, frame)
        pass
    fl.connect(frameline.FRAME_BUT_PRESS, press_sig)

    box = gtk.VBox()

    box.pack_start(fr, False)
    box.pack_start(fl, False)
    window.add(box)
    
    fr.show()
    fl.show()
    box.show()
    window.show()
    gtk.main()
    pass
