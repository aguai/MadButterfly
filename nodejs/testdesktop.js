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
//var an = new animate.alpha(app,video,0,1);
//an.start();
audio = app.get("audio");
picture = app.get("picture");
setting = app.get("setting");

lightbar = app.get("lightbar");
lines=[app.get("line1"),app.get("line2"),app.get("line3"), app.get("line4"),app.get("line5")];
for(i=0;i<lines.length;i++) {
    sys.puts("["+i+"]="+lines[i].center.x);
}
line=0;

items=[video,audio,picture,setting];
item = 0;
an = new animate.scale(app,items[item],1,1.5,0.1);
an.start();
app.refresh();
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

app.addKeyListener(mbapp.KEY_UP, function() {
    var old = lines[line];
    line = line - 1;
    if (line == -1) {
	line = 0;
	return;
    }
    var target = lines[line];
    var sy = target.center.y - 20 - lightbar.center.y;
    sys.puts(sy);
    var an = new animate.linear(app,lightbar,0,sy,0.3);
    an.start();
});
app.addKeyListener(mbapp.KEY_DOWN, function() {
    var old = lines[line];
    line = line + 1;
    if (line == lines.length) {
	line = line - 1; 
	return;
    }
    var target = lines[line];
    sys.puts(target);
    var sy = target.cnter.y - 20 - lightbar.center.y;
    sys.puts("line="+line);
    sys.puts("sy="+sy);
    sys.puts("target.y="+target.center.y);
    sys.puts("lightbar.y="+lightbar.center.y);
    var an = new animate.linear(app,lightbar,0,sy,0.3);
    an.start();
});

app.addKeyListener(mbapp.KEY_ENTER, function() {
    var target = items[item];
    var sx = 500 - target.center.x;
    var sy = 220 - target.center.y;
    sys.puts("target "+sx+','+sy);
    var an = new animate.linear(app,target,sx,sy,1);
    an.start();
    for(i=0;i<items.length;i++) {
	if (i == item) continue;
	var x = Math.random();
	var y = Math.random();
	if (x > 0.5) x = 900;
	else x = -500;
	if (y > 0.5) y = 900;
	else y = -500;
	sx = x - items[i].center.x;
	sy = y - items[i].center.y;
	an = new animate.linear(app,items[i], sx,sy,2);
	an.start();
	alpha = new animate.alpha(app,items[i],0, 1);
	alpha.start();
    }
});

app.loop();
