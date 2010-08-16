var sys=require("sys");

function linear_draw() {
    if (this.end == 1) return;
    var percent = (Date.now() - this.starttime)/this.duration;
	if (percent > 1) percent = 1;
    this.obj[5] = (this.target-this.startpos)*percent+this.startpos;
	this.app.refresh();
	var self = this;
	if (percent < 1) {
	    this.obj.timer=setTimeout(function() { self.draw();}, 20);
		return;
	}
	this.app.refresh();
	this.obj.animated_linear = null;
}
function linear(app,obj,target,duration) {
    try {
        if (obj.animated_linear) {
	        obj[5] = obj.animated_linear.target;
			obj.animated_linear.end = 1;
		}
	} catch(e) {
	    
	}
	obj.animated_linear = this;
	this.app = app;
	this.obj = obj;
	this.end = 0;
	this.starttime = Date.now();
	this.startpos = obj[5];
    this.target = target;
	this.duration = duration*1000;
}


exports.linear = linear;
linear.prototype.start = linear_draw;
linear.prototype.draw = linear_draw;
