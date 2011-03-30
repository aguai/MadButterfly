import pybExtension

def _print_subtree(node, lvl, out):
    def _print_level(txt, lvl, out):
        indent = '    ' * lvl
        print >> out, '%s%s' % (indent, txt)
        pass

    def _print_node_open(node, lvl, out):
        node_name = node.name()
    
        attrs = []
        for attrname in node.allAttributes():
            attrvalue = node.getAttribute(attrname)
	    if attrname[0:13] == 'inkscape:bbox':
	        continue
            attr = '%s="%s"' % (attrname, attrvalue)
            attrs.append(attr)
            pass
        
        parent_node = node.parent()
        if parent_node:
            parent_name = parent_node.name()
            if parent_name == 'svg:g':
                bbox = node.getBBox()
                attr = 'inkscape:bbox-x="%f"' % (bbox[0])
                attrs.append(attr)
                bbox = node.getBBox()
                attr = 'inkscape:bbox-y="%f"' % (bbox[1])
                attrs.append(attr)
                bbox = node.getBBox()
                attr = 'inkscape:bbox-width="%f"' % (bbox[2])
                attrs.append(attr)
                bbox = node.getBBox()
                attr = 'inkscape:bbox-height="%f"' % (bbox[3])
                attrs.append(attr)
                pass
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
        
        line = '</%s>' % (node_name)
        _print_level(line, lvl, out)
        pass
    
    def _print_node_single(node, lvl, out):
        node_name = node.name()
        
        attrs = []
        for attrname in node.allAttributes():
	    if attrname[0:13] == 'inkscape:bbox':
	        continue
            attrvalue = node.getAttribute(attrname)
            attr = '%s="%s"' % (attrname, attrvalue)
            attrs.append(attr)
            pass

        parent_node = node.parent()
        if parent_node:
            parent_name = parent_node.name()
            if parent_name == 'svg:g':
                bbox = node.getBBox()
                attr = 'inkscape:bbox-x="%f"' % (bbox[0])
                attrs.append(attr)
                bbox = node.getBBox()
                attr = 'inkscape:bbox-y="%f"' % (bbox[1])
                attrs.append(attr)
                bbox = node.getBBox()
                attr = 'inkscape:bbox-width="%f"' % (bbox[2])
                attrs.append(attr)
                bbox = node.getBBox()
                attr = 'inkscape:bbox-height="%f"' % (bbox[3])
                attrs.append(attr)
                pass
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

class mbbbox_ext(pybExtension.PYBindExtImp):
    def save(self, module, doc, filename):
        out = file(filename, 'w+')
        
        root = doc.rdoc.root()
        _print_subtree(root, 0, out)

        out.close()
        pass
    pass


extension = (mbbbox_ext(),
             'net.scribboo.mbbbox',
             'SVG with BBox exporter',
             'output',
             {'extension': '.svg',
              'mimetype': 'image/svg+xml',
              '_filetypename': 'SVG+BBox (*.svg)'})
