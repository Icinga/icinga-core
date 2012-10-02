/***********************************************************************
 *
 * CGIUTILS.C - Common utilities for Icinga CGIs
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2012 Nagios Core Development Team and Community Contributors
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
 ***********************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/locations.h"
#include "../include/objects.h"
#include "../include/macros.h"
#include "../include/statusdata.h"
#include "../include/comments.h"

#include "../include/cgiutils.h"

char            main_config_file[MAX_FILENAME_LENGTH];
char            command_file[MAX_FILENAME_LENGTH];

char            physical_html_path[MAX_FILENAME_LENGTH];
char            physical_images_path[MAX_FILENAME_LENGTH];
char            physical_ssi_path[MAX_FILENAME_LENGTH];
char            url_html_path[MAX_FILENAME_LENGTH];
char            url_docs_path[MAX_FILENAME_LENGTH];
char            url_images_path[MAX_FILENAME_LENGTH];
char            url_logo_images_path[MAX_FILENAME_LENGTH];
char            url_stylesheets_path[MAX_FILENAME_LENGTH];
char            url_js_path[MAX_FILENAME_LENGTH];
char            url_jquiryui_path[MAX_FILENAME_LENGTH];
char            url_media_path[MAX_FILENAME_LENGTH];

char            *service_critical_sound = NULL;
char            *service_warning_sound = NULL;
char            *service_unknown_sound = NULL;
char            *host_down_sound = NULL;
char            *host_unreachable_sound = NULL;
char            *normal_sound = NULL;
char            *statusmap_background_image = NULL;

char            *illegal_output_chars = NULL;

char            *http_charset = NULL;

char            *notes_url_target = NULL;
char            *action_url_target = NULL;

char		*csv_delimiter = CSV_DELIMITER;
char		*csv_data_enclosure = CSV_DATA_ENCLOSURE;

int		highlight_table_rows = TRUE;

char            nagios_check_command[MAX_INPUT_BUFFER] = "";
char            nagios_process_info[MAX_INPUT_BUFFER] = "";
int             nagios_process_state = STATE_OK;

int             enable_splunk_integration = FALSE;
char            *splunk_url = NULL;
int             lock_author_names = TRUE;

char		*authorization_config_file = NULL;
char		*authorized_for_all_host_commands = NULL;
char		*authorized_for_all_hosts = NULL;
char		*authorized_for_all_service_commands = NULL;
char		*authorized_for_all_services = NULL;
char		*authorized_for_configuration_information = NULL;
char		*authorized_for_full_command_resolution = NULL;
char		*authorized_for_read_only = NULL;
char		*authorized_for_comments_read_only = NULL;
char		*authorized_for_downtimes_read_only = NULL;
char		*authorized_for_system_commands = NULL;
char		*authorized_for_system_information = NULL;
char		*authorized_contactgroup_for_all_host_commands = NULL;
char		*authorized_contactgroup_for_all_hosts = NULL;
char		*authorized_contactgroup_for_all_service_commands = NULL;
char		*authorized_contactgroup_for_all_services = NULL;
char		*authorized_contactgroup_for_configuration_information = NULL;
char		*authorized_contactgroup_for_full_command_resolution = NULL;
char		*authorized_contactgroup_for_read_only = NULL;
char		*authorized_contactgroup_for_comments_read_only = NULL;
char		*authorized_contactgroup_for_downtimes_read_only = NULL;
char		*authorized_contactgroup_for_system_commands = NULL;
char		*authorized_contactgroup_for_system_information = NULL;
char		*default_user_name = NULL;

extern time_t   program_start;
extern int      nagios_pid;
extern int      daemon_mode;
extern int      enable_notifications;
extern int      execute_service_checks;
extern int      accept_passive_service_checks;
extern int      enable_event_handlers;
extern int      obsess_over_services;
extern int      enable_failure_prediction;
extern int      process_performance_data;
extern time_t   last_command_check;
extern time_t   last_log_rotation;
extern time_t	status_file_creation_time;

/* resource file */
char		resource_file[MAX_INPUT_BUFFER];
extern char 	*macro_user[MAX_USER_MACROS];

/** readlogs.c **/
int		log_rotation_method = LOG_ROTATION_NONE;
char		log_file[MAX_INPUT_BUFFER];
char		log_archive_path[MAX_INPUT_BUFFER];

int		status_update_interval = 60;
int             check_external_commands = 0;

int             date_format = DATE_FORMAT_US;

int             use_authentication = TRUE;

int             interval_length = 60;

int		show_all_services_host_is_authorized_for = TRUE;

int             use_pending_states = TRUE;

int             host_status_has_been_read = FALSE;
int             service_status_has_been_read = FALSE;
int             program_status_has_been_read = FALSE;

int             refresh_rate = DEFAULT_REFRESH_RATE;
int             refresh_type = JAVASCRIPT_REFRESH;

int             escape_html_tags = FALSE;

int             persistent_ack_comments = FALSE;

int             use_ssl_authentication = FALSE;

int             lowercase_user_name = FALSE;

int             default_statusmap_layout_method = 0;

int		color_transparency_index_r = 255;
int		color_transparency_index_g = 255;
int		color_transparency_index_b = 255;

int		status_show_long_plugin_output = FALSE;
int		tac_show_only_hard_state = FALSE;
int		extinfo_show_child_hosts = SHOW_CHILD_HOSTS_NONE;
int		suppress_maintenance_downtime = FALSE;
int		show_tac_header = TRUE;
int		show_tac_header_pending = TRUE;
int		showlog_initial_states = FALSE;
int		showlog_current_states = FALSE;
int		tab_friendly_titles = FALSE;
int		add_notif_num_hard = 0;
int		add_notif_num_soft = 0;
int		enforce_comments_on_actions = FALSE;
int		week_starts_on_monday = FALSE;

int		show_partial_hostgroups = FALSE;
int		default_downtime_duration = 7200;
int		default_expiring_acknowledgement_duration = 86400;
int		default_expiring_disabled_notifications_duration = 86400;

int		result_limit = 50;

extern hostgroup       *hostgroup_list;
extern contactgroup    *contactgroup_list;
extern command         *command_list;
extern timeperiod      *timeperiod_list;
extern contact         *contact_list;
extern serviceescalation *serviceescalation_list;

extern hoststatus      *hoststatus_list;
extern servicestatus   *servicestatus_list;


char encoded_url_string[4][MAX_INPUT_BUFFER]; // 2 to be able use url_encode 4 times
char encoded_html_string[2][(MAX_COMMAND_BUFFER * 6)]; // 2 to be able use html_encode twice

#ifdef HAVE_TZNAME
#ifdef CYGWIN
extern char     *_tzname[2] __declspec(dllimport);
#else
extern char     *tzname[2];
#endif
#endif

int content_type = HTML_CONTENT;
int embedded = FALSE;
int display_header = TRUE;
int display_status_totals = FALSE;
int refresh = TRUE;
int daemon_check = TRUE;
int tac_header = FALSE;

extern int CGI_ID;

/* used for logging function */
char		cgi_log_file[MAX_FILENAME_LENGTH] = "";
char		cgi_log_archive_path[MAX_FILENAME_LENGTH] = "";
int		use_logging = FALSE;
int		cgi_log_rotation_method = LOG_ROTATION_NONE;

/*
 * These function stubs allow us to compile a lot of the
 * source-files common to cgi's and daemon without adding
 * a whole bunch of #ifdef's everywhere. Note that we can't
 * have them as macros, since the goal is to compile the
 * source-files once. A decent linker will make the call
 * a no-op anyway, so it's not a big issue
 */
void logit(int data_type, int display, const char *fmt, ...) {
	return;
}
int log_debug_info(int leve, int verbosity, const char *fmt, ...) {
	return 0;
}

/**********************************************************
 ***************** CLEANUP FUNCTIONS **********************
 **********************************************************/

/* reset all variables used by the CGIs */
void reset_cgi_vars(void) {

	strcpy(main_config_file, "");

	strcpy(physical_html_path, "");
	strcpy(physical_images_path, "");
	strcpy(physical_ssi_path, "");

	strcpy(url_html_path, "");
	strcpy(url_docs_path, "");
	strcpy(url_stylesheets_path, "");
	strcpy(url_js_path, "");
	strcpy(url_media_path, "");
	strcpy(url_images_path, "");

	strcpy(log_file, "");
	strcpy(log_archive_path, DEFAULT_LOG_ARCHIVE_PATH);
	if (log_archive_path[strlen(log_archive_path) - 1] != '/' && strlen(log_archive_path) < sizeof(log_archive_path) - 2)
		strcat(log_archive_path, "/");
	strcpy(command_file, get_cmd_file_location());

	strcpy(nagios_check_command, "");
	strcpy(nagios_process_info, "");
	nagios_process_state = STATE_OK;

	log_rotation_method = LOG_ROTATION_NONE;
	cgi_log_rotation_method = LOG_ROTATION_NONE;

	use_authentication = TRUE;

	interval_length = 60;

	refresh_rate = DEFAULT_REFRESH_RATE;
	refresh_type = JAVASCRIPT_REFRESH;

	default_statusmap_layout_method = 0;
	default_statusmap_layout_method = 0;

	service_critical_sound = NULL;
	service_warning_sound = NULL;
	service_unknown_sound = NULL;
	host_down_sound = NULL;
	host_unreachable_sound = NULL;
	normal_sound = NULL;

	my_free(http_charset);
	http_charset = strdup(DEFAULT_HTTP_CHARSET);

	statusmap_background_image = NULL;
	color_transparency_index_r = 255;
	color_transparency_index_g = 255;
	color_transparency_index_b = 255;

	return;
}

/* free all memory for object definitions */
void free_memory(void) {

	/* free memory for common object definitions */
	free_object_data();

	/* free memory for status data */
	free_status_data();

	/* free misc data */
	free(service_critical_sound);
	free(service_warning_sound);
	free(service_unknown_sound);
	free(host_down_sound);
	free(host_unreachable_sound);
	free(normal_sound);
	free(statusmap_background_image);

	return;
}


/**********************************************************
 *************** CONFIG FILE FUNCTIONS ********************
 **********************************************************/

/* read the CGI config file location from an environment variable */
char * get_cgi_config_location(void) {
	static char *cgiloc = NULL;

	if (!cgiloc) {
		cgiloc = getenv("ICINGA_CGI_CONFIG");
		if (!cgiloc) {
			/* stay compatible */
			cgiloc = getenv("NAGIOS_CGI_CONFIG");
			if (!cgiloc) {
				cgiloc = DEFAULT_CGI_CONFIG_FILE;
			}
		}
	}

	return cgiloc;
}

/* read the command file location from an environment variable */
char * get_cmd_file_location(void) {
	static char *cmdloc = NULL;

	if (!cmdloc) {
		cmdloc = getenv("ICINGA_COMMAND_FILE");
		if (!cmdloc) {
			/* stay compatible */
			cmdloc = getenv("NAGIOS_COMMAND_FILE");
			if (!cmdloc) {
				cmdloc = DEFAULT_COMMAND_FILE;
			}
		}
	}
	return cmdloc;
}

/*read the CGI configuration file */
int read_cgi_config_file(char *filename) {
	char *input = NULL;
	mmapfile *thefile;
	char *var = NULL;
	char *val = NULL;


	if ((thefile = mmap_fopen(filename)) == NULL)
		return ERROR;

	while (1) {

		/* free memory */
		free(input);

		/* read the next line */
		if ((input = mmap_fgets_multiline(thefile)) == NULL)
			break;

		strip(input);

		var = strtok(input, "=");
		val = strtok(NULL, "\n");

		if (var == NULL || val == NULL)
			continue;

		if (!strcmp(var, "main_config_file")) {
			strncpy(main_config_file, val, sizeof(main_config_file));
			main_config_file[sizeof(main_config_file) - 1] = '\x0';
			strip(main_config_file);
		}

		else if (!strcmp(var, "show_all_services_host_is_authorized_for"))
			show_all_services_host_is_authorized_for = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "show_partial_hostgroups"))
			show_partial_hostgroups = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "use_pending_states"))
			use_pending_states = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "use_authentication"))
			use_authentication = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "nagios_check_command")) {
			strncpy(nagios_check_command, val, sizeof(nagios_check_command));
			nagios_check_command[sizeof(nagios_check_command) - 1] = '\x0';
			strip(nagios_check_command);
		}

		else if (!strcmp(var, "refresh_rate"))
			refresh_rate = atoi(val);

		else if (!strcmp(var, "refresh_type"))
			refresh_type = (atoi(val) > 0) ? JAVASCRIPT_REFRESH : HTTPHEADER_REFRESH;

		else if (!strcmp(var, "physical_html_path")) {
			strncpy(physical_html_path, val, sizeof(physical_html_path));
			physical_html_path[sizeof(physical_html_path) - 1] = '\x0';
			strip(physical_html_path);
			if (physical_html_path[strlen(physical_html_path) - 1] != '/' && (strlen(physical_html_path) < sizeof(physical_html_path) - 1))
				strcat(physical_html_path, "/");

			snprintf(physical_images_path, sizeof(physical_images_path), "%simages/", physical_html_path);
			physical_images_path[sizeof(physical_images_path) - 1] = '\x0';

			snprintf(physical_ssi_path, sizeof(physical_images_path), "%sssi/", physical_html_path);
			physical_ssi_path[sizeof(physical_ssi_path) - 1] = '\x0';
		}

		else if (!strcmp(var, "url_html_path")) {

			strncpy(url_html_path, val, sizeof(url_html_path));
			url_html_path[sizeof(url_html_path) - 1] = '\x0';

			strip(url_html_path);
			if (url_html_path[strlen(url_html_path) - 1] != '/' && (strlen(url_html_path) < sizeof(url_html_path) - 1))
				strcat(url_html_path, "/");

			snprintf(url_docs_path, sizeof(url_docs_path), "%sdocs/", url_html_path);
			url_docs_path[sizeof(url_docs_path) - 1] = '\x0';

			snprintf(url_images_path, sizeof(url_images_path), "%simages/", url_html_path);
			url_images_path[sizeof(url_images_path) - 1] = '\x0';

			snprintf(url_logo_images_path, sizeof(url_logo_images_path), "%slogos/", url_images_path);
			url_logo_images_path[sizeof(url_logo_images_path) - 1] = '\x0';

			/*
			snprintf(url_stylesheets_path,sizeof(url_stylesheets_path),"%sstylesheets/",url_html_path);
			url_stylesheets_path[sizeof(url_stylesheets_path)-1]='\x0';
			*/

			snprintf(url_js_path, sizeof(url_js_path), "%sjs/", url_html_path);
			url_js_path[sizeof(url_js_path) - 1] = '\x0';

			snprintf(url_jquiryui_path, sizeof(url_jquiryui_path), "%sjquery-ui/", url_html_path);
			url_jquiryui_path[sizeof(url_jquiryui_path) - 1] = '\x0';

			snprintf(url_media_path, sizeof(url_media_path), "%smedia/", url_html_path);
			url_media_path[sizeof(url_media_path) - 1] = '\x0';
		}

		else if (!strcmp(var, "url_stylesheets_path")) {

			strncpy(url_stylesheets_path, val, sizeof(url_stylesheets_path));
			url_stylesheets_path[sizeof(url_stylesheets_path) - 1] = '\x0';

			strip(url_stylesheets_path);
			if (url_stylesheets_path[strlen(url_stylesheets_path) - 1] != '/' && (strlen(url_stylesheets_path) < sizeof(url_stylesheets_path) - 1))
				strcat(url_stylesheets_path, "/");

		} else if (!strcmp(var, "cgi_log_archive_path")) {

			strncpy(cgi_log_archive_path, val, sizeof(cgi_log_archive_path));
			cgi_log_archive_path[sizeof(cgi_log_archive_path) - 1] = '\x0';

			strip(cgi_log_archive_path);
			if (cgi_log_archive_path[strlen(cgi_log_archive_path) - 1] != '/' && (strlen(cgi_log_archive_path) < sizeof(cgi_log_archive_path) - 1))
				strcat(cgi_log_archive_path, "/");

		} else if (!strcmp(var, "cgi_log_file")) {

			strncpy(cgi_log_file, val, sizeof(cgi_log_file));
			cgi_log_file[sizeof(cgi_log_file) - 1] = '\x0';
			strip(cgi_log_file);

		} else if (!strcmp(var, "cgi_log_rotation_method")) {
			if (!strcmp(val, "h"))
				cgi_log_rotation_method = LOG_ROTATION_HOURLY;
			else if (!strcmp(val, "d"))
				cgi_log_rotation_method = LOG_ROTATION_DAILY;
			else if (!strcmp(val, "w"))
				cgi_log_rotation_method = LOG_ROTATION_WEEKLY;
			else if (!strcmp(val, "m"))
				cgi_log_rotation_method = LOG_ROTATION_MONTHLY;
		} else if (!strcmp(var, "use_logging"))
			use_logging = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "enforce_comments_on_actions"))
			enforce_comments_on_actions = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "first_day_of_week"))
			week_starts_on_monday = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "service_critical_sound"))
			service_critical_sound = strdup(val);

		else if (!strcmp(var, "service_warning_sound"))
			service_warning_sound = strdup(val);

		else if (!strcmp(var, "service_unknown_sound"))
			service_unknown_sound = strdup(val);

		else if (!strcmp(var, "host_down_sound"))
			host_down_sound = strdup(val);

		else if (!strcmp(var, "host_unreachable_sound"))
			host_unreachable_sound = strdup(val);

		else if (!strcmp(var, "normal_sound"))
			normal_sound = strdup(val);

		else if (!strcmp(var, "statusmap_background_image"))
			statusmap_background_image = strdup(val);

		else if (!strcmp(var, "color_transparency_index_r"))
			color_transparency_index_r = atoi(val);

		else if (!strcmp(var, "color_transparency_index_g"))
			color_transparency_index_g = atoi(val);

		else if (!strcmp(var, "color_transparency_index_b"))
			color_transparency_index_b = atoi(val);

		else if (!strcmp(var, "default_statusmap_layout"))
			default_statusmap_layout_method = atoi(val);

		else if (!strcmp(var, "action_url_target"))
			action_url_target = strdup(val);

		else if (!strcmp(var, "illegal_macro_output_chars"))
			illegal_output_chars = strdup(val);

		else if (!strcmp(var, "http_charset"))
			http_charset = strdup(val);

		else if (!strcmp(var, "notes_url_target"))
			notes_url_target = strdup(val);

		else if (!strcmp(var, "enable_splunk_integration"))
			enable_splunk_integration = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "splunk_url"))
			splunk_url = strdup(val);

		else if (!strcmp(var, "escape_html_tags"))
			escape_html_tags = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "persistent_ack_comments"))
			persistent_ack_comments = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "default_expiring_acknowledgement_duration"))
			default_expiring_acknowledgement_duration = atoi(val);

		else if (!strcmp(var, "default_expiring_disabled_notifications_duration"))
			default_expiring_disabled_notifications_duration = atoi(val);

		else if (!strcmp(var, "lock_author_names"))
			lock_author_names = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "default_downtime_duration"))
			default_downtime_duration = atoi(val);

		else if (!strcmp(var, "result_limit"))
			result_limit = atoi(val);

		else if (!strcmp(var, "use_ssl_authentication"))
			use_ssl_authentication = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "lowercase_user_name"))
			lowercase_user_name = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "status_show_long_plugin_output"))
			status_show_long_plugin_output = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "tac_show_only_hard_state"))
			tac_show_only_hard_state = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "extinfo_show_child_hosts")) {
			if (atoi(val) == SHOW_CHILD_HOSTS_IMMEDIATE)
				extinfo_show_child_hosts = SHOW_CHILD_HOSTS_IMMEDIATE;
			else if (atoi(val) == SHOW_CHILD_HOSTS_ALL)
				extinfo_show_child_hosts = SHOW_CHILD_HOSTS_ALL;
		} else if (!strcmp(var, "suppress_maintenance_downtime"))
			suppress_maintenance_downtime = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "show_tac_header"))
			show_tac_header = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "show_tac_header_pending"))
			show_tac_header_pending = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "showlog_initial_state") || !strcmp(var, "showlog_initial_states"))
			showlog_initial_states = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "showlog_current_state") || !strcmp(var, "showlog_current_states"))
			showlog_current_states = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "tab_friendly_titles"))
			tab_friendly_titles = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "add_notif_num_hard"))
			add_notif_num_hard = atoi(val);

		else if (!strcmp(var, "add_notif_num_soft"))
			add_notif_num_soft = atoi(val);

		else if (!strcmp(var, "csv_delimiter"))
			csv_delimiter = strdup(val);

		else if (!strcmp(var, "csv_data_enclosure"))
			csv_data_enclosure = strdup(val);

		else if (!strcmp(var, "highlight_table_rows"))
			highlight_table_rows = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "display_status_totals"))
			display_status_totals = (atoi(val) > 0) ? TRUE : FALSE;

		else if (!strcmp(var, "authorization_config_file")) {
			authorization_config_file = strdup(val);
			strip(authorization_config_file);

		} else if (!strcmp(var, "authorized_for_all_host_commands")) {
			authorized_for_all_host_commands = strdup(val);
			strip(authorized_for_all_host_commands);

		} else if (!strcmp(var, "authorized_for_all_hosts")) {
			authorized_for_all_hosts = strdup(val);
			strip(authorized_for_all_hosts);

		} else if (!strcmp(var, "authorized_for_all_service_commands")) {
			authorized_for_all_service_commands = strdup(val);
			strip(authorized_for_all_service_commands);

		} else if (!strcmp(var, "authorized_for_all_services")) {
			authorized_for_all_services = strdup(val);
			strip(authorized_for_all_services);

		} else if (!strcmp(var, "authorized_for_configuration_information")) {
			authorized_for_configuration_information = strdup(val);
			strip(authorized_for_configuration_information);

		} else if (!strcmp(var, "authorized_for_full_command_resolution")) {
			authorized_for_full_command_resolution = strdup(val);
			strip(authorized_for_full_command_resolution);

		} else if (!strcmp(var, "authorized_for_read_only")) {
			authorized_for_read_only = strdup(val);
			strip(authorized_for_read_only);

		} else if (!strcmp(var, "authorized_for_comments_read_only")) {
			authorized_for_comments_read_only = strdup(val);
			strip(authorized_for_comments_read_only);

		} else if (!strcmp(var, "authorized_for_downtimes_read_only")) {
			authorized_for_downtimes_read_only = strdup(val);
			strip(authorized_for_downtimes_read_only);

		} else if (!strcmp(var, "authorized_for_system_commands")) {
			authorized_for_system_commands = strdup(val);
			strip(authorized_for_system_commands);

		} else if (!strcmp(var, "authorized_for_system_information")) {
			authorized_for_system_information = strdup(val);
			strip(authorized_for_system_information);

		} else if (!strcmp(var, "authorized_contactgroup_for_all_host_commands")) {
			authorized_contactgroup_for_all_host_commands = strdup(val);
			strip(authorized_contactgroup_for_all_host_commands);

		} else if (!strcmp(var, "authorized_contactgroup_for_all_hosts")) {
			authorized_contactgroup_for_all_hosts = strdup(val);
			strip(authorized_contactgroup_for_all_hosts);

		} else if (!strcmp(var, "authorized_contactgroup_for_all_service_commands")) {
			authorized_contactgroup_for_all_service_commands = strdup(val);
			strip(authorized_contactgroup_for_all_service_commands);

		} else if (!strcmp(var, "authorized_contactgroup_for_all_services")) {
			authorized_contactgroup_for_all_services = strdup(val);
			strip(authorized_contactgroup_for_all_services);

		} else if (!strcmp(var, "authorized_contactgroup_for_configuration_information")) {
			authorized_contactgroup_for_configuration_information = strdup(val);
			strip(authorized_contactgroup_for_configuration_information);

		} else if (!strcmp(var, "authorized_contactgroup_for_full_command_resolution")) {
			authorized_contactgroup_for_full_command_resolution = strdup(val);
			strip(authorized_contactgroup_for_full_command_resolution);

		} else if (!strcmp(var, "authorized_contactgroup_for_read_only")) {
			authorized_contactgroup_for_read_only = strdup(val);
			strip(authorized_contactgroup_for_read_only);

		} else if (!strcmp(var, "authorized_contactgroup_for_comments_read_only")) {
			authorized_contactgroup_for_comments_read_only = strdup(val);
			strip(authorized_contactgroup_for_comments_read_only);

		} else if (!strcmp(var, "authorized_contactgroup_for_downtimes_read_only")) {
			authorized_contactgroup_for_downtimes_read_only = strdup(val);
			strip(authorized_contactgroup_for_downtimes_read_only);

		} else if (!strcmp(var, "authorized_contactgroup_for_system_commands")) {
			authorized_contactgroup_for_system_commands = strdup(val);
			strip(authorized_contactgroup_for_system_commands);

		} else if (!strcmp(var, "authorized_contactgroup_for_system_information")) {
			authorized_contactgroup_for_system_information = strdup(val);
			strip(authorized_contactgroup_for_system_information);

		} else if (!strcmp(var, "default_user_name")) {
			default_user_name = strdup(val);
			strip(default_user_name);
		}
	}

	/* free memory and close the file */
	free(input);
	mmap_fclose(thefile);

	/* check if stylesheet path was set */
	if (!strcmp(url_stylesheets_path, "")) {
		snprintf(url_stylesheets_path, sizeof(url_stylesheets_path), "%sstylesheets/", url_html_path);
		url_stylesheets_path[sizeof(url_stylesheets_path) - 1] = '\x0';
	}

	if (!strcmp(main_config_file, ""))
		return ERROR;
	else
		return OK;
}

/* read the main configuration file */
int read_main_config_file(char *filename) {
	char *input = NULL;
	char *temp_buffer;
	mmapfile *thefile;


	if ((thefile = mmap_fopen(filename)) == NULL)
		return ERROR;

	while (1) {

		/* free memory */
		free(input);

		/* read the next line */
		if ((input = mmap_fgets_multiline(thefile)) == NULL)
			break;

		strip(input);

		if (strstr(input, "resource_file=") == input) {
			temp_buffer = strtok(input, "=");
			temp_buffer = strtok(NULL, "\x0");
			strncpy(resource_file, (temp_buffer == NULL) ? "" : temp_buffer, sizeof(resource_file));
			resource_file[sizeof(resource_file) - 1] = '\x0';
			strip(resource_file);
		}

		else if (strstr(input, "interval_length=") == input) {
			temp_buffer = strtok(input, "=");
			temp_buffer = strtok(NULL, "\x0");
			interval_length = (temp_buffer == NULL) ? 60 : atoi(temp_buffer);
		}

		else if (strstr(input, "status_update_interval=") == input) {
			temp_buffer = strtok(input, "=");
			temp_buffer = strtok(NULL, "\x0");
			status_update_interval = (temp_buffer == NULL) ? 60 : atoi(temp_buffer);
		}

		else if (strstr(input, "log_file=") == input) {
			temp_buffer = strtok(input, "=");
			temp_buffer = strtok(NULL, "\x0");
			strncpy(log_file, (temp_buffer == NULL) ? "" : temp_buffer, sizeof(log_file));
			log_file[sizeof(log_file) - 1] = '\x0';
			strip(log_file);
		}

		else if (strstr(input, "log_archive_path=") == input) {
			temp_buffer = strtok(input, "=");
			temp_buffer = strtok(NULL, "\n");
			strncpy(log_archive_path, (temp_buffer == NULL) ? "" : temp_buffer, sizeof(log_archive_path));
			log_archive_path[sizeof(log_archive_path) - 1] = '\x0';
			strip(physical_html_path);
			if (log_archive_path[strlen(log_archive_path) - 1] != '/' && (strlen(log_archive_path) < sizeof(log_archive_path) - 1))
				strcat(log_archive_path, "/");
		}

		else if (strstr(input, "log_rotation_method=") == input) {
			temp_buffer = strtok(input, "=");
			temp_buffer = strtok(NULL, "\x0");
			if (temp_buffer == NULL)
				log_rotation_method = LOG_ROTATION_NONE;
			else if (!strcmp(temp_buffer, "h"))
				log_rotation_method = LOG_ROTATION_HOURLY;
			else if (!strcmp(temp_buffer, "d"))
				log_rotation_method = LOG_ROTATION_DAILY;
			else if (!strcmp(temp_buffer, "w"))
				log_rotation_method = LOG_ROTATION_WEEKLY;
			else if (!strcmp(temp_buffer, "m"))
				log_rotation_method = LOG_ROTATION_MONTHLY;
		}

		else if (strstr(input, "command_file=") == input) {
			temp_buffer = strtok(input, "=");
			temp_buffer = strtok(NULL, "\x0");
			strncpy(command_file, (temp_buffer == NULL) ? "" : temp_buffer, sizeof(command_file));
			command_file[sizeof(command_file) - 1] = '\x0';
			strip(command_file);
		}

		else if (strstr(input, "check_external_commands=") == input) {
			temp_buffer = strtok(input, "=");
			temp_buffer = strtok(NULL, "\x0");
			check_external_commands = (temp_buffer == NULL) ? 0 : atoi(temp_buffer);
		}

		else if (strstr(input, "date_format=") == input) {
			temp_buffer = strtok(input, "=");
			temp_buffer = strtok(NULL, "\x0");
			if (temp_buffer == NULL)
				date_format = DATE_FORMAT_US;
			else if (!strcmp(temp_buffer, "euro"))
				date_format = DATE_FORMAT_EURO;
			else if (!strcmp(temp_buffer, "iso8601"))
				date_format = DATE_FORMAT_ISO8601;
			else if (!strcmp(temp_buffer, "strict-iso8601"))
				date_format = DATE_FORMAT_STRICT_ISO8601;
			else
				date_format = DATE_FORMAT_US;
		}
	}

	/* free memory and close the file */
	free(input);
	mmap_fclose(thefile);

	return OK;
}

/* read all object definitions */
int read_all_object_configuration_data(char *config_file, int options) {
	int result = OK;

	/* read in all external config data of the desired type(s) */
	result = read_object_config_data(config_file, options, FALSE, FALSE);

	return result;
}

/* read all status data */
int read_all_status_data(char *config_file, int options) {
	int result = OK;

	/* don't duplicate things we've already read in */
	if (program_status_has_been_read == TRUE && (options & READ_PROGRAM_STATUS))
		options -= READ_PROGRAM_STATUS;
	if (host_status_has_been_read == TRUE && (options & READ_HOST_STATUS))
		options -= READ_HOST_STATUS;
	if (service_status_has_been_read == TRUE && (options & READ_SERVICE_STATUS))
		options -= READ_SERVICE_STATUS;

	/* bail out if we've already read what we need */
	if (options <= 0)
		return OK;

	/* read in all external status data */
	result = read_status_data(config_file, options);

	/* mark what items we've read in... */
	if (options & READ_PROGRAM_STATUS)
		program_status_has_been_read = TRUE;
	if (options & READ_HOST_STATUS)
		host_status_has_been_read = TRUE;
	if (options & READ_SERVICE_STATUS)
		service_status_has_been_read = TRUE;

	return result;
}


/* processes macros in resource file */
int read_icinga_resource_file(char *resource_file) {
	char *input = NULL;
	char *variable = NULL;
	char *value = NULL;
	char *temp_ptr = NULL;
	mmapfile *thefile = NULL;
	int current_line = 1;
	int error = FALSE;
	int user_index = 0;

	if ((thefile = mmap_fopen(resource_file)) == NULL) {
		return ERROR;
	}

	/* process all lines in the resource file */
	while (1) {

		/* free memory */
		my_free(input);

		/* read the next line */
		if ((input = mmap_fgets_multiline(thefile)) == NULL)
			break;

		current_line = thefile->current_line;

		/* skip blank lines and comments */
		if (input[0] == '#' || input[0] == '\x0' || input[0] == '\n' || input[0] == '\r')
			continue;

		strip(input);

		/* get the variable name */
		if ((temp_ptr = my_strtok(input, "=")) == NULL) {
			error = TRUE;
			break;
		}
		if ((variable = (char *)strdup(temp_ptr)) == NULL) {
			error = TRUE;
			break;
		}

		/* get the value */
		if ((temp_ptr = my_strtok(NULL, "\n")) == NULL) {
			error = TRUE;
			break;
		}
		if ((value = (char *)strdup(temp_ptr)) == NULL) {
			error = TRUE;
			break;
		}

		/* what should we do with the variable/value pair? */

		/* check for macro declarations */
		if (variable[0] == '$' && variable[strlen(variable) - 1] == '$') {

			/* $USERx$ macro declarations */
			if (strstr(variable, "$USER") == variable  && strlen(variable) > 5) {
				user_index = atoi(variable + 5) - 1;
				if (user_index >= 0 && user_index < MAX_USER_MACROS) {
					my_free(macro_user[user_index]);
					macro_user[user_index] = (char *)strdup(value);
				}
			}
		}
		my_free(variable);
		my_free(value);
	}

	my_free(variable);
	my_free(value);

	/* free leftover memory and close the file */
	my_free(input);
	mmap_fclose(thefile);

	if (error == TRUE)
		return ERROR;

	return OK;
}


/**********************************************************
 *************** COMMON HEADER AND FOOTER *****************
 **********************************************************/

void document_header(int cgi_id, int use_stylesheet, char *cgi_title) {
	char date_time[MAX_DATETIME_LENGTH];
	char *cgi_name = NULL;
	char *cgi_css = NULL;
	char *cgi_body_class = NULL;
	time_t expire_time;
	time_t current_time;

	switch (cgi_id) {
	case STATUS_CGI_ID:
		cgi_name        = STATUS_CGI;
		cgi_css         = STATUS_CSS;
		cgi_body_class  = "status";
		break;
	case AVAIL_CGI_ID:
		cgi_name        = AVAIL_CGI;
		cgi_css         = AVAIL_CSS;
		cgi_body_class  = "avail";
		refresh         = FALSE;
		break;
	case CMD_CGI_ID:
		cgi_name        = CMD_CGI;
		cgi_css         = CMD_CSS;
		cgi_body_class  = "cmd";
		refresh         = FALSE;
		break;
	case CONFIG_CGI_ID:
		cgi_name        = CONFIG_CGI;
		cgi_css         = CONFIG_CSS;
		cgi_body_class  = "config";
		refresh         = FALSE;
		break;
	case EXTINFO_CGI_ID:
		cgi_name        = EXTINFO_CGI;
		cgi_css         = EXTINFO_CSS;
		cgi_body_class  = "extinfo";
		break;
	case HISTOGRAM_CGI_ID:
		cgi_name        = HISTOGRAM_CGI;
		cgi_css         = HISTOGRAM_CSS;
		cgi_body_class  = "histogram";
		refresh         = FALSE;
		break;
	case HISTORY_CGI_ID:
		cgi_name        = HISTORY_CGI;
		cgi_css         = HISTORY_CSS;
		cgi_body_class  = "history";
		refresh         = FALSE;
		break;
	case NOTIFICATIONS_CGI_ID:
		cgi_name        = NOTIFICATIONS_CGI;
		cgi_css         = NOTIFICATIONS_CSS;
		cgi_body_class  = "notifications";
		refresh         = FALSE;
		break;
	case OUTAGES_CGI_ID:
		cgi_name        = OUTAGES_CGI;
		cgi_css         = OUTAGES_CSS;
		cgi_body_class  = "outages";
		break;
	case SHOWLOG_CGI_ID:
		cgi_name        = SHOWLOG_CGI;
		cgi_css         = SHOWLOG_CSS;
		cgi_body_class  = "showlog";
		refresh         = FALSE;
		break;
	case STATUSMAP_CGI_ID:
		cgi_name        = STATUSMAP_CGI;
		cgi_css         = STATUSMAP_CSS;
		cgi_body_class  = "statusmap";
		break;
	case SUMMARY_CGI_ID:
		cgi_name        = SUMMARY_CGI;
		cgi_css         = SUMMARY_CSS;
		cgi_body_class  = "summary";
		refresh         = FALSE;
		break;
	case TAC_CGI_ID:
		cgi_name        = TAC_CGI;
		cgi_css         = TAC_CSS;
		cgi_body_class  = "tac";
		if (tac_header == TRUE && show_tac_header == FALSE)
			refresh = FALSE;
		break;
	case TRENDS_CGI_ID:
		cgi_name        = TRENDS_CGI;
		cgi_css         = TRENDS_CSS;
		cgi_body_class  = "trends";
		refresh         = FALSE;
		break;
	case ERROR_CGI_ID:
		cgi_name        = "";
		cgi_css         = CMD_CSS;
		cgi_body_class  = "error";
		break;
	}


	// don't refresh non html output
	if (content_type == JSON_CONTENT || content_type == CSV_CONTENT)
		refresh = FALSE;

	// send top http header
	if (cgi_id != ERROR_CGI_ID) {
		printf("Cache-Control: no-store\r\n");
		printf("Pragma: no-cache\r\n");

		if (refresh_type == HTTPHEADER_REFRESH && refresh == TRUE)
			printf("Refresh: %d\r\n", refresh_rate);

		time(&current_time);
		get_time_string(&current_time, date_time, (int)sizeof(date_time), HTTP_DATE_TIME);
		printf("Last-Modified: %s\r\n", date_time);

		expire_time = (time_t)0L;
		get_time_string(&expire_time, date_time, (int)sizeof(date_time), HTTP_DATE_TIME);
		printf("Expires: %s\r\n", date_time);
	}

	if (content_type == IMAGE_CONTENT) {
		printf("Content-Type: image/png\r\n\r\n");
		return;
	}

	if (content_type == CSV_CONTENT) {
		printf("Content-type: text/plain; charset=\"%s\"\r\n\r\n", http_charset);
		return;
	}

	if (content_type == JSON_CONTENT) {
		printf("Content-type: text/json; charset=\"%s\"\r\n\r\n", http_charset);
		printf("{ \"cgi_json_version\": \"%s\",\n", JSON_OUTPUT_VERSION);
		printf("\"%s\": {\n", cgi_body_class);
		return;
	}

	if (content_type == XML_CONTENT) {
		printf("Content-type: text/xml; charset=\"%s\"\r\n\r\n", http_charset);
		printf("<?xml version=\"1.0\" encoding=\"%s\"?>\n", http_charset);
		return;
	}

	if (cgi_id != ERROR_CGI_ID) {
		// send HTML CONTENT
		printf("Content-type: text/html; charset=\"%s\"\r\n\r\n", http_charset);
	}

	if (embedded == TRUE)
		return;

	printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
	printf("<html>\n");
	printf("<head>\n");
	printf("<link rel=\"shortcut icon\" href=\"%sfavicon.ico\" type=\"image/ico\">\n", url_images_path);
	printf("<META HTTP-EQUIV='Pragma' CONTENT='no-cache'>\n");
	printf("<meta http-equiv=\"content-type\" content=\"text/html; charset=%s\">\n", http_charset);
	printf("<title>%s</title>\n", cgi_title);

	if (cgi_id == TAC_CGI_ID && tac_header == TRUE) {
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n", url_stylesheets_path, (show_tac_header == TRUE) ? TAC_HEADER_CSS : COMMON_CSS);
	} else if (use_stylesheet) {
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n", url_stylesheets_path, COMMON_CSS);
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n", url_stylesheets_path, cgi_css);
	}

	// javascript refresh
	if (refresh_type == JAVASCRIPT_REFRESH) {
		printf("<script type=\"text/javascript\">\n");
		printf("var refresh_rate=%d;\n", refresh_rate);
		printf("var do_refresh=%s;\n", (refresh == TRUE) ? "true" : "false");
		printf("var counter_seconds=refresh_rate;\n");
		printf("</script>\n");
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_js_path, PAGE_REFRESH_JS);
	}

	/* jQuery JavaScript library */
	printf("<script type='text/javascript' src='%s%s'></script>\n", url_js_path, JQUERY_MAIN_JS);

	/* datetimepicker libs and css */
	if (cgi_id == CMD_CGI_ID || cgi_id == NOTIFICATIONS_CGI_ID || cgi_id == SHOWLOG_CGI_ID) {
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_jquiryui_path, JQ_UI_CORE_JS);
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_jquiryui_path, JQ_UI_WIDGET_JS);
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_jquiryui_path, JQ_UI_MOUSE_JS);
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_jquiryui_path, JQ_UI_SLIDER_JS);
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_jquiryui_path, JQ_UI_DATEPICKER_JS);
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_jquiryui_path, JQ_UI_TIMEPICKER_JS);

		printf("<link rel='stylesheet' type='text/css' href='%s%s'>\n", url_jquiryui_path, JQ_UI_ALL_CSS);
		printf("<link rel='stylesheet' type='text/css' href='%s%s'>\n", url_jquiryui_path, JQ_UI_TIMEPICKER_CSS);

		printf("<script type=\"text/javascript\">\n");
		printf("$(function() {\n");
		printf("\t$( \".timepicker\" ).datetimepicker({\n");
		printf("\t\tfirstDay: %d,\n", week_starts_on_monday);

		if (date_format == DATE_FORMAT_EURO)
			printf("\t\tdateFormat: 'dd-mm-yy',\n");
		else if (date_format == DATE_FORMAT_ISO8601 || date_format == DATE_FORMAT_STRICT_ISO8601)
			printf("\t\tdateFormat: 'yy-mm-dd%s',\n", (date_format == DATE_FORMAT_STRICT_ISO8601) ? "T" : "");
		else
			printf("\t\tdateFormat: 'mm-dd-yy',\n");

		printf("\t\ttimeFormat: 'hh:mm:ss',\n");
		printf("\t\tshowWeek: true,\n");
		printf("\t\tchangeMonth: true,\n");
		printf("\t\tchangeYear: true\n");
		printf("\t});\n");
		printf("});\n");
		printf("</script>\n");
	}

	/* Add jQuery MSDropDown to sites with pagination and status.cgi */
	if (cgi_id == STATUS_CGI_ID || cgi_id == EXTINFO_CGI_ID || cgi_id == CONFIG_CGI_ID || cgi_id == HISTORY_CGI_ID || cgi_id == NOTIFICATIONS_CGI_ID || cgi_id == SHOWLOG_CGI_ID) {

		/* This CSS IS needed for proper dropdown menu's (bypass the use_stylesheets above, who does without anyway?) */
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_js_path, JQUERY_DD_JS);
		printf("<link rel='stylesheet' type='text/css' href='%s%s'>\n", url_stylesheets_path, JQUERY_DD_CSS);

		/* functions to handle the checkboxes and dropdown menus */
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_js_path, CHECKBOX_FUNCTIONS_JS);
	}

	if (cgi_id == STATUSMAP_CGI_ID || cgi_id == TRENDS_CGI_ID) {
		/* write JavaScript code for popup window */
		write_popup_code(cgi_id);
	}

	printf("</head>\n");

	if (cgi_id == STATUSMAP_CGI_ID)
		printf("<body CLASS='%s' name='mappage' id='mappage'>\n", cgi_body_class);
	else if (cgi_id == TAC_CGI_ID && tac_header == FALSE)
		printf("<body CLASS='%s' marginwidth=2 marginheight=2 topmargin=0 leftmargin=0 rightmargin=0>\n", cgi_body_class);
	else
		printf("<body CLASS='%s'>\n", cgi_body_class);

	/* include user SSI header */
	if (tac_header == FALSE)
		include_ssi_files(cgi_name, SSI_HEADER);

	/* this line was also in histogram.c, is this necessary??? */
	if (cgi_id == HISTOGRAM_CGI_ID || cgi_id == STATUSMAP_CGI_ID || cgi_id == TRENDS_CGI_ID)
		printf("<div id=\"popup\" style=\"position:absolute; z-index:1; visibility: hidden\"></div>\n");

	if (cgi_id == STATUS_CGI_ID || cgi_id == CMD_CGI_ID || cgi_id == OUTAGES_CGI_ID) {
		printf("\n<script type='text/javascript' src='%s%s'>\n<!-- SkinnyTip (c) Elliott Brueggeman -->\n</script>\n", url_js_path, SKINNYTIP_JS);
		printf("<div id='tiplayer' style='position:absolute; visibility:hidden; z-index:1000;'></div>\n");
	}

	return;
}


void document_footer(int cgi_id) {
	char *cgi_name = NULL;

	switch (cgi_id) {
	case STATUS_CGI_ID:
		cgi_name = STATUS_CGI;
		break;
	case AVAIL_CGI_ID:
		cgi_name = AVAIL_CGI;
		break;
	case CMD_CGI_ID:
		cgi_name = CMD_CGI;
		break;
	case CONFIG_CGI_ID:
		cgi_name = CONFIG_CGI;
		break;
	case EXTINFO_CGI_ID:
		cgi_name = EXTINFO_CGI;
		break;
	case HISTOGRAM_CGI_ID:
		cgi_name = HISTOGRAM_CGI;
		break;
	case HISTORY_CGI_ID:
		cgi_name = HISTORY_CGI;
		break;
	case NOTIFICATIONS_CGI_ID:
		cgi_name = NOTIFICATIONS_CGI;
		break;
	case OUTAGES_CGI_ID:
		cgi_name = OUTAGES_CGI;
		break;
	case SHOWLOG_CGI_ID:
		cgi_name = SHOWLOG_CGI;
		break;
	case STATUSMAP_CGI_ID:
		cgi_name = STATUSMAP_CGI;
		break;
	case SUMMARY_CGI_ID:
		cgi_name = SUMMARY_CGI;
		break;
	case TAC_CGI_ID:
		cgi_name = TAC_CGI;
		break;
	case TRENDS_CGI_ID:
		cgi_name = TRENDS_CGI;
		break;
	}

	if (content_type == XML_CONTENT)
		return;

	if (content_type == JSON_CONTENT) {
		printf("}\n}\n");
		return;
	}

	/*
	   top is embedded, so if this is top we don't want to return
	   otherwise if embedded or HTML_CONTENT we do want to return
	*/
	if (embedded || content_type != HTML_CONTENT)
		return;

	/* include user SSI footer */
	if (tac_header == FALSE)
		include_ssi_files(cgi_name, SSI_FOOTER);

	printf("</body>\n");
	printf("</html>\n");

	return;
}

/* write JavaScript code an layer for popup window */
void write_popup_code(int cgi_id) {
	char *border_color = "#000000";
	char *background_color = "#ffffcc";
	int border = 1;
	int padding = 3;
	int x_offset = 3;
	int y_offset = 3;

	printf("<SCRIPT LANGUAGE='JavaScript' type='text/javascript'>\n");
	printf("<!--\n");
	printf("// JavaScript popup based on code originally found at http://www.helpmaster.com/htmlhelp/javascript/popjbpopup.htm\n");
	printf("function showPopup(text, eventObj){\n");
	printf("if(!document.all && document.getElementById)\n");
	printf("{ document.all=document.getElementsByTagName(\"*\")}\n");
	printf("ieLayer = 'document.all[\\'popup\\']';\n");
	printf("nnLayer = 'document.layers[\\'popup\\']';\n");
	printf("moLayer = 'document.getElementById(\\'popup\\')';\n");

	printf("if(!(document.all||document.layers||document.documentElement)) return;\n");

	printf("if(document.all) { document.popup=eval(ieLayer); }\n");
	printf("else {\n");
	printf("  if (document.documentElement) document.popup=eval(moLayer);\n");
	printf("  else document.popup=eval(nnLayer);\n");
	printf("}\n");

	printf("var table = \"\";\n");

	printf("if (document.all||document.documentElement){\n");
	printf("table += \"<table bgcolor='%s' border=%d cellpadding=%d cellspacing=0>\";\n", background_color, border, padding);
	printf("table += \"<tr><td>\";\n");
	printf("table += \"<table cellspacing=0 cellpadding=%d>\";\n", padding);
	printf("table += \"<tr><td bgcolor='%s' class='popupText'>\" + text + \"</td></tr>\";\n", background_color);
	printf("table += \"</table></td></tr></table>\"\n");
	printf("document.popup.innerHTML = table;\n");

	if (cgi_id == STATUSMAP_CGI_ID) {
		printf("document.popup.style.left = document.body.scrollLeft + %d;\n", x_offset);
		printf("document.popup.style.top = document.body.scrollTop + %d;\n", y_offset);
	} else if (cgi_id == TRENDS_CGI_ID) {
		printf("document.popup.style.left = (document.all ? eventObj.x : eventObj.layerX) + %d;\n", x_offset);
		printf("document.popup.style.top  = (document.all ? eventObj.y : eventObj.layerY) + %d;\n", y_offset);
	}

	printf("document.popup.style.visibility = \"visible\";\n");
	printf("} \n");


	printf("else{\n");
	printf("table += \"<table cellpadding=%d border=%d cellspacing=0 bordercolor='%s'>\";\n", padding, border, border_color);
	printf("table += \"<tr><td bgcolor='%s' class='popupText'>\" + text + \"</td></tr></table>\";\n", background_color);
	printf("document.popup.document.open();\n");
	printf("document.popup.document.write(table);\n");
	printf("document.popup.document.close();\n");

	/* set x coordinate */
	printf("document.popup.left = eventObj.layerX + %d;\n", x_offset);

	/* make sure we don't overlap the right side of the screen */
	printf("if(document.popup.left + document.popup.document.width + %d > window.innerWidth) document.popup.left = window.innerWidth - document.popup.document.width - %d - 16;\n", x_offset, x_offset);

	/* set y coordinate */
	printf("document.popup.top  = eventObj.layerY + %d;\n", y_offset);

	/* make sure we don't overlap the bottom edge of the screen */
	printf("if(document.popup.top + document.popup.document.height + %d > window.innerHeight) document.popup.top = window.innerHeight - document.popup.document.height - %d - 16;\n", y_offset, y_offset);

	/* make the popup visible */
	printf("document.popup.visibility = \"visible\";\n");
	printf("}\n");
	printf("}\n");

	printf("function hidePopup(){ \n");
	printf("if (!(document.all || document.layers || document.documentElement)) return;\n");
	printf("if (document.popup == null){ }\n");
	printf("else if (document.all||document.documentElement) document.popup.style.visibility = \"hidden\";\n");
	printf("else document.popup.visibility = \"hidden\";\n");
	printf("document.popup = null;\n");
	printf("}\n");
	printf("//-->\n");

	printf("</SCRIPT>\n");

	return;
}


/**********************************************************
 *************** MISC UTILITY FUNCTIONS *******************
 **********************************************************/

/* unescapes newlines in a string */
char *unescape_newlines(char *rawbuf) {
	register int x, y;

	for (x = 0, y = 0; rawbuf[x] != (char)'\x0'; x++) {

		if (rawbuf[x] == '\\') {

			/* unescape newlines */
			if (rawbuf[x + 1] == 'n') {
				rawbuf[y++] = '\n';
				x++;
			}

			/* unescape backslashes and other stuff */
			if (rawbuf[x + 1] != '\x0') {
				rawbuf[y++] = rawbuf[x + 1];
				x++;
			}

		} else
			rawbuf[y++] = rawbuf[x];
	}
	rawbuf[y] = '\x0';

	return rawbuf;
}

/* escapes newlines in a string */
char *escape_newlines(char *rawbuf) {
	char *newbuf = NULL;
	register int x, y;

	if (rawbuf == NULL)
		return NULL;

	/* allocate enough memory to escape all chars if necessary */
	if ((newbuf = malloc((strlen(rawbuf) * 2) + 1)) == NULL)
		return NULL;

	for (x = 0, y = 0; rawbuf[x] != (char)'\x0'; x++) {

		/* escape backslashes */
		if (rawbuf[x] == '\\') {
			newbuf[y++] = '\\';
			newbuf[y++] = '\\';
		}

		/* escape newlines */
		else if (rawbuf[x] == '\n') {
			newbuf[y++] = '\\';
			newbuf[y++] = 'n';
		}

		else
			newbuf[y++] = rawbuf[x];
	}
	newbuf[y] = '\x0';

	return newbuf;
}

/* strips HTML and bad stuff from plugin output */
void sanitize_plugin_output(char *buffer) {
	int x = 0;
	int y = 0;
	int in_html = FALSE;
	char *new_buffer;

	if (buffer == NULL)
		return;

	new_buffer = strdup(buffer);
	if (new_buffer == NULL)
		return;

	/* check each character */
	for (x = 0, y = 0; buffer[x] != '\x0'; x++) {

		/* we just started an HTML tag */
		if (buffer[x] == '<') {
			in_html = TRUE;
			continue;
		}

		/* end of an HTML tag */
		else if (buffer[x] == '>') {
			in_html = FALSE;
			continue;
		}

		/* skip everything inside HTML tags */
		else if (in_html == TRUE)
			continue;

		/* strip single and double quotes */
		else if (buffer[x] == '\'' || buffer[x] == '\"')
			new_buffer[y++] = ' ';

		/* strip semicolons (replace with colons) */
		else if (buffer[x] == ';')
			new_buffer[y++] = ':';

		/* strip pipe and ampersand */
		else if (buffer[x] == '&' || buffer[x] == '|')
			new_buffer[y++] = ' ';

		/* normal character */
		else
			new_buffer[y++] = buffer[x];
	}

	/* terminate sanitized buffer */
	new_buffer[y++] = '\x0';

	/* copy the sanitized buffer back to the original */
	strcpy(buffer, new_buffer);

	/* free memory allocated to the new buffer */
	free(new_buffer);

	return;
}

/* get date/time string */
void get_time_string(time_t *raw_time, char *buffer, int buffer_length, int type) {
	time_t t;
	struct tm *tm_ptr = NULL;
	int hour = 0;
	int minute = 0;
	int second = 0;
	int month = 0;
	int day = 0;
	int year = 0;
	char *weekdays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char *tzone = "";

	if (raw_time == NULL)
		time(&t);
	else
		t = *raw_time;

	if (type == HTTP_DATE_TIME)
		tm_ptr = gmtime(&t);
	else
		tm_ptr = localtime(&t);

	hour = tm_ptr->tm_hour;
	minute = tm_ptr->tm_min;
	second = tm_ptr->tm_sec;
	month = tm_ptr->tm_mon + 1;
	day = tm_ptr->tm_mday;
	year = tm_ptr->tm_year + 1900;

#ifdef HAVE_TM_ZONE
	tzone = (char *)tm_ptr->tm_zone;
#else
	tzone = (tm_ptr->tm_isdst) ? tzname[1] : tzname[0];
#endif

	/* ctime() style */
	if (type == LONG_DATE_TIME)
		snprintf(buffer, buffer_length, "%s %s %d %02d:%02d:%02d %s %d", weekdays[tm_ptr->tm_wday], months[tm_ptr->tm_mon], day, hour, minute, second, tzone, year);

	/* short style */
	else if (type == SHORT_DATE_TIME) {
		if (date_format == DATE_FORMAT_EURO)
			snprintf(buffer, buffer_length, "%02d-%02d-%04d %02d:%02d:%02d", tm_ptr->tm_mday, tm_ptr->tm_mon + 1, tm_ptr->tm_year + 1900, tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
		else if (date_format == DATE_FORMAT_ISO8601 || date_format == DATE_FORMAT_STRICT_ISO8601)
			snprintf(buffer, buffer_length, "%04d-%02d-%02d%c%02d:%02d:%02d", tm_ptr->tm_year + 1900, tm_ptr->tm_mon + 1, tm_ptr->tm_mday, (date_format == DATE_FORMAT_STRICT_ISO8601) ? 'T' : ' ', tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
		else
			snprintf(buffer, buffer_length, "%02d-%02d-%04d %02d:%02d:%02d", tm_ptr->tm_mon + 1, tm_ptr->tm_mday, tm_ptr->tm_year + 1900, tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
	}

	/* short date */
	else if (type == SHORT_DATE) {
		if (date_format == DATE_FORMAT_EURO)
			snprintf(buffer, buffer_length, "%02d-%02d-%04d", day, month, year);
		else if (date_format == DATE_FORMAT_ISO8601 || date_format == DATE_FORMAT_STRICT_ISO8601)
			snprintf(buffer, buffer_length, "%04d-%02d-%02d", year, month, day);
		else
			snprintf(buffer, buffer_length, "%02d-%02d-%04d", month, day, year);
	}

	/* expiration date/time for HTTP headers */
	else if (type == HTTP_DATE_TIME)
		snprintf(buffer, buffer_length, "%s, %02d %s %d %02d:%02d:%02d GMT", weekdays[tm_ptr->tm_wday], day, months[tm_ptr->tm_mon], year, hour, minute, second);

	/* short time */
	else
		snprintf(buffer, buffer_length, "%02d:%02d:%02d", hour, minute, second);

	buffer[buffer_length - 1] = '\x0';

	return;
}

/* get time string for an interval of time */
void get_interval_time_string(double time_units, char *buffer, int buffer_length) {
	unsigned long total_seconds;
	int hours = 0;
	int minutes = 0;
	int seconds = 0;

	total_seconds = (unsigned long)(time_units * interval_length);
	hours = (int)total_seconds / 3600;
	total_seconds %= 3600;
	minutes = (int)total_seconds / 60;
	total_seconds %= 60;
	seconds = (int)total_seconds;
	snprintf(buffer, buffer_length, "%dh %dm %ds", hours, minutes, seconds);
	buffer[buffer_length - 1] = '\x0';

	return;
}

/* encodes a string in proper URL format */
char * url_encode(char *input) {
	int len, output_len;
	int x, y;
	char temp_expansion[4];
	static int num_encoded_url = 0;
	char* str = encoded_url_string[num_encoded_url];

	/* initialize return string */
	strcpy(str, "");

	if (input == NULL)
		return str;

	len = (int)strlen(input);
	output_len = (int)sizeof(encoded_url_string[num_encoded_url]);

	str[0] = '\x0';

	for (x = 0, y = 0; x <= len && y < output_len - 1; x++) {

		/* end of string */
		if ((char)input[x] == (char)'\x0') {
			str[y] = '\x0';
			break;
		}

		/* alpha-numeric characters and a few other characters don't get encoded */
		else if (((char)input[x] >= '0' && (char)input[x] <= '9') || ((char)input[x] >= 'A' && (char)input[x] <= 'Z') || ((char)input[x] >= (char)'a' && (char)input[x] <= (char)'z') || (char)input[x] == (char)'.' || (char)input[x] == (char)'-' || (char)input[x] == (char)'_') {
			str[y] = input[x];
			y++;
		}

		/* high bit characters don't get encoded */
		else if ((unsigned char)input[x] >= 0x7f) {
			str[y] = input[x];
			y++;
		}

		/* spaces are pluses */
		else if ((char)input[x] == (char)' ') {
			str[y] = '+';
			y++;
		}

		/* anything else gets represented by its hex value */
		else {
			str[y] = '\x0';
			if ((int)strlen(str) < (output_len - 3)) {
				sprintf(temp_expansion, "%%%02X", (unsigned int)input[x]);
				strcat(str, temp_expansion);
				y += 3;
			}
		}
	}

	str[ sizeof(encoded_url_string[num_encoded_url]) - 1] = '\x0';

	if (num_encoded_url >= 3)
		num_encoded_url = 0;
	else
		num_encoded_url++;

	return str;
}

/* escapes a string used in HTML */
char * html_encode(char *input, int escape_newlines) {
	int len, output_len;
	int x, y;
	char temp_expansion[10];
	static int num_encoded_html = 0;
	char* str = encoded_html_string[num_encoded_html];

	/* initialize return string */
	strcpy(str, "");

	if (input == NULL)
		return str;

	len = (int)strlen(input);
	output_len = (int)sizeof(encoded_html_string[num_encoded_html]);

	str[0] = '\x0';

	for (x = 0, y = 0; x <= len && y < output_len - 1; x++) {

		/* end of string */
		if ((char)input[x] == (char)'\x0') {
			str[y] = '\x0';
			break;
		}

		/* alpha-numeric characters and spaces don't get encoded */
		else if (((char)input[x] == (char)' ') || ((char)input[x] >= '0' && (char)input[x] <= '9') || ((char)input[x] >= 'A' && (char)input[x] <= 'Z') || ((char)input[x] >= (char)'a' && (char)input[x] <= (char)'z'))
			str[y++] = input[x];

		/* newlines turn to <BR> tags */
		else if (escape_newlines == TRUE && (char)input[x] == (char)'\n') {
			strcpy(&str[y], "<BR>");
			y += 4;
		} else if (escape_newlines == TRUE && (char)input[x] == (char)'\\' && (char)input[x + 1] == (char)'n') {
			strcpy(&str[y], "<BR>");
			y += 4;
			x++;
		}

		/* TODO - strip all but allowed HTML tags out... */

		else if ((char)input[x] == (char)'<') {

			if (escape_html_tags == FALSE)
				str[y++] = input[x];
			else {
				str[y] = '\x0';
				if ((int)strlen(str) < (output_len - 4)) {
					strcat(str, "&lt;");
					y += 4;
				}
			}
		}

		else if ((char)input[x] == (char)'>') {

			if (escape_html_tags == FALSE)
				str[y++] = input[x];
			else {
				str[y] = '\x0';
				if ((int)strlen(str) < (output_len - 4)) {
					strcat(str, "&gt;");
					y += 4;
				}
			}
		}

		/* high bit chars don't get encoded, so we won't be breaking utf8 characters */
		else if ((unsigned char)input[x] >= 0x7f)
			str[y++] = input[x];

		/* for simplicity, all other chars represented by their numeric value */
		else {
			if (escape_html_tags == FALSE)
				str[y++] = input[x];
			else {
				str[y] = '\x0';
				sprintf(temp_expansion, "&#%d;", (unsigned char)input[x]);
				if ((int)strlen(str) < (output_len - strlen(temp_expansion))) {
					strcat(str, temp_expansion);
					y += strlen(temp_expansion);
				}
			}
		}
	}

	str[y++] = '\x0';

	num_encoded_html = (num_encoded_html == 0) ? 1 : 0;

	return str;
}

/* strip > and < from string */
void strip_html_brackets(char *buffer) {
	register int x;
	register int y;
	register int z;

	if (buffer == NULL || buffer[0] == '\x0')
		return;

	/* remove all occurances in string */
	z = (int)strlen(buffer);
	for (x = 0, y = 0; x < z; x++) {
		if (buffer[x] == '<' || buffer[x] == '>')
			continue;
		buffer[y++] = buffer[x];
	}
	buffer[y++] = '\x0';

	return;
}

/* escape string for html form usage */
char * escape_string(char *input) {
	int len, output_len;
	int x, y;
	char temp_expansion[10];
	static int num_encoded_html = 0;
	char* str = encoded_html_string[num_encoded_html];

	/* initialize return string */
	strcpy(str, "");

	if (input == NULL)
		return str;

	len = (int)strlen(input);
	output_len = (int)sizeof(encoded_html_string[num_encoded_html]);

	str[0] = '\x0';

	for (x = 0, y = 0; x <= len && y < output_len - 1; x++) {

		/* end of string */
		if ((char)input[x] == (char)'\x0') {
			str[y] = '\x0';
			break;
		}

		/* alpha-numeric characters don't get encoded */
		else if (((char)input[x] >= '0' && (char)input[x] <= '9') || ((char)input[x] >= 'A' && (char)input[x] <= 'Z') || ((char)input[x] >= (char)'a' && (char)input[x] <= (char)'z'))
			str[y++] = input[x];

		/* spaces, hyphens, periods, underscores and colons don't get encoded */
		else if (((char)input[x] == (char)' ') || ((char)input[x] == (char)'-') || ((char)input[x] == (char)'.') || ((char)input[x] == (char)'_') || ((char)input[x] == (char)':'))
			str[y++] = input[x];

		/* high bit characters don't get encoded */
		else if ((unsigned char)input[x] >= 0x7f)
			str[y++] = input[x];

		/* for simplicity, all other chars represented by their numeric value */
		else {
			str[y] = '\x0';
			sprintf(temp_expansion, "&#%d;", (unsigned char)input[x]);
			if ((int)strlen(str) < (output_len - strlen(temp_expansion))) {
				strcat(str, temp_expansion);
				y += strlen(temp_expansion);
			}
		}
	}

	str[y++] = '\x0';

	num_encoded_html = (num_encoded_html == 0) ? 1 : 0;

	return str;
}


/**********************************************************
 *************** COMMON HTML FUNCTIONS ********************
 **********************************************************/

void display_info_table(char *title, authdata *current_authdata, int daemon_check) {
	char date_time[MAX_DATETIME_LENGTH];
	char *dir_to_check = NULL;
	time_t current_time;
	int result;
	int x, last = 0, dummy;

	/* read program status */
	result = read_all_status_data(get_cgi_config_location(), READ_PROGRAM_STATUS);

	printf("<TABLE CLASS='infoBox' BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD CLASS='infoBox' nowrap>\n");
	printf("<DIV CLASS='infoBoxTitle'>%s</DIV>\n", title);

	time(&current_time);
	get_time_string(&current_time, date_time, (int)sizeof(date_time), LONG_DATE_TIME);

	printf("Last Updated: %s ", date_time);

	/* display only if refresh is supported */
	if (CGI_ID == EXTINFO_CGI_ID || CGI_ID == OUTAGES_CGI_ID || CGI_ID == STATUS_CGI_ID || CGI_ID == STATUSMAP_CGI_ID || CGI_ID == TAC_CGI_ID) {
		if (CGI_ID == STATUS_CGI_ID && display_status_totals == TRUE)
			printf("<BR>");
		else
			printf("- ");
		if (refresh_type == JAVASCRIPT_REFRESH)
			printf("<span id='refresh_text'>Refresh done......</span>&nbsp;<small><a href='#' onClick='icinga_toggle_refresh(); return false;'><span id='refresh_button'></span></a> <a href='#' onClick='icinga_do_refresh(); return false;'><img src='%s%s' border=0 style='margin-bottom:-2px;'></a></small>\n", url_images_path, RELOAD_ICON);
		else
			printf("Update every %d seconds\n", refresh_rate);
	}

	printf("<br><A HREF='http://www.icinga.org' TARGET='_new' CLASS='homepageURL'>%s %s</A> -\n", PROGRAM_NAME, PROGRAM_VERSION);

	if (current_authdata != NULL)
		printf("Logged in as <i>%s</i>\n", (!strcmp(current_authdata->username, "")) ? "?" : current_authdata->username);

	/* add here every cgi_id which uses logging, this should limit the testing of write access to the necessary amount */
	if (use_logging == TRUE && CGI_ID == CMD_CGI_ID) {

		dummy = asprintf(&dir_to_check, "%s", cgi_log_file);

		for (x = 0; x <= (int)strlen(dir_to_check); x++) {
			/* end of string */
			if ((char)dir_to_check[x] == (char)'\x0') {
				break;
			}
			if ((char)dir_to_check[x] == '/')
				last = x;
		}
		dir_to_check[last] = '\x0';

		if (!strcmp(cgi_log_file, "")) {
			printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: Logging is activated but no logfile is configured</DIV>");
		} else {
			if (access(dir_to_check, W_OK) != 0)
				printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: No permission to write logfile to %s</DIV>", dir_to_check);
		}
		if (cgi_log_rotation_method != LOG_ROTATION_NONE) {
			if (!strcmp(cgi_log_archive_path, ""))
				printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: Log rotation is configured but option \"cgi_log_archive_path\" isn't</DIV>");
			else {
				if (access(cgi_log_archive_path, W_OK) != 0)
					printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: No permission to write to \"cgi_log_archive_path\": %s</DIV>", cgi_log_archive_path);
			}
		}
		free(dir_to_check);
	}

	if (nagios_process_state != STATE_OK)
		printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: Monitoring process may not be running!<BR>Click <A HREF='%s?type=%d'>here</A> for more info.</DIV>", EXTINFO_CGI, DISPLAY_PROCESS_INFO);

	if (result == ERROR && daemon_check == TRUE)
		printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: Could not read program status information!</DIV>");

	else {
		if (enable_notifications == FALSE)
			printf("<DIV CLASS='infoBoxBadProcStatus'>- Notifications are disabled</DIV>");

		if (execute_service_checks == FALSE)
			printf("<DIV CLASS='infoBoxBadProcStatus'>- Service checks are disabled</DIV>");
	}

	if (CGI_ID == CONFIG_CGI_ID && is_authorized_for_full_command_resolution(current_authdata)) {
		if (access(resource_file, R_OK) != 0)
			printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: Could not read resource file, raw command line could be incomplete!</DIV>");
	}

	/* must have missed 2 update intervals */
	if (status_file_creation_time < (current_time - (2 * status_update_interval)))
		printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: Status data OUTDATED! Last status data update was %d seconds ago!</DIV>", (int)(current_time - status_file_creation_time));

	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	return;
}

void display_nav_table(time_t ts_start, time_t ts_end) {
	char *temp_buffer;
	char url[MAX_INPUT_BUFFER] = "";
	char stripped_query_string[MAX_INPUT_BUFFER] = "";
	char date_time[MAX_INPUT_BUFFER];
	struct tm *t;
	time_t ts_midnight = 0L;
	time_t current_time = 0L;

	/* define base url */
	switch (CGI_ID) {
	case HISTORY_CGI_ID:
		strcat(url, HISTORY_CGI);
		break;
	case NOTIFICATIONS_CGI_ID:
		strcat(url, NOTIFICATIONS_CGI);
		break;
	case SHOWLOG_CGI_ID:
		strcat(url, SHOWLOG_CGI);
		break;
	default:
		strcat(url, "NO_URL_DEFINED");
		break;
	}

	/* get url options but filter out "limit" and "status" */
	if (getenv("QUERY_STRING") != NULL && strcmp(getenv("QUERY_STRING"), "")) {
		strcpy(stripped_query_string, getenv("QUERY_STRING"));
		strip_html_brackets(stripped_query_string);

		for (temp_buffer = my_strtok(stripped_query_string, "&"); temp_buffer != NULL; temp_buffer = my_strtok(NULL, "&")) {
			if (strncmp(temp_buffer, "ts_start=", 9) != 0 && strncmp(temp_buffer, "ts_end=", 6) != 0 && strncmp(temp_buffer, "start=", 6) != 0) {
				if (strstr(url, "?"))
					strcat(url, "&");
				else
					strcat(url, "?");
				strcat(url, temp_buffer);
			}
		}
	}

	/* get the current time */
	time(&current_time);
	t = localtime(&current_time);

	t->tm_sec = 0;
	t->tm_min = 0;
	t->tm_hour = 0;
	t->tm_isdst = -1;

	/* get timestamp for midnight today to find out if we have to show past log entries or present. (Also to give the right description to the info table)*/
	ts_midnight = mktime(t);

	/* show table */
	printf("<table border=0 cellspacing=0 cellpadding=0 CLASS='navBox'>\n");
	printf("<tr>\n");
	printf("<td align=center valign=center CLASS='navBoxItem'>\n");
	if (ts_end > ts_midnight) {
		printf("Latest Archive<br>");
		printf("<a href='%s%sts_start=%lu&ts_end=%lu'><img src='%s%s' border=0 alt='Latest Archive' title='Latest Archive'></a>", url, (strstr(url, "?")) ? "&" : "?", ts_midnight - 86400, ts_midnight - 1, url_images_path, LEFT_ARROW_ICON);
	} else {
		printf("Earlier Archive<br>");
		printf("<a href='%s%sts_start=%lu&ts_end=%lu'><img src='%s%s' border=0 alt='Earlier Archive' title='Earlier Archive'></a>", url, (strstr(url, "?")) ? "&" : "?", ts_start - 86400, ts_start - 1, url_images_path, LEFT_ARROW_ICON);
	}
	printf("</td>\n");

	printf("<td width=15></td>\n");

	printf("<td align=center CLASS='navBoxDate'>\n");
	printf("<DIV CLASS='navBoxTitle'>Log Navigation</DIV>\n");
	get_time_string(&ts_start, date_time, (int)sizeof(date_time), LONG_DATE_TIME);
	printf("%s", date_time);
	printf("<br>to<br>");
	if (ts_end > ts_midnight)
		printf("Present..");
	else {
		get_time_string(&ts_end, date_time, (int)sizeof(date_time), LONG_DATE_TIME);
		printf("%s", date_time);
	}
	printf("</td>\n");

	printf("<td width=15></td>\n");

	if (ts_end <= ts_midnight) {

		printf("<td align=center valign=center CLASS='navBoxItem'>\n");
		if (ts_end == ts_midnight) {
			printf("Current Log<br>");
			printf("<a href='%s%sts_start=%lu&ts_end=%lu'><img src='%s%s' border=0 alt='Current Log' title='Current Log'></a>", url, (strstr(url, "?")) ? "&" : "?", ts_midnight + 1, ts_midnight + 86400, url_images_path, RIGHT_ARROW_ICON);
		} else {
			printf("More Recent Archive<br>");
			printf("<a href='%s%sts_start=%lu&ts_end=%lu'><img src='%s%s' border=0 alt='More Recent Archive' title='More Recent Archive'></a>", url, (strstr(url, "?")) ? "&" : "?", ts_end + 1, ts_end + 86400, url_images_path, RIGHT_ARROW_ICON);
		}
		printf("</td>\n");
	} else
		printf("<td><img src='%s%s' border=0 width=75 height=1></td>\n", url_images_path, EMPTY_ICON);

	printf("</tr>\n");

	printf("</table>\n");

	return;
}

/* prints the additional notes or action url for a hostgroup (with macros substituted) */
void print_extra_hostgroup_url(char *group_name, char *url) {
	char input_buffer[MAX_INPUT_BUFFER] = "";
	char output_buffer[MAX_INPUT_BUFFER] = "";
	char *temp_buffer;
	int in_macro = FALSE;
	hostgroup *temp_hostgroup = NULL;

	if (group_name == NULL || url == NULL)
		return;

	temp_hostgroup = find_hostgroup(group_name);
	if (temp_hostgroup == NULL) {
		printf("%s", url);
		return;
	}

	strncpy(input_buffer, url, sizeof(input_buffer) - 1);
	input_buffer[sizeof(input_buffer) - 1] = '\x0';

	for (temp_buffer = my_strtok(input_buffer, "$"); temp_buffer != NULL; temp_buffer = my_strtok(NULL, "$")) {

		if (in_macro == FALSE) {
			if (strlen(output_buffer) + strlen(temp_buffer) < sizeof(output_buffer) - 1) {
				strncat(output_buffer, temp_buffer, sizeof(output_buffer) - strlen(output_buffer) - 1);
				output_buffer[sizeof(output_buffer) - 1] = '\x0';
			}
			in_macro = TRUE;
		} else {

			if (strlen(output_buffer) + strlen(temp_buffer) < sizeof(output_buffer) - 1) {

				if (!strcmp(temp_buffer, "HOSTGROUPNAME"))
					strncat(output_buffer, url_encode(temp_hostgroup->group_name), sizeof(output_buffer) - strlen(output_buffer) - 1);
			}

			in_macro = FALSE;
		}
	}

	printf("%s", output_buffer);

	return;
}

/* prints the additional notes or action url for a servicegroup (with macros substituted) */
void print_extra_servicegroup_url(char *group_name, char *url) {
	char input_buffer[MAX_INPUT_BUFFER] = "";
	char output_buffer[MAX_INPUT_BUFFER] = "";
	char *temp_buffer;
	int in_macro = FALSE;
	servicegroup *temp_servicegroup = NULL;

	if (group_name == NULL || url == NULL)
		return;

	temp_servicegroup = find_servicegroup(group_name);
	if (temp_servicegroup == NULL) {
		printf("%s", url);
		return;
	}

	strncpy(input_buffer, url, sizeof(input_buffer) - 1);
	input_buffer[sizeof(input_buffer) - 1] = '\x0';

	for (temp_buffer = my_strtok(input_buffer, "$"); temp_buffer != NULL; temp_buffer = my_strtok(NULL, "$")) {

		if (in_macro == FALSE) {
			if (strlen(output_buffer) + strlen(temp_buffer) < sizeof(output_buffer) - 1) {
				strncat(output_buffer, temp_buffer, sizeof(output_buffer) - strlen(output_buffer) - 1);
				output_buffer[sizeof(output_buffer) - 1] = '\x0';
			}
			in_macro = TRUE;
		} else {

			if (strlen(output_buffer) + strlen(temp_buffer) < sizeof(output_buffer) - 1) {

				if (!strcmp(temp_buffer, "SERVICEGROUPNAME"))
					strncat(output_buffer, url_encode(temp_servicegroup->group_name), sizeof(output_buffer) - strlen(output_buffer) - 1);
			}

			in_macro = FALSE;
		}
	}

	printf("%s", output_buffer);

	return;
}

/* include user-defined SSI footers or headers */
void include_ssi_files(char *cgi_name, int type) {
	char common_ssi_file[MAX_INPUT_BUFFER];
	char cgi_ssi_file[MAX_INPUT_BUFFER];
	char raw_cgi_name[MAX_INPUT_BUFFER];
	char *stripped_cgi_name;

	/* common header or footer */
	snprintf(common_ssi_file, sizeof(common_ssi_file) - 1, "%scommon-%s.ssi", physical_ssi_path, (type == SSI_HEADER) ? "header" : "footer");
	common_ssi_file[sizeof(common_ssi_file) - 1] = '\x0';

	/* CGI-specific header or footer */
	strncpy(raw_cgi_name, cgi_name, sizeof(raw_cgi_name) - 1);
	raw_cgi_name[sizeof(raw_cgi_name) - 1] = '\x0';
	stripped_cgi_name = strtok(raw_cgi_name, ".");
	snprintf(cgi_ssi_file, sizeof(cgi_ssi_file) - 1, "%s%s-%s.ssi", physical_ssi_path, (stripped_cgi_name == NULL) ? "" : stripped_cgi_name, (type == SSI_HEADER) ? "header" : "footer");
	cgi_ssi_file[sizeof(cgi_ssi_file) - 1] = '\x0';

	if (type == SSI_HEADER) {
		printf("\n<!-- Produced by %s (http://www.%s.org).\nCopyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)\nCopyright (c) 2009-2012 Icinga Development Team -->\n", PROGRAM_NAME, PROGRAM_NAME_LC);
		include_ssi_file(common_ssi_file);
		include_ssi_file(cgi_ssi_file);
	} else {
		include_ssi_file(cgi_ssi_file);
		include_ssi_file(common_ssi_file);
		printf("\n<!-- Produced by %s (http://www.%s.org).\nCopyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)\nCopyright (c) 2009-2012 Icinga Development Team -->\n", PROGRAM_NAME, PROGRAM_NAME_LC);
	}

	return;
}

/* include user-defined SSI footer or header */
void include_ssi_file(char *filename) {
	char buffer[MAX_INPUT_BUFFER];
	FILE *fp;
	struct stat stat_result;
	int call_return;

	/* if file is executable, we want to run it rather than print it */
	call_return = stat(filename, &stat_result);

	/* file is executable */
	if (call_return == 0 && (stat_result.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {

		/* must flush output stream first so that output
		   from script shows up in correct place. Other choice
		   is to open program under pipe and copy the data from
		   the program to our output stream.
		*/
		fflush(stdout);

		/* ignore return status from system call. */
		call_return = system(filename);

		return;
	}

	/* an error occurred trying to stat() the file */
	else if (call_return != 0) {

		/* Handle error conditions. Assume that standard posix error codes and errno are available. If not, comment this section out. */
		switch (errno) {
		case ENOTDIR: /* - A component of the path is not a directory. */
		case ELOOP: /* Too many symbolic links encountered while traversing the path. */
		case EFAULT: /* Bad address. */
		case ENOMEM: /* Out of memory (i.e. kernel memory). */
		case ENAMETOOLONG: /* File name too long. */
			printf("<br> A stat call returned %d while looking for the file %s.<br>", errno, filename);
			return;
		case EACCES: /* Permission denied. -- The file should be accessible by nagios. */
			printf("<br> A stat call returned a permissions error(%d) while looking for the file %s.<br>", errno, filename);
			return;
		case ENOENT: /* A component of the path file_name does not exist, or the path is an empty string. Just return if the file doesn't exist. */
			return;
		default:
			return;
		}
	}

	fp = fopen(filename, "r");
	if (fp == NULL)
		return;

	/* print all lines in the SSI file */
	while (fgets(buffer, (int)(sizeof(buffer) - 1), fp) != NULL)
		printf("%s", buffer);

	fclose(fp);

	return;
}

/* displays an error if CGI config file could not be read */
void cgi_config_file_error(char *config_file) {

	printf("<H1>Whoops!</H1>\n");

	printf("<P><STRONG><FONT COLOR='RED'>Error: Could not open CGI config file '%s' for reading!</FONT></STRONG></P>\n", config_file);

	printf("<P>\n");
	printf("Here are some things you should check in order to resolve this error:\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("<OL>\n");

	printf("<LI>Make sure you've installed a CGI config file in its proper location.  A sample CGI configuration file (named <b>cgi.cfg</b>) can be found in the <b>sample-config/</b> subdirectory of the %s source code distribution.\n", PROGRAM_NAME);
	printf("<LI>Make sure the user your web server is running as has permission to read the CGI config file.\n");

	printf("</OL>\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("Make sure you read the documentation on installing and configuring %s thoroughly before continuing.  If everything else fails, try sending a message to one of the mailing lists.  More information can be found at <a href='http://www.icinga.org'>http://www.icinga.org</a>.\n", PROGRAM_NAME);
	printf("</P>\n");

	return;
}

/* displays an error if main config file could not be read */
void main_config_file_error(char *config_file) {

	printf("<H1>Whoops!</H1>\n");

	printf("<P><STRONG><FONT COLOR='RED'>Error: Could not open main config file '%s' for reading!</FONT></STRONG></P>\n", config_file);

	printf("<P>\n");
	printf("Here are some things you should check in order to resolve this error:\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("<OL>\n");

	printf("<LI>Make sure you've installed a main config file in its proper location. A sample main configuration file (named <b>icinga.cfg</b>) can be found in the <b>sample-config/</b> subdirectory of the %s source code distribution.\n", PROGRAM_NAME);
	printf("<LI>Make sure the user your web server is running as has permission to read the main config file.\n");

	printf("</OL>\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("Make sure you read the documentation on installing and configuring %s thoroughly before continuing.  If everything else fails, try sending a message to one of the mailing lists.  More information can be found at <a href='http://www.icinga.org'>http://www.icinga.org</a>.\n", PROGRAM_NAME);
	printf("</P>\n");

	return;
}

/* displays an error if resource file could not be read */
void icinga_resource_file_error(char *config_file) {

	printf("<H1>Whoops!</H1>\n");

	printf("<P><STRONG><FONT COLOR='RED'>Error: Could not open resource file '%s' for reading!</FONT></STRONG></P>\n", config_file);

	printf("<P>\n");
	printf("It seems that you enabled the cgis to read your local resource file (verify that in your cgi.cfg)\n");
	printf("Here are some things you should check in order to resolve this error:\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("<OL>\n");

	printf("<LI>Make sure you've installed the resource file in its proper location, defined in main config. A sample resource file (named <b>resource.cfg</b>) can be found in the <b>sample-config/</b> subdirectory of the %s source code distribution.\n", PROGRAM_NAME);
	printf("<LI>Make sure the user your web server is running as has permission to read the resource file.\n");
	printf("<LI>If you don't want to read your resource file (used e.g. for command expander in config.cgi) then disable it in cgi.cfg.\n");

	printf("</OL>\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("Make sure you read the documentation on installing and configuring %s thoroughly before continuing.  If everything else fails, try sending a message to one of the mailing lists.  More information can be found at <a href='http://www.icinga.org'>http://www.icinga.org</a>.\n", PROGRAM_NAME);
	printf("</P>\n");

	return;
}

/* displays an error if object data could not be read */
void object_data_error(void) {

	printf("<H1>Whoops!</H1>\n");

	printf("<P><STRONG><FONT COLOR='RED'>Error: Could not read object configuration data!</FONT></STRONG></P>\n");

	printf("<P>\n");
	printf("Here are some things you should check in order to resolve this error:\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("<OL>\n");

	printf("<LI>Verify configuration options using the <b>-v</b> command-line option to check for errors.\n");
	printf("<LI>Check the %s log file for messages relating to startup or status data errors.\n", PROGRAM_NAME);

	printf("</OL>\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("Make sure you read the documentation on installing, configuring and running %s thoroughly before continuing.  If everything else fails, try sending a message to one of the mailing lists.  More information can be found at <a href='http://www.icinga.org'>http://www.icinga.org</a>.\n", PROGRAM_NAME);
	printf("</P>\n");

	return;
}

/* displays an error if status data could not be read */
void status_data_error(void) {

	printf("<H1>Whoops!</H1>\n");

	printf("<P><STRONG><FONT COLOR='RED'>Error: Could not read host and service status information!</FONT></STRONG></P>\n");

	printf("<P>\n");
	printf("It seems that %s is not running or has not yet finished the startup procedure and then creating the status data file. If %s is indeed not running, this is a normal error message.\n", PROGRAM_NAME, PROGRAM_NAME);
	printf("Please note that event broker modules and/or rdbms backends may slow down the overall (re)start and the cgis cannot retrieve any status information.");
	printf("</P>\n");

	printf("<P>\n");
	printf("Things to check in order to resolve this error include:\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("<OL>\n");

	printf("<LI>Check the %s log file for messages relating to startup or status data errors.\n", PROGRAM_NAME);
	printf("<LI>Always verify configuration options using the <b>-v</b> command-line option before starting or restarting %s!\n", PROGRAM_NAME);
	printf("<LI>If using any event broker module for %s, look into their respective logs and/or on their behavior!\n", PROGRAM_NAME);

	printf("</OL>\n");
	printf("</P>\n");

	printf("<P>\n");
	printf("Make sure you read the documentation on installing, configuring and running %s thoroughly before continuing.  If everything else fails, try sending a message to one of the mailing lists.  More information can be found at <a href='http://www.icinga.org'>http://www.icinga.org</a>.\n", PROGRAM_NAME);
	printf("</P>\n");

	return;
}

/** print an error depending on error_type */
void print_error(char *config_file, int error_type) {

	/* if cgi.cfg is missing, we don't know which fancy style to use, take our own */
	if (error_type != ERROR_CGI_CFG_FILE) {
		document_header(ERROR_CGI_ID, TRUE, "Error");
	}

	/* Giving credits to stop.png image source */
	printf("\n<!-- Image \"stop.png\" has been taken from \"http://fedoraproject.org/wiki/Template:Admon/caution\" -->\n\n");

	printf("<BR><DIV align='center'><DIV CLASS='errorBox'>\n");
	if (error_type == ERROR_CGI_CFG_FILE) {
		printf("<DIV style='font-family:  Helvetica, serif; background-color: #fff; color: #000; font-size: 8pt; text-align:left; font-weight: bold; margin:1em; border:1px red solid; background-color: #FFE5E5;' CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55 valign=top></td>");
	} else {
		printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55 valign=top><img src=\"%s%s\" border=0></td>", url_images_path, CMD_STOP_ICON);
	}
	printf("<td class='errorDescription'>");

	switch (error_type) {
	case ERROR_CGI_STATUS_DATA:
		status_data_error();
		break;
	case ERROR_CGI_OBJECT_DATA:
		object_data_error();
		break;
	case ERROR_CGI_CFG_FILE:
		cgi_config_file_error(config_file);
		break;
	case ERROR_CGI_MAIN_CFG:
		main_config_file_error(config_file);
		break;
	case ERROR_CGI_RESOURCE_CFG:
		icinga_resource_file_error(config_file);
		break;
	}

	printf("</td></tr></table></DIV>\n");
	printf("</DIV>\n");

	return;
}

void display_splunk_host_url(host *hst) {

	if (enable_splunk_integration == FALSE)
		return;
	if (hst == NULL)
		return;

	printf("<a href='%s?q=search %s' target='_blank'><img src='%s%s' alt='Splunk It' title='Splunk It' border='0'></a>\n", splunk_url, url_encode(hst->name), url_images_path, SPLUNK_SMALL_WHITE_ICON);

	return;
}

void display_splunk_service_url(service *svc) {

	if (enable_splunk_integration == FALSE)
		return;
	if (svc == NULL)
		return;

	printf("<a href='%s?q=search %s%%20", splunk_url, url_encode(svc->host_name));
	printf("%s' target='_blank'><img src='%s%s' alt='Splunk It' title='Splunk It' border='0'></a>\n", url_encode(svc->description), url_images_path, SPLUNK_SMALL_WHITE_ICON);

	return;
}

void display_splunk_generic_url(char *buf, int icon) {
	char *newbuf = NULL;

	if (enable_splunk_integration == FALSE)
		return;
	if (buf == NULL)
		return;

	if ((newbuf = (char *)strdup(buf)) == NULL)
		return;

	strip_splunk_query_terms(newbuf);

	printf("<a href='%s?q=search %s' target='_blank'>", splunk_url, url_encode(newbuf));

	if (icon > 0)
		printf("<img src='%s%s' alt='Splunk It' title='Splunk It' border='0'>", url_images_path, (icon == 1) ? SPLUNK_SMALL_WHITE_ICON : SPLUNK_SMALL_BLACK_ICON);
	printf("</a>\n");

	free(newbuf);

	return;
}

/* strip quotes and from string */
void strip_splunk_query_terms(char *buffer) {
	register int x;
	register int y;
	register int z;

	if (buffer == NULL || buffer[0] == '\x0')
		return;

	/* remove all occurances in string */
	z = (int)strlen(buffer);
	for (x = 0, y = 0; x < z; x++) {
		if (buffer[x] == '\'' || buffer[x] == '\"' || buffer[x] == ';' || buffer[x] == ':' || buffer[x] == ',' || buffer[x] == '-' || buffer[x] == '=')
			buffer[y++] = ' ';
		else
			buffer[y++] = buffer[x];
	}
	buffer[y++] = '\x0';

	return;
}

void print_generic_error_message(char *title, char *text, int returnlevels) {

	if (content_type == CSV_CONTENT) {
		if (title != NULL && title[0] != '\x0')
			printf("ERROR: %s\n", title);
		if (text != NULL && text[0] != '\x0')
			printf("ERROR: %s\n", text);
	} else if (content_type == JSON_CONTENT) {
		printf("\"error\": {\n\"title\": ");
		if (title != NULL && title[0] != '\x0')
			printf("\"%s\",\n", title);
		else
			printf("null,\n");
		printf("\"text\": ");
		if (text != NULL && text[0] != '\x0')
			printf("\"%s\"\n}", text);
		else
			printf("null\n}");
	} else {
		printf("<BR><DIV align='center'><DIV CLASS='errorBox'>\n");
		printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>", url_images_path, CMD_STOP_ICON);

		if (title != NULL && title[0] != '\x0')
			printf("<td CLASS='errorMessage'>%s</td></tr></table></DIV>\n", title);

		if (text != NULL && text[0] != '\x0')
			printf("<DIV CLASS='errorDescription'>%s</DIV><br>", text);

		printf("</DIV>\n");

		if (returnlevels != 0)
			printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-%d);' class='submitButton'>\n", returnlevels);

		printf("</DIV><BR>\n");
	}

	return;
}

/** @brief prints export link with little icons
 *  @param [in] content_type can be \c CSV_CONTENT , \c JSON_CONTENT , \c XML_CONTENT or \c HTML_CONTENT
 *  @param [in] cgi name of cgi as defined in include/cgiutils.h
 *  @param [in] add_to_url is additional string to add to url (set to NULL if nothing should be added)
 *  @note takes care that each link is XSS save #1275
 *
 *  This function prints a little icon, depending on @ref content_type, which points to
 *  a new page with the desired content.
**/
void print_export_link(int content_type, char *cgi, char *add_to_url) {
	char stripped_query_string[MAX_INPUT_BUFFER] = "";
	char link[MAX_INPUT_BUFFER] = "";

	if (cgi == NULL)
		return;

	strcat(link, cgi);

	/* just do stuff if some options are requested */
	if (getenv("QUERY_STRING") != NULL && strcmp(getenv("QUERY_STRING"), "")) {
		strcpy(stripped_query_string, getenv("QUERY_STRING"));
		strip_html_brackets(stripped_query_string);
		strcat(link, "?");
		strcat(link, stripped_query_string);
	}

	/* add string to url */
	if (add_to_url != NULL && (strlen(add_to_url) != 0)) {
		if (strlen(stripped_query_string) != 0)
			strcat(link, "&");
		else
			strcat(link, "?");
		strcat(link, add_to_url);
	}

	/* print formatted link */
	if (content_type == CSV_CONTENT)
		printf("<a href='%s%scsvoutput' target='_blank'><img src='%s%s' style='vertical-align: middle;' border=0 alt='%s' title='%s'></a>\n", link, (strlen(stripped_query_string) != 0) ? "&" : "?", url_images_path, EXPORT_CSV_ICON, EXPORT_CSV_ICON_ALT, EXPORT_CSV_ICON_ALT);
	else if (content_type == JSON_CONTENT)
		printf("<a href='%s%sjsonoutput' target='_blank'><img src='%s%s' style='vertical-align: middle;' border=0 alt='%s' title='%s'></a>\n", link, (strlen(stripped_query_string) != 0) ? "&" : "?", url_images_path, EXPORT_JSON_ICON, EXPORT_JSON_ICON_ALT, EXPORT_JSON_ICON_ALT);
	else if (content_type == XML_CONTENT)
		printf("<a href='%s%sxmloutput' target='_blank'><img src='%s%s' style='vertical-align: middle;' border=0 alt='%s' title='%s'></a>\n", link, (strlen(stripped_query_string) != 0) ? "&" : "?", url_images_path, EXPORT_XML_ICON, EXPORT_XML_ICON_ALT, EXPORT_XML_ICON_ALT);
	else
		printf("<a href='%s' target='_blank'><img src='%s%s' style='vertical-align: middle;' border=0 alt='%s' title='%s'></a>\n", link, url_images_path, EXPORT_LINK_ICON, EXPORT_LINK_ICON_ALT, EXPORT_LINK_ICON_ALT);

	return;
}


/**
 * Logging and file functions
**/

int write_to_cgi_log(char *buffer) {
	FILE *fp;
	time_t log_time;
	int write_retries = 10, i = 0;

	/* we don't do anything if logging is deactivated or no logfile configured */
	if (use_logging == FALSE || !strcmp(cgi_log_file, ""))
		return OK;

	time(&log_time);

	// allways check if log file has to be rotated
	rotate_cgi_log_file();

	// open log file and try again if failed
	while ((fp = fopen(cgi_log_file, "a+")) == NULL && i < write_retries) {
		usleep(10);
		i++;
	}

	if (i >= write_retries)
		return ERROR;

	/* strip any newlines from the end of the buffer */
	strip(buffer);

	/* write the buffer to the log file */
	fprintf(fp, "[%lu] %s\n", log_time, buffer);

	fclose(fp);

	return OK;
}

/* rotates the cgi log file */
int rotate_cgi_log_file() {
	char temp_buffer[MAX_INPUT_BUFFER] = "";
	char method_string[16] = "";
	char *log_archive = NULL;
	struct tm *ts;
	int rename_result = 0;
	int stat_result = -1;
	struct stat log_file_stat;
	int dummy, sub = 0, weekday;
	time_t current_time, rotate_ts;

	/* if there is no log arhive configured we don't do anything */
	if (!strcmp(cgi_log_archive_path, ""))
		return ERROR;

	/* get the current time */
	time(&current_time);

	ts = localtime(&current_time);

	ts->tm_sec = 0;
	ts->tm_min = 0;
	ts->tm_isdst = -1;

	weekday = ts->tm_wday;
	/* implement start of week (Sunday/Monday) as config option
	weekday=ts->tm_wday;
	weekday--;
	if (weekday==-1)
		weekday=6;
	*/

	if (cgi_log_rotation_method == LOG_ROTATION_NONE)
		return OK;
	else if (cgi_log_rotation_method == LOG_ROTATION_HOURLY)
		strcpy(method_string, "HOURLY");
	else if (cgi_log_rotation_method == LOG_ROTATION_DAILY) {
		strcpy(method_string, "DAILY");
		ts->tm_hour = 0;
	} else if (cgi_log_rotation_method == LOG_ROTATION_WEEKLY) {
		strcpy(method_string, "WEEKLY");
		ts->tm_hour = 0;
		sub = (60 * 60 * 24 * weekday);
	} else if (cgi_log_rotation_method == LOG_ROTATION_MONTHLY) {
		strcpy(method_string, "MONTHLY");
		ts->tm_hour = 0;
		ts->tm_mday = 1;
	} else
		return ERROR;

	// determine the timestamp for next rotation
	rotate_ts = (time_t)(mktime(ts) - sub);

	/* get stats of current log file */
	stat_result = stat(cgi_log_file, &log_file_stat);

	// timestamp for rotation hasn't passed. don't rotate log file
	if (rotate_ts < log_file_stat.st_atime)
		return OK;

	// from here on file gets rotated.

	/* get the archived filename to use */
	dummy = asprintf(&log_archive, "%s/icinga-cgi-%02d-%02d-%d-%02d.log", cgi_log_archive_path, ts->tm_mon + 1, ts->tm_mday, ts->tm_year + 1900, ts->tm_hour);

	/* rotate the log file */
	rename_result = my_rename(cgi_log_file, log_archive);

	if (rename_result) {
		my_free(log_archive);
		return ERROR;
	}

	/* record the log rotation after it has been done... */
	snprintf(temp_buffer, sizeof(temp_buffer) - 1, "LOG ROTATION: %s\n", method_string);
	temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
	write_to_cgi_log(temp_buffer);

	/* give a warning about use */
	write_to_cgi_log("This log is highly experimental and changes may occure without notice. Use at your own risk!!");

	if (stat_result == 0) {
		chmod(cgi_log_file, log_file_stat.st_mode);
		dummy = chown(cgi_log_file, log_file_stat.st_uid, log_file_stat.st_gid);
	}

	my_free(log_archive);

	return OK;
}

/* renames a file - works across filesystems (Mike Wiacek) */
int my_rename(char *source, char *dest) {
	int rename_result = 0;


	/* make sure we have something */
	if (source == NULL || dest == NULL)
		return -1;

	/* first see if we can rename file with standard function */
	rename_result = rename(source, dest);

	/* handle any errors... */
	if (rename_result == -1) {

		/* an error occurred because the source and dest files are on different filesystems */
		if (errno == EXDEV) {

			/* try copying the file */
			if (my_fcopy(source, dest) == ERROR) {
				logit(NSLOG_RUNTIME_ERROR, TRUE, "Error: Unable to rename file '%s' to '%s': %s\n", source, dest, strerror(errno));
				return -1;
			}

			/* delete the original file */
			unlink(source);

			/* reset result since we successfully copied file */
			rename_result = 0;
		}

		/* some other error occurred */
		else {
			logit(NSLOG_RUNTIME_ERROR, TRUE, "Error: Unable to rename file '%s' to '%s': %s\n", source, dest, strerror(errno));
			return rename_result;
		}
	}

	return rename_result;
}

/* copies a file */
int my_fcopy(char *source, char *dest) {
	int dest_fd, result;

	/* make sure we have something */
	if (source == NULL || dest == NULL)
		return ERROR;

	/* unlink destination file first (not doing so can cause problems on network file systems like CIFS) */
	unlink(dest);

	/* open destination file for writing */
	if ((dest_fd = open(dest, O_WRONLY | O_TRUNC | O_CREAT | O_APPEND, 0644)) < 0) {
		logit(NSLOG_RUNTIME_ERROR, TRUE, "Error: Unable to open file '%s' for writing: %s\n", dest, strerror(errno));
		return ERROR;
	}

	result = my_fdcopy(source, dest, dest_fd);
	close(dest_fd);
	return result;
}

/*
 * copy a file from the path at source to the already opened
 * destination file dest.
 * This is handy when creating tempfiles with mkstemp()
 */
int my_fdcopy(char *source, char *dest, int dest_fd) {
	int source_fd, rd_result = 0, wr_result = 0;
	unsigned long tot_written = 0, tot_read = 0, buf_size = 0;
	struct stat st;
	char *buf;

	/* open source file for reading */
	if ((source_fd = open(source, O_RDONLY, 0644)) < 0) {
		logit(NSLOG_RUNTIME_ERROR, TRUE, "Error: Unable to open file '%s' for reading: %s\n", source, strerror(errno));
		return ERROR;
	}

	/*
	 * find out how large the source-file is so we can be sure
	 * we've written all of it
	 */
	if (fstat(source_fd, &st) < 0) {
		logit(NSLOG_RUNTIME_ERROR, TRUE, "Error: Unable to stat source file '%s' for my_fcopy(): %s\n", source, strerror(errno));
		close(source_fd);
		return ERROR;
	}

	/*
	 * If the file is huge, read it and write it in chunks.
	 * This value (128K) is the result of "pick-one-at-random"
	 * with some minimal testing and may not be optimal for all
	 * hardware setups, but it should work ok for most. It's
	 * faster than 1K buffers and 1M buffers, so change at your
	 * own peril. Note that it's useful to make it fit in the L2
	 * cache, so larger isn't necessarily better.
	 */
	buf_size = st.st_size > 128 << 10 ? 128 << 10 : st.st_size;
	buf = malloc(buf_size);
	if (!buf) {
		logit(NSLOG_RUNTIME_ERROR, TRUE, "Error: Unable to malloc(%lu) bytes: %s\n", buf_size, strerror(errno));
		close(source_fd);
		return ERROR;
	}
	/* most of the times, this loop will be gone through once */
	while (tot_written < st.st_size) {
		int loop_wr = 0;

		rd_result = read(source_fd, buf, buf_size);
		if (rd_result < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			logit(NSLOG_RUNTIME_ERROR, TRUE, "Error: my_fcopy() failed to read from '%s': %s\n", source, strerror(errno));
			break;
		}
		tot_read += rd_result;

		while (loop_wr < rd_result) {
			wr_result = write(dest_fd, buf + loop_wr, rd_result - loop_wr);

			if (wr_result < 0) {
				if (errno == EAGAIN || errno == EINTR)
					continue;
				logit(NSLOG_RUNTIME_ERROR, TRUE, "Error: my_fcopy() failed to write to '%s': %s\n", dest, strerror(errno));
				break;
			}
			loop_wr += wr_result;
		}
		if (wr_result < 0)
			break;
		tot_written += loop_wr;
	}

	/*
	 * clean up irregardless of how things went. dest_fd comes from
	 * our caller, so we mustn't close it.
	 */
	close(source_fd);
	free(buf);

	if (rd_result < 0 || wr_result < 0) {
		/* don't leave half-written files around */
		unlink(dest);
		return ERROR;
	}

	return OK;
}

/* Checks if the given time is in daylight time saving period */
int is_dlst_time(time_t *time) {
	struct tm *bt = localtime(time);
	return bt->tm_isdst;
}

/* convert timeperiodes to timestamps */
void convert_timeperiod_to_times(int type, time_t *ts_start, time_t *ts_end) {
	time_t current_time;
	int weekday = 0;
	struct tm *t;

	/* get the current time */
	time(&current_time);

	/* everything before start of unix time is invalid */
	if ((unsigned long int)ts_start > (unsigned long int)current_time)
		*ts_start = 0L;

	t = localtime(&current_time);

	t->tm_sec = 0;
	t->tm_min = 0;
	t->tm_hour = 0;
	t->tm_isdst = -1;

	/* see if weeks starts on sunday or monday */
	weekday = t->tm_wday;
	if (week_starts_on_monday == TRUE) {
		weekday--;
		if (weekday == -1)
			weekday = 6;
	}

	switch (type) {
	case TIMEPERIOD_LAST24HOURS:
		*ts_start = current_time - (60 * 60 * 24);
		*ts_end = current_time;
		break;
	case TIMEPERIOD_TODAY:
		*ts_start = mktime(t);
		*ts_end = current_time;
		break;
	case TIMEPERIOD_SINGLE_DAY:
		if (*ts_start == 0L && *ts_end == 0L) {
			*ts_start = mktime(t);
			*ts_end = current_time;
		}
		break;
	case TIMEPERIOD_YESTERDAY:
		*ts_start = (time_t)(mktime(t) - (60 * 60 * 24));
		*ts_end = (time_t)mktime(t) - 1;
		break;
	case TIMEPERIOD_THISWEEK:
		*ts_start = (time_t)(mktime(t) - (60 * 60 * 24 * weekday));
		*ts_end = current_time;
		break;
	case TIMEPERIOD_LASTWEEK:
		t->tm_wday--;
		*ts_start = (time_t)(mktime(t) - (60 * 60 * 24 * weekday) - (60 * 60 * 24 * 7));
		*ts_end = (time_t)(mktime(t) - (60 * 60 * 24 * weekday) - 1);
		break;
	case TIMEPERIOD_THISMONTH:
		t->tm_mday = 1;
		*ts_start = mktime(t);
		*ts_end = current_time;
		break;
	case TIMEPERIOD_LASTMONTH:
		t->tm_mday = 1;
		*ts_end = mktime(t) - 1;
		if (t->tm_mon == 0) {
			t->tm_mon = 11;
			t->tm_year--;
		} else
			t->tm_mon--;
		*ts_start = mktime(t);
		break;
	case TIMEPERIOD_THISYEAR:
		t->tm_mon = 0;
		t->tm_mday = 1;
		*ts_start = mktime(t);
		*ts_end = current_time;
		break;
	case TIMEPERIOD_LASTYEAR:
		t->tm_mon = 0;
		t->tm_mday = 1;
		*ts_end = mktime(t) - 1;
		t->tm_year--;
		*ts_start = mktime(t);
		break;
	case TIMEPERIOD_LAST7DAYS:
		*ts_start = (time_t)(mktime(t) - (60 * 60 * 24 * 7));
		*ts_end = current_time;
		break;
	case TIMEPERIOD_LAST31DAYS:
		*ts_start = (time_t)(mktime(t) - (60 * 60 * 24 * 31));
		*ts_end = current_time;
		break;
	case TIMEPERIOD_THISQUARTER:
		/* not implemented */
		break;
	case TIMEPERIOD_LASTQUARTER:
		/* not implemented */
		break;
	case TIMEPERIOD_NEXTPROBLEM:
		/* Time period will be defined later */
		/* only used in trends.cgi */
		break;
	default:
		break;
	}

	/* check if interval is across dlst change and adjust timestamps with compensation */
	if (is_dlst_time(ts_start) != is_dlst_time(ts_end)) {
		t = localtime(&current_time);
		/* in DST */
		if (t->tm_isdst == 1) {
			/* if end is also in DST */
			if (is_dlst_time(ts_end) == 1)
				*ts_start = *ts_start + 3600;
		} else {
			if (is_dlst_time(ts_end) == 0)
				*ts_start = *ts_start - 3600;
		}

	}

	return;
}

/* converts a time string to a UNIX timestamp, respecting the date_format option */
int string_to_time(char *buffer, time_t *t) {
	struct tm lt;
	int ret = 0;

	/* Initialize some variables just in case they don't get parsed
	   by the sscanf() call.  A better solution is to also check the
	   CGI input for validity, but this should suffice to prevent
	   strange problems if the input is not valid.
	   Jan 15 2003	Steve Bonds */
	lt.tm_mon = 0;
	lt.tm_mday = 1;
	lt.tm_year = 1900;
	lt.tm_hour = 0;
	lt.tm_min = 0;
	lt.tm_sec = 0;
	lt.tm_wday = 0;
	lt.tm_yday = 0;


	if (date_format == DATE_FORMAT_EURO)
		ret = sscanf(buffer, "%02d-%02d-%04d %02d:%02d:%02d", &lt.tm_mday, &lt.tm_mon, &lt.tm_year, &lt.tm_hour, &lt.tm_min, &lt.tm_sec);
	else if (date_format == DATE_FORMAT_ISO8601 || date_format == DATE_FORMAT_STRICT_ISO8601)
		ret = sscanf(buffer, "%04d-%02d-%02d%*[ T]%02d:%02d:%02d", &lt.tm_year, &lt.tm_mon, &lt.tm_mday, &lt.tm_hour, &lt.tm_min, &lt.tm_sec);
	else
		ret = sscanf(buffer, "%02d-%02d-%04d %02d:%02d:%02d", &lt.tm_mon, &lt.tm_mday, &lt.tm_year, &lt.tm_hour, &lt.tm_min, &lt.tm_sec);

	if (ret != 6)
		return ERROR;

	lt.tm_mon--;
	lt.tm_year -= 1900;

	/* tell mktime() to try and compute DST automatically */
	lt.tm_isdst = -1;

	*t = mktime(&lt);

	return OK;
}

char *json_encode(char *input) {
	char *encoded_string;
	int len = 0;
	int i, j;

	/* we need up to twice the space to do the conversion */
	len = (int)strlen(input);
	if ((encoded_string = (char *)malloc(len * 2 + 1)) == NULL)
		return "";

	for (i = 0, j = 0; i < len; i++) {

		/* escape quotes */
		if ((char)input[i] == (char)'"') {
			encoded_string[j++] = '\\';
			encoded_string[j++] = input[i];

			/* escape newlines */
		} else if ((char)input[i] == (char)'\n') {
			encoded_string[j++] = '\\';
			encoded_string[j++] = 'n';

		} else
			encoded_string[j++] = input[i];
	}

	encoded_string[j] = '\x0';

	return encoded_string;
}

/******************************************************************/
/*********  print a tooltip to show comments  *********************/
/******************************************************************/
void print_comment_icon(char *host_name, char *svc_description) {
	comment *temp_comment = NULL;
	char *comment_entry_type = "";
	char comment_data[MAX_INPUT_BUFFER] = "";
	char entry_time[MAX_DATETIME_LENGTH];
	int len, output_len;
	int x, y;
	char *escaped_output_string = NULL;
	int saved_escape_html_tags_var = FALSE;

	if (svc_description == NULL)
		printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s'", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(host_name));
	else {
		printf("<TD ALIGN=center valign=center><A HREF='%s?type=%d&host=%s", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(host_name));
		printf("&service=%s#comments'", url_encode(svc_description));
	}
	/* possible to implement a config option to show and hide comments tooltip in status.cgi */
	/* but who wouldn't like to have these fancy tooltips ;-) */
	if (TRUE) {
		printf(" onMouseOver=\"return tooltip('<table border=0 width=100%% height=100%% cellpadding=3>");
		printf("<tr style=font-weight:bold;><td width=10%% nowrap>Type&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td><td width=12%%>Time</td><td>Author / Comment</td></tr>");
		for (temp_comment = get_first_comment_by_host(host_name); temp_comment != NULL; temp_comment = get_next_comment_by_host(host_name, temp_comment)) {
			if ((svc_description == NULL && temp_comment->comment_type == HOST_COMMENT) || \
			        (svc_description != NULL && temp_comment->comment_type == SERVICE_COMMENT && !strcmp(temp_comment->service_description, svc_description))) {
				switch (temp_comment->entry_type) {
				case USER_COMMENT:
					comment_entry_type = "User";
					break;
				case DOWNTIME_COMMENT:
					comment_entry_type = "Downtime";
					break;
				case FLAPPING_COMMENT:
					comment_entry_type = "Flapping";
					break;
				case ACKNOWLEDGEMENT_COMMENT:
					comment_entry_type = "Ack";
					break;
				}
				snprintf(comment_data, sizeof(comment_data) - 1, "%s", temp_comment->comment_data);
				comment_data[sizeof(comment_data) - 1] = '\x0';

				/* we need up to twice the space to do the conversion of single, double quotes and back slash's */
				len = (int)strlen(comment_data);
				output_len = len * 2;
				if ((escaped_output_string = (char *)malloc(output_len + 1)) != NULL) {

					strcpy(escaped_output_string, "");

					for (x = 0, y = 0; x <= len; x++) {
						/* end of string */
						if ((char)comment_data[x] == (char)'\x0') {
							escaped_output_string[y] = '\x0';
							break;
						} else if ((char)comment_data[x] == (char)'\n' || (char)comment_data[x] == (char)'\r') {
							escaped_output_string[y] = ' ';
						} else if ((char)comment_data[x] == (char)'\'') {
							escaped_output_string[y] = '\x0';
							if ((int)strlen(escaped_output_string) < (output_len - 2)) {
								strcat(escaped_output_string, "\\'");
								y += 2;
							}
						} else if ((char)comment_data[x] == (char)'"') {
							escaped_output_string[y] = '\x0';
							if ((int)strlen(escaped_output_string) < (output_len - 2)) {
								strcat(escaped_output_string, "\\\"");
								y += 2;
							}
						} else if ((char)comment_data[x] == (char)'\\') {
							escaped_output_string[y] = '\x0';
							if ((int)strlen(escaped_output_string) < (output_len - 2)) {
								strcat(escaped_output_string, "\\\\");
								y += 2;
							}
						} else
							escaped_output_string[y++] = comment_data[x];

					}
					escaped_output_string[++y] = '\x0';
				} else
					strcpy(escaped_output_string, comment_data);

				/* get entry time */
				get_time_string(&temp_comment->entry_time, entry_time, (int)sizeof(entry_time), SHORT_DATE_TIME);

				/* in the tooltips we have to escape all characters */
				saved_escape_html_tags_var = escape_html_tags;
				escape_html_tags = TRUE;

				printf("<tr><td nowrap>%s</td><td nowrap>%s</td><td><span style=font-weight:bold;>%s</span><br>%s</td></tr>", comment_entry_type, entry_time, html_encode(temp_comment->author, TRUE), html_encode(escaped_output_string, TRUE));

				escape_html_tags = saved_escape_html_tags_var;

				free(escaped_output_string);
			}
		}
		/* under http://www.ebrueggeman.com/skinnytip/documentation.php#reference you can find the config options of skinnytip */
		printf("</table>', '&nbsp;&nbsp;&nbsp;Comments', 'border:1, width:600, bordercolor:#333399, title_padding:2px, titletextcolor:#FFFFFF, backcolor:#CCCCFF');\" onMouseOut=\"return hideTip()\"");
	}
	printf("><IMG SRC='%s%s' BORDER=0 WIDTH=%d HEIGHT=%d></A></TD>", url_images_path, COMMENT_ICON, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT);

	return;
}

/** @brief prints modified attributes as string, by line seperator
 *  @param [in] content_type can be \c CSV_CONTENT , \c JSON_CONTENT , \c XML_CONTENT or \c HTML_CONTENT
 *  @param [in] cgi name of cgi as defined in include/cgiutils.h
 *  @param [in] modified_attributes is the number to compare with
 *  @note takes care that modified_attributes is represented as string
 *
 *  This function prints modified_attributes as string
 *
#define MODATTR_NONE                            0
#define MODATTR_NOTIFICATIONS_ENABLED           1
#define MODATTR_ACTIVE_CHECKS_ENABLED           2
#define MODATTR_PASSIVE_CHECKS_ENABLED          4
#define MODATTR_EVENT_HANDLER_ENABLED           8
#define MODATTR_FLAP_DETECTION_ENABLED          16
#define MODATTR_FAILURE_PREDICTION_ENABLED      32
#define MODATTR_PERFORMANCE_DATA_ENABLED        64
#define MODATTR_OBSESSIVE_HANDLER_ENABLED       128
#define MODATTR_EVENT_HANDLER_COMMAND           256
#define MODATTR_CHECK_COMMAND                   512
#define MODATTR_NORMAL_CHECK_INTERVAL           1024
#define MODATTR_RETRY_CHECK_INTERVAL            2048
#define MODATTR_MAX_CHECK_ATTEMPTS              4096
#define MODATTR_FRESHNESS_CHECKS_ENABLED        8192
#define MODATTR_CHECK_TIMEPERIOD                16384
#define MODATTR_CUSTOM_VARIABLE                 32768
#define MODATTR_NOTIFICATION_TIMEPERIOD         65536
 *
**/
void print_modified_attributes(int content_type, char *cgi, unsigned long modified_attributes) {
	char attr[MAX_INPUT_BUFFER] = "";

	if (cgi == NULL)
		return;

	if (modified_attributes == MODATTR_NONE) {
		/* nothing modified, return early */
		printf("None");
		return;
	}

	/* loop until no more attributes matched */
	while (modified_attributes != MODATTR_NONE) {
		if (modified_attributes & MODATTR_NOTIFICATIONS_ENABLED) {
			strcat(attr, "notifications_enabled");
			modified_attributes -= MODATTR_NOTIFICATIONS_ENABLED;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_ACTIVE_CHECKS_ENABLED) {
			strcat(attr, "active_checks_enabled");
			modified_attributes -= MODATTR_ACTIVE_CHECKS_ENABLED;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_PASSIVE_CHECKS_ENABLED) {
			strcat(attr, "passive_checks_enabled");
			modified_attributes -= MODATTR_PASSIVE_CHECKS_ENABLED;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_EVENT_HANDLER_ENABLED) {
			strcat(attr, "event_handler_enabled");
			modified_attributes -= MODATTR_EVENT_HANDLER_ENABLED;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_FLAP_DETECTION_ENABLED) {
			strcat(attr, "flap_detection_enabled");
			modified_attributes -= MODATTR_FLAP_DETECTION_ENABLED;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_FAILURE_PREDICTION_ENABLED) {
			strcat(attr, "failure_prediction_enabled");
			modified_attributes -= MODATTR_FAILURE_PREDICTION_ENABLED;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_PERFORMANCE_DATA_ENABLED) {
			strcat(attr, "performance_data_enabled");
			modified_attributes -= MODATTR_PERFORMANCE_DATA_ENABLED;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_OBSESSIVE_HANDLER_ENABLED) {
			strcat(attr, "obsessive_handler_enabled");
			modified_attributes -= MODATTR_OBSESSIVE_HANDLER_ENABLED;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_EVENT_HANDLER_COMMAND) {
			strcat(attr, "event_handler_command");
			modified_attributes -= MODATTR_EVENT_HANDLER_COMMAND;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_CHECK_COMMAND) {
			strcat(attr, "check_command");
			modified_attributes -= MODATTR_CHECK_COMMAND;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_NORMAL_CHECK_INTERVAL) {
			strcat(attr, "check_interval");
			modified_attributes -= MODATTR_NORMAL_CHECK_INTERVAL;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_RETRY_CHECK_INTERVAL) {
			strcat(attr, "retry_interval");
			modified_attributes -= MODATTR_RETRY_CHECK_INTERVAL;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_MAX_CHECK_ATTEMPTS) {
			strcat(attr, "max_check_attemps");
			modified_attributes -= MODATTR_MAX_CHECK_ATTEMPTS;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_FRESHNESS_CHECKS_ENABLED) {
			strcat(attr, "freshness_checks_enabled");
			modified_attributes -= MODATTR_FRESHNESS_CHECKS_ENABLED;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_CHECK_TIMEPERIOD) {
			strcat(attr, "check_timeperiod");
			modified_attributes -= MODATTR_CHECK_TIMEPERIOD;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_CUSTOM_VARIABLE) {
			strcat(attr, "custom_variable");
			modified_attributes -= MODATTR_CUSTOM_VARIABLE;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
		if (modified_attributes & MODATTR_NOTIFICATION_TIMEPERIOD) {
			strcat(attr, "Notification Timeperiod");
			modified_attributes -= MODATTR_NOTIFICATION_TIMEPERIOD;
			if (modified_attributes != MODATTR_NONE)
				strcat(attr, ", ");
		}
	}

	if (content_type == HTML_CONTENT) {
		printf("<div class=\"serviceWARNING\">%s</div>", attr);
	} else if (content_type == JSON_CONTENT) {
		printf("%s", attr);
	}
	return;
}

/******************************************************************/
/*******************  pagination functions ************************/
/******************************************************************/
void page_num_selector(int result_start, int total_entries, int displayed_entries) {

	char link[MAX_INPUT_BUFFER] = "";
	char stripped_query_string[MAX_INPUT_BUFFER] = "";
	char *temp_buffer;
	int total_pages = 1;
	int current_page = 1;
	int next_page = 0;
	int previous_page = 0;
	int display_from = 0;
	int display_to = 0;

	/* define base url */
	switch (CGI_ID) {
	/* not used in this case, cause status.cgi has a own page number selector function */
	// case STATUS_CGI_ID:
	// 	strcat(link, STATUS_CGI);
	// 	break;
	case CONFIG_CGI_ID:
		strcat(link, CONFIG_CGI);
		break;
	case EXTINFO_CGI_ID:
		strcat(link, EXTINFO_CGI);
		break;
	case HISTORY_CGI_ID:
		strcat(link, HISTORY_CGI);
		break;
	case NOTIFICATIONS_CGI_ID:
		strcat(link, NOTIFICATIONS_CGI);
		break;
	case SHOWLOG_CGI_ID:
		strcat(link, SHOWLOG_CGI);
		break;
	default:
		strcat(link, "NO_URL_DEFINED");
		break;
	}

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
	if (result_limit > 0 && total_entries > 0) {
		total_pages = (total_entries / result_limit);

		if ((total_entries % result_limit) != 0)
			total_pages++;

		current_page = (result_start / result_limit) + 1;
		previous_page = (result_start - result_limit) > 0 ? (result_start - result_limit) : 0;
		next_page = (result_start + result_limit) > total_entries ? result_start : (result_start + result_limit);
	}

	/* links page select elements and counters */
	printf("<div class='page_selector'>\n");
	printf("<div id='page_navigation' class='page_select_dd'>");

	if (current_page != 1 || (result_limit != 0 && result_start != 1))
		printf("<a href='%s%sstart=1&limit=%d' title='First Page'><img src='%s%s' style='vertical-align: middle;' height='16' width='16' alt='<<'></a>\n", link, (strstr(link, "?")) ? "&" : "?", result_limit, url_images_path, FIRST_PAGE_ACTIVE_ICON);
	else
		printf("<img src='%s%s' style='vertical-align: middle;' height='16' width='16'>\n", url_images_path, FIRST_PAGE_INACTIVE_ICON);

	if (current_page != 1)
		printf("<a href='%s%sstart=%d&limit=%d' title='Previous Page'><img src='%s%s' style='vertical-align: middle;' height='16' width='16' alt='<'></a>\n", link, (strstr(link, "?")) ? "&" : "?", previous_page, result_limit, url_images_path, PREVIOUS_PAGE_ACTIVE_ICON);
	else
		printf("<img src='%s%s' style='vertical-align: middle;' height='16' width='16'>\n", url_images_path, PREVIOUS_PAGE_INACTIVE_ICON);

	printf("<span style='vertical-align:middle; font-size:8pt;'> Page </span>");

	/* with inline javascript to send new page on "Enter" */
	printf("<input type='text' value='%d' style='width:30px; vertical-align:middle; border:1px #D0D0D0 solid;text-align:center; font-size:8pt;'", current_page);
	printf("onkeydown='if (event.keyCode == 13) window.location.href = \"%s\" + \"%slimit=%d&start=\" + (((this.value -1) * %d) + 1) ;'>", link, (strstr(link, "?")) ? "&" : "?", result_limit, result_limit);

	printf("<span style='vertical-align:middle; font-size:8pt;'> of %d </span>", total_pages);

	if (current_page != total_pages) {
		printf("<a href='%s%sstart=%d&limit=%d' title='Next Page'><img src='%s%s' style='vertical-align: middle;' height='16' width='16' alt='>'></a>\n", link, (strstr(link, "?")) ? "&" : "?", (result_start + result_limit), result_limit, url_images_path, NEXT_PAGE_ACTIVE_ICON);
		printf("<a href='%s%sstart=%d&limit=%d' title='Last Page'><img src='%s%s' style='vertical-align: middle;' height='16' width='16' alt='>>'></a>\n", link, (strstr(link, "?")) ? "&" : "?", ((total_pages - 1)*result_limit) + 1, result_limit, url_images_path, LAST_PAGE_ACTIVE_ICON);
	} else
		printf("<img src='%s%s' style='vertical-align: middle;' height='16' width='16'><img src='%s%s' style='vertical-align: middle;' height='16' width='16'>\n", url_images_path, NEXT_PAGE_INACTIVE_ICON, url_images_path, LAST_PAGE_INACTIVE_ICON);

	printf("</div>\n");
	page_limit_selector(result_start);
	printf("</div>\n");

	/* calculating the displayed reults */
	if (result_start > total_entries || displayed_entries == 0) {
		display_from = 0;
		display_to = 0;
	} else {
		display_from = result_start;
		display_to = result_start + displayed_entries - 1;
	}

	printf("<div style='text-align:center;padding-top:6px;font-size:8pt;'>Displaying Result %d - %d of %d Matching Results</div>\n", display_from, display_to, total_entries);

	/* copy page navigation to top of the page */
	printf("<script language='javascript' type='text/javascript'>\n");
	printf("$(document).ready(function() { \n");
	printf("$('#page_navigation').clone(true).appendTo('#page_navigation_copy');\n");
	printf("});\n");
	printf("</script>\n");

	return;
}

void page_limit_selector(int result_start) {

	static int id = 0;	// gets every dropdown a single id to activate msdropdown
	char link[MAX_INPUT_BUFFER] = "";
	char stripped_query_string[MAX_INPUT_BUFFER] = "";
	char *temp_buffer;

	/* define base url */
	switch (CGI_ID) {
	case STATUS_CGI_ID:
		strcat(link, STATUS_CGI);
		break;
	case CONFIG_CGI_ID:
		strcat(link, CONFIG_CGI);
		break;
	case EXTINFO_CGI_ID:
		strcat(link, EXTINFO_CGI);
		break;
	case HISTORY_CGI_ID:
		strcat(link, HISTORY_CGI);
		break;
	case NOTIFICATIONS_CGI_ID:
		strcat(link, NOTIFICATIONS_CGI);
		break;
	case SHOWLOG_CGI_ID:
		strcat(link, SHOWLOG_CGI);
		break;
	default:
		strcat(link, "NO_URL_DEFINED");
		break;
	}

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

	/* display drop down menu to select result limit */
	printf("<div class='page_select_dd'>\n");
	printf("<select style='display:none; vertical-align:middle; width:140px;' name='limit' id='limit_dd_%d' class='result_limit_dd' onChange='if (this.value) window.location.href = \"%s\" + \"%slimit=\" + this.value ", id, link, (strstr(link, "?")) ? "&" : "?");
	if (result_start != 0)
		printf("+ \"&start=%d\"", result_start);
	printf(";'>\n");

	if (result_limit == 0)
		printf("<option>Results: All</option>\n");
	else
		printf("<option>Results: %d</option>\n", result_limit);

	printf("<option value='50'>50</option>\n");
	printf("<option value='100'>100</option>\n");
	printf("<option value='250'>250</option>\n");
	printf("<option value='1000'>1000</option>\n");
	printf("<option value='0'>All</option>\n");

	printf("</select>\n");

	/* Print out the activator for the dropdown (which must be between the body tags */
	printf("<script language='javascript' type='text/javascript'>\n");
	printf("$(document).ready(function() { \n");
	printf("try { \n$(\"#limit_dd_%d\").msDropDown({visibleRows:6}).data(\"dd\").visible(true);\n", id);
	printf("} catch(e) {\n");
	printf("if (console) { console.log(e); }\n}\n");
	printf("});\n");
	printf("</script>\n");

	printf("</div>\n");

	id++;

	return;
}
