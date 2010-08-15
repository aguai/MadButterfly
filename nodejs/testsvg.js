var svg = require("./svg");
var mbapp = require("./mbapp");
var sys=require("sys");

app = new mbapp.app();
app.loadSVG("test.svg");
lightbar = app.get("item_lightbar")
item=1
lightbar[5] = app.get("item"+item)[5]

function animated(app,obj) {
	var d=obj.animated_loc - obj[5];
	var dd=d;
	if (dd<0) dd = -dd;

	if (dd > 5) {
		if (d > 0) d = 5; else d = -5;
	}

    obj[5] += d;
	sys.puts(d);
	app.refresh()
	if (dd > 1) {
	    setTimeout(function() { animated(app,obj);}, 20);
		return;
	}
	app.animated_end = 1;
}
function animated_start(app,obj,target) {
    if (obj.animated_end==0)
	    obj[5] = obj.animated_loc;
    obj.animated_loc = target[5];
	obj.animated_end = 0;
	animated(app,obj);
}
lightbar.animated_loc = 1;
app.addKeyboardListener(mbapp.EVT_KB_PRESS, function(evt) {
    if (evt.keycode == mbapp.KEY_UP) {
		item = item - 1;
		if (item == 0) item = 1;
		else
            animated_start(app,lightbar,app.get("item"+item));
	} else if (evt.keycode == mbapp.KEY_DOWN) {
	    item = item + 1;
		if (item == 10) {
		    item = 9;
		} else
            animated_start(app,lightbar,app.get("item"+item));
	}
});
app.loop();
