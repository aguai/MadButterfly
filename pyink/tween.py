# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; -*-
# vim: sw=4:ts=8:sts=4
import traceback
import math


def _shift_matrix(x, y):
    return (1, 0, 0, 1, x, y)


def _rotate_matrix(a):
    return (math.cos(a), math.sin(a), -math.sin(a), math.cos(a), 0, 0)


def _scale_matrix(scale_x, scale_y):
    return (scale_x, 0, 0, scale_y, 0, 0)


_id_matrix = (1, 0, 0, 1, 0, 0)


def _mulA(a, b):
    return (a[0] * b[0] + a[2] * b[1],
	    a[1] * b[0] + a[3] * b[1],
	    a[0] * b[2] + a[2] * b[3],
	    a[1] * b[2] + a[3] * b[3],
	    a[0] * b[4] + a[2] * b[5] + a[4],
	    a[1] * b[4] + a[3] * b[5] + a[5])


def _parse_style(style):
    attrs = {}
    
    style_parts = style.split(';')
    for part in style_parts:
	part = part.strip()
	if not part:
	    continue
	nv_pair = part.split(':')
	if len(nv_pair) != 2:
	    raise ValueError, 'invalid format for style "%s"' % (style)

	name = nv_pair[0].strip()
	value = nv_pair[1].strip()
	attrs[name] = value
	pass

    return attrs


def _gen_style(attrs):
    parts = [name + ':' + value for name, value in attrs.items()]
    style = ';'.join(parts)
    return style


def _parse_transform_str(txt):
    if txt[0:9] == 'translate':
	fields = txt[10:].split(',')
	x = float(fields[0])
	fields = fields[1].split(')')
	y = float(fields[0])
	return [1, 0, 0, 1 , x, y]
    elif txt[0:6] == 'matrix':
	fields = txt[7:].split(')')
	fields = fields[0].split(',')
	return [float(field) for field in fields]
    pass


## \brief Parse style attributes about animation.
#
def _parse_style_ani(node, ani_attrs):
    try:
	style = node.getAttribute('style')
    except:			# has no style
	style_attrs = {}
    else:
	style_attrs = _parse_style(style)
	pass

    if 'opacity' in style_attrs:
	ani_attrs['opacity'] = float(style_attrs['opacity'])
	pass

    if 'display' in style_attrs:
	ani_attrs['display'] = style_attrs['display'] != 'none'
	pass
    pass


## \brief Parse all attributes about animation
#
def _parse_attr_ani(node, ani_attrs):
    def _parse_transform_with_center(attr_value):
	value = _parse_transform_str(attr_value)
	x, y = node.spitem.getCenter()
	return (value, (x, y))
    
    attr_defs = {'x': float, 'y': float,
		 'width': float, 'height': float,
		 'transform': _parse_transform_with_center}

    for attrname, parser in attr_defs.items():
	try:
	    value = node.getAttribute(attrname)
	except:			# has no this attribute
	    pass
	else:
	    parsed_value = parser(value)
	    ani_attrs[attrname] = parsed_value
	    pass
	pass
    pass


## \brief Interpolate float values.
#
def _interp_float(start_value, stop_value, percent):
    if start_value == None or stop_value == None:
	if percent == 1:
	    return stop_value
	return start_value
    
    return start_value * (1 - percent) + stop_value * percent


## \brief Interpolate matric.
#
def _interp_transform(start_value, stop_value, percent):
    start_matrix = start_value[0]
    start_center_x, start_center_y = start_value[1]
    stop_matrix = stop_value[0]
    stop_center_x, stop_center_y = stop_value[1]

    start_scale_x, start_scale_y, start_ang, start_x, start_y = \
	_decomposition(start_matrix)
    stop_scale_x, stop_scale_y, stop_ang, stop_x, stop_y = \
	_decomposition(stop_matrix)

    interp = lambda x, y: _interp_float(x, y, percent)
    
    factor_x = interp(start_scale_x, stop_scale_x) / start_scale_x
    factor_y = interp(start_scale_y, stop_scale_y) / start_scale_y
    angle = interp(start_ang, stop_ang)
    shift_x = interp(start_center_x, stop_center_x)
    shift_y = interp(start_center_y, stop_center_y)

    # Shift center point back to origin
    matrix = start_matrix
    shift_matrix = _shift_matrix(-start_center_x, -start_center_y)
    matrix = _mulA(shift_matrix, matrix)
    # Remove rotation
    rotate_matrix = _rotate_matrix(-start_ang)
    matrix = _mulA(rotate_matrix, matrix)
    
    # Apply new scaling
    scale_matrix = _scale_matrix(factor_x, factor_y)
    matrix = _mulA(scale_matrix, matrix)
    # Rotate to new angle
    rotate_matrix = _rotate_matrix(angle)
    matrix = _mulA(rotate_matrix, matrix)
    # Shift space to aim center point on new position.
    shift_matrix = _shift_matrix(shift_x, shift_y)
    matrix = _mulA(shift_matrix, matrix)
    
    return matrix


## \brief Interpolate for value of display style.
#
def _interp_display(start_value, stop_value, percent):
    if percent < 1:
	return start_value
    return stop_value


_interp_funcs = {
    'x': _interp_float, 'y': _interp_float,
    'width': _interp_float, 'height': _interp_float,
    'opacity': _interp_float, 'display': _interp_display,
    'transform': _interp_transform}


def _tween_interpolation(attrname, start_value, stop_value, percent):
    interp = _interp_funcs[attrname]
    _interp_value = interp(start_value, stop_value, percent)
    return _interp_value


def _apply_animation_attrs(ani_attrs, node):
    for attr in ('x', 'y', 'width', 'height'):
	if attr in ani_attrs:
	    node.setAttribute(attr, str(ani_attrs[attr]))
	    pass
	pass

    if 'transform' in ani_attrs:
	try:
	    style = node.getAttribute('style')
	except:
	    style = ''
	    pass

	transform = [str(elm) for elm in ani_attrs['transform']]
	transform_str = 'matrix(' + ','.join(transform) + ')'
	node.setAttribute('transform', transform_str)
	pass

    chg_style = []
    for attrname in 'opacity display'.split():
	if attrname in ani_attrs:
	    chg_style.append((attrname, str(ani_attrs[attrname])))
	    pass
	pass
    if chg_style:
	try:
	    style = node.getAttribute('style')
	except:
	    style_attrs = chg_style
	else:
	    style_attrs = _parse_style(style)
	    style_attrs.update(dict(chg_style))
	    pass
	style = _gen_style(style_attrs)
	node.setAttribute('style', style)
	pass
    pass


def _decomposition(m):
    """
    Decompose the affine matrix into production of
    translation,rotation,shear and scale.  The algorithm is
    documented at
    http://lists.w3.org/Archives/Public/www-style/2010Jun/0602.html
    """
    if m[0]*m[3] == m[1]*m[2]:
	print "The affine matrix is singular"
	return [1,0,0,1,0,0]
    A=m[0]
    B=m[2]
    C=m[1]
    D=m[3]
    E=m[4]
    F=m[5]
    sx = math.sqrt(A*A+B*B)
    A = A/sx
    B = B/sx
    shear = m[0]*m[1]+m[2]*m[3]
    C = C - A*shear
    D = D - B*shear
    sy = math.sqrt(C*C+D*D)
    C = C/sy
    D = D/sy
    r = A*D-B*C
    if r == -1:
	shear = -shear
	sy = -sy
	pass
    R = math.atan2(-B,A)
    return [sx,sy, R, E, F]


def _normalize_attrs(node1, attrs1, node2, attrs2):
    if node2.name() == 'svg:use':
	for name in 'x y width height'.split():
	    if name in attrs1:
		del attrs1[name]
		pass
	    if name in attrs2:
		del attrs2[name]
		pass
	    pass
	pass
    
    names = set(attrs1.keys() + attrs2.keys())

    if 'transform' in names:
	if 'transform' not in attrs1:
	    center = node1.spitem.getCenter()
	    attrs1['transform'] = (_id_matrix, center)
	    pass
	if 'transform' not in attrs2:
	    center = node2.spitem.getCenter()
	    attrs2['transform'] = (_id_matrix, center)
	    pass
	
	root = node1.root()
	try:
	    root_h = float(root.getAttribute('height'))
	except:
	    root_h = 600	# 800x600
	    pass
	
	for attrs in (attrs1, attrs2):
	    transform = attrs['transform']
	    center = (transform[1][0], root_h - transform[1][1])
	    attrs['transform'] = (transform[0], center)
	    
	    if 'x' in attrs:
		del attrs['x']
		pass
	    if 'y' in attrs:
		del attrs['y']
		pass
	    pass
	pass

    defaults = {'x': 0, 'y': 0,
		'width': 0, 'height': 0,
		'opacity': 1.0, 'display': 'inline'}
    
    for attrname in defaults:
	if attrname in names:
	    if attrname not in attrs1:
		attrs1[attrname] = defaults[attrname]
		pass
	    if attrname not in attrs2:
		attrs2[attrname] = defaults[attrname]
		pass
	    pass
	pass

    if node2.name() == 'svg:use':
	accumulators = {'opacity':
			    (lambda attr_v1, attr_v2: attr_v1 * attr_v2),
			'transform':
			    (lambda m1, m2: (_mulA(m2[0], m1[0]), m2[1]))
			}
	for attrname in accumulators:
	    if attrname not in names:
		continue
	    accumulator = accumulators[attrname]
	    acc = accumulator(attrs1[attrname], attrs2[attrname])
	    attrs2[attrname] = acc
	    pass
	pass
    pass


class TweenObject(object):
    TWEEN_TYPE_NORMAL = 0
    #TWEEN_TYPE_RELOCATE = 1
    TWEEN_TYPE_SCALE = 1

    def __init__(self, doc, root):
	super(TweenObject, self).__init__()
        self._doc = doc
	self._root = root
	try:
	    self.width = float(root.getAttribute("width"))
	    self.height = float(root.getAttribute("height"))
	except:
	    self.width = 640
	    self.height = 480
	    pass
	pass
    
    def updateTweenContent(self, duplicate_group, tween_type,
			   start_scene_group, stop_scene_group, percent):
	"""
	    Update the content of the duplicate scene group.  We will
	    use precent, start_scene_group, stop_scene_group to
	    compute transform matrix and update duplicate scene group
	    specified.
	"""
	# Collect ref from the obj
	node = duplicate_group.firstChild()
	dup_nodes = {}
	while node:
	    try:
	        ref = node.getAttribute("ref")
		dup_nodes[ref] = node
	    except:
		ref = None
		pass
	    node = node.next()
	    pass

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

	# Remove duplicate nodes that is not in the set of start nodes
	for node_ref in dup_nodes:
	    if node_ref not in start_nodes:
		node = dup_nodes[node_ref]
		duplicate_group.removeChild(node)
		pass
	    pass

	#
	# Node ID of a node of start scene must be mapped to
	# 'ns0:duplicate-src' attribute of a node of stop scene.  The
	# nodes which can not be mapped to a node of stop scene are
	# not manipulated by the tween.
	#
	# When a scene is duplicated, 'ns0:duplicate-src' attribute of
	# nodes, in the new scene, must be setted to ID of respective
	# one in the duplicated scene.
	#
	start_node = start_scene_group.firstChild()
	while start_node:
	    start_node_id = start_node.getAttribute('id')
	    dup_node = dup_nodes.setdefault(start_node_id, None)
	    try:
		stop_node = stop_nodes[start_node_id]
	    except KeyError:
		stop_node = start_node
		pass
	    
	    self.updateTweenObject(duplicate_group, tween_type,
				   start_node, stop_node,
				   percent, dup_node)
	    start_node = start_node.next()
	    pass
	pass

	    
    def updateTweenObject(self, obj, typ, s, d, p, newobj):
	"""
	    Generate tweened object in the @obj by using s and d in the @p percent
	    http://lists.w3.org/Archives/Public/www-style/2010Jun/0602.html
	"""
	if typ == self.TWEEN_TYPE_SCALE:
	    if newobj == None:
		newobj = s.duplicate(self._doc)
		newobj.setAttribute("ref", s.getAttribute("id"))
		obj.appendChild(newobj)
		pass
	    self._update_tween_object_scale(s, d, p, newobj)
	    pass
	elif typ == self.TWEEN_TYPE_NORMAL and newobj == None:
	    newobj = s.duplicate(self._doc)
	    newobj.setAttribute("ref", s.getAttribute("id"))
	    obj.appendChild(newobj)
	    pass
	pass

    def _update_tween_object_scale(self, start, stop, percent, newobj):
	start_attrs = {}
	_parse_style_ani(start, start_attrs)
	_parse_attr_ani(start, start_attrs)

	stop_attrs = {}
	_parse_style_ani(stop, stop_attrs)
	_parse_attr_ani(stop, stop_attrs)

	_normalize_attrs(start, start_attrs, stop, stop_attrs)

	tween_attrs = {}
	attrs = set(start_attrs.keys() + stop_attrs.keys())
	for attr in attrs:
	    start_v = start_attrs[attr]
	    stop_v = stop_attrs[attr]

	    if start_v != stop_v:
		new_v = _tween_interpolation(attr, start_v, stop_v, percent)
		tween_attrs[attr] = new_v
		pass
	    pass
	
	_apply_animation_attrs(tween_attrs, newobj)
	pass
    pass

## \brief Providing capability of showing scenes.
#
# This class computes and shows scenes for a \ref domview_ui.  The
# content of layers and frames are read from domview_ui to generate
# scenes properly.  When caller requests to show a scene 'n', this
# class compute content of frame 'n' for every layer of the
# domview_ui.
#
class scenes_director(object):
    _tween_obj_tween_types = (TweenObject.TWEEN_TYPE_NORMAL,
			      TweenObject.TWEEN_TYPE_SCALE)
    
    def __init__(self, domview_ui):
	super(scenes_director, self).__init__()
	self._domview = domview_ui
	self._tween_obj = TweenObject(domview_ui.doc, domview_ui.root)
	pass
    
    def show_scene(self, idx):
	"""
	    Update the scene group according to the curretn scene
	    data. There are a couple of cases.
	    1. If the type of the scene is normal, we display it when 
	    it contains the current frame. Otherwise hide it.
	    2. If the type of the scene is relocate or scale, we need 
	       to duplicate the scene group and then modify its 
	       transform matrix according to the definition of the 
	       scene. Then, hide the original scenr group and display 
	       the duplciate scene group. In addition, we may need to 
	       delete the old duplicated scene group as well.

	    For each layer, we will always use the duplicated scene 
	    group whose name as dup.
	    We will put the duplicated scene group inside it. We will 
	    create this group if it is not
	    available.
	"""
	for layer_idx in range(self._domview.get_layer_num()):
	    dup_group = self._domview.get_layer_dup_group(layer_idx)
	    dup_group.setAttribute('style', 'display: none')

	    all_key_tweens = self._domview.get_layer_keys(layer_idx)
	    for start, end, tween_type in all_key_tweens:
		if start == idx: # at key frame
		    scene_group = \
			self._domview.get_key_group(layer_idx, start)
		    scene_group.setAttribute('style', '')
		elif start < idx and end >= idx: # in Tween
		    dup_group.setAttribute('style', '')
		    scene_group = \
			self._domview.get_key_group(layer_idx, start)
		    scene_group.setAttribute('style', 'display: none')
		    
		    try:
			next_scene_group = \
			    self._domview.get_key_group(layer_idx, end + 1)
		    except:	# no next key frame
			next_scene_group = scene_group
			pass

		    tween_obj_type = self._tween_obj_tween_types[tween_type]
		    nframes = end - start + 1
		    percent = float(idx - start) / nframes
		    self._tween_obj.updateTweenContent(dup_group,
						       tween_obj_type,
						       scene_group,
						       next_scene_group,
						       percent)
		    pass
		else:		# this scene should not be showed.
		    scene_group = \
			self._domview.get_key_group(layer_idx, start)
		    scene_group.setAttribute('style', 'display: none')
		    pass
		pass
	    pass
	pass
    pass
