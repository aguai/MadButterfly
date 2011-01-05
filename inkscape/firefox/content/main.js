var last_select;
var wizard;
jQuery(document).ready(function() {
		if (loadOldProject()) {
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
			last_select = null;
			wizard = new Wizard();
			wizard.cb = onLoadProject;
			$('#filedialog').dialog({ width:500});
			$('#frame').draggable();
			$('#btns').draggable({cursor:'crosshair'});
			$('#list').tabs();
			$('#display').tabs();
		}
		});

