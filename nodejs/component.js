var mbfly = require("mbfly");
var svg = require("./svg");
var sys=require("sys");
/*
The Component and ComponentManager is used to keep track of the symbol 
table in different frame. 


*/
function mul(a,b)
{
    var m = [a[0]*b[0]+a[1]*b[3], a[0]*b[1]+a[1]*b[4], a[0]*b[2]+a[1]*b[5]+a[2], 
            a[3]*b[0]+a[4]*b[3], a[3]*b[1]+a[4]*b[4], a[3]*b[2]+a[4]*b[5]+a[5]];
	a[0] = m[0];
	a[1] = m[1];
	a[2] = m[2];
	a[3] = m[3];
	a[4] = m[4];
	a[5] = m[5];
}

function Component(app, name) {
    this.app = app;
	this.name = name;
	this.coord = null;
}

Component.prototype.translate=function(tx,ty) {
    if (this.coord) {
	    mul(this.coord, [1,0,tx,0,1,ty]);
	}
}

Component.prototype.resize=function(sx,sy) {

    if (this.coord) {
	    mul(this.coord, [sx,0,0,0,sy,0]);
	}
}

Component.prototype.set=function(m) {

    if (this.coord) {
	    this.coord[0] = m[0];
	    this.coord[1] = m[1];
	    this.coord[2] = m[2];
	    this.coord[3] = m[3];
	    this.coord[4] = m[4];
	    this.coord[5] = m[5];
	}
}
Component.prototype.hide=function(m) {
    if (this.coord) {
	    this.coord.hide();
	}
}

Component.prototype.show=function(m) {
    if (this.coord) {
	    this.coord.show();
	}
}

Component.prototype.search=function() {
    this.coord = this.app._componentmanager.search(this.name);
}

Component.prototype.realize=function() {
    if (this.coord == null) {
	    this.search();
	}
	return this.coord;
}

Component.prototype.toCoord=function() {
    return this.realize();
}

Component.prototype.set_text=function(text) {
	this.realize();
	this.coord.set_text("asdsad");
}

function ComponentManager(app)
{
    this.app = app;
	this.object_table = {};
}

/*  \brief add an object into the current current component table. 
 *  This first argument is the source node of the screen object. 
 *  The second argument is the coord object which is displayed at
 *  the screen now. 
 *     
 *  We need to use the soucer node to get the name of the object.
 */
ComponentManager.prototype.add=function(source,obj) {
    if (source.refid==undefined) {
	    sys.puts("Internal Error: no refid is defined\n");
		return;
	}
    this.object_table[source.refid] = obj;
}


ComponentManager.prototype.del=function(name) {
    delete this.object_table[name];
}

ComponentManager.prototype.dump=function(name) {
    for(i in this.object_table) {
	    sys.puts(i);
	}
}

ComponentManager.prototype.search=function(name) {
    return this.object_table[name];
}


exports.Component = Component;
exports.ComponentManager = ComponentManager;
