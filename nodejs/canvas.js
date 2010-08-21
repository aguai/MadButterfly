var mbfly = require("mbfly");
var sys=require("sys");


function canvas(app,root) {
    this.mb_rt = app.mb_rt;
    this.root = this.mb_rt.coord_new(root);
	this.bg_r = 0;
	this.bg_g = 0;
	this.bg_b = 0;
	this.bg_a = 0;
	this.stroke_r = 0;
	this.stroke_g = 0;
	this.stroke_b = 0;
	this.stroke_a = 0;
	this.stroke_w = 1;
}

canvas.prototype.background=function(r,g,b,a) {
    this.bg_r = r;
	this.bg_g = g;
	this.bg_b = b;
	this.bg_a = a;
}

canvas.prototype.rect=function(x,y,w,h) {
    var rect = this.mb_rt.rect_new(x,y,w,h,0,0);
    var paint = this.mb_rt.paint_color_new(this.bg_r,this.bg_g,this.bg_b,this.bg_a);
	paint.fill(rect);
	this.root.add_shape(rect);
}

canvas.prototype.stroke=function(r,g,b,a) {
    this.stroke_r = r;
    this.stroke_g = g;
    this.stroke_b = b;
    this.stroke_a = a;
}


canvas.prototype.line=function(x1,y1,x2,y2) {
    var s = "M "+x1+","+y1+" L "+x2+","+y2;
	sys.puts(s);

    var p = this.mb_rt.path_new(s);
	this.root.add_shape(p);
	var paint = this.mb_rt.paint_color_new(this.stroke_r,this.stroke_g,this.stroke_b,this.stroke_a);
	paint.stroke(p);
	p.stroke_width = this.stroke_w;
}

canvas.prototype.strokeWeight=function(w) {
    this.stroke_w = w;
}


exports.canvas = canvas;
