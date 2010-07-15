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
var path = mb_rt.path_new("m 100,50 L 120,50 L 200,150 L 180,150 z");
sys.puts(path);
sys.puts(coord.add_shape);
coord.add_shape(path);

sys.puts(mb_rt.paint_color_new);
var paint = mb_rt.paint_color_new(1, 1, 1, 1);
sys.puts(paint);
paint.stroke(path);

sys.puts(path.stroke_width);
path.stroke_width = 2;
sys.puts(path.stroke_width);

var face = mb_rt.font_face_query("courier", 2, 100);
var blks = [[5, face, 20]];
var stext = mb_rt.stext_new("Hello", 100, 50);
stext.set_style(blks);
paint.fill(stext);
coord.add_shape(stext);

mb_rt.redraw_all();

var i = 0;
setInterval(function() {
	var deg = (i++) * 0.1;
	coord[2] = (i % 20) * 10;
	mb_rt.redraw_changed();
    }, 20);
setTimeout(function() { sys.puts("timeout"); }, 1000);
