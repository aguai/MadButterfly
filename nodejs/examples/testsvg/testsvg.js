// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("svg");
var mbapp = require("mbapp");
var sys=require("sys");
var animate=require("animate");
var fs = require("fs");

app = new mbapp.app();
app.loadSVG("test.svg");
lightbar = app.get("item_lightbar");
item=1;
var target = app.get("item" + item);
lightbar.center.move_pnt(target.center);

app.files=fs.readdirSync("/tmp/");
for(i=1;i<10;i++) {
    var o = app.get("item"+i+"text");
    o.set_text(app.files[i]);
}




app.addKeyListener(mbapp.KEY_UP, function() {
    item = item - 1;
    if (item == 0)
	item = 1;
    else {
	var target = app.get("item"+item);
	var shx = target.center.x - lightbar.center.x;
	var shy = target.center.y - lightbar.center.y;
	var action = new animate.shift(app, lightbar, shx, shy);
	var an = new animate.linear(action, 0, 0.3);
	an.start();
    }
});

app.addKeyListener(mbapp.KEY_DOWN, function() {
    item = item + 1;
    if (item == 10) {
	item = 9;
    } else {
	var target = app.get("item"+item);
	var shx = target.center.x - lightbar.center.x;
	var shy = target.center.y - lightbar.center.y;
	var action = new animate.shift(app, lightbar, shx, shy);
	var an = new animate.linear(action, 0, 0.3);
	an.start();
    }
});

app.loop();
