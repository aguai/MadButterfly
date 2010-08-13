var mbfly = require("mbfly");
var svg = require("./svg");
var mb_rt = new mbfly.mb_rt(":0.0", 720,480);
var ldr = mbfly.img_ldr_new(".");
var background = mb_rt.rect_new(0, 0, 720, 480, 0, 0);
var paint = mb_rt.paint_color_new(1, 1, 1, 1);
paint.fill(background);
mb_rt.root.add_shape(background);

app=function() {
    this.mb_rt = mb_rt;

}
app.prototype.loadSVG=function(fname) {
    svg.loadSVG(mb_rt,mb_rt.root,fname);
}

app.prototype.loop=function() {
    mb_rt.redraw_all();
    mb_rt.flush();
}

exports.app=app;
