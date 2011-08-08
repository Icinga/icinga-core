/************************************************************************
 *
 * CGIUTILS.H - Header file for common CGI functions
 *
 * Copyright (c) 1999-2008  Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org) 
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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


/**************************** CGI REFRESH RATE ******************************/

#define DEFAULT_REFRESH_RATE	60	/* 60 second refresh rate for CGIs */


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
#define STATUSWML_CGI		"statuswml.cgi"
#define STATUSWRL_CGI		"statuswrl.cgi"
#define SUMMARY_CGI		"summary.cgi"
#define TAC_CGI			"tac.cgi"
#define TRENDS_CGI		"trends.cgi"

/* Are these ones still in use??? */
#define TRACEROUTE_CGI		"traceroute.cgi"
#define CHECKSANITY_CGI		"checksanity.cgi"
#define MINISTATUS_CGI		"ministatus.cgi"


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
#define STATUSWML_CGI_ID	12
#define STATUSWRL_CGI_ID	13
#define SUMMARY_CGI_ID		14
#define TAC_CGI_ID		15
#define TRENDS_CGI_ID		16

/* Are these ones still in use??? */
#define TRACEROUTE_CGI_ID	17
#define CHECKSANITY_CGI_ID	18
#define MINISTATUS_CGI_ID	19

/* for error msg */
#define ERROR_CGI_ID	20


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

/* Are these ones still in use??? */
#define CHECKSANITY_CSS		"checksanity.css"
#define MINISTATUS_CSS		"ministatus.css"


/**************************** JAVASCRIPT NAMES ******************************/

#define CHECKBOX_FUNCTIONS_JS   "checkbox_functions.js"
#define JQUERY_MAIN_JS		"jquery-1.6.2.min.js"
#define JQUERY_DD_JS		"jquery.dd.js"
#define SKINNYTIP_JS		"skinnytip.js"
#define HIGHLIGHT_TABLE_JS	"htable.js"
#define HIGHLIGHT_TABLE_JS_CODE	" onMouseOver=\"javascript:trackTableHighlight(event, '#BBC3BB');\" onMouseOut=\"javascript:highlightTableRow(0);\""

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

#define STATUS_DETAIL_ICON		"status2.gif"
#define STATUS_OVERVIEW_ICON		"status.gif"
#define STATUSMAP_ICON			"status3.gif"
#define STATUSWORLD_ICON		"status4.gif"
#define EXTINFO_ICON			"extinfo.gif"
#define HISTORY_ICON			"history.gif"
#define CONTACTGROUP_ICON		"contactgroup.gif"
#define TRENDS_ICON			"trends.gif"
#define COLLAPSE_ICON			"icon_collapse.gif"
#define EXPAND_ICON			"icon_expand.gif"

#define DISABLED_ICON			"disabled.gif"
#define ENABLED_ICON			"enabled.gif"
#define PASSIVE_ONLY_ICON		"passiveonly.gif"
#define NOTIFICATIONS_DISABLED_ICON	"ndisabled.gif"
#define ACKNOWLEDGEMENT_ICON		"ack.gif"
#define REMOVE_ACKNOWLEDGEMENT_ICON	"noack.gif"
#define COMMENT_ICON			"comment.gif"
#define DELETE_ICON			"delete.gif"
#define DELAY_ICON			"delay.gif"
#define DOWNTIME_ICON			"downtime.gif"
#define PASSIVE_ICON			"passiveonly.gif"
#define RIGHT_ARROW_ICON		"right.gif"
#define LEFT_ARROW_ICON			"left.gif"
#define UP_ARROW_ICON			"up.gif"
#define DOWN_ARROW_ICON			"down.gif"
#define FLAPPING_ICON			"flapping.gif"
#define SCHEDULED_DOWNTIME_ICON		"downtime.gif"
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

#define CONFIGURATION_ICON		"config.gif"
#define NOTES_ICON			"notes.gif"
#define ACTION_ICON			"action.gif"
#define DETAIL_ICON			"detail.gif"

#define PARENT_TRAVERSAL_ICON		"parentup.gif"

#define TAC_DISABLED_ICON		"tacdisabled.png"
#define TAC_ENABLED_ICON		"tacenabled.png"

#define ZOOM1_ICON			"zoom1.gif"
#define ZOOM2_ICON			"zoom2.gif"

#define CONTEXT_HELP_ICON1		"contexthelp1.gif"
#define CONTEXT_HELP_ICON2		"contexthelp2.gif"

#define SPLUNK_SMALL_WHITE_ICON		"splunk1.gif"
#define SPLUNK_SMALL_BLACK_ICON		"splunk2.gif"

#define DATABASE_ICON			"database.gif"
#define AUTOSAVE_ICON			"save.gif"
#define DAEMON_WARNING_ICON		"warning_triangle.gif"
#define STATS_ICON			"stats.gif"

#define TAC_HEADER_DEFAULT_LOGO		"Icinga_Header_Webinterface.jpg"
#define TAC_HEADER_DEFAULT_LOGO_ALT	"Icinga"
#define TAC_HEADER_LOGO			"Icinga_TAC_Header_Webinterface.jpg"
#define TAC_HEADER_HOST_ICON		"server.png"
#define TAC_HEADER_SERVICE_ICON		"application-monitor.png"
#define TAC_HEADER_EXECUTION_ICON	"hourglass-exclamation.png"
#define TAC_HEADER_LATENCY_ICON		"hourglass-arrow.png"

/* icons taken from http://findicons.com */
#define EXPORT_CSV_ICON			"export_csv.png"
#define EXPORT_CSV_ICON_ALT		"Export to CSV"
#define EXPORT_JSON_ICON		"export_json.png"
#define EXPORT_JSON_ICON_ALT		"Export to JSON"
#define EXPORT_XML_ICON			"export_xml.png"
#define EXPORT_XML_ICON_ALT		"Export to XML"
#define EXPORT_LINK_ICON		"export_link.png"
#define EXPORT_LINK_ICON_ALT		"Link to this page"

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
#define WML_CONTENT		1
#define IMAGE_CONTENT		2
#define CSV_CONTENT		3
#define JSON_CONTENT		4
#define XML_CONTENT		5


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

/****************** HOST AND SERVICE FILTER PROPERTIES  *******************/

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
#define HOST_PASSIVE_CHECK		65536
#define HOST_ACTIVE_CHECK		131072
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
#define SERVICE_PASSIVE_CHECK		65536
#define SERVICE_ACTIVE_CHECK		131072
#define SERVICE_HARD_STATE		262144
#define SERVICE_SOFT_STATE		524288
#define SERVICE_STATE_HANDLED		1048576
#define SERVICE_NOT_ALL_CHECKS_DISABLED	2097152


/****************************** SSI TYPES  ********************************/

#define SSI_HEADER			0
#define SSI_FOOTER			1


/************************ CONTEXT-SENSITIVE HELP  *************************/

#define CONTEXTHELP_STATUS_DETAIL	"A1"
#define CONTEXTHELP_STATUS_HGOVERVIEW	"A2"
#define CONTEXTHELP_STATUS_HGSUMMARY	"A3"
#define CONTEXTHELP_STATUS_HGGRID	"A4"
#define CONTEXTHELP_STATUS_SVCPROBLEMS	"A5"
#define CONTEXTHELP_STATUS_HOST_DETAIL	"A6"
#define CONTEXTHELP_STATUS_HOSTPROBLEMS	"A7"
#define CONTEXTHELP_STATUS_SGOVERVIEW	"A8"
#define CONTEXTHELP_STATUS_SGSUMMARY	"A9"
#define CONTEXTHELP_STATUS_SGGRID	"A10"

#define CONTEXTHELP_TAC			"B1"

#define CONTEXTHELP_MAP			"C1"

#define CONTEXTHELP_LOG			"D1"

#define CONTEXTHELP_HISTORY		"E1"

#define CONTEXTHELP_NOTIFICATIONS	"F1"

#define CONTEXTHELP_TRENDS_MENU1	"G1"
#define CONTEXTHELP_TRENDS_MENU2	"G2"
#define CONTEXTHELP_TRENDS_MENU3	"G3"
#define CONTEXTHELP_TRENDS_MENU4	"G4"
#define CONTEXTHELP_TRENDS_HOST		"G5"
#define CONTEXTHELP_TRENDS_SERVICE	"G6"

#define CONTEXTHELP_AVAIL_MENU1		"H1"
#define CONTEXTHELP_AVAIL_MENU2		"H2"
#define CONTEXTHELP_AVAIL_MENU3		"H3"
#define CONTEXTHELP_AVAIL_MENU4		"H4"
#define CONTEXTHELP_AVAIL_MENU5		"H5"
#define CONTEXTHELP_AVAIL_HOSTGROUP	"H6"
#define CONTEXTHELP_AVAIL_HOST		"H7"
#define CONTEXTHELP_AVAIL_SERVICE	"H8"
#define CONTEXTHELP_AVAIL_SERVICEGROUP	"H9"

#define CONTEXTHELP_EXT_HOST		"I1"
#define CONTEXTHELP_EXT_SERVICE		"I2"
#define CONTEXTHELP_EXT_HOSTGROUP	"I3"
#define CONTEXTHELP_EXT_PROCESS		"I4"
#define CONTEXTHELP_EXT_PERFORMANCE	"I5"
#define CONTEXTHELP_EXT_COMMENTS	"I6"
#define CONTEXTHELP_EXT_DOWNTIME	"I7"
#define CONTEXTHELP_EXT_QUEUE		"I8"
#define CONTEXTHELP_EXT_SERVICEGROUP	"I9"

#define CONTEXTHELP_CMD_INPUT		"J1"
#define CONTEXTHELP_CMD_COMMIT		"J2"

#define CONTEXTHELP_OUTAGES		"K1"

#define CONTEXTHELP_CONFIG_MENU			"L1"
#define CONTEXTHELP_CONFIG_HOSTS		"L2"
#define CONTEXTHELP_CONFIG_HOSTDEPENDENCIES	"L3"
#define CONTEXTHELP_CONFIG_HOSTESCALATIONS	"L4"
#define CONTEXTHELP_CONFIG_HOSTGROUPS		"L5"
#define CONTEXTHELP_CONFIG_HOSTGROUPESCALATIONS	"L6"
#define CONTEXTHELP_CONFIG_SERVICES		"L7"
#define CONTEXTHELP_CONFIG_SERVICEDEPENDENCIES	"L8"
#define CONTEXTHELP_CONFIG_SERVICEESCALATIONS	"L9"
#define CONTEXTHELP_CONFIG_CONTACTS		"L10"
#define CONTEXTHELP_CONFIG_CONTACTGROUPS	"L11"
#define CONTEXTHELP_CONFIG_TIMEPERIODS		"L12"
#define CONTEXTHELP_CONFIG_COMMANDS		"L13"
#define CONTEXTHELP_CONFIG_HOSTEXTINFO		"L14"
#define CONTEXTHELP_CONFIG_SERVICEEXTINFO	"L15"
#define CONTEXTHELP_CONFIG_SERVICEGROUPS	"L16"

#define CONTEXTHELP_HISTOGRAM_MENU1	"M1"
#define CONTEXTHELP_HISTOGRAM_MENU2	"M2"
#define CONTEXTHELP_HISTOGRAM_MENU3	"M3"
#define CONTEXTHELP_HISTOGRAM_MENU4	"M4"
#define CONTEXTHELP_HISTOGRAM_HOST	"M5"
#define CONTEXTHELP_HISTOGRAM_SERVICE	"M6"

#define CONTEXTHELP_SUMMARY_MENU			"N1"
#define CONTEXTHELP_SUMMARY_RECENT_ALERTS		"N2"
#define CONTEXTHELP_SUMMARY_ALERT_TOTALS		"N3"
#define CONTEXTHELP_SUMMARY_HOSTGROUP_ALERT_TOTALS	"N4"
#define CONTEXTHELP_SUMMARY_HOST_ALERT_TOTALS		"N5"
#define CONTEXTHELP_SUMMARY_SERVICE_ALERT_TOTALS	"N6"
#define CONTEXTHELP_SUMMARY_ALERT_PRODUCERS		"N7"
#define CONTEXTHELP_SUMMARY_SERVICEGROUP_ALERT_TOTALS	"N8"

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

#define JSON_OUTPUT_VERSION "1.5.0"


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

/******************************** FUNCTIONS *******************************/

void reset_cgi_vars(void);
void free_memory(void);

char * get_cgi_config_location(void);				/* gets location of the CGI config file to read */
char * get_cmd_file_location(void);				/* gets location of external command file to write to */

int read_cgi_config_file(char *);
int read_main_config_file(char *);
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

void display_info_table(char *,int,authdata *, int);
void display_nav_table(char *,int);

void display_splunk_host_url(host *);
void display_splunk_service_url(service *);
void display_splunk_generic_url(char *,int);
void strip_splunk_query_terms(char *);

void include_ssi_files(char *,int);				/* include user-defined SSI footers/headers */
void include_ssi_file(char *);					/* include user-defined SSI footer/header */

void cgi_config_file_error(char *);
void main_config_file_error(char *);
void object_data_error(void);
void status_data_error(void);
void print_error(char*, int);

void display_context_help(char *);				/* displays context-sensitive help window */

void document_header(int,int);					/* print document header */
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

