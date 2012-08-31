// Written by Rune Darrud
// Modified by Ricardo Bartels
// Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)

function isValidForSubmit(form) {
	var group = document.getElementById(form);
	var x, len = group.length;
	var checkboxvalue = "false";
	for(x=0; x<len; x++) {
		if(group[x].checked) {
			break;
		}
	}
	if(x < len)
		checkboxvalue = "true";

	if (document.forms[form].CommandButton)
		document.forms[form].buttonCheckboxChecked.value=checkboxvalue;

	enableDisableButton(form);

	if (checkboxvalue=="false")
		return false;

	return true;
}

function enableDisableButton(form) {
	var disabled = true;

	if (form=='tableformhostcomment' || form=='tableformservicecomment' || form=='tableformhostdowntime' || form=='tableformservicedowntime') {
		if (document.forms[form].buttonCheckboxChecked.value=='true'){
			disabled = false;
		}
	}
	if (form=='tableformhost' || form=='tableformservice' ) {
		if (document.forms[form].buttonValidChoice.value=='true' && document.forms[form].buttonCheckboxChecked.value=='true'){
			disabled=false;
		}
	}
	
	if (document.forms[form].CommandButton)
		document.forms[form].CommandButton.disabled=disabled;

	// update refresh counter
	icinga_reset_counter();
}

function showValue(form,arg,schedule_host_check,schedule_host_svc_check) {
	if (arg!='nothing') {
		document.forms[form].hiddencmdfield.value = arg;
		// Set the value to true.
		document.forms[form].buttonValidChoice.value = 'true';

		if (arg==schedule_host_check || arg==schedule_host_svc_check)
			document.forms[form].hiddenforcefield.value = 'yes';
		else
			document.forms[form].hiddenforcefield.value = 'no';

		enableDisableButton(form);
	} else {
		// Set the value to false, cant submit
		document.forms[form].buttonValidChoice.value = 'false';
		enableDisableButton(form);
	}
}

checked=false;
function checkAll(form) {
	var checkboxes = document.getElementById(form);
	checked = (checked == false) ? true : false;
	for (var i=0; i < checkboxes.elements.length; i++) {
		if (checkboxes.elements[i].type == "checkbox" ) {
			checkboxes.elements[i].checked = checked;
		}
	}
}

function toggle_checkbox(id, form) {
	var x = document.getElementById(id);

	x.checked = ( x.checked == true ) ? false : true;

	isValidForSubmit(form);

	return true;
}
