// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("svg");
var mbapp = require("mbapp");
var sys=require("sys");
var animate=require("animate");

app = new mbapp.app();

coord_parent = app.mb_rt.coord_new(app.mb_rt.root);
coord = app.mb_rt.coord_new(coord_parent)
data=mbapp.ldr.load("sample.png");
paint = app.mb_rt.paint_image_new(data);
img = app.mb_rt.image_new(10,10,50,50);
paint.fill(img);
coord.opacity = 0.9;
coord.add_shape(img);

var shift = 0;
function translate_handler() {
    coord_parent[2] = shift;
    app.mb_rt.redraw_changed();
    setTimeout(translate_handler, 100);
    shift = shift + 10;
}

translate_handler();
app.loop()
