var mbfly = require("mbfly");
var r = mbfly.Hello(" test");
var sys = require("sys");
sys.puts(r);

var mb_rt = new mbfly.mb_rt(":0.0", 300, 200);
var root = mb_rt.root;
sys.puts("root matrix: " +
	 [root[0], root[1], root[2], root[3], root[4], root[5]]);
var coord = mb_rt.coord_new(root);
sys.puts("coord matrix: " + 
	 [coord[0], coord[1], coord[2], coord[3], coord[4], coord[5]]);

sys.puts(mb_rt.path_new);
var path = mb_rt.path_new("m 100,50 L 120,50 L 200,150 L 150,150 z");
sys.puts(path);
sys.puts(coord.add_shape);
coord.add_shape(path);

sys.puts(mb_rt.paint_color_new);
var paint = mb_rt.paint_color_new(1.0, 1.0, 1.0, 1.0);
sys.puts(paint);
paint.stroke(path);

sys.puts(path.stroke_width);
path.stroke_width = 2;
sys.puts(path.stroke_width);

setTimeout(function() { sys.puts("timeout"); }, 1000);
