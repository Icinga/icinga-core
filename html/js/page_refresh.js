// Written by Jannis Moßhammer and Ricardo Bartels
// Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)

/* set via document_header in *.cgi
var refresh_rate=60;
var do_refresh=true;
*/

function update_text(id,text) {
	if (document.getElementById(id) != null )
		document.getElementById(id).innerHTML = text;
}

function update_refresh_counter() {
	if (counter_seconds<=0) {
		update_text('refresh_text','- Updating now');
		window.location.href=window.location.href;
	} else if(do_refresh){ 
		update_text('refresh_text','- Update in '+counter_seconds+' second'+((counter_seconds != 1) ? 's':''));
		update_text('refresh_button','[pause]');
		counter_seconds--;
		setTimeout("update_refresh_counter()",1000);
	} else {
		update_text('refresh_text','- Update is PAUSED');
		update_text('refresh_button','[continue]');
	}
}

function reset_counter() {
	counter_seconds=refresh_rate;
}

function toggle_refresh() {
	do_refresh = (do_refresh) ? false : true;
	reset_counter();
	update_refresh_counter();
}

reset_counter();
update_refresh_counter();
