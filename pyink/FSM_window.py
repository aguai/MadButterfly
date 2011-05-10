import gtk
import os
import math
import data_monitor
import pybInkscape


## \brief Wrap domview to provide a view for FSM of a component.
#
# This class is a decorator of domview to provide a view for FSM of a
# component.  All accesses to FSM states and transitions will be
# dispatched to domview with name of a specified component as one of
# argument list.  Caller don't need to know the component that it is
# working on.
#
class _compview(object):
    _domview = None
    _comp_name = None
    
    def __init__(self, domview, comp_name):
        self._domview = domview
        self._comp_name = comp_name
        pass

    def switch_component(self, comp_name):
        self._comp_name = comp_name
        pass

    def all_state_names(self):
        return self._domview.all_state_names(self._comp_name)

    def get_start_state_name(self):
        return self._domview.get_start_state_name(self._comp_name)

    def rm_state(self, state_name):
        self._domview.rm_state(self._comp_name, state_name)
        pass

    def add_state(self, state_name):
        self._domview.add_state(self._comp_name, state_name)
        pass

    def rename_state(self, state_name, new_name):
        self._domview.rename_state(self._comp_name, state_name, new_name)
        pass

    def set_start_state(self, state_name):
        self._domview.set_start_state(self._comp_name, state_name)
        pass

    def set_state_entry_action(self, state_name, entry_action):
        self._domview.set_state_entry_action(self._comp_name,
                                             state_name, entry_action)
        pass

    def set_state_r(self, state_name, r):
        self._domview.set_state_r(self._comp_name, state_name, r)
        pass

    def set_state_xy(self, state_name, x, y):
        self._domview.set_state_xy(self._comp_name, state_name, x, y)
        pass

    def get_state_entry_action(self, state_name):
        return self._domview.get_state_entry_action(self._comp_name,
                                                    state_name)

    def get_state_r(self, state_name):
        return self._domview.get_state_r(self._comp_name, state_name)

    def get_state_xy(self, state_name):
        return self._domview.get_state_xy(self._comp_name, state_name)

    def all_transitions(self, state_name):
        return self._domview.all_transitions(self._comp_name, state_name)

    def add_transition(self, state_name, cond, target):
        self._domview.add_transition(self._comp_name, state_name,
                                     cond, target)
        pass

    def rm_transition(self, state_name, cond):
        self._domview.rm_transition(self._comp_name, state_name, cond)
        pass

    def change_transition_cond(self, state_name, old_cond, new_cond):
        self._domview.change_transition_cond(self._comp_name,
                                             state_name,
                                             old_cond, new_cond)
        pass

    def get_transition(self, state_name, cond):
        return self._domview.get_transition(self._comp_name, state_name, cond)

    def set_transition_action(self, state_name, cond, action):
        self._domview.set_transition_action(self._comp_name,
                                            state_name, cond, action)
        pass

    def set_transition_path(self, state_name, cond, path):
        self._domview.set_transition_path(self._comp_name,
                                          state_name, cond, path)
        pass

    def chg_transition_cond(self, state_name, cond, new_cond):
        self._domview.chg_transition_cond(self._comp_name,
                                          state_name, cond, new_cond)
        pass
    pass

class _dragger(object):
    _node = None
    _start_x = None
    _start_y = None
    _state = 0
    
    def __init__(self):
        pass
    
    ## \brief Mouse event handler
    #
    # This is a placeholder for mouse vent handlers.  This attribute
    # of instances is switched between _mouse_event_waiting and
    # _mouse_event_pressed.
    #
    def mouse_event(self, evtype, button, x, y):
        raise RuntimeError, 'should not be here'
    
    def _mouse_event_waiting(self, evtype, button, x, y):
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_PRESS and \
                button == 1:
            self._start_x = x
            self._start_y = y
            self.mouse_event = self._mouse_event_pressed
            self.start_drag()
            pass
        pass
    
    def _mouse_event_pressed(self, evtype, button, x, y):
        rx = x - self._start_x
        ry = y - self._start_y
        
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE:
            self.mouse_event = self._mouse_event_waiting
            self.stop_drag(rx, ry)
            pass

        self.update(rx, ry)
        pass

    def start(self):
        self.mouse_event = self._mouse_event_waiting
        pass

    def stop(self):
        pass

    def connect(self, node):
        self.start()
        
        def handler(item, evtype, button, x, y):
            self.mouse_event(evtype, button, x, y)
            pass
        
        self._node = node
        hdl_id = node.spitem.connect('mouse-event', handler)
        self._hdl_id = hdl_id
        pass

    def disconnect(self):
        self.stop()
        node = self._node
        hdl_id = self._hdl_id
        node.disconnect(hdl_id)
        pass

    def start_drag(self):
        pass

    def stop_drag(self, rx, ry):
        pass

    def update(self, rx, ry):
        pass
    pass


class FSM_window_base(object):
    _add_state_button = None
    _move_state_button = None
    
    _state_editor = None
    _state_name = None
    _state_radius = None

    _error_dialog = None
    _error_dialog_label = None

    _state_menu = None
    
    def __init__(self):
        super(FSM_window_base, self).__init__()
        
        dirname = os.path.dirname(__file__)
        fname = os.path.join(dirname, 'FSM_window.glade')

        builder = gtk.Builder()
        builder.add_from_file(fname)

        main_win = builder.get_object("FSM_main_win")
        view_box = builder.get_object("view_box")
        add_state_button = builder.get_object('add_state')
        move_state_button = builder.get_object('move_state')
        
        state_editor = builder.get_object("state_editor")
        state_name = builder.get_object('state_name')
        state_radius = builder.get_object('state_radius')
        state_entry_action = builder.get_object('state_entry_action')

        error_dialog = builder.get_object('error_dialog')
        error_dialog_label = builder.get_object('error_dialog_label')

        transition_editor = builder.get_object('transition_editor')
        transition_cond = builder.get_object('transition_cond')
        transition_action = builder.get_object('transition_action')

        state_menu = builder.get_object('state_menu')
        transition_menu = builder.get_object('transition_menu')

        builder.connect_signals(self)
        
        self._builder = builder
        self._main_win = main_win
        self._view_box = view_box
        self._add_state_button = add_state_button
        self._move_state_button = move_state_button

        self._state_editor = state_editor
        self._state_name = state_name
        self._state_radius = state_radius
        self._state_entry_action = state_entry_action

        self._transition_editor = transition_editor
        self._transition_cond = transition_cond
        self._transition_action = transition_action

        self._error_dialog = error_dialog
        self._error_dialog_label = error_dialog_label

        self._state_menu = state_menu
        self._transition_menu = transition_menu
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

    def show_state_editor(self, state_name='', radius=30, entry_action=''):
        state_name_inp = self._state_name
        state_radius_inp = self._state_radius
        state_entry_action = self._state_entry_action
        state_editor = self._state_editor
        
        state_name_inp.set_text(state_name)
        state_radius_inp.set_text(str(radius))
        state_entry_action.set_text(entry_action or '')
        state_editor.show()
        pass

    def hide_state_editor(self):
        state_editor = self._state_editor
        state_editor.hide()
        pass

    def show_transition_editor(self, cond='', action=''):
        transition_cond = self._transition_cond
        transition_action = self._transition_action
        transition_editor = self._transition_editor

        transition_cond.set_text(cond)
        transition_action.set_text(action)
        transition_editor.show()
        pass

    def hide_transition_editor(self):
        transition_editor = self._transition_editor
        transition_editor.hide()
        pass

    def popup_state_menu(self):
        menu = self._state_menu
        menu.popup(None, None, None, 0, 0)
        pass

    def popup_transition_menu(self):
        menu = self._transition_menu
        menu.popup(None, None, None, 0, 0)
        pass
    
    def show(self):
        self._main_win.show()
        self._add_state_button.set_active(True)
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

    def on_add_transition_activate(self, *args):
        pass

    def on_del_state_activate(self, *args):
        pass

    def on_edit_state_activate(self, *args):
        pass

    def on_transition_apply_clicked(self, *args):
        pass

    def on_transition_cancel_clicked(self, *args):
        transition_editor = self._transition_editor
        transition_editor.hide()
        pass

    def on_del_transition_activate(self, *args):
        pass

    def on_edit_transition_activate(self, *args):
        pass
    pass


class FSM_transition(object):
    _doc = None
    _compview = None
    _fsm_layer = None
    _control_layer = None
    _state = None
    _states = None
    trn_cond = None
    trn_g = None
    _arrow_node = None
    _path_node = None
    _control_points = None
    _selected_rect = None

    def __init__(self, trn_cond):
       self.trn_cond = trn_cond
       pass

    def init(self, doc, compview, state, states, fsm_layer, control_layer):
        self._doc = doc
        self._compview = compview
        self._state = state
        self._states = states
        self._fsm_layer = fsm_layer
        self._control_layer = control_layer
        pass

    def _translate_page_xy(self, x, y):
        return x, y

    @staticmethod
    def _update_graph(path, arrow_node, path_node):
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

    def _gen_path(self):
        states = self._states
        target_name = self.target
        target_state = states[target_name]
        src_state = self._state

        src_x, src_y = src_state.xy
        src_r = src_state.r
        target_x, target_y = target_state.xy
        target_r = target_state.r

        src_target_v = (target_x - src_x, target_y - src_y)
        src_target_len = \
            math.sqrt(src_target_v[0] ** 2 + src_target_v[1] ** 2)
        distance = src_target_len - src_r - target_r
        distance3 = distance / 3
        src_target_uv = (src_target_v[0] / src_target_len,
                         src_target_v[1] / src_target_len)

        c0x = src_x + src_target_uv[0] * src_r
        c0y = src_y + src_target_uv[1] * src_r
        c1x = c0x + src_target_uv[0] * distance3
        c1y = c0y + src_target_uv[1] * distance3
        c3x = target_x - src_target_uv[0] * target_r
        c3y = target_y - src_target_uv[1] * target_r
        c2x = c3x - src_target_uv[0] * distance3
        c2y = c3y - src_target_uv[1] * distance3

        path = [c0x, c0y, c1x, c1y, c2x, c2y, c3x, c3y]
        return path

    @property
    def path(self):
        compview = self._compview
        state_name = self._state.state_name
        trn_cond = self.trn_cond
        trn = compview.get_transition(state_name, trn_cond)
        path = trn[3]

        if not path:
            path = self._gen_path()
            pass
        
        return path

    @property
    def target(self):
        compview = self._compview
        state_name = self._state.state_name
        trn_cond = self.trn_cond
        trn = compview.get_transition(state_name, trn_cond)
        return trn[1]

    @property
    def state(self):
        return self._state

    @property
    def action(self):
        compview = self._compview
        state_name = self._state.state_name
        trn_cond = self.trn_cond
        trn = compview.get_transition(state_name, trn_cond)
        return trn[2]

    def draw(self):
        path = self.path
        fsm_layer = self._fsm_layer
        trn_g, path_node, arrow_node = \
            self._draw_transition_real(fsm_layer, path)
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

    def adjust_by_ends(self):
        states = self._states

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
        
        compview = self._compview
        compview.set_transition_path(state_name, trn_cond, new_path)
        pass

    def show_control_points(self):
        if not self._control_points:
            doc = self._doc
            
            c1 = doc.createElement('svg:circle')
            c1.setAttribute('r', '3')
            c1.setAttribute('style', 'stroke: black; stroke-width: 1; '
                            'fill: white')
            l01 = doc.createElement('svg:line')
            l01.setAttribute('style', 'stroke: black; stroke-width: 1; '
                             'stroke-dasharray: 3 2')

            c2 = doc.createElement('svg:circle')
            c2.setAttribute('r', '3')
            c2.setAttribute('style', 'stroke: black; stroke-width: 1; '
                            'fill: white')
            l32 = doc.createElement('svg:line')
            l32.setAttribute('style', 'stroke: black; stroke-width: 1; '
                             'stroke-dasharray: 3 2')

            control_layer = self._control_layer
            
            control_layer.appendChild(c1)
            control_layer.appendChild(l01)
            control_layer.appendChild(c2)
            control_layer.appendChild(l32)
            self._control_points = (c1, l01, c2, l32)
            pass

        c1, l01, c2, l32 = self._control_points
        path = self.path
        c0x, c0y, c1x, c1y, c2x, c2y, c3x, c3y = tuple(path)
        
        c1.setAttribute('cx', str(c1x))
        c1.setAttribute('cy', str(c1y))
        l01.setAttribute('x1', str(c0x))
        l01.setAttribute('y1', str(c0y))
        l01.setAttribute('x2', str(c1x))
        l01.setAttribute('y2', str(c1y))
        
        c2.setAttribute('cx', str(c2x))
        c2.setAttribute('cy', str(c2y))
        l32.setAttribute('x1', str(c3x))
        l32.setAttribute('y1', str(c3y))
        l32.setAttribute('x2', str(c2x))
        l32.setAttribute('y2', str(c2y))
        pass

    def hide_control_points(self):
        if not self._control_points:
            return

        control_layer = self._control_layer
        for node in self._control_points:
            control_layer.removeChild(node)
            pass
        self._control_points = None
        pass

    def start_hint(self):
        path_node = self._path_node
        arrow_node = self._arrow_node
        if path_node:
            path_node.setAttribute('style',
                                   'stroke: #404040; stroke-width: 3; '
                                   'fill: none')
            arrow_node.setAttribute('style',
                                    'stroke: #404040; stroke-width: 2; '
                                    'fill: #404040')
            pass
        pass

    def stop_hint(self):
        path_node = self._path_node
        arrow_node = self._arrow_node
        if path_node:
            path_node.setAttribute('style',
                                   'stroke: #000000; stroke-width: 1; ' \
                                       'fill: none')
            arrow_node.setAttribute('style',
                                    'stroke: #000000; stroke-width: 1; ' \
                                        'fill: #000000')
            pass
        pass
    
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

        trn_g = self.trn_g
        rect = self._selected_rect
        
        px, py, pw, ph = trn_g.getBBox()
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
    pass

class FSM_state(object):
    _doc = None
    _compview = None
    _states = None
    _fsm_layer = None
    _control_layer = None
    state_name = None
    state_g = None
    _text_node = None
    _circle_node = None
    transitions = None
    from_states = None          # There is one or more transitions
                                # from these states (name).

    _state_g_hdl_id = None
    _selected_rect = None
    
    def __init__(self, state_name):
        self.state_name = state_name
        self.transitions = {}
        self.from_states = set()
        pass

    def init(self, doc, compview, states, fsm_layer, control_layer):
        self._doc = doc
        self._compview = compview
        self._states = states
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

        text_node.setAttribute('style', 'stroke: #000000; fill: #000000; '
                               'font-size: 16px')

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
        compview = self._compview
        state_name = self.state_name
        r = compview.get_state_r(state_name)
        return r

    @property
    def xy(self):
        compview = self._compview
        state_name = self.state_name
        xy = compview.get_state_xy(state_name)
        return xy

    @property
    def entry_action(self):
        compview = self._compview
        state_name = self.state_name
        entry_action = compview.get_state_entry_action(state_name)
        return entry_action

    @property
    def all_transitions(self):
        compview = self._compview
        state_name = self.state_name
        conds = compview.all_transitions(state_name)
        return conds

    def _load_transition_compview(self, parent, condition):
        compview = self._compview
        states = self._states
        
        trn = FSM_transition(condition)
        trn.init(self._doc, compview, self, states,
                 self._fsm_layer, self._control_layer)
        trn.draw()
        self.transitions[condition] = trn
        pass

    def draw(self):
        state_name = self.state_name
        fsm_layer = self._fsm_layer

        r = self.r
        x, y = self.xy
        state_g, text_node, text_content, circle_node = \
            self._draw_state_real(fsm_layer, state_name, r, x, y)
        self.state_g = state_g
        self._text_node = text_node
        self._text_content = text_content
        self._circle_node = circle_node

        for trn_cond in self.all_transitions:
            self._load_transition_compview(fsm_layer, trn_cond)
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

    ## \brief Tell states there are transitions to them.
    #
    # This function is only called when loading states of a FSM from
    # compview.  When loading, not all states was loaded that target
    # state may not in the memory.  So, we call this function after
    # all states being loaded.  Transitions added later does need to
    # call this function to notify end state.
    #
    def tell_target_states(self):
        states = self._states
        transitions = self.transitions
        target_state_names = [trn.target for trn in transitions.values()]
        target_states = [states[target_name]
                         for target_name in target_state_names]
        state_name = self.state_name
        for target_state in target_states:
            target_state.from_states.add(state_name)
            pass
        pass

    def rm_target_from_states(self):
        state_name = self.state_name
        
        transitions = self.transitions.values()
        target_names = [trn.target for trn in transitions]

        states = self._states
        for target_name in target_names:
            target_state = states[target_name]
            target_state.from_states.remove(state_name)
            pass
        pass

    def adjust_transitions(self):
        import itertools

        states = self._states
        
        for trn in self.transitions.values():
            trn.adjust_by_ends()
            trn.update()
            pass

        state_name = self.state_name
        from_states = [states[from_state_name]
                      for from_state_name in self.from_states]
        states_transitions = [state.transitions.values()
                              for state in from_states]
        in_state_transitions = [[trn for trn in state_transitions
                                 if trn.target == state_name]
                                for state_transitions in states_transitions]
        in_transitions = itertools.chain(*in_state_transitions)
        for trn in in_transitions:
            trn.adjust_by_ends()
            trn.update()
            pass
        pass

    def add_transition(self, parent, condition):
        self._load_transition_compview(parent, condition)

        transitions = self.transitions
        trn = transitions[condition]
        target_name = trn.target

        state_name = self.state_name
        states = self._states
        target_state = states[target_name]
        target_state.from_states.add(state_name)
        pass

    ## \brief Remove state from from_states of a given target.
    #
    # The state was removed only when there is no transition targeted
    # on given target state.
    #
    def _rm_from_states_for_target(self, target_name):
        transitions = self.transitions
        same_targets = [trn.target for trn in transitions.values()
                        if trn.target == target_name]
        same_target_cnt = len(same_targets)
        if same_target_cnt == 0:
            states = self._states
            target_state = states[target_name]
            state_name = self.state_name
            target_state.from_states.remove(state_name)
            pass
        pass

    def rm_transition(self, condition):
        transitions = self.transitions
        trn = transitions[condition]
        target_name = trn.target
        del transitions[condition]
        trn.clear()

        self._rm_from_states_for_target(target_name)
        pass

    def start_hint(self):
        circle_node = self._circle_node
        circle_node.setAttribute('style', 'stroke: #000000; stroke-width: 3; '
                                 'fill: #ffffff')
        pass

    def stop_hint(self):
        circle_node = self._circle_node
        circle_node.setAttribute('style', 'stroke: #000000; stroke-width: 1; '
                                 'fill: #ffffff')
        pass
    pass


class _select_manager(object):
    selected_state = None
    selected_transition = None
    controlled_transition = None

    def deselect(self):
        pass

    def select_state(self, state):
        self.deselect()
        
        self.selected_state = state
        state.show_selected()
        
        def hide():
            state.hide_selected()
            self.reset()
            pass
        self.deselect = hide
        pass

    def select_transition(self, transition):
        self.deselect()
        
        self.selected_transition = transition
        transition.show_selected()
        
        def hide():
            transition.hide_selected()
            self.reset()
            pass
        self.deselect = hide
        pass

    def control_transition(self, transition):
        self.deselect()
        
        self.controlled_transition = transition
        transition.show_control_points()
        
        def hide():
            transition.hide_control_points()
            self.reset()
            pass
        self.deselect = hide
        pass

    def reset(self):
        try:
            del self.deselect
        except AttributeError:
            pass
        self.selected_state = None
        self.selected_transition = None
        self.controlled_transition = None
        pass
    pass


class _FSM_popup(object):
    _window = None
    _compview = None
    
    _menu_state = None
    _menu_transition = None
    
    _candidate_target = None

    _select = None
    
    def __init__(self, window, compview, select_man):
        super(_FSM_popup, self).__init__()
        self._window = window
        self._compview = compview
        self._select = select_man
        pass

    def _show_state_menu(self, state):
        self._menu_state = state
        window = self._window
        window.popup_state_menu()
        pass

    def _show_transition_menu(self, trn):
        self._menu_transition = trn
        window = self._window
        window.popup_transition_menu()
        pass

    ## \brief Handle mouse events for state objects.
    #
    # This method must be called by mode object to handle mouse events
    # that is not handled by them.
    #
    def _handle_state_mouse_events(self, state, evtype, button, x, y):
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_PRESS and \
                button == 3:
            self._show_state_menu(state)
            self._select.select_state(state)
        elif evtype == pybInkscape.PYSPItem.PYB_EVENT_MOUSE_ENTER:
            state.start_hint()
        elif evtype == pybInkscape.PYSPItem.PYB_EVENT_MOUSE_LEAVE:
            state.stop_hint()
            pass
        pass

    ## \brief Handle mouse events for transition objects.
    #
    # This method must be called by mode object to handle mouse events
    # that is not handled by them.
    #
    def _handle_transition_mouse_events(self, trn, evtype, button, x, y):
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_PRESS and \
                button == 3:
            self._select.select_transition(trn)
            self._show_transition_menu(trn)
        elif evtype == pybInkscape.PYSPItem.PYB_EVENT_MOUSE_ENTER:
            trn.start_hint()
        elif evtype == pybInkscape.PYSPItem.PYB_EVENT_MOUSE_LEAVE:
            trn.stop_hint()
            pass
        pass

    def _handle_select_transition_target(self, state, evtype, button, x, y):
        if self._candidate_target != state and self._menu_state != state:
            if self._candidate_target:
                self._candidate_target.stop_hint()
                pass
            self._candidate_target = state
            state.start_hint()
            pass

        if evtype != pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE:
            return
        if button != 1:
            return
        
        window = self._window
        
        if state == self._menu_state:
            window.pop_grabs()
            return
        
        fsm_layer = window._fsm_layer
        
        target_state = state
        target_name = target_state.state_name
        src_state = self._menu_state
        src_name = src_state.state_name
        cond = ''

        compview = self._compview
        try:
            compview.add_transition(src_name, cond, target_name)
        except:
            import traceback
            traceback.print_exc()
            window.show_error('invalid condition: %s' % (cond))
        else:
            src_state.add_transition(fsm_layer, cond)
            
            trn = src_state.transitions[cond]
            window._install_transition_event_handler(trn)
            pass
        
        window.pop_grabs()

        target_state.stop_hint()
        select = self._select
        select.deselect()
        pass

    def _handle_add_transition(self, *args):
        def restore_bg(item, evtype, *args):
            if evtype != pybInkscape.PYSPItem.PYB_EVENT_BUTTON_PRESS:
                if self._candidate_target:
                    self._candidate_target.stop_hint()
                    self._candidate_target = None
                    pass
                return
            self._select.deselect()
            window.pop_grabs()
            pass
        
        window = self._window
        window.push_grabs()
        window.ungrab_bg()
        window.grab_bg(restore_bg)

        window.ungrab_state()
        window.grab_state(self._handle_select_transition_target)
        self._select.select_state(self._menu_state)
        self._menu_state.stop_hint()
        pass

    def _handle_edit_transition(self, *args):
        trn = self._select.selected_transition

        cond = trn.trn_cond
        action = trn.action or ''

        window = self._window
        window.show_transition_editor(cond, action)
        pass

    def _handle_transition_apply(self, *args):
        trn = self._select.selected_transition
        window = self._window
        compview = self._compview
        transition_cond = window._transition_cond
        transition_action = window._transition_action
        
        trn_cond = trn.trn_cond
        trn_action = trn.action
        trn_state = trn.state
        trn_state_name = trn_state.state_name
        
        new_cond = transition_cond.get_text()
        new_action = transition_action.get_text()

        if new_action != trn_action:
            compview.set_transition_action(trn_state_name,
                                           trn_cond, new_action)
            pass

        if new_cond != trn_cond:
            trn_state.rm_transition(trn_cond)
            
            compview.chg_transition_cond(trn_state_name, trn_cond, new_cond)
            
            fsm_layer = window._fsm_layer
            trn_state.add_transition(fsm_layer, new_cond)
            
            transitions = trn_state.transitions
            new_trn = transitions[new_cond]
            window._install_transition_event_handler(new_trn)
            pass

        window.hide_transition_editor()
        pass

    def _handle_edit_state(self, *args):
        select = self._select
        state = select.selected_state
        name = state.state_name
        r = state.r
        entry_action = state.entry_action
        
        window = self._window
        window.show_state_editor(name, r, entry_action)
        pass

    def _handle_state_change(self):
        window = self._window
        compview = self._compview
        select = self._select

        state_name_txt = window._state_name
        state_radius_txt = window._state_radius
        state_entry_action_txt = window._state_entry_action

        state_name = state_name_txt.get_text()
        state_radius = state_radius_txt.get_text()
        state_entry_action = state_entry_action_txt.get_text()

        state_radius_f = float(state_radius)

        state = select.selected_state
        old_state_name = state.state_name
        if old_state_name != state_name:
            compview.rename_state(old_state_name, state_name)
            state.state_name = state_name
            pass
        
        compview.set_state_r(state_name, state_radius_f)
        compview.set_state_entry_action(state_name, state_entry_action)
        
        state.update()

        window.hide_state_editor()
        select.deselect()
        pass

    def _handle_del_state(self, *args):
        window = self._window
        select = self._select
        compview = self._compview
        
        state = select.selected_state
        state_name = state.state_name

        select.deselect()
        
        states = window._states
        del states[state_name]

        state.clear()

        # Remove out transitions
        transitions = state.transitions
        for trn in transitions.values():
            trn.clear()
            pass

        state.rm_target_from_states()
        
        # Remove in-transition
        src_state_names = state.from_states
        for src_state_name in src_state_names:
            src_state = states[src_state_name]
            in_cond_trns = [(in_cond, in_trn)
                            for in_cond, in_trn in \
                                src_state.transitions.items()
                            if in_trn.target == state_name]
            for in_cond, in_trn in in_cond_trns:
                in_trn.clear()
                del src_state.transitions[in_cond]
                compview.rm_transition(src_state_name, in_cond)
                pass
            pass

        compview.rm_state(state_name)
        pass

    def _handle_del_transition(self, *args):
        window = self._window
        select = self._select
        compview = self._compview

        trn = select.selected_transition
        trn.clear()
        trn_cond = trn.trn_cond
        
        trn_state = trn.state
        trn_state_name = trn_state.state_name
        trn_target_name = trn.target
        target_names = [trn.target for trn in trn_state.transitions.values()]
        
        # the transition must live until getting all info.
        compview.rm_transition(trn_state_name, trn_cond)

        if trn_target_name not in target_names:
            trn_target_state = window._states[trn_target_name]
            trn_target_state.from_states.remove(trn_state_name)
            pass

        del trn_state.transitions[trn_cond]
        pass

    def popup_install_handler(self):
        window = self._window

        window.grab_add_transition(self._handle_add_transition)
        window.grab_edit_transition(self._handle_edit_transition)
        window.grab_edit_state(self._handle_edit_state)
        window.grab_transition_apply(self._handle_transition_apply)
        window.grab_state_apply(self._handle_state_change)
        window.grab_del_state(self._handle_del_state)
        window.grab_del_transition(self._handle_del_transition)
        pass
    pass


class _FSM_move_state_mode(object):
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'

    _popup = None
    
    _window = None
    _compview = None
    
    _select = None

    _on_deactivate = None
    
    def __init__(self, window, compview, select_man):
        super(_FSM_move_state_mode, self).__init__()
        
        self._window = window
        self._compview = compview
        self._locker = compview

        self._popup = _FSM_popup(window, compview, select_man)
        self._select = select_man

        self._on_deactivate = []
        pass

    def _handle_move_state_background(self, item, evtype, button, x, y):
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE:
            self._select.deselect()
            pass
        pass

    def _handle_move_state_state(self, state, evtype, button, x, y):
        window = self._window

        def moving_state(item, evtype, button, x, y):
            if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE:
                window.ungrab_mouse()
                pass
            new_state_x = orign_state_x + x - start_x
            new_state_y = orign_state_y + y - start_y

            compview = self._compview
            compview.set_state_xy(state.state_name, new_state_x, new_state_y)
            state.update()
            state.adjust_transitions()
            state.show_selected()
            pass
        
        window = self._window
        
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_PRESS and \
                button == 1:
            start_x = x
            start_y = y
            orign_state_x, orign_state_y = state.xy
            
            self._select.select_state(state)
            window.grab_mouse(moving_state)
            pass
        elif evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE and \
                button == 1:
            window.ungrab_mouse()
            pass
        else:
            self._popup._handle_state_mouse_events(state, evtype, button, x, y)
            pass
        pass

    ## \brief Install event handler for control points of a transitions.
    #
    def _install_trn_cps_mouse(self, trn):
        c1, l01, c2, l32 = trn._control_points
        path = trn.path
        c0x, c0y, c1x, c1y, c2x, c2y, c3x, c3y = tuple(path)

        state_src = trn.state
        target_name = trn.target
        states = trn._states
        state_target = states[target_name]
        compview = self._compview
        window = self._window
        select = self._select

        def c1_update(rx, ry):
            nc1x = c1x + rx
            nc1y = c1y + ry
            cx, cy = state_src.xy
            r = state_src.r
            
            cv = nc1x - cx, nc1y - cy
            cv_len = math.sqrt(cv[0] ** 2 + cv[1] ** 2)
            nc0x = cx + cv[0] * r / cv_len
            nc0y = cy + cv[1] * r / cv_len
            
            path = list(trn.path)
            path[:4] = [nc0x, nc0y, nc1x, nc1y]

            state_name = state_src.state_name
            cond = trn.trn_cond
            compview.set_transition_path(state_name, cond, path)

            trn.update()
            trn.show_control_points()
            pass

        def c1_start():
            def relay_event(item, evtype, button, x, y):
                c1_dragger.mouse_event(evtype, button, x, y)
                pass
            
            window.push_grabs()
            window.ungrab_bg()
            window.grab_bg(relay_event)
            pass
        
        def c1_stop(rx, ry):
            window.pop_grabs()
            pass
        
        def c2_update(rx, ry):
            nc2x = c2x + rx
            nc2y = c2y + ry
            cx, cy = state_target.xy
            r = state_target.r
            
            cv = nc2x - cx, nc2y - cy
            cv_len = math.sqrt(cv[0] ** 2 + cv[1] ** 2)
            nc3x = cx + cv[0] * r / cv_len
            nc3y = cy + cv[1] * r / cv_len
            
            path = list(trn.path)
            path[4:] = [nc2x, nc2y, nc3x, nc3y]

            state_name = state_src.state_name
            cond = trn.trn_cond
            compview.set_transition_path(state_name, cond, path)

            trn.update()
            trn.show_control_points()
            pass

        def c2_start():
            def relay_event(item, evtype, button, x, y):
                c2_dragger.mouse_event(evtype, button, x, y)
                pass
            
            window.push_grabs()
            window.ungrab_bg()
            window.grab_bg(relay_event)
            pass

        def c2_stop(rx, ry):
            window.pop_grabs()
            pass
        
        c1_dragger = _dragger()
        c1_dragger.update = c1_update
        c1_dragger.start_drag = c1_start
        c1_dragger.stop_drag = c1_stop
        c1_dragger.connect(c1)

        c2_dragger = _dragger()
        c2_dragger.update = c2_update
        c2_dragger.start_drag = c2_start
        c2_dragger.stop_drag = c2_stop
        c2_dragger.connect(c2)
        pass

    ## \brief A transition was selected.
    #
    def _select_transition(self, trn):
        def handle_bg(item, evtype, button, x, y):
            if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_PRESS:
                window.pop_grabs()
                window.ungrab_bg()
                
                self._on_deactivate.pop()
                self._on_deactivate.pop()
                
                select.deselect()
                del self._hint_transition
                pass
            pass
        
        select = self._select
        select.control_transition(trn)
        
        self._hint_transition = lambda trn: None
        trn.stop_hint()
        window = self._window
        window.push_grabs()
        window.ungrab_bg()
        window.grab_bg(handle_bg)
        
        self._install_trn_cps_mouse(trn)
        self._on_deactivate.append(window.pop_grabs)
        def del_hint_transition():
            del self._hint_transition
            pass
        self._on_deactivate.append(del_hint_transition)
        pass

    ## \brief Hint for mouse over a transition.
    #
    def _hint_transition(self, trn):
        def stop_hint(*args):
            trn.stop_hint()
            window.ungrab_bg()
            window.grab_bg(self._handle_move_state_background)
            pass

        trn.start_hint()
        
        window = self._window
        window.ungrab_bg()
        window.grab_bg(stop_hint)
        pass

    def _handle_del_transition(self, *args):
        pass

    ## \brief Handle mouse events when selecting no transition.
    #
    def _handle_transitoin_mouse_events(self, trn, evtype, button, x, y):
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE and \
                button == 1:
            self._select_transition(trn)
        else:
            self._popup._handle_transition_mouse_events(trn, evtype, button,
                                                        x, y)
            pass
        pass

    def activate(self):
        window = self._window
        window._emit_leave_mode()
        window._clear_leave_mode_cb()
        window.ungrab_all()
        
        window.ungrab_bg()
        window.grab_bg(self._handle_move_state_background)
        window.grab_state(self._handle_move_state_state)
        window.grab_transition(self._handle_transitoin_mouse_events)

        self._popup.popup_install_handler()
        pass

    def deactivate(self):
        self._select.deselect()
        while self._on_deactivate:
            deactivate = self._on_deactivate.pop()
            deactivate()
            pass
        pass
    pass


class _FSM_add_state_mode(object):
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'

    _window = None
    _compview = None

    _saved_x = 0
    _saved_y = 0

    _select_state = None
    _candidate_state = None

    _popup = None
    _select = None
    
    def __init__(self, window, compview, select_man):
        super(_FSM_add_state_mode, self).__init__()
        
        self._window = window
        self._compview = compview
        self._locker = compview

        self._select = select_man
        self._popup = _FSM_popup(window, compview, select_man)
        pass

    def _handle_add_new_state(self):
        import traceback
        
        compview = self._compview
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
            compview.add_state(state_name)
        except:
            traceback.print_exc()
            window.show_error('Invalid state name: "%s" is existing' %
                              (state_name))
            return
        compview.set_state_xy(state_name, x, y)
        compview.set_state_r(state_name, r)

        window._load_new_state_incr(state_name)

        window.hide_state_editor()
        pass
    
    def _handle_add_state_background(self, item, evtype, button, x, y):
        window = self._window
        select = self._select
        
        if evtype == pybInkscape.PYSPItem.PYB_EVENT_BUTTON_RELEASE and \
                button == 1:
            self._saved_x = x
            self._saved_y = y
            select.deselect()
            window.show_state_editor()
            pass
        pass

    ## \brief Dispatching state apply event to _FSM_popup or this class.
    #
    # When user change properties of a state, this event is deliveried to
    # _FSM_popup.  Otherwise, dispatch the event to this class.
    #
    def _handle_state_apply(self):
        select = self._select
        if select.selected_state:
            # selected the state while user request to edit a state.
            popup = self._popup
            popup._handle_state_change()
        else:
            # deselected while user click on bg to add a new state.
            self._handle_add_new_state()
            pass
        pass

    def activate(self):
        window = self._window
        
        window._emit_leave_mode()
        window.ungrab_all()
        
        window.grab_bg(self._handle_add_state_background)
        
        #
        # Install event handlers for _FSM_popup
        #
        window.grab_state(self._popup._handle_state_mouse_events)
        window.grab_transition(self._popup._handle_transition_mouse_events)
        self._popup.popup_install_handler()

        #
        # Workaround to make state apply events dispatched to right
        # place according history of user actions.
        #
        # see _handle_state_apply()
        #
        window.ungrab_state_apply()
        window.grab_state_apply(self._handle_state_apply)
        pass

    def deactivate(self):
        if self._select_state:
            self._select_state.hide_selected()
            pass
        if self._candidate_state:
            self._candidate_state.hide_selected()
            self._candidate_state = None
            pass
        pass
    pass

class FSM_window(FSM_window_base):
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'

    _domview = None
    _comp_name = 'main'
    _comview = None

    _background = None
    _fsm_layer = None
    _control_layer = None
    width = 1024
    height = 768

    _grab_mouse_hdl = None
    _bg_hdl = None

    _bg_mouse_event_cb = None
    _leave_mode_cb = None
    _move_state_mode = None
    _add_state_mode = None
    _state_mouse_event_handler = None
    _add_transition_cb = None
    _edit_transition_cb = None
    _transition_apply_cb = None
    _transition_mouse_event_handler = None
    _edit_state_cb = None
    _state_apply_cb = None
    _del_state_cb = None
    _del_transition_cb = None

    _grab_stack = None

    _select = None
    
    def __init__(self, domview_ui, close_cb, destroy_cb):
        super(FSM_window, self).__init__()

        self._locker = domview_ui

        self._domview = domview_ui
        self._compview = _compview(domview_ui, self._comp_name)
        self._states = {}
        
        self._close_cb = close_cb # callback to close editor window (hide)
        self._destroy_cb = destroy_cb # callback to destroy editor window

        _select = _select_manager()
        self._select = _select
        
        self._move_state_mode = \
            _FSM_move_state_mode(self, self._compview, _select)
        self._add_state_mode = \
            _FSM_add_state_mode(self, self._compview, _select)

        self._grab_stack = []
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

        bg_hdl = background.spitem.connect('mouse-event',
                                           self.on_bg_mouse_events)
        self._bg_hdl = bg_hdl
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

    def _make_state_compview(self, state_name):
        compview = self._compview
        doc = self._doc()
        fsm_layer = self._fsm_layer
        states = self._states
        
        state = FSM_state(state_name)
        state.init(doc, compview, states, self._fsm_layer, self._control_layer)
        self._states[state_name] = state

        return state

    def _set_leave_mode_cb(self, callback):
        self._leave_mode_cb = callback
        pass

    def _clear_leave_mode_cb(self):
        self._leave_mode_cb = None
        pass

    def _emit_leave_mode(self):
        if self._leave_mode_cb:
            self._leave_mode_cb()
            self._leave_mode_cb = None
            pass
        pass

    def ungrab_all(self):
        self.ungrab_bg()
        self.ungrab_state()
        self.ungrab_add_transition()
        self.ungrab_transition()
        self.ungrab_edit_transition()
        self.ungrab_edit_state()
        self.ungrab_transition_apply()
        self.ungrab_state_apply()
        self.ungrab_del_state()
        self.ungrab_del_transition()
        pass

    def save_grabs(self):
        save = (self._bg_mouse_event_cb,
                self._state_mouse_event_handler,
                self._transition_mouse_event_handler,
                self._add_transition_cb,
                self._edit_transition_cb,
                self._transition_apply_cb,
                self._edit_state_cb,
                self._state_apply_cb,
                self._del_state_cb,
                self._del_transition_cb)
        return save

    def restore_grabs(self, save):
        self._bg_mouse_event_cb, \
            self._state_mouse_event_handler, \
            self._transition_mouse_event_handler, \
            self._add_transition_cb, \
            self._edit_transition_cb, \
            self._transition_apply_cb, \
            self._edit_state_cb, \
            self._state_apply_cb, \
            self._del_state_cb, \
            self._del_transition_cb \
            = save
        pass

    ## \brief Push current setting of grab handles into the stack.
    #
    def push_grabs(self):
        save = self.save_grabs()
        self._grab_stack.append(save)
        pass

    ## \brief Restore setting of grab handles from the stack.
    #
    def pop_grabs(self):
        save = self._grab_stack.pop()
        self.restore_grabs(save)
        pass

    def on_state_mouse_event(self, state, evtype, button, x, y):
        if self._state_mouse_event_handler:
            self._state_mouse_event_handler(state, evtype, button, x, y)
            pass
        pass

    def _install_state_event_handler(self, state):
        def mouse_event_handler(item, evtype, button, x, y):
            self.on_state_mouse_event(state, evtype, button, x, y)
            pass
        state.grab(mouse_event_handler)
        pass

    def on_transition_mouse_event(self, trn, evtype, button, x, y):
        if self._transition_mouse_event_handler:
            self._transition_mouse_event_handler(trn, evtype, button, x, y)
            pass
        pass

    def _install_transition_event_handler(self, trn):
        def mouse_event_handler(item, evtype, button, x, y):
            self.on_transition_mouse_event(trn, evtype, button, x, y)
            pass
        trn_g = trn.trn_g
        trn_g.spitem.connect('mouse-event', mouse_event_handler)
        pass

    def grab_transition(self, callback):
        assert self._transition_mouse_event_handler is None
        self._transition_mouse_event_handler = callback
        pass

    def ungrab_transition(self):
        self._transition_mouse_event_handler = None
        pass

    def grab_state(self, callback):
        assert self._state_mouse_event_handler is None
        self._state_mouse_event_handler = callback
        pass

    def ungrab_state(self):
        self._state_mouse_event_handler = None
        pass

    def grab_add_transition(self, callback):
        assert self._add_transition_cb is None
        self._add_transition_cb = callback
        pass

    def ungrab_add_transition(self):
        self._add_transition_cb = None
        pass

    def grab_edit_transition(self, callback):
        assert self._edit_transition_cb is None
        self._edit_transition_cb = callback
        pass

    def ungrab_edit_transition(self):
        self._edit_transition_cb = None
        pass

    def grab_transition_apply(self, callback):
        assert self._transition_apply_cb is None
        self._transition_apply_cb = callback
        pass

    def ungrab_transition_apply(self):
        self._transition_apply_cb = None
        pass

    def grab_edit_state(self, callback):
        assert self._edit_state_cb is None
        self._edit_state_cb = callback
        pass

    def ungrab_edit_state(self):
        self._edit_state_cb = None
        pass

    def grab_state_apply(self, callback):
        assert self._state_apply_cb is None
        self._state_apply_cb = callback
        pass

    def ungrab_state_apply(self):
        self._state_apply_cb = None
        pass

    def grab_del_state(self, callback):
        assert self._del_state_cb is None
        self._del_state_cb = callback
        pass
    
    def ungrab_del_state(self):
        self._del_state_cb = None
        pass

    def grab_del_transition(self, callback):
        assert self._del_transition_cb is None
        self._del_transition_cb = callback
        pass

    def ungrab_del_transition(self):
        self._del_transition_cb = None
        pass

    def _draw_new_state(self, state_name):
        states = self._states
        
        state = states[state_name]
        state.draw()
        self._install_state_event_handler(state)

        for trn in state.transitions.values():
            self._install_transition_event_handler(trn)
            pass
        pass

    def _load_new_state(self, state_name):
        state = self._make_state_compview(state_name)
        self._draw_new_state(state_name)
        pass

    ## \brief Load new state incrementally.
    #
    def _load_new_state_incr(self, state_name):
        self._load_new_state(state_name)
        states = self._states
        state = states[state_name]
        state.tell_target_states()
        pass
    
    def _rebuild_from_states(self):
        states = self._states
        compview = self._compview
        state_names = compview.all_state_names()
        for state_name in state_names:
            state = states[state_name]
            self._draw_new_state(state_name)
            state.tell_target_states()
            pass
        pass
    
    def _update_view(self):
        self._clear_view()
        states = self._states
        
        compview = self._compview
        
        state_names = compview.all_state_names()
        for state_name in state_names:
            self._make_state_compview(state_name)
            pass
        self._rebuild_from_states()
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
        self._emit_leave_mode()
        self._close_cb()
        pass
    
    def on_FSM_main_win_destroy_event(self, *args):
        self._emit_leave_mode()
        self._destroy_cb()
        pass
    
    def on_FSM_main_win_delete_event(self, *args):
        self._emit_leave_mode()
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
        if self._state_apply_cb:
            self._state_apply_cb()
            pass
        pass

    def on_add_transition_activate(self, *args):
        if self._add_transition_cb:
            self._add_transition_cb(*args)
            pass
        pass

    def on_edit_transition_activate(self, *args):
        if self._edit_transition_cb:
            self._edit_transition_cb(*args)
            pass
        pass

    def on_transition_apply_clicked(self, *args):
        if self._transition_apply_cb:
            self._transition_apply_cb(*args)
            pass
        pass

    def on_edit_state_activate(self, *args):
        if self._edit_state_cb:
            self._edit_state_cb(*args)
            pass
        pass

    def on_del_state_activate(self, *args):
        if self._del_state_cb:
            self._del_state_cb(*args)
            pass
        pass

    def on_del_transition_activate(self, *args):
        if self._del_transition_cb:
            self._del_transition_cb(*args)
            pass
        pass

    def _install_test_data(self):
        self._init_layers()
        
        compview = self._compview
        
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
        compview.add_state(state1)
        compview.set_state_r(state1, 50)
        compview.set_state_xy(state1, 200, 100)
        state2 = 'state 2'
        compview.add_state(state2)
        compview.set_state_r(state2, 30)
        compview.set_state_xy(state2, 300, 100)
        compview.add_transition(state1, 'event1', state2)
        compview.set_transition_path(state1, 'event1', (200, 150,
                                                       240, 180,
                                                       260, 180,
                                                       300, 130))
        pass

    def show(self):
        if _install_test_data_flag:
            self._install_test_data()
            self._install_test_data = lambda: None
            pass
        
        #
        # Crash if without line.
        # This line remove selection infor to prevent select manager to
        # deselect an object selected in previous session.  It would crash
        # if we don't reset it and compview of previous session were
        # removed.
        #
        self._select.reset()
        
        self._update_view()
        self._add_state_mode.activate()
        super(FSM_window, self).show()
        pass

    def grab_mouse(self, callback):
        assert self._grab_mouse_hdl is None
        
        root = self._root()
        root.setAttribute('inkscape:groupmode', '')
        self._grab_mouse_hdl = root.spitem.connect('mouse-event', callback)
        pass

    def ungrab_mouse(self):
        if not self._grab_mouse_hdl:
            return
        
        root = self._root()
        root.spitem.disconnect(self._grab_mouse_hdl)
        self._grab_mouse_hdl = None
        root.setAttribute('inkscape:groupmode', 'layer')
        pass

    def on_bg_mouse_events(self, item, evtype, button, x, y):
        if not self._bg_mouse_event_cb:
            return

        self._bg_mouse_event_cb(item, evtype, button, x, y)
        pass

    def grab_bg(self, callback):
        assert self._bg_mouse_event_cb is None

        self._bg_mouse_event_cb = callback
        pass

    def ungrab_bg(self):
        if not self._bg_mouse_event_cb:
            return

        self._bg_mouse_event_cb = None
        pass

    def switch_component(self, comp_name):
        self._compview.switch_component(comp_name)
        self._comp_name = comp_name
        pass

    def current_component(self):
        return self._comp_name
    pass

_install_test_data_flag = False

if __name__ == '__main__':
    _install_test_data_flag = True
    win = FSM_window()
    win._main_win.connect('destroy', gtk.main_quit)
    win.show()
    gtk.main()
    pass
