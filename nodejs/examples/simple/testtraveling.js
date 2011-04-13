// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("svg");
var mbapp = require("mbapp");
var sys=require("sys");
var animate=require("animate");

app = new mbapp.app();

coord_parent = app.mb_rt.coord_new(app.mb_rt.root);
coord0 = app.mb_rt.coord_new(coord_parent)
coord = app.mb_rt.coord_new(coord_parent)
data=mbapp.ldr.load("sample.png");
paint = app.mb_rt.paint_image_new(data);
img = app.mb_rt.image_new(10,10,50,50);
paint.fill(img);
coord.add_shape(img);

sys.puts(coord_parent.num_children() == 2);
sys.puts(coord_parent.get_child(0) == coord0);
sys.puts(coord_parent.get_child(0) != coord);
sys.puts(coord_parent.get_child(1) == coord);
sys.puts(coord_parent.get_child(1).num_children() == 1);
sys.puts(coord_parent.get_child(1).get_child(0) == img);
