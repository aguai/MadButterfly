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
            components_model.append((comp_name,))
            pass

        timelines_model = self._timelines_model
        timelines_model.clear()

        for timeline_name in self._domview_ui.all_timeline_names():
            timelines_model.append((timeline_name,))
            pass
        pass
    
    def on_add_comp_clicked(self, *args):
        print args
        pass

    def on_remove_comp_clicked(self, *args):
        print args
        pass

    def on_treeview_components_cursor_changed(self, *args):
        print args
        pass
    
    def on_add_timeline_clicked(self, *args):
        print args
        pass

    def on_remove_timeline_clicked(self, *args):
        print args
        pass

    def on_treeview_timelines_cursor_changed(self, *args):
        print args
        pass
    pass
