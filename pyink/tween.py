# -*- indent-tabs-mode: t; tab-width: 8; python-indent: 4; -*-
# vim: sw=4:ts=8:sts=4
import traceback
import math
class TweenObject:
    def __init__(self,doc,dom):
        self.document = doc
	self.dom = dom
	self.width = float(dom.attribute("width"))
	self.height = float(dom.attribute("height"))

    def updateMapping(self):
	self.nodeToItem={}
	root = self.dom
	self.updateMappingNode(root)
    def updateMappingNode(self,node):
	for c in node.childList():
	    self.updateMappingNode(c)
	    self.nodeToItem[c.getId()] = c
    def updateTweenContent(self,obj, typ, source,dest,cur):
	"""
	    Update the content of the duplicate scene group. We will use the (start,end) and cur to calculate the percentage of
	    the tween motion effect and then use it to update the transform matrix of the duplicated scene group.
	"""

	start = source.idx
	end = dest.idx
	print cur,start,end
	percent = (cur-start)*1.0/(end-start)
	i = 0
	s = source.ref.firstChild()
	d = dest.ref.firstChild()
	sources={}
	dests={}
	
	# Collect all objects
	while d:
	    try:
		label = d.attribute("inkscape:label")
	    except:
		d = d.getNext()
		continue
	    dests[label] = d
	    d = d.getNext()
	# Check if the object in the source exists in the destination
	s = source.ref.firstChild()
	d = dest.ref.firstChild()
	while s:
	    print s,d
	    try:
		label = s.attribute("inkscape:label")
		# Use i8nkscape:label to identidy the equipvalent objects
		if label:
		    if dests.hasattr(label.value()):
			self.updateTweenObject(obj,typ,s,dests[label.value()],percent)
			s = s.getNext()
			continue
	    except:
		pass
	    # Search obejcts in the destination
	    while d:
		try:
		    d.attribute("inkscape:label")
		    d = d.getNext()
		    continue
		except:
		    pass
		if s.name() == d.name():
		    self.updateTweenObject(obj,typ,s,d,percent)
		    d = d.getNext()
		    break
		d = d.getNext()
	    s = s.getNext()

    def parseTransform(self,obj):
	"""
	    Return the transform matrix of an object
	"""
	try:
	    t = obj.attribute("transform")
	    print t
	    if t[0:9] == 'translate':
		print "translate"
		fields = t[10:].split(',')
		x = float(fields[0])
		fields = fields[1].split(')')
		y = float(fields[0])
		return [1,0,0,1,x,y]
	    elif t[0:6] == 'matrix':
		print "matrix"
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

	    
    def updateTweenObject(self,obj,typ,s,d,p):
	"""
	    Generate tweened object in the @obj by using s and d in the @p percent
	    http://lists.w3.org/Archives/Public/www-style/2010Jun/0602.html
	"""
	if typ == 'relocate':
	    newobj = s.duplicate(self.document)
	    newobj.setAttribute("ref", s.getId())
	    top = self.document.createElement("svg:g")
	    top.appendChild(newobj)
	    obj.appendChild(top)
	    if s.name() == 'svg:g':
		# Parse the translate or matrix
		sm = self.parseTransform(s)
		dm = self.parseTransform(d)
		top.setAttribute("transform","translate(%g,%g)" % ((dm[2]-sm[2])*p,(dm[5]-sm[5])*p))
	    else:
		try:
		    sx = float(s.attribute("x"))
		    sy = float(s.attribute("y"))
		    dx = float(d.attribute("x"))
		    dy = float(d.attribute("y"))
		    tx = (dx-sx)*p
		    ty = (dy-sy)*p
		    print tx,ty
		    top.setAttribute("transform","translate(%g,%g)" % (tx,ty))
		except:
		    traceback.print_exc()
		    pass
	    pass
	elif typ == 'scale':
	    self.updateTweenObjectScale(obj,s,d,p)
	    pass
	elif typ == 'normal':
	    newobj = s.duplicate(self.document)
	    newobj.setAttribute("ref", s.getId())
	    top = self.document.createElement("svg:g")
	    top.appendChild(newobj)
	    obj.appendChild(top)
	pass

    def updateTweenObjectScale(self,obj,s,d,p):
        """
	    Generate a new group which contains the original group and then 
	    add the transform matrix to generate a tween frame between the 
	    origin and destination scene group. 

	    We will parse the transform matrix of the @s and @d and then 
	    generate the matrix which is (1-p) of @s and p percent of @d.
	"""
        newobj = s.duplicate(self.document)
        top = self.document.createElement("svg:g")
	top.appendChild(newobj)
	obj.appendChild(top)
	        
	if s.name() == 'svg:g':
	    # Parse the translate or matrix
	    # 
	    # D  = B inv(A)
	    try:
	        item = self.nodeToItem[s.attribute("id")]
	        (ox,oy) = item.getCenter()
	    except:
	        ox = 0
   	        oy = 0
	    try:
	        item = self.nodeToItem[d.attribute("id")]
	        (dx,dy) = item.getCenter()
	    except:
	        dx = 0
	        dy = 0
		    
	    sm = self.parseTransform(s)
	    ss = self.decomposition(sm)
	    dm = self.parseTransform(d)
	    dd = self.decomposition(dm)
	    sx = (ss[0]*(1-p)+dd[0]*p)/ss[0]
	    sy = (ss[1]*(1-p)+dd[1]*p)/ss[0]
	    a  = ss[2]*(1-p)+dd[2]*p-ss[2]
	    tx = ox*(1-p)+dx*p-ox
	    ty = oy*(1-p)+dy*p-oy
	    m = [math.cos(a),math.sin(a),-math.sin(a),math.cos(a),0,0]
	    m = self.mulA([sx,0,0,sy,0,0],m)
	    m = self.mulA(m,[1,0,0,1,-ox,oy-self.height])
	    m = self.mulA([1,0,0,1,tx,self.height-ty],m)

	    top.setAttribute("transform","matrix(%g,%g,%g,%g,%g,%g)" % (m[0],m[2],m[1],m[3],m[4],m[5]))
        else:
	    try:
	        sw = float(s.attribute("width"))
  	        sh = float(s.attribute("height"))
		dw = float(d.attribute("width"))
		dh = float(d.attribute("height"))
	        try:
	            item = self.nodeToItem[s.attribute("id")]
		    (ox,oy) = item.getCenter()
	        except:
		    ox = 0
		    oy = 0
	        try:
	            item = self.nodeToItem[d.attribute("id")]
		    (dx,dy) = item.getCenter()
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
		dd[0] = ss[0]*dw/sw
		dd[1] = ss[1]*dh/sh
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
