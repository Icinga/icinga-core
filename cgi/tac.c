/***********************************************************************
 *
 * TAC.C - Icinga Tactical Monitoring Overview CGI
 *
 * Copyright (c) 2001-2008 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 * This CGI program will display the contents of the Icinga
 * log file.
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
 ***********************************************************************/

/** @file tac.c
 *  @brief display a tactical monitoring overview and the tac header 
**/

/* #define DEBUG 1*/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/statusdata.h"

#include "../include/getcgi.h"
#include "../include/cgiutils.h"
#include "../include/cgiauth.h"

/** @name external vars
    @{ **/
extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];
extern char url_media_path[MAX_FILENAME_LENGTH];

extern char *service_critical_sound;
extern char *service_warning_sound;
extern char *service_unknown_sound;
extern char *host_down_sound;
extern char *host_unreachable_sound;
extern char *normal_sound;

extern host *host_list;
extern hostgroup *hostgroup_list;
extern hoststatus *hoststatus_list;
extern servicestatus *servicestatus_list;

extern int enable_notifications;
extern int execute_service_checks;
extern int execute_host_checks;
extern int accept_passive_service_checks;
extern int accept_passive_host_checks;
extern int enable_event_handlers;
extern int enable_flap_detection;
extern int nagios_process_state;
extern int tac_show_only_hard_state;
extern int show_tac_header;
extern int show_tac_header_pending;
extern int embedded;
extern int refresh;
extern int refresh_type;
extern int display_header;
extern int daemon_check;
extern int tac_header;
extern int content_type;
/** @} */


/** @name TAC WARNING/CRITICAL PERCENTAGE
 @{**/
#define HEALTH_WARNING_PERCENTAGE	90			/**< health below this value is considered as in warning state */
#define HEALTH_CRITICAL_PERCENTAGE	75			/**< health below this value is considered as in critical state */
/** @} */


/** @brief host outage struct
 *
 * holds a list hosts causing network outages
**/
typedef struct hostoutage_struct {
	host *hst;						/**< pointer to host element which cuase outage */
	int  affected_child_hosts;				/**< number of affected hosts */
	struct hostoutage_struct *next;				/**< pointer to next host outage element */
} hostoutage;

hostoutage *hostoutage_list = NULL;				/**< list of all host outage elements */

authdata current_authdata;					/**< struct to hold current authentication data */
int CGI_ID = TAC_CGI_ID;					/**< ID to identify the cgi for functions in cgiutils.c */

/** @name outages counters
 @{**/
int total_blocking_outages = 0;
int total_nonblocking_outages = 0;
/** @} */

/** @name health counters
 @{**/
int total_service_health = 0;
int total_host_health = 0;
int potential_service_health = 0;
int potential_host_health = 0;
double percent_service_health = 0.0;
double percent_host_health = 0.0;
/** @} */

/** @name statistics counters
 @{**/
double min_service_execution_time = -1.0;
double max_service_execution_time = -1.0;
double total_service_execution_time = 0.0;
double average_service_execution_time = -1.0;
double min_host_execution_time = -1.0;
double max_host_execution_time = -1.0;
double total_host_execution_time = 0.0;
double average_host_execution_time = -1.0;
double min_service_latency = -1.0;
double max_service_latency = -1.0;
double total_service_latency = 0.0;
double average_service_latency = -1.0;
double min_host_latency = -1.0;
double max_host_latency = -1.0;
double total_host_latency = 0.0;
double average_host_latency = -1.0;
/** @} */

/** @name total counters
 @{**/
int total_hosts = 0;
int total_services = 0;
int total_active_host_checks = 0;
int total_passive_host_checks = 0;
int total_disabled_host_checks = 0;
int total_active_host_checks_with_passive_disabled = 0;
int total_active_service_checks = 0;
int total_passive_service_checks = 0;
int total_disabled_service_checks = 0;
int total_active_service_checks_with_passive_disabled = 0;
/** @} */

/** @name flapping and disabled counters
 @{**/
int flapping_services = 0;
int flapping_hosts = 0;
int flap_disabled_services = 0;
int flap_disabled_hosts = 0;
int notification_disabled_services = 0;
int notification_disabled_hosts = 0;
int event_handler_disabled_services = 0;
int event_handler_disabled_hosts = 0;
/** @} */

/** @name host status counters
 @{**/
int hosts_pending = 0;
int hosts_pending_active = 0;
int hosts_pending_passive = 0;
int hosts_pending_disabled = 0;

int hosts_up = 0;
int hosts_up_active = 0;
int hosts_up_passive = 0;
int hosts_up_disabled = 0;

int hosts_down = 0;
int hosts_down_active = 0;
int hosts_down_passive = 0;
int hosts_down_disabled = 0;

int hosts_down_scheduled = 0;
int hosts_down_active_scheduled = 0;
int hosts_down_passive_scheduled = 0;
int hosts_down_disabled_scheduled = 0;

int hosts_down_acknowledged = 0;
int hosts_down_active_acknowledged = 0;
int hosts_down_passive_acknowledged = 0;
int hosts_down_disabled_acknowledged = 0;

int hosts_down_unacknowledged = 0;
int hosts_down_active_unacknowledged = 0;
int hosts_down_passive_unacknowledged = 0;
int hosts_down_disabled_unacknowledged = 0;

int hosts_unreachable = 0;
int hosts_unreachable_active = 0;
int hosts_unreachable_passive = 0;
int hosts_unreachable_disabled = 0;

int hosts_unreachable_scheduled = 0;
int hosts_unreachable_active_scheduled = 0;
int hosts_unreachable_passive_scheduled = 0;
int hosts_unreachable_disabled_scheduled = 0;

int hosts_unreachable_acknowledged = 0;
int hosts_unreachable_active_acknowledged = 0;
int hosts_unreachable_passive_acknowledged = 0;
int hosts_unreachable_disabled_acknowledged = 0;

int hosts_unreachable_unacknowledged = 0;
int hosts_unreachable_active_unacknowledged = 0;
int hosts_unreachable_passive_unacknowledged = 0;
int hosts_unreachable_disabled_unacknowledged = 0;
/** @} */

/** @name service status counters
 @{**/
int services_pending = 0;
int services_pending_host_down = 0;
int services_pending_active = 0;
int services_pending_active_host_down = 0;
int services_pending_passive = 0;
int services_pending_passive_host_down = 0;
int services_pending_disabled = 0;
int services_pending_disabled_host_down = 0;

int services_ok = 0;
int services_ok_host_down = 0;
int services_ok_active = 0;
int services_ok_active_host_down = 0;
int services_ok_passive = 0;
int services_ok_passive_host_down = 0;
int services_ok_disabled = 0;
int services_ok_disabled_host_down = 0;

int services_warning = 0;
int services_warning_host_down = 0;
int services_warning_active = 0;
int services_warning_active_host_down = 0;
int services_warning_passive = 0;
int services_warning_passive_host_down = 0;
int services_warning_disabled = 0;
int services_warning_disabled_host_down = 0;

int services_warning_scheduled = 0;
int services_warning_scheduled_host_down = 0;
int services_warning_active_scheduled = 0;
int services_warning_active_scheduled_host_down = 0;
int services_warning_passive_scheduled = 0;
int services_warning_passive_scheduled_host_down = 0;
int services_warning_disabled_scheduled = 0;
int services_warning_disabled_scheduled_host_down = 0;

int services_warning_acknowledged = 0;
int services_warning_acknowledged_host_down = 0;
int services_warning_active_acknowledged = 0;
int services_warning_active_acknowledged_host_down = 0;
int services_warning_passive_acknowledged = 0;
int services_warning_passive_acknowledged_host_down = 0;
int services_warning_disabled_acknowledged = 0;
int services_warning_disabled_acknowledged_host_down = 0;

int services_warning_unacknowledged = 0;
int services_warning_unacknowledged_host_down = 0;
int services_warning_active_unacknowledged = 0;
int services_warning_active_unacknowledged_host_down = 0;
int services_warning_passive_unacknowledged = 0;
int services_warning_passive_unacknowledged_host_down = 0;
int services_warning_disabled_unacknowledged = 0;
int services_warning_disabled_unacknowledged_host_down = 0;

int services_unknown = 0;
int services_unknown_host_down = 0;
int services_unknown_active = 0;
int services_unknown_active_host_down = 0;
int services_unknown_passive = 0;
int services_unknown_passive_host_down = 0;
int services_unknown_disabled = 0;
int services_unknown_disabled_host_down = 0;

int services_unknown_scheduled = 0;
int services_unknown_scheduled_host_down = 0;
int services_unknown_active_scheduled = 0;
int services_unknown_active_scheduled_host_down = 0;
int services_unknown_passive_scheduled = 0;
int services_unknown_passive_scheduled_host_down = 0;
int services_unknown_disabled_scheduled = 0;
int services_unknown_disabled_scheduled_host_down = 0;

int services_unknown_acknowledged = 0;
int services_unknown_acknowledged_host_down = 0;
int services_unknown_active_acknowledged = 0;
int services_unknown_active_acknowledged_host_down = 0;
int services_unknown_passive_acknowledged = 0;
int services_unknown_passive_acknowledged_host_down = 0;
int services_unknown_disabled_acknowledged = 0;
int services_unknown_disabled_acknowledged_host_down = 0;

int services_unknown_unacknowledged = 0;
int services_unknown_unacknowledged_host_down = 0;
int services_unknown_active_unacknowledged = 0;
int services_unknown_active_unacknowledged_host_down = 0;
int services_unknown_passive_unacknowledged = 0;
int services_unknown_passive_unacknowledged_host_down = 0;
int services_unknown_disabled_unacknowledged = 0;
int services_unknown_disabled_unacknowledged_host_down = 0;

int services_critical = 0;
int services_critical_host_down = 0;
int services_critical_active = 0;
int services_critical_active_host_down = 0;
int services_critical_passive = 0;
int services_critical_passive_host_down = 0;
int services_critical_disabled = 0;
int services_critical_disabled_host_down = 0;

int services_critical_scheduled = 0;
int services_critical_scheduled_host_down = 0;
int services_critical_active_scheduled = 0;
int services_critical_active_scheduled_host_down = 0;
int services_critical_passive_scheduled = 0;
int services_critical_passive_scheduled_host_down = 0;
int services_critical_disabled_scheduled = 0;
int services_critical_disabled_scheduled_host_down = 0;

int services_critical_acknowledged = 0;
int services_critical_acknowledged_host_down = 0;
int services_critical_active_acknowledged = 0;
int services_critical_active_acknowledged_host_down = 0;
int services_critical_passive_acknowledged = 0;
int services_critical_passive_acknowledged_host_down = 0;
int services_critical_disabled_acknowledged = 0;
int services_critical_disabled_acknowledged_host_down = 0;

int services_critical_unacknowledged = 0;
int services_critical_unacknowledged_host_down = 0;
int services_critical_active_unacknowledged = 0;
int services_critical_active_unacknowledged_host_down = 0;
int services_critical_passive_unacknowledged = 0;
int services_critical_passive_unacknowledged_host_down = 0;
int services_critical_disabled_unacknowledged = 0;
int services_critical_disabled_unacknowledged_host_down = 0;
/** @} */


/** @brief Parses the requested GET/POST variables
 *  @retval TRUE
 *  @retval FALSE
 *  @return wether parsing was successful or not
 *
 *  @n This function parses the request and set's the necessary variables
**/
int process_cgivars(void);


/** @brief fills all the counters
 *
 *  Iterates through @ref servicestatus_list and @ref hoststatus_list to count all host and service states
**/
void analyze_status_data(void);

/** @brief displays all data
 *
 *  After we calculated all data we have to display it.
 *  This functions is also responsible for the "tac header".
**/
void display_tac_overview(void);

/** @brief find all hosts that are causing network outages
 *
 *  Function tries to find hosts causig outages and calculates how many child hosts are affected by that.
 *
 *  Loops through @ref hoststatus_list and looks for hosts which are DOWN or UNREACHABLE and haven't been
 *  blocked by other DOWN/UNREACHABLE hosts (@ref is_route_to_host_blocked).
 *  These hosts get added to @ref hostoutage_list via @ref add_hostoutage
 *
 *  Then it loops through @ref hostoutage_list and calls @ref calculate_outage_effect_of_host to find
 *  out how many hosts are affected by this outage.
**/
void find_hosts_causing_outages(void);

/** @brief calculates network outage effect of a particular host being down or unreachable
 *  @param [in] hst host element to calculate outage for
 *  @param [out] affected_hosts returns number of affected hosts
 *
 *  Finds immediate childs of "hst" and calls itself again to finaly find all child hosts and sums them up to affected_hosts.
 *  This is a recursice function.
**/
void calculate_outage_effect_of_host(host *, int *);

/** @brief tests whether or not a host is "blocked" by upstream parents (host is already assumed to be down or unreachable)
 *  @param [in] hst host element to see if this host is blocked
 *	@retval TRUE
 *	@retval FALSE
 *  @return wether host is completely blocked or not
 *
 *  Loops through all parent hosts of "hst" and tries to find a parent in state UP or PENDING. If it finds a parent in UP or
 *  PENDING state the function returns FALSE otherwise TRUE
**/
int is_route_to_host_blocked(host *);

/** @brief adds a host to @ref hostoutage_list
 *  @param [in] hst host element to add to hostoutage_list
**/
void add_hostoutage(host *);

/** @brief frees all memory allocated to @ref hostoutage_list entries in memory **/
void free_hostoutage_list(void);

/** @brief Yes we need a main function **/
int main(void) {
	int result = OK;
	char *sound = NULL;
#ifdef DEBUG
	time_t t1, t2, t3, t4, t5, t6, t7, t8, t9;
#endif


#ifdef DEBUG
	time(&t1);
#endif

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

#ifdef DEBUG
	time(&t2);
#endif

	/* read the main configuration file */
	result = read_main_config_file(main_config_file);
	if (result == ERROR) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(main_config_file, ERROR_CGI_MAIN_CFG);
		document_footer(CGI_ID);
		return ERROR;
	}

#ifdef DEBUG
	time(&t3);
#endif

	/* read all object configuration data */
	result = read_all_object_configuration_data(main_config_file, READ_ALL_OBJECT_DATA);
	if (result == ERROR) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(NULL, ERROR_CGI_OBJECT_DATA);
		document_footer(CGI_ID);
		return ERROR;
	}

#ifdef DEBUG
	time(&t4);
#endif

	/* read all status data */
	result = read_all_status_data(get_cgi_config_location(), READ_ALL_STATUS_DATA);
	if (result == ERROR && daemon_check == TRUE) {
		document_header(CGI_ID, FALSE, "Error");
		print_error(NULL, ERROR_CGI_STATUS_DATA);
		document_footer(CGI_ID);
		free_memory();
		return ERROR;
	}

#ifdef DEBUG
	time(&t5);
#endif

	if (tac_header == TRUE)
		document_header(CGI_ID, TRUE, "Icinga");
	else
		document_header(CGI_ID, TRUE, "Tactical Monitoring Overview");

	/* get authentication information */
	get_authentication_information(&current_authdata);

#ifdef DEBUG
	time(&t6);
#endif

	/* analyze current host and service status data for tac overview */
	analyze_status_data();

#ifdef DEBUG
	time(&t7);
#endif

	/* find all hosts that are causing network outages */
	find_hosts_causing_outages();


#ifdef DEBUG
	time(&t8);
#endif

	/* embed sound tag if necessary... */
	if (hosts_unreachable_unacknowledged > 0 && host_unreachable_sound != NULL)
		sound = host_unreachable_sound;
	else if (hosts_down_unacknowledged > 0 && host_down_sound != NULL)
		sound = host_down_sound;
	else if (services_critical_unacknowledged > 0 && service_critical_sound != NULL)
		sound = service_critical_sound;
	else if (services_warning_unacknowledged > 0 && service_warning_sound != NULL)
		sound = service_warning_sound;
	else if (services_unknown_unacknowledged == 0 && services_warning_unacknowledged == 0 && services_critical_unacknowledged == 0 && hosts_down_unacknowledged == 0 && hosts_unreachable_unacknowledged == 0 && normal_sound != NULL)
		sound = normal_sound;
	if (sound != NULL && content_type != JSON_CONTENT) {
		printf("<object type=\"audio/x-wav\" data=\"%s%s\" height=\"-\" width=\"0\">", url_media_path, sound);
		printf("<param name=\"filename\" value=\"%s%s\">", url_media_path, sound);
		printf("<param name=\"autostart\" value=\"true\">");
		printf("<param name=\"playcount\" value=\"1\">");
		printf("</object>");
	}


	/**** display main tac screen ****/
	display_tac_overview();

#ifdef DEBUG
	time(&t9);
#endif

	document_footer(CGI_ID);

	/* free memory allocated to the host outage list */
	free_hostoutage_list();

	/* free allocated memory */
	free_memory();

#ifdef DEBUG
	printf("T1: %lu\n", (unsigned long)t1);
	printf("T2: %lu\n", (unsigned long)t2);
	printf("T3: %lu\n", (unsigned long)t3);
	printf("T4: %lu\n", (unsigned long)t4);
	printf("T5: %lu\n", (unsigned long)t5);
	printf("T6: %lu\n", (unsigned long)t6);
	printf("T7: %lu\n", (unsigned long)t7);
	printf("T8: %lu\n", (unsigned long)t8);
	printf("T9: %lu\n", (unsigned long)t9);
#endif

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

		/* we found the tac_header option */
		else if (!strcmp(variables[x], "tac_header"))
			tac_header = TRUE;

		/* we found the JSON output option */
		else if (!strcmp(variables[x], "jsonoutput")) {
			display_header = FALSE;
			content_type = JSON_CONTENT;
		}

		/* we received an invalid argument */
		else
			error = TRUE;

	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

void analyze_status_data(void) {
	servicestatus *temp_servicestatus;
	service *temp_service;
	hoststatus *temp_hoststatus;
	host *temp_host;
	char *last_host_name = NULL;
	int host_is_down = FALSE;

	/* check all services */
	for (temp_servicestatus = servicestatus_list; temp_servicestatus != NULL; temp_servicestatus = temp_servicestatus->next) {

		/* see if user is authorized to view this service */
		temp_service = find_service(temp_servicestatus->host_name, temp_servicestatus->description);
		if (is_authorized_for_service(temp_service, &current_authdata) == FALSE)
			continue;

		/* check if only hard states to be shown */
		if (tac_show_only_hard_state == TRUE && temp_servicestatus->state_type != HARD_STATE)
			continue;

		/* get hoststatus only once for each host */
		if (last_host_name == NULL || strcmp(last_host_name, temp_servicestatus->host_name)) {
			last_host_name = temp_servicestatus->host_name;

			// find host status
			temp_hoststatus = find_hoststatus(temp_servicestatus->host_name);

			host_is_down = FALSE;
			if (temp_hoststatus != NULL && (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE))
				host_is_down = TRUE;
		}


		/******** CHECK FEATURES *******/

		/* check flapping */
		if (temp_servicestatus->flap_detection_enabled == FALSE)
			flap_disabled_services++;
		else if (temp_servicestatus->is_flapping == TRUE)
			flapping_services++;

		/* check notifications */
		if (temp_servicestatus->notifications_enabled == FALSE)
			notification_disabled_services++;

		/* check event handler */
		if (temp_servicestatus->event_handler_enabled == FALSE)
			event_handler_disabled_services++;


		/********* CHECK STATUS ********/

		if (temp_servicestatus->status == SERVICE_OK) {
			if (temp_servicestatus->checks_enabled == TRUE)
				(host_is_down == FALSE) ? services_ok_active++ : services_ok_active_host_down++;

			else if (temp_servicestatus->accept_passive_service_checks == TRUE)
				(host_is_down == FALSE) ? services_ok_passive++ : services_ok_passive_host_down++;
			else
				(host_is_down == FALSE) ? services_ok_disabled++ : services_ok_disabled_host_down++;

			(host_is_down == FALSE) ? services_ok++ : services_ok_host_down++;
		} else if (temp_servicestatus->status == SERVICE_WARNING) {
			if (temp_servicestatus->scheduled_downtime_depth > 0) {
				if (temp_servicestatus->checks_enabled == TRUE)
					(host_is_down == FALSE) ? services_warning_active_scheduled++ : services_warning_active_scheduled_host_down++;

				else if (temp_servicestatus->accept_passive_service_checks == TRUE)
					(host_is_down == FALSE) ? services_warning_passive_scheduled++ : services_warning_passive_scheduled_host_down++;
				else
					(host_is_down == FALSE) ? services_warning_disabled_scheduled++ : services_warning_disabled_scheduled_host_down++;

				(host_is_down == FALSE) ? services_warning_scheduled++ : services_warning_scheduled_host_down++;
			} else if (temp_servicestatus->problem_has_been_acknowledged == TRUE) {
				if (temp_servicestatus->checks_enabled == TRUE)
					(host_is_down == FALSE) ? services_warning_active_acknowledged++ : services_warning_active_acknowledged_host_down++;

				else if (temp_servicestatus->accept_passive_service_checks == TRUE)
					(host_is_down == FALSE) ? services_warning_passive_acknowledged++ : services_warning_passive_acknowledged_host_down++;
				else
					(host_is_down == FALSE) ? services_warning_disabled_acknowledged++ : services_warning_disabled_acknowledged_host_down++;

				(host_is_down == FALSE) ? services_warning_acknowledged++ : services_warning_acknowledged_host_down++;
			} else {
				if (temp_servicestatus->checks_enabled == TRUE)
					(host_is_down == FALSE) ? services_warning_active_unacknowledged++ : services_warning_active_unacknowledged_host_down++;

				else if (temp_servicestatus->accept_passive_service_checks == TRUE)
					(host_is_down == FALSE) ? services_warning_passive_unacknowledged++ : services_warning_passive_unacknowledged_host_down++;
				else
					(host_is_down == FALSE) ? services_warning_disabled_unacknowledged++ : services_warning_disabled_unacknowledged_host_down++;

				(host_is_down == FALSE) ? services_warning_unacknowledged++ : services_warning_unacknowledged_host_down++;
			}

			if (temp_servicestatus->checks_enabled == TRUE)
				(host_is_down == FALSE) ? services_warning_active++ : services_warning_active_host_down++;

			else if (temp_servicestatus->accept_passive_service_checks == TRUE)
				(host_is_down == FALSE) ? services_warning_passive++ : services_warning_passive_host_down++;
			else
				(host_is_down == FALSE) ? services_warning_disabled++ : services_warning_disabled_host_down++;

			(host_is_down == FALSE) ? services_warning++ : services_warning_host_down++;
		}

		else if (temp_servicestatus->status == SERVICE_UNKNOWN) {
			if (temp_servicestatus->scheduled_downtime_depth > 0) {
				if (temp_servicestatus->checks_enabled == TRUE)
					(host_is_down == FALSE) ? services_unknown_active_scheduled++ : services_unknown_active_scheduled_host_down++;

				else if (temp_servicestatus->accept_passive_service_checks == TRUE)
					(host_is_down == FALSE) ? services_unknown_passive_scheduled++ : services_unknown_passive_scheduled_host_down++;
				else
					(host_is_down == FALSE) ? services_unknown_disabled_scheduled++ : services_unknown_disabled_scheduled_host_down++;

				(host_is_down == FALSE) ? services_unknown_scheduled++ : services_unknown_scheduled_host_down++;
			} else if (temp_servicestatus->problem_has_been_acknowledged == TRUE) {
				if (temp_servicestatus->checks_enabled == TRUE)
					(host_is_down == FALSE) ? services_unknown_active_acknowledged++ : services_unknown_active_acknowledged_host_down++;

				else if (temp_servicestatus->accept_passive_service_checks == TRUE)
					(host_is_down == FALSE) ? services_unknown_passive_acknowledged++ : services_unknown_passive_acknowledged_host_down++;
				else
					(host_is_down == FALSE) ? services_unknown_disabled_acknowledged++ : services_unknown_disabled_acknowledged_host_down++;

				(host_is_down == FALSE) ? services_unknown_acknowledged++ : services_unknown_acknowledged_host_down++;
			} else {
				if (temp_servicestatus->checks_enabled == TRUE)
					(host_is_down == FALSE) ? services_unknown_active_unacknowledged++ : services_unknown_active_unacknowledged_host_down++;

				else if (temp_servicestatus->accept_passive_service_checks == TRUE)
					(host_is_down == FALSE) ? services_unknown_passive_unacknowledged++ : services_unknown_passive_unacknowledged_host_down++;
				else
					(host_is_down == FALSE) ? services_unknown_disabled_unacknowledged++ : services_unknown_disabled_unacknowledged_host_down++;

				(host_is_down == FALSE) ? services_unknown_unacknowledged++ : services_unknown_unacknowledged_host_down++;
			}

			if (temp_servicestatus->checks_enabled == TRUE)
				(host_is_down == FALSE) ? services_unknown_active++ : services_unknown_active_host_down++;

			else if (temp_servicestatus->accept_passive_service_checks == TRUE)
				(host_is_down == FALSE) ? services_unknown_passive++ : services_unknown_passive_host_down++;
			else
				(host_is_down == FALSE) ? services_unknown_disabled++ : services_unknown_disabled_host_down++;

			(host_is_down == FALSE) ? services_unknown++ : services_unknown_host_down++;
		}

		else if (temp_servicestatus->status == SERVICE_CRITICAL) {
			if (temp_servicestatus->scheduled_downtime_depth > 0) {
				if (temp_servicestatus->checks_enabled == TRUE)
					(host_is_down == FALSE) ? services_critical_active_scheduled++ : services_critical_active_scheduled_host_down++;

				else if (temp_servicestatus->accept_passive_service_checks == TRUE)
					(host_is_down == FALSE) ? services_critical_passive_scheduled++ : services_critical_passive_scheduled_host_down++;
				else
					(host_is_down == FALSE) ? services_critical_disabled_scheduled++ : services_critical_disabled_scheduled_host_down++;

				(host_is_down == FALSE) ? services_critical_scheduled++ : services_critical_scheduled_host_down++;
			} else if (temp_servicestatus->problem_has_been_acknowledged == TRUE) {
				if (temp_servicestatus->checks_enabled == TRUE)
					(host_is_down == FALSE) ? services_critical_active_acknowledged++ : services_critical_active_acknowledged_host_down++;

				else if (temp_servicestatus->accept_passive_service_checks == TRUE)
					(host_is_down == FALSE) ? services_critical_passive_acknowledged++ : services_critical_passive_acknowledged_host_down++;
				else
					(host_is_down == FALSE) ? services_critical_disabled_acknowledged++ : services_critical_disabled_acknowledged_host_down++;

				(host_is_down == FALSE) ? services_critical_acknowledged++ : services_critical_acknowledged_host_down++;
			} else {
				if (temp_servicestatus->checks_enabled == TRUE)
					(host_is_down == FALSE) ? services_critical_active_unacknowledged++ : services_critical_active_unacknowledged_host_down++;

				else if (temp_servicestatus->accept_passive_service_checks == TRUE)
					(host_is_down == FALSE) ? services_critical_passive_unacknowledged++ : services_critical_passive_unacknowledged_host_down++;
				else
					(host_is_down == FALSE) ? services_critical_disabled_unacknowledged++ : services_critical_disabled_unacknowledged_host_down++;

				(host_is_down == FALSE) ? services_critical_unacknowledged++ : services_critical_unacknowledged_host_down++;
			}

			if (temp_servicestatus->checks_enabled == TRUE)
				(host_is_down == FALSE) ? services_critical_active++ : services_critical_active_host_down++;

			else if (temp_servicestatus->accept_passive_service_checks == TRUE)
				(host_is_down == FALSE) ? services_critical_passive++ : services_critical_passive_host_down++;
			else
				(host_is_down == FALSE) ? services_critical_disabled++ : services_critical_disabled_host_down++;

			(host_is_down == FALSE) ? services_critical++ : services_critical_host_down++;
		}

		else if (temp_servicestatus->status == SERVICE_PENDING) {
			if (temp_servicestatus->checks_enabled == TRUE)
				(host_is_down == FALSE) ? services_pending_active++ : services_pending_active_host_down++;

			else if (temp_servicestatus->accept_passive_service_checks == TRUE)
				(host_is_down == FALSE) ? services_pending_passive++ : services_pending_passive_host_down++;
			else
				(host_is_down == FALSE) ? services_pending_disabled++ : services_pending_disabled_host_down++;

			(host_is_down == FALSE) ? services_pending++ : services_pending_host_down++;
		}


		/* get health stats */
		if (temp_servicestatus->status == SERVICE_OK)
			total_service_health += 2;

		else if (temp_servicestatus->status == SERVICE_WARNING || temp_servicestatus->status == SERVICE_UNKNOWN)
			total_service_health++;

		if (temp_servicestatus->status != SERVICE_PENDING)
			potential_service_health += 2;


		/* calculate execution time and latency stats */
		if (temp_servicestatus->checks_enabled == TRUE) {

			total_active_service_checks++;

			if (temp_servicestatus->accept_passive_service_checks == FALSE)
				total_active_service_checks_with_passive_disabled++;

			if (min_service_latency == -1.0 || temp_servicestatus->latency < min_service_latency)
				min_service_latency = temp_servicestatus->latency;
			if (max_service_latency == -1.0 || temp_servicestatus->latency > max_service_latency)
				max_service_latency = temp_servicestatus->latency;

			if (min_service_execution_time == -1.0 || temp_servicestatus->execution_time < min_service_execution_time)
				min_service_execution_time = temp_servicestatus->execution_time;
			if (max_service_execution_time == -1.0 || temp_servicestatus->execution_time > max_service_execution_time)
				max_service_execution_time = temp_servicestatus->execution_time;

			total_service_latency += temp_servicestatus->latency;
			total_service_execution_time += temp_servicestatus->execution_time;

		} else if (temp_servicestatus->accept_passive_service_checks == TRUE)
			total_passive_service_checks++;
		else
			total_disabled_service_checks++;

		total_services++;
	}


	/* check all hosts */
	for (temp_hoststatus = hoststatus_list; temp_hoststatus != NULL; temp_hoststatus = temp_hoststatus->next) {

		/* see if user is authorized to view this host */
		temp_host = find_host(temp_hoststatus->host_name);
		if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		/* check if only hard states to be shown */
		if (tac_show_only_hard_state == TRUE && temp_hoststatus->state_type != HARD_STATE)
			continue;


		/******** CHECK FEATURES *******/

		/* check flapping */
		if (temp_hoststatus->flap_detection_enabled == FALSE)
			flap_disabled_hosts++;
		else if (temp_hoststatus->is_flapping == TRUE)
			flapping_hosts++;

		/* check notifications */
		if (temp_hoststatus->notifications_enabled == FALSE)
			notification_disabled_hosts++;

		/* check event handler */
		if (temp_hoststatus->event_handler_enabled == FALSE)
			event_handler_disabled_hosts++;


		/********* CHECK STATUS ********/

		if (temp_hoststatus->status == HOST_UP) {
			if (temp_hoststatus->checks_enabled == TRUE)
				hosts_up_active++;
			else if (temp_hoststatus->accept_passive_host_checks == TRUE)
				hosts_up_passive++;
			else
				hosts_up_disabled++;

			hosts_up++;
		}

		else if (temp_hoststatus->status == HOST_DOWN) {
			if (temp_hoststatus->scheduled_downtime_depth > 0) {
				if (temp_hoststatus->checks_enabled == TRUE)
					hosts_down_active_scheduled++;
				else if (temp_hoststatus->accept_passive_host_checks == TRUE)
					hosts_down_passive_scheduled++;
				else
					hosts_down_disabled_scheduled++;

				hosts_down_scheduled++;

			} else if (temp_hoststatus->problem_has_been_acknowledged == TRUE) {
				if (temp_hoststatus->checks_enabled == TRUE)
					hosts_down_active_acknowledged++;
				else if (temp_hoststatus->accept_passive_host_checks == TRUE)
					hosts_down_passive_acknowledged++;
				else
					hosts_down_disabled_acknowledged++;

				hosts_down_acknowledged++;
			} else {
				if (temp_hoststatus->checks_enabled == TRUE)
					hosts_down_active_unacknowledged++;
				else if (temp_hoststatus->accept_passive_host_checks == TRUE)
					hosts_down_passive_unacknowledged++;
				else
					hosts_down_disabled_unacknowledged++;

				hosts_down_unacknowledged++;
			}

			if (temp_hoststatus->checks_enabled == TRUE)
				hosts_down_active++;
			else if (temp_hoststatus->accept_passive_host_checks == TRUE)
				hosts_down_passive++;
			else
				hosts_down_disabled++;

			hosts_down++;
		}

		else if (temp_hoststatus->status == HOST_UNREACHABLE) {
			if (temp_hoststatus->scheduled_downtime_depth > 0) {
				if (temp_hoststatus->checks_enabled == TRUE)
					hosts_unreachable_active_scheduled++;
				else if (temp_hoststatus->accept_passive_host_checks == TRUE)
					hosts_unreachable_passive_scheduled++;
				else
					hosts_unreachable_disabled_scheduled++;

				hosts_unreachable_scheduled++;

			} else if (temp_hoststatus->problem_has_been_acknowledged == TRUE) {
				if (temp_hoststatus->checks_enabled == TRUE)
					hosts_unreachable_active_acknowledged++;
				else if (temp_hoststatus->accept_passive_host_checks == TRUE)
					hosts_unreachable_passive_acknowledged++;
				else
					hosts_unreachable_disabled_acknowledged++;

				hosts_unreachable_acknowledged++;
			} else {
				if (temp_hoststatus->checks_enabled == TRUE)
					hosts_unreachable_active_unacknowledged++;
				else if (temp_hoststatus->accept_passive_host_checks == TRUE)
					hosts_unreachable_passive_unacknowledged++;
				else
					hosts_unreachable_disabled_unacknowledged++;

				hosts_unreachable_unacknowledged++;
			}

			if (temp_hoststatus->checks_enabled == TRUE)
				hosts_unreachable_active++;
			else if (temp_hoststatus->accept_passive_host_checks == TRUE)
				hosts_unreachable_passive++;
			else
				hosts_unreachable_disabled++;

			hosts_unreachable++;
		}

		else if (temp_hoststatus->status == HOST_PENDING) {
			if (temp_hoststatus->checks_enabled == TRUE)
				hosts_pending_active++;
			else if (temp_hoststatus->accept_passive_host_checks == TRUE)
				hosts_pending_passive++;
			else
				hosts_pending_disabled++;

			hosts_pending++;
		}

		/* get health stats */
		if (temp_hoststatus->status == HOST_UP)
			total_host_health++;

		if (temp_hoststatus->status != HOST_PENDING)
			potential_host_health++;

		/* check type stats */
		if (temp_hoststatus->checks_enabled == TRUE) {

			total_active_host_checks++;

			if (temp_hoststatus->accept_passive_host_checks == FALSE)
				total_active_host_checks_with_passive_disabled++;

			if (min_host_latency == -1.0 || temp_hoststatus->latency < min_host_latency)
				min_host_latency = temp_hoststatus->latency;
			if (max_host_latency == -1.0 || temp_hoststatus->latency > max_host_latency)
				max_host_latency = temp_hoststatus->latency;

			if (min_host_execution_time == -1.0 || temp_hoststatus->execution_time < min_host_execution_time)
				min_host_execution_time = temp_hoststatus->execution_time;
			if (max_host_execution_time == -1.0 || temp_hoststatus->execution_time > max_host_execution_time)
				max_host_execution_time = temp_hoststatus->execution_time;

			total_host_latency += temp_hoststatus->latency;
			total_host_execution_time += temp_hoststatus->execution_time;

		} else if (temp_hoststatus->accept_passive_host_checks == TRUE)
			total_passive_host_checks++;
		else
			total_disabled_host_checks++;

		total_hosts++;
	}


	/* calculate service health */
	if (potential_service_health == 0)
		percent_service_health = 0.0;
	else
		percent_service_health = ((double)total_service_health / (double)potential_service_health) * 100.0;

	/* calculate host health */
	if (potential_host_health == 0)
		percent_host_health = 0.0;
	else
		percent_host_health = ((double)total_host_health / (double)potential_host_health) * 100.0;

	/* calculate service latency */
	if (total_service_latency == 0L)
		average_service_latency = 0.0;
	else
		average_service_latency = ((double)total_service_latency / (double)total_active_service_checks);

	/* calculate host latency */
	if (total_host_latency == 0L)
		average_host_latency = 0.0;
	else
		average_host_latency = ((double)total_host_latency / (double)total_active_host_checks);

	/* calculate service execution time */
	if (total_service_execution_time == 0.0)
		average_service_execution_time = 0.0;
	else
		average_service_execution_time = ((double)total_service_execution_time / (double)total_active_service_checks);

	/* calculate host execution time */
	if (total_host_execution_time == 0.0)
		average_host_execution_time = 0.0;
	else
		average_host_execution_time = ((double)total_host_execution_time / (double)total_active_host_checks);

	return;
}

/* determine what hosts are causing network outages */
void find_hosts_causing_outages(void) {
	hoststatus *temp_hoststatus;
	hostoutage *temp_hostoutage;
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


	/* check all hosts that are causing problems and calculate the extent of the problem */
	for (temp_hostoutage = hostoutage_list; temp_hostoutage != NULL; temp_hostoutage = temp_hostoutage->next) {

		/* calculate the outage effect of this particular hosts */
		calculate_outage_effect_of_host(temp_hostoutage->hst, &temp_hostoutage->affected_child_hosts);

		if (temp_hostoutage->affected_child_hosts > 1)
			total_blocking_outages++;
		else
			total_nonblocking_outages++;
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
	new_hostoutage->affected_child_hosts = 0;

	/* add the structure to the head of the list in memory */
	new_hostoutage->next = hostoutage_list;
	hostoutage_list = new_hostoutage;

	return;
}

/* frees all memory allocated to the host outage list */
void free_hostoutage_list(void) {
	hostoutage *this_hostoutage;
	hostoutage *next_hostoutage;

	for (this_hostoutage = hostoutage_list; this_hostoutage != NULL; this_hostoutage = next_hostoutage) {
		next_hostoutage = this_hostoutage->next;
		free(this_hostoutage);
	}

	return;
}

/* calculates network outage effect of a particular host being down or unreachable */
void calculate_outage_effect_of_host(host *hst, int *affected_hosts) {
	int total_child_hosts_affected = 0;
	int temp_child_hosts_affected = 0;
	host *temp_host;

	/* find all child hosts of this host */
	for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {

		/* skip this host if it is not a child */
		if (is_host_immediate_child_of_host(hst, temp_host) == FALSE)
			continue;

		/* calculate the outage effect of the child */
		calculate_outage_effect_of_host(temp_host, &temp_child_hosts_affected);

		/* keep a running total of outage effects */
		total_child_hosts_affected += temp_child_hosts_affected;
	}

	*affected_hosts = total_child_hosts_affected + 1;

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

		/* at least one parent it up (or pending), so this host is not blocked */
		if (temp_hoststatus->status == HOST_UP || temp_hoststatus->status == HOST_PENDING)
			return FALSE;
	}

	return TRUE;
}

void display_tac_overview(void) {
	char host_health_image[16];
	char service_health_image[16];
	char *tacheader_color = NULL;
	int handled_count = 0;
	int problem_found = FALSE;

	if (tac_header == TRUE && show_tac_header == FALSE) { // we want the top header, but not the tac version

		printf("	<div class='tac_banner' align='center'><img src='%s%s' alt='%s' /></div>", url_images_path, TAC_HEADER_DEFAULT_LOGO, TAC_HEADER_DEFAULT_LOGO_ALT);
		return; //we're done here

	} else if (tac_header == TRUE && show_tac_header == TRUE) { // we want the tac header

		printf("<table width='100%%' border='0'>\n");

		printf("<tr>\n");
		printf("<td width='auto'><table border='0'>\n");

		printf("<tr>\n");
		printf("<td nowrap='nowrap'><img src='%s%s' alt='Hosts' width='16' height='16' align='right' /></td>\n", url_images_path, TAC_HEADER_HOST_ICON);
		printf("<td><table width='95%%' border='0'>\n");

		/* 1. Row Hosts */
		printf("<tr>\n");

		/* Hosts UP */
		printf("<td nowrap>\n");
		printf("<div class='tacheader-overall-status-item'>\n");
		printf("<div class='tacheader-status %s'>", (hosts_up > 0) ? "tacheader-status-up color" : "gray");
		if (hosts_up_disabled > 0) {
			printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s UP'> %d </a>/\n", STATUS_CGI, HOST_UP, HOST_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_HOST_NOT_DISABLED, hosts_up_active + hosts_up_passive);
			printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s UP'> %d </a>\n", STATUS_CGI, HOST_UP, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED, TAC_TITLE_HOST_DISABLED, hosts_up_disabled);
			printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d' title='%s UP'> UP </a></div>\n", STATUS_CGI, HOST_UP, TAC_TITLE_HOST_ALL);
		} else
			printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d' title='%s UP'> %d UP </a></div>\n", STATUS_CGI, HOST_UP, TAC_TITLE_HOST_ALL, hosts_up);
		printf("</div>\n");
		printf("</td>\n");

		/* Hosts DOWN */
		printf("<td nowrap>\n");
		printf("<div class='tacheader-overall-status-item'>\n");

		handled_count = hosts_down_active_scheduled + hosts_down_passive_scheduled + hosts_down_disabled_scheduled + hosts_down_disabled_acknowledged + hosts_down_disabled_unacknowledged;

		if (hosts_down_active_unacknowledged + hosts_down_passive_unacknowledged > 0)
			tacheader_color = "tacheader-status-down color";
		else if (hosts_down_active_acknowledged + hosts_down_passive_acknowledged > 0)
			tacheader_color = "tacheader-status-down-acknowledged color";
		else if (handled_count > 0)
			tacheader_color = "tacheader-status-down-handled color";
		else
			tacheader_color = "gray";

		printf("<div class='tacheader-status %s'>", tacheader_color);
		printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s DOWN'> %d </a>/", STATUS_CGI, HOST_DOWN, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_HOST_UNACK_HOSTS, hosts_down_active_unacknowledged + hosts_down_passive_unacknowledged);
		printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s DOWN'> %d </a>/", STATUS_CGI, HOST_DOWN, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_ACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_HOST_ACK_HOSTS, hosts_down_active_acknowledged + hosts_down_passive_acknowledged);
		printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s DOWN'> %d </a>", STATUS_CGI, HOST_DOWN, HOST_STATE_HANDLED, TAC_TITLE_HOST_NON_URGENT, handled_count);
		printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d' title='%s DOWN'>DOWN</a>&nbsp;</div>\n", STATUS_CGI, HOST_DOWN, TAC_TITLE_HOST_ALL);
		printf("</div>\n");
		printf("</td>\n");

		/* Hosts UNREACHABLE */
		printf("<td nowrap>\n");
		printf("<div class='tacheader-overall-status-item'>\n");

		handled_count = hosts_unreachable_active_scheduled + hosts_unreachable_passive_scheduled + hosts_unreachable_disabled_scheduled + hosts_unreachable_disabled_acknowledged + hosts_unreachable_disabled_unacknowledged;

		if (hosts_unreachable_active_unacknowledged + hosts_unreachable_passive_unacknowledged > 0)
			tacheader_color = "tacheader-status-unreachable color";
		else if (hosts_unreachable_active_acknowledged + hosts_unreachable_passive_acknowledged > 0)
			tacheader_color = "tacheader-status-unreachable-acknowledged color";
		else if (handled_count > 0)
			tacheader_color = "tacheader-status-unreachable-handled color";
		else
			tacheader_color = "gray";

		printf("<div class='tacheader-status %s'>", tacheader_color);
		printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s UNREACHABLE'> %d </a>/", STATUS_CGI, HOST_UNREACHABLE, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_HOST_UNACK_HOSTS, hosts_unreachable_active_unacknowledged + hosts_unreachable_passive_unacknowledged);
		printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s UNREACHABLE'> %d </a>/", STATUS_CGI, HOST_UNREACHABLE, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_ACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_HOST_ACK_HOSTS, hosts_unreachable_active_acknowledged + hosts_unreachable_passive_acknowledged);
		printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s UNREACHABLE'> %d </a>", STATUS_CGI, HOST_UNREACHABLE, HOST_STATE_HANDLED, TAC_TITLE_HOST_NON_URGENT, handled_count);
		printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d' title='%s UNREACHABLE'>UNREACHABLE</a>&nbsp;</div>\n", STATUS_CGI, HOST_UNREACHABLE, TAC_TITLE_HOST_ALL);
		printf("</div>\n");
		printf("</td>\n");

		/* Hosts PENDING */
		if (show_tac_header_pending == TRUE) {
			printf("<td nowrap>\n");
			printf("<div class='tacheader-overall-status-item'>\n");
			printf("<div class='tacheader-status %s'>", (hosts_pending > 0) ? "tacheader-status-pending color" : "gray");
			if (hosts_pending_disabled > 0) {
				printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s PENDING'> %d </a>/\n", STATUS_CGI, HOST_PENDING, HOST_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_HOST_NOT_DISABLED, hosts_pending_active + hosts_pending_passive);
				printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d' title='%s PENDING'> %d </a>\n", STATUS_CGI, HOST_PENDING, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED, TAC_TITLE_HOST_DISABLED, hosts_pending_disabled);
				printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d' title='%s PENDING'> PENDING </a></div>\n", STATUS_CGI, HOST_PENDING, TAC_TITLE_HOST_ALL);
			} else
				printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d' title='%s PENDING'> %d PENDING </a></div>\n", STATUS_CGI, HOST_PENDING, TAC_TITLE_HOST_ALL, hosts_pending);
			printf("</div>\n");
			printf("</td>\n");
		}

		/* Hosts IN TOTAL */
		printf("<td nowrap>\n");
		printf("<div class='tacheader-overall-status-item'>\n");
		printf("<div class='tacheader-status gray'>");
		if (show_tac_header_pending == TRUE)
			printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d' title='%s'> %d /</a>", STATUS_CGI, HOST_UNREACHABLE | HOST_DOWN | HOST_PENDING, TAC_TITLE_HOST_PROBLEM_ALL, hosts_down + hosts_unreachable + hosts_pending);
		else
			printf("<a target='main' href='%s?host=all&style=hostdetail&hoststatustypes=%d' title='%s'> %d /</a>", STATUS_CGI, HOST_UNREACHABLE | HOST_DOWN, TAC_TITLE_HOST_ALL, hosts_down + hosts_unreachable);
		printf("<a target='main' href='%s?host=all&style=hostdetail' title='%s'> %d TOTAL </a></div>\n", STATUS_CGI, TAC_TITLE_HOST_ALL, total_hosts);
		printf("</div>\n");
		printf("</td>\n");

		if (refresh_type == JAVASCRIPT_REFRESH) {
			printf("<td nowrap align=center>\n");
			printf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href='#' onClick='icinga_do_refresh(); return false;' title=\"I'm so lonely up here. Where should I go?\"><img src='%s%s' border=0 style='margin-bottom:-2px;'></a>\n", url_images_path, RELOAD_ICON);
			printf("</td>\n");
		}

		printf("</tr>\n");
		printf("</table></td>\n");
		printf("</tr>\n");


		/* 2. Row Services */
		printf("<tr>\n");
		printf("<td><img src='%s%s' alt='Services' width='16' height='16' align='right' /></td>\n", url_images_path, TAC_HEADER_SERVICE_ICON);
		printf("<td nowrap='nowrap'><table width=auto border='0'>\n");
		printf("<tr>\n");

		/* Services OK */
		printf("<td nowrap>\n");
		printf("<div class='tacheader-overall-status-item'>\n");
		printf("<div class='tacheader-status %s'>", (services_ok + services_ok_host_down > 0) ? "tacheader-status-ok color" : "gray");
		if (services_ok_disabled + services_ok_disabled_host_down > 0) {
			printf("<a target='main' href='%s?host=all&style=detail&servicestatustypes=%d&serviceprops=%d' title='%s OK'> %d </a>/", STATUS_CGI, SERVICE_OK, SERVICE_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_SVC_NOT_DISABLED, services_ok_active + services_ok_active_host_down + services_ok_passive + services_ok_passive_host_down);
			printf("<a target='main' href='%s?host=all&style=detail&servicestatustypes=%d&serviceprops=%d' title='%s OK'> %d </a>", STATUS_CGI, SERVICE_OK, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, TAC_TITLE_SVC_DISABLED, services_ok_disabled + services_ok_disabled_host_down);
			printf("<a target='main' href='%s?host=all&style=detail&servicestatustypes=%d' title='%s OK'> OK </a></div>\n", STATUS_CGI, SERVICE_OK, TAC_TITLE_SVC_ALL);
		} else
			printf("<a target='main' href='%s?host=all&style=detail&servicestatustypes=%d' title='%s OK'> %d OK </a></div>\n", STATUS_CGI, SERVICE_OK, TAC_TITLE_SVC_ALL, services_ok + services_ok_host_down);
		printf("</div>\n");
		printf("</td>\n");

		/* Services WARNING */
		printf("<td nowrap>\n");
		printf("<div class='tacheader-overall-status-item'>\n");

		// Unacknowledged
		handled_count = services_warning_active_unacknowledged_host_down + services_warning_passive_unacknowledged_host_down +
		                services_warning_disabled_unacknowledged + services_warning_disabled_unacknowledged_host_down +
		                // Acknowledeged host down
		                services_warning_active_acknowledged_host_down + services_warning_passive_acknowledged_host_down +
		                services_warning_disabled_acknowledged + services_warning_disabled_acknowledged_host_down +
		                // Scheduled
		                services_warning_scheduled + services_warning_scheduled_host_down;

		if (services_warning_active_unacknowledged + services_warning_passive_unacknowledged > 0)
			tacheader_color = "tacheader-status-warning color";
		else if (services_warning_active_acknowledged + services_warning_passive_acknowledged > 0)
			tacheader_color = "tacheader-status-warning-acknowledged color";
		else if (handled_count > 0)
			tacheader_color = "tacheader-status-warning-handled color";
		else
			tacheader_color = "gray";


		printf("<div class='tacheader-status %s'>", tacheader_color);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d' title='%s WARNING'> %d </a>/", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_SVC_UNACK_SERVICES, services_warning_active_unacknowledged + services_warning_passive_unacknowledged);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d' title='%s WARNING'> %d </a>/", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_ACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_SVC_ACK_SERVICES, services_warning_active_acknowledged + services_warning_passive_acknowledged);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&serviceprops=%d' title='%s WARNING'> %d </a>", STATUS_CGI, SERVICE_WARNING, SERVICE_STATE_HANDLED, TAC_TITLE_SVC_NON_URGENT, handled_count);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d' title='%s WARNING'>WARNING</a>&nbsp;</div>\n", STATUS_CGI, SERVICE_WARNING, TAC_TITLE_SVC_ALL);
		printf("</div>\n");
		printf("</td>\n");

		/* Services CRITICAL */
		printf("<td nowrap>\n");
		printf("<div class='tacheader-overall-status-item'>\n");

		// Unacknowledged
		handled_count = services_critical_active_unacknowledged_host_down + services_critical_passive_unacknowledged_host_down +
		                services_critical_disabled_unacknowledged + services_critical_disabled_unacknowledged_host_down +
		                // Acknowledeged host down
		                services_critical_active_acknowledged_host_down + services_critical_passive_acknowledged_host_down +
		                services_critical_disabled_acknowledged + services_critical_disabled_acknowledged_host_down +
		                // Scheduled
		                services_critical_scheduled + services_critical_scheduled_host_down;

		if (services_critical_active_unacknowledged + services_critical_passive_unacknowledged > 0)
			tacheader_color = "tacheader-status-critical color";
		else if (services_critical_active_acknowledged + services_critical_passive_acknowledged > 0)
			tacheader_color = "tacheader-status-critical-acknowledged color";
		else if (handled_count > 0)
			tacheader_color = "tacheader-status-critical-handled color";
		else
			tacheader_color = "gray";


		printf("<div class='tacheader-status %s'>", tacheader_color);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d' title='%s CRITICAL'> %d </a>/", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_SVC_UNACK_SERVICES, services_critical_active_unacknowledged + services_critical_passive_unacknowledged);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d' title='%s CRITICAL'> %d </a>/", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_ACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_SVC_ACK_SERVICES, services_critical_active_acknowledged + services_critical_passive_acknowledged);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&serviceprops=%d' title='%s CRITICAL'> %d </a>", STATUS_CGI, SERVICE_CRITICAL, SERVICE_STATE_HANDLED, TAC_TITLE_SVC_NON_URGENT, handled_count);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d' title='%s CRITICAL'>CRITICAL</a>&nbsp;</div>\n", STATUS_CGI, SERVICE_CRITICAL, TAC_TITLE_SVC_ALL);
		printf("</div>\n");
		printf("</td>\n");

		/* Services UNKNOWN */
		printf("<td nowrap>\n");
		printf("<div class='tacheader-overall-status-item'>\n");

		// Unacknowledged
		handled_count = services_unknown_active_unacknowledged_host_down + services_unknown_passive_unacknowledged_host_down +
		                services_unknown_disabled_unacknowledged + services_unknown_disabled_unacknowledged_host_down +
		                // Acknowledeged host down
		                services_unknown_active_acknowledged_host_down + services_unknown_passive_acknowledged_host_down +
		                services_unknown_disabled_acknowledged + services_unknown_disabled_acknowledged_host_down +
		                // Scheduled
		                services_unknown_scheduled + services_unknown_scheduled_host_down;

		if (services_unknown_active_unacknowledged + services_unknown_passive_unacknowledged > 0)
			tacheader_color = "tacheader-status-unknown color";
		else if (services_unknown_active_acknowledged + services_unknown_passive_acknowledged > 0)
			tacheader_color = "tacheader-status-unknown-acknowledged color";
		else if (handled_count > 0)
			tacheader_color = "tacheader-status-unknown-handled color";
		else
			tacheader_color = "gray";

		printf("<div class='tacheader-status %s'>", tacheader_color);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d' title='%s UNKNOWN'> %d </a>/", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_SVC_UNACK_SERVICES, services_unknown_active_unacknowledged + services_unknown_passive_unacknowledged);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d' title='%s UNKNOWN'> %d </a>/", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_ACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_SVC_ACK_SERVICES, services_unknown_active_acknowledged + services_unknown_passive_acknowledged);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&serviceprops=%d' title='%s UNKNOWN'> %d </a>", STATUS_CGI, SERVICE_UNKNOWN, SERVICE_STATE_HANDLED, TAC_TITLE_SVC_NON_URGENT, handled_count);
		printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d' title='%s UNKNOWN'>UNKNOWN</a>&nbsp;</div>\n", STATUS_CGI, SERVICE_UNKNOWN, TAC_TITLE_SVC_ALL);
		printf("</div>\n");
		printf("</td>\n");


		/* Services PENDING */
		if (show_tac_header_pending == TRUE) {
			printf("<td nowrap>\n");
			printf("<div class='tacheader-overall-status-item'>\n");
			printf("<div class='tacheader-status %s'>", (services_pending > 0) ? "tacheader-status-pending color" : "gray");
			if (services_pending_disabled + services_pending_disabled_host_down > 0) {
				printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&serviceprops=%d' title='%s PENDING'> %d </a>/", STATUS_CGI, SERVICE_PENDING, SERVICE_NOT_ALL_CHECKS_DISABLED, TAC_TITLE_SVC_NOT_DISABLED, services_pending_active + services_pending_active_host_down + services_pending_passive + services_pending_passive_host_down);
				printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d&serviceprops=%d' title='%s PENDING'> %d </a>", STATUS_CGI, SERVICE_PENDING, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, TAC_TITLE_SVC_DISABLED, services_pending_disabled + services_pending_disabled_host_down);
				printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d' title='%s PENDING'> PENDING </a></div>\n", STATUS_CGI, SERVICE_PENDING, TAC_TITLE_SVC_ALL);
			} else
				printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d' title='%s PENDING'> %d PENDING </a></div>\n", STATUS_CGI, SERVICE_PENDING, TAC_TITLE_SVC_ALL, services_pending + services_pending_host_down);
			printf("</div>\n");
			printf("</td>\n");
		}

		/* Services IN TOTAL */
		printf("<td nowrap>\n");
		printf("<div class='tacheader-overall-status-item'>\n");
		printf("<div class='tacheader-status gray'>");

		handled_count = services_warning + services_warning_host_down +
		                services_unknown + services_unknown_host_down +
		                services_critical + services_critical_host_down;

		if (show_tac_header_pending == FALSE)
			printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d' title='%s'> %d /</a>", STATUS_CGI, SERVICE_UNKNOWN | SERVICE_CRITICAL | SERVICE_WARNING, TAC_TITLE_SVC_PROBLEM_ALL, handled_count);
		else {
			handled_count = handled_count + services_pending + services_pending_host_down;
			printf("<a target='main' href='%s?host=all&type=detail&servicestatustypes=%d' title='%s'> %d /</a>", STATUS_CGI, SERVICE_UNKNOWN | SERVICE_CRITICAL | SERVICE_WARNING | SERVICE_PENDING, TAC_TITLE_SVC_PROBLEM_ALL, handled_count);
		}

		printf("<a target='main' href='%s?host=all' title='%s'> %d TOTAL </a></div>\n", STATUS_CGI, TAC_TITLE_SVC_ALL, total_services);
		printf("</div>\n");
		printf("</td>\n");

		printf("</tr>\n");
		printf("</table></td>\n");

		printf("</tr>\n");
		printf("</table></td>\n");

		/* Monitor Performance */
		printf("<td width='460px' height='70px' style='background-image: url(%s%s)'><table width='280px' border='0' align='right' class='tacheader-monitor-performance-container'>\n", url_images_path, TAC_HEADER_LOGO);
		printf("<tr>\n");
		printf("<td><img src='%s%s' width='16' height='16' alt='Hosts (active/passive)' /></td>\n", url_images_path, TAC_HEADER_HOST_ICON);
		printf("<td>\n");
		printf("<div class='tacheader-monitor'>");
		printf("<a target='main' href='%s?host=all&hostprops=%d&style=hostdetail' title='Hosts Active'>%d</a> / <a target='main' href='%s?host=all&hostprops=%d&style=hostdetail' title='Hosts Passive'>%d</a> / <a target='main' href='%s?host=all&hostprops=%d&style=hostdetail' title='Hosts Disabled'>%d</a></div>\n", STATUS_CGI, HOST_CHECKS_ENABLED, total_active_host_checks, STATUS_CGI, HOST_PASSIVE_CHECKS_ENABLED | HOST_CHECKS_DISABLED, total_passive_host_checks, STATUS_CGI, HOST_PASSIVE_CHECKS_DISABLED | HOST_CHECKS_DISABLED, total_disabled_host_checks);
		printf("</td>\n");
		printf("<td><img src='%s%s' width='16' height='16' alt='Services (active/passive)' /></td>\n", url_images_path, TAC_HEADER_SERVICE_ICON);
		printf("<td>\n");
		printf("<div class='tacheader-monitor'>");
		printf("<a target='main' href='%s?host=all&serviceprops=%d' title='Services Active'>%d</a> / <a target='main' href='%s?host=all&serviceprops=%d' title='Services Passive'>%d</a> / <a target='main' href='%s?host=all&serviceprops=%d' title='Services Disabled'>%d</a></div>\n", STATUS_CGI, SERVICE_CHECKS_ENABLED, total_active_service_checks, STATUS_CGI, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, total_passive_service_checks, STATUS_CGI, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, total_disabled_service_checks);
		printf("</td>\n");
		printf("</tr>\n");
		printf("<tr>\n");
		printf("<td><img src='%s%s' width='16' height='16' alt='Host Execution Time (min/max/avg)' /></td>\n", url_images_path, TAC_HEADER_EXECUTION_ICON);
		printf("<td nowrap='nowrap'>\n");
		printf("<div class='tacheader-monitor'>");
		printf("<a target='main' href='%s?type=%d' title='Min Host Check Execution Time'>%.2f </a>/", EXTINFO_CGI, DISPLAY_PERFORMANCE, min_host_execution_time);
		printf("<a target='main' href='%s?type=%d' title='Max Host Check Execution Time'> %.2f </a>/", EXTINFO_CGI, DISPLAY_PERFORMANCE, max_host_execution_time);
		printf("<a target='main' href='%s?type=%d' title='Average Host Check Execution Time'> %.3f </a>s", EXTINFO_CGI, DISPLAY_PERFORMANCE, average_host_execution_time);
		printf("</div>\n</td>\n");
		printf("<td><img src='%s%s' width='16' height='16' alt='Service Execution Time (min/max/avg)' /></td>\n", url_images_path, TAC_HEADER_EXECUTION_ICON);
		printf("<td nowrap='nowrap'>\n");
		printf("<div class='tacheader-monitor'>");
		printf("<a target='main' href='%s?type=%d' title='Min Service Check Execution Time'>%.2f </a>/", EXTINFO_CGI, DISPLAY_PERFORMANCE, min_service_execution_time);
		printf("<a target='main' href='%s?type=%d' title='Max Service Check Execution Time'> %.2f </a>/", EXTINFO_CGI, DISPLAY_PERFORMANCE, max_service_execution_time);
		printf("<a target='main' href='%s?type=%d' title='Average Service Check Execution Time'> %.3f </a>s", EXTINFO_CGI, DISPLAY_PERFORMANCE, average_service_execution_time);
		printf("</div>\n</td>\n");
		printf("</tr>\n");
		printf("<tr>\n");
		printf("<td><img src='%s%s' width='16' height='16' alt='Host Latency (min/max/avg)' /></td>\n", url_images_path, TAC_HEADER_LATENCY_ICON);
		printf("<td nowrap='nowrap'>\n");
		printf("<div class='tacheader-monitor'>");
		printf("<a target='main' href='%s?type=%d' title='Min Host Check Latency'>%.2f </a>/", EXTINFO_CGI, DISPLAY_PERFORMANCE, min_host_latency);
		printf("<a target='main' href='%s?type=%d' title='Max Host Check Latency'> %.2f </a>/", EXTINFO_CGI, DISPLAY_PERFORMANCE, max_host_latency);
		printf("<a target='main' href='%s?type=%d' title='Average Host Check Latency'> %.3f </a>s", EXTINFO_CGI, DISPLAY_PERFORMANCE, average_host_latency);
		printf("</div>\n</td>\n");
		printf("<td><img src='%s%s' width='16' height='16' alt='Service Latency (min/max/avg)' /></td>\n", url_images_path, TAC_HEADER_LATENCY_ICON);
		printf("<td nowrap='nowrap'>\n");
		printf("<div class='tacheader-monitor'>");
		printf("<a target='main' href='%s?type=%d' title='Min Service Check Latency'>%.2f </a>/", EXTINFO_CGI, DISPLAY_PERFORMANCE, min_service_latency);
		printf("<a target='main' href='%s?type=%d' title='Max Service Check Latency'> %.2f </a>/", EXTINFO_CGI, DISPLAY_PERFORMANCE, max_service_latency);
		printf("<a target='main' href='%s?type=%d' title='Average Service Check Latency'> %.3f </a>s", EXTINFO_CGI, DISPLAY_PERFORMANCE, average_service_latency);
		printf("</td>\n");
		printf("</tr>\n");
		printf("</table></td>\n");
		printf("</tr>\n");
		printf("</table>\n");

		return; //we're done here
	}


	if (content_type == JSON_CONTENT) {
		printf("\"tac_overview\": {\n");

		/* outages */
		printf("\"network_outages\": %d,\n", total_blocking_outages);

		/* network health */
		printf("\"percent_host_health\": %2.1f,\n", percent_host_health);
		printf("\"percent_service_health\": %2.1f,\n", percent_service_health);

		printf("\"total_hosts\": %d,\n", total_hosts);
		printf("\"total_services\": %d,\n", total_services);

		/* host data */

		/* Pending */
		printf("\"hosts_pending\": %d,\n", hosts_pending);
		printf("\"hosts_pending_active\": %d,\n", hosts_pending_active);
		printf("\"hosts_pending_passive\": %d,\n", hosts_pending_passive);
		printf("\"hosts_pending_disabled\": %d,\n", hosts_pending_disabled);

		/* UP */
		printf("\"hosts_up\": %d,\n", hosts_up);
		printf("\"hosts_up_active\": %d,\n", hosts_up_active);
		printf("\"hosts_up_passive\": %d,\n", hosts_up_passive);
		printf("\"hosts_up_disabled\": %d,\n", hosts_up_disabled);

		/* DOWN */
		printf("\"hosts_down\": %d,\n", hosts_down);
		printf("\"hosts_down_active\": %d,\n", hosts_down_active);
		printf("\"hosts_down_passive\": %d,\n", hosts_down_passive);
		printf("\"hosts_down_disabled\": %d,\n", hosts_down_disabled);

		printf("\"hosts_down_scheduled\": %d,\n", hosts_down_scheduled);
		printf("\"hosts_down_active_scheduled\": %d,\n", hosts_down_active_scheduled);
		printf("\"hosts_down_passive_scheduled\": %d,\n", hosts_down_passive_scheduled);
		printf("\"hosts_down_disabled_scheduled\": %d,\n", hosts_down_disabled_scheduled);

		printf("\"hosts_down_acknowledged\": %d,\n", hosts_down_acknowledged);
		printf("\"hosts_down_active_acknowledged\": %d,\n", hosts_down_active_acknowledged);
		printf("\"hosts_down_passive_acknowledged\": %d,\n", hosts_down_passive_acknowledged);
		printf("\"hosts_down_disabled_acknowledged\": %d,\n", hosts_down_disabled_acknowledged);

		printf("\"hosts_down_unacknowledged\": %d,\n", hosts_down_unacknowledged);
		printf("\"hosts_down_active_unacknowledged\": %d,\n", hosts_down_active_unacknowledged);
		printf("\"hosts_down_passive_unacknowledged\": %d,\n", hosts_down_passive_unacknowledged);
		printf("\"hosts_down_disabled_unacknowledged\": %d,\n", hosts_down_disabled_unacknowledged);

		/* UNREACHABLE */
		printf("\"hosts_unreachable\": %d,\n", hosts_unreachable);
		printf("\"hosts_unreachable_active\": %d,\n", hosts_unreachable_active);
		printf("\"hosts_unreachable_passive\": %d,\n", hosts_unreachable_passive);
		printf("\"hosts_unreachable_disabled\": %d,\n", hosts_unreachable_disabled);

		printf("\"hosts_unreachable_scheduled\": %d,\n", hosts_unreachable_scheduled);
		printf("\"hosts_unreachable_active_scheduled\": %d,\n", hosts_unreachable_active_scheduled);
		printf("\"hosts_unreachable_passive_scheduled\": %d,\n", hosts_unreachable_passive_scheduled);
		printf("\"hosts_unreachable_disabled_scheduled\": %d,\n", hosts_unreachable_disabled_scheduled);

		printf("\"hosts_unreachable_acknowledged\": %d,\n", hosts_unreachable_acknowledged);
		printf("\"hosts_unreachable_active_acknowledged\": %d,\n", hosts_unreachable_active_acknowledged);
		printf("\"hosts_unreachable_passive_acknowledged\": %d,\n", hosts_unreachable_passive_acknowledged);
		printf("\"hosts_unreachable_disabled_acknowledged\": %d,\n", hosts_unreachable_disabled_acknowledged);

		printf("\"hosts_unreachable_unacknowledged\": %d,\n", hosts_unreachable_unacknowledged);
		printf("\"hosts_unreachable_active_unacknowledged\": %d,\n", hosts_unreachable_active_unacknowledged);
		printf("\"hosts_unreachable_passive_unacknowledged\": %d,\n", hosts_unreachable_passive_unacknowledged);
		printf("\"hosts_unreachable_disabled_unacknowledged\": %d,\n", hosts_unreachable_disabled_unacknowledged);

		/* service data */

		/* PENDING */
		printf("\"services_pending\": %d,\n", services_pending);
		printf("\"services_pending_host_down\": %d,\n", services_pending_host_down);
		printf("\"services_pending_active\": %d,\n", services_pending_active);
		printf("\"services_pending_active_host_down\": %d,\n", services_pending_active_host_down);
		printf("\"services_pending_passive\": %d,\n", services_pending_passive);
		printf("\"services_pending_passive_host_down\": %d,\n", services_pending_passive_host_down);
		printf("\"services_pending_disabled\": %d,\n", services_pending_disabled);
		printf("\"services_pending_disabled_host_down\": %d,\n", services_pending_disabled_host_down);

		/* OK */
		printf("\"services_ok\": %d,\n", services_ok);
		printf("\"services_ok_host_down\": %d,\n", services_ok_host_down);
		printf("\"services_ok_active\": %d,\n", services_ok_active);
		printf("\"services_ok_active_host_down\": %d,\n", services_ok_active_host_down);
		printf("\"services_ok_passive\": %d,\n", services_ok_passive);
		printf("\"services_ok_passive_host_down\": %d,\n", services_ok_passive_host_down);
		printf("\"services_ok_disabled\": %d,\n", services_ok_disabled);
		printf("\"services_ok_disabled_host_down\": %d,\n", services_ok_disabled_host_down);

		/* WARNING */
		printf("\"services_warning\": %d,\n", services_warning);
		printf("\"services_warning_host_down\": %d,\n", services_warning_host_down);
		printf("\"services_warning_active\": %d,\n", services_warning_active);
		printf("\"services_warning_active_host_down\": %d,\n", services_warning_active_host_down);
		printf("\"services_warning_passive\": %d,\n", services_warning_passive);
		printf("\"services_warning_passive_host_down\": %d,\n", services_warning_passive_host_down);
		printf("\"services_warning_disabled\": %d,\n", services_warning_disabled);
		printf("\"services_warning_disabled_host_down\": %d,\n", services_warning_disabled_host_down);

		printf("\"services_warning_scheduled\": %d,\n", services_warning_scheduled);
		printf("\"services_warning_scheduled_host_down\": %d,\n", services_warning_scheduled_host_down);
		printf("\"services_warning_active_scheduled\": %d,\n", services_warning_active_scheduled);
		printf("\"services_warning_active_scheduled_host_down\": %d,\n", services_warning_active_scheduled_host_down);
		printf("\"services_warning_passive_scheduled\": %d,\n", services_warning_passive_scheduled);
		printf("\"services_warning_passive_scheduled_host_down\": %d,\n", services_warning_passive_scheduled_host_down);
		printf("\"services_warning_disabled_scheduled\": %d,\n", services_warning_disabled_scheduled);
		printf("\"services_warning_disabled_scheduled_host_down\": %d,\n", services_warning_disabled_scheduled_host_down);

		printf("\"services_warning_acknowledged\": %d,\n", services_warning_acknowledged);
		printf("\"services_warning_acknowledged_host_down\": %d,\n", services_warning_acknowledged_host_down);
		printf("\"services_warning_active_acknowledged\": %d,\n", services_warning_active_acknowledged);
		printf("\"services_warning_active_acknowledged_host_down\": %d,\n", services_warning_active_acknowledged_host_down);
		printf("\"services_warning_passive_acknowledged\": %d,\n", services_warning_passive_acknowledged);
		printf("\"services_warning_passive_acknowledged_host_down\": %d,\n", services_warning_passive_acknowledged_host_down);
		printf("\"services_warning_disabled_acknowledged\": %d,\n", services_warning_disabled_acknowledged);
		printf("\"services_warning_disabled_acknowledged_host_down\": %d,\n", services_warning_disabled_acknowledged_host_down);

		printf("\"services_warning_unacknowledged\": %d,\n", services_warning_unacknowledged);
		printf("\"services_warning_unacknowledged_host_down\": %d,\n", services_warning_unacknowledged_host_down);
		printf("\"services_warning_active_unacknowledged\": %d,\n", services_warning_active_unacknowledged);
		printf("\"services_warning_active_unacknowledged_host_down\": %d,\n", services_warning_active_unacknowledged_host_down);
		printf("\"services_warning_passive_unacknowledged\": %d,\n", services_warning_passive_unacknowledged);
		printf("\"services_warning_passive_unacknowledged_host_down\": %d,\n", services_warning_passive_unacknowledged_host_down);
		printf("\"services_warning_disabled_unacknowledged\": %d,\n", services_warning_disabled_unacknowledged);
		printf("\"services_warning_disabled_unacknowledged_host_down\": %d,\n", services_warning_disabled_unacknowledged_host_down);

		/* CRITICAL */
		printf("\"services_critical\": %d,\n", services_critical);
		printf("\"services_critical_host_down\": %d,\n", services_critical_host_down);
		printf("\"services_critical_active\": %d,\n", services_critical_active);
		printf("\"services_critical_active_host_down\": %d,\n", services_critical_active_host_down);
		printf("\"services_critical_passive\": %d,\n", services_critical_passive);
		printf("\"services_critical_passive_host_down\": %d,\n", services_critical_passive_host_down);
		printf("\"services_critical_disabled\": %d,\n", services_critical_disabled);
		printf("\"services_critical_disabled_host_down\": %d,\n", services_critical_disabled_host_down);

		printf("\"services_critical_scheduled\": %d,\n", services_critical_scheduled);
		printf("\"services_critical_scheduled_host_down\": %d,\n", services_critical_scheduled_host_down);
		printf("\"services_critical_active_scheduled\": %d,\n", services_critical_active_scheduled);
		printf("\"services_critical_active_scheduled_host_down\": %d,\n", services_critical_active_scheduled_host_down);
		printf("\"services_critical_passive_scheduled\": %d,\n", services_critical_passive_scheduled);
		printf("\"services_critical_passive_scheduled_host_down\": %d,\n", services_critical_passive_scheduled_host_down);
		printf("\"services_critical_disabled_scheduled\": %d,\n", services_critical_disabled_scheduled);
		printf("\"services_critical_disabled_scheduled_host_down\": %d,\n", services_critical_disabled_scheduled_host_down);

		printf("\"services_critical_acknowledged\": %d,\n", services_critical_acknowledged);
		printf("\"services_critical_acknowledged_host_down\": %d,\n", services_critical_acknowledged_host_down);
		printf("\"services_critical_active_acknowledged\": %d,\n", services_critical_active_acknowledged);
		printf("\"services_critical_active_acknowledged_host_down\": %d,\n", services_critical_active_acknowledged_host_down);
		printf("\"services_critical_passive_acknowledged\": %d,\n", services_critical_passive_acknowledged);
		printf("\"services_critical_passive_acknowledged_host_down\": %d,\n", services_critical_passive_acknowledged_host_down);
		printf("\"services_critical_disabled_acknowledged\": %d,\n", services_critical_disabled_acknowledged);
		printf("\"services_critical_disabled_acknowledged_host_down\": %d,\n", services_critical_disabled_acknowledged_host_down);

		printf("\"services_critical_unacknowledged\": %d,\n", services_critical_unacknowledged);
		printf("\"services_critical_unacknowledged_host_down\": %d,\n", services_critical_unacknowledged_host_down);
		printf("\"services_critical_active_unacknowledged\": %d,\n", services_critical_active_unacknowledged);
		printf("\"services_critical_active_unacknowledged_host_down\": %d,\n", services_critical_active_unacknowledged_host_down);
		printf("\"services_critical_passive_unacknowledged\": %d,\n", services_critical_passive_unacknowledged);
		printf("\"services_critical_passive_unacknowledged_host_down\": %d,\n", services_critical_passive_unacknowledged_host_down);
		printf("\"services_critical_disabled_unacknowledged\": %d,\n", services_critical_disabled_unacknowledged);
		printf("\"services_critical_disabled_unacknowledged_host_down\": %d,\n", services_critical_disabled_unacknowledged_host_down);

		/* UNKNOWN */
		printf("\"services_unknown\": %d,\n", services_unknown);
		printf("\"services_unknown_host_down\": %d,\n", services_unknown_host_down);
		printf("\"services_unknown_active\": %d,\n", services_unknown_active);
		printf("\"services_unknown_active_host_down\": %d,\n", services_unknown_active_host_down);
		printf("\"services_unknown_passive\": %d,\n", services_unknown_passive);
		printf("\"services_unknown_passive_host_down\": %d,\n", services_unknown_passive_host_down);
		printf("\"services_unknown_disabled\": %d,\n", services_unknown_disabled);
		printf("\"services_unknown_disabled_host_down\": %d,\n", services_unknown_disabled_host_down);

		printf("\"services_unknown_scheduled\": %d,\n", services_unknown_scheduled);
		printf("\"services_unknown_scheduled_host_down\": %d,\n", services_unknown_scheduled_host_down);
		printf("\"services_unknown_active_scheduled\": %d,\n", services_unknown_active_scheduled);
		printf("\"services_unknown_active_scheduled_host_down\": %d,\n", services_unknown_active_scheduled_host_down);
		printf("\"services_unknown_passive_scheduled\": %d,\n", services_unknown_passive_scheduled);
		printf("\"services_unknown_passive_scheduled_host_down\": %d,\n", services_unknown_passive_scheduled_host_down);
		printf("\"services_unknown_disabled_scheduled\": %d,\n", services_unknown_disabled_scheduled);
		printf("\"services_unknown_disabled_scheduled_host_down\": %d,\n", services_unknown_disabled_scheduled_host_down);

		printf("\"services_unknown_acknowledged\": %d,\n", services_unknown_acknowledged);
		printf("\"services_unknown_acknowledged_host_down\": %d,\n", services_unknown_acknowledged_host_down);
		printf("\"services_unknown_active_acknowledged\": %d,\n", services_unknown_active_acknowledged);
		printf("\"services_unknown_active_acknowledged_host_down\": %d,\n", services_unknown_active_acknowledged_host_down);
		printf("\"services_unknown_passive_acknowledged\": %d,\n", services_unknown_passive_acknowledged);
		printf("\"services_unknown_passive_acknowledged_host_down\": %d,\n", services_unknown_passive_acknowledged_host_down);
		printf("\"services_unknown_disabled_acknowledged\": %d,\n", services_unknown_disabled_acknowledged);
		printf("\"services_unknown_disabled_acknowledged_host_down\": %d,\n", services_unknown_disabled_acknowledged_host_down);

		printf("\"services_unknown_unacknowledged\": %d,\n", services_unknown_unacknowledged);
		printf("\"services_unknown_unacknowledged_host_down\": %d,\n", services_unknown_unacknowledged_host_down);
		printf("\"services_unknown_active_unacknowledged\": %d,\n", services_unknown_active_unacknowledged);
		printf("\"services_unknown_active_unacknowledged_host_down\": %d,\n", services_unknown_active_unacknowledged_host_down);
		printf("\"services_unknown_passive_unacknowledged\": %d,\n", services_unknown_passive_unacknowledged);
		printf("\"services_unknown_passive_unacknowledged_host_down\": %d,\n", services_unknown_passive_unacknowledged_host_down);
		printf("\"services_unknown_disabled_unacknowledged\": %d,\n", services_unknown_disabled_unacknowledged);
		printf("\"services_unknown_disabled_unacknowledged_host_down\": %d,\n", services_unknown_disabled_unacknowledged_host_down);

		/* monitoring features */
		printf("\"flap_detection_enabled\": %s,\n", (enable_flap_detection == TRUE) ? "true" : "false");
		printf("\"flap_disabled_services\": %d,\n", flap_disabled_services);
		printf("\"flapping_services\": %d,\n", flapping_services);
		printf("\"flap_disabled_hosts\": %d,\n", flap_disabled_hosts);
		printf("\"flapping_hosts\": %d,\n", flapping_hosts);

		printf("\"notifications_enabled\": %s,\n", (enable_notifications == TRUE) ? "true" : "false");
		printf("\"notification_disabled_services\": %d,\n", notification_disabled_services);
		printf("\"notification_disabled_hosts\": %d,\n", notification_disabled_hosts);

		printf("\"event_handlers_enabled\": %s,\n", (enable_event_handlers == TRUE) ? "true" : "false");
		printf("\"event_handler_disabled_services\": %d,\n", event_handler_disabled_services);
		printf("\"event_handler_disabled_hosts\": %d,\n", event_handler_disabled_hosts);

		printf("\"execute_service_checks\": %s,\n", (execute_service_checks == TRUE) ? "true" : "false");
		printf("\"execute_host_checks\": %s,\n", (execute_host_checks == TRUE) ? "true" : "false");
		printf("\"accept_passive_service_checks\": %s,\n", (accept_passive_service_checks == TRUE) ? "true" : "false");
		printf("\"accept_passive_host_checks\": %s,\n", (accept_passive_host_checks == TRUE) ? "true" : "false");

		/* monitoring performance */
		printf("\"min_service_check_execution_time\": %.2f,\n", min_service_execution_time);
		printf("\"max_service_check_execution_time\": %.2f,\n", max_service_execution_time);
		printf("\"average_service_check_execution_time\": %.3f,\n", average_service_execution_time);

		printf("\"min_service_check_latency\": %.2f,\n", min_service_latency);
		printf("\"max_service_check_latency\": %.2f,\n", max_service_latency);
		printf("\"average_service_check_latency\": %.3f,\n", average_service_latency);

		printf("\"min_host_check_execution_time\": %.2f,\n", min_host_execution_time);
		printf("\"max_host_check_execution_time\": %.2f,\n", max_host_execution_time);
		printf("\"average_host_check_execution_time\": %.3f,\n", average_host_execution_time);

		printf("\"min_host_check_latency\": %.2f,\n", min_host_latency);
		printf("\"max_host_check_latency\": %.2f,\n", max_host_latency);
		printf("\"average_host_check_latency\": %.3f,\n", average_host_latency);

		printf("\"total_active_host_checks\": %d,\n", total_active_host_checks);
		printf("\"total_passive_host_checks\": %d,\n", total_passive_host_checks);
		printf("\"total_disabled_host_checks\": %d,\n", total_disabled_host_checks);
		printf("\"total_active_host_checks_with_passive_disabled\": %d,\n", total_active_host_checks_with_passive_disabled);

		printf("\"total_active_service_checks\": %d,\n", total_active_service_checks);
		printf("\"total_passive_service_checks\": %d,\n", total_passive_service_checks);
		printf("\"total_disabled_service_checks\": %d,\n", total_disabled_service_checks);
		printf("\"total_active_service_checks_with_passive_disabled\": %d\n", total_active_service_checks_with_passive_disabled);

		printf(" }\n");

		// we return here when finished with generating JSON content.
		return;
	}

	if (display_header == TRUE) {
		printf("<p align=left>\n");

		printf("<table border=0 align=left width=100%% cellspacing=4 cellpadding=0>\n");
		printf("<tr>\n");

		/* left column */
		printf("<td align=left valign=top width=50%%>\n");

		display_info_table("Tactical Monitoring Overview", &current_authdata, daemon_check);

		printf("</td>\n");


		/* right column */
		printf("<td align=right valign=bottom width=50%%>\n");

		printf("<table border=0 cellspacing=0 cellspadding=0>\n");

		printf("<tr>\n");

		printf("<td valign=bottom align=right>\n");
		printf("</td>\n");

		printf("<td>\n");

		if (show_tac_header == FALSE) {
			printf("<table border=0 cellspacing=4 cellspadding=0>\n");
			printf("<tr>\n");
			printf("<td class='perfTitle'>&nbsp;<a href='%s?type=%d' class='perfTitle'>Monitoring Performance</a></td>\n", EXTINFO_CGI, DISPLAY_PERFORMANCE);
			printf("</tr>\n");

			printf("<tr>\n");
			printf("<td>\n");

			printf("<table border=0 cellspacing=0 cellspadding=0>\n");
			printf("<tr>\n");
			printf("<td class='perfBox'>\n");
			printf("<table border=0 cellspacing=4 cellspadding=0>\n");
			printf("<tr>\n");
			printf("<td align=left valign=center class='perfItem'><a href='%s?type=%d' class='perfItem'>Service Check Execution Time:</a></td>", EXTINFO_CGI, DISPLAY_PERFORMANCE);
			printf("<td valign=top class='perfValue' nowrap><a href='%s?type=%d' class='perfValue'>%.2f / %.2f / %.3f sec</a></td>\n", EXTINFO_CGI, DISPLAY_PERFORMANCE, min_service_execution_time, max_service_execution_time, average_service_execution_time);
			printf("</tr>\n");
			printf("<tr>\n");
			printf("<td align=left valign=center class='perfItem'><a href='%s?type=%d' class='perfItem'>Service Check Latency:</a></td>", EXTINFO_CGI, DISPLAY_PERFORMANCE);
			printf("<td valign=top class='perfValue' nowrap><a href='%s?type=%d' class='perfValue'>%.2f / %.2f / %.3f sec</a></td>\n", EXTINFO_CGI, DISPLAY_PERFORMANCE, min_service_latency, max_service_latency, average_service_latency);
			printf("</tr>\n");
			printf("<tr>\n");
			printf("<td align=left valign=center class='perfItem'><a href='%s?type=%d' class='perfItem'>Host Check Execution Time:</a></td>", EXTINFO_CGI, DISPLAY_PERFORMANCE);
			printf("<td valign=top class='perfValue' nowrap><a href='%s?type=%d' class='perfValue'>%.2f / %.2f / %.3f sec</a></td>\n", EXTINFO_CGI, DISPLAY_PERFORMANCE, min_host_execution_time, max_host_execution_time, average_host_execution_time);
			printf("</tr>\n");
			printf("<tr>\n");
			printf("<td align=left valign=center class='perfItem'><a href='%s?type=%d' class='perfItem'>Host Check Latency:</a></td>", EXTINFO_CGI, DISPLAY_PERFORMANCE);
			printf("<td valign=top class='perfValue' nowrap><a href='%s?type=%d' class='perfValue'>%.2f / %.2f / %2.3f sec</a></td>\n", EXTINFO_CGI, DISPLAY_PERFORMANCE, min_host_latency, max_host_latency, average_host_latency);
			printf("</tr>\n");
			printf("<tr>\n");
			printf("<td align=left valign=center class='perfItem'>");
			printf("<a href='%s?host=all&hostprops=%d&style=hostdetail' title='Active Hosts' class='perfItem'># Active Host</a> / ", STATUS_CGI, HOST_CHECKS_ENABLED);
			printf("<a href='%s?host=all&serviceprops=%d' title='Active Services' class='perfItem'>Service Checks</a></td>", STATUS_CGI, SERVICE_CHECKS_ENABLED);
			printf("<td valign=top class='perfValue' nowrap><a href='%s?host=all&hostprops=%d&style=hostdetail' title='Active Hosts' class='perfValue'>%d</a> / <a href='%s?host=all&serviceprops=%d' title='Active Services' class='perfValue'>%d</a></td>\n", STATUS_CGI, HOST_CHECKS_ENABLED, total_active_host_checks, STATUS_CGI, SERVICE_CHECKS_ENABLED, total_active_service_checks);
			printf("</tr>\n");
			printf("<tr>\n");
			printf("<td align=left valign=center class='perfItem'>");
			printf("<a href='%s?host=all&hostprops=%d&style=hostdetail' title='Passive Hosts' class='perfItem'># Passive Host</a> / ", STATUS_CGI, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_ENABLED);
			printf("<a href='%s?host=all&serviceprops=%d' title='Passive Services' class='perfItem'>Service Checks</a></td>", STATUS_CGI, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED);
			printf("<td valign=top class='perfValue' nowrap><a href='%s?host=all&hostprops=%d&style=hostdetail' title='Passive Hosts' class='perfValue'>%d</a> / <a href='%s?host=all&serviceprops=%d' title='Passive Services' class='perfValue'>%d</a></td>\n", STATUS_CGI, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_ENABLED, total_passive_host_checks, STATUS_CGI, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, total_passive_service_checks);
			printf("</tr>\n");
			printf("<tr>\n");
			printf("<td align=left valign=center class='perfItem'>");
			printf("<a href='%s?host=all&hostprops=%d&style=hostdetail' title='Disabled Hosts' class='perfItem'># Disabled Host</a> / ", STATUS_CGI, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED);
			printf("<a href='%s?host=all&serviceprops=%d' title='Disabled Services' class='perfItem'>Service Checks</a></td>", STATUS_CGI, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED);
			printf("<td valign=top class='perfValue' nowrap><a href='%s?host=all&hostprops=%d&style=hostdetail' title='Disabled Hosts' class='perfValue'>%d</a> / <a href='%s?host=all&serviceprops=%d' title='Disabled Services' class='perfValue'>%d</a></td>\n", STATUS_CGI, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED, total_disabled_host_checks, STATUS_CGI, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, total_disabled_service_checks);
			printf("</tr>\n");
			printf("</table>\n");
			printf("</td>\n");
			printf("</tr>\n");
			printf("</table>\n");
		}

		printf("</td>\n");
		printf("</tr>\n");
		printf("</table>\n");

		printf("</td>\n");
		printf("</tr>\n");
		printf("</table>\n");

		printf("</td>\n");

		printf("</tr>\n");
		printf("</table>\n");
		printf("</p>\n");
	}

	printf("<br clear=all>\n");
	printf("<br>\n");


	printf("<table border=0 cellspacing=0 cellpadding=0 width=100%%>\n");
	printf("<tr>\n");
	printf("<td valign=top align=left width=50%%>\n");


	/******* OUTAGES ********/

	printf("<p>\n");

	printf("<table class='tac' width=125 cellspacing=4 cellpadding=0 border=0>\n");

	printf("<tr><td colspan=1 height=20 class='outageTitle'>&nbsp;Network Outages</td></tr>\n");

	printf("<tr>\n");
	printf("<td class='outageHeader' width=125><a href='%s' class='outageHeader'>", OUTAGES_CGI);

	printf("%d Outages", total_blocking_outages);
	printf("</a></td>\n");
	printf("</tr>\n");

	printf("<tr>\n");

	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;&nbsp;&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (total_blocking_outages > 0)
		printf("<tr><td width=100%% class='outageImportantProblem'><a href='%s'>%d Blocking Outages</a></td></tr>\n", OUTAGES_CGI, total_blocking_outages);

	/*
	if(total_nonblocking_outages>0)
		printf("<tr><td width=100%% class='outageUnimportantProblem'><a href='%s'>%d Nonblocking Outages</a></td></tr>\n",OUTAGES_CGI,total_nonblocking_outages);
	*/

	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");

	printf("</p>\n");

	printf("</td>\n");


	/* right column */
	printf("<td valign=top align=right width=50%%>\n");

	if (percent_host_health < HEALTH_CRITICAL_PERCENTAGE)
		strncpy(host_health_image, THERM_CRITICAL_IMAGE, sizeof(host_health_image));
	else if (percent_host_health < HEALTH_WARNING_PERCENTAGE)
		strncpy(host_health_image, THERM_WARNING_IMAGE, sizeof(host_health_image));
	else
		strncpy(host_health_image, THERM_OK_IMAGE, sizeof(host_health_image));
	host_health_image[sizeof(host_health_image)-1] = '\x0';

	if (percent_service_health < HEALTH_CRITICAL_PERCENTAGE)
		strncpy(service_health_image, THERM_CRITICAL_IMAGE, sizeof(service_health_image));
	else if (percent_service_health < HEALTH_WARNING_PERCENTAGE)
		strncpy(service_health_image, THERM_WARNING_IMAGE, sizeof(service_health_image));
	else
		strncpy(service_health_image, THERM_OK_IMAGE, sizeof(service_health_image));
	service_health_image[sizeof(service_health_image)-1] = '\x0';

	printf("<table border=0 cellspacing=0 cellspadding=0>\n");
	printf("<tr>\n");
	printf("<td>\n");

	printf("<table border=0 cellspacing=4 cellspadding=0>\n");
	printf("<tr>\n");
	printf("<td class='healthTitle'>&nbsp;Network Health</td>\n");
	printf("</tr>\n");

	printf("<tr>\n");
	printf("<td>\n");

	printf("<table border=0 cellspacing=0 cellspadding=0>\n");
	printf("<tr>\n");
	printf("<td class='healthBox'>\n");
	printf("<table border=0 cellspacing=4 cellspadding=0>\n");
	printf("<tr>\n");
	printf("<td align=left valign=center class='healthItem'>Host Health:</td>");
	printf("<td valign=top width=100 class='healthBar'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d'><img src='%s%s' border=0 width=%d height=20 alt='%2.1f%% Health' title='%2.1f%% Health'></a></td>\n", STATUS_CGI, HOST_DOWN | HOST_UNREACHABLE, url_images_path, host_health_image, (percent_host_health < 5.0) ? 5 : (int)percent_host_health, percent_host_health, percent_host_health);
	printf("</tr>\n");
	printf("<tr>\n");
	printf("<td align=left valign=center class='healthItem'>Service Health:</td>");
	printf("<td valign=top width=100 class='healthBar'><a href='%s?host=all&style=detail&servicestatustypes=%d'><img src='%s%s' border=0 width=%d height=20 alt='%2.1f%% Health' title='%2.1f%% Health'></td>\n", STATUS_CGI, SERVICE_CRITICAL | SERVICE_WARNING | SERVICE_PENDING, url_images_path, service_health_image, (percent_service_health < 5.0) ? 5 : (int)percent_service_health, percent_service_health, percent_service_health);
	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");
	printf("</tr>\n");
	printf("</table>\n");

	printf("</td>\n");
	printf("</tr>\n");
	printf("</table>\n");

	printf("</td>\n");
	printf("</tr>\n");
	printf("</table>\n");

	printf("</td>\n");
	printf("</tr>\n");
	printf("</table>\n");



	/******* HOSTS ********/

	printf("<p>\n");

	printf("<table class='tac' width=516 cellspacing=4 cellpadding=0 border=0>\n");

	printf("<tr><td colspan=4 height=20 class='hostTitle'>&nbsp;Hosts</td></tr>\n");

	printf("<tr>\n");
	printf("<td class='hostHeader' width=125><a href='%s?host=all&style=hostdetail&hoststatustypes=%d' class='hostHeader'>%d Down</a></td>\n", STATUS_CGI, HOST_DOWN, hosts_down);
	printf("<td class='hostHeader' width=125><a href='%s?host=all&style=hostdetail&hoststatustypes=%d' class='hostHeader'>%d Unreachable</a></td>\n", STATUS_CGI, HOST_UNREACHABLE, hosts_unreachable);
	printf("<td class='hostHeader' width=125><a href='%s?host=all&style=hostdetail&hoststatustypes=%d' class='hostHeader'>%d Up</a></td>\n", STATUS_CGI, HOST_UP, hosts_up);
	printf("<td class='hostHeader' width=125><a href='%s?host=all&style=hostdetail&hoststatustypes=%d' class='hostHeader'>%d Pending</a></td>\n", STATUS_CGI, HOST_PENDING, hosts_pending);
	printf("</tr>\n");

	printf("<tr>\n");


	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;&nbsp;&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (hosts_down_active_unacknowledged + hosts_down_passive_unacknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='hostImportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>Unhandled Problems</a><br>", STATUS_CGI, HOST_DOWN, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED);

		if (hosts_down_active_unacknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Active</a>\n", STATUS_CGI, HOST_DOWN, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_CHECKS_ENABLED, hosts_down_active_unacknowledged);
		}
		if (hosts_down_passive_unacknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Passive</a>\n", STATUS_CGI, HOST_DOWN, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_PASSIVE_CHECKS_ENABLED | HOST_CHECKS_DISABLED, hosts_down_passive_unacknowledged);
		}
		printf("</td></tr>\n");
	}
	if (hosts_down_disabled_unacknowledged > 0)
		printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>Unacknowledged<br>%d Disabled</a></td></tr>\n", STATUS_CGI, HOST_DOWN, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED, hosts_down_disabled_unacknowledged);

	if (hosts_down_acknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>Acknowledged</a><br>", STATUS_CGI, HOST_DOWN, HOST_STATE_ACKNOWLEDGED | HOST_NO_SCHEDULED_DOWNTIME);

		if (hosts_down_active_acknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Active</a>\n", STATUS_CGI, HOST_DOWN, HOST_STATE_ACKNOWLEDGED | HOST_NO_SCHEDULED_DOWNTIME | HOST_CHECKS_ENABLED, hosts_down_active_acknowledged);
		}
		if (hosts_down_passive_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Passive</a>\n", STATUS_CGI, HOST_DOWN, HOST_STATE_ACKNOWLEDGED | HOST_NO_SCHEDULED_DOWNTIME | HOST_PASSIVE_CHECKS_ENABLED | HOST_CHECKS_DISABLED, hosts_down_passive_acknowledged);
		}
		if (hosts_down_disabled_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a>\n", STATUS_CGI, HOST_DOWN, HOST_STATE_ACKNOWLEDGED | HOST_NO_SCHEDULED_DOWNTIME | HOST_PASSIVE_CHECKS_DISABLED | HOST_CHECKS_DISABLED, hosts_down_disabled_acknowledged);
		}
		printf("</td></tr>\n");
	}

	if (hosts_down_scheduled > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>Scheduled Downtime</a><br>", STATUS_CGI, HOST_DOWN, HOST_SCHEDULED_DOWNTIME);

		if (hosts_down_active_scheduled > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Active</a>\n", STATUS_CGI, HOST_DOWN, HOST_SCHEDULED_DOWNTIME | HOST_CHECKS_ENABLED, hosts_down_active_scheduled);
		}
		if (hosts_down_passive_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Passive</a>\n", STATUS_CGI, HOST_DOWN, HOST_SCHEDULED_DOWNTIME | HOST_PASSIVE_CHECKS_ENABLED | HOST_CHECKS_DISABLED, hosts_down_passive_scheduled);
		}
		if (hosts_down_disabled_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a>\n", STATUS_CGI, HOST_DOWN, HOST_SCHEDULED_DOWNTIME | HOST_PASSIVE_CHECKS_DISABLED | HOST_CHECKS_DISABLED, hosts_down_disabled_scheduled);
		}
		printf("</td></tr>\n");
	}

	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");


	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (hosts_unreachable_active_unacknowledged + hosts_unreachable_passive_unacknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='hostImportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>Unhandled Problems</a><br>", STATUS_CGI, HOST_UNREACHABLE, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED);

		if (hosts_unreachable_active_unacknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Active</a>\n", STATUS_CGI, HOST_UNREACHABLE, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_CHECKS_ENABLED, hosts_unreachable_active_unacknowledged);
		}
		if (hosts_unreachable_passive_unacknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Passive</a>\n", STATUS_CGI, HOST_UNREACHABLE, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_PASSIVE_CHECKS_ENABLED | HOST_CHECKS_DISABLED, hosts_unreachable_passive_unacknowledged);
		}
		printf("</td></tr>\n");
	}
	if (hosts_unreachable_disabled_unacknowledged > 0)
		printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>Unacknowledged<br>%d Disabled</a></td></tr>\n", STATUS_CGI, HOST_UNREACHABLE, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED, hosts_unreachable_disabled_unacknowledged);

	if (hosts_unreachable_acknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>Acknowledged</a><br>", STATUS_CGI, HOST_UNREACHABLE, HOST_STATE_ACKNOWLEDGED | HOST_NO_SCHEDULED_DOWNTIME);

		if (hosts_unreachable_active_acknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Active</a>\n", STATUS_CGI, HOST_UNREACHABLE, HOST_STATE_ACKNOWLEDGED | HOST_NO_SCHEDULED_DOWNTIME | HOST_CHECKS_ENABLED, hosts_unreachable_active_acknowledged);
		}
		if (hosts_unreachable_passive_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Passive</a>\n", STATUS_CGI, HOST_UNREACHABLE, HOST_STATE_ACKNOWLEDGED | HOST_NO_SCHEDULED_DOWNTIME | HOST_PASSIVE_CHECKS_ENABLED | HOST_CHECKS_DISABLED, hosts_unreachable_passive_acknowledged);
		}
		if (hosts_unreachable_disabled_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a>\n", STATUS_CGI, HOST_UNREACHABLE, HOST_STATE_ACKNOWLEDGED | HOST_NO_SCHEDULED_DOWNTIME | HOST_PASSIVE_CHECKS_DISABLED | HOST_CHECKS_DISABLED, hosts_unreachable_disabled_acknowledged);
		}
		printf("</td></tr>\n");
	}

	if (hosts_unreachable_scheduled > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>Scheduled Downtime</a><br>", STATUS_CGI, HOST_UNREACHABLE, HOST_SCHEDULED_DOWNTIME);

		if (hosts_unreachable_active_scheduled > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Active</a>\n", STATUS_CGI, HOST_UNREACHABLE, HOST_SCHEDULED_DOWNTIME | HOST_CHECKS_ENABLED, hosts_unreachable_active_scheduled);
		}
		if (hosts_unreachable_passive_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Passive</a>\n", STATUS_CGI, HOST_UNREACHABLE, HOST_SCHEDULED_DOWNTIME | HOST_PASSIVE_CHECKS_ENABLED | HOST_CHECKS_DISABLED, hosts_unreachable_passive_scheduled);
		}
		if (hosts_unreachable_disabled_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a>\n", STATUS_CGI, HOST_UNREACHABLE, HOST_SCHEDULED_DOWNTIME | HOST_PASSIVE_CHECKS_DISABLED | HOST_CHECKS_DISABLED, hosts_unreachable_disabled_scheduled);
		}
		printf("</td></tr>\n");
	}

	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");


	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (hosts_up_disabled > 0)
		printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, HOST_UP, HOST_PASSIVE_CHECKS_DISABLED | HOST_CHECKS_DISABLED, hosts_up_disabled);

	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");


	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (hosts_pending_disabled > 0)
		printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?host=all&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, HOST_PENDING, HOST_PASSIVE_CHECKS_DISABLED | HOST_CHECKS_DISABLED, hosts_pending_disabled);

	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");

	printf("</p>\n");



	/******* SERVICES ********/

	printf("<p>\n");

	printf("<table class='tac' width=641 cellspacing=4 cellpadding=0 border=0>\n");

	printf("<tr><td colspan=5 height=20 class='serviceTitle'>&nbsp;Services</td></tr>\n");

	printf("<tr>\n");
	printf("<td class='serviceHeader' width=125><a href='%s?host=all&style=detail&servicestatustypes=%d' class='serviceHeader'>%d Critical</a></td>\n", STATUS_CGI, SERVICE_CRITICAL, services_critical + services_critical_host_down);
	printf("<td class='serviceHeader' width=125><a href='%s?host=all&style=detail&servicestatustypes=%d' class='serviceHeader'>%d Warning</a></td>\n", STATUS_CGI, SERVICE_WARNING, services_warning + services_warning_host_down);
	printf("<td class='serviceHeader' width=125><a href='%s?host=all&style=detail&servicestatustypes=%d' class='serviceHeader'>%d Unknown</a></td>\n", STATUS_CGI, SERVICE_UNKNOWN, services_unknown + services_unknown_host_down);
	printf("<td class='serviceHeader' width=125><a href='%s?host=all&style=detail&servicestatustypes=%d' class='serviceHeader'>%d Ok</a></td>\n", STATUS_CGI, SERVICE_OK, services_ok + services_ok_host_down);
	printf("<td class='serviceHeader' width=125><a href='%s?host=all&style=detail&servicestatustypes=%d' class='serviceHeader'>%d Pending</a></td>\n", STATUS_CGI, SERVICE_PENDING, services_pending + services_pending_host_down);
	printf("</tr>\n");

	printf("<tr>\n");


	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;&nbsp;&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (services_critical_active_unacknowledged + services_critical_passive_unacknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='serviceImportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Unhandled Problems</a><br>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED);

		if (services_critical_active_unacknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_ENABLED, services_critical_active_unacknowledged);
		}
		if (services_critical_passive_unacknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_critical_passive_unacknowledged);
		}
		printf("</td></tr>\n");
	}

	if (services_critical_disabled_unacknowledged > 0)
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Unacknowledged<br>%d Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_critical_disabled_unacknowledged);

	if (services_critical_acknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Acknowledged</a><br>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME);

		if (services_critical_active_acknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_critical_active_acknowledged);
		}
		if (services_critical_passive_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_critical_passive_acknowledged);
		}
		if (services_critical_disabled_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_critical_disabled_acknowledged);
		}
		printf("</td></tr>\n");
	}

	if (services_critical_scheduled > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Scheduled Downtime</a><br>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME);

		if (services_critical_active_scheduled > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_critical_active_scheduled);
		}
		if (services_critical_passive_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_critical_passive_scheduled);
		}
		if (services_critical_disabled_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_critical_disabled_scheduled);
		}
		printf("</td></tr>\n");
	}

	if (services_critical_unacknowledged_host_down + services_critical_acknowledged_host_down + services_critical_disabled_host_down > 0) {

		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d'>on Problem Hosts</a><br>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE);

		if (services_critical_unacknowledged_host_down > 0) {

			problem_found = FALSE;
			printf("<div class='tac_break'></div><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Unacknowledged</a><br>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED);

			if (services_critical_active_unacknowledged_host_down > 0) {
				problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_ENABLED, services_critical_active_unacknowledged_host_down);
			}
			if (services_critical_passive_unacknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_critical_passive_unacknowledged_host_down);
			}
			if (services_critical_disabled_unacknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_critical_disabled_unacknowledged_host_down);
			}
		}

		if (services_critical_acknowledged_host_down > 0) {

			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<div class='tac_break'></div><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Acknowledged</a><br>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_ACKNOWLEDGED);
			problem_found = FALSE;

			if (services_critical_active_acknowledged_host_down > 0) {
				problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_critical_active_acknowledged_host_down);
			}
			if (services_critical_passive_acknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_critical_passive_acknowledged_host_down);
			}
			if (services_critical_disabled_acknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_critical_disabled_acknowledged_host_down);
			}
		}

		if (services_critical_scheduled_host_down > 0) {

			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<div class='tac_break'></div><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Scheduled Downtime</a><br>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME);
			problem_found = FALSE;

			if (services_critical_active_scheduled_host_down > 0) {
				problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_critical_active_scheduled_host_down);
			}
			if (services_critical_passive_scheduled_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_critical_passive_scheduled_host_down);
			}
			if (services_critical_disabled_scheduled_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_critical_disabled_scheduled_host_down);
			}
		}
		printf("</td></tr>\n");
	}

	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");



	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (services_warning_active_unacknowledged + services_warning_passive_unacknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='serviceImportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Unhandled Problems</a><br>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED);

		if (services_warning_active_unacknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_ENABLED, services_warning_active_unacknowledged);
		}
		if (services_warning_passive_unacknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_warning_passive_unacknowledged);
		}
		printf("</td></tr>\n");
	}

	if (services_warning_disabled_unacknowledged > 0)
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Unacknowledged<br>%d Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_warning_disabled_unacknowledged);

	if (services_warning_acknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Acknowledged</a><br>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME);

		if (services_warning_active_acknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_warning_active_acknowledged);
		}
		if (services_warning_passive_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_warning_passive_acknowledged);
		}
		if (services_warning_disabled_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_warning_disabled_acknowledged);
		}
		printf("</td></tr>\n");
	}

	if (services_warning_scheduled > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Scheduled Downtime</a><br>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME);

		if (services_warning_active_scheduled > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_warning_active_scheduled);
		}
		if (services_warning_passive_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_warning_passive_scheduled);
		}
		if (services_warning_disabled_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_warning_disabled_scheduled);
		}
		printf("</td></tr>\n");
	}

	if (services_warning_unacknowledged_host_down + services_warning_acknowledged_host_down + services_warning_disabled_host_down > 0) {

		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d'>on Problem Hosts</a><br>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE);

		if (services_warning_unacknowledged_host_down > 0) {

			problem_found = FALSE;
			printf("<div class='tac_break'></div><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Unacknowledged</a><br>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED);

			if (services_warning_active_unacknowledged_host_down > 0) {
				problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_ENABLED, services_warning_active_unacknowledged_host_down);
			}
			if (services_warning_passive_unacknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_warning_passive_unacknowledged_host_down);
			}
			if (services_warning_disabled_unacknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_warning_disabled_unacknowledged_host_down);
			}
		}

		if (services_warning_acknowledged_host_down > 0) {

			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<div class='tac_break'></div><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Acknowledged</a><br>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_ACKNOWLEDGED);
			problem_found = FALSE;

			if (services_warning_active_acknowledged_host_down > 0) {
				problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_warning_active_acknowledged_host_down);
			}
			if (services_warning_passive_acknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_warning_passive_acknowledged_host_down);
			}
			if (services_warning_disabled_acknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_warning_disabled_acknowledged_host_down);
			}
		}

		if (services_warning_scheduled_host_down > 0) {

			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<div class='tac_break'></div><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Scheduled Downtime</a><br>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME);
			problem_found = FALSE;

			if (services_warning_active_scheduled_host_down > 0) {
				problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_warning_active_scheduled_host_down);
			}
			if (services_warning_passive_scheduled_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_warning_passive_scheduled_host_down);
			}
			if (services_warning_disabled_scheduled_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_warning_disabled_scheduled_host_down);
			}
		}
		printf("</td></tr>\n");
	}

	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");



	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (services_unknown_active_unacknowledged + services_unknown_passive_unacknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='serviceImportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Unhandled Problems</a><br>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED);

		if (services_unknown_active_unacknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_ENABLED, services_unknown_active_unacknowledged);
		}
		if (services_unknown_passive_unacknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_unknown_passive_unacknowledged);
		}
		printf("</td></tr>\n");
	}

	if (services_unknown_disabled_unacknowledged > 0)
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Unacknowledged<br>%d Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_unknown_disabled_unacknowledged);

	if (services_unknown_acknowledged > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Acknowledged</a><br>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME);

		if (services_unknown_active_acknowledged > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_unknown_active_acknowledged);
		}
		if (services_unknown_passive_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_unknown_passive_acknowledged);
		}
		if (services_unknown_disabled_acknowledged > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_unknown_disabled_acknowledged);
		}
		printf("</td></tr>\n");
	}

	if (services_unknown_scheduled > 0) {

		problem_found = FALSE;
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Scheduled Downtime</a><br>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME);

		if (services_unknown_active_scheduled > 0) {
			problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_unknown_active_scheduled);
		}
		if (services_unknown_passive_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_unknown_passive_scheduled);
		}
		if (services_unknown_disabled_scheduled > 0) {
			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_unknown_disabled_scheduled);
		}
		printf("</td></tr>\n");
	}

	if (services_unknown_unacknowledged_host_down + services_unknown_acknowledged_host_down + services_unknown_disabled_host_down > 0) {

		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d'>on Problem Hosts</a><br>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE);

		if (services_unknown_unacknowledged_host_down > 0) {

			problem_found = FALSE;
			printf("<div class='tac_break'></div><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Unacknowledged</a><br>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED);

			if (services_unknown_active_unacknowledged_host_down > 0) {
				problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_ENABLED, services_unknown_active_unacknowledged_host_down);
			}
			if (services_unknown_passive_unacknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_unknown_passive_unacknowledged_host_down);
			}
			if (services_unknown_disabled_unacknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_unknown_disabled_unacknowledged_host_down);
			}
		}

		if (services_unknown_acknowledged_host_down > 0) {

			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<div class='tac_break'></div><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Acknowledged</a><br>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_ACKNOWLEDGED);
			problem_found = FALSE;

			if (services_unknown_active_acknowledged_host_down > 0) {
				problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_unknown_active_acknowledged_host_down);
			}
			if (services_unknown_passive_acknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_unknown_passive_acknowledged_host_down);
			}
			if (services_unknown_disabled_acknowledged_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_STATE_ACKNOWLEDGED | SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_unknown_disabled_acknowledged_host_down);
			}
		}

		if (services_unknown_scheduled_host_down > 0) {

			if (problem_found == TRUE) printf("<br>");
			else problem_found = TRUE;
			printf("<div class='tac_break'></div><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>Scheduled Downtime</a><br>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME);
			problem_found = FALSE;

			if (services_unknown_active_scheduled_host_down > 0) {
				problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Active</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_ENABLED, services_unknown_active_scheduled_host_down);
			}
			if (services_unknown_passive_scheduled_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Passive</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, services_unknown_passive_scheduled_host_down);
			}
			if (services_unknown_disabled_scheduled_host_down > 0) {
				if (problem_found == TRUE) printf("<br>");
				else problem_found = TRUE;
				printf("<a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a>\n", STATUS_CGI, SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, SERVICE_SCHEDULED_DOWNTIME | SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_unknown_disabled_scheduled_host_down);
			}
		}
		printf("</td></tr>\n");
	}

	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");




	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (services_ok_disabled > 0)
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_OK, HOST_UP | HOST_PENDING, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_ok_disabled);
	if (services_ok_disabled_host_down > 0)
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled on Problem Hosts</a></td></tr>\n", STATUS_CGI, SERVICE_OK, HOST_DOWN | HOST_UNREACHABLE, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_ok_disabled_host_down);


	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");



	printf("<td valign=top>\n");
	printf("<table border=0 width=125 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=bottom width=25>&nbsp;</td>\n");
	printf("<Td width=10>&nbsp;</td>\n");

	printf("<td valign=top width=100%%>\n");
	printf("<table border=0 width=100%%>\n");

	if (services_pending_disabled > 0)
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_PENDING, HOST_UP | HOST_PENDING, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_pending_disabled);
	if (services_pending_disabled_host_down > 0)
		printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?host=all&type=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Disabled on Problem Hosts</a></td></tr>\n", STATUS_CGI, SERVICE_PENDING, HOST_DOWN | HOST_UNREACHABLE, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_pending_disabled_host_down);

	printf("</table>\n");
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");



	printf("</tr>\n");
	printf("</table>\n");

	printf("</p>\n");



	/******** CHECKS *********/

	printf("<p>\n");

	printf("<table class='tac' cellspacing=4 cellpadding=0 border=0>\n");

	printf("<tr><td colspan=2 height=20 class='featureTitle'>&nbsp;Service Checks</td>\n");
	printf("<td colspan=2 height=20 class='featureTitle'>&nbsp;Host Checks</td></tr>\n");

	printf("<tr>\n");
	printf("<td class='featureHeader' width=135>Active</td>\n");
	printf("<td class='featureHeader' width=135>Passive</td>\n");
	printf("<td class='featureHeader' width=135>Active</td>\n");
	printf("<td class='featureHeader' width=135>Passive</td>\n");

	printf("</tr>\n");

	printf("<tr>\n");


	/******* Service Checks ******/
	printf("<td valign=top>\n");
	printf("<table border=0 width=135 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=top><a href='%s?cmd_typ=%d'><img src='%s%s' border='0' alt='Active Service Checks %s' title='Active Service Checks %s'></a></td>\n", CMD_CGI, (execute_service_checks == TRUE) ? CMD_STOP_EXECUTING_SVC_CHECKS : CMD_START_EXECUTING_SVC_CHECKS, url_images_path, (execute_service_checks == TRUE) ? TAC_ENABLED_ICON : TAC_DISABLED_ICON, (execute_service_checks == TRUE) ? "Enabled" : "Disabled", (execute_service_checks == TRUE) ? "Enabled" : "Disabled");
	printf("<Td width=10>&nbsp;</td>\n");
	if (execute_service_checks == TRUE) {
		printf("<Td valign=top width=100%% class='EnabledActiveServiceChecks'>\n");
		printf("<table border=0 width=100%%>\n");

		if (total_active_service_checks > 0)
			printf("<tr><td width=100%% class='ItemActiveServiceChecks'><a href='%s?host=all&type=detail&serviceprops=%d'>%d Enabled</a></td></tr>\n", STATUS_CGI, SERVICE_CHECKS_ENABLED, total_active_service_checks);
		else
			printf("<tr><td width=100%% class='ItemActiveServiceChecks'>No Active Checks</td></tr>\n");

		if (total_active_service_checks_with_passive_disabled > 0)
			printf("<tr><td width=100%% class='ItemActiveServiceChecksWithPassiveDisabled'><a href='%s?host=all&type=detail&serviceprops=%d'>%d with Passive Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_CHECKS_ENABLED | SERVICE_PASSIVE_CHECKS_DISABLED, total_active_service_checks_with_passive_disabled);

		printf("</table>\n");
		printf("</td>\n");
	} else
		printf("<Td valign=center width=100%% class='DisabledActiveServiceChecks'>N/A</td>\n");
	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");


	printf("<td valign=top>\n");
	printf("<table border=0 width=135 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=top><a href='%s?cmd_typ=%d'><img src='%s%s' border='0' alt='Passive Checks %s' title='Passive Checks %s'></a></td>\n", CMD_CGI, (accept_passive_service_checks == TRUE) ? CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS : CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS, url_images_path, (accept_passive_service_checks == TRUE) ? TAC_ENABLED_ICON : TAC_DISABLED_ICON, (accept_passive_service_checks == TRUE) ? "Enabled" : "Disabled", (accept_passive_service_checks == TRUE) ? "Enabled" : "Disabled");
	printf("<Td width=10>&nbsp;</td>\n");
	if (accept_passive_service_checks == TRUE) {

		printf("<td valign=top width=100%% class='EnabledPassiveServiceChecks'>\n");
		printf("<table border=0 width=100%%>\n");

		if (total_passive_service_checks > 0)
			printf("<tr><td width=100%% class='ItemPassiveServiceChecks'><a href='%s?host=all&type=detail&serviceprops=%d'>%d Enabled</a></td></tr>\n", STATUS_CGI, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_ENABLED, total_passive_service_checks);
		else
			printf("<tr><td width=100%% class='ItemPassiveServiceChecks'>No Passive Checks</td></tr>\n");

		if (total_disabled_service_checks > 0)
			printf("<tr><td width=100%% class='ItemDisabledServiceChecks'><a href='%s?host=all&type=detail&serviceprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, total_disabled_service_checks);

		printf("</table>\n");
		printf("</td>\n");
	} else
		printf("<Td valign=center width=100%% class='DisabledPassiveServiceChecks'>N/A</td>\n");
	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");


	/******* Host Checks ******/
	printf("<td valign=top>\n");
	printf("<table border=0 width=135 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=top><a href='%s?cmd_typ=%d'><img src='%s%s' border='0' alt='Active Host Checks %s' title='Active Host Checks %s'></a></td>\n", CMD_CGI, (execute_host_checks == TRUE) ? CMD_STOP_EXECUTING_HOST_CHECKS : CMD_START_EXECUTING_HOST_CHECKS, url_images_path, (execute_host_checks == TRUE) ? TAC_ENABLED_ICON : TAC_DISABLED_ICON, (execute_host_checks == TRUE) ? "Enabled" : "Disabled", (execute_host_checks == TRUE) ? "Enabled" : "Disabled");
	printf("<Td width=10>&nbsp;</td>\n");
	if (execute_host_checks == TRUE) {
		printf("<Td valign=top width=100%% class='EnabledActiveHostChecks'>\n");
		printf("<table border=0 width=100%%>\n");

		if (total_active_host_checks > 0)
			printf("<tr><td width=100%% class='ItemActiveHostChecks'><a href='%s?host=all&style=hostdetail&hostprops=%d'>%d Enabled</a></td></tr>\n", STATUS_CGI, HOST_CHECKS_ENABLED, total_active_host_checks);
		else
			printf("<tr><td width=100%% class='ItemActiveHostChecks'>No Active Checks</td></tr>\n");

		if (total_active_host_checks_with_passive_disabled > 0)
			printf("<tr><td width=100%% class='ItemActiveHostChecksWithPassiveDisabled'><a href='%s?host=all&style=hostdetail&hostprops=%d'>%d with Passive Disabled</a></td></tr>\n", STATUS_CGI, HOST_CHECKS_ENABLED | HOST_PASSIVE_CHECKS_DISABLED, total_active_host_checks_with_passive_disabled);

		printf("</table>\n");
		printf("</td>\n");
	} else
		printf("<Td valign=center width=100%% class='DisableActiveHostChecks'>N/A</td>\n");
	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");


	printf("<td valign=top>\n");
	printf("<table border=0 width=135 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=top><a href='%s?cmd_typ=%d'><img src='%s%s' border='0' alt='Passive Host Checks %s' title='Passive Host Checks %s'></a></td>\n", CMD_CGI, (accept_passive_host_checks == TRUE) ? CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS : CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS, url_images_path, (accept_passive_host_checks == TRUE) ? TAC_ENABLED_ICON : TAC_DISABLED_ICON, (accept_passive_host_checks == TRUE) ? "Enabled" : "Disabled", (accept_passive_host_checks == TRUE) ? "Enabled" : "Disabled");
	printf("<Td width=10>&nbsp;</td>\n");
	if (accept_passive_host_checks == TRUE) {
		printf("<Td valign=top width=100%% class='EnabledPassiveHostChecks'>\n");
		printf("<table border=0 width=100%%>\n");

		if (total_passive_host_checks > 0)
			printf("<tr><td width=100%% class='ItemPassiveHostChecks'><a href='%s?host=all&style=hostdetail&hostprops=%d'>%d Enabled</a></td></tr>\n", STATUS_CGI, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_ENABLED, total_passive_host_checks);
		else
			printf("<tr><td width=100%% class='ItemPassiveHostChecks'>No Passive Checks</td></tr>\n");

		if (total_disabled_host_checks > 0)
			printf("<tr><td width=100%% class='ItemDisabledHostChecks'><a href='%s?host=all&style=hostdetail&hostprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, HOST_PASSIVE_CHECKS_DISABLED | HOST_CHECKS_DISABLED, total_disabled_host_checks);

		printf("</table>\n");
		printf("</td>\n");
	} else
		printf("<Td valign=center width=100%% class='DisabledPassiveHostChecks'>N/A</td>\n");
	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");


	printf("</tr>\n");

	printf("</table>\n");

	printf("</p>\n");



	/******* MONITORING FEATURES ********/

	printf("<p>\n");

	printf("<table class='tac' cellspacing=4 cellpadding=0 border=0>\n");

	printf("<tr><td colspan=3 height=20 class='featureTitle'>&nbsp;Monitoring Features</td></tr>\n");

	printf("<tr>\n");
	printf("<td class='featureHeader' width=135>Flap Detection</td>\n");
	printf("<td class='featureHeader' width=135>Notifications</td>\n");
	printf("<td class='featureHeader' width=135>Event Handlers</td>\n");
	printf("</tr>\n");

	printf("<tr>\n");

	printf("<td valign=top>\n");
	printf("<table border=0 width=135 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=top><a href='%s?cmd_typ=%d'><img src='%s%s' border=0 alt='Flap Detection %s' title='Flap Detection %s'></a></td>\n", CMD_CGI, (enable_flap_detection == TRUE) ? CMD_DISABLE_FLAP_DETECTION : CMD_ENABLE_FLAP_DETECTION, url_images_path, (enable_flap_detection == TRUE) ? TAC_ENABLED_ICON : TAC_DISABLED_ICON, (enable_flap_detection == TRUE) ? "Enabled" : "Disabled", (enable_flap_detection == TRUE) ? "Enabled" : "Disabled");
	printf("<Td width=10>&nbsp;</td>\n");
	if (enable_flap_detection == TRUE) {
		printf("<Td valign=top width=100%% class='featureEnabledFlapDetection'>\n");
		printf("<table border=0 width=100%%>\n");

		if (flap_disabled_services > 0)
			printf("<tr><td width=100%% class='featureItemDisabledServiceFlapDetection'><a href='%s?host=all&type=detail&serviceprops=%d'>%d Service%s Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_FLAP_DETECTION_DISABLED, flap_disabled_services, (flap_disabled_services == 1) ? "" : "s");
		else
			printf("<tr><td width=100%% class='featureItemEnabledServiceFlapDetection'>All Services Enabled</td></tr>\n");

		if (flapping_services > 0)
			printf("<tr><td width=100%% class='featureItemServicesFlapping'><a href='%s?host=all&type=detail&serviceprops=%d'>%d Service%s Flapping</a></td></tr>\n", STATUS_CGI, SERVICE_IS_FLAPPING, flapping_services, (flapping_services == 1) ? "" : "s");
		else
			printf("<tr><td width=100%% class='featureItemServicesNotFlapping'>No Services Flapping</td></tr>\n");

		if (flap_disabled_hosts > 0)
			printf("<tr><td width=100%% class='featureItemDisabledHostFlapDetection'><a href='%s?host=all&style=hostdetail&hostprops=%d'>%d Host%s Disabled</a></td></tr>\n", STATUS_CGI, HOST_FLAP_DETECTION_DISABLED, flap_disabled_hosts, (flap_disabled_hosts == 1) ? "" : "s");
		else
			printf("<tr><td width=100%% class='featureItemEnabledHostFlapDetection'>All Hosts Enabled</td></tr>\n");

		if (flapping_hosts > 0)
			printf("<tr><td width=100%% class='featureItemHostsFlapping'><a href='%s?host=all&style=hostdetail&hostprops=%d'>%d Host%s Flapping</a></td></tr>\n", STATUS_CGI, HOST_IS_FLAPPING, flapping_hosts, (flapping_hosts == 1) ? "" : "s");
		else
			printf("<tr><td width=100%% class='featureItemHostsNotFlapping'>No Hosts Flapping</td></tr>\n");

		printf("</table>\n");
		printf("</td>\n");
	} else
		printf("<Td valign=center width=100%% class='featureDisabledFlapDetection'>N/A</td>\n");
	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");


	printf("<td valign=top>\n");
	printf("<table border=0 width=135 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=top><a href='%s?cmd_typ=%d'><img src='%s%s' border=0 alt='Notifications %s' title='Notifications %s'></a></td>\n", CMD_CGI, (enable_notifications == TRUE) ? CMD_DISABLE_NOTIFICATIONS : CMD_ENABLE_NOTIFICATIONS, url_images_path, (enable_notifications == TRUE) ? TAC_ENABLED_ICON : TAC_DISABLED_ICON, (enable_notifications == TRUE) ? "Enabled" : "Disabled", (enable_notifications == TRUE) ? "Enabled" : "Disabled");
	printf("<Td width=10>&nbsp;</td>\n");
	if (enable_notifications == TRUE) {
		printf("<Td valign=top width=100%% class='featureEnabledNotifications'>\n");
		printf("<table border=0 width=100%%>\n");

		if (notification_disabled_services > 0)
			printf("<tr><td width=100%% class='featureItemDisabledServiceNotifications'><a href='%s?host=all&type=detail&serviceprops=%d'>%d Service%s Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_NOTIFICATIONS_DISABLED, notification_disabled_services, (notification_disabled_services == 1) ? "" : "s");
		else
			printf("<tr><td width=100%% class='featureItemEnabledServiceNotifications'>All Services Enabled</td></tr>\n");

		if (notification_disabled_hosts > 0)
			printf("<tr><td width=100%% class='featureItemDisabledHostNotifications'><a href='%s?host=all&style=hostdetail&hostprops=%d'>%d Host%s Disabled</a></td></tr>\n", STATUS_CGI, HOST_NOTIFICATIONS_DISABLED, notification_disabled_hosts, (notification_disabled_hosts == 1) ? "" : "s");
		else
			printf("<tr><td width=100%% class='featureItemEnabledHostNotifications'>All Hosts Enabled</td></tr>\n");

		printf("</table>\n");
		printf("</td>\n");
	} else
		printf("<Td valign=center width=100%% class='featureDisabledNotifications'>N/A</td>\n");
	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");



	printf("<td valign=top>\n");
	printf("<table border=0 width=135 cellspacing=0 cellpadding=0>\n");
	printf("<tr>\n");
	printf("<td valign=top><a href='%s?cmd_typ=%d'><img src='%s%s' border=0 alt='Event Handlers %s' title='Event Handlers %s'></a></td>\n", CMD_CGI, (enable_event_handlers == TRUE) ? CMD_DISABLE_EVENT_HANDLERS : CMD_ENABLE_EVENT_HANDLERS, url_images_path, (enable_event_handlers == TRUE) ? TAC_ENABLED_ICON : TAC_DISABLED_ICON, (enable_event_handlers == TRUE) ? "Enabled" : "Disabled", (enable_event_handlers == TRUE) ? "Enabled" : "Disabled");
	printf("<Td width=10>&nbsp;</td>\n");
	if (enable_event_handlers == TRUE) {
		printf("<Td valign=top width=100%% class='featureEnabledHandlers'>\n");
		printf("<table border=0 width=100%%>\n");

		if (event_handler_disabled_services > 0)
			printf("<tr><td width=100%% class='featureItemDisabledServiceHandlers'><a href='%s?host=all&type=detail&serviceprops=%d'>%d Service%s Disabled</a></td></tr>\n", STATUS_CGI, SERVICE_EVENT_HANDLER_DISABLED, event_handler_disabled_services, (event_handler_disabled_services == 1) ? "" : "s");
		else
			printf("<tr><td width=100%% class='featureItemEnabledServiceHandlers'>All Services Enabled</td></tr>\n");

		if (event_handler_disabled_hosts > 0)
			printf("<tr><td width=100%% class='featureItemDisabledHostHandlers'><a href='%s?host=all&style=hostdetail&hostprops=%d'>%d Host%s Disabled</a></td></tr>\n", STATUS_CGI, HOST_EVENT_HANDLER_DISABLED, event_handler_disabled_hosts, (event_handler_disabled_hosts == 1) ? "" : "s");
		else
			printf("<tr><td width=100%% class='featureItemEnabledHostHandlers'>All Hosts Enabled</td></tr>\n");

		printf("</table>\n");
		printf("</td>\n");
	} else
		printf("<Td valign=center width=100%% class='featureDisabledHandlers'>N/A</td>\n");
	printf("</tr>\n");
	printf("</table>\n");
	printf("</td>\n");


	printf("</tr>\n");

	printf("</table>\n");

	printf("</p>\n");


	return;
}

