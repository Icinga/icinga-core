/*****************************************************************************
 *
 * EXTINFO.C -  Icinga Extended Information CGI
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2014 Icinga Development Team (http://www.icinga.org)
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

#include "../include/cgiutils.h"
#include "../include/getcgi.h"
#include "../include/cgiauth.h"

extern char             nagios_check_command[MAX_INPUT_BUFFER];
extern char             nagios_process_info[MAX_INPUT_BUFFER];

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
extern int              buffer_stats[1][3];
extern int              program_stats[MAX_CHECK_STATS_TYPES][3];
extern char		*status_file_icinga_version;

extern int              suppress_maintenance_downtime;
extern int		extinfo_show_child_hosts;
extern int		tab_friendly_titles;
extern int		result_limit;

extern char		main_config_file[MAX_FILENAME_LENGTH];
extern char		url_images_path[MAX_FILENAME_LENGTH];
extern char		url_logo_images_path[MAX_FILENAME_LENGTH];

extern int              enable_splunk_integration;

extern char             *notes_url_target;
extern char             *action_url_target;

extern host		*host_list;
extern service		*service_list;
extern hoststatus	*hoststatus_list;
extern servicestatus	*servicestatus_list;

extern hostgroup	*hostgroup_list;
extern servicegroup	*servicegroup_list;

extern servicedependency  *servicedependency_list;
extern hostdependency     *hostdependency_list;

extern comment		  *comment_list;
extern scheduled_downtime *scheduled_downtime_list;


#define MAX_MESSAGE_BUFFER		4096

#define HEALTH_WARNING_PERCENTAGE       85
#define HEALTH_CRITICAL_PERCENTAGE      75

/* this is only necessary to distinguish between comments and downtime in single host/service view */
#define CSV_DEFAULT			0
#define CSV_COMMENT			1
#define CSV_DOWNTIME			2

/* SORTDATA structure */
typedef struct sortdata_struct {
	int data_type;
	servicestatus *svcstatus;
	hoststatus *hststatus;
	comment *comment;
	scheduled_downtime *downtime;
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
int sort_option = SORT_NOTHING;
int csv_type = CSV_DEFAULT;
int get_result_limit = -1;
int result_start = 1;
int total_entries = 0;
int displayed_entries = 0;

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
	char *last_hd_hostname = "";
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
		print_error(get_cgi_config_location(), ERROR_CGI_CFG_FILE, FALSE);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read the main configuration file */
	result = read_main_config_file(main_config_file);
	if (result == ERROR) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(main_config_file, ERROR_CGI_MAIN_CFG, FALSE);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read all object configuration data */
	result = read_all_object_configuration_data(main_config_file, READ_ALL_OBJECT_DATA);
	if (result == ERROR) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(NULL, ERROR_CGI_OBJECT_DATA, FALSE);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read all status data */
	result = read_all_status_data(main_config_file, READ_ALL_STATUS_DATA);
	if (result == ERROR && daemon_check == TRUE) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(NULL, ERROR_CGI_STATUS_DATA, FALSE);
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
			asprintf(&cgi_title, "[%s]", html_encode(host_name, FALSE));
		else if (display_type == DISPLAY_SERVICE_INFO && service_desc && *service_desc != '\0' && host_name && *host_name != '\0')
			asprintf(&cgi_title, "%s @ %s", html_encode(service_desc, FALSE), html_encode(host_name, FALSE));
		else if (display_type == DISPLAY_HOSTGROUP_INFO && hostgroup_name && *hostgroup_name != '\0')
			asprintf(&cgi_title, "{%s}", html_encode(hostgroup_name, FALSE));
		else if (display_type == DISPLAY_SERVICEGROUP_INFO && servicegroup_name && *servicegroup_name != '\0')
			asprintf(&cgi_title, "(%s)", html_encode(servicegroup_name, FALSE));
	}

	document_header(CGI_ID, TRUE, (tab_friendly_titles == TRUE && cgi_title != NULL) ? cgi_title : "Extended Information");

	my_free(cgi_title);

	/* get authentication information */
	get_authentication_information(&current_authdata);


	if (display_header == TRUE) {

		/* begin top table */
		printf("<table border='0' width='100%%'>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align='left' valign='top' width='33%%'>\n");

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

			printf("<table border='1' cellpadding='0' cellspacing='0' class='linkBox'>\n");
			printf("<tr><td class='linkBox'>\n");
			if (display_type == DISPLAY_SERVICE_INFO)
				printf("<a href='%s?type=%d&amp;host=%s'>View <b>Information</b> For <b>This Host</b></a><br>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(host_name));
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
				printf("<a href='%s?host=%s&amp;show_log_entries'>View <b>Availability Report</b> For <b>This Host</b></a><br>\n", AVAIL_CGI, url_encode(host_name));
				printf("<a href='%s?host=%s'>View <b>Notifications</b> For <b>This Host</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(host_name));
				printf("<a href='%s?type=%d&amp;host=%s'>View <b>Scheduling Queue</b> For <b>This Host</b></a><br>\n", EXTINFO_CGI, DISPLAY_SCHEDULING_QUEUE, url_encode(host_name));
				if (is_authorized_for_configuration_information(&current_authdata) == TRUE)
					printf("<a href='%s?type=hosts&amp;item_name=%s'>View <b>Config</b> For <b>This Host</b></a>\n", CONFIG_CGI, url_encode(host_name));
			} else if (display_type == DISPLAY_SERVICE_INFO) {
				printf("<a href='%s?host=%s&amp;service=%s'>View <b>Alert History</b> For <b>This Service</b></a><br>\n", HISTORY_CGI, url_encode(host_name), url_encode(service_desc));
#ifdef USE_TRENDS
				printf("<a href='%s?host=%s&amp;service=%s'>View <b>Trends</b> For <b>This Service</b></a><br>\n", TRENDS_CGI, url_encode(host_name), url_encode(service_desc));
#endif
#ifdef USE_HISTOGRAM
				printf("<a href='%s?host=%s&amp;service=%s'>View <b>Alert Histogram</b> For <b>This Service</b></a><br>\n", HISTOGRAM_CGI, url_encode(host_name), url_encode(service_desc));
#endif
				printf("<a href='%s?host=%s&amp;service=%s&show_log_entries'>View <b>Availability Report</b> For <b>This Service</b></a><br>\n", AVAIL_CGI, url_encode(host_name), url_encode(service_desc));
				printf("<a href='%s?host=%s&amp;service=%s'>View <b>Notifications</b> For <b>This Service</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(host_name), url_encode(service_desc));
				printf("<a href='%s?type=%d&host=%s&amp;service=%s'>View <b>Scheduling Queue</b> For <b>This Service</b></a><br>\n", EXTINFO_CGI, DISPLAY_SCHEDULING_QUEUE, url_encode(host_name), url_encode(service_desc));
				if (is_authorized_for_configuration_information(&current_authdata) == TRUE)
					printf("<a href='%s?type=services&amp;item_name=%s^%s'>View <b>Config</b> For <b>This Service</b></a>\n", CONFIG_CGI, url_encode(host_name), url_encode(service_desc));
			} else if (display_type == DISPLAY_HOSTGROUP_INFO) {
				printf("<a href='%s?hostgroup=%s&amp;style=detail'>View <b>Status Detail</b> For <b>This Hostgroup</b></a><br>\n", STATUS_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s&amp;style=overview'>View <b>Status Overview</b> For <b>This Hostgroup</b></a><br>\n", STATUS_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s&amp;style=grid'>View <b>Status Grid</b> For <b>This Hostgroup</b></a><br>\n", STATUS_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s'>View <b>Alert History</b> For <b>This Hostgroup</b></a><br>\n", HISTORY_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s'>View <b>Availability Report</b> For <b>This Hostgroup</b></a><br>\n", AVAIL_CGI, url_encode(hostgroup_name));
				printf("<a href='%s?hostgroup=%s'>View <b>Notifications</b> For <b>This Hostgroup</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(hostgroup_name));
				if (is_authorized_for_configuration_information(&current_authdata) == TRUE)
					printf("<a href='%s?type=hostgroups&item_name=%s'>View <b>Config</b> For <b>This Hostgroup</b></a>\n", CONFIG_CGI, url_encode(hostgroup_name));
			} else if (display_type == DISPLAY_SERVICEGROUP_INFO) {
				printf("<a href='%s?servicegroup=%s&amp;style=detail'>View <b>Status Detail</b> For <b>This Servicegroup</b></a><br>\n", STATUS_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s&amp;style=overview'>View <b>Status Overview</b> For <b>This Servicegroup</b></a><br>\n", STATUS_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s&amp;style=grid'>View <b>Status Grid</b> For <b>This Servicegroup</b></a><br>\n", STATUS_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s'>View <b>Alert History</b> For <b>This Servicegroup</b></a><br>\n", HISTORY_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s'>View <b>Availability Report</b> For <b>This Servicegroup</b></a><br>\n", AVAIL_CGI, url_encode(servicegroup_name));
				printf("<a href='%s?servicegroup=%s'>View <b>Notifications</b> For <b>This Servicegroup</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(servicegroup_name));
				if (is_authorized_for_configuration_information(&current_authdata) == TRUE)
					printf("<a href='%s?type=servicegroups&amp;item_name=%s'>View <b>Config</b> For <b>This Servicegroup</b></a>\n", CONFIG_CGI, url_encode(servicegroup_name));
			}
			printf("</td></tr>\n");
			printf("</table>\n");
		}

		printf("</td>\n");

		/* middle column of top row */
		printf("<td align='center' valign='middle' width='33%%'>\n");

		if ((display_type == DISPLAY_HOST_INFO && temp_host != NULL) || (display_type == DISPLAY_SERVICE_INFO && temp_host != NULL && temp_service != NULL) || (display_type == DISPLAY_HOSTGROUP_INFO && temp_hostgroup != NULL) || (display_type == DISPLAY_SERVICEGROUP_INFO && temp_servicegroup != NULL)) {

			if (display_type == DISPLAY_HOST_INFO) {

				printf("<div class='data'>Host</div>\n");
				printf("<div class='dataTitle'>%s</div>\n", temp_host->alias);
				printf("<div class='dataTitle'>(%s)</div><br>\n", (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);

				if (temp_host->parent_hosts != NULL) {
					/* print all parent hosts */
					printf("<div class='data'>Parents:</div>\n");
					for (temp_parenthost = temp_host->parent_hosts; temp_parenthost != NULL; temp_parenthost = temp_parenthost->next)
						printf("<div class='dataTitle'><a href='%s?host=%s'>%s</a></div>\n", STATUS_CGI, url_encode(temp_parenthost->host_name), temp_parenthost->host_name);
				}

				/* Hostgroups */
				printf("<div class='data'>Member of</div><div class='dataTitle'>");

				for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {
					if (is_host_member_of_hostgroup(temp_hostgroup, temp_host) == TRUE) {
						if (found == TRUE)
							printf(", ");

						printf("<a href='%s?hostgroup=%s&amp;style=overview'>%s</a>", STATUS_CGI, url_encode(temp_hostgroup->group_name), html_encode((temp_hostgroup->alias != NULL) ? temp_hostgroup->alias : temp_hostgroup->group_name, TRUE));
						found = TRUE;
					}
				}

				if (found == FALSE)
					printf("No hostgroups");

				printf("</div>\n");

				/* Child Hosts */
				if (extinfo_show_child_hosts == SHOW_CHILD_HOSTS_IMMEDIATE || extinfo_show_child_hosts == SHOW_CHILD_HOSTS_ALL) {
					found = FALSE;

					printf("<div class='data'>Immediate Child Hosts ");
					printf("<img id='expand_image_immediate' src='%s%s' border='0' onClick=\"if (document.getElementById('immediate_child_hosts').style.display == 'none') { document.getElementById('immediate_child_hosts').style.display = ''; document.getElementById('immediate_child_hosts_gap').style.display = 'none'; document.getElementById('expand_image_immediate').src = '%s%s'; } else { document.getElementById('immediate_child_hosts').style.display = 'none'; document.getElementById('immediate_child_hosts_gap').style.display = ''; document.getElementById('expand_image_immediate').src = '%s%s'; }\">", url_images_path, EXPAND_ICON, url_images_path, COLLAPSE_ICON, url_images_path, EXPAND_ICON);
					printf("</div><div class='dataTitle' id='immediate_child_hosts_gap' style='display:;'>&nbsp;</div><div class='dataTitle' id='immediate_child_hosts' style='display:none;'>");

					for (child_host = host_list; child_host != NULL; child_host = child_host->next) {
						if (is_host_immediate_child_of_host(temp_host, child_host) == TRUE) {
							if (found == TRUE)
								printf(", ");

							printf("<a href='%s?type=%d&amp;host=%s'>%s</a>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(child_host->name), html_encode(child_host->name, TRUE));
							found = TRUE;
						}
					}

					if (found == FALSE)
						printf("None");

					printf("</div>\n");

					if (extinfo_show_child_hosts == SHOW_CHILD_HOSTS_ALL) {
						found = FALSE;

						printf("<div class='data'>All Child Hosts ");
						printf("<img id='expand_image_all' src='%s%s' border='0' onClick=\"if (document.getElementById('all_child_hosts').style.display == 'none') { document.getElementById('all_child_hosts').style.display = ''; document.getElementById('all_child_hosts_gap').style.display = 'none'; document.getElementById('expand_image_all').src = '%s%s'; } else { document.getElementById('all_child_hosts').style.display = 'none'; document.getElementById('all_child_hosts_gap').style.display = ''; document.getElementById('expand_image_all').src = '%s%s'; }\">", url_images_path, EXPAND_ICON, url_images_path, COLLAPSE_ICON, url_images_path, EXPAND_ICON);
						printf("</div><div class='dataTitle' id='all_child_hosts_gap' style='display:;'>&nbsp;</div><div class='dataTitle' id='all_child_hosts' style='display:none;'>");

						for (child_host = host_list; child_host != NULL; child_host = child_host->next) {
							if (is_host_child_of_host(temp_host, child_host) == TRUE) {
								if (found == TRUE)
									printf(", ");

								printf("<a href='%s?type=%d&amp;host=%s'>%s</a>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(child_host->name), html_encode(child_host->name, TRUE));

								found = TRUE;
							}
						}

						if (found == FALSE)
							printf("None");

						printf("</div>\n");
					}
				}

				/* Host Dependencies */
				found = FALSE;

				printf("<div class='data'>Host Dependencies ");
				printf("<img id='expand_image_hd' src='%s%s' border='0' onClick=\"if (document.getElementById('host_dependencies').style.display == 'none') { document.getElementById('host_dependencies').style.display = ''; document.getElementById('host_dependencies_gap').style.display = 'none'; document.getElementById('expand_image_hd').src = '%s%s'; } else { document.getElementById('host_dependencies').style.display = 'none'; document.getElementById('host_dependencies_gap').style.display = ''; document.getElementById('expand_image_hd').src = '%s%s'; }\">", url_images_path, EXPAND_ICON, url_images_path, COLLAPSE_ICON, url_images_path, EXPAND_ICON);
				printf("</div><div class='dataTitle' id='host_dependencies_gap' style='display:;'>&nbsp;</div><div class='dataTitle' id='host_dependencies' style='display:none;'>");

				for (temp_hd = hostdependency_list; temp_hd != NULL; temp_hd = temp_hd->next) {

					if (!strcmp(temp_hd->dependent_host_name, temp_host->name)) {
						if (!strcmp(temp_hd->host_name, last_hd_hostname)) {
							if (found == TRUE)
								printf(", ");

							printf("<a href='%s?type=%d&amp;host=%s'>%s</a><br>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_hd->host_name), html_encode(temp_hd->host_name, FALSE));
							found = TRUE;
						}
						last_hd_hostname = temp_hd->host_name;
					}
				}

				if (found == FALSE)
					printf("None");

				printf("</div>\n");

				/* Host address(6) */
				if (!strcmp(temp_host->address6, temp_host->name)) {
					printf("<div class='data'>%s</div>\n", temp_host->address);
				} else {
					printf("<div class='data'>%s, %s</div>\n", temp_host->address, temp_host->address6);
				}
			}
			if (display_type == DISPLAY_SERVICE_INFO) {

				printf("<div class='data'>Service</div><div class='dataTitle'>%s</div><div class='data'>On Host</div>\n", (temp_service->display_name != NULL) ? temp_service->display_name : temp_service->description);
				printf("<div class='dataTitle'>%s</div>\n", temp_host->alias);
				printf("<div class='dataTitle'>(<a href='%s?type=%d&amp;host=%s'>%s</a>)</div><br>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_host->name), (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);

				/* Servicegroups */
				printf("<div class='data'>Member of</div><div class='dataTitle'>");

				for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {
					if (is_service_member_of_servicegroup(temp_servicegroup, temp_service) == TRUE) {
						if (found == TRUE)
							printf(", ");

						printf("<a href='%s?servicegroup=%s&amp;style=overview'>%s</a>", STATUS_CGI, url_encode(temp_servicegroup->group_name), html_encode((temp_servicegroup->alias != NULL) ? temp_servicegroup->alias : temp_servicegroup->group_name, TRUE));
						found = TRUE;
					}
				}

				if (found == FALSE)
					printf("No servicegroups.");

				printf("</div>\n");

				/* Service Dependencies */
				found = FALSE;

				printf("<div class='data'>Service Dependencies ");
				printf("<img id='expand_image_sd' src='%s%s' border='0' onClick=\"if (document.getElementById('service_dependencies').style.display == 'none') { document.getElementById('service_dependencies').style.display = ''; document.getElementById('service_dependencies_gap').style.display = 'none'; document.getElementById('expand_image_sd').src = '%s%s'; } else { document.getElementById('service_dependencies').style.display = 'none'; document.getElementById('service_dependencies_gap').style.display = ''; document.getElementById('expand_image_sd').src = '%s%s'; }\">", url_images_path, EXPAND_ICON, url_images_path, COLLAPSE_ICON, url_images_path, EXPAND_ICON);
				printf("</div><div class='dataTitle' id='service_dependencies_gap' style='display:;'>&nbsp;</div><div class='dataTitle' id='service_dependencies' style='display:none;'>");

				for (temp_sd = servicedependency_list; temp_sd != NULL; temp_sd = temp_sd->next) {

					if (!strcmp(temp_sd->dependent_service_description, temp_service->description) && !strcmp(temp_sd->dependent_host_name, temp_host->name)) {
					        if (!(!strcmp(temp_sd->service_description, last_sd_svc_desc) && !strcmp(temp_sd->host_name, last_sd_hostname))) {
							if (found == TRUE)
								printf(", ");

							printf("<a href='%s?type=%d&amp;host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_sd->host_name));
							printf("&amp;service=%s'>%s on %s</a>\n", url_encode(temp_sd->service_description), html_encode(temp_sd->service_description, FALSE), html_encode(temp_sd->host_name, FALSE));
							found = TRUE;
						}
						last_sd_svc_desc = temp_sd->service_description;
						last_sd_hostname = temp_sd->host_name;
					}
				}

				if (found == FALSE)
					printf("None");

				printf("</div>\n");


				if (!strcmp(temp_host->address6, temp_host->name)) {
					printf("<div class='data'>%s</div>\n", temp_host->address);
				} else {
					printf("<div class='data'>%s, %s</div>\n", temp_host->address, temp_host->address6);
				}
			}
			if (display_type == DISPLAY_HOSTGROUP_INFO) {

				printf("<div class='data'>Hostgroup</div>\n");
				printf("<div class='dataTitle'>%s</div>\n", temp_hostgroup->alias);
				printf("<div class='dataTitle'>(%s)</div>\n", temp_hostgroup->group_name);

				if (temp_hostgroup->notes != NULL) {
					process_macros_r(mac, temp_hostgroup->notes, &processed_string, 0);
					printf("<p>%s</p>", processed_string);
					free(processed_string);
				}
			}
			if (display_type == DISPLAY_SERVICEGROUP_INFO) {

				printf("<div class='data'>Servicegroup</div>\n");
				printf("<div class='dataTitle'>%s</div>\n", temp_servicegroup->alias);
				printf("<div class='dataTitle'>(%s)</div>\n", temp_servicegroup->group_name);

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
					printf("' border='0' alt='%s' title='%s'><br clear='all'>", (temp_service->icon_image_alt == NULL) ? "" : temp_service->icon_image_alt, (temp_service->icon_image_alt == NULL) ? "" : temp_service->icon_image_alt);
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
					printf("' border='0' alt='%s' title='%s'><br clear='all'>", (temp_host->icon_image_alt == NULL) ? "" : temp_host->icon_image_alt, (temp_host->icon_image_alt == NULL) ? "" : temp_host->icon_image_alt);
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
		printf("<td align='right' valign='bottom' width='33%%'>\n");

		if (display_type == DISPLAY_HOST_INFO && temp_host != NULL) {

			printf("<table border='0'>\n");
			if (temp_host->action_url != NULL && strcmp(temp_host->action_url, "")) {
				process_macros_r(mac, temp_host->action_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<tr><td align='right'>\n");
				printf("<a href='");
				printf("%s", processed_string);
				printf("' target='%s'><img src='%s%s%s' border='0' alt='Perform Additional Actions On This Host' title='Perform Additional Actions On This Host'></a>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, MU_iconstr, ACTION_ICON);
				printf("<br clear='all'><font size='-1'><i>Extra Actions</i></font><br clear='all'><br clear='all'>\n");
				printf("</td></tr>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_host->notes_url != NULL && strcmp(temp_host->notes_url, "")) {
				process_macros_r(mac, temp_host->notes_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<tr><td align='right'>\n");
				printf("<a href='");
				printf("%s", processed_string);
				/*print_extra_host_url(temp_host->name,temp_host->notes_url);*/
				printf("' target='%s'><img src='%s%s%s' border='0' alt='View Additional Notes For This Host' title='View Additional Notes For This Host'></a>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, MU_iconstr, NOTES_ICON);
				printf("<br clear='all'><font size='-1'><i>Extra Notes</i></font><br clear='all'><br clear='all'>\n");
				printf("</td></tr>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			printf("</table>\n");
		}

		else if (display_type == DISPLAY_SERVICE_INFO && temp_service != NULL) {

			printf("<table border='0'><tr><td align='right'>\n");

			if (temp_service->action_url != NULL && strcmp(temp_service->action_url, "")) {
				process_macros_r(mac, temp_service->action_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<a href='");
				printf("%s", processed_string);
				printf("' target='%s'><img src='%s%s%s' border='0' alt='Perform Additional Actions On This Service' title='Perform Additional Actions On This Service'></a>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, MU_iconstr, ACTION_ICON);
				printf("<br clear='all'><font size='-1'><i>Extra Actions</i></font><br clear='all'><br clear='all'>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_service->notes_url != NULL && strcmp(temp_service->notes_url, "")) {
				process_macros_r(mac, temp_service->notes_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<a href='");
				printf("%s", processed_string);
				printf("' target='%s'><img src='%s%s%s' border='0' alt='View Additional Notes For This Service' title='View Additional Notes For This Service'></a>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, MU_iconstr, NOTES_ICON);
				printf("<br clear='all'><font size='-1'><i>Extra Notes</i></font><br clear='all'><br clear='all'>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			printf("</td></tr></table>\n");
		}

		if (display_type == DISPLAY_HOSTGROUP_INFO && temp_hostgroup != NULL) {
			printf("<table border='0'>\n");

			if (temp_hostgroup->action_url != NULL && strcmp(temp_hostgroup->action_url, "")) {
				printf("<tr><td align='right'>\n");
				printf("<a href='");
				print_extra_hostgroup_url(temp_hostgroup->group_name, temp_hostgroup->action_url);
				printf("' target='%s'><img src='%s%s' border='0' alt='Perform Additional Actions On This Hostgroup' title='Perform Additional Actions On This Hostgroup'></a>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, ACTION_ICON);
				printf("<br clear='all'><font size='-1'><i>Extra Actions</i></font><br clear='all'><br clear='all'>\n");
				printf("</td></tr>\n");
			}
			if (temp_hostgroup->notes_url != NULL && strcmp(temp_hostgroup->notes_url, "")) {
				printf("<tr><td align='right'>\n");
				printf("<a href='");
				print_extra_hostgroup_url(temp_hostgroup->group_name, temp_hostgroup->notes_url);
				printf("' target='%s'><img src='%s%s' border='0' alt='View Additional Notes For This Hostgroup' title='View Additional Notes For This Hostgroup'></a>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, NOTES_ICON);
				printf("<br clear='all'><font size='-1'><i>Extra Notes</i></font><br clear='all'><br clear='all'>\n");
				printf("</td></tr>\n");
			}
			printf("</table>\n");
		}

		else if (display_type == DISPLAY_SERVICEGROUP_INFO && temp_servicegroup != NULL) {
			printf("<table border='0'>\n");

			if (temp_servicegroup->action_url != NULL && strcmp(temp_servicegroup->action_url, "")) {
				printf("<tr><td align='right'>\n");
				printf("<a href='");
				print_extra_servicegroup_url(temp_servicegroup->group_name, temp_servicegroup->action_url);
				printf("' target='%s'><img src='%s%s' border='0' alt='Perform Additional Actions On This Servicegroup' title='Perform Additional Actions On This Servicegroup'></a>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, ACTION_ICON);
				printf("<br clear='all'><font size='-1'><i>Extra Actions</i></font><br clear='all'><br clear='all'>\n");
				printf("</td></tr>\n");
			}
			if (temp_servicegroup->notes_url != NULL && strcmp(temp_servicegroup->notes_url, "")) {
				printf("<tr><td align='right'>\n");
				printf("<a href='");
				print_extra_servicegroup_url(temp_servicegroup->group_name, temp_servicegroup->notes_url);
				printf("' target='%s'><img src='%s%s' border='0' alt='View Additional Notes For This Servicegroup' title='View Additional Notes For This Servicegroup'></a>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, NOTES_ICON);
				printf("<br clear='all'><font size='-1'><i>Extra Notes</i></font><br clear='all'><br clear='all'>\n");
				printf("</td></tr>\n");
			}
			printf("</table>\n");
		}

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");

	}

	if (content_type == HTML_CONTENT) {
		if (display_type == DISPLAY_HOST_INFO || display_type == DISPLAY_SERVICE_INFO) {
			printf("<div style='padding-right:6px;' class='csv_export_link'>");
			print_export_link(JSON_CONTENT, EXTINFO_CGI, NULL);
			print_export_link(HTML_CONTENT, EXTINFO_CGI, NULL);
			printf("</div>");
		} else
			printf("<br>\n");
	}

	if (display_type == DISPLAY_HOST_INFO) {
		if (content_type == CSV_CONTENT) {
			if (csv_type == CSV_COMMENT)
				show_comments(HOST_COMMENT);
			else if (csv_type == CSV_DOWNTIME)
				show_downtime(HOST_DOWNTIME);
			else
				printf("Please specify the correct csvtype! possible is \"csvtype=comment\" or \"csvtype=downtime\".\n");
		} else {
			/* sort comments and downtime */
			sort_data(sort_type, sort_option);
			show_host_info();
		}
	} else if (display_type == DISPLAY_SERVICE_INFO) {
		if (content_type == CSV_CONTENT) {
			if (csv_type == CSV_COMMENT)
				show_comments(SERVICE_COMMENT);
			else if (csv_type == CSV_DOWNTIME)
				show_downtime(SERVICE_DOWNTIME);
			else
				printf("Please specify the correct csvtype! possible is \"csvtype=comment\" or \"csvtype=downtime\".\n");
		} else {
			/* sort comments and downtime */
			sort_data(sort_type, sort_option);
			show_service_info();
		}
	} else if (display_type == DISPLAY_COMMENTS) {
		if (is_authorized_for_read_only(&current_authdata) == TRUE && is_authorized_for_comments_read_only(&current_authdata) == FALSE)
			printf("<div align='center' class='infoMessage'>Your account does not have permissions to view comments.<br>\n");
		else {
			if (content_type == CSV_CONTENT || content_type == JSON_CONTENT) {
				show_comments(HOST_COMMENT);
				if (content_type == JSON_CONTENT)
					printf(",\n");
				show_comments(SERVICE_COMMENT);
			} else {
				printf("<br>\n");
				printf("<div class='commentNav'>[&nbsp;<a href='#HOSTCOMMENTS' class='commentNav'>Host Comments</a>&nbsp;|&nbsp;<a href='#SERVICECOMMENTS' class='commentNav'>Service Comments</a>&nbsp;]</div>\n");
				printf("<br>\n");

				/* sort comments and downtime */
				sort_data(sort_type, sort_option);

				show_comments(HOST_COMMENT);
				printf("<br>\n");
				show_comments(SERVICE_COMMENT);
			}
		}
	} else if (display_type == DISPLAY_DOWNTIME) {
		if (is_authorized_for_read_only(&current_authdata) == TRUE && is_authorized_for_downtimes_read_only(&current_authdata) == FALSE)
			printf("<div align='center' class='infoMessage'>Your account does not have permissions to view downtimes.<br>\n");
		else {
			if (content_type == CSV_CONTENT || content_type == JSON_CONTENT) {
				show_downtime(HOST_DOWNTIME);
				if (content_type == JSON_CONTENT)
					printf(",\n");
				show_downtime(SERVICE_DOWNTIME);
			} else {
				printf("<br>\n");
				printf("<div class='downtimeNav'>[&nbsp;<a href='#HOSTDOWNTIME' class='downtimeNav'>Host Downtime</a>&nbsp;|&nbsp;<a href='#SERVICEDOWNTIME' class='downtimeNav'>Service Downtime</a>&nbsp;]</div>\n");
				printf("<br>\n");

				/* sort comments and downtime */
				sort_data(sort_type, sort_option);

				show_downtime(HOST_DOWNTIME);
				printf("<br>\n");
				show_downtime(SERVICE_DOWNTIME);
			}
		}
	} else if (display_type == DISPLAY_PERFORMANCE && (content_type == HTML_CONTENT || content_type == JSON_CONTENT))
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
	free_sortdata_list();

	return OK;
}

int process_cgivars(void) {
	char **variables;
	char *key = NULL;
	char *value = NULL;
	int error = FALSE;
	int temp_type;
	int x;

	variables = getcgivars();

	for (x = 0; variables[x] != NULL; x+=2) {
		key = variables[x];
		value = variables[x+1];

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if (strlen(key) >= MAX_INPUT_BUFFER - 1) {
			error = TRUE;
			break;
		}
		/* likewise, check the value if it exists */
		if (value != NULL)
			if (strlen(value) >= MAX_INPUT_BUFFER - 1) {
				error = TRUE;
				break;
		}

		/* we found the display type */
		if (!strcmp(key, "type")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}
			temp_type = atoi(value);
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
		else if (!strcmp(key, "host")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}

			host_name = strdup(value);
			if (host_name == NULL)
				host_name = "";
			strip_html_brackets(host_name);
		}

		/* we found the hostgroup name */
		else if (!strcmp(key, "hostgroup")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}

			hostgroup_name = strdup(value);
			if (hostgroup_name == NULL)
				hostgroup_name = "";
			strip_html_brackets(hostgroup_name);
		}

		/* we found the service name */
		else if (!strcmp(key, "service")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}

			service_desc = strdup(value);
			if (service_desc == NULL)
				service_desc = "";
			strip_html_brackets(service_desc);
		}

		/* we found the servicegroup name */
		else if (!strcmp(key, "servicegroup")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}

			servicegroup_name = strdup(value);
			if (servicegroup_name == NULL)
				servicegroup_name = "";
			strip_html_brackets(servicegroup_name);
		}

		/* we found the sort type argument */
		else if (!strcmp(key, "sorttype")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}

			sort_type = atoi(value);
		}

		/* we found the sort option argument */
		else if (!strcmp(key, "sortoption")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}

			sort_option = atoi(value);
		}

		/* we found the CSV output option */
		else if (!strcmp(key, "csvoutput")) {
			display_header = FALSE;
			content_type = CSV_CONTENT;
		}

		/* we found the JSON output option */
		else if (!strcmp(key, "jsonoutput")) {
			display_header = FALSE;
			content_type = JSON_CONTENT;
		}

		else if (!strcmp(key, "csvtype")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(value, "comment"))
				csv_type = CSV_COMMENT;
			else if (!strcmp(value, "downtime"))
				csv_type = CSV_DOWNTIME;
			else
				csv_type = CSV_DEFAULT;
		}

		/* we found the embed option */
		else if (!strcmp(key, "embedded"))
			embedded = TRUE;

		/* we found the noheader option */
		else if (!strcmp(key, "noheader"))
			display_header = FALSE;

		/* we found the pause option */
		else if (!strcmp(key, "paused"))
			refresh = FALSE;

		/* we found the nodaemoncheck option */
		else if (!strcmp(key, "nodaemoncheck"))
			daemon_check = FALSE;

		/* start num results to skip on displaying statusdata */
		else if (!strcmp(key, "start")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}

			result_start = atoi(value);

			if (result_start < 1)
				result_start = 1;
		}

		/* amount of results to display */
		else if (!strcmp(key, "limit")) {
			if (value == NULL) {
				error = TRUE;
				break;
			}

			get_result_limit = atoi(value);
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
		printf("\"program_version\": \"%s\",\n", status_file_icinga_version);
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
		if (disable_notifications_expire_time == (time_t)0)
			printf("\"disable_notifications_expire_time\": null,\n");
		else
			printf("\"disable_notifications_expire_time\": \"%s\",\n", disable_notif_expire_time);
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
		printf("%sNOTIFICATIONS_DISABLED_EXPIRE_TIME%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
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
		printf("%s%s%s%s", csv_data_enclosure, status_file_icinga_version, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, start_time, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, run_time_string, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (last_command_check == (time_t)0) ? "N/A" : last_external_check_time, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (last_log_rotation == (time_t)0) ? "N/A" : last_log_rotation_time, csv_data_enclosure, csv_delimiter);
		printf("%s%d%s%s", csv_data_enclosure, nagios_pid, csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (enable_notifications == TRUE) ? "YES" : "NO", csv_data_enclosure, csv_delimiter);
		printf("%s%s%s%s", csv_data_enclosure, (disable_notifications_expire_time == (time_t)0) ? "N/A" : disable_notif_expire_time, csv_data_enclosure, csv_delimiter);
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

		printf("<table border='0' cellpadding='20' align='center'>\n");
		printf("<tr><td valign='top'>\n");

		printf("<div class='dataTitle'>Process Information</div>\n");

		printf("<table border='1' cellspacing='0' cellpadding='0' class='data'>\n");
		printf("<tr><td class='stateInfoTable1'>\n");
		printf("<table border='0'>\n");

		/* program version */
		printf("<tr><td class='dataVar'>Program Version:</td><td class='dataVal'>%s</td></tr>\n", status_file_icinga_version);

		/* program start time */
		printf("<tr><td class='dataVar'>Program Start Time:</td><td class='dataVal'>%s</td></tr>\n", start_time);

		/* total running time */
		printf("<tr><td class='dataVar'>Total Running Time:</td><td class='dataVal'>%s</td></tr>\n", run_time_string);

		/* last external check */
		printf("<tr><td class='dataVar'>Last External Command Check:</td><td class='dataVal'>%s</td></tr>\n", (last_command_check == (time_t)0) ? "N/A" : last_external_check_time);

		/* last log file rotation */
		printf("<tr><td class='dataVar'>Last Log File Rotation:</td><td class='dataVal'>%s</td></tr>\n", (last_log_rotation == (time_t)0) ? "N/A" : last_log_rotation_time);

		/* PID */
		printf("<tr><td class='dataVar'>Icinga PID</td><td class='dataVal'>%d</td></tr>\n", nagios_pid);

		/* notifications enabled */
		printf("<tr><td class='dataVar'>Notifications Enabled?</td><td class='dataVal'><div class='notifications%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (enable_notifications == TRUE) ? "ENABLED" : "DISABLED", (enable_notifications == TRUE) ? "YES" : "NO");
		if (enable_notifications == FALSE && disable_notifications_expire_time != 0)
			printf("<tr><td class='dataVar'>Notifications Disabled Expire Time:</td><td class='dataVal'>%s</td></tr>\n", disable_notif_expire_time);
		else
			printf("<tr><td class='dataVar'>Notifications Disabled Expire Time:</td><td class='dataVal'><div class='notificationsUNKNOWN'>&nbsp;&nbsp;NOT SET&nbsp;&nbsp;</div></td></tr>\n");


		/* service check execution enabled */
		printf("<tr><td class='dataVar'>Service Checks Being Executed?</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (execute_service_checks == TRUE) ? "ENABLED" : "DISABLED", (execute_service_checks == TRUE) ? "YES" : "NO");

		/* passive service check acceptance */
		printf("<tr><td class='dataVar'>Passive Service Checks Being Accepted?</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (accept_passive_service_checks == TRUE) ? "ENABLED" : "DISABLED", (accept_passive_service_checks == TRUE) ? "YES" : "NO");

		/* host check execution enabled */
		printf("<tr><td class='dataVar'>Host Checks Being Executed?</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (execute_host_checks == TRUE) ? "ENABLED" : "DISABLED", (execute_host_checks == TRUE) ? "YES" : "NO");

		/* passive host check acceptance */
		printf("<tr><td class='dataVar'>Passive Host Checks Being Accepted?</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (accept_passive_host_checks == TRUE) ? "ENABLED" : "DISABLED", (accept_passive_host_checks == TRUE) ? "YES" : "NO");

		/* event handlers enabled */
		printf("<tr><td class='dataVar'>Event Handlers Enabled?</td><td class='dataVal'>%s</td></tr>\n", (enable_event_handlers == TRUE) ? "Yes" : "No");

		/* obsessing over services */
		printf("<tr><td class='dataVar'>Obsessing Over Services?</td><td class='dataVal'>%s</td></tr>\n", (obsess_over_services == TRUE) ? "Yes" : "No");

		/* obsessing over hosts */
		printf("<tr><td class='dataVar'>Obsessing Over Hosts?</td><td class='dataVal'>%s</td></tr>\n", (obsess_over_hosts == TRUE) ? "Yes" : "No");

		/* flap detection enabled */
		printf("<tr><td class='dataVar'>Flap Detection Enabled?</td><td class='dataVal'>%s</td></tr>\n", (enable_flap_detection == TRUE) ? "Yes" : "No");

#ifdef PREDICT_FAILURES
		/* failure prediction enabled */
		printf("<tr><td class='dataVar'>Failure Prediction Enabled?</td><td class='dataVal'>%s</td></tr>\n", (enable_failure_prediction == TRUE) ? "Yes" : "No");
#endif

		/* process performance data */
		printf("<tr><td class='dataVar'>Performance Data Being Processed?</td><td class='dataVal'>%s</td></tr>\n", (process_performance_data == TRUE) ? "Yes" : "No");

		/* Notifications disabled will expire? */
		if(enable_notifications == TRUE && disable_notifications_expire_time > 0)
			printf("<tr><td class='dataVar'>Notifications?</td><td class='dataVal'>%s</td></tr>\n", (process_performance_data == TRUE) ? "Yes" : "No");


#ifdef USE_OLDCRUD
		/* daemon mode */
		printf("<tr><td class='dataVar'>Running As A Daemon?</td><td class='dataVal'>%s</td></tr>\n", (daemon_mode == TRUE) ? "Yes" : "No");
#endif

		printf("</table>\n");
		printf("</td></tr>\n");
		printf("</table>\n");


		printf("</td><td valign='top'>\n");

		printf("<div class='commandTitle'>Process Commands</div>\n");

		printf("<table border='1' cellpadding='0' cellspacing='0' class='command'>\n");
		printf("<tr><td>\n");

		printf("<table border='0' cellpadding='0' cellspacing='0' class='command'>\n");

#ifndef DUMMY_INSTALL
		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Shutdown the Icinga Process' title='Shutdown the Icinga Process'></td><td class='command'><a href='%s?cmd_typ=%d'>Shutdown the Icinga process</a></td></tr>\n", url_images_path, STOP_ICON, CMD_CGI, CMD_SHUTDOWN_PROCESS);
		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Restart the Icinga Process' title='Restart the Icinga Process'></td><td class='command'><a href='%s?cmd_typ=%d'>Restart the Icinga process</a></td></tr>\n", url_images_path, RESTART_ICON, CMD_CGI, CMD_RESTART_PROCESS);
#endif

		if (enable_notifications == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Notifications' title='Disable Notifications'></td><td class='command'><a href='%s?cmd_typ=%d'>Disable notifications</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_NOTIFICATIONS);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Notifications' title='Enable Notifications'></td><td class='command'><a href='%s?cmd_typ=%d'>Enable notifications</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_NOTIFICATIONS);

		if (execute_service_checks == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Executing Service Checks' title='Stop Executing Service Checks'></td><td class='command'><a href='%s?cmd_typ=%d'>Stop executing service checks</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_EXECUTING_SVC_CHECKS);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Executing Service Checks' title='Start Executing Service Checks'></td><td class='command'><a href='%s?cmd_typ=%d'>Start executing service checks</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_EXECUTING_SVC_CHECKS);

		if (accept_passive_service_checks == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Accepting Passive Service Checks' title='Stop Accepting Passive Service Checks'></td><td class='command'><a href='%s?cmd_typ=%d'>Stop accepting passive service checks</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Accepting Passive Service Checks' title='Start Accepting Passive Service Checks'></td><td class='command'><a href='%s?cmd_typ=%d'>Start accepting passive service checks</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS);

		if (execute_host_checks == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Executing Host Checks' title='Stop Executing Host Checks'></td><td class='command'><a href='%s?cmd_typ=%d'>Stop executing host checks</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_EXECUTING_HOST_CHECKS);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Executing Host Checks' title='Start Executing Host Checks'></td><td class='command'><a href='%s?cmd_typ=%d'>Start executing host checks</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_EXECUTING_HOST_CHECKS);

		if (accept_passive_host_checks == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Accepting Passive Host Checks' title='Stop Accepting Passive Host Checks'></td><td class='command'><a href='%s?cmd_typ=%d'>Stop accepting passive host checks</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Accepting Passive Host Checks' title='Start Accepting Passive Host Checks'></td><td class='command'><a href='%s?cmd_typ=%d'>Start accepting passive host checks</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS);

		if (enable_event_handlers == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Event Handlers' title='Disable Event Handlers'></td><td class='command'><a href='%s?cmd_typ=%d'>Disable event handlers</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_EVENT_HANDLERS);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Event Handlers' title='Enable Event Handlers'></td><td class='command'><a href='%s?cmd_typ=%d'>Enable event handlers</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_EVENT_HANDLERS);

		if (obsess_over_services == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Obsessing Over Services' title='Stop Obsessing Over Services'></td><td class='command'><a href='%s?cmd_typ=%d'>Stop obsessing over services</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_OBSESSING_OVER_SVC_CHECKS);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Obsessing Over Services' title='Start Obsessing Over Services'></td><td class='command'><a href='%s?cmd_typ=%d'>Start obsessing over services</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_OBSESSING_OVER_SVC_CHECKS);

		if (obsess_over_hosts == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Obsessing Over Hosts' title='Stop Obsessing Over Hosts'></td><td class='command'><a href='%s?cmd_typ=%d'>Stop obsessing over hosts</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_OBSESSING_OVER_HOST_CHECKS);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Obsessing Over Hosts' title='Start Obsessing Over Hosts'></td><td class='command'><a href='%s?cmd_typ=%d'>Start obsessing over hosts</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_OBSESSING_OVER_HOST_CHECKS);

		if (enable_flap_detection == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Flap Detection' title='Disable Flap Detection'></td><td class='command'><a href='%s?cmd_typ=%d'>Disable flap detection</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_FLAP_DETECTION);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Flap Detection' title='Enable Flap Detection'></td><td class='command'><a href='%s?cmd_typ=%d'>Enable flap detection</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_FLAP_DETECTION);

#ifdef PREDICT_FAILURES
		if (enable_failure_prediction == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Failure Prediction' title='Disable Failure Prediction'></td><td class='command'><a href='%s?cmd_typ=%d'>Disable failure prediction</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_FAILURE_PREDICTION);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Failure Prediction' title='Enable Failure Prediction'></td><td class='command'><a href='%s?cmd_typ=%d'>Enable failure prediction</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_FAILURE_PREDICTION);
#endif
		if (process_performance_data == TRUE)
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Performance Data' title='Disable Performance Data'></td><td class='command'><a href='%s?cmd_typ=%d'>Disable performance data</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_PERFORMANCE_DATA);
		else
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Performance Data' title='Enable Performance Data'></td><td class='command'><a href='%s?cmd_typ=%d'>Enable performance data</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_PERFORMANCE_DATA);

		printf("</table>\n");

		printf("</td></tr>\n");
		printf("</table>\n");

		printf("</td></tr></table>\n");
	}
}

void show_host_info(void) {
	hoststatus *temp_hoststatus;
	host *temp_host;
	customvariablesmember *temp_customvar;
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
	int json_start = TRUE;


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

			/* Custom Variables */
			if (temp_host->custom_variables) {
				printf("\"custom_variables\": [\n");
				for (temp_customvar = temp_host->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {
					if (check_exclude_customvar(temp_customvar) == FALSE) {
						if (json_start == FALSE) printf(",");
						printf(" { \"%s\": \"%s\" }\n", json_encode(temp_customvar->variable_name), json_encode(temp_customvar->variable_value));
						json_start = FALSE;
					}
				}
				printf("],\n");
			}

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
		printf("<table border='0' cellpadding='0' width='100%%' align='center'>\n");
		printf("<tr>\n");

		printf("<td align='center' valign='top' class='stateInfoPanel'>\n");

		printf("<div class='dataTitle'>Host State Information</div>\n");

		if (temp_hoststatus->has_been_checked == FALSE)
			printf("<div align='center'>This host has not yet been checked, so status information is not available.</div>\n");

		else {

			printf("<table border='0'>\n");
			printf("<tr><td>\n");

			printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
			printf("<tr><td class='stateInfoTable1'>\n");
			printf("<table border='0'>\n");

			printf("<tr><td class='dataVar'>Host Status:</td><td class='dataVal'><div class='%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div>&nbsp;(for %s)%s</td></tr>\n", bg_class, state_string, state_duration, (temp_hoststatus->problem_has_been_acknowledged == TRUE) ? "&nbsp;&nbsp;(Has been acknowledged)" : "");

			printf("<tr><td class='dataVar' valign='top'>Status Information:</td><td class='dataVal'>%s", (temp_hoststatus->plugin_output == NULL) ? "" : html_encode(temp_hoststatus->plugin_output, TRUE));
			if (enable_splunk_integration == TRUE) {
				printf("&nbsp;&nbsp;");
				asprintf(&buf, "%s %s", temp_host->name, temp_hoststatus->plugin_output);
				buf[sizeof(buf) - 1] = '\x0';
				display_splunk_generic_url(buf, 1);
				free(buf);
			}
			if (temp_hoststatus->long_plugin_output != NULL)
				printf("<br>%s", html_encode(temp_hoststatus->long_plugin_output, TRUE));
			printf("</td></tr>\n");

			printf("<tr><td class='dataVar' valign='top'>Performance Data:</td><td class='dataVal'>%s</td></tr>\n", (temp_hoststatus->perf_data == NULL) ? "" : html_encode(temp_hoststatus->perf_data, TRUE));

			printf("<tr><td class='dataVar'>Current Attempt:</td><td class='dataVal'>%d/%d", temp_hoststatus->current_attempt, temp_hoststatus->max_attempts);
			printf("&nbsp;&nbsp;(%s state)</td></tr>\n", (temp_hoststatus->state_type == HARD_STATE) ? "HARD" : "SOFT");

			get_time_string(&temp_hoststatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Last Check Time:</td><td class='dataVal'>%s</td></tr>\n", date_time);

			if (temp_hoststatus->checks_enabled == TRUE)
				printf("<tr><td class='dataVar'>Check Type:</td><td class='dataVal'><a href='%s?type=command&amp;host=%s&expand=%s'>ACTIVE</a></td></tr>\n", CONFIG_CGI, url_encode(host_name), url_encode(temp_host->host_check_command));
			else if (temp_hoststatus->accept_passive_host_checks == TRUE)
				printf("<tr><td class='dataVar'>Check Type:</td><td class='dataVal'>PASSIVE</td></tr>\n");
			else
				printf("<tr><td class='dataVar'>Check Type:</td><td class='dataVal'>DISABLED</td></tr>\n");

			/* Icinga 2 */
			printf("<tr><td class='dataVar' nowrap>Check Source / Reachability:</td><td class='dataVal'>");
			if (temp_hoststatus->check_source != NULL)
				printf("%s / %s", temp_hoststatus->check_source, (temp_hoststatus->is_reachable == TRUE) ? "true" : "false");
			else
				printf("N/A");

			printf("</td></tr>\n");

			printf("<tr><td class='dataVar' nowrap>Check Latency / Duration:</td><td class='dataVal'>");
			if (temp_hoststatus->checks_enabled == TRUE)
				printf("%.3f", temp_hoststatus->latency);
			else
				printf("N/A");
			printf("&nbsp;/&nbsp;%.3f seconds", temp_hoststatus->execution_time);
			printf("</td></tr>\n");

			get_time_string(&temp_hoststatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Next Scheduled Active Check:&nbsp;&nbsp;</td><td class='dataVal'>%s</td></tr>\n", (temp_hoststatus->checks_enabled && temp_hoststatus->next_check != (time_t)0 && temp_hoststatus->should_be_scheduled == TRUE) ? date_time : "N/A");

			get_time_string(&temp_hoststatus->last_state_change, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Last State Change:</td><td class='dataVal'>%s</td></tr>\n", (temp_hoststatus->last_state_change == (time_t)0) ? "N/A" : date_time);

			get_time_string(&temp_hoststatus->last_notification, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Last Notification:</td><td class='dataVal'>%s&nbsp;(notification %d)</td></tr>\n", (temp_hoststatus->last_notification == (time_t)0) ? "N/A" : date_time, temp_hoststatus->current_notification_number);

			printf("<tr><td class='dataVar'>Is This Host Flapping?</td><td class='dataVal'>");
			if (temp_hoststatus->flap_detection_enabled == FALSE || enable_flap_detection == FALSE)
				printf("N/A");
			else
				printf("<div class='%sflapping'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div>&nbsp;(%3.2f%% state change)", (temp_hoststatus->is_flapping == TRUE) ? "" : "not", (temp_hoststatus->is_flapping == TRUE) ? "YES" : "NO", temp_hoststatus->percent_state_change);
			printf("</td></tr>\n");

			printf("<tr><td class='dataVar'>In Scheduled Downtime?</td><td class='dataVal'><div class='downtime%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_hoststatus->scheduled_downtime_depth > 0) ? "ACTIVE" : "INACTIVE", (temp_hoststatus->scheduled_downtime_depth > 0) ? "YES" : "NO");


			get_time_string(&temp_hoststatus->last_update, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Last Update:</td><td class='dataVal'>%s&nbsp;&nbsp;(%s ago)</td></tr>\n", (temp_hoststatus->last_update == (time_t)0) ? "N/A" : date_time, status_age);

			printf("<tr><td class='dataVar'>Modified Attributes:</td><td class='dataVal'>");
			print_modified_attributes(HTML_CONTENT, EXTINFO_CGI, temp_hoststatus->modified_attributes);
			printf("</td></tr>\n");

			printf("<tr><td class='dataVar'>Executed Command:</td><td class='dataVal'><a href='%s?type=command&amp;host=%s&amp;expand=%s'>Command Expander</a></td></tr>\n", CONFIG_CGI, url_encode(host_name), url_encode(temp_host->host_check_command));

			printf("</table>\n");
			printf("</td></tr>\n");
			printf("</table>\n");

			printf("</td></tr>\n");
			printf("<tr><td>\n");

			printf("<table border='1' cellspacing='0' cellpadding='0' align='left' width='100%%'>\n");
			printf("<tr><td style='vertical-align: top; padding-right:0.5em;'>\n");
			printf("<table border='0' class='stateInfoTable2'>\n");

			if ((temp_host->host_check_command) && (*temp_host->host_check_command != '\0'))
				printf("<tr><td class='dataVar' nowrap><a href='%s?type=command&amp;expand=%s'>Active Checks:</a></td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", CONFIG_CGI, url_encode(temp_host->host_check_command), (temp_hoststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED");
			else printf("<tr><td class='dataVar' nowrap>Active Checks:</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_hoststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED");

			printf("<tr><td class='dataVar' nowrap>Passive Checks:</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_hoststatus->accept_passive_host_checks == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->accept_passive_host_checks) ? "ENABLED" : "DISABLED");

			printf("<tr><td class='dataVar' nowrap>Obsessing:</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_hoststatus->obsess_over_host == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->obsess_over_host) ? "ENABLED" : "DISABLED");

			printf("<tr><td class='dataVar' nowrap>Notifications:</td><td class='dataVal'><div class='notifications%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_hoststatus->notifications_enabled) ? "ENABLED" : "DISABLED", (temp_hoststatus->notifications_enabled) ? "ENABLED" : "DISABLED");

			if ((temp_host->event_handler) && (*temp_host->event_handler != '\0'))
				printf("<tr><td class='dataVar' nowrap><a href='%s?type=command&amp;expand=%s'>Event Handler:</a></td><td class='dataVal'><div class='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", CONFIG_CGI, url_encode(temp_host->event_handler), (temp_hoststatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_hoststatus->event_handler_enabled) ? "ENABLED" : "DISABLED");
			else printf("<tr><td class='dataVar' nowrap>Event Handler:</td><td class='dataVal'><div class='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_hoststatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_hoststatus->event_handler_enabled) ? "ENABLED" : "DISABLED");

			printf("<tr><td class='dataVar' nowrap>Flap Detection:</td><td class='dataVal'><div class='flapdetection%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_hoststatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED");

			printf("</table></td><td width='100%%' style='vertical-align: top;'>\n");

			/* Custom Variables */
			if (temp_host->custom_variables) {
				printf("<table border='0' cellspacing='3' class='CustomVarTable' width='100%%'>\n");
				printf("<tr><td colspan='2' class='CustomVarHead' align='center' style='font-weight: bold; padding:0em'>Custom Variables</td></tr>\n");
				printf("<tr><td class='CustomVarHead' width='50%%'>Name</td><td class='CustomVarHead'>Value</td></tr>\n");
				for (temp_customvar = temp_host->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {
					if (check_exclude_customvar(temp_customvar) == FALSE)
						printf("<tr><td class='CustomVarLine'>%s</td><td class='CustomVarLine'>%s</td></tr>\n",temp_customvar->variable_name, temp_customvar->variable_value);
				}
				printf("</table>\n");
			}
			printf("</td></tr>\n");
			printf("</table>\n");

			printf("</td></tr>\n");
			printf("</table>\n");
		}

		printf("</td>\n");

		printf("<td align='center' valign='top'>\n");
		printf("<table border='0' cellpadding='0' cellspacing='0'><tr>\n");

		printf("<td align='center' valign='top' class='commandPanel'>\n");

		printf("<div class='commandTitle'>Host Commands</div>\n");

		printf("<table border='1' cellpadding='0' cellspacing='0'><tr><td>\n");

		if (is_authorized_for_read_only(&current_authdata) == FALSE) {

			printf("<table border='0' cellspacing='0' cellpadding='0' class='command'>\n");
#ifdef USE_STATUSMAP
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Locate Host On Map' title='Locate Host On Map'></td><td class='command'><a href='%s?host=%s'>Locate host on map</a></td></tr>\n", url_images_path, STATUSMAP_ICON, STATUSMAP_CGI, url_encode(host_name));
#endif
			if (temp_hoststatus->checks_enabled == TRUE) {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Active Checks Of This Host' title='Disable Active Checks Of This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Disable active checks of this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_CHECK, url_encode(host_name));
			} else
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Active Checks Of This Host' title='Enable Active Checks Of This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Enable active checks of this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_CHECK, url_encode(host_name));
			printf("<tr class='data'><td><img src='%s%s' border='0' alt='Re-schedule Next Host Check' title='Re-schedule Next Host Check'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s%s'>Re-schedule the next check of this host</a></td></tr>\n", url_images_path, DELAY_ICON, CMD_CGI, CMD_SCHEDULE_HOST_CHECK, url_encode(host_name), (temp_hoststatus->checks_enabled == TRUE) ? "&amp;force_check" : "");

			if (temp_hoststatus->accept_passive_host_checks == TRUE) {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Submit Passive Check Result For This Host' title='Submit Passive Check Result For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Submit passive check result for this host</a></td></tr>\n", url_images_path, PASSIVE_ICON, CMD_CGI, CMD_PROCESS_HOST_CHECK_RESULT, url_encode(host_name));
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Accepting Passive Checks For This Host' title='Stop Accepting Passive Checks For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Stop accepting passive checks for this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_PASSIVE_HOST_CHECKS, url_encode(host_name));
			} else
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Accepting Passive Checks For This Host' title='Start Accepting Passive Checks For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Start accepting passive checks for this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_PASSIVE_HOST_CHECKS, url_encode(host_name));

			if (temp_hoststatus->obsess_over_host == TRUE)
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Obsessing Over This Host' title='Stop Obsessing Over This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Stop obsessing over this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_OBSESSING_OVER_HOST, url_encode(host_name));
			else
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Obsessing Over This Host' title='Start Obsessing Over This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Start obsessing over this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_OBSESSING_OVER_HOST, url_encode(host_name));

			if (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE) {
				if (temp_hoststatus->problem_has_been_acknowledged == FALSE)
					printf("<tr class='command'><td><img src='%s%s' border='0' alt='Acknowledge This Host Problem' title='Acknowledge This Host Problem'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Acknowledge this host problem</a></td></tr>\n", url_images_path, ACKNOWLEDGEMENT_ICON, CMD_CGI, CMD_ACKNOWLEDGE_HOST_PROBLEM, url_encode(host_name));
				else
					printf("<tr class='command'><td><img src='%s%s' border='0' alt='Remove Problem Acknowledgement' title='Remove Problem Acknowledgement'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Remove problem acknowledgement</a></td></tr>\n", url_images_path, REMOVE_ACKNOWLEDGEMENT_ICON, CMD_CGI, CMD_REMOVE_HOST_ACKNOWLEDGEMENT, url_encode(host_name));
			}

			if (temp_hoststatus->notifications_enabled == TRUE)
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Notifications For This Host' title='Disable Notifications For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Disable notifications for this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_NOTIFICATIONS, url_encode(host_name));
			else
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Notifications For This Host' title='Enable Notifications For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Enable notifications for this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_NOTIFICATIONS, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Send Custom Notification' title='Send Custom Notification'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Send custom host notification</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_SEND_CUSTOM_HOST_NOTIFICATION, url_encode(host_name));

			if (temp_hoststatus->status != HOST_UP)
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Delay Next Host Notification' title='Delay Next Host Notification'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Delay next host notification</a></td></tr>\n", url_images_path, DELAY_ICON, CMD_CGI, CMD_DELAY_HOST_NOTIFICATION, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Schedule Downtime For This Host' title='Schedule Downtime For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Schedule downtime for this host</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_HOST_DOWNTIME, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Schedule Downtime For This Host and All Services' title='Schedule Downtime For This Host and All Services'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Schedule downtime for this host and all services</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_HOST_SVC_DOWNTIME, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Remove Downtime(s) for this host and all services' title='Remove Downtime(s) for this host and all services'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Remove Downtime(s) for this host and all services</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DEL_DOWNTIME_BY_HOST_NAME, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Notifications For All Services On This Host' title='Disable Notifications For All Services On This Host'></td><td class='command' nowrap><a href='%s?cmd_typ=%d&amp;host=%s'>Disable notifications for all services on this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_SVC_NOTIFICATIONS, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Notifications For All Services On This Host' title='Enable Notifications For All Services On This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Enable notifications for all services on this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_SVC_NOTIFICATIONS, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Schedule A Check Of All Services On This Host' title='Schedule A Check Of All Services On This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Schedule a check of all services on this host</a></td></tr>\n", url_images_path, DELAY_ICON, CMD_CGI, CMD_SCHEDULE_HOST_SVC_CHECKS, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Checks Of All Services On This Host' title='Disable Checks Of All Services On This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Disable checks of all services on this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_SVC_CHECKS, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Checks Of All Services On This Host' title='Enable Checks Of All Services On This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Enable checks of all services on this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_SVC_CHECKS, url_encode(host_name));

			if (temp_hoststatus->event_handler_enabled == TRUE)
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Event Handler For This Host' title='Disable Event Handler For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Disable event handler for this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_EVENT_HANDLER, url_encode(host_name));
			else
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Event Handler For This Host' title='Enable Event Handler For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Enable event handler for this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_EVENT_HANDLER, url_encode(host_name));
			if (temp_hoststatus->flap_detection_enabled == TRUE)
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Flap Detection For This Host' title='Disable Flap Detection For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Disable flap detection for this host</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOST_FLAP_DETECTION, url_encode(host_name));
			else
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Flap Detection For This Host' title='Enable Flap Detection For This Host'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>Enable flap detection for this host</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOST_FLAP_DETECTION, url_encode(host_name));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Add a new Host comment' title='Add a new Host comment'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s'>", url_images_path, COMMENT_ICON, CMD_CGI, CMD_ADD_HOST_COMMENT, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name));
			printf("Add a new Host comment</a></td>");

			if (status_file_icinga_version != NULL && status_file_icinga_version[0] != '1') {
				/* Modify Check/Retry Interval */
				printf("<tr class='command'><td><a href='%s?cmd_typ=%d&amp;interval=0.015&amp;host=%s&amp;cmd_mod=2'><img src='%s%s' border='0' alt='Modify Check/Retry Interval' title='Modify Check/Retry Interval'></a></td><td class='command'><a href='%s?cmd_typ=%d&amp;interval=0.015&amp;host=%s'>",
				       CMD_CGI, CMD_INTERNAL_CHANGE_HOST_CHECK_RETRY_INTERVAL, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name),
				       url_images_path, RELOAD_ICON,
				       CMD_CGI, CMD_INTERNAL_CHANGE_HOST_CHECK_RETRY_INTERVAL, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name));
				printf("Modify Check/Retry Interval</a></td>");
			}

			/* allow modified attributes to be reset */
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Reset Modified Attributes' title='Reset Modified Attributes'></td><td class='command'><a href='%s?cmd_typ=%d&amp;attr=0.015&amp;attr=%d&amp;host=%s'>",
			       url_images_path, DISABLED_ICON,
			       CMD_CGI, CMD_CHANGE_HOST_MODATTR, MODATTR_NONE, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name));
			printf("Reset Modified Attributes</a></td>");

			printf("</table>\n");
		} else {
			print_generic_error_message("Your account does not have permissions to execute commands.", NULL, 0);
		}
		printf("</td></tr></table>\n");

		printf("</td>\n");

		printf("</tr>\n");
		printf("</table></tr>\n");

		printf("<tr>\n");

		printf("<td colspan='2' valign='top' class='commentPanel'>\n");

		if (is_authorized_for_read_only(&current_authdata) == FALSE || is_authorized_for_comments_read_only(&current_authdata) == TRUE) {
			/* display comments */
			show_comments(HOST_COMMENT);
			printf("<br>");
			/* display downtimes */
			show_downtime(HOST_DOWNTIME);
		}

		printf("</td>\n");

		printf("</tr>\n");
		printf("</table>\n");
	}

	return;
}

void show_service_info(void) {
	service *temp_service;
	host *temp_host;
	customvariablesmember *temp_customvar;
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
	int json_start = TRUE;
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

			/* Custom Variables */
			if (temp_service->custom_variables) {
				printf("\"custom_variables\": [\n");
				for (temp_customvar = temp_service->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {
					if (check_exclude_customvar(temp_customvar) == FALSE) {
						if (json_start == FALSE) printf(",");
						printf(" { \"%s\": \"%s\" }\n", json_encode(temp_customvar->variable_name), json_encode(temp_customvar->variable_value));
						json_start = FALSE;
					}
				}
				printf("],\n");
			}

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
		printf("<table border='0' cellpadding='0' cellspacing='0' width=100%%>\n");
		printf("<tr>\n");

		printf("<td align='center' valign='top' class='stateInfoPanel'>\n");

		printf("<div class='dataTitle'>Service State Information</div>\n");

		if (temp_svcstatus->has_been_checked == FALSE)
			printf("<p><div align='center'>This service has not yet been checked, so status information is not available.</div></p>\n");

		else {

			printf("<table border='0'>\n");

			printf("<tr><td>\n");

			printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
			printf("<tr><td class='stateInfoTable1'>\n");
			printf("<table border='0'>\n");

			printf("<tr><td class='dataVar'>Current Status:</td><td class='dataVal'><div class='%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div>&nbsp;(for %s)%s</td></tr>\n", bg_class, state_string, state_duration, (temp_svcstatus->problem_has_been_acknowledged == TRUE) ? "&nbsp;&nbsp;(Has been acknowledged)" : "");

			printf("<tr><td class='dataVar' valign='top'>Status Information:</td><td class='dataVal'>%s", (temp_svcstatus->plugin_output == NULL) ? "" : html_encode(temp_svcstatus->plugin_output, TRUE));
			if (enable_splunk_integration == TRUE) {
				printf("&nbsp;&nbsp;");
				asprintf(&buf, "%s %s %s", temp_service->host_name, temp_service->description, temp_svcstatus->plugin_output);
				buf[sizeof(buf) - 1] = '\x0';
				display_splunk_generic_url(buf, 1);
				free(buf);
			}
			if (temp_svcstatus->long_plugin_output != NULL)
				printf("<br>%s", html_encode(temp_svcstatus->long_plugin_output, TRUE));
			printf("</td></tr>\n");

			printf("<tr><td class='dataVar' valign='top'>Performance Data:</td><td class='dataVal'>%s</td></tr>\n", (temp_svcstatus->perf_data == NULL) ? "" : html_encode(temp_svcstatus->perf_data, TRUE));

			printf("<tr><td class='dataVar'>Current Attempt:</td><td class='dataVal'>%d/%d", temp_svcstatus->current_attempt, temp_svcstatus->max_attempts);
			printf("&nbsp;&nbsp;(%s state)</td></tr>\n", (temp_svcstatus->state_type == HARD_STATE) ? "HARD" : "SOFT");

			get_time_string(&temp_svcstatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Last Check Time:</td><td class='dataVal'>%s</td></tr>\n", date_time);

			if (temp_svcstatus->checks_enabled == TRUE)
				printf("<tr><td class='dataVar'>Check Type:</td><td class='dataVal'><a href='%s?type=command&amp;host=%s&amp;service=%s&amp;expand=%s'>ACTIVE</a></td></tr>\n",
				       CONFIG_CGI, url_encode(host_name), url_encode(service_desc), url_encode(temp_service->service_check_command));
			else if (temp_svcstatus->accept_passive_service_checks == TRUE)
				printf("<tr><td class='dataVar'>Check Type:</td><td class='dataVal'>PASSIVE</td></tr>\n");
			else
				printf("<tr><td class='dataVar'>Check Type:</td><td class='dataVal'>DISABLED</td></tr>\n");

			/* Icinga 2 */
			printf("<tr><td class='dataVar' nowrap>Check Source / Reachability:</td><td class='dataVal'>");
			if (temp_svcstatus->check_source != NULL)
				printf("%s / %s", temp_svcstatus->check_source, (temp_svcstatus->is_reachable == TRUE) ? "true" : "false");
			else
				printf("N/A");

			printf("</td></tr>\n");

			printf("<tr><td class='dataVar' nowrap>Check Latency / Duration:</td><td class='dataVal'>");
			if (temp_svcstatus->checks_enabled == TRUE)
				printf("%.3f", temp_svcstatus->latency);
			else
				printf("N/A");
			printf("&nbsp;/&nbsp;%.3f seconds", temp_svcstatus->execution_time);
			printf("</td></tr>\n");

			get_time_string(&temp_svcstatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Next Scheduled Active Check:&nbsp;&nbsp;</td><td class='dataVal'>%s</td></tr>\n", (temp_svcstatus->checks_enabled && temp_svcstatus->next_check != (time_t)0 && temp_svcstatus->should_be_scheduled == TRUE) ? date_time : "N/A");

			get_time_string(&temp_svcstatus->last_state_change, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Last State Change:</td><td class='dataVal'>%s</td></tr>\n", (temp_svcstatus->last_state_change == (time_t)0) ? "N/A" : date_time);

			get_time_string(&temp_svcstatus->last_notification, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Last Notification:</td><td class='dataVal'>%s&nbsp;(notification %d)</td></tr>\n", (temp_svcstatus->last_notification == (time_t)0) ? "N/A" : date_time, temp_svcstatus->current_notification_number);

			printf("<tr><td class='dataVar'>Is This Service Flapping?</td><td class='dataVal'>");
			if (temp_svcstatus->flap_detection_enabled == FALSE || enable_flap_detection == FALSE)
				printf("N/A");
			else
				printf("<div class='%sflapping'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div>&nbsp;(%3.2f%% state change)", (temp_svcstatus->is_flapping == TRUE) ? "" : "not", (temp_svcstatus->is_flapping == TRUE) ? "YES" : "NO", temp_svcstatus->percent_state_change);
			printf("</td></tr>\n");

			printf("<tr><td class='dataVar'>In Scheduled Downtime?</td><td class='dataVal'><div class='downtime%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_svcstatus->scheduled_downtime_depth > 0) ? "ACTIVE" : "INACTIVE", (temp_svcstatus->scheduled_downtime_depth > 0) ? "YES" : "NO");


			get_time_string(&temp_svcstatus->last_update, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr><td class='dataVar'>Last Update:</td><td class='dataVal'>%s&nbsp;&nbsp;(%s ago)</td></tr>\n", (temp_svcstatus->last_update == (time_t)0) ? "N/A" : date_time, status_age);

			printf("<tr><td class='dataVar'>Modified Attributes:</td><td class='dataVal'>");
			print_modified_attributes(HTML_CONTENT, EXTINFO_CGI, temp_svcstatus->modified_attributes);
			printf("</td></tr>\n");

			printf("<tr><td class='dataVar'>Executed Command:</td><td class='dataVal'><a href='%s?type=command&amp;host=%s&amp;service=%s&amp;expand=%s'>Command Expander</a></td></tr>\n", CONFIG_CGI, url_encode(host_name), url_encode(service_desc), url_encode(temp_service->service_check_command));

			printf("</table>\n");
			printf("</td></tr>\n");
			printf("</table>\n");

			printf("</td></tr>\n");
			printf("<tr><td>\n");


			printf("<table border='1' cellspacing='0' cellpadding='0' align='left' width='100%%'>\n");
			printf("<tr><td style='vertical-align: top; padding-right:0.5em;'>\n");
			printf("<table border='0' class='stateInfoTable2'>\n");

			if ((temp_service->service_check_command) && (*temp_service->service_check_command != '\0'))
				printf("<tr><td class='dataVar' nowrap><a href='%s?type=command&amp;expand=%s'>Active Checks:</a></td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", CONFIG_CGI, url_encode(temp_service->service_check_command), (temp_svcstatus->checks_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->checks_enabled) ? "ENABLED" : "DISABLED");
			else printf("<tr><td class='dataVar' nowrap>Active Checks:</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_svcstatus->checks_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->checks_enabled) ? "ENABLED" : "DISABLED");

			printf("<tr><td class='dataVar' nowrap>Passive Checks:</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_svcstatus->accept_passive_service_checks == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->accept_passive_service_checks) ? "ENABLED" : "DISABLED");

			printf("<tr><td class='dataVar' nowrap>Obsessing:</td><td class='dataVal'><div class='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_svcstatus->obsess_over_service == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->obsess_over_service) ? "ENABLED" : "DISABLED");

			printf("<tr><td class='dataVar' nowrap>Notifications:</td><td class='dataVal'><div class='notifications%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_svcstatus->notifications_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->notifications_enabled) ? "ENABLED" : "DISABLED");

			if ((temp_service->event_handler) && (*temp_service->event_handler != '\0'))
				printf("<tr><td class='dataVar' nowrap><a href='%s?type=command&amp;expand=%s'>Event Handler:</a></td><td class='dataVal'><div class='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", CONFIG_CGI, url_encode(temp_service->event_handler), (temp_svcstatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->event_handler_enabled) ? "ENABLED" : "DISABLED");
			else printf("<tr><td class='dataVar' nowrap>Event Handler:</td><td class='dataVal'><div class='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_svcstatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->event_handler_enabled) ? "ENABLED" : "DISABLED");

			printf("<tr><td class='dataVar' nowrap>Flap Detection:</td><td class='dataVal'><div class='flapdetection%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</div></td></tr>\n", (temp_svcstatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED");


			printf("</table></td><td width='100&%%' style='vertical-align: top;'>\n");

			/* Custom Variables */
			if (temp_service->custom_variables) {
				printf("<table border='0' cellspacing='3' class='CustomVarTable' width='100%%'>\n");
				printf("<tr><td colspan='2' class='CustomVarHead' align='center' style='font-weight: bold; padding:0em'>Custom Variables</td></tr>\n");
				printf("<tr><td class='CustomVarHead' width='50%%'>Name</td><td class='CustomVarHead'>Value</td></tr>\n");
				for (temp_customvar = temp_service->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {
					if (check_exclude_customvar(temp_customvar) == FALSE)
						printf("<tr><td class='CustomVarLine'>%s</td><td class='CustomVarLine'>%s</td></tr>\n",temp_customvar->variable_name, temp_customvar->variable_value);
				}
				printf("</table>\n");
			}

			printf("</td></tr>\n");
			printf("</table>\n");

			printf("</td></tr>\n");
			printf("</table>\n");
		}


		printf("</td>\n");

		printf("<td align='center' valign='top'>\n");
		printf("<table border='0' cellpadding='0' cellspacing='0'><tr>\n");

		printf("<td align='center' valign='top' class='commandPanel'>\n");

		printf("<div class='dataTitle'>Service Commands</div>\n");

		printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
		printf("<tr><td>\n");

		if (is_authorized_for_read_only(&current_authdata) == FALSE) {
			printf("<table border='0' cellspacing='0' cellpadding='0' class='command'>\n");

			if (temp_svcstatus->checks_enabled) {

				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Active Checks Of This Service' title='Disable Active Checks Of This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SVC_CHECK, url_encode(host_name));
				printf("&amp;service=%s'>Disable active checks of this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Active Checks Of This Service' title='Enable Active Checks Of This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SVC_CHECK, url_encode(host_name));
				printf("&amp;service=%s'>Enable active checks of this service</a></td></tr>\n", url_encode(service_desc));
			}
			printf("<tr class='data'><td><img src='%s%s' border='0' alt='Re-schedule Next Service Check' title='Re-schedule Next Service Check'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, DELAY_ICON, CMD_CGI, CMD_SCHEDULE_SVC_CHECK, url_encode(host_name));
			printf("&amp;service=%s%s'>Re-schedule the next check of this service</a></td></tr>\n", url_encode(service_desc), (temp_svcstatus->checks_enabled == TRUE) ? "&amp;force_check" : "");

			if (temp_svcstatus->accept_passive_service_checks == TRUE) {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Submit Passive Check Result For This Service' title='Submit Passive Check Result For This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, PASSIVE_ICON, CMD_CGI, CMD_PROCESS_SERVICE_CHECK_RESULT, url_encode(host_name));
				printf("&amp;service=%s'>Submit passive check result for this service</a></td></tr>\n", url_encode(service_desc));

				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Accepting Passive Checks For This Service' title='Stop Accepting Passive Checks For This Service'></td><td class='command' nowrap><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_PASSIVE_SVC_CHECKS, url_encode(host_name));
				printf("&amp;service=%s'>Stop accepting passive checks for this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Accepting Passive Checks For This Service' title='Start Accepting Passive Checks For This Service'></td><td class='command' nowrap><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_PASSIVE_SVC_CHECKS, url_encode(host_name));
				printf("&amp;service=%s'>Start accepting passive checks for this service</a></td></tr>\n", url_encode(service_desc));
			}

			if (temp_svcstatus->obsess_over_service == TRUE) {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Stop Obsessing Over This Service' title='Stop Obsessing Over This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_STOP_OBSESSING_OVER_SVC, url_encode(host_name));
				printf("&amp;service=%s'>Stop obsessing over this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Start Obsessing Over This Service' title='Start Obsessing Over This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_START_OBSESSING_OVER_SVC, url_encode(host_name));
				printf("&amp;service=%s'>Start obsessing over this service</a></td></tr>\n", url_encode(service_desc));
			}

			if ((temp_svcstatus->status == SERVICE_WARNING || temp_svcstatus->status == SERVICE_UNKNOWN || temp_svcstatus->status == SERVICE_CRITICAL) && temp_svcstatus->state_type == HARD_STATE) {
				if (temp_svcstatus->problem_has_been_acknowledged == FALSE) {
					printf("<tr class='command'><td><img src='%s%s' border='0' alt='Acknowledge This Service Problem' title='Acknowledge This Service Problem'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, ACKNOWLEDGEMENT_ICON, CMD_CGI, CMD_ACKNOWLEDGE_SVC_PROBLEM, url_encode(host_name));
					printf("&amp;service=%s'>Acknowledge this service problem</a></td></tr>\n", url_encode(service_desc));
				} else {
					printf("<tr class='command'><td><img src='%s%s' border='0' alt='Remove Problem Acknowledgement' title='Remove Problem Acknowledgement'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, REMOVE_ACKNOWLEDGEMENT_ICON, CMD_CGI, CMD_REMOVE_SVC_ACKNOWLEDGEMENT, url_encode(host_name));
					printf("&amp;service=%s'>Remove problem acknowledgement</a></td></tr>\n", url_encode(service_desc));
				}
			}
			if (temp_svcstatus->notifications_enabled == TRUE) {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Notifications For This Service' title='Disable Notifications For This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SVC_NOTIFICATIONS, url_encode(host_name));
				printf("&amp;service=%s'>Disable notifications for this service</a></td></tr>\n", url_encode(service_desc));
				if (temp_svcstatus->status != SERVICE_OK) {
					printf("<tr class='command'><td><img src='%s%s' border='0' alt='Delay Next Service Notification' title='Delay Next Service Notification'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, DELAY_ICON, CMD_CGI, CMD_DELAY_SVC_NOTIFICATION, url_encode(host_name));
					printf("&amp;service=%s'>Delay next service notification</a></td></tr>\n", url_encode(service_desc));
				}
			} else {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Notifications For This Service' title='Enable Notifications For This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SVC_NOTIFICATIONS, url_encode(host_name));
				printf("&amp;service=%s'>Enable notifications for this service</a></td></tr>\n", url_encode(service_desc));
			}

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Send Custom Notification' title='Send Custom Notification'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_SEND_CUSTOM_SVC_NOTIFICATION, url_encode(host_name));
			printf("&amp;service=%s'>Send custom service notification</a></td></tr>\n", url_encode(service_desc));

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Schedule Downtime For This Service' title='Schedule Downtime For This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_SVC_DOWNTIME, url_encode(host_name));
			printf("&amp;service=%s'>Schedule downtime for this service</a></td></tr>\n", url_encode(service_desc));

			/*
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Cancel Scheduled Downtime For This Service' title='Cancel Scheduled Downtime For This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s",url_images_path,DOWNTIME_ICON,CMD_CGI,CMD_CANCEL_SVC_DOWNTIME,url_encode(host_name));
			printf("&amp;service=%s'>Cancel scheduled downtime for this service</a></td></tr>\n",url_encode(service_desc));
			*/

			if (temp_svcstatus->event_handler_enabled == TRUE) {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Event Handler For This Service' title='Disable Event Handler For This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SVC_EVENT_HANDLER, url_encode(host_name));
				printf("&amp;service=%s'>Disable event handler for this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Event Handler For This Service' title='Enable Event Handler For This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SVC_EVENT_HANDLER, url_encode(host_name));
				printf("&amp;service=%s'>Enable event handler for this service</a></td></tr>\n", url_encode(service_desc));
			}

			if (temp_svcstatus->flap_detection_enabled == TRUE) {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Flap Detection For This Service' title='Disable Flap Detection For This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SVC_FLAP_DETECTION, url_encode(host_name));
				printf("&amp;service=%s'>Disable flap detection for this service</a></td></tr>\n", url_encode(service_desc));
			} else {
				printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Flap Detection For This Service' title='Enable Flap Detection For This Service'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SVC_FLAP_DETECTION, url_encode(host_name));
				printf("&amp;service=%s'>Enable flap detection for this service</a></td></tr>\n", url_encode(service_desc));
			}

			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Add a new Service comment' title='Add a new Service comment'></td><td class='command'><a href='%s?cmd_typ=%d&amp;host=%s", url_images_path, COMMENT_ICON, CMD_CGI, CMD_ADD_SVC_COMMENT, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name));
			printf("&amp;service=%s'>", (display_type == DISPLAY_COMMENTS) ? "" : url_encode(service_desc));
			printf("Add a new Service comment</a></td>");

			if (status_file_icinga_version != NULL && status_file_icinga_version[0] != '1') {
				/* Modify Check/Retry Interval */
				printf("<tr class='command'><td><a href='%s?cmd_typ=%d&amp;interval=0.015&amp;host=%s&amp;service=%s&amp;cmd_mod=2'><img src='%s%s' border='0' alt='Modify Check/Retry Interval' title='Modify Check/Retry Interval'></a></td><td class='command'><a href='%s?cmd_typ=%d&amp;interval=0.015&amp;host=%s&amp;service=%s'>",
				       CMD_CGI, CMD_INTERNAL_CHANGE_SVC_CHECK_RETRY_INTERVAL,
				       (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name),
				       (display_type == DISPLAY_COMMENTS) ? "" : url_encode(service_desc),
				       url_images_path, RELOAD_ICON,
				       CMD_CGI, CMD_INTERNAL_CHANGE_SVC_CHECK_RETRY_INTERVAL,
				       (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name),
				       (display_type == DISPLAY_COMMENTS) ? "" : url_encode(service_desc));
				printf("Modify Check/Retry Interval</a></td>");
			}

			/* allow modified attributes to be reset */
			printf("<tr class='command'><td><img src='%s%s' border='0' alt='Reset Modified Attributes' title='Reset Modified Attributes'></td><td class='command'><a href='%s?cmd_typ=%d&amp;attr=%d&amp;host=%s", url_images_path, DISABLED_ICON, CMD_CGI, CMD_CHANGE_SVC_MODATTR, MODATTR_NONE, (display_type == DISPLAY_COMMENTS) ? "" : url_encode(host_name));
			printf("&amp;service=%s'>", (display_type == DISPLAY_COMMENTS) ? "" : url_encode(service_desc));
			printf("Reset Modified Attributes</a></td>");


			printf("</table>\n");
		} else {
			print_generic_error_message("Your account does not have permissions to execute commands.", NULL, 0);
		}

		printf("</td></tr>\n");
		printf("</table>\n");

		printf("</td>\n");

		printf("</tr></table></td>\n");
		printf("</tr>\n");

		printf("<tr><td colspan='2'><br></td></tr>\n");

		printf("<tr>\n");
		printf("<td colspan='2' valign='top' class='commentPanel'>\n");

		if (is_authorized_for_read_only(&current_authdata) == FALSE || is_authorized_for_comments_read_only(&current_authdata) == TRUE) {
			/* display comments */
			show_comments(SERVICE_COMMENT);
			printf("<br>");
			/* display downtimes */
			show_downtime(SERVICE_DOWNTIME);
		}
		printf("</td>\n");
		printf("</tr>\n");

		printf("</table>\n");
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

	printf("<div class='dataTitle'>Hostgroup Commands</div>\n");

	if (is_authorized_for_read_only(&current_authdata) == FALSE) {

		printf("<table border='0' cellspacing='0' cellpadding='0' class='command' align='center'>\n");

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Schedule Downtime For All Hosts In This Hostgroup' title='Schedule Downtime For All Hosts In This Hostgroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;hostgroup=%s'>Schedule downtime for all hosts in this hostgroup</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME, url_encode(hostgroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Schedule Downtime For All Services In This Hostgroup' title='Schedule Downtime For All Services In This Hostgroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;hostgroup=%s'>Schedule downtime for all services in this hostgroup</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME, url_encode(hostgroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Notifications For All Hosts In This Hostgroup' title='Enable Notifications For All Hosts In This Hostgroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;hostgroup=%s'>Enable notifications for all hosts in this hostgroup</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Notifications For All Hosts In This Hostgroup' title='Disable Notifications For All Hosts In This Hostgroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;hostgroup=%s'>Disable notifications for all hosts in this hostgroup</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Notifications For All Services In This Hostgroup' title='Enable Notifications For All Services In This Hostgroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;hostgroup=%s'>Enable notifications for all services in this hostgroup</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Notifications For All Services In This Hostgroup' title='Disable Notifications For All Services In This Hostgroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;hostgroup=%s'>Disable notifications for all services in this hostgroup</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Active Checks Of All Services In This Hostgroup' title='Enable Active Checks Of All Services In This Hostgroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;hostgroup=%s'>Enable active checks of all services in this hostgroup</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_HOSTGROUP_SVC_CHECKS, url_encode(hostgroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Active Checks Of All Services In This Hostgroup' title='Disable Active Checks Of All Services In This Hostgroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;hostgroup=%s'>Disable active checks of all services in this hostgroup</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_HOSTGROUP_SVC_CHECKS, url_encode(hostgroup_name));

		printf("</table>\n");

	} else {
		print_generic_error_message("Your account does not have permissions to execute commands.", NULL, 0);
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

	printf("<div class='dataTitle'>Servicegroup Commands</div>\n");

	if (is_authorized_for_read_only(&current_authdata) == FALSE) {

		printf("<table border='1' cellspacing='0' cellpadding='0' class='command' align='center'>\n");

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Schedule Downtime For All Hosts In This Servicegroup' title='Schedule Downtime For All Hosts In This Servicegroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;servicegroup=%s'>Schedule downtime for all hosts in this servicegroup</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME, url_encode(servicegroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Schedule Downtime For All Services In This Servicegroup' title='Schedule Downtime For All Services In This Servicegroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;servicegroup=%s'>Schedule downtime for all services in this servicegroup</a></td></tr>\n", url_images_path, DOWNTIME_ICON, CMD_CGI, CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME, url_encode(servicegroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Notifications For All Hosts In This Servicegroup' title='Enable Notifications For All Hosts In This Servicegroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;servicegroup=%s'>Enable notifications for all hosts in this servicegroup</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Notifications For All Hosts In This Servicegroup' title='Disable Notifications For All Hosts In This Servicegroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;servicegroup=%s'>Disable notifications for all hosts in this servicegroup</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, CMD_CGI, CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Notifications For All Services In This Servicegroup' title='Enable Notifications For All Services In This Servicegroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;servicegroup=%s'>Enable notifications for all services in this servicegroup</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, CMD_CGI, CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Notifications For All Services In This Servicegroup' title='Disable Notifications For All Services In This Servicegroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;servicegroup=%s'>Disable notifications for all services in this servicegroup</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, CMD_CGI, CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Enable Active Checks Of All Services In This Servicegroup' title='Enable Active Checks Of All Services In This Servicegroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;servicegroup=%s'>Enable active checks of all services in this servicegroup</a></td></tr>\n", url_images_path, ENABLED_ICON, CMD_CGI, CMD_ENABLE_SERVICEGROUP_SVC_CHECKS, url_encode(servicegroup_name));

		printf("<tr class='command'><td><img src='%s%s' border='0' alt='Disable Active Checks Of All Services In This Servicegroup' title='Disable Active Checks Of All Services In This Servicegroup'></td><td class='command'><a href='%s?cmd_typ=%d&amp;servicegroup=%s'>Disable active checks of all services in this servicegroup</a></td></tr>\n", url_images_path, DISABLED_ICON, CMD_CGI, CMD_DISABLE_SERVICEGROUP_SVC_CHECKS, url_encode(servicegroup_name));

		printf("</table>\n");

	} else {
		print_generic_error_message("Your account does not have permissions to execute commands.", NULL, 0);
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


	if (content_type == JSON_CONTENT) {
		printf("\"performance_data\": {\n");

		printf("\"active_service_checks_1min_num\": %d,\n", active_service_checks_1min);
		printf("\"active_service_checks_1min_percent\": \"%.1f%%\",\n", (double)(((double)active_service_checks_1min * 100.0) / (double)total_active_service_checks));
		printf("\"active_service_checks_15min_num\": %d,\n", active_service_checks_15min);
		printf("\"active_service_checks_15min_percent\": \"%.1f%%\",\n", (double)(((double)active_service_checks_15min * 100.0) / (double)total_active_service_checks));
		printf("\"active_service_checks_1hour_num\": %d,\n", active_service_checks_1hour);
		printf("\"active_service_checks_1hour_percent\": \"%.1f%%\",\n", (double)(((double)active_service_checks_1hour * 100.0) / (double)total_active_service_checks));
		printf("\"active_service_checks_since_start_num\": %d,\n", active_service_checks_start);
		printf("\"active_service_checks_since_start_percent\": \"%.1f%%\",\n", (double)(((double)active_service_checks_start * 100.0) / (double)total_active_service_checks));

		printf("\"active_service_min_execution_time\": \"%.2f sec\",\n", min_service_execution_time);
		printf("\"active_service_max_execution_time\": \"%.2f sec\",\n", max_service_execution_time);
		printf("\"active_service_average_execution_time\": \"%.3f sec\",\n", (double)((double)total_service_execution_time / (double)total_active_service_checks));

		printf("\"active_service_min_latency\": \"%.2f sec\",\n", min_service_latency);
		printf("\"active_service_max_latency\": \"%.2f sec\",\n", max_service_latency);
		printf("\"active_service_average_latency\": \"%.3f sec\",\n", (double)((double)total_service_latency / (double)total_active_service_checks));

		printf("\"active_service_min_percent_change\": \"%.2f sec\",\n", min_service_percent_change_a);
		printf("\"active_service_max_percent_change\": \"%.2f sec\",\n", max_service_percent_change_a);
		printf("\"active_service_average_percent_change\": \"%.2f sec\",\n", (double)((double)total_service_percent_change_a / (double)total_active_service_checks));

		printf("\"passive_service_checks_1min_num\": %d,\n", passive_service_checks_1min);
		printf("\"passive_service_checks_1min_percent\": \"%.1f%%\",\n", (double)(((double)passive_service_checks_1min * 100.0) / (double)total_passive_service_checks));
		printf("\"passive_service_checks_15min_num\": %d,\n", passive_service_checks_15min);
		printf("\"passive_service_checks_15min_percent\": \"%.1f%%\",\n", (double)(((double)passive_service_checks_15min * 100.0) / (double)total_passive_service_checks));
		printf("\"passive_service_checks_1hour_num\": %d,\n", passive_service_checks_1hour);
		printf("\"passive_service_checks_1hour_percent\": \"%.1f%%\",\n", (double)(((double)passive_service_checks_1hour * 100.0) / (double)total_passive_service_checks));
		printf("\"passive_service_checks_since_start_num\": %d,\n", passive_service_checks_start);
		printf("\"passive_service_checks_since_start_percent\": \"%.1f%%\",\n", (double)(((double)passive_service_checks_start * 100.0) / (double)total_passive_service_checks));

		printf("\"passive_service_min_percent_change\": \"%.2f sec\",\n", min_service_percent_change_b);
		printf("\"passive_service_max_percent_change\": \"%.2f sec\",\n", max_service_percent_change_b);
		printf("\"passive_service_average_percent_change\": \"%.2f sec\",\n", (double)((double)total_service_percent_change_b / (double)total_passive_service_checks));

		printf("\"active_host_checks_1min_num\": %d,\n", active_host_checks_1min);
		printf("\"active_host_checks_1min_percent\": \"%.1f%%\",\n", (double)(((double)active_host_checks_1min * 100.0) / (double)total_active_host_checks));
		printf("\"active_host_checks_15min_num\": %d,\n", active_host_checks_15min);
		printf("\"active_host_checks_15min_percent\": \"%.1f%%\",\n", (double)(((double)active_host_checks_15min * 100.0) / (double)total_active_host_checks));
		printf("\"active_host_checks_1hour_num\": %d,\n", active_host_checks_1hour);
		printf("\"active_host_checks_1hour_percent\": \"%.1f%%\",\n", (double)(((double)active_host_checks_1hour * 100.0) / (double)total_active_host_checks));
		printf("\"active_host_checks_since_start_num\": %d,\n", active_host_checks_start);
		printf("\"active_host_checks_since_start_percent\": \"%.1f%%\",\n", (double)(((double)active_host_checks_start * 100.0) / (double)total_active_host_checks));

		printf("\"active_host_min_execution_time\": \"%.2f sec\",\n", min_host_execution_time);
		printf("\"active_host_max_execution_time\": \"%.2f sec\",\n", max_host_execution_time);
		printf("\"active_host_average_execution_time\": \"%.3f sec\",\n", (double)((double)total_host_execution_time / (double)total_active_host_checks));

		printf("\"active_host_min_latency\": \"%.2f sec\",\n", min_host_latency);
		printf("\"active_host_max_latency\": \"%.2f sec\",\n", max_host_latency);
		printf("\"active_host_average_latency\": \"%.3f sec\",\n", (double)((double)total_host_latency / (double)total_active_host_checks));

		printf("\"active_host_min_percent_change\": \"%.2f sec\",\n", min_host_percent_change_a);
		printf("\"active_host_max_percent_change\": \"%.2f sec\",\n", max_host_percent_change_a);
		printf("\"active_host_average_percent_change\": \"%.2f sec\",\n", (double)((double)total_host_percent_change_a / (double)total_active_host_checks));

		printf("\"passive_host_checks_1min_num\": %d,\n", passive_host_checks_1min);
		printf("\"passive_host_checks_1min_percent\": \"%.1f%%\",\n", (double)(((double)passive_host_checks_1min * 100.0) / (double)total_passive_host_checks));
		printf("\"passive_host_checks_15min_num\": %d,\n", passive_host_checks_15min);
		printf("\"passive_host_checks_15min_percent\": \"%.1f%%\",\n", (double)(((double)passive_host_checks_15min * 100.0) / (double)total_passive_host_checks));
		printf("\"passive_host_checks_1hour_num\": %d,\n", passive_host_checks_1hour);
		printf("\"passive_host_checks_1hour_percent\": \"%.1f%%\",\n", (double)(((double)passive_host_checks_1hour * 100.0) / (double)total_passive_host_checks));
		printf("\"passive_host_checks_since_start_num\": %d,\n", passive_host_checks_start);
		printf("\"passive_host_checks_since_start_percent\": \"%.1f%%\",\n", (double)(((double)passive_host_checks_start * 100.0) / (double)total_passive_host_checks));

		printf("\"passive_host_min_percent_change\": \"%.2f sec\",\n", min_host_percent_change_b);
		printf("\"passive_host_max_percent_change\": \"%.2f sec\",\n", max_host_percent_change_b);
		printf("\"passive_host_average_percent_change\": \"%.2f sec\",\n", (double)((double)total_host_percent_change_b / (double)total_passive_host_checks));

		printf("\"active_scheduled_host_checks_1min\": %d,\n", program_stats[ACTIVE_SCHEDULED_HOST_CHECK_STATS][0]);
		printf("\"active_scheduled_host_checks_5min\": %d,\n", program_stats[ACTIVE_SCHEDULED_HOST_CHECK_STATS][1]);
		printf("\"active_scheduled_host_checks_15min\": %d,\n", program_stats[ACTIVE_SCHEDULED_HOST_CHECK_STATS][2]);

		printf("\"active_on_demand_host_checks_1min\": %d,\n", program_stats[ACTIVE_ONDEMAND_HOST_CHECK_STATS][0]);
		printf("\"active_on_demand_host_checks_5min\": %d,\n", program_stats[ACTIVE_ONDEMAND_HOST_CHECK_STATS][1]);
		printf("\"active_on_demand_host_checks_15min\": %d,\n", program_stats[ACTIVE_ONDEMAND_HOST_CHECK_STATS][2]);

		printf("\"parallel_host_checks_1min\": %d,\n", program_stats[PARALLEL_HOST_CHECK_STATS][0]);
		printf("\"parallel_host_checks_5min\": %d,\n", program_stats[PARALLEL_HOST_CHECK_STATS][1]);
		printf("\"parallel_host_checks_15min\": %d,\n", program_stats[PARALLEL_HOST_CHECK_STATS][2]);

		printf("\"serial_host_checks_1min\": %d,\n", program_stats[SERIAL_HOST_CHECK_STATS][0]);
		printf("\"serial_host_checks_5min\": %d,\n", program_stats[SERIAL_HOST_CHECK_STATS][1]);
		printf("\"serial_host_checks_15min\": %d,\n", program_stats[SERIAL_HOST_CHECK_STATS][2]);

		printf("\"cached_host_checks_1min\": %d,\n", program_stats[ACTIVE_CACHED_HOST_CHECK_STATS][0]);
		printf("\"cached_host_checks_5min\": %d,\n", program_stats[ACTIVE_CACHED_HOST_CHECK_STATS][1]);
		printf("\"cached_host_checks_15min\": %d,\n", program_stats[ACTIVE_CACHED_HOST_CHECK_STATS][2]);

		printf("\"passive_host_checks_1min\": %d,\n", program_stats[PASSIVE_HOST_CHECK_STATS][0]);
		printf("\"passive_host_checks_5min\": %d,\n", program_stats[PASSIVE_HOST_CHECK_STATS][1]);
		printf("\"passive_host_checks_15min\": %d,\n", program_stats[PASSIVE_HOST_CHECK_STATS][2]);

		printf("\"active_scheduled_service_checks_1min\": %d,\n", program_stats[ACTIVE_SCHEDULED_SERVICE_CHECK_STATS][0]);
		printf("\"active_scheduled_service_checks_5min\": %d,\n", program_stats[ACTIVE_SCHEDULED_SERVICE_CHECK_STATS][1]);
		printf("\"active_scheduled_service_checks_15min\": %d,\n", program_stats[ACTIVE_SCHEDULED_SERVICE_CHECK_STATS][2]);

		printf("\"active_on_demand_service_checks_1min\": %d,\n", program_stats[ACTIVE_ONDEMAND_SERVICE_CHECK_STATS][0]);
		printf("\"active_on_demand_service_checks_5min\": %d,\n", program_stats[ACTIVE_ONDEMAND_SERVICE_CHECK_STATS][1]);
		printf("\"active_on_demand_service_checks_15min\": %d,\n", program_stats[ACTIVE_ONDEMAND_SERVICE_CHECK_STATS][2]);

		printf("\"cached_service_checks_1min\": %d,\n", program_stats[ACTIVE_CACHED_SERVICE_CHECK_STATS][0]);
		printf("\"cached_service_checks_5min\": %d,\n", program_stats[ACTIVE_CACHED_SERVICE_CHECK_STATS][1]);
		printf("\"cached_service_checks_15min\": %d,\n", program_stats[ACTIVE_CACHED_SERVICE_CHECK_STATS][2]);

		printf("\"passive_service_checks_1min\": %d,\n", program_stats[PASSIVE_SERVICE_CHECK_STATS][0]);
		printf("\"passive_service_checks_5min\": %d,\n", program_stats[PASSIVE_SERVICE_CHECK_STATS][1]);
		printf("\"passive_service_checks_15min\": %d,\n", program_stats[PASSIVE_SERVICE_CHECK_STATS][2]);

		printf("\"external_commands_1min\": %d,\n", program_stats[EXTERNAL_COMMAND_STATS][0]);
		printf("\"external_commands_5min\": %d,\n", program_stats[EXTERNAL_COMMAND_STATS][1]);
		printf("\"external_commands_15min\": %d,\n", program_stats[EXTERNAL_COMMAND_STATS][2]);

		printf("\"external_command_buffer_in_use\": %d,\n", buffer_stats[0][1]);
		printf("\"external_command_buffer_max_used\": %d,\n", buffer_stats[0][2]);
		printf("\"external_command_buffer_total_available\": %d\n", buffer_stats[0][0]);

		printf("}\n");

		return;
	}

	printf("<div align='center'>\n");


	printf("<div class='dataTitle'>Program-Wide Performance Information</div>\n");

	printf("<table border='0' cellpadding='10'>\n");


	/***** ACTIVE SERVICE CHECKS *****/

	printf("<tr>\n");
	printf("<td valign='middle'><div class='perfTypeTitle'>Services Actively Checked:</div></td>\n");
	printf("<td valign='top'>\n");

	/* fake this so we don't divide by zero for just showing the table */
	if (total_active_service_checks == 0)
		total_active_service_checks = 1;

	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable1'>\n");
	printf("<table border='0'>\n");

	printf("<tr class='data'><th class='data'>Time Frame</th><th class='data'>Services Checked</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1 minute:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_1min, (double)(((double)active_service_checks_1min * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 5 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_5min, (double)(((double)active_service_checks_5min * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 15 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_15min, (double)(((double)active_service_checks_15min * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 1 hour:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_1hour, (double)(((double)active_service_checks_1hour * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>Since program start:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td>", active_service_checks_start, (double)(((double)active_service_checks_start * 100.0) / (double)total_active_service_checks));

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("</td><td valign='top'>\n");

	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable2'>\n");
	printf("<table border='0'>\n");

	printf("<tr class='data'><th class='data'>Metric</th><th class='data'>Min.</th><th class='data'>Max.</th><th class='data'>Average</th></tr>\n");

	printf("<tr><td class='dataVar'>Check Execution Time:&nbsp;&nbsp;</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.3f sec</td></tr>\n", min_service_execution_time, max_service_execution_time, (double)((double)total_service_execution_time / (double)total_active_service_checks));

	printf("<tr><td class='dataVar'>Check Latency:</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.3f sec</td></tr>\n", min_service_latency, max_service_latency, (double)((double)total_service_latency / (double)total_active_service_checks));

	printf("<tr><td class='dataVar'>Percent State Change:</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_service_percent_change_a, max_service_percent_change_a, (double)((double)total_service_percent_change_a / (double)total_active_service_checks));

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");


	printf("</td>\n");
	printf("</tr>\n");


	/***** PASSIVE SERVICE CHECKS *****/

	printf("<tr>\n");
	printf("<td valign='middle'><div class='perfTypeTitle'>Services Passively Checked:</div></td>\n");
	printf("<td valign='top'>\n");


	/* fake this so we don't divide by zero for just showing the table */
	if (total_passive_service_checks == 0)
		total_passive_service_checks = 1;

	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable1'>\n");
	printf("<table border='0'>\n");

	printf("<tr class='data'><th class='data'>Time Frame</th><th class='data'>Services Checked</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1 minute:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_1min, (double)(((double)passive_service_checks_1min * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 5 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_5min, (double)(((double)passive_service_checks_5min * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 15 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_15min, (double)(((double)passive_service_checks_15min * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 1 hour:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_1hour, (double)(((double)passive_service_checks_1hour * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>Since program start:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_start, (double)(((double)passive_service_checks_start * 100.0) / (double)total_passive_service_checks));

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("</td><td valign='top'>\n");

	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable2'>\n");
	printf("<table border='0'>\n");

	printf("<tr class='data'><th class='data'>Metric</th><th class='data'>Min.</th><th class='data'>Max.</th><th class='data'>Average</th></tr>\n");
	printf("<tr><td class='dataVar'>Percent State Change:&nbsp;&nbsp;</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_service_percent_change_b, max_service_percent_change_b, (double)((double)total_service_percent_change_b / (double)total_passive_service_checks));

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("</td>\n");
	printf("</tr>\n");


	/***** ACTIVE HOST CHECKS *****/

	printf("<tr>\n");
	printf("<td valign='middle'><div class='perfTypeTitle'>Hosts Actively Checked:</div></td>\n");
	printf("<td valign='top'>\n");

	/* fake this so we don't divide by zero for just showing the table */
	if (total_active_host_checks == 0)
		total_active_host_checks = 1;

	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable1'>\n");
	printf("<table border='0'>\n");

	printf("<tr class='data'><th class='data'>Time Frame</th><th class='data'>Hosts Checked</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1 minute:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_1min, (double)(((double)active_host_checks_1min * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 5 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_5min, (double)(((double)active_host_checks_5min * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 15 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_15min, (double)(((double)active_host_checks_15min * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 1 hour:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_1hour, (double)(((double)active_host_checks_1hour * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>Since program start:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td>", active_host_checks_start, (double)(((double)active_host_checks_start * 100.0) / (double)total_active_host_checks));

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("</td><td valign='top'>\n");

	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable2'>\n");
	printf("<table border='0'>\n");

	printf("<tr class='data'><th class='data'>Metric</th><th class='data'>Min.</th><th class='data'>Max.</th><th class='data'>Average</th></tr>\n");

	printf("<tr><td class='dataVar'>Check Execution Time:&nbsp;&nbsp;</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.3f sec</td></tr>\n", min_host_execution_time, max_host_execution_time, (double)((double)total_host_execution_time / (double)total_active_host_checks));

	printf("<tr><td class='dataVar'>Check Latency:</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.2f sec</td><td class='dataVal'>%.3f sec</td></tr>\n", min_host_latency, max_host_latency, (double)((double)total_host_latency / (double)total_active_host_checks));

	printf("<tr><td class='dataVar'>Percent State Change:</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_host_percent_change_a, max_host_percent_change_a, (double)((double)total_host_percent_change_a / (double)total_active_host_checks));

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");


	printf("</td>\n");
	printf("</tr>\n");


	/***** PASSIVE HOST CHECKS *****/

	printf("<tr>\n");
	printf("<td valign='middle'><div class='perfTypeTitle'>Hosts Passively Checked:</div></td>\n");
	printf("<td valign='top'>\n");


	/* fake this so we don't divide by zero for just showing the table */
	if (total_passive_host_checks == 0)
		total_passive_host_checks = 1;

	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable1'>\n");
	printf("<table border='0'>\n");

	printf("<tr class='data'><th class='data'>Time Frame</th><th class='data'>Hosts Checked</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1 minute:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_1min, (double)(((double)passive_host_checks_1min * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 5 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_5min, (double)(((double)passive_host_checks_5min * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 15 minutes:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_15min, (double)(((double)passive_host_checks_15min * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 1 hour:</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_1hour, (double)(((double)passive_host_checks_1hour * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>Since program start:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_start, (double)(((double)passive_host_checks_start * 100.0) / (double)total_passive_host_checks));

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("</td><td valign='top'>\n");

	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable2'>\n");
	printf("<table border='0'>\n");

	printf("<tr class='data'><th class='data'>Metric</th><th class='data'>Min.</th><th class='data'>Max.</th><th class='data'>Average</th></tr>\n");
	printf("<tr><td class='dataVar'>Percent State Change:&nbsp;&nbsp;</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_host_percent_change_b, max_host_percent_change_b, (double)((double)total_host_percent_change_b / (double)total_passive_host_checks));

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("</td>\n");
	printf("</tr>\n");



	/***** CHECK STATS *****/

	printf("<tr>\n");
	printf("<td valign='middle'><div class='perfTypeTitle'>Check Statistics:</div></td>\n");
	printf("<td valign='top' colspan='2'>\n");


	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable1'>\n");
	printf("<table border='0'>\n");

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

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("</td>\n");
	printf("</tr>\n");



	/***** BUFFER STATS *****/

	printf("<tr>\n");
	printf("<td valign='middle'><div class='perfTypeTitle'>Buffer Usage:</div></td>\n");
	printf("<td valign='top' colspan='2'>\n");


	printf("<table border='1' cellspacing='0' cellpadding='0'>\n");
	printf("<tr><td class='stateInfoTable1'>\n");
	printf("<table border='0'>\n");

	printf("<tr class='data'><th class='data'>Type</th><th class='data'>In Use</th><th class='data'>Max Used</th><th class='data'>Total Available</th></tr>\n");
	printf("<tr><td class='dataVar'>External Commands&nbsp;</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", buffer_stats[0][1], buffer_stats[0][2], buffer_stats[0][0]);

	printf("</table>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("</td>\n");
	printf("</tr>\n");

	printf("</table>\n");


	printf("</div>\n");

	return;
}

void show_comments(int type) {
	host *temp_host = NULL;
	service *temp_service = NULL;
	sortdata *temp_sortdata = NULL;
	comment *temp_comment = NULL;
	char *bg_class = "";
	char *comment_type;
	char date_time[MAX_DATETIME_LENGTH];
	char expire_time[MAX_DATETIME_LENGTH];
	char temp_url[MAX_INPUT_BUFFER];
	char temp_buffer[MAX_INPUT_BUFFER];
	int odd = 1;
	int total_comments = 0;
	int colspan = 8;
	int json_start = TRUE;
	int first_entry = TRUE;

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
		printf("<a name='%sCOMMENTS'></a>\n", (type == HOST_COMMENT) ? "HOST" : "SERVICE");
		printf("<table border='0' class='comment' style='padding:0px;margin-bottom: -6px;'><tr><td width='33%%'></td><td width='33%%'><div class='commentTitle'>%s Comments</div></td><td width='33%%'>", (type == HOST_COMMENT) ? "Host" : "Service");

		/* add export to csv, json, link */
		printf("<div style='padding-right:6px;' class='csv_export_link'>");
		if (display_type != DISPLAY_COMMENTS)
			print_export_link(CSV_CONTENT, EXTINFO_CGI, "csvtype=comment");
		else if (type == HOST_COMMENT) {
			print_export_link(CSV_CONTENT, EXTINFO_CGI, "csvtype=comment");
			print_export_link(JSON_CONTENT, EXTINFO_CGI, NULL);
			print_export_link(HTML_CONTENT, EXTINFO_CGI, NULL);
		}
		printf("</div>");

		printf("</td></tr></table>\n");

		printf("<form name='tableform%scomment' id='tableform%scomment' action='%s' method='POST' onkeypress='var key = (window.event) ? event.keyCode : event.which; return (key != 13);'>", (type == HOST_COMMENT) ? "host" : "service", (type == HOST_COMMENT) ? "host" : "service", CMD_CGI);
		printf("<input type='hidden' name='buttonCheckboxChecked'>");
		printf("<input type='hidden' name='cmd_typ' value='%d'>", (type == HOST_COMMENT) ? CMD_DEL_HOST_COMMENT : CMD_DEL_SVC_COMMENT);

		printf("<table border='0' class='comment'>\n");

		printf("<tr><td colspan='%d' align='right'>", colspan);

		if (display_type == DISPLAY_COMMENTS && type == HOST_COMMENT) {
			printf("<table width='100%%' cellspacing='0' cellpadding='0'><tr><td width='33%%'></td><td width='33%%' nowrap>");
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

		printf("</td></tr>\n");

		snprintf(temp_url, sizeof(temp_url) - 1, "%s?type=%d", EXTINFO_CGI, display_type);
		temp_url[sizeof(temp_url) - 1] = '\x0';

		if (host_name && *host_name != '\0') {
			strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
			snprintf(temp_url, sizeof(temp_url) - 1, "%s&amp;host=%s", temp_buffer, url_encode(host_name));
			temp_url[sizeof(temp_url) - 1] = '\x0';
		}

		if (service_desc && *service_desc != '\0') {
			strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
			snprintf(temp_url, sizeof(temp_url) - 1, "%s&amp;service=%s", temp_buffer, url_encode(service_desc));
			temp_url[sizeof(temp_url) - 1] = '\x0';
		}

		printf("<tr class='comment'>");
		if (display_type == DISPLAY_COMMENTS) {
			printf("<th class='comment'>Host Name&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by host name (ascending)' title='Sort by host name (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by host name (descending)' title='Sort by host name (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_HOSTNAME_SERVICENAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_HOSTNAME_SERVICENAME, url_images_path, DOWN_ARROW_ICON);

			if (type == SERVICE_COMMENT)
				printf("<th class='comment'>Service&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by service description (ascending)' title='Sort by service description (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by service description (descending)' title='Sort by service description (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_SERVICENAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_SERVICENAME, url_images_path, DOWN_ARROW_ICON);
		}
		printf("<th class='comment'>Entry Time&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by entry time (ascending)' title='Sort by entry time (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by entry time (descending)' title='Sort by entry time (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_ENTRYTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_ENTRYTIME, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='comment'>Author&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by author (ascending)' title='Sort by author (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by author (descending)' title='Sort by author (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_AUTHOR, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_AUTHOR, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='comment'>Comment&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by comment (ascending)' title='Sort by comment (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by comment (descending)' title='Sort by comment (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_COMMENT, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_COMMENT, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='comment'>Comment ID&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by comment id (ascending)' title='Sort by comment id (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by comment id (descending)' title='Sort by comment id (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_ID, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_ID, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='comment'>Persistent&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by persistent (ascending)' title='Sort by persistent (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by persistent (descending)' title='Sort by persistent (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_PERSISTENT, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_PERSISTENT, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='comment'>Type&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by type (ascending)' title='Sort by type (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by type (descending)' title='Sort by type (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_TYPE, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_TYPE, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='comment'>Expires&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by expire time (ascending)' title='Sort by expire time (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by expire time (descending)' title='Sort by expire time (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_EXPIRETIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_EXPIRETIME, url_images_path, DOWN_ARROW_ICON);

		if (is_authorized_for_comments_read_only(&current_authdata) == FALSE)
			printf("<th class='comment' nowrap>Actions&nbsp;&nbsp;<input type='checkbox' value='all' onclick=\"checkAll(\'tableform%scomment\');isValidForSubmit(\'tableform%scomment\');\"></th>", (type == HOST_COMMENT) ? "host" : "service", (type == HOST_COMMENT) ? "host" : "service");
		printf("</tr>\n");
	}

	/* display comments */
	while (1) {

		if (sort_option != SORT_NOTHING) {

			if (first_entry == TRUE) {
				temp_sortdata = sortdata_list;
				first_entry = FALSE;
			} else
				temp_sortdata = temp_sortdata->next;

			if (temp_sortdata == NULL)
				break;

			if (temp_sortdata->data_type != SORT_COMMENT)
				continue;

			temp_comment = temp_sortdata->comment;

		} else {
			if (first_entry == TRUE) {
				temp_comment = comment_list;
				first_entry = FALSE;
			} else
				temp_comment = temp_comment->next;
		}

		if (temp_comment == NULL)
			break;

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
			printf("<tr class='%s' onClick=\"toggle_checkbox('comment_%lu','tableform%scomment');\">", bg_class, temp_comment->comment_id, (type == HOST_COMMENT) ? "host" : "service");
			if (display_type == DISPLAY_COMMENTS) {
				printf("<td><a href='%s?type=%d&amp;host=%s'>%s</a></td>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_comment->host_name), (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);
				if (type == SERVICE_COMMENT) {
					printf("<td><a href='%s?type=%d&amp;host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_comment->host_name));
					printf("&amp;service=%s'>%s</a></td>", url_encode(temp_comment->service_description), (temp_service->display_name != NULL) ? temp_service->display_name : temp_service->description);
				}
			}
			printf("<td name='comment_time'>%s</td><td name='comment_author'>%s</td><td name='comment_data'>%s</td><td name='comment_id'>%lu</td><td name='comment_persist'>%s</td><td name='comment_type'>%s</td><td name='comment_expire'>%s</td>", date_time, temp_comment->author, temp_comment->comment_data, temp_comment->comment_id, (temp_comment->persistent) ? "Yes" : "No", comment_type, (temp_comment->expires == TRUE) ? expire_time : "N/A");
			if (is_authorized_for_comments_read_only(&current_authdata) == FALSE) {
				printf("<td align='center'><a href='%s?cmd_typ=%d&amp;com_id=%lu'><img src='%s%s' border='0' alt='Delete This Comment' title='Delete This Comment'></a>", CMD_CGI, (type == HOST_COMMENT) ? CMD_DEL_HOST_COMMENT : CMD_DEL_SVC_COMMENT, temp_comment->comment_id, url_images_path, DELETE_ICON);
				printf("<input type='checkbox' onClick=\"toggle_checkbox('comment_%lu','tableform%scomment');\" name='com_id' id='comment_%lu' value='%lu'></td>", temp_comment->comment_id, (type == HOST_COMMENT) ? "host" : "service", temp_comment->comment_id, temp_comment->comment_id);
			}
			printf("</tr>\n");
		}
		total_comments++;
	}

	/* see if this host or service has any comments associated with it */
	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		if (total_comments == 0 && total_entries == 0) {
			printf("<tr class='commentOdd'><td align='center' colspan='%d'>", colspan);
			if (display_type == DISPLAY_COMMENTS)
				printf("There are no %s comments", (type == HOST_COMMENT) ? "host" : "service");
			else
				printf("This %s has no comments associated with it", (type == HOST_COMMENT) ? "host" : "service");
			printf("</td></tr>\n");
		}

		if (display_type == DISPLAY_COMMENTS && type == SERVICE_COMMENT) {
			printf("<tr><td colspan='%d'>\n", colspan);
			page_num_selector(result_start, total_entries, displayed_entries);
			printf("</td></tr>\n");
		}

		printf("</table>\n");
		printf("<script language='javascript' type='text/javascript'>\n");
		printf("document.tableform%scomment.buttonCheckboxChecked.value = 'false';\n", (type == HOST_COMMENT) ? "host" : "service");
		printf("checked = true;\n");
		printf("checkAll(\"tableform%scomment\");\n", (type == HOST_COMMENT) ? "host" : "service");
		printf("checked = false;\n");
		printf("</script>\n");
		printf("</form>\n");
	}
	if (content_type == JSON_CONTENT)
		printf("]");

	return;
}

/* shows service and host scheduled downtime */
void show_downtime(int type) {
	char *bg_class = "";
	char date_time[MAX_DATETIME_LENGTH];
	char temp_url[MAX_INPUT_BUFFER];
	char temp_buffer[MAX_INPUT_BUFFER];
	scheduled_downtime *temp_downtime = NULL;
	host *temp_host = NULL;
	service *temp_service = NULL;
	sortdata *temp_sortdata = NULL;
	int days;
	int hours;
	int minutes;
	int seconds;
	int odd = 0;
	int total_downtime = 0;
	int colspan = 12;
	int json_start = TRUE;
	int first_entry = TRUE;

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
		printf("<a name='%sDOWNTIME'></a>\n", (type == HOST_DOWNTIME) ? "HOST" : "SERVICE");
		printf("<table border='0' class='comment' style='padding:0px;margin-bottom: -6px;'><tr><td width='33%%'></td><td width='33%%'><div class='commentTitle'>Scheduled %s Downtime</div></td><td width='33%%'>", (type == HOST_DOWNTIME) ? "Host" : "Service");

		/* add export to csv, json, link */
		printf("<div style='padding-right:6px;' class='csv_export_link'>");
		if (display_type != DISPLAY_DOWNTIME)
			print_export_link(CSV_CONTENT, EXTINFO_CGI, "csvtype=downtime");
		else if (type == HOST_DOWNTIME) {
			print_export_link(CSV_CONTENT, EXTINFO_CGI, "csvtype=downtime");
			print_export_link(JSON_CONTENT, EXTINFO_CGI, NULL);
			print_export_link(HTML_CONTENT, EXTINFO_CGI, NULL);
		}
		printf("</div>");

		printf("</td></tr></table>\n");

		printf("<form name='tableform%sdowntime' id='tableform%sdowntime' action='%s' method='POST' onkeypress='var key = (window.event) ? event.keyCode : event.which; return (key != 13);'>", (type == HOST_DOWNTIME) ? "host" : "service", (type == HOST_DOWNTIME) ? "host" : "service", CMD_CGI);
		printf("<input type='hidden' name='buttonCheckboxChecked'>");
		printf("<input type='hidden' name='cmd_typ' value='%d'>", (type == HOST_DOWNTIME) ? CMD_DEL_HOST_DOWNTIME : CMD_DEL_SVC_DOWNTIME);

		printf("<table border='0' class='downtime'>\n");

		printf("<tr><td colspan='%d' align='right'>", colspan);

		if (display_type == DISPLAY_DOWNTIME && type == HOST_DOWNTIME) {
			printf("<table width='100%%' cellspacing='0' cellpadding='0'><tr><td width='33%%'></td><td width='33%%' nowrap>");
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

		printf("</td></tr>\n");

		snprintf(temp_url, sizeof(temp_url) - 1, "%s?type=%d", EXTINFO_CGI, display_type);
		temp_url[sizeof(temp_url) - 1] = '\x0';

		if (host_name && *host_name != '\0') {
			strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
			snprintf(temp_url, sizeof(temp_url) - 1, "%s&amp;host=%s", temp_buffer, url_encode(host_name));
			temp_url[sizeof(temp_url) - 1] = '\x0';
		}

		if (service_desc && *service_desc != '\0') {
			strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
			snprintf(temp_url, sizeof(temp_url) - 1, "%s&amp;service=%s", temp_buffer, url_encode(service_desc));
			temp_url[sizeof(temp_url) - 1] = '\x0';
		}

		printf("<tr class='downtime'>");
		if (display_type == DISPLAY_DOWNTIME) {
			printf("<th class='downtime'>Host Name&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by host name (ascending)' title='Sort by host name (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by host name (descending)' title='Sort by host name (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_HOSTNAME_SERVICENAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_HOSTNAME_SERVICENAME, url_images_path, DOWN_ARROW_ICON);

			if (type == SERVICE_DOWNTIME)
				printf("<th class='downtime'>Service&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by service description (ascending)' title='Sort by service description (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by service description (descending)' title='Sort by service description (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_SERVICENAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_SERVICENAME, url_images_path, DOWN_ARROW_ICON);
		}
		printf("<th class='downtime'>Entry Time&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by entry time (ascending)' title='Sort by entry time (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by entry time (descending)' title='Sort by entry time (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_ENTRYTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_ENTRYTIME, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>Author&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by author (ascending)' title='Sort by author (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by author (descending)' title='Sort by author (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_AUTHOR, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_AUTHOR, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>Comment&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by comment (ascending)' title='Sort by comment (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by comment (descending)' title='Sort by comment (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_COMMENT, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_COMMENT, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>Start Time&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by start time (ascending)' title='Sort by start time (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by start time (descending)' title='Sort by start time (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_STARTTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_STARTTIME, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>End Time&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by end time (ascending)' title='Sort by end time (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by end time (descending)' title='Sort by end time (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_ENDTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_ENDTIME, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>Type&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by type (ascending)' title='Sort by type (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by type (descending)' title='Sort by type (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_TYPE, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_TYPE, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>Trigger Time&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by trigger time (ascending)' title='Sort by trigger time (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by trigger time (descending)' title='Sort by trigger time (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_TRIGGERTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_TRIGGERTIME, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>Duration&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by duration (ascending)' title='Sort by duration (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by duration (descending)' title='Sort by duration (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_DURATION, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_DURATION, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>Is in effect&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by in effect (ascending)' title='Sort by in effect (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by in effect (descending)' title='Sort by in effect (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_INEFFECT, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_INEFFECT, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>Downtime ID&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by downtime id (ascending)' title='Sort by downtime id (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by downtime id (descending)' title='Sort by downtime id (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_ID, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_ID, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='downtime'>Trigger ID&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by trigger id (ascending)' title='Sort by trigger id (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by trigger id (descending)' title='Sort by trigger id (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_EX_TRIGGERID, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_EX_TRIGGERID, url_images_path, DOWN_ARROW_ICON);

		if (is_authorized_for_downtimes_read_only(&current_authdata) == FALSE)
			printf("<th class='comment' nowrap>Actions&nbsp;&nbsp;<input type='checkbox' value='all' onclick=\"checkAll(\'tableform%sdowntime\');isValidForSubmit(\'tableform%sdowntime\');\"></th>", (type == HOST_DOWNTIME) ? "host" : "service", (type == HOST_DOWNTIME) ? "host" : "service");
		printf("</tr>\n");
	}

	/* display downtime data */
	while (1) {

		if (sort_option != SORT_NOTHING) {

			if (first_entry == TRUE) {
				temp_sortdata = sortdata_list;
				first_entry = FALSE;
			} else
				temp_sortdata = temp_sortdata->next;

			if (temp_sortdata == NULL)
				break;

			if (temp_sortdata->data_type != SORT_DOWNTIME)
				continue;

			temp_downtime = temp_sortdata->downtime;
		} else {
			if (first_entry == TRUE) {
				temp_downtime = scheduled_downtime_list;
				first_entry = FALSE;
			} else
				temp_downtime = temp_downtime->next;
		}

		if (temp_downtime == NULL)
			break;

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
			printf("<tr class='%s' onClick=\"toggle_checkbox('downtime_%lu','tableform%sdowntime');\">", bg_class, temp_downtime->downtime_id, (type == HOST_DOWNTIME) ? "host" : "service");
			if (display_type == DISPLAY_DOWNTIME) {
				printf("<td class='%s'><a href='%s?type=%d&amp;host=%s'>%s</a></td>", bg_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_downtime->host_name), (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);
				if (type == SERVICE_DOWNTIME) {
					printf("<td class='%s'><a href='%s?type=%d&amp;host=%s", bg_class, EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_downtime->host_name));
					printf("&amp;service=%s'>%s</a></td>", url_encode(temp_downtime->service_description), (temp_service->display_name != NULL) ? temp_service->display_name : temp_service->description);
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
			if (temp_downtime->comment == NULL)
				printf("\"comment\": null, ");
			else
				printf("\"comment\": \"%s\", ", json_encode(temp_downtime->comment));
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_downtime->author == NULL) ? "N/A" : temp_downtime->author, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_downtime->comment == NULL) ? "N/A" : temp_downtime->comment, csv_data_enclosure, csv_delimiter);
		} else {
			printf("<td class='%s'>%s</td>", bg_class, date_time);
			printf("<td class='%s'>%s</td>", bg_class, (temp_downtime->author == NULL) ? "N/A" : temp_downtime->author);
			printf("<td class='%s'>%s</td>", bg_class, (temp_downtime->comment == NULL) ? "N/A" : temp_downtime->comment);
		}

		get_time_string(&temp_downtime->start_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		if (content_type == JSON_CONTENT)
			printf("\"start_time\": \"%s\", ", date_time);
		else if (content_type == CSV_CONTENT)
			printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
		else
			printf("<td class='%s'>%s</td>", bg_class, date_time);

		get_time_string(&temp_downtime->end_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		if (content_type == JSON_CONTENT) {
			printf("\"end_time\": \"%s\", ", date_time);
			printf("\"type\": \"%s\", ", (temp_downtime->fixed == TRUE) ? "Fixed" : "Flexible");
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, date_time, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_downtime->fixed == TRUE) ? "Fixed" : "Flexible", csv_data_enclosure, csv_delimiter);
		} else {
			printf("<td class='%s'>%s</td>", bg_class, date_time);
			printf("<td class='%s'>%s</td>", bg_class, (temp_downtime->fixed == TRUE) ? "Fixed" : "Flexible");
		}

		get_time_string(&temp_downtime->trigger_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		if (content_type == JSON_CONTENT) {
			if (temp_downtime->trigger_time != 0)
				printf("\"trigger_time\": \"%s\", ", date_time);
			else
				printf("\"trigger_time\": null, ");
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, (temp_downtime->trigger_time != 0) ? date_time : "N/A", csv_data_enclosure, csv_delimiter);
		} else {
			printf("<td class='%s'>%s</td>", bg_class, (temp_downtime->trigger_time != 0) ? date_time : "N/A");
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
			printf("<td class='%s'>%dd %dh %dm %ds</td>", bg_class, days, hours, minutes, seconds);
			printf("<td class='%s'>%s</td>", bg_class, (temp_downtime->is_in_effect == TRUE) ? "True" : "False");
			printf("<td class='%s'>%lu</td>", bg_class, temp_downtime->downtime_id);
			printf("<td class='%s'>", bg_class);
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
				printf("<td align='center' class='%s'><a href='%s?cmd_typ=%d", bg_class, CMD_CGI, (type == HOST_DOWNTIME) ? CMD_DEL_HOST_DOWNTIME : CMD_DEL_SVC_DOWNTIME);
				printf("&amp;down_id=%lu'><img src='%s%s' border='0' alt='Delete/Cancel This Scheduled Downtime Entry' title='Delete/Cancel This Scheduled Downtime Entry'></a>", temp_downtime->downtime_id, url_images_path, DELETE_ICON);
				printf("<input type='checkbox' onClick=\"toggle_checkbox('downtime_%lu','tableform%sdowntime');\" name='down_id' id='downtime_%lu' value='%lu'></td>", temp_downtime->downtime_id, (type == HOST_DOWNTIME) ? "host" : "service", temp_downtime->downtime_id, temp_downtime->downtime_id);
			}
			printf("</tr>\n");
		}
		total_downtime++;
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		if (total_downtime == 0 && total_entries == 0) {
			printf("<tr class='downtimeOdd'><td  align='center' colspan='%d'>", colspan);
			if (display_type == DISPLAY_DOWNTIME)
				printf("There are no %s with scheduled downtime", (type == HOST_DOWNTIME) ? "host" : "service");
			else
				printf("This %s has no scheduled downtime associated with it", (type == HOST_DOWNTIME) ? "host" : "service");
			printf("</td></tr>\n");
		}

		if (display_type == DISPLAY_DOWNTIME && type == SERVICE_DOWNTIME) {
			printf("<tr><td colspan='%d'>\n", colspan);
			page_num_selector(result_start, total_entries, displayed_entries);
			printf("</td></tr>\n");
		}

		printf("</table>\n");
		printf("<script language='javascript' type='text/javascript'>\n");
		printf("document.tableform%sdowntime.buttonCheckboxChecked.value = 'false';\n", (type == HOST_DOWNTIME) ? "host" : "service");
		printf("checked = true;\n");
		printf("checkAll(\"tableform%sdowntime\");\n", (type == HOST_DOWNTIME) ? "host" : "service");
		printf("checked = false;\n");
		printf("</script>\n");
		printf("</form>\n");
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

	if (sort_option == SORT_NOTHING) {
		sort_option = SORT_NEXTCHECKTIME;
	}

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
		printf("<div align='center' class='statusSort'>Entries sorted by <b>");
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
		printf("</div>\n");

		printf("<table border='0' class='queue' align='center'>\n");

		/* add export to csv link */
		printf("<tr><td colspan='7'>\n");
		printf("<table width='100%%' cellspacing='0' cellpadding='0'><tr><td width='15%%'></td><td width='70%%' nowrap>");

		printf("<div class='page_selector'>\n");
		printf("<div id='page_navigation_copy'></div>\n");
		page_limit_selector(result_start);
		printf("</div>\n");

		printf("</td><td width='15%%' align='right'>\n<div class='csv_export_link'>\n");
		print_export_link(CSV_CONTENT, EXTINFO_CGI, NULL);
		print_export_link(JSON_CONTENT, EXTINFO_CGI, NULL);
		print_export_link(HTML_CONTENT, EXTINFO_CGI, NULL);
		printf("</div>\n");
		printf("</td></tr></table>");
		printf("</td></tr>\n");

		printf("<tr class='queue'>");

		snprintf(temp_url, sizeof(temp_url) - 1, "%s?type=%d", EXTINFO_CGI, DISPLAY_SCHEDULING_QUEUE);
		temp_url[sizeof(temp_url) - 1] = '\x0';

		if (host_name && *host_name != '\0') {
			strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
			snprintf(temp_url, sizeof(temp_url) - 1, "%s&amp;host=%s", temp_buffer, url_encode(host_name));
			temp_url[sizeof(temp_url) - 1] = '\x0';
		}

		if (service_desc && *service_desc != '\0') {
			strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
			snprintf(temp_url, sizeof(temp_url) - 1, "%s&amp;service=%s", temp_buffer, url_encode(service_desc));
			temp_url[sizeof(temp_url) - 1] = '\x0';
		}

		printf("<th class='queue'>Host&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by host name (ascending)' title='Sort by host name (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by host name (descending)' title='Sort by host name (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_HOSTNAME_SERVICENAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_HOSTNAME_SERVICENAME, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='queue'>Service&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by service name (ascending)' title='Sort by service name (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by service name (descending)' title='Sort by service name (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_SERVICENAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_SERVICENAME, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='queue'>Last Check&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by last check time (ascending)' title='Sort by last check time (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by last check time (descending)' title='Sort by last check time (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_LASTCHECKTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_LASTCHECKTIME, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='queue'>Next Check&nbsp;<a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by next check time (ascending)' title='Sort by next check time (ascending)'></a><a href='%s&amp;sorttype=%d&amp;sortoption=%d'><img src='%s%s' border='0' alt='Sort by next check time (descending)' title='Sort by next check time (descending)'></a></th>", temp_url, SORT_ASCENDING, SORT_NEXTCHECKTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_NEXTCHECKTIME, url_images_path, DOWN_ARROW_ICON);
		printf("<th class='queue'>Type</th><th class='queue'>Active Checks</th><th class='queue'>Actions</th></tr>\n");
	}

	/* display all services and hosts */
	for (temp_sortdata = sortdata_list; temp_sortdata != NULL; temp_sortdata = temp_sortdata->next) {

		/* skip hosts and services that shouldn't be scheduled */
		if (temp_sortdata->data_type == SORT_SERVICESTATUS) {
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

		} else if (temp_sortdata->data_type == SORT_HOSTSTATUS) {
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
		} else {
			continue;
		}

		/* get the service status */
		if (temp_sortdata->data_type == SORT_SERVICESTATUS) {

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
			snprintf(service_link, sizeof(service_link) - 1, "%s?type=%d&amp;host=%s&amp;service=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encoded_host, url_encoded_service);
			service_link[sizeof(service_link) - 1] = '\x0';

			/* last check */
			get_time_string(&temp_svcstatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			last_check = (temp_svcstatus->last_check == (time_t)0) ? strdup("N/A") : strdup(date_time);

			/* next check */
			get_time_string(&temp_svcstatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			next_check = (temp_svcstatus->next_check == (time_t)0) ? strdup("N/A") : strdup(date_time);

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
				snprintf(action_link_enable_disable, sizeof(action_link_enable_disable) - 1, "%s?cmd_typ=%d&amp;host=%s&amp;service=%s", CMD_CGI, CMD_DISABLE_SVC_CHECK, url_encoded_host, url_encoded_service);
			else
				snprintf(action_link_enable_disable, sizeof(action_link_enable_disable) - 1, "%s?cmd_typ=%d&amp;host=%s&amp;service=%s", CMD_CGI, CMD_ENABLE_SVC_CHECK, url_encoded_host, url_encoded_service);
			action_link_enable_disable[sizeof(action_link_enable_disable) - 1] = '\x0';

			snprintf(action_link_schedule, sizeof(action_link_schedule) - 1, "%s?cmd_typ=%d&amp;host=%s&amp;service=%s%s", CMD_CGI, CMD_SCHEDULE_SVC_CHECK, url_encoded_host, url_encoded_service, (temp_svcstatus->checks_enabled == TRUE) ? "&force_check" : "");
			action_link_schedule[sizeof(action_link_schedule) - 1] = '\x0';

			/* get the host status */
		} else if (temp_sortdata->data_type == SORT_HOSTSTATUS) {
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
			last_check = (temp_hststatus->last_check == (time_t)0) ? strdup("N/A") : strdup(date_time);

			/* next check */
			get_time_string(&temp_hststatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			next_check = (temp_hststatus->next_check == (time_t)0) ? strdup("N/A") : strdup(date_time);

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
				snprintf(action_link_enable_disable, sizeof(action_link_enable_disable) - 1, "%s?cmd_typ=%d&amp;host=%s", CMD_CGI, CMD_DISABLE_HOST_CHECK, url_encoded_host);
			else
				snprintf(action_link_enable_disable, sizeof(action_link_enable_disable) - 1, "%s?cmd_typ=%d&amp;host=%s", CMD_CGI, CMD_ENABLE_HOST_CHECK, url_encoded_host);
			action_link_enable_disable[sizeof(action_link_enable_disable) - 1] = '\x0';

			snprintf(action_link_schedule, sizeof(action_link_schedule) - 1, "%s?cmd_typ=%d&amp;host=%s%s", CMD_CGI, CMD_SCHEDULE_HOST_CHECK, url_encoded_host, (temp_hststatus->checks_enabled == TRUE) ? "&amp;force_check" : "");
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
			if (temp_sortdata->data_type == SORT_SERVICESTATUS) {
				printf("\"service_description\": \"%s\", ", json_encode(service_native_name));
				printf("\"service_display_name\": \"%s\", ", json_encode(service_display_name));
				printf("\"type\": \"SERVICE_CHECK\", ");
			} else if (temp_sortdata->data_type == SORT_HOSTSTATUS) {
				printf("\"type\": \"HOST_CHECK\", ");
			}

			printf("\"last_check\": \"%s\", ", last_check);
			printf("\"next_check\": \"%s\", ", next_check);
			printf("\"type\": \"%s\", ", type);
			printf("\"active_check\": %s }", (checks_enabled == TRUE) ? "true" : "false");
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, host_native_name, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_sortdata->data_type == SORT_SERVICESTATUS) ? service_native_name : "", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, last_check, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, next_check, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, type, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s\n", csv_data_enclosure, (checks_enabled == TRUE) ? "ENABLED" : "DISABLED", csv_data_enclosure);
		} else {
			printf("<tr class='queue%s'>", bgclass);

			/* Host */
			printf("<td class='queue%s'><a href='%s?type=%d&amp;host=%s'>%s</a></td>", bgclass, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encoded_host, html_encode(host_display_name,TRUE));

			/* Service */
			if (temp_sortdata->data_type == SORT_SERVICESTATUS)
				printf("<td class='queue%s'><a href='%s'>%s</a></td>", bgclass, service_link, html_encode(service_display_name,TRUE));
			else if (temp_sortdata->data_type == SORT_HOSTSTATUS)
				printf("<td class='queue%s'>&nbsp;</td>", bgclass);

			/* last check */
			printf("<td class='queue%s'>%s</td>", bgclass, last_check);

			/* next check */
			printf("<td class='queue%s'>%s</td>", bgclass, next_check);

			/* type */
			printf("<td align='center' class='queue%s'>%s</td>", bgclass, type);

			/* active checks */
			printf("<td class='queue%s'>%s</td>", (checks_enabled == TRUE) ? "ENABLED" : "DISABLED", (checks_enabled == TRUE) ? "ENABLED" : "DISABLED");

			/* actions */
			printf("<td align='center' class='queue%s'>", bgclass);
			printf("<a href='%s'><img src='%s%s' border='0' alt='%s Active Checks Of This %s' title='%s Active Checks Of This %s'></a>\n", action_link_enable_disable, url_images_path, (checks_enabled == TRUE) ? DISABLED_ICON : ENABLED_ICON, (checks_enabled == TRUE) ? "Disable" : "Enable", (temp_sortdata->data_type == SORT_SERVICESTATUS) ? "Service" : "Host", (checks_enabled == TRUE) ? "Disable" : "Enable", (temp_sortdata->data_type == SORT_SERVICESTATUS) ? "Service" : "Host");
			printf("<a href='%s'><img src='%s%s' border='0' alt='Re-schedule This %s Check' title='Re-schedule This %s Check'></a>", action_link_schedule, url_images_path, DELAY_ICON, (temp_sortdata->data_type == SORT_SERVICESTATUS) ? "Service" : "Host", (temp_sortdata->data_type == SORT_SERVICESTATUS) ? "Service" : "Host");

			printf("</td></tr>\n");
		}

		my_free(last_check);
		my_free(next_check);
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("<tr><td colspan='7'>\n");
		page_num_selector(result_start, total_entries, displayed_entries);
		printf("</td></tr>\n");
		printf("</table>\n");
	} else if (content_type == JSON_CONTENT)
		printf(" ] \n");

	return;
}

/* sorts host and service data */
int sort_data(int s_type, int s_option) {
	sortdata *new_sortdata;
	sortdata *last_sortdata;
	sortdata *temp_sortdata;
	servicestatus *temp_svcstatus;
	hoststatus *temp_hststatus;
	comment *temp_comment;
	scheduled_downtime *temp_downtime;
	int svcstatus_start = TRUE;
	int hststatus_start = TRUE;
	int comment_start   = TRUE;
	int downtime_start  = TRUE;
	int data_type = SORT_NOTHING;

	if (s_type == SORT_NONE || s_option == SORT_NOTHING)
		return ERROR;

	temp_svcstatus = servicestatus_list;
	temp_hststatus = hoststatus_list;
	temp_comment = comment_list;
	temp_downtime = scheduled_downtime_list;

	while (1) {

		data_type = SORT_NOTHING;

		if (display_type == DISPLAY_HOST_INFO || display_type == DISPLAY_SERVICE_INFO || display_type == DISPLAY_COMMENTS || display_type == DISPLAY_DOWNTIME) {

			/* sort comments */
			if (display_type != DISPLAY_DOWNTIME) {
				if (temp_comment != NULL) {

					if (comment_start == TRUE) {
						comment_start = FALSE;
					} else {
						temp_comment = temp_comment->next;
					}

					if (temp_comment == NULL) {
						continue;
					}

					data_type = SORT_COMMENT;
				}
			}

			/* sort downtimes */
			if (display_type != DISPLAY_COMMENTS) {
				if (temp_downtime != NULL) {

					if (downtime_start == TRUE) {
						downtime_start = FALSE;
					} else {
						temp_downtime = temp_downtime->next;
					}

					if (temp_downtime == NULL) {
						continue;
					}

					data_type = SORT_DOWNTIME;
				}
			}

		} else if (display_type == DISPLAY_SCHEDULING_QUEUE) {

			/* sort service status */
			if (temp_svcstatus != NULL) {

				if (svcstatus_start == TRUE) {
					svcstatus_start = FALSE;
				} else {
					temp_svcstatus = temp_svcstatus->next;
				}

				if (temp_svcstatus == NULL) {
					continue;
				}

				data_type = SORT_SERVICESTATUS;

			/* sort host status */
			} else if (temp_hststatus != NULL) {

				if (hststatus_start == TRUE) {
					hststatus_start = FALSE;
				} else {
					temp_hststatus = temp_hststatus->next;
				}

				if (temp_hststatus == NULL) {
					continue;
				}

				data_type = SORT_HOSTSTATUS;
			}
		}

		if (data_type == SORT_NOTHING) {
			break;
		}

		/* allocate memory for a new sort structure */
		new_sortdata = (sortdata *)malloc(sizeof(sortdata));
		if (new_sortdata == NULL)
			return ERROR;

		new_sortdata->data_type = data_type;
		new_sortdata->svcstatus = (data_type == SORT_SERVICESTATUS && temp_svcstatus != NULL) ? temp_svcstatus : NULL;
		new_sortdata->hststatus = (data_type == SORT_HOSTSTATUS && temp_hststatus != NULL) ? temp_hststatus : NULL;
		new_sortdata->comment   = (data_type == SORT_COMMENT && temp_comment != NULL) ? temp_comment : NULL;
		new_sortdata->downtime  = (data_type == SORT_DOWNTIME && temp_downtime != NULL) ? temp_downtime : NULL;

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
	comment *temp_comment = NULL;
	scheduled_downtime *temp_downtime = NULL;
	time_t last_check[2];
	time_t next_check[2];
	time_t entry_time[2];
	time_t expire_time[2];
	time_t start_time[2];
	time_t end_time[2];
	time_t trigger_time[2];
	time_t duration[2];
	int id[2];
	int persistent[2];
	int type[2];
	int in_effect[2];
	int trigger_id[2];
	char *host_name[2];
	char *service_description[2];
	char *author[2];
	char *comment_data[2];
	char tmp_buffer_a[MAX_INPUT_BUFFER] = "";
	char tmp_buffer_b[MAX_INPUT_BUFFER] = "";

	/* initialize vars */
	last_check[0] = last_check[1] = 0;
	next_check[0] = next_check[1] = 0;
	entry_time[0] = entry_time[1] = 0;
	expire_time[0] = expire_time[1] = 0;
	start_time[0] = start_time[1] = 0;
	end_time[0] = end_time[1] = 0;
	trigger_time[0] = trigger_time[1] = 0;
	duration[0] = duration[1] = 0;
	id[0] = id[1] = 0;
	persistent[0] = persistent[1] = 0;
	type[0] = type[1] = 0;
	in_effect[0] = in_effect[1] = 0;
	trigger_id[0] = trigger_id[1] = 0;
	host_name[0] = host_name[1] = NULL;
	service_description[0] = service_description[1] = NULL;
	author[0] = author[1] = NULL;
	comment_data[0] = comment_data[1] = NULL;

	if (new_sortdata->data_type == SORT_SERVICESTATUS) {
		temp_svcstatus = new_sortdata->svcstatus;
		last_check[0] = temp_svcstatus->last_check;
		next_check[0] = temp_svcstatus->next_check;
		host_name[0] = temp_svcstatus->host_name;
		service_description[0] = temp_svcstatus->description;
	} else if (new_sortdata->data_type == SORT_HOSTSTATUS) {
		temp_hststatus = new_sortdata->hststatus;
		last_check[0] = temp_hststatus->last_check;
		next_check[0] = temp_hststatus->next_check;
		host_name[0] = temp_hststatus->host_name;
		service_description[0] = "";
	} else if (new_sortdata->data_type == SORT_COMMENT) {
		temp_comment = new_sortdata->comment;
		host_name[0] = temp_comment->host_name;
		service_description[0] = (temp_comment->comment_type == SERVICE_COMMENT) ? temp_comment->service_description : "";
		entry_time[0] = temp_comment->entry_time;
		author[0] = temp_comment->author;
		comment_data[0] = temp_comment->comment_data;
		id[0] = temp_comment->comment_id;
		persistent[0] = temp_comment->persistent;
		type[0] = temp_comment->entry_type;
		expire_time[0] = temp_comment->expire_time;
	} else if (new_sortdata->data_type == SORT_DOWNTIME) {
		temp_downtime = new_sortdata->downtime;
		host_name[0] = temp_downtime->host_name;
		service_description[0] = (temp_downtime->type == SERVICE_DOWNTIME) ? temp_downtime->service_description : "";
		entry_time[0] = temp_downtime->entry_time;
		author[0] = temp_downtime->author;
		comment_data[0] = temp_downtime->comment;
		start_time[0] = temp_downtime->start_time;
		end_time[0] = temp_downtime->end_time;
		type[0] = temp_downtime->fixed;
		trigger_time[0] = temp_downtime->trigger_time;
		duration[0] = temp_downtime->duration;
		in_effect[0] = temp_downtime->is_in_effect;
		id[0] = temp_downtime->downtime_id;
		trigger_id[0] = temp_downtime->triggered_by;
	} else {
		return TRUE;
	}

	if (temp_sortdata->data_type == SORT_SERVICESTATUS) {
		temp_svcstatus = temp_sortdata->svcstatus;
		last_check[1] = temp_svcstatus->last_check;
		next_check[1] = temp_svcstatus->next_check;
		host_name[1] = temp_svcstatus->host_name;
		service_description[1] = temp_svcstatus->description;
	} else if (temp_sortdata->data_type == SORT_HOSTSTATUS) {
		temp_hststatus = temp_sortdata->hststatus;
		last_check[1] = temp_hststatus->last_check;
		next_check[1] = temp_hststatus->next_check;
		host_name[1] = temp_hststatus->host_name;
		service_description[1] = "";
	} else if (temp_sortdata->data_type == SORT_COMMENT) {
		temp_comment = temp_sortdata->comment;
		host_name[1] = temp_comment->host_name;
		service_description[1] = (temp_comment->comment_type == SERVICE_COMMENT) ? temp_comment->service_description : "";
		entry_time[1] = temp_comment->entry_time;
		author[1] = temp_comment->author;
		comment_data[1] = temp_comment->comment_data;
		id[1] = temp_comment->comment_id;
		persistent[1] = temp_comment->persistent;
		type[1] = temp_comment->entry_type;
		expire_time[1] = temp_comment->expire_time;
	} else if (temp_sortdata->data_type == SORT_DOWNTIME) {
		temp_downtime = temp_sortdata->downtime;
		host_name[1] = temp_downtime->host_name;
		service_description[1] = (temp_downtime->type == SERVICE_DOWNTIME) ? temp_downtime->service_description : "";
		entry_time[1] = temp_downtime->entry_time;
		author[1] = temp_downtime->author;
		comment_data[1] = temp_downtime->comment;
		start_time[1] = temp_downtime->start_time;
		end_time[1] = temp_downtime->end_time;
		type[1] = temp_downtime->fixed;
		trigger_time[1] = temp_downtime->trigger_time;
		duration[1] = temp_downtime->duration;
		in_effect[1] = temp_downtime->is_in_effect;
		id[1] = temp_downtime->downtime_id;
		trigger_id[1] = temp_downtime->triggered_by;
	} else {
		return TRUE;
	}

	if (s_type == SORT_ASCENDING) {

		if (s_option == SORT_LASTCHECKTIME) {
			if (last_check[0] <= last_check[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_NEXTCHECKTIME) {
			if (next_check[0] <= next_check[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_ENTRYTIME) {
			if (entry_time[0] <= entry_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_EXPIRETIME) {
			if (expire_time[0] <= expire_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_STARTTIME) {
			if (start_time[0] <= start_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_ENDTIME) {
			if (end_time[0] <= end_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_TRIGGERTIME) {
			if (trigger_time[0] <= trigger_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_DURATION) {
			if (duration[0] <= duration[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_ID) {
			if (id[0] <= id[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_PERSISTENT) {
			if (persistent[0] <= persistent[1])
				return TRUE;
			else
				return FALSE;

		/* this one is reverse due to type names */
		} else if (s_option == SORT_EX_TYPE) {
			if (type[0] > type[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_INEFFECT) {
			if (in_effect[0] <= in_effect[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_TRIGGERID) {
			if (trigger_id[0] <= trigger_id[1])
				return TRUE;
			else
				return FALSE;
		} else 	if (s_option == SORT_HOSTNAME) {
			if (strcasecmp(host_name[0], host_name[1]) < 0)
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_SERVICENAME) {
			if (strcasecmp(service_description[0], service_description[1]) < 0)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_HOSTNAME_SERVICENAME) {
			snprintf(tmp_buffer_a, sizeof(tmp_buffer_a) - 1, "%s%s", host_name[0], service_description[0]);
			tmp_buffer_a[sizeof(tmp_buffer_a) - 1] = '\x0';

			snprintf(tmp_buffer_b, sizeof(tmp_buffer_b) - 1, "%s%s", host_name[1], service_description[1]);
			tmp_buffer_b[sizeof(tmp_buffer_b) - 1] = '\x0';

			if (strcasecmp(tmp_buffer_a, tmp_buffer_b) < 0)
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_AUTHOR) {
			if (strcasecmp(author[0], author[1]) < 0)
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_COMMENT) {
			if (strcasecmp(comment_data[0], comment_data[1]) < 0)
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
		} else if (s_option == SORT_NEXTCHECKTIME) {
			if (next_check[0] > next_check[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_ENTRYTIME) {
			if (entry_time[0] > entry_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_EXPIRETIME) {
			if (expire_time[0] > expire_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_STARTTIME) {
			if (start_time[0] > start_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_ENDTIME) {
			if (end_time[0] > end_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_TRIGGERTIME) {
			if (trigger_time[0] > trigger_time[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_DURATION) {
			if (duration[0] > duration[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_ID) {
			if (id[0] > id[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_PERSISTENT) {
			if (persistent[0] > persistent[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_TYPE) {
			if (type[0] <= type[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_INEFFECT) {
			if (in_effect[0] > in_effect[1])
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_TRIGGERID) {
			if (trigger_id[0] > trigger_id[1])
				return TRUE;
			else
				return FALSE;
		} else 	if (s_option == SORT_HOSTNAME) {
			if (strcasecmp(host_name[0], host_name[1]) > 0)
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_SERVICENAME) {
			if (strcasecmp(service_description[0], service_description[1]) > 0)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_HOSTNAME_SERVICENAME) {
			snprintf(tmp_buffer_a, sizeof(tmp_buffer_a) - 1, "%s%s", host_name[0], service_description[0]);
			tmp_buffer_a[sizeof(tmp_buffer_a) - 1] = '\x0';

			snprintf(tmp_buffer_b, sizeof(tmp_buffer_b) - 1, "%s%s", host_name[1], service_description[1]);
			tmp_buffer_b[sizeof(tmp_buffer_b) - 1] = '\x0';

			if (strcasecmp(tmp_buffer_a, tmp_buffer_b) > 0)
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_AUTHOR) {
			if (strcasecmp(author[0], author[1]) > 0)
				return TRUE;
			else
				return FALSE;
		} else if (s_option == SORT_EX_COMMENT) {
			if (strcasecmp(comment_data[0], comment_data[1]) > 0)
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

	if (sortdata_list == NULL)
		return;

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

