// Written by Rune Darrud
// Modified by Ricardo Bartels
// For Icinga

function replaceCGIString(string,status_cgi,cmd_cgi) {
	sInString = string.replace( status_cgi, cmd_cgi );
	return sInString;
}

function replaceArgString(string) {
	ToBeStripped = location.search;
	sInString = string.replace( ToBeStripped, '' );
	return sInString;
}

function cmd_submit(form) {
	command_arguments = get_check_value(form);
	cmd_typ = 'cmd_typ=' + document.forms[form].hiddencmdfield.value

	arguments = cmd_typ + command_arguments;
	if (form=="tableform") {
		if (document.forms[form].hiddenforcefield.value == 'yes') {
			arguments = arguments + '&force_check';
		}
	}

	bazinga = '?' + arguments;
	fullurl = replaceCGIString(location.href,'status.cgi','cmd.cgi');
	fullurl = replaceCGIString(fullurl,'extinfo.cgi','cmd.cgi');
	fullurl = replaceCGIString(fullurl,'#comments','');
	fullurl = replaceArgString(fullurl);
	fullurl = fullurl + bazinga;
	self.location.assign(fullurl);
	// Remove comment below for debugging of the URL
	//alert(fullurl);
	return fullurl;
}

function isValidForSubmit(form) {
	var group = document.getElementById(form);
	var x, len = group.length;
	var checkboxvalue = "false";
	for(x=0; x<len; x++) {
		if(group[x].checked) {
			break;
		}
	}
	if(x < len) {
		checkboxvalue = "true";
	}

	if (document.forms[form].CommandButton) {
		document.forms[form].buttonCheckboxChecked.value=checkboxvalue;
	}

	enableDisableButton(form);

	if (checkboxvalue=="false") {
		return false;
	}

	return true;
}

function enableDisableButton(form) {
	var disabled = true;

	if (form=='tableformhostcomment' || form=='tableformservicecomment' || form=='tableformhostdowntime' || form=='tableformservicedowntime') {
		if (document.forms[form].buttonCheckboxChecked.value=='true'){
			disabled = false;
		}
	}
	if (form=='tableform') {
		if (document.forms[form].buttonValidChoice.value=='true' && document.forms[form].buttonCheckboxChecked.value=='true'){
			disabled=false;
		}
	}
	
	if (document.forms[form].CommandButton) {
		document.forms[form].CommandButton.disabled=disabled;
	}
}

function get_check_value(form) {
	var check_value = '';
	var checkboxes = document.getElementById(form);

	for (var i =0; i < checkboxes.elements.length; i++) {
		if (checkboxes.elements[i].checked==true){
			if (checkboxes.elements[i].type=="checkbox" && checkboxes.elements[i].value!="all"){
				check_value = check_value + checkboxes.elements[i].value;
			}
		}
	}
	//Remove comment below for debugging
	//alert(check_value);
	return check_value;
}

function showValue(arg,schedule_host_check,schedule_host_svc_check) {
	if (arg!='nothing') {
		document.tableform.hiddencmdfield.value = arg;
		// Set the value to true.
		document.tableform.buttonValidChoice.value = 'true';

		if (arg==schedule_host_check || arg==schedule_host_svc_check) {
			document.tableform.hiddenforcefield.value = 'yes';
		} else {
			document.tableform.hiddenforcefield.value = 'no';
		}
		enableDisableButton("tableform");
	} else {
		// Set the value to false, cant submit
		document.tableform.buttonValidChoice.value = 'false';
		enableDisableButton("tableform");
	}
}

checked=false;
function checkAll(form) {
	var checkboxes = document.getElementById(form);
	if (checked == false) {
		checked = true
	} else {
		checked = false
	}
	for (var i=0; i < checkboxes.elements.length; i++) {
		if (checkboxes.elements[i].type == "checkbox" ) {
			checkboxes.elements[i].checked = checked;
		}
	}
}