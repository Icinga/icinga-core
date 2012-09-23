/*****************************************************************************
 *
 * NOTIFICATIONS.C - Icinga Notifications CGI
 *
 * Copyright (c) 1999-2008 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 * This CGI program will display the notification events for
 * a given host or contact or for all contacts/hosts.
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
 *****************************************************************************/

/** @file notifications.c
 *  @brief cgi to browse through Icinga notification history
**/


#include "../include/config.h"
#include "../include/common.h"
#include "../include/getcgi.h"
#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/readlogs.h"

/** @name External vars
    @{ **/
extern char 	*csv_delimiter;
extern char 	*csv_data_enclosure;

extern char 	main_config_file[MAX_FILENAME_LENGTH];

extern int 	embedded;
extern int 	display_header;
extern int 	daemon_check;
extern int 	content_type;
extern int	result_limit;
extern int	show_partial_hostgroups;
/** @} */

/** @name Internal vars
    @{ **/
int query_type = DISPLAY_HOSTS;			/**< holds requested notifications type  */
int find_all = TRUE;				/**< display all or just one requested host / contact */
int notification_options = NOTIFICATION_ALL;	/**< determine type of notifications */
int reverse = FALSE;				/**< determine if log should be viewed in reverse order */
int timeperiod_type = TIMEPERIOD_SINGLE_DAY;	/**< determines the time period to view see cgiutils.h */
int result_start = 1;				/**< keep track from where we have to start displaying results */
int get_result_limit = -1;			/**< needed to overwrite config value with result_limit we get vie GET */

char *query_contact_name = "";			/**< the requested contact */
char *query_host_name = "";			/**< the requested host name */
char *query_svc_description = "";		/**< the requested service */
char *query_hostgroup_name = "";		/**< the requested host group */
char *query_servicegroup_name = "";		/**< the requested service group */
char *start_time_string = "";			/**< the requested start time */
char *end_time_string = "";			/**< the requested end time */

time_t ts_start = 0L;				/**< start time as unix timestamp */
time_t ts_end = 0L;				/**< end time as unix timestamp */

authdata current_authdata;			/**< struct to hold current authentication data */

int CGI_ID = NOTIFICATIONS_CGI_ID;		/**< ID to identify the cgi for functions in cgiutils.c */
/** @} */

/** @brief displays the requested notification entries
 *
 * Applies the requested filters, reads in all necessary log files
 * and afterwards showing each matching notification log entry.
**/
void display_notifications(void);

/** @brief Parses the requested GET/POST variables
 *  @return wether parsing was successful or not
 *	@retval TRUE
 *	@retval FALSE
 *
 *  @n This function parses the request and set's the necessary variables
**/
int process_cgivars(void);

/** @brief Yes we need a main function **/
int main(void) {
	char buffer[MAX_DATETIME_LENGTH];
	int result = OK;

	/* get the arguments passed in the URL */
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

	document_header(CGI_ID, TRUE, "Alert Notifications");

	/* get authentication information */
	get_authentication_information(&current_authdata);

	/* calculate timestamps for reading logs */
	convert_timeperiod_to_times(timeperiod_type, &ts_start, &ts_end);

	if (display_header == TRUE) {

		/* begin top table */
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of top row */
		printf("<td align=left valign=top width=33%%>\n");

		if (query_type == DISPLAY_SERVICES)
			display_info_table("Service Notifications", &current_authdata, daemon_check);
		else if (query_type == DISPLAY_HOSTGROUPS)
			display_info_table("Hostgroup Notifications", &current_authdata, daemon_check);
		else if (query_type == DISPLAY_SERVICEGROUPS)
			display_info_table("Servicegroup Notifications", &current_authdata, daemon_check);
		else if (query_type == DISPLAY_HOSTS) {
			if (find_all == TRUE)
				display_info_table("Notifications", &current_authdata, daemon_check);
			else
				display_info_table("Host Notifications", &current_authdata, daemon_check);
		} else
			display_info_table("Contact Notifications", &current_authdata, daemon_check);

		if (query_type == DISPLAY_HOSTS || query_type == DISPLAY_SERVICES || query_type == DISPLAY_HOSTGROUPS || query_type == DISPLAY_SERVICEGROUPS) {
			printf("<TABLE BORDER=1 CELLPADDING=0 CELLSPACING=0 CLASS='linkBox'>\n");
			printf("<TR><TD CLASS='linkBox'>\n");
			if (query_type == DISPLAY_HOSTS) {
				printf("<a href='%s?host=%s'>View <b>Status Detail</b> For <b>%s</b></a><br>\n", STATUS_CGI, (find_all == TRUE) ? "all" : url_encode(query_host_name), (find_all == TRUE) ? "All Hosts" : "This Host");
				printf("<a href='%s?host=%s'>View <b>Alert History</b> For <b>%s</b></a><br>\n", HISTORY_CGI, (find_all == TRUE) ? "all" : url_encode(query_host_name), (find_all == TRUE) ? "All Hosts" : "This Host");
#ifdef USE_TRENDS
				if (find_all == FALSE)
					printf("<a href='%s?host=%s'>View <b>Trends</b> For <b>This Host</b></a><br>\n", TRENDS_CGI, url_encode(query_host_name));
#endif
				printf("<a href='%s?type=%d&host=%s'>View <b>Information</b> For <b>This Host</b></a><br>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(query_host_name));
				printf("<a href='%s?host=%s&show_log_entries'>View <b>Availability Report</b> For <b>This Host</b></a><br>\n", AVAIL_CGI, url_encode(query_host_name));
			} else if (query_type == DISPLAY_SERVICES) {
				printf("<a href='%s?host=%s&", HISTORY_CGI, (find_all == TRUE) ? "all" : url_encode(query_host_name));
				printf("service=%s'>View <b>Alert History</b> For <b>This Service</b></a><br>\n", url_encode(query_svc_description));
#ifdef USE_TRENDS
				printf("<a href='%s?host=%s&", TRENDS_CGI, (find_all == TRUE) ? "all" : url_encode(query_host_name));
				printf("service=%s'>View <b>Trends</b> For <b>This Service</b></a><br>\n", url_encode(query_svc_description));
#endif
				printf("<a href='%s?type=%d&host=%s&service=%s'>View <b>Information</b> For <b>This Service</b></a><br>\n", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(query_host_name), url_encode(query_svc_description));
				printf("<a href='%s?host=%s&service=%s&show_log_entries'>View <b>Availability Report</b> For <b>This Service</b></a><br>\n", AVAIL_CGI, url_encode(query_host_name), url_encode(query_svc_description));
				printf("<a href='%s?host=%s'>View <b>Notifications</b> For <b>This Host</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(query_host_name));
			} else if (query_type == DISPLAY_HOSTGROUPS) {
				printf("<a href='%s?hostgroup=%s&style=hostdetail'>View <b>Host Status Detail</b> For <b>This Hostgroup</b></a><br>\n", STATUS_CGI, url_encode(query_hostgroup_name));
				printf("<a href='%s?hostgroup=%s&style=detail'>View <b>Service Status Detail</b> For <b>This Hostgroup</b></a><br>\n", STATUS_CGI, url_encode(query_hostgroup_name));
				printf("<a href='%s?hostgroup=%s'>View <b>Alert History</b> For <b>This Hostgroup</b></a><br>\n", HISTORY_CGI, url_encode(query_hostgroup_name));
			} else if (query_type == DISPLAY_SERVICEGROUPS) {
				printf("<a href='%s?servicegroup=%s&style=hostdetail'>View <b>Host Status Detail</b> For <b>This Servicegroup</b></a><br>\n", STATUS_CGI, url_encode(query_servicegroup_name));
				printf("<a href='%s?servicegroup=%s&style=detail'>View <b>Service Status Detail</b> For <b>This Servicegroup</b></a><br>\n", STATUS_CGI, url_encode(query_servicegroup_name));
				printf("<a href='%s?servicegroup=%s'>View <b>Alert History</b> For <b>This Servicegroup</b></a><br>\n", HISTORY_CGI, url_encode(query_servicegroup_name));
			}
			printf("</TD></TR>\n");
			printf("</TABLE>\n");
		}

		printf("</td>\n");

		/* middle column of top row */
		printf("<td align=center valign=top width=33%%>\n");

		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>\n");
		if (query_type == DISPLAY_SERVICES)
			printf("Service '%s' on Host '%s'", query_svc_description, query_host_name);
		else if (query_type == DISPLAY_HOSTS) {
			if (find_all == TRUE)
				printf("All Hosts and Services");
			else
				printf("Host '%s'", html_encode(query_host_name, TRUE));
		} else if (query_type == DISPLAY_HOSTGROUPS) {
			printf("Host Group '%s'", html_encode(query_hostgroup_name, TRUE));
		} else if (query_type == DISPLAY_SERVICEGROUPS) {
			printf("Service Group '%s'", html_encode(query_servicegroup_name, TRUE));
		} else {
			if (find_all == TRUE)
				printf("All Contacts");
			else
				printf("Contact '%s'", html_encode(query_contact_name, TRUE));
		}
		printf("</DIV>\n");
		printf("<BR>\n");

		display_nav_table(ts_start, ts_end);

		printf("</td>\n");

		/* right hand column of top row */
		printf("<td align=right valign=top width=33%%>\n");

		printf("<form method='GET' action='%s'>\n", NOTIFICATIONS_CGI);
		if (query_type == DISPLAY_SERVICES) {
			printf("<input type='hidden' name='host' value='%s'>\n", escape_string(query_host_name));
			printf("<input type='hidden' name='service' value='%s'>\n", escape_string(query_svc_description));
		} else if (query_type == DISPLAY_HOSTGROUPS) {
			printf("<input type='hidden' name='hostgroup' value='%s'>\n", escape_string(query_hostgroup_name));
		} else if (query_type == DISPLAY_SERVICEGROUPS) {
			printf("<input type='hidden' name='servicegroup' value='%s'>\n", escape_string(query_servicegroup_name));
		} else
			printf("<input type='hidden' name='%s' value='%s'>\n", (query_type == DISPLAY_HOSTS) ? "host" : "contact", (query_type == DISPLAY_HOSTS) ? escape_string(query_host_name) : escape_string(query_contact_name));
		printf("<input type='hidden' name='ts_start' value='%lu'>\n", ts_start);
		printf("<input type='hidden' name='ts_end' value='%lu'>\n", ts_end);
		printf("<input type='hidden' name='limit' value='%d'>\n", result_limit);

		printf("<table border=0 CLASS='optBox'>\n");
		printf("<tr>\n");
		if (query_type == DISPLAY_SERVICES)
			printf("<td align=left colspan=2 CLASS='optBoxItem'>Notification detail level for this service:</td>");
		if (query_type == DISPLAY_HOSTGROUPS || query_type == DISPLAY_SERVICEGROUPS)
			printf("<td align=left colspan=2 CLASS='optBoxItem'>Notification detail level for this %sgroup:</td>", (query_type == DISPLAY_HOSTGROUPS) ? "host" : "service");
		else
			printf("<td align=left colspan=2 CLASS='optBoxItem'>Notification detail level for %s %s%s:</td>", (find_all == TRUE) ? "all" : "this", (query_type == DISPLAY_HOSTS) ? "host" : "contact", (find_all == TRUE) ? "s" : "");
		printf("</tr>\n");
		printf("<tr><td></td>\n");
		printf("<td align=left CLASS='optBoxItem'><select name='type'>\n");
		printf("<option value=%d %s>All notifications\n", NOTIFICATION_ALL, (notification_options == NOTIFICATION_ALL) ? "selected" : "");
		if (query_type != DISPLAY_SERVICES) {
			printf("<option value=%d %s>All service notifications\n", NOTIFICATION_SERVICE_ALL, (notification_options == NOTIFICATION_SERVICE_ALL) ? "selected" : "");
			printf("<option value=%d %s>All host notifications\n", NOTIFICATION_HOST_ALL, (notification_options == NOTIFICATION_HOST_ALL) ? "selected" : "");
		}
		printf("<option value=%d %s>Service custom\n", NOTIFICATION_SERVICE_CUSTOM, (notification_options == NOTIFICATION_SERVICE_CUSTOM) ? "selected" : "");
		printf("<option value=%d %s>Service acknowledgements\n", NOTIFICATION_SERVICE_ACK, (notification_options == NOTIFICATION_SERVICE_ACK) ? "selected" : "");
		printf("<option value=%d %s>Service warning\n", NOTIFICATION_SERVICE_WARNING, (notification_options == NOTIFICATION_SERVICE_WARNING) ? "selected" : "");
		printf("<option value=%d %s>Service unknown\n", NOTIFICATION_SERVICE_UNKNOWN, (notification_options == NOTIFICATION_SERVICE_UNKNOWN) ? "selected" : "");
		printf("<option value=%d %s>Service critical\n", NOTIFICATION_SERVICE_CRITICAL, (notification_options == NOTIFICATION_SERVICE_CRITICAL) ? "selected" : "");
		printf("<option value=%d %s>Service recovery\n", NOTIFICATION_SERVICE_RECOVERY, (notification_options == NOTIFICATION_SERVICE_RECOVERY) ? "selected" : "");
		printf("<option value=%d %s>Service flapping\n", NOTIFICATION_SERVICE_FLAP, (notification_options == NOTIFICATION_SERVICE_FLAP) ? "selected" : "");
		if (query_type != DISPLAY_SERVICES) {
			printf("<option value=%d %s>Host custom\n", NOTIFICATION_HOST_CUSTOM, (notification_options == NOTIFICATION_HOST_CUSTOM) ? "selected" : "");
			printf("<option value=%d %s>Host acknowledgements\n", NOTIFICATION_HOST_ACK, (notification_options == NOTIFICATION_HOST_ACK) ? "selected" : "");
			printf("<option value=%d %s>Host down\n", NOTIFICATION_HOST_DOWN, (notification_options == NOTIFICATION_HOST_DOWN) ? "selected" : "");
			printf("<option value=%d %s>Host unreachable\n", NOTIFICATION_HOST_UNREACHABLE, (notification_options == NOTIFICATION_HOST_UNREACHABLE) ? "selected" : "");
			printf("<option value=%d %s>Host recovery\n", NOTIFICATION_HOST_RECOVERY, (notification_options == NOTIFICATION_HOST_RECOVERY) ? "selected" : "");
			printf("<option value=%d %s>Host flapping\n", NOTIFICATION_HOST_FLAP, (notification_options == NOTIFICATION_HOST_FLAP) ? "selected" : "");
		}
		printf("</select></td>\n");
		printf("</tr>\n");

		/* Order */
		printf("<tr><td align=right>Order:</td>");
		printf("<td nowrap><input type=radio name='order' value='new2old' %s> Newer Entries First&nbsp;&nbsp;| <input type=radio name='order' value='old2new' %s> Older Entries First</td></tr>\n", (reverse == TRUE) ? "" : "checked", (reverse == TRUE) ? "checked" : "");

		/* Timeperiod */
		printf("<tr><td align=left>Timeperiod:</td>");
		printf("<td align=left>\n");

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

		/* submit Button */
		printf("<tr><td><input type='submit' value='Update'></td><td align=right><input type='reset' value='Reset' onClick=\"window.location.href='%s?order=new2old&timeperiod=singleday&limit=%d'\">&nbsp;</td></tr>\n", NOTIFICATIONS_CGI, result_limit);

		printf("</table>\n");
		printf("</form>\n");

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");
	}

	/* display notifications */
	display_notifications();

	/* display footer */
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
		if (strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
			x++;
			continue;
		}

		/* we found the host argument */
		else if (!strcmp(variables[x], "host")) {
			query_type = DISPLAY_HOSTS;
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if ((query_host_name = strdup(variables[x])) == NULL)
				query_host_name = "";
			strip_html_brackets(query_host_name);

			if (!strcmp(query_host_name, "all"))
				find_all = TRUE;
			else
				find_all = FALSE;
		}

		/* we found the contact argument */
		else if (!strcmp(variables[x], "contact")) {
			query_type = DISPLAY_CONTACTS;
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if ((query_contact_name = strdup(variables[x])) == NULL)
				query_contact_name = "";
			strip_html_brackets(query_contact_name);

			if (!strcmp(query_contact_name, "all"))
				find_all = TRUE;
			else
				find_all = FALSE;
		}

		/* we found the service argument */
		else if (!strcmp(variables[x], "service")) {
			query_type = DISPLAY_SERVICES;
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}
			if ((query_svc_description = strdup(variables[x])) == NULL)
				query_svc_description = "";
			strip_html_brackets(query_svc_description);
		}

		/* we found the hostgroup argument */
		else if (!strcmp(variables[x], "hostgroup")) {
			query_type = DISPLAY_HOSTGROUPS;
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}
			if ((query_hostgroup_name = strdup(variables[x])) == NULL)
				query_hostgroup_name = "";
			strip_html_brackets(query_hostgroup_name);
		}

		/* we found the servicegroup argument */
		else if (!strcmp(variables[x], "servicegroup")) {
			query_type = DISPLAY_SERVICEGROUPS;
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}
			if ((query_servicegroup_name = strdup(variables[x])) == NULL)
				query_servicegroup_name = "";
			strip_html_brackets(query_servicegroup_name);
		}

		/* we found the notification type argument */
		else if (!strcmp(variables[x], "type")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			notification_options = atoi(variables[x]);
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

		/* we found the CSV output option */
		else if (!strcmp(variables[x], "csvoutput")) {
			display_header = FALSE;
			content_type = CSV_CONTENT;
		}

		/* we found the JSON output option */
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
	}

	/*
	 * Set some default values if not already set.
	 * Done here as they won't be set if variable
	 * not provided via cgi parameters
	 * Only required for hosts & contacts, not services
	 * as there is no service_name=all option
	 */
	if (query_type == DISPLAY_HOSTS && strlen(query_host_name) == 0) {
		query_host_name = "all";
		find_all = TRUE;
	}
	if (query_type == DISPLAY_CONTACTS && strlen(query_contact_name) == 0) {
		query_contact_name = "all";
		find_all = TRUE;
	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

void display_notifications(void) {
	char *temp_buffer;
	char *error_text = NULL;
	char date_time[MAX_DATETIME_LENGTH];
	char alert_level[MAX_INPUT_BUFFER];
	char alert_level_class[MAX_INPUT_BUFFER];
	char contact_name[MAX_INPUT_BUFFER];
	char service_name[MAX_INPUT_BUFFER];
	char host_name[MAX_INPUT_BUFFER];
	char method_name[MAX_INPUT_BUFFER];
	char displayed_host_name[MAX_INPUT_BUFFER];
	char displayed_service_desc[MAX_INPUT_BUFFER];
	int show_entry;
	int total_notifications = 0;
	int displayed_entries = 0;
	int notification_detail_type = NOTIFICATION_SERVICE_CRITICAL;
	int status = READLOG_OK;
	int odd = 0;
	int json_start = TRUE;
	host *temp_host = NULL;
	service *temp_service = NULL;
	hostgroup *temp_hostgroup = NULL;
	servicegroup *temp_servicegroup = NULL;
	logentry *temp_entry = NULL;
	logentry *entry_list = NULL;
	logfilter *filter_list = NULL;

	if (query_type == DISPLAY_HOSTGROUPS) {

		temp_hostgroup = find_hostgroup(query_hostgroup_name);

		if (temp_hostgroup == NULL) {
			print_generic_error_message("There are no host groups with this name defined.", NULL, 0);
			return;
		}
		/* make sure the user is authorized to view this hostgroup */
		if (show_partial_hostgroups == FALSE && is_authorized_for_hostgroup(temp_hostgroup, &current_authdata) == FALSE) {
			print_generic_error_message("It appears as though you do not have permission to view information for the host group you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
			return;
		}
	}

	if (query_type == DISPLAY_SERVICEGROUPS) {

		temp_servicegroup = find_servicegroup(query_servicegroup_name);

		if (temp_servicegroup == NULL) {
			print_generic_error_message("There are no service groups with this name defined.", NULL, 0);
			return;
		}
		/* make sure the user is authorized to view this servicegroup */
		if (is_authorized_for_servicegroup(temp_servicegroup, &current_authdata) == FALSE) {
			print_generic_error_message("It appears as though you do not have permission to view information for the service group you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
			return;
		}
	}

	add_log_filter(&filter_list, LOGENTRY_HOST_NOTIFICATION, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_SERVICE_NOTIFICATION, LOGFILTER_INCLUDE);

	/* scan the log file for notification data */
	status = get_log_entries(&entry_list, &filter_list, &error_text, NULL, reverse, ts_start, ts_end);

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

		return;

	/* now we start displaying the notification entries */
	} else {
		if (content_type == JSON_CONTENT) {
			if (status != READLOG_OK)
				printf(",\n");
			printf("\"notifications\": [\n");
		} else if (content_type == CSV_CONTENT) {
			printf("%sHOST%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sSERVICE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sTYPE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sTIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sCONTACT%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sNOTIFICATION_COMMAND%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sINFORMATION%s\n", csv_data_enclosure, csv_data_enclosure);
		} else {
			printf("<table border=0 CLASS='notifications' align='center'>\n");

			/* add export to csv, json, link */
			printf("<TR><TD colspan='7'>");
			printf("<table width='100%%' cellspacing=0 cellpadding=0><tr><td width='33%%'></td><td width='33%%' nowrap>");
			printf("<div class='page_selector'>\n");
			printf("<div id='page_navigation_copy'></div>");
			page_limit_selector(result_start);
			printf("</div>\n");
			printf("</td><td width='33%%' align='right' style='padding-right:2px'>\n");
			printf("<div class='csv_export_link'>");
			print_export_link(CSV_CONTENT, NOTIFICATIONS_CGI, NULL);
			print_export_link(JSON_CONTENT, NOTIFICATIONS_CGI, NULL);
			print_export_link(HTML_CONTENT, NOTIFICATIONS_CGI, NULL);
			printf("</div></td></tr></table>");

			printf("<tr>\n");
			printf("<th CLASS='notifications'>Host</th>\n");
			printf("<th CLASS='notifications'>Service</th>\n");
			printf("<th CLASS='notifications'>Type</th>\n");
			printf("<th CLASS='notifications'>Time</th>\n");
			printf("<th CLASS='notifications'>Contact</th>\n");
			printf("<th CLASS='notifications'>Notification Command</th>\n");
			printf("<th CLASS='notifications'>Information</th>\n");
			printf("</tr>\n");
		}

		/* check all entries */
		for (temp_entry = entry_list; temp_entry != NULL; temp_entry = temp_entry->next) {

			/* get the date/time */
			get_time_string(&temp_entry->timestamp, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			strip(date_time);

			/* get the contact name */
			temp_buffer = (char *)strtok(temp_entry->entry_text, ":");
			temp_buffer = (char *)strtok(NULL, ";");
			snprintf(contact_name, sizeof(contact_name), "%s", (temp_buffer == NULL) ? "" : temp_buffer + 1);
			contact_name[sizeof(contact_name) - 1] = '\x0';

			/* get the host name */
			temp_buffer = (char *)strtok(NULL, ";");
			snprintf(host_name, sizeof(host_name), "%s", (temp_buffer == NULL) ? "" : temp_buffer);
			host_name[sizeof(host_name) - 1] = '\x0';

			/* get the service name */
			service_name[0] = '\x0';
			if (temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION) {
				temp_buffer = (char *)strtok(NULL, ";");
				snprintf(service_name, sizeof(service_name), "%s", (temp_buffer == NULL) ? "" : temp_buffer);
				service_name[sizeof(service_name) - 1] = '\x0';
			}

			/* get the alert level */
			temp_buffer = (char *)strtok(NULL, ";");
			snprintf(alert_level, sizeof(alert_level), "%s", (temp_buffer == NULL) ? "" : temp_buffer);
			alert_level[sizeof(alert_level) - 1] = '\x0';

			if (temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION) {

				if (!strcmp(alert_level, "CRITICAL")) {
					notification_detail_type = NOTIFICATION_SERVICE_CRITICAL;
					strcpy(alert_level_class, "CRITICAL");
				} else if (!strcmp(alert_level, "WARNING")) {
					notification_detail_type = NOTIFICATION_SERVICE_WARNING;
					strcpy(alert_level_class, "WARNING");
				} else if (!strcmp(alert_level, "RECOVERY") || !strcmp(alert_level, "OK")) {
					strcpy(alert_level, "OK");
					notification_detail_type = NOTIFICATION_SERVICE_RECOVERY;
					strcpy(alert_level_class, "OK");
				} else if (strstr(alert_level, "CUSTOM (")) {
					notification_detail_type = NOTIFICATION_SERVICE_CUSTOM;
					strcpy(alert_level_class, "CUSTOM");
				} else if (strstr(alert_level, "ACKNOWLEDGEMENT (")) {
					notification_detail_type = NOTIFICATION_SERVICE_ACK;
					strcpy(alert_level_class, "ACKNOWLEDGEMENT");
				} else if (strstr(alert_level, "FLAPPINGSTART (")) {
					strcpy(alert_level, "FLAPPING START");
					notification_detail_type = NOTIFICATION_SERVICE_FLAP;
					strcpy(alert_level_class, "UNKNOWN");
				} else if (strstr(alert_level, "FLAPPINGSTOP (")) {
					strcpy(alert_level, "FLAPPING STOP");
					notification_detail_type = NOTIFICATION_SERVICE_FLAP;
					strcpy(alert_level_class, "UNKNOWN");
				} else {
					strcpy(alert_level, "UNKNOWN");
					notification_detail_type = NOTIFICATION_SERVICE_UNKNOWN;
					strcpy(alert_level_class, "UNKNOWN");
				}
			} else {

				if (!strcmp(alert_level, "DOWN")) {
					strncpy(alert_level, "HOST DOWN", sizeof(alert_level));
					strcpy(alert_level_class, "HOSTDOWN");
					notification_detail_type = NOTIFICATION_HOST_DOWN;
				} else if (!strcmp(alert_level, "UNREACHABLE")) {
					strncpy(alert_level, "HOST UNREACHABLE", sizeof(alert_level));
					strcpy(alert_level_class, "HOSTUNREACHABLE");
					notification_detail_type = NOTIFICATION_HOST_UNREACHABLE;
				} else if (!strcmp(alert_level, "RECOVERY") || !strcmp(alert_level, "UP")) {
					strncpy(alert_level, "HOST UP", sizeof(alert_level));
					strcpy(alert_level_class, "HOSTUP");
					notification_detail_type = NOTIFICATION_HOST_RECOVERY;
				} else if (strstr(alert_level, "CUSTOM (")) {
					strcpy(alert_level_class, "HOSTCUSTOM");
					notification_detail_type = NOTIFICATION_HOST_CUSTOM;
				} else if (strstr(alert_level, "ACKNOWLEDGEMENT (")) {
					strcpy(alert_level_class, "HOSTACKNOWLEDGEMENT");
					notification_detail_type = NOTIFICATION_HOST_ACK;
				} else if (strstr(alert_level, "FLAPPINGSTART (")) {
					strcpy(alert_level, "FLAPPING START");
					strcpy(alert_level_class, "UNKNOWN");
					notification_detail_type = NOTIFICATION_HOST_FLAP;
				} else if (strstr(alert_level, "FLAPPINGSTOP (")) {
					strcpy(alert_level, "FLAPPING STOP");
					strcpy(alert_level_class, "UNKNOWN");
					notification_detail_type = NOTIFICATION_HOST_FLAP;
				}
			}

			/* get the method name */
			temp_buffer = (char *)strtok(NULL, ";");
			snprintf(method_name, sizeof(method_name), "%s", (temp_buffer == NULL) ? "" : temp_buffer);
			method_name[sizeof(method_name) - 1] = '\x0';

			/* move to the informational message */
			temp_buffer = strtok(NULL, ";");

			show_entry = FALSE;

			/* if we're searching by contact, filter out unwanted contact */
			if (query_type == DISPLAY_CONTACTS) {
				if (find_all == TRUE)
					show_entry = TRUE;
				else if (!strcmp(query_contact_name, contact_name))
					show_entry = TRUE;
			}

			/* search host */
			else if (query_type == DISPLAY_HOSTS) {
				if (find_all == TRUE)
					show_entry = TRUE;
				else if (!strcmp(query_host_name, host_name))
					show_entry = TRUE;
			}

			/* searching service */
			else if (query_type == DISPLAY_SERVICES) {
				if (!strcmp(query_host_name, host_name) && !strcmp(query_svc_description, service_name))
					show_entry = TRUE;
			}

			/* Set TRUE here, get's checked later on */
			else if (query_type == DISPLAY_HOSTGROUPS || query_type == DISPLAY_SERVICEGROUPS) {
				show_entry = TRUE;
			}

			if (show_entry == TRUE) {
				if (notification_options == NOTIFICATION_ALL)
					show_entry = TRUE;
				else if (notification_options == NOTIFICATION_HOST_ALL && temp_entry->type == LOGENTRY_HOST_NOTIFICATION)
					show_entry = TRUE;
				else if (notification_options == NOTIFICATION_SERVICE_ALL && temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION)
					show_entry = TRUE;
				else if (notification_detail_type & notification_options)
					show_entry = TRUE;
				else
					show_entry = FALSE;
			}

			/* make sure user has authorization to view this notification */
			temp_host = find_host(host_name);
			if (temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION)
				temp_service = find_service(host_name, service_name);

			if (temp_host != NULL) {
				snprintf(displayed_host_name, sizeof(displayed_host_name), "%s", (temp_host->display_name != NULL && content_type == HTML_CONTENT) ? temp_host->display_name : temp_host->name);
				displayed_host_name[sizeof(displayed_host_name) - 1] = '\x0';

				if (temp_entry->type == LOGENTRY_HOST_NOTIFICATION) {
					if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
						show_entry = FALSE;
					else if (query_type == DISPLAY_HOSTGROUPS && is_host_member_of_hostgroup(temp_hostgroup, temp_host) == FALSE)
						show_entry = FALSE;
					else if (query_type == DISPLAY_SERVICEGROUPS && is_host_member_of_servicegroup(temp_servicegroup, temp_host) == FALSE)
						show_entry = FALSE;
				} else {
					if (temp_service != NULL) {
						snprintf(displayed_service_desc, sizeof(displayed_service_desc), "%s", (temp_service->display_name != NULL && content_type == HTML_CONTENT) ? temp_service->display_name : temp_service->description);
						displayed_service_desc[sizeof(displayed_service_desc) - 1] = '\x0';

						if (is_authorized_for_service(temp_service, &current_authdata) == FALSE)
							show_entry = FALSE;
						else if (query_type == DISPLAY_HOSTGROUPS && is_host_member_of_hostgroup(temp_hostgroup, temp_host) == FALSE)
							show_entry = FALSE;
						else if (query_type == DISPLAY_SERVICEGROUPS && is_service_member_of_servicegroup(temp_servicegroup, temp_service) == FALSE)
							show_entry = FALSE;
					} else {
						if (is_authorized_for_all_services(&current_authdata) == FALSE)
							show_entry = FALSE;

						snprintf(displayed_service_desc, sizeof(displayed_service_desc), "%s", service_name);
						displayed_service_desc[sizeof(displayed_service_desc) - 1] = '\x0';
					}
				}
			} else {
				if (temp_entry->type == LOGENTRY_HOST_NOTIFICATION) {
					if (is_authorized_for_all_hosts(&current_authdata) == FALSE)
						show_entry = FALSE;
				} else {
					if (is_authorized_for_all_services(&current_authdata) == FALSE)
						show_entry = FALSE;

					snprintf(displayed_service_desc, sizeof(displayed_service_desc), "%s", service_name);
					displayed_service_desc[sizeof(displayed_service_desc) - 1] = '\x0';
				}

				snprintf(displayed_host_name, sizeof(displayed_host_name), "%s", host_name);
				displayed_host_name[sizeof(displayed_host_name) - 1] = '\x0';
			}

			if (show_entry == TRUE) {

				if (result_limit != 0  && (((total_notifications + 1) < result_start) || (total_notifications >= ((result_start + result_limit) - 1)))) {
					total_notifications++;
					continue;
				}

				displayed_entries++;
				total_notifications++;

				if (odd)
					odd = 0;
				else
					odd = 1;

				if (content_type == JSON_CONTENT) {
					if (json_start == FALSE)
						printf(",\n");
					printf("{\"host_name\": \"%s\", ", json_encode(temp_host->name));
					printf("\"host_display_name\": \"%s\", ", (temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(temp_host->name));
					if (temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION) {
						printf("\"service_description\": \"%s\", ", json_encode(temp_service->description));
						printf("\"service_display_name\": \"%s\", ", (temp_service->display_name != NULL) ? json_encode(temp_service->display_name) : json_encode(temp_service->description));
					} else {
						printf("\"service_description\": null, ");
						printf("\"service_display_name\": null, ");
					}
					printf("\"type\": \"%s\", ", alert_level);
					printf("\"time\": \"%s\", ", date_time);
					printf("\"contact\": \"%s\", ", json_encode(contact_name));
					printf("\"notification_command\": \"%s\", ", json_encode(method_name));
					printf("\"information\": \"%s\"}", json_encode(escape_newlines(temp_buffer)));
				} else if (content_type == CSV_CONTENT) {
					printf("%s%s%s%s", csv_data_enclosure, displayed_host_name, csv_data_enclosure, csv_delimiter);
					if (temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION)
						printf("%s%s%s%s", csv_data_enclosure, displayed_service_desc, csv_data_enclosure, csv_delimiter);
					else
						printf("%sN/A%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
					printf("%s%s%s%s", csv_data_enclosure, alert_level, csv_data_enclosure, csv_delimiter);
					printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
					printf("%s%s%s%s", csv_data_enclosure, contact_name, csv_data_enclosure, csv_delimiter);
					printf("%s%s%s%s", csv_data_enclosure, method_name, csv_data_enclosure, csv_delimiter);
					printf("%s%s%s\n", csv_data_enclosure, escape_newlines(temp_buffer), csv_data_enclosure);
				} else {
					printf("<tr CLASS='notifications%s'>\n", (odd) ? "Even" : "Odd");
					if (temp_host != NULL)
						printf("<td CLASS='notifications%s'><a href='%s?type=%d&host=%s'>%s</a></td>\n", (odd) ? "Even" : "Odd", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(host_name), displayed_host_name);
					else
						printf("<td CLASS='notifications%s'>%s</td>\n", (odd) ? "Even" : "Odd", displayed_host_name);
					if (temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION) {
						if (temp_service != NULL) {
							printf("<td CLASS='notifications%s'><a href='%s?type=%d&host=%s", (odd) ? "Even" : "Odd", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(host_name));
							printf("&service=%s'>%s</a></td>\n", url_encode(service_name), displayed_service_desc);
						} else
							printf("<td CLASS='notifications%s'>%s</td>\n", (odd) ? "Even" : "Odd", displayed_service_desc);

					} else
						printf("<td CLASS='notifications%s'>N/A</td>\n", (odd) ? "Even" : "Odd");
					printf("<td CLASS='notifications%s'>%s</td>\n", alert_level_class, alert_level);
					printf("<td CLASS='notifications%s'>%s</td>\n", (odd) ? "Even" : "Odd", date_time);
					printf("<td CLASS='notifications%s'><a href='%s?type=contacts#%s'>%s</a></td>\n", (odd) ? "Even" : "Odd", CONFIG_CGI, url_encode(contact_name), contact_name);
					printf("<td CLASS='notifications%s'><a href='%s?type=commands#%s'>%s</a></td>\n", (odd) ? "Even" : "Odd", CONFIG_CGI, url_encode(method_name), method_name);
					printf("<td CLASS='notifications%s'>%s</td>\n", (odd) ? "Even" : "Odd", html_encode(temp_buffer, FALSE));
					printf("</tr>\n");
				}
				if (json_start == TRUE)
					json_start = FALSE;
			}
		}
	}

	free_log_entries(&entry_list);

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</table>\n");

		if (total_notifications == 0) {
			printf("<DIV CLASS='errorMessage' style='text-align:center;'>No notifications have been recorded");
			if (find_all == FALSE) {
				if (query_type == DISPLAY_SERVICES)
					printf(" for this service");
				else if (query_type == DISPLAY_CONTACTS)
					printf(" for this contact");
				else
					printf(" for this host");
			}
			printf(" in log files for selected date.</DIV>");
		}

		page_num_selector(result_start, total_notifications, displayed_entries);

	} else if (content_type == JSON_CONTENT) {
		printf("\n]\n");
	}

	return;
}
