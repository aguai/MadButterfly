import gtk
import os
import data_monitor
import pybInkscape
import FSM_window


## \brief User interface for management components and their timelines.
#
# This class provide base functions to show components and timelines.
#
class comp_dock_base(object):
    def __init__(self, domview_ui, fname=None):
        super(comp_dock_base, self).__init__(domview_ui, fname)
        
        if not fname:
            dirname = os.path.dirname(__file__)
            fname = os.path.join(dirname, 'component_dock.glade')
            pass

        builder = gtk.Builder()
        builder.add_from_file(fname)
        dock_top = builder.get_object('component_dock_top')
        components_model = builder.get_object('components_model')
        timelines_model = builder.get_object('timelines_model')
        components_treeview = builder.get_object('treeview_components')
        components_menu = builder.get_object('components_menu')
        timelines_treeview = builder.get_object('treeview_timelines')
        timelines_menu = builder.get_object('timelines_menu')
        
        dock_top_parent = dock_top.get_parent()
        dock_top_parent.remove(dock_top)

        self._domview_ui = domview_ui
        self._builder = builder
        self._dock_top = dock_top
        self._desktop = None
        self._dock_item = None
        
        self._components_model = components_model
        self._timelines_model = timelines_model
        self._components_treeview = components_treeview
        self._components_menu = components_menu
        self._timelines_treeview = timelines_treeview
        self._timelines_menu = timelines_menu

        self._cur_component = -1
        self._cur_timeline = -1
        pass

    def install_dock(self, desktop):
        self._desktop = desktop

        dock = desktop.getDock()
        item = dock.new_item('component_dock',
                             'Component and timeline manager',
                             'feBlend-icon', dock.ITEM_ST_DOCKED_STATE)
        item_vbox = item.get_vbox()
        self._dock_item = item
        
        item_vbox.pack_start(self._dock_top)
        self._dock_top.show()
        pass

    ## \brief Update the list of components.
    #
    # The cursor is still keeping on the name of current component.
    #
    def refresh_components(self):
        components_model = self._components_model
        components_model.clear()
        
        cur_comp_name = self._domview_ui.get_current_component()
        
        all_comp_names = self._domview_ui.all_comp_names()
        for comp_name in all_comp_names:
            editable = False
            selected = comp_name == cur_comp_name
            icon = 'gtk-apply'
            components_model.append((comp_name, editable, selected, icon))
            pass
        
        cur_comp_idx = all_comp_names.index(cur_comp_name)
        self._components_treeview.set_cursor((cur_comp_idx,))
        pass

    ## \brief Update the list of timelines.
    #
    # The cursor is still keeping on the name of current component.
    #
    def refresh_timelines(self):
        timelines_model = self._timelines_model
        timelines_model.clear()

        cur_tl_name = self._domview_ui.get_current_timeline()
        
        all_timeline_names = self._domview_ui.all_timeline_names()
        for timeline_name in all_timeline_names:
            if timeline_name == cur_tl_name:
                timelines_model.append((timeline_name, False,
                                        True, 'gtk-apply'))
            else:
                timelines_model.append((timeline_name, False,
                                        False, 'gtk-apply'))
                pass
            pass

        cur_tl_idx = all_timeline_names.index(cur_tl_name)
        self._timelines_treeview.set_cursor((cur_tl_idx,))
        pass

    ## \brief Refresh content of component list and timeline list.
    #
    def refresh(self):
        self.refresh_components()
        self.refresh_timelines()
        pass

    def dom_add_component(self, name):
        model = self._components_model
        model.append((name, False, False, 'gtk-apply'))
        pass

    def dom_rm_component(self, name):
        model = self._components_model
        
        itr = model.get_iter_first()
        while itr:
            itr_name = model.get_value(itr, 0)
            if itr_name == name:
                model.remove(itr)
                return
            
            itr = model.iter_next(itr)
            pass
        
        raise ValueError, 'unknown component name - %s' % (name)

    def dom_add_timeline(self, name):
        model = self._timelines_model
        model.append((name, False, False, 'gtk-apply'))
        pass

    def dom_rm_timeline(self, name):
        model = self._timelines_model
        
        itr = model.get_iter_first()
        while itr:
            itr_name = model.get_value(itr, 0)
            if itr_name == name:
                model.remove(itr)
                return
            
            itr = itr.iter_next()
            pass
        
        raise ValueError, 'unknown component name - %s' % (name)
    pass


## \brief UI interactive handlers
#
# A mix-in to handle UI events.
#
class comp_dock_ui(object):
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'

    def __init__(self, domview_ui, fname=None):
        super(comp_dock_ui, self).__init__()
        
        self._locker = domview_ui
        self._fsm_editor_win = None
        pass

    def _drop_undo(self):
        self._doc.commit()
        self._doc.beginTransaction()
        pass

    ## \brief Start handle UI events.
    #
    def start_handle_ui_events(self):
        self._builder.connect_signals(self)
        pass

    def install_dock(self, desktop):
        self._doc = desktop.doc().rdoc
        pass
    
    def _current_component(self):
        treeview = self._components_treeview
        path, col = treeview.get_cursor()

        model = self._components_model
        itr = model.get_iter(path)
        name = model.get_value(itr, 0)
        return name

    def _current_timeline(self):
        treeview = self._timelines_treeview
        path, col = treeview.get_cursor()

        model = self._timelines_model
        itr = model.get_iter(path)
        name = model.get_value(itr, 0)
        return name

    def _add_component(self):
        def _make_component_name():
            comp_name = 'New Component'
            idx = 0
            while comp_name in self._domview_ui.all_comp_names():
                comp_name = 'New Component %d' % (idx)
                idx = idx + 1
                pass
            return comp_name

        comp_name = _make_component_name()
        print comp_name
        self._domview_ui.add_component(comp_name)
        pass

    def _rm_component(self):
        if self._current_component() == 'main':
            return
        
        treeview = self._components_treeview
        path, col = treeview.get_cursor()
        
        model = self._components_model
        itr = model.get_iter(path)
        comp_name = model.get_value(itr, 0)

        self._domview_ui.rm_component(comp_name)
        pass

    ## \brief Hint user that given component is selected.
    #
    # Show an icon before the component name.
    #
    def _hint_sel_component(self, comp_name):
        all_comp_names = self._domview_ui.all_comp_names()
        sel_idx = all_comp_names.index(comp_name)
        assert sel_idx >= 0
        
        for idx in range(len(all_comp_names)):
            model = self._components_model
            itr = model.get_iter((idx,))
            if idx == sel_idx:
                model.set_value(itr, 2, True)
            else:
                model.set_value(itr, 2, False)
                pass
            pass
        pass

    def _switch_component(self):
        domview_ui = self._domview_ui
        
        comp_name = self._current_component()
        domview_ui.switch_component(comp_name)
        
        group = domview_ui.get_layer_group(0)
        desktop = self._desktop # from comp_dock_base
        desktop.setCurrentLayer(group.spitem)

        self._hint_sel_component(comp_name)
        tl_name = self._current_timeline()
        self._hint_sel_timeline(tl_name)
        pass
    
    def _add_timeline(self):
        def _make_timeline_name():
            tl_name = 'New Timeline'
            idx = 0
            while tl_name in self._domview_ui.all_timeline_names():
                tl_name = 'New Timeline %d' % (idx)
                idx = idx + 1
                pass
            return tl_name

        if self._domview_ui.get_current_component() == 'main':
            return

        tl_name = _make_timeline_name()
        print tl_name
        self._domview_ui.add_timeline(tl_name)
        pass

    def _rm_timeline(self):
        if self._current_component() == 'main':
            return
        
        treeview = self._timelines_treeview
        path, col = treeview.get_cursor()
        
        model = self._timelines_model
        itr = model.get_iter(path)
        tl_name = model.get_value(itr, col)
        print tl_name

        self._domview_ui.rm_timeline(tl_name)
        pass

    ## \brief Hint user that given timeline is selected.
    #
    # Show an icon before the timeline name.
    #
    def _hint_sel_timeline(self, tl_name):
        all_tl_names = self._domview_ui.all_timeline_names()
        sel_idx = all_tl_names.index(tl_name)
        assert sel_idx >= 0
        
        for idx in range(len(all_tl_names)):
            model = self._timelines_model
            itr = model.get_iter((idx,))
            if idx == sel_idx:
                model.set_value(itr, 2, True)
            else:
                model.set_value(itr, 2, False)
                pass
            pass
        pass

    def _switch_timeline(self):
        domview_ui = self._domview_ui
        
        timeline_name = self._current_timeline()
        domview_ui.switch_timeline(timeline_name)

        self._hint_sel_timeline(timeline_name)
        pass

    def _prepare_FSM_editor(self):
        def FSM_editor_close():
            self._fsm_editor_win.hide()
            pass
        
        def FSM_editor_destroy():
            self._fsm_editor_win = None
            pass
        
        fsm_win = FSM_window.FSM_window(self._domview_ui,
                                        FSM_editor_close,
                                        FSM_editor_destroy)
        self._fsm_editor_win = fsm_win

        doc = pybInkscape.createSPDocument()
        view_widget = pybInkscape.create_svg_view_widget(doc)
        view_widget.show()

        self._FSM_doc = doc
        self._FSM_view_widget = view_widget

        fsm_win.set_svg_view(view_widget)
        pass

    def _show_FSM_for_comp(self, comp_name):
        if not self._fsm_editor_win:
            self._prepare_FSM_editor()
            fsm_win = self._fsm_editor_win
        else:
            fsm_win = self._fsm_editor_win
            pass
        fsm_win.switch_component(comp_name)
        fsm_win.show()
        pass

    def on_add_comp_clicked(self, *args):
        self._add_component()
        self._drop_undo()
        pass

    def on_remove_comp_clicked(self, *args):
        self._rm_component()
        self._drop_undo()
        pass

    def on_treeview_components_button_press_event(self, widget, event, *args):
        if event.type != gtk.gdk.BUTTON_PRESS:
            return

        if event.button != 3:   # not right button
            return

        self._components_menu.popup(None, None, None, event.button, event.time)
        pass

    def on_treeview_components_row_activated(self, *args):
        self._switch_component()
        self._drop_undo()
        pass

    ## \brief Handle of changing component name.
    #
    def on_cellrenderer_comp_edited(self, renderer, path,
                                    new_text, *args):
        model = self._components_model
        itr = model.get_iter(path)

        old_name = model.get_value(itr, 0)

        model.set_value(itr, 0, new_text)
        model.set_value(itr, 1, False)

        self._domview_ui.rename_component(old_name, new_text)
        
        self._drop_undo()
        pass

    def on_rename_component_activate(self, *args):
        treeview = self._components_treeview
        path, col = treeview.get_cursor()
        
        model = self._components_model
        itr = model.get_iter(path)
        model.set_value(itr, 1, True)

        treeview.set_cursor(path, col, True)
        pass

    def on_link_component_activate(self, *args):
        desktop = self._desktop
        
        comp_name = self._current_component()
        cur_layer_group_sp = desktop.currentLayer()
        cur_layer_group = cur_layer_group_sp.repr
        
        self._domview_ui.link_to_component(comp_name, cur_layer_group)
        
        self._drop_undo()
        pass
    
    def on_switch_component_activate(self, *args):
        self._switch_component()
        self._drop_undo()
        pass

    ## \brief User clicks "State Machine" on context menu for a component.
    #
    def on_edit_FSM_activate(self, *args):
        comp_name = self._current_component()
        self._show_FSM_for_comp(comp_name)
        pass
    
    def on_add_timeline_clicked(self, *args):
        self._add_timeline()
        self._drop_undo()
        pass

    def on_remove_timeline_clicked(self, *args):
        self._rm_timeline()
        self._drop_undo()
        pass

    def on_treeview_timelines_button_press_event(self, widget, event, *args):
        if event.type != gtk.gdk.BUTTON_PRESS:
            return

        if event.button != 3:   # not right button
            return

        self._timelines_menu.popup(None, None, None, event.button, event.time)
        pass

    def on_treeview_timelines_row_activated(self, *args):
        self._switch_timeline()
        self._drop_undo()
        pass

    def on_cellrenderer_timelines_edited(self, renderer, path,
                                         new_text, *args):
        model = self._timelines_model
        itr = model.get_iter(path)
        
        old_name = model.get_value(itr, 0)
        
        model.set_value(itr, 0, new_text)
        model.set_value(itr, 1, False)
        
        self._domview_ui.rename_timeline(old_name, new_text)
        
        self._drop_undo()
        pass
    
    def on_rename_timeline_activate(self, *args):
        treeview = self._timelines_treeview
        path, col = treeview.get_cursor()
        
        model = self._timelines_model
        itr = model.get_iter(path)
        model.set_value(itr, 1, True)

        treeview.set_cursor(path, col, True)
        pass
    
    def on_switch_timeline_activate(self, *args):
        self._switch_timeline()
        self._drop_undo()
        pass
    pass

## \brief Component dock
#
# Mix base functions and event handlers together.
#
class comp_dock(comp_dock_base, comp_dock_ui):
    def __init__(self, domview_ui, fname=None):
        super(comp_dock, self).__init__(domview_ui, fname)
        
        self.start_handle_ui_events()
        pass
    
    def install_dock(self, desktop):
        comp_dock_base.install_dock(self, desktop)
        comp_dock_ui.install_dock(self, desktop)
        pass
    pass
