/*****************************************************************************
 *
 * EXTINFO.C -  Icinga Extended Information CGI
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *****************************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/macros.h"
#include "../include/comments.h"
#include "../include/downtime.h"
#include "../include/statusdata.h"

static icinga_macros *mac;

/* make sure gcc3 won't hit here */
#ifndef GCCTOOOLD
#include "../include/statsprofiler.h"
#endif

#include "../include/cgiutils.h"
#include "../include/getcgi.h"
#include "../include/cgiauth.h"

extern char             nagios_check_command[MAX_INPUT_BUFFER];
extern char             nagios_process_info[MAX_INPUT_BUFFER];
extern int              nagios_process_state;

extern time_t		program_start;
extern int              nagios_pid;
extern int              daemon_mode;
extern time_t           last_command_check;
extern time_t           last_log_rotation;
extern int              enable_notifications;
extern time_t		disable_notifications_expire_time;
extern int              execute_service_checks;
extern int              accept_passive_service_checks;
extern int              execute_host_checks;
extern int              accept_passive_host_checks;
extern int              enable_event_handlers;
extern int              obsess_over_services;
extern int              obsess_over_hosts;
extern int              enable_flap_detection;
extern int              enable_failure_prediction;
extern int              process_performance_data;
/* make sure gcc3 won't hit here */
#ifndef GCCTOOOLD
extern int		event_profiling_enabled;
#endif
extern int              buffer_stats[1][3];
extern int              program_stats[MAX_CHECK_STATS_TYPES][3];

extern int              suppress_maintenance_downtime;
extern int		extinfo_show_child_hosts;
extern int		tab_friendly_titles;
extern int		result_limit;

extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];
extern char url_logo_images_path[MAX_FILENAME_LENGTH];

extern int              enable_splunk_integration;

extern char             *notes_url_target;
extern char             *action_url_target;

extern host *host_list;
extern service *service_list;
extern hoststatus *hoststatus_list;
extern servicestatus *servicestatus_list;

extern comment           *comment_list;
extern scheduled_downtime  *scheduled_downtime_list;
extern hoststatus *hoststatus_list;
extern servicestatus *servicestatus_list;
extern hostgroup *hostgroup_list;
extern servicegroup *servicegroup_list;
extern servicedependency *servicedependency_list;
extern hostdependency *hostdependency_list;

/* make sure gcc3 won't hit here */
#ifndef GCCTOOOLD
extern profile_object* profiled_data;
#endif

#define MAX_MESSAGE_BUFFER		4096

#define HEALTH_WARNING_PERCENTAGE       85
#define HEALTH_CRITICAL_PERCENTAGE      75

/* this is only necessary to distinguish between comments and downtime in single host/service view */
#define CSV_DEFAULT			0
#define CSV_COMMENT			1
#define CSV_DOWNTIME			2


/* SORTDATA structure */
typedef struct sortdata_struct {
	int is_service;
	servicestatus *svcstatus;
	hoststatus *hststatus;
	struct sortdata_struct *next;
} sortdata;

int process_cgivars(void);

void show_process_info(void);
void show_host_info(void);
void show_service_info(void);
void show_performance_data(void);
void show_hostgroup_info(void);
void show_servicegroup_info(void);
void show_downtime(int);
void show_scheduling_queue(void);
void show_comments(int);

int sort_data(int, int);
int compare_sortdata_entries(int, int, sortdata *, sortdata *);
void free_sortdata_list(void);

int is_host_child_of_host(host *, host *);

authdata current_authdata;

sortdata *sortdata_list = NULL;

char *host_name = "";
char *hostgroup_name = "";
char *servicegroup_name = "";
char *service_desc = "";

int display_type = DISPLAY_PROCESS_INFO;
int sort_type = SORT_ASCENDING;
int sort_option = SORT_NEXTCHECKTIME;
int csv_type = CSV_DEFAULT;
int get_result_limit = -1;
int result_start = 1;
int total_entries = 0;
int displayed_entries = 0;


int dummy;	/* reduce compiler warnings */

extern int embedded;
extern int refresh;
extern int display_header;
extern int daemon_check;
extern int content_type;

extern char *csv_delimiter;
extern char *csv_data_enclosure;

int CGI_ID = EXTINFO_CGI_ID;

int main(void) {
	int result = OK;
	int found = FALSE;
	char temp_buffer[MAX_INPUT_BUFFER] = "";
	char *processed_string = NULL;
	char *cgi_title = NULL;
	host *temp_host = NULL;
	hostsmember *temp_parenthost = NULL;
	hostgroup *temp_hostgroup = NULL;
	service *temp_service = NULL;
	servicegroup *temp_servicegroup = NULL;
	servicedependency *temp_sd = NULL;
	char *last_sd_svc_desc = "";
	char *last_sd_hostname = "";
	hostdependency *temp_hd = NULL;
	host * child_host;

	mac = get_global_macros();

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

	/* read all status data */
	result = read_all_status_data(get_cgi_config_location(), READ_ALL_STATUS_DATA);
	if (result == ERROR && daemon_check == TRUE) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(NULL, ERROR_CGI_STATUS_DATA);
		document_footer(CGI_ID);
		free_memory();
		return ERROR;
	}

	/* overwrite config value with amount we got via GET */
	result_limit = (get_result_limit != -1) ? get_result_limit : result_limit;

	/* for json and csv output return all by default */
	if (get_result_limit == -1 && (content_type == JSON_CONTENT || content_type == CSV_CONTENT))
		result_limit = 0;

	/* initialize macros */
	init_macros();

	if (tab_friendly_titles == TRUE) {
		if (display_type == DISPLAY_HOST_INFO && host_name && (*host_name != '\0'))
			dummy = asprintf(&cgi_title, "[%s]", html_encode(host_name, FALSE));
		else if (display_type == DISPLAY_SERVICE_INFO && service_desc && *service_desc != '\0' && host_name && *host_name != '\0')
			dummy = asprintf(&cgi_title, "%s @ %s", html_encode(service_desc, FALSE), html_encode(host_name, FALSE));
		else if (display_type == DISPLAY_HOSTGROUP_INFO && hostgroup_name && *hostgroup_name != '\0')
			dummy = asprintf(&cgi_title, "{%s}", html_encode(hostgroup_name, FALSE));
		else if (display_type == DISPLAY_SERVICEGROUP_INFO && servicegroup_name && *servicegroup_name != '\0')
			dummy = asprintf(&cgi_title, "(%s)", html_encode(servicegroup_name, FALSE));
	}

	document_header(CGI_ID, TRUE, (tab_friendly_titles == TRUE && cgi_title != NULL) ? cgi_title : "Extended Information");

	my_free(cgi_title);

	/* get authentication information */
	get_authentication_information(&current_authdata);


	if (display_header == TRUE) {

		/* begin top table */
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=33%%>\n");

		if (display_type == DISPLAY_HOST_INFO)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Host Information");
		else if (display_type == DISPLAY_SERVICE_INFO)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Service Information");
		else if (display_type == DISPLAY_COMMENTS)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "All Host and Service Comments");
		else if (display_type == DISPLAY_PERFORMANCE)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Performance Information");
		else if (display_type == DISPLAY_HOSTGROUP_INFO)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Hostgroup Information");
		else if (display_type == DISPLAY_SERVICEGROUP_INFO)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Servicegroup Information");
		else if (display_type == DISPLAY_DOWNTIME)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "All Host and Service Scheduled Downtime");
		else if (display_type == DISPLAY_SCHEDULING_QUEUE)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Check Scheduling Queue");
		else
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Icinga Process Information");

		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';

		display_info_table(temp_buffer, &current_authdata, daemon_check);

		/* find the host */
		if (display_type == DISPLAY_HOST_INFO || display_type == DISPLAY_SERVICE_INFO) {

			temp_host = find_host(host_name);
			grab_host_macros_r(mac, temp_host);

			if (display_type == DISPLAY_SERVICE_INFO) {
				temp_service = find_service(host_name, service_desc);
				grab_service_macros_r(mac, temp_service);
			}
		}

		/* find the hostgroup */
		else if (display_type == DISPLAY_HOSTGROUP_INFO) {
			temp_hostgroup = find_hostgroup(hostgroup_name);
			grab_hostgroup_macros_r(mac, temp_hostgroup);
		}

		/* find the servicegroup */
		else if (display_type == DISPLAY_SERVICEGROUP_INFO) {
			temp_servicegroup = find_servicegroup(servicegroup_name);
			grab_servicegroup_macros_r(mac, temp_servicegroup);
		}

		if ((display_type == DISPLAY_HOST_INFO && temp_host != NULL) || (display_type == DISPLAY_SERVICE_INFO && temp_host != NULL && temp_service != NULL) || (display_type == DISPLAY_HOSTGROUP_INFO && temp_hostgroup != NULL) || (display_type == DISPLAY_SERVICEGROUP_INFO && temp_servicegroup != NULL)) {

			printf("<TABLE BORDER=1 CELLPADDING=0 CELLSPACING=0 CLASS='linkBox'>\n");
			printf("<TR><TD CLASS='linkBox'>\n");
			if (display_type == DISPLAY_SERVICE_INFO)
				printf("<a href='%s?type=%d&host=%s'>View <b>Information</b> For <b>This Host</b></a><br>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(host_name));
			if (display_type == DISPLAY_SERVICE_INFO || display_type == DISPLAY_HOST_INFO)
				printf("<a href='%s?host=%s'>View <b>Service Status Detail</b> For <b>This Host</b></a><br>\n", STATUS_CGI, url_encode(host_name));
			if (display_type == DISPLAY_HOST_INFO) {
				printf("<a href='%s?host=%s'>View <b>Alert History</b> For <b>This Host</b></a><br>\n", HISTORY_CGI, url_encode(host_name));
#ifdef USE_TRENDS
				printf("<a href='%s?host=%s'>View <b>Trends</b> For <b>This Host</b></a><br>\n", TRENDS_CGI, url_encode(host_name));
#endif
#ifdef USE_HISTOGRAM
				printf("<a href='%s?host=%s'>View <b>Alert Histogram</b> For <b>This Host</b></a><br>\n", HISTOGRAM_CGI, url_encode(host_name));
#endif
				printf("<a href='%s?host=%s&show_log_entries'>View <b>Availability Report</b> For <b>This Host</b></a><br>\n", AVAIL_CGI, url_encode(host_name));
				printf("<a href='%s?host=%s'>View <b>Notifications</b> For <b>This Host</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(host_name));
				printf("<a href='%s?type=%d&host=%s'>View <b>Scheduling Queue</b> For <b>This Host</b></a><br>\n", EXTINFO_CGI, DISPLAY_SCHEDULING_QUEUE, url_encode(host_name));
				if (is_authorized_for_configuration_information(&current_authdata) == TRUE)
					printf("<a href='%s?type=hosts&item_name=%s'>View <b>Config</b> For <b>This Host</b></a>\n", CONFIG_CGI, url_encode(host_name));
			} else if (display_type == DISPLAY_SERVICE_INFO) {
				printf("<a href='%s?host=%s&service=%s'>View <b>Alert History</b> For <b>This Service</b></a><br>\n", HISTORY_CGI, url_encode(host_name), url_encode(service_desc));
#ifdef USE_TRENDS
				printf("<a href='%s?host=%s&service=%s'>View <b>Trends</b> For <b>This Service</b></a><br>\n", TRENDS_CGI, url_encode(host_name), url_encode(service_desc));
#endif
#ifdef USE_HISTOGRAM
				printf("<a href='%s?host=%s&service=%s'>View <b>Alert Histogram</b> For <b>This Service</b></a><br>\n", HISTOGRAM_CGI, url_encode(host_name), url_encode(service_desc));
#endif
				printf("<a href='%s?host=%s&service=%s&show_log_entries'>View <b>Availability Report</b> For <b>This Service</b></a><br>\n", AVAIL_CGI, url_encode(host_name), url_encode(service_desc));
				printf("<a href='%s?host=%s&service=%s'>View <b>Notifications</b> For <b>This Service</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(host_name), url_encode(service_desc));
				printf("<a href='%s?type=%d&host=%s&service=%s'>View <b>Scheduling Queue</b> For <b>This Service</b></a><br>\n", EXTINFO_CGI, DISPLAY_SCHEDULING_QUEUE, url_encode(host_name), url_encode(service_desc));
				if (is_authorized_for_configuration_information(&current_authdata) == TRUE)
					printf("<a href='%s?type=services&item_name=%s^%s'>View <b>Config</b> For <b>This Service</b></a>\n", CONFIG_CGI, url_encode(host_name), url_encode(service_desc));
			} else if (display_type == DISPLAY_HOSTGROUP_INFO) {
				printf("<a href='%s?hostgroup=%s&style=detail'>View <b>Status Detail</b> For <b>This Hostgroup</b></a><br>\n", STATUS_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s&style=overview'>View <b>Status Overview</b> For <b>This Hostgroup</b></a><br>\n", STATUS_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s&style=grid'>View <b>Status Grid</b> For <b>This Hostgroup</b></a><br>\n", STATUS_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s'>View <b>Alert History</b> For <b>This Hostgroup</b></a><br>\n", HISTORY_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s'>View <b>Availability Report</b> For <b>This Hostgroup</b></a><br>\n", AVAIL_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s'>View <b>Notifications</b> For <b>This Hostgroup</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(hostgroup_name));
				if (is_authorized_for_configuration_information(&current_authdata) == TRUE)
					printf("<a href='%s?type=hostgroups&item_name=%s'>View <b>Config</b> For <b>This Hostgroup</b></a>\n", CONFIG_CGI, url_encode(hostgroup_name));
			} else if (display_type == DISPLAY_SERVICEGROUP_INFO) {
				printf("<a href='%s?servicegroup=%s&style=detail'>View <b>Status Detail</b> For <b>This Servicegroup</b></a><br>\n", STATUS_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s&style=overview'>View <b>Status Overview</b> For <b>This Servicegroup</b></a><br>\n", STATUS_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s&style=grid'>View <b>Status Grid</b> For <b>This Servicegroup</b></a><br>\n", STATUS_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s'>View <b>Alert History</b> For <b>This Servicegroup</b></a><br>\n", HISTORY_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s'>View <b>Availability Report</b> For <b>This Servicegroup</b></a><br>\n", AVAIL_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s'>View <b>Notifications</b> For <b>This Servicegroup</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(servicegroup_name));
				if (is_authorized_for_configuration_information(&current_authdata) == TRUE)
					printf("<a href='%s?type=servicegroups&item_name=%s'>View <b>Config</b> For <b>This Servicegroup</b></a>\n", CONFIG_CGI, url_encode(servicegroup_name));
			}
			printf("</TD></TR>\n");
			printf("</TABLE>\n");
		}

		printf("</td>\n");

		/* middle column of top row */
		printf("<td align=center valign=middle width=33%%>\n");

		if ((display_type == DISPLAY_HOST_INFO && temp_host != NULL) || (display_type == DISPLAY_SERVICE_INFO && temp_host != NULL && temp_service != NULL) || (display_type == DISPLAY_HOSTGROUP_INFO && temp_hostgroup != NULL) || (display_type == DISPLAY_SERVICEGROUP_INFO && temp_servicegroup != NULL)) {

			if (display_type == DISPLAY_HOST_INFO) {

				printf("<DIV CLASS='data'>Host</DIV>\n");
				printf("<DIV CLASS='dataTitle'>%s</DIV>\n", temp_host->alias);
				printf("<DIV CLASS='dataTitle'>(%s)</DIV><BR>\n", (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);

				if (temp_host->parent_hosts != NULL) {
					/* print all parent hosts */
					printf("<DIV CLASS='data'>Parents:</DIV>\n");
					for (temp_parenthost = temp_host->parent_hosts; temp_parenthost != NULL; temp_parenthost = temp_parenthost->next)
						printf("<DIV CLASS='dataTitle'><A HREF='%s?host=%s'>%s</A></DIV>\n", STATUS_CGI, url_encode(temp_parenthost->host_name), temp_parenthost->host_name);
				}

				/* Hostgroups */
				printf("<DIV CLASS='data'>Member of</DIV><DIV CLASS='dataTitle'>");

				for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {
					if (is_host_member_of_hostgroup(temp_hostgroup, temp_host) == TRUE) {
						if (found == TRUE)
							printf(", ");

						printf("<A HREF='%s?hostgroup=%s&style=overview'>%s</A>", STATUS_CGI, url_encode(temp_hostgroup->group_name), html_encode((temp_hostgroup->alias != NULL) ? temp_hostgroup->alias : temp_hostgroup->group_name, TRUE));
						found = TRUE;
					}
				}

				if (found == FALSE)
					printf("No hostgroups");

				printf("</DIV>\n");

				/* Child Hosts */
				if (extinfo_show_child_hosts == SHOW_CHILD_HOSTS_IMMEDIATE || extinfo_show_child_hosts == SHOW_CHILD_HOSTS_ALL) {
					found = FALSE;

					printf("<DIV CLASS='data'>Immediate Child Hosts ");
					printf("<img id='expand_image_immediate' src='%s%s' border=0 onClick=\"if (document.getElementById('immediate_child_hosts').style.display == 'none') { document.getElementById('immediate_child_hosts').style.display = ''; document.getElementById('immediate_child_hosts_gap').style.display = 'none'; document.getElementById('expand_image_immediate').src = '%s%s'; } else { document.getElementById('immediate_child_hosts').style.display = 'none'; document.getElementById('immediate_child_hosts_gap').style.display = ''; document.getElementById('expand_image_immediate').src = '%s%s'; }\">", url_images_path, EXPAND_ICON, url_images_path, COLLAPSE_ICON, url_images_path, EXPAND_ICON);
					printf("</DIV><DIV CLASS='dataTitle' id='immediate_child_hosts_gap' style='display:;'>&nbsp;</DIV><DIV CLASS='dataTitle' id='immediate_child_hosts' style='display:none;'>");

					for (child_host = host_list; child_host != NULL; child_host = child_host->next) {
						if (is_host_immediate_child_of_host(temp_host, child_host) == TRUE) {
							if (found == TRUE)
								printf(", ");

							printf("<A HREF='%s?type=%d&host=%s'>%s</A>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(child_host->name), html_encode(child_host->name, TRUE));
							found = TRUE;
						}
					}

					if (found == FALSE)
						printf("None");

					printf("</DIV>\n");

					if (extinfo_show_child_hosts == SHOW_CHILD_HOSTS_ALL) {
						found = FALSE;

						printf("<DIV CLASS='data'>All Child Hosts ");
						printf("<img id='expand_image_all' src='%s%s' border=0 onClick=\"if (document.getElementById('all_child_hosts').style.display == 'none') { document.getElementById('all_child_hosts').style.display = ''; document.getElementById('all_child_hosts_gap').style.display = 'none'; document.getElementById('expand_image_all').src = '%s%s'; } else { document.getElementById('all_child_hosts').style.display = 'none'; document.getElementById('all_child_hosts_gap').style.display = ''; document.getElementById('expand_image_all').src = '%s%s'; }\">", url_images_path, EXPAND_ICON, url_images_path, COLLAPSE_ICON, url_images_path, EXPAND_ICON);
						printf("</DIV><DIV CLASS='dataTitle' id='all_child_hosts_gap' style='display:;'>&nbsp;</DIV><DIV CLASS='dataTitle' id='all_child_hosts' style='display:none;'>");

						for (child_host = host_list; child_host != NULL; child_host = child_host->next) {
							if (is_host_child_of_host(temp_host, child_host) == TRUE) {
								if (found == TRUE)
									printf(", ");

								printf("<A HREF='%s?type=%d&host=%s'>%s</A>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(child_host->name), html_encode(child_host->name, TRUE));

								found = TRUE;
							}
						}

						if (found == FALSE)
							printf("None");

						printf("</DIV>\n");
					}
				}

				/* Host Dependencies */
				found = FALSE;

				printf("<DIV CLASS='data'>Host Dependencies</DIV><DIV CLASS='dataTitle'>");

				for (temp_hd = hostdependency_list; temp_hd != NULL; temp_hd = temp_hd->next) {

					if (!strcmp(temp_hd->dependent_host_name, temp_host->name)) {
						if (found == TRUE)
							printf(", ");

						printf("<A HREF='%s?type=%d&host=%s'>%s</A>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_hd->host_name), html_encode(temp_hd->host_name, FALSE));
						found = TRUE;
					}
				}

				if (found == FALSE)
					printf("None");

				printf("</DIV>\n");

				/* Host address(6) */
				if (!strcmp(temp_host->address6, temp_host->name)) {
					printf("<DIV CLASS='data'>%s</DIV>\n", temp_host->address);
				} else {
					printf("<DIV CLASS='data'>%s, %s</DIV>\n", temp_host->address, temp_host->address6);
				}
			}
			if (display_type == DISPLAY_SERVICE_INFO) {

				printf("<DIV CLASS='data'>Service</DIV><DIV CLASS='dataTitle'>%s</DIV><DIV CLASS='data'>On Host</DIV>\n", (temp_service->display_name != NULL) ? temp_service->display_name : temp_service->description);
				printf("<DIV CLASS='dataTitle'>%s</DIV>\n", temp_host->alias);
				printf("<DIV CLASS='dataTitle'>(<A HREF='%s?type=%d&host=%s'>%s</a>)</DIV><BR>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_host->name), (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);

				/* Servicegroups */
				printf("<DIV CLASS='data'>Member of</DIV><DIV CLASS='dataTitle'>");

				for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {
					if (is_service_member_of_servicegroup(temp_servicegroup, temp_service) == TRUE) {
						if (found == TRUE)
							printf(", ");

						printf("<A HREF='%s?servicegroup=%s&style=overview'>%s</A>", STATUS_CGI, url_encode(temp_servicegroup->group_name), html_encode((temp_servicegroup->alias != NULL) ? temp_servicegroup->alias : temp_servicegroup->group_name, TRUE));
						found = TRUE;
					}
				}

				if (found == FALSE)
					printf("No servicegroups.");

				printf("</DIV>\n");

				/* Service Dependencies */
				found = FALSE;

				printf("<DIV CLASS='data'>Service Dependencies</DIV><DIV CLASS='dataTitle'>");

				for (temp_sd = servicedependency_list; temp_sd != NULL; temp_sd = temp_sd->next) {

					if (!strcmp(temp_sd->dependent_service_description, temp_service->description) && !strcmp(temp_sd->dependent_host_name, temp_host->name) && \
					        !(!strcmp(temp_sd->service_description, last_sd_svc_desc) && !strcmp(temp_sd->host_name, last_sd_hostname))) {
						if (found == TRUE)
							printf(", ");

						printf("<A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_sd->host_name));
						printf("&service=%s'>%s on %s</A>\n", url_encode(temp_sd->service_description), html_encode(temp_sd->service_description, FALSE), html_encode(temp_sd->host_name, FALSE));
						found = TRUE;
					}
					last_sd_svc_desc = temp_sd->service_description;
					last_sd_hostname = temp_sd->host_name;
				}

				if (found == FALSE)
					printf("None");

				printf("</DIV>\n");


				if (!strcmp(temp_host->address6, temp_host->name)) {
					printf("<DIV CLASS='data'>%s</DIV>\n", temp_host->address);
				} else {
					printf("<DIV CLASS='data'>%s, %s</DIV>\n", temp_host->address, temp_host->address6);
				}
			}
			if (display_type == DISPLAY_HOSTGROUP_INFO) {

				printf("<DIV CLASS='data'>Hostgroup</DIV>\n");
				printf("<DIV CLASS='dataTitle'>%s</DIV>\n", temp_hostgroup->alias);
				printf("<DIV CLASS='dataTitle'>(%s)</DIV>\n", temp_hostgroup->group_name);

				if (temp_hostgroup->notes != NULL) {
					process_macros_r(mac, temp_hostgroup->notes, &processed_string, 0);
					printf("<p>%s</p>", processed_string);
					free(processed_string);
				}
			}
			if (display_type == DISPLAY_SERVICEGROUP_INFO) {

				printf("<DIV CLASS='data'>Servicegroup</DIV>\n");
				printf("<DIV CLASS='dataTitle'>%s</DIV>\n", temp_servicegroup->alias);
				printf("<DIV CLASS='dataTitle'>(%s)</DIV>\n", temp_servicegroup->group_name);

				if (temp_servicegroup->notes != NULL) {
					process_macros_r(mac, temp_servicegroup->notes, &processed_string, 0);
					printf("<p>%s</p>", processed_string);
					free(processed_string);
				}
			}

			if (display_type == DISPLAY_SERVICE_INFO) {
				if (temp_service->icon_image != NULL) {
					printf("<img src='%s", url_logo_images_path);
					process_macros_r(mac, temp_service->icon_image, &processed_string, 0);
					printf("%s", processed_string);
					free(processed_string);
					printf("' border=0 alt='%s' title='%s'><BR CLEAR=ALL>", (temp_service->icon_image_alt == NULL) ? "" : temp_service->icon_image_alt, (temp_service->icon_image_alt == NULL) ? "" : temp_service->icon_image_alt);
				}
				if (temp_service->icon_image_alt != NULL)
					printf("<font size=-1><i>( %s )</i></font>\n", temp_service->icon_image_alt);
				if (temp_service->notes != NULL) {
					process_macros_r(mac, temp_service->notes, &processed_string, 0);
					printf("<p>%s</p>\n", processed_string);
					free(processed_string);
				}
			}

			if (display_type == DISPLAY_HOST_INFO) {
				if (temp_host->icon_image != NULL) {
					printf("<img src='%s", url_logo_images_path);
					process_macros_r(mac, temp_host->icon_image, &processed_string, 0);
					printf("%s", processed_string);
					free(processed_string);
					printf("' border=0 alt='%s' title='%s'><BR CLEAR=ALL>", (temp_host->icon_image_alt == NULL) ? "" : temp_host->icon_image_alt, (temp_host->icon_image_alt == NULL) ? "" : temp_host->icon_image_alt);
				}
				if (temp_host->icon_image_alt != NULL)
					printf("<font size=-1><i>( %s )</i><font>\n", temp_host->icon_image_alt);
				if (temp_host->notes != NULL) {
					process_macros_r(mac, temp_host->notes, &processed_string, 0);
					printf("<p>%s</p>\n", processed_string);
					free(processed_string);
				}
			}
		}

		printf("</td>\n");

		/* right column of top row */
		printf("<td align=right valign=bottom width=33%%>\n");

		if (display_type == DISPLAY_HOST_INFO && temp_host != NULL) {

			printf("<TABLE BORDER='0'>\n");
			if (temp_host->action_url != NULL && strcmp(temp_host->action_url, "")) {
				process_macros_r(mac, temp_host->action_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<TR><TD ALIGN='right'>\n");
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'><img src='%s%s%s' border=0 alt='Perform Additional Actions On This Host' title='Perform Additional Actions On This Host'></A>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, MU_iconstr, ACTION_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>Extra Actions</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				printf("</TD></TR>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_host->notes_url != NULL && strcmp(temp_host->notes_url, "")) {
				process_macros_r(mac, temp_host->notes_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<TR><TD ALIGN='right'>\n");
				printf("<A HREF='");
				printf("%s", processed_string);
				/*print_extra_host_url(temp_host->name,temp_host->notes_url);*/
				printf("' TARGET='%s'><img src='%s%s%s' border=0 alt='View Additional Notes For This Host' title='View Additional Notes For This Host'></A>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, MU_iconstr, NOTES_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>Extra Notes</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				printf("</TD></TR>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			printf("</TABLE>\n");
		}

		else if (display_type == DISPLAY_SERVICE_INFO && temp_service != NULL) {

			printf("<TABLE BORDER='0'><TR><TD ALIGN='right'>\n");

			if (temp_service->action_url != NULL && strcmp(temp_service->action_url, "")) {
				process_macros_r(mac, temp_service->action_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'><img src='%s%s%s' border=0 alt='Perform Additional Actions On This Service' title='Perform Additional Actions On This Service'></A>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, MU_iconstr, ACTION_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>Extra Actions</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_service->notes_url != NULL && strcmp(temp_service->notes_url, "")) {
				process_macros_r(mac, temp_service->notes_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'><img src='%s%s%s' border=0 alt='View Additional Notes For This Service' title='View Additional Notes For This Service'></A>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, MU_iconstr, NOTES_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>Extra Notes</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			printf("</TD></TR></TABLE>\n");
		}

		if (display_type == DISPLAY_HOSTGROUP_INFO && temp_hostgroup != NULL) {
			printf("<TABLE BORDER='0'>\n");

			if (temp_hostgroup->action_url != NULL && strcmp(temp_hostgroup->action_url, "")) {
				printf("<TR><TD ALIGN='right'>\n");
				printf("<A HREF='");
				print_extra_hostgroup_url(temp_hostgroup->group_name, temp_hostgroup->action_url);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='Perform Additional Actions On This Hostgroup' title='Perform Additional Actions On This Hostgroup'></A>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, ACTION_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>Extra Actions</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				printf("</TD></TR>\n");
			}
			if (temp_hostgroup->notes_url != NULL && strcmp(temp_hostgroup->notes_url, "")) {
				printf("<TR><TD ALIGN='right'>\n");
				printf("<A HREF='");
				print_extra_hostgroup_url(temp_hostgroup->group_name, temp_hostgroup->notes_url);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='View Additional Notes For This Hostgroup' title='View Additional Notes For This Hostgroup'></A>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, NOTES_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>Extra Notes</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				printf("</TD></TR>\n");
			}
			printf("</TABLE>\n");
		}

		else if (display_type == DISPLAY_SERVICEGROUP_INFO && temp_servicegroup != NULL) {
			printf("<TABLE BORDER='0'>\n");

			if (temp_servicegroup->action_url != NULL && strcmp(temp_servicegroup->action_url, "")) {
				printf("<A HREF='");
				print_extra_servicegroup_url(temp_servicegroup->group_name, temp_servicegroup->action_url);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='Perform Additional Actions On This Servicegroup' title='Perform Additional Actions On This Servicegroup'></A>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, ACTION_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>Extra Actions</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
			}
			if (temp_servicegroup->notes_url != NULL && strcmp(temp_servicegroup->notes_url, "")) {
				printf("<A HREF='");
				print_extra_servicegroup_url(temp_servicegroup->group_name, temp_servicegroup->notes_url);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='View Additional Notes For This Servicegroup' title='View Additional Notes For This Servicegroup'></A>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, NOTES_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>Extra Notes</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
			}
			printf("</TABLE>\n");
		}

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");

	}

	if (content_type == HTML_CONTENT) {
		if (display_type == DISPLAY_HOST_INFO || display_type == DISPLAY_SERVICE_INFO) {
			printf("<DIV style='padding-right:6px;' class='csv_export_link'>");
			print_export_link(JSON_CONTENT, EXTINFO_CGI, NULL);
			print_export_link(HTML_CONTENT, EXTINFO_CGI, NULL);
			printf("</DIV>");
		} else
			printf("<BR>\n");
	}

	if (display_type == DISPLAY_HOST_INFO) {
		if (content_type == CSV_CONTENT) {
			if (csv_type == CSV_COMMENT)
				show_comments(HOST_COMMENT);
			else if (csv_type == CSV_DOWNTIME)
				show_downtime(HOST_DOWNTIME);
			else
				printf("Please specify the correct csvtype! possible is \"csvtype=comment\" or \"csv_type=downtime\".\n");
		} else
			show_host_info();
	} else if (display_type == DISPLAY_SERVICE_INFO) {
		if (content_type == CSV_CONTENT) {
			if (csv_type == CSV_COMMENT)
				show_comments(SERVICE_COMMENT);
			else if (csv_type == CSV_DOWNTIME)
				show_downtime(SERVICE_DOWNTIME);
			else
				printf("Please specify the correct csvtype! possible is \"csvtype=comment\" or \"csv_type=downtime\".\n");
		} else
			show_service_info();
	} else if (display_type == DISPLAY_COMMENTS) {
		if (is_authorized_for_read_only(&current_authdata) == TRUE && is_authorized_for_comments_read_only(&current_authdata) == FALSE)
			printf("<DIV ALIGN=CENTER CLASS='infoMessage'>Your account does not have permissions to view comments.<br>\n");
		else {
			if (content_type == CSV_CONTENT || content_type == JSON_CONTENT) {
				show_comments(HOST_COMMENT);
				if (content_type == JSON_CONTENT)
					printf(",\n");
				show_comments(SERVICE_COMMENT);
			} else {
				printf("<BR>\n");
				printf("<DIV CLASS='commentNav'>[&nbsp;<A HREF='#HOSTCOMMENTS' CLASS='commentNav'>Host Comments</A>&nbsp;|&nbsp;<A HREF='#SERVICECOMMENTS' CLASS='commentNav'>Service Comments</A>&nbsp;]</DIV>\n");
				printf("<BR>\n");

				show_comments(HOST_COMMENT);
				printf("<br>\n");
				show_comments(SERVICE_COMMENT);
			}
		}
	} else if (display_type == DISPLAY_DOWNTIME) {
		if (is_authorized_for_read_only(&current_authdata) == TRUE && is_authorized_for_downtimes_read_only(&current_authdata) == FALSE)
			printf("<DIV ALIGN=CENTER CLASS='infoMessage'>Your account does not have permissions to view downtimes.<br>\n");
		else {
			if (content_type == CSV_CONTENT || content_type == JSON_CONTENT) {
				show_downtime(HOST_DOWNTIME);
				if (content_type == JSON_CONTENT)
					printf(",\n");
				show_downtime(SERVICE_DOWNTIME);
			} else {
				printf("<br>\n");
				printf("<DIV CLASS='downtimeNav'>[&nbsp;<A HREF='#HOSTDOWNTIME' CLASS='downtimeNav'>Host Downtime</A>&nbsp;|&nbsp;<A HREF='#SERVICEDOWNTIME' CLASS='downtimeNav'>Service Downtime</A>&nbsp;]</DIV>\n");
				printf("<br>\n");

				show_downtime(HOST_DOWNTIME);
				printf("<br>\n");
				show_downtime(SERVICE_DOWNTIME);
			}
		}
	} else if (display_type == DISPLAY_PERFORMANCE)
		show_performance_data();
	else if (display_type == DISPLAY_HOSTGROUP_INFO)
		show_hostgroup_info();
	else if (display_type == DISPLAY_SERVICEGROUP_INFO)
		show_servicegroup_info();
	else if (display_type == DISPLAY_SCHEDULING_QUEUE)
		show_scheduling_queue();
	else
		show_process_info();

	document_footer(CGI_ID);

	/* free all allocated memory */
	free_memory();
	free_comment_data();
	free_downtime_data();

	return OK;
}

int process_cgivars(void) {
	char **variables;
	int error = FALSE;
	int temp_type;
	int x;

	variables = getcgivars();

	for (x = 0; variables[x] != NULL; x++) {

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if (strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
			x++;
			continue;
		}

		/* we found the display type */
		else if (!strcmp(variables[x], "type")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}
			temp_type = atoi(variables[x]);
			if (temp_type == DISPLAY_HOST_INFO)
				display_type = DISPLAY_HOST_INFO;
			else if (temp_type == DISPLAY_SERVICE_INFO)
				display_type = DISPLAY_SERVICE_INFO;
			else if (temp_type == DISPLAY_COMMENTS)
				display_type = DISPLAY_COMMENTS;
			else if (temp_type == DISPLAY_PERFORMANCE)
				display_type = DISPLAY_PERFORMANCE;
			else if (temp_type == DISPLAY_HOSTGROUP_INFO)
				display_type = DISPLAY_HOSTGROUP_INFO;
			else if (temp_type == DISPLAY_SERVICEGROUP_INFO)
				display_type = DISPLAY_SERVICEGROUP_INFO;
			else if (temp_type == DISPLAY_DOWNTIME)
				display_type = DISPLAY_DOWNTIME;
			else if (temp_type == DISPLAY_SCHEDULING_QUEUE)
				display_type = DISPLAY_SCHEDULING_QUEUE;
			else
				display_type = DISPLAY_PROCESS_INFO;
		}

		/* we found the host name */
		else if (!strcmp(variables[x], "host")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			host_name = strdup(variables[x]);
			if (host_name == NULL)
				host_name = "";
			strip_html_brackets(host_name);
		}

		/* we found the hostgroup name */
		else if (!strcmp(variables[x], "hostgroup")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			hostgroup_name = strdup(variables[x]);
			if (hostgroup_name == NULL)
				hostgroup_name = "";
			strip_html_brackets(hostgroup_name);
		}

		/* we found the service name */
		else if (!strcmp(variables[x], "service")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			service_desc = strdup(variables[x]);
			if (service_desc == NULL)
				service_desc = "";
			strip_html_brackets(service_desc);
		}

		/* we found the servicegroup name */
		else if (!strcmp(variables[x], "servicegroup")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			servicegroup_name = strdup(variables[x]);
			if (servicegroup_name == NULL)
				servicegroup_name = "";
			strip_html_brackets(servicegroup_name);
		}

		/* we found the sort type argument */
		else if (!strcmp(variables[x], "sorttype")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			sort_type = atoi(variables[x]);
		}

		/* we found the sort option argument */
		else if (!strcmp(variables[x], "sortoption")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			sort_option = atoi(variables[x]);
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

		else if (!strcmp(variables[x], "csvtype")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "comment"))
				csv_type = CSV_COMMENT;
			else if (!strcmp(variables[x], "downtime"))
				csv_type = CSV_DOWNTIME;
			else
				csv_type = CSV_DEFAULT;
		}

		/* we found the embed option */
		else if (!strcmp(variables[x], "embedded"))
			embedded = TRUE;

		/* we found the noheader option */
		else if (!strcmp(variables[x], "noheader"))
			display_header = FALSE;

		/* we found the pause option */
		else if (!strcmp(variables[x], "paused"))
			refresh = FALSE;

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


	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

void show_process_info(void) {
	char start_time[MAX_DATETIME_LENGTH];
	char last_external_check_time[MAX_DATETIME_LENGTH];
	char last_log_rotation_time[MAX_DATETIME_LENGTH];
	char disable_notif_expire_time[MAX_DATETIME_LENGTH];
	time_t current_time;
	unsigned long run_time;
	char run_time_string[24];
	int days = 0;
	int hours = 0;
	int minutes = 0;
	int seconds = 0;

	/* make sure the user has rights to view system information */
	if (is_authorized_for_system_information(&current_authdata) == FALSE) {

		print_generic_error_message("It appears as though you do not have permission to view process information...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);

		return;
	}

	/* program start time */
	get_time_string(&program_start, start_time, (int)sizeof(start_time), SHORT_DATE_TIME);

	/* total running time */
	time(&current_time);
	run_time = (unsigned long)(current_time - program_start);
	get_time_breakdown(run_time, &days, &hours, &minutes, &seconds);
	sprintf(run_time_string, "%dd %dh %dm %ds", days, hours, minutes, seconds);

	/* last external check */
	get_time_string(&last_command_check, last_external_check_time, (int)sizeof(last_external_check_time), SHORT_DATE_TIME);

	/* last log file rotation */
	get_time_string(&last_log_rotation, last_log_rotation_time, (int)sizeof(last_log_rotation_time), SHORT_DATE_TIME);

	/* disabled notifications expire time */
	get_time_string(&disable_notifications_expire_time, disable_notif_expire_time, (int)sizeof(disable_notif_expire_time), SHORT_DATE_TIME);

	if (content_type == JSON_CONTENT) {
		printf("\"process_info\": {\n");
		printf("\"program_version\": \"%s\",\n", PROGRAM_VERSION);
		printf("\"program_start_time\": \"%s\",\n", start_time);
		printf("\"total_running_time\": \"%s\",\n", run_time_string);
		if (last_command_check == (time_t)0)
			printf("\"last_external_command_check\": null,\n");
		else
			printf("\"last_external_command_check\": \"%s\",\n", last_external_check_time);
		if (last_log_rotation == (time_t)0)
			printf("\"last_log_file_rotation\": null,\n");
		else
			printf("\"last_log_file_rotation\": \"%s\",\n", last_log_rotation_time);

		printf("\"icinga_pid\": %d,\n", nagios_pid);
		printf("\"notifications_enabled\": %s,\n", (enable_notifications == TRUE) ? "true" : "false");
		printf("\"service_checks_being_executed\": %s,\n", (execute_service_checks == TRUE) ? "true" : "false");
		printf("\"passive_service_checks_being_accepted\": %s,\n", (accept_passive_service_checks == TRUE) ? "true" : "false");
		printf("\"host_checks_being_executed\": %s,\n", (execute_host_checks == TRUE) ? "true" : "false");
		printf("\"passive_host_checks_being_accepted\": %s,\n", (accept_passive_host_checks == TRUE) ? "true" : "false");
		printf("\"event_handlers_enabled\": %s,\n", (enable_event_handlers == TRUE) ? "true" : "false");
		printf("\"obsessing_over_services\": %s,\n", (obsess_over_services == TRUE) ? "true" : "false");
		printf("\"obsessing_over_hosts\": %s,\n", (obsess_over_hosts == TRUE) ? "true" : "false");
		printf("\"flap_detection_enabled\": %s,\n", (enable_flap_detection == TRUE) ? "true" : "false");
		printf("\"performance_data_being_processed\": %s\n", (process_performance_data == TRUE) ? "true" : "false");
#ifdef PREDICT_FAILURES
		printf(",\"failure_prediction_enabled\": %s\n", (enable_failure_prediction == TRUE) ? "true" : "false");
#endif
#ifdef USE_OLDCRUD
		printf(",\"running_as_a_daemon\": %s\n", (daemon_mode == TRUE) ? "true" : "false");
#endif
		printf("}\n");
	} else if (content_type == CSV_CONTENT) {
		/* csv header line */
		printf("%sPROGRAM_VERSION%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sPROGRAM_START_TIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sTOTAL_RUNNING_TIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLAST_EXTERNAL_COMMAND_CHECK%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLAST_LOG_FILE_ROTATION%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sICINGA_PID%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNOTIFICATIONS_ENABLED%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sSERVICE_CHECKS_BEING_EXECUTED%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sPASSIVE_SERVICE_CHECKS_BEING_ACCEPTED%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHOST_CHECKS_BEING_EXECUTED%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sPASSIVE_HOST_CHECKS_BEING_ACCEPTED%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEVENT_HANDLERS_ENABLED%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sOBSESSING_OVER_SERVICES%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sOBSESSING_OVER_HOSTS%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFLAP_DETECTION_ENABLED%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sPERFORMANCE_DATA_BEING_PROCESSED%s", csv_data_enclosure, csv_data_enclosure);
#ifdef PREDICT_FAILURES
		printf("%s%sFAILURE_PREDICTION_ENABLED%s", csv_delimiter, csv_data_enclosure, csv_data_enclosure);
#endif
#ifdef USE_OLDCRUD
		printf("%s%sRUNNING_AS_A_DAEMON%s", csv_delimiter, csv_data_enclosure, csv_data_enclosure, csv_delimiter);
#endif
		printf("\n");

		/* csv data line */
		printf("%s%s%s%s", csv_data_enclosure, PROGRAM_VERSION, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, start_time, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, run_time_string, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (last_command_check == (time_t)0) ? "N/A" : last_external_check_time, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (last_log_rotation == (time_t)0) ? "N/A" : last_log_rotation_time, csv_data_enclosure, csv_delimiter);
		printf("%s%d%s%s", csv_data_enclosure, nagios_pid, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (enable_notifications == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (execute_service_checks == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (accept_passive_service_checks == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (execute_host_checks == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (accept_passive_host_checks == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (enable_event_handlers == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (obsess_over_services == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (obsess_over_hosts == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (enable_flap_detection == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s", csv_data_enclosure, (process_performance_data == TRUE) ? "YES" : "NO", csv_data_enclosure);
#ifdef PREDICT_FAILURES
		printf("%s%s%s%s", csv_delimiter, csv_data_enclosure, (enable_failure_prediction == TRUE) ? "YES" : "NO", csv_data_enclosure);
#endif
#ifdef USE_OLDCRUD
		printf("%s%s%s%s", csv_delimiter, csv_data_enclosure, (daemon_mode == TRUE) ? "YES" : "NO", csv_data_enclosure);
#endif
		printf("\n");
	} else {
		printf("<br>\n");

		/* add export to csv, json, link */
		printf("<div class='csv_export_link'>");
		print_export_link(CSV_CONTENT, EXTINFO_CGI, NULL);
		print_export_link(JSON_CONTENT, EXTINFO_CGI, NULL);
		print_export_link(HTML_CONTENT, EXTINFO_CGI, NULL);
		printf("</div>");

		printf("<TABLE BORDER=0 CELLPADDING=20 align='center'>\n");
		printf("<TR><TD VALIGN=TOP>\n");

		printf("<DIV CLASS='dataTitle'>Process Information</DIV>\n");

		printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0 CLASS='data'>\n");
		printf("<TR><TD class='stateInfoTable1'>\n");
		printf("<TABLE BORDER=0>\n");

		/* program version */
		printf("<TR><TD CLASS='dataVar'>Program Version:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", PROGRAM_VERSION);

		/* program start time */
		printf("<TR><TD CLASS='dataVar'>Program Start Time:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", start_time);

		/* total running time */
		printf("<TR><TD CLASS='dataVar'>Total Running Time:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", run_time_string);

		/* last external check */
		printf("<TR><TD CLASS='dataVar'>Last External Command Check:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (last_command_check == (time_t)0) ? "N/A" : last_external_check_time);

		/* last log file rotation */
		printf("<TR><TD CLASS='dataVar'>Last Log File Rotation:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (last_log_rotation == (time_t)0) ? "N/A" : last_log_rotation_time);

		/* PID */
		printf("<TR><TD CLASS='dataVar'>Icinga PID</TD><TD CLASS='dataVal'>%d</TD></TR>\n", nagios_pid);

		/* notifications enabled */
		printf("<TR><TD CLASS='dataVar'>Notifications Enabled?</TD><TD CLASS='dataVal'><DIV CLASS='notifications%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (enable_notifications == TRUE) ? "ENABLED" : "DISABLED", (enable_notifications == TRUE) ? "YES" : "NO");
		if (enable_notifications == FALSE)
			printf("<TR><TD CLASS='dataVar'>Notifications Disabled Expire Time:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", disable_notif_expire_time);
		else
			printf("<TR><TD CLASS='dataVar'>Notifications Disabled Expire Time:</TD><TD CLASS='dataVal'><DIV CLASS='notificationsUNKNOWN'>&nbsp;&nbsp;NOT SET&nbsp;&nbsp;</DIV></TD></TR>\n");


		/* service check execution enabled */
		printf("<TR><TD CLASS='dataVar'>Service Checks Being Executed?</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (execute_service_checks == TRUE) ? "ENABLED" : "DISABLED", (execute_service_checks == TRUE) ? "YES" : "NO");

		/* passive service check acceptance */
		printf("<TR><TD CLASS='dataVar'>Passive Service Checks Being Accepted?</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (accept_passive_service_checks == TRUE) ? "ENABLED" : "DISABLED", (accept_passive_service_checks == TRUE) ? "YES" : "NO");

		/* host check execution enabled */
		printf("<TR><TD CLASS='dataVar'>Host Checks Being Executed?</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (execute_host_checks == TRUE) ? "ENABLED" : "DISABLED", (execute_host_checks == TRUE) ? "YES" : "NO");

		/* passive host check acceptance */
		printf("<TR><TD CLASS='dataVar'>Passive Host Checks Being Accepted?</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (accept_passive_host_checks == TRUE) ? "ENABLED" : "DISABLED", (accept_passive_host_checks == TRUE) ? "YES" : "NO");

		/* event handlers enabled */
		printf("<TR><TD CLASS='dataVar'>Event Handlers Enabled?</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (enable_event_handlers == TRUE) ? "Yes" : "No");

		/* obsessing over services */
		printf("<TR><TD CLASS='dataVar'>Obsessing Over Services?</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (obsess_over_services == TRUE) ? "Yes" : "No");

		/* obsessing over hosts */
		printf("<TR><TD CLASS='dataVar'>Obsessing Over Hosts?</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (obsess_over_hosts == TRUE) ? "Yes" : "No");

		/* flap detection enabled */
		printf("<TR><TD CLASS='dataVar'>Flap Detection Enabled?</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (enable_flap_detection == TRUE) ? "Yes" : "No");

#ifdef PREDICT_FAILURES
		/* failure prediction enabled */
		printf("<TR><TD CLASS='dataVar'>Failure Prediction Enabled?</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (enable_failure_prediction == TRUE) ? "Yes" : "No");
#endif

		/* process performance data */
		printf("<TR><TD CLASS='dataVar'>Performance Data Being Processed?</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (process_performance_data == TRUE) ? "Yes" : "No");

		/* Notifications disabled will expire? */
		if(enable_notifications == TRUE && disable_notifications_expire_time > 0)
			printf("<TR><TD CLASS='dataVar'>Notifications?</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (process_performance_data == TRUE) ? "Yes" : "No");


#ifdef USE_OLDCRUD
		/* daemon mode */
		printf("<TR><TD CLASS='dataVar'>Running As A Daemon?</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (daemon_mode == TRUE) ? "Yes" : "No");
#endif

		printf("</TABLE>\n");
		printf("</TD></TR>\n");
		printf("</TABLE>\n");


		printf("</TD><TD VALIGN=TOP>\n");

		printf("<DIV CLASS='commandTitle'>Process Commands</DIV>\n");

		printf("<TABLE BORDER=1 CELLPADDING=0 CELLSPACING=0 CLASS='command'>\n");
		printf("<TR><TD>\n");

		if (nagios_process_state == STATE_OK) {
			printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 CLASS='command'>\n");

#ifndef DUMMY_INSTALL
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Shutdown the Icinga Process' TITLE='Shutdown the Icinga Process'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Shutdown the Icinga process</a></td></tr>\n", url_images_path, STOP_ICON, CMD_CGI, CMD_SHUTDOWN_PROCESS);
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Restart the Icinga Process' TITLE='Restart the Icinga Process'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Restart the Icinga process</a></td></tr>\n", url_images_path, RESTART_ICON, CMD_CGI, CMD_RESTART_PROCESS);
#endif

			if (enable_notifications == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Disable Notifications' TITLE='Disable Notifications'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Disable notifications</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_NOTIFICATIONS);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Enable Notifications' TITLE='Enable Notifications'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Enable notifications</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_NOTIFICATIONS);

			if (execute_service_checks == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Stop Executing Service Checks' TITLE='Stop Executing Service Checks'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Stop executing service checks</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_EXECUTING_SVC_CHECKS);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Start Executing Service Checks' TITLE='Start Executing Service Checks'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Start executing service checks</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_EXECUTING_SVC_CHECKS);

			if (accept_passive_service_checks == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Stop Accepting Passive Service Checks' TITLE='Stop Accepting Passive Service Checks'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Stop accepting passive service checks</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Start Accepting Passive Service Checks' TITLE='Start Accepting Passive Service Checks'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Start accepting passive service checks</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS);

			if (execute_host_checks == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Stop Executing Host Checks' TITLE='Stop Executing Host Checks'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Stop executing host checks</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_EXECUTING_HOST_CHECKS);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Start Executing Host Checks' TITLE='Start Executing Host Checks'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Start executing host checks</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_EXECUTING_HOST_CHECKS);

			if (accept_passive_host_checks == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Stop Accepting Passive Host Checks' TITLE='Stop Accepting Passive Host Checks'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Stop accepting passive host checks</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Start Accepting Passive Host Checks' TITLE='Start Accepting Passive Host Checks'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Start accepting passive host checks</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS);

			if (enable_event_handlers == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Disable Event Handlers' TITLE='Disable Event Handlers'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Disable event handlers</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_EVENT_HANDLERS);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Enable Event Handlers' TITLE='Enable Event Handlers'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Enable event handlers</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_EVENT_HANDLERS);

			if (obsess_over_services == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Stop Obsessing Over Services' TITLE='Stop Obsessing Over Services'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Stop obsessing over services</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_OBSESSING_OVER_SVC_CHECKS);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Start Obsessing Over Services' TITLE='Start Obsessing Over Services'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Start obsessing over services</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_OBSESSING_OVER_SVC_CHECKS);

			if (obsess_over_hosts == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Stop Obsessing Over Hosts' TITLE='Stop Obsessing Over Hosts'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Stop obsessing over hosts</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_OBSESSING_OVER_HOST_CHECKS);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Start Obsessing Over Hosts' TITLE='Start Obsessing Over Hosts'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Start obsessing over hosts</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_OBSESSING_OVER_HOST_CHECKS);

			if (enable_flap_detection == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Disable Flap Detection' TITLE='Disable Flap Detection'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Disable flap detection</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_FLAP_DETECTION);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Enable Flap Detection' TITLE='Enable Flap Detection'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Enable flap detection</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_FLAP_DETECTION);

#ifdef PREDICT_FAILURES
			if (enable_failure_prediction == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Disable Failure Prediction' TITLE='Disable Failure Prediction'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Disable failure prediction</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_FAILURE_PREDICTION);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Enable Failure Prediction' TITLE='Enable Failure Prediction'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Enable failure prediction</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_FAILURE_PREDICTION);
#endif
			if (process_performance_data == TRUE)
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Disable Performance Data' TITLE='Disable Performance Data'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Disable performance data</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_PERFORMANCE_DATA);
			else
				printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Enable Performance Data' TITLE='Enable Performance Data'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Enable performance data</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_PERFORMANCE_DATA);

			printf("</TABLE>\n");
		} else {
			printf("<DIV ALIGN=CENTER CLASS='infoMessage'>It appears as though Icinga is not running, so commands are temporarily unavailable...\n");
			if (!strcmp(nagios_check_command, "")) {
				printf("<BR><BR>\n");
				printf("Hint: It looks as though you have not defined a command for checking the process state by supplying a value for the <b>nagios_check_command</b> option in the CGI configuration file.<BR>\n");
				printf("Read the documentation for more information on checking the status of the Icinga process in the CGIs.\n");
			}
			printf("</DIV>\n");
		}

		printf("</TD></TR>\n");
		printf("</TABLE>\n");

		printf("</TD></TR></TABLE>\n");
	}
}

void show_host_info(void) {
	hoststatus *temp_hoststatus;
	host *temp_host;
	char date_time[MAX_DATETIME_LENGTH];
	char state_duration[48];
	char status_age[48];
	char state_string[MAX_INPUT_BUFFER];
	char *bg_class = "";
	char *buf = NULL;
	int days;
	int hours;
	int minutes;
	int seconds;
	time_t current_time;
	time_t ts_state_duration;
	time_t ts_state_age;
	int duration_error = FALSE;


	/* get host info */
	temp_host = find_host(host_name);

	/* make sure the user has rights to view host information */
	if (is_authorized_for_host(temp_host, &current_authdata) == FALSE) {
		print_generic_error_message("It appears as though you do not have permission to view information for this host...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		return;
	}

	/* get host status info */
	temp_hoststatus = find_hoststatus(host_name);

	/* make sure host information exists */
	if (temp_host == NULL) {
		print_generic_error_message("Error: Host Not Found!", NULL, 0);
		return;
	}
	if (temp_hoststatus == NULL) {
		print_generic_error_message("Error: Host Status Information Not Found!", NULL, 0);
		return;
	}

	/* calculate state duration */
	current_time = time(NULL);
	ts_state_duration = 0;
	duration_error = FALSE;
	if (temp_hoststatus->last_state_change == (time_t)0) {
		if (program_start > current_time)
			duration_error = TRUE;
		else
			ts_state_duration = current_time - program_start;
	} else {
		if (temp_hoststatus->last_state_change > current_time)
			duration_error = TRUE;
		else
			ts_state_duration = current_time - temp_hoststatus->last_state_change;
	}
	get_time_breakdown((unsigned long)ts_state_duration, &days, &hours, &minutes, &seconds);
	if (duration_error == TRUE)
		snprintf(state_duration, sizeof(state_duration) - 1, "???");
	else
		snprintf(state_duration, sizeof(state_duration) - 1, "%2dd %2dh %2dm %2ds%s", days, hours, minutes, seconds, (temp_hoststatus->last_state_change == (time_t)0) ? "+" : "");
	state_duration[sizeof(state_duration) - 1] = '\x0';

	/* calculate state age */
	ts_state_age = 0;
	duration_error = FALSE;
	if (temp_hoststatus->last_check > current_time)
		duration_error = TRUE;
	else
		/*t=current_time-temp_hoststatus->last_check;*/
		ts_state_age = current_time - temp_hoststatus->last_update;
	get_time_breakdown((unsigned long)ts_state_age, &days, &hours, &minutes, &seconds);
	if (duration_error == TRUE)
		snprintf(status_age, sizeof(status_age) - 1, "???");
	else if (temp_hoststatus->last_check == (time_t)0)
		snprintf(status_age, sizeof(status_age) - 1, "N/A");
	else
		snprintf(status_age, sizeof(status_age) - 1, "%2dd %2dh %2dm %2ds", days, hours, minutes, seconds);
	status_age[sizeof(status_age) - 1] = '\x0';

	/* first, we mark and color it as maintenance if that is preferred */
	if (suppress_maintenance_downtime == TRUE && temp_hoststatus->scheduled_downtime_depth > 0) {
		if (temp_hoststatus->status == HOST_UP)
			strcpy(state_string, "UP (MAINTENANCE)");
		else if (temp_hoststatus->status == HOST_DOWN)
			strcpy(state_string, "DOWN (MAINTENANCE)");
		else if (temp_hoststatus->status == HOST_UNREACHABLE)
			strcpy(state_string, "UNREACHABLE (MAINTENANCE)");
		else //catch any other state (just in case)
			strcpy(state_string, "MAINTENANCE");
		bg_class = "hostDOWNTIME";

		/* otherwise we mark and color it with its appropriate state */
	} else if (temp_hoststatus->status == HOST_UP) {
		strcpy(state_string, "UP");
		bg_class = "hostUP";
	} else if (temp_hoststatus->status == HOST_DOWN) {
		strcpy(state_string, "DOWN");
		bg_class = "hostDOWN";
	} else if (temp_hoststatus->status == HOST_UNREACHABLE) {
		strcpy(state_string, "UNREACHABLE");
		bg_class = "hostUNREACHABLE";
	}

	if (content_type == JSON_CONTENT) {
		printf("\"host_info\": {\n");
		printf("\"host_name\": \"%s\",\n", json_encode(host_name));
		printf("\"host_display_name\": \"%s\",\n", (temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(host_name));
		if (temp_hoststatus->has_been_checked == FALSE)
			printf("\"has_been_checked\": false\n");
		else {
			printf("\"has_been_checked\": true,\n");
			printf("\"status\": \"%s\",\n", state_string);
			printf("\"state_type\": \"%s\",\n", (temp_hoststatus->state_type == HARD_STATE) ? "HARD" : "SOFT");
			if (duration_error == TRUE)
				printf("\"status_duration\": false,\n");
			else
				printf("\"status_duration\": \"%s\",\n", state_duration);
			printf("\"status_duration_in_seconds\": %lu,\n", (unsigned long)ts_state_duration);
			if (temp_hoststatus->long_plugin_output != NULL)
				printf("\"status_information\": \"%s\\n%s\",\n", json_encode(temp_hoststatus->plugin_output), json_encode(temp_hoststatus->long_plugin_output));
			else if (temp_hoststatus->plugin_output != NULL)
				printf("\"status_information\": \"%s\",\n", json_encode(temp_hoststatus->plugin_output));
			else
				printf("\"status_information\": null,\n");
			if (temp_hoststatus->perf_data == NULL)
				printf("\"performance_data\": null,\n");
			else
				printf("\"performance_data\": \"%s\",\n", json_encode(temp_hoststatus->perf_data));
			printf("\"current_attempt\": %d,\n", temp_hoststatus->current_attempt);
			printf("\"max_attempts\": %d,\n", temp_hoststatus->max_attempts);

			if (temp_hoststatus->checks_enabled == TRUE)
				printf("\"check_type\": \"active\",\n");
			else if (temp_hoststatus->accept_passive_host_checks == TRUE)
				printf("\"check_type\": \"passive\",\n");
			else
				printf("\"check_type\": \"disabled\",\n");

			get_time_string(&temp_hoststatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("\"last_check_time\": \"%s\",\n", date_time);

			if (temp_hoststatus->checks_enabled == TRUE)
				printf("\"check_latency\": %.3f,\n", temp_hoststatus->latency);
			else
				printf("\"check_latency\": null,\n");

			printf("\"check_duration\": %.3f,\n", temp_hoststatus->execution_time);

			get_time_string(&temp_hoststatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			if (temp_hoststatus->checks_enabled && temp_hoststatus->next_check != (time_t)0 && temp_hoststatus->should_be_scheduled == TRUE)
				printf("\"next_scheduled_active_check\": \"%s\",\n", date_time);
			else
				printf("\"next_scheduled_active_check\": null,\n");

			get_time_string(&temp_hoststatus->last_state_change, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			if (temp_hoststatus->last_state_change == (time_t)0)
				printf("\"last_state_change\": null,\n");
			else
				printf("\"last_state_change\": \"%s\",\n", date_time);

			get_time_string(&temp_hoststatus->last_notification, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			if (temp_hoststatus->last_notification == (time_t)0)
				printf("\"last_notification\": null,\n");
			else
				printf("\"last_notification\": \"%s\",\n", date_time);
			printf("\"current_notification_number\": %d,\n", temp_hoststatus->current_notification_number);
			if (temp_hoststatus->flap_detection_enabled == FALSE || enable_flap_detection == FALSE)
				printf("\"host_is_flapping\": null,\n");
			else
				printf("\"host_is_flapping\": %s,\n", (temp_hoststatus->is_flapping == TRUE) ? "true" : "false");
			printf("\"flapping_percent_state_change\": %3.2f,\n", temp_hoststatus->percent_state_change);
			printf("\"host_in_scheduled_downtime\": %s,\n", (temp_hoststatus->scheduled_downtime_depth > 0) ? "true" : "false");
			printf("\"host_has_been_acknowledged\": %s,\n", (temp_hoststatus->problem_has_been_acknowledged == TRUE) ? "true" : "false");

			get_time_string(&temp_hoststatus->last_update, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("\"last_update\": \"%s\",\n", date_time);

			printf("\"modified_attributes\": \"");
			print_modified_attributes(JSON_CONTENT, EXTINFO_CGI, temp_hoststatus->modified_attributes);
			printf("\",\n");

			printf("\"active_checks_enabled\": %s,\n", (temp_hoststatus->checks_enabled == TRUE) ? "true" : "false");
			printf("\"passive_checks_enabled\": %s,\n", (temp_hoststatus->accept_passive_host_checks == TRUE) ? "true" : "false");
			printf("\"obsess_over_host\": %s,\n", (temp_hoststatus->obsess_over_host == TRUE) ? "true" : "false");
			printf("\"notifications_enabled\": %s,\n", (temp_hoststatus->notifications_enabled == TRUE) ? "true" : "false");
			printf("\"event_handler_enabled\": %s,\n", (temp_hoststatus->event_handler_enabled == TRUE) ? "true" : "false");
			printf("\"flap_detection_enabled\": %s\n", (temp_hoststatus->flap_detection_enabled == TRUE) ? "true" : "false");
			if (is_authorized_for_read_only(&current_authdata) == FALSE || is_authorized_for_comments_read_only(&current_authdata) == TRUE) {

				/* display comments */
				printf(",\n");
				show_comments(HOST_COMMENT);

				/* display downtimes */
				printf(",\n");
				show_downtime(HOST_DOWNTIME);
			}
			printf(" }\n");
		}
	} else {
		printf("<TABLE BORDER=0 CELLPADDING=0 WIDTH='100%%' align='center'>\n");
		printf("<TR>\n");

		printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel'>\n");

		printf("<DIV CLASS='dataTitle'>Host State Information</DIV>\n");

		if (temp_hoststatus->has_been_checked == FALSE)
			printf("<DIV ALIGN=CENTER>This host has not yet been checked, so status information is not available.</DIV>\n");

		else {

			printf("<TABLE BORDER=0>\n");
			printf("<TR><TD>\n");

			printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
			printf("<TR><TD class='stateInfoTable1'>\n");
			printf("<TABLE BORDER=0>\n");

			printf("<TR><TD CLASS='dataVar'>Host Status:</td><td CLASS='dataVal'><DIV CLASS='%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV>&nbsp;(for %s)%s</td></tr>\n", bg_class, state_string, state_duration, (temp_hoststatus->problem_has_been_acknowledged == TRUE) ? "&nbsp;&nbsp;(Has been acknowledged)" : "");

			printf("<TR><TD CLASS='dataVar' VALIGN='top'>Status Information:</td><td CLASS='dataVal'>%s", (temp_hoststatus->plugin_output == NULL) ? "" : html_encode(temp_hoststatus->plugin_output, TRUE));
			if (enable_splunk_integration == TRUE) {
				printf("&nbsp;&nbsp;");
				dummy = asprintf(&buf, "%s %s", temp_host->name, temp_hoststatus->plugin_output);
				buf[sizeof(buf) - 1] = '\x0';
				display_splunk_generic_url(buf, 1);
				free(buf);
			}
			if (temp_hoststatus->long_plugin_output != NULL)
				printf("<BR>%s", html_encode(temp_hoststatus->long_plugin_output, TRUE));
			printf("</TD></TR>\n");

			printf("<TR><TD CLASS='dataVar' VALIGN='top'>Performance Data:</td><td CLASS='dataVal'>%s</td></tr>\n", (temp_hoststatus->perf_data == NULL) ? "" : html_encode(temp_hoststatus->perf_data, TRUE));

			printf("<TR><TD CLASS='dataVar'>Current Attempt:</TD><TD CLASS='dataVal'>%d/%d", temp_hoststatus->current_attempt, temp_hoststatus->max_attempts);
			printf("&nbsp;&nbsp;(%s state)</TD></TR>\n", (temp_hoststatus->state_type == HARD_STATE) ? "HARD" : "SOFT");

			get_time_string(&temp_hoststatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Last Check Time:</td><td CLASS='dataVal'>%s</td></tr>\n", date_time);

			if (temp_hoststatus->checks_enabled == TRUE)
				printf("<TR><TD CLASS='dataVar'>Check Type:</TD><TD CLASS='dataVal'><A HREF='%s?type=command&host=%s&expand=%s'>ACTIVE</A></TD></TR>\n", CONFIG_CGI, host_name, url_encode(temp_host->host_check_command));
			else if (temp_hoststatus->accept_passive_host_checks == TRUE)
				printf("<TR><TD CLASS='dataVar'>Check Type:</TD><TD CLASS='dataVal'>PASSIVE</TD></TR>\n");
			else
				printf("<TR><TD CLASS='dataVar'>Check Type:</TD><TD CLASS='dataVal'>DISABLED</TD></TR>\n");

			printf("<TR><TD CLASS='dataVar' NOWRAP>Check Latency / Duration:</TD><TD CLASS='dataVal'>");
			if (temp_hoststatus->checks_enabled == TRUE)
				printf("%.3f", temp_hoststatus->latency);
			else
				printf("N/A");
			printf("&nbsp;/&nbsp;%.3f seconds", temp_hoststatus->execution_time);
			printf("</TD></TR>\n");

			get_time_string(&temp_hoststatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Next Scheduled Active Check:&nbsp;&nbsp;</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (temp_hoststatus->checks_enabled && temp_hoststatus->next_check != (time_t)0 && temp_hoststatus->should_be_scheduled == TRUE) ? date_time : "N/A");

			get_time_string(&temp_hoststatus->last_state_change, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Last State Change:</td><td CLASS='dataVal'>%s</td></tr>\n", (temp_hoststatus->last_state_change == (time_t)0) ? "N/A" : date_time);

			get_time_string(&temp_hoststatus->last_notification, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Last Notification:</td><td CLASS='dataVal'>%s&nbsp;(notification %d)</td></tr>\n", (temp_hoststatus->last_notification == (time_t)0) ? "N/A" : date_time, temp_hoststatus->current_notification_number);

			printf("<TR><TD CLASS='dataVar'>Is This Host Flapping?</td><td CLASS='dataVal'>");
			if (temp_hoststatus->flap_detection_enabled == FALSE || enable_flap_detection == FALSE)
				printf("N/A");
			else
				printf("<DIV CLASS='%sflapping'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV>&nbsp;(%3.2f%% state change)", (temp_hoststatus->is_flapping == TRUE) ? "" : "not", (temp_hoststatus->is_flapping == TRUE) ? "YES" : "NO", temp_hoststatus->percent_state_change);
			printf("</td></tr>\n");

			printf("<TR><TD CLASS='dataVar'>In Scheduled Downtime?</td><td CLASS='dataVal'><DIV CLASS='downtime%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", (temp_hoststatus->scheduled_downtime_depth > 0) ? "ACTIVE" : "INACTIVE", (temp_hoststatus->scheduled_downtime_depth > 0) ? "YES" : "NO");


			get_time_string(&temp_hoststatus->last_update, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Last Update:</td><td CLASS='dataVal'>%s&nbsp;&nbsp;(%s ago)</td></tr>\n", (temp_hoststatus->last_update == (time_t)0) ? "N/A" : date_time, status_age);

			printf("<TR><TD CLASS='dataVar'>Modified Attributes:</td><td CLASS='dataVal'>");
			print_modified_attributes(HTML_CONTENT, EXTINFO_CGI, temp_hoststatus->modified_attributes);
			printf("</td></tr>\n");

			printf("</TABLE>\n");
			printf("</TD></TR>\n");
			printf("</TABLE>\n");

			printf("</TD></TR>\n");
			printf("<TR><TD>\n");

			printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0 align='left'>\n");
			printf("<TR><TD class='stateInfoTable2'>\n");
			printf("<TABLE BORDER=0>\n");

			if ((temp_host->host_check_command) && (*temp_host->host_check_command != '\0'))
				printf("<TR><TD CLASS='dataVar'><A HREF='%s?type=command&expand=%s'>Active Checks:</A></TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", CONFIG_CGI, url_encode(temp_host->host_check_command), (temp_hoststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED");
			else printf("<TR><TD CLASS='dataVar'>Active Checks:</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_hoststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED");

			printf("<TR><TD CLASS='dataVar'>Passive Checks:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_hoststatus->accept_passive_host_checks == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->accept_passive_host_checks) ? "ENABLED" : "DISABLED");

			printf("<TR><TD CLASS='dataVar'>Obsessing:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_hoststatus->obsess_over_host == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->obsess_over_host) ? "ENABLED" : "DISABLED");

			printf("<TR><TD CLASS='dataVar'>Notifications:</td><td CLASS='dataVal'><DIV CLASS='notifications%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", (temp_hoststatus->notifications_enabled) ? "ENABLED" : "DISABLED", (temp_hoststatus->notifications_enabled) ? "ENABLED" : "DISABLED");

			if ((temp_host->event_handler) && (*temp_host->event_handler != '\0'))
				printf("<TR><TD CLASS='dataVar'><A HREF='%s?type=command&expand=%s'>Event Handler:</A></td><td CLASS='dataVal'><DIV CLASS='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", CONFIG_CGI, url_encode(temp_host->event_handler), (temp_hoststatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_hoststatus->event_handler_enabled) ? "ENABLED" : "DISABLED");
			else printf("<TR><TD CLASS='dataVar'>Event Handler:</td><td CLASS='dataVal'><DIV CLASS='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", (temp_hoststatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_hoststatus->event_handler_enabled) ? "ENABLED" : "DISABLED");


			printf("<TR><TD CLASS='dataVar'>Flap Detection:</td><td CLASS='dataVal'><DIV CLASS='flapdetection%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", (temp_hoststatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED");

			printf("</TABLE>\n");
			printf("</TD></TR>\n");
			printf("</TABLE>\n");

			printf("</TD></TR>\n");
			printf("</TABLE>\n");
		}

		printf("</TD>\n");

		printf("<TD ALIGN=CENTER VALIGN=TOP>\n");
		printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0><TR>\n");

		printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='commandPanel'>\n");

		printf("<DIV CLASS='commandTitle'>Host Commands</DIV>\n");

		printf("<TABLE BORDER='1' CELLPADDING=0 CELLSPACING=0><TR><TD>\n");

		if (nagios_process_state == STATE_OK && is_authorized_for_read_only(&current_authdata) == FALSE) {

			printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS='command'>\n");
#ifdef USE_STATUSMAP
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Locate Host On Map' TITLE='Locate Host On Map'></td><td CLASS='command'><a href='%s?host=%s'>Locate host on map</a></td></tr>\n", url_images_path, STATUSMAP_ICON, STATUSMAP_CGI, url_encode(host_name));
#endif
			if (temp_hoststatus->checks_enabled == TRUE) {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Active Checks Of This Host' TITLE='Disable Active Checks Of This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Disable active checks of this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_CHECK, url_encode(host_name));
			} else
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Active Checks Of This Host' TITLE='Enable Active Checks Of This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Enable active checks of this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_CHECK, url_encode(host_name));
			printf("<tr CLASS='data'><td><img src='%s%s' border=0 ALT='Re-schedule Next Host Check' TITLE='Re-schedule Next Host Check'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s%s'>Re-schedule the next check of this host</a></td></tr>\n", url_images_path, DELAY_ICON, CMD_CGI, CMD_SCHEDULE_HOST_CHECK, url_encode(host_name), (temp_hoststatus->checks_enabled == TRUE) ? "&force_check" : "");

			if (temp_hoststatus->accept_passive_host_checks == TRUE) {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Submit Passive Check Result For This Host' TITLE='Submit Passive Check Result For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Submit passive check result for this host</a></td></tr>\n", url_images_path, PASSIVE_ICON, CMD_CGI, CMD_PROCESS_HOST_CHECK_RESULT, url_encode(host_name));
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Stop Accepting Passive Checks For This Host' TITLE='Stop Accepting Passive Checks For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Stop accepting passive checks for this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_PASSIVE_HOST_CHECKS, url_encode(host_name));
			} else
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Start Accepting Passive Checks For This Host' TITLE='Start Accepting Passive Checks For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Start accepting passive checks for this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_PASSIVE_HOST_CHECKS, url_encode(host_name));

			if (temp_hoststatus->obsess_over_host == TRUE)
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Stop Obsessing Over This Host' TITLE='Stop Obsessing Over This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Stop obsessing over this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_OBSESSING_OVER_HOST, url_encode(host_name));
			else
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Start Obsessing Over This Host' TITLE='Start Obsessing Over This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Start obsessing over this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_OBSESSING_OVER_HOST, url_encode(host_name));

			if (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE) {
				if (temp_hoststatus->problem_has_been_acknowledged == FALSE)
					printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Acknowledge This Host Problem' TITLE='Acknowledge This Host Problem'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Acknowledge this host problem</a></td></tr>\n", url_images_path, ACKNOWLEDGEMENT_ICON, CMD_CGI, CMD_ACKNOWLEDGE_HOST_PROBLEM, url_encode(host_name));
				else
					printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Remove Problem Acknowledgement' TITLE='Remove Problem Acknowledgement'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Remove problem acknowledgement</a></td></tr>\n", url_images_path, REMOVE_ACKNOWLEDGEMENT_ICON, CMD_CGI, CMD_REMOVE_HOST_ACKNOWLEDGEMENT, url_encode(host_name));
			}

			if (temp_hoststatus->notifications_enabled == TRUE)
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Notifications For This Host' TITLE='Disable Notifications For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Disable notifications for this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_NOTIFICATIONS, url_encode(host_name));
			else
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Notifications For This Host' TITLE='Enable Notifications For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Enable notifications for this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_NOTIFICATIONS, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Send Custom Notification' TITLE='Send Custom Notification'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Send custom host notification</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_SEND_CUSTOM_HOST_NOTIFICATION, url_encode(host_name));

			if (temp_hoststatus->status != HOST_UP)
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Delay Next Host Notification' TITLE='Delay Next Host Notification'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Delay next host notification</a></td></tr>\n", url_images_path, DELAY_ICON, CMD_CGI, CMD_DELAY_HOST_NOTIFICATION, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Schedule Downtime For This Host' TITLE='Schedule Downtime For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Schedule downtime for this host</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_HOST_DOWNTIME, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Schedule Downtime For This Host and All Services' TITLE='Schedule Downtime For This Host and All Services'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Schedule downtime for this host and all services</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_HOST_SVC_DOWNTIME, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Remove Downtime(s) for this host and all services' TITLE='Remove Downtime(s) for this host and all services'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Remove Downtime(s) for this host and all services</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DEL_DOWNTIME_BY_HOST_NAME, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Notifications For All Services On This Host' TITLE='Disable Notifications For All Services On This Host'></td><td CLASS='command' NOWRAP><a href='%s?cmd_typ=%d&host=%s'>Disable notifications for all services on this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_SVC_NOTIFICATIONS, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Notifications For All Services On This Host' TITLE='Enable Notifications For All Services On This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Enable notifications for all services on this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_SVC_NOTIFICATIONS, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Schedule A Check Of All Services On This Host' TITLE='Schedule A Check Of All Services On This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Schedule a check of all services on this host</a></td></tr>\n", url_images_path, DELAY_ICON, CMD_CGI, CMD_SCHEDULE_HOST_SVC_CHECKS, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Checks Of All Services On This Host' TITLE='Disable Checks Of All Services On This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Disable checks of all services on this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_SVC_CHECKS, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Checks Of All Services On This Host' TITLE='Enable Checks Of All Services On This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Enable checks of all services on this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_SVC_CHECKS, url_encode(host_name));

			if (temp_hoststatus->event_handler_enabled == TRUE)
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Event Handler For This Host' TITLE='Disable Event Handler For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Disable event handler for this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_EVENT_HANDLER, url_encode(host_name));
			else
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Event Handler For This Host' TITLE='Enable Event Handler For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Enable event handler for this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_EVENT_HANDLER, url_encode(host_name));
			if (temp_hoststatus->flap_detection_enabled == TRUE)
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Flap Detection For This Host' TITLE='Disable Flap Detection For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Disable flap detection for this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_FLAP_DETECTION, url_encode(host_name));
			else
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Flap Detection For This Host' TITLE='Enable Flap Detection For This Host'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>Enable flap detection for this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_FLAP_DETECTION, url_encode(host_name));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Add a new Host comment' TITLE='Add a new Host comment'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>", url_images_path, COMMENT_ICON, CMD_CGI, CMD_ADD_HOST_COMMENT, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name));
			printf("Add a new Host comment</a></td>");

			/* allow modified attributes to be reset */
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Reset Modified Attributes' TITLE='Reset Modified Attributes'></td><td CLASS='command'><a href='%s?cmd_typ=%d&attr=%d&host=%s'>", url_images_path, DISABLED_ICON, CMD_CGI, CMD_CHANGE_HOST_MODATTR, MODATTR_NONE, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name));
			printf("Reset Modified Attributes</a></td>");

			printf("</TABLE>\n");
		} else if (is_authorized_for_read_only(&current_authdata) == TRUE) {
			printf("<DIV ALIGN=CENTER CLASS='infoMessage'>Your account does not have permissions to execute commands.<br>\n");
		} else {
			printf("<DIV ALIGN=CENTER CLASS='infoMessage'>It appears as though Icinga is not running, so commands are temporarily unavailable...<br>\n");
			printf("Click <a href='%s?type=%d'>here</a> to view Icinga process information</DIV>\n", EXTINFO_CGI, DISPLAY_PROCESS_INFO);
		}
		printf("</TD></TR></TABLE>\n");

		printf("</TD>\n");

		printf("</TR>\n");
		printf("</TABLE></TR>\n");

		printf("<TR>\n");

		printf("<TD COLSPAN=2 VALIGN=TOP CLASS='commentPanel'>\n");

		if (is_authorized_for_read_only(&current_authdata) == FALSE || is_authorized_for_comments_read_only(&current_authdata) == TRUE) {
			/* display comments */
			show_comments(HOST_COMMENT);
			printf("<BR>");
			/* display downtimes */
			show_downtime(HOST_DOWNTIME);
		}

		printf("</TD>\n");

		printf("</TR>\n");
		printf("</TABLE>\n");
	}

	return;
}

void show_service_info(void) {
	service *temp_service;
	host *temp_host;
	char date_time[MAX_DATETIME_LENGTH];
	char status_age[48];
	char state_duration[48];
	servicestatus *temp_svcstatus;
	char state_string[MAX_INPUT_BUFFER];
	char *bg_class = "";
	char *buf = NULL;
	int days;
	int hours;
	int minutes;
	int seconds;
	time_t ts_state_duration = 0L;
	time_t ts_state_age = 0L;
	time_t current_time;
	int duration_error = FALSE;

	/* get host info */
	temp_host = find_host(host_name);

	/* find the service */
	temp_service = find_service(host_name, service_desc);

	/* make sure the user has rights to view service information */
	if (is_authorized_for_service(temp_service, &current_authdata) == FALSE) {

		print_generic_error_message("It appears as though you do not have permission to view information for this service...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);

		return;
	}

	/* get service status info */
	temp_svcstatus = find_servicestatus(host_name, service_desc);

	/* make sure service information exists */
	if (temp_service == NULL) {
		print_generic_error_message("Error: Service Not Found!", NULL, 0);
		return;
	}
	if (temp_svcstatus == NULL) {
		print_generic_error_message("Error: Service Status Not Found!", NULL, 0);
		return;
	}


	current_time = time(NULL);
	duration_error = FALSE;
	if (temp_svcstatus->last_state_change == (time_t)0) {
		if (program_start > current_time)
			duration_error = TRUE;
		else
			ts_state_duration = current_time - program_start;
	} else {
		if (temp_svcstatus->last_state_change > current_time)
			duration_error = TRUE;
		else
			ts_state_duration = current_time - temp_svcstatus->last_state_change;
	}
	get_time_breakdown((unsigned long)ts_state_duration, &days, &hours, &minutes, &seconds);
	if (duration_error == TRUE)
		snprintf(state_duration, sizeof(state_duration) - 1, "???");
	else
		snprintf(state_duration, sizeof(state_duration) - 1, "%2dd %2dh %2dm %2ds%s", days, hours, minutes, seconds, (temp_svcstatus->last_state_change == (time_t)0) ? "+" : "");
	state_duration[sizeof(state_duration) - 1] = '\x0';

	/* first, we mark and color it as maintenance if that is preferred */
	if (suppress_maintenance_downtime == TRUE && temp_svcstatus->scheduled_downtime_depth > 0) {
		strcpy(state_string, "MAINTENANCE");
		bg_class = "serviceDOWNTIME";

		/* otherwise we mark and color it with its appropriate state */
	} else if (temp_svcstatus->status == SERVICE_OK) {
		strcpy(state_string, "OK");
		bg_class = "serviceOK";
	} else if (temp_svcstatus->status == SERVICE_WARNING) {
		strcpy(state_string, "WARNING");
		bg_class = "serviceWARNING";
	} else if (temp_svcstatus->status == SERVICE_CRITICAL) {
		strcpy(state_string, "CRITICAL");
		bg_class = "serviceCRITICAL";
	} else {
		strcpy(state_string, "UNKNOWN");
		bg_class = "serviceUNKNOWN";
	}

	duration_error = FALSE;
	if (temp_svcstatus->last_check > current_time)
		duration_error = TRUE;
	else
		ts_state_age = current_time - temp_svcstatus->last_update;
	get_time_breakdown((unsigned long)ts_state_age, &days, &hours, &minutes, &seconds);
	if (duration_error == TRUE)
		snprintf(status_age, sizeof(status_age) - 1, "???");
	else if (temp_svcstatus->last_check == (time_t)0)
		snprintf(status_age, sizeof(status_age) - 1, "N/A");
	else
		snprintf(status_age, sizeof(status_age) - 1, "%2dd %2dh %2dm %2ds", days, hours, minutes, seconds);
	status_age[sizeof(status_age) - 1] = '\x0';

	if (content_type == JSON_CONTENT) {
		printf("\"service_info\": {\n");
		printf("\"host_name\": \"%s\",\n", json_encode(host_name));
		printf("\"host_display_name\": \"%s\",\n", (temp_host != NULL && temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(host_name));
		printf("\"service_description\": \"%s\",\n", json_encode(service_desc));
		printf("\"service_display_name\": \"%s\",\n", (temp_service->display_name != NULL) ? json_encode(temp_service->display_name) : json_encode(service_desc));
		if (temp_svcstatus->has_been_checked == FALSE)
			printf("\"has_been_checked\": false\n");
		else {
			printf("\"has_been_checked\": true,\n");
			printf("\"status\": \"%s\",\n", state_string);
			printf("\"state_type\": \"%s\",\n", (temp_svcstatus->state_type == HARD_STATE) ? "HARD" : "SOFT");
			if (duration_error == TRUE)
				printf("\"status_duration\": false,\n");
			else
				printf("\"status_duration\": \"%s\",\n", state_duration);
			printf("\"status_duration_in_seconds\": %lu,\n", (unsigned long)ts_state_duration);
			if (temp_svcstatus->long_plugin_output != NULL)
				printf("\"status_information\": \"%s\\n%s\",\n", json_encode(temp_svcstatus->plugin_output), json_encode(temp_svcstatus->long_plugin_output));
			else if (temp_svcstatus->plugin_output != NULL)
				printf("\"status_information\": \"%s\",\n", json_encode(temp_svcstatus->plugin_output));
			else
				printf("\"status_information\": null,\n");
			if (temp_svcstatus->perf_data == NULL)
				printf("\"performance_data\": null,\n");
			else
				printf("\"performance_data\": \"%s\",\n", json_encode(temp_svcstatus->perf_data));
			printf("\"current_attempt\": %d,\n", temp_svcstatus->current_attempt);
			printf("\"max_attempts\": %d,\n", temp_svcstatus->max_attempts);

			if (temp_svcstatus->checks_enabled == TRUE)
				printf("\"check_type\": \"active\",\n");
			else if (temp_svcstatus->accept_passive_service_checks == TRUE)
				printf("\"check_type\": \"passive\",\n");
			else
				printf("\"check_type\": \"disabled\",\n");

			get_time_string(&temp_svcstatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("\"last_check_time\": \"%s\",\n", date_time);

			if (temp_svcstatus->checks_enabled == TRUE)
				printf("\"check_latency\": %.3f,\n", temp_svcstatus->latency);
			else
				printf("\"check_latency\": null,\n");
			printf("\"check_duration\": %.3f,\n", temp_svcstatus->execution_time);

			get_time_string(&temp_svcstatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			if (temp_svcstatus->checks_enabled && temp_svcstatus->next_check != (time_t)0 && temp_svcstatus->should_be_scheduled == TRUE)
				printf("\"next_scheduled_active_check\": \"%s\",\n", date_time);
			else
				printf("\"next_scheduled_active_check\": null,\n");

			get_time_string(&temp_svcstatus->last_state_change, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			if (temp_svcstatus->last_state_change == (time_t)0)
				printf("\"last_state_change\": null,\n");
			else
				printf("\"last_state_change\": \"%s\",\n", date_time);

			get_time_string(&temp_svcstatus->last_notification, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			if (temp_svcstatus->last_notification == (time_t)0)
				printf("\"last_notification\": null,\n");
			else
				printf("\"last_notification\": \"%s\",\n", date_time);
			printf("\"current_notification_number\": %d,\n", temp_svcstatus->current_notification_number);
			if (temp_svcstatus->flap_detection_enabled == FALSE || enable_flap_detection == FALSE)
				printf("\"service_is_flapping\": null,\n");
			else
				printf("\"service_is_flapping\": %s,\n", (temp_svcstatus->is_flapping == TRUE) ? "true" : "false");
			printf("\"flapping_percent_state_change\": %3.2f,\n", temp_svcstatus->percent_state_change);
			printf("\"service_in_scheduled_downtime\": %s,\n", (temp_svcstatus->scheduled_downtime_depth > 0) ? "true" : "false");
			printf("\"service_has_been_acknowledged\": %s,\n", (temp_svcstatus->problem_has_been_acknowledged == TRUE) ? "true" : "false");

			get_time_string(&temp_svcstatus->last_update, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("\"last_update\": \"%s\",\n", date_time);

			printf("\"modified_attributes\": \"");
			print_modified_attributes(JSON_CONTENT, EXTINFO_CGI, temp_svcstatus->modified_attributes);
			printf("\",\n");

			printf("\"active_checks_enabled\": %s,\n", (temp_svcstatus->checks_enabled == TRUE) ? "true" : "false");
			printf("\"passive_checks_enabled\": %s,\n", (temp_svcstatus->accept_passive_service_checks == TRUE) ? "true" : "false");
			printf("\"obsess_over_service\": %s,\n", (temp_svcstatus->obsess_over_service == TRUE) ? "true" : "false");
			printf("\"notifications_enabled\": %s,\n", (temp_svcstatus->notifications_enabled == TRUE) ? "true" : "false");
			printf("\"event_handler_enabled\": %s,\n", (temp_svcstatus->event_handler_enabled == TRUE) ? "true" : "false");
			printf("\"flap_detection_enabled\": %s\n", (temp_svcstatus->flap_detection_enabled == TRUE) ? "true" : "false");
			if (is_authorized_for_read_only(&current_authdata) == FALSE || is_authorized_for_comments_read_only(&current_authdata) == TRUE) {

				/* display comments */
				printf(",\n");
				show_comments(SERVICE_COMMENT);

				/* display downtimes */
				printf(",\n");
				show_downtime(SERVICE_DOWNTIME);
			}
			printf(" }\n");
		}
	} else {
		printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 WIDTH=100%%>\n");
		printf("<TR>\n");

		printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel'>\n");

		printf("<DIV CLASS='dataTitle'>Service State Information</DIV>\n");

		if (temp_svcstatus->has_been_checked == FALSE)
			printf("<P><DIV ALIGN=CENTER>This service has not yet been checked, so status information is not available.</DIV></P>\n");

		else {

			printf("<TABLE BORDER=0>\n");

			printf("<TR><TD>\n");

			printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
			printf("<TR><TD class='stateInfoTable1'>\n");
			printf("<TABLE BORDER=0>\n");

			printf("<TR><TD CLASS='dataVar'>Current Status:</TD><TD CLASS='dataVal'><DIV CLASS='%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV>&nbsp;(for %s)%s</TD></TR>\n", bg_class, state_string, state_duration, (temp_svcstatus->problem_has_been_acknowledged == TRUE) ? "&nbsp;&nbsp;(Has been acknowledged)" : "");

			printf("<TR><TD CLASS='dataVar' VALIGN='top'>Status Information:</TD><TD CLASS='dataVal'>%s", (temp_svcstatus->plugin_output == NULL) ? "" : html_encode(temp_svcstatus->plugin_output, TRUE));
			if (enable_splunk_integration == TRUE) {
				printf("&nbsp;&nbsp;");
				dummy = asprintf(&buf, "%s %s %s", temp_service->host_name, temp_service->description, temp_svcstatus->plugin_output);
				buf[sizeof(buf) - 1] = '\x0';
				display_splunk_generic_url(buf, 1);
				free(buf);
			}
			if (temp_svcstatus->long_plugin_output != NULL)
				printf("<BR>%s", html_encode(temp_svcstatus->long_plugin_output, TRUE));
			printf("</TD></TR>\n");

			printf("<TR><TD CLASS='dataVar' VALIGN='top'>Performance Data:</td><td CLASS='dataVal'>%s</td></tr>\n", (temp_svcstatus->perf_data == NULL) ? "" : html_encode(temp_svcstatus->perf_data, TRUE));

			printf("<TR><TD CLASS='dataVar'>Current Attempt:</TD><TD CLASS='dataVal'>%d/%d", temp_svcstatus->current_attempt, temp_svcstatus->max_attempts);
			printf("&nbsp;&nbsp;(%s state)</TD></TR>\n", (temp_svcstatus->state_type == HARD_STATE) ? "HARD" : "SOFT");

			get_time_string(&temp_svcstatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Last Check Time:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", date_time);

			if (temp_svcstatus->checks_enabled == TRUE)
				printf("<TR><TD CLASS='dataVar'>Check Type:</TD><TD CLASS='dataVal'><A HREF='%s?type=command&host=%s&service=%s&expand=%s'>ACTIVE</A></TD></TR>\n",
				       CONFIG_CGI, host_name, service_desc, url_encode(temp_service->service_check_command));
			else if (temp_svcstatus->accept_passive_service_checks == TRUE)
				printf("<TR><TD CLASS='dataVar'>Check Type:</TD><TD CLASS='dataVal'>PASSIVE</TD></TR>\n");
			else
				printf("<TR><TD CLASS='dataVar'>Check Type:</TD><TD CLASS='dataVal'>DISABLED</TD></TR>\n");

			printf("<TR><TD CLASS='dataVar' NOWRAP>Check Latency / Duration:</TD><TD CLASS='dataVal'>");
			if (temp_svcstatus->checks_enabled == TRUE)
				printf("%.3f", temp_svcstatus->latency);
			else
				printf("N/A");
			printf("&nbsp;/&nbsp;%.3f seconds", temp_svcstatus->execution_time);
			printf("</TD></TR>\n");

			get_time_string(&temp_svcstatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Next Scheduled Check:&nbsp;&nbsp;</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (temp_svcstatus->checks_enabled && temp_svcstatus->next_check != (time_t)0 && temp_svcstatus->should_be_scheduled == TRUE) ? date_time : "N/A");

			get_time_string(&temp_svcstatus->last_state_change, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Last State Change:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (temp_svcstatus->last_state_change == (time_t)0) ? "N/A" : date_time);

			get_time_string(&temp_svcstatus->last_notification, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Last Notification:</TD><TD CLASS='dataVal'>%s&nbsp;(notification %d)</TD></TR>\n", (temp_svcstatus->last_notification == (time_t)0) ? "N/A" : date_time, temp_svcstatus->current_notification_number);

			printf("<TR><TD CLASS='dataVar'>Is This Service Flapping?</TD><TD CLASS='dataVal'>");
			if (temp_svcstatus->flap_detection_enabled == FALSE || enable_flap_detection == FALSE)
				printf("N/A");
			else
				printf("<DIV CLASS='%sflapping'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV>&nbsp;(%3.2f%% state change)", (temp_svcstatus->is_flapping == TRUE) ? "" : "not", (temp_svcstatus->is_flapping == TRUE) ? "YES" : "NO", temp_svcstatus->percent_state_change);
			printf("</TD></TR>\n");

			printf("<TR><TD CLASS='dataVar'>In Scheduled Downtime?</TD><TD CLASS='dataVal'><DIV CLASS='downtime%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->scheduled_downtime_depth > 0) ? "ACTIVE" : "INACTIVE", (temp_svcstatus->scheduled_downtime_depth > 0) ? "YES" : "NO");


			get_time_string(&temp_svcstatus->last_update, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TR><TD CLASS='dataVar'>Last Update:</TD><TD CLASS='dataVal'>%s&nbsp;&nbsp;(%s ago)</TD></TR>\n", (temp_svcstatus->last_update == (time_t)0) ? "N/A" : date_time, status_age);

			printf("<TR><TD CLASS='dataVar'>Modified Attributes:</td><td CLASS='dataVal'>");
			print_modified_attributes(HTML_CONTENT, EXTINFO_CGI, temp_svcstatus->modified_attributes);
			printf("</td></tr>\n");

			printf("</TABLE>\n");
			printf("</TD></TR>\n");
			printf("</TABLE>\n");

			printf("</TD></TR>\n");

			printf("<TR><TD>\n");

			printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0 align='left'>\n");
			printf("<TR><TD class='stateInfoTable2'>\n");
			printf("<TABLE BORDER=0>\n");

			if ((temp_service->service_check_command) && (*temp_service->service_check_command != '\0'))
				printf("<TR><TD CLASS='dataVar'><A HREF='%s?type=command&expand=%s'>Active Checks:</A></TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", CONFIG_CGI, url_encode(temp_service->service_check_command), (temp_svcstatus->checks_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->checks_enabled) ? "ENABLED" : "DISABLED");
			else printf("<TR><TD CLASS='dataVar'>Active Checks:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->checks_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->checks_enabled) ? "ENABLED" : "DISABLED");

			printf("<TR><TD CLASS='dataVar'>Passive Checks:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->accept_passive_service_checks == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->accept_passive_service_checks) ? "ENABLED" : "DISABLED");

			printf("<TR><TD CLASS='dataVar'>Obsessing:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->obsess_over_service == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->obsess_over_service) ? "ENABLED" : "DISABLED");

			printf("<TR><td CLASS='dataVar'>Notifications:</TD><td CLASS='dataVal'><DIV CLASS='notifications%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->notifications_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->notifications_enabled) ? "ENABLED" : "DISABLED");

			if ((temp_service->event_handler) && (*temp_service->event_handler != '\0'))
				printf("<TR><TD CLASS='dataVar'><A HREF='%s?type=command&expand=%s'>Event Handler:</A></td><td CLASS='dataVal'><DIV CLASS='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", CONFIG_CGI, url_encode(temp_service->event_handler), (temp_svcstatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->event_handler_enabled) ? "ENABLED" : "DISABLED");
			else printf("<TR><TD CLASS='dataVar'>Event Handler:</td><td CLASS='dataVal'><DIV CLASS='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", (temp_svcstatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->event_handler_enabled) ? "ENABLED" : "DISABLED");

			printf("<TR><TD CLASS='dataVar'>Flap Detection:</TD><td CLASS='dataVal'><DIV CLASS='flapdetection%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED");


			printf("</TABLE>\n");
			printf("</TD></TR>\n");
			printf("</TABLE>\n");

			printf("</TD></TR>\n");

			printf("</TABLE>\n");
		}


		printf("</TD>\n");

		printf("<TD ALIGN=CENTER VALIGN=TOP>\n");
		printf("<TABLE BORDER='0' CELLPADDING=0 CELLSPACING=0><TR>\n");

		printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='commandPanel'>\n");

		printf("<DIV CLASS='dataTitle'>Service Commands</DIV>\n");

		printf("<TABLE BORDER='1' CELLSPACING=0 CELLPADDING=0>\n");
		printf("<TR><TD>\n");

		if (nagios_process_state == STATE_OK &&  is_authorized_for_read_only(&current_authdata) == FALSE) {
			printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS='command'>\n");

			if (temp_svcstatus->checks_enabled) {

				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Active Checks Of This Service' TITLE='Disable Active Checks Of This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SVC_CHECK, url_encode(host_name));
				printf("&service=%s'>Disable active checks of this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Active Checks Of This Service' TITLE='Enable Active Checks Of This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SVC_CHECK, url_encode(host_name));
				printf("&service=%s'>Enable active checks of this service</a></td></tr>\n", url_encode(service_desc));
			}
			printf("<tr CLASS='data'><td><img src='%s%s' border=0 ALT='Re-schedule Next Service Check' TITLE='Re-schedule Next Service Check'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DELAY_ICON, CMD_CGI, CMD_SCHEDULE_SVC_CHECK, url_encode(host_name));
			printf("&service=%s%s'>Re-schedule the next check of this service</a></td></tr>\n", url_encode(service_desc), (temp_svcstatus->checks_enabled == TRUE) ? "&force_check" : "");

			if (temp_svcstatus->accept_passive_service_checks == TRUE) {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Submit Passive Check Result For This Service' TITLE='Submit Passive Check Result For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, PASSIVE_ICON, CMD_CGI, CMD_PROCESS_SERVICE_CHECK_RESULT, url_encode(host_name));
				printf("&service=%s'>Submit passive check result for this service</a></td></tr>\n", url_encode(service_desc));

				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Stop Accepting Passive Checks For This Service' TITLE='Stop Accepting Passive Checks For This Service'></td><td CLASS='command' NOWRAP><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_PASSIVE_SVC_CHECKS, url_encode(host_name));
				printf("&service=%s'>Stop accepting passive checks for this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Start Accepting Passive Checks For This Service' TITLE='Start Accepting Passive Checks For This Service'></td><td CLASS='command' NOWRAP><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_PASSIVE_SVC_CHECKS, url_encode(host_name));
				printf("&service=%s'>Start accepting passive checks for this service</a></td></tr>\n", url_encode(service_desc));
			}

			if (temp_svcstatus->obsess_over_service == TRUE) {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Stop Obsessing Over This Service' TITLE='Stop Obsessing Over This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_OBSESSING_OVER_SVC, url_encode(host_name));
				printf("&service=%s'>Stop obsessing over this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Start Obsessing Over This Service' TITLE='Start Obsessing Over This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_OBSESSING_OVER_SVC, url_encode(host_name));
				printf("&service=%s'>Start obsessing over this service</a></td></tr>\n", url_encode(service_desc));
			}

			if ((temp_svcstatus->status == SERVICE_WARNING || temp_svcstatus->status == SERVICE_UNKNOWN || temp_svcstatus->status == SERVICE_CRITICAL) && temp_svcstatus->state_type == HARD_STATE) {
				if (temp_svcstatus->problem_has_been_acknowledged == FALSE) {
					printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Acknowledge This Service Problem' TITLE='Acknowledge This Service Problem'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ACKNOWLEDGEMENT_ICON, CMD_CGI, CMD_ACKNOWLEDGE_SVC_PROBLEM, url_encode(host_name));
					printf("&service=%s'>Acknowledge this service problem</a></td></tr>\n", url_encode(service_desc));
				} else {
					printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Remove Problem Acknowledgement' TITLE='Remove Problem Acknowledgement'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, REMOVE_ACKNOWLEDGEMENT_ICON, CMD_CGI, CMD_REMOVE_SVC_ACKNOWLEDGEMENT, url_encode(host_name));
					printf("&service=%s'>Remove problem acknowledgement</a></td></tr>\n", url_encode(service_desc));
				}
			}
			if (temp_svcstatus->notifications_enabled == TRUE) {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Notifications For This Service' TITLE='Disable Notifications For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SVC_NOTIFICATIONS, url_encode(host_name));
				printf("&service=%s'>Disable notifications for this service</a></td></tr>\n", url_encode(service_desc));
				if (temp_svcstatus->status != SERVICE_OK) {
					printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Delay Next Service Notification' TITLE='Delay Next Service Notification'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DELAY_ICON, CMD_CGI, CMD_DELAY_SVC_NOTIFICATION, url_encode(host_name));
					printf("&service=%s'>Delay next service notification</a></td></tr>\n", url_encode(service_desc));
				}
			} else {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Notifications For This Service' TITLE='Enable Notifications For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SVC_NOTIFICATIONS, url_encode(host_name));
				printf("&service=%s'>Enable notifications for this service</a></td></tr>\n", url_encode(service_desc));
			}

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Send Custom Notification' TITLE='Send Custom Notification'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_SEND_CUSTOM_SVC_NOTIFICATION, url_encode(host_name));
			printf("&service=%s'>Send custom service notification</a></td></tr>\n", url_encode(service_desc));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Schedule Downtime For This Service' TITLE='Schedule Downtime For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_SVC_DOWNTIME, url_encode(host_name));
			printf("&service=%s'>Schedule downtime for this service</a></td></tr>\n", url_encode(service_desc));

			/*
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Cancel Scheduled Downtime For This Service' TITLE='Cancel Scheduled Downtime For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s",url_images_path,DOWNTIME_ICON,CMD_CGI,CMD_CANCEL_SVC_DOWNTIME,url_encode(host_name));
			printf("&service=%s'>Cancel scheduled downtime for this service</a></td></tr>\n",url_encode(service_desc));
			*/

			if (temp_svcstatus->event_handler_enabled == TRUE) {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Event Handler For This Service' TITLE='Disable Event Handler For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SVC_EVENT_HANDLER, url_encode(host_name));
				printf("&service=%s'>Disable event handler for this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Event Handler For This Service' TITLE='Enable Event Handler For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SVC_EVENT_HANDLER, url_encode(host_name));
				printf("&service=%s'>Enable event handler for this service</a></td></tr>\n", url_encode(service_desc));
			}

			if (temp_svcstatus->flap_detection_enabled == TRUE) {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Flap Detection For This Service' TITLE='Disable Flap Detection For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SVC_FLAP_DETECTION, url_encode(host_name));
				printf("&service=%s'>Disable flap detection for this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Flap Detection For This Service' TITLE='Enable Flap Detection For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SVC_FLAP_DETECTION, url_encode(host_name));
				printf("&service=%s'>Enable flap detection for this service</a></td></tr>\n", url_encode(service_desc));
			}

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Add a new Service comment' TITLE='Add a new Service comment'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s&", url_images_path, COMMENT_ICON, CMD_CGI, CMD_ADD_SVC_COMMENT, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name));
			printf("service=%s'>", (display_type == DISPLAY_COMMENTS) ? "" : url_encode(service_desc));
			printf("Add a new Service comment</a></td>");

			/* allow modified attributes to be reset */
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Reset Modified Attributes' TITLE='Reset Modified Attributes'></td><td CLASS='command'><a href='%s?cmd_typ=%d&attr=%d&host=%s&", url_images_path, DISABLED_ICON, CMD_CGI, CMD_CHANGE_SVC_MODATTR, MODATTR_NONE, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name));
			printf("service=%s'>", (display_type == DISPLAY_COMMENTS) ? "" : url_encode(service_desc));
			printf("Reset Modified Attributes</a></td>");


			printf("</table>\n");
		} else if (is_authorized_for_read_only(&current_authdata) == TRUE) {
			printf("<DIV ALIGN=CENTER CLASS='infoMessage'>Your account does not have permissions to execute commands.<br>\n");
		} else {
			printf("<DIV CLASS='infoMessage'>It appears as though Icinga is not running, so commands are temporarily unavailable...<br>\n");
			printf("Click <a href='%s?type=%d'>here</a> to view Icinga process information</DIV>\n", EXTINFO_CGI, DISPLAY_PROCESS_INFO);
		}

		printf("</td></tr>\n");
		printf("</table>\n");

		printf("</TD>\n");

		printf("</TR></TABLE></TD>\n");
		printf("</TR>\n");

		printf("<TR><TD COLSPAN=2><BR></TD></TR>\n");

		printf("<TR>\n");
		printf("<TD COLSPAN=2 VALIGN=TOP CLASS='commentPanel'>\n");

		if (is_authorized_for_read_only(&current_authdata) == FALSE || is_authorized_for_comments_read_only(&current_authdata) == TRUE) {
			/* display comments */
			show_comments(SERVICE_COMMENT);
			printf("<BR>");
			/* display downtimes */
			show_downtime(SERVICE_DOWNTIME);
		}
		printf("</TD>\n");
		printf("</TR>\n");

		printf("</TABLE>\n");
	}

	return;
}

void show_hostgroup_info(void) {
	hostgroup *temp_hostgroup;

	/* get hostgroup info */
	temp_hostgroup = find_hostgroup(hostgroup_name);

	/* make sure the user has rights to view hostgroup information */
	if (is_authorized_for_hostgroup(temp_hostgroup, &current_authdata) == FALSE) {
		print_generic_error_message("It appears as though you do not have permission to view information for this hostgroup...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		return;
	}

	/* make sure hostgroup information exists */
	if (temp_hostgroup == NULL) {
		print_generic_error_message("Error: Hostgroup Not Found!", NULL, 0);
		return;
	}

	printf("<DIV CLASS='dataTitle'>Hostgroup Commands</DIV>\n");

	if (nagios_process_state == STATE_OK && is_authorized_for_read_only(&current_authdata) == FALSE) {

		printf("<TABLE border=0 CELLSPACING=0 CELLPADDING=0 CLASS='command' align='center'>\n");

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Schedule Downtime For All Hosts In This Hostgroup' TITLE='Schedule Downtime For All Hosts In This Hostgroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>Schedule downtime for all hosts in this hostgroup</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Schedule Downtime For All Services In This Hostgroup' TITLE='Schedule Downtime For All Services In This Hostgroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>Schedule downtime for all services in this hostgroup</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Notifications For All Hosts In This Hostgroup' TITLE='Enable Notifications For All Hosts In This Hostgroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>Enable notifications for all hosts in this hostgroup</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Notifications For All Hosts In This Hostgroup' TITLE='Disable Notifications For All Hosts In This Hostgroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>Disable notifications for all hosts in this hostgroup</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Notifications For All Services In This Hostgroup' TITLE='Enable Notifications For All Services In This Hostgroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>Enable notifications for all services in this hostgroup</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Notifications For All Services In This Hostgroup' TITLE='Disable Notifications For All Services In This Hostgroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>Disable notifications for all services in this hostgroup</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Active Checks Of All Services In This Hostgroup' TITLE='Enable Active Checks Of All Services In This Hostgroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>Enable active checks of all services in this hostgroup</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOSTGROUP_SVC_CHECKS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Active Checks Of All Services In This Hostgroup' TITLE='Disable Active Checks Of All Services In This Hostgroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>Disable active checks of all services in this hostgroup</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOSTGROUP_SVC_CHECKS, url_encode(hostgroup_name));

		printf("</table>\n");

	} else if (is_authorized_for_read_only(&current_authdata) == TRUE) {
		print_generic_error_message("Your account does not have permissions to execute commands.", NULL, 0);
	} else {
		printf("<DIV CLASS='infoMessage' align='center'>It appears as though Icinga is not running, so commands are temporarily unavailable...<br>\n");
		printf("Click <a href='%s?type=%d'>here</a> to view Icinga process information</DIV>\n", EXTINFO_CGI, DISPLAY_PROCESS_INFO);
	}

	return;
}

void show_servicegroup_info() {
	servicegroup *temp_servicegroup;

	/* get servicegroup info */
	temp_servicegroup = find_servicegroup(servicegroup_name);

	/* make sure the user has rights to view servicegroup information */
	if (is_authorized_for_servicegroup(temp_servicegroup, &current_authdata) == FALSE) {
		print_generic_error_message("It appears as though you do not have permission to view information for this servicegroup...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		return;
	}

	/* make sure servicegroup information exists */
	if (temp_servicegroup == NULL) {
		print_generic_error_message("Error: Servicegroup Not Found!", NULL, 0);
		return;
	}

	printf("<DIV CLASS='dataTitle'>Servicegroup Commands</DIV>\n");

	if (nagios_process_state == STATE_OK) {

		printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0 CLASS='command' align='center'>\n");

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Schedule Downtime For All Hosts In This Servicegroup' TITLE='Schedule Downtime For All Hosts In This Servicegroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>Schedule downtime for all hosts in this servicegroup</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Schedule Downtime For All Services In This Servicegroup' TITLE='Schedule Downtime For All Services In This Servicegroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>Schedule downtime for all services in this servicegroup</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Notifications For All Hosts In This Servicegroup' TITLE='Enable Notifications For All Hosts In This Servicegroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>Enable notifications for all hosts in this servicegroup</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Notifications For All Hosts In This Servicegroup' TITLE='Disable Notifications For All Hosts In This Servicegroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>Disable notifications for all hosts in this servicegroup</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, CMD_CGI, CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Notifications For All Services In This Servicegroup' TITLE='Enable Notifications For All Services In This Servicegroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>Enable notifications for all services in this servicegroup</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Notifications For All Services In This Servicegroup' TITLE='Disable Notifications For All Services In This Servicegroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>Disable notifications for all services in this servicegroup</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, CMD_CGI, CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Enable Active Checks Of All Services In This Servicegroup' TITLE='Enable Active Checks Of All Services In This Servicegroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>Enable active checks of all services in this servicegroup</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SERVICEGROUP_SVC_CHECKS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Disable Active Checks Of All Services In This Servicegroup' TITLE='Disable Active Checks Of All Services In This Servicegroup'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>Disable active checks of all services in this servicegroup</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SERVICEGROUP_SVC_CHECKS, url_encode(servicegroup_name));

		printf("</table>\n");

	} else if (is_authorized_for_read_only(&current_authdata) == TRUE) {
		print_generic_error_message("Your account does not have permissions to execute commands.", NULL, 0);
	} else {
		printf("<DIV CLASS='infoMessage'>It appears as though Icinga is not running, so commands are temporarily unavailable...<br>\n");
		printf("Click <a href='%s?type=%d'>here</a> to view Icinga process information</DIV>\n", EXTINFO_CGI, DISPLAY_PROCESS_INFO);
	}

	return;
}

void show_performance_data(void) {
	service *temp_service = NULL;
	servicestatus *temp_servicestatus = NULL;
	host *temp_host = NULL;
	hoststatus *temp_hoststatus = NULL;
	int total_active_service_checks = 0;
	int total_passive_service_checks = 0;
	double min_service_execution_time = 0.0;
	double max_service_execution_time = 0.0;
	double total_service_execution_time = 0.0;
	int have_min_service_execution_time = FALSE;
	int have_max_service_execution_time = FALSE;
	double min_service_latency = 0.0;
	double max_service_latency = 0.0;
	double long total_service_latency = 0.0;
	int have_min_service_latency = FALSE;
	int have_max_service_latency = FALSE;
	double min_host_latency = 0.0;
	double max_host_latency = 0.0;
	double total_host_latency = 0.0;
	int have_min_host_latency = FALSE;
	int have_max_host_latency = FALSE;
	double min_service_percent_change_a = 0.0;
	double max_service_percent_change_a = 0.0;
	double total_service_percent_change_a = 0.0;
	int have_min_service_percent_change_a = FALSE;
	int have_max_service_percent_change_a = FALSE;
	double min_service_percent_change_b = 0.0;
	double max_service_percent_change_b = 0.0;
	double total_service_percent_change_b = 0.0;
	int have_min_service_percent_change_b = FALSE;
	int have_max_service_percent_change_b = FALSE;
	int active_service_checks_1min = 0;
	int active_service_checks_5min = 0;
	int active_service_checks_15min = 0;
	int active_service_checks_1hour = 0;
	int active_service_checks_start = 0;
	int active_service_checks_ever = 0;
	int passive_service_checks_1min = 0;
	int passive_service_checks_5min = 0;
	int passive_service_checks_15min = 0;
	int passive_service_checks_1hour = 0;
	int passive_service_checks_start = 0;
	int passive_service_checks_ever = 0;
	int total_active_host_checks = 0;
	int total_passive_host_checks = 0;
	double min_host_execution_time = 0.0;
	double max_host_execution_time = 0.0;
	double total_host_execution_time = 0.0;
	int have_min_host_execution_time = FALSE;
	int have_max_host_execution_time = FALSE;
	double min_host_percent_change_a = 0.0;
	double max_host_percent_change_a = 0.0;
	double total_host_percent_change_a = 0.0;
	int have_min_host_percent_change_a = FALSE;
	int have_max_host_percent_change_a = FALSE;
	double min_host_percent_change_b = 0.0;
	double max_host_percent_change_b = 0.0;
	double total_host_percent_change_b = 0.0;
	int have_min_host_percent_change_b = FALSE;
	int have_max_host_percent_change_b = FALSE;
	int active_host_checks_1min = 0;
	int active_host_checks_5min = 0;
	int active_host_checks_15min = 0;
	int active_host_checks_1hour = 0;
	int active_host_checks_start = 0;
	int active_host_checks_ever = 0;
	int passive_host_checks_1min = 0;
	int passive_host_checks_5min = 0;
	int passive_host_checks_15min = 0;
	int passive_host_checks_1hour = 0;
	int passive_host_checks_start = 0;
	int passive_host_checks_ever = 0;
	time_t current_time;
	/* make sure gcc3 won't hit here */
#ifndef GCCTOOOLD
	profile_object *t, *p = profiled_data;
	int count = 0;
	double elapsed = 0.0, total_time = 0.0;
	char *name = NULL;
#endif

	time(&current_time);

	/* check all services */
	for (temp_servicestatus = servicestatus_list; temp_servicestatus != NULL; temp_servicestatus = temp_servicestatus->next) {

		/* find the service */
		temp_service = find_service(temp_servicestatus->host_name, temp_servicestatus->description);

		/* make sure the user has rights to view service information */
		if (is_authorized_for_service(temp_service, &current_authdata) == FALSE)
			continue;

		/* is this an active or passive check? */
		if (temp_servicestatus->checks_enabled == TRUE) {

			total_active_service_checks++;

			total_service_execution_time += temp_servicestatus->execution_time;
			if (have_min_service_execution_time == FALSE || temp_servicestatus->execution_time < min_service_execution_time) {
				have_min_service_execution_time = TRUE;
				min_service_execution_time = temp_servicestatus->execution_time;
			}
			if (have_max_service_execution_time == FALSE || temp_servicestatus->execution_time > max_service_execution_time) {
				have_max_service_execution_time = TRUE;
				max_service_execution_time = temp_servicestatus->execution_time;
			}

			total_service_percent_change_a += temp_servicestatus->percent_state_change;
			if (have_min_service_percent_change_a == FALSE || temp_servicestatus->percent_state_change < min_service_percent_change_a) {
				have_min_service_percent_change_a = TRUE;
				min_service_percent_change_a = temp_servicestatus->percent_state_change;
			}
			if (have_max_service_percent_change_a == FALSE || temp_servicestatus->percent_state_change > max_service_percent_change_a) {
				have_max_service_percent_change_a = TRUE;
				max_service_percent_change_a = temp_servicestatus->percent_state_change;
			}

			total_service_latency += temp_servicestatus->latency;
			if (have_min_service_latency == FALSE || temp_servicestatus->latency < min_service_latency) {
				have_min_service_latency = TRUE;
				min_service_latency = temp_servicestatus->latency;
			}
			if (have_max_service_latency == FALSE || temp_servicestatus->latency > max_service_latency) {
				have_max_service_latency = TRUE;
				max_service_latency = temp_servicestatus->latency;
			}

			if (temp_servicestatus->last_check >= (current_time - 60))
				active_service_checks_1min++;
			if (temp_servicestatus->last_check >= (current_time - 300))
				active_service_checks_5min++;
			if (temp_servicestatus->last_check >= (current_time - 900))
				active_service_checks_15min++;
			if (temp_servicestatus->last_check >= (current_time - 3600))
				active_service_checks_1hour++;
			if (temp_servicestatus->last_check >= program_start)
				active_service_checks_start++;
			if (temp_servicestatus->last_check != (time_t)0)
				active_service_checks_ever++;

		} else if (temp_servicestatus->accept_passive_service_checks == TRUE) {
			total_passive_service_checks++;

			total_service_percent_change_b += temp_servicestatus->percent_state_change;
			if (have_min_service_percent_change_b == FALSE || temp_servicestatus->percent_state_change < min_service_percent_change_b) {
				have_min_service_percent_change_b = TRUE;
				min_service_percent_change_b = temp_servicestatus->percent_state_change;
			}
			if (have_max_service_percent_change_b == FALSE || temp_servicestatus->percent_state_change > max_service_percent_change_b) {
				have_max_service_percent_change_b = TRUE;
				max_service_percent_change_b = temp_servicestatus->percent_state_change;
			}

			if (temp_servicestatus->last_check >= (current_time - 60))
				passive_service_checks_1min++;
			if (temp_servicestatus->last_check >= (current_time - 300))
				passive_service_checks_5min++;
			if (temp_servicestatus->last_check >= (current_time - 900))
				passive_service_checks_15min++;
			if (temp_servicestatus->last_check >= (current_time - 3600))
				passive_service_checks_1hour++;
			if (temp_servicestatus->last_check >= program_start)
				passive_service_checks_start++;
			if (temp_servicestatus->last_check != (time_t)0)
				passive_service_checks_ever++;
		}
	}

	/* check all hosts */
	for (temp_hoststatus = hoststatus_list; temp_hoststatus != NULL; temp_hoststatus = temp_hoststatus->next) {

		/* find the host */
		temp_host = find_host(temp_hoststatus->host_name);

		/* make sure the user has rights to view host information */
		if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		/* is this an active or passive check? */
		if (temp_hoststatus->check_type == HOST_CHECK_ACTIVE) {

			total_active_host_checks++;

			total_host_execution_time += temp_hoststatus->execution_time;
			if (have_min_host_execution_time == FALSE || temp_hoststatus->execution_time < min_host_execution_time) {
				have_min_host_execution_time = TRUE;
				min_host_execution_time = temp_hoststatus->execution_time;
			}
			if (have_max_host_execution_time == FALSE || temp_hoststatus->execution_time > max_host_execution_time) {
				have_max_host_execution_time = TRUE;
				max_host_execution_time = temp_hoststatus->execution_time;
			}

			total_host_percent_change_a += temp_hoststatus->percent_state_change;
			if (have_min_host_percent_change_a == FALSE || temp_hoststatus->percent_state_change < min_host_percent_change_a) {
				have_min_host_percent_change_a = TRUE;
				min_host_percent_change_a = temp_hoststatus->percent_state_change;
			}
			if (have_max_host_percent_change_a == FALSE || temp_hoststatus->percent_state_change > max_host_percent_change_a) {
				have_max_host_percent_change_a = TRUE;
				max_host_percent_change_a = temp_hoststatus->percent_state_change;
			}

			total_host_latency += temp_hoststatus->latency;
			if (have_min_host_latency == FALSE || temp_hoststatus->latency < min_host_latency) {
				have_min_host_latency = TRUE;
				min_host_latency = temp_hoststatus->latency;
			}
			if (have_max_host_latency == FALSE || temp_hoststatus->latency > max_host_latency) {
				have_max_host_latency = TRUE;
				max_host_latency = temp_hoststatus->latency;
			}

			if (temp_hoststatus->last_check >= (current_time - 60))
				active_host_checks_1min++;
			if (temp_hoststatus->last_check >= (current_time - 300))
				active_host_checks_5min++;
			if (temp_hoststatus->last_check >= (current_time - 900))
				active_host_checks_15min++;
			if (temp_hoststatus->last_check >= (current_time - 3600))
				active_host_checks_1hour++;
			if (temp_hoststatus->last_check >= program_start)
				active_host_checks_start++;
			if (temp_hoststatus->last_check != (time_t)0)
				active_host_checks_ever++;
		}

		else {
			total_passive_host_checks++;

			total_host_percent_change_b += temp_hoststatus->percent_state_change;
			if (have_min_host_percent_change_b == FALSE || temp_hoststatus->percent_state_change < min_host_percent_change_b) {
				have_min_host_percent_change_b = TRUE;
				min_host_percent_change_b = temp_hoststatus->percent_state_change;
			}
			if (have_max_host_percent_change_b == FALSE || temp_hoststatus->percent_state_change > max_host_percent_change_b) {
				have_max_host_percent_change_b = TRUE;
				max_host_percent_change_b = temp_hoststatus->percent_state_change;
			}

			if (temp_hoststatus->last_check >= (current_time - 60))
				passive_host_checks_1min++;
			if (temp_hoststatus->last_check >= (current_time - 300))
				passive_host_checks_5min++;
			if (temp_hoststatus->last_check >= (current_time - 900))
				passive_host_checks_15min++;
			if (temp_hoststatus->last_check >= (current_time - 3600))
				passive_host_checks_1hour++;
			if (temp_hoststatus->last_check >= program_start)
				passive_host_checks_start++;
			if (temp_hoststatus->last_check != (time_t)0)
				passive_host_checks_ever++;
		}
	}


	printf("<div align=center>\n");


	printf("<DIV CLASS='dataTitle'>Program-Wide Performance Information</DIV>\n");

	printf("<table border='0' cellpadding='10'>\n");


	/***** ACTIVE SERVICE CHECKS *****/

	printf("<tr>\n");
	printf("<td valign=middle><div class='perfTypeTitle'>Services Actively Checked:</div></td>\n");
	printf("<td valign=top>\n");

	/* fake this so we don't divide by zero for just showing the table */
	if (total_active_service_checks == 0)
		total_active_service_checks = 1;

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Time Frame</th><th class='data'>Services Checked</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1 minute:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_1min, (double)(((double)active_service_checks_1min * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 5 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_5min, (double)(((double)active_service_checks_5min * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 15 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_15min, (double)(((double)active_service_checks_15min * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 1 hour:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_1hour, (double)(((double)active_service_checks_1hour * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>Since program start:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td>", active_service_checks_start, (double)(((double)active_service_checks_start * 100.0) / (double)total_active_service_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td><td valign=top>\n");

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable2'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Metric</th><th class='data'>Min.</th><th class='data'>Max.</th><th class='data'>Average</th></tr>\n");

	printf("<tr><td class='dataVar'>Check Execution Time:&nbsp;&nbsp;</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.3f sec</td></tr>\n", min_service_execution_time, max_service_execution_time, (double)((double)total_service_execution_time / (double)total_active_service_checks));

	printf("<tr><td class='dataVar'>Check Latency:</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.3f sec</td></tr>\n", min_service_latency, max_service_latency, (double)((double)total_service_latency / (double)total_active_service_checks));

	printf("<tr><td class='dataVar'>Percent State Change:</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_service_percent_change_a, max_service_percent_change_a, (double)((double)total_service_percent_change_a / (double)total_active_service_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");


	printf("</td>\n");
	printf("</tr>\n");


	/***** PASSIVE SERVICE CHECKS *****/

	printf("<tr>\n");
	printf("<td valign=middle><div class='perfTypeTitle'>Services Passively Checked:</div></td>\n");
	printf("<td valign=top>\n");


	/* fake this so we don't divide by zero for just showing the table */
	if (total_passive_service_checks == 0)
		total_passive_service_checks = 1;

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Time Frame</th><th class='data'>Services Checked</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1 minute:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_1min, (double)(((double)passive_service_checks_1min * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 5 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_5min, (double)(((double)passive_service_checks_5min * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 15 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_15min, (double)(((double)passive_service_checks_15min * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 1 hour:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_1hour, (double)(((double)passive_service_checks_1hour * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>Since program start:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_start, (double)(((double)passive_service_checks_start * 100.0) / (double)total_passive_service_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td><td valign=top>\n");

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable2'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Metric</th><th class='data'>Min.</th><th class='data'>Max.</th><th class='data'>Average</th></tr>\n");
	printf("<tr><td class='dataVar'>Percent State Change:&nbsp;&nbsp;</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_service_percent_change_b, max_service_percent_change_b, (double)((double)total_service_percent_change_b / (double)total_passive_service_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td>\n");
	printf("</tr>\n");


	/***** ACTIVE HOST CHECKS *****/

	printf("<tr>\n");
	printf("<td valign=middle><div class='perfTypeTitle'>Hosts Actively Checked:</div></td>\n");
	printf("<td valign=top>\n");

	/* fake this so we don't divide by zero for just showing the table */
	if (total_active_host_checks == 0)
		total_active_host_checks = 1;

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Time Frame</th><th class='data'>Hosts Checked</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1 minute:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_1min, (double)(((double)active_host_checks_1min * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 5 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_5min, (double)(((double)active_host_checks_5min * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 15 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_15min, (double)(((double)active_host_checks_15min * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 1 hour:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_1hour, (double)(((double)active_host_checks_1hour * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>Since program start:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td>", active_host_checks_start, (double)(((double)active_host_checks_start * 100.0) / (double)total_active_host_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td><td valign=top>\n");

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable2'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Metric</th><th class='data'>Min.</th><th class='data'>Max.</th><th class='data'>Average</th></tr>\n");

	printf("<tr><td class='dataVar'>Check Execution Time:&nbsp;&nbsp;</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.3f sec</td></tr>\n", min_host_execution_time, max_host_execution_time, (double)((double)total_host_execution_time / (double)total_active_host_checks));

	printf("<tr><td class='dataVar'>Check Latency:</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.3f sec</td></tr>\n", min_host_latency, max_host_latency, (double)((double)total_host_latency / (double)total_active_host_checks));

	printf("<tr><td class='dataVar'>Percent State Change:</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_host_percent_change_a, max_host_percent_change_a, (double)((double)total_host_percent_change_a / (double)total_active_host_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");


	printf("</td>\n");
	printf("</tr>\n");


	/***** PASSIVE HOST CHECKS *****/

	printf("<tr>\n");
	printf("<td valign=middle><div class='perfTypeTitle'>Hosts Passively Checked:</div></td>\n");
	printf("<td valign=top>\n");


	/* fake this so we don't divide by zero for just showing the table */
	if (total_passive_host_checks == 0)
		total_passive_host_checks = 1;

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Time Frame</th><th class='data'>Hosts Checked</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1 minute:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_1min, (double)(((double)passive_host_checks_1min * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 5 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_5min, (double)(((double)passive_host_checks_5min * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 15 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_15min, (double)(((double)passive_host_checks_15min * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 1 hour:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_1hour, (double)(((double)passive_host_checks_1hour * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>Since program start:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_start, (double)(((double)passive_host_checks_start * 100.0) / (double)total_passive_host_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td><td valign=top>\n");

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable2'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Metric</th><th class='data'>Min.</th><th class='data'>Max.</th><th class='data'>Average</th></tr>\n");
	printf("<tr><td class='dataVar'>Percent State Change:&nbsp;&nbsp;</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_host_percent_change_b, max_host_percent_change_b, (double)((double)total_host_percent_change_b / (double)total_passive_host_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td>\n");
	printf("</tr>\n");



	/***** CHECK STATS *****/

	printf("<tr>\n");
	printf("<td valign=center><div class='perfTypeTitle'>Check Statistics:</div></td>\n");
	printf("<td valign=top colspan='2'>\n");


	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Type</th><th class='data'>Last 1 Min</th><th class='data'>Last 5 Min</th><th class='data'>Last 15 Min</th></tr>\n");
	printf("<tr><td class='dataVar'>Active Scheduled Host Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_SCHEDULED_HOST_CHECK_STATS][0], program_stats[ACTIVE_SCHEDULED_HOST_CHECK_STATS][1], program_stats[ACTIVE_SCHEDULED_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>Active On-Demand Host Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_ONDEMAND_HOST_CHECK_STATS][0], program_stats[ACTIVE_ONDEMAND_HOST_CHECK_STATS][1], program_stats[ACTIVE_ONDEMAND_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>Parallel Host Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[PARALLEL_HOST_CHECK_STATS][0], program_stats[PARALLEL_HOST_CHECK_STATS][1], program_stats[PARALLEL_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>Serial Host Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[SERIAL_HOST_CHECK_STATS][0], program_stats[SERIAL_HOST_CHECK_STATS][1], program_stats[SERIAL_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>Cached Host Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_CACHED_HOST_CHECK_STATS][0], program_stats[ACTIVE_CACHED_HOST_CHECK_STATS][1], program_stats[ACTIVE_CACHED_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>Passive Host Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[PASSIVE_HOST_CHECK_STATS][0], program_stats[PASSIVE_HOST_CHECK_STATS][1], program_stats[PASSIVE_HOST_CHECK_STATS][2]);

	printf("<tr><td class='dataVar'>Active Scheduled Service Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_SCHEDULED_SERVICE_CHECK_STATS][0], program_stats[ACTIVE_SCHEDULED_SERVICE_CHECK_STATS][1], program_stats[ACTIVE_SCHEDULED_SERVICE_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>Active On-Demand Service Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_ONDEMAND_SERVICE_CHECK_STATS][0], program_stats[ACTIVE_ONDEMAND_SERVICE_CHECK_STATS][1], program_stats[ACTIVE_ONDEMAND_SERVICE_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>Cached Service Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_CACHED_SERVICE_CHECK_STATS][0], program_stats[ACTIVE_CACHED_SERVICE_CHECK_STATS][1], program_stats[ACTIVE_CACHED_SERVICE_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>Passive Service Checks</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[PASSIVE_SERVICE_CHECK_STATS][0], program_stats[PASSIVE_SERVICE_CHECK_STATS][1], program_stats[PASSIVE_SERVICE_CHECK_STATS][2]);

	printf("<tr><td class='dataVar'>External Commands</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[EXTERNAL_COMMAND_STATS][0], program_stats[EXTERNAL_COMMAND_STATS][1], program_stats[EXTERNAL_COMMAND_STATS][2]);

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td>\n");
	printf("</tr>\n");



	/***** BUFFER STATS *****/

	printf("<tr>\n");
	printf("<td valign=center><div class='perfTypeTitle'>Buffer Usage:</div></td>\n");
	printf("<td valign=top colspan='2'>\n");


	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>Type</th><th class='data'>In Use</th><th class='data'>Max Used</th><th class='data'>Total Available</th></tr>\n");
	printf("<tr><td class='dataVar'>External Commands&nbsp;</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", buffer_stats[0][1], buffer_stats[0][2], buffer_stats[0][0]);

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	/* make sure gcc3 won't hit here */
#ifndef GCCTOOOLD
	if (event_profiling_enabled) {
		printf("<tr>\n");
		printf("<td valign=center><div class='perfTypeTitle'>Event profiling:</div></td>\n");
		printf("<td valign=top colspan='2'>\n");

		printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
		printf("<TR><TD class='stateInfoTable1'>\n");
		printf("<TABLE BORDER=0>\n");


		printf("<tr class='data'><th class='data'>EVENT PROFILE DATA:</th><th class='data'>total seconds spent</th><th class='data'>number of events</th><th class='data'>avg time per event</th><th class='data'>events per second</th></tr>\n");
		while (p) {
			name = p->name;
			count = p->count;
			elapsed = p->elapsed;
			t = profile_object_find_by_name("EVENT_LOOP_COMPLETION");
			total_time = t->elapsed;

			printf("<tr><td class='dataVar'>%s&nbsp;</td><td class='dataVal'>%.2f</td><td class='dataVal'>%d</td><td class='dataVal'>%.3f</td><td class='dataVal'>%.3f</td></tr>", name, elapsed, count, safe_divide(elapsed, count, 0), safe_divide(total_time, count, 1));
			p = p->next;
		}


		printf("</TABLE>\n");
		printf("</TD></TR>\n");
		printf("</TABLE>\n");
	}
#endif


	printf("</td>\n");
	printf("</tr>\n");



	printf("</table>\n");


	printf("</div>\n");

	return;
}

void show_comments(int type) {
	host *temp_host = NULL;
	service *temp_service = NULL;
	int total_comments = 0;
	char *bg_class = "";
	int odd = 1;
	char date_time[MAX_DATETIME_LENGTH];
	comment *temp_comment;
	char *comment_type;
	char expire_time[MAX_DATETIME_LENGTH];
	int colspan = 8;
	int json_start = TRUE;

	/* define colspan */
	if (display_type == DISPLAY_COMMENTS)
		colspan = (type != SERVICE_COMMENT) ? colspan + 1 : colspan + 2;

	if (is_authorized_for_comments_read_only(&current_authdata) == TRUE)
		colspan--;

	if (content_type == JSON_CONTENT) {
		if (type == HOST_COMMENT)
			printf("\"host_comments\": [\n");
		if (type == SERVICE_COMMENT)
			printf("\"service_comments\": [\n");
	} else if (content_type == CSV_CONTENT) {
		/* csv header */
		if (display_type == DISPLAY_COMMENTS && type == HOST_COMMENT) {
			printf("%sHOST_NAME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sSERVICE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		}
		if (display_type != DISPLAY_COMMENTS || (display_type == DISPLAY_COMMENTS && type == HOST_COMMENT)) {
			printf("%sENTRY_TIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sAUTHOR%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sCOMMENT%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sCOMMENT_ID%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sPERSISTENT%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sTYPE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sEXPIRES%s\n", csv_data_enclosure, csv_data_enclosure);
		}
	} else {
		printf("<A NAME=%sCOMMENTS></A>\n", (type == HOST_COMMENT) ? "HOST" : "SERVICE");
		printf("<TABLE BORDER=0 CLASS='comment' style='padding:0px;margin-bottom: -6px;'><TR><TD width='33%%'></TD><TD width='33%%'><DIV CLASS='commentTitle'>%s Comments</DIV></TD><TD width='33%%'>", (type == HOST_COMMENT) ? "Host" : "Service");

		/* add export to csv, json, link */
		printf("<DIV style='padding-right:6px;' class='csv_export_link'>");
		if (display_type != DISPLAY_COMMENTS)
			print_export_link(CSV_CONTENT, EXTINFO_CGI, "csvtype=comment");
		else if (type == HOST_COMMENT) {
			print_export_link(CSV_CONTENT, EXTINFO_CGI, "csvtype=comment");
			print_export_link(JSON_CONTENT, EXTINFO_CGI, NULL);
			print_export_link(HTML_CONTENT, EXTINFO_CGI, NULL);
		}
		printf("</div>");

		printf("</TD></TR></TABLE>\n");

		printf("<form name='tableform%scomment' id='tableform%scomment' action='%s' method='POST' onkeypress='var key = (window.event) ? event.keyCode : event.which; return (key != 13);'>", (type == HOST_COMMENT) ? "host" : "service", (type == HOST_COMMENT) ? "host" : "service", CMD_CGI);
		printf("<input type=hidden name=buttonCheckboxChecked>");
		printf("<input type=hidden name='cmd_typ' value=%d>", (type == HOST_COMMENT) ? CMD_DEL_HOST_COMMENT : CMD_DEL_SVC_COMMENT);

		printf("<TABLE BORDER=0 CLASS='comment'>\n");

		printf("<TR><TD colspan='%d' align='right'>", colspan);

		if (display_type == DISPLAY_COMMENTS && type == HOST_COMMENT) {
			printf("<table width='100%%' cellspacing=0 cellpadding=0><tr><td width='33%%'></td><td width='33%%' nowrap>");
			printf("<div class='page_selector'>\n");
			printf("<div id='page_navigation_copy'></div>\n");
			page_limit_selector(result_start);
			printf("</div>\n");
			printf("</td><td width='33%%' align='right'>\n");
		}

		if (is_authorized_for_comments_read_only(&current_authdata) == FALSE)
			printf("<input type='submit' name='CommandButton' value='Delete Comments' disabled=\"disabled\">");

		if (display_type == DISPLAY_COMMENTS && type == HOST_COMMENT)
			printf("</td></tr></table>");

		printf("</TD></TR>\n");

		printf("<TR CLASS='comment'>");
		if (display_type == DISPLAY_COMMENTS) {
			printf("<TH CLASS='comment'>Host Name</TH>");
			if (type == SERVICE_COMMENT)
				printf("<TH CLASS='comment'>Service</TH>");
		}
		printf("<TH CLASS='comment'>Entry Time</TH><TH CLASS='comment'>Author</TH><TH CLASS='comment'>Comment</TH><TH CLASS='comment'>Comment ID</TH><TH CLASS='comment'>Persistent</TH><TH CLASS='comment'>Type</TH><TH CLASS='comment'>Expires</TH>");
		if (is_authorized_for_comments_read_only(&current_authdata) == FALSE)
			printf("<TH CLASS='comment' nowrap>Actions&nbsp;&nbsp;<input type='checkbox' value=all onclick=\"checkAll(\'tableform%scomment\');isValidForSubmit(\'tableform%scomment\');\"></TH>", (type == HOST_COMMENT) ? "host" : "service", (type == HOST_COMMENT) ? "host" : "service");
		printf("</TR>\n");
	}

	/* display all the service comments */
	for (temp_comment = comment_list, total_comments = 0; temp_comment != NULL; temp_comment = temp_comment->next) {

		if (type == HOST_COMMENT && temp_comment->comment_type != HOST_COMMENT)
			continue;

		if (type == SERVICE_COMMENT && temp_comment->comment_type != SERVICE_COMMENT)
			continue;

		if (display_type != DISPLAY_COMMENTS) {
			/* if not our host -> continue */
			if (strcmp(temp_comment->host_name, host_name))
				continue;

			if (type == SERVICE_COMMENT) {
				/* if not our service -> continue */
				if (strcmp(temp_comment->service_description, service_desc))
					continue;
			}
		} else {
			temp_host = find_host(temp_comment->host_name);

			/* make sure the user has rights to view host information */
			if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
				continue;

			if (type == SERVICE_COMMENT) {
				temp_service = find_service(temp_comment->host_name, temp_comment->service_description);

				/* make sure the user has rights to view service information */
				if (is_authorized_for_service(temp_service, &current_authdata) == FALSE)
					continue;
			}
		}

		if (result_limit != 0  && (((total_entries + 1) < result_start) || (total_entries >= ((result_start + result_limit) - 1)))) {
			total_entries++;
			continue;
		}

		displayed_entries++;
		total_entries++;

		if (odd) {
			odd = 0;
			bg_class = "commentOdd";
		} else {
			odd = 1;
			bg_class = "commentEven";
		}

		switch (temp_comment->entry_type) {
		case USER_COMMENT:
			comment_type = "User";
			break;
		case DOWNTIME_COMMENT:
			comment_type = "Scheduled Downtime";
			break;
		case FLAPPING_COMMENT:
			comment_type = "Flap Detection";
			break;
		case ACKNOWLEDGEMENT_COMMENT:
			comment_type = "Acknowledgement";
			break;
		default:
			comment_type = "?";
		}

		get_time_string(&temp_comment->entry_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		get_time_string(&temp_comment->expire_time, expire_time, (int)sizeof(date_time), SHORT_DATE_TIME);

		if (content_type == JSON_CONTENT) {
			// always add a comma, except for the first line
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;

			printf("{ ");
			if (display_type == DISPLAY_COMMENTS) {
				printf("\"host_name\": \"%s\", ", json_encode(temp_host->name));
				printf("\"host_display_name\": \"%s\", ", (temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(temp_host->name));
				if (type == SERVICE_COMMENT) {
					printf("\"service_description\": \"%s\", ", json_encode(temp_service->description));
					printf("\"service_display_name\": \"%s\", ", (temp_service->display_name != NULL) ? json_encode(temp_service->display_name) : json_encode(temp_service->description));
				}
			}
			printf("\"entry_time\": \"%s\", ", date_time);
			printf("\"author\": \"%s\", ", json_encode(temp_comment->author));
			printf("\"comment\": \"%s\", ", json_encode(temp_comment->comment_data));
			printf("\"comment_id\": %lu, ", temp_comment->comment_id);
			printf("\"persistent\": %s, ", (temp_comment->persistent == TRUE) ? "true" : "false");
			printf("\"comment_type\": \"%s\", ", comment_type);
			if (temp_comment->expires == TRUE)
				printf("\"expires\": null }");
			else
				printf("\"expires\": \"%s\" }", expire_time);
		} else if (content_type == CSV_CONTENT) {
			if (display_type == DISPLAY_COMMENTS) {
				printf("%s%s%s%s", csv_data_enclosure, temp_host->name, csv_data_enclosure, csv_delimiter);
				if (type == SERVICE_COMMENT)
					printf("%s%s%s%s", csv_data_enclosure, temp_service->description, csv_data_enclosure, csv_delimiter);
				else
					printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			}
			printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_comment->author, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_comment->comment_data, csv_data_enclosure, csv_delimiter);
			printf("%s%lu%s%s", csv_data_enclosure, temp_comment->comment_id, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_comment->persistent) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, comment_type, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s\n", csv_data_enclosure, (temp_comment->expires == TRUE) ? expire_time : "N/A", csv_data_enclosure);
		} else {
			printf("<tr CLASS='%s' onClick=\"toggle_checkbox('comment_%lu','tableform%scomment');\">", bg_class, temp_comment->comment_id, (type == HOST_COMMENT) ? "host" : "service");
			if (display_type == DISPLAY_COMMENTS) {
				printf("<td><A HREF='%s?type=%d&host=%s'>%s</A></td>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_comment->host_name), (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);
				if (type == SERVICE_COMMENT) {
					printf("<td><A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_comment->host_name));
					printf("&service=%s'>%s</A></td>", url_encode(temp_comment->service_description), (temp_service->display_name != NULL) ? temp_service->display_name : temp_service->description);
				}
			}
			printf("<td name='comment_time'>%s</td><td name='comment_author'>%s</td><td name='comment_data'>%s</td><td name='comment_id'>%lu</td><td name='comment_persist'>%s</td><td name='comment_type'>%s</td><td name='comment_expire'>%s</td>", date_time, temp_comment->author, temp_comment->comment_data, temp_comment->comment_id, (temp_comment->persistent) ? "Yes" : "No", comment_type, (temp_comment->expires == TRUE) ? expire_time : "N/A");
			if (is_authorized_for_comments_read_only(&current_authdata) == FALSE) {
				printf("<td align='center'><a href='%s?cmd_typ=%d&com_id=%lu'><img src='%s%s' border=0 ALT='Delete This Comment' TITLE='Delete This Comment'></a>", CMD_CGI, (type == HOST_COMMENT) ? CMD_DEL_HOST_COMMENT : CMD_DEL_SVC_COMMENT, temp_comment->comment_id, url_images_path, DELETE_ICON);
				printf("<input type='checkbox' onClick=\"toggle_checkbox('comment_%lu','tableform%scomment');\" name='com_id' id='comment_%lu' value='%lu'></td>", temp_comment->comment_id, (type == HOST_COMMENT) ? "host" : "service", temp_comment->comment_id, temp_comment->comment_id);
			}
			printf("</tr>\n");
		}
		total_comments++;
	}

	/* see if this host or service has any comments associated with it */
	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		if (total_comments == 0 && total_entries == 0) {
			printf("<TR CLASS='commentOdd'><TD align='center' COLSPAN='%d'>", colspan);
			if (display_type == DISPLAY_COMMENTS)
				printf("There are no %s comments", (type == HOST_COMMENT) ? "host" : "service");
			else
				printf("This %s has no comments associated with it", (type == HOST_COMMENT) ? "host" : "service");
			printf("</TD></TR>\n");
		}

		if (display_type == DISPLAY_COMMENTS && type == SERVICE_COMMENT) {
			printf("<TR><TD colspan='%d'>\n", colspan);
			page_num_selector(result_start, total_entries, displayed_entries);
			printf("</TD></TR>\n");
		}

		printf("</TABLE>\n");
		printf("<script language='javascript' type='text/javascript'>\n");
		printf("document.tableform%scomment.buttonCheckboxChecked.value = 'false';\n", (type == HOST_COMMENT) ? "host" : "service");
		printf("checked = true;\n");
		printf("checkAll(\"tableform%scomment\");\n", (type == HOST_COMMENT) ? "host" : "service");
		printf("checked = false;\n");
		printf("</script>\n");
		printf("</FORM>\n");
	}
	if (content_type == JSON_CONTENT)
		printf("]");

	return;
}

/* shows service and host scheduled downtime */
void show_downtime(int type) {
	char *bg_class = "";
	char date_time[MAX_DATETIME_LENGTH];
	scheduled_downtime *temp_downtime;
	host *temp_host = NULL;
	service *temp_service = NULL;
	int days;
	int hours;
	int minutes;
	int seconds;
	int odd = 0;
	int total_downtime = 0;
	int colspan = 12;
	int json_start = TRUE;

	/* define colspan */
	if (display_type == DISPLAY_DOWNTIME)
		colspan = (type != SERVICE_DOWNTIME) ? colspan + 1 : colspan + 2;

	if (is_authorized_for_downtimes_read_only(&current_authdata) == TRUE)
		colspan--;

	if (content_type == JSON_CONTENT) {
		if (type == HOST_DOWNTIME)
			printf("\"host_downtimes\": [\n");
		if (type == SERVICE_DOWNTIME)
			printf("\"service_downtimes\": [\n");
	} else if (content_type == CSV_CONTENT) {
		/* csv header */
		if (display_type == DISPLAY_DOWNTIME && type == HOST_DOWNTIME) {
			printf("%sHOST_NAME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sSERVICE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		}
		if (display_type != DISPLAY_DOWNTIME || (display_type == DISPLAY_DOWNTIME && type == HOST_DOWNTIME)) {
			printf("%sENTRY_TIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sAUTHOR%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sCOMMENT%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sSTART_TIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sEND_TIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sTYPE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sTRIGGER_TIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sDURATION%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sIS_IN_EFFECT%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sDOWNTIME_ID%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			printf("%sTRIGGER_ID%s\n", csv_data_enclosure, csv_data_enclosure);
		}
	} else {
		printf("<A NAME=%sDOWNTIME></A>\n", (type == HOST_DOWNTIME) ? "HOST" : "SERVICE");
		printf("<TABLE BORDER=0 CLASS='comment' style='padding:0px;margin-bottom: -6px;'><TR><TD width='33%%'></TD><TD width='33%%'><DIV CLASS='commentTitle'>Scheduled %s Downtime</DIV></TD><TD width='33%%'>", (type == HOST_DOWNTIME) ? "Host" : "Service");

		/* add export to csv, json, link */
		printf("<DIV style='padding-right:6px;' class='csv_export_link'>");
		if (display_type != DISPLAY_DOWNTIME)
			print_export_link(CSV_CONTENT, EXTINFO_CGI, "csvtype=downtime");
		else if (type == HOST_DOWNTIME) {
			print_export_link(CSV_CONTENT, EXTINFO_CGI, "csvtype=downtime");
			print_export_link(JSON_CONTENT, EXTINFO_CGI, NULL);
			print_export_link(HTML_CONTENT, EXTINFO_CGI, NULL);
		}
		printf("</div>");

		printf("</TD></TR></TABLE>\n");

		printf("<form name='tableform%sdowntime' id='tableform%sdowntime' action='%s' method='POST' onkeypress='var key = (window.event) ? event.keyCode : event.which; return (key != 13);'>", (type == HOST_DOWNTIME) ? "host" : "service", (type == HOST_DOWNTIME) ? "host" : "service", CMD_CGI);
		printf("<input type=hidden name=buttonCheckboxChecked>");
		printf("<input type=hidden name='cmd_typ' value=%d>", (type == HOST_DOWNTIME) ? CMD_DEL_HOST_DOWNTIME : CMD_DEL_SVC_DOWNTIME);

		printf("<TABLE BORDER=0 CLASS='downtime'>\n");

		printf("<TR><TD colspan='%d' align='right'>", colspan);

		if (display_type == DISPLAY_DOWNTIME && type == HOST_DOWNTIME) {
			printf("<table width='100%%' cellspacing=0 cellpadding=0><tr><td width='33%%'></td><td width='33%%' nowrap>");
			printf("<div class='page_selector'>\n");
			printf("<div id='page_navigation_copy'></div>\n");
			page_limit_selector(result_start);
			printf("</div>\n");
			printf("</td><td width='33%%' align='right'>\n");
		}

		if (is_authorized_for_downtimes_read_only(&current_authdata) == FALSE)
			printf("<input type='submit' name='CommandButton' value='Delete Downtimes' disabled=\"disabled\">");

		if (display_type == DISPLAY_DOWNTIME && type == HOST_DOWNTIME)
			printf("</td></tr></table>");

		printf("</TD></TR>\n");

		printf("<TR CLASS='downtime'>");
		if (display_type == DISPLAY_DOWNTIME) {
			printf("<TH CLASS='downtime'>Host Name</TH>");
			if (type == SERVICE_DOWNTIME)
				printf("<TH CLASS='downtime'>Service</TH>");
		}
		printf("<TH CLASS='downtime'>Entry Time</TH><TH CLASS='downtime'>Author</TH><TH CLASS='downtime'>Comment</TH><TH CLASS='downtime'>Start Time</TH><TH CLASS='downtime'>End Time</TH><TH CLASS='downtime'>Type</TH><TH CLASS='downtime'>Trigger Time</TH><TH CLASS='downtime'>Duration</TH><TH CLASS='downtime'>Is in effect</TH><TH CLASS='downtime'>Downtime ID</TH><TH CLASS='downtime'>Trigger ID</TH>");
		if (is_authorized_for_downtimes_read_only(&current_authdata) == FALSE)
			printf("<TH CLASS='comment' nowrap>Actions&nbsp;&nbsp;<input type='checkbox' value='all' onclick=\"checkAll(\'tableform%sdowntime\');isValidForSubmit(\'tableform%sdowntime\');\"></TH>", (type == HOST_DOWNTIME) ? "host" : "service", (type == HOST_DOWNTIME) ? "host" : "service");
		printf("</TR>\n");
	}

	/* display all the downtime */
	for (temp_downtime = scheduled_downtime_list, total_downtime = 0; temp_downtime != NULL; temp_downtime = temp_downtime->next) {

		if (type == HOST_DOWNTIME && temp_downtime->type != HOST_DOWNTIME)
			continue;

		if (type == SERVICE_DOWNTIME && temp_downtime->type != SERVICE_DOWNTIME)
			continue;

		if (display_type != DISPLAY_DOWNTIME) {
			/* if not our host -> continue */
			if (strcmp(temp_downtime->host_name, host_name))
				continue;

			if (type == SERVICE_DOWNTIME) {
				/* if not our service -> continue */
				if (strcmp(temp_downtime->service_description, service_desc))
					continue;
			}
		} else {
			temp_host = find_host(temp_downtime->host_name);

			/* make sure the user has rights to view host information */
			if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
				continue;

			if (type == SERVICE_DOWNTIME) {
				temp_service = find_service(temp_downtime->host_name, temp_downtime->service_description);

				/* make sure the user has rights to view service information */
				if (is_authorized_for_service(temp_service, &current_authdata) == FALSE)
					continue;
			}
		}

		if (result_limit != 0  && (((total_entries + 1) < result_start) || (total_entries >= ((result_start + result_limit) - 1)))) {
			total_entries++;
			continue;
		}

		displayed_entries++;
		total_entries++;

		if (odd) {
			odd = 0;
			bg_class = "downtimeOdd";
		} else {
			odd = 1;
			bg_class = "downtimeEven";
		}

		if (content_type == JSON_CONTENT) {
			// always add a comma, except for the first line
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;

			printf("{ ");
			if (display_type == DISPLAY_DOWNTIME) {
				printf("\"host_name\": \"%s\", ", json_encode(temp_host->name));
				printf("\"host_display_name\": \"%s\", ", (temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(temp_host->name));
				if (type == SERVICE_DOWNTIME) {
					printf("\"service_description\": \"%s\", ", json_encode(temp_service->description));
					printf("\"service_display_name\": \"%s\", ", (temp_service->display_name != NULL) ? json_encode(temp_service->display_name) : json_encode(temp_service->description));
				}
			}
		} else if (content_type == CSV_CONTENT) {
			if (display_type == DISPLAY_DOWNTIME) {
				printf("%s%s%s%s", csv_data_enclosure, temp_host->name, csv_data_enclosure, csv_delimiter);
				if (type == SERVICE_DOWNTIME)
					printf("%s%s%s%s", csv_data_enclosure, temp_service->description, csv_data_enclosure, csv_delimiter);
				else
					printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
			}
		} else {
			printf("<tr CLASS='%s' onClick=\"toggle_checkbox('downtime_%lu','tableform%sdowntime');\">", bg_class, temp_downtime->downtime_id, (type == HOST_DOWNTIME) ? "host" : "service");
			if (display_type == DISPLAY_DOWNTIME) {
				printf("<td CLASS='%s'><A HREF='%s?type=%d&host=%s'>%s</A></td>", bg_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_downtime->host_name), (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);
				if (type == SERVICE_DOWNTIME) {
					printf("<td CLASS='%s'><A HREF='%s?type=%d&host=%s", bg_class, EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_downtime->host_name));
					printf("&service=%s'>%s</A></td>", url_encode(temp_downtime->service_description), (temp_service->display_name != NULL) ? temp_service->display_name : temp_service->description);
				}
			}
		}

		get_time_string(&temp_downtime->entry_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		if (content_type == JSON_CONTENT) {
			printf("\"entry_time\": \"%s\", ", date_time);
			if (temp_downtime->author == NULL)
				printf("\"author\": null, ");
			else
				printf("\"author\": \"%s\", ", json_encode(temp_downtime->author));
			if (temp_downtime->author == NULL)
				printf("\"comment\": null, ");
			else
				printf("\"comment\": \"%s\", ", json_encode(temp_downtime->comment));
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_downtime->author == NULL) ? "N/A" : temp_downtime->author, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_downtime->comment == NULL) ? "N/A" : temp_downtime->comment, csv_data_enclosure, csv_delimiter);
		} else {
			printf("<td CLASS='%s'>%s</td>", bg_class, date_time);
			printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->author == NULL) ? "N/A" : temp_downtime->author);
			printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->comment == NULL) ? "N/A" : temp_downtime->comment);
		}

		get_time_string(&temp_downtime->start_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		if (content_type == JSON_CONTENT)
			printf("\"start_time\": \"%s\", ", date_time);
		else if (content_type == CSV_CONTENT)
			printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
		else
			printf("<td CLASS='%s'>%s</td>", bg_class, date_time);

		get_time_string(&temp_downtime->end_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		if (content_type == JSON_CONTENT) {
			printf("\"end_time\": \"%s\", ", date_time);
			printf("\"type\": \"%s\", ", (temp_downtime->fixed == TRUE) ? "Fixed" : "Flexible");
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_downtime->fixed == TRUE) ? "Fixed" : "Flexible", csv_data_enclosure, csv_delimiter);
		} else {
			printf("<td CLASS='%s'>%s</td>", bg_class, date_time);
			printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->fixed == TRUE) ? "Fixed" : "Flexible");
		}

		get_time_string(&temp_downtime->trigger_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		if (content_type == JSON_CONTENT) {
			printf("\"trigger_time\": \"%s\", ", date_time);
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
		} else {
			printf("<td CLASS='%s'>%s</td>", bg_class, date_time);
		}

		get_time_breakdown(temp_downtime->duration, &days, &hours, &minutes, &seconds);
		if (content_type == JSON_CONTENT) {
			printf("\"duration\": \"%dd %dh %dm %ds\", ", days, hours, minutes, seconds);
			printf("\"is_in_effect\": %s, ", (temp_downtime->is_in_effect == TRUE) ? "true" : "false");
			printf("\"downtime_id\": %lu, ", temp_downtime->downtime_id);
			printf("\"trigger_id\": \"");
		} else if (content_type == CSV_CONTENT) {
			printf("%s%dd %dh %dm %ds%s%s", csv_data_enclosure, days, hours, minutes, seconds, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_downtime->is_in_effect == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%lu%s%s", csv_data_enclosure, temp_downtime->downtime_id, csv_data_enclosure, csv_delimiter);
			printf("%s", csv_data_enclosure);
		} else {
			printf("<td CLASS='%s'>%dd %dh %dm %ds</td>", bg_class, days, hours, minutes, seconds);
			printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->is_in_effect == TRUE) ? "True" : "False");
			printf("<td CLASS='%s'>%lu</td>", bg_class, temp_downtime->downtime_id);
			printf("<td CLASS='%s'>", bg_class);
		}
		if (temp_downtime->triggered_by == 0) {
			if (content_type == JSON_CONTENT)
				printf("null");
			else
				printf("N/A");
		} else
			printf("%lu", temp_downtime->triggered_by);

		if (content_type == JSON_CONTENT) {
			printf("\" }\n");
		} else if (content_type == CSV_CONTENT) {
			printf("%s\n", csv_data_enclosure);
		} else {
			printf("</td>\n");
			if (is_authorized_for_downtimes_read_only(&current_authdata) == FALSE) {
				printf("<td align='center' CLASS='%s'><a href='%s?cmd_typ=%d", bg_class, CMD_CGI, (type == HOST_DOWNTIME) ? CMD_DEL_HOST_DOWNTIME : CMD_DEL_SVC_DOWNTIME);
				printf("&down_id=%lu'><img src='%s%s' border=0 ALT='Delete/Cancel This Scheduled Downtime Entry' TITLE='Delete/Cancel This Scheduled Downtime Entry'></a>", temp_downtime->downtime_id, url_images_path, DELETE_ICON);
				printf("<input type='checkbox' onClick=\"toggle_checkbox('downtime_%lu','tableform%sdowntime');\" name='down_id' id='downtime_%lu' value='%lu'></td>", temp_downtime->downtime_id, (type == HOST_DOWNTIME) ? "host" : "service", temp_downtime->downtime_id, temp_downtime->downtime_id);
			}
			printf("</tr>\n");
		}
		total_downtime++;
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		if (total_downtime == 0 && total_entries == 0) {
			printf("<TR CLASS='downtimeOdd'><TD  align='center' COLSPAN=%d>", colspan);
			if (display_type == DISPLAY_DOWNTIME)
				printf("There are no %s with scheduled downtime", (type == HOST_DOWNTIME) ? "host" : "service");
			else
				printf("This %s has no scheduled downtime associated with it", (type == HOST_DOWNTIME) ? "host" : "service");
			printf("</TD></TR>\n");
		}

		if (display_type == DISPLAY_DOWNTIME && type == SERVICE_DOWNTIME) {
			printf("<TR><TD colspan='%d'>\n", colspan);
			page_num_selector(result_start, total_entries, displayed_entries);
			printf("</TD></TR>\n");
		}

		printf("</TABLE>\n");
		printf("<script language='javascript' type='text/javascript'>\n");
		printf("document.tableform%sdowntime.buttonCheckboxChecked.value = 'false';\n", (type == HOST_DOWNTIME) ? "host" : "service");
		printf("checked = true;\n");
		printf("checkAll(\"tableform%sdowntime\");\n", (type == HOST_DOWNTIME) ? "host" : "service");
		printf("checked = false;\n");
		printf("</script>\n");
		printf("</FORM>\n");
	}
	if (content_type == JSON_CONTENT)
		printf("]");

	return;
}

/* shows check scheduling queue */
void show_scheduling_queue(void) {
	sortdata *temp_sortdata;
	host *temp_host = NULL;
	service *temp_service = NULL;
	servicestatus *temp_svcstatus = NULL;
	hoststatus *temp_hststatus = NULL;
	char date_time[MAX_DATETIME_LENGTH];
	char temp_url[MAX_INPUT_BUFFER];
	char service_link[MAX_INPUT_BUFFER];
	char action_link_enable_disable[MAX_INPUT_BUFFER];
	char action_link_schedule[MAX_INPUT_BUFFER];
	char host_native_name[MAX_INPUT_BUFFER];
	char service_native_name[MAX_INPUT_BUFFER];
	char host_display_name[MAX_INPUT_BUFFER];
	char service_display_name[MAX_INPUT_BUFFER];
	char url_encoded_service[MAX_INPUT_BUFFER];
	char url_encoded_host[MAX_INPUT_BUFFER];
	char temp_buffer[MAX_INPUT_BUFFER];
	char *last_check = "", *next_check = "", *type = "";
	int checks_enabled = FALSE;
	int odd = 0;
	char *bgclass = "";
	int json_start = TRUE;

	/* sort hosts and services */
	sort_data(sort_type, sort_option);

	if (content_type == JSON_CONTENT) {
		printf("\"scheduling_queue\": [\n");
	} else if (content_type == CSV_CONTENT) {
		/* csv header line */
		printf("%sHOST%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sSERVICE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLAST_CHECK%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNEXT_CHECK%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sTYPE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sACTIVE_CHECKS%s\n", csv_data_enclosure, csv_data_enclosure);
	} else {
		printf("<DIV ALIGN=CENTER CLASS='statusSort'>Entries sorted by <b>");
		if (sort_option == SORT_HOSTNAME)
			printf("host name");
		else if (sort_option == SORT_SERVICENAME)
			printf("service name");
		else if (sort_option == SORT_SERVICESTATUS)
			printf("service status");
		else if (sort_option == SORT_LASTCHECKTIME)
			printf("last check time");
		else if (sort_option == SORT_NEXTCHECKTIME)
			printf("next check time");
		printf("</b> (%s)\n", (sort_type == SORT_ASCENDING) ? "ascending" : "descending");
		printf("</DIV>\n");

		printf("<TABLE BORDER=0 CLASS='queue' align='center'>\n");

		/* add export to csv link */
		printf("<TR><TD colspan='7'>\n");
		printf("<table width='100%%' cellspacing=0 cellpadding=0><tr><td width='15%%'></td><td width='70%%' nowrap>");

		printf("<div class='page_selector'>\n");
		printf("<div id='page_navigation_copy'></div>\n");
		page_limit_selector(result_start);
		printf("</div>\n");

		printf("</td><td width='15%%' align='right'>\n<DIV class='csv_export_link'>\n");
		print_export_link(CSV_CONTENT, EXTINFO_CGI, NULL);
		print_export_link(JSON_CONTENT, EXTINFO_CGI, NULL);
		print_export_link(HTML_CONTENT, EXTINFO_CGI, NULL);
		printf("</DIV>\n");
		printf("</td></tr></table>");
		printf("</TD></TR>\n");

		printf("<TR CLASS='queue'>");

		snprintf(temp_url, sizeof(temp_url) - 1, "%s?type=%d", EXTINFO_CGI, DISPLAY_SCHEDULING_QUEUE);
		temp_url[sizeof(temp_url) - 1] = '\x0';

		if (host_name && *host_name != '\0') {
			strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
			snprintf(temp_url, sizeof(temp_url) - 1, "%s&host=%s", temp_buffer, url_encode(host_name));
			temp_url[sizeof(temp_url) - 1] = '\x0';
		}

		if (service_desc && *service_desc != '\0') {
			strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
			snprintf(temp_url, sizeof(temp_url) - 1, "%s&service=%s", temp_buffer, url_encode(service_desc));
			temp_url[sizeof(temp_url) - 1] = '\x0';
		}

		printf("<TH CLASS='queue'>Host&nbsp;<A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by host name (ascending)' TITLE='Sort by host name (ascending)'></A><A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by host name (descending)' TITLE='Sort by host name (descending)'></A></TH>", temp_url, SORT_ASCENDING, SORT_HOSTNAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_HOSTNAME, url_images_path, DOWN_ARROW_ICON);
		printf("<TH CLASS='queue'>Service&nbsp;<A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by service name (ascending)' TITLE='Sort by service name (ascending)'></A><A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by service name (descending)' TITLE='Sort by service name (descending)'></A></TH>", temp_url, SORT_ASCENDING, SORT_SERVICENAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_SERVICENAME, url_images_path, DOWN_ARROW_ICON);
		printf("<TH CLASS='queue'>Last Check&nbsp;<A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by last check time (ascending)' TITLE='Sort by last check time (ascending)'></A><A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by last check time (descending)' TITLE='Sort by last check time (descending)'></A></TH>", temp_url, SORT_ASCENDING, SORT_LASTCHECKTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_LASTCHECKTIME, url_images_path, DOWN_ARROW_ICON);
		printf("<TH CLASS='queue'>Next Check&nbsp;<A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by next check time (ascending)' TITLE='Sort by next check time (ascending)'></A><A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by next check time (descending)' TITLE='Sort by next check time (descending)'></A></TH>", temp_url, SORT_ASCENDING, SORT_NEXTCHECKTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_NEXTCHECKTIME, url_images_path, DOWN_ARROW_ICON);
		printf("<TH CLASS='queue'>Type</TH><TH CLASS='queue'>Active Checks</TH><TH CLASS='queue'>Actions</TH></TR>\n");
	}

	/* display all services and hosts */
	for (temp_sortdata = sortdata_list; temp_sortdata != NULL; temp_sortdata = temp_sortdata->next) {

		/* skip hosts and services that shouldn't be scheduled */
		if (temp_sortdata->is_service == TRUE) {
			temp_svcstatus = temp_sortdata->svcstatus;
			if (temp_svcstatus->should_be_scheduled == FALSE) {
				/* passive-only checks should appear if they're being forced */
				if (!(temp_svcstatus->checks_enabled == FALSE && temp_svcstatus->next_check != (time_t)0L && (temp_svcstatus->check_options & CHECK_OPTION_FORCE_EXECUTION)))
					continue;
			}
			if (host_name && *host_name != '\0' && strcmp(host_name, temp_svcstatus->host_name))
				continue;

			if (service_desc && *service_desc != '\0' && strcmp(service_desc, temp_svcstatus->description))
				continue;

		} else {
			temp_hststatus = temp_sortdata->hststatus;
			if (temp_hststatus->should_be_scheduled == FALSE) {
				/* passive-only checks should appear if they're being forced */
				if (!(temp_hststatus->checks_enabled == FALSE && temp_hststatus->next_check != (time_t)0L && (temp_hststatus->check_options & CHECK_OPTION_FORCE_EXECUTION)))
					continue;
			}
			if (host_name && *host_name != '\0' && strcmp(host_name, temp_hststatus->host_name))
				continue;

			/* skip host if users just want to see a service */
			if (service_desc && *service_desc != '\0')
				continue;
		}

		/* get the service status */
		if (temp_sortdata->is_service == TRUE) {

			/* find the host */
			temp_host = find_host(temp_svcstatus->host_name);

			if (temp_host == NULL)
				continue;

			/* make sure user has rights to see this... */
			if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
				continue;

			snprintf(url_encoded_host, sizeof(url_encoded_host) - 1, "%s", url_encode(temp_svcstatus->host_name));
			url_encoded_host[sizeof(url_encoded_host) - 1] = '\x0';

			/* find the service */
			temp_service = find_service(temp_svcstatus->host_name, temp_svcstatus->description);

			/* if we couldn't find the service, go to the next service */
			if (temp_service == NULL)
				continue;

			/* make sure user has rights to see this... */
			if (is_authorized_for_service(temp_service, &current_authdata) == FALSE)
				continue;

			snprintf(url_encoded_service, sizeof(url_encoded_service) - 1, "%s", url_encode(temp_svcstatus->description));
			url_encoded_service[sizeof(url_encoded_service) - 1] = '\x0';

			/* host name */
			snprintf(host_native_name, sizeof(host_native_name) - 1, "%s", temp_svcstatus->host_name);
			host_native_name[sizeof(host_native_name) - 1] = '\x0';
			snprintf(host_display_name, sizeof(host_display_name) - 1, "%s", (temp_host != NULL && temp_host->display_name != NULL) ? temp_host->display_name : temp_hststatus->host_name);
			host_display_name[sizeof(host_display_name) - 1] = '\x0';

			/* service name */
			snprintf(service_native_name, sizeof(service_native_name) - 1, "%s", temp_svcstatus->description);
			service_native_name[sizeof(service_native_name) - 1] = '\x0';
			snprintf(service_display_name, sizeof(service_display_name) - 1, "%s", (temp_service != NULL && temp_service->display_name != NULL) ? temp_service->display_name : temp_svcstatus->description);
			service_display_name[sizeof(service_display_name) - 1] = '\x0';

			/* service link*/
			snprintf(service_link, sizeof(service_link) - 1, "%s?type=%d&host=%s&service=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encoded_host, url_encoded_service);
			service_link[sizeof(service_link) - 1] = '\x0';

			/* last check */
			get_time_string(&temp_svcstatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			last_check = (temp_svcstatus->last_check == (time_t)0) ? "N/A" : strdup(date_time);

			/* next check */
			get_time_string(&temp_svcstatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			next_check = (temp_svcstatus->next_check == (time_t)0) ? "N/A" : strdup(date_time);

			/* type */
			if (temp_svcstatus->check_options == CHECK_OPTION_NONE)
				type = "Normal";
			else {
				if (temp_svcstatus->check_options & CHECK_OPTION_FORCE_EXECUTION)
					type = "Forced";
				if (temp_svcstatus->check_options & CHECK_OPTION_FRESHNESS_CHECK)
					type = "Freshness";
				if (temp_svcstatus->check_options & CHECK_OPTION_ORPHAN_CHECK)
					type = "Orphan";
			}

			/* active checks */
			checks_enabled = temp_svcstatus->checks_enabled;

			/* action links */
			if (temp_svcstatus->checks_enabled == TRUE)
				snprintf(action_link_enable_disable, sizeof(action_link_enable_disable) - 1, "%s?cmd_typ=%d&host=%s&service=%s", CMD_CGI, CMD_DISABLE_SVC_CHECK, url_encoded_host, url_encoded_service);
			else
				snprintf(action_link_enable_disable, sizeof(action_link_enable_disable) - 1, "%s?cmd_typ=%d&host=%s&service=%s", CMD_CGI, CMD_ENABLE_SVC_CHECK, url_encoded_host, url_encoded_service);
			action_link_enable_disable[sizeof(action_link_enable_disable) - 1] = '\x0';

			snprintf(action_link_schedule, sizeof(action_link_schedule) - 1, "%s?cmd_typ=%d&host=%s&service=%s%s", CMD_CGI, CMD_SCHEDULE_SVC_CHECK, url_encoded_host, url_encoded_service, (temp_svcstatus->checks_enabled == TRUE) ? "&force_check" : "");
			action_link_schedule[sizeof(action_link_schedule) - 1] = '\x0';

			/* get the host status */
		} else {
			/* find the host */
			temp_host = find_host(temp_hststatus->host_name);

			if (temp_host == NULL)
				continue;

			/* make sure user has rights to see this... */
			if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
				continue;

			snprintf(url_encoded_host, sizeof(url_encoded_host) - 1, "%s", url_encode(temp_hststatus->host_name));
			url_encoded_host[sizeof(url_encoded_host) - 1] = '\x0';

			/* host name */
			snprintf(host_native_name, sizeof(host_native_name) - 1, "%s", temp_hststatus->host_name);
			host_native_name[sizeof(host_native_name) - 1] = '\x0';
			snprintf(host_display_name, sizeof(host_display_name) - 1, "%s", (temp_host != NULL && temp_host->display_name != NULL) ? temp_host->display_name : temp_hststatus->host_name);
			host_display_name[sizeof(host_display_name) - 1] = '\x0';

			/* last check */
			get_time_string(&temp_hststatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			last_check = (temp_hststatus->last_check == (time_t)0) ? "N/A" : strdup(date_time);

			/* next check */
			get_time_string(&temp_hststatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			next_check = (temp_hststatus->next_check == (time_t)0) ? "N/A" : strdup(date_time);

			/* type */
			if (temp_hststatus->check_options == CHECK_OPTION_NONE)
				type = "Normal";
			else {
				if (temp_hststatus->check_options & CHECK_OPTION_FORCE_EXECUTION)
					type = "Forced";
				if (temp_hststatus->check_options & CHECK_OPTION_FRESHNESS_CHECK)
					type = "Freshness";
				if (temp_hststatus->check_options & CHECK_OPTION_ORPHAN_CHECK)
					type = "Orphan";
			}

			/* active checks */
			checks_enabled = temp_hststatus->checks_enabled;

			/* action links */
			if (temp_hststatus->checks_enabled == TRUE)
				snprintf(action_link_enable_disable, sizeof(action_link_enable_disable) - 1, "%s?cmd_typ=%d&host=%s", CMD_CGI, CMD_DISABLE_HOST_CHECK, url_encoded_host);
			else
				snprintf(action_link_enable_disable, sizeof(action_link_enable_disable) - 1, "%s?cmd_typ=%d&host=%s", CMD_CGI, CMD_ENABLE_HOST_CHECK, url_encoded_host);
			action_link_enable_disable[sizeof(action_link_enable_disable) - 1] = '\x0';

			snprintf(action_link_schedule, sizeof(action_link_schedule) - 1, "%s?cmd_typ=%d&host=%s%s", CMD_CGI, CMD_SCHEDULE_HOST_CHECK, url_encoded_host, (temp_hststatus->checks_enabled == TRUE) ? "&force_check" : "");
			action_link_schedule[sizeof(action_link_schedule) - 1] = '\x0';
		}

		if (result_limit != 0  && (((total_entries + 1) < result_start) || (total_entries >= ((result_start + result_limit) - 1)))) {
			total_entries++;
			my_free(last_check);
			my_free(next_check);
			continue;
		}

		displayed_entries++;
		total_entries++;

		if (odd) {
			odd = 0;
			bgclass = "Even";
		} else {
			odd = 1;
			bgclass = "Odd";
		}

		if (content_type == JSON_CONTENT) {
			// always add a comma, except for the first line
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;

			printf("{ \"host_name\": \"%s\", ", json_encode(host_native_name));
			printf("\"host_display_name\": \"%s\", ", json_encode(host_display_name));
			if (temp_sortdata->is_service == TRUE) {
				printf("\"service_description\": \"%s\", ", json_encode(service_native_name));
				printf("\"service_display_name\": \"%s\", ", json_encode(service_display_name));
				printf("\"type\": \"SERVICE_CHECK\", ");
			} else
				printf("\"type\": \"HOST_CHECK\", ");

			printf("\"last_check\": \"%s\", ", last_check);
			printf("\"next_check\": \"%s\", ", next_check);
			printf("\"type\": \"%s\", ", type);
			printf("\"active_check\": %s }", (checks_enabled == TRUE) ? "true" : "false");
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, host_native_name, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_sortdata->is_service == TRUE) ? service_native_name : "", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, last_check, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, next_check, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, type, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s\n", csv_data_enclosure, (checks_enabled == TRUE) ? "ENABLED" : "DISABLED", csv_data_enclosure);
		} else {
			printf("<TR CLASS='queue%s'>", bgclass);

			/* Host */
			printf("<TD CLASS='queue%s'><A HREF='%s?type=%d&host=%s'>%s</A></TD>", bgclass, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encoded_host, html_encode(host_display_name,TRUE));

			/* Service */
			if (temp_sortdata->is_service == TRUE)
				printf("<TD CLASS='queue%s'><A HREF='%s'>%s</A></TD>", bgclass, service_link, html_encode(service_display_name,TRUE));
			else
				printf("<TD CLASS='queue%s'>&nbsp;</TD>", bgclass);

			/* last check */
			printf("<TD CLASS='queue%s'>%s</TD>", bgclass, last_check);

			/* next check */
			printf("<TD CLASS='queue%s'>%s</TD>", bgclass, next_check);

			/* type */
			printf("<TD align='center' CLASS='queue%s'>%s</TD>", bgclass, type);

			/* active checks */
			printf("<TD CLASS='queue%s'>%s</TD>", (checks_enabled == TRUE) ? "ENABLED" : "DISABLED", (checks_enabled == TRUE) ? "ENABLED" : "DISABLED");

			/* actions */
			printf("<TD align='center' CLASS='queue%s'>", bgclass);
			printf("<a href='%s'><img src='%s%s' border=0 ALT='%s Active Checks Of This %s' TITLE='%s Active Checks Of This %s'></a>\n", action_link_enable_disable, url_images_path, (checks_enabled == TRUE) ? DISABLED_ICON : ENABLED_ICON, (checks_enabled == TRUE) ? "Disable" : "Enable", (temp_sortdata->is_service == TRUE) ? "Service" : "Host", (checks_enabled == TRUE) ? "Disable" : "Enable", (temp_sortdata->is_service == TRUE) ? "Service" : "Host");
			printf("<a href='%s'><img src='%s%s' border=0 ALT='Re-schedule This %s Check' TITLE='Re-schedule This %s Check'></a>", action_link_schedule, url_images_path, DELAY_ICON, (temp_sortdata->is_service == TRUE) ? "Service" : "Host", (temp_sortdata->is_service == TRUE) ? "Service" : "Host");

			printf("</TD></TR>\n");
		}

		my_free(last_check);
		my_free(next_check);
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("<TR><TD colspan='7'>\n");
		page_num_selector(result_start, total_entries, displayed_entries);
		printf("</TD></TR>\n");
		printf("</TABLE>\n");
	} else if (content_type == JSON_CONTENT)
		printf(" ] \n");

	/* free memory allocated to sorted data list */
	free_sortdata_list();

	return;
}

/* sorts host and service data */
int sort_data(int s_type, int s_option) {
	sortdata *new_sortdata;
	sortdata *last_sortdata;
	sortdata *temp_sortdata;
	servicestatus *temp_svcstatus;
	hoststatus *temp_hststatus;

	if (s_type == SORT_NONE)
		return ERROR;

	/* sort all service status entries */
	for (temp_svcstatus = servicestatus_list; temp_svcstatus != NULL; temp_svcstatus = temp_svcstatus->next) {

		/* allocate memory for a new sort structure */
		new_sortdata = (sortdata *)malloc(sizeof(sortdata));
		if (new_sortdata == NULL)
			return ERROR;

		new_sortdata->is_service = TRUE;
		new_sortdata->svcstatus = temp_svcstatus;
		new_sortdata->hststatus = NULL;

		last_sortdata = sortdata_list;
		for (temp_sortdata = sortdata_list; temp_sortdata != NULL; temp_sortdata = temp_sortdata->next) {

			if (compare_sortdata_entries(s_type, s_option, new_sortdata, temp_sortdata) == TRUE) {
				new_sortdata->next = temp_sortdata;
				if (temp_sortdata == sortdata_list)
					sortdata_list = new_sortdata;
				else
					last_sortdata->next = new_sortdata;
				break;
			} else
				last_sortdata = temp_sortdata;
		}

		if (sortdata_list == NULL) {
			new_sortdata->next = NULL;
			sortdata_list = new_sortdata;
		} else if (temp_sortdata == NULL) {
			new_sortdata->next = NULL;
			last_sortdata->next = new_sortdata;
		}
	}

	/* sort all host status entries */
	for (temp_hststatus = hoststatus_list; temp_hststatus != NULL; temp_hststatus = temp_hststatus->next) {

		/* allocate memory for a new sort structure */
		new_sortdata = (sortdata *)malloc(sizeof(sortdata));
		if (new_sortdata == NULL)
			return ERROR;

		new_sortdata->is_service = FALSE;
		new_sortdata->svcstatus = NULL;
		new_sortdata->hststatus = temp_hststatus;

		last_sortdata = sortdata_list;
		for (temp_sortdata = sortdata_list; temp_sortdata != NULL; temp_sortdata = temp_sortdata->next) {

			if (compare_sortdata_entries(s_type, s_option, new_sortdata, temp_sortdata) == TRUE) {
				new_sortdata->next = temp_sortdata;
				if (temp_sortdata == sortdata_list)
					sortdata_list = new_sortdata;
				else
					last_sortdata->next = new_sortdata;
				break;
			} else
				last_sortdata = temp_sortdata;
		}

		if (sortdata_list == NULL) {
			new_sortdata->next = NULL;
			sortdata_list = new_sortdata;
		} else if (temp_sortdata == NULL) {
			new_sortdata->next = NULL;
			last_sortdata->next = new_sortdata;
		}
	}

	return OK;
}

int compare_sortdata_entries(int s_type, int s_option, sortdata *new_sortdata, sortdata *temp_sortdata) {
	hoststatus *temp_hststatus = NULL;
	servicestatus *temp_svcstatus = NULL;
	time_t last_check[2];
	time_t next_check[2];
	int current_attempt[2];
	int status[2];
	char *host_name[2];
	char *service_description[2];

	if (new_sortdata->is_service == TRUE) {
		temp_svcstatus = new_sortdata->svcstatus;
		last_check[0] = temp_svcstatus->last_check;
		next_check[0] = temp_svcstatus->next_check;
		status[0] = temp_svcstatus->status;
		host_name[0] = temp_svcstatus->host_name;
		service_description[0] = temp_svcstatus->description;
		current_attempt[0] = temp_svcstatus->current_attempt;
	} else {
		temp_hststatus = new_sortdata->hststatus;
		last_check[0] = temp_hststatus->last_check;
		next_check[0] = temp_hststatus->next_check;
		status[0] = temp_hststatus->status;
		host_name[0] = temp_hststatus->host_name;
		service_description[0] = "";
		current_attempt[0] = temp_hststatus->current_attempt;
	}
	if (temp_sortdata->is_service == TRUE) {
		temp_svcstatus = temp_sortdata->svcstatus;
		last_check[1] = temp_svcstatus->last_check;
		next_check[1] = temp_svcstatus->next_check;
		status[1] = temp_svcstatus->status;
		host_name[1] = temp_svcstatus->host_name;
		service_description[1] = temp_svcstatus->description;
		current_attempt[1] = temp_svcstatus->current_attempt;
	} else {
		temp_hststatus = temp_sortdata->hststatus;
		last_check[1] = temp_hststatus->last_check;
		next_check[1] = temp_hststatus->next_check;
		status[1] = temp_hststatus->status;
		host_name[1] = temp_hststatus->host_name;
		service_description[1] = "";
		current_attempt[1] = temp_hststatus->current_attempt;
	}

	if (s_type == SORT_ASCENDING) {

		if (s_option == SORT_LASTCHECKTIME) {
			if (last_check[0] <= last_check[1])
				return TRUE;
			else
				return FALSE;
		}
		if (s_option == SORT_NEXTCHECKTIME) {
			if (next_check[0] <= next_check[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_CURRENTATTEMPT) {
			if (current_attempt[0] <= current_attempt[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_SERVICESTATUS) {
			if (status[0] <= status[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_HOSTNAME) {
			if (strcasecmp(host_name[0], host_name[1]) < 0)
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_SERVICENAME) {
			if (strcasecmp(service_description[0], service_description[1]) < 0)
				return TRUE;
			else
				return FALSE;
		}
	} else {
		if (s_option == SORT_LASTCHECKTIME) {
			if (last_check[0] > last_check[1])
				return TRUE;
			else
				return FALSE;
		}
		if (s_option == SORT_NEXTCHECKTIME) {
			if (next_check[0] > next_check[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_CURRENTATTEMPT) {
			if (current_attempt[0] > current_attempt[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_SERVICESTATUS) {
			if (status[0] > status[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_HOSTNAME) {
			if (strcasecmp(host_name[0], host_name[1]) > 0)
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_SERVICENAME) {
			if (strcasecmp(service_description[0], service_description[1]) > 0)
				return TRUE;
			else
				return FALSE;
		}
	}

	return TRUE;
}

/* free all memory allocated to the sortdata structures */
void free_sortdata_list(void) {
	sortdata *this_sortdata;
	sortdata *next_sortdata;

	/* free memory for the sortdata list */
	for (this_sortdata = sortdata_list; this_sortdata != NULL; this_sortdata = next_sortdata) {
		next_sortdata = this_sortdata->next;
		free(this_sortdata);
	}

	return;
}

/* determines whether or not a specific host is an child of another host */
/* NOTE: this could be expensive in large installations, so use with care! */
int is_host_child_of_host(host *parent_host, host *child_host) {
	host *temp_host;

	/* not enough data */
	if (child_host == NULL)
		return FALSE;

	/* root/top-level hosts */
	if (parent_host == NULL) {
		if (child_host->parent_hosts == NULL)
			return TRUE;

		/* mid-level/bottom hosts */
	} else {

		for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {

			/* skip this host if it is not a child */
			if (is_host_immediate_child_of_host(parent_host, temp_host) == FALSE)
				continue;
			else {
				if (!strcmp(temp_host->name, child_host->name))
					return TRUE;
				else {
					if (is_host_child_of_host(temp_host, child_host) == FALSE)
						continue;

					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

