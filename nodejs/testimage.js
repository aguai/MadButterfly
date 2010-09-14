// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("./svg");
var mbapp = require("./mbapp");
var sys=require("sys");
var animate=require("./animate");
var fs = require("fs");

app = new mbapp.app();

coord = app.mb_rt.coord_new(app.mb_rt.root);
data=mbapp.ldr.load("sample.png");
paint = app.mb_rt.paint_image_new(data);
img = app.mb_rt.image_new(10,10,50,50);
paint.fill(img);
coord.opacity = 0.9;
coord.add_shape(img);

app.loop();
