// -*- indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4:ai
var mbapp = require("./mbapp");
var sys=require("sys");
var animate=require("./animate");

app = new mbapp.app(":0.0", 320, 480);
app.loadSVG("phone_ui.svg");

var icons = [];
var r, c;
var mbname;

for(r = 0; r < 4; r++) {
    for(c = 0; c < 5; c++) {
	mbname = "icon" + r + "" + c;
	icons.push(app.get(mbname));
    }
}

var overhint, presshint;
var overholder, pressholder;
overhint = app.get("overhint");
presshint = app.get("presshint");
overholder = new animate.holder(app, overhint);
pressholder = new animate.holder(app, presshint);

var dock;
var dockholder;
dock = app.get("dock");
dockholder = new animate.holder(app, dock);

var i;
var icon;
for(i = 0; i < icons.length; i++) {
    icon = icons[i];
    icon.mouse_event.add_event_observer(1, function(evt) {
	    var tgt = evt.cur_tgt;

	    overholder.go_center(tgt);
	});
    icon.mouse_event.add_event_observer(2, function(evt) {
	    overholder.home();
	});
    icon.mouse_event.add_event_observer(4, function(evt) {
	    var tgt = evt.cur_tgt;

	    pressholder.go_center(tgt);
	});
}

var sw = 0;
dock.mouse_event.add_event_observer(4, function(evt) {
	if(sw == 0) {
	    var an = new animate.linear(app, dock, 0, -300, 0.5);
	    an.start();
	} else {
	    var an = new animate.linear(app, dock, 0, 0, 0.5);
	    an.start();
	}
	sw = (sw + 1) % 2;
    });

app.loop();
