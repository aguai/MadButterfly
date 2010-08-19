var svg = require("./svg");
var mbapp = require("./mbapp");
var sys=require("sys");
var animate=require("./animate");

app = new mbapp.app();
app.loadSVG("test.svg");
lightbar = app.get("item_lightbar");
item=1;
lightbar[5] = app.get("item"+item)[5];

app.addKeyListener(mbapp.KEY_UP, function() {
		item = item - 1;
		if (item == 0) item = 1;
		else {
		    var target = app.get("item"+item);
			var an = new animate.linear(app,lightbar,target[2],target[5],0.3);
		    an.start();
		}
});

app.addKeyListener(mbapp.KEY_DOWN, function() {
	    item = item + 1;
		if (item == 10) {
		    item = 9;
		} else {
		    var target = app.get("item"+item);
			var an = new animate.linear(app,lightbar,target[2],target[5],0.3);
		    an.start();
		}
});

app.loop();
