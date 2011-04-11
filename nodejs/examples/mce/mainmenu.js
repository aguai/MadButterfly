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
	this.items.push(app.getComponent("cat"+i));
    }
    this.app = app;

    this.lines = [];
    this.line=0;
    this.item = 0;
    this.itemToScene=[0,15,31,47,63,79,95];

    app.addKeyListener(mbapp.KEY_UP, function() { 
	var target = self.items[self.item].toCoord();
	sys.puts(target);
	target.center.move(target.center.x, target.center.y-50);
	sys.puts(target.center.y);
	self.app.refresh();
    });
    app.addKeyListener(mbapp.KEY_DOWN, function() { 
	var target = self.items[self.item].toCoord();
	target.center.move(target.center.x, target.center.y+50);
	sys.puts("move "+target.center.owner.id);
	sys.puts(target.center.y);
	self.app.refresh();
    });

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
        try {
            //self.app.runToScene(self.itemToScene[self.item]);
            self.key_enter();
	} catch(e) {
	    sys.puts("aaa"+e.stack);
	    for(k in e) {
	        sys.puts(k+"="+e[k]);
	    }
	}
    });
}


MainMenu.prototype.key_enter=function() 
{
    var self = this;
    var target = this.items[this.item].toCoord();
    //var an = new animate.scale(this.app, target, 1, 1/1.5);

    this.changePage(self.item);
    /*
    animate.run(this.app,[an], 0, 0.3,function() {
        var sx = 259 - target.center.x;
        var sy = 355 - target.center.y;
	sys.puts("target="+target.id);
	var an1 = new animate.shift(self.app,target,sx,sy);
	animate.run(self.app, [an1],0,1,function() {
	    self.changePage(self.item);
	});
    });
    */
    return;
    var list=[];
    for(i=0;i<this.items.length;i++) {
	if (i == this.item) continue;
	var obj = this.items[i].toCoord();
	sys.puts(obj);
	if (obj == undefined) continue;
	if (i > this.item) {
	    sx = 1920 - this.items[i].toCoord().center.x;
	    sy = 0;
	} else {
	    sx =  -this.items[i].toCoord().center.x;
	    sy = 0;
	}
	//alpha = new animate.alpha(this.app,this.items[i], 0);
	sys.puts("111");
	an = new animate.shift(this.app,this.items[i].toCoord(), sx,sy);
	list.push(an);
    }
    animate.run(this.app,list, 0, 1);
    sys.puts("enter");
}

MainMenu.prototype.onNextPage=function() {
    sys.puts("aaaaaaaaaaaaaaaaaaaaaa");
    this.app.changeScene(97);
    var l = this.app.getComponent('line2');
    var coord = l.toCoord();
    if (coord == undefined) return;
    sys.puts(coord.isdup);
    sys.puts(coord.id);
    sys.puts(coord.refid);
    try {
        l.set_text('aaaaaaqa');
    } catch(e) {
	sys.puts(e);
    }
    this.app.refresh();
}
MainMenu.prototype.changePage=function(item) {
    this.app.epg.getList(item,this.onNextPage);
}

exports.MainMenu=MainMenu;
