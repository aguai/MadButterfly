import pygtk
pygtk.require("2.0")
import gtk
import gtk.gdk
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

## Show frame status of a layer
#
class frameline(gtk.DrawingArea):
    _type_id = 0
    _frame_width = 10           # Width for each frame is 10 pixels
    _select_color = 0xee2222    # color of border of selected frame
    _key_mark_color = 0x000000  # color of marks for key frames.
    _key_mark_sz = 4            # width and height of a key frame mark
    _tween_color = 0x000000     # color of tween line
    _tween_bgcolors = [0x80ff80, 0xff8080] # bg colors of tween frames
    # Colors for normal frames
    _normal_bgcolors = [0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xcccccc]
    _normal_border = 0xaaaaaa   # border color of normal frames.
    _active_border = 0xff3030   # border color of an active frame
    
    def __new__(clz, *args):
        if not frameline._type_id:
            frameline._type_id = gobject.type_register(frameline)
            pass
        fl_obj = gobject.new(frameline._type_id)
        return fl_obj
    
    def __init__(self, num_frames=20):
        self.connect('expose_event', self._fl_expose)
        self._num_frames = num_frames
        self._keys = []
        self._active_frame = -1
        pass

    def _fl_expose(self, widget, event):
        print 'Expose %s' % (repr(event))
        win = self.window
        x, y, w, h, depth = win.get_geometry()
        print '  Geometry of window: %dx%d+%d+%d' % (w, h, x, y)
        self.update()
        pass

    def _draw_tween(self, first_key, last_key):
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
        pass

    def _draw_frame(self, idx):
        win = self.window
        w_x, w_y, w_w, w_h, depth = win.get_geometry()
        
        gc = self._gc
        bg_idx = idx % len(self._normal_bgcolors)
        rgb = color_to_rgb(self._normal_bgcolors[bg_idx])
        color = gtk.gdk.Color(*rgb)
        gc.set_rgb_fg_color(color)
        
        f_x = self._frame_width * idx
        win.draw_rectangle(gc, True, f_x + 1, 0, self._frame_width, w_h)
        next_f_x = f_x + self._frame_width
        
        border_rgb = color_to_rgb(self._normal_border)
        border_color = gtk.gdk.Color(*border_rgb)
        gc.set_rgb_fg_color(border_color)
        gc.set_line_attributes(1, gtk.gdk.LINE_SOLID,
                               gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER)
        win.draw_line(gc, next_f_x, 0, next_f_x, w_h)
        pass
    
    def _draw_frames(self):
        win = self.window
        gc = gtk.gdk.GC(win)
        self._gc = gc
        
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
                self._draw_frame(i)
                i = i + 1
                pass
            pass
        pass

    def _draw_keyframes(self):
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
        pass
    
    def update(self):
        win = self.window
        x, y, w, h, depth = win.get_geometry()
        
        self._draw_frames()
        self._draw_keyframes()
        if self._active_frame != -1:
            self._draw_active()
            pass
        pass

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
        pass

    ## Tween the key frame specified by an index and the frame at right.
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

        self._active_frame = idx
        pass

    def deactive(self):
        self._active_frame = -1
        pass

    def set_num_frames(self, num):
        self._num_frames = num
        pass

    def __len__(self):
        return self._num_frames
    pass

if __name__ == '__main__':
    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    fl = frameline(40)
    fl.set_size_request(300, 30)
    fl.add_keyframe(3)
    fl.add_keyframe(9)
    fl.add_keyframe(15)
    fl.add_keyframe(20)
    fl.tween(3)
    fl.tween(15, 1)
    fl.active_frame(15)
    print 'num of frames: %d' % (len(fl))
    
    window.add(fl)
    fl.show()
    window.show()
    gtk.main()
    pass
