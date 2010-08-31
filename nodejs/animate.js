// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4:ai
var sys=require("sys");

function linear_draw() {
    if (this.end == 1) return;
    var percent = (Date.now() - this.starttime)/this.duration;
	if (percent > 1) percent = 1;
    this.obj[2] = (this.targetx-this.startposx)*percent+this.startposx;
    this.obj[5] = (this.targety-this.startposy)*percent+this.startposy;
	this.app.refresh();
	var self = this;
	if (percent < 1) {
	    this.obj.timer=setTimeout(function() { self.draw();}, 20);
		return;
	}
	this.app.refresh();
	this.obj.animated_linear = null;
}
function linear(app,obj,targetx,targety,duration) {
    try {
        if (obj.animated_linear) {
	    obj[5] = obj.animated_linear.targety;
	    obj[2] = obj.animated_linear.targetx;
	    obj.animated_linear.end = 1;
	}
    } catch(e) {
	    
    }
    obj.animated_linear = this;
    this.app = app;
    this.obj = obj;
    this.end = 0;
    this.starttime = Date.now();
    this.startposx = obj[2];
    this.startposy = obj[5];
    this.targetx = targetx;
    this.targety = targety;
    this.duration = duration*1000;
}

exports.linear = linear;
linear.prototype.start = linear_draw;
linear.prototype.draw = linear_draw;

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
}


function scale_draw() {
    if (this.end == 1) return;
    var percent = (Date.now() - this.starttime)/this.duration;
    if (percent > 1) percent = 1;
    var sx = (this.targetx-this.startsx)*percent+this.startsx;
    var sy = (this.targety-this.startsy)*percent+this.startsy;
    var t=[sx,0,0,0,sy,0];
    this.obj[0] = sx;
    this.obj[4] = sy;
    this.obj[2] = this.origin_offset_x - (sx-this.startsx)*this.obj.center.x;
    this.obj[5] = this.origin_offset_y - (sy-this.startsy)*this.obj.center.y;

    this.app.refresh();
    var self = this;
    if (percent < 1) {
	this.obj.timer=setTimeout(function() { self.draw();}, 20);
	return;
    }
    this.app.refresh();
    this.obj.animated_scale = null;
}

function scale(app,obj,targetx,targety, duration) {
    try {
        if (obj.animated_scale) {
	    //obj[0] = obj.animated_scale.targetx;
	    //obj[4] = obj.animated_scale.targety;
	    //obj[2] = obj.animated_scale.final_offset_x;
	    //obj[5] = obj.aninated_scale.final_offset_y;
	    obj.animated_scale.end = 1;
	}
    } catch(e) {
	    
    }
    obj.animated_scale = this;
    this.app = app;
    this.obj = obj;
    this.end = 0;
    this.starttime = Date.now();
    this.startsx = obj[0];
    this.startsy = obj[4];
    this.targetx = targetx;
    this.targety = targety;
    this.duration = duration*1000;
    this.origin_offset_x = obj[2];
    this.origin_offset_y = obj[5];
    this.final_offset_x = this.origin_offset_x-(targetx-this.startsx)*obj.center.x;
    this.final_offset_y = this.origin_offset_y-(targety-this.startsy)*obj.center.y;
}


exports.scale = scale;
scale.prototype.start = scale_draw;
scale.prototype.draw = scale_draw;

function holder(app, coord) {
    var mtx = [coord[0], coord[1], coord[2], coord[3], coord[4], coord[5]];
    
    this._mtx = mtx;
    this._coord = coord;
    this._app = app;
}

holder.prototype = {
    go_center: function(o) {
	var sx, sy;

	sx = o.center_x - this._coord.center_x;
	sy = o.center_y - this._coord.center_y;
	this.shift(sx, sy);
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
