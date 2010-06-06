var mbfly = require("mbfly");
var r = mbfly.Hello(" test");
var sys = require("sys");
sys.puts(r);

var mb_rt = mbfly.mb_rt(":0.0", 300, 200);

setTimeout(function() { sys.puts("timeout"); }, 1000);
