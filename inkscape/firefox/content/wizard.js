function Wizard()
{
	this.dialog = $('#wizard');
	this.dialog.dialog({width:600,autoOpen:false});
	this.step2 = $('#wizard_step2');
	this.step2.dialog({width:600,autoOpen:false});
	this.step3 = $('#wizard_step3');
	this.step3.dialog({width:600,autoOpen:false});
}


Wizard.prototype.execute=function(cb)
{
	this.dialog.dialog('open');
}

aaa=1
// In the first step, users will select the project type.
Wizard.prototype.step1_cb=function(type)
{
	this.type = type;
	var obj = $('#wizardname');
	this.name = obj.attr('value');
	this.step2.dialog('open');
	this.dialog.dialog('close');
	this.step3.dialog('close');
}

// In the step 2, get the output path
Wizard.prototype.step2_cb=function()
{
	this.dir = $('#outputpath').attr('value');
	this.step2.dialog('close');
	this.step3.dialog('open');
	system_mkdir(this.dir);
	this.generate_source('main.c','main.c');
	this.step3.append('main.c<br>');
	this.generate_source('list.mbsvg','list.mbsvg');
	this.step3.append('list.mbsvg<br>');
	this.generate_source('app.h',this.name+'.h');
	this.step3.append(this.name+'.h<br>');
	this.generate_source('app.c',this.name+'.c');
	this.step3.append(this.name+'.c<br>');
	this.generate_source('app.prj',this.name+'.prj');
	this.step3.append('app.prj<br>');
	this.generate_source('Makefile','Makefile');
	this.step3.append('Makefile<br>');
	this.done_cb();
}

Wizard.prototype.done_cb=function()
{
	this.step3.dialog('close');
	this.cb(this.dir+'/'+this.name+'.prj');
}


Wizard.prototype.generate_source=function (tmpl,fname)
{
	var file = system_open_write(this.dir+'/'+fname);
	var template = system_open_read('/usr/local/share/mb/template/'+this.type+'/'+tmpl);
	if (template == null) {
		return;
	}
	if (file == null) {
		return;
	}
	var data = template.read(template.available());
	var regex = /\%n/gi;
	// FIXME: replace name here
	data=data.replace(regex,this.name);
	file.write(data,data.length);
	file.close();
	template.close();
}

