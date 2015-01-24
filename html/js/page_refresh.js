// Written by Jannis Moßhammer, Ricardo Bartels and Michael Friedrich
// Copyright (c) 2009-2015 Icinga Development Team (http://www.icinga.org)

/* set via document_header in *.cgi
var refresh_rate=60;
var do_refresh=true;
*/

/* why can't we get a proper standard for html objects ...
http://stackoverflow.com/questions/871399/cross-browser-method-for-detecting-the-scrolltop-of-the-browser-window
inspired by http://www.redips.net/javascript/maintain-scroll-position/?param1=a&param2=b&param3=c&scroll=319
*/

/* get the scroll position,  */
function icinga_get_scroll_position() {

	var scroll_pos;

	//most browsers
	if (typeof window.pageYOffset != 'undefined') {
		scroll_pos = window.pageYOffset;
	}
	//IE6
	else if (typeof document.documentElement.scrollTop != 'undefined') {
		scroll_pos = document.documentElement.scrollTop;
	}
	//DOM (IE quirks mode)
	else {
		scroll_pos = document.body.scrollTop;
	}

	//debug - overwrite the title with the current scroll position
	//document.title = 'icinga_get_scroll_position=' + window.scroll_pos;

	return scroll_pos;
}

/* reload the page, but push the scroll position into the url */
function icinga_reload_scroll_position() {
	/* save current scrolling position */
	scroll_pos = icinga_get_scroll_position();

	/* if scroll position is zero, remove it from the url and reload
	   if scroll position is NOT zero, add/update scroll option and reload */
	window.location.href = icinga_update_url_option(window.location.href, "scroll", (scroll_pos <= 0) ? null : scroll_pos);
}

/* check if url provided a scroll position, and scroll there */
function icinga_set_scroll_position() {

//	scroll_pos = 666;
	var search = window.location.search;
	var match;

	if (search) {
		match = /scroll=(\d+)/.exec(search);
		if(match) {
			scroll_pos = match[1];
			window.scrollTo(0, scroll_pos);
			//debug
			//document.title = 'icinga_set_scroll_position=' + scroll_pos;
		}
	}
}

function icinga_update_refresh_counter() {
	if (counter_seconds<=0) {
		icinga_update_text('refresh_text','Updating now');
		icinga_reload_scroll_position();
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

/* we need to wait until the page is loaded to scroll to the url saved scroll pos */
if (window.addEventListener)
	window.addEventListener('load', icinga_set_scroll_position, false)
else if (window.attachEvent)
	 window.attachEvent('onload', icinga_set_scroll_position)

