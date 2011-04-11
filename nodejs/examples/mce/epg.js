// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
var http = require('http');
var URL = require('url');
var sys = require("sys");
var fs = require("fs");
var os = require("child_process");
function EPG() 
{
    var epgsrv = http.createClient(8080, '211.23.50.144');
    var cmd = '{"Protocol":"EPG-CSP","Command":"SearchRequest","ProgramCat":"MainCat"}';
    var headers={
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
        'Host':'211.23.50.144:8080',
        'User-Agent':'MadButterfly',
        'Content-Type':'application/x-www-form-urlencoded'
    };
    headers['Content-Length'] = cmd.length;
    var request = epgsrv.request('POST', '/IPTV_EPG/EPGService.do?timestamp='+new Date().getTime(),headers);
    var self = this;
    sys.puts("aaaa");
    var js = '';
    request.write(cmd);
    request.end();
    request.on('response', function(res) {
        sys.puts("connected");
 	res.on('data',function (data) {
		js = js + data;
	});
	res.on('end', function () {
		try {
		    res = JSON.parse(js);
		} catch(e) {
		    sys.puts(e);
		    sys.puts(js);
		}
		sys.puts("parsed");
		self.onLoad(res);

	});
    });
}


/**
 *   Check if the file has been cached. Create a symbolic to link if it is cached already.
 *
 */
function isCached(cachepath,file,obj) {
    var fields = cachepath.split('.');
    try {
        var ext = fields.pop();
	var pngfile = cachepath;
	sys.puts("ext="+ext);
	//if (ext != 'png') {
	//    fields.push('png');
	//    pngfile = fields.join('.');
	//}
        var st = fs.statSync(pngfile);
	try {
	    fs.unlinkSync(file);
	} catch(e) {
	}
	fs.linkSync(pngfile, file);
        obj.pend = obj.pend - 1;
	try {
            if (obj.pend == 0) {
                obj.onInitDone();
            } 
	} catch(e) {
	    sys.puts(e);
	}
	return 1;
    } catch(e) {
	sys.puts(e);
    }
    return 0;
}

/**
 *  Implement the mkdir -p to create the directory 
 */
function CreateDirectory(cachepath)
{
    var fields = cachepath.split('/');
    var p='';
    for(i=0;i<fields.length-1;i++) {
        p = p + fields[i]+'/';
	try {
            fs.mkdirSync(p,0777);
	} catch(e) {
	}
    }
}
/*
 *  We will check the cache directory. If the file is available, we will create a symbolic link only. Otherwise, 
 *  we will fetch it before create the symbolic link.
 */
function httpGetFile(url,file,obj)
{
    sys.puts("fetch "+ file);
    var u = URL.parse(url);
    var cachepath =  'cache/'+u.pathname;
    if (isCached(cachepath,file,obj)) {
        return;
    }
    CreateDirectory(cachepath);

    // Fetch file from the server and convert it tyo PNG if it is not PNG format.
    var f = fs.openSync(cachepath,'w');
    sys.puts("f="+f);
    var headers={
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
        'Host':'211.23.50.144:8080',
        'User-Agent':'MadButterfly',
        'Content-Type':'application/x-www-form-urlencoded'
    };
    sys.puts("host="+u.host+' '+u.port+' '+u.pathname);
    for(k in u) {
        sys.puts(k+"--->"+u[k]);
    }
    var c = http.createClient(8080,'211.23.50.144');
    var req = c.request('GET',u.pathname,headers);
    req.end();
    req.on('response', function(res) {
        res.on('data',function(data) {
            fs.writeSync(f,data,0,data.length);
	});
	res.on('end',function() {
	    fs.close(f);
	    var fields = cachepath.split('.');
	    var ext = fields.pop();
	    if (ext != "png" && ext != 'jpg') {
	        fields.push("png");
	        newf = fields.join(".");
		sys.puts("cachepath="+cachepath+" newf="+newf);
	        os.spawn("convert",[cachepath,newf]);
	    } else {
	        newf = cachepath;
	    }
	    try {
	        fs.unlinkSync(file);
	    } catch(e) {
	    }
	    sys.puts("end of "+cachepath+" to "+file);
	    fs.symlinkSync(newf, file);
	    obj.pend = obj.pend - 1;
	    if (obj.pend == 0) {
	        obj.onInitDone();
		sys.puts("done");
	    }

	});

    });
}

EPG.prototype.onLoad = function(res) {
    cats = res['ProgramCat'];
    this.pend = cats.length;
    this.maincat = cats;
    for (i in cats) {
	c = cats[i];
	httpGetFile(c['ProgramPIC'],'cat'+i+'.jpg',this);
        sys.puts("this.pend="+this.pend);
    }
    if (this.pend == 0)
        this.onInitDone();
}

EPG.prototype.getList=function(item,func) {
    var epgsrv = http.createClient(8080, '211.23.50.144');
    for (k in this.maincat[item]) {
	sys.puts(k+"--->"+this.maincat[item][k]);
    }
    var catID = this.maincat[item]['Category'];
    sys.puts(catID);
    var cmd = '{"Protocol":"EPG-CSP","Command":"SearchRequest","ProgramSub":"'+catID+'"}';
    var headers={
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
        'Host':'211.23.50.144:8080',
        'User-Agent':'MadButterfly',
        'Content-Type':'application/x-www-form-urlencoded;charset=utf-8'
    };
    //headers['Content-Length'] = cmd.length;
    var request = epgsrv.request('POST', '/IPTV_EPG/EPGService.do?timestamp='+new Date().getTime(),headers);
    var self = this;
    var js = '';
    request.write(cmd,encoding='utf-8');
    request.end();
    request.on('response', function(res) {
 	res.on('data',function (data) {
		js = js + data;
	});
	res.on('end', function () {
		res = JSON.parse(unescape(js));
		sys.puts("parsed");
		sys.puts(js);
		func();

	});
    });
}
EPG.prototype.onInitDone=function() {
    if (this.loadCallback)
        this.loadCallback();
}

EPG.prototype.registerInitDone=function(cb) {
    this.loadCallback = cb;
}

exports.EPG = EPG;
