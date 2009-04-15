asdasd=1;
function MadSwatter(inkscape)
{
	this.inkscape = inkscape;
}


function MadSwatter_callback(mbsvg,self)
{
	var ch = $(mbsvg).find("channels");
	try {
		var oldch = $(inkscape.mbsvg.doc).find("channels");
		var newch = $(mbsvg).find("channels");
		oldch.replaceWith(newch);
		$('#display').tabs('select',0);
	} catch(e) {
		alert(e);
	}
	//self.inkscape.updateDocument();
}



// Call MadSwatter, we will create file in the /tmp and then create an iframe inside thge madswatter div to
// load this file. After the MadSwatter finish the job, it should send an event editcb back to us. In this way,
// we can avoid the security issue.
function MadSwatter_invoke(dom,callback,self)
{
	var f = system_open_write("/tmp/swatter.tmp");
	var serializer = new XMLSerializer();
	var cont = serializer.serializeToString(dom[0]);

	f.write(cont,cont.length);
	f.close();
	$('#madswatter').html('<iframe width="900" height="700" src="chrome://madswatter/content/editor.xhtml"></iframe>');
}

MadSwatter.prototype.edit=function()
{
	// Generate a new document with the current scene
	// FIXME: Do we need to do animation filter here?
	// FIXME: Do we need to support hiearchical animation?
	var kk = this.inkscape.mbsvg.generateCurrentSVG();
	$('#display').tabs('select',2);
	MadSwatter_invoke(kk,MadSwatter_callback,this);
	// Simulate the MadSwatter
	//kk.find("metadata").append("<channels></channels>");
	//setTimeout(function () { MadSwatter_callback(kk,this);}, 3000);
}
