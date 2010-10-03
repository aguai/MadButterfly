// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("./svg");
var mbapp = require("./mbapp");

app = new mbapp.app();
app.loadSVG("test.svg");
app.loop();
