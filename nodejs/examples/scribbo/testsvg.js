// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("svg");
var mbapp = require("mbapp");
var sys=require("sys");
var animate=require("animate");
var fs = require("fs");
var s=0;
app = new mbapp.app(":0.0",1920,1080);
app.loadSVG("test1.svg");
app.addKeyListener(mbapp.KEY_LEFT, function() { 
    s--;
    app.runToScene(s);
});
app.addKeyListener(mbapp.KEY_RIGHT, function() { 
    s++;
    app.runToScene(s);
});
app.loop();


