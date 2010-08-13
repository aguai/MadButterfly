var libxml = require('libxmljs');
var sys=require('sys');
var mbfly = require("mbfly");
var mb_rt = new mbfly.mb_rt(":0.0", 720,480);
var ldr = mbfly.img_ldr_new(".");

function MB_loadSVG(mb_rt,root,filename) {
    var doc = libxml.parseXmlFile(filename);
    var nodes = doc.root().childNodes();
    var coord = mb_rt.coord_new(root);
	var k;

    for(k in nodes) {
	    var n = nodes[k].name();
		if (n == "defs") {
		    _MB_parseDefs(root,nodes[k]);
		} else if (n == "g") {
		    _MB_parseGroup(root,'root_coord',nodes[k]);
		} 
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

function _MB_parseTSpan(coord, n,style)
{
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
	var tcoord = mb_rt.coord_new(coord);
	var nodes = n.childNodes();
	var k;

    sys.puts(n.text());
    var obj = mb_rt.stext_new(n.text(),x,y);
    parseTextStyle(style,n);
    style.paint = mb_rt.paint_color_new(1,1,1,1);
    style.face=mb_rt.font_face_query(style.family, 2, 100);
	obj.set_style([[20,style.face,style.fs]]);
	style.paint.fill(obj);
	tcoord.add_shape(obj);
	for(k in nodes) {
	    var name = nodes[k].name();
		if (name == "tspan") {
		    _MB_parseTSpan(tcoord,nodes[k]);
		} else {
		}
	}
}

function _prepare_paint_color(color, alpha) {
    var paint;
    
    if (color[0]=='#') {
	var r,g,b;
	r = parseInt(color.substring(1,3),16)/256;
	g = parseInt(color.substring(3,5),16)/256;
	b = parseInt(color.substring(5,7),16)/256;
	paint = mb_rt.paint_color_new(r, g, b, alpha);
    } else {
	paint = mb_rt.paint_color_new(0,0,0,1);
    }
    return paint;
}

function _MB_parsePath(coord,id, n)
{
    var d = n.attr('d').value();
    var style = n.attr('style');
    var path = mb_rt.path_new(d);
    var paint;
    
    if (style==null) {
	paint = mb_rt.paint_color_new(0,0,0,0.1);
	paint.stroke(path);
    } else {
	var items = style.value().split(';');
	var fill_alpha = 1;
	var stroke_alpha = 1;
	var fill_color;
	var stroke_color;
	var alpha;
	
	for(i in items) {
	    sys.puts(items[i]);
	    var f = items[i].split(':');
	    if (f[0] == 'opacity') {
		alpha = f[1];
	    } else if (f[0] == 'fill') {
		if(f[1] != "none")
		    fill_color = f[1];
	    } else if (f[0] == 'fill-opacity') {
		fill_alpha = parseFloat(f[1]);
	    } else if (f[0] == 'stroke') {
		if(f[1] != "none")
		    stroke_color = f[1];
	    } else if (f[0] == 'stroke-width') {
		path.stroke_width = parseFloat(f[1]);
	    } else if (f[0] == 'stroke-opacity') {
		stroke_alpha = parseFloat(f[1]);
	    }
	}

	if(fill_color) {
	    paint = _prepare_paint_color(fill_color, fill_alpha);
	    paint.fill(path);
	}
	if(stroke_color) {
	    paint = _prepare_paint_color(stroke_color, stroke_alpha);
	    paint.stroke(path);
	}

    }
    coord.add_shape(path);
}

function _MB_parseText(coord,id, n)
{
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
	var tcoord = mb_rt.coord_new(coord);
    var style = new Object();
	style.fs = 20;
	style.family = 'courier';
	parseTextStyle(style,n);
	var nodes = n.childNodes();
	var k;
	for(k in nodes) {
	    var n = nodes[k].name();
		if (n == "tspan") {
	        _MB_parseTSpan(tcoord,nodes[k],style);
		} else {
		}
	}
	
    
}

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
		coord[0] = 1;
		coord[1] = 0;
		coord[2] = x;
		coord[3] = 0;
		coord[4] = 1;
		coord[5] = y;
	}
	off = s.indexOf('matrix');
	if (off != -1) {
		var end = s.indexOf(')');
		var m = s.substring(7,end);
		var fields = m.split(',');
		coord[0] = parseFloat(fields[0]);
		coord[1] = parseFloat(fields[2]);
		coord[2] = parseFloat(fields[4]);
		coord[3] = parseFloat(fields[1]);
		coord[4] = parseFloat(fields[3]);
		coord[5] = parseFloat(fields[5]);
	}
}

function _MB_parseRect(coord, id, n) 
{
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
    var w = getInteger(n,'width');
    var h = getInteger(n,'height');
	var paint;
	
	var style = n.attr('style');

	if (style==null) {
		paint = mb_rt.paint_color_new(0,0,0,0.1);
	} else {
	    var items = style.value().split(';');
		var fill = '';
		var alpha;
		for(i in items) {
		    sys.puts(items[i]);
			var f = items[i].split(':');
			if (f[0] == 'opacity') {
			    alpha = f[1];
			} else if (f[0] == 'fill') {
			    fill = f[1];
			} else if (f[0] == 'fill-opacity') {
			} else if (f[0] == 'stroke') {
			} else if (f[0] == 'stroken-width') {
			} else if (f[0] == 'stroke-opacity') {
			}
		}
		sys.puts("fill="+fill);
		if (fill[0]=='#') {
		    var r,g,b;
			r = parseInt(fill.substring(1,3),16)/256;
			g = parseInt(fill.substring(3,5),16)/256;
			b = parseInt(fill.substring(5,7),16)/256;
			sys.puts("r="+r+" g="+g+" b="+b+" a="+alpha);

		    paint = mb_rt.paint_color_new(r,g,b,parseFloat(alpha));
		} else {
	        paint = mb_rt.paint_color_new(0,0,0,1);
		}
	}
	var rect = mb_rt.rect_new(x,y,w,h,10, 10);
	sys.puts("rect x="+x+" y="+y+" w="+w+" h="+h);
	paint.fill(rect);
	coord.add_shape(rect);
}

function _MB_parseGroup(root, group_id, n)
{
    var k;
    var nodes = n.childNodes();
    var coord = mb_rt.coord_new(root);
	// Parse the transform and style here
	var trans = n.attr('transform');
	if (trans!=null) {
	    parseTransform(coord, trans.value());
	}

	for(k in nodes) {
	    var n = nodes[k].name();
		var attr = nodes[k].attr('id');
		var id;
		if (attr) {
		    id = attr.value();
		}
		if (n == "g") {
		    _MB_parseGroup(coord, id, nodes[k]);
		} else if (n == "path") {
		    _MB_parsePath(coord, id, nodes[k]);
		} else if (n == "text") {
		    _MB_parseText(coord, id, nodes[k]);
		} else if (n == "rect") {
		    _MB_parseRect(coord, id, nodes[k]);
		} else if (n == "image") {
			_MB_parseImage(coord, id, nodes[k]);
		}
	}
    
}

function _MB_parseImage(coord,id, n)
{
    sys.puts("---> image");
	var ref = n.attr('href').value();

	if (ref == null) return;
	sys.puts(ref);
	if (ref.substr(0,7) == "file://") {
	    ref = ref.substring(7);
	} else if (ref.substr(0,5)=="file:") {
	    ref = ref.substring(5);
	} else {
	    return;
	}
	sys.puts("Load image "+ref);
	var w;
	var h;
	var x,y;

	w = n.attr("width");
	if (w == null) return;
	w = parseInt(w.value());
	h = n.attr("height");
	if (h == null) return;
	h = parseInt(h.value());	
	x = n.attr("x");
	if (x == null) return;
	x = parseInt(x.value());
	y = n.attr("y");
	if (y == null) return;
	y = parseInt(y.value());
	sys.puts("x="+x+",y="+y+",w="+w+",h="+h);
	var img = mb_rt.image_new(x,y,w,h);
	var img_data = ldr.load(ref);
	sys.puts(img_data);
	var paint = mb_rt.paint_image_new(img_data);
	paint.fill(img);
	coord.add_shape(img);
}

function _MB_parseDefs(root,n) 
{
    var k;
	var nodes = n.childNodes();
    
	for(k in nodes) {
	    var name = nodes[k].name();
	    if (name == "linearGradient") {
		    //_MB_parseLinearGradient(root,nodes[k]);
		}
	}
}


MB_loadSVG(mb_rt,mb_rt.root,"test.svg");
mb_rt.redraw_all();
mb_rt.flush();
