var isInProgress=0;

var MAX_DUMP_DEPTH = 10;
var inkscape;

      

function dumpObj(obj, name, indent, depth) {
      if (depth > MAX_DUMP_DEPTH) {
             return indent + name + ": <Maximum Depth Reached>\n";
      }
      if (typeof obj == "object") {
             var child = null;
             var output = indent + name + "\n";
             indent += "\t";
             for (var item in obj)
             {
                   try {
                          child = obj[item];
                   } catch (e) {
                          child = "<Unable to Evaluate>";
                   }
                   if (typeof child == "object") {
                          output += dumpObj(child, item, indent, depth + 1);
                   } else {
                          output += indent + item + ": " + child + "\n";
                   }
             }
             return output;
      } else {
             return obj;
      }
}
function dumpObjItem(obj, name, indent, depth) {
      if (depth > MAX_DUMP_DEPTH) {
             return indent + name + ": <Maximum Depth Reached>\n";
      }
      if (typeof obj == "object") {
             var child = null;
             var output = indent + name + "\n";
             indent += "\t";
             for (var item in obj)
             {
                   try {
                          child = obj[item];
                   } catch (e) {
                          child = "<Unable to Evaluate>";
                   }
                   if (typeof child == "object") {
                          output += dumpObjItem(child, item, indent, depth + 1);
                   } else {
                          output += indent + item + ":\n";
                   }
             }
             return output;
      } else {
             return obj;
      }
}
/**
 *   Inkscape class
 *
 */
function Inkscape(file) 
{
	var ink = document.getElementById('inkscape');
	ink.innerHTML = "<embed src="+file+" width=700 height=700 />";
	this.isInProgress = 0;

	setTimeout("inkscape.fetchDocument()",4000);
}

Inkscape.prototype.gotoScene = function (n)
{
	nextScene = n;
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.gotoScene1(resp);},this);
	this.isInProgress++;
}
Inkscape.prototype.gotoScene1 = function (resp,n)
{
	var soapBody = new SOAPObject("SCENE");
	var v1 = new SOAPObject("v1");
	v1.val(nextScene);
	soapBody.appendChild(v1);
	var sr = new SOAPRequest("SCENE", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.gotoScene2(resp);},this);
}	
Inkscape.prototype.gotoScene2 = function (resp)
{
	var soapBody = new SOAPObject("PUBLISH");
	var sr = new SOAPRequest("PUBLISH", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.gotoScene3(resp);},this);
}

Inkscape.prototype.gotoScene3 = function (resp)
{
	this.isInProgress--;
}
Inkscape.prototype.publishDocument= function(resp)
{
	mbsvg = new MBSVGString(resp.Body[0].GETDOCResponse[0].Result[0].Text);
	mbsvg.renderUI();

	var soapBody = new SOAPObject("PUBLISH");
	var sr = new SOAPRequest("PUBLISH", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function(resp,arg) {arg.operationDone(resp);},this);
}

Inkscape.prototype.refreshDocument = function(resp)
{
	var soapBody = new SOAPObject("GETDOC");
	var sr = new SOAPRequest("GETDOC", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function(resp,arg) { arg.publishDocument(resp);},this);
}	

Inkscape.prototype.operationDone = function (res)
{
	this.isInProgress--;
}
Inkscape.prototype.insertKey= function(n)
{
	nextScene = n;
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.insertKey1(resp);},this);
	this.isInProgress++;
}
Inkscape.prototype.insertKey1 = function(resp)
{
	var soapBody = new SOAPObject("INSERTKEY");
	var v1 = new SOAPObject("v1");
	v1.attr('type','string');
	v1.val(currentLayer);
	soapBody.appendChild(v1);
	var v2 = new SOAPObject("v2");
	v2.val(nextScene);
	soapBody.appendChild(v2);
	var sr = new SOAPRequest("INSERTKEY", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.refreshDocument(resp);},this);
}

Inkscape.prototype.extendScene=function()
{
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.extendScene1(resp);},this);
	this.isInProgress++;
}


Inkscape.prototype.extendScene1 = function(resp)
{
	var soapBody = new SOAPObject("EXTENDSCENE");
	var v1 = new SOAPObject("v1");
	v1.attr('type','string');
	v1.val(currentLayer);
	soapBody.appendChild(v1);
	var v2 = new SOAPObject("v2");
	v2.val(currentScene);
	soapBody.appendChild(v2);
	var sr = new SOAPRequest("EXTENDSCENE", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.refreshDocument(resp);},this);
}	


Inkscape.prototype.deleteScene=function()
{
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.deleteScene1(resp);},this);
	this.isInProgress++;
}

Inkscape.prototype.deleteScene1=function(resp)
{
	var soapBody = new SOAPObject("DELETESCENE");
	var v1 = new SOAPObject("v1");
	v1.attr('type','string');
	v1.val(currentLayer);
	soapBody.appendChild(v1);
	var v2 = new SOAPObject("v2");
	v2.val(currentScene);
	soapBody.appendChild(v2);
	var sr = new SOAPRequest("EXTENDSCENE", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.refreshDocument(resp);},this);
}	

Inkscape.prototype.fetchDocument = function()
{	
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr,function(resp,arg) {arg.refreshDocument(resp);},this);
	this.isInProgress++;
}



function MBSVG(file)
{
	var xmlDoc=document.implementation.createDocument("http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd","",null);
	xmlDoc.async=false;
	xmlDoc.load(file);
	MBSVG_loadFromDoc(this,xmlDoc);

}
function MBSVGString(xml)
{
	var xmlParser = new DOMParser();
	var xmlDoc = xmlParser.parseFromString( xml, 'text/xml');
	MBSVG_loadFromDoc(this,xmlDoc);
}



function MBSVG_loadFromDoc(self,xmlDoc)
{
	var scenesNode = xmlDoc.getElementsByTagNameNS("http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd","scene");
	if (scenesNode == null) {
		alert('This is not a valid scene file');
	}
	var len = scenesNode.length;
	var i,j;
	var max = 0;
	var scenes = new Array();

	// Get the length of scenes
	for(i=0;i<len;i++) {
		var s = scenesNode[i];
		var start = s.getAttribute("start");
		var end = s.getAttribute("end");
		var ref = s.getAttribute("ref");
		var ss = new Object();

		if (end == null) end = start
		if (max <end) max = end;
		ss.node = s;
		ss.start = start;
		ss.end = end;
		ss.ref = ref;
		ss.layer = null;
		scenes.push(ss);
	}
	if (max < 20) max = 20;
	// Collect all layers
	var nodes = xmlDoc.getElementsByTagNameNS("http://www.w3.org/2000/svg","svg")[0].childNodes;
	var layers = new Array();
	len = nodes.length;
	for(i=0;i<len;i++) {
		if (nodes[i].localName == 'g') {
			var subnodes = nodes[i].childNodes;
			for(j=0;j<subnodes.length;j++) {
				if (subnodes[j].localName == 'g') {
					for(var k=0;k<scenes.length;k++) {
						if (scenes[k].ref == subnodes[j].getAttribute('id')) {
							scenes[k].layer = nodes[i].getAttribute('id');
							break;
						}
					}
				}
			}
			layers.push(nodes[i]);
		}
	}
	self.layers = layers;
	self.scenes = scenes;
	self.maxframe = max;
}

MBSVGString.prototype=MBSVG.prototype;
MBSVG.prototype.renderUI=function()
{
	var layers = this.layers;
	var scenes = this.scenes;
	var max = this.maxframe;
	var cmd = "<table border=1>\n";
	cmd = cmd + "<tr><td></td>";
	for(var j=1;j<=max;j++) 
		cmd = cmd + "<td>"+j+"</td>";
	
	for(var i=layers.length-1;i>=0;i--) {
		var l = layers[i];
		var id = l.getAttribute('id');
		var label = l.getAttribute('inkscape:label');
		cmd = cmd + "<tr><td>"+label+"</td>";
		for(j=0;j<max;j++) {
			var empty = 1;
			var n = j +1;
			var id_str = id+"#"+n
			for(var k=0;k<scenes.length;k++) {
				if (id != scenes[k].layer) continue;
				if (n == scenes[k].start) {
					cmd = cmd + "<td><img class='normal' src=start.png id='"+id_str+"' onClick='selectCell(this)' /></td>";
					empty = 0;
					break;
				} else if ((n>scenes[k].start)&&(n <= scenes[k].end)) {
					cmd = cmd + "<td><img class='normal' src=fill.png id='"+id_str+"' onClick='selectCell(this)' /></td>";
					empty = 0;
					break;
				}
			}
			if (empty) {
				cmd = cmd + "<td><img class='normal' src=empty.png id='"+id_str+"'onClick='selectCell(this)' /></td>";
			}

		}
		cmd = cmd + "</tr>\n";
	}
	cmd = cmd + "</table>\n";
	var frame = document.getElementById('frame');
	frame.innerHTML=cmd;
}



/**
 *    UI for madbuilder.html to build the scene editor
 */

function selectCell(obj)
{
	var id = obj.getAttribute('id');
	var layer,n;
	var f = id.split('#');
	layer=f[0];
	n = f[1];
	var img = obj.getAttribute('src');
	var f = img.split('-');

	if (f[0] == 'active')
		return;
	else {
		obj.setAttribute('src', 'active-'+img);
	}

	if (last_select != null) {
		f = last_select.getAttribute('src').split('-');
		last_select.setAttribute('src', f[1]);
	}
	last_select = obj;
	currentScene = n;
	currentLayer = layer;
}


function onButtonClick(obj)
{
	if (inkscape.isInProgress != 0) return;
	var id = obj.getAttribute('id');
	if (id == 'Jump') {
		if (currentScene != 0)
			inkscape.gotoScene(currentScene);
	} else if (id == 'InsertKey') {
		inkscape.insertKey(currentScene);
	} else if (id == 'ExtendScene') {
		inkscape.extendScene(currentScene);
	} else if (id == 'DeleteScene') {
		inkscape.deleteScene(currentScene);
	} else {
		alert(id+' has not been implemented yet');
	}
}

function gotoScene_cb(resObj)
{

}
var nextScene;
var currentScene = 0;
var currentLayer = '';


function dump(n)
{
	cmd = "";
	for(k in n) {
		cmd = cmd + k+"="+n[k]+"\n";
	}
	alert(cmd);
}






function loadInkscapeFile()
{
  ele = $('#mbsvg');
  file = ele.attr('value');
  inkscape = new Inkscape("file://"+file);

  file1_animation = [
	  {
		attributes: {id:"an1-1"},
		data: "animation1"
	  },
	  {
		attributes: {id:"an1-2"},
		data: "animation2"
	  }
  ];
  file1 = {
	attributes:{id:"file1"},
	data: "scene1.mbsvg",
	children: file1_animation
  };
  file2 = {
	attributes:{id:"file2"},
	data: "scene2.mbsvg",
  };
  file3 = {
	attributes:{id:"file3"},
	data: "scene3.mbsvg",
  };

  scenes = [ file1,file2,file3];
  src1 = {attributes:{id:"src1"},data:"src1.c"};
  src2 = {attributes:{id:"src1"},data:"src2.c"};
  src3 = {attributes:{id:"src1"},data:"src3.c"};
	  
  sources = [src1,src2,src3];
	   
}


function project_parse(xml)
{

	var xmlParser = new DOMParser();
	var xmlDoc = xmlParser.parseFromString( xml, 'text/xml');
	var scenesNode = xmlDoc.getElementsByTagName("scene");
	if (scenesNode == null) {
		alert('This is not a valid scene file');
	}
	var len = scenesNode.length;
	var i,j;
	var max = 0;
	var scenes = new Array();

	// Get the length of scenes
	for(i=0;i<len;i++) {
		var n = scenesNode[i];
		var s = new Object();
		s.attributes = new Object();
		s.attributes.id = "scene"+i;
		s.state = "open";
		s.data = n.getAttribute("src");
		scenes.push(s);
	}

	var nodes = xmlDoc.getElementsByTagName("source");
	var len = nodes.length;
	var i,j;
	var max = 0;
	var sources = [];

	// Get the length of scenes
	for(i=0;i<len;i++) {
		var n = nodes[i];
		var s = new Object();
		s.attributes = new Object();
		s.attributes.id = "sources"+i;
		s.state = "open";
		s.data = n.getAttribute("src");
		sources.push(s);
	}

  	var tree = $.tree_create();
  	tree.init($("#filelist"), {
	    data: {
  		type: "json",
		json : [
			{
				attributes: {id: "prj"}, 
				state: "open", 
				data: "Project", 
				children: [
					{ attributes:{id:"scenes"}, data:"scene", children: scenes},
					{ attributes:{id:"sources"},data:"sources",children: sources}
				]
			}
		],
	    },
	    ui : {
		context :  [ 
			{
				id: "Open",
				label: "Open",
				icon: "open.png",
				visible: function(NODE,TREE_OBJ) {  if(NODE.length != 1) return false; return NODE[0].id == "prj";},
				action: function(NODE,TREE_OBJ) { openFile(TREE_OBJ);}
			},
			{
				id: "New",
				label: "New",
				icon: "create.png",
				visible: function(NODE,TREE_OBJ) {  if(NODE.length != 1) return false; return NODE[0].id == "prj";},
				action: function(NODE,TREE_OBJ) { alert("open is not support yet");}
			},
			{
				id: "Rename",
				label: "Rename",
				icon: "rename.png",
				visible: function(NODE,TREE_OBJ) {  if(NODE.length != 1) return false; return NODE[0].id == "prj";},
				action: function(NODE,TREE_OBJ) { alert("open is not support yet");}
			}
		]
    		}

  	});
}

function system_read(fname) {
	try {
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
	} catch (e) {
		alert("Permission to read file was denied.");
	}
	var file = Components.classes["@mozilla.org/file/local;1"]
					.createInstance(Components.interfaces.nsILocalFile);
	try {
		file.initWithPath( fname );
		if ( file.exists() == false ) {
			alert("File does not exist");
		}
		var is = Components.classes["@mozilla.org/network/file-input-stream;1"]
						.createInstance( Components.interfaces.nsIFileInputStream );
		is.init( file,0x01, 00004, null);
		var sis = Components.classes["@mozilla.org/scriptableinputstream;1"]
						.createInstance( Components.interfaces.nsIScriptableInputStream );
		sis.init( is );
		var output = sis.read( sis.available() );
		sis.close();
	} catch(e) {
		alert(fname+" does not exist");
	}
	return output;
}

function project_loadFile()
{
	prjname = $('#mbsvg').attr('value');
	var prj = system_read(prjname);
	project_parse(prj);
}

var last_select = null;

$('#inkscape').html('Please select the project file<br>');
$('#inkscape').append('<input type=file value="Select the project file" id="mbsvg" accept="image/png">');
$('#inkscape').append('<input type=button value="Load" onclick="project_loadFile()">');
