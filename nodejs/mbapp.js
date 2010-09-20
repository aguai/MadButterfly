// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var mbfly = require("mbfly");
var svg = require("./svg");
var sys = require("sys");
var ldr = mbfly.img_ldr_new(".");

function _decorate_mb_rt(mb_rt) {
    mb_rt._mbapp_saved_coord_new = mb_rt.coord_new;
    mb_rt.coord_new = function(parent) {
	var coord;
	
	coord = this._mbapp_saved_coord_new(parent);
	coord._mbapp_saved_mtx = [coord[0], coord[1], coord[2],
				  coord[3], coord[4], coord[5]];
	return coord;
    };
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
}
app.prototype.loadSVG=function(fname) {
    svg.loadSVG(this.mb_rt,this.mb_rt.root,fname);
}

app.prototype.KeyPress = function(evt) {
    sys.puts(evt.sym);
    if (this.onKeyPress) this.onKeyPress(evt.sym);
	if (evt.sym in this.keymap) this.keymap[evt.sym]();
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
    this.mb_rt.redraw_all();
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

exports.app=app;

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
