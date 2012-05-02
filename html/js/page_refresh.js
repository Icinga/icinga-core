// Written by Jannis Moßhammer and Ricardo Bartels
// Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)

/* set via document_header in *.cgi
var refresh_rate=60;
var do_refresh=true;
*/

function icinga_update_text(id,text) {
	if (document.getElementById(id) != null )
		document.getElementById(id).innerHTML = text;
}

function icinga_update_refresh_counter() {
	if (counter_seconds<=0) {
		icinga_update_text('refresh_text','Updating now');
		window.location.href=window.location.href;
	} else if(do_refresh) {
		icinga_update_text('refresh_text','Update in '+counter_seconds+' second'+((counter_seconds != 1) ? 's':''));
		icinga_update_text('refresh_button','[pause]');
		counter_seconds--;
		setTimeout("icinga_update_refresh_counter()",1000);
	} else {
		icinga_update_text('refresh_text','Update is PAUSED');
		icinga_update_text('refresh_button','[continue]');
	}
}

function icinga_reset_counter() {
	counter_seconds = refresh_rate;
}

function icinga_toggle_refresh() {
	do_refresh = (do_refresh) ? false : true;
	icinga_reset_counter();
	icinga_update_refresh_counter();
}

function icinga_do_refresh() {
	do_refresh = true;
	counter_seconds = 0;
	icinga_update_refresh_counter();
}

function icinga_stop_refresh() {
	do_refresh = false;
	icinga_update_refresh_counter();
}

function icinga_start_refresh() {
	do_refresh = true;
	icinga_update_refresh_counter();
}

setTimeout("icinga_reset_counter()",100);
setTimeout("icinga_update_refresh_counter()",100);
