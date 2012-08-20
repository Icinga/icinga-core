/**************************************************************************
 *
 * OUTAGES.C -  Icinga Network Outages CGI
 *
 * Copyright (c) 1999-2008 Ethan Galstad (egalstad@nagios.org)
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
 *************************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/comments.h"
#include "../include/statusdata.h"

#include "../include/cgiutils.h"
#include "../include/getcgi.h"
#include "../include/cgiauth.h"

extern time_t		program_start;

extern host *host_list;
extern service *service_list;
extern hoststatus *hoststatus_list;
extern servicestatus *servicestatus_list;

extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];

/* AFFECTEDHOSTS structure */
typedef struct affected_host {
	char *host_name;
	struct affected_host *next;
} affected_host;

/* HOSTOUTAGE structure */
typedef struct hostoutage_struct {
	host *hst;
	int  severity;
	int  affected_child_hosts;
	int  affected_child_services;
	unsigned long monitored_time;
	unsigned long time_up;
	float percent_time_up;
	unsigned long time_down;
	float percent_time_down;
	unsigned long time_unreachable;
	float percent_time_unreachable;
	struct affected_host *affected_hosts;
	struct hostoutage_struct *next;
} hostoutage;


/* HOSTOUTAGESORT structure */
typedef struct hostoutagesort_struct {
	hostoutage *outage;
	struct hostoutagesort_struct *next;
} hostoutagesort;

int process_cgivars(void);

void display_network_outages(void);
void find_hosts_causing_outages(void);
void calculate_outage_effects(void);
void calculate_outage_effect_of_host(host *, int *, int *);
int is_route_to_host_blocked(host *);
int number_of_host_services(host *);
void add_hostoutage(host *);
void sort_hostoutages(void);
void free_hostoutage_list(void);
void free_hostoutagesort_list(void);
void add_affected_host(char *);


authdata current_authdata;

hostoutage *hostoutage_list = NULL;
hostoutagesort *hostoutagesort_list = NULL;
hostoutage *currently_checked_host = NULL;

int service_severity_divisor = 4;          /* default = services are 1/4 as important as hosts */

extern int embedded;
extern int refresh;
extern int display_header;
extern int daemon_check;
extern int content_type;

extern char *csv_delimiter;
extern char *csv_data_enclosure;

int CGI_ID = OUTAGES_CGI_ID;

int main(void) {
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

	/* read all status data */
	result = read_all_status_data(get_cgi_config_location(), READ_ALL_STATUS_DATA);
	if (result == ERROR && daemon_check == TRUE) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(NULL, ERROR_CGI_STATUS_DATA);
		document_footer(CGI_ID);
		free_memory();
		return ERROR;
	}

	document_header(CGI_ID, TRUE, "Network Outages");

	/* get authentication information */
	get_authentication_information(&current_authdata);

	if (display_header == TRUE) {

		/* begin top table */
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=33%%>\n");
		display_info_table("Network Outages", &current_authdata, daemon_check);
		printf("</td>\n");

		/* middle column of top row */
		printf("<td align=center valign=top width=33%%>\n");
		printf("</td>\n");

		/* right column of top row */
		printf("<td align=right valign=bottom width=33%%>\n");
		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");

	}


	/* display network outage info */
	display_network_outages();

	document_footer(CGI_ID);

	/* free memory allocated to comment data */
	free_comment_data();

	/* free all allocated memory */
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

		/* we found the service severity divisor option */
		if (!strcmp(variables[x], "service_divisor")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			service_severity_divisor = atoi(variables[x]);
			if (service_severity_divisor < 1)
				service_severity_divisor = 1;
		}

		/* we found the CSV output option */
		else if (!strcmp(variables[x], "csvoutput")) {
			display_header = FALSE;
			content_type = CSV_CONTENT;
		}

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

		/* we found the pause option */
		else if (!strcmp(variables[x], "paused"))
			refresh = FALSE;

		/* we found the nodaemoncheck option */
		else if (!strcmp(variables[x], "nodaemoncheck"))
			daemon_check = FALSE;

	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

/* shows all hosts that are causing network outages */
void display_network_outages(void) {
	int number_of_problem_hosts = 0;
	int number_of_blocking_problem_hosts = 0;
	hostoutagesort *temp_hostoutagesort;
	hostoutage *temp_hostoutage;
	hoststatus *temp_hoststatus;
	affected_host * temp_affected_host;
	int odd = 0;
	char *bg_class = "";
	char *status = "";
	int days;
	int hours;
	int minutes;
	int seconds;
	int total_comments;
	time_t t;
	time_t current_time;
	char state_duration[48];
	int total_entries = 0;
	int json_start = TRUE;

	/* find all hosts that are causing network outages */
	find_hosts_causing_outages();

	/* calculate outage effects */
	calculate_outage_effects();

	/* sort the outage list by severity */
	sort_hostoutages();

	/* count the number of top-level hosts that are down and the ones that are actually blocking children hosts */
	for (temp_hostoutage = hostoutage_list; temp_hostoutage != NULL; temp_hostoutage = temp_hostoutage->next) {
		number_of_problem_hosts++;
		if (temp_hostoutage->affected_child_hosts > 1)
			number_of_blocking_problem_hosts++;
	}

	if (content_type == JSON_CONTENT) {
		printf("\"outages\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sSEVERITY%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHOST%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sSTATE%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNOTES%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sSTATE_DURATION%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHOSTS_AFFECTED%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sSERVICES_AFFECTED%s\n", csv_data_enclosure, csv_data_enclosure);
	} else {
		/* display the problem hosts... */

		printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 align='center'><TR><TD WIDTH='33%%'></TD><TD WIDTH='33%%'><DIV CLASS='dataTitle'>Blocking Outages</DIV><TD WIDTH='33%%'>");

		/* add export to csv link */
		printf("<DIV style='padding-right:6px;' class='csv_export_link'>");
		print_export_link(CSV_CONTENT, OUTAGES_CGI, NULL);
		print_export_link(JSON_CONTENT, OUTAGES_CGI, NULL);
		print_export_link(HTML_CONTENT, OUTAGES_CGI, NULL);

		printf("</DIV></TD></TR></TABLE>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");
		printf("<TR>\n");
		printf("<TH CLASS='data'>Severity</TH><TH CLASS='data'>Host</TH><TH CLASS='data'>State</TH><TH CLASS='data'>Notes</TH><TH CLASS='data'>State Duration</TH><TH CLASS='data'># Hosts Affected</TH><TH CLASS='data'># Services Affected</TH><TH CLASS='data'>Actions</TH>\n");
		printf("</TR>\n");
	}

	for (temp_hostoutagesort = hostoutagesort_list; temp_hostoutagesort != NULL; temp_hostoutagesort = temp_hostoutagesort->next) {

		temp_hostoutage = temp_hostoutagesort->outage;
		if (temp_hostoutage == NULL)
			continue;

		/* skip hosts that are not blocking anyone */
		if (temp_hostoutage->affected_child_hosts <= 1)
			continue;

		temp_hoststatus = find_hoststatus(temp_hostoutage->hst->name);
		if (temp_hoststatus == NULL)
			continue;

		/* make	sure we only caught valid state types */
		if (temp_hoststatus->status != HOST_DOWN && temp_hoststatus->status != HOST_UNREACHABLE)
			continue;

		total_entries++;

		if (odd == 0) {
			odd = 1;
			bg_class = "dataOdd";
		} else {
			odd = 0;
			bg_class = "dataEven";
		}

		if (temp_hoststatus->status == HOST_UNREACHABLE)
			status = "UNREACHABLE";
		else if (temp_hoststatus->status == HOST_DOWN)
			status = "DOWN";

		if (content_type == JSON_CONTENT) {
			// always add a comma, except for the first line
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
			printf("{ \"severity\": %d, ", temp_hostoutage->severity);
			printf(" \"host_name\": \"%s\", ", json_encode(temp_hostoutage->hst->name));
			printf(" \"host_display_name\": \"%s\", ", (temp_hostoutage->hst->display_name != NULL) ? json_encode(temp_hostoutage->hst->display_name) : json_encode(temp_hostoutage->hst->name));
			printf(" \"state\": \"%s\", ", status);
		} else if (content_type == CSV_CONTENT) {
			printf("%s%d%s%s", csv_data_enclosure, temp_hostoutage->severity, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_hostoutage->hst->name, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, status, csv_data_enclosure, csv_delimiter);
		} else {
			printf("<TR CLASS='%s'>\n", bg_class);

			printf("<TD CLASS='%s'>%d</TD>\n", bg_class, temp_hostoutage->severity);
			printf("<TD CLASS='%s'><A HREF='%s?type=%d&host=%s'>%s</A></TD>\n", bg_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_hostoutage->hst->name), (temp_hostoutage->hst->display_name != NULL) ? temp_hostoutage->hst->display_name : temp_hostoutage->hst->name);
			printf("<TD CLASS='host%s'>%s</TD>\n", status, status);
		}

		total_comments = number_of_host_comments(temp_hostoutage->hst->name);
		if (content_type == JSON_CONTENT) {
			printf(" \"notes\": %d, ", total_comments);
		} else if (content_type == CSV_CONTENT) {
			printf("%s%d%s%s", csv_data_enclosure, total_comments, csv_data_enclosure, csv_delimiter);
		} else {
			if (total_comments > 0)
				print_comment_icon(temp_hostoutage->hst->name, NULL);
			else
				printf("<TD CLASS='%s'>N/A</TD>\n", bg_class);
		}


		current_time = time(NULL);
		if (temp_hoststatus->last_state_change == (time_t)0)
			t = current_time - program_start;
		else
			t = current_time - temp_hoststatus->last_state_change;
		get_time_breakdown((unsigned long)t, &days, &hours, &minutes, &seconds);
		snprintf(state_duration, sizeof(state_duration) - 1, "%2dd %2dh %2dm %2ds%s", days, hours, minutes, seconds, (temp_hoststatus->last_state_change == (time_t)0) ? "+" : "");
		state_duration[sizeof(state_duration)-1] = '\x0';

		if (content_type == JSON_CONTENT) {
			printf(" \"state_duration\": \"%s\", ", state_duration);
			printf(" \"hosts_affected\": %d, ", temp_hostoutage->affected_child_hosts);
			printf(" \"services_affected\": %d }\n", temp_hostoutage->affected_child_services);
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, state_duration, csv_data_enclosure, csv_delimiter);
			printf("%s%d%s%s", csv_data_enclosure, temp_hostoutage->affected_child_hosts, csv_data_enclosure, csv_delimiter);
			printf("%s%d%s\n", csv_data_enclosure, temp_hostoutage->affected_child_services, csv_data_enclosure);
		} else {
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, state_duration);
			printf("<TD CLASS='%s'>%d</TD>\n", bg_class, temp_hostoutage->affected_child_hosts);
			printf("<TD CLASS='%s'>%d</TD>\n", bg_class, temp_hostoutage->affected_child_services);

			printf("<TD CLASS='%s'>", bg_class);
			printf("<A HREF='%s?host=%s'><IMG SRC='%s%s' BORDER=0 ALT='View status detail for this host' TITLE='View status detail for this host'></A>\n", STATUS_CGI, url_encode(temp_hostoutage->hst->name), url_images_path, STATUS_DETAIL_ICON);
#ifdef USE_STATUSMAP
			printf("<A HREF='%s?host=%s'><IMG SRC='%s%s' BORDER=0 ALT='View status map for this host and its children' TITLE='View status map for this host and its children'></A>\n", STATUSMAP_CGI, url_encode(temp_hostoutage->hst->name), url_images_path, STATUSMAP_ICON);
#endif
#ifdef USE_TRENDS
			printf("<A HREF='%s?host=%s'><IMG SRC='%s%s' BORDER=0 ALT='View trends for this host' TITLE='View trends for this host'></A>\n", TRENDS_CGI, url_encode(temp_hostoutage->hst->name), url_images_path, TRENDS_ICON);
#endif
			printf("<A HREF='%s?host=%s'><IMG SRC='%s%s' BORDER=0 ALT='View alert history for this host' TITLE='View alert history for this host'></A>\n", HISTORY_CGI, url_encode(temp_hostoutage->hst->name), url_images_path, HISTORY_ICON);
			printf("<A HREF='%s?host=%s'><IMG SRC='%s%s' BORDER=0 ALT='View notifications for this host' TITLE='View notifications for this host'></A>\n", NOTIFICATIONS_CGI, url_encode(temp_hostoutage->hst->name), url_images_path, NOTIFICATION_ICON);

			/* add Link to acknowledge all affected hosts */
			printf("<a href='%s?cmd_typ=%d&host=%s", CMD_CGI, CMD_ACKNOWLEDGE_HOST_PROBLEM, url_encode(temp_hostoutage->hst->name));
			for (temp_affected_host = temp_hostoutage->affected_hosts; temp_affected_host != NULL; temp_affected_host = temp_affected_host->next)
				printf("&host=%s", url_encode(temp_affected_host->host_name));
			printf("'><img src='%s%s' border=0 ALT='Acknowledge All Affected Hosts' TITLE='Acknowledge All Affected Hosts'></a>\n", url_images_path, ACKNOWLEDGEMENT_ICON);

			printf("</TD>\n");

			printf("</TR>\n");
		}
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");

		if (total_entries == 0)
			printf("<DIV CLASS='itemTotalsTitle'>%d Blocking Outages Displayed</DIV>\n", total_entries);
	} else if (content_type == JSON_CONTENT) {
		printf("\n]\n");
	}

	/* free memory allocated to the host outage list */
	free_hostoutage_list();
	free_hostoutagesort_list();

	return;
}

/* determine what hosts are causing network outages */
void find_hosts_causing_outages(void) {
	hoststatus *temp_hoststatus;
	host *temp_host;

	/* check all hosts */
	for (temp_hoststatus = hoststatus_list; temp_hoststatus != NULL; temp_hoststatus = temp_hoststatus->next) {

		/* check only hosts that are not up and not pending */
		if (temp_hoststatus->status != HOST_UP && temp_hoststatus->status != HOST_PENDING) {

			/* find the host entry */
			temp_host = find_host(temp_hoststatus->host_name);

			if (temp_host == NULL)
				continue;

			if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
				continue;

			/* if the route to this host is not blocked, it is a causing an outage */
			if (is_route_to_host_blocked(temp_host) == FALSE)
				add_hostoutage(temp_host);
		}
	}

	return;
}

/* adds a host outage entry */
void add_hostoutage(host *hst) {
	hostoutage *new_hostoutage;

	/* allocate memory for a new structure */
	new_hostoutage = (hostoutage *)malloc(sizeof(hostoutage));

	if (new_hostoutage == NULL)
		return;

	new_hostoutage->hst = hst;
	new_hostoutage->severity = 0;
	new_hostoutage->affected_child_hosts = 0;
	new_hostoutage->affected_child_services = 0;
	new_hostoutage->affected_hosts = NULL;

	/* add the structure to the head of the list in memory */
	new_hostoutage->next = hostoutage_list;
	hostoutage_list = new_hostoutage;

	return;
}

/* frees all memory allocated to the host outage list */
void free_hostoutage_list(void) {
	hostoutage *this_hostoutage;
	hostoutage *next_hostoutage;
	affected_host *this_affected_host;
	affected_host *next_affected_host;

	/* free all list members */
	for (this_hostoutage = hostoutage_list; this_hostoutage != NULL; this_hostoutage = next_hostoutage) {
		next_hostoutage = this_hostoutage->next;

		/* free affected host list */
		for (this_affected_host = this_hostoutage->affected_hosts; this_affected_host != NULL; this_affected_host = next_affected_host) {
			next_affected_host = this_affected_host->next;
			my_free(this_affected_host->host_name);
			my_free(this_affected_host);
		}
		my_free(this_hostoutage);
	}

	/* reset list pointer */
	hostoutage_list = NULL;

	return;
}

/* frees all memory allocated to the host outage sort list */
void free_hostoutagesort_list(void) {
	hostoutagesort *this_hostoutagesort;
	hostoutagesort *next_hostoutagesort;

	/* free all list members */
	for (this_hostoutagesort = hostoutagesort_list; this_hostoutagesort != NULL; this_hostoutagesort = next_hostoutagesort) {
		next_hostoutagesort = this_hostoutagesort->next;
		my_free(this_hostoutagesort);
	}

	/* reset list pointer */
	hostoutagesort_list = NULL;

	return;
}

/* calculates network outage effect of all hosts that are causing blockages */
void calculate_outage_effects(void) {
	hostoutage *temp_hostoutage;

	/* check all hosts causing problems */
	for (temp_hostoutage = hostoutage_list; temp_hostoutage != NULL; temp_hostoutage = temp_hostoutage->next) {

		currently_checked_host = temp_hostoutage;

		/* calculate the outage effect of this particular hosts */
		calculate_outage_effect_of_host(temp_hostoutage->hst, &temp_hostoutage->affected_child_hosts, &temp_hostoutage->affected_child_services);

		temp_hostoutage->severity = (temp_hostoutage->affected_child_hosts + (temp_hostoutage->affected_child_services / service_severity_divisor));
	}

	return;
}

/* calculates network outage effect of a particular host being down or unreachable */
void calculate_outage_effect_of_host(host *hst, int *affected_hosts, int *affected_services) {
	int total_child_hosts_affected = 0;
	int total_child_services_affected = 0;
	int temp_child_hosts_affected = 0;
	int temp_child_services_affected = 0;
	host *temp_host;


	/* find all child hosts of this host */
	for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {

		/* skip this host if it is not a child */
		if (is_host_immediate_child_of_host(hst, temp_host) == FALSE)
			continue;

		/* calculate the outage effect of the child */
		calculate_outage_effect_of_host(temp_host, &temp_child_hosts_affected, &temp_child_services_affected);

		/* keep a running total of outage effects */
		total_child_hosts_affected += temp_child_hosts_affected;
		total_child_services_affected += temp_child_services_affected;

		add_affected_host(temp_host->name);
	}

	*affected_hosts = total_child_hosts_affected + 1;
	*affected_services = total_child_services_affected + number_of_host_services(hst);

	return;
}

/* tests whether or not a host is "blocked" by upstream parents (host is already assumed to be down or unreachable) */
int is_route_to_host_blocked(host *hst) {
	hostsmember *temp_hostsmember;
	hoststatus *temp_hoststatus;

	/* if the host has no parents, it is not being blocked by anyone */
	if (hst->parent_hosts == NULL)
		return FALSE;

	/* check all parent hosts */
	for (temp_hostsmember = hst->parent_hosts; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {

		/* find the parent host's status */
		temp_hoststatus = find_hoststatus(temp_hostsmember->host_name);

		if (temp_hoststatus == NULL)
			continue;

		/* at least one parent is up (or pending), so this host is not blocked */
		if (temp_hoststatus->status == HOST_UP || temp_hoststatus->status == HOST_PENDING)
			return FALSE;
	}

	return TRUE;
}

/* calculates the number of services associated a particular host */
int number_of_host_services(host *hst) {
	int total_services = 0;
	service *temp_service;

	/* check all services */
	for (temp_service = service_list; temp_service != NULL; temp_service = temp_service->next) {

		if (!strcmp(temp_service->host_name, hst->name))
			total_services++;
	}

	return total_services;
}

/* sort the host outages by severity */
void sort_hostoutages(void) {
	hostoutagesort *last_hostoutagesort;
	hostoutagesort *new_hostoutagesort;
	hostoutagesort *temp_hostoutagesort;
	hostoutage *temp_hostoutage;

	if (hostoutage_list == NULL)
		return;

	/* sort all host outage entries */
	for (temp_hostoutage = hostoutage_list; temp_hostoutage != NULL; temp_hostoutage = temp_hostoutage->next) {

		/* allocate memory for a new sort structure */
		new_hostoutagesort = (hostoutagesort *)malloc(sizeof(hostoutagesort));
		if (new_hostoutagesort == NULL)
			return;

		new_hostoutagesort->outage = temp_hostoutage;

		last_hostoutagesort = hostoutagesort_list;
		for (temp_hostoutagesort = hostoutagesort_list; temp_hostoutagesort != NULL; temp_hostoutagesort = temp_hostoutagesort->next) {

			if (new_hostoutagesort->outage->severity >= temp_hostoutagesort->outage->severity) {
				new_hostoutagesort->next = temp_hostoutagesort;
				if (temp_hostoutagesort == hostoutagesort_list)
					hostoutagesort_list = new_hostoutagesort;
				else
					last_hostoutagesort->next = new_hostoutagesort;
				break;
			} else
				last_hostoutagesort = temp_hostoutagesort;
		}

		if (hostoutagesort_list == NULL) {
			new_hostoutagesort->next = NULL;
			hostoutagesort_list = new_hostoutagesort;
		} else if (temp_hostoutagesort == NULL) {
			new_hostoutagesort->next = NULL;
			last_hostoutagesort->next = new_hostoutagesort;
		}
	}

	return;
}

void add_affected_host(char *host_name) {
	affected_host *new_affected_host;
	affected_host *temp_affected_host;

	if (currently_checked_host->affected_hosts!=NULL) {
		for (temp_affected_host = currently_checked_host->affected_hosts; temp_affected_host != NULL; temp_affected_host = temp_affected_host->next) {
			if (!strcmp(temp_affected_host->host_name, host_name))
				return;
		}
	}

	/* allocate memory for a new structure */
	new_affected_host = (affected_host *)malloc(sizeof(affected_host));

	if (new_affected_host == NULL)
		return;

	new_affected_host->host_name = strdup(host_name);

	/* add the structure to the head of the list in memory */
	new_affected_host->next = currently_checked_host->affected_hosts;
	currently_checked_host->affected_hosts = new_affected_host;

	return;
}
