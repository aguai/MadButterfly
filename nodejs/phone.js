// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var mbapp = require("./mbapp");
var sys=require("sys");
var animate=require("./animate");

app = new mbapp.app(":0.0", 320, 480);
app.loadSVG("phone_ui.svg");

var icons = [];
var r, c;
var mbname;
for(c = 0; c < 4; c++) {	// 4 columns
    for(r = 0; r < 5; r++) {	// 5 rows
	mbname = "icon" + c + "" + r;
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
	    overholder.go_center(evt.cur_tgt);
	});
    icon.mouse_event.add_event_observer(2, function(evt) {
	    overholder.home();
	});
    icon.mouse_event.add_event_observer(4, function(evt) {
	    pressholder.go_center(evt.cur_tgt);
	    var rotate = new animate.rotate(app, evt.cur_tgt, 2 * 3.1415);
	    animate.run([rotate], 0, 0.7);
	});
}

var sw = 0;
var dock_up = new animate.shift(app, dock, 0, -300);
var dock_down = new animate.shift(app, dock, 0, 0);
dock.mouse_event.add_event_observer(4, function(evt) {
	if(sw == 0) {
	    animate.run([dock_up], 0, 0.5);
	} else {
	    animate.run([dock_down], 0, 0.2);
	}
	sw = sw ^ 1;
    });

app.loop();
