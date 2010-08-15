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
    try:
	prop_map = node.style_map
    except:
        style_str = node.getAttribute('style')
        prop_map = get_style_map(style_str)
        pass
    
    if node.hasAttribute('fill-opacity'):
        opacity = float(node.getAttribute('fill-opacity'))
    elif node.hasAttribute('opacity'):
        opacity = float(node.getAttribute('opacity'))
    else:
        opacity = 1
        pass

    try:
	opacity = float(prop_map['opacity'])
    except:
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

    if node.hasAttribute('stroke-opacity'):
        stroke_opacity = float(node.getAttribute('stroke-opacity'))
    elif node.hasAttribute('opacity'):
        stroke_opacity = float(node.getAttribute('opacity'))
    else:
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
            stroke_width = float(prop_map['stroke-width'][:-2]) / 2
        else:
            stroke_width = float(prop_map['stroke-width']) / 2
            pass
        print >> codefo, 'STROKE_WIDTH([%s], %f)dnl' % (
            node_id, stroke_width)
        pass
    elif prop_map.has_key('stroke') and prop_map['stroke'] != 'none':
        print >> codefo, 'STROKE_WIDTH([%s], %f)dnl' % (
            node_id, 0.5)
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

## \brief Calculate geometry of ellipse where the arc is on.
#
# This function calculate the ellipse with information from SVG path data.
#
# \see calc_center_and_x_aix()
def _calc_ellipse_of_arc(x0, y0, rx, ry, x_rotate, large, sweep, x, y):
    import math
    
    _sin = math.sin(x_rotate)
    _cos = math.cos(x_rotate)
    
    nrx = x * _cos + y * _sin   # Not Rotated X
    nry = x * -_sin + y * _cos
    nrx0 = x0 * _cos + y0 * _sin
    nry0 = x0 * -_sin + y0 * _cos
    
    udx = (nrx - nrx0) / 2 / rx # ux - umx
    udy = (nry - nry0) / 2 / ry # uy - umy
    umx = (nrx + nrx0) / 2 / rx
    umy = (nry + nry0) / 2 / ry

    udx2 = udx * udx
    udy2 = udy * udy
    udl2 = udx2 + udy2
    if udl2 > 1:                # make sure never > 1
        udl2 = 1
        pass

    if udy != 0:
	# center is at left-side of arc
	udcx = -math.sqrt((1 - udl2) * udl2) / (udy + udx2 / udy)
	udcy = -udcx * udx / udy
    else:
	# center is at down-side of arc
	udcx = 0
	udcy = math.sqrt((1 - udl2) * udl2) / udx
        pass

    reflect = 0
    if large:
	reflect ^= 1
        pass
    if sweep != 1:
	reflect ^= 1
        pass
    if reflect:
	udcx = -udcx
	udcy = -udcy
        pass

    nrcx = rx * (udcx + umx)
    nrcy = ry * (udcy + umy)
    
    cx = nrcx * _cos - nrcy * _sin
    cy = nrcx * _sin + nrcy * _cos
    
    return cx, cy

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
                'T': 2, 't':2,
                'A': 7, 'a':7}

def _angle_rotated_ellipse(x, y, rx, ry, x_rotate):
    import math
    
    _cos = math.cos(x_rotate)
    _sin = math.sin(x_rotate)

    nrx = (x * _cos + y * _sin) / rx
    nry = (-x * _sin + y * _cos) / ry
    
    xy_tan = nry / nrx
    xy_angle = math.atan(xy_tan)

    if nrx < 0:
        xy_angle = math.pi + xy_angle
        pass
    
    return xy_angle

def rotate(x, y, angle):
    import math
    _cos = math.cos(angle)
    _sin = math.sin(angle)
    nx = x * _cos - y * _sin
    ny = x * _sin + y * _cos
    return nx, ny

def translate_path_data(data, codefo):
    import string
    import math
    
    temp = data.split()
    fields=[]
    for f in temp:
        for s in f.split(','):
            if s != '':
                fields.append(s)
    cmd = ''
    cmd_args=0
    commands=''
    pnts=[]
    float_args=[]
    for f in fields:
        if f in command_length:
            if cmd_args != 0 and (narg % cmd_args) != 0:
                raise ValueError, 'invalid path data %s' % (repr(fields))
            cmd = f
            cmd_args = command_length[f]
            narg = 0
            continue

        if (narg % cmd_args) == 0:
            commands = commands + cmd
            pass

        if cmd == 'H':
            arg = float(f)
            pnts.append(arg)
            pnts.append(pnts[-2])
            continue
        if cmd == 'h':
            arg = float(f)
            pnts.append(arg + pnts[-2])
            pnts.append(pnts[-2])
            continue
        if cmd == 'V':
            arg = float(f)
            pnts.append(pnts[-2])
            pnts.append(arg)
            continue
        if cmd == 'v':
            arg = float(f)
            pnts.append(pnts[-2])
            pnts.append(arg + pnts[-2])
            continue
            
        arg = float(f)
        if (cmd not in 'am') and (cmd in string.lowercase):
            # relative and not arc or moveto
            arg = arg + pnts[-2]
            pass
        pnts.append(arg)
        narg = narg + 1

        # For arc curve
        if (narg % cmd_args) == 0 and (cmd in 'Aa'):
            x0, y0, rx, ry, x_rotate, large, sweep, x, y = \
                tuple(pnts[-9:])
            if cmd == 'a':
                # relative position
                abs_x = x + x0
                abs_y = y + y0
            else:
                abs_x = x
                abs_y = y
                pass
            x_rotate = int(x_rotate)
            large = int(large)
            sweep = int(sweep)
            cx, cy = _calc_ellipse_of_arc(x0, y0, rx, ry,
                                          x_rotate, large,
                                          sweep, abs_x, abs_y)
            # Corners
            c0x, c0y = rotate(-rx, -ry, x_rotate)
            c0x, c0y = c0x + cx, c0y + cy

            c1x, c1y = rotate(rx, -ry, x_rotate)
            c1x, c1y = c1x + cx, c1y + cy

            c2x, c2y = rotate(rx, ry, x_rotate)
            c2x, c2y = c2x + cx, c2y + cy

            c3x, c3y = rotate(-rx, ry, x_rotate)
            c3x, c3y = c3x + cx, c3y + cy
            
            pnts[-7:] = [c0x, c0y, c1x, c1y, c2x, c2y, c3x, c3y, abs_x, abs_y]
            
            start_angle = _angle_rotated_ellipse(x0 - cx, y0 - cy,
                                             rx, ry, x_rotate)
            stop_angle = _angle_rotated_ellipse(x - cx, y - cy,
                                            rx, ry, x_rotate)
            
            # sweep == 1 for positive-angle direction
            # sweep == 0 for negative-angle direction
            if start_angle > stop_angle and sweep:
                stop_angle = math.pi * 2 + stop_angle
            elif start_angle < stop_angle and not sweep:
                start_angle = math.pi * 2 + start_angle
                pass
            
            float_args.extend([cx, cy, rx, ry,
                               start_angle, stop_angle, x_rotate])
            pass
        pass
    return commands, pnts, float_args

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
        if obj.hasAttribute('mbname') and obj.getAttribute('mbname'):
            ## \note mbname declare that this node should be in the
            # symbol table.
            mbname = obj.getAttribute('mbname')
            id = obj.getAttribute('id')
            print >> codefo, 'ADD_SYMBOL([%s],[%s])dnl' % (id,mbname)
            pass
        func(obj, coord_id, codefo, doc)
        pass
    return deco

@check_mbname
def translate_path(path, coord_id, codefo, doc):
    coord_id = translate_shape_transform(path, coord_id, codefo)

    path_id = path.getAttribute('id')
    d = path.getAttribute('d')
    (commands, pnts, float_args) = translate_path_data(d, codefo)
    print >> codefo, 'dnl'
    #print >> codefo, 'ADD_PATH([%s], [%s], [%s])dnl' % (path_id, d, coord_id)
    spnts=''
    for c in pnts:
        spnts = spnts + "%f," % c
    s_float_arg=''
    for c in float_args:
        s_float_arg = s_float_arg + ("%f," % c)

    print >> codefo, 'ADD_PATH([%s], [%s], [%s], [%s], [%d], [%s], [%d])dnl' % (
        path_id, coord_id, commands, spnts, len(pnts),
        s_float_arg, len(float_args))

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

    return style_map

def merge_style(tspan, text):
    newmap = tspan.style_map
    map = text.style_map
    for k,v in text.style_map.items():
        if not newmap.has_key(k):
            newmap[k] = v
            pass
        pass
    pass

def translate_tspan(tspan, text, coord_id, codefo, doc, txt_strs, attrs):
    try:
   	map = tspan.style_map
    except:
        map = translate_font_style(tspan, codefo)
        tspan.style_map = map
        pass
    if tspan.hasAttribute('x'):
        # Render the tspan as an independent text if the x
        # attribute is defined. All elements inside 
        # the tspan will be ignore by the outter text or tspan elements.
        # FIXME: We need to apply the style map recursively.
        merge_style(tspan, text)
        translate_text(tspan, coord_id, codefo, doc)
        return ''
    attr = [len(txt_strs.encode('utf8')) - 1, 0, tspan]
    attrs.append(attr)
    for node in tspan.childNodes:
        if node.localName == None:
            txt_strs = txt_strs + node.data
        elif node.localName == 'tspan':
	    txt_strs = translate_tspan(node, tspan, coord_id, codefo,
                                       doc, txt_strs, attrs)
            pass
        pass
    attr[1] = len(txt_strs.encode('utf8'))
    return txt_strs

def pango_generate_font_attributes(attrs,coord_id, codefo,doc):
    for a in attrs:
	start = a[0]
	end   = a[1]
	node  = a[2]
	#print "generate attributes from %d to %d" %(start,end)
	style_map = node.style_map
	#print [style_map]
    	if style_map.has_key('font-size'):
    	    # FIXME: Implement all units here
            if style_map['font-size'].endswith('px'):
                font_sz = float(style_map['font-size'][:-2])
                print >> codefo, 'PANGO_SIZE(%d,%d,%d)dnl' % (font_sz*1024,start,end)
	    else:
                font_sz = float(style_map['font-size'])
                print >> codefo, 'PANGO_SIZE(%d,%d,%d)dnl' % (font_sz*1024,start,end)
                pass
            pass
        
        if style_map.has_key('font-style'):
            font_style = style_map['font-style'].lower()
	    if font_style == 'normal':
                print >> codefo, 'PANGO_STYLE(PANGO_STYLE_NORMAL,%d,%d)dnl' % (start,end)
	    elif font_style == 'italic':
                print >> codefo, 'PANGO_STYLE(PANGO_STYLE_ITALIC,%d,%d)dnl' % (start,end)
	    elif font_style == 'oblique':
                print >> codefo, 'PANGO_STYLE(PANGO_STYLE_OBLIQUE,%d,%d)dnl' % (start,end)
                pass
            pass

        if style_map.has_key('font-family'):
            font_family = style_map['font-family']
            print >> codefo, 'PANGO_FAMILY(%s,%d,%d)dnl' % (font_family,start,end)
            pass
        if style_map.has_key('text-anchor'):
            text_anchor = style_map['text-anchor'].lower()
	    # FIXME: We need to implement a mb_text_set_aligment to implement SVG-styled alignment.
	    print "The text-anchor is not implemented yet"
            pass
        if style_map.has_key('font-variant'):
            font_variant = style_map['font-variant'].lower()
	    print "The font-variant is not implemented yet"
            pass
        if style_map.has_key('font-weight'):
            font_weight = style_map['font-weight'].lower()
	    if font_weight == 'normal':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_NORMAL,%d,%d)dnl' % (start,end)
	    elif font_weight == 'bold':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_BOLD,%d,%d)dnl' % (start,end)
	    elif font_weight == 'bolder':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_HEAVY,%d,%d)dnl' % (start,end)
	    elif font_weight == 'lighter':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_ULTRALIGHT,%d,%d)dnl' % (start,end)
	    elif font_weight == '100':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_ULTRALIGHT,%d,%d)dnl' % (start,end)
	    elif font_weight == '200':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_ULTRALIGHT,%d,%d)dnl' % (start,end)
	    elif font_weight == '300':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_LIGHT,%d,%d)dnl' % (start,end)
	    elif font_weight == '400':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_NORMAL,%d,%d)dnl' % (start,end)
	    elif font_weight == '500':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_NORMAL,%d,%d)dnl' % (start,end)
	    elif font_weight == '600':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_SEMIBOLD,%d,%d)dnl' % (start,end)
	    elif font_weight == '700':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_BOLD,%d,%d)dnl' % (start,end)
	    elif font_weight == '800':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_ULTRABOLD,%d,%d)dnl' % (start,end)
	    elif font_weight == '900':
                print >> codefo, 'PANGO_STYLE(PANGO_WEIGHT_HEAVY,%d,%d)dnl' % (start,end)
	    else:
		print "The font-weight %s is not supported" % font_weight
                pass
            pass
        if style_map.has_key('direction'):
            direction = style_map['direction'].lower()
	    print "The direction is not implemented yet"
            pass
        if style_map.has_key('unicode-bidi'):
            bidi = style_map['unicode-bidi'].lower()
	    print "The bidi is not implemented yet"
            pass
        pass
    pass

def pango_gen_text(text, coord_id, codefo, doc, txt_strs, attrs):
    if txt_strs:
        text_id = _get_id(text)
        x = float(text.getAttribute('x'))
        y = float(text.getAttribute('y'))
        print >> codefo, 'dnl'
        print >> codefo, \
            'PANGO_BEGIN_TEXT([%s], [%s], %f, %f, 16, [%s])dnl' % (
            text_id.encode('utf8'), u''.join(txt_strs).encode('utf8'),
            x, y, coord_id.encode('utf8'))
        pango_generate_font_attributes(attrs, coord_id, codefo, doc)
        print >> codefo, \
            'PANGO_END_TEXT([%s], [%s], %f, %f, 16, [%s])dnl' % (
            text_id.encode('utf8'), u''.join(txt_strs).encode('utf8'),
            x, y, coord_id.encode('utf8'))
	translate_style(text, coord_id, codefo, doc, 'TEXT_')
        pass
    pass

def stext_generate_font_attributes(text, attrs, coord_id, codefo, doc):
    text_id = _get_id(text)
    
    for start, end, node in attrs:
        style_map = node.style_map
        
        font_sz = 10
        if style_map.has_key('font-size'):
            fsz = style_map['font-size']
            if fsz.endswith('px'):
                font_sz = float(fsz[:-2])
            else:
                font_sz = float(fsz)
                pass
            pass
        
        if style_map.has_key('font-family'):
            font_family = style_map['font-family']
        else:
            font_family = 'serif'
            pass
        
        font_slant = 0
        if style_map.has_key('font-style'):
            fn_style = style_map['font-style']
            if fn_style == 'normal':
                font_slant = 0
            elif fn_style == 'italic':
                font_slant = 100
            elif fn_style == 'oblique':
                font_slant = 110
            else:
                raise ValueError, '%s is not a valid font-style' % (fn_style)
            pass
        
        font_weight = 80
        if style_map.has_key('font-weight'):
            fn_weight = style_map['font-weight']
            if fn_weight == 'normal':
                font_weight = 80
            elif fn_weight == 'medium':
                font_weight = 100
            elif fn_weight == 'bold':
                font_weight = 200
            elif fn_weight == 'bolder':
                font_weight = 150
            elif fn_weight == 'light':
                font_weight = 50
            elif fn_weight == 'lighter':
                font_weight = 70
            else:
                font_weight = int(fn_weight)
                pass
            pass
        
        print >> codefo, 'STYLE_BLOCK([%s], %d, [%s], %f, %d, %d)dnl' % (
            text_id, end - start, font_family, font_sz,
            font_slant, font_weight)
        pass
    pass

def stext_gen_text(text, coord_id, codefo, doc, txt_strs, attrs):
    if not txt_strs:
        return
    
    text_id = _get_id(text)
    x = float(text.getAttribute('x'))
    y = float(text.getAttribute('y'))
    print >> codefo, 'dnl'
    print >> codefo, \
        'ADD_STEXT([%s], [%s], %f, %f, [%s])dnl' % \
        (text_id, txt_strs.encode('utf8'), x, y, coord_id)
    translate_style(text, coord_id, codefo, doc, 'STEXT_')
    stext_generate_font_attributes(text, attrs, coord_id, codefo, doc)
    pass

def gen_text(text, coord_id, codefo, doc, txt_strs, attrs):
    raise NotImplementedError, \
        'gen_text should be assigned to an implementation'

@check_mbname
def translate_text(text, coord_id, codefo, doc):
    coord_id = translate_shape_transform(text, coord_id, codefo)
    try:
        map = text.style_map
    except:	
        map = translate_font_style(text, codefo)
        text.style_map = map
        pass
    attrs = []
    attr = [0, 0, text]
    attrs.append(attr)

    txt_strs = ''
    for node in text.childNodes:
        if node.localName == None:
            txt_strs = txt_strs + node.data
        elif node.localName == 'tspan':
	    txt_strs = translate_tspan(node, text, coord_id, codefo,
                                       doc,txt_strs, attrs)
            pass
        pass
    attr[1] = len(txt_strs.encode('utf8'))
    gen_text(text, coord_id, codefo, doc, txt_strs, attrs)
    pass

@check_mbname
def translate_image(image, coord_id, codefo, doc):
    coord_id = translate_shape_transform(image, coord_id, codefo)
    
    image_id = _get_id(image)
    if not image.hasAttributeNS(xlinkns, 'href'):
	raise ValueError, 'image %s must has a href attribute.' % (image_id)
    href = image.getAttributeNS(xlinkns, 'href')
    if image.hasAttribute('x'):
	x_str = image.getAttribute('x')
        x = float(x_str)
    else:
	x = 0
        pass
    if image.hasAttribute('y'):
	y_str = image.getAttribute('y')
        y = float(y_str)
    else:
	y = 0
        pass
    if image.hasAttribute('width'):
	width_str = image.getAttribute('width')
        width = float(width_str)
    else:
	width = -1
        pass
    if image.hasAttribute('height'):
	height_str = image.getAttribute('height')
        height = float(height_str)
    else:
	height = -1
        pass
    print >> codefo, 'dnl'
    print >> codefo, \
		'ADD_IMAGE([%s], [%s], %f, %f, %f, %f, [%s])dnl' % (
		        image_id, href, x, y, width, height, coord_id)
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

    mock_sn = 0
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
        elif node.localName == 'image':
            translate_image(node, group_id, codefo, doc)
        elif node.localName == 'textarea':
            translate_textarea(node, group_id, codefo, doc)
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
    import optparse
    
    usage='usage: %prog [options] <SVG file> [<output>]'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-s', '--stext', dest='stext',
                      action='store_true', default=False,
                      help='Use sh_stext instead of sh_text');
    options, args = parser.parse_args()
    
    if len(args) == 2:
        svgfn = args[0]
        codefn = args[1]
    elif len(args) == 1:
        svgfn = args[0]
        codefn = 'out.mb'
    else:
        parser.print_help()
        sys.exit(1)
        pass
    
    struct_name = path.basename(codefn).split('.')[0]

    if options.stext:
        gen_text = stext_gen_text
    else:
        gen_text = pango_gen_text
        pass

    dom = parse(svgfn)
    codefo = file(codefn, 'w+')
    print >> codefo, 'MADBUTTERFLY([%s],[dnl' % (struct_name)
    svg_2_code(dom, codefo)
    print >> codefo, '])dnl'
    pass

