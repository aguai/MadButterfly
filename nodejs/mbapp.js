// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var mbfly = require("mbfly");
var svg = require("./svg");
var sys = require("sys");
var ldr = mbfly.img_ldr_new(".");

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

function _decorate_mb_rt(mb_rt) {
    var coord;
    
    mb_rt._mbapp_saved_coord_new = mb_rt.coord_new;
    mb_rt.coord_new = function(parent) {
	var coord;
	
	coord = this._mbapp_saved_coord_new(parent);
	coord.type = "coord";
	coord.children = [];
	coord._mbapp_saved_mtx = [coord[0], coord[1], coord[2],
				  coord[3], coord[4], coord[5]];
	coord._mbapp_saved_rev_mtx = _reverse(coord._mbapp_saved_mtx);
	coord.parent = parent;
	coord._mbapp_saved_add_shape = coord.add_shape;
	coord.add_shape = function(shape) {
	    var coord;
	    
	    this._mbapp_saved_add_shape(shape);
	    shape.parent = this;
	    this.children.push(shape);
	}

	parent.children.push(coord);
	
	return coord;
    };

    /*
     * Decorate root coord
     */
    coord = mb_rt.root;
    coord.type = "coord";
    coord.children = [];
    coord._mbapp_saved_mtx = [coord[0], coord[1], coord[2],
			      coord[3], coord[4], coord[5]];
	coord._mbapp_saved_rev_mtx = _reverse(coord._mbapp_saved_mtx);
    coord._mbapp_saved_add_shape = coord.add_shape;
    coord.add_shape = function(shape) {
	var coord;
	
	this._mbapp_saved_add_shape(shape);
	shape.parent = this;
    }
}

app=function(display, w, h) {
    var self = this;
    var mb_rt;

    if(typeof display == "undefined")
	display = ":0.0";
    if(typeof w == "undefined")
	w = 720;
    if(typeof h == "undefined")
	h = 480;
    
    mb_rt = this.mb_rt = new mbfly.mb_rt(display, w, h);
    _decorate_mb_rt(mb_rt);
    var background = mb_rt.rect_new(0, 0, 720, 480, 0, 0);
    var paint = mb_rt.paint_color_new(1, 1, 1, 1);
    paint.fill(background);
    mb_rt.root.add_shape(background);

    this.mb_rt.kbevents.add_event_observer(exports.EVT_KB_PRESS, function(evt) { self.KeyPress(evt);});
    this.keymap={};
    this.onKeyPress = null;
    this.svg = new svg.loadSVG(this.mb_rt,this.mb_rt.root,null);
    this.frame_interval = 1000/12; // 12 frame per second
}
app.prototype.loadSVG=function(fname) {
    this.svg.load(this.mb_rt,this.mb_rt.root,fname);
    this.changeScene(0);
}

app.prototype.KeyPress = function(evt) {
    if (this.onKeyPress) this.onKeyPress(evt.sym);
	if (evt.sym in this.keymap) {
	    this.keymap[evt.sym]();
	}
}

app.prototype.loop=function() {
    this.mb_rt.redraw_all();
    this.mb_rt.flush();
}
app.prototype.update=function() {
    this.mb_rt.redraw_all();
    this.mb_rt.flush();
}
app.prototype.get=function(name) {
    return this.mb_rt.mbnames[name];
}
app.prototype.addKeyboardListener=function(type,f) {
    return this.mb_rt.kbevents.add_event_observer(type,f);    
}
app.prototype.refresh=function() {
    this.mb_rt.redraw_changed();
    this.mb_rt.flush();
}
app.prototype.dump=function() {
    sys.puts(this.onKeyPress);
}

app.prototype.addKeyListener=function(key,f) {
    if (typeof(key) == 'number')
        this.keymap[key] = f;
    else {
        for(k in key) {
	    this.keymap[k] = f;
	}
    }
}

app.prototype.generateScaleTween=function(src,dest,p) {
    sys.puts("p="+ p);
    src.hide();
    // Duplicate the group
    var nodes = src.node.childNodes();
    if (src.dup == null) {
        dup = this.mb_rt.coord_new(src.parent);
	for (i in nodes) {
	    n = nodes[i];
	    g = dup.clone_from_subtree(n.coord);
	    n.coord.dup = g;
	}
	src.dup = dup;
    } else {
	dup = src.dup;
	src.dup.show();
    }
    //dup.hide();
    //dest.hide();
    //sys.puts(dup);

    for(i in nodes) {
        n= nodes[i];
	coord = n.coord;
	var attr = n.attr('id');
	if (attr == null) continue;
	var id = attr.value();
	sys.puts("id="+id);
	this.generateScaleTweenObject(coord.dup,coord,coord.target,p);
    }
}
function printcoord(s)
{
    sys.puts(s[0]+" "+s[1]+" "+s[2]+" "+s[3]+" "+s[4]+" "+s[5]);
}

function mul(a,b)
{
    return [a[0]*b[0]+a[1]*b[3], a[0]*b[1]+a[1]*b[4], a[0]*b[2]+a[1]*b[5]+a[2], 
            a[3]*b[0]+a[4]*b[3], a[3]*b[1]+a[4]*b[4], a[3]*b[2]+a[4]*b[5]+a[5]];
}

app.prototype.generateScaleTweenObject=function(coord,src,dest,p) {
    sys.puts("src=["+src.sx+","+src.sy+","+src.r+","+src.tx+","+src.ty);
    sys.puts("dest=["+dest.sx+","+dest.sy+","+dest.r+","+dest.tx+","+dest.ty);
    var p1 = 1-p;
    var sx = src.sx*p+dest.sx*p1;
    var sy = src.sy*p+dest.sy*p1;
    var r = src.r*p+dest.r*p1;
    var tx = src.tx*p+dest.tx*p1;
    var ty = src.ty*p+dest.ty*p1;
    var mt = [1, 0, -src.tx, 0, 1, -src.ty];
    var opacity;
    var ms;

    opacity = src.opacity*p + dest.opacity*p1;
    coord.opacity = opacity;

    if (r == 0) {
        m = mt;
    } else {
        ms= [Math.cos(r), Math.sin(r),0,-Math.sin(r), Math.cos(r),0];
	m = mul(ms,mt);
    }
    m = mul([sx,0,0,0,sy,0],m);
    m = mul([1,0,tx,0,1,ty],m);
    m[2] = -m[2];
    coord[0] = m[0];
    coord[1] = m[1];
    coord[2] = m[2];
    coord[3] = m[3];
    coord[4] = m[4];
    coord[5] = m[5];
    //sys.puts(coord);
    //sys.puts("p="+p+" "+m[0]+","+m[1]+","+m[2]+","+m[3]+","+m[4]+","+m[5]);
}

app.prototype.changeScene=function(s) {
    var nth;
    sys.puts(s);
    sys.puts(typeof(s));
    if (typeof(s)=='number') {
        var i;
	nth = s;
    } else {
        nth = this.svg.getFrameNumber(s);
	if (nth == -1) return;
    }
    sys.puts("goto to scene "+nth);
    this.currentScene = nth;
    var scenes = this.svg.scenes;
    for(i=0;i<scenes.length-1;i++) {
        try {
            if (nth >=scenes[i].start && nth <=scenes[i].end) {
	        sys.puts("find");
		if (scenes[i].type == 'normal' || i == scenes.length-1) {
	            this.get(scenes[i].ref).show();
		} else if (scenes[i].type == 'scale') {
		    sys.puts(i+","+scenes[i+1].start+","+scenes[i].end);
		    if (scenes[i].end == (scenes[i+1].start-1)) {
			var p = 1-(nth-scenes[i].start)/(scenes[i].end-scenes[i].start+1);
			this.generateScaleTween(this.get(scenes[i].ref),this.get(scenes[i+1].ref),p);
		    } else {
		        // If there is no second key frame defined, fall back to the normal
	                this.get(scenes[i].ref).show();
		    }
		    this.get(scenes[i+1].ref).hide();
		}

	    } else {
	        sys.puts("hide "+scenes[i].ref);
	        this.get(scenes[i].ref).hide();
	    }
	} catch(e) {
	    sys.puts(e);
	    sys.puts(scenes[i].ref);
	}
    }
    this.refresh();
}

app.prototype.runToScene=function(s) {
    sys.puts(s);
    sys.puts(typeof(s));
    if (typeof(s)=='number') {
        var i;
	nth = s;
    } else {
        nth = this.svg.getFrameNumber(s);
	if (nth == -1) return;
    }
    var self = this;
    sys.puts(this.currentScene+","+nth);
    if (nth > this.currentScene) {
        this.targetScene = nth;
	this.startScene = this.currentScene;
	this.starttime = Date.now();
        setTimeout(function() {
	    self.skipFrame()
	}, this.frame_interval);
    } else {
        this.changeScene(nth);
    }
}

app.prototype.skipFrame=function() {
    var self = this;
    sys.puts("n="+this.currentScene);
    if (this.currentScene != this.targetScene) {
        nextframe = this.startScene + Math.round((Date.now() - this.starttime)/this.frame_interval);
	if (nextframe > this.targetScene)
	    nextframe = this.targetScene;
	sys.puts("change to scene "+nextframe);
        this.changeScene(nextframe);
        setTimeout(function() {
	    self.skipFrame()
	}, this.frame_interval);
    }
}

app.prototype.addSceneListener=function(n, cb) {
    sys.puts("This is not implemented yet")
}

var app_with_win = function(display, win) {
    var self = this;
    var mb_rt;
    var background;
    var paint;

    if(typeof display == "undefined" || typeof win == "undefined")
	throw "Invalid argument";
    
    mb_rt = this.mb_rt = new mbfly.mb_rt_with_win(display, win);
    _decorate_mb_rt(mb_rt);
    background = mb_rt.rect_new(0, 0, 720, 480, 0, 0);
    paint = mb_rt.paint_color_new(1, 1, 1, 1);
    paint.fill(background);
    mb_rt.root.add_shape(background);

    this.mb_rt.kbevents.
	add_event_observer(exports.EVT_KB_PRESS,
			   function(evt) { self.KeyPress(evt); });
    this.keymap = {};
    this.onKeyPress = null;
}

app_with_win.prototype = app.prototype;

exports.app=app;
exports.app_with_win = app_with_win;

// Put all key definition here
exports.KEY_LEFT = 0xff51;
exports.KEY_UP = 0xff52;
exports.KEY_RIGHT = 0xff53;
exports.KEY_DOWN = 0xff54;
exports.KEY_ENTER = 0xff0d;
exports.EVT_ANY=0;
exports.EVT_MOUSE_OVER=1;
exports.EVT_MOUSE_OUT=2;
exports.EVT_MOUSE_MOVE=3;
exports.EVT_MOUSE_BUT_PRESS4;
exports.EVT_MOUSE_BUT_RELEASE=5
exports.EVT_KB_PRESS=6;
exports.EVT_KB_RELEASE=7;
exports.EVT_PROGM_COMPLETE=8;
exports.EVT_RDMAN_REDRAW=9;
exports.EVT_MONITOR_ADD=10;
exports.EVT_MONITOR_REMOVE=11;
exports.EVT_MONITOR_FREE=12;
exports.EVT_MOUSE_MOVE_RAW=13;
exports.ldr = ldr;
