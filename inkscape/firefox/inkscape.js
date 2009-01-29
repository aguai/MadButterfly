var isInProgress=0;

var MAX_DUMP_DEPTH = 10;
var mbsvg;

      

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

function inkscape_load(file) 
{
	var inkscape = document.getElementById('inkscape');
	inkscape.innerHTML = "<embed src="+file+" width=1000 height=800 />";
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

function renderUI(mbsvg)
{
	var layers = mbsvg.layers;
	var scenes = mbsvg.scenes;
	var max = mbsvg.maxframe;
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
	if (isInProgress != 0) return;
	var id = obj.getAttribute('id');
	if (id == 'Jump') {
		if (currentScene != 0)
			gotoScene(currentScene);
	} else if (id == 'InsertKey') {
		InsertKey(currentScene);
	} else if (id == 'ExtendScene') {
		ExtendScene(currentScene);
	} else if (id == 'DeleteScene') {
		DeleteScene(currentScene);
	}
}

function gotoScene_cb(resObj)
{

}
var nextScene;
var currentScene = 0;
var currentLayer = '';
function gotoScene(n)
{
	nextScene = n;
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, gotoScene1);
	isInProgress++;
}
function gotoScene1(resp,n)
{
	var soapBody = new SOAPObject("SCENE");
	var v1 = new SOAPObject("v1");
	v1.val(nextScene);
	soapBody.appendChild(v1);
	var sr = new SOAPRequest("SCENE", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, gotoScene2);
}
function gotoScene2(resp)
{
	var soapBody = new SOAPObject("PUBLISH");
	var sr = new SOAPRequest("PUBLISH", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, gotoScene3);
}

function gotoScene3(resp)
{
	isInProgress--;
}



function InsertKey(n)
{
	nextScene = n;
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, InsertKey1);
	isInProgress++;
}
function InsertKey1(resp)
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
	SOAPClient.SendRequest(sr, RefreshDocument);
}
function PublishDocument(resp)
{
	mbsvg = new MBSVGString(resp.Body[0].GETDOCResponse[0].Result[0].Text);
	renderUI(mbsvg);

	var soapBody = new SOAPObject("PUBLISH");
	var sr = new SOAPRequest("PUBLISH", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, OperationDone);
}

function RefreshDocument(resp)
{
	var soapBody = new SOAPObject("GETDOC");
	var sr = new SOAPRequest("GETDOC", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, PublishDocument);
}

function OperationDone(res)
{
	isInProgress--;
}


function ExtendScene()
{
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr,ExtendScene1);
	isInProgress++;
}


function ExtendScene1(resp)
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
	SOAPClient.SendRequest(sr, RefreshDocument);
}


function DeleteScene()
{
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr,DeleteScene1);
	isInProgress++;
}

function DeleteScene1(resp)
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
	SOAPClient.SendRequest(sr, RefreshDocument);

}

function FetchDocument()
{
	var soapBody = new SOAPObject("START");
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr,RefreshDocument);
	isInProgress++;
}

var last_select = null;
inkscape_load("scene.mbsvg");
setTimeout("FetchDocument()",4000);

$('a.button').mouseover(function () {
			if (isInProgress==0)
				this.style.MozOpacity = 0.1;
		}).mouseout(function () {
			this.style.MozOpacity= 1;
		});	

