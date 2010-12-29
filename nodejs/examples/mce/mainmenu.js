// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("svg");
var mbapp = require("mbapp");
var sys=require("sys");
var animate=require("animate");
var fs = require("fs");
var EPG = require('./epg');
/**
 *   We will fetch the EPG file from the server and fetch all images required for the main category from it.
 *   If these files are cached, we will not fetch it again. Otherwise, we will fetch them. The EPG class is
 *   responsible for the cache management.
 */
function MainMenu(app) 
{
    var self = this;
    var epg = new EPG.EPG();
    epg.registerInitDone(function() { self.init();});
    app.epg = epg;
    //self.init();
}
MainMenu.prototype.init=function()
{
    app.loadSVG("main.svg");
    app.changeScene(1);

    var i;
    var self = this;
    this.items=[];
    for(i=0;i<8;i++) {
	this.items.push(app.get("cat"+i));
    }
    this.app = app;

    this.lightbar = app.get("lightbar");
    this.lines = [];
    for(i = 0; i < 5; i++) {
        var line = app.get("line" + (i + 1));
        this.lines.push(line);
    }
    this.line=0;
    this.item = 0;

    animate.run([new animate.scale(app,this.items[this.item], 1, 1.5)], 0, 0.1);
    app.refresh();

    app.addKeyListener(mbapp.KEY_LEFT, function() { self.key_left();});
    app.addKeyListener(mbapp.KEY_RIGHT, function() { self.key_right();});
    app.addKeyListener(mbapp.KEY_UP, function() {self.key_up();});
    app.addKeyListener(mbapp.KEY_DOWN, function() {self.key_down();});
    app.addKeyListener(mbapp.KEY_ENTER, function() {self.key_enter();});
}

MainMenu.prototype.key_left=function () 
{
    var old = this.items[this.item];
    this.item = this.item - 1;
    if (this.item == -1) {
	this.item = 0;
	return;
    }
    
    var target = this.items[this.item];

    old.bbox.update();
    target.bbox.update();
    
    var an = new animate.scale(this.app, old, 1/1.1, 1/1.5);
    animate.run([an], 0, 0.1);
    an = new animate.scale(this.app, target, 1.1, 1.5);
    animate.run([an], 0, 0.3);
    var sx = target.center.x - this.lightbar.center.x;
    var an = new animate.shift(this.app, this.lightbar, sx, 0);
    animate.run([an], 0, 0.3);
}

MainMenu.prototype.key_right=function() 
{
    var old = this.items[this.item];
    this.item = this.item + 1;
    if (this.item == this.items.length) {
	this.item = this.item - 1;
	return;
    }
    
    var target = this.items[this.item];

    old.bbox.update();
    target.bbox.update();
    
    var an = new animate.scale(this.app, old, 1/1.1, 1/1.5);
    animate.run([an], 0, 0.1);
    an = new animate.scale(this.app, target, 1.1, 1.5);
    animate.run([an], 0, 0.3);
    var sx = target.center.x - this.lightbar.center.x;
    var an = new animate.shift(this.app, this.lightbar, sx, 0);
    animate.run([an], 0, 0.3);
}

MainMenu.prototype.key_up=function() 
{
    var old = this.lines[this.line];
    this.line = this.line - 1;
    if (this.line == -1) {
	this.line = 0;
	return;
    }
    var target = this.lines[this.line];
    var sy = target.center.y - this.lightbar.center.y;
    var an = new animate.shift(this.app, this.lightbar, 0, sy);
    animate.run([an], 0, 0.3);
}


MainMenu.prototype.key_down=function () 
{
    var old = this.lines[this.line];
    this.line = this.line + 1;
    if (this.line == this.lines.length) {
	this.line = this.line - 1; 
	return;
    }
    var target = this.lines[this.line];
    var sy = target.center.y - this.lightbar.center.y;
    var an = new animate.shift(this.app, this.lightbar, 0, sy);
    animate.run([an], 0, 0.3);
}

MainMenu.prototype.key_enter=function() 
{
    var self = this;
    var target = this.items[this.item];
    var an = new animate.scale(this.app, target, 1/1.1, 1/1.5);
    animate.run([an], 0, 0.3,function() {
        var sx = 259 - target.center.x;
        var sy = 355 - target.center.y;
	var an1 = new animate.shift(self.app,target,sx,sy);
	animate.run([an1],0,1,function() {self.changePage(self.item);});
    });
    for(i=0;i<this.items.length;i++) {
	if (i == this.item) continue;
	if (i > this.item) {
	    sx = 1920*2 - this.items[i].center.x;
	    sy = 0;
	} else {
	    sx =  -this.items[i].center.x*2;
	    sy = 0;
	}
	an = new animate.shift(this.app,this.items[i], sx, sy);
	animate.run([an], 0, 2);
	alpha = new animate.alpha(this.app,this.items[i], 0);
	animate.run([an], 0, 2);
    }
}

MainMenu.prototype.onNextPage=function() {
    this.app.changeScene(2);
}
MainMenu.prototype.changePage=function(item) {
    this.epg.getList(item,self.onNextPage());
}

exports.MainMenu=MainMenu;
