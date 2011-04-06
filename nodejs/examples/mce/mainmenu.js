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
    app.loadSVG("main1.svg");
    app.changeScene(0);

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
    this.itemToScene=[0,15,31,47,63,79,95];

    app.addKeyListener(mbapp.KEY_LEFT, function() { 
        self.item = self.item - 1;
        if (self.item == -1) {
	    self.item = 0;
	    return;
        }
        self.app.runToScene(self.itemToScene[self.item]);
    });
    app.addKeyListener(mbapp.KEY_RIGHT, function() { 
        self.item = self.item + 1;
        if (self.item == self.items.length) {
	    self.item = self.item - 1;
	    return;
        }
        self.app.runToScene(self.itemToScene[self.item]);
    });
    app.addKeyListener(mbapp.KEY_ENTER, function() {
        self.key_enter();
    });
}


MainMenu.prototype.key_enter=function() 
{
    var self = this;
    var target = this.items[this.item];
    var an = new animate.scale(this.app, target, 1, 1/1.5);
    animate.run([an], 0, 0.3,function() {
        var sx = 259 - target.center.x;
        var sy = 355 - target.center.y;
	var an1 = new animate.shift(self.app,target,sx,sy);
	animate.run([an1],0,1,function() {
	    self.changePage(self.item);
	});
    });
    for(i=0;i<this.items.length;i++) {
	if (i == this.item) continue;
	if (i > this.item) {
	    sx = 1920 - this.items[i].center.x;
	    sy = 0;
	} else {
	    sx =  -this.items[i].center.x*2;
	    sy = 0;
	}
	//alpha = new animate.alpha(this.app,this.items[i], 0);
	an = new animate.shift(this.app,this.items[i], sx,sy);
	animate.run([an], 0, 1);
    }
}

MainMenu.prototype.onNextPage=function() {
    this.app.changeScene(97);
}
MainMenu.prototype.changePage=function(item) {
    this.app.epg.getList(item,this.onNextPage);
}

exports.MainMenu=MainMenu;
