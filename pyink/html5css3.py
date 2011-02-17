import pybExtension
from domview import component_manager, layers_parser, scenes_parser
from trait import composite


def _scene_node_range(node):
    start = node.getAttribute('start')
    try:
        end = node.getAttribute('end')
    except:
        end = start
        pass
    try:
        scene_type = node.getAttribute('type')
    except:
        scene_type = 'normal'
        pass
    
    return int(start), int(stop), scene_type


@composite
class dom_parser(object):
    use_traits = (component_manager, layers_parser, scenes_parser)
    method_map_traits = {
        scenes_parser._find_maxframe: '_find_maxframe',
        scenes_parser._collect_all_scenes: '_collect_all_scenes',
        scenes_parser._collect_node_ids: '_collect_node_ids',
        component_manager._start_component_manager:
            '_start_component_manager'
        }

    def __init__(self):
        self._metadata_node = None
        self._scenes_node = None
        
        self._doc = None
        self._root = None
        self._layers = []
        self._layers_parent = None
        self._maxframe = 0
        self.current = 0
        self._id2node = {}
        self._group2scene = {}
        pass

    def _find_meta(self):
        for child in self._root.childList():
            if child.name() == 'svg:metadata':
                self._metadata_node = child
                break
            pass
        else:
            raise RuntimeError, 'can not find \'svg:metadata\' node'

        for child in self._metadata_node.childList():
            if child.name() == 'ns0:scenes':
                self._scenes_node = child
                break
            pass
        else:
            raise RuntimeError, \
                'can not find \'ns0:scenes\' node under \'svg:metadata\''

        pass

    def start_handle(self, doc):
        self._doc = doc
        self._root = doc.root()
        self._layers_parent = self._root

        self._find_meta()
        
        self._collect_node_ids()
        self._collect_all_scenes()

        self.parse_all_layers()

        self._start_component_manager()
        pass

    def reset(self):
        self.reset_layers()
        pass

    def get_maxframe(self):
        return self._maxframe

    def _find_scene_node(self, frame_idx, layer_idx):
        layer = self._layers[layer_idx]
        for scene_node in layer.scenes:
            start, end, scene_type = _scene_node_range(scene_node)
            if start <= frame_idx and frame_idx <= end:
                return scene_node
            pass
        pass
    
    def get_scene(self, frame_idx, layer_idx):
        scene_node = self._find_scene_node(frame_idx, layer_idx)
        if scene_node:
            start, end, scene_type = _scene_node_range(scene_node)
            return start, end, scene_type
        return None

    def get_scene_group(self, frame_idx, layer_idx):
        scene_node = self._find_scene_node(frame_idx, layer_idx)
        if scene_node:
            gid = scene_node.getAttribute('ref')
            scene_group = self.get_node(gid)
            return scene_group
        return None
    pass


def _print_level(txt, lvl, out):
    indent = '    ' * lvl
    print >> out, '%s%s' % (indent, txt)
    pass

def _print_node_open(node, lvl, out):
    attrs = []
    for attrname in node.allAttributes():
        attrvalue = node.getAttribute(attrname)
        attr = '%s="%s"' % (attrname, attrvalue)
        attrs.append(attr)
        pass
    
    if attrs:
        attrs_str = ' '.join(attrs)
        line = '<%s %s>' % (node.name(), attrs_str)
    else:
        line = '<%s>' % (node.name())
        pass
    _print_level(line, lvl, out)
    pass

def _print_node_close(node, lvl, out):
    line = '</%s>' % (node.name())
    _print_level(line, lvl, out)
    pass

def _print_node_single(node, lvl, out):
    attrs = []
    for attrname in node.allAttributes():
        attrvalue = node.getAttribute(attrname)
        attr = '%s="%s"' % (attrname, attrvalue)
        attrs.append(attr)
        pass
    
    if attrs:
        attrs_str = ' '.join(attrs)
        line = '<%s %s/>' % (node.name(), attrs_str)
    else:
        line = '<%s/>' % (node.name())
        pass
    _print_level(line, lvl, out)
    pass

def _print_node_content(node, lvl, out):
    line = node.content()
    _print_level(line, lvl, out)
    pass

def _print_subtree(node, lvl, out):
    children = node.childList()
    if not children:
        if node.name() != 'string':
            _print_node_single(node, lvl, out)
        else:
            _print_node_content(node, lvl, out)
            pass
        return

    _print_node_open(node, lvl, out)
    for child in children:
        _print_subtree(child, lvl + 1, out)
        pass
    _print_node_close(node, lvl, out)
    pass

class html5css3_ext(pybExtension.PYBindExtImp):
    def save(self, module, doc, filename):
        import sys
        parser = dom_parser()
        parser.start_handle(doc.rdoc)
        
        print parser._maxframe
        print doc.rdoc.root().allAttributes()
        print parser.all_comp_names()
        print parser._layers
        print 'save to ' + filename
        pass
    pass

extension = (html5css3_ext(),
             'net.scribboo.html5css3',
             'HTML5/CSS3 exporter',
             'output',
             {'extension': '.html',
              'mimetype': 'text/html',
              '_filetypename': 'HTML5/CSS3 (*.html)'})
