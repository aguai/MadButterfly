var mbfly = require("mbfly");
var r = mbfly.Hello(" test");
var sys = require("sys");
sys.puts(r);


setTimeout(function() { sys.puts("timeout"); }, 1000);
