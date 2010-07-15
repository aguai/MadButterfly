var libxml = require('libxmljs');
var sys=require('sys');
var mbfly = require("mbfly");
var mb_rt = new mbfly.mb_rt(":0.0", 720,480);

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

function parseTextStyle(obj,n)
{
	var attr;
    if (n) {
        attr = n.attr('style');
	} else {
	    attr = null;
	}
	var fs = 20;
	var family="ciurier";
	if (attr == null) {
	    var paint = mb_rt.paint_color_new(1,1,1,1);
	    var face=mb_rt.font_face_query(family, 2, 100);
	    obj.set_style([[20,face,fs]]);
		return paint;
	}
	var f = attr.value().split(';');

	for(i in f) {
	    var kv = f[i].split(':');
		if (kv[0] == 'font-size') {
		    fs = parsePointSize(kv[1]);
		} else if (kv[0] == "font-style") {
		} else if (kv[0] == "font-weight") {
		} else if (kv[0] == "fill") {
		} else if (kv[0] == "fill-opacity") {
		} else if (kv[0] == "stroke") {
		} else if (kv[0] == "stroke-width") {
		} else if (kv[0] == "stroke-linecap") {
		} else if (kv[0] == "stroke-linejoin") {
		} else if (kv[0] == "stroke-lineopacity") {
		} else if (kv[0] == "font-family") {
		} else if (kv[0] == "font-stretch") {
		} else if (kv[0] == "font-variant") {
		} else if (kv[0] == "text-anchor") {
		} else if (kv[0] == "text-align") {
		} else if (kv[0] == "writing-mode") {
		} else if (kv[0] == "line-height") {
		    sys.puts("Unknown style: "+kv[0]);
		}
	}
	var paint = mb_rt.paint_color_new(1,1,1,1);
	var face=mb_rt.font_face_query(family, 2, 100);
	obj.set_style([[20,face,fs]]);
	return paint;
}

function _MB_parseTSpan(coord, n)
{
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
	var tcoord = mb_rt.coord_new(coord);
	var nodes = n.childNodes();
	var k;

    sys.puts(n.text());
    var obj = mb_rt.stext_new(n.text(),x,y);
    var paint = parseTextStyle(obj,n);
	paint.fill(obj);
	tcoord.add_shape(obj);
	for(k in nodes) {
	    var name = nodes[k].name();
		if (name == "tspan") {
		    _MB_parseTSpan(tcoord,nodes[k]);
		} else {
		}
	}
}

function _MB_parseText(coord,id, n)
{
    var x = getInteger(n,'x');
    var y = getInteger(n,'y');
	var tcoord = mb_rt.coord_new(coord);
    //var paint = parseTextStyle(n);
	var nodes = n.childNodes();
	var k;
	for(k in nodes) {
	    var n = nodes[k].name();
		if (n == "tspan") {
	        _MB_parseTSpan(tcoord,nodes[k]);
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
		x = parseInt(f[0]);
		y = parseInt(f[1]);
        coord[2] = x;
		coord[5] = y;
	}
	off = s.indexOf('matrix');
	if (off != -1) {
	    sys.puts("matrix");
	}
}

function _MB_parseRect(coord, id, n) 
{
    
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
		} else if (n == "text") {
		    _MB_parseText(coord, id, nodes[k]);
		} else if (n == "rect") {
		    _MB_parseRect(coord, id, nodes[k]);
		}
	}
    
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
