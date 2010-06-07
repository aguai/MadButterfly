var mbfly = require("mbfly");
var r = mbfly.Hello(" test");
var sys = require("sys");
sys.puts(r);

var mb_rt = new mbfly.mb_rt(":0.0", 300, 200);
sys.puts(mb_rt.root);

setTimeout(function() { sys.puts("timeout"); }, 1000);
