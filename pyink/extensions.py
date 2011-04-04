import pybExtension
import html5css3, mbbbox

_all_extensions = [html5css3.extension, mbbbox.extension]

_DEBUG_FLAG_NAME = 'PYINK_EXT_DBG_ENABLE'

def _reg_extensions():
    import os

    if os.environ.has_key(_DEBUG_FLAG_NAME):
        debug_level = int(os.environ[_DEBUG_FLAG_NAME])
    else:
        debug_level = 0
        pass
    
    if debug_level > 0:
        print 'Loading extensions'
        pass
    
    for ext_imp, ext_id, ext_name, ioe_name, ioe_items in _all_extensions:
        if debug_level > 0:
            print '    register %s -- %s' % (ext_id, ext_name)
            pass
        
        #
        # ioe_items is description items for input, output, and effect.
        #
        if ioe_name not in ('input', 'output', 'effect'):
            raise ValueError, 'invalid extension type (%s)' % (ioe_name)
        
        kws = {ioe_name: ioe_items}
        pybExtension.register_extension(ext_imp, ext_id, ext_name, **kws)
        pass
    pass

def initial():
    _reg_extensions()
    pass
