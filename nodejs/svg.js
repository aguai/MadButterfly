// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var libxml = require('libxmljs');
var sys=require('sys');
var mbfly = require("mbfly");
var ldr = mbfly.img_ldr_new(".");


var _std_colors = {
    "white": [1, 1, 1],
    "black": [0, 0, 0],
    "red": [1, 0, 0]
};

exports.loadSVG=function(mb_rt,root,filename) {
    return new loadSVG(mb_rt, root, filename);
};


function loadSVG(mb_rt, root, filename) {
    var doc = libxml.parseXmlFile(filename);
    var _root = doc.root();
    var nodes = _root.childNodes();
    var coord = mb_rt.coord_new(root);
    var k;
    var accu=[1,0,0,0,1,0];
    this.mb_rt = mb_rt;
    this.stop_ref={};
    this.gradients={};
    root.center=new Object();
    root.center.x = 10000;
    root.center.y = 10000;
    
    if(_root.attr("width")) {
	k = _root.attr("width").value();
	this.width = parseFloat(k);
    }
    if(_root.attr("height")) {
	k = _root.attr("height").value();
	this.height = parseFloat(k);
    }
    
    for(k in nodes) {
	var n = nodes[k].name();
        if (n == "defs") {
            this.parseDefs(root,nodes[k]);
        } else if (n == "g") {
            this.parseGroup(accu,root,'root_coord',nodes[k]);
        } 
    }
}

function make_mbnames(mb_rt, n, obj) {
    var mbname;
    var name;
    
    if(!mb_rt.mbnames)
	mb_rt.mbnames = {};
    
    mbname = n.attr("mbname");
    if(mbname) {
	name = mbname.value();
	mb_rt.mbnames[name] = obj;
    }
}

function getInteger(n,name)
{
    if (n == null) return 0;
    var a = n.attr(name);
    if (a==null) return 0;
    return parseInt(a.value());
}

function parsePointSize(s)
{
    var fs=0;
    var i;

    for(i=0;i<s.length;i++) {
        if (s[i]<'0' || s[i] > '9') break;
        fs = fs*10 + (s[i]-'0');
    }
    return fs;
	
}

function parseColor(c)
{
    if (c[0] == '#') {
        return parseInt(c.substring(1,3),16)<<16 | parseInt(c.substring(3,5),16)<<8 | parseInt(c.substring(5,7),16);
    }
}

function parseTextStyle(style,n)
{
    var attr;
    if (n) {
        attr = n.attr('style');
    } else {
        attr = null;
    }
    if (attr == null) {
        return;
    }
    var f = attr.value().split(';');

    for(i in f) {
        var kv = f[i].split(':');
        if (kv[0] == 'font-size') {
            style.fs = parsePointSize(kv[1]);
        } else if (kv[0] == "font-style") {
        } else if (kv[0] == "font-weight") {
        } else if (kv[0] == "fill") {
            style.color = parseColor(kv[1]);
        } else if (kv[0] == "fill-opacity") {
        } else if (kv[0] == "stroke-opacity") {
        } else if (kv[0] == "stroke") {
        } else if (kv[0] == "stroke-width") {
        } else if (kv[0] == "stroke-linecap") {
        } else if (kv[0] == "stroke-linejoin") {
        } else if (kv[0] == "stroke-lineopacity") {
        } else if (kv[0] == "font-family") {
            style.family = kv[1];
        } else if (kv[0] == "font-stretch") {
        } else if (kv[0] == "font-variant") {
        } else if (kv[0] == "text-anchor") {
        } else if (kv[0] == "text-align") {
        } else if (kv[0] == "writing-mode") {
        } else if (kv[0] == "line-height") {
        } else {
            sys.puts("Unknown style: "+kv[0]);
        }
    }
}
function tspan_set_text(text)
{
    this.text.set_text(text); 
}

loadSVG.prototype.parseTSpan = function(coord, n,style) {
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
    var tcoord = this.mb_rt.coord_new(coord);
    var nodes = n.childNodes();
    var k;
    
    var obj = this.mb_rt.stext_new(n.text(),x,y);
    parseTextStyle(style,n);
    style.paint = this.mb_rt.paint_color_new(1,1,1,1);
    style.face=this.mb_rt.font_face_query(style.family, 2, 100);
    obj.set_style([[20,style.face,style.fs]]);
    style.paint.fill(obj);
    tcoord.add_shape(obj);
    for(k in nodes) {
        var name = nodes[k].name();
        if (name == "tspan") {
            this.parseTSpan(tcoord,nodes[k]);
        } else {
        }
    }
    tcoord.set_text=tspan_set_text;
    tcoord.text = obj;
    make_mbnames(this.mb_rt, n, tcoord);
};

loadSVG.prototype._prepare_paint_color = function(color, alpha) {
    var paint;
    var c;
    
    if (color[0]=='#') {
	var r,g,b;
	r = parseInt(color.substring(1,3),16)/255;
	g = parseInt(color.substring(3,5),16)/255;
	b = parseInt(color.substring(5,7),16)/255;
	paint = this.mb_rt.paint_color_new(r, g, b, alpha);
    } else if(_std_colors[color]) {
	c = _std_colors[color];
	paint = this.mb_rt.paint_color_new(c[0], c[1], c[2], alpha);
    } else if (color.substring(0,3) == 'url') {
	var id = color.substring(5, color.length-1);
	var gr = this.gradients[id];
	paint = this.mb_rt.paint_linear_new(gr[0],gr[1],gr[2],gr[3]);
	paint.set_stops(this.stop_ref[id]);
    } else {
	paint = this.mb_rt.paint_color_new(0,0,0,1);
    }
    return paint;
};
	
function guessPathBoundingBox(coord,d)
{
    return;
    var items = d.split(' ');
    var len = items.length;
    var pair;
    var i;
    var minx,miny;

    minx = 10000;
    miny = 10000;

    for(i=0;i<len;i++) {
	var type = items[i].toLowerCase();
	x = minx;y = miny;
	switch(type) {
	case 'm':
	case 'l':
	case 'a':
	case 'x':
	    pair = items[i+1].split(',');
	    if (pair.length==2) {
		x = parseFloat(pair[0]);
		y = parseFloat(pair[1]);
		i++;
	    } else {
		x = parseFloat(items[i+1]);
		y = parseFloat(items[i+2]);
		i+=2;
	    }
	    break;
	case 'q':
	    // Implement this latter
	    break;
	case 'c':
	    // Implement this latter
	    break;
	case 's':
	    // Implement this latter
	    break;
	case 'h':
	    x = parseFloat(items[i+1]);
	    break;
	case 'v':
	    y = parseFloat(items[i+1]);
	    break;
	default:
	    continue;
	}
	if (x < minx) minx = x;
	if (y < miny) miny = y;
    }
    if (coord.center.x >  minx)
	coord.center.x = minx;
    if (coord.center.y >  miny)
	coord.center.y = miny;
};

function _mul(m1, m2) {
    var res = new Array();

    res.push(m1[0] * m2[0] + m1[1] * m2[3]);
    res.push(m1[0] * m2[1] + m1[1] * m2[4]);
    res.push(m1[0] * m2[2] + m1[1] * m2[5] + m1[2]);
    res.push(m1[3] * m2[0] + m1[4] * m2[3]);
    res.push(m1[3] * m2[1] + m1[4] * m2[4]);
    res.push(m1[3] * m2[2] + m1[4] * m2[5] + m1[5]);

    return res;
}

function _pnt_transform(x, y, matrix) {
    var rx, ry;

    rx = x * matrix[0] + y * matrix[1] + matrix[2];
    ry = x * matrix[3] + y * matrix[4] + matrix[5];
    return new Array(rx, ry);
}

function _shift_transform(x, y, matrix) {
    var rx, ry;

    rx = x * matrix[0] + y * matrix[1];
    ry = x * matrix[3] + y * matrix[4];
    return new Array(rx, ry);
}

function _transform_bbox(bbox, matrix) {
    var min_x, min_y, max_x, max_y;
    var x, y;
    var pnt;
    var pnts = new Array();
    var i;

    pnt = _pnt_transform(bbox.x, bbox.y, matrix);
    pnts.push(pnt);
    pnt = _pnt_transform(bbox.x + bbox.width, bbox.y, matrix);
    pnts.push(pnt);
    pnt = _pnt_transform(bbox.x, bbox.y + bbox.height, matrix);
    pnts.push(pnt);
    pnt = _pnt_transform(bbox.x + bbox.width, bbox.y + bbox.height, matrix);
    pnts.push(pnt);

    min_x = max_x = pnts[0][0];
    min_y = max_y = pnts[0][1];
    for(i = 1; i < pnts.length; i++) {
	pnt = pnts[i];
	if(pnt[0] < min_x)
	    min_x = pnt[0];
	if(pnt[1] < min_y)
	    min_y = pnt[1];
	if(pnt[0] > max_x)
	    max_x = pnt[0];
	if(pnt[1] > max_y)
	    max_y = pnt[1];
    }

    bbox.x = min_x;
    bbox.y = min_y;
    bbox.width = max_x - min_x;
    bbox.height = max_y - min_y;
}

function _reverse(m1) {
    var rev = new Array(1, 0, 0, 0, 1, 0);
    var m = new Array(m1[0], m1[1], m1[2], m1[3], m1[4], m1[5]);

    rev[3] = -m[3] / m[0];
    m[3] = 0;
    m[4] += rev[3] * m[1];
    m[5] += rev[3] * m[2];
    
    rev[1] = -m[1] / m[4];
    rev[0] += rev[1] * rev[3];
    m[1] = 0;
    m[2] += rev[1] * m[5];
    
    rev[2] = -m[2];
    rev[5] = -m[5];
    
    rev[0] = rev[0] / m[0];
    rev[1] = rev[1] / m[0];
    rev[2] = rev[2] / m[0];
    
    rev[3] = rev[3] / m[4];
    rev[4] = rev[4] / m[4];
    rev[5] = rev[5] / m[4];

    return rev;
}

var _bbox_proto = {
    _get_ac_saved_rev: function() {
	var c = this.owner;
	var mtx;
	
	if(c.type != "coord")
	    c = c.parent;	// is a shape!
	
	mtx = c._mbapp_saved_rev_mtx;
	while(c.parent && typeof c.parent != "undefined") {
	    c = c.parent;
	    mtx = _mul(mtx, c._mbapp_saved_rev_mtx);
	}

	return mtx;
    },
    
    _get_ac_mtx: function() {
	var c = this.owner;
	var mtx;
	
	if(c.type != "coord")
	    c = c.parent;	// is a shape!

	mtx = [c[0], c[1], c[2], c[3], c[4], c[5]];
	while(c.parent) {
	    c = c.parent;
	    mtx = _mul(c, mtx);
	}

	return mtx;
    },

    _saved_to_current: function() {
	var r;
	
	r = _mul(this._get_ac_mtx(), this._get_ac_saved_rev());
	
	return r;
    },

    /*! \brief Update x, y, width, and height of the bbox.
     */
    update: function() {
	var mtx;

	this.x = this._svg_saved_x;
	this.y = this._svg_saved_y;
	this.width = this._svg_saved_width;
	this.height = this._svg_saved_height;
	
	mtx = this._saved_to_current();
	_transform_bbox(this, mtx);
    },    
};

var _center_proto = {
    _get_ac_saved_rev: function() {
	var c = this.owner;
	var mtx;
	
	if(c.type != "coord")
	    c = c.parent;	// is a shape!
	
	mtx = c._mbapp_saved_rev_mtx;
	while(c.parent && typeof c.parent != "undefined") {
	    c = c.parent;
	    mtx = _mul(mtx, c._mbapp_saved_rev_mtx);
	}

	return mtx;
    },
    
    _get_ac_mtx: function() {
	var c = this.owner;
	var mtx;
	
	if(c.type != "coord")
	    c = c.parent;	// is a shape!

	mtx = [c[0], c[1], c[2], c[3], c[4], c[5]];
	while(c.parent) {
	    c = c.parent;
	    mtx = _mul(c, mtx);
	}

	return mtx;
    },

    _get_ac_rev: function() {
	var c = this.owner;
	var mtx;
	
	if(c.type != "coord")
	    c = c.parent;	// is a shape!

	mtx = _reverse([c[0], c[1], c[2], c[3], c[4], c[5]]);
	while(c.parent) {
	    c = c.parent;
	    mtx = _mul(mtx, _reverse([c[0], c[1], c[2], c[3], c[4], c[5]]));
	}

	return mtx;
    },

    _saved_to_current: function() {
	var r;
	
	r = _mul(this._get_ac_mtx(), this._get_ac_saved_rev());
	
	return r;
    },

    /*! \brief Update x, y of center point of an object.
     */
    update: function() {
	var mtx;
	var xy;

	mtx = this._saved_to_current();
	xy = _pnt_transform(this._svg_saved_x, this._svg_saved_y, mtx);

	this._x = xy[0];
	this._y = xy[1];
    },

    /*! \brief Move owner object to make center at (x, y).
     */
    move: function(x, y) {
	var mtx;
	var xdiff = x - this._x;
	var ydiff = y - this._y;
	var shiftxy;
	var c;

	mtx = this._get_ac_rev();
	shiftxy = _shift_transform(xdiff, ydiff, mtx);

	c = this.owner;
	if(c.type != "coord")
	    c = c.parent;

	c[2] += shiftxy[0];
	c[5] += shiftxy[1];

	this._x = x;
	this._y = y;
    },

    /*! \brief Move owner object to make center at position specified by pnt.
     */
    move_pnt: function(pnt) {
	this.move(pnt.x, pnt.y);
    },

    /*! \brief Prevent user to modify value.
     */
    get x() { return this._x; },
    
    /*! \brief Prevent user to modify value.
     */
    get y() { return this._y; },

    get rel() {
	var rev;
	var xy;
	
	rev = this._get_ac_rev();
	xy = _pnt_transform(this._svg_saved_x, this._svg_saved_y, rev);

	return {x: xy[0], y: xy[1]};
    },
};

loadSVG.prototype._set_bbox = function(node, tgt) {
    var a;
    var vstr;
    var bbox, center;
    
    a = node.attr("bbox-x");
    sys.puts("a="+a);
    if(!a)
	return 0;
    
    tgt.bbox = bbox = new Object();
    bbox.owner = tgt;
    bbox.__proto__ = _bbox_proto;
    vstr = a.value();
    bbox._svg_saved_x = parseFloat(vstr);

    a = node.attr("bbox-y");
    vstr = a.value();
    bbox._svg_saved_y = this.height - parseFloat(vstr);

    a = node.attr("bbox-width");
    vstr = a.value();
    bbox._svg_saved_width = parseFloat(vstr);

    a = node.attr("bbox-height");
    vstr = a.value();
    bbox._svg_saved_height = parseFloat(vstr);
    bbox._svg_saved_y -= bbox._svg_saved_height;
    
    bbox.update();

    tgt.center = center = new Object();
    
    center._svg_saved_x = bbox._svg_saved_width / 2 + bbox._svg_saved_x;
    center._svg_saved_y = bbox._svg_saved_height / 2 + bbox._svg_saved_y;
    a = node.attr("transform-center-x");
    if(a) {
	vstr = a.value();
	center._svg_saved_x += parseFloat(vstr);
	a = node.attr("transform-center-y");
	vstr = a.value();
	center._svg_saved_y -= parseFloat(vstr);
    }
    center.__proto__ = _center_proto;
    center.owner = tgt;
    center.update();
    
    return 1;
}

loadSVG.prototype._set_paint = function(node, tgt) {
    var style = node.attr('style');
    var paint;
    var fill_alpha = 1;
    var stroke_alpha = 1;
    var fill_color;
    var stroke_color;
    var black_paint;
    var i;
    
    if(style != null) {
	var items = style.value().split(';');
	var alpha;
	
	for(i in items) {
	    var f = items[i].split(':');
	    if (f[0] == 'opacity') {
		alpha = f[1];
	    } else if (f[0] == 'fill') {
		fill_color = f[1];
	    } else if (f[0] == 'fill-opacity') {
		fill_alpha = parseFloat(f[1]);
	    } else if (f[0] == 'stroke') {
		stroke_color = f[1];
	    } else if (f[0] == 'stroke-width') {
		tgt.stroke_width = parseFloat(f[1]);
	    } else if (f[0] == 'stroke-opacity') {
		stroke_alpha = parseFloat(f[1]);
	    } else if (f[0] == 'display') {
		if(f[1] == 'none')
		    return;
	    }
	}

    }

    if(!fill_color || !stroke_color)
	black_paint = this.mb_rt.paint_color_new(0, 0, 0, 1);
    
    if(fill_color) {
	if(fill_color != "none") {
	    paint = this._prepare_paint_color(fill_color, fill_alpha);
	    paint.fill(tgt);
	}
    } else {
	black_paint.fill(tgt);
    }
    if(stroke_color) {
	if(stroke_color != "none") {
	    paint = this._prepare_paint_color(stroke_color, stroke_alpha);
	    paint.stroke(tgt);
	}
    } else {
	black_paint.stroke(tgt);
    }
};

loadSVG.prototype.parsePath=function(accu, coord,id, n)
{
    var d = n.attr('d').value();
    var style = n.attr('style');
    var path = this.mb_rt.path_new(d);
    
    guessPathBoundingBox(coord,d);
    coord.add_shape(path);
    this._set_paint(n, path);
    this._set_bbox(n, path);

    make_mbnames(this.mb_rt, n, path);
};

loadSVG.prototype.parseText=function(accu,coord,id, n)
{
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
    var tcoord = this.mb_rt.coord_new(coord);
    var style = new Object();

    if (n.attr('x')) {
	var nx = coord[0]*x+coord[1]*y+coord[2];
	if (coord.center.x > nx)
	    coord.center.x = nx;
    }
    if (n.attr('y')) {
	var ny = coord[3]*x+coord[4]*y+coord[5];
	if (coord.center.y > ny)
	    coord.center.y = ny;
    }
    style.fs = 20;
    style.family = 'courier';
    parseTextStyle(style,n);
    var nodes = n.childNodes();
    var k;
    for(k in nodes) {
	var c= nodes[k].name();
	if (c == "tspan") {
	    this.parseTSpan(tcoord,nodes[k],style);
	} else {
	}
    }
    sys.puts(y);
    this._set_bbox(n, tcoord);
	
    make_mbnames(this.mb_rt, n, tcoord);
};


function multiply(s,d) {
    var m=[];
    m[0] = s[0]*d[0]+s[1]*d[3];
    m[1] = s[0]*d[1]+s[1]*d[4];
    m[2] = s[0]*d[2]+s[1]*d[5]+s[2];
    m[3] = s[3]*d[0]+s[4]*d[3];
    m[4] = s[3]*d[1]+s[4]*d[4];
    m[5] = s[3]*d[2]+s[4]*d[5]+s[5];
    s[0] = m[0];
    s[1] = m[1];
    s[2] = m[2];
    s[3] = m[3];
    s[4] = m[4];
    s[5] = m[5];
};

function parseTransform(coord, s)
{
    var off = s.indexOf('translate');
    if (off != -1) {
        var ss = s.substring(off+9);
        for(i=0;i<ss.length;i++) {
            if (ss[i] == '(') break;
        }
        ss = ss.substring(i+1);
        for(i=0;i<ss.length;i++) {
            if (ss[i] == ')') {
                ss = ss.substring(0,i);
                break;
            }
        }
        var f = ss.split(',');
        var x,y;
        x = parseFloat(f[0]);
        y = parseFloat(f[1]);
        coord[2] += x;
        coord[5] += y;
    }
    off = s.indexOf('matrix');
    if (off != -1) {
        var end = s.indexOf(')');
        var m = s.substring(7,end);
        var fields = m.split(',');
        var newm=[];
        newm[0] = parseFloat(fields[0]);
        newm[1] = parseFloat(fields[2]);
        newm[2] = parseFloat(fields[4]);
        newm[3] = parseFloat(fields[1]);
        newm[4] = parseFloat(fields[3]);
        newm[5] = parseFloat(fields[5]);
        multiply(coord,newm);
    }
}

loadSVG.prototype.parseRect=function(accu_matrix,coord, id, n) 
{
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
    var rx,ry;
    var w = getInteger(n,'width');
    var h = getInteger(n,'height');
    var trans = n.attr('transform');
    var paint;
    var tcoord = this.mb_rt.coord_new(coord);
	
    var style = n.attr('style');

    if (trans)
        parseTransform(tcoord,trans.value());

    var rect = this.mb_rt.rect_new(x,y,w,h,10, 10);
    tcoord.add_shape(rect);
    this._set_paint(n, rect);
    this._set_bbox(n, tcoord);

    make_mbnames(this.mb_rt, n, tcoord);
};

// When we parse a group, we need to calculate the origin of the group
// so that we can resize the group without changing its origin point.
// This must be done recursively. For text/rect/image, we can get its
// origin point directly by using the (x,y) and apply their
// transformation matrix. For group, we need to send the acculumated
// matrix so that each group can get their origin correctly.
//
// Each element must be responsible to calculate its absolute origin
// point and update the origin of its parent.

function parseGroupStyle(style,n)
{
    var attr;
    if (n) {
        attr = n.attr('style');
    } else {
        attr = null;
    }
    if (attr == null) {
        return;
    }
    var f = attr.value().split(';');

    for(i in f) {
        var kv = f[i].split(':');
        if (kv[0] == 'opacity') {
            style.opacity = parseFloat(kv[1]);
        } else {
            sys.puts("Unknown style: "+kv[0]);
        }
    }
}

loadSVG.prototype.parseGroup=function(accu_matrix,root, group_id, n) {
    var k;
    var nodes = n.childNodes();
    var coord = this.mb_rt.coord_new(root);
    // Parse the transform and style here
    var trans = n.attr('transform');
    var accu=[1,0,0,0,1,0];
    var style;

    coord.center= new Object();
    coord.center.x = 10000;
    coord.center.y = 10000;
    if (trans!=null) {
	parseTransform(coord, trans.value());
    } 
    multiply(accu,accu_matrix);
    multiply(accu,coord);

    style = {};
    parseGroupStyle(style, n);
    if(style.opacity) {
	sys.puts(style.opacity);
	coord.opacity=style.opacity;
    }

    for(k in nodes) {
	var c = nodes[k].name();
	var attr = nodes[k].attr('id');
	var id;
	if (attr) {
	    id = attr.value();
	}
	if (c == "g") {
	    this.parseGroup(accu,coord, id, nodes[k]);
	} else if (c == "path") {
	    this.parsePath(accu,coord, id, nodes[k]);
	} else if (c == "text") {
	    this.parseText(accu,coord, id, nodes[k]);
	} else if (c == "rect") {
	    this.parseRect(accu_matrix,coord, id, nodes[k]);
	} else if (c == "image") {
	    this.parseImage(accu_matrix,coord, id, nodes[k]);
	}
    }
    if (root.center.x > coord.center.x)
	root.center.x = coord.center.x;
    if (root.center.y > coord.center.y)
	root.center.y = coord.center.y;

    this._set_bbox(n, coord);
    
    make_mbnames(this.mb_rt, n, coord);
};

loadSVG.prototype.parseImage=function(accu,coord,id, n)
{
    var ref = n.attr('href').value();
    var tcoord = this.mb_rt.coord_new(coord);
    var trans = n.attr('transform');

    if (ref == null) return;
    if (ref.substr(0,7) == "file://") {
	ref = ref.substring(7);
    } else if (ref.substr(0,5)=="file:") {
	ref = ref.substring(5);
    } else {
    }
    var w;
    var h;
    var x,y,nx,ny;
    coord.center= new Object();
    coord.center.x = 10000;
    coord.center.y = 10000;
    if (trans!=null) {
	parseTransform(coord, trans.value());
    } 

    w = n.attr("width");
    if (w == null) return;
    w = parseFloat(w.value());
    h = n.attr("height");
    if (h == null) return;
    h = parseFloat(h.value());	
    x = n.attr("x");
    if (x == null) return;
    x = parseFloat(x.value());
    y = n.attr("y");
    if (y == null) return;
    y = parseFloat(y.value());
    nx = tcoord[0]*x+tcoord[1]*y+tcoord[2];
    ny = tcoord[3]*x+tcoord[4]*y+tcoord[5];
    if (coord.center.x > nx) 
	coord.center.x = nx;
    if (coord.center.y > ny)
	coord.center.y = ny;
    var img = this.mb_rt.image_new(x,y,w,h);
    var img_data = ldr.load(ref);
    var paint = this.mb_rt.paint_image_new(img_data);
    paint.fill(img);
    tcoord.add_shape(img);
    
    this._set_bbox(n, img);
    
    make_mbnames(this.mb_rt, n, img);
};

loadSVG.prototype._MB_parseLinearGradient=function(root,n)
{
    var id = n.attr('id');
    var k;
    var nodes = n.childNodes();

    if (id == null) return;
    var x1 = n.attr("x1");
    var y1 = n.attr("y1");
    var x2 = n.attr("x2");
    var y2 = n.attr("y2");
    var gr;
    var color, opacity;
    var stops;
    var r,g,b;
    stops=[];
    for(k in nodes) {
	var ss = nodes[k];
	if (ss.name()=="stop") {
	    var style = ss.attr("style").value();
	    var items = style.split(';');
	    var off = parseInt(ss.attr('offset').value());
	    color = 'black';
	    opacity = 1;
	    for (i in items) {
		it = items[i];
		var f = it.split(':');
		k = f[0];
		v = f[1];
		if (k == 'stop-color') {
		    color = v.substring(1);
		    if (v == 'white') {
			r = 1;
			g = 1;
			b = 1;
		    } else if (v == 'black') {
			r = 0;
			g = 0;
			b = 0;
		    } else {
			r = parseInt(color.substring(0,2),16)/255.0;
			g = parseInt(color.substring(2,4),16)/255.0;
			b = parseInt(color.substring(4,6),16)/255.0;
		    }
		} else if (k=='stop-opacity') {
		    opacity = parseFloat(v);
		}
	    }
	    stops.push([off, r,g,b,opacity]);
	}
    }
    var href = n.attr('href');
    if (href != null) {
	href = href.value();
	pstops = this.stop_ref[href.substring(1)];
	stops = pstops.concat(stops);
    }
    id = id.value();
    this.stop_ref[id] = stops;
    if (x1)
	x1 = parseFloat(x1.value());
    if (x2)
	x2 = parseFloat(x2.value());
    if (y1)
	y1 = parseFloat(y1.value());
    if (y2)
	y2 = parseFloat(y2.value());
    this.gradients[id] = [x1,y1,x2,y2];
};

loadSVG.prototype.parseDefs=function(root,n)
{
    var k;
    var nodes = n.childNodes();
    
    for(k in nodes) {
	var name = nodes[k].name();
	if (name == "linearGradient") {
	    this._MB_parseLinearGradient(root,nodes[k]);
	}
    }
};


