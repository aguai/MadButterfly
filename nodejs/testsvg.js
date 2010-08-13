var svg = require("./svg");
var mbapp = require("./mbapp");

app = new mbapp.app();
app.loadSVG("test.svg");
app.loop();
