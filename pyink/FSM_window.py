import gtk
import os
import data_monitor

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
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'
    
    def __init__(self, domview_ui, close_cb, destroy_cb):
        super(FSM_window, self).__init__()

        self._locker = domview_ui

        self._domview = domview_ui
        self._state_nodes = {}
        
        self._close_cb = close_cb # callback to close editor window (hide)
        self._destroy_cb = destroy_cb # callback to destroy editor window
        pass

    def _doc(self):
        view_widget = self._view_widget
        view = view_widget.view
        doc = view.doc().rdoc
        return doc

    def _root(self):
        doc = self._doc()
        root = doc.root()
        return root

    def _clear_view(self):
        root = self._root()
        
        children = [child for child in root.childList()
                    if child.name() == 'svg:g']
        for child in children:
            root.removeChild(child)
            pass

        self._state_nodes = {}
        pass

    def _draw_transition_real(self, state_g, path):
        import math
        doc = self._doc()

        path_node = doc.createElement('svg:path')
        path_txt = 'M %f,%f C %f,%f %f,%f %f,%f' % tuple(path)
        path_node.setAttribute('d', path_txt)
        path_node.setAttribute('style', 'stroke: #000000; stroke-width: 1; '
                               'fill: none')
        state_g.appendChild(path_node)

        # c0 c1 c2 c3 of cubic curve
        c3 = (path[6], path[7])
        c2 = (path[4], path[5])
        c23_v = (c3[0] - c2[0], c3[1] - c2[1])
        c23_len = math.sqrt(c23_v[0] * c23_v[0] + c23_v[1] * c23_v[1])
        adir = (c23_v[0] / c23_len, c23_v[1] / c23_len) # arrow direction
        odir = (-adir[1], adir[0]) # othogonal direction
        arrow_pts = (c3[0], c3[1],
                     -adir[0] * 10 + odir[0] * 4, -adir[1] * 10 + odir[1] * 4,
                     -odir[0] * 8, -odir[1] * 8)
        arrow_txt = 'M %f,%f l %f,%f l %f,%f z' % arrow_pts
        arrow_node = doc.createElement('svg:path')
        arrow_node.setAttribute('d', arrow_txt)
        arrow_node.setAttribute('style', 'stroke: #000000; stroke-width: 1; '
                                'fill: #000000')
        state_g.appendChild(arrow_node)
        pass

    def _draw_state_real(self, state_name, r, x, y):
        doc = self._doc()
        root = self._root()
        
        state_g = doc.createElement('svg:g')
        state_g.setAttribute('inkscape:groupmode', 'layer')
        
        circle = doc.createElement('svg:circle')
        circle.setAttribute('r', str(r))
        circle.setAttribute('cx', str(x))
        circle.setAttribute('cy', str(y))
        circle.setAttribute('style', 'stroke: #000000; stroke-width: 1; '
                            'fill: #ffffff')
        state_g.appendChild(circle)

        text = doc.createElement('svg:text')
        text_content = doc.createTextNode(state_name)
        text.appendChild(text_content)
        text.setAttribute('font-size', '16')
        text.setAttribute('style', 'stroke: #000000; fill: #000000')
        state_g.appendChild(text)

        root.appendChild(state_g)
        
        tx, ty, tw, th = text.getBBox()
        text.setAttribute('x', str(x - tw / 2))
        text.setAttribute('y', str(y + th / 2))

        return state_g

    def _draw_state(self, state_name):
        domview = self._domview

        r = domview.get_state_r(state_name)
        x, y = domview.get_state_xy(state_name)
        state_g = self._draw_state_real(state_name, r, x, y)
        self._state_nodes[state_name] = state_g

        transitions = [domview.get_transition(state_name, trn_name)[3]
                       for trn_name in domview.all_transitions()]
        for trn in transitions:
            self._draw_transition_real(state_g, trn)
            pass
        pass

    def _update_view(self):
        self._clear_view()
        
        domview = self._domview
        state_names = domview.all_state_names()
        for state_name in state_names:
            self._draw_state(state_name)
            pass
        pass

    def set_svg_view(self, view):
        self._view_box.add(view)
        self._view_widget = view
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

    def on_add_state_toggled(self, *args):
        domview = self._domview
        domview.add_state('test0')
        domview.add_state('test1')
        domview.add_transition('test0', 'event1', 'test1')
        
        view = self._view_widget.view
        doc = view.doc()
        rdoc = doc.rdoc
        root_node = doc.root().repr
        
        line_node = rdoc.createElement('svg:line')
        line_node.setAttribute('x1', '10')
        line_node.setAttribute('y1', '10')
        line_node.setAttribute('x2', '100')
        line_node.setAttribute('y2', '100')
        line_node.setAttribute('style', 'stroke: #000000; stroke-width:2')
        
        print root_node.name()
        print root_node.childList()[-1].name()
        root_node.setAttribute('inkscape:groupmode', 'layer')
        root_node.appendChild(line_node)

        def show_msg(*args, **kws):
            print 'mouse_event'
            print args
            pass
        print 'before connect'
        hdl_id = line_node.spitem.connect('mouse-event', show_msg)
        print hdl_id

        state_g = self._draw_state_real('test1', 40, 100, 50)
        self._draw_transition_real(state_g, (100, 100, 140, 120, 160, 120, 200, 100))
        pass
    pass

if __name__ == '__main__':
    win = FSM_window()
    win._main_win.connect('destroy', gtk.main_quit)
    win.show()
    gtk.main()
    pass
