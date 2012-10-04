/*****************************************************************************
 *
 * SHOWLOG.C - Icinga Log File CGI
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 * License:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *****************************************************************************/

/** @file showlog.c
 *  @brief cgi to browse through Icinga log data
**/


#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"

#include "../include/getcgi.h"
#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/readlogs.h"

/** @name External vars
    @{ **/
extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];

extern char *csv_delimiter;
extern char *csv_data_enclosure;

extern int enable_splunk_integration;
extern int showlog_initial_states;
extern int showlog_current_states;
extern int escape_html_tags;
extern int result_limit;

extern int embedded;
extern int display_header;
extern int daemon_check;
extern int content_type;
/** @} */

/** @name Internal vars
    @{ **/
int display_frills = TRUE;			/**< determine if icons should be shown in listing */
int display_timebreaks = TRUE;			/**< determine if time breaks should be shown */
int reverse = FALSE;				/**< determine if log should be viewed in reverse order */
int timeperiod_type = TIMEPERIOD_SINGLE_DAY;	/**< determines the time period to view see cgiutils.h */
int num_displayed = -1;				/**< holds amount of displayed log entries */
int result_start = 1;				/**< keep track from where we have to start displaying results */
int get_result_limit = -1;			/**< needed to overwrite config value with result_limit we get vie GET */

int display_filter = FALSE;			/**< show filter */
int show_notifications = TRUE;			/**< filter option */
int show_host_status = TRUE;			/**< filter option */
int show_service_status = TRUE;			/**< filter option */
int show_external_commands = TRUE;		/**< filter option */
int show_system_messages = TRUE;		/**< filter option */
int show_event_handler = TRUE;			/**< filter option */
int show_flapping = TRUE;			/**< filter option */
int show_downtime = TRUE;			/**< filter option */

char *query_string = NULL;			/**< the request query string */
char *start_time_string = "";			/**< the requested start time */
char *end_time_string = "";			/**< the requested end time */

time_t ts_start = 0L;				/**< start time as unix timestamp */
time_t ts_end = 0L;				/**< end time as unix timestamp */

authdata current_authdata;			/**< struct to hold current authentication data */

int CGI_ID = SHOWLOG_CGI_ID;			/**< ID to identify the cgi for functions in cgiutils.c */
/** @} */

/** @brief Parses the requested GET/POST variables
 *  @return wether parsing was successful or not
 *	@retval TRUE
 *	@retval FALSE
 *
 *  @n This function parses the request and set's the necessary variables
**/
int process_cgivars(void);

/** @brief displays the requested log entries
 *
 * Applies the requested filters, reads in all necessary log files
 * and afterwards showing each log entry.
**/
void display_logentries(void);

/** @brief displays the little filter list in the top right corner
 *
 * Just to show the filter in an own function. Get's called in \c main
**/
void show_filter(void);

/** @brief Yes we need a main function **/
int main(void) {
	int result = OK;

	/* get the CGI variables passed in the URL */
	process_cgivars();

	/* reset internal variables */
	reset_cgi_vars();

	/* read the CGI configuration file */
	result = read_cgi_config_file(get_cgi_config_location());
	if (result == ERROR) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(get_cgi_config_location(), ERROR_CGI_CFG_FILE);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read the main configuration file */
	result = read_main_config_file(main_config_file);
	if (result == ERROR) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(main_config_file, ERROR_CGI_MAIN_CFG);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read all object configuration data */
	result = read_all_object_configuration_data(main_config_file, READ_ALL_OBJECT_DATA);
	if (result == ERROR) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(NULL, ERROR_CGI_OBJECT_DATA);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* This requires the date_format parameter in the main config file */
	if (timeperiod_type == TIMEPERIOD_CUSTOM) {
		if (strcmp(start_time_string, ""))
			string_to_time(start_time_string, &ts_start);

		if (strcmp(end_time_string, ""))
			string_to_time(end_time_string, &ts_end);
	}

	/* overwrite config value with amount we got via GET */
	result_limit = (get_result_limit != -1) ? get_result_limit : result_limit;

	/* for json and csv output return all by default */
	if (get_result_limit == -1 && (content_type == JSON_CONTENT || content_type == CSV_CONTENT))
		result_limit = 0;

	document_header(CGI_ID, TRUE, "Log File");

	/* calculate timestamps for reading logs */
	convert_timeperiod_to_times(timeperiod_type, &ts_start, &ts_end);

	/* get authentication information */
	get_authentication_information(&current_authdata);

	if (display_header == TRUE) {

		/* start input form */
		printf("<form method='GET' style='margin:0;' action='%s'>\n", SHOWLOG_CGI);
		printf("<input type='hidden' name='ts_start' value='%lu'>\n", ts_start);
		printf("<input type='hidden' name='ts_end' value='%lu'>\n", ts_end);
		printf("<input type='hidden' name='limit' value='%d'>\n", result_limit);

		/* begin top table */
		printf("<table border=0 width=100%% cellpadding=0 cellspacing=0>\n");
		printf("<tr>\n");

		/* left column of top table - info box */
		printf("<td align=left valign=top width=33%%>\n");
		display_info_table("Event Log", &current_authdata, daemon_check);
		printf("</td>\n");

		/* middle column of top table - log file navigation options */
		printf("<td align=center valign=top width=33%%>\n");
		display_nav_table(ts_start, ts_end);
		printf("</td>\n");

		/* right hand column of top row */
		printf("<td align=right valign=top width=33%%>\n");

		/* show filter */
		printf("<table border=0 cellspacing=0 cellpadding=0 CLASS='optBox' align=right><tr><td>\n");
		show_filter();
		printf("</td></tr>\n");

		printf("</table>\n");

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");

		printf("</form>\n");
	}

	/* check to see if the user is authorized to view the log file */
	if (is_authorized_for_system_information(&current_authdata) == FALSE) {
		print_generic_error_message("It appears as though you do not have permission to view the log file...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		return ERROR;
	}

	/* display the contents of the log file */
	display_logentries();

	document_footer(CGI_ID);

	/* free allocated memory */
	free_memory();

	return OK;
}

int process_cgivars(void) {
	char **variables;
	int error = FALSE;
	int x;

	variables = getcgivars();

	for (x = 0; variables[x] != NULL; x++) {

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if (strlen(variables[x]) >= MAX_INPUT_BUFFER - 1)
			continue;

		/* found query string */
		else if (!strcmp(variables[x], "query_string")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			query_string = strdup(variables[x]);
			strip_html_brackets(query_string);

			if (strlen(query_string) == 0)
				my_free(query_string);
		}

		/* we found first time argument */
		else if (!strcmp(variables[x], "ts_start")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			ts_start = (time_t)strtoul(variables[x], NULL, 10);
		}

		/* we found last time argument */
		else if (!strcmp(variables[x], "ts_end")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			ts_end = (time_t)strtoul(variables[x], NULL, 10);
		}

		/* we found the start time */
		else if (!strcmp(variables[x], "start_time")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			start_time_string = (char *)malloc(strlen(variables[x]) + 1);
			if (start_time_string == NULL)
				start_time_string = "";
			else
				strcpy(start_time_string, variables[x]);
		}

		/* we found the end time */
		else if (!strcmp(variables[x], "end_time")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			end_time_string = (char *)malloc(strlen(variables[x]) + 1);
			if (end_time_string == NULL)
				end_time_string = "";
			else
				strcpy(end_time_string, variables[x]);
		}

		/* we found the standard timeperiod argument */
		else if (!strcmp(variables[x], "timeperiod")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "today"))
				timeperiod_type = TIMEPERIOD_TODAY;
			else if (!strcmp(variables[x], "singelday"))
				timeperiod_type = TIMEPERIOD_SINGLE_DAY;
			else if (!strcmp(variables[x], "last24hours"))
				timeperiod_type = TIMEPERIOD_LAST24HOURS;
			else if (!strcmp(variables[x], "thisweek"))
				timeperiod_type = TIMEPERIOD_THISWEEK;
			else if (!strcmp(variables[x], "lastweek"))
				timeperiod_type = TIMEPERIOD_LASTWEEK;
			else if (!strcmp(variables[x], "thismonth"))
				timeperiod_type = TIMEPERIOD_THISMONTH;
			else if (!strcmp(variables[x], "lastmonth"))
				timeperiod_type = TIMEPERIOD_LASTMONTH;
			else if (!strcmp(variables[x], "thisyear"))
				timeperiod_type = TIMEPERIOD_THISYEAR;
			else if (!strcmp(variables[x], "lastyear"))
				timeperiod_type = TIMEPERIOD_LASTYEAR;
			else if (!strcmp(variables[x], "last7days"))
				timeperiod_type = TIMEPERIOD_LAST7DAYS;
			else if (!strcmp(variables[x], "last31days"))
				timeperiod_type = TIMEPERIOD_LAST31DAYS;
			else if (!strcmp(variables[x], "custom"))
				timeperiod_type = TIMEPERIOD_CUSTOM;
			else
				continue;

			convert_timeperiod_to_times(timeperiod_type, &ts_start, &ts_end);
		}

		/* we found the order argument */
		else if (!strcmp(variables[x], "order")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "new2old"))
				reverse = FALSE;
			else if (!strcmp(variables[x], "old2new"))
				reverse = TRUE;
		}

		/* show filter */
		else if (!strcmp(variables[x], "display_filter")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "true"))
				display_filter = TRUE;
		}

		/* notification filter */
		else if (!strcmp(variables[x], "noti")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "off"))
				show_notifications = FALSE;
		}

		/* host status filter */
		else if (!strcmp(variables[x], "hst")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "off"))
				show_host_status = FALSE;
		}

		/* service status filter */
		else if (!strcmp(variables[x], "sst")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "off"))
				show_service_status = FALSE;
		}

		/* external commands filter */
		else if (!strcmp(variables[x], "cmd")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "off"))
				show_external_commands = FALSE;
		}

		/* system messages filter */
		else if (!strcmp(variables[x], "sms")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "off"))
				show_system_messages = FALSE;
		}

		/* event handler filter */
		else if (!strcmp(variables[x], "evh")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "off"))
				show_event_handler = FALSE;
		}

		/* flapping filter */
		else if (!strcmp(variables[x], "flp")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "off"))
				show_flapping = FALSE;
		}

		/* downtime filter */
		else if (!strcmp(variables[x], "dwn")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "off"))
				show_downtime = FALSE;
		}

		/* we found the CSV output option */
		else if (!strcmp(variables[x], "csvoutput")) {
			display_header = FALSE;
			content_type = CSV_CONTENT;
		}

		/* we found the CSV output option */
		else if (!strcmp(variables[x], "jsonoutput")) {
			display_header = FALSE;
			content_type = JSON_CONTENT;
		}

		/* we found the embed option */
		else if (!strcmp(variables[x], "embedded"))
			embedded = TRUE;

		/* we found the noheader option */
		else if (!strcmp(variables[x], "noheader"))
			display_header = FALSE;

		/* we found the nofrills option */
		else if (!strcmp(variables[x], "nofrills"))
			display_frills = FALSE;

		/* we found the notimebreaks option */
		else if (!strcmp(variables[x], "notimebreaks"))
			display_timebreaks = FALSE;

		/* we found the nodaemoncheck option */
		else if (!strcmp(variables[x], "nodaemoncheck"))
			daemon_check = FALSE;


		/* start num results to skip on displaying statusdata */
		else if (!strcmp(variables[x], "start")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			result_start = atoi(variables[x]);

			if (result_start < 1)
				result_start = 1;
		}

		/* amount of results to display */
		else if (!strcmp(variables[x], "limit")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			get_result_limit = atoi(variables[x]);
		}

		/* we received an invalid argument */
		else
			error = TRUE;

	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

void display_logentries() {
	char image[MAX_INPUT_BUFFER];
	char image_alt[MAX_INPUT_BUFFER];
	char last_message_date[MAX_INPUT_BUFFER] = "";
	char current_message_date[MAX_INPUT_BUFFER] = "";
	char date_time[MAX_DATETIME_LENGTH];
	char *error_text = NULL;
	int status = READLOG_OK;
	int i = 0;
	int user_has_seen_something = FALSE;
	int json_start = TRUE;
	int total_entries = 0;
	int displayed_entries = 0;
	struct tm *time_ptr = NULL;
	logentry *entry_list = NULL;
	logentry *temp_entry = NULL;
	logfilter *filter_list = NULL;


	/* Add default filters */
	if (showlog_initial_states == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_SERVICE_INITIAL_STATE, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_INITIAL_STATE, LOGFILTER_EXCLUDE);
	}
	if (showlog_current_states == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_SERVICE_CURRENT_STATE, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_CURRENT_STATE, LOGFILTER_EXCLUDE);
	}

	/* Add requested filters */
	if (show_notifications == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_HOST_NOTIFICATION, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SERVICE_NOTIFICATION, LOGFILTER_EXCLUDE);
	}
	if (show_host_status == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_HOST_UP, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_DOWN, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_UNREACHABLE, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_RECOVERY, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_PASSIVE_HOST_CHECK, LOGFILTER_EXCLUDE);
	}
	if (show_service_status == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_SERVICE_OK, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SERVICE_WARNING, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SERVICE_CRITICAL, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SERVICE_UNKNOWN, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SERVICE_RECOVERY, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_PASSIVE_SERVICE_CHECK, LOGFILTER_EXCLUDE);
	}
	if (show_external_commands == FALSE)
		add_log_filter(&filter_list, LOGENTRY_EXTERNAL_COMMAND, LOGFILTER_EXCLUDE);

	if (show_system_messages == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_SYSTEM_WARNING, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_STARTUP, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SHUTDOWN, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_BAILOUT, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_RESTART, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_LOG_ROTATION, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_AUTOSAVE, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_IDOMOD, LOGFILTER_EXCLUDE);
	}
	if (show_event_handler == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_SERVICE_EVENT_HANDLER, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_EVENT_HANDLER, LOGFILTER_EXCLUDE);
	}
	if (show_flapping == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_SERVICE_FLAPPING_STARTED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SERVICE_FLAPPING_STOPPED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SERVICE_FLAPPING_DISABLED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_FLAPPING_STARTED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_FLAPPING_STOPPED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_FLAPPING_DISABLED, LOGFILTER_EXCLUDE);
	}
	if (show_downtime == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_SERVICE_DOWNTIME_STARTED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SERVICE_DOWNTIME_STOPPED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SERVICE_DOWNTIME_CANCELLED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_DOWNTIME_STARTED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_DOWNTIME_STOPPED, LOGFILTER_EXCLUDE);
		add_log_filter(&filter_list, LOGENTRY_HOST_DOWNTIME_CANCELLED, LOGFILTER_EXCLUDE);
	}

	/* scan the log file for archived state data */
	status = get_log_entries(&entry_list, &filter_list, &error_text, query_string, reverse, ts_start, ts_end);

	free_log_filters(&filter_list);


	/* dealing with errors */
	if (status == READLOG_ERROR_WARNING) {
		if (error_text != NULL) {
			print_generic_error_message(error_text, NULL, 0);
			my_free(error_text);
		} else
			print_generic_error_message("Unkown error!", NULL, 0);
	}

	if (status == READLOG_ERROR_MEMORY)
			print_generic_error_message("Out of memory...", "showing all I could get!", 0);


	if (status == READLOG_ERROR_FATAL) {
		if (error_text != NULL) {
			print_generic_error_message(error_text, NULL, 0);
			my_free(error_text);
		}
		user_has_seen_something = TRUE;

	/* now we start displaying the log entries */
	} else {

		if (content_type == JSON_CONTENT) {
			display_timebreaks = FALSE;
			if (status != READLOG_OK)
				printf(",\n");
			printf("\"log_entries\": [\n");
		} else if (content_type == CSV_CONTENT) {
			display_timebreaks = FALSE;

			printf("%sTimestamp%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sDate Time%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sLog Entry%s\n", csv_data_enclosure, csv_data_enclosure);

		} else {
			/* add export to csv, json, link */
			printf("<table width='100%%' cellspacing=0 cellpadding=0 border=0><tr><td width='33%%'></td><td width='33%%' align=center nowrap>");
			printf("<div class='page_selector' id='log_page_selector'>\n");
			printf("<div id='page_navigation_copy'></div>");
			page_limit_selector(result_start);
			printf("</div>\n");
			printf("</td><td width='33%%' align='right'>\n");
			printf("<div class='csv_export_link' style='margin-right:1em;'>");
			print_export_link(CSV_CONTENT, SHOWLOG_CGI, NULL);
			print_export_link(JSON_CONTENT, SHOWLOG_CGI, NULL);
			print_export_link(HTML_CONTENT, SHOWLOG_CGI, NULL);
			printf("</div></td></tr></table>");
			printf("</div>\n");

			printf("<DIV CLASS='logEntries'>\n");
		}

		for (temp_entry = entry_list; temp_entry != NULL; temp_entry = temp_entry->next) {

			if (result_limit != 0  && (((total_entries + 1) < result_start) || (total_entries >= ((result_start + result_limit) - 1)))) {
				total_entries++;
				continue;
			}

			total_entries++;
			displayed_entries++;

			/* set the correct icon and icon alt text for current log entry */
			if (temp_entry->type == LOGENTRY_STARTUP) {
				strcpy(image, START_ICON);
				strcpy(image_alt, START_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_SHUTDOWN || temp_entry->type == LOGENTRY_BAILOUT) {
				strcpy(image, STOP_ICON);
				strcpy(image_alt, STOP_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_RESTART) {
				strcpy(image, RESTART_ICON);
				strcpy(image_alt, RESTART_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_HOST_DOWN) {
				strcpy(image, HOST_DOWN_ICON);
				strcpy(image_alt, HOST_DOWN_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_HOST_UNREACHABLE) {
				strcpy(image, HOST_UNREACHABLE_ICON);
				strcpy(image_alt, HOST_UNREACHABLE_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_HOST_RECOVERY || temp_entry->type == LOGENTRY_HOST_UP) {
				strcpy(image, HOST_UP_ICON);
				strcpy(image_alt, HOST_UP_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_HOST_NOTIFICATION) {
				strcpy(image, HOST_NOTIFICATION_ICON);
				strcpy(image_alt, HOST_NOTIFICATION_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_SERVICE_CRITICAL) {
				strcpy(image, CRITICAL_ICON);
				strcpy(image_alt, CRITICAL_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_SERVICE_WARNING) {
				strcpy(image, WARNING_ICON);
				strcpy(image_alt, WARNING_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_SERVICE_UNKNOWN) {
				strcpy(image, UNKNOWN_ICON);
				strcpy(image_alt, UNKNOWN_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_SERVICE_RECOVERY || temp_entry->type == LOGENTRY_SERVICE_OK) {
				strcpy(image, OK_ICON);
				strcpy(image_alt, OK_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION) {
				strcpy(image, NOTIFICATION_ICON);
				strcpy(image_alt, NOTIFICATION_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_SERVICE_EVENT_HANDLER) {
				strcpy(image, SERVICE_EVENT_ICON);
				strcpy(image_alt, SERVICE_EVENT_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_HOST_EVENT_HANDLER) {
				strcpy(image, HOST_EVENT_ICON);
				strcpy(image_alt, HOST_EVENT_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_EXTERNAL_COMMAND) {
				strcpy(image, EXTERNAL_COMMAND_ICON);
				strcpy(image_alt, EXTERNAL_COMMAND_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_PASSIVE_SERVICE_CHECK) {
				strcpy(image, PASSIVE_ICON);
				strcpy(image_alt, "Passive Service Check");
			} else if (temp_entry->type == LOGENTRY_PASSIVE_HOST_CHECK) {
				strcpy(image, PASSIVE_ICON);
				strcpy(image_alt, "Passive Host Check");
			} else if (temp_entry->type == LOGENTRY_LOG_ROTATION) {
				strcpy(image, LOG_ROTATION_ICON);
				strcpy(image_alt, LOG_ROTATION_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_ACTIVE_MODE) {
				strcpy(image, ACTIVE_ICON);
				strcpy(image_alt, ACTIVE_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_STANDBY_MODE) {
				strcpy(image, STANDBY_ICON);
				strcpy(image_alt, STANDBY_ICON_ALT);
			} else if (temp_entry->type == LOGENTRY_SERVICE_FLAPPING_STARTED) {
				strcpy(image, FLAPPING_ICON);
				strcpy(image_alt, "Service started flapping");
			} else if (temp_entry->type == LOGENTRY_SERVICE_FLAPPING_STOPPED) {
				strcpy(image, FLAPPING_ICON);
				strcpy(image_alt, "Service stopped flapping");
			} else if (temp_entry->type == LOGENTRY_SERVICE_FLAPPING_DISABLED) {
				strcpy(image, FLAPPING_ICON);
				strcpy(image_alt, "Service flap detection disabled");
			} else if (temp_entry->type == LOGENTRY_HOST_FLAPPING_STARTED) {
				strcpy(image, FLAPPING_ICON);
				strcpy(image_alt, "Host started flapping");
			} else if (temp_entry->type == LOGENTRY_HOST_FLAPPING_STOPPED) {
				strcpy(image, FLAPPING_ICON);
				strcpy(image_alt, "Host stopped flapping");
			} else if (temp_entry->type == LOGENTRY_HOST_FLAPPING_DISABLED) {
				strcpy(image, FLAPPING_ICON);
				strcpy(image_alt, "Host flap detection disabled");
			} else if (temp_entry->type == LOGENTRY_SERVICE_DOWNTIME_STARTED) {
				strcpy(image, DOWNTIME_ICON);
				strcpy(image_alt, "Service entered a period of scheduled downtime");
			} else if (temp_entry->type == LOGENTRY_SERVICE_DOWNTIME_STOPPED) {
				strcpy(image, DOWNTIME_ICON);
				strcpy(image_alt, "Service exited a period of scheduled downtime");
			} else if (temp_entry->type == LOGENTRY_SERVICE_DOWNTIME_CANCELLED) {
				strcpy(image, DOWNTIME_ICON);
				strcpy(image_alt, "Service scheduled downtime has been cancelled");
			} else if (temp_entry->type == LOGENTRY_HOST_DOWNTIME_STARTED) {
				strcpy(image, DOWNTIME_ICON);
				strcpy(image_alt, "Host entered a period of scheduled downtime");
			} else if (temp_entry->type == LOGENTRY_HOST_DOWNTIME_STOPPED) {
				strcpy(image, DOWNTIME_ICON);
				strcpy(image_alt, "Host exited a period of scheduled downtime");
			} else if (temp_entry->type == LOGENTRY_HOST_DOWNTIME_CANCELLED) {
				strcpy(image, DOWNTIME_ICON);
				strcpy(image_alt, "Host scheduled downtime has been cancelled");
			} else if (temp_entry->type == LOGENTRY_IDOMOD) {
				strcpy(image, DATABASE_ICON);
				strcpy(image_alt, "IDOMOD Information");
			} else if (temp_entry->type == LOGENTRY_NPCDMOD) {
				strcpy(image, STATS_ICON);
				strcpy(image_alt, "NPCDMOD Information");
			} else if (temp_entry->type == LOGENTRY_AUTOSAVE) {
				strcpy(image, AUTOSAVE_ICON);
				strcpy(image_alt, "Auto-save retention data");
			} else if (temp_entry->type == LOGENTRY_SYSTEM_WARNING) {
				strcpy(image, DAEMON_WARNING_ICON);
				strcpy(image_alt, "Icinga warning message");
			} else {
				strcpy(image, INFO_ICON);
				strcpy(image_alt, INFO_ICON_ALT);
			}

			time_ptr = localtime(&temp_entry->timestamp);
			strftime(current_message_date, sizeof(current_message_date), "%B %d, %Y %H:00", time_ptr);
			current_message_date[sizeof(current_message_date) - 1] = '\x0';

			if (strcmp(last_message_date, current_message_date) != 0 && display_timebreaks == TRUE) {
				printf("</DIV>\n");
				printf("<BR>\n");
				printf("<DIV>\n");
				printf("<table border=0 width=99%% CLASS='dateTimeBreak' align=center><tr>");
				printf("<td width=40%%><hr width=100%%></td>");
				printf("<td align=center CLASS='dateTimeBreak'>%s</td>", current_message_date);
				printf("<td width=40%%><hr width=100%%></td>");
				printf("</tr></table>\n");
				printf("</DIV>\n");
				printf("<BR><DIV CLASS='logEntries'>\n");
				strncpy(last_message_date, current_message_date, sizeof(last_message_date));
				last_message_date[sizeof(last_message_date) - 1] = '\x0';
			}

			get_time_string(&temp_entry->timestamp, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			strip(date_time);

			/* preparing logentries for json and csv output */
			if (content_type == CSV_CONTENT || content_type == JSON_CONTENT) {
				for (i = 0; i < strlen(temp_entry->entry_text) - 1; i++)
					*(temp_entry->entry_text + i) = *(temp_entry->entry_text + i + 1);
				temp_entry->entry_text[strlen(temp_entry->entry_text) - 1] = '\x0';
			}

			/* displays log entry depending on requested content type */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;
				printf("{ \"timestamp\": %lu, ", temp_entry->timestamp);
				printf(" \"date_time\": \"%s\", ", date_time);
				printf(" \"log_entry\": \"%s\"}", json_encode(temp_entry->entry_text));
			} else if (content_type == CSV_CONTENT) {
				printf("%s%lu%s%s", csv_data_enclosure, temp_entry->timestamp, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s\n", csv_data_enclosure, temp_entry->entry_text, csv_data_enclosure);
			} else {
				if (display_frills == TRUE)
					printf("<img align=left src='%s%s' alt='%s' title='%s'>", url_images_path, image, image_alt, image_alt);
				printf("[%s] %s", date_time, (temp_entry->entry_text == NULL) ? "" : html_encode(temp_entry->entry_text, FALSE));
				if (enable_splunk_integration == TRUE) {
					printf("&nbsp;&nbsp;&nbsp;");
					display_splunk_generic_url(temp_entry->entry_text, 2);
				}
				printf("<br clear=all>\n");
			}

			user_has_seen_something = TRUE;
		}

		if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
			if (user_has_seen_something == TRUE) {
				printf("</DIV><hr>\n");
				page_num_selector(result_start, total_entries, displayed_entries);
			} else {
				printf("<script type='text/javascript'>document.getElementById('log_page_selector').style.display='none';</script>");
			}
		} else if (content_type == JSON_CONTENT)
			printf("\n]\n");
	}

	free_log_entries(&entry_list);

	if (user_has_seen_something == FALSE && content_type != CSV_CONTENT && content_type != JSON_CONTENT)
		printf("<DIV CLASS='warningMessage'>No log entries found!</DIV>");

	return;
}

void show_filter(void) {
	char buffer[MAX_INPUT_BUFFER];
	int temp_htmlencode = escape_html_tags;

	// escape all characters, otherwise they won't show up in search box
	escape_html_tags = TRUE;

	printf("<table border=0 cellspacing=0 cellpadding=0 width='100%%'>\n");
	printf("<tr><td valign=top align=right style='padding-right:21.5em;'>Filters:&nbsp;&nbsp;");
	printf("<img id='expand_image' src='%s%s' border=0 onClick=\"if (document.getElementById('filters').style.display == 'none') { document.getElementById('display_filter').value = 'true'; document.getElementById('filters').style.display = ''; document.getElementById('expand_image').src = '%s%s'; } else { document.getElementById('display_filter').value = 'false'; document.getElementById('filters').style.display = 'none'; document.getElementById('expand_image').src = '%s%s'; }\">", url_images_path, (display_filter == TRUE) ? COLLAPSE_ICON : EXPAND_ICON, url_images_path, COLLAPSE_ICON, url_images_path, EXPAND_ICON);
	printf("<input type='hidden' name='display_filter' id='display_filter' value='true'>\n");
	printf("</td></tr></table>");

	printf("<table id='filters' border=0 cellspacing=2 cellpadding=2 style='margin-bottom:0.5em;display:%s;'>\n", (display_filter == TRUE) ? "" : "none");

	/* search box */
	printf("<tr><td align=right width='10%%'>Search:</td>");
	printf("<td nowrap><input type='text' name='query_string' id='query_string' size='15' class='NavBarSearchItem' value='%s'>", (query_string == NULL) ? "" : html_encode(query_string, TRUE));
	printf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type='button' value='Clear' onClick=\"document.getElementById('query_string').value = '';\"></td></tr>");

	/* Order */
	printf("<tr><td align=right>Order:</td>");
	printf("<td nowrap><input type=radio name='order' value='new2old' %s> Newer Entries First&nbsp;&nbsp;| <input type=radio name='order' value='old2new' %s> Older Entries First</td></tr>", (reverse == TRUE) ? "" : "checked", (reverse == TRUE) ? "checked" : "");

	/* Timeperiod */
	printf("<tr><td align=left>Timeperiod:</td>");
	printf("<td align=left>");

	printf("<select id='selecttp' name='timeperiod' onChange=\"var i=document.getElementById('selecttp').selectedIndex; if (document.getElementById('selecttp').options[i].value == 'custom') { document.getElementById('custtime').style.display = ''; } else { document.getElementById('custtime').style.display = 'none';}\">\n");
	printf("<option value=singleday %s>Single Day\n", (timeperiod_type == TIMEPERIOD_SINGLE_DAY) ? "selected" : "");
	printf("<option value=today %s>Today\n", (timeperiod_type == TIMEPERIOD_TODAY) ? "selected" : "");
	printf("<option value=last24hours %s>Last 24 Hours\n", (timeperiod_type == TIMEPERIOD_LAST24HOURS) ? "selected" : "");
	printf("<option value=thisweek %s>This Week\n", (timeperiod_type == TIMEPERIOD_THISWEEK) ? "selected" : "");
	printf("<option value=last7days %s>Last 7 Days\n", (timeperiod_type == TIMEPERIOD_LAST7DAYS) ? "selected" : "");
	printf("<option value=lastweek %s>Last Week\n", (timeperiod_type == TIMEPERIOD_LASTWEEK) ? "selected" : "");
	printf("<option value=thismonth %s>This Month\n", (timeperiod_type == TIMEPERIOD_THISMONTH) ? "selected" : "");
	printf("<option value=last31days %s>Last 31 Days\n", (timeperiod_type == TIMEPERIOD_LAST31DAYS) ? "selected" : "");
	printf("<option value=lastmonth %s>Last Month\n", (timeperiod_type == TIMEPERIOD_LASTMONTH) ? "selected" : "");
	printf("<option value=thisyear %s>This Year\n", (timeperiod_type == TIMEPERIOD_THISYEAR) ? "selected" : "");
	printf("<option value=lastyear %s>Last Year\n", (timeperiod_type == TIMEPERIOD_LASTYEAR) ? "selected" : "");
	printf("<option value=custom %s>* CUSTOM PERIOD *\n", (timeperiod_type == TIMEPERIOD_CUSTOM) ? "selected" : "");
	printf("</select>\n");
	printf("<div id='custtime' style='display:%s;'>", (timeperiod_type == TIMEPERIOD_CUSTOM) ? "" : "none");

	printf("<br><table border=0 cellspacing=0 cellpadding=0>\n");
	get_time_string(&ts_start, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);
	printf("<tr><td>Start:&nbsp;&nbsp;</td><td><INPUT TYPE='TEXT' class='timepicker' NAME='start_time' VALUE='%s' SIZE=\"25\"></td></tr>", buffer);

	get_time_string(&ts_end, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);
	printf("<tr><td>End:&nbsp;&nbsp;</td><td><INPUT TYPE='TEXT' class='timepicker' NAME='end_time' VALUE='%s' SIZE=\"25\"></td></tr></table></div>", buffer);

	printf("</td></tr>\n");

	/* Filter Entry types */
	printf("<tr><td>Entry Type:</td><td>\n");
	printf("<table border=0 cellspacing=0 cellpadding=0>\n");
	printf("<tr><td align=center>on</td><td align=center>off</td><td>Type</td></tr>\n");
	printf("<tr><td><input type=radio name='noti' value=on %s></td><td><input type=radio name='noti' value=off %s></td><td>Notifications</td></tr>\n", (show_notifications == TRUE) ? "checked" : "", (show_notifications == FALSE) ? "checked" : "");
	printf("<tr><td><input type=radio name='hst' value=on %s></td><td><input type=radio name='hst' value=off %s></td><td>Host Status</td></tr>\n", (show_host_status == TRUE) ? "checked" : "", (show_host_status == FALSE) ? "checked" : "");
	printf("<tr><td><input type=radio name='sst' value=on %s></td><td><input type=radio name='sst' value=off %s></td><td>Service Status</td></tr>\n", (show_service_status == TRUE) ? "checked" : "", (show_service_status == FALSE) ? "checked" : "");
	printf("<tr><td><input type=radio name='cmd' value=on %s></td><td><input type=radio name='cmd' value=off %s></td><td>External Commands</td></tr>\n", (show_external_commands == TRUE) ? "checked" : "", (show_external_commands == FALSE) ? "checked" : "");
	printf("<tr><td><input type=radio name='sms' value=on %s></td><td><input type=radio name='sms' value=off %s></td><td>System Messages</td></tr>\n", (show_system_messages == TRUE) ? "checked" : "", (show_system_messages == FALSE) ? "checked" : "");
	printf("<tr><td><input type=radio name='evh' value=on %s></td><td><input type=radio name='evh' value=off %s></td><td>Event Handler</td></tr>\n", (show_event_handler == TRUE) ? "checked" : "", (show_event_handler == FALSE) ? "checked" : "");
	printf("<tr><td><input type=radio name='flp' value=on %s></td><td><input type=radio name='flp' value=off %s></td><td>Flapping</td></tr>\n", (show_flapping == TRUE) ? "checked" : "", (show_flapping == FALSE) ? "checked" : "");
	printf("<tr><td><input type=radio name='dwn' value=on %s></td><td><input type=radio name='dwn' value=off %s></td><td>Downtime</td></tr>\n", (show_downtime == TRUE) ? "checked" : "", (show_downtime == FALSE) ? "checked" : "");

	printf("</table>\n");
	printf("</td></tr>\n");

	/* submit Button */
	printf("<tr><td><input type='submit' value='Apply'></td><td align=right><input type='reset' value='Reset' onClick=\"window.location.href='%s?order=new2old&timeperiod=singleday&limit=%d'\">&nbsp;</td></tr>\n", SHOWLOG_CGI, result_limit);

	printf("</table>\n");

	escape_html_tags = temp_htmlencode;
	return;
}
