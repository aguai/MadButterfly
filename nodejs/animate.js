// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var sys=require("sys");

/*
 * This is configuration for animate module.  For slower or speeder
 * machines, ffs can be decreased or increased respective.
 */
var ffs = 12;
var frame_interval = 1000 / ffs;

function shift_draw(percent) {
    var x, y;
    
    x = (this.targetx - this.startposx) * percent + this.startposx;
    y = (this.targety - this.startposy) * percent + this.startposy;
    this.obj.center.move(x, y);
    this.app.refresh();
}

function shift(app,obj,shiftx,shifty) {
    obj.animated_shift = this;
    this.app = app;
    this.obj = obj;
    this.end = 0;
    this.targetx = shiftx + obj.center.x;
    this.targety = shifty + obj.center.y;
    this.startposx = obj.center.x;
    this.startposy = obj.center.y;
}

exports.shift = shift;
shift.prototype.draw = shift_draw;

/* ------------------------------------------------------------ */
function rotate(app, obj, ang, duration) {
    this._app = app;
    this._obj = obj;
    this._ang = ang;
    this._start_mtx = [obj[0], obj[1], obj[2], obj[3], obj[4], obj[5]];
}

function rotate_draw(percent) {
    var percent;
    var ang;
    var sv, cv;
    var obj = this._obj;
    var mtx, shift;


    ang = percent * this._ang;
    sv = Math.sin(ang);
    cv = Math.cos(ang);
    mtx = [cv, -sv, 0, sv, cv, 0];
    sys.puts('x='+obj.center.x+',y='+obj.center.y);
    shift = [1, 0, -obj.center.x, 0, 1, -obj.center.y];
    mtx = multiply(mtx, shift);
    shift = [1, 0, obj.center.x, 0, 1, obj.center.y];
    mtx = multiply(shift, mtx);
    mtx = multiply(mtx, this._start_mtx);

    obj[0] = mtx[0];
    obj[1] = mtx[1];
    obj[2] = mtx[2];
    obj[3] = mtx[3];
    obj[4] = mtx[4];
    obj[5] = mtx[5];

    this._app.refresh();
}

rotate.prototype.draw = rotate_draw;
exports.rotate = rotate;

function multiply(s,d) {
    var m=[];
    m[0] = s[0]*d[0]+s[1]*d[3];
    m[1] = s[0]*d[1]+s[1]*d[4];
    m[2] = s[0]*d[2]+s[1]*d[5]+s[2];
    m[3] = s[3]*d[0]+s[4]*d[3];
    m[4] = s[3]*d[1]+s[4]*d[4];
    m[5] = s[3]*d[2]+s[4]*d[5]+s[5];
    return m;
}


function scale_draw(percent) {
    var sx = 1 + (this.totalsx - 1) * percent;
    var sy = 1 + (this.totalsy - 1) * percent;
    var sh1 = [1, 0, -this.center_x, 0, 1, -this.center_y];
    var sh2 = [1, 0, this.center_x, 0, 1, this.center_y];
    var scale = [sx, 0, 0, 0, sy, 0];
    var obj = this.obj;
    var mtx;

    mtx = multiply(scale, sh1);
    mtx = multiply(sh2, mtx);
    mtx = multiply(this.orig_mtx, mtx);
    obj[0] = mtx[0];
    obj[1] = mtx[1];
    obj[2] = mtx[2];
    obj[3] = mtx[3];
    obj[4] = mtx[4];
    obj[5] = mtx[5];

    this.app.refresh();
}

function scale(app, obj, fact_x, fact_y, duration) {
    var bbox;
    
    try {
        if (obj.animated_scale) {
	    obj.animated_scale.end = 1;
	}
    } catch(e) {
	    
    }

    bbox = obj.bbox;
    bbox.update();
    obj.animated_scale = this;
    this.app = app;
    this.obj = obj;
    this.end = 0;
    this.starttime = Date.now();
    this.totalsx = fact_x * bbox.orig.width / bbox.width;
    this.totalsy = fact_y * bbox.orig.height / bbox.height;
    this.duration = duration*1000;
    this.center_x = obj.center.rel.x;
    this.center_y = obj.center.rel.y;
    this.orig_mtx = [obj[0], obj[1], obj[2], obj[3], obj[4], obj[5]];
}


exports.scale = scale;
scale.prototype.draw = scale_draw;

function holder(app, coord) {
    var mtx = [coord[0], coord[1], coord[2], coord[3], coord[4], coord[5]];
    
    this._mtx = mtx;
    this._coord = coord;
    this._app = app;
}

holder.prototype = {
    go: function(pos) {
	var sx, sy;

	sx = pos.x - this._coord.center.x;
	sy = pos.y - this._coord.center.y;
	this.shift(sx, sy);
    },

    go_center: function(o) {
	this.go(o.center);
    },
    
    home: function() {
	this._coord[2] = this._mtx[2];
	this._coord[5] = this._mtx[5];
	this._app.refresh();
    },

    shift: function(sx, sy) {
	this._coord[2] = this._mtx[2] + sx;
	this._coord[5] = this._mtx[5] + sy;
	this._app.refresh();
    }
};

exports.holder = holder;



function alpha_draw(percent) {

    if (this.end == 1) return;
    var sx = (this.targetalpha-this.startalpha)*percent+this.startalpha;
    this.obj.opacity=sx;
    this.app.refresh();
    this.obj.animated_alpha = null;
}

function alpha(app,obj,alpha, duration) {
    try {
        if (obj.animated_alpha) {
	    obj.animated_alpha.end = 1;
	}
    } catch(e) {
	    
    }
    obj.animated_alpha = this;
    this.app = app;
    this.obj = obj;
    this.end = 0;
    this.starttime = Date.now();
    this.startalpha = obj.opacity;
    this.targetalpha = alpha;
    this.duration = duration*1000;
}

alpha.prototype.draw = alpha_draw;
exports.alpha = alpha;

function linear_update()
{
    var now = Date.now();
    var i;
    
    sys.puts("real time is "+now);
    sys.puts("end is "+this.end);
    if (now >= this.end) {
        this.timer.stop();
	now = this.end;
    }
    if (now < this.startmove) return;
    sys.puts("now is "+now+" offset is "+(now-this.startmove));
    var per = (now-this.startmove)/this.duration/1000;
    if (per > 1) per = 1;
    this.action.draw(per);
}

function linear_start()
{
    var self = this;
    if (this.timer)
        this.timer.stop();
    this.timer = setInterval(function() { self.update();}, frame_interval);
    this.startmove = Date.now()+this.starttime*1000;
    this.end = this.startmove+this.duration*1000;
}
function linear_stop() 
{
    if (this.timer) {
        this.timer.stop();
	this.timer = null;
    }
}

function linear_finish()
{
    this.action.draw(1);
}
function linear(action,start, duration) 
{
    this.action = action;
    this.duration = duration;
    this.starttime = start;
    this.timer=null;
}
linear.prototype.update = linear_update;
linear.prototype.start = linear_start;
linear.prototype.stop = linear_stop;
linear.prototype.finish = linear_finish;
exports.linear = linear;


function program(words)
{
    this.words = wrods;
}

program.prototype.start=function() {
    for(w in this.words) {
        w.start();
    }
}

program.prototype.stop=function(s) {
    for(w in this.words) {
        w.stop();
    }
}
program.prototype.finish=function() {
    for(w in this.words) {
        w.finish();
    }
}

exports.run = function(actions,start,duration) {
    for(a in actions) {
        var li = new linear(actions[a],start,duration);
	sys.puts(li);
	li.start();
    }
}
exports.runexp=function(actions,start,exp) {
}
