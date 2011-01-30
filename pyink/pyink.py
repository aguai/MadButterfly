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

all_desktop_mbscenes = {}

def _init_mbscene(inkscape, ptr):
    global all_desktop_mbscenes
    
    desktop = pybInkscape.GPointer_2_PYSPDesktop(ptr)

    top = desktop.getToplevel()
    mbscene = MBScene(desktop,top)
    mbscene.show()

    all_desktop_mbscenes[desktop] = mbscene

    print hash(desktop)
    pass


## \brief Handler for events of activating a desktop.
#
def act_desktop(inkscape, ptr):
    # Get Python wrapper of SPDesktop passed by ptr.
    desktop = pybInkscape.GPointer_2_PYSPDesktop(ptr)
    
    top = desktop.getToplevel()
    if not top:                 # has not top window.
        return

    if desktop in all_desktop_mbscenes:
        return                  # already initialized
    
    _init_mbscene(inkscape, ptr)
    pass


def pyink_start():
    pybInkscape.inkscape.connect('activate_desktop', act_desktop)
    pass


def pyink_context_menu(view, item, menu_factory):
    print hash(view)
    if view in all_desktop_mbscenes:
        mbscene = all_desktop_mbscenes[view]
        mbscene.context_menu(item, menu_factory)
        pass
    pass
