// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("svg");
var mbapp = require("mbapp");
var sys=require("sys");
var animate=require("animate");
var fs = require("fs");
var main=require("./mainmenu");
app = new mbapp.app(":0.0",1920,1080);
app.setFrameRate(50);
scene=new main.MainMenu(app);
app.loop();
