/**************************************************************************
 *
 * CMD.C - Icinga Command CGI
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *************************************************************************/

/** @file cmd.c
 *  @brief submits commands to Icinga command pipe
**/


#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/comments.h"
#include "../include/downtime.h"
#include "../include/statusdata.h"

#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/getcgi.h"

/** @name External vars
    @{ **/
extern const char *extcmd_get_name(int id);

extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_html_path[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];
extern char command_file[MAX_FILENAME_LENGTH];
extern char comment_file[MAX_FILENAME_LENGTH];

extern int  check_external_commands;
extern int  use_authentication;
extern int  lock_author_names;
extern int  persistent_ack_comments;
extern int  default_expiring_acknowledgement_duration;
extern int  default_expiring_disabled_notifications_duration;

extern int  display_header;
extern int  daemon_check;

extern int enforce_comments_on_actions;
extern int date_format;
extern int use_logging;
extern int default_downtime_duration;

extern scheduled_downtime *scheduled_downtime_list;
extern comment *comment_list;
/** @} */

/** @name LIMITS
 @{**/
#define MAX_AUTHOR_LENGTH		64
#define MAX_COMMENT_LENGTH		1024
#define NUMBER_OF_STRUCTS		((MAX_CGI_INPUT_PAIRS*2)+100)		/**< Depends on amount of MAX_CGI_INPUT_PAIRS */
/** @}*/

/** @name ELEMET TEMPLATE TYPES
 @{**/
#define PRINT_COMMON_HEADER		1
#define PRINT_AUTHOR			2
#define PRINT_STICKY_ACK		3
#define PRINT_PERSISTENT		4
#define PRINT_SEND_NOTFICATION		5
#define PRINT_COMMENT_BOX		6
#define PRINT_NOTIFICATION_DELAY	7
#define PRINT_START_TIME		8
#define PRINT_END_TIME			9
#define PRINT_CHECK_TIME		10
#define PRINT_FORCE_CHECK		11
#define PRINT_CHECK_OUTPUT_BOX		12
#define PRINT_PERFORMANCE_DATA_BOX	13
#define PRINT_FIXED_FLEXIBLE_TYPE	14
#define PRINT_BROADCAST_NOTIFICATION	15
#define PRINT_FORCE_NOTIFICATION	16
#define PRINT_EXPIRE_ACKNOWLEDGEMENT	17
/** @}*/

/** @name OBJECT LIST TYPES
 @{**/
#define PRINT_HOST_LIST			17
#define PRINT_SERVICE_LIST		18
#define PRINT_COMMENT_LIST		19
#define PRINT_DOWNTIME_LIST		20
/** @}*/

/** @name NOTIFICATION LIST TYPES
 @{**/
#define PRINT_EXPIRE_DISABLE_NOTIFICATIONS	21
/** @}*/

/** @brief host/service list structure
 *
 *  Struct to hold information of hosts and services for batch processing
**/
struct hostlist {
	char *host_name;
	char *description;
};

/** @brief error list structure
 *
 *  hold the errors we find during processing of @ref commit_command_data
**/
struct errorlist {
	char *message;
};


/** @name Internal vars
    @{ **/
char *host_name = "";				/**< requested host name */
char *hostgroup_name = "";			/**< requested hostgroup name */
char *servicegroup_name = "";			/**< requested servicegroup name */
char *service_desc = "";				/**< requested service name */
char *comment_author = "";			/**< submitted comment author */
char *comment_data = "";				/**< submitted comment data */
char *start_time_string = "";			/**< the requested start time */
char *end_time_string = "";			/**< the requested end time */

char help_text[MAX_INPUT_BUFFER] = "";		/**< help string */
char plugin_output[MAX_INPUT_BUFFER] = "";	/**< plugin output text for passive submitted check */
char performance_data[MAX_INPUT_BUFFER] = "";	/**< plugin performance data for passive submitted check */

int notification_delay = 0;			/**< delay for submitted notification in minutes */
int schedule_delay = 0;				/**< delay for sheduled actions in minutes (Icinga restart, Notfications enable/disable)
							!not implemented in GUI! */
int persistent_comment = FALSE;			/**< bool if omment should survive Icinga restart */
int sticky_ack = TRUE;				/**< bool to disable notifications until recover */
int send_notification = TRUE;			/**< bool sends a notification if service gets acknowledged */
int use_ack_end_time = FALSE;			/**< bool if expire acknowledgement is selected or not */
int use_disabled_notif_end_time = FALSE;	/**< bool if expire disabled notifications is selected or not */
int force_check = FALSE;			/**< bool if check should be forced */
int plugin_state = STATE_OK;			/**< plugin state for passive submitted check */
int affect_host_and_services = FALSE;		/**< bool if notifiactions or else affect all host and services */
int propagate_to_children = FALSE;		/**< bool if en/disable host notifications should propagated to children */
int fixed = FALSE;				/**< bool if downtime is fixed or flexible */
unsigned long duration = 0L;			/**< downtime duration */
unsigned long triggered_by = 0L;		/**< downtime id which triggers submited downtime */
int child_options = 0;				/**< if downtime should trigger child host downtimes */
int force_notification = 0;			/**< force a notification to be send out through event handler */
int broadcast_notification = 0;			/**< this options determines if notification should be broadcasted */

int command_type = CMD_NONE;			/**< the requested command ID */
int command_mode = CMDMODE_REQUEST;		/**< if command mode is request or commit */

time_t start_time = 0L;				/**< start time as unix timestamp */
time_t end_time = 0L;				/**< end time as unix timestamp */

int CGI_ID = CMD_CGI_ID;				/**< ID to identify the cgi for functions in cgiutils.c */

unsigned long attr = MODATTR_NONE;		/**< default modified_attributes */

authdata current_authdata;			/**< struct to hold current authentication data */

/** Initialize the struct */
struct hostlist commands[NUMBER_OF_STRUCTS];

/** initialze the error list */
struct errorlist error[NUMBER_OF_STRUCTS];

/** Hold IDs of comments and downtimes */
unsigned long multi_ids[NUMBER_OF_STRUCTS];

/** store the authentication status when data gets checked to submited */
short is_authorized[NUMBER_OF_STRUCTS];

/** store the result of each object which get submited */
short submit_result[NUMBER_OF_STRUCTS];
/** @} */


/** @brief Print form for all details to submit command
 *  @param [in] cmd ID of requested command
 *
 *  This function generates the form for the command with all requested
 *  host/services/downtimes/comments items. This is the first page you get
 *  when you submit a command.
**/
void request_command_data(int);

/** @brief submits the command data and checks for sanity
 *  @param [in] cmd ID of requested command
 *
 *  This function checks the submitted data (@ref request_command_data)
 *  for sanity. If everything is alright it passes the data to @ref commit_command.
**/
void commit_command_data(int);

/** @brief checks the authorization and passes the data to cmd_submitf
 *  @param [in] cmd ID of requested command
 *  @retval OK
 *  @retval ERROR
 *  @return success / fail
 *
 *  Here the command get formatted properly to be readable by icinga
 *  core. It passes the data to @c cmd_submitf .
**/
int commit_command(int);

/** @brief write the command to Icinga command pipe
 *  @param [in] cmd the formatted command string
 *  @retval OK
 *  @retval ERROR
 *  @return success / fail
 *
 *  This function actually writes the formatted string into Icinga command pipe.
 *  And if configured also to Icinga CGI log.
**/
int write_command_to_file(char *);

/** @brief strips out semicolons and newlines from comment data
 *  @param [in,out] buffer the stringt which should be cleaned
 *
 *  Converts semicolons, newline and carriage return to space.
**/
void clean_comment_data(char *);

/** @brief strips out semicolons and newlines from comment data
 *  @param [in] element ID of the element which should be printed
 *  @param [in] cmd ID of requested command
 *
 *  These are templates for the different form elements. Specify
 *  the element you want to print with element id.
**/
void print_form_element(int, int);

/** @brief print the list of affected objects
 *  @param [in] list_type ID of the item list which should be printed
 *
 *  Used to print the list of requested objects. Depending on the command
 *  you can specify the list (HOST/SERVICE/COMMENT/DOWNTIME).
**/
void print_object_list(int);

/** @brief print the mouseover box with help text
 *  @param [in] content string which should be printed as help box
 *
 *  This writes the mousover help box.
**/
void print_help_box(char *);

/** @brief checks start and end time and if start_time is before end_time
 *  @param [in] e the error element list
 *
 *  Checks if author or comment is empty. If so it adds an error to error list.
**/
void check_comment_sanity(int*);

/** @brief checks if comment and author are not empty strings
 *  @param [in] e the error element list
 *
 *  Checks the sanity of given start and end time. Checks if time is
 *  wrong or start_time is past end_time then if found an error it
 *  adds an error to error list.
**/
void check_time_sanity(int*);

/** @brief Parses the requested GET/POST variables
 *  @retval TRUE
 *  @retval FALSE
 *  @return wether parsing was successful or not
 *
 *  @n This function parses the request and set's the necessary variables
**/
int process_cgivars(void);


/** @brief Yes we need a main function **/
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

	/* read environment var ICINGA_COMMAND_FILE */
	strcpy(command_file, get_cmd_file_location());

	/* This requires the date_format parameter in the main config file */
	if (strcmp(start_time_string, ""))
		string_to_time(start_time_string, &start_time);

	if (strcmp(end_time_string, ""))
		string_to_time(end_time_string, &end_time);


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


	document_header(CGI_ID, TRUE, "External Command Interface");

	/* get authentication information */
	get_authentication_information(&current_authdata);

	if (display_header == TRUE) {

		/* Giving credits to stop.png image source */
		printf("\n<!-- Image \"stop.png\" has been taken from \"http://fedoraproject.org/wiki/Template:Admon/caution\" -->\n\n");

		/* begin top table */
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=33%%>\n");
		display_info_table("External Command Interface", &current_authdata, daemon_check);
		printf("</td>\n");

		/* center column of the first row */
		printf("<td align=center valign=top width=33%%>\n");
		printf("</td>\n");

		/* right column of the first row */
		printf("<td align=right valign=bottom width=33%%>\n");
		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");
	}

	/* if no command was specified... */
	if (command_type == CMD_NONE) {
		print_generic_error_message("Error: No command was specified!", NULL, 2);
	}

	/* if not authorized to perform commands*/
	else if (is_authorized_for_read_only(&current_authdata) == TRUE) {
		print_generic_error_message("Error: It appears as though you do not have permission to perform any commands!", NULL, 1);
	}

	/* if this is the first request for a command, present option */
	else if (command_mode == CMDMODE_REQUEST)
		request_command_data(command_type);

	/* the user wants to commit the command */
	else if (command_mode == CMDMODE_COMMIT)
		commit_command_data(command_type);

	document_footer(CGI_ID);

	/* free allocated memory */
	free_memory();
	free_object_data();

	return OK;
}

int process_cgivars(void) {
	char **variables;
	char *temp_buffer = NULL;
	int error = FALSE;
	int x;
	int z = 0;

	variables = getcgivars();

	/* Process the variables */
	for (x = 0; variables[x] != NULL; x++) {

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if (strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
			x++;
			continue;
		}

		/* we found the command type */
		else if (!strcmp(variables[x], "cmd_typ")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			command_type = atoi(variables[x]);
		}

                /* we found the attr */
                else if (!strcmp(variables[x], "attr")) {
                        x++;
                        if (variables[x] == NULL) {
                                error = TRUE;
                                break;
                        }

                        attr = strtoul(variables[x], NULL, 10);
                }

		/* we found the command mode */
		else if (!strcmp(variables[x], "cmd_mod")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			command_mode = atoi(variables[x]);
		}

		/* we found a comment id or a downtime id*/
		else if (!strcmp(variables[x], "com_id") || !strcmp(variables[x], "down_id")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			multi_ids[z] = strtoul(variables[x], NULL, 10);
			z++;
		}

		/* we found the notification delay */
		else if (!strcmp(variables[x], "not_dly")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			notification_delay = atoi(variables[x]);
		}

		/* we found the schedule delay */
		else if (!strcmp(variables[x], "sched_dly")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			schedule_delay = atoi(variables[x]);
		}

		/* we found the comment author */
		else if (!strcmp(variables[x], "com_author")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if ((comment_author = (char *)strdup(variables[x])) == NULL)
				comment_author = "";
			strip_html_brackets(comment_author);
		}

		/* we found the comment data */
		else if (!strcmp(variables[x], "com_data")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if ((comment_data = (char *)strdup(variables[x])) == NULL)
				comment_data = "";
			strip_html_brackets(comment_data);
		}

		/* we found the host name */
		else if (!strcmp(variables[x], "host")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if ((host_name = (char *)strdup(variables[x])) == NULL)
				host_name = "";
			else {
				strip_html_brackets(host_name);

				/* Store hostname in struct */
				commands[x].host_name = host_name;
			}
		}

		/* we found the hostgroup name */
		else if (!strcmp(variables[x], "hostgroup")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if ((hostgroup_name = (char *)strdup(variables[x])) == NULL)
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

			if ((service_desc = (char *)strdup(variables[x])) == NULL)
				service_desc = "";
			else {
				strip_html_brackets(service_desc);

				/* Store service description in struct */
				commands[(x-2)].description = service_desc;
			}
		}

		/* we found a combined host/service */
		else if (!strcmp(variables[x], "hostservice")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			temp_buffer = strtok(variables[x], "^");

			if ((host_name = (char *)strdup(temp_buffer)) == NULL)
				host_name = "";
			else {
				strip_html_brackets(host_name);
				commands[x].host_name = host_name;
			}

			temp_buffer = strtok(NULL, "");

			if ((service_desc = (char *)strdup(temp_buffer)) == NULL)
				service_desc = "";
			else {
				strip_html_brackets(service_desc);
				commands[x].description = service_desc;
			}
		}

		/* we found the servicegroup name */
		else if (!strcmp(variables[x], "servicegroup")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if ((servicegroup_name = (char *)strdup(variables[x])) == NULL)
				servicegroup_name = "";
			strip_html_brackets(servicegroup_name);
		}

		/* we got the persistence option for a comment */
		else if (!strcmp(variables[x], "persistent"))
			persistent_comment = TRUE;

		/* we got the notification option for an acknowledgement */
		else if (!strcmp(variables[x], "send_notification")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}
			send_notification = (atoi(variables[x]) > 0) ? TRUE : FALSE;
		}

		/* we got the acknowledgement type */
		else if (!strcmp(variables[x], "sticky_ack")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}
			sticky_ack = (atoi(variables[x]) > 0) ? TRUE : FALSE;
		}

		/* we use the end_time as expire time */
		else if (!strcmp(variables[x], "use_ack_end_time"))
			use_ack_end_time = TRUE;

		/* we use the end_time as disabled notifcations expire time */
		else if (!strcmp(variables[x], "use_disabled_notif_end_time"))
			use_disabled_notif_end_time = TRUE;

		/* we got the service check force option */
		else if (!strcmp(variables[x], "force_check"))
			force_check = TRUE;

		/* we got the option to affect host and all its services */
		else if (!strcmp(variables[x], "ahas"))
			affect_host_and_services = TRUE;

		/* we got the option to propagate to child hosts */
		else if (!strcmp(variables[x], "ptc"))
			propagate_to_children = TRUE;

		/* we got the option for fixed downtime */
		else if (!strcmp(variables[x], "fixed")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			fixed = (atoi(variables[x]) > 0) ? TRUE : FALSE;
		}

		/* we got the triggered by downtime option */
		else if (!strcmp(variables[x], "trigger")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			triggered_by = strtoul(variables[x], NULL, 10);
		}

		/* we got the child options */
		else if (!strcmp(variables[x], "childoptions")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			child_options = atoi(variables[x]);
		}

		/* we found the plugin output */
		else if (!strcmp(variables[x], "plugin_output")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			/* protect against buffer overflows */
			if (strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
				error = TRUE;
				break;
			} else
				strcpy(plugin_output, variables[x]);
		}

		/* we found the performance data */
		else if (!strcmp(variables[x], "performance_data")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			/* protect against buffer overflows */
			if (strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
				error = TRUE;
				break;
			} else
				strcpy(performance_data, variables[x]);
		}

		/* we found the plugin state */
		else if (!strcmp(variables[x], "plugin_state")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			plugin_state = atoi(variables[x]);
		}

		/* we found the hour duration */
		else if (!strcmp(variables[x], "hours")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (atoi(variables[x]) < 0) {
				error = TRUE;
				break;
			}
			duration += (unsigned long)(atoi(variables[x]) * 3600);
		}

		/* we found the minute duration */
		else if (!strcmp(variables[x], "minutes")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (atoi(variables[x]) < 0) {
				error = TRUE;
				break;
			}
			duration += (unsigned long)(atoi(variables[x]) * 60);
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

		/* we found the forced notification option */
		else if (!strcmp(variables[x], "force_notification"))
			force_notification = NOTIFICATION_OPTION_FORCED;

		/* we found the broadcast notification option */
		else if (!strcmp(variables[x], "broadcast_notification"))
			broadcast_notification = NOTIFICATION_OPTION_BROADCAST;

		/* we got the persistence option for a comment */
		else if (!strcmp(variables[x], "nodaemoncheck"))
			daemon_check = FALSE;

	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

void print_object_list(int list_type) {
	hoststatus *temp_hoststatus = NULL;
	servicestatus *temp_servicestatus = NULL;
	int x = 0;
	int row_color = 0;
	int host_passive = FALSE;
	int service_passive = FALSE;


	printf("<tr><td colspan=\"2\">&nbsp;</td></tr>\n");
	printf("<tr class=\"sectionHeader\"><td colspan=\"2\" >Affected Objects</td></tr>\n");

	printf("<tr><td colspan=\"2\">\n");

	printf("<script language='javascript' type=\"text/javascript\">\nchecked=false;\n");
	printf("function checkAllBoxes() {\n"
		"	checked = (checked == false) ? true : false;\n"
		"	for (var i=0; i < %d; i++) {\n"
		"		var checkboxes = document.getElementById(\"cb_\" + i);\n"
		"		if (checkboxes != null ) { checkboxes.checked = checked; }\n"
		"	}\n"
		"}\n", NUMBER_OF_STRUCTS);
	printf("</script>\n");

	printf("<TABLE cellspacing='2' cellpadding='0' border='0' width='100%%'>\n");

	if (list_type == PRINT_SERVICE_LIST)
		printf("<tr class=\"objectTableHeader\"><td width=\"46%%\">Host</td><td width=\"46%%\">Service</td><td width='16'><input type='checkbox' onclick=\"checkAllBoxes();\" title=\"Check All\"></td></tr>\n");
	else if (list_type == PRINT_HOST_LIST)
		printf("<tr class=\"objectTableHeader\"><td colspan=\"2\" width=\"96%%\">Hosts</td><td width='16'><input type='checkbox' onclick=\"checkAllBoxes();\" title=\"Check All\"></td></tr>\n");
	else
		printf("<tr><td colspan=\"3\">&nbsp;</td></tr>\n");

	for (x = 0; x < NUMBER_OF_STRUCTS; x++) {

		if (list_type == PRINT_HOST_LIST || list_type == PRINT_SERVICE_LIST) {
			host_passive = FALSE;
			service_passive = FALSE;

			if (commands[x].host_name == NULL)
				continue;

			if (list_type == PRINT_SERVICE_LIST && commands[x].description == NULL)
				continue;

			if (strlen(commands[x].host_name) != 0 && (
				command_type == CMD_SCHEDULE_HOST_CHECK ||
				command_type == CMD_DISABLE_HOST_CHECK ||
				command_type == CMD_SCHEDULE_SVC_CHECK ||
				command_type == CMD_DISABLE_SVC_CHECK )) {
				if((temp_hoststatus = find_hoststatus(commands[x].host_name)) != NULL) {
					if (temp_hoststatus->checks_enabled == FALSE)
						host_passive = TRUE;
				}

				if (list_type == PRINT_SERVICE_LIST && strlen(commands[x].description) != 0 ) {
					if((temp_servicestatus = find_servicestatus(commands[x].host_name, commands[x].description)) != NULL) {
						if (temp_servicestatus->checks_enabled == FALSE)
							service_passive = TRUE;
					}
				}
			}

		} else {
			if (multi_ids[x] == FALSE)
				continue;
		}

		row_color = (row_color == 0) ? 1 : 0;

		printf("<tr class=\"status%s\"><td width=\"50%%\"", (row_color == 0) ? "Even" : "Odd ");
		if (list_type == PRINT_SERVICE_LIST) {
			/* hostname and service description are present */
			if (strlen(commands[x].host_name) != 0  && strlen(commands[x].description) != 0) {
				printf(">%s</td><td>%s",
					escape_string(commands[x].host_name), escape_string(commands[x].description)
				);
				if (service_passive == TRUE) {
					printf("<img src='%s%s' align=right border=0 style='padding-right:2px' alt='Passive' title='Passive Service'>",
						url_images_path, PASSIVE_ICON
					);
				}
                                printf("</td>\n");

				printf("<td align='center'><input type='checkbox' name='hostservice' id=\"cb_%d\" value='%s^%s' title=\"%s Service\" %s></td></tr>\n",
					x, escape_string(commands[x].host_name), escape_string(commands[x].description),
					(service_passive == FALSE) ? "Active" : "Passive", (service_passive == FALSE) ? "checked" : "");
			} else {
				/* if hostname is empty print inputbox instead */
				if (!strcmp(commands[x].host_name, ""))
					printf("><INPUT TYPE='TEXT' NAME='host' SIZE=30></td>");
				else
					printf("><INPUT TYPE='HIDDEN' NAME='host' VALUE='%s'>%s</td>", escape_string(commands[x].host_name), escape_string(commands[x].host_name));
				/* if service description is empty print inputbox instead */
				if (!strcmp(commands[x].description, ""))
					printf("<td><INPUT TYPE='TEXT' NAME='service' SIZE=30></td>");
				else
					printf("<td><INPUT TYPE='HIDDEN' NAME='service' VALUE='%s'>%s</td>", escape_string(commands[x].description), escape_string(commands[x].description));

				printf("<td></td></tr>\n");
			}
		} else if (list_type == PRINT_HOST_LIST) {
			/* if hostname is empty print inputbox instead */
			if (!strcmp(commands[x].host_name, ""))
				printf(" style=\"font-weight:bold;\">Host:</td><td><INPUT TYPE='TEXT' NAME='host' SIZE=30></td><td></td></tr>\n");
			else {
				printf(" style=\"font-weight:bold;\">Host:</td><td>%s", escape_string(commands[x].host_name));
				if (host_passive == TRUE) {
					printf("<img src='%s%s' align=right border=0 style='padding-right:2px' alt='Passive' title='Passive Service'>",
						url_images_path, PASSIVE_ICON
					);
				}
                                printf("</td>\n");

				printf("<td align='center'><input type='checkbox' name='host' id=\"cb_%d\" value='%s' title=\"%s Host\" %s></td></tr>\n",
					x, escape_string(commands[x].host_name),
					(host_passive == FALSE) ? "Active" : "Passive", (host_passive == FALSE) ? "checked" : ""
				);
			}
		} else if (list_type == PRINT_COMMENT_LIST) {
			printf(" style=\"font-weight:bold;\">Comment ID:</td><td><INPUT TYPE='HIDDEN' NAME='com_id' VALUE='%lu'>%lu</td></tr>\n", multi_ids[x], multi_ids[x]);
		} else if (list_type == PRINT_DOWNTIME_LIST) {
			printf(" style=\"font-weight:bold;\">Scheduled Downtime ID:</td><td><INPUT TYPE='HIDDEN' NAME='down_id' VALUE='%lu'>%lu</td></tr>\n", multi_ids[x], multi_ids[x]);
		}
	}

	printf("</td><tr></table>\n</td></tr>\n");

	return;
}

void print_help_box(char *content) {

	printf("<img src='%s%s' onMouseOver=\"return tooltip('<table border=0 width=100%% height=100%%>", url_images_path, CONTEXT_HELP_ICON);
	printf("<tr><td>%s</td></tr>", content);
	printf("</table>', '&nbsp;&nbsp;&nbsp;Help', 'border:1, width:500, xoffset:-250, yoffset:25, bordercolor:#333399, title_padding:2px, titletextcolor:#FFFFFF, backcolor:#CCCCFF');\" onMouseOut=\"return hideTip()\"");
	printf(" BORDER=0>");
	return;
}

void print_form_element(int element, int cmd) {
	time_t t;
	int t_hour, t_min;
	char buffer[MAX_INPUT_BUFFER];

	switch (element) {

	case PRINT_COMMON_HEADER:
		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
		printf("<tr><td COLSPAN=\"2\" CLASS='sectionHeader'>Common Data</td></tr>\n");
		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
		break;

	case PRINT_AUTHOR:
		printf("<tr><td class=\"objectDescription descriptionleft\">Author (Your Name):</td><td align=\"left\">");
		if (lock_author_names == TRUE)
			printf("<INPUT TYPE='HIDDEN' NAME='com_author' VALUE='%s'>%s</td></tr>\n", escape_string(comment_author), escape_string(comment_author));
		else
			printf("<INPUT TYPE='INPUT' NAME='com_author' VALUE='%s'></td></tr>\n", escape_string(comment_author));
		break;

	case PRINT_COMMENT_BOX:

		strcpy(help_text, "If you work with other administrators, you may find it useful to share information about a host/service "
		       "that is having problems if more than one of you may be working on it. "
		       "Make sure to enter a brief description of what you are doing.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Comment:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<TEXTAREA ID=\"com_data\" NAME='com_data' COLS=25 ROWS=2 onkeyup=\"check_input();\">%s</TEXTAREA>", escape_string(comment_data));
		printf("<BR><DIV ID='com_data_error' class=\"inputError\" style=\"display:none;\">Comment data can't be sent empty</DIV>");
		printf("</td></tr>\n");
		break;

	case PRINT_CHECK_OUTPUT_BOX:

		snprintf(help_text, sizeof(help_text), "Fill in the exact output string which sould be sent to %s", PROGRAM_NAME);
		help_text[sizeof(help_text)-1] = '\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Check Output:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<TEXTAREA ID=\"plugin_output\" NAME='plugin_output' COLS=25 ROWS=2  onkeyup=\"check_input();\"></TEXTAREA>");
		printf("<BR><DIV ID='plugin_output_error' class=\"inputError\" style=\"display:none;\">Output string can't be send empty</DIV>");
		printf("</td></tr>\n");
		break;

	case PRINT_PERFORMANCE_DATA_BOX:

		snprintf(help_text, sizeof(help_text), "Fill in the exact performance data string which sould be sent to %s", PROGRAM_NAME);
		help_text[sizeof(help_text)-1] = '\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Performance Data:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<TEXTAREA NAME='performance_data' COLS=25 ROWS=2></TEXTAREA></td></tr>\n");
		break;

	case PRINT_STICKY_ACK:

		strcpy(help_text, "If you want acknowledgement to disable notifications until the host/service recovers, check this option.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Sticky Acknowledgement:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='sticky_ack' %s></td></tr>\n", (sticky_ack == TRUE) ? "CHECKED" : "");
		break;

	case PRINT_SEND_NOTFICATION:

		strcpy(help_text, "If you do not want an acknowledgement notification sent out to the appropriate contacts, uncheck this option.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Send Notification:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='send_notification' %s></td></tr>\n", (send_notification == TRUE) ? "CHECKED" : "");
		break;

	case PRINT_PERSISTENT:

		if (cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM || cmd == CMD_ACKNOWLEDGE_SVC_PROBLEM)
			strcpy(help_text, "If you would like the comment to remain once the acknowledgement is removed, check this checkbox.");
		else {
			snprintf(help_text, sizeof(help_text), "If you uncheck this option, the comment will automatically be deleted the next time %s is restarted.", PROGRAM_NAME);
			help_text[sizeof(help_text)-1] = '\x0';
		}
		printf("<tr><td class=\"objectDescription descriptionleft\">Persistent%s:", (cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM || cmd == CMD_ACKNOWLEDGE_SVC_PROBLEM) ? " Comment" : "");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='persistent' %s></td></tr>\n", (persistent_ack_comments == TRUE || cmd == CMD_ADD_HOST_COMMENT || cmd == CMD_ADD_SVC_COMMENT) ? "CHECKED" : "");
		break;

	case PRINT_NOTIFICATION_DELAY:

		strcpy(help_text, "The notification delay will be disregarded if the host/service changes state before the next notification is scheduled to be sent out.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Notification Delay (minutes from now):");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='TEXT' ID='not_dly' NAME='not_dly' VALUE='%d' SIZE=\"4\">", notification_delay);
		printf("<BR><DIV ID='not_dly_error' class=\"inputError\" style=\"display:none;\">Notification delay can't be zero</DIV>");
		printf("</td></tr>\n");
		break;

	case PRINT_START_TIME:
	case PRINT_END_TIME:
	case PRINT_CHECK_TIME:
		time(&t);
		if (element == PRINT_END_TIME)
			t += (unsigned long)default_downtime_duration;
		get_time_string(&t, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);
		printf("<tr><td class=\"objectDescription descriptionleft\">");
		if (element == PRINT_START_TIME) {
			strcpy(help_text, "Set the start date/time for the downtime.");
			printf("Start Time:");
		} else if (element == PRINT_END_TIME) {
			strcpy(help_text, "Set the end date/time for the downtime.");
			printf("End Time:");
		} else {
			strcpy(help_text, "Set the date/time when this check should be schedule to.");
			printf("Check Time:");
		}
		print_help_box(help_text);
		printf("</td><td align=\"left\"><INPUT TYPE='TEXT' class='timepicker' NAME='%s_time' VALUE='%s' SIZE=\"25\"></td></tr>\n", (element == PRINT_END_TIME) ? "end" : "start", buffer);
		break;

	case PRINT_FIXED_FLEXIBLE_TYPE:
		default_downtime_duration = default_downtime_duration / 60;
		t_min = default_downtime_duration % 60;
		default_downtime_duration = default_downtime_duration - t_min;
		t_hour = (default_downtime_duration / 60) ;

		snprintf(help_text, sizeof(help_text), "If you select the <i>fixed</i> option, the downtime will be in effect between the start and end times you specify. If you do not select the <i>fixed</i> "
		         "option, %s will treat this as <i>flexible</i> downtime. Flexible downtime starts when the host goes down or becomes unreachable / service becomes critical (sometime between the "
		         "start and end times you specified) and lasts as long as the duration of time you enter. The duration fields do not apply for fixed downtime.", PROGRAM_NAME);
		help_text[sizeof(help_text)-1] = '\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Type:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">\n");

		printf("\t<SELECT ID=\"flexible_selection\" NAME='fixed' onChange=\"if (document.getElementById('flexible_selection').selectedIndex == 0) document.getElementById('fd_row').style.display = 'none'; else document.getElementById('fd_row').style.display = '';\">\n");
		printf("\t\t<OPTION VALUE=1\">Fixed</OPTION>\n");
		printf("\t\t<OPTION VALUE=0\">Flexible</OPTION>\n");
		printf("\t</SELECT>\n");

		snprintf(help_text, sizeof(help_text), "Enter here the duration of the downtime. %s will automatically delete the downtime after this time expired.", PROGRAM_NAME);
		help_text[sizeof(help_text)-1] = '\x0';

		printf("<tr id=\"fd_row\" style=\"display:none;\"><td class=\"objectDescription descriptionleft\">Flexible Duration:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">\n");
		printf("\t<table border=0  cellspacing=0 cellpadding=0>\n");
		printf("\t\t<tr>\n");
		printf("\t\t\t<td><INPUT TYPE='TEXT' NAME='hours' VALUE='%d' SIZE=2 MAXLENGTH=2></td>\n", t_hour);
		printf("\t\t\t<td width=\"50\">&nbsp;Hours</td>\n");
		printf("\t\t\t<td><INPUT TYPE='TEXT' NAME='minutes' VALUE='%d' SIZE=2 MAXLENGTH=2></td>\n", t_min);
		printf("\t\t\t<td width=\"50\">&nbsp;Minutes</td>\n");
		printf("\t\t</tr>\n");
		printf("\t</table>\n");
		printf("</td></tr>\n");
		break;

	case PRINT_EXPIRE_ACKNOWLEDGEMENT:

		strcpy(help_text, "If you want to let the acknowledgement expire, check this option.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Use Expire Time:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' ID='expire_checkbox' NAME='use_ack_end_time' onClick=\"if (document.getElementById('expire_checkbox').checked == true) document.getElementById('expired_date_row').style.display = ''; else document.getElementById('expired_date_row').style.display = 'none';\"></td></tr>\n");

		snprintf(help_text, sizeof(help_text), "Enter here the expire date/time for this acknowledgement. %s will automatically delete the acknowledgement after this time expired.", PROGRAM_NAME);
		help_text[sizeof(help_text)-1] = '\x0';

		time(&t);
		t += (unsigned long)default_expiring_acknowledgement_duration;
		get_time_string(&t, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);

		printf("<tr id=\"expired_date_row\" style=\"display:none;\"><td class=\"objectDescription descriptionleft\">Expire Time:");
		print_help_box(help_text);
		printf("</td><td align=\"left\"><INPUT TYPE='TEXT' class='timepicker' NAME='end_time' VALUE='%s' SIZE=\"25\"></td></tr>\n", buffer);
		break;

        case PRINT_EXPIRE_DISABLE_NOTIFICATIONS:

                strcpy(help_text, "If you want to let the disabled notifications expire, check this option.");

                printf("<tr><td class=\"objectDescription descriptionleft\">Use Expire Time:");
                print_help_box(help_text);
                printf("</td><td align=\"left\">");
                printf("<INPUT TYPE='checkbox' ID='expire_checkbox' NAME='use_disabled_notif_end_time' onClick=\"if (document.getElementById('expire_checkbox').checked == true) document.getElementById('expired_date_row').style.display = ''; else document.getElementById('expired_date_row').style.display = 'none';\"></td></tr>\n");

                snprintf(help_text, sizeof(help_text), "Enter the expire date/time for disabled notifications. %s will automatically re-enable all notifications after this time expired.", PROGRAM_NAME);
                help_text[sizeof(help_text)-1] = '\x0';

                time(&t);
                t += (unsigned long)default_expiring_disabled_notifications_duration;
                get_time_string(&t, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);

                printf("<tr id=\"expired_date_row\" style=\"display:none;\"><td class=\"objectDescription descriptionleft\">Expire Time:");
                print_help_box(help_text);
                printf("</td><td align=\"left\"><INPUT TYPE='TEXT' class='timepicker' NAME='end_time' VALUE='%s' SIZE=\"25\"></td></tr>\n", buffer);
                break;

	case PRINT_FORCE_CHECK:

		snprintf(help_text, sizeof(help_text), "If you select this option, %s will force a check of the host/service regardless of both what time the scheduled check occurs and whether or not checks are enabled for the host/service.", PROGRAM_NAME);
		help_text[sizeof(help_text)-1] = '\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Force Check:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='force_check' %s></td></tr>\n", (force_check == TRUE) ? "CHECKED" : "");
		break;

	case PRINT_BROADCAST_NOTIFICATION:

		strcpy(help_text, "Selecting this option causes the notification to be sent out to all normal (non-escalated) and escalated contacts. These options allow you to override the normal notification logic if you need to get an important message out.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Broadcast:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='broadcast_notification'></td></tr>\n");
		break;

	case PRINT_FORCE_NOTIFICATION:

		snprintf(help_text, sizeof(help_text), "Custom notifications normally follow the regular notification logic in %s.  Selecting this option will force the notification to be sent out, regardless of the time restrictions, whether or not notifications are enabled, etc.", PROGRAM_NAME);
		help_text[sizeof(help_text)-1] = '\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Forced:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='force_notification'></td></tr>\n");
		break;

	default:
		break;
	}

	return;
}

void request_command_data(int cmd) {
	char start_time[MAX_DATETIME_LENGTH];
	contact *temp_contact;
	scheduled_downtime *temp_downtime;
	host *temp_host = NULL;
	char action[MAX_INPUT_BUFFER];
	int found_trigger_objects = FALSE;

	/* get default name to use for comment author */
	temp_contact = find_contact(current_authdata.username);
	if (temp_contact != NULL && temp_contact->alias != NULL)
		comment_author = temp_contact->alias;
	else
		comment_author = current_authdata.username;

	printf("<BR>");

	switch (cmd) {

	case CMD_ADD_HOST_COMMENT:
	case CMD_ADD_SVC_COMMENT:
		snprintf(action, sizeof(action), "Add %s comments", (cmd == CMD_ADD_HOST_COMMENT) ? "host" : "service");
		break;

	case CMD_DEL_HOST_COMMENT:
	case CMD_DEL_SVC_COMMENT:
		snprintf(action, sizeof(action), "Delete %s comments", (cmd == CMD_DEL_HOST_COMMENT) ? "host" : "service");
		break;

	case CMD_DELAY_HOST_NOTIFICATION:
	case CMD_DELAY_SVC_NOTIFICATION:
		snprintf(help_text, sizeof(help_text), "This command is used to delay the next problem notification that is sent out for specified %s. The notification delay will be disregarded if "
		         "the %s changes state before the next notification is scheduled to be sent out.	 This command has no effect if the %s are currently %s.", (cmd == CMD_DELAY_HOST_NOTIFICATION) ? "hosts" : "services", (cmd == CMD_DELAY_HOST_NOTIFICATION) ? "hosts" : "services", (cmd == CMD_DELAY_HOST_NOTIFICATION) ? "hosts" : "services", (cmd == CMD_DELAY_HOST_NOTIFICATION) ? "UP" : "in an OK state");
		snprintf(action, sizeof(action), "Delay a %s notification", (cmd == CMD_DELAY_HOST_NOTIFICATION) ? "host" : "service");
		break;

	case CMD_SCHEDULE_HOST_CHECK:
	case CMD_SCHEDULE_SVC_CHECK:
		snprintf(help_text, sizeof(help_text), "This command is used to schedule the next check of these %s. %s will re-queue the %s to be checked at the time you specify.", (cmd == CMD_SCHEDULE_HOST_CHECK) ? "hosts" : "services", PROGRAM_NAME, (cmd == CMD_SCHEDULE_HOST_CHECK) ? "host" : "service");
		snprintf(action, sizeof(action), "Schedule %s checks", (cmd == CMD_SCHEDULE_HOST_CHECK) ? "host" : "service");
		break;

	case CMD_ENABLE_SVC_CHECK:
	case CMD_DISABLE_SVC_CHECK:
		snprintf(action, sizeof(action), "%s active service checks on a program-wide basis", (cmd == CMD_ENABLE_SVC_CHECK) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_NOTIFICATIONS:
	case CMD_DISABLE_NOTIFICATIONS:
		snprintf(help_text, sizeof(help_text), "This command is used to %s host and service notifications on a program-wide basis", (cmd == CMD_ENABLE_NOTIFICATIONS) ? "enable" : "disable");
		snprintf(action, sizeof(action), "%s notifications on a program-wide basis", (cmd == CMD_ENABLE_NOTIFICATIONS) ? "Enable" : "Disable");
		break;

	case CMD_DISABLE_NOTIFICATIONS_EXPIRE_TIME:
		snprintf(help_text, sizeof(help_text), "This command is used to disable host and service notifications on a program-wide basis, with a given expire time");
		snprintf(action, sizeof(action), "Disable notifications on a program-wide basis, with expire time");
		break;

	case CMD_SHUTDOWN_PROCESS:
	case CMD_RESTART_PROCESS:
		snprintf(action, sizeof(action), "%s the %s process", (cmd == CMD_SHUTDOWN_PROCESS) ? "Shutdown" : "Restart", PROGRAM_NAME);
		break;

	case CMD_ENABLE_HOST_SVC_CHECKS:
	case CMD_DISABLE_HOST_SVC_CHECKS:
		if (cmd == CMD_ENABLE_HOST_SVC_CHECKS)
			snprintf(help_text, sizeof(help_text), "This command is used to enable active checks of all services associated with the specified host");
		else {
			snprintf(help_text, sizeof(help_text), "This command is used to disable active checks of all services associated with the specified host. "
			         "When a service is disabled %s will not monitor the service. Doing this will prevent any notifications being sent out for "
			         "the specified service while it is disabled. In order to have %s check the service in the future you will have to re-enable the service. "
			         "Note that disabling service checks may not necessarily prevent notifications from being sent out about the host which those services are associated with.", PROGRAM_NAME, PROGRAM_NAME);
		}
		snprintf(action, sizeof(action), "%s active checks of all services on these hosts", (cmd == CMD_ENABLE_HOST_SVC_CHECKS) ? "Enable" : "Disable");
		break;

	case CMD_SCHEDULE_HOST_SVC_CHECKS:
		snprintf(action, sizeof(action), "Schedule a check of all services for these hosts");
		break;

	case CMD_DEL_ALL_HOST_COMMENTS:
	case CMD_DEL_ALL_SVC_COMMENTS:
		snprintf(action, sizeof(action), "Delete all comments for these %s", (cmd == CMD_DEL_ALL_HOST_COMMENTS) ? "hosts" : "services");
		break;

	case CMD_ENABLE_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SVC_NOTIFICATIONS:
		snprintf(action, sizeof(action), "%s notifications for these services", (cmd == CMD_ENABLE_SVC_NOTIFICATIONS) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOST_NOTIFICATIONS:
		snprintf(action, sizeof(action), "%s notifications for these hosts", (cmd == CMD_ENABLE_HOST_NOTIFICATIONS) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
	case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
		snprintf(help_text, sizeof(help_text), "This command is used to %s notifications for all hosts and services that lie <i>beyond</i> the specified host (from the view of %s).", (cmd == CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST) ? "enable" : "disable", PROGRAM_NAME);
		snprintf(action, sizeof(action), "%s notifications for all hosts and services beyond these hosts", (cmd == CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
		snprintf(action, sizeof(action), "%s notifications for all services on these hosts", (cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS) ? "Enable" : "Disable");
		break;

	case CMD_ACKNOWLEDGE_HOST_PROBLEM:
	case CMD_ACKNOWLEDGE_SVC_PROBLEM:
		snprintf(action, sizeof(action), "Acknowledge %s problems", (cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM) ? "host" : "service");
		break;

	case CMD_START_EXECUTING_HOST_CHECKS:
	case CMD_STOP_EXECUTING_HOST_CHECKS:
		snprintf(action, sizeof(action), "%s executing host checks on a program-wide basis", (cmd == CMD_START_EXECUTING_HOST_CHECKS) ? "Start" : "Stop");
		break;

	case CMD_START_EXECUTING_SVC_CHECKS:
	case CMD_STOP_EXECUTING_SVC_CHECKS:
		if (cmd == CMD_START_EXECUTING_SVC_CHECKS)
			snprintf(help_text, sizeof(help_text), "This command is used to resume execution of active service checks on a program-wide basis. Individual services which are disabled will still not be checked.");
		else
			snprintf(help_text, sizeof(help_text), "This command is used to temporarily stop %s from actively executing any service checks.  This will have the side effect of preventing any notifications from being sent out (for any and all services and hosts). "
			         "Service checks will not be executed again until you issue a command to resume service check execution.", PROGRAM_NAME);
		snprintf(action, sizeof(action), "%s executing active service checks", (cmd == CMD_START_EXECUTING_SVC_CHECKS) ? "Start" : "Stop");
		break;

	case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
		snprintf(help_text, sizeof(help_text), "This command is used to make %s %s accepting passive service check results that it finds in the external command file.", PROGRAM_NAME, (cmd == CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS) ? "start" : "stop");
		snprintf(action, sizeof(action), "%s accepting passive service checks on a program-wide basis", (cmd == CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS) ? "Start" : "Stop");
		break;

	case CMD_ENABLE_PASSIVE_SVC_CHECKS:
	case CMD_DISABLE_PASSIVE_SVC_CHECKS:
		if (cmd == CMD_ENABLE_PASSIVE_SVC_CHECKS)
			snprintf(help_text, sizeof(help_text), "This command is used to allow %s to accept passive service check results that it finds in the external command file for this particular service.", PROGRAM_NAME);
		else
			snprintf(help_text, sizeof(help_text), "This command is used to stop %s accepting passive service check results that it finds in the external command file for this particular service. All passive check results that are found for this service will be ignored.", PROGRAM_NAME);
		snprintf(action, sizeof(action), "%s accepting passive service checks for these services", (cmd == CMD_ENABLE_PASSIVE_SVC_CHECKS) ? "Start" : "Stop");
		break;

	case CMD_ENABLE_EVENT_HANDLERS:
	case CMD_DISABLE_EVENT_HANDLERS:
		if (cmd == CMD_ENABLE_EVENT_HANDLERS)
			snprintf(help_text, sizeof(help_text), "This command is used to allow %s to run host and service event handlers.", PROGRAM_NAME);
		else
			snprintf(help_text, sizeof(help_text), "This command is used to temporarily prevent %s from running any host or service event handlers.", PROGRAM_NAME);
		snprintf(action, sizeof(action), "%s event handlers on a program-wide basis", (cmd == CMD_ENABLE_EVENT_HANDLERS) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_HOST_EVENT_HANDLER:
	case CMD_DISABLE_HOST_EVENT_HANDLER:
		snprintf(help_text, sizeof(help_text), "This command is used to %s the event handler for the selected hosts", (cmd == CMD_ENABLE_HOST_EVENT_HANDLER) ? "enable" : "disable");
		snprintf(action, sizeof(action), "%s the event handler for these hosts", (cmd == CMD_ENABLE_HOST_EVENT_HANDLER) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_SVC_EVENT_HANDLER:
	case CMD_DISABLE_SVC_EVENT_HANDLER:
		snprintf(help_text, sizeof(help_text), "This command is used to %s the event handler for the selected services", (cmd == CMD_ENABLE_SVC_EVENT_HANDLER) ? "enable" : "disable");
		snprintf(action, sizeof(action), "%s the event handler for these services", (cmd == CMD_ENABLE_SVC_EVENT_HANDLER) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_HOST_CHECK:
	case CMD_DISABLE_HOST_CHECK:
		if (cmd == CMD_DISABLE_HOST_CHECK)
			snprintf(help_text, sizeof(help_text), "This command is used to temporarily prevent %s from actively checking the status of a particular host. If %s needs to check the status of this host, it will assume that it is in the same state that it was in before checks were disabled.", PROGRAM_NAME, PROGRAM_NAME);
		snprintf(action, sizeof(action), "%s active host checks", (cmd == CMD_ENABLE_HOST_CHECK) ? "Enable" : "Disable");
		break;

	case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
	case CMD_START_OBSESSING_OVER_SVC_CHECKS:
		if (cmd == CMD_START_OBSESSING_OVER_SVC_CHECKS)
			snprintf(help_text, sizeof(help_text), "This command is used to have %s start obsessing over service checks. Read the documentation on distributed monitoring for more information on this.", PROGRAM_NAME);
		snprintf(action, sizeof(action), "%s obsessing over service checks on a program-wide basis", (cmd == CMD_STOP_OBSESSING_OVER_SVC_CHECKS) ? "Stop" : "Start");
		break;

	case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
	case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
		snprintf(help_text, sizeof(help_text), "This command is used to remove an acknowledgement for %s problems. Once the acknowledgement is removed, notifications may start being "
		         "sent out about the %s problem.", (cmd == CMD_REMOVE_HOST_ACKNOWLEDGEMENT) ? "host" : "service", (cmd == CMD_REMOVE_HOST_ACKNOWLEDGEMENT) ? "host" : "service");
		snprintf(action, sizeof(action), "Remove %s acknowledgements", (cmd == CMD_REMOVE_HOST_ACKNOWLEDGEMENT) ? "host" : "service");
		break;

	case CMD_SCHEDULE_HOST_DOWNTIME:
	case CMD_SCHEDULE_SVC_DOWNTIME:
		snprintf(help_text, sizeof(help_text), "This command is used to schedule downtime for these %s. During the specified downtime, %s will not send notifications out about the %s. "
		         "When the scheduled downtime expires, %s will send out notifications for this %s as it normally would.	Scheduled downtimes are preserved "
		         "across program shutdowns and restarts.", (cmd == CMD_SCHEDULE_HOST_DOWNTIME) ? "hosts" : "services", PROGRAM_NAME, (cmd == CMD_SCHEDULE_HOST_DOWNTIME) ? "hosts" : "services", PROGRAM_NAME, (cmd == CMD_SCHEDULE_HOST_DOWNTIME) ? "hosts" : "services");
		snprintf(action, sizeof(action), "Schedule downtime for these %s", (cmd == CMD_SCHEDULE_HOST_DOWNTIME) ? "hosts" : "services");
		break;

	case CMD_DEL_DOWNTIME_BY_HOST_NAME:
                snprintf(help_text, sizeof(help_text), "This command is used to delete all downtimes for a host and all its services specified by the host name already supplied.");
		snprintf(action, sizeof(action), "Remove downtimes for all services for these hosts and the hosts themself");
                break;

	case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
		snprintf(help_text, sizeof(help_text), "This command is used to schedule downtime for a particular host and all of its services.	During the specified downtime, %s will not send notifications out about the host. "
		         "Normally, a host in downtime will not send alerts about any services in a failed state. This option will explicitly set downtime for all services for this host. "
		         "When the scheduled downtime expires, %s will send out notifications for this host as it normally would. Scheduled downtimes are preserved "
		         "across program shutdowns and restarts.", PROGRAM_NAME, PROGRAM_NAME);
		snprintf(action, sizeof(action), "Schedule downtime for all services for these hosts and the hosts themself");
		break;

	case CMD_PROCESS_HOST_CHECK_RESULT:
	case CMD_PROCESS_SERVICE_CHECK_RESULT:
		snprintf(help_text, sizeof(help_text), "This command is used to submit a passive check result for these %s. "
		         "It is particularly useful for resetting security-related %s to %s states once they have been dealt with.", (cmd == CMD_PROCESS_HOST_CHECK_RESULT) ? "hosts" : "services", (cmd == CMD_PROCESS_HOST_CHECK_RESULT) ? "hosts" : "services", (cmd == CMD_PROCESS_HOST_CHECK_RESULT) ? "UP" : "OK");

		snprintf(action, sizeof(action), "Submit a passive check result for these %s", (cmd == CMD_PROCESS_HOST_CHECK_RESULT) ? "hosts" : "services");
		break;

	case CMD_ENABLE_HOST_FLAP_DETECTION:
	case CMD_DISABLE_HOST_FLAP_DETECTION:
		snprintf(action, sizeof(action), "%s flap detection for these hosts", (cmd == CMD_ENABLE_HOST_FLAP_DETECTION) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_SVC_FLAP_DETECTION:
	case CMD_DISABLE_SVC_FLAP_DETECTION:
		snprintf(action, sizeof(action), "%s flap detection for these services", (cmd == CMD_ENABLE_SVC_FLAP_DETECTION) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_FLAP_DETECTION:
	case CMD_DISABLE_FLAP_DETECTION:
		snprintf(action, sizeof(action), "%s flap detection for hosts and services on a program-wide basis", (cmd == CMD_ENABLE_FLAP_DETECTION) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		snprintf(action, sizeof(action), "%s notifications for all services in a particular hostgroup", (cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		snprintf(action, sizeof(action), "%s notifications for all hosts in a particular hostgroup", (cmd == CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
	case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
		snprintf(action, sizeof(action), "%s active checks of all services in a particular hostgroup", (cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS) ? "Enable" : "Disable");
		break;

	case CMD_DEL_HOST_DOWNTIME:
	case CMD_DEL_SVC_DOWNTIME:
		snprintf(action, sizeof(action), "Cancel scheduled downtime for these %s", (cmd == CMD_DEL_HOST_DOWNTIME) ? "hosts" : "services");
		break;

	case CMD_ENABLE_FAILURE_PREDICTION:
	case CMD_DISABLE_FAILURE_PREDICTION:
		snprintf(action, sizeof(action), "%s failure prediction for hosts and service on a program-wide basis", (cmd == CMD_ENABLE_FAILURE_PREDICTION) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_PERFORMANCE_DATA:
	case CMD_DISABLE_PERFORMANCE_DATA:
		snprintf(action, sizeof(action), "%s performance data processing for hosts and services on a program-wide basis", (cmd == CMD_ENABLE_PERFORMANCE_DATA) ? "Enable" : "Disable");
		break;

	case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
	case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
		snprintf(action, sizeof(action), "Schedule downtime for all %s in a particular hostgroup", (cmd == CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME) ? "hosts" : "services");
		break;

	case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
		snprintf(action, sizeof(action), "%s accepting passive host checks on a program-wide basis", (cmd == CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS) ? "Start" : "Stop");
		break;

	case CMD_ENABLE_PASSIVE_HOST_CHECKS:
	case CMD_DISABLE_PASSIVE_HOST_CHECKS:
		snprintf(action, sizeof(action), "%s accepting passive checks for these hosts", (cmd == CMD_ENABLE_PASSIVE_HOST_CHECKS) ? "Start" : "Stop");
		break;

	case CMD_START_OBSESSING_OVER_HOST_CHECKS:
	case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:
		snprintf(action, sizeof(action), "%s obsessing over host checks on a program-wide basis", (cmd == CMD_START_OBSESSING_OVER_HOST_CHECKS) ? "Start" : "Stop");
		break;

	case CMD_START_OBSESSING_OVER_SVC:
	case CMD_STOP_OBSESSING_OVER_SVC:
		snprintf(action, sizeof(action), "%s obsessing over these services", (cmd == CMD_START_OBSESSING_OVER_SVC) ? "Start" : "Stop");
		break;

	case CMD_START_OBSESSING_OVER_HOST:
	case CMD_STOP_OBSESSING_OVER_HOST:
		snprintf(action, sizeof(action), "%s obsessing over these hosts", (cmd == CMD_START_OBSESSING_OVER_HOST) ? "Start" : "Stop");
		break;

	case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		snprintf(action, sizeof(action), "%s notifications for all services in a particular servicegroup", (cmd == CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		snprintf(action, sizeof(action), "%s notifications for all hosts in a particular servicegroup", (cmd == CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS) ? "Enable" : "Disable");
		break;

	case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
	case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
		snprintf(action, sizeof(action), "%s active checks of all services in a particular servicegroup", (cmd == CMD_ENABLE_SERVICEGROUP_SVC_CHECKS) ? "Enable" : "Disable");
		break;

	case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
		snprintf(action, sizeof(action), "Schedule downtime for all hosts in a particular servicegroup");
		break;

	case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:
		snprintf(action, sizeof(action), "Schedule downtime for all services in a particular servicegroup");
		break;

	case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
	case CMD_SEND_CUSTOM_SVC_NOTIFICATION:
		snprintf(help_text, sizeof(help_text), "This command is used to send a custom notification about the specified %s.  Useful in emergencies when you need to notify admins of an issue regarding a monitored system or service.", (cmd == CMD_SEND_CUSTOM_HOST_NOTIFICATION) ? "host" : "service");
		snprintf(action, sizeof(action), "Send a custom %s notification", (cmd == CMD_SEND_CUSTOM_HOST_NOTIFICATION) ? "host" : "service");
		break;

        case CMD_CHANGE_HOST_MODATTR:
		snprintf(action, sizeof(action), "Reset modified attributes for Host(s).");
		break;

        case CMD_CHANGE_SVC_MODATTR:
		snprintf(action, sizeof(action), "Reset modified attributes for Service(s).");
		break;

	default:
		print_generic_error_message("Sorry Dave, I can't let you do that...", "Executing an unknown command? Shame on you!", 2);

		return;
	}

	help_text[sizeof(help_text)-1] = '\x0';
	action[sizeof(action)-1] = '\x0';

	/* Javascript to check input */
	printf("<script language=\"JavaScript\">\n");
	printf("function check_input(){\n"
	       "	if (document.getElementById('com_data')) {\n"
	       "		if (document.getElementById('com_data').value == '') {\n"
	       "			document.getElementById('com_data_error').style.display = '';\n"
	       "			return false;\n"
	       "		} else {\n"
	       "			document.getElementById('com_data_error').style.display = 'none';\n"
	       "		}\n"
	       "	}\n"
	       "	if (document.getElementById('plugin_output')) {\n"
	       "		if (document.getElementById('plugin_output').value == '') {\n"
	       "			document.getElementById('plugin_output_error').style.display = '';\n"
	       "			return false;\n"
	       "		} else {\n"
	       "			document.getElementById('plugin_output_error').style.display = 'none';\n"
	       "		}\n"
	       "	}\n"
	       "	if (document.getElementById('not_dly')) {\n"
	       "		if (parseInt(document.getElementById('not_dly').value) == 0 ) {\n"
	       "			document.getElementById('not_dly_error').style.display = '';\n"
	       "			return false;\n"
	       "		}\n"
	       "	}\n"
	       "	return true;\n"
	       "}\n"
	       "</script>\n");

	printf("<div align='center'>\n");

	printf("<form method='post' action='%s' onSubmit=\"return check_input();\">\n", CMD_CGI);

	printf("<INPUT TYPE='HIDDEN' NAME='cmd_typ' VALUE='%d'><INPUT TYPE='HIDDEN' NAME='cmd_mod' VALUE='%d'>\n", cmd, CMDMODE_COMMIT);

	/* creating an extra table to make it compatible to IE6 & IE7 to have a nice frame around the form, damn it */
	printf("<TABLE CELLSPACING='0' CELLPADDING='0'><TR><TD CLASS='boxFrame BoxWidth'>\n");

	printf("<TABLE CELLSPACING='2' CELLPADDING='0' class='contentTable'>\n");

	printf("<tr CLASS='sectionHeader'><td COLSPAN='2'>Action</td></tr>\n");
	printf("<tr><td COLSPAN='2'>%s ", action);
	if (strlen(help_text) > 2)
		print_help_box(help_text);
	printf("</td></tr>\n");

	switch (cmd) {

	case CMD_ADD_SVC_COMMENT:
	case CMD_ACKNOWLEDGE_SVC_PROBLEM:
	case CMD_ADD_HOST_COMMENT:
	case CMD_ACKNOWLEDGE_HOST_PROBLEM:

		if (cmd == CMD_ACKNOWLEDGE_SVC_PROBLEM || cmd == CMD_ADD_SVC_COMMENT)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER, cmd);
		print_form_element(PRINT_AUTHOR, cmd);
		print_form_element(PRINT_COMMENT_BOX, cmd);
		print_form_element(PRINT_PERSISTENT, cmd);

		if (cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM || cmd == CMD_ACKNOWLEDGE_SVC_PROBLEM) {
			print_form_element(PRINT_EXPIRE_ACKNOWLEDGEMENT, cmd);
			print_form_element(PRINT_STICKY_ACK, cmd);
			print_form_element(PRINT_SEND_NOTFICATION, cmd);
		}

		break;

	case CMD_DEL_HOST_DOWNTIME:
	case CMD_DEL_SVC_DOWNTIME:
	case CMD_DEL_HOST_COMMENT:
	case CMD_DEL_SVC_COMMENT:

		if (cmd == CMD_DEL_HOST_COMMENT || cmd == CMD_DEL_SVC_COMMENT)
			print_object_list(PRINT_COMMENT_LIST);
		else
			print_object_list(PRINT_DOWNTIME_LIST);

		if (enforce_comments_on_actions == TRUE) {
			print_form_element(PRINT_COMMON_HEADER, cmd);
			print_form_element(PRINT_AUTHOR, cmd);
			print_form_element(PRINT_COMMENT_BOX, cmd);
		}

		break;

	case CMD_DEL_DOWNTIME_BY_HOST_NAME:
                print_object_list(PRINT_HOST_LIST);

                print_form_element(PRINT_COMMON_HEADER, cmd);

                if (enforce_comments_on_actions == TRUE) {
                        print_form_element(PRINT_AUTHOR, cmd);
                        print_form_element(PRINT_COMMENT_BOX, cmd);
                }

                break;

	case CMD_DELAY_SVC_NOTIFICATION:
	case CMD_DELAY_HOST_NOTIFICATION:

		if (cmd == CMD_DELAY_SVC_NOTIFICATION)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER, cmd);

		if (enforce_comments_on_actions == TRUE) {
			print_form_element(PRINT_AUTHOR, cmd);
			print_form_element(PRINT_COMMENT_BOX, cmd);
		}

		print_form_element(PRINT_NOTIFICATION_DELAY, cmd);

		break;

	case CMD_SCHEDULE_SVC_CHECK:
	case CMD_SCHEDULE_HOST_CHECK:
	case CMD_SCHEDULE_HOST_SVC_CHECKS:

		if (cmd == CMD_SCHEDULE_SVC_CHECK)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER, cmd);

		if (enforce_comments_on_actions == TRUE) {
			print_form_element(PRINT_AUTHOR, cmd);
			print_form_element(PRINT_COMMENT_BOX, cmd);
		}

		print_form_element(PRINT_CHECK_TIME, cmd);
		print_form_element(PRINT_FORCE_CHECK, cmd);

		break;

	case CMD_ENABLE_SVC_CHECK:
	case CMD_DISABLE_SVC_CHECK:
	case CMD_DEL_ALL_SVC_COMMENTS:
	case CMD_ENABLE_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SVC_NOTIFICATIONS:
	case CMD_ENABLE_PASSIVE_SVC_CHECKS:
	case CMD_DISABLE_PASSIVE_SVC_CHECKS:
	case CMD_ENABLE_SVC_EVENT_HANDLER:
	case CMD_DISABLE_SVC_EVENT_HANDLER:
	case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
	case CMD_ENABLE_SVC_FLAP_DETECTION:
	case CMD_DISABLE_SVC_FLAP_DETECTION:
	case CMD_START_OBSESSING_OVER_SVC:
	case CMD_STOP_OBSESSING_OVER_SVC:

		print_object_list(PRINT_SERVICE_LIST);

		if (enforce_comments_on_actions == TRUE) {
			print_form_element(PRINT_COMMON_HEADER, cmd);
			print_form_element(PRINT_AUTHOR, cmd);
			print_form_element(PRINT_COMMENT_BOX, cmd);
		}

		break;

	case CMD_ENABLE_HOST_SVC_CHECKS:
	case CMD_DISABLE_HOST_SVC_CHECKS:
	case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
	case CMD_ENABLE_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOST_NOTIFICATIONS:
	case CMD_DEL_ALL_HOST_COMMENTS:
	case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
	case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
	case CMD_ENABLE_HOST_EVENT_HANDLER:
	case CMD_DISABLE_HOST_EVENT_HANDLER:
	case CMD_ENABLE_HOST_CHECK:
	case CMD_DISABLE_HOST_CHECK:
	case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
	case CMD_ENABLE_HOST_FLAP_DETECTION:
	case CMD_DISABLE_HOST_FLAP_DETECTION:
	case CMD_ENABLE_PASSIVE_HOST_CHECKS:
	case CMD_DISABLE_PASSIVE_HOST_CHECKS:
	case CMD_START_OBSESSING_OVER_HOST:
	case CMD_STOP_OBSESSING_OVER_HOST:

		print_object_list(PRINT_HOST_LIST);

		if (enforce_comments_on_actions == TRUE) {
			print_form_element(PRINT_COMMON_HEADER, cmd);
			print_form_element(PRINT_AUTHOR, cmd);
			print_form_element(PRINT_COMMENT_BOX, cmd);
		}

		if (cmd == CMD_ENABLE_HOST_SVC_CHECKS || cmd == CMD_DISABLE_HOST_SVC_CHECKS || cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS || cmd == CMD_DISABLE_HOST_SVC_NOTIFICATIONS || cmd == CMD_ENABLE_HOST_NOTIFICATIONS || cmd == CMD_DISABLE_HOST_NOTIFICATIONS) {
			if (enforce_comments_on_actions != TRUE)
				print_form_element(PRINT_COMMON_HEADER, cmd);
		}

		if (cmd == CMD_ENABLE_HOST_SVC_CHECKS || cmd == CMD_DISABLE_HOST_SVC_CHECKS || cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS || cmd == CMD_DISABLE_HOST_SVC_NOTIFICATIONS) {

			snprintf(help_text, sizeof(help_text), "This %s %s of the host too.", (cmd == CMD_ENABLE_HOST_SVC_CHECKS || cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS) ? "enables" : "disables", (cmd == CMD_ENABLE_HOST_SVC_CHECKS || cmd == CMD_DISABLE_HOST_SVC_CHECKS) ? "checks" : "notifications");
			help_text[sizeof(help_text)-1] = '\x0';

			printf("<tr><td class=\"objectDescription descriptionleft\">%s For Host Too:", (cmd == CMD_ENABLE_HOST_SVC_CHECKS || cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS) ? "Enable" : "Disable");
			print_help_box(help_text);
			printf("</td><td align=\"left\"><INPUT TYPE='checkbox' NAME='ahas'></td></tr>\n");
		}

		if (cmd == CMD_ENABLE_HOST_NOTIFICATIONS || cmd == CMD_DISABLE_HOST_NOTIFICATIONS) {

			snprintf(help_text, sizeof(help_text), "%s notifications te be sent out to child hosts.", (cmd == CMD_ENABLE_HOST_NOTIFICATIONS) ? "Enable" : "Disable");
			help_text[sizeof(help_text)-1] = '\x0';

			printf("<tr><td class=\"objectDescription descriptionleft\">%s Notifications For Child Hosts Too:", (cmd == CMD_ENABLE_HOST_NOTIFICATIONS) ? "Enable" : "Disable");
			print_help_box(help_text);
			printf("</td><td align=\"left\"><INPUT TYPE='checkbox' NAME='ptc'></td></tr>\n");
		}
		break;

	case CMD_ENABLE_NOTIFICATIONS:
	case CMD_DISABLE_NOTIFICATIONS:
	case CMD_SHUTDOWN_PROCESS:
	case CMD_RESTART_PROCESS:
	case CMD_START_EXECUTING_SVC_CHECKS:
	case CMD_STOP_EXECUTING_SVC_CHECKS:
	case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
	case CMD_ENABLE_EVENT_HANDLERS:
	case CMD_DISABLE_EVENT_HANDLERS:
	case CMD_START_OBSESSING_OVER_SVC_CHECKS:
	case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
	case CMD_ENABLE_FLAP_DETECTION:
	case CMD_DISABLE_FLAP_DETECTION:
	case CMD_ENABLE_FAILURE_PREDICTION:
	case CMD_DISABLE_FAILURE_PREDICTION:
	case CMD_ENABLE_PERFORMANCE_DATA:
	case CMD_DISABLE_PERFORMANCE_DATA:
	case CMD_START_EXECUTING_HOST_CHECKS:
	case CMD_STOP_EXECUTING_HOST_CHECKS:
	case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
	case CMD_START_OBSESSING_OVER_HOST_CHECKS:
	case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:

		if (cmd == CMD_DISABLE_NOTIFICATIONS) {
			print_form_element(PRINT_EXPIRE_DISABLE_NOTIFICATIONS, cmd);
		}
		if (enforce_comments_on_actions == TRUE) {
			print_form_element(PRINT_COMMON_HEADER, cmd);
			print_form_element(PRINT_AUTHOR, cmd);
			print_form_element(PRINT_COMMENT_BOX, cmd);
		} else	{
			if (cmd != CMD_DISABLE_NOTIFICATIONS) {
				printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
				printf("<tr><td CLASS='objectDescription' colspan=2>There are no options for this command.<br>Click the 'Commit' button to submit the command.</td></tr>\n");
			}
		}

		break;

	case CMD_PROCESS_HOST_CHECK_RESULT:
	case CMD_PROCESS_SERVICE_CHECK_RESULT:

		if (cmd == CMD_PROCESS_SERVICE_CHECK_RESULT)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER, cmd);

		if (enforce_comments_on_actions == TRUE) {
			print_form_element(PRINT_AUTHOR, cmd);
			print_form_element(PRINT_COMMENT_BOX, cmd);
		}

		snprintf(help_text, sizeof(help_text), "Set the state which should be send to %s for this %s.", PROGRAM_NAME, (cmd == CMD_PROCESS_HOST_CHECK_RESULT) ? "hosts" : "services");
		help_text[sizeof(help_text)-1] = '\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Check Result:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">\n");
		printf("\t<SELECT NAME='plugin_state'>\n");
		if (cmd == CMD_PROCESS_SERVICE_CHECK_RESULT) {
			printf("\t\t<OPTION VALUE=%d SELECTED>OK</OPTION>\n", STATE_OK);
			printf("\t\t<OPTION VALUE=%d>WARNING</OPTION>\n", STATE_WARNING);
			printf("\t\t<OPTION VALUE=%d>UNKNOWN</OPTION>\n", STATE_UNKNOWN);
			printf("\t\t<OPTION VALUE=%d>CRITICAL</OPTION>\n", STATE_CRITICAL);
		} else {
			printf("\t\t<OPTION VALUE=0 SELECTED>UP</OPTION>\n");
			printf("\t\t<OPTION VALUE=1>DOWN</OPTION>\n");
			printf("\t\t<OPTION VALUE=2>UNREACHABLE</OPTION>\n");
		}
		printf("\t</SELECT>\n");
		printf("</td></tr>\n");

		print_form_element(PRINT_CHECK_OUTPUT_BOX, cmd);
		print_form_element(PRINT_PERFORMANCE_DATA_BOX, cmd);

		break;

	case CMD_SCHEDULE_HOST_DOWNTIME:
	case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
	case CMD_SCHEDULE_SVC_DOWNTIME:

		if (cmd == CMD_SCHEDULE_SVC_DOWNTIME)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER, cmd);
		print_form_element(PRINT_AUTHOR, cmd);
		print_form_element(PRINT_COMMENT_BOX, cmd);

		snprintf(help_text, sizeof(help_text), "Define here if this downtime should get triggerd by another downtime of a particular %s.", (cmd == CMD_PROCESS_HOST_CHECK_RESULT) ? "host" : "service");
		help_text[sizeof(help_text)-1] = '\x0';

		printf("<tr id=\"trigger_select\"><td class=\"objectDescription descriptionleft\">Triggered By:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">\n");
		printf("\t<SELECT name='trigger'>\n");
		printf("\t\t<OPTION VALUE='0'>N/A</OPTION>\n");

		for (temp_downtime = scheduled_downtime_list; temp_downtime != NULL; temp_downtime = temp_downtime->next) {
			if (temp_downtime->type != HOST_DOWNTIME)
				continue;

			/* find the host... */
			temp_host = find_host(temp_downtime->host_name);

			/* make sure user has rights to view this host */
			if (is_authorized_for_host(temp_host, &current_authdata) == FALSE)
				continue;

			printf("\t\t<OPTION VALUE='%lu'>", temp_downtime->downtime_id);
			get_time_string(&temp_downtime->start_time, start_time, sizeof(start_time), SHORT_DATE_TIME);
			printf("ID: %lu, Host '%s' starting @ %s</OPTION>\n", temp_downtime->downtime_id, temp_downtime->host_name, start_time);
			found_trigger_objects = TRUE;
		}
		for (temp_downtime = scheduled_downtime_list; temp_downtime != NULL; temp_downtime = temp_downtime->next) {
			if (temp_downtime->type != SERVICE_DOWNTIME)
				continue;

			printf("\t\t<OPTION VALUE='%lu'>", temp_downtime->downtime_id);
			get_time_string(&temp_downtime->start_time, start_time, sizeof(start_time), SHORT_DATE_TIME);
			printf("ID: %lu, Service '%s' on host '%s' starting @ %s</OPTION>\n", temp_downtime->downtime_id, temp_downtime->service_description, temp_downtime->host_name, start_time);
			found_trigger_objects = TRUE;
		}

		printf("\t</SELECT>\n");
		printf("</td></tr>\n");

		/* hide "Triggerd by" selction if nothing is found to get triggerd from */
		if (!found_trigger_objects)
			printf("<tr style=\"display:none;\"><td colspan=2><script language=\"JavaScript\">document.getElementById('trigger_select').style.display = 'none';</script></td></tr>\n");

		print_form_element(PRINT_START_TIME, cmd);
		print_form_element(PRINT_END_TIME, cmd);
		print_form_element(PRINT_FIXED_FLEXIBLE_TYPE, cmd);

		if (cmd == CMD_SCHEDULE_HOST_DOWNTIME) {
			snprintf(help_text, sizeof(help_text), "Define here what should be done with the child hosts of these hosts.");
			help_text[sizeof(help_text)-1] = '\x0';

			printf("<tr><td class=\"objectDescription descriptionleft\">Child Hosts:");
			print_help_box(help_text);
			printf("</td><td align=\"left\">\n");
			printf("\t<SELECT name='childoptions'>\n");
			printf("\t\t<OPTION VALUE='0'>Do nothing with child hosts</OPTION>\n");
			printf("\t\t<OPTION VALUE='1'>Schedule triggered downtime for all child hosts</OPTION>\n");
			printf("\t\t<OPTION VALUE='2'>Schedule non-triggered downtime for all child hosts</OPTION>\n");
			printf("\t</SELECT>\n");
			printf("</td></tr>\n");
		}

		break;

	case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
	case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
	case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
	case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:

		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
		printf("<tr class=\"statusEven\" ><td width=\"50%%\" style=\"font-weight:bold;\">Hostgroup Name:</td>");
		printf("<td><INPUT TYPE='HIDDEN' NAME='hostgroup' VALUE='%s'>%s</td></tr>\n", escape_string(hostgroup_name), escape_string(hostgroup_name));

		if (enforce_comments_on_actions == TRUE) {
			print_form_element(PRINT_COMMON_HEADER, cmd);
			print_form_element(PRINT_AUTHOR, cmd);
			print_form_element(PRINT_COMMENT_BOX, cmd);
		}

		if (cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS || cmd == CMD_DISABLE_HOSTGROUP_SVC_CHECKS || cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS || cmd == CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS) {

			if (enforce_comments_on_actions != TRUE)
				print_form_element(PRINT_COMMON_HEADER, cmd);

			printf("<tr><td class=\"objectDescription descriptionleft\">%s For Hosts Too:</td><td align=\"left\">\n", (cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS || cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS) ? "Enable" : "Disable");
			printf("<INPUT TYPE='checkbox' NAME='ahas'></td></tr>\n");
		}
		break;

	case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
	case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
	case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
	case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:

		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
		printf("<tr class=\"statusEven\"><td width=\"50%%\" style=\"font-weight:bold;\">Servicegroup Name:</td>");
		printf("<td><INPUT TYPE='HIDDEN' NAME='servicegroup' VALUE='%s'>%s</td></tr>\n", escape_string(servicegroup_name), escape_string(servicegroup_name));

		if (enforce_comments_on_actions == TRUE) {
			print_form_element(PRINT_COMMON_HEADER, cmd);
			print_form_element(PRINT_AUTHOR, cmd);
			print_form_element(PRINT_COMMENT_BOX, cmd);
		}

		if (cmd == CMD_ENABLE_SERVICEGROUP_SVC_CHECKS || cmd == CMD_DISABLE_SERVICEGROUP_SVC_CHECKS || cmd == CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS || cmd == CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS) {

			if (enforce_comments_on_actions != TRUE)
				print_form_element(PRINT_COMMON_HEADER, cmd);

			printf("<tr><td class=\"objectDescription descriptionleft\">%s For Hosts Too:</td><td align=\"left\">\n", (cmd == CMD_ENABLE_SERVICEGROUP_SVC_CHECKS || cmd == CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS) ? "Enable" : "Disable");
			printf("<INPUT TYPE='checkbox' NAME='ahas'></td></tr>\n");
		}
		break;

	case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
	case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
	case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
	case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:

		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
		printf("<tr class=\"statusEven\"><td width=\"50%%\" style=\"font-weight:bold;\">");
		if (cmd == CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME || cmd == CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME)
			printf("Hostgroup Name:</td><td><INPUT TYPE='HIDDEN' NAME='hostgroup' VALUE='%s'>%s</td></tr>\n", escape_string(hostgroup_name), escape_string(hostgroup_name));
		else
			printf("Servicegroup Name:</td><td><INPUT TYPE='HIDDEN' NAME='servicegroup' VALUE='%s'>%s</td></tr>\n", escape_string(servicegroup_name), escape_string(servicegroup_name));

		print_form_element(PRINT_COMMON_HEADER, cmd);
		print_form_element(PRINT_AUTHOR, cmd);
		print_form_element(PRINT_COMMENT_BOX, cmd);
		print_form_element(PRINT_START_TIME, cmd);
		print_form_element(PRINT_END_TIME, cmd);
		print_form_element(PRINT_FIXED_FLEXIBLE_TYPE, cmd);

		if (cmd == CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME || cmd == CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME) {
			printf("<tr><td class=\"objectDescription descriptionleft\">Schedule Downtime For Hosts Too:</td><td align=\"left\">\n");
			printf("<INPUT TYPE='checkbox' NAME='ahas'></td></tr>\n");
		}
		break;

	case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
	case CMD_SEND_CUSTOM_SVC_NOTIFICATION:

		if (cmd == CMD_SEND_CUSTOM_SVC_NOTIFICATION)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER, cmd);
		print_form_element(PRINT_AUTHOR, cmd);
		print_form_element(PRINT_COMMENT_BOX, cmd);
		print_form_element(PRINT_FORCE_NOTIFICATION, cmd);
		print_form_element(PRINT_BROADCAST_NOTIFICATION, cmd);

		break;

        case CMD_CHANGE_HOST_MODATTR:
		print_object_list(PRINT_HOST_LIST);
		print_form_element(PRINT_COMMON_HEADER, cmd);
		printf("<tr class=\"statusEven\"><td width=\"50%%\" style=\"font-weight:bold;\">Modified Attributes:</td>");
		printf("<td><INPUT TYPE='HIDDEN' NAME='attr' VALUE='%lu'>", attr);
		print_modified_attributes(HTML_CONTENT, CMD_CGI, attr);
		printf("</td></tr>\n");
		break;

        case CMD_CHANGE_SVC_MODATTR:
		print_object_list(PRINT_SERVICE_LIST);
		print_form_element(PRINT_COMMON_HEADER, cmd);
		printf("<tr class=\"statusEven\"><td width=\"50%%\" style=\"font-weight:bold;\">Modified Attributes:</td>");
		printf("<td><INPUT TYPE='HIDDEN' NAME='attr' VALUE='%lu'>", attr);
		print_modified_attributes(HTML_CONTENT, CMD_CGI, attr);
		printf("</td></tr>\n");
		break;

	default:
		printf("<tr><td CLASS='objectDescription' COLSPAN=\"2\">This should not be happening... :-(</td></tr>\n");
	}


	printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
	printf("<tr CLASS='sectionHeader'><td COLSPAN=\"2\" class=\"commitButton\"><INPUT TYPE=\"submit\" NAME=\"btnSubmit\" VALUE=\"Commit\" class=\"submitButton\">&nbsp;&nbsp;|&nbsp;&nbsp;<a HREF=\"javascript:window.history.go(-1)\">Cancel</a></td></tr>\n");

	printf("</table>\n");
	printf("</td></tr></table>\n"); /* Outer frame */
	printf("</form>\n");

	printf("</div>\n");

	return;
}

void commit_command_data(int cmd) {
	char error_string[MAX_INPUT_BUFFER];
	service *temp_service;
	host *temp_host;
	hostgroup *temp_hostgroup;
	comment *temp_comment;
	scheduled_downtime *temp_downtime;
	servicegroup *temp_servicegroup = NULL;
	contact *temp_contact = NULL;
	int x = 0;
	int e = 0;
	short error_found = FALSE;
	short cmd_has_objects = FALSE;
	short row_color = 0;

	/* get authentication information */
	get_authentication_information(&current_authdata);

	/* allways set the first element to FALSE*/
	/* If there is a single COMMAND witch is not coverd correctly throught the following cases it won't get executed */
	is_authorized[x] = FALSE;

	/* get name to use for author */
	if (lock_author_names == TRUE) {
		temp_contact = find_contact(current_authdata.username);
		if (temp_contact != NULL && temp_contact->alias != NULL)
			comment_author = temp_contact->alias;
		else
			comment_author = current_authdata.username;
	}

	switch (cmd) {


	case CMD_ADD_HOST_COMMENT:
	case CMD_ADD_SVC_COMMENT:
	case CMD_ACKNOWLEDGE_HOST_PROBLEM:
	case CMD_ACKNOWLEDGE_SVC_PROBLEM:
	case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
	case CMD_SEND_CUSTOM_SVC_NOTIFICATION:

		/* make sure we have author name, and comment data... */
		check_comment_sanity(&e);

		/* clean up the comment data */
		clean_comment_data(comment_author);
		clean_comment_data(comment_data);

		if (use_ack_end_time == TRUE && (cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM || cmd == CMD_ACKNOWLEDGE_SVC_PROBLEM)) {

			time(&start_time);

			/* make sure we have end time if required */
			check_time_sanity(&e);
		} else
			end_time = 0L;

		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {

			cmd_has_objects = TRUE;

			if (commands[x].host_name == NULL)
				continue;

			/* see if the user is authorized to issue a command... */
			is_authorized[x] = FALSE;
			if (cmd == CMD_ADD_HOST_COMMENT || cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM || cmd == CMD_SEND_CUSTOM_HOST_NOTIFICATION) {
				temp_host = find_host(commands[x].host_name);
				if (is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE)
					is_authorized[x] = TRUE;
			} else {
				temp_service = find_service(commands[x].host_name, commands[x].description);
				if (is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE)
					is_authorized[x] = TRUE;
			}
		}
		break;

	case CMD_DEL_HOST_COMMENT:
	case CMD_DEL_SVC_COMMENT:

		if (enforce_comments_on_actions == TRUE) {
			check_comment_sanity(&e);
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		}

		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {

			cmd_has_objects = TRUE;

			if (multi_ids[x] == FALSE)
				continue;

			/* check the sanity of the comment id */
			if (multi_ids[x] == 0) {
				error[e++].message = strdup("Comment id cannot be 0");
				continue;
			}

			/* find the comment */
			if (cmd == CMD_DEL_HOST_COMMENT)
				temp_comment = find_host_comment(multi_ids[x]);
			else
				temp_comment = find_service_comment(multi_ids[x]);

			/* see if the user is authorized to issue a command... */
			is_authorized[x] = FALSE;
			if (cmd == CMD_DEL_HOST_COMMENT && temp_comment != NULL) {
				temp_host = find_host(temp_comment->host_name);
				if (is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE)
					is_authorized[x] = TRUE;
			}
			if (cmd == CMD_DEL_SVC_COMMENT && temp_comment != NULL) {
				temp_service = find_service(temp_comment->host_name, temp_comment->service_description);
				if (is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE)
					is_authorized[x] = TRUE;
			}
		}

		/* free comment data */
		free_comment_data();

		break;

	case CMD_DEL_HOST_DOWNTIME:
	case CMD_DEL_SVC_DOWNTIME:

		if (enforce_comments_on_actions == TRUE) {
			check_comment_sanity(&e);
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		}

		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {

			cmd_has_objects = TRUE;

			if (multi_ids[x] == FALSE)
				continue;

			/* check the sanity of the downtime id */
			if (multi_ids[x] == 0) {
				error[e++].message = strdup("Downtime id cannot be 0");
				continue;
			}

			/* find the downtime entry */
			if (cmd == CMD_DEL_HOST_DOWNTIME)
				temp_downtime = find_host_downtime(multi_ids[x]);
			else
				temp_downtime = find_service_downtime(multi_ids[x]);

			/* see if the user is authorized to issue a command... */
			is_authorized[x] = FALSE;
			if (cmd == CMD_DEL_HOST_DOWNTIME && temp_downtime != NULL) {
				temp_host = find_host(temp_downtime->host_name);
				if (is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE)
					is_authorized[x] = TRUE;
			}
			if (cmd == CMD_DEL_SVC_DOWNTIME && temp_downtime != NULL) {
				temp_service = find_service(temp_downtime->host_name, temp_downtime->service_description);
				if (is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE)
					is_authorized[x] = TRUE;
			}
		}

		/* free downtime data */
		free_downtime_data();

		break;

	case CMD_SCHEDULE_SVC_CHECK:
	case CMD_ENABLE_SVC_CHECK:
	case CMD_DISABLE_SVC_CHECK:
	case CMD_DEL_ALL_SVC_COMMENTS:
	case CMD_ENABLE_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SVC_NOTIFICATIONS:
	case CMD_ENABLE_PASSIVE_SVC_CHECKS:
	case CMD_DISABLE_PASSIVE_SVC_CHECKS:
	case CMD_ENABLE_SVC_EVENT_HANDLER:
	case CMD_DISABLE_SVC_EVENT_HANDLER:
	case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
	case CMD_PROCESS_SERVICE_CHECK_RESULT:
	case CMD_SCHEDULE_SVC_DOWNTIME:
	case CMD_DELAY_SVC_NOTIFICATION:
	case CMD_ENABLE_SVC_FLAP_DETECTION:
	case CMD_DISABLE_SVC_FLAP_DETECTION:
	case CMD_START_OBSESSING_OVER_SVC:
	case CMD_STOP_OBSESSING_OVER_SVC:

		if (cmd == CMD_SCHEDULE_SVC_DOWNTIME || enforce_comments_on_actions == TRUE) {
			/* make sure we have author and comment data */
			check_comment_sanity(&e);

			/* make sure we have start/end times for downtime */
			if (cmd == CMD_SCHEDULE_SVC_DOWNTIME)
				check_time_sanity(&e);

			/* clean up the comment data if scheduling downtime */
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		}

		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {

			cmd_has_objects = TRUE;

			if (commands[x].host_name == NULL || commands[x].description == NULL)
				continue;

			is_authorized[x] = FALSE;
			temp_service = find_service(commands[x].host_name, commands[x].description);
			if (is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE)
				is_authorized[x] = TRUE;
		}

		/* make sure we have passive check info (if necessary) */
		if (cmd == CMD_PROCESS_SERVICE_CHECK_RESULT && !strcmp(plugin_output, ""))
			error[e++].message = strdup("Check output cannot be blank");

		/* make sure we have a notification delay (if necessary) */
		if (cmd == CMD_DELAY_SVC_NOTIFICATION && notification_delay <= 0)
			error[e++].message = strdup("Notification delay must be greater than 0");

		/* make sure we have check time (if necessary) */
		if (cmd == CMD_SCHEDULE_SVC_CHECK && start_time == (time_t)0)
			error[e++].message = strdup("Start time must be non-zero or bad format has been submitted");

		break;

	case CMD_ENABLE_NOTIFICATIONS:
	case CMD_DISABLE_NOTIFICATIONS:
	case CMD_SHUTDOWN_PROCESS:
	case CMD_RESTART_PROCESS:
	case CMD_START_EXECUTING_SVC_CHECKS:
	case CMD_STOP_EXECUTING_SVC_CHECKS:
	case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
	case CMD_ENABLE_EVENT_HANDLERS:
	case CMD_DISABLE_EVENT_HANDLERS:
	case CMD_START_OBSESSING_OVER_SVC_CHECKS:
	case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
	case CMD_ENABLE_FLAP_DETECTION:
	case CMD_DISABLE_FLAP_DETECTION:
	case CMD_ENABLE_FAILURE_PREDICTION:
	case CMD_DISABLE_FAILURE_PREDICTION:
	case CMD_ENABLE_PERFORMANCE_DATA:
	case CMD_DISABLE_PERFORMANCE_DATA:
	case CMD_START_EXECUTING_HOST_CHECKS:
	case CMD_STOP_EXECUTING_HOST_CHECKS:
	case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
	case CMD_START_OBSESSING_OVER_HOST_CHECKS:
	case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:

                if (use_disabled_notif_end_time == TRUE && cmd == CMD_DISABLE_NOTIFICATIONS) {

                        time(&start_time);

                        /* make sure we have end time if required */
                        check_time_sanity(&e);
                } else
                        end_time = 0L;

		if (enforce_comments_on_actions == TRUE) {
			check_comment_sanity(&e);
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		}

		/* see if the user is authorized to issue a command... */
		is_authorized[x] = FALSE;
		if (is_authorized_for_system_commands(&current_authdata) == TRUE)
			is_authorized[x] = TRUE;
		break;

	case CMD_ENABLE_HOST_SVC_CHECKS:
	case CMD_DISABLE_HOST_SVC_CHECKS:
	case CMD_DEL_ALL_HOST_COMMENTS:
	case CMD_SCHEDULE_HOST_SVC_CHECKS:
	case CMD_ENABLE_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOST_NOTIFICATIONS:
	case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
	case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
	case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
	case CMD_ENABLE_HOST_EVENT_HANDLER:
	case CMD_DISABLE_HOST_EVENT_HANDLER:
	case CMD_ENABLE_HOST_CHECK:
	case CMD_DISABLE_HOST_CHECK:
	case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
	case CMD_SCHEDULE_HOST_DOWNTIME:
	case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
	case CMD_DELAY_HOST_NOTIFICATION:
	case CMD_ENABLE_HOST_FLAP_DETECTION:
	case CMD_DISABLE_HOST_FLAP_DETECTION:
	case CMD_PROCESS_HOST_CHECK_RESULT:
	case CMD_ENABLE_PASSIVE_HOST_CHECKS:
	case CMD_DISABLE_PASSIVE_HOST_CHECKS:
	case CMD_SCHEDULE_HOST_CHECK:
	case CMD_START_OBSESSING_OVER_HOST:
	case CMD_STOP_OBSESSING_OVER_HOST:
	case CMD_DEL_DOWNTIME_BY_HOST_NAME:

		if (cmd == CMD_SCHEDULE_HOST_DOWNTIME || cmd == CMD_SCHEDULE_HOST_SVC_DOWNTIME || enforce_comments_on_actions == TRUE) {
			/* make sure we have author and comment data */
			check_comment_sanity(&e);

			/* make sure we have start/end times for downtime */
			if (cmd == CMD_SCHEDULE_HOST_DOWNTIME || cmd == CMD_SCHEDULE_HOST_SVC_DOWNTIME)
				check_time_sanity(&e);

			/* clean up the comment data if scheduling downtime */
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		}

		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {

			cmd_has_objects = TRUE;

			if (commands[x].host_name == NULL)
				continue;

			/* see if the user is authorized to issue a command... */
			is_authorized[x] = FALSE;
			temp_host = find_host(commands[x].host_name);
			if (is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE)
				is_authorized[x] = TRUE;
		}

		/* make sure we have a notification delay (if necessary) */
		if (cmd == CMD_DELAY_HOST_NOTIFICATION && notification_delay <= 0)
			error[e++].message = strdup("Notification delay must be greater than 0");

		/* make sure we have check time (if necessary) */
		if ((cmd == CMD_SCHEDULE_HOST_CHECK || cmd == CMD_SCHEDULE_HOST_SVC_CHECKS) && start_time == (time_t)0)
			error[e++].message = strdup("Start time must be non-zero or bad format has been submitted");

		/* make sure we have passive check info (if necessary) */
		if (cmd == CMD_PROCESS_HOST_CHECK_RESULT && !strcmp(plugin_output, ""))
			error[e++].message = strdup("Check output cannot be blank");

		break;

	case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
	case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
	case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
	case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
	case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
	case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
	case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
	case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
	case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
	case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
	case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
	case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:


		if (cmd == CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME || cmd == CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME \
		        || cmd == CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME || cmd == CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME) {
			/* make sure we have author and comment data */
			check_comment_sanity(&e);

			/* make sure we have start/end times for downtime */
			check_time_sanity(&e);

			/* clean up the comment data if scheduling downtime */
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		} else if (enforce_comments_on_actions == TRUE) {
			check_comment_sanity(&e);
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		}

		/* see if the user is authorized to issue a command... */
		is_authorized[x] = FALSE;
		if (cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS	|| cmd == CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS || \
		        cmd == CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS || cmd == CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS || \
		        cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS		|| cmd == CMD_DISABLE_HOSTGROUP_SVC_CHECKS || \
		        cmd == CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME	|| cmd == CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME) {
			temp_hostgroup = find_hostgroup(hostgroup_name);
			if (is_authorized_for_hostgroup_commands(temp_hostgroup, &current_authdata) == TRUE)
				is_authorized[x] = TRUE;
		} else {
			temp_servicegroup = find_servicegroup(servicegroup_name);
			if (is_authorized_for_servicegroup_commands(temp_servicegroup, &current_authdata) == TRUE)
				is_authorized[x] = TRUE;
		}

		break;

	case CMD_CHANGE_HOST_MODATTR:
	case CMD_CHANGE_SVC_MODATTR:

                for (x = 0; x < NUMBER_OF_STRUCTS; x++) {

                        cmd_has_objects = TRUE;

                        if (commands[x].host_name == NULL)
                                continue;

                        /* see if the user is authorized to issue a command... */
                        is_authorized[x] = FALSE;
                        if (cmd == CMD_CHANGE_HOST_MODATTR) {
                                temp_host = find_host(commands[x].host_name);
                                if (is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE)
                                        is_authorized[x] = TRUE;
                        } else {
                                temp_service = find_service(commands[x].host_name, commands[x].description);
                                if (is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE)
                                        is_authorized[x] = TRUE;
                        }

			/* do not allow other attributes than reset (0) */
			if (attr != MODATTR_NONE) {
				error[e++].message = strdup("You cannot change modified attributes other than reset them!");
			}
                }

		break;

	default:
		print_generic_error_message("Sorry Dave, I can't let you do that...", "Executing an unknown command? Shame on you!", 2);

		return;
	}


	/*
	 * these are supposed to be implanted inside the
	 * completed commands shipped off to Icinga and
	 * must therefore never contain ';'
	 */
	for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
		if (commands[x].host_name == NULL)
			continue;

		if (strchr(commands[x].host_name, ';')) {
			snprintf(error_string, sizeof(error_string), "The hostname \"%s\" contains a semicolon", commands[x].host_name);
			error_string[sizeof(error_string)-1] = '\x0';
			error[e++].message = (char *)strdup(error_string);
		}
		if (commands[x].description != NULL && strchr(commands[x].description, ';')) {
			snprintf(error_string, sizeof(error_string), "The service description \"%s\" on host \"%s\" contains a semicolon", commands[x].description, commands[x].host_name);
			error_string[sizeof(error_string)-1] = '\x0';
			error[e++].message = strdup(error_string);
		}
	}
	if (hostgroup_name && strchr(hostgroup_name, ';'))
		error[e++].message = strdup("The hostgroup name contains a semicolon");
	if (servicegroup_name && strchr(servicegroup_name, ';'))
		error[e++].message = strdup("The servicegroup name  contains a semicolon");

	printf("<BR><DIV align='center'>\n");

	/* if Icinga isn't checking external commands, don't do anything... */
	if (check_external_commands == FALSE) {
		print_generic_error_message("Sorry, but Icinga is currently not checking for external commands, so your command will not be committed!", "Read the documentation for information on how to enable external commands...", 2);

		return;
	}

	/* to be safe, we are going to REQUIRE that the authentication functionality is enabled... */
	if (use_authentication == FALSE) {
		print_generic_error_message("Sorry Dave, I can't let you do that...", "It seems that you have chosen to not use the authentication functionality of the CGIs. I don't want to be personally responsible for what may happen as a result of allowing unauthorized users to issue commands to Icinga, so you'll have to disable this safeguard if you are really stubborn and want to invite trouble. Read the section on CGI authentication in the HTML documentation to learn how you can enable authentication and why you should want to.", 2);

		return;
	}

	/* Check if we found errors which preventing us from submiting the command */
	if (e > 0) {
		printf("<DIV CLASS='errorBox'>\n");
		printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>", url_images_path, CMD_STOP_ICON);
		printf("<td CLASS='errorMessage'>Following errors occured.</td></tr></table></DIV>\n");
		printf("<table cellspacing=0 cellpadding=0 border=0 class='errorTable'>\n");
		for (e = 0; e < NUMBER_OF_STRUCTS; e++) {
			if (error[e].message == NULL)
				continue;
			printf("<tr><td class='errorString'>ERROR:</td><td class='errorContent'>%s</td></tr>\n", error[e].message);
		}
		printf("</table>\n</DIV>\n");
		printf("<BR>\n");
		printf("<table cellspacing=0 cellpadding=0 border=0 class='BoxWidth'><tr>\n");
		printf("<td align='left' width='50%%'><input type='submit' value='< Go back and fix it' onClick='window.history.go(-1);' class='submitButton'></td>\n");
		printf("<td align='right' width='50%%'><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></td>\n");
		printf("</tr></table></DIV>");
		return;
	}

	/* Let's see if we have a command witch dosn't have any host, services or downtime/comment id's and check the authorisation */
	if (cmd_has_objects == FALSE && is_authorized[0] == FALSE) {
		print_generic_error_message("Sorry, but you are not authorized to commit the specified command.", "Read the section of the documentation that deals with authentication and authorization in the CGIs for more information.", 2);

		return;
	}

	/* everything looks okay, so let's go ahead and commit the command... */
	commit_command(cmd);

	/* for commands without objects get the first result*/
	if (cmd_has_objects == FALSE) {
		if (submit_result[0] == OK) {
			printf("<DIV CLASS='successBox'>\n");
			printf("<DIV CLASS='successMessage'>Your command request was successfully submitted to %s for processing.<BR><BR>\n", PROGRAM_NAME);
			printf("Note: It may take a while before the command is actually processed.</DIV>\n");
			printf("</DIV>\n");
			printf("<BR><input type='submit' value='Done' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
		} else {
			print_generic_error_message("An error occurred while attempting to commit your command for processing.", "Unfortunately I can't determine the root cause of this problem.", 2);
		}
	} else {
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (cmd == CMD_DEL_HOST_COMMENT || cmd == CMD_DEL_SVC_COMMENT || cmd == CMD_DEL_HOST_DOWNTIME || cmd == CMD_DEL_SVC_DOWNTIME) {
				if (multi_ids[x] == FALSE)
					continue;
			} else {
				if (commands[x].host_name == NULL)
					continue;
			}

			if (is_authorized[x] == FALSE || submit_result[x] == ERROR) {
				error_found = TRUE;
				break;
			}
		}

		if (error_found) {
			print_generic_error_message("An error occurred while attempting to commit your command for processing.", "Not all commands could be send off successfully...", 0);
		} else {
			printf("<DIV CLASS='successBox'>\n");
			printf("<DIV CLASS='successMessage'>Your command requests were successfully submitted to %s for processing.<BR><BR>\n", PROGRAM_NAME);
			printf("Note: It may take a while before the commands are actually processed.</DIV>\n");
			printf("</DIV>\n");
		}

		printf("<BR>\n");
		printf("<TABLE CELLSPACING='0' CELLPADDING=0 BORDER=0 CLASS='BoxWidth'>\n");
		printf("<tr class='BoxWidth'><td width='33%%'></td><td width='33%%' align='center'><input type='submit' value='Done' onClick='window.history.go(-2);' class='submitButton'></td><td width='33%%' align='right'>\n");
		if (!error_found)
			printf("<input type='submit' value='Let me see what has been done' onClick=\"document.getElementById('sumCommit').style.display = '';\" class='submitButton'>\n");
		printf("</td></TR></TABLE>\n");
		printf("<BR><BR>\n");

		printf("<TABLE CELLSPACING='0' CELLPADDING='0' ID='sumCommit' %s><TR><TD CLASS='boxFrame BoxWidth'>\n", (error_found) ? "" : "style='display:none;'");
		printf("<table cellspacing=2 cellpadding=0 border=0 class='contentTable'>\n");
		if (cmd == CMD_DEL_HOST_COMMENT || cmd == CMD_DEL_SVC_COMMENT)
			printf("<tr class='sumHeader'><td width='80%%'>Comment ID</td><td width='20%%'>Status</td></tr>\n");
		else if (cmd == CMD_DEL_HOST_DOWNTIME || cmd == CMD_DEL_SVC_DOWNTIME)
			printf("<tr class='sumHeader'><td width='80%%'>Downtime ID</td><td width='20%%'>Status</td></tr>\n");
		else
			printf("<tr class='sumHeader'><td width='40%%'>Host</td><td width='40%%'>Service</td><td width='20%%'>Status</td></tr>\n");

		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {

			if (cmd == CMD_DEL_HOST_COMMENT || cmd == CMD_DEL_SVC_COMMENT || cmd == CMD_DEL_HOST_DOWNTIME || cmd == CMD_DEL_SVC_DOWNTIME) {
				if (multi_ids[x] == FALSE)
					continue;
				row_color = (row_color == 0) ? 1 : 0;
				printf("<tr class='status%s'><td>%lu</td><td>", (row_color == 0) ? "Even" : "Odd ", multi_ids[x]);
			} else {
				if (commands[x].host_name == NULL)
					continue;
				row_color = (row_color == 0) ? 1 : 0;

				printf("<tr class='status%s'><td>%s</td><td>%s</td><td>", (row_color == 0) ? "Even" : "Odd ", commands[x].host_name, (commands[x].description != NULL) ? commands[x].description : "N/A");
			}
			if (is_authorized[x] == FALSE)
				printf("<DIV class='commitFailed'>Not Authorized</DIV>");
			else if (submit_result[x] == ERROR)
				printf("<DIV class='commitFailed'>FAILED</DIV>");
			else if (submit_result[x] == OK)
				printf("<DIV class='commitSuccess'>Successful</DIV>");
			else
				printf("<DIV class='commitUnknown'>Unknown</DIV>");

			printf("</TD><TR>\n");
		}
		printf("</TABLE>\n");
		printf("</TD></TR></TABLE></DIV>\n");
	}
	return;
}


/** @brief doe's some checks before passing data to write_command_to_file
 *
 *  Actually defines the command cmd_submitf.
**/
__attribute__((format(printf, 2, 3)))
static int cmd_submitf(int id, const char *fmt, ...) {
	char cmd[MAX_EXTERNAL_COMMAND_LENGTH];
	const char *command;
	int len, len2;
	va_list ap;

	command = extcmd_get_name(id);

	/*
	 * We disallow sending 'CHANGE' commands from the cgi's
	 * until we do proper session handling to prevent cross-site
	 * request forgery
	 * 2012-04-23 MF: Allow those and do proper checks on the cmds
	 * for changed mod attr
	 */
	/*if (!command || (strlen(command) > 6 && !memcmp("CHANGE", command, 6)))
		return ERROR;
	*/

	len = snprintf(cmd, sizeof(cmd) - 1, "[%lu] %s;", time(NULL), command);

	if (len < 0)
		return ERROR;

	if (fmt) {
		va_start(ap, fmt);
		len2 = vsnprintf(&cmd[len], sizeof(cmd) - len - 1, fmt, ap);
		va_end(ap);
		if (len2 < 0)
			return ERROR;
	}

	return write_command_to_file(cmd);
}

int commit_command(int cmd) {
	time_t current_time;
	time_t scheduled_time;
	time_t notification_time;
	char *temp_buffer = NULL;
	int x = 0, dummy;

	/* get the current time */
	time(&current_time);

	/* get the scheduled time */
	scheduled_time = current_time + (schedule_delay * 60);

	/* get the notification time */
	notification_time = current_time + (notification_delay * 60);

	/* decide how to form the command line... */
	switch (cmd) {

		/* commands without arguments */
	case CMD_START_EXECUTING_SVC_CHECKS:
	case CMD_STOP_EXECUTING_SVC_CHECKS:
	case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
	case CMD_ENABLE_EVENT_HANDLERS:
	case CMD_DISABLE_EVENT_HANDLERS:
	case CMD_START_OBSESSING_OVER_SVC_CHECKS:
	case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
	case CMD_ENABLE_FLAP_DETECTION:
	case CMD_DISABLE_FLAP_DETECTION:
	case CMD_ENABLE_FAILURE_PREDICTION:
	case CMD_DISABLE_FAILURE_PREDICTION:
	case CMD_ENABLE_PERFORMANCE_DATA:
	case CMD_DISABLE_PERFORMANCE_DATA:
	case CMD_START_EXECUTING_HOST_CHECKS:
	case CMD_STOP_EXECUTING_HOST_CHECKS:
	case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
	case CMD_START_OBSESSING_OVER_HOST_CHECKS:
	case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, NULL);
		break;

		/* simple host commands */
	case CMD_ENABLE_HOST_FLAP_DETECTION:
	case CMD_DISABLE_HOST_FLAP_DETECTION:
	case CMD_ENABLE_PASSIVE_HOST_CHECKS:
	case CMD_DISABLE_PASSIVE_HOST_CHECKS:
	case CMD_START_OBSESSING_OVER_HOST:
	case CMD_STOP_OBSESSING_OVER_HOST:
	case CMD_DEL_ALL_HOST_COMMENTS:
	case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
	case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
	case CMD_ENABLE_HOST_EVENT_HANDLER:
	case CMD_DISABLE_HOST_EVENT_HANDLER:
	case CMD_ENABLE_HOST_CHECK:
	case CMD_DISABLE_HOST_CHECK:
	case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s", commands[x].host_name);
		}
		break;

		/* simple service commands */
	case CMD_ENABLE_SVC_FLAP_DETECTION:
	case CMD_DISABLE_SVC_FLAP_DETECTION:
	case CMD_ENABLE_PASSIVE_SVC_CHECKS:
	case CMD_DISABLE_PASSIVE_SVC_CHECKS:
	case CMD_START_OBSESSING_OVER_SVC:
	case CMD_STOP_OBSESSING_OVER_SVC:
	case CMD_DEL_ALL_SVC_COMMENTS:
	case CMD_ENABLE_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SVC_NOTIFICATIONS:
	case CMD_ENABLE_SVC_EVENT_HANDLER:
	case CMD_DISABLE_SVC_EVENT_HANDLER:
	case CMD_ENABLE_SVC_CHECK:
	case CMD_DISABLE_SVC_CHECK:
	case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%s", commands[x].host_name, commands[x].description);
		}
		break;

	case CMD_ADD_HOST_COMMENT:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%d;%s;%s", commands[x].host_name, persistent_comment, comment_author, comment_data);
		}
		break;

	case CMD_ADD_SVC_COMMENT:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%s;%d;%s;%s", commands[x].host_name, commands[x].description, persistent_comment, comment_author, comment_data);
		}
		break;

	case CMD_DEL_HOST_COMMENT:
	case CMD_DEL_SVC_COMMENT:
	case CMD_DEL_HOST_DOWNTIME:
	case CMD_DEL_SVC_DOWNTIME:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (multi_ids[x] == FALSE)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%lu", multi_ids[x]);
		}
		break;

	case CMD_DEL_DOWNTIME_BY_HOST_NAME:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s", commands[x].host_name);
		}
		break;

	case CMD_DELAY_HOST_NOTIFICATION:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%lu", commands[x].host_name, notification_time);
		}
		break;

	case CMD_DELAY_SVC_NOTIFICATION:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%s;%lu", commands[x].host_name, commands[x].description, notification_time);
		}
		break;

	case CMD_SCHEDULE_SVC_CHECK:
	case CMD_SCHEDULE_FORCED_SVC_CHECK:
		if (force_check == TRUE)
			cmd = CMD_SCHEDULE_FORCED_SVC_CHECK;
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%s;%lu", commands[x].host_name, commands[x].description, start_time);
		}
		break;

	case CMD_ENABLE_NOTIFICATIONS:
	case CMD_SHUTDOWN_PROCESS:
	case CMD_RESTART_PROCESS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%lu", scheduled_time);
		break;

	case CMD_DISABLE_NOTIFICATIONS:
		if (is_authorized[x]) {
			/* we should expire the disabled notifications */
			if(end_time > 0) {
				cmd = CMD_DISABLE_NOTIFICATIONS_EXPIRE_TIME;
				submit_result[x] = cmd_submitf(cmd, "%lu;%lu", scheduled_time, end_time);
				my_free(temp_buffer);
			} else {
				submit_result[x] = cmd_submitf(cmd, "%lu", scheduled_time);
			}
		}
		break;

	case CMD_ENABLE_HOST_SVC_CHECKS:
	case CMD_DISABLE_HOST_SVC_CHECKS:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s", commands[x].host_name);
		}
		if (affect_host_and_services == TRUE) {
			cmd = (cmd == CMD_ENABLE_HOST_SVC_CHECKS) ? CMD_ENABLE_HOST_CHECK : CMD_DISABLE_HOST_CHECK;
			for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
				if (commands[x].host_name == NULL)
					continue;
				if (is_authorized[x])
					submit_result[x] |= cmd_submitf(cmd, "%s", commands[x].host_name);
			}
		}
		break;

	case CMD_SCHEDULE_HOST_SVC_CHECKS:
		if (force_check == TRUE)
			cmd = CMD_SCHEDULE_FORCED_HOST_SVC_CHECKS;
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%lu", commands[x].host_name, scheduled_time);
		}
		break;

	case CMD_ENABLE_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOST_NOTIFICATIONS:
		if (propagate_to_children == TRUE)
			cmd = (cmd == CMD_ENABLE_HOST_NOTIFICATIONS) ? CMD_ENABLE_HOST_AND_CHILD_NOTIFICATIONS : CMD_DISABLE_HOST_AND_CHILD_NOTIFICATIONS;
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s", commands[x].host_name);
		}
		break;

	case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s", commands[x].host_name);
		}
		if (affect_host_and_services == TRUE) {
			cmd = (cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS) ? CMD_ENABLE_HOST_NOTIFICATIONS : CMD_DISABLE_HOST_NOTIFICATIONS;
			for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
				if (commands[x].host_name == NULL)
					continue;
				if (is_authorized[x])
					submit_result[x] |= cmd_submitf(cmd, "%s", commands[x].host_name);
			}
		}
		break;

	case CMD_ACKNOWLEDGE_HOST_PROBLEM:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x]) {
				if (end_time > 0) {
					cmd = CMD_ACKNOWLEDGE_HOST_PROBLEM_EXPIRE;
					dummy = asprintf(&temp_buffer, "%s - The acknowledgement expires at: %s.", comment_data, end_time_string);
					submit_result[x] = cmd_submitf(cmd, "%s;%d;%d;%d;%lu;%s;%s", commands[x].host_name, (sticky_ack == TRUE) ? ACKNOWLEDGEMENT_STICKY : ACKNOWLEDGEMENT_NORMAL, send_notification, persistent_comment, end_time, comment_author, temp_buffer);
					my_free(temp_buffer);
				} else
					submit_result[x] = cmd_submitf(cmd, "%s;%d;%d;%d;%s;%s", commands[x].host_name, (sticky_ack == TRUE) ? ACKNOWLEDGEMENT_STICKY : ACKNOWLEDGEMENT_NORMAL, send_notification, persistent_comment, comment_author, comment_data);
			}
		}
		break;

	case CMD_ACKNOWLEDGE_SVC_PROBLEM:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x]) {
				if (end_time > 0) {
					cmd = CMD_ACKNOWLEDGE_SVC_PROBLEM_EXPIRE;
					dummy = asprintf(&temp_buffer, "%s - The acknowledgement expires at: %s.", comment_data, end_time_string);
					submit_result[x] = cmd_submitf(cmd, "%s;%s;%d;%d;%d;%lu;%s;%s", commands[x].host_name, commands[x].description, (sticky_ack == TRUE) ? ACKNOWLEDGEMENT_STICKY : ACKNOWLEDGEMENT_NORMAL, send_notification, persistent_comment, end_time, comment_author, temp_buffer);
					my_free(temp_buffer);
				} else
					submit_result[x] = cmd_submitf(cmd, "%s;%s;%d;%d;%d;%s;%s", commands[x].host_name, commands[x].description, (sticky_ack == TRUE) ? ACKNOWLEDGEMENT_STICKY : ACKNOWLEDGEMENT_NORMAL, send_notification, persistent_comment, comment_author, comment_data);
			}
		}
		break;

	case CMD_PROCESS_SERVICE_CHECK_RESULT:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%s;%d;%s|%s", commands[x].host_name, commands[x].description, plugin_state, plugin_output, performance_data);
		}
		break;

	case CMD_PROCESS_HOST_CHECK_RESULT:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%d;%s|%s", commands[x].host_name, plugin_state, plugin_output, performance_data);
		}
		break;

	case CMD_SCHEDULE_HOST_DOWNTIME:
		if (child_options == 1)
			cmd = CMD_SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME;
		else if (child_options == 2)
			cmd = CMD_SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME;
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%lu;%lu;%d;%lu;%lu;%s;%s", commands[x].host_name, start_time, end_time, fixed, triggered_by, duration, comment_author, comment_data);
		}
		break;

	case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%lu;%lu;%d;%lu;%lu;%s;%s", commands[x].host_name, start_time, end_time, fixed, triggered_by, duration, comment_author, comment_data);
		}
		break;

	case CMD_SCHEDULE_SVC_DOWNTIME:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%s;%lu;%lu;%d;%lu;%lu;%s;%s", commands[x].host_name, commands[x].description, start_time, end_time, fixed, triggered_by, duration, comment_author, comment_data);
		}
		break;

	case CMD_SCHEDULE_HOST_CHECK:
		if (force_check == TRUE)
			cmd = CMD_SCHEDULE_FORCED_HOST_CHECK;
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%lu", commands[x].host_name, start_time);
		}
		break;

	case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%d;%s;%s", commands[x].host_name, (force_notification | broadcast_notification), comment_author, comment_data);
		}
		break;

	case CMD_SEND_CUSTOM_SVC_NOTIFICATION:
		for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%s;%d;%s;%s", commands[x].host_name, commands[x].description, (force_notification | broadcast_notification), comment_author, comment_data);
		}
		break;


		/***** HOSTGROUP COMMANDS *****/

	case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s", hostgroup_name);
		if (affect_host_and_services == TRUE) {
			cmd = (cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS) ? CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS : CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS;
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(cmd, "%s", hostgroup_name);
		}
		break;

	case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s", hostgroup_name);
		break;

	case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
	case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s", hostgroup_name);
		if (affect_host_and_services == TRUE) {
			cmd = (cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS) ? CMD_ENABLE_HOSTGROUP_HOST_CHECKS : CMD_DISABLE_HOSTGROUP_HOST_CHECKS;
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(cmd, "%s", hostgroup_name);
		}
		break;

	case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s;%lu;%lu;%d;0;%lu;%s;%s", hostgroup_name, start_time, end_time, fixed, duration, comment_author, comment_data);
		break;

	case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s;%lu;%lu;%d;0;%lu;%s;%s", hostgroup_name, start_time, end_time, fixed, duration, comment_author, comment_data);
		if (affect_host_and_services == TRUE) {
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME, "%s;%lu;%lu;%d;0;%lu;%s;%s", hostgroup_name, start_time, end_time, fixed, duration, comment_author, comment_data);
		}
		break;


		/***** SERVICEGROUP COMMANDS *****/

	case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s", servicegroup_name);
		if (affect_host_and_services == TRUE) {
			cmd = (cmd == CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS) ? CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS : CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS;
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(cmd, "%s", servicegroup_name);
		}
		break;

	case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s", servicegroup_name);
		break;

	case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
	case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s", servicegroup_name);
		if (affect_host_and_services == TRUE) {
			cmd = (cmd == CMD_ENABLE_SERVICEGROUP_SVC_CHECKS) ? CMD_ENABLE_SERVICEGROUP_HOST_CHECKS : CMD_DISABLE_SERVICEGROUP_HOST_CHECKS;
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(cmd, "%s", servicegroup_name);
		}
		break;

	case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s;%lu;%lu;%d;0;%lu;%s;%s", servicegroup_name, start_time, end_time, fixed, duration, comment_author, comment_data);
		break;

	case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd, "%s;%lu;%lu;%d;0;%lu;%s;%s", servicegroup_name, start_time, end_time, fixed, duration, comment_author, comment_data);
		if (affect_host_and_services == TRUE) {
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME, "%s;%lu;%lu;%d;0;%lu;%s;%s", servicegroup_name, start_time, end_time, fixed, duration, comment_author, comment_data);
		}
		break;

        case CMD_CHANGE_HOST_MODATTR:
                for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
                        if (commands[x].host_name == NULL)
                                continue;

			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%lu", commands[x].host_name, attr);
		}
		break;

        case CMD_CHANGE_SVC_MODATTR:
                for (x = 0; x < NUMBER_OF_STRUCTS; x++) {
                        if (commands[x].host_name == NULL)
                                continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd, "%s;%s;%lu", commands[x].host_name, commands[x].description, attr);
		}
		break;

	default:
		submit_result[x] = ERROR;
		break;
	}

	return OK;
}

int write_command_to_file(char *cmd) {
	char *buffer;
	char *ip_address;
	int dummy;
	char *p;
	FILE *fp;
	struct stat statbuf;
	char error_string[MAX_INPUT_BUFFER];

	/*
	 * Commands are not allowed to have newlines in them, as
	 * that allows malicious users to hand-craft requests that
	 * bypass the access-restrictions.
	 */
	if (!cmd || !*cmd || strchr(cmd, '\n'))
		return ERROR;

	/* bail out if the external command file doesn't exist */
	if (stat(command_file, &statbuf)) {
		snprintf(error_string, sizeof(error_string), "Error: Could not stat() command file '%s'!", command_file);
		error_string[sizeof(error_string)-1] = '\x0';

		print_generic_error_message(error_string, "The external command file may be missing, Icinga may not be running, and/or Icinga may not be checking external commands.", 2);

		return ERROR;
	}

	/* open the command for writing (since this is a pipe, it will really be appended) */
	fp = fopen(command_file, "w");
	if (fp == NULL) {
		snprintf(error_string, sizeof(error_string), "Error: Could not open command file '%s' for update!", command_file);
		error_string[sizeof(error_string)-1] = '\x0';

		print_generic_error_message(error_string, "The permissions on the external command file and/or directory may be incorrect. Read the FAQs on how to setup proper permissions.", 2);

		return ERROR;
	}

	if (use_logging == TRUE) {
		// find closing bracket in cmd line
		p = strchr(cmd, ']');
		// if found get everything after closing bracket
		if (p != NULL)
			p += 2;
		else	// get complete command line
			p = &cmd[0];

		/* get remote address */
		ip_address = strdup(getenv("REMOTE_ADDR"));

		/* construct log entry */
		dummy = asprintf(&buffer, "EXTERNAL COMMAND: %s;%s;%s", current_authdata.username, (ip_address != NULL) ? ip_address : "unknown remote address", p);

		/* write command to cgi log */
		write_to_cgi_log(buffer);

		/* log comments if forced */
		if (enforce_comments_on_actions == TRUE) {
			my_free(buffer);
			dummy = asprintf(&buffer, "FORCED COMMENT: %s;%s;%s;%s", current_authdata.username, (ip_address != NULL) ? ip_address : "unknown remote address", comment_author, comment_data);
			write_to_cgi_log(buffer);
		}
		my_free(buffer);
	}

	/* write the command to file */
	fprintf(fp, "%s\n", cmd);

	/* flush buffer */
	fflush(fp);

	fclose(fp);

	return OK;
}

void clean_comment_data(char *buffer) {
	int x;
	int y;

	y = (int)strlen(buffer);

	for (x = 0; x < y; x++) {
		if (buffer[x] == ';' || buffer[x] == '\n' || buffer[x] == '\r')
			buffer[x] = ' ';
	}

	return;
}

void check_comment_sanity(int *e) {
	if (!strcmp(comment_author, ""))
		error[(*e)++].message = strdup("Author name was not entered");
	if (!strcmp(comment_data, ""))
		error[(*e)++].message = strdup("Comment data was not entered");

	return;
}

void check_time_sanity(int *e) {
	if (start_time == (time_t)0)
		error[(*e)++].message = strdup("Start time can't be zero or date format couldn't be recognized correctly");
	if (end_time == (time_t)0)
		error[(*e)++].message = strdup("End time can't be zero or date format couldn't be recognized correctly");
	if (end_time < start_time)
		error[(*e)++].message = strdup("End date before start date");

	return;
}
