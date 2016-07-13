/************************************************************************
 *
 * CGIUTILS.H - Header file for common CGI functions
 *
 * Copyright (c) 1999-2008  Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2015 Icinga Development Team (http://www.icinga.org) 
 *
 * License:
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 ************************************************************************/

#ifndef _CGIUTILS_H
#define _CGIUTILS_H

#include "config.h"
#include "logging.h"
#include "objects.h"
#include "cgiauth.h"
#include "readlogs.h"

#ifdef __cplusplus
extern "C" {
#endif


/**************************** CGI REFRESH ******************************/

#define DEFAULT_REFRESH_RATE	60	/* 60 second refresh rate for CGIs */

#define HTTPHEADER_REFRESH	0
#define JAVASCRIPT_REFRESH	1


/******************************* CGI NAMES **********************************/

#define AVAIL_CGI		"avail.cgi"
#define CMD_CGI			"cmd.cgi"
#define CONFIG_CGI		"config.cgi"
#define EXTINFO_CGI		"extinfo.cgi"
#define HISTOGRAM_CGI		"histogram.cgi"
#define HISTORY_CGI		"history.cgi"
#define NOTIFICATIONS_CGI	"notifications.cgi"
#define OUTAGES_CGI		"outages.cgi"
#define SHOWLOG_CGI		"showlog.cgi"
#define STATUS_CGI		"status.cgi"
#define STATUSMAP_CGI		"statusmap.cgi"
#define SUMMARY_CGI		"summary.cgi"
#define TAC_CGI			"tac.cgi"
#define TRENDS_CGI		"trends.cgi"


/******************************* CGI IDS **********************************/

#define AVAIL_CGI_ID		1
#define CMD_CGI_ID		2
#define CONFIG_CGI_ID		3
#define EXTINFO_CGI_ID		4
#define HISTOGRAM_CGI_ID	5
#define HISTORY_CGI_ID		6
#define NOTIFICATIONS_CGI_ID	7
#define OUTAGES_CGI_ID		8
#define SHOWLOG_CGI_ID		9
#define STATUS_CGI_ID		10
#define STATUSMAP_CGI_ID	11
#define SUMMARY_CGI_ID		12
#define TAC_CGI_ID		13
#define TRENDS_CGI_ID		14


/******************************* ERROR CGI IDS **********************************/

#define ERROR_CGI_STATUS_DATA	1
#define ERROR_CGI_OBJECT_DATA	2
#define ERROR_CGI_CFG_FILE	3
#define ERROR_CGI_MAIN_CFG	4


/**************************** STYLE SHEET NAMES ******************************/

#define COMMON_CSS		"common.css"
#define JQUERY_DD_CSS		"dd.css"

#define AVAIL_CSS		"avail.css"
#define CMD_CSS 		"cmd.css"
#define CONFIG_CSS		"config.css"
#define EXTINFO_CSS		"extinfo.css"
#define HISTOGRAM_CSS		"histogram.css"
#define HISTORY_CSS		"history.css"
#define NOTIFICATIONS_CSS	"notifications.css"
#define OUTAGES_CSS		"outages.css"
#define SHOWLOG_CSS		"showlog.css"
#define STATUS_CSS		"status.css"
#define STATUSMAP_CSS		"statusmap.css"
#define SUMMARY_CSS		"summary.css"
#define TAC_CSS			"tac.css"
#define TAC_HEADER_CSS		"tacheader.css"
#define TRENDS_CSS		"trends.css"


/**************************** JAVASCRIPT NAMES ******************************/

#define CHECKBOX_FUNCTIONS_JS   "checkbox_functions.js"
#define COMMON_FUNCTIONS_JS	"common_functions.js"
#define JQUERY_MAIN_JS		"jquery-1.12.4.min.js"
#define JQUERY_DD_JS		"jquery.dd.min.js"
#define SKINNYTIP_JS		"skinnytip.js"
#define PAGE_REFRESH_JS		"page_refresh.js"
#define STATUS_FILTER_JS	"status_filter_functions.js"


/**************************** JQUERY-UI PATH ******************************/

#define JQ_UI_CORE_JS		"ui/minified/jquery.ui.core.min.js"
#define JQ_UI_WIDGET_JS		"ui/minified/jquery.ui.widget.min.js"
#define JQ_UI_MOUSE_JS		"ui/minified/jquery.ui.mouse.min.js"
#define JQ_UI_SLIDER_JS		"ui/minified/jquery.ui.slider.min.js"
#define JQ_UI_DATEPICKER_JS	"ui/minified/jquery.ui.datepicker.min.js"
#define JQ_UI_EFFECT_JS		"ui/minified/jquery.ui.effect.min.js"
#define JQ_UI_EFFECT_BLIND_JS	"ui/minified/jquery.ui.effect-blind.min.js"
#define JQ_UI_BUTTON_JS		"ui/minified/jquery.ui.button.min.js"
#define JQ_UI_TIMEPICKER_JS	"jquery.ui.timepicker-addon.min.js"

#define JQ_UI_ALL_CSS		"themes/base/jquery.ui.all.css"
#define JQ_UI_TIMEPICKER_CSS	"jquery.ui.timepicker-addon.css"


/********************************* ICONS ************************************/

#define STATUS_ICON_WIDTH		20
#define STATUS_ICON_HEIGHT		20

#define INFO_ICON			"info.png"
#define INFO_ICON_ALT			"Informational Message"
#define START_ICON			"start.gif"
#define START_ICON_ALT			"Program Start"
#define STOP_ICON			"stop.gif"
#define STOP_ICON_ALT			"Program End"
#define RESTART_ICON			"restart.gif"
#define RESTART_ICON_ALT		"Program Restart"
#define OK_ICON				"recovery.png"
#define OK_ICON_ALT			"Service Ok"
#define CRITICAL_ICON			"critical.png"
#define CRITICAL_ICON_ALT		"Service Critical"
#define WARNING_ICON			"warning.png"
#define WARNING_ICON_ALT		"Service Warning"
#define UNKNOWN_ICON			"unknown.png"
#define UNKNOWN_ICON_ALT		"Service Unknown"
#define NOTIFICATION_ICON		"notify.gif"
#define NOTIFICATION_ICON_ALT		"Service Notification"
#define LOG_ROTATION_ICON		"logrotate.png"
#define LOG_ROTATION_ICON_ALT		"Log Rotation"
#define EXTERNAL_COMMAND_ICON		"command.png"
#define EXTERNAL_COMMAND_ICON_ALT	"External Command"

#define STATUS_DETAIL_ICON		"status_detail.gif"
#define STATUSMAP_ICON			"status_map.gif"
#define HISTORY_ICON			"history.gif"
#define TRENDS_ICON			"trends.gif"
#define COLLAPSE_ICON			"icon_collapse.gif"
#define EXPAND_ICON			"icon_expand.gif"
#define DISABLED_ICON			"disabled.gif"
#define ENABLED_ICON			"enabled.gif"
#define NOTIFICATIONS_DISABLED_ICON	"ndisabled.gif"
#define ACKNOWLEDGEMENT_ICON		"ack.gif"
#define REMOVE_ACKNOWLEDGEMENT_ICON	"noack.gif"
#define COMMENT_ICON			"comment.gif"
#define DELETE_ICON			"delete.gif"
#define DELAY_ICON			"delay.gif"
#define DOWNTIME_ICON			"downtime.gif"
#define ACTIVEONLY_ICON			"activeonly.gif"
#define PASSIVE_ICON			"passiveonly.gif"
#define RIGHT_ARROW_ICON		"right.gif"
#define LEFT_ARROW_ICON			"left.gif"
#define UP_ARROW_ICON			"up.gif"
#define DOWN_ARROW_ICON			"down.gif"
#define FLAPPING_ICON			"flapping.gif"
#define EMPTY_ICON			"empty.gif"
#define CMD_STOP_ICON			"cmd_stop.png"

#define ACTIVE_ICON			"active.gif"
#define ACTIVE_ICON_ALT			"Active Mode"
#define STANDBY_ICON			"standby.gif"
#define STANDBY_ICON_ALT		"Standby Mode"

#define HOST_DOWN_ICON			"critical.png"
#define HOST_DOWN_ICON_ALT		"Host Down"
#define HOST_UNREACHABLE_ICON		"critical.png"
#define HOST_UNREACHABLE_ICON_ALT	"Host Unreachable"
#define HOST_UP_ICON			"recovery.png"
#define HOST_UP_ICON_ALT		"Host Up"
#define HOST_NOTIFICATION_ICON		"notify.gif"
#define HOST_NOTIFICATION_ICON_ALT	"Host Notification"

#define SERVICE_EVENT_ICON		"serviceevent.gif"
#define SERVICE_EVENT_ICON_ALT		"Service Event Handler"
#define HOST_EVENT_ICON			"hostevent.gif"
#define HOST_EVENT_ICON_ALT		"Host Event Handler"

#define THERM_OK_IMAGE			"thermok.png"
#define THERM_WARNING_IMAGE		"thermwarn.png"
#define THERM_CRITICAL_IMAGE		"thermcrit.png"

#define NOTES_ICON			"notes.gif"
#define ACTION_ICON			"action.gif"
#define DETAIL_ICON			"detail.gif"
#define TAC_DISABLED_ICON		"tacdisabled.png"
#define TAC_ENABLED_ICON		"tacenabled.png"
#define ZOOM1_ICON			"zoom1.gif"
#define ZOOM2_ICON			"zoom2.gif"
#define EVENTHANDLING_DISABLED_ICON	"eventhandlingdisabled.gif"

#define CONTEXT_HELP_ICON		"contexthelp.gif"

#define SPLUNK_SMALL_WHITE_ICON		"splunk1.gif"
#define SPLUNK_SMALL_BLACK_ICON		"splunk2.gif"

#define DATABASE_ICON			"database.gif"
#define AUTOSAVE_ICON			"save.gif"
#define DAEMON_WARNING_ICON		"warning_triangle.gif"
#define STATS_ICON			"stats.gif"
#define RELOAD_ICON			"icon_reload.png"

#define UNKNOWN_GD2_ICON		"unknown.gd2"
#define UNKNOWN_ICON_IMAGE		"unknown.gif"
#define ICINGA_GD2_ICON			"icinga.gd2"

#define TAC_HEADER_DEFAULT_LOGO		"Icinga_Header_Webinterface.jpg"
#define TAC_HEADER_DEFAULT_LOGO_ALT	"Icinga"
#define TAC_HEADER_LOGO			"Icinga_TAC_Header_Webinterface.jpg"
#define TAC_HEADER_HOST_ICON		"server.png"
#define TAC_HEADER_SERVICE_ICON		"application-monitor.png"
#define TAC_HEADER_EXECUTION_ICON	"hourglass-exclamation.png"
#define TAC_HEADER_LATENCY_ICON		"hourglass-arrow.png"

#define FIRST_PAGE_ACTIVE_ICON		"icon_first_active.gif"
#define FIRST_PAGE_INACTIVE_ICON	"icon_first_inactive.gif"
#define PREVIOUS_PAGE_ACTIVE_ICON	"icon_previous_active.gif"
#define PREVIOUS_PAGE_INACTIVE_ICON	"icon_previous_inactive.gif"
#define NEXT_PAGE_ACTIVE_ICON		"icon_next_active.gif"
#define NEXT_PAGE_INACTIVE_ICON		"icon_next_inactive.gif"
#define LAST_PAGE_ACTIVE_ICON		"icon_last_active.gif"
#define LAST_PAGE_INACTIVE_ICON		"icon_last_inactive.gif"

/* icons taken from http://findicons.com */
#define EXPORT_CSV_ICON			"export_csv.png"
#define EXPORT_CSV_ICON_ALT		"Export to CSV"
#define EXPORT_JSON_ICON		"export_json.png"
#define EXPORT_JSON_ICON_ALT		"Export to JSON"
#define EXPORT_XML_ICON			"export_xml.png"
#define EXPORT_XML_ICON_ALT		"Export to XML"
#define EXPORT_LINK_ICON		"export_link.png"
#define EXPORT_LINK_ICON_ALT		"Link to this page"

/* images */
#define HISTOGRAM_IMAGE			"histogram.png"
#define ICINGA_VRML_IMAGE		"icingavrml.png"
#define TRENDSHOSTS_IMAGE		"trendshost.png"
#define TRENDSSERVICES_IMAGE		"trendssvc.png"


/************************** PLUGIN RETURN VALUES ****************************/

#define STATE_OK		0
#define STATE_WARNING		1
#define STATE_CRITICAL		2
#define STATE_UNKNOWN		3       /* changed from -1 on 02/24/2001 */


/********************* EXTENDED INFO CGI DISPLAY TYPES  *********************/

#define DISPLAY_PROCESS_INFO		0
#define DISPLAY_HOST_INFO		1
#define DISPLAY_SERVICE_INFO		2
#define DISPLAY_COMMENTS		3
#define DISPLAY_PERFORMANCE		4
#define DISPLAY_HOSTGROUP_INFO		5
#define DISPLAY_DOWNTIME		6
#define DISPLAY_SCHEDULING_QUEUE	7
#define DISPLAY_SERVICEGROUP_INFO	8


/************************ COMMAND CGI COMMAND MODES *************************/

#define CMDMODE_NONE		0
#define CMDMODE_REQUEST		1
#define CMDMODE_COMMIT		2


/************************ CGI CONTENT TYPE *********************************/
#define HTML_CONTENT		0
#define IMAGE_CONTENT		1
#define CSV_CONTENT		2
#define JSON_CONTENT		3
#define XML_CONTENT		4


/************************ CSV OUTPUT CHARACTERS ****************************/
#define CSV_DELIMITER		";"
#define CSV_DATA_ENCLOSURE	"'"


/******************** HOST AND SERVICE NOTIFICATION TYPES ******************/

#define NOTIFICATION_ALL		0	/* all service and host notifications */
#define NOTIFICATION_SERVICE_ALL	1	/* all types of service notifications */
#define NOTIFICATION_HOST_ALL		2	/* all types of host notifications */
#define NOTIFICATION_SERVICE_WARNING	4
#define NOTIFICATION_SERVICE_UNKNOWN	8
#define NOTIFICATION_SERVICE_CRITICAL	16
#define NOTIFICATION_SERVICE_RECOVERY	32
#define NOTIFICATION_HOST_DOWN		64
#define NOTIFICATION_HOST_UNREACHABLE	128
#define NOTIFICATION_HOST_RECOVERY	256
#define NOTIFICATION_SERVICE_ACK	512
#define NOTIFICATION_HOST_ACK		1024
#define NOTIFICATION_SERVICE_FLAP	2048
#define NOTIFICATION_HOST_FLAP		4096
#define NOTIFICATION_SERVICE_CUSTOM	8192
#define NOTIFICATION_HOST_CUSTOM	16384


/********************** HOST AND SERVICE ALERT TYPES **********************/

#define HISTORY_ALL			0	/* all service and host alert */
#define HISTORY_SERVICE_ALL		1	/* all types of service alerts */
#define HISTORY_HOST_ALL		2	/* all types of host alerts */
#define HISTORY_SERVICE_WARNING		4
#define HISTORY_SERVICE_UNKNOWN		8
#define HISTORY_SERVICE_CRITICAL	16
#define HISTORY_SERVICE_RECOVERY	32
#define HISTORY_HOST_DOWN		64
#define HISTORY_HOST_UNREACHABLE	128
#define HISTORY_HOST_RECOVERY		256


/****************************** SORT TYPES  *******************************/

#define SORT_NONE			0
#define SORT_ASCENDING			1
#define SORT_DESCENDING			2


/***************************** SORT OPTIONS  ******************************/

#define SORT_NOTHING			0
#define SORT_HOSTNAME			1
#define SORT_SERVICENAME		2
#define SORT_SERVICESTATUS		3
#define SORT_LASTCHECKTIME		4
#define SORT_CURRENTATTEMPT		5
#define SORT_STATEDURATION		6
#define SORT_NEXTCHECKTIME		7
#define SORT_HOSTSTATUS			8
#define SORT_HOSTURGENCY                9
#define SORT_HOSTNAME_SERVICENAME       10
#define SORT_COMMENT			11
#define SORT_DOWNTIME			12
#define SORT_EX_ENTRYTIME		13
#define SORT_EX_AUTHOR			14
#define SORT_EX_COMMENT			15
#define SORT_EX_ID			16
#define SORT_EX_PERSISTENT		17
#define SORT_EX_TYPE			18
#define SORT_EX_EXPIRETIME		19
#define SORT_EX_STARTTIME		20
#define SORT_EX_ENDTIME			21
#define SORT_EX_TRIGGERTIME		22
#define SORT_EX_DURATION		23
#define SORT_EX_INEFFECT		24
#define SORT_EX_TRIGGERID		25


/****************** HOST AND SERVICE FILTER PROPERTIES  *******************/

/*
	Changes here must be represented in "html/js/status_filter_functions.js"
*/

#define HOST_SCHEDULED_DOWNTIME		1
#define HOST_NO_SCHEDULED_DOWNTIME	2
#define HOST_STATE_ACKNOWLEDGED		4
#define HOST_STATE_UNACKNOWLEDGED	8
#define HOST_CHECKS_DISABLED		16
#define HOST_CHECKS_ENABLED		32
#define HOST_EVENT_HANDLER_DISABLED	64
#define HOST_EVENT_HANDLER_ENABLED	128
#define HOST_FLAP_DETECTION_DISABLED	256
#define HOST_FLAP_DETECTION_ENABLED	512
#define HOST_IS_FLAPPING		1024
#define HOST_IS_NOT_FLAPPING		2048
#define HOST_NOTIFICATIONS_DISABLED	4096
#define HOST_NOTIFICATIONS_ENABLED	8192
#define HOST_PASSIVE_CHECKS_DISABLED	16384
#define HOST_PASSIVE_CHECKS_ENABLED	32768
#define HOST_MODIFIED_ATTRIBUTES	65536
#define HOST_NO_MODIFIED_ATTRIBUTES	131072
#define HOST_HARD_STATE			262144
#define HOST_SOFT_STATE			524288
#define HOST_STATE_HANDLED		1048576
#define HOST_NOT_ALL_CHECKS_DISABLED	2097152


#define SERVICE_SCHEDULED_DOWNTIME	1
#define SERVICE_NO_SCHEDULED_DOWNTIME	2
#define SERVICE_STATE_ACKNOWLEDGED	4
#define SERVICE_STATE_UNACKNOWLEDGED	8
#define SERVICE_CHECKS_DISABLED		16
#define SERVICE_CHECKS_ENABLED		32
#define SERVICE_EVENT_HANDLER_DISABLED	64
#define SERVICE_EVENT_HANDLER_ENABLED	128
#define SERVICE_FLAP_DETECTION_ENABLED	256
#define SERVICE_FLAP_DETECTION_DISABLED	512
#define SERVICE_IS_FLAPPING		1024
#define SERVICE_IS_NOT_FLAPPING		2048
#define SERVICE_NOTIFICATIONS_DISABLED	4096
#define SERVICE_NOTIFICATIONS_ENABLED	8192
#define SERVICE_PASSIVE_CHECKS_DISABLED	16384
#define SERVICE_PASSIVE_CHECKS_ENABLED	32768
#define SERVICE_MODIFIED_ATTRIBUTES	65536
#define SERVICE_NO_MODIFIED_ATTRIBUTES	131072
#define SERVICE_HARD_STATE		262144
#define SERVICE_SOFT_STATE		524288
#define SERVICE_STATE_HANDLED		1048576
#define SERVICE_NOT_ALL_CHECKS_DISABLED	2097152


/****************************** SSI TYPES  ********************************/

#define SSI_HEADER			0
#define SSI_FOOTER			1


/************************** TAC TITLES ****************************/

#define TAC_TITLE_HOST_NOT_DISABLED			"Hosts Not Disabled"
#define TAC_TITLE_HOST_DISABLED				"Hosts Disabled"
#define TAC_TITLE_HOST_UNACK_HOSTS			"Unacknowledged Hosts"
#define TAC_TITLE_HOST_ACK_HOSTS			"Acknowledged Hosts"
#define TAC_TITLE_HOST_NON_URGENT			"Handled Hosts"
#define TAC_TITLE_HOST_PROBLEM_ALL			"All Problem Hosts"
#define TAC_TITLE_HOST_ALL				"All Hosts"

#define TAC_TITLE_SVC_NOT_DISABLED			"Services Not Disabled"
#define TAC_TITLE_SVC_DISABLED				"Services Disabled"
#define TAC_TITLE_SVC_UNACK_SERVICES			"Unacknowledged Services"
#define TAC_TITLE_SVC_ACK_SERVICES			"Acknowledged Services"
#define TAC_TITLE_SVC_NON_URGENT			"Handled Services"
#define TAC_TITLE_SVC_PROBLEM_ALL			"All Problem Services"
#define TAC_TITLE_SVC_ALL				"All Services"


/************************** HTTP CHARSET ****************************/

#define DEFAULT_HTTP_CHARSET "utf-8"

/************************** JSON OUTPUT VERSION ************************/

#define JSON_OUTPUT_VERSION "1.11.0"


/************************** BUFFER  ***************************************/

#define MAX_MESSAGE_BUFFER              4096

#define MAX_CGI_INPUT_PAIRS		1000		/**< max number of cgi vars excepted */

/************************** DISPLAY STYLE  ********************************/

#define DISPLAY_NONE                    -1
#define DISPLAY_HOSTS                   0
#define DISPLAY_HOSTGROUPS              1
#define DISPLAY_SERVICEGROUPS           2
#define DISPLAY_CONTACTS                3
#define DISPLAY_CONTACTGROUPS           4
#define DISPLAY_SERVICES                5
#define DISPLAY_TIMEPERIODS             6
#define DISPLAY_COMMANDS                7
#define DISPLAY_HOSTGROUPESCALATIONS    8    /* no longer implemented */
#define DISPLAY_SERVICEDEPENDENCIES     9
#define DISPLAY_SERVICEESCALATIONS      10
#define DISPLAY_HOSTDEPENDENCIES        11
#define DISPLAY_HOSTESCALATIONS         12
#define DISPLAY_ALL			13
#define DISPLAY_MODULES			14
#define DISPLAY_CGICONFIG		15
#define DISPLAY_COMMAND_EXPANSION       16211

#define STYLE_OVERVIEW                  0
#define STYLE_DETAIL                    1
#define STYLE_SUMMARY                   2
#define STYLE_GRID                      3
#define STYLE_HOST_DETAIL               4

/************************** HISTORY  ************************************/

#define SERVICE_HISTORY                 0
#define HOST_HISTORY                    1
#define SERVICE_FLAPPING_HISTORY        2
#define HOST_FLAPPING_HISTORY           3
#define SERVICE_DOWNTIME_HISTORY        4
#define HOST_DOWNTIME_HISTORY           5

/************************** STATE  **************************************/

#define STATE_ALL                       0
#define STATE_SOFT                      1
#define STATE_HARD                      2

/********************* standard report times ****************************/

#define TIMEPERIOD_CUSTOM	0
#define TIMEPERIOD_TODAY	1
#define TIMEPERIOD_YESTERDAY	2
#define TIMEPERIOD_THISWEEK	3
#define TIMEPERIOD_LASTWEEK	4
#define TIMEPERIOD_THISMONTH	5
#define TIMEPERIOD_LASTMONTH	6
#define TIMEPERIOD_THISQUARTER	7
#define TIMEPERIOD_LASTQUARTER	8
#define TIMEPERIOD_THISYEAR	9
#define TIMEPERIOD_LASTYEAR	10
#define TIMEPERIOD_LAST24HOURS	11
#define TIMEPERIOD_LAST7DAYS	12
#define TIMEPERIOD_LAST31DAYS	13
#define TIMEPERIOD_SINGLE_DAY	14
#define TIMEPERIOD_NEXTPROBLEM	15

/********************* CHILD HOST DISPLAY *********************************/

#define SHOW_CHILD_HOSTS_NONE		0
#define SHOW_CHILD_HOSTS_IMMEDIATE	1
#define SHOW_CHILD_HOSTS_ALL		2

/******************************** FUNCTIONS *******************************/

int check_exclude_customvar(customvariablesmember *);

void reset_cgi_vars(void);
void free_memory(void);

char * get_cgi_config_location(void);				/* gets location of the CGI config file to read */
char * get_cmd_file_location(void);				/* gets location of external command file to write to */

int read_cgi_config_file(char *);
int read_main_config_file(char *);
int read_icinga_resource_file(char *);
int read_all_object_configuration_data(char *,int);
int read_all_status_data(char *,int);

char *unescape_newlines(char *);
char *escape_newlines(char *);
void sanitize_plugin_output(char *);				/* strips HTML and bad characters from plugin output */
void strip_html_brackets(char *);				/* strips > and < from string */

void get_time_string(time_t *,char *,int,int);			/* gets a date/time string */
void get_interval_time_string(double,char *,int);		/* gets a time string for an interval of time */

char * url_encode(char *);					/* encodes a string in proper URL format */
char * html_encode(char *,int);					/* encodes a string in HTML format (for what the user sees) */
char * escape_string(char *);					/* escape string for html form usage */

void print_extra_hostgroup_url(char *,char *);
void print_extra_servicegroup_url(char *,char *);

void display_info_table(char *,authdata *, int);

void display_splunk_host_url(host *);
void display_splunk_service_url(service *);
void display_splunk_generic_url(char *,int);
void strip_splunk_query_terms(char *);

void include_ssi_files(char *,int);				/* include user-defined SSI footers/headers */
void include_ssi_file(char *);					/* include user-defined SSI footer/header */

void cgi_config_file_error(char *,int);
void main_config_file_error(char *,int);
void object_data_error(int);
void status_data_error(int);

/** @brief prints main errors depending on error_type
 *  @param [in] config_file name otherwise set to NULL
 *  @param [in] error_type to set the correct error message
 *  @param [in] tac_header TRUE if tac_header otherwise FALSE
**/
void print_error(char*, int, int);

void document_header(int,int, char *);				/* print document header */
void document_footer(int);					/* print document footer */

void write_popup_code(int);					/* PopUp's for graphics */
int check_daemon_running(void);

void print_generic_error_message(char *, char *, int);

void print_export_link(int, char *, char *);			/* print csv, json, xml and html export link */

int write_to_cgi_log(char *);
int rotate_cgi_log_file(void);
int my_rename(char *,char *);					/* renames a file - works across filesystems */
int my_fcopy(char *,char *);					/* copies a file - works across filesystems */
int my_fdcopy(char *, char *, int);				/* copies a named source to an already opened destination file */

void convert_timeperiod_to_times(int, time_t *, time_t *);	/* converts time period to start and end unix timestamps */
int string_to_time(char *, time_t *);				/* converts a defined formated string to unix timestamp */

int is_dlst_time(time_t *);
char *json_encode(char *);

/** @brief print's a little comment icon in status lists
 *  @param [in] host_name of host to display comments
 *  @param [in] svc_description if comment's for service is requested
 *
 *  This function print's a little comment icon and generates html code
 *  to display a tooltip box which pops up on mouse over.
**/
void print_comment_icon(char *, char *);

/** @brief prints modified attributes as string, by line seperator
 *  @param [in] content_type can be \c CSV_CONTENT , \c JSON_CONTENT , \c XML_CONTENT or \c HTML_CONTENT
 *  @param [in] cgi name of cgi as defined in include/cgiutils.h
 *  @param [in] modified_attributes is the number to compare with
 *  @note takes care that modified_attributes is represented as string
 *
 *  This function prints modified_attributes as string
**/
void print_modified_attributes(int , char *, unsigned long);

/** @brief Display's the page number selector
 *  @param [in] result_start the result start for the current displayed list
 *  @param [in] total_entries number of total entries available to display
 *  @param [in] displayed_entries number of actually displayed entries
 *
 *  Display's the page number selector and gernerates all links to select next/previouse page. Also copy's selector to top of the page
**/
void page_num_selector(int result_start, int total_entries, int displayed_entries);

/** @brief Display's the page limit selector
 *  @param [in] result_start needed to keep track from which result num the user want's to change the amount of displayed entries.
 *
 *  Display's the page limit selector which overwrites the config value
**/
void page_limit_selector(int result_start);

/** @brief Display's navigation through log files based on timestamps
 *  @param [in] ts_start start timestamp to display and calculate next/previous pages from
 *  @param [in] ts_end end timestamp to display and calculate next/previous pages from
 *
 *  Display's the log file navigation which is used in history, notifications and showlog.
 *  Generates url's/link's to next and previous pages.
**/
void display_nav_table(time_t ts_start, time_t ts_end);

/******************************** MULTIURL PATCH *******************************/

#ifndef DISABLE_MULTIURL

#define MU_PATCH_ID	"+MU"

int MU_lasturl, MU_thisurl;
char MU_iconstr[16], *MU_origstr, *MU_ptr;

/* Have process_macros() generate processed_string *BEFORE* starting the loop */

#define	BEGIN_MULTIURL_LOOP										\
	/* Init counters */	MU_lasturl=0; MU_iconstr[0]='\0';					\
	/* MAIN LOOP */		for (MU_origstr=MU_ptr=processed_string; (*MU_ptr)!='\0'; ) {		\
		/* Internal init */	MU_thisurl=MU_lasturl;						\
		/* Skip whitespace */	for (;isspace(*MU_ptr);MU_ptr++) ;				\
		/* Detect+skip ap. */	for (;(*MU_ptr)=='\'';MU_ptr++) MU_thisurl=MU_lasturl+1;	\
		/* Ap. found? */	if (MU_thisurl>MU_lasturl) {					\
			/* yes->split str */	sprintf(MU_iconstr,"%u-",MU_thisurl);			\
						processed_string=MU_ptr;				\
						for (;((*MU_ptr)!='\0')&&((*MU_ptr)!='\'');MU_ptr++) ;	\
						if ((*MU_ptr)=='\'') { (*MU_ptr)='\0'; MU_ptr++;	\
							for (;isspace(*MU_ptr);MU_ptr++) ; }		\
					} else {							\
			/* no->end loop */	MU_iconstr[0]='\0'; MU_ptr="";				\
					}

/* Do the original printf()s, additionally inserting MU_iconstr between icon path and icon (file)name */

#define	END_MULTIURL_LOOP										\
		/* Int -> ext ctr */	MU_lasturl=MU_thisurl; processed_string=MU_ptr;			\
	/* MAIN LOOP */		}									\
	/* Hide evidence */	processed_string=MU_origstr;

/* Do the free(processed_string) *AFTER* ending the loop */

#else /* ndef DISABLE_MULTIURL */

#define MU_PATCH_ID	""
char *MU_iconstr="";

#endif /* ndef DISABLE_MULTIURL */



#ifdef __cplusplus
}
#endif

#endif

