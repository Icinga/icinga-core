// Written by Ricardo Bartels
// Copyright (c) 2009-2015 Icinga Development Team (http://www.icinga.org)

function icinga_update_text(id,text) {
	if (document.getElementById(id) != null )
		document.getElementById(id).innerHTML = text;
}

/* Adds or removes options=value from url string, set value to "null" to remove option from url */
function icinga_update_url_option(url, option, new_value) {

	var options = '';
	var base_url = '';
	var new_options_array = new Array();

	if (url.indexOf('?') === -1) {
		base_url = url;
	} else {
		base_url = url.substring(0,url.indexOf('?'));
		options = url.substring(url.indexOf('?')+1);
		options_array = options.split('&');
		for (var i=0; i<options_array.length; i++) {
			if (options_array[i].substring(0,options_array[i].indexOf('=')) != option)
				new_options_array.push(options_array[i]);
		}
	}

	if (new_value != undefined && new_value.length != 0 )
		new_options_array.push(option + '=' + new_value);

	return base_url + '?' + new_options_array.join('&');
}

// EOF