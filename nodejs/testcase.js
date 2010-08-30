var mbfly = require("mbfly");
var r = mbfly.Hello(" test");
var sys = require("sys");
sys.puts(r);

var display_name = ":0.0";
if(process.argv.length == 3)
   display_name = process.argv[2];

var mb_rt = new mbfly.mb_rt(display_name, 300, 200);
var root = mb_rt.root;
sys.puts("root matrix: " +
	 [root[0], root[1], root[2], root[3], root[4], root[5]]);
var coord = mb_rt.coord_new(root);
sys.puts("coord matrix: " + 
	 [coord[0], coord[1], coord[2], coord[3], coord[4], coord[5]]);

/* Testcase for image shapes */
var img = mb_rt.image_new(10, 10, 50, 50);
var ldr = mbfly.img_ldr_new(".");
var img_data = ldr.load("sample.png");
var paint = mb_rt.paint_image_new(img_data);
var img_coord = mb_rt.coord_new(root);
paint.fill(img);
img_coord.add_shape(img);

/* test linear paint and rectangle */
var rect = mb_rt.rect_new(100, 100, 50, 50, 10, 10);
sys.puts(mb_rt.paint_linear_new);
var paint = mb_rt.paint_linear_new(100, 100, 150, 150);
paint.set_stops([[0, 0, 1, 0, 1], [1, 0, 0, 1, 1]]);
paint.fill(rect);
root.add_shape(rect);

/* test radial paint and rectangle */
var rect = mb_rt.rect_new(150, 100, 50, 50, 10, 10);
sys.puts(mb_rt.paint_radial_new);
var paint = mb_rt.paint_radial_new(175, 125, 25);
paint.set_stops([[0, 0, 1, 0, 1], [1, 0, 0, 1, 1]]);
paint.fill(rect);
root.add_shape(rect);

/* test alpha blending and rectangle */
var rect = mb_rt.rect_new(40, 40, 100, 100, 10, 10);
sys.puts(mb_rt.paint_color_new);
var paint = mb_rt.paint_color_new(1, 0.5, 0.5, 0.5);
paint.fill(rect);
root.add_shape(rect);

/* test hide of shapes */
var sw = 1;
setInterval(function() {
	if(sw) {
	    rect.hide();
	    sw = 0;
	} else {
	    rect.show();
	    sw = 1;
	}
    }, 1000);

/* test hide of coord */
var cw = 1;
setInterval(function() {
	if(sw) {
	    coord.hide();
	    cw = 0;
	} else {
	    coord.show();
	    cw = 1;
	}
    }, 3000);

/* test removing a coord */
var rm_coord = mb_rt.coord_new(root);
var rm_rect1 = mb_rt.rect_new(150, 150, 50, 50, 10, 10);
paint.fill(rm_rect1);
rm_coord.add_shape(rm_rect1);
var rm_rect2 = mb_rt.rect_new(100, 150, 50, 50, 10, 10);
paint.fill(rm_rect2);
rm_coord.add_shape(rm_rect2);
setTimeout(function() {
	rm_coord.remove();
	mb_rt.redraw_changed();
	mb_rt.flush();
    }, 3000);

/* test removing a shape */
setTimeout(function() {
	rm_rect1.remove();
	mb_rt.redraw_changed();
	mb_rt.flush();
    }, 2000);

/* Moving a path */
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
mb_rt.flush();

var i = 0;
setInterval(function() {
	var deg = (i++) * 0.1;
	coord[2] = (i % 40) * 5;
	mb_rt.redraw_changed();
	mb_rt.flush();
    }, 20);
setTimeout(function() { sys.puts("timeout"); }, 1000);

sys.puts(root.mouse_event);
var observer;
/* Mouse button pressed */
observer = root.mouse_event.add_event_observer(4, function(evt) {
	var c = 1 - (i % 40) / 40;
	sys.puts(c);

	sys.puts("mouse " + evt.x + " " + evt.y);
	sys.puts(c);
	sys.puts(paint.set_color);
	paint.set_color(c, 1, 1, 1);
	mb_rt.redraw_changed();
	mb_rt.flush();
    });

var resize_sw = 0;
setInterval(function() {
	var sz;
	
	resize_sw++;
	
	sz = (resize_sw % 20) - 10;
	if(sz < 0)
	    sz = -sz;
	sz = 2 - sz / 10;
	
	img_coord[0] = sz;
	img_coord[4] = sz;
	mb_rt.redraw_changed();
	mb_rt.flush();
    }, 50);

var kbobserver;
/* Keyboard event */
kbobserver = mb_rt.kbevents.add_event_observer(6, function(evt) {
	sys.puts("keycode = " + evt.keycode);
	sys.puts("sym = " + evt.sym);
    });
