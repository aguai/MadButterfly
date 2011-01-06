import os

try:
    if os.environ['PYINK_DBG_ENABLE'] == 'yes':
        import pdb
        pdb.set_trace()
        pass
    pass
except:
    pass

import pybInkscape
import pygtk
import gtk
from MBScene import *
global ink_inited
ink_inited=0
def start_desktop(inkscape,ptr):
    global ink_inited
    if ink_inited == 1:
    	desktop = pybInkscape.GPointer_2_PYSPDesktop(ptr)
	top = desktop.getToplevel()
    	#dock = desktop.getDock()
    	#item = dock.new_item("scene", "scene", "feBlend-icon", dock.ITEM_ST_DOCKED_STATE)
    	scene = MBScene(desktop,top)
    	scene.show()
        return
        

    ink_inited = 1


def pyink_start():
    print 'pyink_start()'
    pybInkscape.inkscape.connect('activate_desktop', start_desktop)
    pass
