asdasd=1;
function MadSwatter(inkscape)
{
	this.inkscape = inkscape;
}


function MadSwatter_callback(mbsvg,self)
{

	$(self.inkscape.mbsvg.doc).find("channels").replaceWith(mbsvg.find("channels"));
	$('#display').tabs('select',0);
	//self.inkscape.updateDocument();
}

MadSwatter.prototype.edit=function()
{
	// Generate a new document with the current scene
	// FIXME: Do we need to do animation filter here?
	// FIXME: Do we need to support hiearchical animation?
	$('#display').tabs('select',2);
	var kk = this.inkscape.mbsvg.generateCurrentSVG();
	//MadSwatter_invoke(kk,MadSwatter_callback,this);
	// Simulate the MadSwatter
	kk.find("metadata").append("<channels></channels>");
	setTimeout(function () { MadSwatter_callback(kk,this);}, 3000);
}
