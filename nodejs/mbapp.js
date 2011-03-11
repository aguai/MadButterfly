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
}
app.prototype.loadSVG=function(fname) {
    this.svg.load(this.mb_rt,this.mb_rt.root,fname)
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
    if (p == 0) {
	if (src.dup) src.dup.hide();
	src.show();
	return;
    }
    src.hide();
    // Duplicate the group
    if (src.dup == null) {
        dup = src.parent.clone_from_subtree(src);
	src.dup = dup;
    } else {
	dup = src.dup;
    }
    dup.hide();
    sys.puts(dup);


    for(n in dup) {
	
        
    }
}

app.prototype.changeScene=function(s) {
    var nth;
    if (typeof(s)=='number') {
        var i;
	nth = s;
    } else {
        nth = this.svg.getFrameNumber(s);
	if (nth == -1) return;
    }
    var scenes = this.svg.scenes;
    for(i=0;i<scenes.length-1;i++) {
        try {
            if (nth >=scenes[i].start && nth <=scenes[i].end) {
		if (scenes[i].type == 'normal' || i == scenes.length-1) {
	            this.get(scenes[i].ref).show();
		} else if (scenes[i].type == 'scale') {
		    if (scenes[i].end == (scenes[i+1].start-1)) {
			var p = (nth-scenes[i].start)/(scenes[i].end-scenes[i].start+1);
			this.generateScaleTween(this.get(scenes[i].ref),this.get(scenes[i+1].ref),p);
		    } else {
		        // If there is no second key frame defined, fall back to the normal
	                this.get(scenes[i].ref).show();
		    }
		}

	    } else {
	        this.get(scenes[i].ref).hide();
	    }
	} catch(e) {
	    sys.puts(e);
	    sys.puts(scenes[i].ref);
	}
    }
}

app.prototype.runToScene=function(n) {
    
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
