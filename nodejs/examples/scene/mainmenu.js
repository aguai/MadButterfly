// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var svg = require("svg");
var mbapp = require("mbapp");
var sys=require("sys");
var animate=require("animate");
var fs = require("fs");
/**
 *   We will fetch the EPG file from the server and fetch all images required for the main category from it.
 *   If these files are cached, we will not fetch it again. Otherwise, we will fetch them. The EPG class is
 *   responsible for the cache management.
 */
function MainMenu(app) 
{
    var self = this;
    this.n = 1;
    this.app = app;
    self.init(app);
}
MainMenu.prototype.init=function(app)
{
    var self = this;
    app.loadSVG("mbtest.svg");

    app.addKeyListener(mbapp.KEY_LEFT, function() { self.key_left();});
    app.addKeyListener(mbapp.KEY_RIGHT, function() { self.key_right();});
    app.addKeyListener(mbapp.KEY_UP, function() {self.key_up();});
    app.addKeyListener(mbapp.KEY_DOWN, function() {self.key_down();});
    app.addKeyListener(mbapp.KEY_ENTER, function() {self.key_enter();});
    app.changeScene(this.n);
}

MainMenu.prototype.key_left=function () 
{
   this.n = this.n - 1;
   this.app.changeScene(this.n);
   sys.puts("scene "+this.n);
}

MainMenu.prototype.key_right=function() 
{
   this.n = this.n + 1;
   this.app.changeScene(this.n);
   sys.puts("scene "+this.n);
}

MainMenu.prototype.key_up=function() 
{
}


MainMenu.prototype.key_down=function () 
{
}

MainMenu.prototype.key_enter=function() 
{
}

exports.MainMenu=MainMenu;
