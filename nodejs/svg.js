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

function parse_color(color) {
    var r, g, b;
    var c;
    
    if (color[0] == "#") {
	r = parseInt(color.substring(1, 3), 16) / 255;
	g = parseInt(color.substring(3, 5), 16) / 255;
	b = parseInt(color.substring(5, 7), 16) / 255;
    } else if(_std_colors[color]) {
	c = _std_colors[color];
	r = c[0];
	g = c[1];
	b = c[2];
    } else {
	r = g = b = 0;
    }
    
    return [r, g, b];
}

function loadSVG (mb_rt, root, filename) {
    this.pgstack=[];
    if (filename)
        this.load(mb_rt,root,filename);
}

loadSVG.prototype.load=function(mb_rt, root, filename) {
    var doc = libxml.parseXmlFile(filename);
    var _root = doc.root();
    var nodes = _root.childNodes();
    var coord = mb_rt.coord_new(root);
    var k;
    var accu=[1,0,0,0,1,0];
    this.mb_rt = mb_rt;
    this.stop_ref={};
    this.gradients={};
    this.radials = {};
    this._groupMap={};
    coord.center=new Object();
    coord.center.x = 10000;
    coord.center.y = 10000;
    if (this.pgstack.length > 0)
        this.pgstack[this.pgstack.length-1].hide();
    this.pgstack.push(coord);

    
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
            this.parseDefs(coord,nodes[k]);
	} else if (n == "metadata") {
	    this.parseMetadata(coord,nodes[k]);
        } else if (n == "g") {
            this.parseGroup(accu,coord,'root_coord',nodes[k]);
        } 
    }
}



loadSVG.prototype.leaveSVG=function()
{
    var p = this.pgstack.pop();
    p.hide();
    if (this.pgstack.length > 0)
	this.pgstack[this.pgstack.length-1].show();
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
    mbname = n.attr("label");
    if(mbname&&(mbname.value()!="")) {
	name = mbname.value();
	mb_rt.mbnames[name] = obj;
    }
    try {
        var gname = n.attr('id').value();
        sys.puts("defone object "+ gname);
        mb_rt.mbnames[gname] = obj;
    } catch(e) {
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

function parse_style(node) {
    var style_attr;
    var style;
    var parts, part;
    var kv, key, value;
    var content = {};
    var i;
    
    style_attr = node.attr('style');
    if(!style_attr)
	return content;

    style = style_attr.value();
    parts = style.split(';');
    for(i = 0; i < parts.length; i++) {
	part = parts[i].trim();
	if(part) {
	    kv = part.split(':');
	    key = kv[0].trim();
	    value = kv[1].trim();
	    content[key] = value;
	}
    }

    return content;
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

function _parse_font_size(fn_sz_str) {
    var pos;

    pos = fn_sz_str.search("px");
    if(pos >= 0)
	fn_sz_str = fn_sz_str.substring(0, pos);
    pos = fn_sz_str.search("pt");
    if(pos >= 0)
	fn_sz_str = fn_sz_str.substring(0, pos);

    return parseFloat(fn_sz_str);
}

loadSVG.prototype.parseTSpan = function(coord, n, pstyle) {
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
    var tcoord = this.mb_rt.coord_new(coord);
    var style;
    var family = "Courier";
    var weight = 80;
    var slant = 0;
    var sz = 10;
    var face;
    var k;
    var obj = this.mb_rt.stext_new(n.text(),x,y);
    
    style = parse_style(n);
    for(k in pstyle) {
	if(k in style)
	    continue;
	style[k] = pstyle[k];
    }

    if("font-family" in style)
	family = style["font-family"];
    if("font-size" in style)
	sz = _parse_font_size(style["font-size"]);
    if("font-weight" in style) {
	if(style["font-weight"] == "bold")
	    weight = 200;
    }
    if("font-style" in style) {
	if(style["font-style"] == "oblique")
	    slant = 110;
    }

    face = this.mb_rt.font_face_query(family, slant, weight);
    obj.set_style([[n.text().length, face, sz]]);

    tcoord.add_shape(obj);
    tcoord.set_text = tspan_set_text;
    tcoord.text = obj;
    
    this._set_paint_style(style, obj);
    this._set_bbox(n, obj);
    
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
	if(id in this.gradients) {
	    var gr = this.gradients[id];
	    paint = this.mb_rt.paint_linear_new(gr[0],gr[1],gr[2],gr[3]);
	} else {
	    var radial = this.radials[id];
	    paint = this.mb_rt.paint_radial_new(radial[0],
						radial[1],
						radial[2]);
	}
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
	
	c = c.parent;
	
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
	
	c = c.parent;

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

	this.x = this.orig.x;
	this.y = this.orig.y;
	this.width = this.orig.width;
	this.height = this.orig.height;
	
	mtx = this._saved_to_current();
	_transform_bbox(this, mtx);
    },    
};

var _center_proto = {
    _get_ac_saved_rev: function() {
	var c = this.owner;
	var mtx;
	
	c = c.parent;
	
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
	
	c = c.parent;

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
	xy = _pnt_transform(this.orig.x, this.orig.y, mtx);

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

    /*! \brief Center position in the relative space defined by parent.
     */
    get rel() {
	var rev;
	var xy;
	
	rev = this._get_ac_rev();
	xy = _pnt_transform(this.orig.x, this.orig.y, rev);

	return {x: xy[0], y: xy[1]};
    },
};

loadSVG.prototype._set_bbox = function(node, tgt) {
    var a;
    var vstr;
    var bbox, center;
    var orig;

    a = node.attr("bbox-x");
    if(!a) {
	tgt.center = new Object();
	tgt.center.x=0;
	tgt.center.y=0;
	return 0;
    }
    
    /* bbox.orig is initial values of bbox.  The bbox is recomputed
     * according bbox.orig and current matrix.  See bbox.update().
     */
    tgt.bbox = bbox = new Object();
    bbox.orig = orig = new Object();
    bbox.owner = tgt;
    bbox.__proto__ = _bbox_proto;
    vstr = a.value();
    orig.x = parseFloat(vstr);

    a = node.attr("bbox-y");
    vstr = a.value();
    orig.y = this.height - parseFloat(vstr);

    a = node.attr("bbox-width");
    vstr = a.value();
    orig.width = parseFloat(vstr);

    a = node.attr("bbox-height");
    vstr = a.value();
    orig.height = parseFloat(vstr);
    orig.y -= orig.height;
    
    bbox.update();

    /* center.orig is initial values of center.  The center is
     * recomputed according center.orig and current matrix.  See
     * center.update().
     */
    tgt.center = center = new Object();
    center.orig = orig = new Object();
    
    orig.x = bbox.orig.width / 2 + bbox.orig.x;
    orig.y = bbox.orig.height / 2 + bbox.orig.y;
    a = node.attr("transform-center-x");
    if(a) {
	vstr = a.value();
	orig.x += parseFloat(vstr);
	a = node.attr("transform-center-y");
	vstr = a.value();
	orig.y -= parseFloat(vstr);
    }
    center.__proto__ = _center_proto;
    center.owner = tgt;
    center.update();
    
    return 1;
}

loadSVG.prototype._set_paint_style = function(style, tgt) {
    var paint;
    var fill_alpha = 1;
    var stroke_alpha = 1;
    var alpha = 1;
    var fill_color;
    var stroke_color;
    var stroke_width = 1;
    var i;
    
    if(style) {
	if('opacity' in style)
	    alpha = parseFloat(style['opacity']);
	if('fill' in style)
	    fill_color = style['fill'];
	if('fill-opacity' in style)
	    fill_alpha = parseFloat(style['fill-opacity']);
	if('stroke' in style)
	    stroke_color = style['stroke'];
	if('stroke-width' in style)
	    stroke_width = parseFloat(style['stroke-width']);
	if('stroke-opacity' in style)
	    stroke_alpha = parseFloat(style['stroke-opacity']);
	if('display' in style && style['display'] == 'none')
	    return;
    }

    if(fill_color) {
	if(fill_color != "none") {
	    paint = this._prepare_paint_color(fill_color, fill_alpha);
	    paint.fill(tgt);
	}
    }
    if(stroke_color) {
	if(stroke_color != "none") {
	    paint = this._prepare_paint_color(stroke_color, stroke_alpha);
	    paint.stroke(tgt);
	}
    }

    tgt.stroke_width = stroke_width;
    
    if(alpha < 1)
	tgt.parent.opacity = alpha;
};

loadSVG.prototype._set_paint = function(node, tgt) {
    var style;

    style = parse_style(node);
    this._set_paint_style(style, tgt);
};

function formalize_path_data(d) {
    var posM, posm;
    var pos;
    var nums = "0123456789+-.";
    var rel = false;
    var cmd;

    posM = d.search("M");
    posm = d.search("m");
    pos = posm < posM? posm: posM;
    if(pos == -1)
	pos = posM == -1? posm: posM;
    if(pos == -1)
	return d;

    if(posm == pos)
	rel = true;
    
    pos = pos + 1;
    while(pos < d.length && " ,".search(d[pos]) >= 0)
	pos++;
    while(pos < d.length && nums.search(d[pos]) >= 0)
	pos++;
    while(pos < d.length && " ,".search(d[pos]) >= 0)
	pos++;
    while(pos < d.length && nums.search(d[pos]) >= 0)
	pos++;
    while(pos < d.length && " ,".search(d[pos]) >= 0)
	pos++;
    if(nums.search(d[pos]) >= 0) {
	if(rel)
	    cmd = "l";
	else
	    cmd = "L";
	d = d.substring(0, pos) + cmd + formalize_path_data(d.substring(pos));
    }
    return d;
}

loadSVG.prototype.parsePath=function(accu, coord,id, n)
{
    var d = formalize_path_data(n.attr('d').value());
    var style = n.attr('style');
    var path = this.mb_rt.path_new(d);
    var pcoord = this.mb_rt.coord_new(coord);
    pcoord.node = n;
    n.coord = pcoord;
    this._check_duplicate_src(n,pcoord);

    //guessPathBoundingBox(pcoord,d);
    var trans = n.attr('transform');
    if (trans)
        parseTransform(pcoord,trans.value());
    else {
        pcoord.sx = 1;
	pcoord.sy = 1;
	pcoord.r = 0;
	pcoord.tx = 0;
	pcoord.ty = 0;
    }

    pcoord.add_shape(path);
    this._set_paint(n, path);
    this._set_bbox(n, pcoord);

    make_mbnames(this.mb_rt, n, pcoord);
};

loadSVG.prototype.parseText=function(accu,coord,id, n)
{
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
    var tcoord = this.mb_rt.coord_new(coord);
    var style;

    tcoord.node = n;
    n.coord = tcoord;
    this._check_duplicate_src(n,tcoord);

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
    style = parse_style(n);
    var nodes = n.childNodes();
    var k;
    for(k in nodes) {
	var c= nodes[k].name();
	if (c == "tspan") {
	    this.parseTSpan(tcoord,nodes[k],style);
	} else {
	}
    }
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
    if (coord[0]*coord[4] == coord[1]*coord[3]) {
        sys.puts("Singular affine matrix\n");
	coord.sx = 1;
	coord.sy = 1;
	coord.r = 0;
	coord.tx = 0;
	coord.ty = 0;
	return;
    }
    A = coord[0];
    B = coord[3];
    C = coord[1];
    D = coord[4];
    E = coord[2];
    F = coord[5];
    sx = Math.sqrt(A*A+B*B);
    A = A / sx;
    B = B / sx;
    shear = A*C+B*D;
    C = C - A*shear;
    D = D - B*shear;
    sy = Math.sqrt(C*C+D*D);
    C = C / sy;
    D = D / sy;
    r = A*D - B*C;
    if (r == -1) {
        shear = - shear;
	sy = -sy;
    }
    R = Math.atan2(-B,A);
    coord.sx = sx;
    coord.sy = sy;
    coord.r = R;
    coord.tx = E;
    coord.ty = F;
    sys.puts("transform="+s);
    sys.puts("coord[0]="+coord[0]);
    sys.puts("coord[1]="+coord[1]);
    sys.puts("coord[2]="+coord[2]);
    sys.puts("coord[3]="+coord[3]);
    sys.puts("coord[4]="+coord[4]);
    sys.puts("coord[5]="+coord[5]);
    sys.puts("coord.sx="+coord.sx);
    sys.puts("coord.sy="+coord.sy);
    sys.puts("coord.r="+coord.r);
    sys.puts("coord.tx="+coord.tx);
    sys.puts("coord.ty="+coord.ty);
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
    tcoord.node = n;
    n.coord = tcoord;
    this._check_duplicate_src(n,tcoord);
	
    var style = n.attr('style');

    if (trans)
        parseTransform(tcoord,trans.value());
    else {
        tcoord.sx = 1;
	tcoord.sy = 1;
	tcoord.r = 0;
	tcoord.tx = 0;
	tcoord.ty = 0;
    }

    tcoord.tx += x;
    tcoord.ty += y;
    attr = n.attr('duplicate-src');
    if (attr) {
        var id = attr.value();
        var orign = this.mb_rt.mbnames[id].node;
        sys.puts("xxxxxxxxxxxxxx");
        var nw = getInteger(orign,'width');
        var nh = getInteger(orign,'height');
	sys.puts("nw="+nw);
	sys.puts("nh="+nh);
	sys.puts("w="+w);
	sys.puts("h="+h);
	tcoord.sx *= w/nw;
	tcoord.sy *= h/nh;
    }
	
	

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

loadSVG.prototype.duplicateGroup=function(id,root) {
    n =  this._groupMap[id];
    var m = [1,0,0,1,0,0]
    this.parseGroup(m,root,id, n)
}

loadSVG.prototype._check_duplicate_src=function(n,coord) {
    if (n.name()=="use") {
        n.coord.isuse = true
    } else {
        n.coord.isuse = false;
    }
    var attr = n.attr('duplicate-src');
    if (attr == null) return;
    var id = attr.value();
    try {
        this.mb_rt.mbnames[id].target = coord;
    } catch(e) {
        sys.puts("id "+id+" is not defined");
    }
}

loadSVG.prototype.parseGroup=function(accu_matrix,root, group_id, n) {
    var label = n.attr('label');
    if (label && label.value()=='dup') return;
    var k;
    var nodes = n.childNodes();
    var coord = this.mb_rt.coord_new(root);
    // Parse the transform and style here
    var trans = n.attr('transform');
    var accu=[1,0,0,0,1,0];
    var style;
    
    n.coord = coord;
    coord.node = n;
    this._check_duplicate_src(n,coord);

    coord.center= new Object();
    coord.center.x = 10000;
    coord.center.y = 10000;
    if (trans!=null) {
	parseTransform(coord, trans.value());
    } else {
        coord.sx = 1;
	coord.sy = 1;
	coord.r = 0;
	coord.tx = 0;
	coord.ty = 0;
    }
    multiply(accu,accu_matrix);
    multiply(accu,coord);

    style = {};
    parseGroupStyle(style, n);
    if(style.opacity) {
	sys.puts("opacity=" + style.opacity);
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
	} else if (c == "use") {
	    this.parseUse(accu_matrix,coord, id, nodes[k]);
	}
    }
    if (root.center.x > coord.center.x)
	root.center.x = coord.center.x;
    if (root.center.y > coord.center.y)
	root.center.y = coord.center.y;

    this._set_bbox(n, coord);
    // Set the group map only it is not defined before. The group might be 
    // redefined by the svg:use tag
    if (this._groupMap[group_id]==undefined)
        this._groupMap[group_id] = n;
    
    make_mbnames(this.mb_rt, n, coord);
    return coord;
};

loadSVG.prototype.parseUse=function(accu_matrix,root, use_id, n) {
    var k;
    var nodes;
    var coord;
    // Parse the transform and style here
    var trans = n.attr('transform');
    var accu=[1,0,0,0,1,0];
    var style;
    var attr = n.attr('duplicate-src');
    if (attr) {
        var src = this.mb_rt.mbnames[attr.value()];
	coord = root.clone_from_subtree(src);
    } else {
        coord = this.mb_rt.coord_new(root);
    }
    n.coord = coord;
    coord.node = n;
    this._check_duplicate_src(n,coord);

    coord.center= new Object();
    coord.center.x = 10000;
    coord.center.y = 10000;
    sys.puts("use id="+use_id+" "+trans);
    sys.puts(n);
    if (trans!=null) {
	parseTransform(coord, trans.value());
    } else {
        coord.sx = 1;
	coord.sy = 1;
	coord.r = 0;
	coord.tx = 0;
	coord.ty = 0;
        
    }
    multiply(accu,accu_matrix);
    multiply(accu,coord);

    style = {};
    parseGroupStyle(style, n);
    if(style.opacity) {
	sys.puts("opacity=" + style.opacity);
	coord.opacity=style.opacity;
    }

    if (root.center.x > coord.center.x)
	root.center.x = coord.center.x;
    if (root.center.y > coord.center.y)
	root.center.y = coord.center.y;
    this._set_bbox(n, coord);
    this._groupMap[n.name()] = n;
    
    make_mbnames(this.mb_rt, n, coord);
};
loadSVG.prototype.parseImage=function(accu,coord,id, n)
{
    var ref = n.attr('href').value();
    var tcoord = this.mb_rt.coord_new(coord);
    var trans = n.attr('transform');
    n.coord = tcoord;
    tcoord.node = n;
    this._check_duplicate_src(n,tcoord);

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
    tcoord.center= new Object();
    tcoord.center.x = 10000;
    tcoord.center.y = 10000;
    if (trans!=null) {
	parseTransform(coord, trans.value());
    } else { 
        tcoord.sx = 1;
	tcoord.sy = 1;
	tcoord.r = 0;
	tcoord.tx = 0;
	tcoord.ty = 0;
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
    tcoord.tx += x;
    tcoord.ty += y;
    attr = n.attr('duplicate-src');
    if (attr) {
        var id = attr.value();
	var origcoord = this.mb_rt.mbnames[id];
	if (origcoord==null) {
	    sys.puts("Unknown reference "+id);
	} else {
            var orign = origcoord.node;
            var nw = getInteger(orign,'width');
            var nh = getInteger(orign,'height');
	    tcoord.sx *= w/nw;
	    tcoord.sy *= h/nh;
	}
    }
    nx = tcoord[0]*x+tcoord[1]*y+tcoord[2];
    ny = tcoord[3]*x+tcoord[4]*y+tcoord[5];
    if (tcoord.center.x > nx) 
	tcoord.center.x = nx;
    if (tcoord.center.y > ny)
	tcoord.center.y = ny;
    var img = this.mb_rt.image_new(x,y,w,h);
    var img_data = ldr.load(ref);
    try {
        var paint = this.mb_rt.paint_image_new(img_data);
        paint.fill(img);
    } catch(e) {
        sys.puts(e);
        sys.puts("--Can not load image "+ref);
    }
    tcoord.add_shape(img);
    
    this._set_bbox(n, tcoord);
    
    make_mbnames(this.mb_rt, n, tcoord);
};

function _parse_stops(n) {
    var children;
    var child;
    var style;
    var color;
    var rgb;
    var opacity;
    var r, g, b, a;
    var offset_atr, offset;
    var stops = [];
    var i;

    children = n.childNodes();
    for(i = 0; i < children.length; i++) {
	child = children[i];
	if(child.name() == "stop") {
	    style = parse_style(child);
	    
	    color = style["stop-color"];
	    if(color) {
		rgb = parse_color(color);
		r = rgb[0];
		g = rgb[1];
		b = rgb[2];
	    }
	    
	    opacity = style["stop-opacity"];
	    if(opacity)
		a = parseFloat(opacity);
	    else
		a = 1;
	    
	    offset_attr = child.attr("offset");
	    if(offset_attr)
		offset = parseFloat(offset_attr.value());
	    else
		offset = 0;
	    
	    stops.push([offset, r, g, b, a]);
	}
    }

    return stops;
};

loadSVG.prototype._MB_parseLinearGradient=function(root,n)
{
    var id = n.attr('id');
    var k;
    var nodes = n.childNodes();
    var mtx = [1, 0, 0, 0, 1, 0];

    if (id == null) return;
    id = id.value();

    var x1 = n.attr("x1");
    var y1 = n.attr("y1");
    var x2 = n.attr("x2");
    var y2 = n.attr("y2");
    var xy;
    var gr;
    var color, opacity;
    var stops;
    var r,g,b;
    
    if(x1)
	x1 = parseFloat(x1.value());
    if(x2)
	x2 = parseFloat(x2.value());
    if(y1)
	y1 = parseFloat(y1.value());
    if(y2)
	y2 = parseFloat(y2.value());
    
    stops = _parse_stops(n);
    
    var href = n.attr('href');
    if(href != null) {
	href = href.value();
	var hrefid = href.substring(1);
	pstops = this.stop_ref[hrefid];
	if (pstops)
	    stops = pstops.concat(stops);
	
	var hrefgr = this.gradients[hrefid];
	if(typeof x1 == "undefined")
	    x1 = hrefgr[0];
	if(typeof y1 == "undefined")
	    y1 = hrefgr[1];
	if(typeof x2 == "undefined")
	    x2 = hrefgr[2];
	if(typeof y2 == "undefined")
	    y2 = hrefgr[3];
    }

    if(n.attr("gradientTransform")) {
	parseTransform(mtx, n.attr("gradientTransform").value());
	xy = _pnt_transform(x1, y1, mtx);
	x1 = xy[0];
	y1 = xy[1];
	xy = _pnt_transform(x2, y2, mtx);
	x2 = xy[0];
	y2 = xy[1];
    }

    this.stop_ref[id] = stops;
    this.gradients[id] = [x1,y1,x2,y2];
};

loadSVG.prototype._MB_parseRadialGradient = function(root,n) {
    var stops;
    var cx, cy;
    var xy;
    var mtx = [1, 0, 0, 0, 1, 0];
    var id;
    var href;
    var r;

    id = n.attr("id");
    if(!id)
	throw "Require an id";
    id = id.value();

    stops = _parse_stops(n);
    
    cx = n.attr("cx");
    if(!cx)
	throw "Miss cx attribute";
    cy = n.attr("cy");
    if(!cy)
	throw "Miss cy attribute";
    cx = parseFloat(cx.value());
    cy = parseFloat(cy.value());

    r = n.attr("r");
    if(!r)
	throw "Miss r attribute";
    r = parseFloat(r.value());

    href = n.attr("href");
    if(href) {
	href = href.value().substring(1);
	stops = this.stop_ref[href];
    }

    if(n.attr("gradientTransform")) {
	parseTransform(mtx, n.attr("gradientTransform").value());
	xy = _pnt_transform(cx, cy, mtx);
	cx = xy[0];
	cy = xy[1];
    }

    this.radials[id] = [cx, cy, r];
    this.stop_ref[id] = stops;
}

loadSVG.prototype.parseScenes=function(coord,node) {
    var nodes = node.childNodes();

    for(k in nodes) {
        var name = nodes[k].name();
	if (name == 'scene') {
	    var node = nodes[k];

	    scene = new Object();
	    scene.start = parseInt(node.attr('start').value());
	    try { 
	        scene.end = parseInt(node.attr('end').value());
	    } catch(e) {
	        scene.end = scene.start;
	    }
	    scene.ref = node.attr('ref').value();
	    try {
	        scene.type = node.attr('type').value();
	    } catch(e) {
	        scene.type='normal';
	    }

	    try {
	        this.scenenames[node.attr('name').value()] = scene.start;
	    } catch(e) {
	    }
	    this.scenes.push(scene);
	}
    }
}

loadSVG.prototype.parseComponents=function(coord,node) {
    
    var nodes = node.childNodes();

    for(k in nodes) {
        var name = nodes[k].name();
	if (name == 'component') {
	    // Parse the component here
	}
    }
}

loadSVG.prototype.parseMetadata=function(coord,node) {
    var nodes = node.childNodes();

    for(k in nodes) {
        var name = nodes[k].name();
	if (name == 'scenes') {
	    this.scenes=[];
	    this.scenenames={};
	    this.parseScenes(coord,nodes[k]);
	} else if (name == "components") {
	    this.parseComponents(coord,nodes[k]);
	}
    }
}
loadSVG.prototype.parseDefs=function(root,n)
{
    var k;
    var nodes = n.childNodes();
    
    for(k in nodes) {
	var name = nodes[k].name();
	if (name == "linearGradient") {
	    this._MB_parseLinearGradient(root,nodes[k]);
	} else if(name == "radialGradient") {
	    this._MB_parseRadialGradient(root,nodes[k]);
	}
    }
};


exports.loadSVG = loadSVG;
