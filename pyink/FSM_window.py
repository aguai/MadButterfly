import gtk
import os

class FSM_window_base(object):
    def __init__(self):
        super(FSM_window_base, self).__init__()
        
        dirname = os.path.dirname(__file__)
        fname = os.path.join(dirname, 'FSM_window.glade')

        builder = gtk.Builder()
        builder.add_from_file(fname)

        main_win = builder.get_object("FSM_main_win")
        view_box = builder.get_object("view_box")

        builder.connect_signals(self)
        
        self._builder = builder
        self._main_win = main_win
        self._view_box = view_box
        pass

    def show(self):
        self._main_win.show()
        pass

    def hide(self):
        self._main_win.hide()
        pass
    
    def on_start_state_activate(self, *args):
        pass
    
    def on_rename_state_activate(self, *args):
        pass
    
    def on_remove_state_activate(self, *args):
        pass
    
    def on_zoom_out_clicked(self, *args):
        pass
    
    def on_zoom_in_clicked(self, *args):
        pass
    
    def on_move_state_toggled(self, *args):
        pass
    
    def on_add_state_toggled(self, *args):
        pass
    
    def on_close_window_activate(self, *args):
        pass

    def on_FSM_main_win_destroy_event(self, *args):
        pass
    pass

class FSM_window(FSM_window_base):
    def __init__(self, close_cb, destroy_cb):
        super(FSM_window, self).__init__()

        self._close_cb = close_cb
        self._destroy_cb = destroy_cb
        pass

    def set_svg_view(self, view):
        self._view_box.add(view)
        pass

    def on_close_window_activate(self, *args):
        self._close_cb()
        pass
    
    def on_FSM_main_win_destroy_event(self, *args):
        self._destroy_cb()
        pass
    
    def on_FSM_main_win_delete_event(self, *args):
        self._destroy_cb()
        pass
    pass

if __name__ == '__main__':
    win = FSM_window()
    win._main_win.connect('destroy', gtk.main_quit)
    win.show()
    gtk.main()
    pass
