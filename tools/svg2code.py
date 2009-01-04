#! /usr/bin/env python
from xml.dom.minidom import parse
import sys
import re

svgns='http://www.w3.org/2000/svg'
xlinkns='http://www.w3.org/1999/xlink'

re_rgb = re.compile('rgb\\( *([0-9]+(\\.[0-9]+)?) *, *([0-9]+(\\.[0-9]+)?) *, *([0-9]+(\\.[0-9]+)?) *\\)')
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
            elif color.lower() == 'white':
                r, g, b = 1, 1, 1
            elif color.lower() == 'black':
                r, g, b = 0, 0, 0
            else:
                mo = re_rgb.match(color)
                if mo:
                    r = float(mo.group(1))
                    g = float(mo.group(3))
                    b = float(mo.group(5))
                else:
                    raise ValueError, '\'%s\' is invalid color value.' % (color)
                pass

            try:
                opacity = style_map['stop-opacity']
            except:
                opacity = 1
                pass
            offset = node.getAttribute('offset')
            stops.append('[COLOR_STOP([%s], %f, %f, %f, %f, %f)]' % (
                parent_id, r, g, b, float(opacity), float(offset)))
            pass
        pass
    print >> codefo, '%sdnl' % (', '.join(stops))
    pass

def translate_linearGradient(linear, codefo, doc):
    linear_id = _get_id(linear)
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
        print >> codefo, 'REF_STOPS_LINEAR([%s], [%s])dnl' % (
            linear_id, href[1:])
        pass
    pass

def translate_radialGradient(radial, codefo, doc):
    radial_id = _get_id(radial)
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
        print >> codefo, 'REF_STOPS_RADIAL([%s], [%s])dnl' % (
            radial_id, href[1:])
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

def get_style_map(style_str):
    prop_strs = [s.strip() for s in style_str.split(';')]
    prop_kvs = [s.split(':') for s in prop_strs if s]
    prop_kvs = [(k.strip(), v.strip()) for k, v in prop_kvs]
    prop_map = dict(prop_kvs)
    return prop_map

def translate_style(node, coord_id, codefo, doc, prefix):
    node_id = node.getAttribute('id')
    style_str = node.getAttribute('style')
    prop_map = get_style_map(style_str)

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
        elif fill.lower() == 'none':
            pass
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
        elif stroke.lower() == 'none':
            pass
        else:
            raise ValueError, '\'%s\' is an invalid value for stroke.' \
                % (stroke)
        pass

    if prop_map.has_key('stroke-width'):
        if prop_map['stroke-width'].endswith('px'):
            stroke_width = float(prop_map['stroke-width'][:-2])
        else:
            stroke_width = float(prop_map['stroke-width'])
            pass
        print >> codefo, 'STROKE_WIDTH([%s], %f)dnl' % (
            node_id, stroke_width)
        pass

    if prop_map.has_key('display'):
        display = prop_map['display'].strip().lower()
        if display == 'none':
            print >> codefo, '%sHIDE([%s])dnl' % (prefix, node_id)
            pass
        pass
    pass

def translate_shape_transform(shape, coord_id, codefo):
    shape_id = shape.getAttribute('id')

    if shape.hasAttribute('transform'):
        shape_coord_id = shape_id + '_coord'
        ## \page shape_coord Coordinate Transformation for Shapes.
        #
        # svg2code.py add a coord_t for a shape if transform attribute
        # of it's tag is setted.  The name of coord_t object is
        # <shape_id> + "_coord".
        print >> codefo, 'dnl'
        print >> codefo, 'ADD_COORD([%s], [%s])dnl' % (
            shape_coord_id, coord_id)
        transform = shape.getAttribute('transform')
        translate_transform(shape_coord_id, transform, codefo, 'SHAPE_')
        coord_id = shape_coord_id
        pass
    return coord_id

# M x y             : Move to (x,y)
# Z                 : close path
# L x y             : lineto (x,y)
# H x               : horizontal line to x
# V y               : Vertical line to y
# C x1 y1 x2 y2 x y : Draw a segment of bezier curve
# S x2 y2 x t       : Draw a segment of bezier curve from the last
#                     control point
# Q x1 y1 x y       : Draw a segment of quadratic curve
# T x y             : Draw a segment of quadratic curve from the last
#                     control pint
# A x y r f s x y   : Draw an arc
# translate the path data into two arrays. The first is an integer whose
# upper 8 bits are the command type The second array hold all arguments.

command_length={'M': 2, 'm':2,
                'Z': 0, 'z':0,
                'L': 2, 'l':2,
                'H': 1, 'h':1,
                'V': 1, 'v':1,
                'C': 6, 'c':6,
                'S': 4, 's':4,
                'Q': 4, 'q':4,
                'T': 2, 't':2}


def translate_path_data(data,codefo):
    temp = data.split()
    fields=[]
    for f in temp:
        for s in f.split(','):
            if s != '':
                fields.append(s)
    cmd = ''
    commands=''
    args=[]
    fix_args=[]
    for f in fields:
        if cmd == 'A' or cmd == 'a':
            try:
                d = int(f)
                fix_args.append(d)
                if (narg % 7) == 0:
                    commands = commands + cmd
                narg = narg + 1
            except:
	        pass
        else:
            try:
	        d = float(f)
            	args.append(d)
            	if (narg % command_length[cmd]) == 0:
                    commands = commands + cmd
            	narg = narg + 1
            	continue
            except:
                pass
        cmd = f
        narg=0
    pass
    return [commands,args,fix_args]

_id_sn = 0

def _get_id(obj):
    global _id_sn

    if obj.hasAttribute('id'):
        oid = obj.getAttribute('id')
    else:
        oid = '_MB_RAND_%s_' % (_id_sn)
        obj.setAttribute('id', oid)
        _id_sn = _id_sn + 1
        pass
    return oid

##
# Decorator that check if objects have attribute 'mbname'.
#
def check_mbname(func):
    def deco(obj, coord_id, codefo, doc):
        if obj.hasAttribute('mbname'):
            ## \note mbname declare that this node should be in the
            # symbol table.
            mbname = obj.getAttribute('mbname')
            print >> codefo, 'ADD_SYMBOL([%s])dnl' % (mbname)
            pass
        func(obj, coord_id, codefo, doc)
        pass
    return deco

@check_mbname
def translate_path(path, coord_id, codefo, doc):
    coord_id = translate_shape_transform(path, coord_id, codefo)

    path_id = path.getAttribute('id')
    d = path.getAttribute('d')
    (commands,args,fix_args) = translate_path_data(d,codefo)
    print >> codefo, 'dnl'
    #print >> codefo, 'ADD_PATH([%s], [%s], [%s])dnl' % (path_id, d, coord_id)
    sarg=''
    for c in args:
        sarg = sarg + "%f," % c
    s_fix_arg=''
    for c in fix_args:
        s_fix_arg = s_fix_arg + ("%d," % c)

    print >> codefo, 'ADD_PATH([%s], [%s],[%s],[%s],[%d],[%s],[%d])dnl' % (path_id, coord_id,commands,sarg,len(args),s_fix_arg,len(fix_args))

    translate_style(path, coord_id, codefo, doc, 'PATH_')
    pass

@check_mbname
def translate_rect(rect, coord_id, codefo, doc):
    coord_id = translate_shape_transform(rect, coord_id, codefo)

    rect_id = _get_id(rect)
    x = float(rect.getAttribute('x'))
    y = float(rect.getAttribute('y'))
    rx = 0.0
    if rect.hasAttribute('rx'):
        rx = float(rect.getAttribute('rx'))
        pass
    ry = 0.0
    if rect.hasAttribute('ry'):
        ry = float(rect.getAttribute('ry'))
        pass
    width = float(rect.getAttribute('width'))
    height = float(rect.getAttribute('height'))
    print >> codefo, 'dnl'
    print >> codefo, 'ADD_RECT([%s], %f, %f, %f, %f, %f, %f, [%s])dnl' % (
        rect_id, x, y, width, height, rx, ry, coord_id)
    translate_style(rect, coord_id, codefo, doc, 'RECT_')
    pass

def translate_font_style(text, codefo):
    text_id = _get_id(text)
    style_str = text.getAttribute('style')
    style_map = get_style_map(style_str)

    font_sz = 10.0
    if style_map.has_key('font-size'):
        if style_map['font-size'].endswith('px'):
            font_sz = float(style_map['font-size'][:-2])
            print >> codefo, 'define([MB_FONT_SZ], %f)dnl' % (font_sz)
            pass
        pass

    font_style = 'normal'
    if style_map.has_key('font-style'):
        font_style = style_map['font-style'].lower()
        pass

    font_family = 'Roman'
    if style_map.has_key('font-family'):
        font_family = style_map['font-family'].lower()
        pass
    pass

def translate_text(text, coord_id, codefo, doc):
    translate_font_style(text, codefo)

    txt_strs = []
    for node in text.childNodes:
        if node.localName == None:
            txt_strs.append(node.data)
        elif node.localName == 'tspan':
            node.setAttribute('style', text.getAttribute('style'))
            translate_text(node, coord_id, codefo, doc)
            pass
        pass
    if txt_strs:
        text_id = _get_id(text)
        x = float(text.getAttribute('x'))
        y = float(text.getAttribute('y'))
        print >> codefo, 'dnl'
        print >> codefo, \
            'ADD_TEXT([%s], [%s], %f, %f, MB_FONT_SZ, [%s])dnl' % (
            text_id.encode('utf8'), u''.join(txt_strs).encode('utf8'),
            x, y, coord_id.encode('utf8'))
        translate_style(text, coord_id, codefo, doc, 'TEXT_')
        pass
    pass

reo_func = re.compile('([a-zA-Z]+)\\([^\\)]*\\)')
reo_translate = re.compile('translate\\(([-+]?[0-9]+(\\.[0-9]+)?),([-+]?[0-9]+(\\.[0-9]+)?)\\)')
reo_matrix = re.compile('matrix\\(([-+]?[0-9]+(\\.[0-9]+)?),([-+]?[0-9]+(\\.[0-9]+)?),([-+]?[0-9]+(\\.[0-9]+)?),([-+]?[0-9]+(\\.[0-9]+)?),([-+]?[0-9]+(\\.[0-9]+)?),([-+]?[0-9]+(\\.[0-9]+)?)\\)')
def translate_transform(coord_id, transform, codefo, prefix):
    transform = transform.strip()
    mo = reo_func.match(transform)
    if not mo:
        return
    name = mo.group(1)
    if name == 'translate':
        mo = reo_translate.match(transform)
        if mo:
            x = float(mo.group(1))
            y = float(mo.group(3))
            print >> codefo, '%sTRANSLATE([%s], %f, %f)dnl' % (
                prefix, coord_id, x, y)
            pass
    elif name == 'matrix':
        mo = reo_matrix.match(transform)
        if mo:
            r10, r11, r12 = \
                float(mo.group(1)), float(mo.group(3)), float(mo.group(5))
            r20, r21, r22 = \
                float(mo.group(7)), float(mo.group(9)), float(mo.group(11))
            print >> codefo, \
                '%sMATRIX([%s], %f, %f, %f, %f, %f, %f)dnl' % (
                prefix, coord_id, r10, r11, r12, r20, r21, r22)
            pass
        pass
    pass

@check_mbname
def translate_group(group, parent_id, codefo, doc):
    group_id = _get_id(group)
    print >> codefo, 'dnl'
    print >> codefo, 'ADD_COORD([%s], [%s])dnl' % (group_id, parent_id)

    if group.hasAttribute('transform'):
        transform = group.getAttribute('transform')
        translate_transform(group_id, transform, codefo, 'COORD_')
        pass

    translate_style(group, group_id, codefo, doc, 'GROUP_')
    for node in group.childNodes:
        if node.namespaceURI != svgns:
            continue
        if node.localName == 'g':
            translate_group(node, group_id, codefo, doc)
        elif node.localName == 'path':
            translate_path(node, group_id, codefo, doc)
        elif node.localName == 'rect':
            translate_rect(node, group_id, codefo, doc)
        elif node.localName == 'text':
            translate_text(node, group_id, codefo, doc)
            pass
        pass
    pass

## \brief Translate "scenes" tag in "metadata" tag.
#
def translate_scenes(scenes_node, codefo, doc):
    scenes = []
    for scene in scenes_node.childNodes:
        if scene.localName != 'scene' or \
                not scene.hasAttribute('ref') or \
                not scene.hasAttribute('start'):
            continue
        
        start_str = scene.getAttribute('start')
        start = end = int(start_str)
        if scene.hasAttribute('end'):
            end_str = scene.getAttribute('end')
            end = int(end_str)
            pass
        ref = scene.getAttribute('ref')
        
        while len(scenes) <= end:
            scenes.append([])
            pass
        
        for i in range(start, end + 1):
            scenes[i].append(ref)
            pass
        pass

    for scene_idx, groups_in_scene in enumerate(scenes):
        if groups_in_scene:
            groups_str = '[' + '],['.join(groups_in_scene) + ']'
        else:
            groups_str = ''
            pass
        print >> codefo, \
            'SCENE(%d, [%s])dnl' % (scene_idx, groups_str)
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
        elif node.localName == 'metadata' and node.namespaceURI == svgns:
            for meta_node in node.childNodes:
                if meta_node.localName == 'scenes':
                    translate_scenes(meta_node, codefo, dom)
                    pass
                pass
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
        sys.exit(1)
        pass
    
    struct_name = path.basename(codefn).split('.')[0]

    dom = parse(svgfn)
    codefo = file(codefn, 'w+')
    print >> codefo, 'MADBUTTERFLY([%s],[dnl' % (struct_name)
    svg_2_code(dom, codefo)
    print >> codefo, '])dnl'
    pass

