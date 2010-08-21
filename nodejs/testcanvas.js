var mbapp = require("./mbapp");
var sys=require("sys");
var canvas=require("./canvas");

app = new mbapp.app();

app.canvas = new canvas.canvas(app,app.mb_rt.root);

app.canvas.background(0,0,0,1);
app.canvas.rect(0,0,400,400);
app.canvas.strokeWeight(8);
width=200;
height=200;
for(i=0;i<width;i++) {
    x = Math.random()*255;
	y = Math.random()*200;
	cr = Math.random()
	cg = Math.random()
	cb = Math.random()
	app.canvas.stroke(cr,cg,cb,1);
	app.canvas.line(i,0,x,height);
}

app.loop();
