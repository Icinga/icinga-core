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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../include/config.h"
#include "../include/common.h"

#include "../include/getcgi.h"
#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/readlogs.h"

extern char 	main_config_file[MAX_FILENAME_LENGTH];

extern int	log_rotation_method;

extern int 	embedded;
extern int 	display_header;
extern int 	daemon_check;
extern int 	content_type;

extern logentry *entry_list;

extern char 	*csv_delimiter;
extern char 	*csv_data_enclosure;

#define FIND_HOST		1
#define FIND_CONTACT		2
#define FIND_SERVICE		3

authdata current_authdata;

int log_archive = 0;
int query_type = FIND_HOST;
int find_all = TRUE;
int notification_options = NOTIFICATION_ALL;
int reverse = FALSE;
int display_type = DISPLAY_HOSTS;

char log_file_to_use[MAX_FILENAME_LENGTH];
char *query_contact_name = "";
char *query_host_name = "";
char *query_svc_description = "";

int CGI_ID = NOTIFICATIONS_CGI_ID;

void display_notifications(void);
int process_cgivars(void);

int main(void) {
	int result = OK;
	char temp_buffer[MAX_INPUT_BUFFER];
	char temp_buffer2[MAX_INPUT_BUFFER];

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

	document_header(CGI_ID, TRUE, "Alert Notifications");

	/* get authentication information */
	get_authentication_information(&current_authdata);

	/* determine what log file we should use */
	get_log_archive_to_use(log_archive, log_file_to_use, (int)sizeof(log_file_to_use));

	if (display_header == TRUE) {

		/* begin top table */
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of top row */
		printf("<td align=left valign=top width=33%%>\n");

		if (query_type == FIND_SERVICE)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Service Notifications");
		else if (query_type == FIND_HOST) {
			if (find_all == TRUE)
				snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Notifications");
			else
				snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Host Notifications");
		} else
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Contact Notifications");

		display_info_table(temp_buffer, &current_authdata, daemon_check);

		if (query_type == FIND_HOST || query_type == FIND_SERVICE) {
			printf("<TABLE BORDER=1 CELLPADDING=0 CELLSPACING=0 CLASS='linkBox'>\n");
			printf("<TR><TD CLASS='linkBox'>\n");
			if (query_type == FIND_HOST) {
				printf("<A HREF='%s?host=%s'>View Status Detail For %s</A><BR>\n", STATUS_CGI, (find_all == TRUE) ? "all" : url_encode(query_host_name), (find_all == TRUE) ? "All Hosts" : "This Host");
				printf("<A HREF='%s?host=%s'>View History For %s</A><BR>\n", HISTORY_CGI, (find_all == TRUE) ? "all" : url_encode(query_host_name), (find_all == TRUE) ? "All Hosts" : "This Host");
#ifdef USE_TRENDS
				if (find_all == FALSE)
					printf("<A HREF='%s?host=%s'>View Trends For This Host</A><BR>\n", TRENDS_CGI, url_encode(query_host_name));
#endif
			} else if (query_type == FIND_SERVICE) {
				printf("<A HREF='%s?host=%s&", HISTORY_CGI, (find_all == TRUE) ? "all" : url_encode(query_host_name));
				printf("service=%s'>View History For This Service</A><BR>\n", url_encode(query_svc_description));
#ifdef USE_TRENDS
				printf("<A HREF='%s?host=%s&", TRENDS_CGI, (find_all == TRUE) ? "all" : url_encode(query_host_name));
				printf("service=%s'>View Trends For This Service</A><BR>\n", url_encode(query_svc_description));
#endif
			}
			printf("</TD></TR>\n");
			printf("</TABLE>\n");
		}

		printf("</td>\n");

		/* middle column of top row */
		printf("<td align=center valign=top width=33%%>\n");

		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>\n");
		if (query_type == FIND_SERVICE)
			printf("Service '%s' on Host '%s'", query_svc_description, query_host_name);
		else if (query_type == FIND_HOST) {
			if (find_all == TRUE)
				printf("All Hosts and Services");
			else
				printf("Host '%s'", query_host_name);
		} else {
			if (find_all == TRUE)
				printf("All Contacts");
			else
				printf("Contact '%s'", query_contact_name);
		}
		printf("</DIV>\n");
		printf("<BR>\n");

		if (query_type == FIND_SERVICE) {
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "%s?%shost=%s&", NOTIFICATIONS_CGI, (reverse == TRUE) ? "oldestfirst&" : "", url_encode(query_host_name));
			snprintf(temp_buffer2, sizeof(temp_buffer2) - 1, "service=%s&type=%d&", url_encode(query_svc_description), notification_options);
			strncat(temp_buffer, temp_buffer2, sizeof(temp_buffer) - strlen(temp_buffer) - 1);
		} else
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "%s?%s%s=%s&type=%d&", NOTIFICATIONS_CGI, (reverse == TRUE) ? "oldestfirst&" : "", (query_type == FIND_HOST) ? "host" : "contact", (query_type == FIND_HOST) ? url_encode(query_host_name) : url_encode(query_contact_name), notification_options);

		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		display_nav_table(temp_buffer, log_archive);

		printf("</td>\n");

		/* right hand column of top row */
		printf("<td align=right valign=top width=33%%>\n");

		printf("<table border=0 CLASS='optBox'>\n");
		printf("<form method='GET' action='%s'>\n", NOTIFICATIONS_CGI);
		if (query_type == FIND_SERVICE) {
			printf("<input type='hidden' name='host' value='%s'>\n", escape_string(query_host_name));
			printf("<input type='hidden' name='service' value='%s'>\n", escape_string(query_svc_description));
		} else
			printf("<input type='hidden' name='%s' value='%s'>\n", (query_type == FIND_HOST) ? "host" : "contact", (query_type == FIND_HOST) ? escape_string(query_host_name) : escape_string(query_contact_name));
		printf("<input type='hidden' name='archive' value='%d'>\n", log_archive);
		printf("<tr>\n");
		if (query_type == FIND_SERVICE)
			printf("<td align=left colspan=2 CLASS='optBoxItem'>Notification detail level for this service:</td>");
		else
			printf("<td align=left colspan=2 CLASS='optBoxItem'>Notification detail level for %s %s%s:</td>", (find_all == TRUE) ? "all" : "this", (query_type == FIND_HOST) ? "host" : "contact", (find_all == TRUE) ? "s" : "");
		printf("</tr>\n");
		printf("<tr>\n");
		printf("<td align=left colspan=2 CLASS='optBoxItem'><select name='type'>\n");
		printf("<option value=%d %s>All notifications\n", NOTIFICATION_ALL, (notification_options == NOTIFICATION_ALL) ? "selected" : "");
		if (query_type != FIND_SERVICE) {
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
		if (query_type != FIND_SERVICE) {
			printf("<option value=%d %s>Host custom\n", NOTIFICATION_HOST_CUSTOM, (notification_options == NOTIFICATION_HOST_CUSTOM) ? "selected" : "");
			printf("<option value=%d %s>Host acknowledgements\n", NOTIFICATION_HOST_ACK, (notification_options == NOTIFICATION_HOST_ACK) ? "selected" : "");
			printf("<option value=%d %s>Host down\n", NOTIFICATION_HOST_DOWN, (notification_options == NOTIFICATION_HOST_DOWN) ? "selected" : "");
			printf("<option value=%d %s>Host unreachable\n", NOTIFICATION_HOST_UNREACHABLE, (notification_options == NOTIFICATION_HOST_UNREACHABLE) ? "selected" : "");
			printf("<option value=%d %s>Host recovery\n", NOTIFICATION_HOST_RECOVERY, (notification_options == NOTIFICATION_HOST_RECOVERY) ? "selected" : "");
			printf("<option value=%d %s>Host flapping\n", NOTIFICATION_HOST_FLAP, (notification_options == NOTIFICATION_HOST_FLAP) ? "selected" : "");
		}
		printf("</select></td>\n");
		printf("</tr>\n");
		printf("<tr>\n");
		printf("<td align=left CLASS='optBoxItem'>Older Entries First:</td>\n");
		printf("<td></td>\n");
		printf("</tr>\n");
		printf("<tr>\n");
		printf("<td align=left valign=bottom CLASS='optBoxItem'><input type='checkbox' name='oldestfirst' %s></td>", (reverse == TRUE) ? "checked" : "");
		printf("<td align=right CLASS='optBoxItem'><input type='submit' value='Update'></td>\n");
		printf("</tr>\n");

		/* display context-sensitive help */
		printf("<tr><td></td><td align=right valign=bottom>\n");
		display_context_help(CONTEXTHELP_NOTIFICATIONS);
		printf("</td></tr>\n");

		printf("</form>\n");
		printf("</table>\n");

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
			query_type = FIND_HOST;
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
			query_type = FIND_CONTACT;
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
			query_type = FIND_SERVICE;
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}
			if ((query_svc_description = strdup(variables[x])) == NULL)
				query_svc_description = "";
			strip_html_brackets(query_svc_description);
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

		/* we found the log archive argument */
		else if (!strcmp(variables[x], "archive")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			log_archive = atoi(variables[x]);
			if (log_archive < 0)
				log_archive = 0;
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

		/* we found the order argument */
		else if (!strcmp(variables[x], "oldestfirst"))
			reverse = TRUE;

		/* we found the embed option */
		else if (!strcmp(variables[x], "embedded"))
			embedded = TRUE;

		/* we found the noheader option */
		else if (!strcmp(variables[x], "noheader"))
			display_header = FALSE;

		/* we found the nodaemoncheck option */
		else if (!strcmp(variables[x], "nodaemoncheck"))
			daemon_check = FALSE;
	}

	/*
	 * Set some default values if not already set.
	 * Done here as they won't be set if variable
	 * not provided via cgi parameters
	 * Only required for hosts & contacts, not services
	 * as there is no service_name=all option
	 */
	if (query_type == FIND_HOST && strlen(query_host_name) == 0) {
		query_host_name = "all";
		find_all = TRUE;
	}
	if (query_type == FIND_CONTACT && strlen(query_contact_name) == 0) {
		query_contact_name = "all";
		find_all = TRUE;
	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

void display_notifications(void) {
	char *temp_buffer;
	char date_time[MAX_DATETIME_LENGTH];
	char alert_level[MAX_INPUT_BUFFER];
	char alert_level_class[MAX_INPUT_BUFFER];
	char contact_name[MAX_INPUT_BUFFER];
	char service_name[MAX_INPUT_BUFFER];
	char host_name[MAX_INPUT_BUFFER];
	char method_name[MAX_INPUT_BUFFER];
	char error_text[MAX_INPUT_BUFFER];
	char displayed_host_name[MAX_INPUT_BUFFER];
	char displayed_service_desc[MAX_INPUT_BUFFER];
	int show_entry, status;
	int total_notifications = 0;
	int notification_detail_type = NOTIFICATION_SERVICE_CRITICAL;
	int odd = 0;
	int json_start = TRUE;
	host *temp_host = NULL;
	service *temp_service = NULL;
	logentry *temp_entry = NULL;


	add_log_filter(LOGENTRY_HOST_NOTIFICATION, LOGFILTER_INCLUDE);
	add_log_filter(LOGENTRY_SERVICE_NOTIFICATION, LOGFILTER_INCLUDE);

	status = get_log_entries(log_file_to_use, NULL, reverse, -1, -1);

	free_log_filters();

	if (status == READLOG_ERROR_MEMORY) {
		printf("<P><DIV CLASS='warningMessage'>Run out of memory..., showing all I could gather!</DIV></P>");
	}

	if (status == READLOG_ERROR_NOFILE) {
		snprintf(error_text, sizeof(error_text), "Error: Could not open log file '%s' for reading!", log_file_to_use);
		error_text[sizeof(error_text)-1] = '\x0';
		print_generic_error_message(error_text, NULL, 0);
		free_log_entries();
		return;
	}

	if (status == READLOG_OK) {
		if (content_type == JSON_CONTENT)
			printf("\"notifications\": [\n");
		else if (content_type == CSV_CONTENT) {
			printf("%sHOST%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sSERVICE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sTYPE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sTIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sCONTACT%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sNOTIFICATION_COMMAND%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sINFORMATION%s\n", csv_data_enclosure, csv_data_enclosure);
		} else {
			printf("<p>\n");
			printf("<div align='center'>\n");

			printf("<table border=0 CLASS='notifications'>\n");

			/* add export to csv, json, link */
			printf("<TR><TD colspan='7'>");
			printf("<div class='csv_export_link'>");
			print_export_link(CSV_CONTENT, NOTIFICATIONS_CGI, NULL);
			print_export_link(JSON_CONTENT, NOTIFICATIONS_CGI, NULL);
			print_export_link(HTML_CONTENT, NOTIFICATIONS_CGI, NULL);
			printf("</DIV></TD></TR>\n");

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
	}

	if (status == READLOG_OK) {

		/* check all entries */
		for (temp_entry = entry_list; temp_entry != NULL; temp_entry = temp_entry->next) {

			/* get the date/time */
			get_time_string(&temp_entry->timestamp, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			strip(date_time);

			/* get the contact name */
			temp_buffer = (char *)strtok(temp_entry->entry_text, ":");
			temp_buffer = (char *)strtok(NULL, ";");
			snprintf(contact_name, sizeof(contact_name), "%s", (temp_buffer == NULL) ? "" : temp_buffer + 1);
			contact_name[sizeof(contact_name)-1] = '\x0';

			/* get the host name */
			temp_buffer = (char *)strtok(NULL, ";");
			snprintf(host_name, sizeof(host_name), "%s", (temp_buffer == NULL) ? "" : temp_buffer);
			host_name[sizeof(host_name)-1] = '\x0';

			/* get the service name */
			service_name[0] = '\x0';
			if (temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION) {
				temp_buffer = (char *)strtok(NULL, ";");
				snprintf(service_name, sizeof(service_name), "%s", (temp_buffer == NULL) ? "" : temp_buffer);
				service_name[sizeof(service_name)-1] = '\x0';
			}

			/* get the alert level */
			temp_buffer = (char *)strtok(NULL, ";");
			snprintf(alert_level, sizeof(alert_level), "%s", (temp_buffer == NULL) ? "" : temp_buffer);
			alert_level[sizeof(alert_level)-1] = '\x0';

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
			method_name[sizeof(method_name)-1] = '\x0';

			/* move to the informational message */
			temp_buffer = strtok(NULL, ";");

			show_entry = FALSE;

			/* if we're searching by contact, filter out unwanted contact */
			if (query_type == FIND_CONTACT) {
				if (find_all == TRUE)
					show_entry = TRUE;
				else if (!strcmp(query_contact_name, contact_name))
					show_entry = TRUE;
			}

			/* search host */
			else if (query_type == FIND_HOST) {
				if (find_all == TRUE)
					show_entry = TRUE;
				else if (!strcmp(query_host_name, host_name))
					show_entry = TRUE;
			}

			/* searching service */
			else if (query_type == FIND_SERVICE) {
				if (!strcmp(query_host_name, host_name) && !strcmp(query_svc_description, service_name))
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
				displayed_host_name[sizeof(displayed_host_name)-1] = '\x0';

				if (temp_entry->type == LOGENTRY_HOST_NOTIFICATION) {
					if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
						show_entry = FALSE;
				} else {
					if (temp_service != NULL) {
						snprintf(displayed_service_desc, sizeof(displayed_service_desc), "%s", (temp_service->display_name != NULL && content_type == HTML_CONTENT) ? temp_service->display_name : temp_service->description);
						displayed_service_desc[sizeof(displayed_service_desc)-1] = '\x0';

						if (is_authorized_for_service(temp_service, &current_authdata) == FALSE)
							show_entry = FALSE;
					} else {
						if (is_authorized_for_all_services(&current_authdata) == FALSE)
							show_entry = FALSE;

						snprintf(displayed_service_desc, sizeof(displayed_service_desc), "%s", service_name);
						displayed_service_desc[sizeof(displayed_service_desc)-1] = '\x0';
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
					displayed_service_desc[sizeof(displayed_service_desc)-1] = '\x0';
				}

				snprintf(displayed_host_name, sizeof(displayed_host_name), "%s", host_name);
				displayed_host_name[sizeof(displayed_host_name)-1] = '\x0';
			}

			if (show_entry == TRUE) {

				total_notifications++;

				if (odd)
					odd = 0;
				else
					odd = 1;

				if (content_type == JSON_CONTENT) {
					if (json_start == FALSE)
						printf(",\n");
					printf("{\"host\": \"%s\", ", json_encode(displayed_host_name));
					if (temp_entry->type == LOGENTRY_SERVICE_NOTIFICATION)
						printf("\"service\": \"%s\", ", json_encode(displayed_service_desc));
					else
						printf("\"service\": null, ");
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

	free_log_entries();

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</table>\n");

		printf("</div>\n");
		printf("</p>\n");

		if (total_notifications == 0) {
			printf("<P><DIV CLASS='errorMessage' style='text-align:center;'>No notifications have been recorded");
			if (find_all == FALSE) {
				if (query_type == FIND_SERVICE)
					printf(" for this service");
				else if (query_type == FIND_CONTACT)
					printf(" for this contact");
				else
					printf(" for this host");
			}
			printf(" in %s log file</DIV></P>", (log_archive == 0) ? "the current" : "this archived");
		} else
			printf("<DIV align=center>%d Notification%s</DIV>", total_notifications, (total_notifications == 1) ? "" : "s");
	} else if (content_type == JSON_CONTENT) {
		printf("\n]\n");
	}

	return;
}
