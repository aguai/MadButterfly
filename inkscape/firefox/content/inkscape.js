var isInProgress=0;

var MAX_DUMP_DEPTH = 10;
var inkscape;


function endsWith(str, s){
	var reg = new RegExp (s + "$");
	return reg.test(str);
}

function dumpXML(xml)
{
	return (new XMLSerializer()).serializeToString(xml);
}


function showInWindow(content)
{
	$('#debug').append("<pre>"+content+"</pre>");
}

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
 *   TextEditor class
 *
 */

function TextEditor(file) 
{
	var editor = document.getElementById('inkscape');
	editor.innerHTML = "<embed src="+file+" width=900 height=700 />";
	this.isInProgress = 0;
}

/**
 *   Inkscape class
 *
 */
function Inkscape(file) 
{
	var ink = document.getElementById('inkscape');
	ink.innerHTML = "<embed src="+file+" width=900 height=700 />";
	this.isInProgress = 0;
	this.callback = null;
	this.animation = new MadSwatter(this);

	setTimeout("inkscape.fetchDocument()",3000);
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
	this.mbsvg = mbsvg;
	if (this.callback)
		this.callback(mbsvg);

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

Inkscape.prototype.fetchDocument = function(callback)
{	
	var soapBody = new SOAPObject("START");
	this.callback = callback
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr,function(resp,arg) {arg.refreshDocument(resp);},this);
	this.isInProgress++;
}

Inkscape.prototype.changeSymbolName_cb = function(callback)
{
	var soapBody = new SOAPObject("CHANGESYMBOL");
	var v1 = new SOAPObject("v1");
	v1.attr('type','string');
	v1.val(this.v1);
	soapBody.appendChild(v1);
	var v2 = new SOAPObject("v2");
	v2.val(this.v2);
	soapBody.appendChild(v2);
	var sr = new SOAPRequest("CHANGESYMBOL", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	SOAPClient.SendRequest(sr, function (resp,arg) {arg.refreshDocument(resp);},this);
	this.inProgress--;
}

Inkscape.prototype.changeSymbolName = function(id,newname,callback)
{	
	var soapBody = new SOAPObject("START");
	this.callback = callback
	var sr = new SOAPRequest("START", soapBody);
	SOAPClient.Proxy = "http://localhost:19192/";
	this.v1 = id;
	this.v2 = newname;
	SOAPClient.SendRequest(sr,function(resp,arg) {arg.changeSymbolName_cb(resp);},this);
	this.isInProgress++;
}

/*
 *  This module is used to define a symbol for the MadButterfly. This function will search for symbol which is defined in the current select object. We will list all SVG elements 
 *  in the left side, multiple variables can be defined at one time. When any element is selected, the defined symbol will be listed in the right side. 
 *
 */

Inkscape.prototype.MakeSymbol=function()
{
	function callback(mbsvg) {
		inkscape.loadSymbolScreen(mbsvg);
	}
	inkscape.fetchDocument(callback);
}


Inkscape.prototype.onChangeSymbolName=function()
{
	inkscape.changeSymbolName(inkscape.current_symbol, $('#newsymbolname').val());
	symboldialog.dialog('close')
}

Inkscape.prototype.refreshSymbolPanel=function(node)
{
	var reg = new RegExp('(.*)\\((.*)\\)');
	var m = reg.exec(node.textContent);
	var val = m[2];
	inkscape.current_symbol = node.textContent;
	$('#newsymbolname').val(val);
}

Inkscape.prototype.editAnimation=function () {
	inkscape.fetchDocument(inkscape.editAnimationCallback);
}

Inkscape.prototype.editAnimationCallback=function(mbsvg) {

	inkscape.animation.edit(mbsvg);
	return;


	var sodi = mbsvg.getElementsByTag('sodipodi:namedview')[0];	

	var layer=sodi.getAttribute('inkscape:current-layer');
	var animation = mbsvg.getElementsByTag('animationlist')[0];
	var alist = animation.getElementsByTag('animation');
	var len = alist.length;
	var dialog = $('animation');
	dialog.dialog('open');
	var html = new Array();
	html.append('<ul>');
	for(i=0;i<alist.len;i++) {
		html.append('<li><a href="#" onClick="">'+alist[i].getAttribute('name')+"</a></li>");
	}
	html.append('</ul>');
	$('animation_list').html(html.join("\n"));
	dialog.show();
}


Inkscape.prototype.loadSymbolScreen=function (mbsvg) {
	// Swap the left side to be the SVG element tree.
	var i,l;

	symboldialog.dialog('open');
	this.mbsvg = mbsvg;
	l = mbsvg.selected_objects.length;
	var jsonobj = []
	for(i=0;i<l;i++) {
		// Add symbol into the tree
		var name = mbsvg.findSymbolName(mbsvg.selected_objects[i]);
		var title=mbsvg.selected_objects[i]+"("+name+")";
		var obj = { attributes: {id: 'sym'+i}, data : title};
		if (i == 0) {
			this.current_symbol = mbsvg.selected_objects[i];
			$('#newsymbolname').val(name);
		}
		jsonobj.push(obj);
	}
  	this.symboltree = $.tree_create();
  	this.symboltree.init($("#symbollist"), {
	    data: {
  		type: "json",
		json : jsonobj
	    },
	    callback : {
	        ondblclk : function(NODE,TREE_OBJ) { inkscape.refreshSymbolPanel(NODE);}
	    }

  	});
	var s = $('#changename');
	s.click(this.onChangeSymbolName);
	// Swap the right side to be the symbol editor screen.
	symboldialog.show();
}

jQuery(document).ready(function() {
		symboldialog = jQuery('#symboldialog');
		symboldialog.dialog({width:500,
				   modal: true,
			           autoOpen:false,
				   title:'Please select a file'});
		symboldialog.hide();
		symboldialog.append("<div id='symbollist'/>");
		symboldialog.append("<div id='symbol'><input type='text' id='newsymbolname'> <input type='submit' value='change' id='changename'></div> ");
		});

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
	var top = xmlDoc.getElementsByTagNameNS("http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd","scenes")[0];
	self.current = top.getAttribute("current");
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
	if (max < 20) max = 30;
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
	var select = xmlDoc.getElementsByTagNameNS("http://madbutterfly.sourceforge.net/DTD/madbutterfly.dtd","select");
	len = select.length;
	selectobjs = [];
	for(i=0;i<len;i++) {
		selectobjs.push(select[i].getAttribute('ref'));
	}
	self.selected_objects = selectobjs;
	self.layers = layers;
	self.scenes = scenes;
	self.maxframe = max;
	self.doc = xmlDoc;
}

MBSVGString.prototype=MBSVG.prototype;
MBSVG.prototype.renderUI=function()
{
	var layers = this.layers;
	var scenes = this.scenes;
	var max = this.maxframe;
	var cmd = "<table border=0>\n";
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
					cmd = cmd + "<td><img class='normal' width='16' src=start.png id='"+id_str+"' onClick='selectCell(this)' /></td>";
					empty = 0;
					break;
				} else if ((n>scenes[k].start)&&(n <= scenes[k].end)) {
					cmd = cmd + "<td><img class='normal' width='16' src=fill.png id='"+id_str+"' onClick='selectCell(this)' /></td>";
					empty = 0;
					break;
				}
			}
			if (empty) {
				cmd = cmd + "<td><img class='normal' width='16' src=empty.png id='"+id_str+"'onClick='selectCell(this)' /></td>";
			}

		}
		cmd = cmd + "</tr>\n";
	}
	cmd = cmd + "</table>\n";
	var frame = document.getElementById('frame');
	frame.innerHTML=cmd;
}


MBSVG.prototype.findSymbolName=function(id)
{
	var obj = this.doc.getElementById(id);
	var name = obj.getAttribute('mbname');
	return name;

}


/**
 *   Return a new XML document that all objects which is not in the current scene are deleted.
 *   
 *   This function will traverse all scenes. For each scene which is not in the current scene, we will delete the reference group.
 */

function deepcopy(obj)
{
   var seenObjects = [];
   var mappingArray = [];
   var	f = function(simpleObject) {
      var indexOf = seenObjects.indexOf(simpleObject);
      if (indexOf == -1) {			
         switch (Ext.type(simpleObject)) {
            case 'object':
               seenObjects.push(simpleObject);
               var newObject = {};
               mappingArray.push(newObject);
               for (var p in simpleObject) 
                  newObject[p] = f(simpleObject[p]);
               newObject.constructor = simpleObject.constructor;				
            return newObject;
 
            case 'array':
               seenObjects.push(simpleObject);
               var newArray = [];
               mappingArray.push(newArray);
               for(var i=0,len=simpleObject.length; i<len; i++)
                  newArray.push(f(simpleObject[i]));
            return newArray;
 
            default:	
            return simpleObject;
         }
      } else {
         return mappingArray[indexOf];
      }
   };
   return f(obj);		
}

MBSVG.prototype.generateCurrentSVG=function()
{
	var i;
	var scenes = this.scenes;
	var len = scenes.length;
	var newcopy = $(this.doc).clone();

	for(i=0;i<len;i++) {
		if (scenes[i].start > this.current || scenes[i].end < this.current) {
			var obj = newcopy.find('#'+scenes[i].ref);
			obj.remove();
		}
	}
	return newcopy;
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
	var id = obj.getAttribute('id');
	if (id == 'Jump') {
		if (inkscape.isInProgress != 0) return;
		if (currentScene != 0)
			inkscape.gotoScene(currentScene);
	} else if (id == 'InsertKey') {
		if (inkscape.isInProgress != 0) return;
		inkscape.insertKey(currentScene);
	} else if (id == 'ExtendScene') {
		if (inkscape.isInProgress != 0) return;
		inkscape.extendScene(currentScene);
	} else if (id == 'DeleteScene') {
		if (inkscape.isInProgress != 0) return;
		inkscape.deleteScene(currentScene);
	} else if (id == 'MakeSymbol') {
		if (inkscape.isInProgress != 0) return;
		inkscape.MakeSymbol();
	} else if (id == 'Save') {
		project_save();
	} else if (id == 'Test') {
		if (project_compile()) {
			project_run();
		} else {
		}
	} else if (id == 'Open') {
		filedialog = jQuery('#filedialog');
		filedialog.dialog({width:500,
			   modal: true,
		           autoOpen:false,
			   title:'Please select a file'});
		filedialog.show();
		filedialog.html('Please select the project file<br>');
		filedialog.append('<input type=file value="Select the project file" id="mbsvg" accept="image/png">');
		filedialog.append('<input type=button value="Load" onclick="project_loadFile()">');
		filedialog.dialog("open");
	} else if (id == 'EditAnimation') {
		inkscape.editAnimation();
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

function project_compile()
{
}


function project_showFile(node)
{
	var file = node.textContent;
	if (endsWith(file,"mbsvg")) {
		project_loadScene(node);
	} else {
		project_loadEditor(node);
	}

}
function project_loadScene(node)
{
	var file = node.textContent;
	if (file.substr(0,1) == '/') 
		inkscape = new Inkscape("file://"+file);
	else
		inkscape = new Inkscape("file://"+project_dir+'/'+file);
}


function project_loadEditor(node)
{
	var file = node.textContent;
	if (file.substr(0,1) == '/')
		editor = new TextEditor("file://"+file);
	else
		editor = new TextEditor("file://"+project_dir+'/'+file);
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
	project_tree = tree;
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
	    callback : {
	        ondblclk : function(NODE,TREE_OBJ) { project_showFile(NODE); TREE_OBJ.toggle_branch.call(TREE_OBJ, NODE); TREE_OBJ.select_branch.call(TREE_OBJ, NODE);}
	    },
	    ui : {
		context :  [ 
			{
				id: "Open",
				label: "Open",
				icon: "open.png",
				visible: function(NODE,TREE_OBJ) {  if(NODE.length != 1) return false; return true;},
				action: function(NODE,TREE_OBJ) { onTree_openFile(NODE,TREE_OBJ);}
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

function fileDialog_cb()
{
	var file = $('#filedialogsrc').attr('value');
	filedialog.dialog('close');
	filedialog_cb(file,filedialog_arg);
}

function openFileDialog(callback,arg)
{
	filedialog_cb = callback;
	filedialog_arg = arg;
	filedialog.html('Please select the scene file<br>');
	filedialog.append('<input type=file value="Select the scene file" id="filedialogsrc">');
	filedialog.append('<input type=button value="Load" onclick="fileDialog_cb()">');
	filedialog.show();
	filedialog.dialog('open');
}


function project_addScene(file,treeobj)
{
	if (file == '') {
		return;
	}
	treeobj.create(false,treeobj.selected,file);
}

function onTree_addSceneFile(node,treeobj)
{
	openFileDialog(project_addScene,treeobj);
}

function project_addSource(file,treeobj)
{
	treeobj.create(false,treeobj.selected,file);
}

function onTree_addSourceFile(node,treeobj)
{
	openFileDialog(project_addSource,treeobj);
}

function onTree_openFile(node,treeobj)
{
	if (node[0].id == "scenes") {
		onTree_addSceneFile(node,treeobj);
	} else if (node[0].id == "sources") {
		onTree_addSourceFile(node,treeobj);
	}
}

function system_open_read(fname) {
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
	} catch(e) {
		alert(fname+" does not exist");
	}
	return sis;
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
function system_open_write(fname) {
	try {
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
	} catch (e) {
		alert("Permission to read file was denied.");
	}
	var file = Components.classes["@mozilla.org/file/local;1"]
					.createInstance(Components.interfaces.nsILocalFile);
	try {
		file.initWithPath( fname );
		var fostream = Components.classes["@mozilla.org/network/file-output-stream;1"]
						.createInstance( Components.interfaces.nsIFileOutputStream );
		fostream.init( file,0x02|0x8|0x20, 0666,0);
	} catch(e) {
		alert('can not create '+fname);
	}
	return fostream;
}
function system_write(fname,xml) {
	try {
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
	} catch (e) {
		alert("Permission to read file was denied.");
	}
	var file = Components.classes["@mozilla.org/file/local;1"]
					.createInstance(Components.interfaces.nsILocalFile);
	try {
		file.initWithPath( fname );
		var fostream = Components.classes["@mozilla.org/network/file-output-stream;1"]
						.createInstance( Components.interfaces.nsIFileOutputStream );
		fostream.init( file,0x02|0x8|0x20, 0666,0);
		fostream.write( xml,xml.length );
		fostream.close();
	} catch(e) {
		alert(fname+" does not exist");
	}
}

function system_mkdir(path)
{
	try {
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
	} catch (e) {
		alert("Permission to read file was denied.");
	}
	var file = Components.classes["@mozilla.org/file/local;1"]
					.createInstance(Components.interfaces.nsILocalFile);
	try {
		file.initWithPath(path);
		if( !file.exists() || !file.isDirectory() ) {   // if it doesn't exist, create
			   file.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0777);
		}
	} catch(e) {
		alert('Failed to create directopry '+path+e);
	}
}

function getPathDirectory(path)
{
	var s = path.lastIndexOf('/');
	if (s == -1)
		return '';
	else
		return path.substr(0,s);
}

function project_loadFile()
{
	prjname = $('#mbsvg').attr('value');
	project_name = prjname;
	project_dir = getPathDirectory(prjname);
	var f = system_open_write("/tmp/madbuilder.ws");
	var s = "last="+prjname;
	f.write(s,s.length);
	f.close();
	var prj = system_read(prjname);
	project_parse(prj);
	filedialog.dialog('close');
}


function project_save()
{
	var i;
	
	var xml = "<project>\n";
	var scenes = $('#scenes');
	var sources = $('#sources');
	var list = project_tree.getJSON(scenes);
	var len = list.children.length;

	for(i=0;i<len;i++) {
		xml = xml + "\t<scene src='"+list.children[i].data+"' />\n";
	}
	list = project_tree.getJSON(sources);
	len = list.children.length;
	for(i=0;i<len;i++) {
		xml = xml + "\t<source src='"+list.children[i].data+"' />\n";
	}
	xml = xml + "</project>\n";
	system_write(project_name,xml);

}


function onLoadProject(path)
{
	project_name = path;
	project_dir = getPathDirectory(project_name);
	var prj = system_read(project_name);
	project_parse(prj);
}

function loadOldProject()
{
	return -1;
	var f = system_open_read("/tmp/madbuilder.ws");
	if (f == null) return -1;
	var s = f.read(f.available());
	f.close();

	var pos = s.indexOf("last=");
	if (pos == -1) return -1;
	var m = s.match("last=([^\s]*)");
	if (m[1]) {
		var prj = system_read(m[1]);
		project_dir = getPathDirectory(m[1]);
		project_parse(prj);
		$('#filedialog').dialog("close");
	}
	return 0;
	
}

