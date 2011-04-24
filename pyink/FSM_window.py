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

class FSM_transition(object):
    _doc = None
    _domview = None
    _state = None
    trn_cond = None
    trn_g = None

    def __init__(self, trn_cond):
       self.trn_cond = trn_cond
       pass

    def init(self, doc, domview, state):
        self._doc = doc
        self._domview = domview
        self._state = state
        pass
    
    def _draw_transition_real(self, parent, path):
        import math
        doc = self._doc

        trn_g = doc.createElement('svg:g')

        path_node = doc.createElement('svg:path')
        path_txt = 'M %f,%f C %f,%f %f,%f %f,%f' % tuple(path)
        path_node.setAttribute('d', path_txt)
        path_node.setAttribute('style', 'stroke: #000000; stroke-width: 1; '
                               'fill: none')
        trn_g.appendChild(path_node)

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
        trn_g.appendChild(arrow_node)

        parent.appendChild(trn_g)

        self.trn_g = trn_g
        pass

    @property
    def path(self):
        domview = self._domview
        state_name = self._state.state_name
        trn_cond = self.trn_cond
        trn = domview.get_transition(state_name, trn_cond)
        return trn[3]

    def draw(self, parent):
        path = self.path
        self._draw_transition_real(parent, path)
        pass
    pass

class FSM_state(object):
    _doc = None
    _domview = None
    state_name = None
    state_g = None
    transitions = None
    
    def __init__(self, state_name):
        self.state_name = state_name
        self.transitions = {}
        pass

    def init(self, doc, domview):
        self._doc = doc
        self._domview = domview
        pass
    
    def _draw_state_real(self, parent, state_name, r, x, y):
        doc = self._doc
        
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

        parent.appendChild(state_g)
        
        tx, ty, tw, th = text.getBBox()
        text.setAttribute('x', str(x - tw / 2))
        text.setAttribute('y', str(y + th / 2))

        return state_g

    @property
    def r(self):
        domview = self._domview
        state_name = self.state_name
        r = domview.get_state_r(state_name)
        return r

    @property
    def xy(self):
        domview = self._domview
        state_name = self.state_name
        xy = domview.get_state_xy(state_name)
        return xy

    @property
    def all_transitions(self):
        domview = self._domview
        state_name = self.state_name
        conds = domview.all_transitions(state_name)
        return conds

    def draw(self, parent):
        domview = self._domview
        state_name = self.state_name

        r = self.r
        x, y = self.xy
        state_g = self._draw_state_real(parent, state_name, r, x, y)
        self.state_g = state_g

        for trn_cond in self.all_transitions:
            trn = FSM_transition(trn_cond)
            trn.init(self._doc, domview, self)
            trn.draw(parent)
            self.transitions[trn_cond] = trn
            pass
        pass
    pass

class FSM_window(FSM_window_base):
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'
    
    def __init__(self, domview_ui, close_cb, destroy_cb):
        super(FSM_window, self).__init__()

        self._locker = domview_ui

        self._domview = domview_ui
        self._states = {}
        
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

        self._states = {}
        pass

    def _update_view(self):
        self._clear_view()
        
        domview = self._domview
        doc = self._doc()
        root = self._root()
        
        state_names = domview.all_state_names()
        for state_name in state_names:
            state = FSM_state(state_name)
            state.init(doc, domview)
            self._states[state_name] = state
            
            state.draw(root)
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

        state1 = 'state 1'
        domview.add_state(state1)
        domview.set_state_r(state1, 50)
        domview.set_state_xy(state1, 200, 100)
        state2 = 'state 2'
        domview.add_state(state2)
        domview.set_state_r(state2, 30)
        domview.set_state_xy(state2, 300, 100)
        domview.add_transition(state1, 'event1', state2)
        domview.set_transition_path(state1, 'event1', (200, 150,
                                                       240, 180,
                                                       260, 180,
                                                       300, 130))
        self._update_view()
        
        state = FSM_state('test1')
        state.init(rdoc, domview)
        state._draw_state_real(root_node, 'test1', 40, 100, 50)

        trn = FSM_transition('event1')
        trn.init(rdoc, domview, state)
        trn._draw_transition_real(root_node, (100, 100,
                                              140, 120,
                                              160, 120,
                                              200, 100))
        pass
    pass

if __name__ == '__main__':
    win = FSM_window()
    win._main_win.connect('destroy', gtk.main_quit)
    win.show()
    gtk.main()
    pass
