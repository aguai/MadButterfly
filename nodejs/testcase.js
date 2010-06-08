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
var path = mb_rt.path_new("m 100,100 L 200,200");
sys.puts(path);
sys.puts(coord.add_shape);
coord.add_shape(path);

setTimeout(function() { sys.puts("timeout"); }, 1000);
