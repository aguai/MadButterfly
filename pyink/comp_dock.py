import gtk
import os
import data_monitor


## \brief User interface for management components and their timelines.
#
class comp_dock(object):
    __metaclass__ = data_monitor.data_monitor
    __data_monitor_prefix__ = 'on_'

    def __init__(self, domview_ui, fname=None):
        super(comp_dock, self).__init__()
        
        if not fname:
            dirname = os.path.dirname(__file__)
            fname = os.path.join(dirname, 'component_dock.glade')
            pass

        builder = gtk.Builder()
        builder.add_from_file(fname)
        builder.connect_signals(self)
        dock_top = builder.get_object('component_dock_top')
        components_model = builder.get_object('components_model')
        timelines_model = builder.get_object('timelines_model')
        components_treeview = builder.get_object('treeview_components')
        timelines_treeview = builder.get_object('treeview_timelines')
        
        dock_top_parent = dock_top.get_parent()
        dock_top_parent.remove(dock_top)

        self._domview_ui = domview_ui
        self._locker = domview_ui
        self._builder = builder
        self._dock_top = dock_top
        self._desktop = None
        self._dock_item = None
        
        self._components_model = components_model
        self._timelines_model = timelines_model
        self._components_treeview = components_treeview
        self._timelines_treeview = timelines_treeview

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

    ## \brief Refresh content of component list and timeline list.
    #
    def refresh(self):
        components_model = self._components_model
        components_model.clear()
        
        for comp_name in self._domview_ui.all_comp_names():
            components_model.append((comp_name, True))
            pass

        timelines_model = self._timelines_model
        timelines_model.clear()

        for timeline_name in self._domview_ui.all_timeline_names():
            timelines_model.append((timeline_name, True))
            pass
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
        print comp_name

        self._domview_ui.rm_component(comp_name)
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

        if self._current_component() == 'main':
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
    
    def on_add_comp_clicked(self, *args):
        self._add_component()
        pass

    def on_remove_comp_clicked(self, *args):
        self._rm_component()
        pass

    def on_treeview_components_cursor_changed(self, *args):
        print args
        pass
    
    def on_cellrenderer_comp_edited(self, renderer, path,
                                    new_text, *args):
        print '%s - %s' % (path, new_text)
        pass
    
    def on_add_timeline_clicked(self, *args):
        self._add_timeline()
        pass

    def on_remove_timeline_clicked(self, *args):
        self._rm_timeline()
        pass

    def on_treeview_timelines_cursor_changed(self, *args):
        print args
        pass

    def on_cellrenderer_timelines_edited(self, renderer, path,
                                         new_text, *args):
        print '%s - %s' % (path, new_text)
        pass
    pass
