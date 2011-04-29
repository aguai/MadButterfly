import gtk
import os
import data_monitor

class FSM_window_base(object):
    _state_editor = None
    _state_name = None
    _state_radius = None

    _error_dialog = None
    _error_dialog_label = None
    
    def __init__(self):
        super(FSM_window_base, self).__init__()
        
        dirname = os.path.dirname(__file__)
        fname = os.path.join(dirname, 'FSM_window.glade')

        builder = gtk.Builder()
        builder.add_from_file(fname)

        main_win = builder.get_object("FSM_main_win")
        view_box = builder.get_object("view_box")
        
        state_editor = builder.get_object("state_editor")
        state_name = builder.get_object('state_name')
        state_radius = builder.get_object('state_radius')

        error_dialog = builder.get_object('error_dialog')
        error_dialog_label = builder.get_object('error_dialog_label')

        builder.connect_signals(self)
        
        self._builder = builder
        self._main_win = main_win
        self._view_box = view_box

        self._state_editor = state_editor
        self._state_name = state_name
        self._state_radius = state_radius

        self._error_dialog = error_dialog
        self._error_dialog_label = error_dialog_label
        pass

    def show_error(self, msg):
        error_dialog = self._error_dialog
        error_dialog_label = self._error_dialog_label
        
        error_dialog_label.set_text(msg)
        error_dialog.show()
        pass

    def hide_error(self):
        error_dialog = self._error_dialog
        error_dialog.hide()
        pass

    def show_state_editor(self, state_name=''):
        state_name_inp = self._state_name
        state_radius_inp = self._state_radius
        state_editor = self._state_editor
        
        state_name_inp.set_text(state_name)
        state_radius_inp.set_text('30')
        state_editor.show()
        pass

    def hide_state_editor(self):
        state_editor = self._state_editor
        state_editor.hide()
        pass

    def show(self):
        self._main_win.show()
        pass

    def hide(self):
        self._main_win.hide()
        pass

    def gtk_widget_hide(self, widget, event, *data):
        widget.hide()
        return True
    
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

    def on_state_apply_clicked(self, *args):
        pass
    
    def on_state_cancel_clicked(self, *args):
        state_editor = self._state_editor
        state_editor.hide()
        pass

    def on_error_dialog_ok_clicked(self, *args):
        error_dialog = self._error_dialog
        error_dialog.hide()
        pass
    pass

class FSM_transition(object):
    _doc = None
    _domview = None
    _fsm_layer = None
    _control_layer = None
    _state = None
    trn_cond = None
    trn_g = None
    _arrow_node = None
    _path_node = None

    def __init__(self, trn_cond):
       self.trn_cond = trn_cond
       pass

    def init(self, doc, domview, state, fsm_layer, control_layer):
        self._doc = doc
        self._domview = domview
        self._state = state
        self._fsm_layer = fsm_layer
        self._control_layer = control_layer
        pass

    @staticmethod
    def _update_graph(path, arrow_node, path_node):
        import math
        
        path_txt = 'M %f,%f C %f,%f %f,%f %f,%f' % tuple(path)
        path_node.setAttribute('d', path_txt)
        path_node.setAttribute('style', 'stroke: #000000; stroke-width: 1; '
                               'fill: none')

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
        arrow_node.setAttribute('d', arrow_txt)
        arrow_node.setAttribute('style', 'stroke: #000000; stroke-width: 1; '
                                'fill: #000000')
        pass
    
    def _draw_transition_real(self, parent, path):
        doc = self._doc

        trn_g = doc.createElement('svg:g')

        path_node = doc.createElement('svg:path')
        arrow_node = doc.createElement('svg:path')
        self._update_graph(path, arrow_node, path_node)

        trn_g.appendChild(path_node)
        trn_g.appendChild(arrow_node)

        parent.appendChild(trn_g)

        return trn_g, path_node, arrow_node

    @property
    def path(self):
        domview = self._domview
        state_name = self._state.state_name
        trn_cond = self.trn_cond
        trn = domview.get_transition(state_name, trn_cond)
        return trn[3]

    @property
    def target(self):
        domview = self._domview
        state_name = self._state.state_name
        trn_cond = self.trn_cond
        trn = domview.get_transition(state_name, trn_cond)
        return trn[1]

    def draw(self, parent):
        path = self.path
        trn_g, arrow_node, path_node = self._draw_transition_real(parent, path)
        self.trn_g = trn_g
        self._arrow_node = arrow_node
        self._path_node = path_node
        pass

    def clear(self):
        trn_g = self.trn_g
        parent = trn_g.parent()
        parent.removeChild(trn_g)
        pass

    def update(self):
        path = self.path
        arrow_node = self._arrow_node
        path_node = self._path_node
        self._update_graph(path, arrow_node, path_node)
        pass

    def adjust_by_ends(self, states):
        import math
        
        state = self._state
        state_name = state.state_name
        trn_cond = self.trn_cond
        
        path = self.path
        
        start_state = self._state
        start_x, start_y = start_state.xy
        start_r = start_state.r

        target_name = self.target
        stop_state = states[target_name]
        stop_x, stop_y = stop_state.xy
        stop_r = stop_state.r

        c0x, c0y, c1x, c1y, c2x, c2y, c3x, c3y = tuple(path)

        c0c1 = (c1x - c0x, c1y - c0y)
        c0c1_len = math.sqrt(c0c1[0] ** 2 + c0c1[1] ** 2)
        start_v = (c0c1[0] / c0c1_len, c0c1[1] / c0c1_len)

        c3c2 = (c2x - c3x, c2y - c3y)
        c3c2_len = math.sqrt(c3c2[0] ** 2 + c3c2[1] ** 2)
        stop_v = (c3c2[0] / c3c2_len, c3c2[1] / c3c2_len)

        c0x = start_v[0] * start_r + start_x
        c0y = start_v[1] * start_r + start_y
        c1x = start_v[0] * c0c1_len + c0x
        c1y = start_v[1] * c0c1_len + c0y
        c3x = stop_v[0] * stop_r + stop_x
        c3y = stop_v[1] * stop_r + stop_y
        c2x = stop_v[0] * c3c2_len + c3x
        c2y = stop_v[1] * c3c2_len + c3y
        new_path = [c0x, c0y, c1x, c1y, c2x, c2y, c3x, c3y]
        
        domview = self._domview
        domview.set_transition_path(state_name, trn_cond, new_path)
        pass
    pass

class FSM_state(object):
    _doc = None
    _domview = None
    _fsm_layer = None
    _control_layer = None
    state_name = None
    state_g = None
    _text_node = None
    _circle_node = None
    transitions = None
    from_srcs = None

    _state_g_hdl_id = None
    _selected_rect = None
    
    def __init__(self, state_name):
        self.state_name = state_name
        self.transitions = {}
        self.from_srcs = set()
        pass

    def init(self, doc, domview, fsm_layer, control_layer):
        self._doc = doc
        self._domview = domview
        self._fsm_layer = fsm_layer
        self._control_layer = control_layer
        pass

    def _update_graph(self, text_node, text_content, circle_node,
                      state_name, r, x, y):
        circle_node.setAttribute('r', str(r))
        circle_node.setAttribute('cx', str(x))
        circle_node.setAttribute('cy', str(y))
        circle_node.setAttribute('style', 'stroke: #000000; stroke-width: 1; '
                                 'fill: #ffffff')

        text_node.setAttribute('style', 'stroke: #000000; fill: #000000; font-size: 16px')

        text_content.setContent(state_name)

        doc = self._doc
        spdoc = doc.spdoc
        spdoc.ensureUpToDate()
        tx, ty, tw, th = text_node.getBBox()
        text_node.setAttribute('x', str(x - tw / 2))
        text_node.setAttribute('y', str(y + th / 2))
        pass

    def grab(self, callback):
        assert not self._state_g_hdl_id
        
        state_g = self.state_g
        state_g_spitem = state_g.spitem
        state_g_hdl_id = state_g_spitem.connect('mouse-event', callback)
        self._state_g_hdl_id = state_g_hdl_id
        pass

    def ungrab(self):
        if not self._state_g_hdl:
            return
        state_g = self.state_g
        state_g_hdl_id = self._state_g_hdl_id
        state_g.disconnect(state_g_hdl_id)
        pass

    def _translate_page_xy(self, x, y):
        doc = self._doc
        root = doc.root()
        page_h_txt = root.getAttribute('height')
        page_h = float(page_h_txt)
        svgx = x
        svgy = page_h - y
        return svgx, svgy

    def show_selected(self):
        if not self._selected_rect:
            doc = self._doc
            rect = doc.createElement('svg:rect')
            control_layer = self._control_layer
            rect.setAttribute('style',
                              'stroke: #404040; stroke-width: 1; '
                              'stroke-dasharray: 6 4; fill: none')
            control_layer.appendChild(rect)
            self._selected_rect = rect
            pass

        state_g = self.state_g
        rect = self._selected_rect
        
        px, py, pw, ph = state_g.getBBox()
        x, y = self._translate_page_xy(px, py)
        y = y - ph              # px, py is left-bottom corner
        
        rect.setAttribute('x', str(x - 2))
        rect.setAttribute('y', str(y - 2))
        rect.setAttribute('width', str(pw + 4))
        rect.setAttribute('height', str(ph + 4))
        pass

    def hide_selected(self):
        if not self._selected_rect:
            return

        control_layer = self._control_layer
        rect = self._selected_rect
        control_layer.removeChild(rect)
        self._selected_rect = None
        pass
    
    def _draw_state_real(self, parent, state_name, r, x, y):
        doc = self._doc
        
        state_g = doc.createElement('svg:g')
        
        text = doc.createElement('svg:text')
        circle = doc.createElement('svg:circle')
        text_content = doc.createTextNode(state_name)

        text.appendChild(text_content)
        state_g.appendChild(circle)
        state_g.appendChild(text)
        parent.appendChild(state_g)

        self._update_graph(text, text_content, circle, state_name, r, x, y)

        return state_g, text, text_content, circle

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
        state_g, text_node, text_content, circle_node = \
            self._draw_state_real(parent, state_name, r, x, y)
        self.state_g = state_g
        self._text_node = text_node
        self._text_content = text_content
        self._circle_node = circle_node

        for trn_cond in self.all_transitions:
            trn = FSM_transition(trn_cond)
            trn.init(self._doc, domview, self,
                     self._fsm_layer, self._control_layer)
            trn.draw(parent)
            self.transitions[trn_cond] = trn
            pass
        pass

    def clear(self):
        state_g = self.state_g
        parent = state_g.parent()
        parent.removeChild(state_g)
        pass

    def update(self):
        text_node = self._text_node
        text_content = self._text_content
        circle_node = self._circle_node
        state_name = self.state_name
        r = self.r
        x, y = self.xy
        self._update_graph(text_node, text_content, circle_node, state_name,
                           r, x, y)
        pass

    def tell_target_states(self, states):
        transitions = self.transitions
        target_state_names = [trn.target for trn in transitions.values()]
        target_states = [states[target_name]
                         for target_name in target_state_names]
        state_name = self.state_name
        for target_state in target_states:
            target_state.from_srcs.add(state_name)
            pass
        pass

    def adjust_transitions(self, states):
        import itertools
        
        for trn in self.transitions.values():
            trn.adjust_by_ends(states)
            trn.update()
            pass

        state_name = self.state_name
        src_states = [states[src_state_name]
                      for src_state_name in self.from_srcs]
        states_transitions = [state.transitions.values()
                              for state in src_states]
        in_state_transitions = [[trn for trn in state_transitions
                                 if trn.target == state_name]
                                for state_transitions in states_transitions]
        in_transitions = itertools.chain(*in_state_transitions)
        for trn in in_transitions:
            trn.adjust_by_ends(states)
            trn.update()
            pass
        pass
    pass


class FSM_move_state_mode(object):
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'
    
    _window = None
    _selected_state = None
    
    def __init__(self, window, domview_ui):
        super(FSM_move_state_mode, self).__init__()
        
        self._window = window
        self._domview = domview_ui
        self._locker = domview_ui
        pass

    def on_move_state_background(self, item, evtype, buttons, x, y):
        pass

    def _select_state(self, state):
        if self._selected_state:
            self._selected_state.hide_selected()
            pass
        self._selected_state = state
        state.show_selected()
        pass

    def _clear_select(self):
        if self._selected_state:
            self._selected_state.hide_selected()
            pass
        self._selected_state = None
        pass

    def handle_move_state_state(self, state, evtype, buttons, x, y):
        import pybInkscape

        window = self._window
        states = window._states

        def moving_state(item, evtype, buttons, x, y):
            if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE:
                window.ungrab_mouse()
                pass
            new_state_x = orign_state_x + start_x - x
            new_state_y = orign_state_y + start_y - y

            domview = self._domview
            domview.set_state_xy(state.state_name, x, y)
            state.update()
            state.adjust_transitions(states)
            state.show_selected()
            pass
        
        window = self._window
        
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_PRESS:
            start_x = x
            start_y = y
            orign_state_x, orign_state_y = state.xy
            
            self._select_state(state)
            window.grab_mouse(moving_state)
            pass
        
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE:
            window.ungrab_mouse()
            pass
        pass

    def activate(self):
        window = self._window
        window._emit_leave_mode()
        window._clear_leave_mode_cb()
        window.ungrab_all()
        
        window.grab_bg(self.on_move_state_background)
        window.grab_state(self.handle_move_state_state)
        pass

    def deactivate(self):
        if self._selected_state:
            self._clear_select()
            pass
        pass
    pass


class FSM_add_state_mode(object):
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'

    _saved_x = 0
    _saved_y = 0

    def __init__(self, window, domview_ui):
        super(FSM_add_state_mode, self).__init__()
        
        self._window = window
        self._domview = domview_ui
        self._locker = domview_ui
        pass

    def handle_new_state(self):
        import traceback
        
        domview = self._domview
        window = self._window
        x, y = window._translate_xy(self._saved_x, self._saved_y)

        state_name = window._state_name.get_text()
        r_txt = window._state_radius.get_text()
        try:
            r = float(r_txt)
        except ValueError:
            traceback.print_exc()
            window.show_error('Invalid value: "%s" is not a valid value '
                              'for radius.' % (r_txt))
            return
        
        try:
            domview.add_state(state_name)
        except:
            traceback.print_exc()
            window.show_error('Invalid state name: "%s" is existing' %
                              (state_name))
            return
        domview.set_state_xy(state_name, x, y)
        domview.set_state_r(state_name, r)

        window._load_new_state_incr(state_name)

        window.hide_state_editor()
        pass
    
    def on_add_state_background(self, item, evtype, buttons, x, y):
        import pybInkscape

        window = self._window
        
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE and \
                buttons == 1:
            self._saved_x = x
            self._saved_y = y
            window.show_state_editor()
            pass
        pass

    def activate(self):
        window = self._window
        
        window._emit_leave_mode()
        window.ungrab_all()
        
        window.grab_bg(self.on_add_state_background)
        pass

    def deactivate(self):
        pass
    pass

class FSM_window(FSM_window_base):
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'

    _background = None
    _fsm_layer = None
    _control_layer = None
    width = 1024
    height = 768

    _grab_hdl = None
    _bg_hdl = None

    _leave_mode_cb = None
    _move_state_mode = None
    _add_state_mode = None
    _state_mouse_event_handler = None
    
    def __init__(self, domview_ui, close_cb, destroy_cb):
        super(FSM_window, self).__init__()

        self._locker = domview_ui

        self._domview = domview_ui
        self._states = {}
        
        self._close_cb = close_cb # callback to close editor window (hide)
        self._destroy_cb = destroy_cb # callback to destroy editor window

        self._move_state_mode = FSM_move_state_mode(self, domview_ui)
        self._add_state_mode = FSM_add_state_mode(self, domview_ui)
        pass

    def _init_layers(self):
        doc = self._doc()
        root = self._root()
        
        root.setAttribute('inkscape:groupmode', 'layer')
        
        background = doc.createElement('svg:rect')
        background.setAttribute('x', '0')
        background.setAttribute('y', '0')
        background.setAttribute('width', str(self.width))
        background.setAttribute('height', str(self.height))
        background.setAttribute('style', 'fill: #ffffff')
        root.appendChild(background)
        self._background = background
        
        fsm_layer = doc.createElement('svg:g')
        fsm_layer.setAttribute('inkscape:groupmode', 'layer')
        root.appendChild(fsm_layer)
        self._fsm_layer = fsm_layer

        control_layer = doc.createElement('svg:g')
        control_layer.setAttribute('inkscape:groupmode', 'layer')
        root.appendChild(control_layer)
        self._control_layer = control_layer
        
        self._add_state_mode.activate()
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

    def _translate_xy(self, x, y):
        return x, y

    def _clear_view(self):
        if not self._background:
            self._init_layers()
            return
        
        children = [child for child in self._fsm_layer.childList()] + \
            [child for child in self._control_layer.childList()]
        for child in children:
            parent = child.parent()
            parent.removeChild(child)
            pass

        self._states = {}
        pass

    def _draw_state_domview(self, state_name):
        domview = self._domview
        doc = self._doc()
        fsm_layer = self._fsm_layer
        
        state = FSM_state(state_name)
        state.init(doc, domview, self._fsm_layer, self._control_layer)
        self._states[state_name] = state
        
        state.draw(fsm_layer)
        pass

    def _set_leave_mode_cb(self, callback):
        self._leave_mode_cb = callback
        pass

    def _clear_leave_mode_cb(self):
        self._leave_mode_cb = None
        pass

    def _emit_leave_mode(self):
        if self._leave_mode_cb:
            self._leave_mode_cb()
            pass
        pass

    def ungrab_all(self):
        self.ungrab_bg()
        self.ungrab_mouse()
        self.ungrab_state()
        pass

    def on_state_mouse_event(self, state, evtype, buttons, x, y):
        if self._state_mouse_event_handler:
            self._state_mouse_event_handler(state, evtype, buttons, x, y)
            pass
        pass

    def _install_state_event_handler(self, state):
        def mouse_event_handler(item, evtype, buttons, x, y):
            self.on_state_mouse_event(state, evtype, buttons, x, y)
            pass
        state.grab(mouse_event_handler)
        pass

    def grab_state(self, callback):
        assert self._state_mouse_event_handler is None
        self._state_mouse_event_handler = callback
        pass

    def ungrab_state(self):
        self._state_mouse_event_handler = None
        pass

    def _load_new_state(self, state_name):
        states = self._states
        
        self._draw_state_domview(state_name)
        state = states[state_name]
        self._install_state_event_handler(state)
        pass

    ## \brief Load new state incrementally.
    #
    def _load_new_state_incr(self, state_name):
        self._load_new_state(state_name)
        states = self._states
        state = states[state_name]
        state.tell_target_states(states)
        pass
    
    def _rebuild_from_srcs(self):
        states = self._states
        domview = self._domview
        state_names = domview.all_state_names()
        for state_name in state_names:
            state = states[state_name]
            state.tell_target_states(states)
            pass
        pass
    
    def _update_view(self):
        self._clear_view()
        states = self._states
        
        domview = self._domview
        
        state_names = domview.all_state_names()
        for state_name in state_names:
            self._load_new_state(state_name)
            pass
        self._rebuild_from_srcs()
        pass

    def set_svg_view(self, view):
        self._view_box.add(view)
        self._view_widget = view
        
        root = self._root()
        root.setAttribute('width', '1024')
        root.setAttribute('height', '768')
        view.setResize(True, 800, 600)
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
        mode = self._add_state_mode
        mode.activate()
        self._set_leave_mode_cb(lambda: mode.deactivate())
        pass

    def on_move_state_toggled(self, *args):
        mode = self._move_state_mode
        mode.activate()
        self._set_leave_mode_cb(lambda: mode.deactivate())
        pass

    def on_state_apply_clicked(self, *args):
        self._add_state_mode.handle_new_state()
        pass

    def _install_test_data(self):
        self._init_layers()
        
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
        pass

    def show(self):
        self._install_test_data()
        self._install_test_data = lambda: None
        self._update_view()
        super(FSM_window, self).show()
        pass

    def grab_mouse(self, callback):
        assert self._grab_hdl is None
        
        root = self._root()
        root.setAttribute('inkscape:groupmode', '')
        self._grab_hdl = root.spitem.connect('mouse-event', callback)
        pass

    def ungrab_mouse(self):
        if not self._grab_hdl:
            return
        
        root = self._root()
        root.spitem.disconnect(self._grab_hdl)
        self._grab_hdl = None
        root.setAttribute('inkscape:groupmode', 'layer')
        pass

    def grab_bg(self, callback):
        assert self._bg_hdl is None
        assert self._background

        background = self._background
        bg_hdl = background.spitem.connect('mouse-event', callback)
        self._bg_hdl = bg_hdl
        pass

    def ungrab_bg(self):
        if not self._bg_hdl:
            return

        background = self._background
        bg_hdl = self._bg_hdl
        background.spitem.disconnect(bg_hdl)
        self._bg_hdl = None
        pass
    pass

if __name__ == '__main__':
    win = FSM_window()
    win._main_win.connect('destroy', gtk.main_quit)
    win.show()
    gtk.main()
    pass
