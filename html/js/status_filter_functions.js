// Written by Ricardo Bartels
// Copyright (c) 2009-2015 Icinga Development Team (http://www.icinga.org)

/****************** HOST AND SERVICE FILTER PROPERTIES  *******************/

/*
	Changes here must be represented in "includes/cgiutils.h"
*/

var SCHEDULED_DOWNTIME		= 1
var NO_SCHEDULED_DOWNTIME	= 2
var STATE_ACKNOWLEDGED		= 4
var STATE_UNACKNOWLEDGED	= 8
var CHECKS_DISABLED		= 16
var CHECKS_ENABLED		= 32
var EVENT_HANDLER_DISABLED	= 64
var EVENT_HANDLER_ENABLED	= 128
var FLAP_DETECTION_DISABLED	= 256		/* They are flipped for services */
var FLAP_DETECTION_ENABLED	= 512		/* No idea why you would do that */
var IS_FLAPPING			= 1024
var IS_NOT_FLAPPING		= 2048
var NOTIFICATIONS_DISABLED	= 4096
var NOTIFICATIONS_ENABLED	= 8192
var PASSIVE_CHECKS_DISABLED	= 16384
var PASSIVE_CHECKS_ENABLED	= 32768
var MODIFIED_ATTRIBUTES		= 65536
var NO_MODIFIED_ATTRIBUTES	= 131072
var HARD_STATE			= 262144
var SOFT_STATE			= 524288
var STATE_HANDLED		= 1048576
var NOT_ALL_CHECKS_DISABLED	= 2097152

/* <RANT>
	<TITLE>  MOST ANNOYING BUG EVER  </TITLE>

	in "includes/cgiutils.h" all property IDs for hosts and services are
	the same, except for "FLAP_DETECTION". These are flipped.

	Why in hell would you do that? How didn't you notice it before?
	You could have changed it in an early release and now
	everybody implemented this crap and changing it back would break
	all implementations.

	And for which insane reason got it implemented twice (hosts and services)
	if name and value are always the same, except ..., you know?

	RB

</RANT> */

/****************** HOST AND SERVICE STATUS TYPES *******************/

var valid_host_status_types	= new Array();
valid_host_status_types[1]	= "Pending";
valid_host_status_types[2]	= "Up";
valid_host_status_types[4]	= "Down";
valid_host_status_types[8]	= "Unreachable";

var valid_service_status_types	= new Array();
valid_service_status_types[1]	= "Pending";
valid_service_status_types[2]	= "Ok";
valid_service_status_types[4]	= "Warning";
valid_service_status_types[8]	= "Unknown";
valid_service_status_types[16]	= "Critical";


/****************** VAR DEFAULTS *******************/

var filter_ids			= [ "host_status_types", "host_properties", "service_status_types", "service_properties" ];
var fade_color_from 		= "#FFFE50";		/* Color a changed filter fades from */
var fade_color_to 		= "#FFFFC5";		/* Color a changed filter fades to */

/*
	Will be set in status.cgi during page load
*/
var all_host_status_types	= 0;
var all_host_problems		= 0;
var host_status_types		= 0;
var host_properties		= 0;
var org_host_status_types	= 0;
var org_host_properties		= 0;
var all_service_status_types	= 0;
var all_service_problems	= 0;
var service_status_types	= 0;
var service_properties		= 0;
var org_service_status_types	= 0;
var org_service_properties	= 0;


var filters_loaded		= false;		/* initialize filter once after first click on "Set Filters" */
var in_toggle			= false;		/* prevent an infinite loop in icinga_filter_toggle() */


/****************** FUNCTIONS *******************/

function icinga_set_status_types_text(filter) {

	var return_status_types_text = new Array();

	if (filter == "host_status_types") {
		var valid_status_types = valid_host_status_types;
		var status_types = host_status_types;
		var all_status_types = all_host_status_types;
		var all_problem_status_types = all_host_problems;
	} else {
		var valid_status_types = valid_service_status_types;
		var status_types = service_status_types;
		var all_status_types = all_service_status_types;
		var all_problem_status_types = all_service_problems;
	}

	if (status_types == all_status_types)
		return_status_types_text[0] = "All";
	else if (status_types == all_problem_status_types)
		return_status_types_text[0] = "All problems";
	else {
		for (var i = 0, len = valid_status_types.length; i < len; i++) {
			if (valid_status_types[i] != undefined && status_types & i)
				return_status_types_text.push(valid_status_types[i]);
		}
	}

	icinga_update_text(filter + '_text', return_status_types_text.join(" | "));

	return;
}

function icinga_set_properties_text(filter) {

	var return_properties_text = new Array();

	status_properties = (filter == "host_properties") ? host_properties : service_properties;

	if (status_properties == 0)
		return_properties_text[0] = "Any";
	else {
		if (status_properties & SCHEDULED_DOWNTIME)
			return_properties_text.push("In Scheduled Downtime");

		if (status_properties & NO_SCHEDULED_DOWNTIME)
			return_properties_text.push("Not In Scheduled Downtime");

		if (status_properties & STATE_ACKNOWLEDGED)
			return_properties_text.push("Has Been Acknowledged");

		if (status_properties & STATE_UNACKNOWLEDGED)
			return_properties_text.push("Has Not Been Acknowledged");

		if (status_properties & CHECKS_DISABLED)
			return_properties_text.push("Active Checks Disabled");

		if (status_properties & CHECKS_ENABLED)
			return_properties_text.push("Active Checks Enabled");

		if (status_properties & EVENT_HANDLER_DISABLED)
			return_properties_text.push("Event Handler Disabled");

		if (status_properties & EVENT_HANDLER_ENABLED)
			return_properties_text.push("Event Handler Enabled");

		/* It's flipped, I can't belive it */
		if (filter == "host_properties") {
			if (status_properties & FLAP_DETECTION_DISABLED)
				return_properties_text.push("Flap Detection Disabled");

			if (status_properties & FLAP_DETECTION_ENABLED)
				return_properties_text.push("Flap Detection Enabled");
		} else {
			if (status_properties & FLAP_DETECTION_DISABLED)
				return_properties_text.push("Flap Detection Enabled");

			if (status_properties & FLAP_DETECTION_ENABLED)
				return_properties_text.push("Flap Detection Disabled");
		}
		if (status_properties & IS_FLAPPING)
			return_properties_text.push("Is Flapping");

		if (status_properties & IS_NOT_FLAPPING)
			return_properties_text.push("Is Not Flapping");

		if (status_properties & NOTIFICATIONS_DISABLED)
			return_properties_text.push("Notifications Disabled");

		if (status_properties & NOTIFICATIONS_ENABLED)
			return_properties_text.push("Notifications Enabled");

		if (status_properties & PASSIVE_CHECKS_DISABLED)
			return_properties_text.push("Passive Checks Disabled");

		if (status_properties & PASSIVE_CHECKS_ENABLED)
			return_properties_text.push("Passive Checks Enabled");

		if (status_properties & MODIFIED_ATTRIBUTES)
			return_properties_text.push("Modified Attributes");

		if (status_properties & NO_MODIFIED_ATTRIBUTES)
			return_properties_text.push("No Modified Attributes");

		if (status_properties & HARD_STATE)
			return_properties_text.push("In Hard State");

		if (status_properties & SOFT_STATE)
			return_properties_text.push("In Soft State");

		if (status_properties & STATE_HANDLED)
			return_properties_text.push("Problem Handled");

		if (status_properties & NOT_ALL_CHECKS_DISABLED)
			return_properties_text.push("Not All Checks Disabled");
	}

	icinga_update_text(filter + '_text', return_properties_text.join("<br>And "));

	return
}

function icinga_set_status_types_checkboxes(filter) {

	if (filter == "host_status_types") {
		var valid_status_types = valid_host_status_types;
		var status_types = host_status_types;
	} else {
		var valid_status_types = valid_service_status_types;
		var status_types = service_status_types;
	}

	for (var i = 0, len = valid_status_types.length; i < len; i++) {
		if (valid_status_types[i] != undefined && status_types & i && document.getElementById(filter + "_" + i) != null)
			document.getElementById(filter + "_" + i).checked = true;
	}

	return;
}

function icinga_set_status_properies(filter) {

	status_properties = (filter == "host_properties") ? host_properties : service_properties;
	t = (filter == "host_properties") ? "h" : "s";

	if (status_properties & SCHEDULED_DOWNTIME)
		document.getElementById("filter_" + t + "p_scheduled_downtime_1").checked = true;
	else if (status_properties & NO_SCHEDULED_DOWNTIME)
		document.getElementById("filter_" + t + "p_scheduled_downtime_2").checked = true;
	else
		document.getElementById("filter_" + t + "p_scheduled_downtime_3").checked = true;

	if (status_properties & STATE_ACKNOWLEDGED)
		document.getElementById("filter_" + t + "p_acknowledge_1").checked = true;
	else if (status_properties & STATE_UNACKNOWLEDGED)
		document.getElementById("filter_" + t + "p_acknowledge_2").checked = true;
	else
		document.getElementById("filter_" + t + "p_acknowledge_3").checked = true;

	if (status_properties & CHECKS_ENABLED)
		document.getElementById("filter_" + t + "p_active_checks_1").checked = true;
	else if (status_properties & CHECKS_DISABLED)
		document.getElementById("filter_" + t + "p_active_checks_2").checked = true;
	else
		document.getElementById("filter_" + t + "p_active_checks_3").checked = true;

	if (status_properties & PASSIVE_CHECKS_ENABLED)
		document.getElementById("filter_" + t + "p_passive_checks_1").checked = true;
	else if (status_properties & PASSIVE_CHECKS_DISABLED)
		document.getElementById("filter_" + t + "p_passive_checks_2").checked = true;
	else
		document.getElementById("filter_" + t + "p_passive_checks_3").checked = true;

	if (status_properties & EVENT_HANDLER_ENABLED)
		document.getElementById("filter_" + t + "p_eventhandler_1").checked = true;
	else if (status_properties & EVENT_HANDLER_DISABLED)
		document.getElementById("filter_" + t + "p_eventhandler_2").checked = true;
	else
		document.getElementById("filter_" + t + "p_eventhandler_3").checked = true;

	/* Again: most annoying bug EVER */
	if (filter == "host_properties") {
		if (status_properties & FLAP_DETECTION_ENABLED)
			document.getElementById("filter_" + t + "p_flap_detection_1").checked = true;
		else if (status_properties & FLAP_DETECTION_DISABLED)
			document.getElementById("filter_" + t + "p_flap_detection_2").checked = true;
		else
			document.getElementById("filter_" + t + "p_flap_detection_3").checked = true;
	} else {
		if (status_properties & FLAP_DETECTION_DISABLED)
			document.getElementById("filter_" + t + "p_flap_detection_1").checked = true;
		else if (status_properties & FLAP_DETECTION_ENABLED)
			document.getElementById("filter_" + t + "p_flap_detection_2").checked = true;
		else
			document.getElementById("filter_" + t + "p_flap_detection_3").checked = true;
	}

	if (status_properties & IS_FLAPPING)
		document.getElementById("filter_" + t + "p_is_flapping_1").checked = true;
	else if (status_properties & IS_NOT_FLAPPING)
		document.getElementById("filter_" + t + "p_is_flapping_2").checked = true;
	else
		document.getElementById("filter_" + t + "p_is_flapping_3").checked = true;

	if (status_properties & NOTIFICATIONS_ENABLED)
		document.getElementById("filter_" + t + "p_notifications_1").checked = true;
	else if (status_properties & NOTIFICATIONS_DISABLED)
		document.getElementById("filter_" + t + "p_notifications_2").checked = true;
	else
		document.getElementById("filter_" + t + "p_notifications_3").checked = true;

	if (status_properties & HARD_STATE)
		document.getElementById("filter_" + t + "p_state_type_1").checked = true;
	else if (status_properties & SOFT_STATE)
		document.getElementById("filter_" + t + "p_state_type_2").checked = true;
	else
		document.getElementById("filter_" + t + "p_state_type_3").checked = true;

	if (status_properties & MODIFIED_ATTRIBUTES)
		document.getElementById("filter_" + t + "p_modified_attributes_1").checked = true;
	else if (status_properties & NO_MODIFIED_ATTRIBUTES)
		document.getElementById("filter_" + t + "p_modified_attributes_2").checked = true;
	else
		document.getElementById("filter_" + t + "p_modified_attributes_3").checked = true;

	if (status_properties & STATE_HANDLED)
		document.getElementById("filter_" + t + "p_state_handled_1").checked = true;
	else
		document.getElementById("filter_" + t + "p_state_handled_2").checked = true;

	if (status_properties & NOT_ALL_CHECKS_DISABLED)
		document.getElementById("filter_" + t + "p_not_all_checks_disabled_1").checked = true;
	else
		document.getElementById("filter_" + t + "p_not_all_checks_disabled_2").checked = true;

	return;
}

/* read status types selected and update status type text */
function icinga_update_status_types(filter) {

	return_status_types = 0

	valid_status_types = (filter == "host_status_types") ? valid_host_status_types : valid_service_status_types;

	for (var i = 0, len = valid_status_types.length; i < len; i++) {
		if (valid_status_types[i] != undefined && document.getElementById(filter + "_" + i) != null && document.getElementById(filter + "_" + i).checked == true)
			return_status_types += i;
	}

	if (filter == "host_status_types")
		host_status_types = (return_status_types != 0) ? return_status_types : all_host_status_types;
	else
		service_status_types = (return_status_types != 0) ? return_status_types : all_service_status_types;

	icinga_set_status_types_text(filter);

	return;
}

/* read status properties selected and update status properties text */
function icinga_update_properties(filter) {
	status_properties = 0;

	t = (filter == "host_properties") ? "h" : "s";

	status_properties += parseInt($('input[name="filter_' + t + 'p_scheduled_downtime"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_acknowledge"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_active_checks"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_passive_checks"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_eventhandler"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_flap_detection"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_is_flapping"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_notifications"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_state_type"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_modified_attributes"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_state_handled"]:checked').val());
	status_properties += parseInt($('input[name="filter_' + t + 'p_not_all_checks_disabled"]:checked').val());

	if (filter == "host_properties")
		host_properties = status_properties;
	else
		service_properties = status_properties;

	icinga_set_properties_text(filter);

	return;
}

/* load filters on first request */
function icinga_load_filters(filter) {

	if (filters_loaded)
		return;

	$(document).ready(function() {

		/* initilize text and options set */
		icinga_set_status_types_text("host_status_types");
		icinga_set_status_types_checkboxes("host_status_types");

		icinga_set_properties_text("host_properties");
		icinga_set_status_properies("host_properties");

		icinga_set_status_types_text("service_status_types");
		icinga_set_status_types_checkboxes("service_status_types");

		icinga_set_properties_text("service_properties");
		icinga_set_status_properies("service_properties");

		/* initilize buttons */
		$("[id=apply_button],#submit_button").button();
		$("[id=radio]").buttonset();

		/* keep submit button on same level as filer box */
		$("#submit_button").css("z-index",$("#display_filters_box").css("z-index"));

		filters_loaded = true;
	});

	return;
}

/* dye background of filter if filtger got changed */
function icinga_changed_filter_coloring(section) {

	var set_color = false;

	if (section == "host_status_types" )
		set_color = (host_status_types != org_host_status_types) ? true : false;

	if (section == "host_properties" )
		set_color = (host_properties != org_host_properties) ? true : false;

	if (section == "service_status_types" )
		set_color = (service_status_types != org_service_status_types) ? true : false;

	if (section == "service_properties" )
		set_color = (service_properties != org_service_properties) ? true : false;

	if (set_color == true) {
		$('#' + section + "_text_cell").css("background-color", fade_color_from);
		$('#' + section + "_text_cell").stop(true, true).delay(300).animate({ backgroundColor: fade_color_to }, 2000 );
	} else {
		$('#' + section + "_text_cell").animate({ backgroundColor: "white" }, 1000 );
		$('#' + section + "_text_cell").css("background-color", "");
	}

	return;
}

/* open and close the different filter options boxes*/
function icinga_filter_toggle(section) {

	/* initilize filter on first load */
	if (section == "display_filters")
		icinga_load_filters();

	/* close other box, before open the next one */
	if(in_toggle == false) {

		/* Don't hit here if try to close all other boxes */
		in_toggle = true;

		for (var i = 0, len = filter_ids.length; i < len; i++) {
			if (filter_ids[i] != undefined && filter_ids[i] != section && document.getElementById(filter_ids[i] + "_box").style.display != 'none') {
				icinga_filter_toggle(filter_ids[i]);
			}
		}

		in_toggle = false;
	}

	if (section == "host_status_types" || section == "service_status_types")
		icinga_update_status_types(section);

	if (section == "host_properties" || section == "service_properties")
		icinga_update_properties(section);

	/* check if filter got changed and add a bit of color if needed :D */
	if (section != "display_filters")
		icinga_changed_filter_coloring(section);

	/* yes we toggle */
	$( '#' + section + '_box' ).toggle( 'blind', {}, 150 );

	/* reset counter to prevent the page from reloading to soon */
	icinga_reset_counter();

	/* allways return false */
	return false;
}

/* apply all filter, update url, reload page with new url */
function icinga_apply_new_filters() {

	/* update status types and properties ahead of submission
	   in case the user changed an option without using the "Apply" button */
	icinga_update_status_types("host_status_types");
	icinga_update_status_types("service_status_types");
	icinga_update_properties("host_properties");
	icinga_update_properties("service_properties");

	/* get current url */
	var url = window.location.href;

	/* manipulate url with new values */
	if (host_properties == 0)
		url = icinga_update_url_option(url, "hostprops", null);
	else
		url = icinga_update_url_option(url, "hostprops", host_properties);

	if (host_status_types == all_host_status_types)
		url = icinga_update_url_option(url, "hoststatustypes", null);
	else
		url = icinga_update_url_option(url, "hoststatustypes", host_status_types);

	if (service_properties == 0)
		url = icinga_update_url_option(url, "serviceprops", null);
	else
		url = icinga_update_url_option(url, "serviceprops", service_properties);

	if (service_status_types == all_service_status_types)
		url = icinga_update_url_option(url, "servicestatustypes", null);
	else
		url = icinga_update_url_option(url, "servicestatustypes", service_status_types);

	/* reload page with new values */
	window.location.href = url;

	return false;
}

// EOF
