var svg = require("./svg");
var mbapp = require("./mbapp");
var sys=require("sys");

app = new mbapp.app();
app.loadSVG("test.svg");
lightbar = app.get("item_lightbar")
item=1
lightbar[5] = app.get("item"+item)[5]
app.addKeyboardListener(6, function(evt) {
    if (evt.keycode == mbapp.KEY_UP) {
		item = item - 1;
		if (item == 0) item = 9;
        lightbar[5] = app.get("item"+item)[5]
	} else if (evt.keycode == mbapp.KEY_DOWN) {
	    item = item + 1;
		if (item == 10) {
		    item = 1;
		}
        lightbar[5] = app.get("item"+item)[5]
	}
});
app.loop();
