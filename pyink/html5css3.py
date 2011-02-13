import pybExtension
from domview import component_manager, layers_parser, scenes_parser
from trait import composite

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
    pass


class html5css3_ext(pybExtension.PYBindExtImp):
    def save(self, module, doc, filename):
        parser = dom_parser()
        parser.start_handle(doc.rdoc)
        
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
