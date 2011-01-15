# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; -*-
# vim: sw=4:ts=8:sts=4
import traceback
import math

def parse_opacity(obj):
    style = obj.getAttribute("style")
    arr = style.split(';')
    for a in arr:
	f = a.split(':')
	if f[0] == 'opacity':
	    return float(f[1])
    return 1

def change_opacity(obj, opacity):
    try:
        style = obj.getAttribute("style")
    except:
        obj.setAttribute("style","opacity:%g" % opacity)
	return
    arr = style.split(';')
    s=''
    for a in arr:
	f = a.split(':')
	f[0] = f[0].replace(' ','')
	if f[0] == 'opacity':
	    if s != '':
		s = s + ('; opacity:%g' % opacity)
	    else:
		s = 'opacity:%g' % opacity
	elif f[0] != '':
	    if s == '':
		s = a
	    else:
		s = s +';'+ a
    obj.setAttribute("style",s)


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
		self.updateTweenObject(duplicate_group, tween_type,
				       start_node, start_node,
				       percent, dup_node)
		start_node = start_node.next()
		continue
	    
	    
	    self.updateTweenObject(duplicate_group, tween_type,
				   start_node, stop_node,
				   percent, dup_node)
	    start_node = start_node.next()
	    pass
	pass

    def parseTransform(self,obj):
	"""
	    Return the transform matrix of an object
	"""
	try:
	    t = obj.getAttribute("transform")
	    if t[0:9] == 'translate':
		fields = t[10:].split(',')
		x = float(fields[0])
		fields = fields[1].split(')')
		y = float(fields[0])
		return [1,0,0,1,x,y]
	    elif t[0:6] == 'matrix':
		fields=t[7:].split(')')
		fields = fields[0].split(',')
		return [float(fields[0]),float(fields[1]),float(fields[2]),float(fields[3]),float(fields[4]),float(fields[5])]
	except:
	    #traceback.print_exc()
	    return [1,0,0,1,0,0]

    def invA(self,m):
        d = m[0]*m[3]-m[2]*m[1]
	return [m[3]/d, -m[1]/d, -m[2]/d, m[0]/d, (m[1]*m[5]-m[4]*m[3])/d, (m[4]*m[2]-m[0]*m[5])/d]
	
    def mulA(self,a,b):
        return [a[0]*b[0]+a[1]*b[2],
	        a[0]*b[1]+a[1]*b[3],
		a[2]*b[0]+a[3]*b[2],
		a[2]*b[1]+a[3]*b[3],
		a[0]*b[4]+a[1]*b[5]+a[4],
		a[2]*b[4]+a[3]*b[5]+a[5]]

    def decomposition(self,m):
	"""
	Decompose the affine matrix into production of translation,rotation,shear and scale.
	The algorithm is documented at http://lists.w3.org/Archives/Public/www-style/2010Jun/0602.html
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
	R = math.atan2(B,A)
	return [sx,sy, R, E,F]

	    
    def updateTweenObject(self,obj,typ,s,d,p,newobj):
	"""
	    Generate tweened object in the @obj by using s and d in the @p percent
	    http://lists.w3.org/Archives/Public/www-style/2010Jun/0602.html
	"""
	if typ == self.TWEEN_TYPE_SCALE:
	    self.updateTweenObjectScale(obj,s,d,p,newobj)
	    pass
	elif typ == self.TWEEN_TYPE_NORMAL:
	    if newobj == None:
	        newobj = s.duplicate(self._doc)
	        newobj.setAttribute("ref", s.getAttribute("id"))
	        obj.appendChild(newobj)
	pass

    def updateTweenObjectScale(self,obj,s,d,p,newobj):
        """
	    Generate a new group which contains the original group and then 
	    add the transform matrix to generate a tween frame between the 
	    origin and destination scene group. 

	    We will parse the transform matrix of the @s and @d and then 
	    generate the matrix which is (1-p) of @s and p percent of @d.
	"""
	if newobj and not newobj.firstChild():
	    # newobj is not with expect structure.
	    #
	    # When a user change tween type of a scene, the structure
	    # of dup group created by old tween type may not satisfy
	    # the requirement of current tween type.
	    newobj.parent().removeChild(newobj)
	    newobj = None
	    pass

	if newobj == None:
            newobj = s.duplicate(self._doc)
            top = self._doc.createElement("svg:g")
	    top.setAttribute("ref",s.getAttribute("id"))
	    top.appendChild(newobj)
	    obj.appendChild(top)
	else:
	    top = newobj
	    newobj = newobj.firstChild()
	    pass
	print s.name() 
	if s.name() == 'svg:g' or s.name() == 'svg:use':
	    # Parse the translate or matrix
	    # 
	    # D  = B inv(A)
	    try:
	        (ox,oy) = s.spitem.getCenter()
	    except:
	        ox = 0
   	        oy = 0
	    try:
	        (dx,dy) = d.spitem.getCenter()
	    except:
	        dx = 0
	        dy = 0
	    try:
		start_opacity = parse_opacity(s)
	    except:
		start_opacity = 1

	    try:
		end_opacity =parse_opacity( d)
	    except:
		end_opacity = 1

		    
	    cur_opacity = start_opacity*(1-p)+end_opacity*p
	    change_opacity(newobj,cur_opacity)
	    sm = self.parseTransform(s)
	    ss = self.decomposition(sm)
	    dm = self.parseTransform(d)
	    dd = self.decomposition(dm)
	    sx = (ss[0]*(1-p)+dd[0]*p)/ss[0]
	    sy = (ss[1]*(1-p)+dd[1]*p)/ss[1]
	    a  = ss[2]*(1-p)+dd[2]*p-ss[2]
	    tx = ox*(1-p)+dx*p
	    ty = oy*(1-p)+dy*p
	    m = [math.cos(a),math.sin(a),-math.sin(a),math.cos(a),0,0]
	    m = self.mulA([sx,0,0,sy,0,0],m)
	    m = self.mulA(m,[1,0,0,1,-ox,oy-self.height])
	    m = self.mulA([1,0,0,1,tx,self.height-ty],m)

	    top.setAttribute("transform","matrix(%g,%g,%g,%g,%g,%g)" % (m[0],m[2],m[1],m[3],m[4],m[5]))
        else:
	    try:
		if d.name() == "svg:use":
		    sw = 1
		    sh = 1
		    dw = 1
		    dh = 1
		else:
		    try:
			sw = float(s.getAttribute("width"))
		    except:
			sw = 1
		    try:
			sh = float(s.getAttribute("height"))
		    except:
			sh = 1

		    try:
			dw = float(d.getAttribute("width"))
		    except:
			dw = 1
		    try:
			dh = float(d.getAttribute("height"))
		    except:
			dh = 1
		    pass

		try:
		    start_opacity = parse_opacity(s)
		except:
		    start_opacity = 1
		    pass

		try:
		    end_opacity =parse_opacity( d)
		    if d.name() == "svg:use":
			end_opacity = end_opacity * start_opacity
			pass
		    pass
		except:
		    if d.name() == "svg:use":
			end_opacity = start_opacity
		    else:
			end_opacity = 1
		    pass

		cur_opacity = start_opacity*(1-p)+end_opacity*p
		change_opacity(newobj,cur_opacity)

	        try:
		    (ox,oy) = s.spitem.getCenter()
	        except:
		    ox = 0
		    oy = 0
	        try:
		    (dx,dy) = d.spitem.getCenter()
	        except:
		    dx = 0
		    dy = 0
		try:
		    sm = self.parseTransform(s)
		    ss = self.decomposition(sm)
		except:
		    ss = [1,1,0,0,0]
		    pass
		try:
		    dm = self.parseTransform(d)
		    dd = self.decomposition(dm)
		except:
		    dd = [1,1,0,0,0]
		dd[0] = dd[0]*dw/sw
		dd[1] = dd[1]*dh/sh
		sx = (ss[0]*(1-p)+dd[0]*p)/ss[0]
		sy = (ss[1]*(1-p)+dd[1]*p)/ss[1]
		a  = ss[2]*(1-p)+dd[2]*p-ss[2]
		tx = ox*(1-p)+dx*p
		ty = oy*(1-p)+dy*p
		m = [math.cos(a),math.sin(a),-math.sin(a),math.cos(a),0,0]
		m = self.mulA([sx,0,0,sy,0,0],m)
		m = self.mulA(m,[1,0,0,1,-ox,oy-self.height])
		m = self.mulA([1,0,0,1,tx,self.height-ty],m)

		top.setAttribute("transform","matrix(%g,%g,%g,%g,%g,%g)" % (m[0],m[2],m[1],m[3],m[4],m[5]))
	    except:
	        traceback.print_exc()
		pass
	    pass
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
