// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("./svg");
var mbapp = require("./mbapp");
var sys=require("sys");
var animate=require("./animate");
var fs = require("fs");

app = new mbapp.app();
app.loadSVG("desktop.svg");

video = app.get("video");
audio = app.get("audio");
picture = app.get("picture");
setting = app.get("setting");
items=[video,audio,picture,setting];
item = 0;
an = new animate.scale(app,items[item],1,1.5,0.1);
an.start();
setInterval(function() {
    

}, 300);
app.addKeyListener(mbapp.KEY_LEFT, function() {
	var old = items[item];
	item = item - 1;
	if (item == -1) {
		item = 0;
		return;
	}
    var target = items[item];
	var an = new animate.scale(app,old,1,1,0.1);
    an.start();
	an = new animate.scale(app,target,1,1.5,0.3);
    an.start();
});

app.addKeyListener(mbapp.KEY_RIGHT, function() {
	var old = items[item];
	item = item + 1;
	if (item == items.length) {
		item = item - 1;
		return;
	}
    var target = items[item];
	var an = new animate.scale(app,old,1,1,0.1);
    an.start();
	an = new animate.scale(app,target,1,1.5,0.3);
    an.start();
});

app.loop();
