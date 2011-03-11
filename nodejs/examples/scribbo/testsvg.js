// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("svg");
var mbapp = require("mbapp");
var sys=require("sys");
var animate=require("animate");
var fs = require("fs");

function renderFrame() {
    cur = cur + 1;
    if (cur == 10) cur=1;
    app.changeScene(cur);
    setTimeout(renderFrame,30);
}

app = new mbapp.app();
app.loadSVG("test.svg");
cur = 1;
setTimeout(renderFrame,30);
app.loop();


