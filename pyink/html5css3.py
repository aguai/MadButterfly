import pybExtension
from domview import component_manager, layers_parser, scenes_parser
from trait import composite, trait, require


@composite
class dom_parser(object):
    use_traits = (component_manager, layers_parser, scenes_parser)
    provide_traits = {layers_parser.get_scene: '_scenes_get_scene'}
    method_map_traits = {
        scenes_parser._find_maxframe: '_find_maxframe',
        scenes_parser._collect_all_scenes: '_collect_all_scenes',
        scenes_parser._collect_node_ids: '_collect_node_ids',
        component_manager._start_component_manager:
            '_start_component_manager',
        scenes_parser.get_scene: '_scenes_get_scene'
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

    def _find_metadata(self):
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
    
    ## \brief Return the range of a scene node.
    #
    @staticmethod
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
        
        return int(start), int(end), scene_type

    ## \brief To handle the parsing for the given document.
    #
    def start_handle(self, doc):
        self._doc = doc
        self._root = doc.root()
        self._layers_parent = self._root

        self._find_metadata()
        
        self._collect_node_ids()
        self._collect_all_scenes()

        self.parse_all_layers()

        self._start_component_manager()
        pass

    ## \brief Reset the content of the parser before next parsing.
    #
    def reset(self):
        self.reset_layers()
        pass

    def get_maxframe(self):
        return self._maxframe

    def _find_scene_node(self, frame_idx, layer_idx):
        layer = self._layers[layer_idx]
        for scene_node in layer.scenes:
            start, end, scene_type = self._scene_node_range(scene_node)
            if start <= frame_idx and frame_idx <= end:
                return scene_node
            pass
        pass
    
    def get_scene(self, frame_idx, layer_idx):
        scene_node = self._find_scene_node(frame_idx, layer_idx)
        if scene_node:
            start, end, scene_type = self._scene_node_range(scene_node)
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


#
# Following functions are used to print XML from a DOM-tree.
#
def _print_level(txt, lvl, out):
    indent = '    ' * lvl
    print >> out, '%s%s' % (indent, txt)
    pass

def _print_node_open(node, lvl, out):
    node_name = node.name()
    if node_name.startswith('svg:'):
        node_name = node_name[4:]
        pass
    
    attrs = []
    for attrname in node.allAttributes():
        attrvalue = node.getAttribute(attrname)
        attr = '%s="%s"' % (attrname, attrvalue)
        attrs.append(attr)
        pass
    
    if attrs:
        attrs_str = ' '.join(attrs)
        line = '<%s %s>' % (node_name, attrs_str)
    else:
        line = '<%s>' % (node_name)
        pass
    _print_level(line, lvl, out)
    pass

def _print_node_close(node, lvl, out):
    node_name = node.name()
    if node_name.startswith('svg:'):
        node_name = node_name[4:]
        pass
    
    line = '</%s>' % (node_name)
    _print_level(line, lvl, out)
    pass

def _print_node_single(node, lvl, out):
    node_name = node.name()
    if node_name.startswith('svg:'):
        node_name = node_name[4:]
        pass
    
    attrs = []
    for attrname in node.allAttributes():
        attrvalue = node.getAttribute(attrname)
        attr = '%s="%s"' % (attrname, attrvalue)
        attrs.append(attr)
        pass
    
    if attrs:
        attrs_str = ' '.join(attrs)
        line = '<%s %s/>' % (node_name, attrs_str)
    else:
        line = '<%s/>' % (node_name)
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


## \brief CSS3 animation code generator.
#
# This trait parses and generate transition code, in CSS3, for two
# nodes.
#
@trait
class css3_ani_gen(object):
    _parse_ani_attrs = require
    
    _passing = lambda name, value: (name, str(value))
    _trans_transform = lambda name, value: \
        (name, 'matrix(' + ','.join([str(e) for e in value[0]]) + ')')
    
    _translators = {'x': _passing, 'y': _passing,
                   'width': _passing, 'height': _passing,
                   'opacity': _passing, 'transform': _trans_transform
                   }
    
    del _trans_transform
    del _passing
    
    @staticmethod
    def _ani_attr_to_css3(attrname, attrvalue, css):
        translator = css3_ani_gen._translators[attrname]
        cssname, cssvalue = translator(attrname, attrvalue)
        css[cssname] = cssvalue
        pass

    def _make_css3_transition(self, node1, node2, duration):
        from tween import _normalize_attrs
        
        ani_attrs1 = self._parse_ani_attrs(node1)
        ani_attrs2 = self._parse_ani_attrs(node2)

        _normalize_attrs(node1, ani_attrs1, node2, ani_attrs2)
        
        css = {}
        
        attrnames = set(ani_attrs1.keys() + ani_attrs2.keys())
        for attrname in attrnames:
            attrvalue = ani_attrs2[attrname]
            self._ani_attr_to_css3(attrname, attrvalue, css)
            pass
        
        properties = css.keys()
        css['transition-property'] = ', '.join(properties)
        css['transition-duration'] = '%gs' % (duration)
        
        return css

    def _write_css(self, selector, css_props, out):
        print >> out, '%s {' % (selector)
        for prop_name, prop_value in css_props.items():
            print >> out, '    %s: %s;' % (prop_name, prop_value)
            pass
        print >> out, '}'
        pass
    pass


@composite
class html5css3_ext(pybExtension.PYBindExtImp):
    use_traits = (css3_ani_gen,)
    method_map_traits = {css3_ani_gen._make_css3_transition:
                             '_make_css3_transition',
                         css3_ani_gen._write_css: '_write_css'
                         }

    _frame_rate = 12

    def __init__(self):
        self._stylesheet = {}
        pass
    
    @staticmethod
    def _parse_ani_attrs(node):
        from tween import _parse_attr_ani, _parse_style_ani        
        attrs = {}
        _parse_attr_ani(node, attrs)
        _parse_style_ani(node, attrs)
        return attrs

    def save(self, module, doc, filename):
        import sys
        parser = dom_parser()
        self._parser = parser
        parser.start_handle(doc.rdoc)
        
        self._handle_transition_layers()
        
        out = file(filename, 'w+')
        print >> out, '''\
<html
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:cc="http://creativecommons.org/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns:xlink="http://www.w3.org/1999/xlink"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   xmlns:ns0="http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd"
   >
<head>
<title>Scribboo Test Page</title>
<!-- Style for animation transitions -->
<style type='text/css'>'''

        for selector, style in self._stylesheet.items():
            self._write_css(selector, style, out)
            pass
        
        print >> out, '''\
</style>
<script>'''

        print >>out, 'var animation_info = '
        import pprint
        pprint.pprint(self._stylesheet, stream=out, indent=4, depth=2)

        print >> out, '''\
</script>
</head>
<body>'''

        root = doc.rdoc.root()
        _print_subtree(root, 1, out)

        print >> out, '''\
</body>
</html>'''

        out.close()
        pass

    ## \brief Find all animation pairs.
    #
    # An animation pair is two nodes, one from start scene group and
    # another from stop scene group.  The node from start scene group
    # will transite to the state of the node from stop scene group.
    # This method is responsible for finding all pairs.
    #
    @staticmethod
    def _find_transition_pairs(start_scene_group, stop_scene_group):
	# Collect all nodes in stop scene
	stop_nodes = {}
	node = stop_scene_group.firstChild()
	while node:
	    try:
		node_label = node.getAttribute("ns0:duplicate-src")
		stop_nodes[node_label] = node
	    except:
		pass
	    node = node.next()
	    pass
	
	# Collect all nodes in start scene
	start_nodes = {}
	node = start_scene_group.firstChild()
	while node:
	    try:
		node_label = node.getAttribute("id")
		start_nodes[node_label] = node
	    except:
		pass
	    node = node.next()
	    pass

        pairs = []
        for start_node in start_scene_group.childList():
            start_node_id = start_node.getAttribute('id')
            try:
                stop_node = stop_nodes[start_node_id]
            except:
                stop_node = start_node
                pass
            pairs.append((start_node, stop_node))
            pass
        
        return pairs

    def _make_scene_group_style(self, frame_idx, layer_idx):
        scene_group = self._parser.get_scene_group(frame_idx, layer_idx)
        gid = scene_group.getAttribute('id')
        selector = '.frame%04d #%s' % (frame_idx, gid)
        style = {'display': 'inline'}
        self._stylesheet[selector] = style
        pass

    def _handle_transition_layer(self, layer_idx):
        parser = self._parser
        maxframe = parser.get_maxframe()
        stylesheet = self._stylesheet

        frame_idx = 0
        while frame_idx < maxframe:
            scene = parser.get_scene(frame_idx, layer_idx)
            if not scene:
                frame_idx = frame_idx + 1
                continue
            
            self._make_scene_group_style(frame_idx, layer_idx)
            
            start, end, tween_type = scene
            if start == end:
                frame_idx = frame_idx + 1
                continue
            
            next_start = end + 1
            scene = parser.get_scene(next_start, layer_idx)
            if not scene:
                frame_idx = next_start + 1
                continue

            scene_group = parser.get_scene_group(start, layer_idx)
            next_scene_group = parser.get_scene_group(next_start, layer_idx)

            duration = float(next_start - start) / self._frame_rate
            
            #
            # Generate style for every transition pair
            #
            transition_pairs = \
                self._find_transition_pairs(scene_group, next_scene_group)
            for start_node, stop_node in transition_pairs:
                css_props = self._make_css3_transition(start_node,
                                                       stop_node,
                                                       duration)
                node_id = start_node.getAttribute('id')
                selector = '.frame%04d #%s' % (start, node_id)
                stylesheet[selector] = css_props
                pass

            frame_idx = next_start
            pass
        pass

    def _handle_transition_layers(self):
        num_layers = self._parser.get_layer_num()
        for layer_idx in range(num_layers):
            self._handle_transition_layer(layer_idx)
            pass
        pass
    pass

extension = (html5css3_ext(),
             'net.scribboo.html5css3',
             'HTML5/CSS3 exporter',
             'output',
             {'extension': '.html',
              'mimetype': 'text/html',
              '_filetypename': 'HTML5/CSS3 (*.html)'})
