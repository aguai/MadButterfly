import gtk
import os

## \brief User interface for management components and their timelines.
#
class comp_dock(gtk.VBox):
    def __init__(self, fname=None):
        super(comp_dock, self).__init__()
        
        if not fname:
            dirname = os.path.dirname(__file__)
            fname = os.path.join(dirname, 'component_dock.glade')
            print fname
            pass

        builder = gtk.Builder()
        builder.add_from_file(fname)
        dock_top = builder.get_object('component_dock_top')
        dock_top_parent = dock_top.get_parent()
        dock_top_parent.remove(dock_top)
        self.pack_start(dock_top)
        dock_top.show()

        self._builder = builder
        self._dock_top = dock_top
        self._desktop = None
        self._dock_item = None
        pass

    def install_dock(self, desktop):
        self._desktop = desktop

        dock = desktop.getDock()
        item = dock.new_item('component_dock',
                             'Component and timeline manager',
                             'feBlend-icon', dock.ITEM_ST_DOCKED_STATE)
        item_vbox = item.get_vbox()
        item_vbox.pack_start(self)
        self._dock_item = item
        
        self.show()
        pass
    pass
