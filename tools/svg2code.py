#! /usr/bin/env python
from xml.dom.minidom import parse
import sys

svgns='http://www.w3.org/2000/svg'
xlinkns='http://www.w3.org/1999/xlink'

def translate_stops(parent, codefo, parent_id):
    stops = []
    for node in parent.childNodes:
        if node.localName == 'stop' and node.namespaceURI == svgns:
            style = node.getAttribute('style')
            style_props = [prop.strip() for prop in style.split(';')
                           if prop.strip()]
            style_kvpairs = [prop.split(':') for prop in style_props]
            style_kvpairs = [(prop[0].strip(), prop[1].strip())
                             for prop in style_kvpairs]
            style_map = dict(style_kvpairs)

            color = style_map['stop-color'].strip()
            if len(color) == 7 and color[0] == '#':
                r = float(int(color[1:3], 16)) / 255.0
                g = float(int(color[3:5], 16)) / 255.0
                b = float(int(color[5:7], 16)) / 255.0
            else:
                raise ValueError, '\'%s\' is invalid color value.' % (color)

            opacity = style_map['stop-opacity']
            offset = node.getAttribute('offset')
            stops.append('[COLOR_STOP([%s], %f, %f, %f, %f, %f)]' % (
                parent_id, r, g, b, float(opacity), float(offset)))
            pass
        pass
    print >> codefo, '%sdnl' % (', '.join(stops))
    pass

def translate_linearGradient(linear, codefo, doc):
    linear_id = linear.getAttribute('id')
    if linear.hasAttribute('x1'):
        x1 = float(linear.getAttribute('x1'))
        y1 = float(linear.getAttribute('y1'))
        x2 = float(linear.getAttribute('x2'))
        y2 = float(linear.getAttribute('y2'))
    else:
        x1 = y1 = x2 = y2 = 0
        pass
    print >> codefo, 'ADD_LINEAR_PAINT([%s], %f, %f, %f, %f, [' % (
        linear_id, x1, y1, x2, y2)
    translate_stops(linear, codefo, linear_id)
    print >> codefo, '])dnl'

    href = linear.getAttributeNS(xlinkns, 'href').strip()
    if href and href[0] == '#':
        print >> codefo, 'REF_STOPS([%s], [%s])dnl' % (linear_id, href[1:])
        pass
    pass

def translate_radialGradient(radial, codefo, doc):
    radial_id = radial.getAttribute('id')
    try:
        cx = float(radial.getAttribute('cx'))
        cy = float(radial.getAttribute('cy'))
    except:
        cx = cy = 0
        pass
    try:
        r = float(radial.getAttribute('r'))
    except:
        r = 0.5
        pass
    print >> codefo, 'ADD_RADIAL_PAINT([%s], %f, %f, %f, [' % (
        radial_id, cx, cy, r)
    translate_stops(radial, codefo, radial_id)
    print >>codefo, '])dnl'

    href = radial.getAttributeNS(xlinkns, 'href').strip()
    if href[0] == '#':
        print >> codefo, 'REF_STOPS([%s], [%s])dnl' % (radial_id, href[1:])
        pass
    pass

def translate_defs(defs, codefo, doc):
    for node in defs.childNodes:
        if node.namespaceURI != svgns:
            continue
        if node.localName == 'linearGradient':
            translate_linearGradient(node, codefo, doc)
            pass
        elif node.localName == 'radialGradient':
            translate_radialGradient(node, codefo, doc)
            pass
        pass
    pass

def trans_color(code):
    return int(code[1:3], 16) / 255.0, \
        int(code[3:5], 16) / 255.0, \
        int(code[5:7], 16) / 255.0

def translate_style(node, coord_id, codefo, doc):
    node_id = node.getAttribute('id')
    style_str = node.getAttribute('style')
    prop_strs = [s.strip() for s in style_str.split(';')]
    prop_kvs = [s.split(':') for s in prop_strs if s]
    prop_kvs = [(k.strip(), v.strip()) for k, v in prop_kvs]
    prop_map = dict(prop_kvs)

    try:
        opacity = float(node.getAttribute('opacity'))
    except:
        opacity = 1.0
        pass

    if prop_map.has_key('fill'):
        fill = prop_map['fill'].strip()
        if fill.startswith('#') and len(fill) == 7:
            r, g, b = trans_color(fill)
            print >> codefo, 'FILL_SHAPE([%s], %f, %f, %f, %f)dnl' % (
                node_id, r, g, b, opacity)
        elif fill.startswith('url(') and fill.endswith(')'):
            paint_id = fill[5:-1]
            print >> codefo, 'FILL_SHAPE_WITH_PAINT([%s], [%s])dnl' % (
                node_id, paint_id)
        else:
            raise ValueError, '\'%s\' is an invalid value for fill.' % (fill)
        pass

    try:
        stroke_opacity = float(node.getAttribute('stroke-opacity'))
    except:
        stroke_opacity = 1.0
        pass

    if prop_map.has_key('stroke'):
        stroke = prop_map['stroke'].strip()
        if stroke.startswith('#') and len(stroke) == 7:
            r, g, b = trans_color(stroke)
            print >> codefo, 'STROKE_SHAPE([%s], %f, %f, %f, %f)dnl' % (
                node_id, r, g, b, stroke_opacity)
        elif stroke.startswith('url(') and stroke.endswith(')'):
            paint_id = stroke[5:-1]
            print >> codefo, 'STROKE_SHAPE_WITH_PAINT([%s], [%s])dnl' % (
                node_id, paint_id)
        else:
            raise ValueError, '\'%s\' is an invalid value for stroke.' \
                % (stroke)
        pass
    pass

def translate_path(path, coord_id, codefo, doc):
    path_id = path.getAttribute('id')
    d = path.getAttribute('d')
    print >> codefo, 'dnl'
    print >> codefo, 'ADD_PATH([%s], [%s], [%s])dnl' % (path_id, d, coord_id)
    translate_style(path, coord_id, codefo, doc)
    pass

def translate_rect(rect, coord_id, codefo, doc):
    rect_id = rect.getAttribute('id')
    x = float(rect.getAttribute('x'))
    y = float(rect.getAttribute('y'))
    width = float(rect.getAttribute('width'))
    height = float(rect.getAttribute('height'))
    print >> codefo, 'dnl'
    print >> codefo, 'ADD_RECT([%s], %f, %f, %f, %f, [%s])dnl' % (
        rect_id, x, y, width, height, coord_id)
    translate_style(rect, coord_id, codefo, doc)
    pass

def translate_group(group, parent_id, codefo, doc):
    group_id = group.getAttribute('id')
    print >> codefo, 'dnl'
    print >> codefo, 'ADD_COORD([%s], [%s])dnl' % (group_id, parent_id)
    for node in group.childNodes:
        if node.namespaceURI != svgns:
            continue
        if node.localName == 'g':
            translate_group(node, group_id, codefo, doc)
        elif node.localName == 'path':
            translate_path(node, group_id, codefo, doc)
        elif node.localName == 'rect':
            translate_rect(node, group_id, codefo, doc)
            pass
        pass
    pass

def svg_2_code(dom, codefo):
    for node in dom.childNodes:
        if node.localName == 'svg' and node.namespaceURI == svgns:
            break;
        pass
    else:
        raise ValueErr, 'no any svg tag node.'
    
    svg = node
    for node in svg.childNodes:
        if node.localName == 'defs' and node.namespaceURI == svgns:
            translate_defs(node, codefo, dom)
            pass
        elif node.localName == 'g' and node.namespaceURI == svgns:
            translate_group(node, 'root_coord', codefo, dom)
            pass
        pass
    pass

if __name__ == '__main__':
    from os import path
    if len(sys.argv) == 3:
        svgfn = sys.argv[1]
        codefn = sys.argv[2]
    elif len(sys.argv) == 2:
        svgfn = sys.argv[1]
        codefn = 'out.mb'
    else:
        print >> sys.stderr, '%s <SVG file> [<output>]' % (sys.argv[0])
        pass
    
    struct_name = path.basename(svgfn).split('.')[0]

    dom = parse(svgfn)
    codefo = file(codefn, 'w+')
    print >> codefo, 'MADBUTTERFLY([%s],[dnl' % (struct_name)
    svg_2_code(dom, codefo)
    print >> codefo, '])dnl'
    pass

