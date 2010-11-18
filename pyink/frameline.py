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

## Show frame status of a layer
#
# \section frameline_sigs Signals
# - 'frame-button-pree' for user press on a frame.
#   - callback(widget, frame_idx, button)
#
class frameline(gtk.DrawingArea):
    _type = 0
    _frame_width = 10           # Width for each frame is 10 pixels
    _select_color = 0xee2222    # color of border of selected frame
    _key_mark_color = 0x000000  # color of marks for key frames.
    _key_mark_sz = 4            # width and height of a key frame mark
    _tween_color = 0x808080     # color of tween line
    _tween_bgcolors = [0x80ff80, 0xff8080] # bg colors of tween frames
    # Colors for normal frames
    _normal_bgcolors = [0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xcccccc]
    _normal_border = 0xaaaaaa   # border color of normal frames.
    _active_border = 0xff3030   # border color of an active frame
    _hover_border_color = 0xa0a0a0 # border when the pointer over a frame

    FRAME_BUT_PRESS = 'frame-button-press'
    
    def __new__(clz, *args):
        if not frameline._type:
            frameline._type = gobject.type_register(frameline)
            but_press = gobject.signal_new(frameline.FRAME_BUT_PRESS,
                                           frameline._type,
                                           gobject.SIGNAL_RUN_FIRST,
                                           gobject.TYPE_NONE,
                                           (gobject.TYPE_INT,
                                            gobject.TYPE_INT))
            frameline._sig_frame_but_press = but_press
            pass
        fl_obj = gobject.new(frameline._type)
        return fl_obj
    
    def __init__(self, num_frames=20):
        self.connect('button-press-event', self._press_hdl)
        self.connect('expose-event', self._fl_expose)
        self.connect('motion-notify-event', self._motion_hdl)
        self._num_frames = num_frames
        self._keys = []
        self._active_frame = -1
        self._last_hover = -1   # frame index of last hover
        self._drawing = False
        pass

    def _press_hdl(self, widget, event):
        frame = event.x / self._frame_width
        but = event.button
        self.emit(frameline.FRAME_BUT_PRESS, frame, but)
        pass
    
    def _motion_hdl(self, widget, event):
        frame = int(event.x / self._frame_width)
        self._draw_hover(frame)
        pass

    def _fl_expose(self, widget, event):
        win = self.window
        x, y, w, h, depth = win.get_geometry()
        if not hasattr(self, '_gc'):
            self._gc = gtk.gdk.GC(win)
            #
            # register for button press event
            #
            emask = win.get_events()
            emask = emask | gtk.gdk.BUTTON_PRESS_MASK | \
                gtk.gdk.POINTER_MOTION_MASK
            win.set_events(emask)
            self._drawing = True
            pass
        self.update()
        pass

    def _draw_tween(self, first_key, last_key):
        if not self._drawing:
            return
        
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        
        #
        # Get background color of a tween
        #
        bg_idx = first_key.right_tween_type
        bg_color_v = self._tween_bgcolors[bg_idx]
        bg_color_rgb = color_to_rgb(bg_color_v)
        bg_color = gtk.gdk.Color(*bg_color_rgb)
        
        gc = self._gc
        gc.set_rgb_fg_color(bg_color)

        draw_x = first_key.idx * self._frame_width + 1
        draw_w = (last_key.idx -  first_key.idx + 1) * self._frame_width - 1

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
        line_x1 = int((first_key.idx + 0.5) * self._frame_width)
        line_x2 = line_x1 + (last_key.idx - first_key.idx) * self._frame_width
        line_y = int(w_h * 2 / 3)
        win.draw_line(gc, line_x1, line_y, line_x2, line_y)
        pass

    def _draw_normal_frame(self, idx):
        if not self._drawing:
            return
        
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
    def _draw_bottom_line(self, start, stop):
        if not self._drawing:
            return
        
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        gc = self._gc
        
        border_rgb = color_to_rgb(self._normal_border)
        border_color = gtk.gdk.Color(*border_rgb)
        gc.set_rgb_fg_color(border_color)
        start_x = start * self._frame_width
        stop_x = stop * self._frame_width
        win.draw_line(gc, start_x, w_h - 1, stop_x, w_h - 1)
        pass
    
    def _draw_all_frames(self):
        if not self._drawing:
            return
        
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        gc = self._gc
        
        i = 0
        key_i = 0
        try:
            key = self._keys[key_i]
        except IndexError:
            key = keyframe(self._num_frames)
            pass
        num_frames = self._num_frames
        while i < num_frames:
            if key.idx == i:
                #
                # Skip tween keys
                #
                first_tween_key = key
                while key.idx == i or key.left_tween:
                    last_tween_key = key
                    key_i = key_i + 1
                    try:
                        key = self._keys[key_i]
                    except IndexError:
                        key = keyframe(self._num_frames)
                        pass
                    pass

                if first_tween_key != last_tween_key:
                    self._draw_tween(first_tween_key, last_tween_key)
                
                i = last_tween_key.idx + 1
                pass
            else:
                self._draw_normal_frame(i)
                i = i + 1
                pass
            pass

        self._draw_bottom_line(0, num_frames)
        pass

    def _draw_keyframe(self, frame_idx):
        if not self._drawing:
            return
        
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

    def _draw_keyframes(self):
        if not self._drawing:
            return
        
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        
        color_v = self._key_mark_color
        color_rgb = color_to_rgb(color_v)
        color = gtk.gdk.Color(*color_rgb)
        
        gc = self._gc
        gc.set_rgb_fg_color(color)
        
        for key in self._keys:
            mark_sz = self._key_mark_sz
            mark_x = int((key.idx + 0.5) * self._frame_width - mark_sz / 2)
            mark_y = w_h * 2 / 3 - mark_sz / 2

            win.draw_rectangle(gc, True, mark_x, mark_y, mark_sz, mark_sz)
            pass
        pass

    def _draw_active(self):
        if not self._drawing:
            return
        
        if self._active_frame == -1:
            return
        
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()

        color_v = self._active_border
        color_rgb = color_to_rgb(color_v)
        color = gtk.gdk.Color(*color_rgb)

        gc = self._gc
        gc.set_rgb_fg_color(color)
        
        idx = self._active_frame
        line_x1 = idx * self._frame_width
        line_x2 = line_x1 + self._frame_width

        win.draw_line(gc, line_x1, 0, line_x1, w_h)
        win.draw_line(gc, line_x2, 0, line_x2, w_h)
        win.draw_line(gc, line_x1, w_h - 1, line_x2, w_h - 1)
        win.draw_line(gc, line_x1, 0, line_x2, 0)
        pass

    ## \brief Find the range a continous tween.
    #
    def _find_tween_range(self, key_pos):
        first_pos = key_pos
        while first_pos and self._keys[first_pos].left_tween:
            first_pos = first_pos - 1
            pass
        
        max_pos = len(self._keys) - 1
        
        last_pos = key_pos
        while last_pos < max_pos and self._keys[last_pos].right_tween:
            last_pos = last_pos + 1
            pass
        
        return first_pox, last_pos

    ## \brief Redraw a frame specified by an index.
    #
    def _redraw_frame(self, frame_idx):
        if not self._drawing:
            return
        
        keys = [key.idx for key in self._keys]
        if len(keys):
            try:
                pos = keys.index(frame_idx)
            except ValueError:
                keys.append(frame_idx)
                keys.sort()
                pos = keys.index(frame_idx) - 1
                pass
            if pos < 0:
                pos = 0
                pass
            key = self._keys[pos]
        else:
            key = None
            pass
        
        if key and (key.right_tween or \
                (key.left_tween and key.idx == frame_idx)):
            #
            # in tween
            #
            first_pos, last_pos = self._find_tween_range(pos)
            first_key = self._keys[first_pos]
            last_key = self._keys[last_pos]
            
            self._draw_tween(first_key, last_key)
            self._draw_bottom_line(first_key.idx, last_key.idx + 1)

            for i in range(first_pos, last_pos + 1):
                key = self._keys[i]
                self._draw_keyframe(key.idx)
                pass
            pass
        else:                   # not in tween
            self._draw_normal_frame(frame_idx)
            self._draw_bottom_line(frame_idx, frame_idx + 1)
            if key and (key.idx == frame_idx):
                self._draw_keyframe(frame_idx)
                pass
            pass
        pass

    ## \brief Show a mark for the pointer for a frame.
    #
    def _draw_hover(self, frame_idx):
        if not self._drawing:
            return
        
        if self._last_hover != -1:
            self._redraw_frame(self._last_hover)
            pass

        self._draw_active()
        
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

        self._last_hover = frame_idx
        pass
    
    def update(self):
        if not self._drawing:
            return
        
        win = self.window
        x, y, w, h, depth = win.get_geometry()
        
        self._draw_all_frames()
        self._draw_keyframes()
        if self._active_frame != -1:
            self._draw_active()
            pass
        pass

    ## Add a key frame
    #
    # A key frame is the frame that user specify actions.  For
    # example, move a object or add new objects at the frame.
    def add_keyframe(self, idx):
        key_indic = [key.idx for key in self._keys]
        if idx in key_indic:
            return

        key_indic.append(idx)
        key_indic.sort()
        insert_pos = key_indic.index(idx)
        
        key = keyframe(idx)
        self._keys[insert_pos:insert_pos] = [key]
        if insert_pos > 0 and self._keys[insert_pos - 1].right_tween:
            key.left_tween = True
            pass
        if insert_pos < (len(self._keys) - 1) and \
                self._keys[insert_pos + 1].left_tween:
            key.right_tween = True
            pass

        self._draw_keyframe(idx)
        pass

    def rm_keyframe(self, idx):
        key = self._keys[idx]
        del self._keys[idx]
        
        if key.right_tween ^ key.left_tween:
            #
            # tween in one side
            #
            if key.right_tween:
                right_key = self._keys[idx]
                right_key.left_tween = False
                rdraw_range = (right_key.idx, idx + 1)
            else:
                left_key = self._keys[idx - 1]
                left_key.right_key = False
                redraw_range = (idx, left_key.idx + 1)
                pass
            
            for i in range(*redraw_range):
                self._redraw_frame(i)
                pass
        else:
            self._redraw_frame(idx)
            pass

        self._draw_active()
        pass

    ## Tween the key frame specified by an index and the key frame at right.
    #
    # \see http://www.entheosweb.com/Flash/shape_tween.asp
    def tween(self, idx, _type=0):
        key_indic = [key.idx for key in self._keys]
        pos = key_indic.index(idx)
        key = self._keys[pos]
        
        try:
            right_key = self._keys[pos + 1]
        except IndexError:
            raise ValueError, 'No right key frame'

        key.right_tween = True
        right_key.left_tween = True
        key.right_tween_type = _type
        pass

    ## Set active frame
    #
    # The active frame is the frame that is working on.
    #
    def active_frame(self, idx):
        if idx < 0 or idx >= self._num_frames:
            raise IndexError, 'value of index (%d) is out of range' % (idx)

        if self._active_frame != -1:
            self._redraw_frame(self._active_frame)
            pass
        self._active_frame = idx
        self._draw_active()
        pass

    def deactive(self):
        self._redraw_frame(self._active_frame)
        self._active_frame = -1
        pass

    def set_num_frames(self, num):
        self._num_frames = num
        pass

    def reset(self):
        self._keys = []
        pass

    ## \brief Start future drawing actions
    #
    def start_drawing(self):
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

    def __len__(self):
        return self._num_frames
    pass

if __name__ == '__main__':
    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    fr = frameruler(40)
    fr.set_size_request(300, 20)
    
    fl = frameline(40)
    fl.set_size_request(300, 20)
    fl.add_keyframe(3)
    fl.add_keyframe(9)
    fl.add_keyframe(15)
    fl.add_keyframe(20)
    fl.tween(3)
    fl.tween(15, 1)
    fl.active_frame(15)
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
