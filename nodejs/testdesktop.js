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
an = new animate.scale(app,items[item],1,2,10);
an.start();

app.addKeyListener(mbapp.KEY_UP, function() {
	var old = items[item];
	item = item - 1;
	if (item == -1) {
		item = 0;
		return;
	}
    var target = items[item];
	var an = new animate.scale(app,old,1,1,0.1);
    an.start();
	an = new animate.scale(app,target,1,2,0.3);
    an.start();
});

app.addKeyListener(mbapp.KEY_DOWN, function() {
	var old = items[item];
	item = item + 1;
	if (item == items.length) {
		item = item - 1;
		return;
	}
    var target = items[item];
	var an = new animate.scale(app,old,1,1,0.1);
    an.start();
	an = new animate.scale(app,target,1,2,0.3);
    an.start();
});

app.loop();
