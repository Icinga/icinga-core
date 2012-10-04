/**************************************************************************
 *
 * STATUS.C -  Icinga Status CGI
 *
 * Copyright (c) 1999-2010 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2012 Nagios Core Development Team and Community Contributors
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 * Last Modified: 08-08-2010
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

/** @file status.c
 *  @brief display host and service status data in list format. Also host and service groups
**/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/comments.h"
#include "../include/macros.h"
#include "../include/statusdata.h"

#include "../include/cgiutils.h"
#include "../include/getcgi.h"
#include "../include/cgiauth.h"

/** @name initializing macros
    @{ **/
static icinga_macros *mac;
/** @} */

/** @name external vars
    @{ **/
extern time_t	       program_start;

extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];
extern char url_logo_images_path[MAX_FILENAME_LENGTH];
extern char url_media_path[MAX_FILENAME_LENGTH];

extern char *service_critical_sound;
extern char *service_warning_sound;
extern char *service_unknown_sound;
extern char *host_down_sound;
extern char *host_unreachable_sound;
extern char *normal_sound;

extern char *notes_url_target;
extern char *action_url_target;

extern char *csv_delimiter;
extern char *csv_data_enclosure;

extern int enable_splunk_integration;
extern int status_show_long_plugin_output;
extern int suppress_maintenance_downtime;
extern int highlight_table_rows;
extern int tab_friendly_titles;

extern int refresh;
extern int result_limit;
extern int embedded;
extern int display_header;
extern int display_status_totals;
extern int daemon_check;
extern int content_type;
extern int escape_html_tags;
extern int show_partial_hostgroups;			/**< show any hosts in hostgroups the user is authorized for */

extern int add_notif_num_hard;
extern int add_notif_num_soft;

extern host *host_list;
extern service *service_list;
extern hostgroup *hostgroup_list;
extern servicegroup *servicegroup_list;
extern hoststatus *hoststatus_list;
extern servicestatus *servicestatus_list;
/** @} */

/** @name DISPLAY TYPES
 @{**/
#define DISPLAY_HOSTS			0		/**< use the standard view */
#define DISPLAY_HOSTGROUPS		1		/**< output is filtered by hostgroup(s)  */
#define DISPLAY_SERVICEGROUPS		2		/**< output is filtered by servicegroup(s)  */
/** @} */

/** @name STYLES
 @{**/
#define STYLE_OVERVIEW			0		/**< host/service group status overview (no status list) */
#define STYLE_SERVICE_DETAIL		1		/**< display service status list */
#define STYLE_SUMMARY			2		/**< host/service group status summary (no status list) */
#define STYLE_GRID			3		/**< host/service group status grid (no status list) */
#define STYLE_HOST_DETAIL		4		/**< display host status list */
#define STYLE_HOST_SERVICE_DETAIL	5		/**< display combined hast and service status list */
/** @} */

/** @name STATUS TYPES
 *	used in statusdata_struct and to determine which dropdown menu to display
 @{**/
#define HOST_STATUS			0
#define SERVICE_STATUS			1
#define NO_STATUS			2
/** @} */

/** @name STATUS ADDED / COUNTED
 *	used in statusdata to see if this antry has been added to statusdata_struct or counted for status totals
 @{**/
#define STATUS_NOT_DISPLAYED		0
#define STATUS_ADDED			1
#define STATUS_COUNTED_UNFILTERED	2
#define STATUS_COUNTED_FILTERED		4
#define STATUS_BELONGS_TO_SG		8
#define STATUS_BELONGS_TO_HG		16
/** @} */

/** @name NUMBER OF NAMED OBJECTS
 @{**/
#define NUM_NAMED_ENTRIES		1000		/**< max number of elements (hosts/hostgroups/servicegroups) submitted  vie GET/POST */
/** @} */

/** @brief status data struct
 *
 *  structure to hold host AND service status data
**/
typedef struct statusdata_struct {
	int		type;				/**< HOST_STATUS / SERVICE_STATUS */
	char		*host_name;			/**< holds host name */
	char		*svc_description;		/**< holds service description */
	int		status;				/**< the actual status OK / UP / CRITICAL / DOWN ... */
	char		*status_string;			/**< the status as strting depending if host or service status */
	char		*last_check;			/**< last time status is checked as string */
	time_t		ts_last_check;			/**< last time status is checked as timestamp */
	char		*state_duration;		/**< duration of this status as string */
	time_t		ts_state_duration;		/**< duration of this status as timestamp */
	char		*attempts;			/**< attempts as string */
	int		current_attempt;		/**< attempts as integer */
	char		*plugin_output;			/**< full processed plugin output */
	int		problem_has_been_acknowledged;	/**< bool if problem is acknowledged */
	int		scheduled_downtime_depth;	/**< int of downtime depth */
	int		notifications_enabled;		/**< bool if notifications are enabled */
	int		checks_enabled;			/**< bool if active checks are enabled */
	int		accept_passive_checks;		/**< bool if passive checks are enabled */
	int		is_flapping;			/**< bool if status is flapping */
	int		state_type;			/**< type of state HARD_STATE / SOFT_STATE */
	struct statusdata_struct *next;			/**< next statusdata */
} statusdata;

statusdata *statusdata_list = NULL;			/**< list of all status data elements */
statusdata *last_statusdata = NULL;			/**< last element of status data list (needed to add new elments) */

/** @brief status sort structure
 *
 *  holds pointers to status data in a sorted order
**/
typedef struct sort_struct {
	statusdata *status;				/**< pointer to status data element */
	struct sort_struct *next;			/**< next sort entry */
} sort;

sort *statussort_list = NULL;				/**< list of all sorted elements */

/** @brief named list structure
 *
 *  holds an char entry. useful for host/service groups
**/
struct namedlist {
	char *entry;
};

/** @name Internal vars
    @{ **/
int num_req_hosts = 0;					/**< number of requestes hosts GET/POST */
int num_req_hostgroups = 0;				/**< number of requestes hostgroups GET/POST */
int num_req_servicegroups = 0;				/**< number of requestes servicegroups GET/POST */
int dummy = 0;						/**< dummy to catch asprintf return value */

int show_all_hosts = TRUE;				/**< define if ALL hosts should be displayed */
int show_all_hostgroups = TRUE;				/**< define if ALL hostgroups should be displayed */
int show_all_servicegroups = TRUE;			/**< define if ALL servicegroups should be displayed */
int display_type = DISPLAY_HOSTS;			/**< set default @c "DISPLAY TYPES" */
int overview_columns = 3;				/**< set default num of columns in OVERVIEW */
int max_grid_width = 8;					/**< set default grid width for OVERVIEW */
int group_style_type = STYLE_SERVICE_DETAIL;		/**< set default @c "STYLES" type */
int navbar_search = FALSE;				/**< set if user used search via menu (legacy) */
int user_is_authorized_for_statusdata = FALSE;		/**< used to see if we return a no autorised or no data error message if no data is found after filtering */
int nostatusheader_option = FALSE;			/**< define of status header should be displayed or not */
int display_all_unhandled_problems = FALSE;		/**< special view of all unhandled problems */
int display_all_problems = FALSE;			/**< special view of all problems */
int result_start = 1;					/**< keep track from where we have to start displaying results */
int get_result_limit = -1;				/**< needed to overwrite config value with result_limit we get vie GET */
int displayed_host_entries = 0;				/**< number of displayed host entries */
int displayed_service_entries = 0;			/**< number of displayed service entries */
int total_host_entries = 0;				/**< number of all host entries found */
int total_service_entries = 0;				/**< number of all service entries found */

/** bitmask of all service status types which are used to filter services, changed via GET/POST */
int service_status_types = SERVICE_PENDING | SERVICE_OK | SERVICE_UNKNOWN | SERVICE_WARNING | SERVICE_CRITICAL;

/** bitmask of all service status types to compare with filter (won't get changed) */
int all_service_status_types = SERVICE_PENDING | SERVICE_OK | SERVICE_UNKNOWN | SERVICE_WARNING | SERVICE_CRITICAL;

/** bitmask of all host status types which are used to filter hosts, changed via GET/POST */
int host_status_types = HOST_PENDING | HOST_UP | HOST_DOWN | HOST_UNREACHABLE;

/** bitmask of all host status types to compare with filter (won't get changed) */
int all_host_status_types = HOST_PENDING | HOST_UP | HOST_DOWN | HOST_UNREACHABLE;

/** bitmask of all problem service status types to compare with filter */
int all_service_problems = SERVICE_UNKNOWN | SERVICE_WARNING | SERVICE_CRITICAL;

/** bitmask of all problem host status types to compare with filter */
int all_host_problems = HOST_DOWN | HOST_UNREACHABLE;

/** bitmask of all unhandled problem host status types to compare with filter */
int host_problems_unhandled = HOST_NO_SCHEDULED_DOWNTIME | HOST_NOT_ALL_CHECKS_DISABLED | HOST_STATE_UNACKNOWLEDGED;

/** bitmask of all unhandled problem service status types to compare with filter */
int service_problems_unhandled = SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_NOT_ALL_CHECKS_DISABLED | SERVICE_STATE_UNACKNOWLEDGED;

unsigned long host_properties = 0L;			/**< bitmask of host property filter */
unsigned long service_properties = 0L;			/**< bitmask of service property filter */

int sort_type = SORT_NONE;				/**< defines sort order  */
int sort_option = SORT_HOSTNAME;			/**< defines after which column is sorted */
int sort_object = SERVICE_STATUS;			/**< defines if service or hoststatus is sorted */

/** @name status data counters vars
    @{ **/
int problem_hosts_down = 0;				/**< num of hosts down which are not already handled to determine if sound shoud be played */
int problem_hosts_unreachable = 0;			/**< num of hosts unreachable which are not already handled to determine if sound shoud be played */
int problem_services_critical = 0;			/**< num of services critical which are not already handled to determine if sound shoud be played */
int problem_services_warning = 0;			/**< num of services warning which are not already handled to determine if sound shoud be played */
int problem_services_unknown = 0;			/**< num of services unknown which are not already handled to determine if sound shoud be played */

int num_hosts_up = 0;					/**< num of hosts up (for @ref show_host_status_totals) */
int num_hosts_down = 0;					/**< num of hosts down (for @ref show_host_status_totals) */
int num_hosts_unreachable = 0;				/**< num of hosts unreachable (for @ref show_host_status_totals) */
int num_hosts_pending = 0;				/**< num of hosts pending (for @ref show_host_status_totals) */

int num_total_hosts_up = 0;				/**< num of total hosts up (for @ref show_host_status_totals) */
int num_total_hosts_down = 0;				/**< num of total hosts down (for @ref show_host_status_totals) */
int num_total_hosts_unreachable = 0;			/**< num of total hosts unreachable (for @ref show_host_status_totals) */
int num_total_hosts_pending = 0;			/**< num of total hosts pending (for @ref show_host_status_totals) */

int num_services_ok = 0;				/**< num of services ok (for @ref show_host_status_totals) */
int num_services_warning = 0;				/**< num of services warning (for @ref show_host_status_totals) */
int num_services_critical = 0;				/**< num of services critical (for @ref show_host_status_totals) */
int num_services_unknown = 0;				/**< num of services unknown (for @ref show_host_status_totals) */
int num_services_pending = 0;				/**< num of services pending (for @ref show_host_status_totals) */

int num_total_services_ok = 0;				/**< num of total services ok (for @ref show_host_status_totals) */
int num_total_services_warning = 0;			/**< num of total services warning (for @ref show_host_status_totals) */
int num_total_services_critical = 0;			/**< num of total services critical (for @ref show_host_status_totals) */
int num_total_services_unknown = 0;			/**< num of total services unknown (for @ref show_host_status_totals) */
int num_total_services_pending = 0;			/**< num of total services pending (for @ref show_host_status_totals) */
/** @} */

int CGI_ID = STATUS_CGI_ID;				/**< ID to identify the cgi for functions in cgiutils.c */

char *url_hosts_part = NULL;				/**< url containing all requested hosts (host=localhost&host=host1&host=...) */
char *url_hostgroups_part = NULL;			/**< url containing all requested hostgroups */
char *url_servicegroups_part = NULL;			/**< url containing all requested servicegroups */

char *search_string = NULL;				/**< contains search string if user searched something */
char *service_filter = NULL;				/**< contains service filter if user wants to filter service status list by a certain service */

time_t current_time;					/**< current timestamp (calculated once in main) */

authdata current_authdata;				/**< struct to hold current authentication data */

struct namedlist req_hosts[NUM_NAMED_ENTRIES];		/**< initialze list of requested hosts */
struct namedlist req_hostgroups[NUM_NAMED_ENTRIES];	/**< initialze list of requested hostgroups */
struct namedlist req_servicegroups[NUM_NAMED_ENTRIES];	/**< initialze list of requested servicegroups */
/** @} */


/** @name handling status data
    @{ **/

/** @brief adds status data to internal status data struct
 *  @param [in] status_type type of statusdata in 2. argument
 *	@arg HOST_STATUS
 *	@arg SERVICE_STATUS
 *  @param [in] data hoststatus/servicestatus datastruct pointer
 *	@retval OK
 *	@retval ERROR
 *  @return wether adding of status data was successfull or not
 *
 *  @n This function parses host/servicedata and uses one data structure for both types.
 *  Data get's added to @ref statusdata_list . It also fills the global status data counters.
**/
int add_status_data(int , void *);

/** @brief frees all memory allocated to @ref statusdata_list entries in memory **/
void free_local_status_data(void);

/** @brief sorts status data by type and option
 *  @param [in] status_type type of statusdata in 2. argument
 *	@arg HOST_STATUS
 *	@arg SERVICE_STATUS
 *  @param [in] sort_type defines sort direction
 *	@arg SORT_ASCENDING
 *	@arg SORT_DESCENDING
 *  @param [in] sort_option the field data is sorted after, see SORT OPTIONS in cgiutils.h
 *
 *	@retval OK
 *	@retval ERROR
 *  @return wether adding of status data was successfull or not
 *
 *  @n It fills @ref statussort_list with pointers of @ref statusdata_list elements in desired order.
**/
int sort_status_data(int, int, int);

/** @brief compares two status data elements by type and option
 *  @param [in] status_type type of statusdata
 *	@arg HOST_STATUS
 *	@arg SERVICE_STATUS
 *  @param [in] sort_type defines sort direction
 *	@arg SORT_ASCENDING
 *	@arg SORT_DESCENDING
 *  @param [in] sort_option the field data is sorted after, see SORT OPTIONS in cgiutils.h
 *  @param [in] new_sort first status of two to comapre
 *  @param [in] temp_sort second status of two to comapre
 *
 *	@retval TRUE
 *	@retval FALSE
 *  @return wether adding of status data was successfull or not
 *
 *  @n Is only used by @ref sort_status_data . Compares first satatus with second one
 *  and retruns TRUE or FALSE depending on sort_type and sort_option
**/
int compare_sort_entries(int, int, int, sort *, sort *);

/** @brief frees all memory allocated to @ref statussort_list entries in memory **/
void free_sort_list(void);

/** @brief check if submited host property filters matches hoststatus
 *  @param [in] temp_hoststatus to check current filter settings against
 *	@retval TRUE
 *	@retval FALSE
 *  @return wether filter was passed or not
**/
int passes_host_properties_filter(hoststatus *);

/** @brief check if submited service property filters matches servicestatus
 *  @param [in] temp_servicestatus to check current filter settings against
 *	@retval TRUE
 *	@retval FALSE
 *  @return wether filter was passed or not
**/
int passes_service_properties_filter(servicestatus *);
/** @} */


/** @name status counter tables
    @{ **/

/** @brief shows table with status counters for hosts **/
void show_host_status_totals(void);

/** @brief shows table with status counters for services **/
void show_service_status_totals(void);
/** @} */


/** @name functions to display the actual status data
    @{ **/

/** @brief Display a detailed listing of all services states
 *
 *  This is the service status list which is the default view.
 *  A list of services and their state.
**/
void show_service_detail(void);

/** @brief Display a detailed listing of all hosts states
 *
 *  This is the host status list. A list of hosts and their state.
**/
void show_host_detail(void);


/** @brief Display's a overview of some/all servicegroups
 *
 *  Iterates through all/selected servicegroups and calls @ref show_servicegroup_overview to display them.
**/
void show_servicegroup_overviews(void);

/** @brief Display's a overview entry of a specific servicegroup
 *  @param [in] temp_servicegroup element to display
 *
 *  Checks if servicegroup members pass all filters and calls @ref show_servicegroup_hostgroup_member_overview to display every member.
**/
void show_servicegroup_overview(servicegroup *);


/** @brief Display's a summary of some/all servicegroups
 *
 *  Iterates through all/selected servicegroups and calls @ref show_servicegroup_summary to display them.
**/
void show_servicegroup_summaries(void);

/** @brief Displays status summary information for a specific servicegroup
 *  @param [in] temp_servicegroup element to display
 *  @param [in] odd can be 1 or 0 to determine which row colour to use
 *
 *  Prints some html code and calls @ref show_servicegroup_host_totals_summary and @ref show_servicegroup_service_totals_summary to display every member.
**/
void show_servicegroup_summary(servicegroup *, int);

/** @brief Displays host total summary information for a specific servicegroup
 *  @param [in] temp_servicegroup element to display
 *
 *  Prints a colour coded numbered list of hosts belonging to this servicegroup in different states
**/
void show_servicegroup_host_totals_summary(servicegroup *);

/** @brief Displays service total summary information for a specific servicegroup
 *  @param [in] temp_servicegroup element to display
 *
 *  Prints a colour coded numbered list of services belonging to this servicegroup in different states
**/
void show_servicegroup_service_totals_summary(servicegroup *);


/** @brief Display's a grid of some/all servicegroups
 *
 *  Iterates through all/selected servicegroups and calls @ref show_servicegroup_grid to display them.
**/
void show_servicegroup_grids(void);

/** @brief Display's a overview entry of a specific servicegroup
 *  @param [in] temp_servicegroup element to display
 *
 *  Checks if servicegroup members pass all filters and displays an entry of every member for host and servicestatus.
**/
void show_servicegroup_grid(servicegroup *);


/** @brief Display's a overview of some/all hostgroups
 *
 *  Iterates through all/selected hostgroups and calls @ref show_hostgroup_overview to display them.
**/
void show_hostgroup_overviews(void);

/** @brief Display's a overview entry of a specific hostgroup
 *  @param [in] hstgrp hostgroup element to display
 *  @retval TRUE
 *  @retval FALSE
 *  @return wether something got displayed or not
 *
 *  Checks if hostgroups members pass all filters and calls @ref show_servicegroup_hostgroup_member_overview to display every member.
**/
int show_hostgroup_overview(hostgroup *);

/** @brief Display's a summary of some/all hostgroups
 *
 *  Iterates through all/selected hostgroups and calls @ref show_hostgroup_summary to display them.
**/
void show_hostgroup_summaries(void);

/** @brief Displays status summary information for a specific hostgroup
 *  @param [in] temp_hostgroup element to display
 *  @param [in] odd can be 1 or 0 to determine which row colour to use
 *
 *  Prints some html code and calls @ref show_hostgroup_host_totals_summary and @ref show_hostgroup_service_totals_summary to display every member.
**/
void show_hostgroup_summary(hostgroup *, int);

/** @brief Displays host total summary information for a specific hostgroup
 *  @param [in] temp_hostgroup element to display
 *
 *  Prints a colour coded numbered list of hosts belonging to this hostgroup in different states
**/
void show_hostgroup_host_totals_summary(hostgroup *);

/** @brief Displays service total summary information for a specific hostgroup
 *  @param [in] temp_hostgroup element to display
 *
 *  Prints a colour coded numbered list of services belonging to this hostgroup in different states
**/
void show_hostgroup_service_totals_summary(hostgroup *);


/** @brief Display's a grid of some/all hostgroups
 *
 *  Iterates through all/selected hostgroups and calls @ref show_hostgroup_grid to display them.
**/
void show_hostgroup_grids(void);

/** @brief Display's a grid entry of a specific hostgroup
 *  @param [in] temp_hostgroup element to display
 *  @retval TRUE
 *  @retval FALSE
 *  @return wether something got displayed or not
 *
 *  Checks if hostgroup members pass all filters and displays an entry of every member for host and servicestatus.
**/
int show_hostgroup_grid(hostgroup *);


/** @brief Display's a single overview entry of a single host
 *  @param [in] hststatus element to display
 *  @param [in] odd can be 1 or 0 to determione which row colour to user
 *  @param [in] data is a servicegroup element which gets passed on to @ref show_servicegroup_hostgroup_member_service_status_totals (only needed when displaying servicegroups)
 *
 *  Prints one line depending on host status and also calls @ref show_servicegroup_hostgroup_member_service_status_totals to print the states of the services for this host.
**/
void show_servicegroup_hostgroup_member_overview(hoststatus *, int, void *);

/** @brief Display's service status totals for host/service group overview
 *  @param [in] host_name of host whom services should be displayed
 *  @param [in] data is a servicegroup element (only needed when displaying servicegroups)
 *
 *  Prints services part of @ show_servicegroup_hostgroup_member_overview
**/
void show_servicegroup_hostgroup_member_service_status_totals(char *, void *);
/** @} */

/** @name command drop down menus
    @{ **/

/** @brief Generates the drop down menu for service commands in service status list **/
void show_servicecommand_table(void);

/** @brief Generates the drop down menu for host commands in host status list **/
void show_hostcommand_table(void);
/** @} */


/** @brief Displays the filter box if host or service status filters are set **/
void show_filters(void);


/** @brief Parses the requested GET/POST variables
 *  @retval TRUE
 *  @retval FALSE
 *  @return wether parsing was successful or not
 *
 *  @n This function parses the request and set's the necessary variables
**/
int process_cgivars(void);


/** @brief print's the table header for differnt styles
 *  @param [in] style id of style type
 *
 *  This function print's the table header depending on style type
**/
void print_displayed_names(int);


/** @brief Display's the page number selector
 *  @param [in] local_result_start the result start for the current displayed list
 *  @param [in] url base url to link to
 *  @param [in] url_add additional url options
 *  @param [in] status_type can be @ref HOST_STATUS or @ref SERVICE_STATUS to determine if function is called from host or service list
 *
 *  Display's the page number selector and gernerates all links to select next/previouse page. Also copy's selector to top of the page
**/
void status_page_num_selector(int local_result_start, int status_type);


/** @brief Yes we need a main function **/
int main(void) {
	int result = OK;
	char *sound = NULL;
	char *search_regex = NULL;
	char *group_url = NULL;
	char *cgi_title = NULL;
	char host_service_name[MAX_INPUT_BUFFER];
	char temp_buffer[MAX_INPUT_BUFFER];
	host *temp_host = NULL;
	service *temp_service = NULL;
	hostgroup *temp_hostgroup = NULL;
	servicegroup *temp_servicegroup = NULL;
	hoststatus *temp_hoststatus = NULL;
	servicestatus *temp_servicestatus = NULL;
	servicesmember *temp_sg_member = NULL;
	hostsmember *temp_hg_member = NULL;
	int regex_i = 0, i = 0;
	int len;
	int show_dropdown = NO_STATUS;
	int found = FALSE;
	int show_all = TRUE;
	int host_items_found = FALSE;
	int service_items_found = FALSE;
	regex_t preg;


	/**
	 *	gather data
	**/

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

	/**
	 *	Initialize some vars
	**/

	mac = get_global_macros();

	time(&current_time);

	/* initialize macros */
	init_macros();

	/* get authentication information */
	get_authentication_information(&current_authdata);

	/* overwrite config value with amount we got via GET */
	result_limit = (get_result_limit != -1) ? get_result_limit : result_limit;

	/* for json and csv output return all by default */
	if (get_result_limit == -1 && (content_type == JSON_CONTENT || content_type == CSV_CONTENT))
		result_limit = 0;


	/**
	 *	check submitted data and create cgi_title
	**/

	/* keep backwards compatibility */
	if (nostatusheader_option == TRUE)
		display_status_totals = FALSE;


	/* determine display of hosts */
	if (req_hosts[0].entry != NULL) {
		show_all_hosts = FALSE;
		for (i = 0; req_hosts[i].entry != NULL; i++) {
			if (!strcmp(req_hosts[i].entry, "all")) {
				show_all_hosts = TRUE;
				my_free(url_hosts_part);
				my_free(cgi_title);
				req_hosts[0].entry = strdup("all");
				req_hosts[1].entry = NULL;
				dummy = asprintf(&url_hosts_part, "host=all");
				break;
			} else {
				if (i != 0) {
					strncpy(temp_buffer, cgi_title, sizeof(temp_buffer));
					my_free(cgi_title);
				}
				dummy = asprintf(&cgi_title, "%s%s[%s]", (i != 0) ? temp_buffer : "", (i != 0) ? ", " : "", html_encode(req_hosts[i].entry, FALSE));

				if (i == 0)
					dummy = asprintf(&url_hosts_part, "host=%s", url_encode(req_hosts[i].entry));
				else {
					strncpy(temp_buffer, url_hosts_part, sizeof(temp_buffer));
					my_free(url_hosts_part);
					dummy = asprintf(&url_hosts_part, "%s&host=%s", temp_buffer, url_encode(req_hosts[i].entry));
				}
			}
		}
	} else {
		req_hosts[0].entry = strdup("all");
		req_hosts[1].entry = NULL;
		dummy = asprintf(&url_hosts_part, "host=all");
	}

	/* determine display of hostgroups */
	if (req_hostgroups[0].entry != NULL) {
		show_all_hostgroups = FALSE;
		for (i = 0; req_hostgroups[i].entry != NULL; i++) {
			if (!strcmp(req_hostgroups[i].entry, "all")) {
				show_all_hostgroups = TRUE;
				my_free(url_hostgroups_part);
				my_free(cgi_title);
				req_hostgroups[0].entry = strdup("all");
				req_hostgroups[1].entry = NULL;
				dummy = asprintf(&url_hostgroups_part, "hostgroup=all");
				break;
			} else {
				if (i != 0) {
					strncpy(temp_buffer, cgi_title, sizeof(temp_buffer));
					my_free(cgi_title);
				}
				dummy = asprintf(&cgi_title, "%s%s{%s}", (i != 0) ? temp_buffer : "", (i != 0) ? ", " : "", html_encode(req_hostgroups[i].entry, FALSE));

				if (i == 0)
					dummy = asprintf(&url_hostgroups_part, "hostgroup=%s", url_encode(req_hostgroups[i].entry));
				else {
					strncpy(temp_buffer, url_hostgroups_part, sizeof(temp_buffer));
					my_free(url_hostgroups_part);
					dummy = asprintf(&url_hostgroups_part, "%s&hostgroup=%s", temp_buffer, url_encode(req_hostgroups[i].entry));
				}
			}
		}
	} else {
		req_hostgroups[0].entry = strdup("all");
		req_hostgroups[1].entry = NULL;
		dummy = asprintf(&url_hostgroups_part, "hostgroup=all");
	}

	/* determine display of servicegroups */
	if (req_servicegroups[0].entry != NULL) {
		show_all_servicegroups = FALSE;
		for (i = 0; req_servicegroups[i].entry != NULL; i++) {
			if (!strcmp(req_servicegroups[i].entry, "all")) {
				show_all_servicegroups = TRUE;
				my_free(url_servicegroups_part);
				my_free(cgi_title);
				req_servicegroups[0].entry = strdup("all");
				req_servicegroups[1].entry = NULL;
				dummy = asprintf(&url_servicegroups_part, "servicegroup=all");
				break;
			} else {
				if (i != 0) {
					strncpy(temp_buffer, cgi_title, sizeof(temp_buffer));
					my_free(cgi_title);
				}
				dummy = asprintf(&cgi_title, "%s%s(%s)", (i != 0) ? temp_buffer : "", (i != 0) ? ", " : "", html_encode(req_servicegroups[i].entry, FALSE));

				if (i == 0)
					dummy = asprintf(&url_servicegroups_part, "servicegroup=%s", url_encode(req_servicegroups[i].entry));
				else {
					strncpy(temp_buffer, url_servicegroups_part, sizeof(temp_buffer));
					my_free(url_servicegroups_part);
					dummy = asprintf(&url_servicegroups_part, "%s&servicegroup=%s", temp_buffer, url_encode(req_servicegroups[i].entry));
				}
			}
		}
	} else {
		req_servicegroups[0].entry = strdup("all");
		req_servicegroups[1].entry = NULL;
		dummy = asprintf(&url_servicegroups_part, "servicegroup=all");
	}


	/**
	 *	send HTML header
	**/

	document_header(CGI_ID, TRUE, (tab_friendly_titles && cgi_title != NULL) ? cgi_title : "Current Network Status");

	my_free(cgi_title);


	/**
	 *	check some more submitted data
	**/

	/* keeps backwards compatibility with old search method */
	if (navbar_search == TRUE && search_string == NULL && req_hosts[0].entry != NULL) {
		group_style_type = STYLE_HOST_SERVICE_DETAIL;
		search_string = strdup(req_hosts[0].entry);
	}

	/* allow service_filter only for status lists */
	if (group_style_type == STYLE_SUMMARY || group_style_type == STYLE_GRID || group_style_type == STYLE_OVERVIEW)
		my_free(service_filter);

	/**
	 *	filter status data if user searched for something
	**/

	/* see if user tried searching something */
	if (search_string != NULL) {

		/* build regex string */

		/* allocate for 3 extra chars, ^, $ and \0 */
		search_regex = malloc(sizeof(char) * (strlen(search_string) * 2 + 3));
		len = strlen(search_string);
		for (i = 0; i < len; i++, regex_i++) {
			if (search_string[i] == '*') {
				search_regex[regex_i++] = '.';
				search_regex[regex_i] = '*';
			} else
				search_regex[regex_i] = search_string[i];
		}

		search_regex[regex_i] = '\0';

		/* check and compile regex */
		if (regcomp(&preg, search_regex, REG_ICASE | REG_NOSUB) == 0) {

			/* regular expression is valid */

			/* now look through all hosts to see which one matches */
			for (temp_hoststatus = hoststatus_list; temp_hoststatus != NULL; temp_hoststatus = temp_hoststatus->next) {
				temp_hoststatus->search_matched = FALSE;

				if ((temp_host = find_host(temp_hoststatus->host_name)) == NULL)
					continue;

				/* try to find a match */
				if (regexec(&preg, temp_host->name, 0, NULL, 0) == 0 || \
				        regexec(&preg, temp_host->display_name, 0, NULL, 0) == 0 || \
				        regexec(&preg, temp_host->alias, 0, NULL, 0) == 0 || \
				        regexec(&preg, temp_host->address, 0, NULL, 0) == 0 || \
				        regexec(&preg, temp_host->address6, 0, NULL, 0) == 0) {

					temp_hoststatus->search_matched = TRUE;
					host_items_found = TRUE;
				}
			}

			for (temp_servicestatus = servicestatus_list; temp_servicestatus != NULL; temp_servicestatus = temp_servicestatus->next) {
				temp_servicestatus->search_matched = FALSE;

				/* find the service  */
				temp_service = find_service(temp_servicestatus->host_name, temp_servicestatus->description);

				if (temp_service == NULL)
					continue;

				/* try to match on combination of host name and service description */
				snprintf(host_service_name, sizeof(host_service_name), "%s %s", temp_service->host_name, temp_service->display_name);
				host_service_name[sizeof(host_service_name) - 1] = '\x0';

				/* try to find a match */
				if (regexec(&preg, temp_service->description, 0, NULL, 0) == 0 || \
				        regexec(&preg, temp_service->display_name, 0, NULL, 0) == 0 || \
				        regexec(&preg, temp_service->host_name, 0, NULL, 0) == 0 || \
				        regexec(&preg, host_service_name, 0, NULL, 0) == 0) {

					temp_servicestatus->search_matched = TRUE;
					service_items_found = TRUE;
				}
			}


			/* if didn't found anything until now we start looking for hostgroups and servicegroups */
			if (host_items_found == FALSE && service_items_found == FALSE) {

				/* try to find hostgroup */
				found = FALSE;
				for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {
					if (regexec(&preg, temp_hostgroup->group_name, 0, NULL, 0) == 0) {
						req_hostgroups[num_req_hostgroups++].entry = strdup(temp_hostgroup->group_name);
						display_type = DISPLAY_HOSTGROUPS;
						show_all_hostgroups = FALSE;
						found = TRUE;
					}
				}

				/* if no hostgroup matched, try to find a serviegroup */
				if (found == FALSE) {
					for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {
						if (regexec(&preg, temp_servicegroup->group_name, 0, NULL, 0) == 0) {
							req_servicegroups[num_req_servicegroups++].entry = strdup(temp_servicegroup->group_name);
							display_type = DISPLAY_SERVICEGROUPS;
							show_all_servicegroups = FALSE;
						}
					}
				}
			}
		}

		/* free regular expression */
		regfree(&preg);
		my_free(search_regex);

		user_is_authorized_for_statusdata = TRUE;

		/* check the search result and trigger the desired view */
		if (host_items_found == TRUE && service_items_found == FALSE && group_style_type == STYLE_HOST_SERVICE_DETAIL)
			group_style_type = STYLE_HOST_DETAIL;
		else if (host_items_found == FALSE && service_items_found == TRUE && group_style_type == STYLE_HOST_SERVICE_DETAIL)
			group_style_type = STYLE_SERVICE_DETAIL;
	}

	/* pre filter for service groups
	   this way we mark all services which belong to a servicegroup we want to see once.
	   otherwise we would have to check every service if it belongs to a servicegroup we want to see
	   and this is very expensive
	*/
	if (display_type == DISPLAY_SERVICEGROUPS) {
		if (show_all_servicegroups == FALSE) {
			for (i = 0; req_servicegroups[i].entry != NULL; i++) {
				temp_servicegroup = find_servicegroup(req_servicegroups[i].entry);
				if (temp_servicegroup != NULL && is_authorized_for_servicegroup(temp_servicegroup, &current_authdata) == TRUE) {
					for (temp_sg_member = temp_servicegroup->members; temp_sg_member != NULL; temp_sg_member = temp_sg_member->next) {
						temp_hoststatus = find_hoststatus(temp_sg_member->host_name);
						if (temp_hoststatus != NULL)
							temp_hoststatus->added |= STATUS_BELONGS_TO_SG;
						temp_servicestatus = find_servicestatus(temp_sg_member->host_name, temp_sg_member->service_description);
						if (temp_servicestatus != NULL)
							temp_servicestatus->added |= STATUS_BELONGS_TO_SG;
					}
				}
			}
		} else {
			for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {
				if (is_authorized_for_servicegroup(temp_servicegroup, &current_authdata) == TRUE) {
					for (temp_sg_member = temp_servicegroup->members; temp_sg_member != NULL; temp_sg_member = temp_sg_member->next) {
						temp_hoststatus = find_hoststatus(temp_sg_member->host_name);
						if (temp_hoststatus != NULL)
							temp_hoststatus->added |= STATUS_BELONGS_TO_SG;
						temp_servicestatus = find_servicestatus(temp_sg_member->host_name, temp_sg_member->service_description);
						if (temp_servicestatus != NULL)
							temp_servicestatus->added |= STATUS_BELONGS_TO_SG;
					}
				}
			}
		}
	}

	/* pre filter for all host groups as well */
	if (display_type == DISPLAY_HOSTGROUPS) {
		if (show_all_hostgroups == FALSE) {
			for (i = 0; req_hostgroups[i].entry != NULL; i++) {
				temp_hostgroup = find_hostgroup(req_hostgroups[i].entry);
				if (temp_hostgroup != NULL && (show_partial_hostgroups == TRUE || is_authorized_for_hostgroup(temp_hostgroup, &current_authdata) == TRUE)) {
					for (temp_hg_member = temp_hostgroup->members; temp_hg_member != NULL; temp_hg_member = temp_hg_member->next) {
						temp_hoststatus = find_hoststatus(temp_hg_member->host_name);
						if (temp_hoststatus != NULL)
							temp_hoststatus->added |= STATUS_BELONGS_TO_HG;
					}
				}
			}
		} else {
			for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {
				if (show_partial_hostgroups == TRUE || is_authorized_for_hostgroup(temp_hostgroup, &current_authdata) == TRUE) {
					for (temp_hg_member = temp_hostgroup->members; temp_hg_member != NULL; temp_hg_member = temp_hg_member->next) {
						temp_hoststatus = find_hoststatus(temp_hg_member->host_name);
						if (temp_hoststatus != NULL)
							temp_hoststatus->added |= STATUS_BELONGS_TO_HG;
					}
				}
			}
		}
	}

	/**
	 *	Now iterate through servicestatus_list and hoststatus_list to find all hosts/services we need to display
	 *	All filtering and authorization is done here
	**/

	/* if user just want's to see all unhandled problems */
	/* prepare for services */
	if (display_all_unhandled_problems == TRUE || display_all_problems == TRUE) {
		host_status_types = HOST_UP | HOST_PENDING;
		service_status_types = all_service_problems;
		group_style_type = STYLE_HOST_SERVICE_DETAIL;
		if (display_all_unhandled_problems == TRUE)
			service_properties |= service_problems_unhandled;
	}

	for (temp_servicestatus = servicestatus_list; temp_servicestatus != NULL; temp_servicestatus = temp_servicestatus->next) {

		/* if user is doing a search and service didn't match try next one */
		if (search_string != NULL && temp_servicestatus->search_matched == FALSE && \
		        show_all_hostgroups == TRUE && show_all_servicegroups == TRUE)
			continue;

		if (service_filter != NULL && strcmp(service_filter, temp_servicestatus->description))
			continue;

		/* find the service  */
		temp_service = find_service(temp_servicestatus->host_name, temp_servicestatus->description);

		/* if we couldn't find the service, go to the next service */
		if (temp_service == NULL)
			continue;

		/* make sure user has rights to see this... */
		if (is_authorized_for_service(temp_service, &current_authdata) == FALSE)
			continue;

		user_is_authorized_for_statusdata = TRUE;

		/* get the host status information */
		temp_hoststatus = find_hoststatus(temp_service->host_name);

		/* check host properties filter */
		if (passes_host_properties_filter(temp_hoststatus) == FALSE)
			continue;

		/* check service properties filter */
		if (passes_service_properties_filter(temp_servicestatus) == FALSE)
			continue;

		/* find the host */
		temp_host = find_host(temp_service->host_name);

		/* see if only one host should be shown */
		if (display_type == DISPLAY_HOSTS && show_all_hosts == FALSE && search_string == NULL) {
			found = FALSE;
			for (i = 0; req_hosts[i].entry != NULL; i++) {
				if (!strcmp(req_hosts[i].entry, temp_hoststatus->host_name) || !strcmp(req_hosts[i].entry, temp_host->display_name)) {
					found = TRUE;
					break;
				}
			}
			if (found == FALSE)
				continue;
		}

		/* see if we should display a hostgroup */
		else if (display_type == DISPLAY_HOSTGROUPS) {
			if (!(temp_hoststatus->added & STATUS_BELONGS_TO_HG))
				continue;
		}

		/* see if we should display a servicegroup */
		else if (display_type == DISPLAY_SERVICEGROUPS) {
			if (!(temp_servicestatus->added & STATUS_BELONGS_TO_SG))
				continue;
		}

		if (!(temp_hoststatus->added & STATUS_COUNTED_UNFILTERED)) {
			/* count host for status totals */
			if (temp_hoststatus->status == HOST_DOWN)
				num_total_hosts_down++;
			else if (temp_hoststatus->status == HOST_UNREACHABLE)
				num_total_hosts_unreachable++;
			else if (temp_hoststatus->status == HOST_PENDING)
				num_total_hosts_pending++;
			else
				num_total_hosts_up++;

			temp_hoststatus->added |= STATUS_COUNTED_UNFILTERED;
		}

		/* see if we should display services for hosts with tis type of status */
		if (!(host_status_types & temp_hoststatus->status))
			continue;


		if (!(temp_servicestatus->added & STATUS_COUNTED_UNFILTERED)) {
			if (temp_servicestatus->status == SERVICE_CRITICAL)
				num_total_services_critical++;
			else if (temp_servicestatus->status == SERVICE_WARNING)
				num_total_services_warning++;
			else if (temp_servicestatus->status == SERVICE_UNKNOWN)
				num_total_services_unknown++;
			else if (temp_servicestatus->status == SERVICE_PENDING)
				num_total_services_pending++;
			else
				num_total_services_ok++;

			temp_servicestatus->added |= STATUS_COUNTED_UNFILTERED;
		}

		/* see if we should display this type of service status */
		if (!(service_status_types & temp_servicestatus->status))
			continue;

		if (display_all_unhandled_problems == FALSE && display_all_problems == FALSE)
			add_status_data(HOST_STATUS, temp_hoststatus);
		add_status_data(SERVICE_STATUS, temp_servicestatus);
	}

	/* if user just want's to see all unhandled problems */
	/* prepare for hosts */
	if (display_all_unhandled_problems == TRUE || display_all_problems == TRUE) {
		host_status_types = all_host_problems;
		if (display_all_unhandled_problems == TRUE)
			host_properties |= host_problems_unhandled;
	}


	/* this is only for hosts with no services attached */
	if (group_style_type != STYLE_SERVICE_DETAIL) {
		for (temp_hoststatus = hoststatus_list; temp_hoststatus != NULL; temp_hoststatus = temp_hoststatus->next) {

			/* see if hoststatus is already recorded */
			if (temp_hoststatus->added & STATUS_ADDED)
				continue;

			/* if user is doing a search and host didn't match try next one */
			if (search_string != NULL && temp_hoststatus->search_matched == FALSE && show_all_hostgroups == TRUE)
				continue;

			/* find the host  */
			temp_host = find_host(temp_hoststatus->host_name);

			/* if we couldn't find the host, go to the next status entry */
			if (temp_host == NULL)
				continue;

			/* make sure user has rights to see this... */
			if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
				continue;

			user_is_authorized_for_statusdata = TRUE;

			/* check host properties filter */
			if (passes_host_properties_filter(temp_hoststatus) == FALSE)
				continue;

			/* see if only one host should be shown */
			if (display_type == DISPLAY_HOSTS && show_all_hosts == FALSE && search_string == NULL) {
				found = FALSE;
				for (i = 0; req_hosts[i].entry != NULL; i++) {
					if (!strcmp(req_hosts[i].entry, temp_hoststatus->host_name) || !strcmp(req_hosts[i].entry, temp_host->display_name)) {
						found = TRUE;
						break;
					}
				}
				if (found == FALSE)
					continue;
			}

			/* see if we should display a hostgroup */
			else if (display_type == DISPLAY_HOSTGROUPS) {
				if (!(temp_hoststatus->added & STATUS_BELONGS_TO_HG))
					continue;

				/* see if we should display a servicegroup */
			} else if (display_type == DISPLAY_SERVICEGROUPS) {
				if (!(temp_hoststatus->added & STATUS_BELONGS_TO_SG))
					continue;
			}

			if (!(temp_hoststatus->added & STATUS_COUNTED_UNFILTERED)) {
				/* count host for status totals */
				if (temp_hoststatus->status == HOST_DOWN)
					num_total_hosts_down++;
				else if (temp_hoststatus->status == HOST_UNREACHABLE)
					num_total_hosts_unreachable++;
				else if (temp_hoststatus->status == HOST_PENDING)
					num_total_hosts_pending++;
				else
					num_total_hosts_up++;

				temp_hoststatus->added |= STATUS_COUNTED_UNFILTERED;
			}

			/* see if we should display services for hosts with this type of status */
			if (!(host_status_types & temp_hoststatus->status))
				continue;

			add_status_data(HOST_STATUS, temp_hoststatus);
		}
	}


	/**
	 *	Now as we have all necessary data we start displaying the page
	**/

	// determine which dropdown menu to show
	if (group_style_type == STYLE_OVERVIEW || group_style_type == STYLE_SUMMARY || group_style_type == STYLE_GRID)
		show_dropdown = NO_STATUS;
	else {
		if (group_style_type == STYLE_HOST_DETAIL || group_style_type == STYLE_HOST_SERVICE_DETAIL)
			show_dropdown = HOST_STATUS;
		else
			show_dropdown = SERVICE_STATUS;
	}

	/* add highlight table row and form elementes (drop down) to page */
	if (show_dropdown != NO_STATUS && content_type == HTML_CONTENT) {

		if (highlight_table_rows == TRUE) {
			printf("<script type=\"text/javascript\">\n");
			printf("\t$(document).ready(function(){\n");
			printf("\t\t$(\"table.status tr\").hover(function( e ) {\n");
			printf("\t\t\t$(this).find(\"td\").each(function(){\n");
			printf("\t\t\t\tif($(this).attr(\"class\")) {\n");
			printf("\t\t\t\t\t$(this).addClass(\"highlightRow\");\n");
			printf("\t\t\t\t}\n");
			printf("\t\t\t});\n");
			printf("\t\t}, function(){\n");
			printf("\t\t\t$(this).find(\"td\").each(function(){\n");
			printf("\t\t\t\tif($(this).attr(\"class\")) {\n");
			printf("\t\t\t\t\t$(this).removeClass(\"highlightRow\");\n");
			printf("\t\t\t\t}\n");
			printf("\t\t\t});\n");
			printf("\t\t});\n");
			printf("\t});\n");
			printf("</script>\n");
		}

		printf("<form name='tableform%s' id='tableform%s' action='%s' method='POST' style='margin:0px' onkeypress='var key = (window.event) ? event.keyCode : event.which; return (key != 13);'>\n", (show_dropdown == HOST_STATUS) ? "host" : "service", (show_dropdown == HOST_STATUS) ? "host" : "service", CMD_CGI);
		printf("<input type=hidden name=hiddenforcefield><input type=hidden name=hiddencmdfield><input type=hidden name=buttonValidChoice><input type=hidden name=buttonCheckboxChecked><input type=hidden name=force_check>\n");
	}

	/**
	 *	display the page header
	**/

	if (display_header == TRUE) {

		/* begin top table */
		/* network status, hosts/service status totals */
		printf("<table border=0 width=100%% cellspacing=0 cellpadding=0>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=33%%>\n");
		/* info table */
		display_info_table("Current Network Status", &current_authdata, daemon_check);
		printf("</td>\n");

		/* middle column of top row */
		printf("<td align=center valign=top width=33%%>\n");
		show_host_status_totals();
		printf("</td>\n");

		/* right hand column of top row */
		printf("<td align=center valign=top width=33%%>\n");
		show_service_status_totals();
		printf("</td>\n");

		printf("</tr>\n");
		printf("</table>\n");

		/* second table below */
		printf("<br>\n");
		/* Links & Commands */

		printf("<table border=0 width=100%% cellspacing=0 cellpadding=0>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=50%%>\n");

		printf("<table border=1 cellpading=0 cellspacing=0 class='linkBox'>\n");
		printf("<tr><td class='linkBox'>\n");

		/* Display all links */
		if (display_type == DISPLAY_HOSTS) {

			if (search_string == NULL && (show_all_hosts == TRUE || num_req_hosts <= 1)) {
				printf("<a href='%s?%s'>View <b>Alert History</b> For <b>%s</b></a><br>\n", HISTORY_CGI, url_hosts_part, (show_all_hosts == TRUE) ? "All Hosts" : "This Host");
				printf("<a href='%s?%s'>View <b>Notifications</b> For <b>%s</b></a>\n", NOTIFICATIONS_CGI, url_hosts_part, (show_all_hosts == TRUE) ? "All Hosts" : "This Host");
			}

			if (search_string != NULL)
				snprintf(temp_buffer, sizeof(temp_buffer), "search_string=%s", url_encode(search_string));
			else
				strncpy(temp_buffer, "host=all", sizeof(temp_buffer));
			temp_buffer[sizeof(temp_buffer) - 1] = '\x0';

			if (group_style_type != STYLE_HOST_SERVICE_DETAIL && search_string == NULL)
				printf("<br><a href='%s?%s&style=hostservicedetail'>View <b>Host AND Services</b> For <b>All Hosts</b></a>\n", STATUS_CGI, temp_buffer);
			if (group_style_type != STYLE_SERVICE_DETAIL)
				printf("<br><a href='%s?%s&style=detail'>View <b>Service Status Detail</b> For <b>All Hosts</b></a>\n", STATUS_CGI, temp_buffer);
			if (group_style_type != STYLE_HOST_DETAIL)
				printf("<br><a href='%s?%s&style=hostdetail'>View <b>Host Status Detail</b> For <b>All Hosts</b></a>\n", STATUS_CGI, temp_buffer);
		} else if (display_type == DISPLAY_SERVICEGROUPS || display_type == DISPLAY_HOSTGROUPS) {
			if (display_type == DISPLAY_HOSTGROUPS) {
				show_all = show_all_hostgroups;
				group_url = url_hostgroups_part;
			} else {
				show_all = show_all_servicegroups;
				group_url = url_servicegroups_part;
			}

			if (show_all == TRUE)
				strncpy(temp_buffer, "All", sizeof(temp_buffer));
			else if ((display_type == DISPLAY_HOSTGROUPS && num_req_hostgroups == 1) || (display_type == DISPLAY_SERVICEGROUPS && num_req_servicegroups == 1))
				strncpy(temp_buffer, "This", sizeof(temp_buffer));
			else
				strncpy(temp_buffer, "These", sizeof(temp_buffer));
			temp_buffer[sizeof(temp_buffer) - 1] = '\x0';

			if (show_all == FALSE) {
				if (group_style_type == STYLE_HOST_SERVICE_DETAIL)
					printf("<a href='%s?%sgroup=all&style=hostservicedetail'>View <b>Host AND Services</b> For <b>All %sgroups</b></a><br>\n", STATUS_CGI, (display_type == DISPLAY_HOSTGROUPS) ? "host" : "service", (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
				if (group_style_type == STYLE_SERVICE_DETAIL)
					printf("<a href='%s?%sgroup=all&style=detail'>View <b>Service Status Detail</b> For <b>All %sgroups</b></a><br>\n", STATUS_CGI, (display_type == DISPLAY_HOSTGROUPS) ? "host" : "service", (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
				if (group_style_type == STYLE_HOST_DETAIL)
					printf("<a href='%s?%sgroup=all&style=hostdetail'>View <b>Host Status Detail</b> For <b>All %sgroups</b></a><br>\n", STATUS_CGI, (display_type == DISPLAY_HOSTGROUPS) ? "host" : "service", (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
				if (group_style_type == STYLE_OVERVIEW)
					printf("<a href='%s?%sgroup=all&style=overview'>View <b>Status Overview</b> For <b>All %sgroups</b></a><br>\n", STATUS_CGI, (display_type == DISPLAY_HOSTGROUPS) ? "host" : "service", (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
				if (group_style_type == STYLE_SUMMARY)
					printf("<a href='%s?%sgroup=all&style=summary'>View <b>Status Summary</b> For <b>All %sgroups</b></a><br>\n", STATUS_CGI, (display_type == DISPLAY_HOSTGROUPS) ? "host" : "service", (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
				if (group_style_type == STYLE_GRID)
					printf("<a href='%s?%sgroup=all&style=grid'>View <b>Status Grid</b> For <b>All %sgroups</b></a><br>\n", STATUS_CGI, (display_type == DISPLAY_HOSTGROUPS) ? "host" : "service", (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
			}

			if (group_style_type != STYLE_HOST_SERVICE_DETAIL)
				printf("<a href='%s?%s&style=hostservicedetail'>View <b>Host AND Services</b> Status Detail For <b>%s %sgroup%s</b></a><br>\n", STATUS_CGI, group_url, temp_buffer, (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service", !strcmp(temp_buffer, "This") ? "" : "s");
			if (group_style_type != STYLE_SERVICE_DETAIL)
				printf("<a href='%s?%s&style=detail'>View <b>Service Status Detail</b> For <b>%s %sgroup%s</b></a><br>\n", STATUS_CGI, group_url, temp_buffer, (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service", !strcmp(temp_buffer, "This") ? "" : "s");
			if (group_style_type != STYLE_HOST_DETAIL)
				printf("<a href='%s?%s&style=hostdetail'>View <b>Host Status Detail</b> For <b>%s %sgroup%s</b></a><br>\n", STATUS_CGI, group_url, temp_buffer, (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service", !strcmp(temp_buffer, "This") ? "" : "s");
			if (group_style_type != STYLE_OVERVIEW)
				printf("<a href='%s?%s&style=overview'>View <b>Status Overview</b> For <b>%s %sgroup%s</b></a><br>\n", STATUS_CGI, group_url, temp_buffer, (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service", !strcmp(temp_buffer, "This") ? "" : "s");
			if (group_style_type != STYLE_SUMMARY)
				printf("<a href='%s?%s&style=summary'>View <b>Status Summary</b> For <b>%s %sgroup%s</b></a><br>\n", STATUS_CGI, group_url, temp_buffer, (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service", !strcmp(temp_buffer, "This") ? "" : "s");
			if (group_style_type != STYLE_GRID)
				printf("<a href='%s?%s&style=grid'>View <b>Status Grid</b> For <b>%s %sgroup%s</b></a><br>\n", STATUS_CGI, group_url, temp_buffer, (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service", !strcmp(temp_buffer, "This") ? "" : "s");

			if (show_all == FALSE && ((display_type == DISPLAY_HOSTGROUPS && num_req_hostgroups == 1) || (display_type == DISPLAY_SERVICEGROUPS && num_req_servicegroups == 1))) {
				printf("<a href='%s?%s'>View <b>Notifications</b> For <b>This %sgroup</b></a><br>\n", NOTIFICATIONS_CGI, group_url, (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
				printf("<a href='%s?%s'>View <b>Alert History</b> For <b>This %sgroup</b></a><br>\n", HISTORY_CGI, group_url, (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
				printf("<a href='%s?type=%d&%s'>View <b>Commands</b> For <b>This %sgroup</b></a><br>\n", EXTINFO_CGI, (display_type == DISPLAY_HOSTGROUPS) ? DISPLAY_HOSTGROUP_INFO : DISPLAY_SERVICEGROUP_INFO, group_url, (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
				if (is_authorized_for_configuration_information(&current_authdata) == TRUE) {
					printf("<a href='%s?type=%sgroups&item_name=%s'>View <b>Config</b> For <b>This %sgroup</b></a>\n", CONFIG_CGI, (display_type == DISPLAY_HOSTGROUPS) ? "host" : "service",
						url_encode((display_type == DISPLAY_HOSTGROUPS) ? req_hostgroups[0].entry : req_servicegroups[0].entry), (display_type == DISPLAY_HOSTGROUPS) ? "Host" : "Service");
				}
			}
		}

		printf("</td></tr>\n");
		printf("</table>\n");

		printf("</td>\n");

		/* Command table */
		printf("<td align=right width=50%%>\n");

		if (show_dropdown == HOST_STATUS)
			show_hostcommand_table();
		else if (show_dropdown == SERVICE_STATUS)
			show_servicecommand_table();
		else
			printf("<br>");

		printf("</td>\n");
		printf("</tr>\n");

		/* end of second table */
		printf("</table>\n");

	} else if (show_dropdown != NO_STATUS && content_type == HTML_CONTENT) {

		printf("<table border=0 width=100%% cellspacing=0 cellpadding=0>\n");
		printf("<tr><td align=right width=100%%>\n");

		if (show_dropdown == HOST_STATUS)
			show_hostcommand_table();
		else if (show_dropdown == SERVICE_STATUS)
			show_servicecommand_table();

		/* end of second table */
		printf("</td></tr></table>\n");
	}


	/**
	 *	embed sound tag if necessary...
	**/

	if (problem_hosts_unreachable > 0 && host_unreachable_sound != NULL)
		sound = host_unreachable_sound;
	else if (problem_hosts_down > 0 && host_down_sound != NULL)
		sound = host_down_sound;
	else if (problem_services_critical > 0 && service_critical_sound != NULL)
		sound = service_critical_sound;
	else if (problem_services_warning > 0 && service_warning_sound != NULL)
		sound = service_warning_sound;
	else if (problem_services_unknown > 0 && service_unknown_sound != NULL)
		sound = service_unknown_sound;
	else if (problem_services_unknown == 0 && problem_services_warning == 0 && problem_services_critical == 0 && problem_hosts_down == 0 && problem_hosts_unreachable == 0 && normal_sound != NULL)
		sound = normal_sound;
	if (sound != NULL && content_type == HTML_CONTENT) {
		printf("<object type=\"audio/x-wav\" data=\"%s%s\" height=\"0\" width=\"0\">", url_media_path, sound);
		printf("<param name=\"filename\" value=\"%s%s\">", url_media_path, sound);
		printf("<param name=\"autostart\" value=\"true\">");
		printf("<param name=\"playcount\" value=\"1\">");
		printf("</object>");
	}

	/* flush the data we allready have
	   but do this only if we display HTML content
	*/
	if (content_type == HTML_CONTENT) {
		printf(" ");
		fflush(NULL);
	}


	/**
	 *	Top part of page is now over
	 *	From here we print the bottom with all the lists
	 *	now print common header for Overview, Summary and Grid
	**/

	if ((group_style_type == STYLE_OVERVIEW || group_style_type == STYLE_SUMMARY || group_style_type == STYLE_GRID) && content_type == HTML_CONTENT) {
		printf("<P>\n");

		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		printf("<td valign=top align=left width=33%%>\n");
		show_filters();
		printf("</td>");

		printf("<td valign=top align=center width=33%%>\n");

		if (group_style_type == STYLE_OVERVIEW)
			printf("<DIV ALIGN=CENTER CLASS='statusTitle'>Status Overview For ");
		else if (group_style_type == STYLE_SUMMARY)
			printf("<DIV ALIGN=CENTER CLASS='statusTitle'>Status Summary For ");
		else
			printf("<DIV ALIGN=CENTER CLASS='statusTitle'>Status Grid For ");

		print_displayed_names(display_type);
		printf("</DIV>\n");

		printf("<br>");

		printf("</td>\n");

		/* add export to csv, json, link */
		printf("<td valign=bottom width=33%%>");
		printf("<div style='padding-right:3px;' class='csv_export_link'>");
		print_export_link(JSON_CONTENT, STATUS_CGI, NULL);
		print_export_link(HTML_CONTENT, STATUS_CGI, NULL);
		printf("</div></td>\n");

		printf("</tr>\n");
		printf("</table>\n");

		printf("</P>\n");
	}

	/**
	 *	Finally Print all the data we talk about the whole time
	 *	This of course depends on the chosen style
	**/

	if (group_style_type == STYLE_OVERVIEW) {
		if (display_type == DISPLAY_SERVICEGROUPS)
			show_servicegroup_overviews();
		else
			show_hostgroup_overviews();
	} else if (group_style_type == STYLE_SUMMARY) {
		if (display_type == DISPLAY_SERVICEGROUPS)
			show_servicegroup_summaries();
		else
			show_hostgroup_summaries();
	} else if (group_style_type == STYLE_GRID) {
		if (display_type == DISPLAY_SERVICEGROUPS)
			show_servicegroup_grids();
		else
			show_hostgroup_grids();
	} else {
		if (group_style_type == STYLE_HOST_DETAIL)
			show_host_detail();
		else if (group_style_type == STYLE_HOST_SERVICE_DETAIL) {

			show_host_detail();

			if (content_type == HTML_CONTENT) {
				printf("<form name='tableformservice' id='tableformservice' action='%s' method='POST' style='margin:0px' onkeypress='var key = (window.event) ? event.keyCode : event.which; return (key != 13);'>\n", CMD_CGI);
				printf("<input type=hidden name=hiddenforcefield><input type=hidden name=hiddencmdfield><input type=hidden name=buttonValidChoice><input type=hidden name=buttonCheckboxChecked><input type=hidden name=force_check>\n");

				printf("<table border=0 width=100%% cellspacing=0 cellpadding=0><tr><td align=right width=50%%></td><td align=right width=50%%>\n");
				show_servicecommand_table();
				printf("</td></tr></table>\n");
			} else if (content_type == JSON_CONTENT)
				printf(",\n");

			show_service_detail();
		} else
			show_service_detail();
	}


	/**
	 *	print document footer and free all allocated data
	**/

	document_footer(CGI_ID);

	/* free all allocated memory */
	free_memory();
	free_comment_data();

	/* free status data */
	free_local_status_data();
	free_status_data();

	/* free lists */
	for (i = 0; req_hosts[i].entry != NULL; i++)
		my_free(req_hosts[i].entry);

	for (i = 0; req_hostgroups[i].entry != NULL; i++)
		my_free(req_hostgroups[i].entry);

	for (i = 0; req_servicegroups[i].entry != NULL; i++)
		my_free(req_servicegroups[i].entry);

	my_free(url_hosts_part);
	my_free(url_hostgroups_part);
	my_free(url_servicegroups_part);
	my_free(search_string);
	my_free(service_filter);

	return OK;
}

int process_cgivars(void) {
	char **variables;
	char *temp_buffer = NULL;
	int error = FALSE;
	int x;

	variables = getcgivars();

	for (x = 0; variables[x] != NULL; x++) {

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if (strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
			x++;
			continue;
		}

		/* we found the search_string argument */
		else if (!strcmp(variables[x], "search_string")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			group_style_type = STYLE_HOST_SERVICE_DETAIL;
			search_string = strdup(variables[x]);
		}

		/* we found the servicefilter argument */
		else if (!strcmp(variables[x], "servicefilter")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			service_filter = (char *)strdup(variables[x]);
		}

		/* we found the navbar search argument */
		/* kept for backwards compatibility */
		else if (!strcmp(variables[x], "navbarsearch")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}
			navbar_search = TRUE;
		}

		/* we found the hostgroup argument */
		else if (!strcmp(variables[x], "hostgroup")) {
			display_type = DISPLAY_HOSTGROUPS;
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			temp_buffer = (char *)strdup(variables[x]);
			strip_html_brackets(temp_buffer);

			if (temp_buffer != NULL)
				req_hostgroups[num_req_hostgroups++].entry = strdup(temp_buffer);

			my_free(temp_buffer);
		}

		/* we found the servicegroup argument */
		else if (!strcmp(variables[x], "servicegroup")) {
			display_type = DISPLAY_SERVICEGROUPS;
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			temp_buffer = strdup(variables[x]);
			strip_html_brackets(temp_buffer);

			if (temp_buffer != NULL)
				req_servicegroups[num_req_servicegroups++].entry = strdup(temp_buffer);

			my_free(temp_buffer);
		}

		/* we found the host argument */
		else if (!strcmp(variables[x], "host")) {
			display_type = DISPLAY_HOSTS;
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			temp_buffer = strdup(variables[x]);
			strip_html_brackets(temp_buffer);

			if (temp_buffer != NULL)
				req_hosts[num_req_hosts++].entry = strdup(temp_buffer);

			my_free(temp_buffer);
		}

		/* we found the columns argument */
		else if (!strcmp(variables[x], "columns")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			overview_columns = atoi(variables[x]);
			if (overview_columns <= 0)
				overview_columns = 1;
		}

		/* we found the service status type argument */
		else if (!strcmp(variables[x], "servicestatustypes")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			service_status_types = atoi(variables[x]);
		}

		/* we found the host status type argument */
		else if (!strcmp(variables[x], "hoststatustypes")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			host_status_types = atoi(variables[x]);
		}

		/* we found the service properties argument */
		else if (!strcmp(variables[x], "serviceprops")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			service_properties = strtoul(variables[x], NULL, 10);
		}

		/* we found the host properties argument */
		else if (!strcmp(variables[x], "hostprops")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			host_properties = strtoul(variables[x], NULL, 10);
		}

		/* we found the host or service group style argument */
		else if (!strcmp(variables[x], "style")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "overview"))
				group_style_type = STYLE_OVERVIEW;
			else if (!strcmp(variables[x], "detail"))
				group_style_type = STYLE_SERVICE_DETAIL;
			else if (!strcmp(variables[x], "summary"))
				group_style_type = STYLE_SUMMARY;
			else if (!strcmp(variables[x], "grid"))
				group_style_type = STYLE_GRID;
			else if (!strcmp(variables[x], "hostdetail"))
				group_style_type = STYLE_HOST_DETAIL;
			else if (!strcmp(variables[x], "hostservicedetail"))
				group_style_type = STYLE_HOST_SERVICE_DETAIL;
			else
				group_style_type = STYLE_SERVICE_DETAIL;
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

		/* we found the sort object argument */
		else if (!strcmp(variables[x], "sortobject")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "hosts"))
				sort_object = HOST_STATUS;
			else if (!strcmp(variables[x], "services"))
				sort_object = SERVICE_STATUS;
		}

		/* we found the embed option */
		else if (!strcmp(variables[x], "embedded"))
			embedded = TRUE;

		/* we found the noheader option */
		else if (!strcmp(variables[x], "noheader"))
			display_header = FALSE;

		/* we found the nostatusheader option */
		else if (!strcmp(variables[x], "nostatusheader"))
			nostatusheader_option = TRUE;

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

		/* we found the pause option */
		else if (!strcmp(variables[x], "paused"))
			refresh = FALSE;

		/* we found the nodaemoncheck option */
		else if (!strcmp(variables[x], "nodaemoncheck"))
			daemon_check = FALSE;

		/* we found the nodaemoncheck option */
		else if (!strcmp(variables[x], "allunhandledproblems"))
			display_all_unhandled_problems = TRUE;

		/* we found the nodaemoncheck option */
		else if (!strcmp(variables[x], "allproblems"))
			display_all_problems = TRUE;

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

	req_hostgroups[num_req_hostgroups].entry = NULL;
	req_servicegroups[num_req_servicegroups].entry = NULL;
	req_hosts[num_req_hosts].entry = NULL;

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}


/* display table with service status totals... */
void show_service_status_totals(void) {
	int num_services_filtered = 0;
	int num_problems_filtered = 0;
	int num_services_unfiltered = 0;
	int num_problems_unfiltered = 0;
	char status_url[MAX_INPUT_BUFFER];
	char temp_buffer[MAX_INPUT_BUFFER];
	char *style = NULL;

	if (display_status_totals == FALSE || group_style_type == STYLE_HOST_DETAIL)
		return;

	num_services_filtered = num_services_ok + num_services_unknown + num_services_warning + num_services_critical + num_services_pending;
	num_problems_filtered = num_services_unknown + num_services_warning + num_services_critical;

	num_services_unfiltered = num_total_services_ok + num_total_services_unknown + num_total_services_warning + num_total_services_critical + num_total_services_pending;
	num_problems_unfiltered = num_total_services_unknown + num_total_services_warning + num_total_services_critical;

	/* construct url start */
	if (group_style_type == STYLE_OVERVIEW)
		style = strdup("overview");
	else if (group_style_type == STYLE_SUMMARY)
		style = strdup("summary");
	else if (group_style_type == STYLE_GRID)
		style = strdup("grid");
	else
		style = strdup("detail");

	if (search_string != NULL)
		snprintf(status_url, sizeof(status_url) - 1, "%s?search_string=%s&style=%s", STATUS_CGI, url_encode(search_string), style);
	else if (display_all_unhandled_problems == TRUE)
		snprintf(status_url, sizeof(status_url) - 1, "%s?hoststatustypes=%d&serviceprops=%d&style=%s", STATUS_CGI, HOST_UP | HOST_PENDING, service_problems_unhandled, style);
	else if (display_all_problems == TRUE)
		snprintf(status_url, sizeof(status_url) - 1, "%s?hoststatustypes=%d&style=%s", STATUS_CGI, HOST_UP | HOST_PENDING, style);
	else if (display_type == DISPLAY_HOSTS)
		snprintf(status_url, sizeof(status_url) - 1, "%s?%s%s%s&style=%s&hoststatustypes=%d", STATUS_CGI, url_hosts_part, (service_filter != NULL) ? "&servicefilter=" : "", (service_filter != NULL) ? url_encode(service_filter) : "", style, host_status_types);
	else if (display_type == DISPLAY_SERVICEGROUPS)
		snprintf(status_url, sizeof(status_url) - 1, "%s?%s%s%s&style=%s&hoststatustypes=%d", STATUS_CGI, url_servicegroups_part, (service_filter != NULL) ? "&servicefilter=" : "", (service_filter != NULL) ? url_encode(service_filter) : "", style, host_status_types);
	else
		snprintf(status_url, sizeof(status_url) - 1, "%s?%s%s%s&style=%s&hoststatustypes=%d", STATUS_CGI, url_hostgroups_part, (service_filter != NULL) ? "&servicefilter=" : "", (service_filter != NULL) ? url_encode(service_filter) : "", style, host_status_types);

	my_free(style);

	status_url[sizeof(status_url) - 1] = '\x0';

	if (service_properties != 0 && display_all_unhandled_problems == FALSE) {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "&serviceprops=%lu", service_properties);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		strncat(status_url, temp_buffer, sizeof(status_url) - strlen(status_url) - 1);
		status_url[sizeof(status_url) - 1] = '\x0';
	}

	if (host_properties != 0) {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "&hostprops=%lu", host_properties);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		strncat(status_url, temp_buffer, sizeof(status_url) - strlen(status_url) - 1);
		status_url[sizeof(status_url) - 1] = '\x0';
	}

	if (get_result_limit != -1) {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "&limit=%d", result_limit);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		strncat(status_url, temp_buffer, sizeof(status_url) - strlen(status_url) - 1);
		status_url[sizeof(status_url) - 1] = '\x0';
	}

	/* display status totals */
	printf("<DIV CLASS='serviceTotals'>Service Status Totals</DIV>\n");

	printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD>\n");

	printf("<TABLE BORDER=1 CLASS='serviceTotals'>\n");
	printf("<TR>\n");

	printf("<TH CLASS='serviceTotals'><A CLASS='serviceTotals' HREF='%s&servicestatustypes=%d'>Ok</A></TH>\n", status_url, SERVICE_OK);
	printf("<TH CLASS='serviceTotals'><A CLASS='serviceTotals' HREF='%s&servicestatustypes=%d'>Warning</A></TH>\n", status_url, SERVICE_WARNING);
	printf("<TH CLASS='serviceTotals'><A CLASS='serviceTotals' HREF='%s&servicestatustypes=%d'>Unknown</A></TH>\n", status_url, SERVICE_UNKNOWN);
	printf("<TH CLASS='serviceTotals'><A CLASS='serviceTotals' HREF='%s&servicestatustypes=%d'>Critical</A></TH>\n", status_url, SERVICE_CRITICAL);
	printf("<TH CLASS='serviceTotals'><A CLASS='serviceTotals' HREF='%s&servicestatustypes=%d'>Pending</A></TH>\n", status_url, SERVICE_PENDING);

	printf("</TR><TR>\n");

	/* total services ok */
	printf("<TD CLASS='serviceTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_services_ok > 0) ? "OK" : "", (num_total_services_ok > 0) ? "serviceTotalsBGOK" : "", num_services_ok, num_total_services_ok);

	/* total services in warning state */
	printf("<TD CLASS='serviceTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_services_warning > 0) ? "WARNING" : "", (num_total_services_warning > 0) ? "serviceTotalsBGWARNING" : "", num_services_warning, num_total_services_warning);

	/* total services in unknown state */
	printf("<TD CLASS='serviceTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_services_unknown > 0) ? "UNKNOWN" : "", (num_total_services_unknown > 0) ? "serviceTotalsBGUNKNOWN" : "", num_services_unknown, num_total_services_unknown);

	/* total services in critical state */
	printf("<TD CLASS='serviceTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_services_critical > 0) ? "CRITICAL" : "", (num_total_services_critical > 0) ? "serviceTotalsBGCRITICAL" : "", num_services_critical, num_total_services_critical);

	/* total services in pending state */
	printf("<TD CLASS='serviceTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_services_pending > 0) ? "PENDING" : "", (num_total_services_pending > 0) ? "serviceTotalsBGPENDING" : "", num_services_pending, num_total_services_pending);


	printf("</TR>\n");
	printf("</TABLE>\n");

	printf("</TD></TR><TR><TD ALIGN=CENTER>\n");

	printf("<TABLE BORDER=1 CLASS='serviceTotals'>\n");
	printf("<TR>\n");

	printf("<TH CLASS='serviceTotals'><A CLASS='serviceTotals' HREF='%s&servicestatustypes=%d'><I>All Problems</I></A></TH>\n", status_url, all_service_problems);
	printf("<TH CLASS='serviceTotals'><A CLASS='serviceTotals' HREF='%s&servicestatustypes=%d'><I>All Types</I></A></TH>\n", status_url, all_service_status_types);

	printf("</TR><TR>\n");

	/* total service problems */
	printf("<TD CLASS='serviceTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_problems_filtered > 0) ? "PROBLEMS" : "", (num_problems_unfiltered > 0) ? "serviceTotalsBGPROBLEMS" : "", num_problems_filtered, num_problems_unfiltered);

	/* total services */
	printf("<TD CLASS='serviceTotals' text='filtered/unfiltered'>&nbsp;%d / %d&nbsp;</TD>\n", num_services_filtered, num_services_unfiltered);

	printf("</TR>\n");
	printf("</TABLE>\n");

	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	return;
}


/* display a table with host status totals... */
void show_host_status_totals(void) {
	int num_hosts_filtered = 0;
	int num_problems_filtered = 0;
	int num_hosts_unfiltered = 0;
	int num_problems_unfiltered = 0;
	char status_url[MAX_INPUT_BUFFER];
	char temp_buffer[MAX_INPUT_BUFFER];
	char *style = NULL;

	if (display_status_totals == FALSE)
		return;

	num_hosts_filtered = num_hosts_up + num_hosts_down + num_hosts_unreachable + num_hosts_pending;
	num_problems_filtered = num_hosts_down + num_hosts_unreachable;

	num_hosts_unfiltered = num_total_hosts_up + num_total_hosts_down + num_total_hosts_unreachable + num_total_hosts_pending;
	num_problems_unfiltered = num_total_hosts_down + num_total_hosts_unreachable;

	/* construct url start */
	if (group_style_type == STYLE_HOST_SERVICE_DETAIL && display_all_unhandled_problems == FALSE && display_all_problems == FALSE)
		style = strdup("hostservicedetail");
	else if (group_style_type == STYLE_HOST_DETAIL || display_all_unhandled_problems == TRUE || display_all_problems == TRUE)
		style = strdup("hostdetail");
	else if (group_style_type == STYLE_OVERVIEW)
		style = strdup("overview");
	else if (group_style_type == STYLE_SUMMARY)
		style = strdup("summary");
	else if (group_style_type == STYLE_GRID)
		style = strdup("grid");
	else
		style = strdup("detail");

	if (search_string != NULL)
		snprintf(status_url, sizeof(status_url) - 1, "%s?search_string=%s", STATUS_CGI, url_encode(search_string));
	else if (display_all_unhandled_problems == TRUE)
		snprintf(status_url, sizeof(status_url) - 1, "%s?hoststatustypes=%d&hostprops=%d&style=%s", STATUS_CGI, all_host_problems, host_problems_unhandled, style);
	else if (display_all_problems == TRUE)
		snprintf(status_url, sizeof(status_url) - 1, "%s?hoststatustypes=%d&style=%s", STATUS_CGI, all_host_problems, style);
	else if (display_type == DISPLAY_HOSTS)
		snprintf(status_url, sizeof(status_url) - 1, "%s?%s%s%s&style=%s", STATUS_CGI, url_hosts_part, (service_filter != NULL) ? "&servicefilter=" : "", (service_filter != NULL) ? url_encode(service_filter) : "", style);
	else if (display_type == DISPLAY_SERVICEGROUPS)
		snprintf(status_url, sizeof(status_url) - 1, "%s?%s%s%s&style=%s", STATUS_CGI, url_servicegroups_part, (service_filter != NULL) ? "&servicefilter=" : "", (service_filter != NULL) ? url_encode(service_filter) : "", style);
	else
		snprintf(status_url, sizeof(status_url) - 1, "%s?%s%s%s&style=%s", STATUS_CGI, url_hostgroups_part, (service_filter != NULL) ? "&servicefilter=" : "", (service_filter != NULL) ? url_encode(service_filter) : "", style);

	my_free(style);

	status_url[sizeof(status_url) - 1] = '\x0';

	if (service_status_types != all_service_status_types) {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "&servicestatustypes=%d", service_status_types);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		strncat(status_url, temp_buffer, sizeof(status_url) - strlen(status_url) - 1);
		status_url[sizeof(status_url) - 1] = '\x0';
	}

	if (service_properties != 0) {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "&serviceprops=%lu", service_properties);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		strncat(status_url, temp_buffer, sizeof(status_url) - strlen(status_url) - 1);
		status_url[sizeof(status_url) - 1] = '\x0';
	}

	if (host_properties != 0 && display_all_unhandled_problems == FALSE) {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "&hostprops=%lu", host_properties);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		strncat(status_url, temp_buffer, sizeof(status_url) - strlen(status_url) - 1);
		status_url[sizeof(status_url) - 1] = '\x0';
	}

	if (get_result_limit != -1) {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "&limit=%d", result_limit);
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		strncat(status_url, temp_buffer, sizeof(status_url) - strlen(status_url) - 1);
		status_url[sizeof(status_url) - 1] = '\x0';
	}

	/* display status totals */
	printf("<DIV CLASS='hostTotals'>Host Status Totals</DIV>\n");

	printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD>\n");


	printf("<TABLE BORDER=1 CLASS='hostTotals'>\n");
	printf("<TR>\n");

	printf("<TH CLASS='hostTotals'><A CLASS='hostTotals' HREF='%s&hoststatustypes=%d'>Up</A></TH>\n", status_url, HOST_UP);
	printf("<TH CLASS='hostTotals'><A CLASS='hostTotals' HREF='%s&hoststatustypes=%d'>Down</A></TH>\n", status_url, HOST_DOWN);
	printf("<TH CLASS='hostTotals'><A CLASS='hostTotals' HREF='%s&hoststatustypes=%d'>Unreachable</A></TH>\n", status_url, HOST_UNREACHABLE);
	printf("<TH CLASS='hostTotals'><A CLASS='hostTotals' HREF='%s&hoststatustypes=%d'>Pending</A></TH>\n", status_url, HOST_PENDING);

	printf("</TR><TR>\n");

	/* total hosts up */
	printf("<TD CLASS='hostTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_hosts_up > 0) ? "UP" : "", (num_total_hosts_up > 0) ? "hostTotalsBGUP" : "", num_hosts_up, num_total_hosts_up);

	/* total hosts down */
	printf("<TD CLASS='hostTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_hosts_down > 0) ? "DOWN" : "", (num_total_hosts_down > 0) ? "hostTotalsBGDOWN" : "", num_hosts_down, num_total_hosts_down);

	/* total hosts unreachable */
	printf("<TD CLASS='hostTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_hosts_unreachable > 0) ? "UNREACHABLE" : "", (num_total_hosts_unreachable > 0) ? "hostTotalsBGUNREACHABLE" : "", num_hosts_unreachable, num_total_hosts_unreachable);

	/* total hosts pending */
	printf("<TD CLASS='hostTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_hosts_pending > 0) ? "PENDING" : "", (num_total_hosts_pending > 0) ? "hostTotalsBGPENDING" : "", num_hosts_pending, num_total_hosts_pending);

	printf("</TR>\n");
	printf("</TABLE>\n");

	printf("</TD></TR><TR><TD ALIGN=CENTER>\n");

	printf("<TABLE BORDER=1 CLASS='hostTotals'>\n");
	printf("<TR>\n");

	printf("<TH CLASS='hostTotals'><A CLASS='hostTotals' HREF='%s&hoststatustypes=%d'><I>All Problems</I></A></TH>\n", status_url, all_host_problems);
	printf("<TH CLASS='hostTotals'><A CLASS='hostTotals' HREF='%s&hoststatustypes=%d'><I>All Types</I></A></TH>\n", status_url, all_host_status_types);

	printf("</TR><TR>\n");

	/* total hosts with problems */
	printf("<TD CLASS='hostTotals%s %s'>&nbsp;%d / %d&nbsp;</TD>\n", (num_problems_filtered > 0) ? "PROBLEMS" : "", (num_problems_unfiltered > 0) ? "hostTotalsBGPROBLEMS" : "", num_problems_filtered, num_problems_unfiltered);

	/* total hosts */
	printf("<TD CLASS='hostTotals'>&nbsp;%d / %d&nbsp;</TD>\n", num_hosts_filtered, num_hosts_unfiltered);

	printf("</TR>\n");
	printf("</TABLE>\n");

	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	return;
}


/* display a detailed listing of the status of all services... */
void show_service_detail(void) {
	char *status = NULL;
	char temp_buffer[MAX_INPUT_BUFFER];
	char *temp_url = NULL;
	char *processed_string = NULL;
	char *status_class = "";
	char *status_bg_class = "";
	char *host_status_bg_class = "";
	char *last_host = "";
	char *style = "";
	hoststatus *temp_hoststatus = NULL;
	host *temp_host = NULL;
	service *temp_service = NULL;
	statusdata *temp_status = NULL;
	sort *temp_sort = NULL;
	int new_host = FALSE;
	int odd = 0;
	int total_comments = 0;
	int use_sort = FALSE;
	int result = OK;
	int first_entry = TRUE;
	int json_start = TRUE;
	int service_start = 0;
	int service_limit = 0;

	/* sort status data if necessary */
	if (sort_type != SORT_NONE && sort_object == SERVICE_STATUS) {
		result = sort_status_data(SERVICE_STATUS, sort_type, sort_option);
		if (result == ERROR)
			use_sort = FALSE;
		else
			use_sort = TRUE;
	} else
		use_sort = FALSE;

	if (content_type == JSON_CONTENT)
		printf("\"service_status\": [\n");
	else if (content_type == CSV_CONTENT) {
		printf("%sHost%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sService%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sStatus%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLast_Check%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDuration%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAttempt%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sStatus_Information%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sIs_Flapping%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sIn_Scheduled_Downtime%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sActive_Checks_Enabled%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sPassive_Checks_Enabled%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotifications_Enabled%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sProblem_Acknowledged%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAction_URL%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes_URL%s\n", csv_data_enclosure, csv_data_enclosure);
	} else {
		printf("<table style='margin-top:5px;' cellspacing='0' cellpadding='0' border=0 width=100%%>\n");
		printf("<tr>\n");

		printf("<td valign=top align=left width=33%%>\n");

		if (display_header == TRUE && group_style_type != STYLE_HOST_SERVICE_DETAIL)
			show_filters();

		printf("</td>");

		printf("<td valign=top align=center width=33%%>\n");

		printf("<DIV class='statusTitle'>Service Status Details For ");
		print_displayed_names(display_type);
		printf("</DIV>\n");

		if (use_sort == TRUE) {
			printf("<DIV ALIGN=CENTER CLASS='statusSort'>Entries sorted by <b>");
			if (sort_option == SORT_HOSTNAME)
				printf("host name");
			else if (sort_option == SORT_SERVICENAME)
				printf("service name");
			else if (sort_option == SORT_SERVICESTATUS)
				printf("service status");
			else if (sort_option == SORT_LASTCHECKTIME)
				printf("last check time");
			else if (sort_option == SORT_CURRENTATTEMPT)
				printf("attempt number");
			else if (sort_option == SORT_STATEDURATION)
				printf("state duration");
			printf("</b> (%s)\n", (sort_type == SORT_ASCENDING) ? "ascending" : "descending");
			printf("</DIV>\n");
		}

		if (group_style_type != STYLE_HOST_SERVICE_DETAIL) {
			printf("<div class='page_selector'>\n");
			printf("<div id='page_navigation_copy'></div>");
			page_limit_selector(result_start);
			printf("</div>\n");
		}
		printf("</td>\n");

		/* add export to csv, json, link */
		printf("<td valign=bottom width=33%%>");
		printf("<div style='padding-right:5px;' class='csv_export_link'>");
		print_export_link(CSV_CONTENT, STATUS_CGI, NULL);
		print_export_link(JSON_CONTENT, STATUS_CGI, NULL);
		print_export_link(HTML_CONTENT, STATUS_CGI, NULL);
		printf("</div></td>\n");

		printf("</tr>\n");
		printf("</table>\n");

		/* construct sort url start */
		if (group_style_type == STYLE_HOST_SERVICE_DETAIL)
			style = strdup("hostservicedetail");
		else
			style = strdup("detail");

		if (search_string != NULL)
			dummy = asprintf(&temp_url, "%s?search_string=%s&sortobject=services", STATUS_CGI, url_encode(search_string));
		else if (display_all_unhandled_problems == TRUE || display_all_problems == TRUE)
			dummy = asprintf(&temp_url, "%s?all%sproblems&sortobject=services", STATUS_CGI, (display_all_unhandled_problems == TRUE) ? "unhandled" : "");
		else if (display_type == DISPLAY_HOSTS)
			dummy = asprintf(&temp_url, "%s?%s&style=%s&sortobject=services", STATUS_CGI, url_hosts_part, style);
		else if (display_type == DISPLAY_SERVICEGROUPS)
			dummy = asprintf(&temp_url, "%s?%s&style=%s&sortobject=services", STATUS_CGI, url_servicegroups_part, style);
		else
			dummy = asprintf(&temp_url, "%s?%s&style=%s&sortobject=services", STATUS_CGI, url_hostgroups_part, style);

		if (display_all_unhandled_problems == FALSE && display_all_problems == FALSE) {
			if (service_status_types != all_service_status_types) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&servicestatustypes=%d", temp_buffer, service_status_types);
			}
			if (host_status_types != all_host_status_types) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&hoststatustypes=%d", temp_buffer, host_status_types);
			}
			if (service_properties != 0) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&serviceprops=%lu", temp_buffer, service_properties);
			}
			if (host_properties != 0) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&hostprops=%lu", temp_buffer, host_properties);
			}
			if (service_filter != NULL) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&servicefilter=%s", temp_buffer, url_encode(service_filter));
			}
		}

		my_free(style);

		/* add limit to sort url's */
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "&limit=%d", result_limit);
		if (get_result_limit != -1)
			temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		else
			temp_buffer[0] = '\x0';

		/* the main list of services */
		printf("<TABLE BORDER=0 width=100%% CLASS='status' align='center'>\n");

		printf("<TR>\n");

		printf("<TH CLASS='status'>Host&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by host name (ascending)' TITLE='Sort by host name (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by host name (descending)' TITLE='Sort by host name (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_HOSTNAME, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_HOSTNAME, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Service&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by service name (ascending)' TITLE='Sort by service name (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by service name (descending)' TITLE='Sort by service name (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_SERVICENAME, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_SERVICENAME, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Status&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by service status (ascending)' TITLE='Sort by service status (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by service status (descending)' TITLE='Sort by service status (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_SERVICESTATUS, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_SERVICESTATUS, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Last Check&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by last check time (ascending)' TITLE='Sort by last check time (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by last check time (descending)' TITLE='Sort by last check time (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_LASTCHECKTIME, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_LASTCHECKTIME, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Duration&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by state duration (ascending)' TITLE='Sort by state duration (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by state duration time (descending)' TITLE='Sort by state duration time (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_STATEDURATION, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_STATEDURATION, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Attempt&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by current attempt (ascending)' TITLE='Sort by current attempt (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by current attempt (descending)' TITLE='Sort by current attempt (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_CURRENTATTEMPT, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_CURRENTATTEMPT, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Status Information</TH>\n");

		if (is_authorized_for_read_only(&current_authdata) == FALSE) {
			/* Add checkbox so every service can be checked */
			printf("<TH CLASS='status' width='16'><input type='checkbox' value=all onclick=\"checkAll('tableformservice');isValidForSubmit('tableformservice');\"></TH>\n");
		}

		printf("</TR>\n");

		my_free(temp_url);
	}

//	result_limit = 5;
//	result_start = 2;
//	total_hosts_entries = 4;
//	displayed_host_entries = 3;

//	result_limit = 2;
//	result_start = 7;
//	total_hosts_entries = 4;
//	displayed_host_entries = 0;

	service_start = result_start;
	service_limit = result_limit;

	if (group_style_type == STYLE_HOST_SERVICE_DETAIL) {
		if (result_start == 1) {
			if (result_limit == displayed_host_entries)
				service_limit = 0;
			else
				service_limit = result_limit - displayed_host_entries;
		} else {
			if (result_start <= (total_host_entries + 1)) {
				service_start = 1;
				service_limit = result_limit - displayed_host_entries;
			} else {
				service_start = result_start - total_host_entries;
			}
		}
	}

	while (1) {

		/* get the next service to display */
		if (use_sort == TRUE) {
			if (first_entry == TRUE)
				temp_sort = statussort_list;
			else
				temp_sort = temp_sort->next;
			if (temp_sort == NULL)
				break;
			temp_status = temp_sort->status;
		} else {
			if (first_entry == TRUE)
				temp_status = statusdata_list;
			else
				temp_status = temp_status->next;
		}

		if (temp_status == NULL)
			break;

		first_entry = FALSE;

		if (temp_status->type != SERVICE_STATUS)
			continue;

		/* find the host */
		temp_host = find_host(temp_status->host_name);

		/* find the service  */
		temp_service = find_service(temp_status->host_name, temp_status->svc_description);

		if (temp_service == NULL)
			continue;

		if (result_limit != 0  && (((total_service_entries + 1) < service_start) || (total_service_entries >= ((service_start + service_limit) - 1)))) {
			total_service_entries++;
			continue;
		}

		if (strcmp(last_host, temp_status->host_name) || displayed_service_entries == 0)
			new_host = TRUE;
		else
			new_host = FALSE;

		if (new_host == TRUE && content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
			if (strcmp(last_host, ""))
				printf("<TR><TD colspan=6></TD></TR>\n");
		}

		if (odd)
			odd = 0;
		else
			odd = 1;

		/* keep track of total number of services we're displaying */
		displayed_service_entries++;
		total_service_entries++;

		status = temp_status->status_string;
		status_class = temp_status->status_string;
		if (temp_status->status == SERVICE_PENDING) {
			status_bg_class = (odd) ? "Even" : "Odd";
		} else if (suppress_maintenance_downtime == TRUE && temp_status->scheduled_downtime_depth > 0) {
			status_class = "DOWNTIME";
			status_bg_class = (odd) ? "Even" : "Odd";
		} else if (temp_status->status == SERVICE_OK) {
			status_bg_class = (odd) ? "Even" : "Odd";
		} else if (temp_status->status == SERVICE_WARNING) {
			if (temp_status->problem_has_been_acknowledged == TRUE)
				status_bg_class = "BGWARNINGACK";
			else if (temp_status->scheduled_downtime_depth > 0)
				status_bg_class = "BGWARNINGSCHED";
			else
				status_bg_class = "BGWARNING";
		} else if (temp_status->status == SERVICE_UNKNOWN) {
			if (temp_status->problem_has_been_acknowledged == TRUE)
				status_bg_class = "BGUNKNOWNACK";
			else if (temp_status->scheduled_downtime_depth > 0)
				status_bg_class = "BGUNKNOWNSCHED";
			else
				status_bg_class = "BGUNKNOWN";
		} else if (temp_status->status == SERVICE_CRITICAL) {
			if (temp_status->problem_has_been_acknowledged == TRUE)
				status_bg_class = "BGCRITICALACK";
			else if (temp_status->scheduled_downtime_depth > 0)
				status_bg_class = "BGCRITICALSCHED";
			else
				status_bg_class = "BGCRITICAL";
		}

		if (content_type == HTML_CONTENT) {

			printf("<TR>\n");

			/* host name column */
			if (new_host == TRUE) {

				/* get the host status information */
				temp_hoststatus = find_hoststatus(temp_status->host_name);

				/* grab macros */
				grab_host_macros_r(mac, temp_host);

				/* first, we color it as maintenance if that is preferred */
				if (suppress_maintenance_downtime == TRUE && temp_hoststatus->scheduled_downtime_depth > 0) {
					host_status_bg_class = "HOSTDOWNTIME";

					/* otherwise we color it as its appropriate state */
				} else if (temp_hoststatus->status == HOST_DOWN) {
					if (temp_hoststatus->problem_has_been_acknowledged == TRUE)
						host_status_bg_class = "HOSTDOWNACK";
					else if (temp_hoststatus->scheduled_downtime_depth > 0)
						host_status_bg_class = "HOSTDOWNSCHED";
					else
						host_status_bg_class = "HOSTDOWN";
				} else if (temp_hoststatus->status == HOST_UNREACHABLE) {
					if (temp_hoststatus->problem_has_been_acknowledged == TRUE)
						host_status_bg_class = "HOSTUNREACHABLEACK";
					else if (temp_hoststatus->scheduled_downtime_depth > 0)
						host_status_bg_class = "HOSTUNREACHABLESCHED";
					else
						host_status_bg_class = "HOSTUNREACHABLE";
				} else
					host_status_bg_class = (odd) ? "Even" : "Odd";

				printf("<TD CLASS='status%s'>", host_status_bg_class);

				printf("<TABLE BORDER=0 WIDTH='100%%' cellpadding=0 cellspacing=0>\n");
				printf("<TR>\n");
				printf("<TD ALIGN=LEFT>\n");
				printf("<TABLE BORDER=0 cellpadding=0 cellspacing=0>\n");
				printf("<TR>\n");
				if (!strcmp(temp_host->address6, temp_host->name))
					printf("<TD align=left valign=center CLASS='status%s'><A HREF='%s?type=%d&host=%s' title='%s'>%s</A></TD>\n", host_status_bg_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_status->host_name), temp_host->address, (temp_host->display_name != NULL) ? html_encode(temp_host->display_name, TRUE) : html_encode(temp_host->name, TRUE));
				else
					printf("<TD align=left valign=center CLASS='status%s'><A HREF='%s?type=%d&host=%s' title='%s,%s'>%s</A></TD>\n", host_status_bg_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_status->host_name), temp_host->address, temp_host->address6, (temp_host->display_name != NULL) ? html_encode(temp_host->display_name, TRUE) : html_encode(temp_host->name, TRUE));

				printf("</TR>\n");
				printf("</TABLE>\n");
				printf("</TD>\n");
				printf("<TD align=right valign=center>\n");
				printf("<TABLE BORDER=0 cellpadding=0 cellspacing=0>\n");
				printf("<TR>\n");
				total_comments = number_of_host_comments(temp_host->name);
				if (temp_hoststatus->problem_has_been_acknowledged == TRUE) {
					printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s#comments'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='This host problem has been acknowledged' TITLE='This host problem has been acknowledged'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_status->host_name), url_images_path, ACKNOWLEDGEMENT_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				}
				if (is_authorized_for_read_only(&current_authdata) == FALSE || is_authorized_for_comments_read_only(&current_authdata) == TRUE) {
					if (total_comments > 0)
						print_comment_icon(temp_host->name, NULL);
				}
				if (temp_hoststatus->notifications_enabled == FALSE) {
					printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='Notifications for this host have been disabled' TITLE='Notifications for this host have been disabled'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_status->host_name), url_images_path, NOTIFICATIONS_DISABLED_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				}
				if (temp_hoststatus->checks_enabled == FALSE) {
					if (temp_hoststatus->accept_passive_host_checks == TRUE)
						printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='Active Checks of this host have been disabled'd TITLE='Active Checks of this host have been disabled'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_status->host_name), url_images_path, PASSIVE_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
					else
						printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='Active and Passive Checks of this host have been disabled'd TITLE='Active and Passive Checks of this host have been disabled'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_status->host_name), url_images_path, DISABLED_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				}
				if (temp_hoststatus->is_flapping == TRUE) {
					printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='This host is flapping between states' TITLE='This host is flapping between states'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_status->host_name), url_images_path, FLAPPING_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				}
				if (temp_hoststatus->scheduled_downtime_depth > 0) {
					printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='This host is currently in a period of scheduled downtime' TITLE='This host is currently in a period of scheduled downtime'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_status->host_name), url_images_path, DOWNTIME_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				}
				if (temp_host->notes_url != NULL) {
					process_macros_r(mac, temp_host->notes_url, &processed_string, 0);
					BEGIN_MULTIURL_LOOP
					printf("<TD align=center valign=center>");
					printf("<A HREF='");
					printf("%s", processed_string);
					printf("' TARGET='%s'>", (notes_url_target == NULL) ? "_blank" : notes_url_target);
					printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, NOTES_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "View Extra Host Notes", "View Extra Host Notes");
					printf("</A>");
					printf("</TD>\n");
					END_MULTIURL_LOOP
					free(processed_string);
				}
				if (temp_host->action_url != NULL) {
					process_macros_r(mac, temp_host->action_url, &processed_string, 0);
					BEGIN_MULTIURL_LOOP
					printf("<TD align=center valign=center>");
					printf("<A HREF='");
					printf("%s", processed_string);
					printf("' TARGET='%s'>", (action_url_target == NULL) ? "_blank" : action_url_target);
//					printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, ACTION_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "Perform Extra Host Actions", "Perform Extra Host Actions");
					printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d>", url_images_path, MU_iconstr, ACTION_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
					printf("</A>");
					printf("</TD>\n");
					END_MULTIURL_LOOP
					free(processed_string);
				}
				if (temp_host->icon_image != NULL) {
					printf("<TD align=center valign=center>");
					printf("<A HREF='%s?type=%d&host=%s'>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_status->host_name));
					printf("<IMG SRC='%s", url_logo_images_path);
					process_macros_r(mac, temp_host->icon_image, &processed_string, 0);
					printf("%s", processed_string);
					free(processed_string);
					printf("' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE), (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE));
					printf("</A>");
					printf("</TD>\n");
				}
				printf("</TR>\n");
				printf("</TABLE>\n");
				printf("</TD>\n");
				printf("</TR>\n");
				printf("</TABLE>\n");
			} else
				printf("<TD>");
			printf("</TD>\n");

			/* grab macros */
			grab_service_macros_r(mac, temp_service);

			/* service name column */
			printf("<TD CLASS='status%s'>", status_bg_class);
			printf("<TABLE BORDER=0 WIDTH='100%%' CELLSPACING=0 CELLPADDING=0>");
			printf("<TR>");
			printf("<TD ALIGN=LEFT>");
			printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0>\n");
			printf("<TR>\n");
			printf("<TD ALIGN=LEFT valign=center CLASS='status%s'><A HREF='%s?type=%d&host=%s", status_bg_class, EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_status->host_name));
			printf("&service=%s'>%s</A></TD>", url_encode(temp_status->svc_description), (temp_service->display_name != NULL) ? html_encode(temp_service->display_name, TRUE) : html_encode(temp_service->description, TRUE));
			printf("</TR>\n");
			printf("</TABLE>\n");
			printf("</TD>\n");
			printf("<TD ALIGN=RIGHT CLASS='status%s'>\n", status_bg_class);
			printf("<TABLE BORDER=0 cellspacing=0 cellpadding=0>\n");
			printf("<TR>\n");
			total_comments = number_of_service_comments(temp_service->host_name, temp_service->description);
			if (temp_status->problem_has_been_acknowledged == TRUE) {
				printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_status->host_name));
				printf("&service=%s#comments'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='This service problem has been acknowledged' TITLE='This service problem has been acknowledged'></A></TD>", url_encode(temp_status->svc_description), url_images_path, ACKNOWLEDGEMENT_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
			}
			if (is_authorized_for_read_only(&current_authdata) == FALSE || is_authorized_for_comments_read_only(&current_authdata) == TRUE) {
				if (total_comments > 0) {
					print_comment_icon(temp_host->name, temp_service->description);
				}
			}
			if (temp_status->checks_enabled == FALSE) {
				if (temp_status->accept_passive_checks == FALSE) {
					printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_status->host_name));
					printf("&service=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='Active and passive checks have been disabled for this service' TITLE='Active and passive checks have been disabled for this service'></A></TD>", url_encode(temp_status->svc_description), url_images_path, DISABLED_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				} else {
					printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_status->host_name));
					printf("&service=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='Active checks of the service have been disabled - only passive checks are being accepted' TITLE='Active checks of the service have been disabled - only passive checks are being accepted'></A></TD>", url_encode(temp_status->svc_description), url_images_path, PASSIVE_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				}
			}
			if (temp_status->notifications_enabled == FALSE) {
				printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_status->host_name));
				printf("&service=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='Notifications for this service have been disabled' TITLE='Notifications for this service have been disabled'></A></TD>", url_encode(temp_status->svc_description), url_images_path, NOTIFICATIONS_DISABLED_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
			}
			if (temp_status->is_flapping == TRUE) {
				printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_status->host_name));
				printf("&service=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='This service is flapping between states' TITLE='This service is flapping between states'></A></TD>", url_encode(temp_status->svc_description), url_images_path, FLAPPING_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
			}
			if (temp_status->scheduled_downtime_depth > 0) {
				printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_status->host_name));
				printf("&service=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='This service is currently in a period of scheduled downtime' TITLE='This service is currently in a period of scheduled downtime'></A></TD>", url_encode(temp_status->svc_description), url_images_path, DOWNTIME_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
			}
			if (temp_service->notes_url != NULL) {
				process_macros_r(mac, temp_service->notes_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<TD align=center valign=center>");
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'>", (notes_url_target == NULL) ? "_blank" : notes_url_target);
				printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, NOTES_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "View Extra Service Notes", "View Extra Service Notes");
				printf("</A>");
				printf("</TD>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_service->action_url != NULL) {
				process_macros_r(mac, temp_service->action_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<TD align=center valign=center>");
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'>", (action_url_target == NULL) ? "_blank" : action_url_target);
				printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d>", url_images_path, MU_iconstr, ACTION_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				printf("</A>");
				printf("</TD>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_service->icon_image != NULL) {
				printf("<TD ALIGN=center valign=center>");
				printf("<A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_service->host_name));
				printf("&service=%s'>", url_encode(temp_service->description));
				printf("<IMG SRC='%s", url_logo_images_path);
				process_macros_r(mac, temp_service->icon_image, &processed_string, 0);
				printf("%s", processed_string);
				free(processed_string);
				printf("' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, (temp_service->icon_image_alt == NULL) ? "" : html_encode(temp_service->icon_image_alt, TRUE), (temp_service->icon_image_alt == NULL) ? "" : html_encode(temp_service->icon_image_alt, TRUE));
				printf("</A>");
				printf("</TD>\n");
			}
			if (enable_splunk_integration == TRUE) {
				printf("<TD ALIGN=center valign=center>");
				display_splunk_service_url(temp_service);
				printf("</TD>\n");
			}
			printf("</TR>\n");
			printf("</TABLE>\n");
			printf("</TD>\n");
			printf("</TR>");
			printf("</TABLE>");
			printf("</TD>\n");


			/* the rest of the columns... */
			printf("<TD onClick=\"toggle_checkbox('service_%d','tableformservice');\" CLASS='status%s'>%s</TD>\n", total_service_entries, status_class, temp_status->status_string);
			printf("<TD onClick=\"toggle_checkbox('service_%d','tableformservice');\" CLASS='status%s' nowrap>%s</TD>\n", total_service_entries, status_bg_class, temp_status->last_check);
			printf("<TD onClick=\"toggle_checkbox('service_%d','tableformservice');\" CLASS='status%s' nowrap>%s</TD>\n", total_service_entries, status_bg_class, temp_status->state_duration);
			printf("<TD onClick=\"toggle_checkbox('service_%d','tableformservice');\" CLASS='status%s'>%s</TD>\n", total_service_entries, status_bg_class, temp_status->attempts);
			printf("<TD onClick=\"toggle_checkbox('service_%d','tableformservice');\" CLASS='status%s' valign='center'>%s</TD>\n", total_service_entries, status_bg_class, temp_status->plugin_output);

			/* Checkbox for service(s) */
			if (is_authorized_for_read_only(&current_authdata) == FALSE) {
				printf("<TD onClick=\"toggle_checkbox('service_%d','tableformservice');\" CLASS='status%s' nowrap align='center'>", total_service_entries, status_bg_class);
				printf("<input onClick=\"toggle_checkbox('service_%d','tableformservice');\" type='checkbox' id='service_%d' name='hostservice' value='%s^%s'></TD>\n", total_service_entries, total_service_entries, temp_status->host_name, temp_status->svc_description);
			}

			if (enable_splunk_integration == TRUE)
				display_splunk_service_url(temp_service);


			printf("</TR>\n");
		}

		/* print list in json format */
		if (content_type == JSON_CONTENT) {
			/* always add a comma, except for the first line */
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
			printf("{ \"host_name\": \"%s\", ", json_encode(temp_host->name));
			printf("\"host_display_name\": \"%s\", ", (temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(temp_host->name));
			printf("\"service_description\": \"%s\", ", json_encode(temp_service->description));
			printf("\"service_display_name\": \"%s\", ", (temp_service->display_name != NULL) ? json_encode(temp_service->display_name) : json_encode(temp_service->description));
			printf("\"status\": \"%s\", ", temp_status->status_string);
			printf("\"last_check\": \"%s\", ", temp_status->last_check);
			printf("\"duration\": \"%s\", ", temp_status->state_duration);
			printf("\"attempts\": \"%s\", ", temp_status->attempts);
			printf("\"state_type\": \"%s\", ", (temp_status->state_type == HARD_STATE) ? "HARD" : "SOFT");
			printf("\"is_flapping\": %s, ", (temp_status->is_flapping == TRUE) ? "true" : "false");
			printf("\"in_scheduled_downtime\": %s, ", (temp_status->scheduled_downtime_depth > 0) ? "true" : "false");
			printf("\"active_checks_enabled\": %s, ", (temp_status->checks_enabled == TRUE) ? "true" : "false");
			printf("\"passive_checks_enabled\": %s, ", (temp_status->accept_passive_checks == TRUE) ? "true" : "false");
			printf("\"notifications_enabled\": %s, ", (temp_status->notifications_enabled == TRUE) ? "true" : "false");
			printf("\"has_been_acknowledged\": %s, ", (temp_status->problem_has_been_acknowledged == TRUE) ? "true" : "false");

			if (temp_service->action_url == NULL)
				printf("\"action_url\": null, ");
			else
				printf("\"action_url\": \"%s\", ", json_encode(temp_service->action_url));

			if (temp_service->notes_url == NULL)
				printf("\"notes_url\": null, ");
			else
				printf("\"notes_url\": \"%s\", ", json_encode(temp_service->notes_url));

			if (temp_status->plugin_output == NULL)
				printf("\"status_information\": null }");
			else
				printf("\"status_information\": \"%s\"}", json_encode(temp_status->plugin_output));

			/* print list in csv format */
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, temp_host->name, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_service->description, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_status->status_string, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_status->last_check, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_status->state_duration, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_status->attempts, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_status->plugin_output == NULL) ? "" : temp_status->plugin_output, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_status->is_flapping == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_status->scheduled_downtime_depth > 0) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_status->checks_enabled == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_status->accept_passive_checks == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_status->notifications_enabled == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_status->problem_has_been_acknowledged == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_service->action_url != NULL) ? temp_service->action_url : "", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s\n", csv_data_enclosure, (temp_service->notes_url != NULL) ? temp_service->notes_url : "", csv_data_enclosure);
		}

		if (displayed_service_entries != 0)
			last_host = temp_status->host_name;
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</FORM>\n");

		/* if user couldn't see anything, print out some helpful info... */
		if (total_service_entries == 0 && user_is_authorized_for_statusdata == FALSE)
			print_generic_error_message("It appears as though you do not have permission to view information for any of the services you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		else
			status_page_num_selector(service_start, SERVICE_STATUS);
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	/* free memory allocated to the sort lists */
	if (use_sort == TRUE)
		free_sort_list();

	return;
}


/* display a detailed listing of the status of all hosts... */
void show_host_detail(void) {
	char *status = NULL;
	char temp_buffer[MAX_INPUT_BUFFER];
	char *temp_url = NULL;
	char *processed_string = NULL;
	char *status_class = "";
	char *status_bg_class = "";
	char *style = NULL;
	host *temp_host = NULL;
	sort *temp_sort = NULL;
	statusdata *temp_statusdata = NULL;
	int odd = 0;
	int total_comments = 0;
	int use_sort = FALSE;
	int result = OK;
	int first_entry = TRUE;
	int json_start = TRUE;

	/* sort status data if necessary */
	if (sort_type != SORT_NONE && sort_object == HOST_STATUS) {
		result = sort_status_data(HOST_STATUS, sort_type, sort_option);
		if (result == ERROR)
			use_sort = FALSE;
		else
			use_sort = TRUE;
	} else
		use_sort = FALSE;

	if (content_type == JSON_CONTENT)
		printf("\"host_status\": [\n");
	else if (content_type == CSV_CONTENT) {
		printf("%sHost%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sStatus%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLast_Check%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDuration%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAttempt%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sStatus_Information%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sIs_Flapping%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sIn_Scheduled_Downtime%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sActive_Checks_Enabled%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sPassive_Checks_Enabled%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotifications_Enabled%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sProblem_Acknowledged%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAction_URL%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes_URL%s\n", csv_data_enclosure, csv_data_enclosure);
	} else {
		printf("<table style='margin-top:5px;' cellspacing='0' cellpadding='0' border=0 width=100%%>\n");
		printf("<tr>\n");

		printf("<td valign=top align=left width=33%%>\n");

		if (display_header == TRUE)
			show_filters();

		printf("</td>");

		printf("<td valign=top align=center width=33%%>\n");

		printf("<DIV class='statusTitle'>Host Status Details For ");
		print_displayed_names(display_type);
		printf("</DIV>\n");

		if (use_sort == TRUE) {
			printf("<DIV ALIGN=CENTER CLASS='statusSort'>Entries sorted by <b>");
			if (sort_option == SORT_HOSTNAME)
				printf("host name");
			else if (sort_option == SORT_HOSTSTATUS)
				printf("host status");
			else if (sort_option == SORT_HOSTURGENCY)
				printf("host urgency");
			else if (sort_option == SORT_LASTCHECKTIME)
				printf("last check time");
			else if (sort_option == SORT_CURRENTATTEMPT)
				printf("attempt number");
			else if (sort_option == SORT_STATEDURATION)
				printf("state duration");
			printf("</b> (%s)\n", (sort_type == SORT_ASCENDING) ? "ascending" : "descending");
			printf("</DIV>\n");
		}

		printf("<div class='page_selector'>\n");
		printf("<div id='page_navigation_copy'></div>");
		page_limit_selector(result_start);
		printf("</div></td>\n");

		/* add export to csv, json, link */
		printf("<td valign=bottom width=33%%>");
		printf("<div style='padding-right:3px;' class='csv_export_link'>");
		print_export_link(CSV_CONTENT, STATUS_CGI, NULL);
		print_export_link(JSON_CONTENT, STATUS_CGI, NULL);
		print_export_link(HTML_CONTENT, STATUS_CGI, NULL);
		printf("</div></td>\n");

		printf("</tr>\n");
		printf("</table>\n");

		/* construct sort url start */
		if (group_style_type == STYLE_HOST_SERVICE_DETAIL)
			style = strdup("hostservicedetail");
		else
			style = strdup("hostdetail");

		if (search_string != NULL)
			dummy = asprintf(&temp_url, "%s?search_string=%s&sortobject=hosts", STATUS_CGI, url_encode(search_string));
		else if (display_all_unhandled_problems == TRUE || display_all_problems == TRUE)
			dummy = asprintf(&temp_url, "%s?all%sproblems&sortobject=hosts", STATUS_CGI, (display_all_unhandled_problems == TRUE) ? "unhandled" : "");
		else if (display_type == DISPLAY_HOSTS)
			dummy = asprintf(&temp_url, "%s?%s&style=%s&sortobject=hosts", STATUS_CGI, url_hosts_part, style);
		else if (display_type == DISPLAY_SERVICEGROUPS)
			dummy = asprintf(&temp_url, "%s?%s&style=%s&sortobject=hosts", STATUS_CGI, url_servicegroups_part, style);
		else
			dummy = asprintf(&temp_url, "%s?%s&style=%s&sortobject=hosts", STATUS_CGI, url_hostgroups_part, style);

		if (display_all_unhandled_problems == FALSE && display_all_problems == FALSE) {
			if (service_status_types != all_service_status_types) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&servicestatustypes=%d", temp_buffer, service_status_types);
			}
			if (host_status_types != all_host_status_types) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&hoststatustypes=%d", temp_buffer, host_status_types);
			}
			if (service_properties != 0) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&serviceprops=%lu", temp_buffer, service_properties);
			}
			if (host_properties != 0) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&hostprops=%lu", temp_buffer, host_properties);
			}
			if (service_filter != NULL) {
				strncpy(temp_buffer, temp_url, sizeof(temp_buffer));
				my_free(temp_url);
				dummy = asprintf(&temp_url, "%s&servicefilter=%s", temp_buffer, url_encode(service_filter));
			}
		}

		my_free(style);

		/* add limit to sort url's */
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "&limit=%d", result_limit);
		if (get_result_limit != -1)
			temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		else
			temp_buffer[0] = '\x0';

		/* the main list of hosts */
		printf("<TABLE BORDER=0 width=100%% CLASS='status' align='center'>\n");

		printf("<TR>\n");

		printf("<TH CLASS='status'>Host&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by host name (ascending)' TITLE='Sort by host name (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by host name (descending)' TITLE='Sort by host name (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_HOSTNAME, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_HOSTNAME, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Status&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by host status (ascending)' TITLE='Sort by host status (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by host status (descending)' TITLE='Sort by host status (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_HOSTSTATUS, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_HOSTSTATUS, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Last Check&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by last check time (ascending)' TITLE='Sort by last check time (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by last check time (descending)' TITLE='Sort by last check time (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_LASTCHECKTIME, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_LASTCHECKTIME, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Duration&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by state duration (ascending)' TITLE='Sort by state duration (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by state duration time (descending)' TITLE='Sort by state duration time (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_STATEDURATION, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_STATEDURATION, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Attempt&nbsp;<A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by current attempt (ascending)' TITLE='Sort by current attempt (ascending)'></A><A HREF='%s%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='Sort by current attempt (descending)' TITLE='Sort by current attempt (descending)'></A></TH>", temp_url, temp_buffer, SORT_ASCENDING, SORT_CURRENTATTEMPT, url_images_path, UP_ARROW_ICON, temp_url, temp_buffer, SORT_DESCENDING, SORT_CURRENTATTEMPT, url_images_path, DOWN_ARROW_ICON);

		printf("<TH CLASS='status'>Status Information</TH>\n");

		if (is_authorized_for_read_only(&current_authdata) == FALSE) {
			/* Add a checkbox so every host can be checked */
			printf("<TH CLASS='status' width='16'><input type='checkbox' value=all onclick=\"checkAll('tableformhost');isValidForSubmit('tableformhost');\"></TH>\n");
		}

		printf("</TR>\n");

		my_free(temp_url);
	}



	/* check all hosts... */
	while (1) {

		/* get the next host to display */
		if (use_sort == TRUE) {
			if (first_entry == TRUE)
				temp_sort = statussort_list;
			else
				temp_sort = temp_sort->next;
			if (temp_sort == NULL)
				break;
			temp_statusdata = temp_sort->status;
		} else {
			if (first_entry == TRUE)
				temp_statusdata = statusdata_list;
			else
				temp_statusdata = temp_statusdata->next;
		}

		if (temp_statusdata == NULL)
			break;

		first_entry = FALSE;

		if (temp_statusdata->type != HOST_STATUS)
			continue;

		temp_host = find_host(temp_statusdata->host_name);

		if (temp_host == NULL)
			continue;

		if (result_limit != 0  && (((total_host_entries + 1) < result_start) || (total_host_entries >= ((result_start + result_limit) - 1)))) {
			total_host_entries++;
			continue;
		}

		if (odd)
			odd = 0;
		else
			odd = 1;

		displayed_host_entries++;
		total_host_entries++;

		status = temp_statusdata->status_string;

		if (temp_statusdata->status == HOST_PENDING) {
			status_class = "PENDING";
			status_bg_class = (odd) ? "Even" : "Odd";
		} else if (suppress_maintenance_downtime == TRUE && temp_statusdata->scheduled_downtime_depth > 0) {
			status_class = "DOWNTIME";
			status_bg_class = (odd) ? "Even" : "Odd";
		} else if (temp_statusdata->status == HOST_UP) {
			status_class = "HOSTUP";
			status_bg_class = (odd) ? "Even" : "Odd";
		} else if (temp_statusdata->status == HOST_DOWN) {
			status_class = "HOSTDOWN";
			if (temp_statusdata->problem_has_been_acknowledged == TRUE)
				status_bg_class = "BGDOWNACK";
			else if (temp_statusdata->scheduled_downtime_depth > 0)
				status_bg_class = "BGDOWNSCHED";
			else
				status_bg_class = "BGDOWN";
		} else if (temp_statusdata->status == HOST_UNREACHABLE) {
			status_class = "HOSTUNREACHABLE";
			if (temp_statusdata->problem_has_been_acknowledged == TRUE)
				status_bg_class = "BGUNREACHABLEACK";
			else if (temp_statusdata->scheduled_downtime_depth > 0)
				status_bg_class = "BGUNREACHABLESCHED";
			else
				status_bg_class = "BGUNREACHABLE";
		}

		grab_host_macros(temp_host);

		if (content_type == HTML_CONTENT) {

			printf("<TR>\n");


			/**** host name column ****/

			printf("<TD CLASS='status%s'>", status_class);

			printf("<TABLE BORDER=0 WIDTH='100%%' cellpadding=0 cellspacing=0>\n");
			printf("<TR>\n");
			printf("<TD ALIGN=LEFT>\n");
			printf("<TABLE BORDER=0 cellpadding=0 cellspacing=0>\n");
			printf("<TR>\n");
			if (!strcmp(temp_host->address6, temp_host->name))
				printf("<TD align=left valign=center CLASS='status%s'><A HREF='%s?type=%d&host=%s' title='%s'>%s</A>&nbsp;</TD>\n", status_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_statusdata->host_name), temp_host->address, (temp_host->display_name != NULL) ? html_encode(temp_host->display_name, TRUE) : html_encode(temp_host->name, TRUE));
			else
				printf("<TD align=left valign=center CLASS='status%s'><A HREF='%s?type=%d&host=%s' title='%s,%s'>%s</A>&nbsp;</TD>\n", status_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_statusdata->host_name), temp_host->address, temp_host->address6, (temp_host->display_name != NULL) ? html_encode(temp_host->display_name, TRUE) : html_encode(temp_host->name, TRUE));

			printf("</TR>\n");
			printf("</TABLE>\n");
			printf("</TD>\n");
			printf("<TD align=right valign=center>\n");
			printf("<TABLE BORDER=0 cellpadding=0 cellspacing=0>\n");
			printf("<TR>\n");
			total_comments = number_of_host_comments(temp_host->name);
			if (temp_statusdata->problem_has_been_acknowledged == TRUE) {
				printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s#comments'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='This host problem has been acknowledged' TITLE='This host problem has been acknowledged'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_statusdata->host_name), url_images_path, ACKNOWLEDGEMENT_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
			}
			if (is_authorized_for_read_only(&current_authdata) == FALSE || is_authorized_for_comments_read_only(&current_authdata) == TRUE) {
				if (total_comments > 0)
					print_comment_icon(temp_host->name, NULL);
			}
			if (temp_statusdata->notifications_enabled == FALSE) {
				printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='Notifications for this host have been disabled' TITLE='Notifications for this host have been disabled'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_statusdata->host_name), url_images_path, NOTIFICATIONS_DISABLED_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
			}
			if (temp_statusdata->checks_enabled == FALSE) {
				if (temp_statusdata->accept_passive_checks == TRUE)
					printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='Active Checks of this host have been disabled'd TITLE='Active Checks of this host have been disabled'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_statusdata->host_name), url_images_path, PASSIVE_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				else
					printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='Active and Passive Checks of this host have been disabled'd TITLE='Active and Passive Checks of this host have been disabled'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_statusdata->host_name), url_images_path, DISABLED_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
			}
			if (temp_statusdata->is_flapping == TRUE) {
				printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='This host is flapping between states' TITLE='This host is flapping between states'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_statusdata->host_name), url_images_path, FLAPPING_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
			}
			if (temp_statusdata->scheduled_downtime_depth > 0) {
				printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='This host is currently in a period of scheduled downtime' TITLE='This host is currently in a period of scheduled downtime'></A></TD>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_statusdata->host_name), url_images_path, DOWNTIME_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
			}
			if (temp_host->notes_url != NULL) {
				process_macros_r(mac, temp_host->notes_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<TD align=center valign=center>");
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'>", (notes_url_target == NULL) ? "_blank" : notes_url_target);
				printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, NOTES_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "View Extra Host Notes", "View Extra Host Notes");
				printf("</A>");
				printf("</TD>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_host->action_url != NULL) {
				process_macros_r(mac, temp_host->action_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<TD align=center valign=center>");
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'>", (action_url_target == NULL) ? "_blank" : action_url_target);
				printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d>", url_images_path, MU_iconstr, ACTION_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);
				printf("</A>");
				printf("</TD>\n");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_host->icon_image != NULL) {
				printf("<TD align=center valign=center>");
				printf("<A HREF='%s?type=%d&host=%s'>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_statusdata->host_name));
				printf("<IMG SRC='%s", url_logo_images_path);
				process_macros_r(mac, temp_host->icon_image, &processed_string, 0);
				printf("%s", processed_string);
				free(processed_string);
				printf("' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE), (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE));
				printf("</A>");
				printf("</TD>\n");
			}
			if (enable_splunk_integration == TRUE) {
				printf("<TD ALIGN=center valign=center>");
				display_splunk_host_url(temp_host);
				printf("</TD>\n");
			}
			printf("<TD>");
			printf("<a href='%s?host=%s'><img src='%s%s' border=0 alt='View Service Details For This Host' title='View Service Details For This Host'></a>", STATUS_CGI, url_encode(temp_statusdata->host_name), url_images_path, STATUS_DETAIL_ICON);
			printf("</TD>\n");
			printf("</TR>\n");
			printf("</TABLE>\n");
			printf("</TD>\n");
			printf("</TR>\n");
			printf("</TABLE>\n");

			printf("</TD>\n");

			/* the rest of the columns... */
			printf("<TD onClick=\"toggle_checkbox('host_%d','tableformhost');\" CLASS='status%s'>%s</TD>\n", total_host_entries, status_class, temp_statusdata->status_string);
			printf("<TD onClick=\"toggle_checkbox('host_%d','tableformhost');\" CLASS='status%s' nowrap>%s</TD>\n", total_host_entries, status_bg_class, temp_statusdata->last_check);
			printf("<TD onClick=\"toggle_checkbox('host_%d','tableformhost');\" CLASS='status%s' nowrap>%s</TD>\n", total_host_entries, status_bg_class, temp_statusdata->state_duration);
			printf("<TD onClick=\"toggle_checkbox('host_%d','tableformhost');\" CLASS='status%s'>%s</TD>\n", total_host_entries, status_bg_class, temp_statusdata->attempts);
			printf("<TD onClick=\"toggle_checkbox('host_%d','tableformhost');\" CLASS='status%s' valign='center'>%s</TD>\n", total_host_entries, status_bg_class, temp_statusdata->plugin_output);

			/* Checkbox for host(s) */
			if (is_authorized_for_read_only(&current_authdata) == FALSE) {
				printf("<TD onClick=\"toggle_checkbox('host_%d','tableformhost');\" CLASS='status%s' nowrap align='center'>", total_host_entries, status_bg_class);
				printf("<input onClick=\"toggle_checkbox('host_%d','tableformhost');\" type='checkbox' id='host_%d' name='host' value='%s'></TD>\n", total_host_entries, total_host_entries, temp_statusdata->host_name);
			}

			if (enable_splunk_integration == TRUE)
				display_splunk_host_url(temp_host);

			printf("</TR>\n");
		}

		/* print list in json format */
		if (content_type == JSON_CONTENT) {
			/* always add a comma, except for the first line */
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
			printf("{ \"host_name\": \"%s\", ", json_encode(temp_host->name));
			printf("\"host_display_name\": \"%s\", ", (temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(temp_host->name));
			printf("\"status\": \"%s\", ", temp_statusdata->status_string);
			printf("\"last_check\": \"%s\", ", temp_statusdata->last_check);
			printf("\"duration\": \"%s\", ", temp_statusdata->state_duration);
			printf("\"attempts\": \"%s\", ", temp_statusdata->attempts);
			printf("\"state_type\": \"%s\", ", (temp_statusdata->state_type == HARD_STATE) ? "HARD" : "SOFT");
			printf("\"is_flapping\": %s, ", (temp_statusdata->is_flapping == TRUE) ? "true" : "false");
			printf("\"in_scheduled_downtime\": %s, ", (temp_statusdata->scheduled_downtime_depth > 0) ? "true" : "false");
			printf("\"active_checks_enabled\": %s, ", (temp_statusdata->checks_enabled == TRUE) ? "true" : "false");
			printf("\"passive_checks_enabled\": %s, ", (temp_statusdata->accept_passive_checks == TRUE) ? "true" : "false");
			printf("\"notifications_enabled\": %s, ", (temp_statusdata->notifications_enabled == TRUE) ? "true" : "false");
			printf("\"has_been_acknowledged\": %s, ", (temp_statusdata->problem_has_been_acknowledged == TRUE) ? "true" : "false");

			if (temp_host->action_url == NULL)
				printf("\"action_url\": null, ");
			else
				printf("\"action_url\": \"%s\", ", json_encode(temp_host->action_url));

			if (temp_host->notes_url == NULL)
				printf("\"notes_url\": null, ");
			else
				printf("\"notes_url\": \"%s\", ", json_encode(temp_host->notes_url));

			if (temp_statusdata->plugin_output == NULL)
				printf("\"status_information\": null }");
			else
				printf("\"status_information\": \"%s\"}", json_encode(temp_statusdata->plugin_output));

			/* print list in csv format */
		} else if (content_type == CSV_CONTENT) {
			printf("%s%s%s%s", csv_data_enclosure, temp_host->name, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_statusdata->status_string, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_statusdata->last_check, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_statusdata->state_duration, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, temp_statusdata->attempts, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_statusdata->plugin_output == NULL) ? "" : temp_statusdata->plugin_output, csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_statusdata->is_flapping == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_statusdata->scheduled_downtime_depth > 0) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_statusdata->checks_enabled == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_statusdata->accept_passive_checks == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_statusdata->notifications_enabled == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_statusdata->problem_has_been_acknowledged == TRUE) ? "true" : "false", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s%s", csv_data_enclosure, (temp_host->action_url != NULL) ? temp_host->action_url : "", csv_data_enclosure, csv_delimiter);
			printf("%s%s%s\n", csv_data_enclosure, (temp_host->notes_url != NULL) ? temp_host->notes_url : "", csv_data_enclosure);
		}
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</FORM>\n");

		/* if user couldn't see anything, print out some helpful info... */
		if (total_host_entries == 0 && user_is_authorized_for_statusdata == FALSE)
			print_generic_error_message("It appears as though you do not have permission to view information for any of the hosts you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		else
			status_page_num_selector(result_start, HOST_STATUS);
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	/* free memory allocated to the sort lists */
	if (use_sort == TRUE)
		free_sort_list();

	return;
}


/* show an overview of servicegroup(s)... */
void show_servicegroup_overviews(void) {
	servicegroup *temp_servicegroup = NULL;
	int current_column;
	int user_has_seen_something = FALSE;
	int json_start = TRUE;
	int i = 0, found = FALSE;

	if (content_type == JSON_CONTENT) {
		printf("\"servicegroup_overview\": [\n");
	} else {
		/* display status overviews for Servicegroups */
		printf("<TABLE BORDER=0 CELLPADDING=10 align='center'>\n");

		current_column = 1;
	}

	/* loop through all servicegroups... */
	for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {

		/* view only selected servicegroups */
		if (show_all_servicegroups == FALSE) {
			found = FALSE;
			for (i = 0; req_servicegroups[i].entry != NULL; i++) {
				if (!strcmp(req_servicegroups[i].entry, temp_servicegroup->group_name)) {
					found = TRUE;
					break;
				}
			}
			if (found == FALSE)
				continue;
		}

		/* make sure the user is authorized to view at least one host in this servicegroup */
		if (is_authorized_for_servicegroup(temp_servicegroup, &current_authdata) == FALSE)
			continue;

		if (content_type == JSON_CONTENT) {
			/* always add a comma, except for the first line */
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
		} else {
			if (current_column == 1)
				printf("<TR>\n");
			printf("<TD VALIGN=top ALIGN=center>\n");
		}

		show_servicegroup_overview(temp_servicegroup);

		user_has_seen_something = TRUE;

		if (content_type != JSON_CONTENT) {
			printf("</TD>\n");
			if (current_column == overview_columns)
				printf("</TR>\n");

			if (current_column < overview_columns)
				current_column++;
			else
				current_column = 1;
		}
	}

	if (content_type == JSON_CONTENT)
		printf(" ]\n");
	else {
		if (current_column != 1) {

			for (; current_column <= overview_columns; current_column++)
				printf("<TD></TD>\n");
			printf("</TR>\n");
		}

		printf("</TABLE>\n");
	}

	/* if user couldn't see anything, print out some helpful info... */
	if (user_has_seen_something == FALSE) {

		if (content_type == JSON_CONTENT)
			printf(",\n");

		if (servicegroup_list != NULL)
			print_generic_error_message("It appears as though you do not have permission to view information for the service group you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		else
			print_generic_error_message("There are no service groups defined.", NULL, 0);
	}

	return;
}


/* shows an overview of a specific servicegroup... */
void show_servicegroup_overview(servicegroup *temp_servicegroup) {
	servicesmember *temp_member;
	servicesmember *temp_member2;
	host *temp_host;
	host *last_host;
	hoststatus *temp_hoststatus = NULL;
	servicestatus *temp_servicestatus = NULL;
	int odd = 0;
	int json_start = TRUE;
	int service_found = FALSE;

	/* make sure the user is authorized to view this servicegroup */
	if (is_authorized_for_servicegroup(temp_servicegroup, &current_authdata) == FALSE)
		return;

	/* print json format */
	if (content_type == JSON_CONTENT) {
		printf("{ \"servicegroup_name\": \"%s\",\n", json_encode(temp_servicegroup->group_name));
		printf("\"members\": [ \n");
	} else {
		printf("<DIV CLASS='status'>\n");
		printf("<A HREF='%s?servicegroup=%s&style=detail'>%s</A>", STATUS_CGI, url_encode(temp_servicegroup->group_name), html_encode(temp_servicegroup->alias, TRUE));
		printf(" (<A HREF='%s?type=%d&servicegroup=%s'>%s</A>)", EXTINFO_CGI, DISPLAY_SERVICEGROUP_INFO, url_encode(temp_servicegroup->group_name), html_encode(temp_servicegroup->group_name, TRUE));
		printf("</DIV>\n");

		printf("<table border=1 CLASS='status' align='center'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='status'>Host</TH><TH CLASS='status'>Status</TH><TH CLASS='status'>Services</TH><TH CLASS='status'>Actions</TH>\n");
		printf("</TR>\n");
	}

	/* find all hosts that have services that are members of the servicegroup */
	last_host = NULL;
	for (temp_member = temp_servicegroup->members; temp_member != NULL; temp_member = temp_member->next) {

		/* find the host */
		temp_host = find_host(temp_member->host_name);
		if (temp_host == NULL)
			continue;

		/* skip this if it isn't a new host... */
		if (temp_host == last_host)
			continue;

		/* find the host status */
		temp_hoststatus = find_hoststatus(temp_host->name);
		if (temp_hoststatus == NULL)
			continue;

		/* make sure we only display hosts of the specified status levels */
		if (!(host_status_types & temp_hoststatus->status))
			continue;

		/* make sure we only display hosts that have the desired properties */
		if (passes_host_properties_filter(temp_hoststatus) == FALSE)
			continue;

		/* check if there are any services to display */
		if (service_status_types != all_service_status_types) {
			service_found = FALSE;

			/* check members if there is anything to display for this host */
			for (temp_member2 = temp_member; temp_member2 != NULL; temp_member2 = temp_member2->next) {

				if (strcmp(temp_member2->host_name, temp_host->name))
					break;

				/* get the status of the service */
				temp_servicestatus = find_servicestatus(temp_member2->host_name, temp_member2->service_description);

				/* make sure we only display services of the specified status levels */
				if (!(service_status_types & temp_servicestatus->status))
					continue;

				/* make sure we only display services that have the desired properties */
				if (passes_service_properties_filter(temp_servicestatus) == FALSE)
					continue;

				service_found = TRUE;
				break;
			}
			if (service_found == FALSE)
				continue;
		}

		if (odd)
			odd = 0;
		else
			odd = 1;

		if (content_type == JSON_CONTENT) {
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
		}

		show_servicegroup_hostgroup_member_overview(temp_hoststatus, odd, temp_servicegroup);

		last_host = temp_host;
	}

	if (content_type == JSON_CONTENT)
		printf(" ] }\n");
	else
		printf("</table>\n");

	return;
}


/* show a summary of servicegroup(s)... */
void show_servicegroup_summaries(void) {
	servicegroup *temp_servicegroup = NULL;
	servicesmember *temp_member = NULL;
	hoststatus *temp_hoststatus = NULL;
	servicestatus *temp_servicestatus = NULL;
	int user_has_seen_something = FALSE;
	int odd = 0;
	int json_start = TRUE;
	int i = 0, found = FALSE;

	if (content_type == JSON_CONTENT) {
		printf("\"servicegroup_summary\": [\n");
	} else {
		printf("<table border=1 CLASS='status' align='center'>\n");
		printf("<TR>\n");
		printf("<TH CLASS='status'>Service Group</TH><TH CLASS='status'>Host Status Summary</TH><TH CLASS='status'>Service Status Summary</TH>\n");
		printf("</TR>\n");
	}

	/* display status summary for servicegroups */
	for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {

		/* view only selected servicegroups */
		if (show_all_servicegroups == FALSE) {
			found = FALSE;
			for (i = 0; req_servicegroups[i].entry != NULL; i++) {
				if (!strcmp(req_servicegroups[i].entry, temp_servicegroup->group_name)) {
					found = TRUE;
					break;
				}
			}
			if (found == FALSE)
				continue;
		}

		/* make sure the user is authorized to view at least one host in this servicegroup */
		if (is_authorized_for_servicegroup(temp_servicegroup, &current_authdata) == FALSE)
			continue;

		user_has_seen_something = TRUE;

		/* find all the hosts that belong to the servicegroup */
		if (host_status_types != all_host_status_types || service_status_types != all_service_status_types) {
			found = FALSE;
			for (temp_member = temp_servicegroup->members; temp_member != NULL; temp_member = temp_member->next) {

				if (host_status_types != all_host_status_types) {
					/* find the host status */
					temp_hoststatus = find_hoststatus(temp_member->host_name);
					if (temp_hoststatus == NULL)
						continue;

					/* make sure we will only be displaying hosts of the specified status levels */
					if (!(host_status_types & temp_hoststatus->status))
						continue;

					/* make sure we will only be displaying hosts that have the desired properties */
					if (passes_host_properties_filter(temp_hoststatus) == FALSE)
						continue;
				}
				if (service_status_types != all_service_status_types) {
					/* find the service status */
					temp_servicestatus = find_servicestatus(temp_member->host_name, temp_member->service_description);
					if (temp_servicestatus == NULL)
						continue;

					/* make sure we only display services of the specified status levels */
					if (!(service_status_types & temp_servicestatus->status))
						continue;

					/* make sure we only display services that have the desired properties */
					if (passes_service_properties_filter(temp_servicestatus) == FALSE)
						continue;
				}

				found = TRUE;
				break;
			}

			if (found == FALSE)
				continue;
		}

		if (odd == 0)
			odd = 1;
		else
			odd = 0;

		if (content_type == JSON_CONTENT) {
			/* always add a comma, except for the first line */
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
		}

		/* show summary for this servicegroup */
		show_servicegroup_summary(temp_servicegroup, odd);
	}

	if (content_type == JSON_CONTENT)
		printf(" ]\n");
	else
		printf("</TABLE>\n");

	/* if user couldn't see anything, print out some helpful info... */
	if (user_has_seen_something == FALSE) {

		if (content_type == JSON_CONTENT)
			printf(",\n");

		if (servicegroup_list != NULL)
			print_generic_error_message("It appears as though you do not have permission to view information for the service group you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		else
			print_generic_error_message("There are no service groups defined.", NULL, 0);
	}

	return;
}


/* displays status summary information for a specific servicegroup */
void show_servicegroup_summary(servicegroup *temp_servicegroup, int odd) {
	char *status_bg_class = "";

	if (content_type == JSON_CONTENT) {
		printf("{ \"servicegroup_name\": \"%s\",\n", json_encode(temp_servicegroup->group_name));
		show_servicegroup_host_totals_summary(temp_servicegroup);
		show_servicegroup_service_totals_summary(temp_servicegroup);
		printf("}\n");
	} else {
		if (odd == 1)
			status_bg_class = "Even";
		else
			status_bg_class = "Odd";

		printf("<TR CLASS='status%s'><TD CLASS='status%s'>\n", status_bg_class, status_bg_class);
		printf("<A HREF='%s?servicegroup=%s&style=overview'>%s</A> ", STATUS_CGI, url_encode(temp_servicegroup->group_name), html_encode(temp_servicegroup->alias, TRUE));
		printf("(<A HREF='%s?type=%d&servicegroup=%s'>%s</a>)", EXTINFO_CGI, DISPLAY_SERVICEGROUP_INFO, url_encode(temp_servicegroup->group_name), html_encode(temp_servicegroup->group_name, TRUE));
		printf("</TD>");

		printf("<TD CLASS='status%s' ALIGN=CENTER VALIGN=CENTER>", status_bg_class);
		show_servicegroup_host_totals_summary(temp_servicegroup);
		printf("</TD>");

		printf("<TD CLASS='status%s' ALIGN=CENTER VALIGN=CENTER>", status_bg_class);
		show_servicegroup_service_totals_summary(temp_servicegroup);
		printf("</TD>");

		printf("</TR>\n");
	}

	return;
}


/* shows host total summary information for a specific servicegroup */
void show_servicegroup_host_totals_summary(servicegroup *temp_servicegroup) {
	servicesmember *temp_member;
	int hosts_up = 0;
	int hosts_down = 0;
	int hosts_unreachable = 0;
	int hosts_pending = 0;
	int hosts_down_scheduled = 0;
	int hosts_down_acknowledged = 0;
	int hosts_down_disabled = 0;
	int hosts_down_unacknowledged = 0;
	int hosts_unreachable_scheduled = 0;
	int hosts_unreachable_acknowledged = 0;
	int hosts_unreachable_disabled = 0;
	int hosts_unreachable_unacknowledged = 0;
	hoststatus *temp_hoststatus = NULL;
	host *temp_host = NULL;
	host *last_host = NULL;
	int problem = FALSE;

	/* find all the hosts that belong to the servicegroup */
	for (temp_member = temp_servicegroup->members; temp_member != NULL; temp_member = temp_member->next) {

		/* find the host... */
		temp_host = find_host(temp_member->host_name);
		if (temp_host == NULL)
			continue;

		/* skip this if it isn't a new host... */
		if (temp_host == last_host)
			continue;

		/* find the host status */
		temp_hoststatus = find_hoststatus(temp_host->name);
		if (temp_hoststatus == NULL)
			continue;

		/* make sure we only display hosts of the specified status levels */
		if (!(host_status_types & temp_hoststatus->status))
			continue;

		/* make sure we only display hosts that have the desired properties */
		if (passes_host_properties_filter(temp_hoststatus) == FALSE)
			continue;

		problem = TRUE;

		if (temp_hoststatus->status == HOST_UP)
			hosts_up++;

		else if (temp_hoststatus->status == HOST_DOWN) {
			if (temp_hoststatus->scheduled_downtime_depth > 0) {
				hosts_down_scheduled++;
				problem = FALSE;
			}
			if (temp_hoststatus->problem_has_been_acknowledged == TRUE) {
				hosts_down_acknowledged++;
				problem = FALSE;
			}
			if (temp_hoststatus->checks_enabled == FALSE && temp_hoststatus->accept_passive_host_checks == FALSE) {
				hosts_down_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				hosts_down_unacknowledged++;
			hosts_down++;
		}

		else if (temp_hoststatus->status == HOST_UNREACHABLE) {
			if (temp_hoststatus->scheduled_downtime_depth > 0) {
				hosts_unreachable_scheduled++;
				problem = FALSE;
			}
			if (temp_hoststatus->problem_has_been_acknowledged == TRUE) {
				hosts_unreachable_acknowledged++;
				problem = FALSE;
			}
			if (temp_hoststatus->checks_enabled == FALSE && temp_hoststatus->accept_passive_host_checks == FALSE) {
				hosts_unreachable_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				hosts_unreachable_unacknowledged++;
			hosts_unreachable++;
		} else
			hosts_pending++;

		last_host = temp_host;
	}

	if (content_type == JSON_CONTENT) {
		printf("\"hosts_up\": %d, ", hosts_up);
		printf("\"hosts_down\": %d, ", hosts_down);
		printf("\"hosts_down_unacknowledged\": %d, ", hosts_down_unacknowledged);
		printf("\"hosts_down_scheduled\": %d, ", hosts_down_scheduled);
		printf("\"hosts_down_acknowledged\": %d, ", hosts_down_acknowledged);
		printf("\"hosts_down_disabled\": %d, ", hosts_down_disabled);
		printf("\"hosts_unreachable\": %d, ", hosts_unreachable);
		printf("\"hosts_unreachable_unacknowledged\": %d, ", hosts_unreachable_unacknowledged);
		printf("\"hosts_unreachable_scheduled\": %d, ", hosts_unreachable_scheduled);
		printf("\"hosts_unreachable_acknowledged\": %d, ", hosts_unreachable_acknowledged);
		printf("\"hosts_unreachable_disabled\": %d, ", hosts_unreachable_disabled);
		printf("\"hosts_pending\": %d, ", hosts_pending);
	} else {
		printf("<TABLE BORDER='0'>\n");

		if (hosts_up > 0) {
			printf("<TR>");
			printf("<TD CLASS='miniStatusUP'><A HREF='%s?servicegroup=%s&style=detail&&hoststatustypes=%d&hostprops=%lu'>%d UP</A></TD>", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_UP, host_properties, hosts_up);
			printf("</TR>\n");
		}

		if (hosts_down > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusDOWN'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusDOWN'><A HREF='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%lu'>%d DOWN</A>&nbsp;:</TD>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_DOWN, host_properties, hosts_down);

			printf("<TD><TABLE BORDER='0'>\n");

			if (hosts_down_unacknowledged > 0)
				printf("<tr><td width=100%% class='hostImportantProblem'><a href='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_DOWN, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED, hosts_down_unacknowledged);

			if (hosts_down_scheduled > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_DOWN, HOST_SCHEDULED_DOWNTIME, hosts_down_scheduled);

			if (hosts_down_acknowledged > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_DOWN, HOST_STATE_ACKNOWLEDGED, hosts_down_acknowledged);

			if (hosts_down_disabled > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_DOWN, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED, hosts_down_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (hosts_unreachable > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusUNREACHABLE'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusUNREACHABLE'><A HREF='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%lu'>%d UNREACHABLE</A>&nbsp;:</TD>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_UNREACHABLE, host_properties, hosts_unreachable);

			printf("<TD><TABLE BORDER='0'>\n");

			if (hosts_unreachable_unacknowledged > 0)
				printf("<tr><td width=100%% class='hostImportantProblem'><a href='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_UNREACHABLE, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED, hosts_unreachable_unacknowledged);

			if (hosts_unreachable_scheduled > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_UNREACHABLE, HOST_SCHEDULED_DOWNTIME, hosts_unreachable_scheduled);

			if (hosts_unreachable_acknowledged > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_UNREACHABLE, HOST_STATE_ACKNOWLEDGED, hosts_unreachable_acknowledged);

			if (hosts_unreachable_disabled > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_UNREACHABLE, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED, hosts_unreachable_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (hosts_pending > 0)
			printf("<TR><TD CLASS='miniStatusPENDING'><A HREF='%s?servicegroup=%s&style=detail&hoststatustypes=%d&hostprops=%lu'>%d PENDING</A></TD></TR>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), HOST_PENDING, host_properties, hosts_pending);

		printf("</TABLE>\n");

		if ((hosts_up + hosts_down + hosts_unreachable + hosts_pending) == 0)
			printf("No matching hosts");
	}

	return;
}


/* shows service total summary information for a specific servicegroup */
void show_servicegroup_service_totals_summary(servicegroup *temp_servicegroup) {
	int services_ok = 0;
	int services_warning = 0;
	int services_unknown = 0;
	int services_critical = 0;
	int services_pending = 0;
	int services_warning_host_problem = 0;
	int services_warning_scheduled = 0;
	int services_warning_acknowledged = 0;
	int services_warning_disabled = 0;
	int services_warning_unacknowledged = 0;
	int services_unknown_host_problem = 0;
	int services_unknown_scheduled = 0;
	int services_unknown_acknowledged = 0;
	int services_unknown_disabled = 0;
	int services_unknown_unacknowledged = 0;
	int services_critical_host_problem = 0;
	int services_critical_scheduled = 0;
	int services_critical_acknowledged = 0;
	int services_critical_disabled = 0;
	int services_critical_unacknowledged = 0;
	servicesmember *temp_member = NULL;
	servicestatus *temp_servicestatus = NULL;
	servicestatus *last_servicestatus = NULL;
	hoststatus *temp_hoststatus = NULL;
	int problem = FALSE;


	/* find all the services that belong to the servicegroup */
	for (temp_member = temp_servicegroup->members; temp_member != NULL; temp_member = temp_member->next) {

		/* find the service status */
		temp_servicestatus = find_servicestatus(temp_member->host_name, temp_member->service_description);
		if (temp_servicestatus == NULL)
			continue;

		/* skip this if it isn't a new service... */
		if (temp_servicestatus == last_servicestatus)
			continue;

		/* find the status of the associated host */
		temp_hoststatus = find_hoststatus(temp_servicestatus->host_name);
		if (temp_hoststatus == NULL)
			continue;

		/* make sure we only display hosts of the specified status levels */
		if (!(host_status_types & temp_hoststatus->status))
			continue;

		/* make sure we only display hosts that have the desired properties */
		if (passes_host_properties_filter(temp_hoststatus) == FALSE)
			continue;

		/* make sure we only display services of the specified status levels */
		if (!(service_status_types & temp_servicestatus->status))
			continue;

		/* make sure we only display services that have the desired properties */
		if (passes_service_properties_filter(temp_servicestatus) == FALSE)
			continue;

		problem = TRUE;

		if (temp_servicestatus->status == SERVICE_OK)
			services_ok++;

		else if (temp_servicestatus->status == SERVICE_WARNING) {
			if (temp_hoststatus != NULL && (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE)) {
				services_warning_host_problem++;
				problem = FALSE;
			}
			if (temp_servicestatus->scheduled_downtime_depth > 0) {
				services_warning_scheduled++;
				problem = FALSE;
			}
			if (temp_servicestatus->problem_has_been_acknowledged == TRUE) {
				services_warning_acknowledged++;
				problem = FALSE;
			}
			if (temp_servicestatus->checks_enabled == FALSE && temp_servicestatus->accept_passive_service_checks == FALSE) {
				services_warning_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				services_warning_unacknowledged++;
			services_warning++;
		}

		else if (temp_servicestatus->status == SERVICE_UNKNOWN) {
			if (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE) {
				services_unknown_host_problem++;
				problem = FALSE;
			}
			if (temp_servicestatus->scheduled_downtime_depth > 0) {
				services_unknown_scheduled++;
				problem = FALSE;
			}
			if (temp_servicestatus->problem_has_been_acknowledged == TRUE) {
				services_unknown_acknowledged++;
				problem = FALSE;
			}
			if (temp_servicestatus->checks_enabled == FALSE && temp_servicestatus->accept_passive_service_checks == FALSE) {
				services_unknown_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				services_unknown_unacknowledged++;
			services_unknown++;
		}

		else if (temp_servicestatus->status == SERVICE_CRITICAL) {
			if (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE) {
				services_critical_host_problem++;
				problem = FALSE;
			}
			if (temp_servicestatus->scheduled_downtime_depth > 0) {
				services_critical_scheduled++;
				problem = FALSE;
			}
			if (temp_servicestatus->problem_has_been_acknowledged == TRUE) {
				services_critical_acknowledged++;
				problem = FALSE;
			}
			if (temp_servicestatus->checks_enabled == FALSE && temp_servicestatus->accept_passive_service_checks == FALSE) {
				services_critical_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				services_critical_unacknowledged++;
			services_critical++;
		}

		else if (temp_servicestatus->status == SERVICE_PENDING)
			services_pending++;

		last_servicestatus = temp_servicestatus;
	}

	if (content_type == JSON_CONTENT) {
		printf("\"services_ok\": %d, ", services_ok);
		printf("\"services_warning\": %d, ", services_warning);
		printf("\"services_warning_unacknowledged\": %d, ", services_warning_unacknowledged);
		printf("\"services_warning_host_problem\": %d, ", services_warning_host_problem);
		printf("\"services_warning_scheduled\": %d, ", services_warning_scheduled);
		printf("\"services_warning_acknowledged\": %d, ", services_warning_acknowledged);
		printf("\"services_warning_disabled\": %d, ", services_warning_disabled);
		printf("\"services_unknown\": %d, ", services_unknown);
		printf("\"services_unknown_unacknowledged\": %d, ", services_unknown_unacknowledged);
		printf("\"services_unknown_host_problem\": %d, ", services_unknown_host_problem);
		printf("\"services_unknown_scheduled\": %d, ", services_unknown_scheduled);
		printf("\"services_unknown_acknowledged\": %d, ", services_unknown_acknowledged);
		printf("\"services_unknown_disabled\": %d, ", services_unknown_disabled);
		printf("\"services_critical\": %d, ", services_critical);
		printf("\"services_critical_unacknowledged\": %d, ", services_critical_unacknowledged);
		printf("\"services_critical_host_problem\": %d, ", services_critical_host_problem);
		printf("\"services_critical_scheduled\": %d, ", services_critical_scheduled);
		printf("\"services_critical_acknowledged\": %d, ", services_critical_acknowledged);
		printf("\"services_critical_disabled\": %d, ", services_critical_disabled);
		printf("\"services_pending\": %d ", services_pending);
	} else {
		printf("<TABLE BORDER=0>\n");

		if (services_ok > 0)
			printf("<TR><TD CLASS='miniStatusOK'><A HREF='%s?servicegroup=%s&style=detail&&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d OK</A></TD></TR>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_OK, host_status_types, service_properties, host_properties, services_ok);

		if (services_warning > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusWARNING'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusWARNING'><A HREF='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d WARNING</A>&nbsp;:</TD>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_WARNING, host_status_types, service_properties, host_properties, services_warning);

			printf("<TD><TABLE BORDER='0'>\n");

			if (services_warning_unacknowledged > 0)
				printf("<tr><td width=100%% class='serviceImportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, services_warning_unacknowledged);

			if (services_warning_host_problem > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d'>%d on Problem Hosts</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, services_warning_host_problem);

			if (services_warning_scheduled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_WARNING, SERVICE_SCHEDULED_DOWNTIME, services_warning_scheduled);

			if (services_warning_acknowledged > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_WARNING, SERVICE_STATE_ACKNOWLEDGED, services_warning_acknowledged);

			if (services_warning_disabled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_WARNING, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_warning_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (services_unknown > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusUNKNOWN'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusUNKNOWN'><A HREF='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d UNKNOWN</A>&nbsp;:</TD>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_UNKNOWN, host_status_types, service_properties, host_properties, services_unknown);

			printf("<TD><TABLE BORDER='0'>\n");

			if (services_unknown_unacknowledged > 0)
				printf("<tr><td width=100%% class='serviceImportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, services_unknown_unacknowledged);

			if (services_unknown_host_problem > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d'>%d on Problem Hosts</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, services_unknown_host_problem);

			if (services_unknown_scheduled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_UNKNOWN, SERVICE_SCHEDULED_DOWNTIME, services_unknown_scheduled);

			if (services_unknown_acknowledged > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_UNKNOWN, SERVICE_STATE_ACKNOWLEDGED, services_unknown_acknowledged);

			if (services_unknown_disabled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_UNKNOWN, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_unknown_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (services_critical > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusCRITICAL'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusCRITICAL'><A HREF='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d CRITICAL</A>&nbsp:</TD>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_CRITICAL, host_status_types, service_properties, host_properties, services_critical);

			printf("<TD><TABLE BORDER='0'>\n");

			if (services_critical_unacknowledged > 0)
				printf("<tr><td width=100%% class='serviceImportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, services_critical_unacknowledged);

			if (services_critical_host_problem > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d'>%d on Problem Hosts</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, services_critical_host_problem);

			if (services_critical_scheduled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_CRITICAL, SERVICE_SCHEDULED_DOWNTIME, services_critical_scheduled);

			if (services_critical_acknowledged > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_CRITICAL, SERVICE_STATE_ACKNOWLEDGED, services_critical_acknowledged);

			if (services_critical_disabled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?servicegroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_CRITICAL, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_critical_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (services_pending > 0)
			printf("<TR><TD CLASS='miniStatusPENDING'><A HREF='%s?servicegroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d PENDING</A></TD></TR>\n", STATUS_CGI, url_encode(temp_servicegroup->group_name), SERVICE_PENDING, host_status_types, service_properties, host_properties, services_pending);

		printf("</TABLE>\n");

		if ((services_ok + services_warning + services_unknown + services_critical + services_pending) == 0)
			printf("No matching services");
	}

	return;
}


/* show a grid layout of servicegroup(s)... */
void show_servicegroup_grids(void) {
	servicegroup *temp_servicegroup = NULL;
	int user_has_seen_something = FALSE;
	int json_start = TRUE;
	int i = 0, found = FALSE;

	if (content_type == JSON_CONTENT) {
		printf("\"servicegroup_grid\": [\n");
	}

	/* display status grids for servicegroups */
	for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {

		/* view only selected servicegroups */
		if (show_all_servicegroups == FALSE) {
			found = FALSE;
			for (i = 0; req_servicegroups[i].entry != NULL; i++) {
				if (!strcmp(req_servicegroups[i].entry, temp_servicegroup->group_name)) {
					found = TRUE;
					break;
				}
			}
			if (found == FALSE)
				continue;
		}

		/* make sure the user is authorized to view at least one host in this servicegroup */
		if (is_authorized_for_servicegroup(temp_servicegroup, &current_authdata) == FALSE)
			continue;

		if (content_type == JSON_CONTENT) {
			/* always add a comma, except for the first line */
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
		}

		/* show grid for this servicegroup */
		show_servicegroup_grid(temp_servicegroup);

		user_has_seen_something = TRUE;
	}

	if (content_type == JSON_CONTENT)
		printf(" ]\n");

	/* if user couldn't see anything, print out some helpful info... */
	if (user_has_seen_something == FALSE) {

		if (content_type == JSON_CONTENT)
			printf(",\n");

		if (servicegroup_list != NULL)
			print_generic_error_message("It appears as though you do not have permission to view information for the service group you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		else
			print_generic_error_message("There are no service groups defined.", NULL, 0);
	}

	return;
}


/* displays status grid for a specific servicegroup */
void show_servicegroup_grid(servicegroup *temp_servicegroup) {
	char *status_bg_class = "";
	char *status = "";
	char *host_status_class = "";
	char *service_status_class = "";
	char *processed_string = NULL;
	servicesmember *temp_member;
	servicesmember *temp_member2;
	host *temp_host;
	host *last_host;
	service *temp_service;
	hoststatus *temp_hoststatus;
	servicestatus *temp_servicestatus;
	int odd = 0;
	int current_item;
	int json_start = TRUE;
	int json_start2 = TRUE;
	int service_found = FALSE;

	if (content_type == JSON_CONTENT) {
		printf("{ \"servicegroup_name\": \"%s\",\n", json_encode(temp_servicegroup->group_name));
		printf("\"members\": [ \n");
	} else {
		printf("<DIV CLASS='status'><A HREF='%s?servicegroup=%s&style=detail'>%s</A>", STATUS_CGI, url_encode(temp_servicegroup->group_name), html_encode(temp_servicegroup->alias, TRUE));
		printf(" (<A HREF='%s?type=%d&servicegroup=%s'>%s</A>)</DIV>", EXTINFO_CGI, DISPLAY_SERVICEGROUP_INFO, url_encode(temp_servicegroup->group_name), html_encode(temp_servicegroup->group_name, TRUE));

		printf("<TABLE BORDER=1 CLASS='status' align='center'>\n");
		printf("<TR><TH CLASS='status'>Host</TH><TH CLASS='status'>Services</a></TH><TH CLASS='status'>Actions</TH></TR>\n");
	}

	/* find all hosts that have services that are members of the servicegroup */
	last_host = NULL;
	for (temp_member = temp_servicegroup->members; temp_member != NULL; temp_member = temp_member->next) {

		/* find the host... */
		temp_host = find_host(temp_member->host_name);
		if (temp_host == NULL)
			continue;

		/* only show if user is authorized to view this host */
		if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		/* find the host status */
		temp_hoststatus = find_hoststatus(temp_host->name);
		if (temp_hoststatus == NULL)
			continue;

		/* make sure we only display hosts of the specified status levels */
		if (!(host_status_types & temp_hoststatus->status))
			continue;

		/* make sure we only display hosts that have the desired properties */
		if (passes_host_properties_filter(temp_hoststatus) == FALSE)
			continue;

		/* check if there are any services to display */
		if (service_status_types != all_service_status_types) {
			service_found = FALSE;

			/* check members if there is anything to display for this host */
			for (temp_member2 = temp_member; temp_member2 != NULL; temp_member2 = temp_member2->next) {

				if (strcmp(temp_member2->host_name, temp_host->name))
					break;

				/* get the status of the service */
				temp_servicestatus = find_servicestatus(temp_member2->host_name, temp_member2->service_description);

				/* make sure we only display services of the specified status levels */
				if (!(service_status_types & temp_servicestatus->status))
					continue;

				/* make sure we only display services that have the desired properties */
				if (passes_service_properties_filter(temp_servicestatus) == FALSE)
					continue;

				service_found = TRUE;
				break;
			}
			if (service_found == FALSE)
				continue;
		}

		/* skip this if it isn't a new host... */
		if (temp_host == last_host)
			continue;

		if (odd == 1) {
			status_bg_class = "Even";
			odd = 0;
		} else {
			status_bg_class = "Odd";
			odd = 1;
		}

		if (content_type != JSON_CONTENT)
			printf("<TR CLASS='status%s'>\n", status_bg_class);

		if (temp_hoststatus->status == HOST_DOWN) {
			status = "DOWN";
			host_status_class = "HOSTDOWN";
		} else if (temp_hoststatus->status == HOST_UNREACHABLE) {
			status = "UNREACHABLE";
			host_status_class = "HOSTUNREACHABLE";
		} else {
			status = "OK";
			host_status_class = status_bg_class;
		}

		if (content_type == JSON_CONTENT) {
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;

			printf("{ \"host_name\": \"%s\",\n", json_encode(temp_host->name));
			printf("\"host_display_name\": \"%s\",\n", (temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(temp_host->name));
			printf("\"host_status\": \"%s\",\n", status);
			printf("\"services\": [ \n");
		} else {
			printf("<TD CLASS='status%s'>", host_status_class);

			printf("<TABLE BORDER=0 WIDTH='100%%' cellpadding=0 cellspacing=0>\n");
			printf("<TR>\n");
			printf("<TD ALIGN=LEFT>\n");
			printf("<TABLE BORDER=0 cellpadding=0 cellspacing=0>\n");
			printf("<TR>\n");
			printf("<TD align=left valign=center CLASS='status%s'>", host_status_class);
			printf("<A HREF='%s?type=%d&host=%s'>%s</A>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_host->name), (temp_host->display_name != NULL) ? html_encode(temp_host->display_name, TRUE) : html_encode(temp_host->name, TRUE));
			printf("</TD>\n");
			printf("</TR>\n");
			printf("</TABLE>\n");
			printf("</TD>\n");
			printf("<TD align=right valign=center nowrap>\n");
			printf("<TABLE BORDER=0 cellpadding=0 cellspacing=0>\n");
			printf("<TR>\n");

			if (temp_host->icon_image != NULL) {
				printf("<TD align=center valign=center>");
				printf("<A HREF='%s?type=%d&host=%s'>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_host->name));
				printf("<IMG SRC='%s", url_logo_images_path);
				process_macros_r(mac, temp_host->icon_image, &processed_string, 0);
				printf("%s", processed_string);
				free(processed_string);
				printf("' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE), (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE));
				printf("</A>");
				printf("<TD>\n");
			}

			printf("</TR>\n");
			printf("</TABLE>\n");
			printf("</TD>\n");
			printf("</TR>\n");
			printf("</TABLE>\n");

			printf("</TD>\n");

			printf("<TD CLASS='status%s' style='text-align:center'>", host_status_class);
		}

		/* display all services on the host that are part of the servicegroup */
		current_item = 1;
		json_start2 = TRUE;
		for (temp_member2 = temp_member; temp_member2 != NULL; temp_member2 = temp_member2->next) {

			/* bail out if we've reached the end of the services that are associated with this servicegroup */
			if (strcmp(temp_member2->host_name, temp_host->name))
				break;

			/* get the status of the service */
			temp_servicestatus = find_servicestatus(temp_member2->host_name, temp_member2->service_description);

			/* make sure we only display services of the specified status levels */
			if (!(service_status_types & temp_servicestatus->status))
				continue;

			/* make sure we only display services that have the desired properties */
			if (passes_service_properties_filter(temp_servicestatus) == FALSE)
				continue;

			if (temp_servicestatus == NULL)
				service_status_class = "NULL";
			else if (temp_servicestatus->status == SERVICE_OK)
				service_status_class = "OK";
			else if (temp_servicestatus->status == SERVICE_WARNING)
				service_status_class = "WARNING";
			else if (temp_servicestatus->status == SERVICE_UNKNOWN)
				service_status_class = "UNKNOWN";
			else if (temp_servicestatus->status == SERVICE_CRITICAL)
				service_status_class = "CRITICAL";
			else
				service_status_class = "PENDING";

			if (content_type == JSON_CONTENT) {
				if (json_start2 == FALSE)
					printf(",\n");
				json_start2 = FALSE;

				printf("{ \"service_description\": \"%s\",\n", json_encode(temp_servicestatus->description));

				temp_service = find_service(temp_servicestatus->host_name, temp_servicestatus->description);
				printf("\"service_display_name\": \"%s\",\n", (temp_service != NULL && temp_service->display_name != NULL) ? json_encode(temp_service->display_name) : json_encode(temp_servicestatus->description));

				if (temp_servicestatus == NULL)
					printf("\"service_status\": null } ");
				else
					printf("\"service_status\": \"%s\" } ", service_status_class);
			} else {
				if (current_item > max_grid_width && max_grid_width > 0) {
					printf("<BR>\n");
					current_item = 1;
				}

				printf("<A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_servicestatus->host_name));
				printf("&service=%s' CLASS='status%s'>%s</A>&nbsp;", url_encode(temp_servicestatus->description), service_status_class, html_encode(temp_servicestatus->description, TRUE));

				current_item++;
			}
		}

		/* Print no matching in case of no services */
		if (current_item == 1 && content_type != JSON_CONTENT)
			printf("No matching services");

		if (content_type == JSON_CONTENT) {
			printf(" ] } \n");
		} else {
			/* actions */
			printf("<TD CLASS='status%s'>", host_status_class);

			/* grab macros */
			grab_host_macros_r(mac, temp_host);

			printf("<A HREF='%s?type=%d&host=%s'>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_host->name));
			printf("<IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, DETAIL_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "View Extended Information For This Host", "View Extended Information For This Host");
			printf("</A>");

			if (temp_host->notes_url != NULL) {
				process_macros_r(mac, temp_host->notes_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'>", (notes_url_target == NULL) ? "_blank" : notes_url_target);
				printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, NOTES_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "View Extra Host Notes", "View Extra Host Notes");
				printf("</A>");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_host->action_url != NULL) {
				process_macros_r(mac, temp_host->action_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'>", (action_url_target == NULL) ? "blank" : action_url_target);
				printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, ACTION_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "Perform Extra Host Actions", "Perform Extra Host Actions");
				printf("</A>");
				END_MULTIURL_LOOP
				free(processed_string);
			}

			printf("<a href='%s?host=%s'><img src='%s%s' border=0 alt='View Service Details For This Host' title='View Service Details For This Host'></a>\n", STATUS_CGI, url_encode(temp_host->name), url_images_path, STATUS_DETAIL_ICON);

#ifdef USE_STATUSMAP
			printf("<A HREF='%s?host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'></A>", STATUSMAP_CGI, url_encode(temp_host->name), url_images_path, STATUSMAP_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "Locate Host On Map", "Locate Host On Map");
#endif
			printf("</TD>\n");
			printf("</TR>\n");
		}

		last_host = temp_host;
	}

	if (content_type == JSON_CONTENT)
		printf(" ] } \n");
	else
		printf("</TABLE>\n");

	return;
}


/* show an overview of hostgroup(s)... */
void show_hostgroup_overviews(void) {
	hostgroup *temp_hostgroup = NULL;
	hostsmember *temp_member = NULL;
	host *temp_host = NULL;
	hoststatus *temp_hoststatus = NULL;
	int current_column;
	int user_has_seen_something = FALSE;
	int json_start = TRUE;
	int partial_hosts = FALSE;
	int i = 0, found = FALSE;

	if (content_type == JSON_CONTENT) {
		printf("\"hostgroup_overview\": [\n");
	} else {
		/* display status overviews for hostgroups */
		printf("<TABLE BORDER=0 CELLPADDING=10 align='center'>\n");
		current_column = 1;
	}

	/* loop through all hostgroups... */
	for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {

		/* view only selected hostgroups */
		if (show_all_hostgroups == FALSE) {
			found = FALSE;
			for (i = 0; req_hostgroups[i].entry != NULL; i++) {
				if (!strcmp(req_hostgroups[i].entry, temp_hostgroup->group_name)) {
					found = TRUE;
					break;
				}
			}
			if (found == FALSE)
				continue;
		}

		/* make sure the user is authorized to view this hostgroup */
		if (show_partial_hostgroups == FALSE && is_authorized_for_hostgroup(temp_hostgroup, &current_authdata) == FALSE)
			continue;

		/* if we're showing partial hostgroups, find out if there will be any hosts that belong to the hostgroup */
		if (show_partial_hostgroups == TRUE) {
			for (temp_member = temp_hostgroup->members; temp_member != NULL; temp_member = temp_member->next) {

				/* find the host... */
				temp_host = find_host(temp_member->host_name);
				if (temp_host == NULL)
					continue;

				/* only shown in partial hostgroups if user is authorized to view this host */
				if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
					continue;

				/* find the host status */
				temp_hoststatus = find_hoststatus(temp_host->name);
				if (temp_hoststatus == NULL)
					continue;

				/* make sure we will only be displaying hosts of the specified status levels */
				if (!(host_status_types & temp_hoststatus->status))
					continue;

				/* make sure we will only be displaying hosts that have the desired properties */
				if (passes_host_properties_filter(temp_hoststatus) == FALSE)
					continue;

				partial_hosts = TRUE;

				break;
			}
		}

		/* if we're showing partial hostgroups, but there are no hosts to display, there's nothing to see here */
		if (show_partial_hostgroups == TRUE && partial_hosts == FALSE)
			continue;

		if (content_type == JSON_CONTENT) {
			/* always add a comma, except for the first line */
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
		} else {
			if (current_column == 1)
				printf("<TR>\n");
		}

		if (show_hostgroup_overview(temp_hostgroup) == FALSE)
			continue;

		user_has_seen_something = TRUE;

		if (content_type != JSON_CONTENT) {
			if (current_column == overview_columns)
				printf("</TR>\n");

			if (current_column < overview_columns)
				current_column++;
			else
				current_column = 1;
		}
	}

	if (content_type == JSON_CONTENT)
		printf(" ]\n");
	else {
		if (current_column != 1) {

			for (; current_column <= overview_columns; current_column++)
				printf("<TD></TD>\n");
			printf("</TR>\n");
		}

		printf("</TABLE>\n");
	}

	/* if user couldn't see anything, print out some helpful info... */
	if (user_has_seen_something == FALSE) {

		if (content_type == JSON_CONTENT)
			printf(",\n");

		if (hostgroup_list != NULL)
			print_generic_error_message("It appears as though you do not have permission to view information for the host group you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		else
			print_generic_error_message("There are no host groups defined.", NULL, 0);
	}

	return;
}


/* shows an overview of a specific hostgroup... */
int show_hostgroup_overview(hostgroup *hstgrp) {
	hostsmember *temp_member = NULL;
	host *temp_host = NULL;
	hoststatus *temp_hoststatus = NULL;
	statusdata *temp_status = NULL;
	int odd = 0;
	int json_start = TRUE;
	int partial_hosts = FALSE;
	int service_found = FALSE;

	/* make sure the user is authorized to view this hostgroup */
	if (show_partial_hostgroups == FALSE && is_authorized_for_hostgroup(hstgrp, &current_authdata) == FALSE)
		return FALSE;

	/* if we're showing partial hostgroups, find out if there will be any hosts that belong to the hostgroup */
	if (show_partial_hostgroups == TRUE) {
		for (temp_member = hstgrp->members; temp_member != NULL; temp_member = temp_member->next) {

			/* find the host... */
			temp_host = find_host(temp_member->host_name);
			if (temp_host == NULL)
				continue;

			/* only shown in partial hostgroups if user is authorized to view this host */
			if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
				continue;

			/* find the host status */
			temp_hoststatus = find_hoststatus(temp_host->name);
			if (temp_hoststatus == NULL)
				continue;

			/* make sure we will only be displaying hosts of the specified status levels */
			if (!(host_status_types & temp_hoststatus->status))
				continue;

			/* make sure we will only be displaying hosts that have the desired properties */
			if (passes_host_properties_filter(temp_hoststatus) == FALSE)
				continue;

			partial_hosts = TRUE;

			break;
		}
	}

	/* if we're showing partial hostgroups, but there are no hosts to display, there's nothing to see here */
	if (show_partial_hostgroups == TRUE && partial_hosts == FALSE)
		return FALSE;

	/* print json format */
	if (content_type == JSON_CONTENT) {
		printf("{ \"hostgroup_name\": \"%s\",\n", json_encode(hstgrp->group_name));
		printf("\"members\": [ \n");
	} else {
		printf("<TD VALIGN=top ALIGN=center>\n");
		printf("<DIV CLASS='status'>\n");
		printf("<A HREF='%s?hostgroup=%s&style=detail'>%s</A>", STATUS_CGI, url_encode(hstgrp->group_name), html_encode(hstgrp->alias, TRUE));
		printf(" (<A HREF='%s?type=%d&hostgroup=%s'>%s</A>)", EXTINFO_CGI, DISPLAY_HOSTGROUP_INFO, url_encode(hstgrp->group_name), html_encode(hstgrp->group_name, TRUE));
		printf("</DIV>\n");

		printf("<table border=1 CLASS='status' align='center'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='status'>Host</TH><TH CLASS='status'>Status</TH><TH CLASS='status'>Services</TH><TH CLASS='status'>Actions</TH>\n");
		printf("</TR>\n");
	}

	/* find all the hosts that belong to the hostgroup */
	for (temp_member = hstgrp->members; temp_member != NULL; temp_member = temp_member->next) {

		/* find the host... */
		temp_host = find_host(temp_member->host_name);
		if (temp_host == NULL)
			continue;

		/* only show in partial hostgroups if user is authorized to view this host */
		if (show_partial_hostgroups == TRUE && is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		/* find the host status */
		temp_hoststatus = find_hoststatus(temp_host->name);
		if (temp_hoststatus == NULL)
			continue;

		/* make sure we only display hosts of the specified status levels */
		if (!(host_status_types & temp_hoststatus->status))
			continue;

		/* make sure we only display hosts that have the desired properties */
		if (passes_host_properties_filter(temp_hoststatus) == FALSE)
			continue;

		/* check if there are any services to display */
		if (service_status_types != all_service_status_types) {
			service_found = FALSE;

			/* check all services... */
			for (temp_status = statusdata_list; temp_status != NULL; temp_status = temp_status->next) {

				if (temp_status->type != SERVICE_STATUS)
					continue;

				if (!strcmp(temp_host->name, temp_status->host_name)) {
					service_found = TRUE;
					break;
				}
			}

			if (service_found == FALSE)
				continue;
		}

		if (odd)
			odd = 0;
		else
			odd = 1;

		if (content_type == JSON_CONTENT) {
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
		}

		show_servicegroup_hostgroup_member_overview(temp_hoststatus, odd, NULL);
	}

	if (content_type == JSON_CONTENT)
		printf(" ] }\n");
	else {
		printf("</table>\n");
		printf("</TD>\n");
	}

	return TRUE;
}


/* shows a host status overview... */
void show_servicegroup_hostgroup_member_overview(hoststatus *hststatus, int odd, void *data) {
	char status[MAX_INPUT_BUFFER];
	char *status_bg_class = "";
	char *status_class = "";
	host *temp_host = NULL;
	char *processed_string = NULL;

	temp_host = find_host(hststatus->host_name);

	if (temp_host == NULL)
		return;

	/* grab macros */
	grab_host_macros_r(mac, temp_host);

	if (hststatus->status == HOST_PENDING) {
		strncpy(status, "PENDING", sizeof(status));
		status_class = "HOSTPENDING";
		status_bg_class = (odd) ? "Even" : "Odd";
	} else if (hststatus->status == HOST_UP) {
		strncpy(status, "UP", sizeof(status));
		status_class = "HOSTUP";
		status_bg_class = (odd) ? "Even" : "Odd";
	} else if (hststatus->status == HOST_DOWN) {
		strncpy(status, "DOWN", sizeof(status));
		status_class = "HOSTDOWN";
		status_bg_class = "HOSTDOWN";
	} else if (hststatus->status == HOST_UNREACHABLE) {
		strncpy(status, "UNREACHABLE", sizeof(status));
		status_class = "HOSTUNREACHABLE";
		status_bg_class = "HOSTUNREACHABLE";
	}

	status[sizeof(status) - 1] = '\x0';

	if (content_type == JSON_CONTENT) {
		printf("{ \"host_name\": \"%s\", ", json_encode(hststatus->host_name));
		printf("\"host_display_name\": \"%s\", ", (temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(temp_host->name));
		printf("\"host_status\": \"%s\", ", status);
		show_servicegroup_hostgroup_member_service_status_totals(hststatus->host_name, data);
		printf("}\n");
	} else {
		printf("<TR CLASS='status%s'>\n", status_bg_class);

		printf("<TD CLASS='status%s'>\n", status_bg_class);

		printf("<TABLE BORDER=0 WIDTH=100%% cellpadding=0 cellspacing=0>\n");
		printf("<TR CLASS='status%s'>\n", status_bg_class);
		if (!strcmp(temp_host->address6, temp_host->name))
			printf("<TD CLASS='status%s' align='left'><A HREF='%s?host=%s&style=detail' title='%s'>%s</A></TD>\n", status_bg_class, STATUS_CGI, url_encode(hststatus->host_name), temp_host->address, (temp_host->display_name != NULL) ? html_encode(temp_host->display_name, TRUE) : html_encode(temp_host->name, TRUE));
		else
			printf("<TD CLASS='status%s' align='left'><A HREF='%s?host=%s&style=detail' title='%s,%s'>%s</A></TD>\n", status_bg_class, STATUS_CGI, url_encode(hststatus->host_name), temp_host->address, temp_host->address6, (temp_host->display_name != NULL) ? html_encode(temp_host->display_name, TRUE) : html_encode(temp_host->name, TRUE));

		if (temp_host->icon_image != NULL) {
			printf("<TD CLASS='status%s' WIDTH=5></TD>\n", status_bg_class);
			printf("<TD CLASS='status%s' ALIGN=right>", status_bg_class);
			printf("<a href='%s?type=%d&host=%s'>", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(hststatus->host_name));
			printf("<IMG SRC='%s", url_logo_images_path);
			process_macros_r(mac, temp_host->icon_image, &processed_string, 0);
			printf("%s", processed_string);
			free(processed_string);
			printf("' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE), (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE));
			printf("</A>");
			printf("</TD>\n");
		}
		printf("</TR>\n");
		printf("</TABLE>\n");
		printf("</TD>\n");

		printf("<td CLASS='status%s'>%s</td>\n", status_class, status);

		printf("<td CLASS='status%s'>\n", status_bg_class);
		show_servicegroup_hostgroup_member_service_status_totals(hststatus->host_name, data);
		printf("</td>\n");

		printf("<td valign=center CLASS='status%s'>", status_bg_class);
		printf("<a href='%s?type=%d&host=%s'><img src='%s%s' border=0 alt='View Extended Information For This Host' title='View Extended Information For This Host'></a>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(hststatus->host_name), url_images_path, DETAIL_ICON);

		if (temp_host->notes_url != NULL) {
			process_macros_r(mac, temp_host->notes_url, &processed_string, 0);
			BEGIN_MULTIURL_LOOP
			printf("<A HREF='");
			printf("%s", processed_string);
			printf("' TARGET='%s'>", (notes_url_target == NULL) ? "_blank" : notes_url_target);
			printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, NOTES_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "View Extra Host Notes", "View Extra Host Notes");
			printf("</A>");
			END_MULTIURL_LOOP
			free(processed_string);
		}
		if (temp_host->action_url != NULL) {
			process_macros_r(mac, temp_host->action_url, &processed_string, 0);
			BEGIN_MULTIURL_LOOP
			printf("<A HREF='");
			printf("%s", processed_string);
			printf("' TARGET='%s'>", (action_url_target == NULL) ? "_blank" : action_url_target);
			printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, ACTION_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "Perform Extra Host Actions", "Perform Extra Host Actions");
			printf("</A>");
			END_MULTIURL_LOOP
			free(processed_string);
		}
		printf("<a href='%s?host=%s'><img src='%s%s' border=0 alt='View Service Details For This Host' title='View Service Details For This Host'></a>\n", STATUS_CGI, url_encode(hststatus->host_name), url_images_path, STATUS_DETAIL_ICON);
#ifdef USE_STATUSMAP
		printf("<A HREF='%s?host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'></A>", STATUSMAP_CGI, url_encode(hststatus->host_name), url_images_path, STATUSMAP_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "Locate Host On Map", "Locate Host On Map");
#endif
		printf("</TD>");

		printf("</TR>\n");
	}

	return;
}

/* shows services to a host status overview... */
void show_servicegroup_hostgroup_member_service_status_totals(char *host_name, void *data) {
	int total_ok = 0;
	int total_warning = 0;
	int total_unknown = 0;
	int total_critical = 0;
	int total_pending = 0;
	servicestatus *temp_servicestatus;
	statusdata *temp_status = NULL;
	servicegroup *temp_servicegroup = NULL;
	servicesmember *temp_member = NULL;
	char temp_buffer[MAX_INPUT_BUFFER];
	int service_found = FALSE;


	if (display_type == DISPLAY_SERVICEGROUPS) {
		temp_servicegroup = (servicegroup *)data;

		for (temp_member = temp_servicegroup->members; temp_member != NULL; temp_member = temp_member->next) {

			if (!strcmp(host_name, temp_member->host_name)) {

				if ((temp_servicestatus = find_servicestatus(temp_member->host_name, temp_member->service_description)) == NULL)
					continue;

				/* make sure we only display services of the specified status levels */
				if (!(service_status_types & temp_servicestatus->status))
					continue;

				/* make sure we only display services that have the desired properties */
				if (passes_service_properties_filter(temp_servicestatus) == FALSE)
					continue;

				if (temp_servicestatus->status == SERVICE_CRITICAL)
					total_critical++;
				else if (temp_servicestatus->status == SERVICE_WARNING)
					total_warning++;
				else if (temp_servicestatus->status == SERVICE_UNKNOWN)
					total_unknown++;
				else if (temp_servicestatus->status == SERVICE_OK)
					total_ok++;
				else if (temp_servicestatus->status == SERVICE_PENDING)
					total_pending++;
				else
					total_ok++;
			}
		}
	} else {
		/* check all services... */
		for (temp_status = statusdata_list; temp_status != NULL; temp_status = temp_status->next) {

			if (temp_status->type != SERVICE_STATUS)
				continue;

			if (!strcmp(host_name, temp_status->host_name)) {

				service_found = TRUE;

				if (temp_status->status == SERVICE_CRITICAL)
					total_critical++;
				else if (temp_status->status == SERVICE_WARNING)
					total_warning++;
				else if (temp_status->status == SERVICE_UNKNOWN)
					total_unknown++;
				else if (temp_status->status == SERVICE_OK)
					total_ok++;
				else if (temp_status->status == SERVICE_PENDING)
					total_pending++;
				else
					total_ok++;

				/* list is in alphabetic order
				   therefore all services for this host appear in a row
				   if host doesn't match anymore we are done
				*/
			} else if (service_found == TRUE)
				break;
		}
	}

	if (content_type == JSON_CONTENT) {
		printf("\"services_status_ok\": %d, ", total_ok);
		printf("\"services_status_warning\": %d, ", total_warning);
		printf("\"services_status_unknown\": %d, ", total_unknown);
		printf("\"services_status_critical\": %d, ", total_critical);
		printf("\"services_status_pending\": %d ", total_pending);
	} else {
		printf("<TABLE BORDER=0 WIDTH=100%%>\n");

		if (display_type == DISPLAY_SERVICEGROUPS)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "servicegroup=%s&style=detail", url_encode(temp_servicegroup->group_name));
		else
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "host=%s", url_encode(host_name));
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';

		if (total_ok > 0)
			printf("<TR><TD CLASS='miniStatusOK'><A HREF='%s?%s&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d OK</A></TD></TR>\n", STATUS_CGI, temp_buffer, SERVICE_OK, host_status_types, service_properties, host_properties, total_ok);
		if (total_warning > 0)
			printf("<TR><TD CLASS='miniStatusWARNING'><A HREF='%s?%s&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d WARNING</A></TD></TR>\n", STATUS_CGI, temp_buffer, SERVICE_WARNING, host_status_types, service_properties, host_properties, total_warning);
		if (total_unknown > 0)
			printf("<TR><TD CLASS='miniStatusUNKNOWN'><A HREF='%s?%s&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d UNKNOWN</A></TD></TR>\n", STATUS_CGI, temp_buffer, SERVICE_UNKNOWN, host_status_types, service_properties, host_properties, total_unknown);
		if (total_critical > 0)
			printf("<TR><TD CLASS='miniStatusCRITICAL'><A HREF='%s?%s&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d CRITICAL</A></TD></TR>\n", STATUS_CGI, temp_buffer, SERVICE_CRITICAL, host_status_types, service_properties, host_properties, total_critical);
		if (total_pending > 0)
			printf("<TR><TD CLASS='miniStatusPENDING'><A HREF='%s?%s&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d PENDING</A></TD></TR>\n", STATUS_CGI, temp_buffer, SERVICE_PENDING, host_status_types, service_properties, host_properties, total_pending);

		printf("</TABLE>\n");

		if ((total_ok + total_warning + total_unknown + total_critical + total_pending) == 0)
			printf("No matching services");
	}

	return;
}


/* show a summary of hostgroup(s)... */
void show_hostgroup_summaries(void) {
	hostgroup *temp_hostgroup = NULL;
	hostsmember *temp_member = NULL;
	host *temp_host = NULL;
	hoststatus *temp_hoststatus = NULL;
	statusdata *temp_status = NULL;
	int user_has_seen_something = FALSE;
	int odd = 0;
	int json_start = TRUE;
	int partial_hosts = FALSE;
	int i = 0, found = FALSE;

	if (content_type == JSON_CONTENT) {
		printf("\"hostgroup_summary\": [\n");
	} else {
		printf("<table border=1 CLASS='status' align='center'>\n");
		printf("<TR>\n");
		printf("<TH CLASS='status'>Host Group</TH><TH CLASS='status'>Host Status Summary</TH><TH CLASS='status'>Service Status Summary</TH>\n");
		printf("</TR>\n");
	}

	/* display status summary for hostgroups */
	for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {
		partial_hosts = FALSE;

		/* view only selected hostgroups */
		if (show_all_hostgroups == FALSE) {
			found = FALSE;
			for (i = 0; req_hostgroups[i].entry != NULL; i++) {
				if (!strcmp(req_hostgroups[i].entry, temp_hostgroup->group_name)) {
					found = TRUE;
					break;
				}
			}
			if (found == FALSE)
				continue;
		}

		/* make sure the user is authorized to view this hostgroup */
		if (show_partial_hostgroups == FALSE && is_authorized_for_hostgroup(temp_hostgroup, &current_authdata) == FALSE)
			continue;

		/* if we're showing partial hostgroups, find out if there will be any hosts that belong to the hostgroup */
		/* or if we have to filter for service status types*/
		if (show_partial_hostgroups == TRUE || service_status_types != all_service_status_types) {
			found = FALSE;
			for (temp_member = temp_hostgroup->members; temp_member != NULL; temp_member = temp_member->next) {

				/* find the host... */
				temp_host = find_host(temp_member->host_name);
				if (temp_host == NULL)
					continue;

				/* only shown in partial hostgroups if user is authorized to view this host */
				if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
					continue;

				/* find the host status */
				temp_hoststatus = find_hoststatus(temp_host->name);
				if (temp_hoststatus == NULL)
					continue;

				/* make sure we will only be displaying hosts of the specified status levels */
				if (!(host_status_types & temp_hoststatus->status))
					continue;

				/* make sure we will only be displaying hosts that have the desired properties */
				if (passes_host_properties_filter(temp_hoststatus) == FALSE)
					continue;

				/* check if there are any services to display */
				if (service_status_types != all_service_status_types && found == FALSE) {

					/* check all services... */
					for (temp_status = statusdata_list; temp_status != NULL; temp_status = temp_status->next) {

						if (temp_status->type != SERVICE_STATUS)
							continue;

						if (!strcmp(temp_host->name, temp_status->host_name)) {
							found = TRUE;
							break;
						}
					}
				}

				partial_hosts = TRUE;

				break;
			}
		}

		/* if we're showing partial hostgroups, but there are no hosts to display, there's nothing to see here */
		if (show_partial_hostgroups == TRUE && partial_hosts == FALSE)
			continue;

		user_has_seen_something = TRUE;

		/* if there are no services to display try next hostgroup */
		if (service_status_types != all_service_status_types && found == FALSE)
			continue;

		if (odd == 0)
			odd = 1;
		else
			odd = 0;

		if (content_type == JSON_CONTENT) {
			/* always add a comma, except for the first line */
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;
		}

		/* show summary for this hostgroup */
		show_hostgroup_summary(temp_hostgroup, odd);
	}

	if (content_type == JSON_CONTENT)
		printf(" ]\n");
	else
		printf("</TABLE>\n");

	/* if user couldn't see anything, print out some helpful info... */
	if (user_has_seen_something == FALSE) {

		if (content_type == JSON_CONTENT)
			printf(",\n");

		if (hostgroup_list != NULL)
			print_generic_error_message("It appears as though you do not have permission to view information for the host group you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		else
			print_generic_error_message("There are no host groups defined.", NULL, 0);
	}

	return;
}


/* displays status summary information for a specific hostgroup */
void show_hostgroup_summary(hostgroup *temp_hostgroup, int odd) {
	char *status_bg_class = "";

	if (content_type == JSON_CONTENT) {
		printf("{ \"hostgroup_name\": \"%s\",\n", json_encode(temp_hostgroup->group_name));
		show_hostgroup_host_totals_summary(temp_hostgroup);
		show_hostgroup_service_totals_summary(temp_hostgroup);
		printf("}\n");
	} else {
		if (odd == 1)
			status_bg_class = "Even";
		else
			status_bg_class = "Odd";

		printf("<TR CLASS='status%s'><TD CLASS='status%s'>\n", status_bg_class, status_bg_class);
		printf("<A HREF='%s?hostgroup=%s&style=overview'>%s</A> ", STATUS_CGI, url_encode(temp_hostgroup->group_name), html_encode(temp_hostgroup->alias, TRUE));
		printf("(<A HREF='%s?type=%d&hostgroup=%s'>%s</a>)", EXTINFO_CGI, DISPLAY_HOSTGROUP_INFO, url_encode(temp_hostgroup->group_name), html_encode(temp_hostgroup->group_name, TRUE));
		printf("</TD>");

		printf("<TD CLASS='status%s' ALIGN=CENTER VALIGN=CENTER>", status_bg_class);
		show_hostgroup_host_totals_summary(temp_hostgroup);
		printf("</TD>");

		printf("<TD CLASS='status%s' ALIGN=CENTER VALIGN=CENTER>", status_bg_class);
		show_hostgroup_service_totals_summary(temp_hostgroup);
		printf("</TD>");

		printf("</TR>\n");
	}

	return;
}


/* shows host total summary information for a specific hostgroup */
void show_hostgroup_host_totals_summary(hostgroup *temp_hostgroup) {
	hostsmember *temp_member;
	int hosts_up = 0;
	int hosts_down = 0;
	int hosts_unreachable = 0;
	int hosts_pending = 0;
	int hosts_down_scheduled = 0;
	int hosts_down_acknowledged = 0;
	int hosts_down_disabled = 0;
	int hosts_down_unacknowledged = 0;
	int hosts_unreachable_scheduled = 0;
	int hosts_unreachable_acknowledged = 0;
	int hosts_unreachable_disabled = 0;
	int hosts_unreachable_unacknowledged = 0;
	hoststatus *temp_hoststatus;
	host *temp_host;
	int problem = FALSE;

	/* find all the hosts that belong to the hostgroup */
	for (temp_member = temp_hostgroup->members; temp_member != NULL; temp_member = temp_member->next) {

		/* find the host... */
		temp_host = find_host(temp_member->host_name);
		if (temp_host == NULL)
			continue;

		/* only shown in partial hostgroups if user is authorized to view this host */
		if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		/* find the host status */
		temp_hoststatus = find_hoststatus(temp_host->name);
		if (temp_hoststatus == NULL)
			continue;

		/* make sure we only display hosts of the specified status levels */
		if (!(host_status_types & temp_hoststatus->status))
			continue;

		/* make sure we only display hosts that have the desired properties */
		if (passes_host_properties_filter(temp_hoststatus) == FALSE)
			continue;

		problem = TRUE;

		if (temp_hoststatus->status == HOST_UP)
			hosts_up++;

		else if (temp_hoststatus->status == HOST_DOWN) {
			if (temp_hoststatus->scheduled_downtime_depth > 0) {
				hosts_down_scheduled++;
				problem = FALSE;
			}
			if (temp_hoststatus->problem_has_been_acknowledged == TRUE) {
				hosts_down_acknowledged++;
				problem = FALSE;
			}
			if (temp_hoststatus->checks_enabled == FALSE && temp_hoststatus->accept_passive_host_checks == FALSE) {
				hosts_down_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				hosts_down_unacknowledged++;
			hosts_down++;
		}

		else if (temp_hoststatus->status == HOST_UNREACHABLE) {
			if (temp_hoststatus->scheduled_downtime_depth > 0) {
				hosts_unreachable_scheduled++;
				problem = FALSE;
			}
			if (temp_hoststatus->problem_has_been_acknowledged == TRUE) {
				hosts_unreachable_acknowledged++;
				problem = FALSE;
			}
			if (temp_hoststatus->checks_enabled == FALSE && temp_hoststatus->accept_passive_host_checks == FALSE) {
				hosts_unreachable_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				hosts_unreachable_unacknowledged++;
			hosts_unreachable++;
		}

		else
			hosts_pending++;
	}

	if (content_type == JSON_CONTENT) {
		printf("\"hosts_up\": %d, ", hosts_up);
		printf("\"hosts_down\": %d, ", hosts_down);
		printf("\"hosts_down_unacknowledged\": %d, ", hosts_down_unacknowledged);
		printf("\"hosts_down_scheduled\": %d, ", hosts_down_scheduled);
		printf("\"hosts_down_acknowledged\": %d, ", hosts_down_acknowledged);
		printf("\"hosts_down_disabled\": %d, ", hosts_down_disabled);
		printf("\"hosts_unreachable\": %d, ", hosts_unreachable);
		printf("\"hosts_unreachable_unacknowledged\": %d, ", hosts_unreachable_unacknowledged);
		printf("\"hosts_unreachable_scheduled\": %d, ", hosts_unreachable_scheduled);
		printf("\"hosts_unreachable_acknowledged\": %d, ", hosts_unreachable_acknowledged);
		printf("\"hosts_unreachable_disabled\": %d, ", hosts_unreachable_disabled);
		printf("\"hosts_pending\": %d, ", hosts_pending);
	} else {
		printf("<TABLE BORDER='0'>\n");

		if (hosts_up > 0) {
			printf("<TR>");
			printf("<TD CLASS='miniStatusUP'><A HREF='%s?hostgroup=%s&style=hostdetail&&hoststatustypes=%d&hostprops=%lu'>%d UP</A></TD>", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_UP, host_properties, hosts_up);
			printf("</TR>\n");
		}

		if (hosts_down > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusDOWN'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusDOWN'><A HREF='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%lu'>%d DOWN</A>&nbsp;:</TD>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_DOWN, host_properties, hosts_down);

			printf("<TD><TABLE BORDER='0'>\n");

			if (hosts_down_unacknowledged > 0)
				printf("<tr><td width=100%% class='hostImportantProblem'><a href='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_DOWN, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED, hosts_down_unacknowledged);

			if (hosts_down_scheduled > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_DOWN, HOST_SCHEDULED_DOWNTIME, hosts_down_scheduled);

			if (hosts_down_acknowledged > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_DOWN, HOST_STATE_ACKNOWLEDGED, hosts_down_acknowledged);

			if (hosts_down_disabled > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_DOWN, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED, hosts_down_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (hosts_unreachable > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusUNREACHABLE'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusUNREACHABLE'><A HREF='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%lu'>%d UNREACHABLE</A>&nbsp;:</TD>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_UNREACHABLE, host_properties, hosts_unreachable);

			printf("<TD><TABLE BORDER='0'>\n");

			if (hosts_unreachable_unacknowledged > 0)
				printf("<tr><td width=100%% class='hostImportantProblem'><a href='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_UNREACHABLE, HOST_NO_SCHEDULED_DOWNTIME | HOST_STATE_UNACKNOWLEDGED | HOST_NOT_ALL_CHECKS_DISABLED, hosts_unreachable_unacknowledged);

			if (hosts_unreachable_scheduled > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_UNREACHABLE, HOST_SCHEDULED_DOWNTIME, hosts_unreachable_scheduled);

			if (hosts_unreachable_acknowledged > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_UNREACHABLE, HOST_STATE_ACKNOWLEDGED, hosts_unreachable_acknowledged);

			if (hosts_unreachable_disabled > 0)
				printf("<tr><td width=100%% class='hostUnimportantProblem'><a href='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_UNREACHABLE, HOST_CHECKS_DISABLED | HOST_PASSIVE_CHECKS_DISABLED, hosts_unreachable_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (hosts_pending > 0)
			printf("<TR><TD CLASS='miniStatusPENDING'><A HREF='%s?hostgroup=%s&style=hostdetail&hoststatustypes=%d&hostprops=%lu'>%d PENDING</A></TD></TR>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), HOST_PENDING, host_properties, hosts_pending);

		printf("</TABLE>\n");

		if ((hosts_up + hosts_down + hosts_unreachable + hosts_pending) == 0)
			printf("No matching hosts");
	}

	return;
}


/* shows service total summary information for a specific hostgroup */
void show_hostgroup_service_totals_summary(hostgroup *temp_hostgroup) {
	int services_ok = 0;
	int services_warning = 0;
	int services_unknown = 0;
	int services_critical = 0;
	int services_pending = 0;
	int services_warning_host_problem = 0;
	int services_warning_scheduled = 0;
	int services_warning_acknowledged = 0;
	int services_warning_disabled = 0;
	int services_warning_unacknowledged = 0;
	int services_unknown_host_problem = 0;
	int services_unknown_scheduled = 0;
	int services_unknown_acknowledged = 0;
	int services_unknown_disabled = 0;
	int services_unknown_unacknowledged = 0;
	int services_critical_host_problem = 0;
	int services_critical_scheduled = 0;
	int services_critical_acknowledged = 0;
	int services_critical_disabled = 0;
	int services_critical_unacknowledged = 0;
	hoststatus *temp_hoststatus = NULL;
	host *temp_host = NULL;
	statusdata *temp_status = NULL;
	int problem = FALSE;

	/* check all services... */
	for (temp_status = statusdata_list; temp_status != NULL; temp_status = temp_status->next) {

		if (temp_status->type != SERVICE_STATUS)
			continue;

		/* find the host this service is associated with */
		temp_host = find_host(temp_status->host_name);
		if (temp_host == NULL)
			continue;

		/* only shown if user is authorized to view this host */
		if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		/* see if this service is associated with a host in the specified hostgroup */
		if (is_host_member_of_hostgroup(temp_hostgroup, temp_host) == FALSE)
			continue;

		/* find the status of the associated host */
		temp_hoststatus = find_hoststatus(temp_status->host_name);
		if (temp_hoststatus == NULL)
			continue;

		/* make sure we only display hosts of the specified status levels */
		if (!(host_status_types & temp_hoststatus->status))
			continue;

		/* make sure we only display hosts that have the desired properties */
		if (passes_host_properties_filter(temp_hoststatus) == FALSE)
			continue;

		problem = TRUE;

		if (temp_status->status == SERVICE_OK)
			services_ok++;

		else if (temp_status->status == SERVICE_WARNING) {
			if (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE) {
				services_warning_host_problem++;
				problem = FALSE;
			}
			if (temp_status->scheduled_downtime_depth > 0) {
				services_warning_scheduled++;
				problem = FALSE;
			}
			if (temp_status->problem_has_been_acknowledged == TRUE) {
				services_warning_acknowledged++;
				problem = FALSE;
			}
			if (temp_status->checks_enabled == FALSE && temp_status->accept_passive_checks == FALSE) {
				services_warning_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				services_warning_unacknowledged++;
			services_warning++;
		}

		else if (temp_status->status == SERVICE_UNKNOWN) {
			if (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE) {
				services_unknown_host_problem++;
				problem = FALSE;
			}
			if (temp_status->scheduled_downtime_depth > 0) {
				services_unknown_scheduled++;
				problem = FALSE;
			}
			if (temp_status->problem_has_been_acknowledged == TRUE) {
				services_unknown_acknowledged++;
				problem = FALSE;
			}
			if (temp_status->checks_enabled == FALSE && temp_status->accept_passive_checks == FALSE) {
				services_unknown_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				services_unknown_unacknowledged++;
			services_unknown++;
		}

		else if (temp_status->status == SERVICE_CRITICAL) {
			if (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE) {
				services_critical_host_problem++;
				problem = FALSE;
			}
			if (temp_status->scheduled_downtime_depth > 0) {
				services_critical_scheduled++;
				problem = FALSE;
			}
			if (temp_status->problem_has_been_acknowledged == TRUE) {
				services_critical_acknowledged++;
				problem = FALSE;
			}
			if (temp_status->checks_enabled == FALSE && temp_status->accept_passive_checks == FALSE) {
				services_critical_disabled++;
				problem = FALSE;
			}
			if (problem == TRUE)
				services_critical_unacknowledged++;
			services_critical++;
		}

		else if (temp_status->status == SERVICE_PENDING)
			services_pending++;
	}

	if (content_type == JSON_CONTENT) {
		printf("\"services_ok\": %d, ", services_ok);
		printf("\"services_warning\": %d, ", services_warning);
		printf("\"services_warning_unacknowledged\": %d, ", services_warning_unacknowledged);
		printf("\"services_warning_host_problem\": %d, ", services_warning_host_problem);
		printf("\"services_warning_scheduled\": %d, ", services_warning_scheduled);
		printf("\"services_warning_acknowledged\": %d, ", services_warning_acknowledged);
		printf("\"services_warning_disabled\": %d, ", services_warning_disabled);
		printf("\"services_unknown\": %d, ", services_unknown);
		printf("\"services_unknown_unacknowledged\": %d, ", services_unknown_unacknowledged);
		printf("\"services_unknown_host_problem\": %d, ", services_unknown_host_problem);
		printf("\"services_unknown_scheduled\": %d, ", services_unknown_scheduled);
		printf("\"services_unknown_acknowledged\": %d, ", services_unknown_acknowledged);
		printf("\"services_unknown_disabled\": %d, ", services_unknown_disabled);
		printf("\"services_critical\": %d, ", services_critical);
		printf("\"services_critical_unacknowledged\": %d, ", services_critical_unacknowledged);
		printf("\"services_critical_host_problem\": %d, ", services_critical_host_problem);
		printf("\"services_critical_scheduled\": %d, ", services_critical_scheduled);
		printf("\"services_critical_acknowledged\": %d, ", services_critical_acknowledged);
		printf("\"services_critical_disabled\": %d, ", services_critical_disabled);
		printf("\"services_pending\": %d ", services_pending);
	} else {
		printf("<TABLE BORDER=0>\n");

		if (services_ok > 0)
			printf("<TR><TD CLASS='miniStatusOK'><A HREF='%s?hostgroup=%s&style=detail&&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d OK</A></TD></TR>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_OK, host_status_types, service_properties, host_properties, services_ok);

		if (services_warning > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusWARNING'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusWARNING'><A HREF='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d WARNING</A>&nbsp;:</TD>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_WARNING, host_status_types, service_properties, host_properties, services_warning);

			printf("<TD><TABLE BORDER='0'>\n");

			if (services_warning_unacknowledged > 0)
				printf("<tr><td width=100%% class='serviceImportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_WARNING, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, services_warning_unacknowledged);

			if (services_warning_host_problem > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d'>%d on Problem Hosts</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_WARNING, HOST_DOWN | HOST_UNREACHABLE, services_warning_host_problem);

			if (services_warning_scheduled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_WARNING, SERVICE_SCHEDULED_DOWNTIME, services_warning_scheduled);

			if (services_warning_acknowledged > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_WARNING, SERVICE_STATE_ACKNOWLEDGED, services_warning_acknowledged);

			if (services_warning_disabled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_WARNING, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_warning_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (services_unknown > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusUNKNOWN'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusUNKNOWN'><A HREF='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d UNKNOWN</A>&nbsp;:</TD>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_UNKNOWN, host_status_types, service_properties, host_properties, services_unknown);

			printf("<TD><TABLE BORDER='0'>\n");

			if (services_unknown_unacknowledged > 0)
				printf("<tr><td width=100%% class='serviceImportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_UNKNOWN, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, services_unknown_unacknowledged);

			if (services_unknown_host_problem > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d'>%d on Problem Hosts</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_UNKNOWN, HOST_DOWN | HOST_UNREACHABLE, services_unknown_host_problem);

			if (services_unknown_scheduled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_UNKNOWN, SERVICE_SCHEDULED_DOWNTIME, services_unknown_scheduled);

			if (services_unknown_acknowledged > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_UNKNOWN, SERVICE_STATE_ACKNOWLEDGED, services_unknown_acknowledged);

			if (services_unknown_disabled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_UNKNOWN, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_unknown_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (services_critical > 0) {
			printf("<TR>\n");
			printf("<TD CLASS='miniStatusCRITICAL'><TABLE BORDER='0'>\n");
			printf("<TR>\n");

			printf("<TD CLASS='miniStatusCRITICAL'><A HREF='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d CRITICAL</A>&nbsp;:</TD>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_CRITICAL, host_status_types, service_properties, host_properties, services_critical);

			printf("<TD><TABLE BORDER='0'>\n");

			if (services_critical_unacknowledged > 0)
				printf("<tr><td width=100%% class='serviceImportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%d'>%d Unhandled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_CRITICAL, HOST_UP | HOST_PENDING, SERVICE_NO_SCHEDULED_DOWNTIME | SERVICE_STATE_UNACKNOWLEDGED | SERVICE_NOT_ALL_CHECKS_DISABLED, services_critical_unacknowledged);

			if (services_critical_host_problem > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d'>%d on Problem Hosts</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_CRITICAL, HOST_DOWN | HOST_UNREACHABLE, services_critical_host_problem);

			if (services_critical_scheduled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Scheduled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_CRITICAL, SERVICE_SCHEDULED_DOWNTIME, services_critical_scheduled);

			if (services_critical_acknowledged > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Acknowledged</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_CRITICAL, SERVICE_STATE_ACKNOWLEDGED, services_critical_acknowledged);

			if (services_critical_disabled > 0)
				printf("<tr><td width=100%% class='serviceUnimportantProblem'><a href='%s?hostgroup=%s&style=detail&servicestatustypes=%d&serviceprops=%d'>%d Disabled</a></td></tr>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_CRITICAL, SERVICE_CHECKS_DISABLED | SERVICE_PASSIVE_CHECKS_DISABLED, services_critical_disabled);

			printf("</TABLE></TD>\n");

			printf("</TR>\n");
			printf("</TABLE></TD>\n");
			printf("</TR>\n");
		}

		if (services_pending > 0)
			printf("<TR><TD CLASS='miniStatusPENDING'><A HREF='%s?hostgroup=%s&style=detail&servicestatustypes=%d&hoststatustypes=%d&serviceprops=%lu&hostprops=%lu'>%d PENDING</A></TD></TR>\n", STATUS_CGI, url_encode(temp_hostgroup->group_name), SERVICE_PENDING, host_status_types, service_properties, host_properties, services_pending);

		printf("</TABLE>\n");

		if ((services_ok + services_warning + services_unknown + services_critical + services_pending) == 0)
			printf("No matching services");
	}

	return;
}


/* show a grid layout of hostgroup(s)... */
void show_hostgroup_grids(void) {
	hostgroup *temp_hostgroup = NULL;
	int user_has_seen_something = FALSE;
	int json_start = TRUE;
	int i = 0, found = FALSE;

	if (content_type == JSON_CONTENT) {
		printf("\"hostgroup_grid\": [\n");
	}

	/* display status grids for hostgroups */
	for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {

		/* view only selected hostgroups */
		if (show_all_hostgroups == FALSE) {
			found = FALSE;
			for (i = 0; req_hostgroups[i].entry != NULL; i++) {
				if (!strcmp(req_hostgroups[i].entry, temp_hostgroup->group_name)) {
					found = TRUE;
					break;
				}
			}
			if (found == FALSE)
				continue;
		}

		/* make sure the user is authorized to view this hostgroup */
		if (show_partial_hostgroups == FALSE && is_authorized_for_hostgroup(temp_hostgroup, &current_authdata) == FALSE)
			continue;

		if (content_type == JSON_CONTENT) {
			/* always add a comma, except for the first line */
			if (json_start == FALSE)
				printf(",\n");
		}

		/* show grid for this hostgroup */
		if (show_hostgroup_grid(temp_hostgroup) == FALSE)
			continue;

		json_start = FALSE;

		user_has_seen_something = TRUE;
	}

	if (content_type == JSON_CONTENT)
		printf(" ]\n");

	/* if user couldn't see anything, print out some helpful info... */
	if (user_has_seen_something == FALSE) {

		if (content_type == JSON_CONTENT)
			printf(",\n");

		if (hostgroup_list != NULL)
			print_generic_error_message("It appears as though you do not have permission to view information for the host group you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		else
			print_generic_error_message("There are no host groups defined.", NULL, 0);
	}

	return;
}


/* displays status grid for a specific hostgroup */
int show_hostgroup_grid(hostgroup *temp_hostgroup) {
	hostsmember *temp_member;
	char *status_bg_class = "";
	char *status = "";
	char *host_status_class = "";
	char *service_status_class = "";
	host *temp_host;
	service *temp_service;
	hoststatus *temp_hoststatus;
	statusdata *temp_status = NULL;
	char *processed_string = NULL;
	int odd = 0;
	int current_item;
	int json_start = TRUE;
	int json_start2 = TRUE;
	int partial_hosts = FALSE;
	int service_found = FALSE;

	/* make sure the user is authorized to view this hostgroup */
	if (show_partial_hostgroups == FALSE && is_authorized_for_hostgroup(temp_hostgroup, &current_authdata) == FALSE)
		return FALSE;

	/* if we're showing partial hostgroups, find out if there will be any hosts that belong to the hostgroup */
	if (show_partial_hostgroups == TRUE) {
		for (temp_member = temp_hostgroup->members; temp_member != NULL; temp_member = temp_member->next) {

			/* find the host... */
			temp_host = find_host(temp_member->host_name);
			if (temp_host == NULL)
				continue;

			/* only shown in partial hostgroups if user is authorized to view this host */
			if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
				continue;

			/* find the host status */
			temp_hoststatus = find_hoststatus(temp_host->name);
			if (temp_hoststatus == NULL)
				continue;

			/* make sure we will only be displaying hosts of the specified status levels */
			if (!(host_status_types & temp_hoststatus->status))
				continue;

			/* make sure we will only be displaying hosts that have the desired properties */
			if (passes_host_properties_filter(temp_hoststatus) == FALSE)
				continue;

			partial_hosts = TRUE;

			break;
		}
	}

	/* if we're showing partial hostgroups, but there are no hosts to display, there's nothing to see here */
	if (show_partial_hostgroups == TRUE && partial_hosts == FALSE)
		return FALSE;

	if (content_type == JSON_CONTENT) {
		printf("{ \"hostgroup_name\": \"%s\",\n", json_encode(temp_hostgroup->group_name));
		printf("\"members\": [ \n");
	} else {
		printf("<DIV CLASS='status'><A HREF='%s?hostgroup=%s&style=detail'>%s</A>", STATUS_CGI, url_encode(temp_hostgroup->group_name), html_encode(temp_hostgroup->alias, TRUE));
		printf(" (<A HREF='%s?type=%d&hostgroup=%s'>%s</A>)</DIV>", EXTINFO_CGI, DISPLAY_HOSTGROUP_INFO, url_encode(temp_hostgroup->group_name), html_encode(temp_hostgroup->group_name, TRUE));

		printf("<TABLE BORDER=1 CLASS='status' align='center'>\n");
		printf("<TR><TH CLASS='status'>Host</TH><TH CLASS='status'>Services</a></TH><TH CLASS='status'>Actions</TH></TR>\n");
	}

	/* find all the hosts that belong to the hostgroup */
	for (temp_member = temp_hostgroup->members; temp_member != NULL; temp_member = temp_member->next) {

		/* find the host... */
		temp_host = find_host(temp_member->host_name);
		if (temp_host == NULL)
			continue;

		/* only show in partial hostgroups if user is authorized to view this host */
		if (show_partial_hostgroups == TRUE && is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		/* find the host status */
		temp_hoststatus = find_hoststatus(temp_host->name);
		if (temp_hoststatus == NULL)
			continue;

		/* make sure we only display hosts of the specified status levels */
		if (!(host_status_types & temp_hoststatus->status))
			continue;

		/* make sure we only display hosts that have the desired properties */
		if (passes_host_properties_filter(temp_hoststatus) == FALSE)
			continue;

		/* check if there is any service to display */
		if (service_status_types != all_service_status_types) {
			service_found = FALSE;

			/* check all services... */
			for (temp_status = statusdata_list; temp_status != NULL; temp_status = temp_status->next) {

				if (temp_status->type != SERVICE_STATUS)
					continue;

				if (!strcmp(temp_host->name, temp_status->host_name)) {
					service_found = TRUE;
					break;
				}
			}

			if (service_found == FALSE)
				continue;
		}

		/* grab macros */
		grab_host_macros_r(mac, temp_host);

		if (odd == 1) {
			status_bg_class = "Even";
			odd = 0;
		} else {
			status_bg_class = "Odd";
			odd = 1;
		}

		if (content_type != JSON_CONTENT)
			printf("<TR CLASS='status%s'>\n", status_bg_class);

		/* get the status of the host */
		if (temp_hoststatus->status == HOST_DOWN) {
			status = "DOWN";
			host_status_class = "HOSTDOWN";
		} else if (temp_hoststatus->status == HOST_UNREACHABLE) {
			status = "UNREACHABLE";
			host_status_class = "HOSTUNREACHABLE";
		} else {
			status = "OK";
			host_status_class = status_bg_class;
		}

		if (content_type == JSON_CONTENT) {
			if (json_start == FALSE)
				printf(",\n");
			json_start = FALSE;

			printf("{ \"host_name\": \"%s\",\n", json_encode(temp_host->name));
			printf("\"host_display_name\": \"%s\", ", (temp_host->display_name != NULL) ? json_encode(temp_host->display_name) : json_encode(temp_host->name));
			printf("\"host_status\": \"%s\",\n", status);
			printf("\"services\": [ \n");
		} else {
			printf("<TD CLASS='status%s'>", host_status_class);

			printf("<TABLE BORDER=0 WIDTH='100%%' cellpadding=0 cellspacing=0>\n");
			printf("<TR>\n");
			printf("<TD ALIGN=LEFT>\n");
			printf("<TABLE BORDER=0 cellpadding=0 cellspacing=0>\n");
			printf("<TR>\n");
			printf("<TD align=left valign=center CLASS='status%s'>", host_status_class);
			printf("<A HREF='%s?type=%d&host=%s'>%s</A>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_host->name), (temp_host->display_name != NULL) ? html_encode(temp_host->display_name, TRUE) : html_encode(temp_host->name, TRUE));
			printf("</TD>\n");
			printf("</TR>\n");
			printf("</TABLE>\n");
			printf("</TD>\n");
			printf("<TD align=right valign=center nowrap>\n");
			printf("<TABLE BORDER=0 cellpadding=0 cellspacing=0>\n");
			printf("<TR>\n");

			if (temp_host->icon_image != NULL) {
				printf("<TD align=center valign=center>");
				printf("<A HREF='%s?type=%d&host=%s'>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_host->name));
				printf("<IMG SRC='%s", url_logo_images_path);
				process_macros_r(mac, temp_host->icon_image, &processed_string, 0);
				printf("%s", processed_string);
				free(processed_string);
				printf("' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE), (temp_host->icon_image_alt == NULL) ? "" : html_encode(temp_host->icon_image_alt, TRUE));
				printf("</A>");
				printf("<TD>\n");
			}
			printf("<TD>\n");

			printf("</TR>\n");
			printf("</TABLE>\n");
			printf("</TD>\n");
			printf("</TR>\n");
			printf("</TABLE>\n");

			printf("</TD>\n");

			printf("<TD CLASS='status%s' style='text-align:center'>", host_status_class);
		}

		/* display all services on the host */
		current_item = 1;
		json_start2 = TRUE;
		service_found = FALSE;
		for (temp_status = statusdata_list; temp_status != NULL; temp_status = temp_status->next) {

			if (temp_status->type != SERVICE_STATUS)
				continue;

			if (!strcmp(temp_host->name, temp_status->host_name)) {

				service_found = TRUE;

				if (temp_status == NULL)
					service_status_class = "NULL";
				else if (temp_status->status == SERVICE_OK)
					service_status_class = "OK";
				else if (temp_status->status == SERVICE_WARNING)
					service_status_class = "WARNING";
				else if (temp_status->status == SERVICE_UNKNOWN)
					service_status_class = "UNKNOWN";
				else if (temp_status->status == SERVICE_CRITICAL)
					service_status_class = "CRITICAL";
				else
					service_status_class = "PENDING";

				if (content_type == JSON_CONTENT) {
					if (json_start2 == FALSE)
						printf(",\n");
					json_start2 = FALSE;

					printf("{ \"service_description\": \"%s\",\n", json_encode(temp_status->svc_description));

					temp_service = find_service(temp_status->host_name, temp_status->svc_description);
					printf("\"service_display_name\": \"%s\",\n", (temp_service != NULL && temp_service->display_name != NULL) ? json_encode(temp_service->display_name) : json_encode(temp_status->svc_description));

					if (temp_status == NULL)
						printf("\"service_status\": null } ");
					else
						printf("\"service_status\": \"%s\" } ", service_status_class);
				} else {
					if (current_item > max_grid_width && max_grid_width > 0) {
						printf("<BR>\n");
						current_item = 1;
					}

					printf("<A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_status->host_name));
					printf("&service=%s' CLASS='status%s'>%s</A>&nbsp;", url_encode(temp_status->svc_description), service_status_class, html_encode(temp_status->svc_description, TRUE));

					current_item++;
				}

				/* list is in alphabetic order
				   therefore all services for this host appear in a row
				   if host doesn't match anymore we are done
				*/
			} else if (service_found == TRUE)
				break;
		}

		/* Print no matching in case of no services */
		if (current_item == 1 && content_type != JSON_CONTENT)
			printf("No matching services");

		if (content_type == JSON_CONTENT) {
			printf(" ] } \n");
		} else {
			printf("</TD>\n");

			/* actions */
			printf("<TD CLASS='status%s'>", host_status_class);

			printf("<A HREF='%s?type=%d&host=%s'>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_host->name));
			printf("<IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, DETAIL_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "View Extended Information For This Host", "View Extended Information For This Host");
			printf("</A>");

			if (temp_host->notes_url != NULL) {
				process_macros_r(mac, temp_host->notes_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'>", (notes_url_target == NULL) ? "_blank" : notes_url_target);
				printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, NOTES_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "View Extra Host Notes", "View Extra Host Notes");
				printf("</A>");
				END_MULTIURL_LOOP
				free(processed_string);
			}
			if (temp_host->action_url != NULL) {
				process_macros_r(mac, temp_host->action_url, &processed_string, 0);
				BEGIN_MULTIURL_LOOP
				printf("<A HREF='");
				printf("%s", processed_string);
				printf("' TARGET='%s'>", (action_url_target == NULL) ? "_blank" : action_url_target);
				printf("<IMG SRC='%s%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'>", url_images_path, MU_iconstr, ACTION_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "Perform Extra Host Actions", "Perform Extra Host Actions");
				printf("</A>");
				END_MULTIURL_LOOP
				free(processed_string);
			}

			printf("<a href='%s?host=%s'><img src='%s%s' border=0 alt='View Service Details For This Host' title='View Service Details For This Host'></a>\n", STATUS_CGI, url_encode(temp_host->name), url_images_path, STATUS_DETAIL_ICON);
#ifdef USE_STATUSMAP
			printf("<A HREF='%s?host=%s'><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d ALT='%s' TITLE='%s'></A>", STATUSMAP_CGI, url_encode(temp_host->name), url_images_path, STATUSMAP_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, "Locate Host On Map", "Locate Host On Map");
#endif
			printf("</TD>\n");

			printf("</TR>\n");
		}
	}

	if (content_type == JSON_CONTENT)
		printf(" ] } \n");
	else
		printf("</TABLE>\n");

	return TRUE;
}


/******************************************************************/
/**********  SERVICE SORTING & FILTERING FUNCTIONS  ***************/
/******************************************************************/

/* add status data to local created status list */
int add_status_data(int status_type, void *data) {
	statusdata *new_statusdata = NULL;
	hoststatus *host_status = NULL;
	servicestatus *service_status = NULL;
	char *status_string = NULL;
	char *host_name = NULL;
	char *svc_description = NULL;
	char *plugin_output_short = NULL;
	char *plugin_output_long = NULL;
	char *plugin_output = NULL;
	char last_check[MAX_DATETIME_LENGTH];
	char state_duration[48];
	char attempts[MAX_INPUT_BUFFER];
	time_t ts_state_duration = 0L;
	time_t ts_last_check = 0L;
	time_t ts_last_state_change = 0L;
	int days;
	int hours;
	int minutes;
	int seconds;
	int duration_error = FALSE;
	int status = OK;
	int dummy = 0;
	int current_attempt = 0;
	int is_flapping = FALSE;
	int problem_has_been_acknowledged = FALSE;
	int scheduled_downtime_depth = 0;
	int notifications_enabled = FALSE;
	int checks_enabled = FALSE;
	int accept_passive_checks = FALSE;
	int state_type = HARD_STATE;

	if (status_type == HOST_STATUS) {

		host_status = (hoststatus*)data;

		if (host_status == NULL)
			return ERROR;

		if (host_status->added & STATUS_ADDED)
			return OK;

		status = host_status->status;
		if (host_status->status == HOST_PENDING)
			status_string = "PENDING";
		else if (host_status->status == HOST_UP)
			status_string = "UP";
		else if (host_status->status == HOST_DOWN)
			status_string = "DOWN";
		else if (host_status->status == HOST_UNREACHABLE)
			status_string = "UNREACHABLE";

		ts_last_check = host_status->last_check;
		ts_last_state_change = host_status->last_state_change;

		host_name = host_status->host_name;
		current_attempt = host_status->current_attempt;

		problem_has_been_acknowledged = host_status->problem_has_been_acknowledged;
		scheduled_downtime_depth = host_status->scheduled_downtime_depth;
		notifications_enabled = host_status->notifications_enabled;
		checks_enabled = host_status->checks_enabled;
		accept_passive_checks = host_status->accept_passive_host_checks;
		is_flapping = host_status->is_flapping;
		state_type = host_status->state_type;

		plugin_output_short = host_status->plugin_output;
		plugin_output_long = host_status->long_plugin_output;

		snprintf(attempts, sizeof(attempts) - 1, "%d/%d", host_status->current_attempt, host_status->max_attempts);
		attempts[sizeof(attempts) - 1] = '\x0';

	} else if (status_type == SERVICE_STATUS) {

		service_status = (servicestatus*)data;

		if (service_status == NULL)
			return ERROR;

		if (service_status->added & STATUS_ADDED)
			return OK;

		status = service_status->status;
		if (service_status->status == SERVICE_PENDING)
			status_string = "PENDING";
		else if (service_status->status == SERVICE_OK)
			status_string = "OK";
		else if (service_status->status == SERVICE_WARNING)
			status_string = "WARNING";
		else if (service_status->status == SERVICE_UNKNOWN)
			status_string = "UNKNOWN";
		else if (service_status->status == SERVICE_CRITICAL)
			status_string = "CRITICAL";

		ts_last_check = service_status->last_check;
		ts_last_state_change = service_status->last_state_change;

		host_name = service_status->host_name;
		svc_description = service_status->description;
		current_attempt = service_status->current_attempt;

		problem_has_been_acknowledged = service_status->problem_has_been_acknowledged;
		scheduled_downtime_depth = service_status->scheduled_downtime_depth;
		notifications_enabled = service_status->notifications_enabled;
		checks_enabled = service_status->checks_enabled;
		accept_passive_checks = service_status->accept_passive_service_checks;
		is_flapping = service_status->is_flapping;
		state_type = service_status->state_type;

		plugin_output_short = service_status->plugin_output;
		plugin_output_long = service_status->long_plugin_output;

		if (content_type == CSV_CONTENT || content_type == JSON_CONTENT)
			snprintf(attempts, sizeof(attempts) - 1, "%d/%d", service_status->current_attempt, service_status->max_attempts);
		else
			snprintf(attempts, sizeof(attempts) - 1, "%d/%d %s#%d%s", service_status->current_attempt, service_status->max_attempts, (service_status->status & (service_status->state_type == HARD_STATE ? add_notif_num_hard : add_notif_num_soft) ? "(" : "<!-- "), service_status->current_notification_number, (service_status->status & (service_status->state_type == HARD_STATE ? add_notif_num_hard : add_notif_num_soft) ? ")" : " -->"));
		attempts[sizeof(attempts) - 1] = '\x0';

	} else {
		return ERROR;
	}

	/* last check timestamp to string */
	get_time_string(&ts_last_check, last_check, (int)sizeof(last_check), SHORT_DATE_TIME);
	if ((unsigned long)ts_last_check == 0L)
		strcpy(last_check, "N/A");

	/* state duration calculation... */
	ts_state_duration = 0;
	duration_error = FALSE;
	if (ts_last_state_change == (time_t)0) {
		if (program_start > current_time)
			duration_error = TRUE;
		else
			ts_state_duration = current_time - program_start;
	} else {
		if (ts_last_state_change > current_time)
			duration_error = TRUE;
		else
			ts_state_duration = current_time - ts_last_state_change;
	}
	get_time_breakdown((unsigned long)ts_state_duration, &days, &hours, &minutes, &seconds);
	if (duration_error == TRUE)
		snprintf(state_duration, sizeof(state_duration) - 1, "???");
	else
		snprintf(state_duration, sizeof(state_duration) - 1, "%2dd %2dh %2dm %2ds%s", days, hours, minutes, seconds, (ts_last_state_change == (time_t)0) ? "+" : "");
	state_duration[sizeof(state_duration) - 1] = '\x0';
	strip(state_duration);

	/* plugin ouput */
	if (status_show_long_plugin_output != FALSE && plugin_output_long != NULL) {
		if (content_type == CSV_CONTENT || content_type == JSON_CONTENT) {
			if (plugin_output_short != NULL)
				dummy = asprintf(&plugin_output, "%s", escape_newlines(plugin_output_long));
			else
				dummy = asprintf(&plugin_output, "%s %s", plugin_output_short, escape_newlines(plugin_output_long));
		} else
			dummy = asprintf(&plugin_output, "%s<BR>%s", (plugin_output_short == NULL) ? "" : html_encode(plugin_output_short, TRUE), html_encode(plugin_output_long, TRUE));
	} else if (plugin_output_short != NULL) {
		if (content_type == CSV_CONTENT || content_type == JSON_CONTENT)
			dummy = asprintf(&plugin_output, "%s", plugin_output_short);
		else
			dummy = asprintf(&plugin_output, "%s&nbsp;", html_encode(plugin_output_short, TRUE));
	} else {
		if (content_type == CSV_CONTENT || content_type == JSON_CONTENT)
			plugin_output = NULL;
		else
			dummy = asprintf(&plugin_output, "&nbsp;");
	}

	/* allocating new memory */
	new_statusdata = (statusdata *)malloc(sizeof(statusdata));
	if (new_statusdata == NULL)
		return ERROR; /* maybe not good. better to return with ERROR ???? */

	new_statusdata->type = status_type;
	new_statusdata->status = status;
	new_statusdata->status_string = strdup(status_string);
	new_statusdata->host_name = strdup(host_name);
	new_statusdata->svc_description = (svc_description == NULL) ? NULL : strdup(svc_description);
	new_statusdata->state_duration = strdup(state_duration);
	new_statusdata->ts_state_duration = ts_state_duration;
	new_statusdata->last_check = strdup(last_check);
	new_statusdata->ts_last_check = ts_last_check;
	new_statusdata->attempts = strdup(attempts);

	new_statusdata->current_attempt = current_attempt;

	new_statusdata->problem_has_been_acknowledged = problem_has_been_acknowledged;
	new_statusdata->scheduled_downtime_depth = scheduled_downtime_depth;
	new_statusdata->notifications_enabled = notifications_enabled;
	new_statusdata->checks_enabled = checks_enabled;
	new_statusdata->accept_passive_checks = accept_passive_checks;
	new_statusdata->is_flapping = is_flapping;
	new_statusdata->state_type = state_type;

	new_statusdata->plugin_output = (plugin_output == NULL) ? NULL : strdup(plugin_output);

	if (statusdata_list == NULL) {
		statusdata_list = new_statusdata;
		statusdata_list->next = NULL;
		last_statusdata = statusdata_list;
	} else {
		last_statusdata->next = new_statusdata;
		last_statusdata = new_statusdata;
		last_statusdata->next = NULL;
	}

	my_free(plugin_output);

	/* count data */
	if (status_type == HOST_STATUS) {

		/* check if host triggers sound */
		if (host_status->problem_has_been_acknowledged == FALSE && \
		        (host_status->checks_enabled == TRUE || \
		         host_status->accept_passive_host_checks == TRUE) && \
		        host_status->notifications_enabled == TRUE && \
		        host_status->scheduled_downtime_depth == 0) {
			if (host_status->status == HOST_DOWN)
				problem_hosts_down++;
			if (host_status->status == HOST_UNREACHABLE)
				problem_hosts_unreachable++;
		}

		/* count host for status totals */
		if (host_status->status == HOST_DOWN)
			num_hosts_down++;
		else if (host_status->status == HOST_UNREACHABLE)
			num_hosts_unreachable++;
		else if (host_status->status == HOST_PENDING)
			num_hosts_pending++;
		else
			num_hosts_up++;

		host_status->added |= STATUS_ADDED | STATUS_COUNTED_FILTERED;
	} else {
		/* check if service triggers sound */
		if (service_status->problem_has_been_acknowledged == FALSE && \
		        (service_status->checks_enabled == TRUE || \
		         service_status->accept_passive_service_checks == TRUE) && \
		        service_status->notifications_enabled == TRUE && \
		        service_status->scheduled_downtime_depth == 0) {
			if (service_status->status == SERVICE_CRITICAL)
				problem_services_critical++;
			else if (service_status->status == SERVICE_WARNING)
				problem_services_warning++;
			else if (service_status->status == SERVICE_UNKNOWN)
				problem_services_unknown++;
		}

		if (service_status->status == SERVICE_CRITICAL)
			num_services_critical++;
		else if (service_status->status == SERVICE_WARNING)
			num_services_warning++;
		else if (service_status->status == SERVICE_UNKNOWN)
			num_services_unknown++;
		else if (service_status->status == SERVICE_PENDING)
			num_services_pending++;
		else
			num_services_ok++;

		service_status->added |= STATUS_ADDED | STATUS_COUNTED_FILTERED;
	}

	return OK;
}

/* free local created status data */
void free_local_status_data(void) {
	statusdata *this_statusdata = NULL;
	statusdata *next_statusdata = NULL;

	/* free memory for the service status list */
	for (this_statusdata = statusdata_list; this_statusdata != NULL; this_statusdata = next_statusdata) {
		next_statusdata = this_statusdata->next;
		my_free(this_statusdata->host_name);
		my_free(this_statusdata->svc_description);
		my_free(this_statusdata->status_string);
		my_free(this_statusdata->last_check);
		my_free(this_statusdata->state_duration);
		my_free(this_statusdata->attempts);
		my_free(this_statusdata->plugin_output);
		my_free(this_statusdata);
	}

	statusdata_list = NULL;

	return;
}

/* sorts the service list */
int sort_status_data(int status_type, int sort_type, int sort_option) {
	sort *new_sort;
	sort *last_sort;
	sort *temp_sort;
	statusdata *temp_status = NULL;

	if (sort_type == SORT_NONE)
		return ERROR;

	if (statusdata_list == NULL)
		return ERROR;

	for (temp_status = statusdata_list; temp_status != NULL; temp_status = temp_status->next) {

		if (temp_status->type != status_type)
			continue;

		/* allocate memory for a new sort structure */
		new_sort = (sort *)malloc(sizeof(sort));
		if (new_sort == NULL)
			return ERROR;

		new_sort->status = temp_status;

		last_sort = statussort_list;
		for (temp_sort = statussort_list; temp_sort != NULL; temp_sort = temp_sort->next) {

			if (compare_sort_entries(status_type, sort_type, sort_option, new_sort, temp_sort) == TRUE) {
				new_sort->next = temp_sort;
				if (temp_sort == statussort_list)
					statussort_list = new_sort;
				else
					last_sort->next = new_sort;
				break;
			} else
				last_sort = temp_sort;
		}

		if (statussort_list == NULL) {
			new_sort->next = NULL;
			statussort_list = new_sort;
		} else if (temp_sort == NULL) {
			new_sort->next = NULL;
			last_sort->next = new_sort;
		}
	}

	return OK;
}

/* compare status data for sorting */
int compare_sort_entries(int status_type, int sort_type, int sort_option, sort *new_sort, sort *temp_sort) {
	statusdata *new_status;
	statusdata *temp_status;

	new_status = new_sort->status;
	temp_status = temp_sort->status;

	if (sort_type == SORT_ASCENDING) {

		if (sort_option == SORT_LASTCHECKTIME) {
			if (new_status->ts_last_check < temp_status->ts_last_check)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_CURRENTATTEMPT) {
			if (new_status->current_attempt < temp_status->current_attempt)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_SERVICESTATUS && status_type == SERVICE_STATUS) {
			if (new_status->status <= temp_status->status)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_HOSTURGENCY) {
			if (HOST_URGENCY(new_status->status) <= HOST_URGENCY(temp_status->status))
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_HOSTNAME) {
			if (strcasecmp(new_status->host_name, temp_status->host_name) < 0)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_HOSTSTATUS && status_type == HOST_STATUS) {
			if (new_status->status <= temp_status->status)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_SERVICENAME && status_type == SERVICE_STATUS) {
			if (strcasecmp(new_status->svc_description, temp_status->svc_description) < 0)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_STATEDURATION) {
			if (new_status->ts_state_duration < temp_status->ts_state_duration)
				return TRUE;
			else
				return FALSE;
		}
	} else {
		if (sort_option == SORT_LASTCHECKTIME) {
			if (new_status->ts_last_check > temp_status->ts_last_check)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_CURRENTATTEMPT) {
			if (new_status->current_attempt > temp_status->current_attempt)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_SERVICESTATUS && status_type == SERVICE_STATUS) {
			if (new_status->status > temp_status->status)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_HOSTURGENCY) {
			if (HOST_URGENCY(new_status->status) > HOST_URGENCY(temp_status->status))
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_HOSTNAME) {
			if (strcasecmp(new_status->host_name, temp_status->host_name) > 0)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_HOSTSTATUS && status_type == HOST_STATUS) {
			if (new_status->status > temp_status->status)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_SERVICENAME && status_type == SERVICE_STATUS) {
			if (strcasecmp(new_status->svc_description, temp_status->svc_description) > 0)
				return TRUE;
			else
				return FALSE;
		} else if (sort_option == SORT_STATEDURATION) {
			if (new_status->ts_state_duration > temp_status->ts_state_duration)
				return TRUE;
			else
				return FALSE;
		}
	}

	return TRUE;
}

/* free list of sorted items */
void free_sort_list(void) {
	sort *this_sort;
	sort *next_sort;

	/* free memory for sort list */
	for (this_sort = statussort_list; this_sort != NULL; this_sort = next_sort) {
		next_sort = this_sort->next;
		free(this_sort);
	}

	return;
}

/* check host properties filter */
int passes_host_properties_filter(hoststatus *temp_hoststatus) {

	if ((host_properties & HOST_SCHEDULED_DOWNTIME) && temp_hoststatus->scheduled_downtime_depth <= 0)
		return FALSE;

	if ((host_properties & HOST_NO_SCHEDULED_DOWNTIME) && temp_hoststatus->scheduled_downtime_depth > 0)
		return FALSE;

	if ((host_properties & HOST_STATE_ACKNOWLEDGED) && temp_hoststatus->problem_has_been_acknowledged == FALSE)
		return FALSE;

	if ((host_properties & HOST_STATE_UNACKNOWLEDGED) && temp_hoststatus->problem_has_been_acknowledged == TRUE)
		return FALSE;

	if ((host_properties & HOST_CHECKS_DISABLED) && temp_hoststatus->checks_enabled == TRUE)
		return FALSE;

	if ((host_properties & HOST_CHECKS_ENABLED) && temp_hoststatus->checks_enabled == FALSE)
		return FALSE;

	if ((host_properties & HOST_EVENT_HANDLER_DISABLED) && temp_hoststatus->event_handler_enabled == TRUE)
		return FALSE;

	if ((host_properties & HOST_EVENT_HANDLER_ENABLED) && temp_hoststatus->event_handler_enabled == FALSE)
		return FALSE;

	if ((host_properties & HOST_FLAP_DETECTION_DISABLED) && temp_hoststatus->flap_detection_enabled == TRUE)
		return FALSE;

	if ((host_properties & HOST_FLAP_DETECTION_ENABLED) && temp_hoststatus->flap_detection_enabled == FALSE)
		return FALSE;

	if ((host_properties & HOST_IS_FLAPPING) && temp_hoststatus->is_flapping == FALSE)
		return FALSE;

	if ((host_properties & HOST_IS_NOT_FLAPPING) && temp_hoststatus->is_flapping == TRUE)
		return FALSE;

	if ((host_properties & HOST_NOTIFICATIONS_DISABLED) && temp_hoststatus->notifications_enabled == TRUE)
		return FALSE;

	if ((host_properties & HOST_NOTIFICATIONS_ENABLED) && temp_hoststatus->notifications_enabled == FALSE)
		return FALSE;

	if ((host_properties & HOST_PASSIVE_CHECKS_DISABLED) && temp_hoststatus->accept_passive_host_checks == TRUE)
		return FALSE;

	if ((host_properties & HOST_PASSIVE_CHECKS_ENABLED) && temp_hoststatus->accept_passive_host_checks == FALSE)
		return FALSE;

	if ((host_properties & HOST_PASSIVE_CHECK) && temp_hoststatus->check_type == HOST_CHECK_ACTIVE)
		return FALSE;

	if ((host_properties & HOST_ACTIVE_CHECK) && temp_hoststatus->check_type == HOST_CHECK_PASSIVE)
		return FALSE;

	if ((host_properties & HOST_HARD_STATE) && temp_hoststatus->state_type == SOFT_STATE)
		return FALSE;

	if ((host_properties & HOST_SOFT_STATE) && temp_hoststatus->state_type == HARD_STATE)
		return FALSE;

	if ((host_properties & HOST_NOT_ALL_CHECKS_DISABLED) && temp_hoststatus->checks_enabled == FALSE && temp_hoststatus->accept_passive_host_checks == FALSE)
		return FALSE;

	if ((host_properties & HOST_STATE_HANDLED) && (temp_hoststatus->scheduled_downtime_depth <= 0 && (temp_hoststatus->checks_enabled == TRUE || temp_hoststatus->accept_passive_host_checks == TRUE)))
		return FALSE;

	return TRUE;
}

/* check service properties filter */
int passes_service_properties_filter(servicestatus *temp_servicestatus) {
	hoststatus *temp_hoststatus = NULL;

	if ((service_properties & SERVICE_SCHEDULED_DOWNTIME) && temp_servicestatus->scheduled_downtime_depth <= 0)
		return FALSE;

	if ((service_properties & SERVICE_NO_SCHEDULED_DOWNTIME) && temp_servicestatus->scheduled_downtime_depth > 0)
		return FALSE;

	if ((service_properties & SERVICE_STATE_ACKNOWLEDGED) && temp_servicestatus->problem_has_been_acknowledged == FALSE)
		return FALSE;

	if ((service_properties & SERVICE_STATE_UNACKNOWLEDGED) && temp_servicestatus->problem_has_been_acknowledged == TRUE)
		return FALSE;

	if ((service_properties & SERVICE_CHECKS_DISABLED) && temp_servicestatus->checks_enabled == TRUE)
		return FALSE;

	if ((service_properties & SERVICE_CHECKS_ENABLED) && temp_servicestatus->checks_enabled == FALSE)
		return FALSE;

	if ((service_properties & SERVICE_EVENT_HANDLER_DISABLED) && temp_servicestatus->event_handler_enabled == TRUE)
		return FALSE;

	if ((service_properties & SERVICE_EVENT_HANDLER_ENABLED) && temp_servicestatus->event_handler_enabled == FALSE)
		return FALSE;

	if ((service_properties & SERVICE_FLAP_DETECTION_DISABLED) && temp_servicestatus->flap_detection_enabled == TRUE)
		return FALSE;

	if ((service_properties & SERVICE_FLAP_DETECTION_ENABLED) && temp_servicestatus->flap_detection_enabled == FALSE)
		return FALSE;

	if ((service_properties & SERVICE_IS_FLAPPING) && temp_servicestatus->is_flapping == FALSE)
		return FALSE;

	if ((service_properties & SERVICE_IS_NOT_FLAPPING) && temp_servicestatus->is_flapping == TRUE)
		return FALSE;

	if ((service_properties & SERVICE_NOTIFICATIONS_DISABLED) && temp_servicestatus->notifications_enabled == TRUE)
		return FALSE;

	if ((service_properties & SERVICE_NOTIFICATIONS_ENABLED) && temp_servicestatus->notifications_enabled == FALSE)
		return FALSE;

	if ((service_properties & SERVICE_PASSIVE_CHECKS_DISABLED) && temp_servicestatus->accept_passive_service_checks == TRUE)
		return FALSE;

	if ((service_properties & SERVICE_PASSIVE_CHECKS_ENABLED) && temp_servicestatus->accept_passive_service_checks == FALSE)
		return FALSE;

	if ((service_properties & SERVICE_PASSIVE_CHECK) && temp_servicestatus->check_type == SERVICE_CHECK_ACTIVE)
		return FALSE;

	if ((service_properties & SERVICE_ACTIVE_CHECK) && temp_servicestatus->check_type == SERVICE_CHECK_PASSIVE)
		return FALSE;

	if ((service_properties & SERVICE_HARD_STATE) && temp_servicestatus->state_type == SOFT_STATE)
		return FALSE;

	if ((service_properties & SERVICE_SOFT_STATE) && temp_servicestatus->state_type == HARD_STATE)
		return FALSE;

	if ((service_properties & SERVICE_NOT_ALL_CHECKS_DISABLED) && temp_servicestatus->checks_enabled == FALSE && temp_servicestatus->accept_passive_service_checks == FALSE)
		return FALSE;

	if (service_properties & SERVICE_STATE_HANDLED) {
		if (temp_servicestatus->scheduled_downtime_depth > 0)
			return TRUE;
		if (temp_servicestatus->checks_enabled == FALSE && temp_servicestatus->accept_passive_service_checks == FALSE)
			return TRUE;
		temp_hoststatus = find_hoststatus(temp_servicestatus->host_name);
		if (temp_hoststatus != NULL && (temp_hoststatus->status == HOST_DOWN || temp_hoststatus->status == HOST_UNREACHABLE))
			return TRUE;
		return FALSE;
	}

	return TRUE;
}

/* shows service and host filters in use */
void show_filters(void) {
	int found = 0;

	/* show filters box if necessary */
	if (host_properties != 0L || service_properties != 0L || host_status_types != all_host_status_types || service_status_types != all_service_status_types) {

		printf("<table border=1 class='filter' cellspacing=0 cellpadding=0>\n");
		printf("<tr><td valign=top align=left CLASS='filterTitle'>Display Filters:&nbsp;");
		printf("<img id='expand_image' src='%s%s' border=0 onClick=\"if (document.getElementById('filters').style.display == 'none') { document.getElementById('filters').style.display = ''; document.getElementById('expand_image').src = '%s%s'; } else { document.getElementById('filters').style.display = 'none'; document.getElementById('expand_image').src = '%s%s'; }\">", url_images_path, EXPAND_ICON, url_images_path, COLLAPSE_ICON, url_images_path, EXPAND_ICON);
		printf("</td></tr>");
		printf("<tr><td><table id='filters' border=0 cellspacing=2 cellpadding=0 style='display:none;'>\n");
		printf("<tr><td valign=top align=left CLASS='filterName'>Host Status Types:</td>");
		printf("<td valign=top align=left CLASS='filterValue'>");
		if (host_status_types == all_host_status_types)
			printf("All");
		else if (host_status_types == all_host_problems)
			printf("All problems");
		else {
			found = 0;
			if (host_status_types & HOST_PENDING) {
				printf(" Pending");
				found = 1;
			}
			if (host_status_types & HOST_UP) {
				printf("%s Up", (found == 1) ? " |" : "");
				found = 1;
			}
			if (host_status_types & HOST_DOWN) {
				printf("%s Down", (found == 1) ? " |" : "");
				found = 1;
			}
			if (host_status_types & HOST_UNREACHABLE)
				printf("%s Unreachable", (found == 1) ? " |" : "");
		}
		printf("</td></tr>");
		printf("<tr><td valign=top align=left CLASS='filterName'>Host Properties:</td>");
		printf("<td valign=top align=left CLASS='filterValue'>");
		if (host_properties == 0)
			printf("Any");
		else {
			found = 0;
			if (host_properties & HOST_SCHEDULED_DOWNTIME) {
				printf(" In Scheduled Downtime");
				found = 1;
			}
			if (host_properties & HOST_NO_SCHEDULED_DOWNTIME) {
				printf("%s Not In Scheduled Downtime", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_STATE_ACKNOWLEDGED) {
				printf("%s Has Been Acknowledged", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_STATE_UNACKNOWLEDGED) {
				printf("%s Has Not Been Acknowledged", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_CHECKS_DISABLED) {
				printf("%s Checks Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_CHECKS_ENABLED) {
				printf("%s Checks Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_EVENT_HANDLER_DISABLED) {
				printf("%s Event Handler Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_EVENT_HANDLER_ENABLED) {
				printf("%s Event Handler Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_FLAP_DETECTION_DISABLED) {
				printf("%s Flap Detection Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_FLAP_DETECTION_ENABLED) {
				printf("%s Flap Detection Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_IS_FLAPPING) {
				printf("%s Is Flapping", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_IS_NOT_FLAPPING) {
				printf("%s Is Not Flapping", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_NOTIFICATIONS_DISABLED) {
				printf("%s Notifications Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_NOTIFICATIONS_ENABLED) {
				printf("%s Notifications Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_PASSIVE_CHECKS_DISABLED) {
				printf("%s Passive Checks Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_PASSIVE_CHECKS_ENABLED) {
				printf("%s Passive Checks Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_PASSIVE_CHECK) {
				printf("%s Passive Checks", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_ACTIVE_CHECK) {
				printf("%s Active Checks", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_HARD_STATE) {
				printf("%s In Hard State", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_SOFT_STATE) {
				printf("%s In Soft State", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_STATE_HANDLED) {
				printf("%s Problem Handled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (host_properties & HOST_NOT_ALL_CHECKS_DISABLED) {
				printf("%s Not All Checks Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
		}
		printf("</td>");
		printf("</tr>\n");


		printf("<tr><td valign=top align=left CLASS='filterName'>Service Status Types:</td>");
		printf("<td valign=top align=left CLASS='filterValue'>");
		if (service_status_types == all_service_status_types)
			printf("All");
		else if (service_status_types == all_service_problems)
			printf("All Problems");
		else {
			found = 0;
			if (service_status_types & SERVICE_PENDING) {
				printf(" Pending");
				found = 1;
			}
			if (service_status_types & SERVICE_OK) {
				printf("%s Ok", (found == 1) ? " |" : "");
				found = 1;
			}
			if (service_status_types & SERVICE_UNKNOWN) {
				printf("%s Unknown", (found == 1) ? " |" : "");
				found = 1;
			}
			if (service_status_types & SERVICE_WARNING) {
				printf("%s Warning", (found == 1) ? " |" : "");
				found = 1;
			}
			if (service_status_types & SERVICE_CRITICAL) {
				printf("%s Critical", (found == 1) ? " |" : "");
				found = 1;
			}
		}
		printf("</td></tr>");
		printf("<tr><td valign=top align=left CLASS='filterName'>Service Properties:</td>");
		printf("<td valign=top align=left CLASS='filterValue'>");
		if (service_properties == 0)
			printf("Any");
		else {
			found = 0;
			if (service_properties & SERVICE_SCHEDULED_DOWNTIME) {
				printf(" In Scheduled Downtime");
				found = 1;
			}
			if (service_properties & SERVICE_NO_SCHEDULED_DOWNTIME) {
				printf("%s Not In Scheduled Downtime", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_STATE_ACKNOWLEDGED) {
				printf("%s Has Been Acknowledged", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_STATE_UNACKNOWLEDGED) {
				printf("%s Has Not Been Acknowledged", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_CHECKS_DISABLED) {
				printf("%s Active Checks Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_CHECKS_ENABLED) {
				printf("%s Active Checks Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_EVENT_HANDLER_DISABLED) {
				printf("%s Event Handler Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_EVENT_HANDLER_ENABLED) {
				printf("%s Event Handler Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_FLAP_DETECTION_DISABLED) {
				printf("%s Flap Detection Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_FLAP_DETECTION_ENABLED) {
				printf("%s Flap Detection Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_IS_FLAPPING) {
				printf("%s Is Flapping", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_IS_NOT_FLAPPING) {
				printf("%s Is Not Flapping", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_NOTIFICATIONS_DISABLED) {
				printf("%s Notifications Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_NOTIFICATIONS_ENABLED) {
				printf("%s Notifications Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_PASSIVE_CHECKS_DISABLED) {
				printf("%s Passive Checks Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_PASSIVE_CHECKS_ENABLED) {
				printf("%s Passive Checks Enabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_PASSIVE_CHECK) {
				printf("%s Passive Checks", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_ACTIVE_CHECK) {
				printf("%s Active Checks", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_HARD_STATE) {
				printf("%s In Hard State", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_SOFT_STATE) {
				printf("%s In Soft State", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_STATE_HANDLED) {
				printf("%s Problem Handled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
			if (service_properties & SERVICE_NOT_ALL_CHECKS_DISABLED) {
				printf("%s Not All Checks Disabled", (found == 1) ? " &amp;" : "");
				found = 1;
			}
		}
		printf("</td></tr>");
		printf("</table>\n");

		printf("</td></tr>");
		printf("</table>\n");
	}

	return;
}

/******************************************************************/
/*********  DROPDOWN MENU's FOR DETAILED VIEWS ;-)  ***************/
/******************************************************************/

/* Display a table with the commands for checked checkboxes, for services */
void show_servicecommand_table(void) {
	if (is_authorized_for_read_only(&current_authdata) == FALSE) {
		/* A new div for the command table */
		printf("<DIV CLASS='serviceTotalsCommands'>Commands for checked services</DIV>\n");
		/* DropDown menu */
		printf("<select style='display:none;width:400px' name='cmd_typ' id='cmd_typ_service' onchange='showValue(\"tableformservice\",this.value,%d,%d)' CLASS='DropDownService'>\n", CMD_SCHEDULE_HOST_CHECK, CMD_SCHEDULE_SVC_CHECK);
		printf("<option value='nothing'>Select command</option>\n");
		printf("<option value='%d' title='%s%s' >Add a Comment to Checked Service(s)</option>\n", CMD_ADD_SVC_COMMENT, url_images_path, COMMENT_ICON);
		printf("<option value='%d' title='%s%s'>Disable Active Checks Of Checked Service(s)</option>\n", CMD_DISABLE_SVC_CHECK, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s'>Enable Active Checks Of Checked Service(s)</option>\n", CMD_ENABLE_SVC_CHECK, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s'>Re-schedule Next Service Check</option>\n", CMD_SCHEDULE_SVC_CHECK, url_images_path, DELAY_ICON);
		printf("<option value='%d' title='%s%s'>Submit Passive Check Result For Checked Service(s)</option>\n", CMD_PROCESS_SERVICE_CHECK_RESULT, url_images_path, PASSIVE_ICON);
		printf("<option value='%d' title='%s%s'>Stop Accepting Passive Checks For Checked Service(s)</option>\n", CMD_DISABLE_PASSIVE_SVC_CHECKS, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s'>Start Accepting Passive Checks For Checked Service(s)</option>\n", CMD_ENABLE_PASSIVE_SVC_CHECKS, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s'>Stop Obsessing Over Checked Service(s)</option>\n", CMD_STOP_OBSESSING_OVER_SVC, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s'>Start Obsessing Over Checked Service(s)</option>\n", CMD_START_OBSESSING_OVER_SVC, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s'>Acknowledge Checked Service(s) Problem</option>\n", CMD_ACKNOWLEDGE_SVC_PROBLEM, url_images_path, ACKNOWLEDGEMENT_ICON);
		printf("<option value='%d' title='%s%s'>Remove Problem Acknowledgement for Checked Service(s)</option>\n", CMD_REMOVE_SVC_ACKNOWLEDGEMENT, url_images_path, REMOVE_ACKNOWLEDGEMENT_ICON);
		printf("<option value='%d' title='%s%s'>Disable Notifications For Checked Service(s)</option>\n", CMD_DISABLE_SVC_NOTIFICATIONS, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s'>Enable Notifications For Checked Service(s)</option>\n", CMD_ENABLE_SVC_NOTIFICATIONS, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s'>Send Custom Notification For Checked Service(s)</option>\n", CMD_SEND_CUSTOM_SVC_NOTIFICATION, url_images_path, NOTIFICATION_ICON);
		printf("<option value='%d' title='%s%s'>Delay Next Notification For Checked Service(s)</option>\n", CMD_DELAY_SVC_NOTIFICATION, url_images_path, DELAY_ICON);
		printf("<option value='%d' title='%s%s'>Schedule Downtime For Checked Service(s)</option>\n", CMD_SCHEDULE_SVC_DOWNTIME, url_images_path, DOWNTIME_ICON);
		printf("<option value='%d' title='%s%s'>Disable Event Handler For Checked Service(s)</option>\n", CMD_DISABLE_SVC_EVENT_HANDLER, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s'>Enable Event Handler For Checked Service(s)</option>\n", CMD_ENABLE_SVC_EVENT_HANDLER, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s'>Disable Flap Detection For Checked Service(s)</option>\n", CMD_DISABLE_SVC_FLAP_DETECTION, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s'>Enable Flap Detection For Checked Service(s)</option>\n", CMD_ENABLE_SVC_FLAP_DETECTION, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s'>Reset Modified Attributes For Checked Service(s)</option>\n", CMD_CHANGE_SVC_MODATTR, url_images_path, DISABLED_ICON);
		printf("</select>\n");

		/* Print out the activator for the dropdown (which must be between the body tags */
		printf("<script language='javascript' type=\"text/javascript\">\n");
		printf("$(document).ready(function() { \n");
		printf("document.tableformservice.cmd_typ.selectedIndex = 0;\n");
		printf("document.tableformservice.cmd_typ.options[0].selected = true;\n");
		printf("document.tableformservice.buttonValidChoice.value = 'false';\n");
		printf("document.tableformservice.buttonCheckboxChecked.value = 'false';\n");
		printf("checked = true;\n");
		printf("checkAll(\"tableformservice\");\n");
		printf("checked = false;\n");
		printf("try { \n$(\".DropDownService\").msDropDown({visibleRows:25}).data(\"dd\").visible(true);\n");
		printf("} catch(e) {\n");
		printf("if (console) { console.log(e); }\n}\n");
		printf("});\n");
		printf("</script>\n");

		printf("<br><br><b><input type='submit' name='CommandButton' value='Submit' class='serviceTotalsCommands' disabled='disabled'></b>\n");
	}
}

/* Display a table with the commands for checked checkboxes, for hosts */
void show_hostcommand_table(void) {
	if (is_authorized_for_read_only(&current_authdata) == FALSE) {
		/* A new div for the command table */
		printf("<DIV CLASS='hostTotalsCommands'>Commands for checked host(s)</DIV>\n");
		/* DropDown menu */
		printf("<select style='display:none;width:400px' name='cmd_typ' id='cmd_typ_host' onchange='showValue(\"tableformhost\",this.value,%d,%d)' CLASS='DropDownHost'>\n", CMD_SCHEDULE_HOST_CHECK, CMD_SCHEDULE_SVC_CHECK);
		printf("<option value='nothing'>Select command</option>\n");
		printf("<option value='%d' title='%s%s' >Add a Comment to Checked Host(s)</option>\n", CMD_ADD_HOST_COMMENT, url_images_path, COMMENT_ICON);
		printf("<option value='%d' title='%s%s' >Disable Active Checks Of Checked Host(s)</option>\n", CMD_DISABLE_HOST_CHECK, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s' >Enable Active Checks Of Checked Host(s)</option>\n", CMD_ENABLE_HOST_CHECK, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s' >Re-schedule Next Host Check</option>\n", CMD_SCHEDULE_HOST_CHECK, url_images_path, DELAY_ICON);
		printf("<option value='%d' title='%s%s' >Submit Passive Check Result For Checked Host(s)</option>\n", CMD_PROCESS_HOST_CHECK_RESULT, url_images_path, PASSIVE_ICON);
		printf("<option value='%d' title='%s%s' >Stop Accepting Passive Checks For Checked Host(s)</option>\n", CMD_DISABLE_PASSIVE_HOST_CHECKS, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s' >Start Accepting Passive Checks For Checked Host(s)</option>\n", CMD_ENABLE_PASSIVE_HOST_CHECKS, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s' >Stop Obsessing Over Checked Host(s)</option>\n", CMD_STOP_OBSESSING_OVER_HOST, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s' >Start Obsessing Over Checked Host(s)</option>\n", CMD_START_OBSESSING_OVER_HOST, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s' >Acknowledge Checked Host(s) Problem</option>\n", CMD_ACKNOWLEDGE_HOST_PROBLEM, url_images_path, ACKNOWLEDGEMENT_ICON);
		printf("<option value='%d' title='%s%s' >Remove Problem Acknowledgement</option>\n", CMD_REMOVE_HOST_ACKNOWLEDGEMENT, url_images_path, REMOVE_ACKNOWLEDGEMENT_ICON);
		printf("<option value='%d' title='%s%s' >Disable Notifications For Checked Host(s)</option>\n", CMD_DISABLE_HOST_NOTIFICATIONS, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s' >Enable Notifications For Checked Host(s)</option>\n", CMD_ENABLE_HOST_NOTIFICATIONS, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s' >Send Custom Notification</option>\n", CMD_SEND_CUSTOM_HOST_NOTIFICATION, url_images_path, NOTIFICATION_ICON);
		printf("<option value='%d' title='%s%s' >Delay Next Host Notification</option>\n", CMD_DELAY_HOST_NOTIFICATION, url_images_path, DELAY_ICON);
		printf("<option value='%d' title='%s%s' >Schedule Downtime For Checked Host(s)</option>\n", CMD_SCHEDULE_HOST_DOWNTIME, url_images_path, DOWNTIME_ICON);
		printf("<option value='%d' title='%s%s' >Schedule Downtime For Checked Host(s) and All Services</option>\n", CMD_SCHEDULE_HOST_SVC_DOWNTIME, url_images_path, DOWNTIME_ICON);
		printf("<option value='%d' title='%s%s' >Remove Downtime(s) For Checked Host(s) and All Services</option>\n", CMD_DEL_DOWNTIME_BY_HOST_NAME, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s' >Disable Notifications For All Services On Checked Host(s)</option>\n", CMD_DISABLE_HOST_SVC_NOTIFICATIONS, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s' >Enable Notifications For All Services On Checked Host(s)</option>\n", CMD_ENABLE_HOST_SVC_NOTIFICATIONS, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s' >Schedule A Check Of All Services On Checked Host(s)</option>\n", CMD_SCHEDULE_HOST_SVC_CHECKS, url_images_path, DELAY_ICON);
		printf("<option value='%d' title='%s%s' >Disable Checks Of All Services On Checked Host(s)</option>\n", CMD_DISABLE_HOST_SVC_CHECKS, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s' >Enable Checks Of All Services On Checked Host(s)</option>\n", CMD_ENABLE_HOST_SVC_CHECKS, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s' >Disable Event Handler For Checked Host(s)</option>\n", CMD_DISABLE_HOST_EVENT_HANDLER, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s' >Enable Event Handler For Checked Host(s)</option>\n", CMD_ENABLE_HOST_EVENT_HANDLER, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s' >Disable Flap Detection For Checked Host(s)</option>\n", CMD_DISABLE_HOST_FLAP_DETECTION, url_images_path, DISABLED_ICON);
		printf("<option value='%d' title='%s%s' >Enable Flap Detection For Checked Host(s)</option>\n", CMD_ENABLE_HOST_FLAP_DETECTION, url_images_path, ENABLED_ICON);
		printf("<option value='%d' title='%s%s' >Reset Modified Attributes For Checked Host(s)</option>\n", CMD_CHANGE_HOST_MODATTR, url_images_path, DISABLED_ICON);
		printf("</select>\n");

		/* Print out the activator for the dropdown (which must be between the body tags */
		printf("<script language='javascript' type=\"text/javascript\">\n");
		printf("$(document).ready(function() { \n");
		printf("document.tableformhost.cmd_typ.selectedIndex = 0;\n");
		printf("document.tableformhost.cmd_typ.options[0].selected = true;\n");
		printf("document.tableformhost.buttonValidChoice.value = 'false';\n");
		printf("document.tableformhost.buttonCheckboxChecked.value = 'false';\n");
		printf("checked = true;\n");
		printf("checkAll(\"tableformhost\");\n");
		printf("checked = false;\n");
		printf("try { \n$(\".DropDownHost\").msDropDown({visibleRows:27}).data(\"dd\").visible(true);\n");
		printf("} catch(e) {\n");
		printf("if (console) { console.log(e); }\n}\n");
		printf("});\n");
		printf("</script>\n");

		printf("<br><br><b><input type='submit' name='CommandButton' value='Submit' class='hostsTotalsCommands' disabled='disabled'></b>\n");
	}
}
/* The cake is a lie! */


/******************************************************************/
/*************  print name for displayed list *********************/
/******************************************************************/
void print_displayed_names(int style) {
	int i = 0;
	int saved_escape_html_tags_var = FALSE;

	if (style == DISPLAY_HOSTS) {
		if (search_string != NULL) {
			saved_escape_html_tags_var = escape_html_tags;
			escape_html_tags = TRUE;
			printf("Host/Services matching '%s'", (content_type == HTML_CONTENT) ? html_encode(search_string, FALSE) : search_string);
			escape_html_tags = saved_escape_html_tags_var;
		} else if (show_all_hosts == TRUE)
			printf("All Hosts");
		else {
			if (num_req_hosts == 1)
				printf("Host '%s'", html_encode(req_hosts[0].entry, TRUE));
			else {
				printf("Hosts ");
				for (i = 0; req_hosts[i].entry != NULL; i++) {
					if (i == 3) {
						printf(", ...");
						break;
					}
					if (i != 0) {
						((num_req_hosts - i) == 1) ? printf(" and ") : printf(", ");
					}
					printf("'%s'", html_encode(req_hosts[i].entry, TRUE));
				}
			}
		}
	} else if (style == DISPLAY_SERVICEGROUPS) {
		if (show_all_servicegroups == TRUE)
			printf("All Service Groups");
		else {
			if (num_req_servicegroups == 1)
				printf("Service Group '%s'", html_encode(req_servicegroups[0].entry, TRUE));
			else {
				printf("Service Groups ");
				for (i = 0; req_servicegroups[i].entry != NULL; i++) {
					if (i == 3) {
						printf(", ...");
						break;
					}
					if (i != 0) {
						((num_req_servicegroups - i) == 1) ? printf(" and ") : printf(", ");
					}
					printf("'%s'", html_encode(req_servicegroups[i].entry, TRUE));
				}
			}
		}
	} else if (style == DISPLAY_HOSTGROUPS) {
		if (show_all_hostgroups == TRUE) {
			printf("All Host Groups");
			if (show_partial_hostgroups == TRUE)
				printf("<br>(Partial Hostgroups Enabled)");
		} else {
			if (num_req_hostgroups == 1)
				printf("Host Group '%s'", html_encode(req_hostgroups[0].entry, TRUE));
			else {
				printf("Host Groups ");
				for (i = 0; req_hostgroups[i].entry != NULL; i++) {
					if (i == 3) {
						printf(", ...");
						break;
					}
					if (i != 0) {
						((num_req_hostgroups - i) == 1) ? printf(" and ") : printf(", ");
					}
					printf("'%s'", html_encode(req_hostgroups[i].entry, TRUE));
				}
			}
		}
	}

	return;
}


/******************************************************************/
/*******************  pagination functions ************************/
/******************************************************************/

void status_page_num_selector(int local_result_start, int status_type) {

	char link[MAX_INPUT_BUFFER] = "";
	char stripped_query_string[MAX_INPUT_BUFFER] = "";
	char *temp_buffer;
	int total_pages = 1;
	int current_page = 1;
	int next_page = 0;
	int previous_page = 0;
	int display_total = 0;
	int display_from = 0;
	int display_to = 0;

	/* define base url */
	strcat(link, STATUS_CGI);

	/* get url options but filter out "limit" and "status" */
	if (getenv("QUERY_STRING") != NULL && strcmp(getenv("QUERY_STRING"), "")) {
		strcpy(stripped_query_string, getenv("QUERY_STRING"));
		strip_html_brackets(stripped_query_string);

		for (temp_buffer = my_strtok(stripped_query_string, "&"); temp_buffer != NULL; temp_buffer = my_strtok(NULL, "&")) {
			if (strncmp(temp_buffer, "limit=", 6) != 0 && strncmp(temp_buffer, "start=", 6) != 0) {
				if (strstr(link, "?"))
					strcat(link, "&");
				else
					strcat(link, "?");
				strcat(link, temp_buffer);
			}
		}
	}

	/* calculate pages */
	if (result_limit > 0 && (total_host_entries + total_service_entries) > 0) {
		total_pages = ((total_host_entries + total_service_entries) / result_limit);

		if (((total_host_entries + total_service_entries) % result_limit) != 0)
			total_pages++;

		current_page = (result_start / result_limit) + 1;
		previous_page = (result_start - result_limit) > 0 ? (result_start - result_limit) : 0;
		next_page = (result_start + result_limit) > (total_host_entries + total_service_entries) ? result_start : (result_start + result_limit);
	}

	/* links page select elements and counters */
	if ((group_style_type == STYLE_HOST_SERVICE_DETAIL && status_type == SERVICE_STATUS) || group_style_type != STYLE_HOST_SERVICE_DETAIL) {

		printf("<div class='page_selector'>\n");
		printf("<div id='page_navigation' class='page_select_dd'>");

		if (current_page != 1 || (result_limit != 0 && result_start != 1))
			printf("<a href='%s%sstart=1&limit=%d' title='First Page'><img src='%s%s' style='vertical-align: middle;' height='16' width='16' alt='<<' /></a>\n", link, (strstr(link, "?")) ? "&" : "?", result_limit, url_images_path, FIRST_PAGE_ACTIVE_ICON);
		else
			printf("<img src='%s%s' style='vertical-align: middle;' height='16' width='16' />\n", url_images_path, FIRST_PAGE_INACTIVE_ICON);

		if (current_page != 1)
			printf("<a href='%s%sstart=%d&limit=%d' title='Previous Page'><img src='%s%s' style='vertical-align: middle;' height='16' width='16' alt='<' /></a>\n", link, (strstr(link, "?")) ? "&" : "?", previous_page, result_limit, url_images_path, PREVIOUS_PAGE_ACTIVE_ICON);
		else
			printf("<img src='%s%s' style='vertical-align: middle;' height='16' width='16' />\n", url_images_path, PREVIOUS_PAGE_INACTIVE_ICON);

		printf("<span style='vertical-align:middle; font-size:8pt;'> Page </span>");

		/* with inline javascript to send new page on "Enter" */
		printf("<input type='text' value='%d' style='width:30px; vertical-align:middle; border:1px #D0D0D0 solid;text-align:center; font-size:8pt;'", current_page);
		printf("onkeydown='if (event.keyCode == 13) window.location.href = \"%s\" + \"%slimit=%d&start=\" + (((this.value -1) * %d) + 1);'>", link, (strstr(link, "?")) ? "&" : "?", result_limit, result_limit);

		printf("<span style='vertical-align:middle; font-size:8pt;'> of %d </span>", total_pages);

		if (current_page != total_pages) {
			printf("<a href='%s%sstart=%d&limit=%d' title='Next Page'><img src='%s%s' style='vertical-align: middle;' height='16' width='16' alt='>' /></a>\n", link, (strstr(link, "?")) ? "&" : "?", (result_start + result_limit), result_limit, url_images_path, NEXT_PAGE_ACTIVE_ICON);
			printf("<a href='%s%sstart=%d&limit=%d' title='Last Page'><img src='%s%s' style='vertical-align: middle;' height='16' width='16' alt='>>' /></a>\n", link, (strstr(link, "?")) ? "&" : "?", ((total_pages - 1)*result_limit) + 1, result_limit, url_images_path, LAST_PAGE_ACTIVE_ICON);
		} else
			printf("<img src='%s%s' style='vertical-align: middle;' height='16' width='16' /><img src='%s%s' style='vertical-align: middle;' height='16' width='16' />\n", url_images_path, NEXT_PAGE_INACTIVE_ICON, url_images_path, LAST_PAGE_INACTIVE_ICON);

		printf("</div>\n");
		page_limit_selector(result_start);
		printf("</div>\n");
	}

	/* calculating the displayed reults */
	if (status_type == HOST_STATUS) {
		display_total = total_host_entries;
		if (local_result_start > total_host_entries || displayed_host_entries == 0) {
			display_from = 0;
			display_to = 0;
		} else {
			display_from = local_result_start;
			display_to = local_result_start + displayed_host_entries - 1;
		}
	} else {
		display_total = total_service_entries;
		if (local_result_start > total_service_entries || displayed_service_entries == 0) {
			display_from = 0;
			display_to = 0;
		} else {
			display_from = local_result_start;
			display_to = local_result_start + displayed_service_entries - 1;
		}
	}
	printf("<div style='text-align:center;padding-top:6px;'>Displaying Result %d - %d of %d Matching %s</div>\n", display_from, display_to, display_total, (status_type == SERVICE_STATUS) ? "Services" : "Hosts");

	/* copy page navigation to top of the page */
	if ((group_style_type == STYLE_HOST_SERVICE_DETAIL && status_type == SERVICE_STATUS) || group_style_type != STYLE_HOST_SERVICE_DETAIL) {
		printf("<script language='javascript' type=\"text/javascript\">\n");
		printf("$(document).ready(function() { \n");
		printf("$('#page_navigation').clone(true).appendTo('#page_navigation_copy');\n");
		printf("});\n");
		printf("</script>\n");
	}

	return;
}

