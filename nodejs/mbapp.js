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
    svg.loadSVG(this.mb_rt,this.mb_rt.root,fname);
}

app.prototype.loop=function() {
    this.mb_rt.redraw_all();
    this.mb_rt.flush();
}
app.prototype.get=function(name) {
    return this.mb_rt.mbnames[name];
}
app.prototype.addKeyboardListener=function(type,f) {
    return this.mb_rt.kbevents.add_event_observer(type,f);    
}

exports.app=app;

// Put all key definition here
exports.KEY_UP=111;
exports.KEY_DOWN=116;
