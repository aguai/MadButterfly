## \file
# \brief MadButterfly with bbox.
#
# This module export a document with bounding box information.
#
import pybExtension

class mbbbox_ext(pybExtension.PYBindExtImp):
    def _translate_attr(self, node, attrname, attrvalue):
        if attrname == 'xlink:href' and attrvalue.startswith('file://'):
            # translate to relative path
            from os.path import relpath, dirname, realpath, abspath
            
            fn = abspath(attrvalue[7:])
            fn = realpath(fn)
            
            doc_name = abspath(self._doc_name)
            doc_name = realpath(doc_name)
            doc_dir = dirname(doc_name)
            
            relfn = relpath(fn, doc_dir)
            
            if not relfn.startswith('../'):
                attrvalue = relfn
                pass
            pass
        
        return attrname, attrvalue
    
    def _print_subtree(self, node, lvl, out):
        def _print_level(txt, lvl, out):
            indent = '    ' * lvl
            print >> out, '%s%s' % (indent, txt)
            pass
        
        def _print_node_open(node, lvl, out):
            node_name = node.name()
            
            attrs = []
            if node_name == 'svg:svg':
                attrs.append('xmlns:dc="http://purl.org/dc/elements/1.1/"')
                attrs.append('xmlns:cc="http://creativecommons.org/ns#"')
                attrs.append('xmlns:rdf='
                             '"http://www.w3.org/1999/02/22-rdf-syntax-ns#"')
                attrs.append('xmlns:svg="http://www.w3.org/2000/svg"')
                attrs.append('xmlns="http://www.w3.org/2000/svg"')
                attrs.append('xmlns:xlink="http://www.w3.org/1999/xlink"')
                attrs.append('xmlns:sodipodi="http://'
                             'sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"')
                attrs.append('xmlns:inkscape='
                             '"http://www.inkscape.org/namespaces/inkscape"')
                pass

            for attrname in node.allAttributes():
                attrvalue = node.getAttribute(attrname)
                attrname, attrvalue = \
                    self._translate_attr(node, attrname, attrvalue)
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
                attrname, attrvalue = \
                    self._translate_attr(node, attrname, attrvalue)
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
            self._print_subtree(child, lvl + 1, out)
            pass
        _print_node_close(node, lvl, out)
        pass
    
    def save(self, module, doc, filename):
        self._doc_name = filename
        out = file(filename, 'w+')
        
        print >>out, '<?xml version="1.0" encoding="UTF-8" standalone="no"?>'
        root = doc.rdoc.root()
        self._print_subtree(root, 0, out)
    
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
