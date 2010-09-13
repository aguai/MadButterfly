var mbfly = require("mbfly");
var sys = require("sys");
var cproc = require("child_process");

function testcase1() {
    var mbrt = new mbfly.mb_rt(":32.0", 300, 200);
    var root = mbrt.root;
    var coords = [];
    var coord;
    var i;

    while(true) {
	for(i = 0; i < 200; i++) {
	    coords.push(mbrt.coord_new(root));
	}
	mbrt.redraw_changed();
	mbrt.flush();
	
	while(coords.length > 0) {
	    coord = coords.pop();
	    coord.remove();
	}
    }
}

function testcase2() {
    var mbrt = new mbfly.mb_rt(":32.0", 300, 200);
    var root = mbrt.root;
    var coords = [];
    var coord;
    var i, j;

    while(true) {
	for(i = 0; i < 200; i++) {
	    coords.push(mbrt.coord_new(root));
	    for(j = 0; j < 10; j++)
		coord = mbrt.coord_new(coords[i]);
	}
	mbrt.redraw_changed();
	mbrt.flush();
	
	while(coords.length > 0) {
	    coord = coords.pop();
	    coord.remove();
	}
    }
}

function testcase3() {
    var mbrt = new mbfly.mb_rt(":32.0", 300, 200);
    var root = mbrt.root;
    var coords = [];
    var coord;
    var shape;
    var i, j;

    while(true) {
	for(i = 0; i < 200; i++) {
	    coords.push(mbrt.coord_new(root));
	    for(j = 0; j < 10; j++) {
		coord = mbrt.coord_new(coords[i]);
		shape = mbrt.rect_new(15, 15, 20, 20, 0, 0);
		coord.add_shape(shape);
	    }
	}
	mbrt.redraw_changed();
	mbrt.flush();
	
	while(coords.length > 0) {
	    coord = coords.pop();
	    coord.remove();
	}
    }
}

function testcase4() {
    var mbrt = new mbfly.mb_rt(":32.0", 300, 200);
    var root = mbrt.root;
    var coords = [];
    var coord;
    var shape;
    var i, j;

    while(true) {
	for(i = 0; i < 200; i++) {
	    coords.push(mbrt.coord_new(root));
	    for(j = 0; j < 10; j++) {
		coord = mbrt.coord_new(coords[i]);
		shape = mbrt.path_new("m 10,10 l 55,27 l -30,-3 z");
		coord.add_shape(shape);
	    }
	}
	mbrt.redraw_changed();
	mbrt.flush();
	
	while(coords.length > 0) {
	    coord = coords.pop();
	    coord.remove();
	}
    }
}

function testcase5() {
    var mbrt = new mbfly.mb_rt(":32.0", 300, 200);
    var root = mbrt.root;
    var paint;

    while(true) {
	paint = mbrt.paint_color_new(0.5, 0.5, 0.5, 1);
    }
}

function testcase6() {
    var mbrt = new mbfly.mb_rt(":32.0", 300, 200);
    var root = mbrt.root;
    var coords = [];
    var coord;
    var shape;
    var paint;
    var i, j;

    while(true) {
	for(i = 0; i < 200; i++) {
	    coords.push(mbrt.coord_new(root));
	    /* Paint is free, but rdman is dirty */
	    paint = mbrt.paint_color_new(0.5, 0.5, 0.5, 1);
	    for(j = 0; j < 10; j++) {
		coord = mbrt.coord_new(coords[i]);
		shape = mbrt.rect_new(5, 5, 20, 20, 0, 0);
		coord.add_shape(shape);
	    }
	}
	mbrt.redraw_changed();
	mbrt.flush();
	
	while(coords.length > 0) {
	    coord = coords.pop();
	    coord.remove();
	}
    }
}

function testcase7() {
    var mbrt = new mbfly.mb_rt(":32.0", 300, 200);
    var root = mbrt.root;
    var coords = [];
    var coord;
    var shape;
    var paint;
    var i, j;

    while(true) {
	for(i = 0; i < 200; i++) {
	    coords.push(mbrt.coord_new(root));
	    for(j = 0; j < 10; j++) {
		/* paint is not free, and rdman is dirty */
		paint = mbrt.paint_color_new(0.5, 0.5, 0.5, 1);
		coord = mbrt.coord_new(coords[i]);
		shape = mbrt.path_new("M 100,100 L 30,30 L 0,30 z");
		coord.add_shape(shape);
		paint.stroke(shape);
	    }
	}
	mbrt.redraw_changed();
	mbrt.flush();
	
	while(coords.length > 0) {
	    coord = coords.pop();
	    coord.remove();
	}
    }
}

cproc.exec("killall -9 Xvfb; Xvfb :32 -screen 0 800x600x24");
setTimeout(function() {
	if(process.argv.length == 1)
	    testcase1();
	else if(process.argv.length == 3) {
	    sys.puts("testcase " + process.argv[2]);
	    switch(process.argv[2]) {
	    case "1":
		testcase1();
		break;
		
	    case "2":
		testcase2();
		break;
		
	    case "3":
		testcase3();
		break;
		
	    case "4":
		testcase4();
		break;
		
	    case "5":
		testcase5();
		break;
		
	    case "6":
		testcase6();
		break;
		
	    case "7":
		testcase7();
		break;
		
	    default:
		sys.puts("Usage: node testleak.js [1|2|3...7]");
	    }
	} else
	    sys.puts("Usage: node testleak.js [1|2|3...7]");
	cproc.exec("killall -9 Xvfb");
    }, 1500);
