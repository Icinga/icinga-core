/**************************************************************************
 *
 * CMD.C - Icinga Command CGI
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2010 Icinga Development Team (http://www.icinga.org)
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *************************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/comments.h"
#include "../include/downtime.h"

#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/getcgi.h"

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
extern int  log_external_commands_user;

extern int  content_type;
extern int  display_header;
extern int  daemon_check;

extern int date_format;

extern scheduled_downtime *scheduled_downtime_list;
extern comment *comment_list;

#define MAX_AUTHOR_LENGTH		64
#define MAX_COMMENT_LENGTH		1024

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
#define PRINT_HOST_LIST			17
#define PRINT_SERVICE_LIST		18
#define PRINT_COMMENT_LIST		19
#define PRINT_DOWNTIME_LIST		20

char *host_name="";
char *hostgroup_name="";
char *servicegroup_name="";
char *service_desc="";
char *comment_author="";
char *comment_data="";
char *start_time_string="";
char *end_time_string="";

char help_text[MAX_INPUT_BUFFER]="";

int notification_delay=0;
int schedule_delay=0;
int persistent_comment=FALSE;
int sticky_ack=FALSE;
int send_notification=FALSE;
int force_check=FALSE;
int plugin_state=STATE_OK;
char plugin_output[MAX_INPUT_BUFFER]="";
char performance_data[MAX_INPUT_BUFFER]="";
time_t start_time=0L;
time_t end_time=0L;
int affect_host_and_services=FALSE;
int propagate_to_children=FALSE;
int fixed=FALSE;
unsigned long duration=0L;
unsigned long triggered_by=0L;
int child_options=0;
int force_notification=0;
int broadcast_notification=0;

int command_type=CMD_NONE;
int command_mode=CMDMODE_REQUEST;

authdata current_authdata;

void request_command_data(int);
void commit_command_data(int);
int commit_command(int);
int write_command_to_file(char *);
void clean_comment_data(char *);

void print_form_element(int,int);
void print_object_list(int);
void print_help_box(char *);

void check_comment_sanity(int*);
void check_time_sanity(int*);

int process_cgivars(void);

int string_to_time(char *,time_t *);

/* Set a limit of 500 structs, which is around 125 checks total*/
#define NUMBER_OF_STRUCTS 500

/* Struct to hold information for batch processing */
struct hostlist {
	char *host_name;
	char *description;
};

/* store the errors we find during processing */
struct errorlist {
	char *message;
};

/* Initialize the struct */
struct hostlist commands[NUMBER_OF_STRUCTS];

/* initialze the error list */
struct errorlist error[NUMBER_OF_STRUCTS];

/* Hold IDs of comments and downtimes */
unsigned long multi_ids[NUMBER_OF_STRUCTS];

/* store the authentication status when data gets checked to submited */
short is_authorized[NUMBER_OF_STRUCTS];

/* store the result of each object which get submited */
short submit_result[NUMBER_OF_STRUCTS];

int CGI_ID=CMD_CGI_ID;

/* Everything needs a main */
int main(void){
	int result=OK;

	/* get the arguments passed in the URL */
	process_cgivars();

	/* reset internal variables */
	reset_cgi_vars();

	/* read the CGI configuration file */
	result=read_cgi_config_file(get_cgi_config_location());
	if(result==ERROR){
		document_header(CGI_ID,FALSE);
		if(content_type==WML_CONTENT)
			printf("<p>Error: Could not open CGI config file!</p>\n");
		else
			cgi_config_file_error(get_cgi_config_location());
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read the main configuration file */
	result=read_main_config_file(main_config_file);
	if(result==ERROR){
		document_header(CGI_ID,FALSE);
		if(content_type==WML_CONTENT)
			printf("<p>Error: Could not open main config file!</p>\n");
		else
			main_config_file_error(main_config_file);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* This requires the date_format parameter in the main config file */
	if (strcmp(start_time_string,""))
		string_to_time(start_time_string,&start_time);

	if (strcmp(end_time_string,""))
		string_to_time(end_time_string,&end_time);


	/* read all object configuration data */
	result=read_all_object_configuration_data(main_config_file,READ_ALL_OBJECT_DATA);
	if(result==ERROR){
		document_header(CGI_ID,FALSE);
		if(content_type==WML_CONTENT)
			printf("<p>Error: Could not read object config data!</p>\n");
		else
			object_data_error();
		document_footer(CGI_ID);
		return ERROR;
	}

	document_header(CGI_ID,TRUE);

	/* get authentication information */
	get_authentication_information(&current_authdata);

	if(display_header==TRUE){

		/* Giving credits to stop.png image source */
		printf("\n<!-- Image \"stop.png\" has been taken from \"http://fedoraproject.org/wiki/Template:Admon/caution\" -->\n\n");

		/* begin top table */
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=33%%>\n");
		display_info_table("External Command Interface",FALSE,&current_authdata, daemon_check);
		printf("</td>\n");

		/* center column of the first row */
		printf("<td align=center valign=top width=33%%>\n");
		printf("</td>\n");

		/* right column of the first row */
		printf("<td align=right valign=bottom width=33%%>\n");

		/* display context-sensitive help */
		if(command_mode==CMDMODE_COMMIT)
			display_context_help(CONTEXTHELP_CMD_COMMIT);
		else
			display_context_help(CONTEXTHELP_CMD_INPUT);

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");
	}

	/* if no command was specified... */
	if(command_type==CMD_NONE){
		if(content_type==WML_CONTENT)
			printf("<p>Error: No command specified!</p>\n");
		else {
			printf("<BR><DIV align='center'><DIV CLASS='errorBox'>\n");
			printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
			printf("<td class='errorMessage'>Error: No command was specified</td></tr></table></DIV>\n");
			printf("</DIV>\n");
			printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
		}
	}

	/* if this is the first request for a command, present option */
	else if(command_mode==CMDMODE_REQUEST)
		request_command_data(command_type);

	/* the user wants to commit the command */
	else if(command_mode==CMDMODE_COMMIT)
		commit_command_data(command_type);

	document_footer(CGI_ID);

	/* free allocated memory */
	free_memory();
	free_object_data();

	return OK;
}

int process_cgivars(void){
	char **variables;
	int error=FALSE;
	int x;
	int z = 0;

	variables=getcgivars();

	/* Process the variables */
	for(x=0;variables[x]!=NULL;x++){

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if(strlen(variables[x])>=MAX_INPUT_BUFFER-1){
			x++;
			continue;
		}

		/* we found the command type */
		else if(!strcmp(variables[x],"cmd_typ")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			command_type=atoi(variables[x]);
		}

		/* we found the command mode */
		else if(!strcmp(variables[x],"cmd_mod")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			command_mode=atoi(variables[x]);
		}

		/* we found a comment id or a downtime id*/
		else if(!strcmp(variables[x],"com_id") || !strcmp(variables[x],"down_id")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			multi_ids[z]=strtoul(variables[x],NULL,10);
			z++;
		}

		/* we found the notification delay */
		else if(!strcmp(variables[x],"not_dly")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			notification_delay=atoi(variables[x]);
		}

		/* we found the schedule delay */
		else if(!strcmp(variables[x],"sched_dly")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			schedule_delay=atoi(variables[x]);
		}

		/* we found the comment author */
		else if(!strcmp(variables[x],"com_author")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			if((comment_author=(char *)strdup(variables[x]))==NULL)
				comment_author="";
			strip_html_brackets(comment_author);
		}

		/* we found the comment data */
		else if(!strcmp(variables[x],"com_data")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			if((comment_data=(char *)strdup(variables[x]))==NULL)
				comment_data="";
			strip_html_brackets(comment_data);
		}

		/* we found the host name */
		else if(!strcmp(variables[x],"host")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			if((host_name=(char *)strdup(variables[x]))==NULL)
				host_name="";
			else {
				strip_html_brackets(host_name);

				/* Store hostname in struct */
				commands[x].host_name = host_name;
			}
		}

		/* we found the hostgroup name */
		else if(!strcmp(variables[x],"hostgroup")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
				}

			if((hostgroup_name=(char *)strdup(variables[x]))==NULL)
				hostgroup_name="";
			strip_html_brackets(hostgroup_name);
		}

		/* we found the service name */
		else if(!strcmp(variables[x],"service")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			if((service_desc=(char *)strdup(variables[x]))==NULL)
				service_desc="";
			else {
				strip_html_brackets(service_desc);

				/* Store service description in struct */
				commands[(x-2)].description = service_desc;
			}
		}

		/* we found the servicegroup name */
		else if(!strcmp(variables[x],"servicegroup")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			if((servicegroup_name=(char *)strdup(variables[x]))==NULL)
				servicegroup_name="";
			strip_html_brackets(servicegroup_name);
		}

		/* we got the persistence option for a comment */
		else if(!strcmp(variables[x],"persistent"))
			persistent_comment=TRUE;

		/* we got the notification option for an acknowledgement */
		else if(!strcmp(variables[x],"send_notification"))
			send_notification=TRUE;

		/* we got the acknowledgement type */
		else if(!strcmp(variables[x],"sticky_ack"))
			sticky_ack=TRUE;

		/* we got the service check force option */
		else if(!strcmp(variables[x],"force_check"))
			force_check=TRUE;

		/* we got the option to affect host and all its services */
		else if(!strcmp(variables[x],"ahas"))
			affect_host_and_services=TRUE;

		/* we got the option to propagate to child hosts */
		else if(!strcmp(variables[x],"ptc"))
			propagate_to_children=TRUE;

		/* we got the option for fixed downtime */
		else if(!strcmp(variables[x],"fixed")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			fixed=(atoi(variables[x])>0)?TRUE:FALSE;
		}

		/* we got the triggered by downtime option */
		else if(!strcmp(variables[x],"trigger")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			triggered_by=strtoul(variables[x],NULL,10);
		}

		/* we got the child options */
		else if(!strcmp(variables[x],"childoptions")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			child_options=atoi(variables[x]);
		}

		/* we found the plugin output */
		else if(!strcmp(variables[x],"plugin_output")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			/* protect against buffer overflows */
			if(strlen(variables[x])>=MAX_INPUT_BUFFER-1){
				error=TRUE;
				break;
			} else
				strcpy(plugin_output,variables[x]);
		}

		/* we found the performance data */
		else if(!strcmp(variables[x],"performance_data")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			/* protect against buffer overflows */
			if(strlen(variables[x])>=MAX_INPUT_BUFFER-1){
				error=TRUE;
				break;
			} else
				strcpy(performance_data,variables[x]);
		}

		/* we found the plugin state */
		else if(!strcmp(variables[x],"plugin_state")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			plugin_state=atoi(variables[x]);
		}

		/* we found the hour duration */
		else if(!strcmp(variables[x],"hours")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			if(atoi(variables[x])<0){
				error=TRUE;
				break;
			}
			duration+=(unsigned long)(atoi(variables[x])*3600);
		}

		/* we found the minute duration */
		else if(!strcmp(variables[x],"minutes")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			if(atoi(variables[x])<0){
				error=TRUE;
				break;
			}
			duration+=(unsigned long)(atoi(variables[x])*60);
		}

		/* we found the start time */
		else if(!strcmp(variables[x],"start_time")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			start_time_string=(char *)malloc(strlen(variables[x])+1);
			if(start_time_string==NULL)
				start_time_string="";
			else
				strcpy(start_time_string,variables[x]);
		}

		/* we found the end time */
		else if(!strcmp(variables[x],"end_time")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			end_time_string=(char *)malloc(strlen(variables[x])+1);
			if(end_time_string==NULL)
				end_time_string="";
			else
				strcpy(end_time_string,variables[x]);
		}

		/* we found the content type argument */
		else if(!strcmp(variables[x],"content")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}
			if(!strcmp(variables[x],"wml")){
				content_type=WML_CONTENT;
				display_header=FALSE;
			} else
				content_type=HTML_CONTENT;
		}

		/* we found the forced notification option */
		else if(!strcmp(variables[x],"force_notification"))
			force_notification=NOTIFICATION_OPTION_FORCED;

		/* we found the broadcast notification option */
		else if(!strcmp(variables[x],"broadcast_notification"))
			broadcast_notification=NOTIFICATION_OPTION_BROADCAST;

		/* we got the persistence option for a comment */
		else if(!strcmp(variables[x],"nodaemoncheck"))
			daemon_check = FALSE;

		}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

/* print the list of affected objects */
void print_object_list(int list_type) {
	int x = 0;
	int row_color = 0;

	printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
	printf("<tr CLASS=\"sectionHeader\"><td COLSPAN=\"2\" >Affected Objects</td></tr>\n");

	if(list_type==PRINT_SERVICE_LIST)
		printf("<tr class=\"objectDescription\"><td width=\"50%%\">Host</td><td width=\"50%%\">Service</td></tr>\n");
	else
		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");

	for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {

		if (list_type==PRINT_HOST_LIST || list_type==PRINT_SERVICE_LIST ){
			if (commands[x].host_name == NULL)
				continue;
		} else {
			if (multi_ids[x] == FALSE)
				continue;
		}

		row_color = (row_color==0)?1:0;

		printf("<tr class=\"status%s\"><td width=\"50%%\"",(row_color==0)?"Even":"Odd ");
		if (list_type==PRINT_SERVICE_LIST){
			/* if hostname is empty print inputbox instead */
			if(!strcmp(commands[x].host_name,""))
				printf("><INPUT TYPE='TEXT' NAME='host' SIZE=30></td>");
			else
				printf("><INPUT TYPE='HIDDEN' NAME='host' VALUE='%s'>%s</td>",escape_string(commands[x].host_name),escape_string(commands[x].host_name));
			/* if service description is empty print inputbox instead */
			if(!strcmp(commands[x].description,""))
				printf("<td><INPUT TYPE='TEXT' NAME='service' SIZE=30></td></tr>\n");
			else
				printf("<td><INPUT TYPE='HIDDEN' NAME='service' VALUE='%s'>%s</td></tr>\n",escape_string(commands[x].description),escape_string(commands[x].description));
		} else if (list_type==PRINT_HOST_LIST){
			/* if hostname is empty print inputbox instead */
			if(!strcmp(commands[x].host_name,""))
				printf(" style=\"font-weight:bold;\">Host:</td><td><INPUT TYPE='TEXT' NAME='host' SIZE=30></td></tr>\n");
			else
				printf(" style=\"font-weight:bold;\">Host:</td><td><INPUT TYPE='HIDDEN' NAME='host' VALUE='%s'>%s</td></tr>\n",escape_string(commands[x].host_name),escape_string(commands[x].host_name));
		} else if (list_type==PRINT_COMMENT_LIST){
			printf(" style=\"font-weight:bold;\">Comment ID:</td><td><INPUT TYPE='HIDDEN' NAME='com_id' VALUE='%lu'>%lu</td></tr>\n",multi_ids[x],multi_ids[x]);
		} else if (list_type==PRINT_DOWNTIME_LIST){
			printf(" style=\"font-weight:bold;\">Scheduled Downtime ID:</td><td><INPUT TYPE='HIDDEN' NAME='down_id' VALUE='%lu'>%lu</td></tr>\n",multi_ids[x],multi_ids[x]);
		}
	}

	return;
}

/* print the mouseover box with help */
void print_help_box(char *content) {

	printf("<img src='%s%s' onMouseOver=\"return tooltip('<table border=0 width=100%% height=100%%>",url_images_path,CONTEXT_HELP_ICON1);
	printf("<tr><td>%s</td></tr>",content);
	printf("</table>', '&nbsp;&nbsp;&nbsp;Help', 'border:1, width:500, xoffset:-250, yoffset:25, bordercolor:#333399, title_padding:2px, titletextcolor:#FFFFFF, backcolor:#CCCCFF');\" onMouseOut=\"return hideTip()\"");
	printf(" BORDER=0>");
	return;
}

/* templates for the different form elements */
void print_form_element(int element,int cmd) {
	time_t t;
	char start_time[MAX_DATETIME_LENGTH];
	char buffer[MAX_INPUT_BUFFER];

	switch(element) {

	case PRINT_COMMON_HEADER:
		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
		printf("<tr><td COLSPAN=\"2\" CLASS='sectionHeader'>Common Data</td></tr>\n");
		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
		break;

	case PRINT_AUTHOR:
		printf("<tr><td class=\"objectDescription descriptionleft\">Author (Your Name):</td><td align=\"left\">");
		if (lock_author_names==TRUE)
			printf("<INPUT TYPE='HIDDEN' NAME='com_author' VALUE='%s'>%s</td></tr>\n",escape_string(comment_author),escape_string(comment_author));
		else
			printf("<INPUT TYPE='INPUT' NAME='com_author' VALUE='%s'></td></tr>\n",escape_string(comment_author));
		break;

	case PRINT_COMMENT_BOX:

		strcpy(help_text,"If you work with other administrators, you may find it useful to share information about a host/service "
				 "that is having problems if more than one of you may be working on it. "
				 "Make sure to enter a brief description of what you are doing.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Comment:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<TEXTAREA ID=\"com_data\" NAME='com_data' COLS=25 ROWS=2 onkeyup=\"check_input();\">%s</TEXTAREA>",escape_string(comment_data));
		printf("<BR><DIV ID='com_data_error' class=\"inputError\" style=\"display:none;\">Comment data can't be send empty</DIV>");
		printf("</td></tr>\n");
		break;

	case PRINT_CHECK_OUTPUT_BOX:

		snprintf(help_text,sizeof(help_text),"Fill in the exact output string which sould be sent to %s",PROGRAM_NAME);
		help_text[sizeof(help_text)-1]='\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Check Output:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<TEXTAREA ID=\"plugin_output\" NAME='plugin_output' COLS=25 ROWS=2  onkeyup=\"check_input();\"></TEXTAREA>");
		printf("<BR><DIV ID='plugin_output_error' class=\"inputError\" style=\"display:none;\">Output string can't be send empty</DIV>");
		printf("</td></tr>\n");
		break;

	case PRINT_PERFORMANCE_DATA_BOX:

		snprintf(help_text,sizeof(help_text),"Fill in the exact performance data string which sould be sent to %s",PROGRAM_NAME);
		help_text[sizeof(help_text)-1]='\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Performance Data:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<TEXTAREA NAME='performance_data' COLS=25 ROWS=2></TEXTAREA></td></tr>\n");
		break;

	case PRINT_STICKY_ACK:

		strcpy(help_text,"If you want acknowledgement to disable notifications until the host/service recovers, check this option.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Sticky Acknowledgement:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='sticky_ack' CHECKED></td></tr>\n");
		break;

	case PRINT_SEND_NOTFICATION:

		strcpy(help_text,"If you do not want an acknowledgement notification sent out to the appropriate contacts, uncheck this option.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Send Notification:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='send_notification' CHECKED></td></tr>\n");
		break;

	case PRINT_PERSISTENT:

		if (cmd==CMD_ACKNOWLEDGE_HOST_PROBLEM || cmd==CMD_ACKNOWLEDGE_SVC_PROBLEM)
			strcpy(help_text,"If you would like the comment to remain once the acknowledgement is removed, check this checkbox.");
		else {
			snprintf(help_text,sizeof(help_text),"If you uncheck this option, the comment will automatically be deleted the next time %s is restarted.",PROGRAM_NAME);
			help_text[sizeof(help_text)-1]='\x0';
		}
		printf("<tr><td class=\"objectDescription descriptionleft\">Persistent%s:",(cmd==CMD_ACKNOWLEDGE_HOST_PROBLEM || cmd==CMD_ACKNOWLEDGE_SVC_PROBLEM)?" Comment":"");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='persistent' %s></td></tr>\n",( persistent_ack_comments==TRUE || cmd==CMD_ADD_HOST_COMMENT || cmd==CMD_ADD_SVC_COMMENT )?"CHECKED":"");
		break;

	case PRINT_NOTIFICATION_DELAY:

		strcpy(help_text,"The notification delay will be disregarded if the host/service changes state before the next notification is scheduled to be sent out.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Notification Delay (minutes from now):");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='TEXT' ID='not_dly' NAME='not_dly' VALUE='%d' SIZE=\"4\">",notification_delay);
		printf("<BR><DIV ID='not_dly_error' class=\"inputError\" style=\"display:none;\">Notification delay can't be zero</DIV>");
		printf("</td></tr>\n");
		break;

	case PRINT_START_TIME:
	case PRINT_END_TIME:
	case PRINT_CHECK_TIME:
		time(&t);
		if (element == PRINT_END_TIME )
			t+=(unsigned long)7200;
		get_time_string(&t,buffer,sizeof(buffer)-1,SHORT_DATE_TIME);
		printf("<tr><td class=\"objectDescription descriptionleft\">");
		if (element == PRINT_START_TIME ){
			strcpy(help_text,"Set the start date/time for the downtime.");
			printf("Start Time:");
		}else if (element == PRINT_END_TIME ){
			strcpy(help_text,"Set the end date/time for the downtime.");
			printf("End Time:");
		}else{
			strcpy(help_text,"Set the date/time when this check should be schedule to.");
			printf("Check Time:");
		}
		print_help_box(help_text);
		printf("</td><td align=\"left\"><INPUT TYPE='TEXT' NAME='%s_time' VALUE='%s' SIZE=\"25\"></td></tr>\n",(element == PRINT_END_TIME )?"end":"start",buffer);
		break;

	case PRINT_FIXED_FLEXIBLE_TYPE:

		snprintf(help_text,sizeof(help_text),"If you select the <i>fixed</i> option, the downtime will be in effect between the start and end times you specify. If you do not select the <i>fixed</i> "
				 "option, %s will treat this as <i>flexible</i> downtime. Flexible downtime starts when the host goes down or becomes unreachable / service becomes critical (sometime between the "
				 "start and end times you specified) and lasts as long as the duration of time you enter. The duration fields do not apply for fixed downtime.",PROGRAM_NAME);
		help_text[sizeof(help_text)-1]='\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Type:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">\n");

		printf("\t<SELECT ID=\"flexible_selection\" NAME='fixed' onChange=\"if (document.getElementById('flexible_selection').selectedIndex == 0) document.getElementById('fd_row').style.display = 'none'; else document.getElementById('fd_row').style.display = '';\">\n");
		printf("\t\t<OPTION VALUE=1\">Fixed</OPTION>\n");
		printf("\t\t<OPTION VALUE=0\">Flexible</OPTION>\n");
		printf("\t</SELECT>\n");

		snprintf(help_text,sizeof(help_text),"Enter here the duration of the downtime. %s will automatically delete the downtime after this time expired.",PROGRAM_NAME);
		help_text[sizeof(help_text)-1]='\x0';

		printf("<tr id=\"fd_row\" style=\"display:none;\"><td class=\"objectDescription descriptionleft\">Flexible Duration:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">\n");
		printf("\t<table border=0  cellspacing=0 cellpadding=0>\n");
		printf("\t\t<tr>\n");
		printf("\t\t\t<td><INPUT TYPE='TEXT' NAME='hours' VALUE='2' SIZE=2 MAXLENGTH=2></td>\n");
		printf("\t\t\t<td width=\"50\">&nbsp;Hours</td>\n");
		printf("\t\t\t<td><INPUT TYPE='TEXT' NAME='minutes' VALUE='0' SIZE=2 MAXLENGTH=2></td>\n");
		printf("\t\t\t<td width=\"50\">&nbsp;Minutes</td>\n");
		printf("\t\t</tr>\n");
		printf("\t</table>\n");
		printf("</td></tr>\n");
		break;

	case PRINT_FORCE_CHECK:

		snprintf(help_text,sizeof(help_text),"If you select this option, %s will force a check of the host/service regardless of both what time the scheduled check occurs and whether or not checks are enabled for the host/service.", PROGRAM_NAME);
		help_text[sizeof(help_text)-1]='\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Force Check:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='force_check' %s></td></tr>\n",(force_check==TRUE)?"CHECKED":"");
		break;

	case PRINT_BROADCAST_NOTIFICATION:

		strcpy(help_text,"Selecting this option causes the notification to be sent out to all normal (non-escalated) and escalated contacts. These options allow you to override the normal notification logic if you need to get an important message out.");

		printf("<tr><td class=\"objectDescription descriptionleft\">Broadcast:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">");
		printf("<INPUT TYPE='checkbox' NAME='broadcast_notification'></td></tr>\n");
		break;

	case PRINT_FORCE_NOTIFICATION:

		snprintf(help_text,sizeof(help_text),"Custom notifications normally follow the regular notification logic in %s.  Selecting this option will force the notification to be sent out, regardless of the time restrictions, whether or not notifications are enabled, etc.", PROGRAM_NAME);
		help_text[sizeof(help_text)-1]='\x0';

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

/* Print form to commit a command */
void request_command_data(int cmd){
	char start_time[MAX_DATETIME_LENGTH];
	contact *temp_contact;
	scheduled_downtime *temp_downtime;
	host *temp_host=NULL;
	char action[MAX_INPUT_BUFFER];
	int found_trigger_objects=FALSE;

	/* get default name to use for comment author */
	temp_contact=find_contact(current_authdata.username);
	if(temp_contact!=NULL && temp_contact->alias!=NULL)
		comment_author=temp_contact->alias;
	else
		comment_author=current_authdata.username;

	printf("<BR>");

	switch(cmd){

	case CMD_ADD_HOST_COMMENT:
	case CMD_ADD_SVC_COMMENT:
		snprintf(action,sizeof(action),"Add %s comments",(cmd==CMD_ADD_HOST_COMMENT)?"host":"service");
		break;

	case CMD_DEL_HOST_COMMENT:
	case CMD_DEL_SVC_COMMENT:
		snprintf(action,sizeof(action),"Delete %s comments",(cmd==CMD_DEL_HOST_COMMENT)?"host":"service");
		break;

	case CMD_DELAY_HOST_NOTIFICATION:
	case CMD_DELAY_SVC_NOTIFICATION:
		snprintf(help_text,sizeof(help_text),"This command is used to delay the next problem notification that is sent out for specified %s. The notification delay will be disregarded if "
			"the %s changes state before the next notification is scheduled to be sent out.	 This command has no effect if the %s are currently %s.",(cmd==CMD_DELAY_HOST_NOTIFICATION)?"hosts":"services",(cmd==CMD_DELAY_HOST_NOTIFICATION)?"hosts":"services",(cmd==CMD_DELAY_HOST_NOTIFICATION)?"hosts":"services",(cmd==CMD_DELAY_HOST_NOTIFICATION)?"UP":"in an OK state");
		snprintf(action,sizeof(action),"Delay a %s notification",(cmd==CMD_DELAY_HOST_NOTIFICATION)?"host":"service");
		break;

	case CMD_SCHEDULE_HOST_CHECK:
	case CMD_SCHEDULE_SVC_CHECK:
		snprintf(help_text,sizeof(help_text),"This command is used to schedule the next check of these %s. %s will re-queue the %s to be checked at the time you specify.",(cmd==CMD_SCHEDULE_HOST_CHECK)?"hosts":"services",PROGRAM_NAME,(cmd==CMD_SCHEDULE_HOST_CHECK)?"host":"service");
		snprintf(action,sizeof(action),"Schedule %s checks",(cmd==CMD_SCHEDULE_HOST_CHECK)?"host":"service");
		break;

	case CMD_ENABLE_SVC_CHECK:
	case CMD_DISABLE_SVC_CHECK:
		snprintf(action,sizeof(action),"%s active service checks on a program-wide basis",(cmd==CMD_ENABLE_SVC_CHECK)?"Enable":"Disable");
		break;

	case CMD_ENABLE_NOTIFICATIONS:
	case CMD_DISABLE_NOTIFICATIONS:
		snprintf(help_text,sizeof(help_text),"This command is used to %s host and service notifications on a program-wide basis",(cmd==CMD_ENABLE_NOTIFICATIONS)?"enable":"disable");
		snprintf(action,sizeof(action),"%s notifications on a program-wide basis",(cmd==CMD_ENABLE_NOTIFICATIONS)?"Enable":"Disable");
		break;

	case CMD_SHUTDOWN_PROCESS:
	case CMD_RESTART_PROCESS:
		snprintf(action,sizeof(action),"%s the %s process",(cmd==CMD_SHUTDOWN_PROCESS)?"Shutdown":"Restart", PROGRAM_NAME);
		break;

	case CMD_ENABLE_HOST_SVC_CHECKS:
	case CMD_DISABLE_HOST_SVC_CHECKS:
		if (cmd==CMD_ENABLE_HOST_SVC_CHECKS)
			snprintf(help_text,sizeof(help_text),"This command is used to enable active checks of all services associated with the specified host");
		else {
			snprintf(help_text,sizeof(help_text),"This command is used to disable active checks of all services associated with the specified host. "
				"When a service is disabled %s will not monitor the service. Doing this will prevent any notifications being sent out for "
				"the specified service while it is disabled. In order to have %s check the service in the future you will have to re-enable the service. "
				"Note that disabling service checks may not necessarily prevent notifications from being sent out about the host which those services are associated with.",PROGRAM_NAME,PROGRAM_NAME);
		}
		snprintf(action,sizeof(action),"%s active checks of all services on these hosts",(cmd==CMD_ENABLE_HOST_SVC_CHECKS)?"Enable":"Disable");
		break;

	case CMD_SCHEDULE_HOST_SVC_CHECKS:
		snprintf(action,sizeof(action),"Schedule a check of all services for these hosts");
		break;

	case CMD_DEL_ALL_HOST_COMMENTS:
	case CMD_DEL_ALL_SVC_COMMENTS:
		snprintf(action,sizeof(action),"Delete all comments for these %s",(cmd==CMD_DEL_ALL_HOST_COMMENTS)?"hosts":"services");
		break;

	case CMD_ENABLE_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SVC_NOTIFICATIONS:
		snprintf(action,sizeof(action),"%s notifications for these services",(cmd==CMD_ENABLE_SVC_NOTIFICATIONS)?"Enable":"Disable");
		break;

	case CMD_ENABLE_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOST_NOTIFICATIONS:
		snprintf(action,sizeof(action),"%s notifications for these hosts",(cmd==CMD_ENABLE_HOST_NOTIFICATIONS)?"Enable":"Disable");
		break;

	case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
	case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
		snprintf(help_text,sizeof(help_text),"This command is used to %s notifications for all hosts and services that lie <i>beyond</i> the specified host (from the view of %s).",(cmd==CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST)?"enable":"disable",PROGRAM_NAME);
		snprintf(action,sizeof(action),"%s notifications for all hosts and services beyond these hosts",(cmd==CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST)?"Enable":"Disable");
		break;

	case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
		snprintf(action,sizeof(action),"%s notifications for all services on these hosts",(cmd==CMD_ENABLE_HOST_SVC_NOTIFICATIONS)?"Enable":"Disable");
		break;

	case CMD_ACKNOWLEDGE_HOST_PROBLEM:
	case CMD_ACKNOWLEDGE_SVC_PROBLEM:
		snprintf(action,sizeof(action),"Acknowledge %s problems",(cmd==CMD_ACKNOWLEDGE_HOST_PROBLEM)?"host":"service");
		break;

	case CMD_START_EXECUTING_HOST_CHECKS:
	case CMD_STOP_EXECUTING_HOST_CHECKS:
		snprintf(action,sizeof(action),"%s executing host checks on a program-wide basis",(cmd==CMD_START_EXECUTING_HOST_CHECKS)?"Start":"Stop");
		break;

	case CMD_START_EXECUTING_SVC_CHECKS:
	case CMD_STOP_EXECUTING_SVC_CHECKS:
		if (cmd==CMD_START_EXECUTING_SVC_CHECKS)
			snprintf(help_text,sizeof(help_text),"This command is used to resume execution of active service checks on a program-wide basis. Individual services which are disabled will still not be checked.");
		else
			snprintf(help_text,sizeof(help_text),"This command is used to temporarily stop %s from actively executing any service checks.  This will have the side effect of preventing any notifications from being sent out (for any and all services and hosts). "
				"Service checks will not be executed again until you issue a command to resume service check execution.", PROGRAM_NAME);
		snprintf(action,sizeof(action),"%s executing active service checks",(cmd==CMD_START_EXECUTING_SVC_CHECKS)?"Start":"Stop");
		break;

	case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
		snprintf(help_text,sizeof(help_text),"This command is used to make %s %s accepting passive service check results that it finds in the external command file.", PROGRAM_NAME,(cmd==CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS)?"start":"stop");
		snprintf(action,sizeof(action),"%s accepting passive service checks on a program-wide basis",(cmd==CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS)?"Start":"Stop");
		break;

	case CMD_ENABLE_PASSIVE_SVC_CHECKS:
	case CMD_DISABLE_PASSIVE_SVC_CHECKS:
		if (cmd==CMD_ENABLE_PASSIVE_SVC_CHECKS)
			snprintf(help_text,sizeof(help_text),"This command is used to allow %s to accept passive service check results that it finds in the external command file for this particular service.", PROGRAM_NAME);
		else
			snprintf(help_text,sizeof(help_text),"This command is used to stop %s accepting passive service check results that it finds in the external command file for this particular service. All passive check results that are found for this service will be ignored.", PROGRAM_NAME);
		snprintf(action,sizeof(action),"%s accepting passive service checks for these services",(cmd==CMD_ENABLE_PASSIVE_SVC_CHECKS)?"Start":"Stop");
		break;

	case CMD_ENABLE_EVENT_HANDLERS:
	case CMD_DISABLE_EVENT_HANDLERS:
		if (cmd==CMD_ENABLE_EVENT_HANDLERS)
			snprintf(help_text,sizeof(help_text),"This command is used to allow %s to run host and service event handlers.", PROGRAM_NAME);
		else
			snprintf(help_text,sizeof(help_text),"This command is used to temporarily prevent %s from running any host or service event handlers.", PROGRAM_NAME);
		snprintf(action,sizeof(action),"%s event handlers on a program-wide basis",(cmd==CMD_ENABLE_EVENT_HANDLERS)?"Enable":"Disable");
		break;

	case CMD_ENABLE_HOST_EVENT_HANDLER:
	case CMD_DISABLE_HOST_EVENT_HANDLER:
		snprintf(help_text,sizeof(help_text),"This command is used to %s the event handler for the selected hosts",(cmd==CMD_ENABLE_HOST_EVENT_HANDLER)?"enable":"disable");
		snprintf(action,sizeof(action),"%s the event handler for these hosts",(cmd==CMD_ENABLE_HOST_EVENT_HANDLER)?"Enable":"Disable");
		break;

	case CMD_ENABLE_SVC_EVENT_HANDLER:
	case CMD_DISABLE_SVC_EVENT_HANDLER:
		snprintf(help_text,sizeof(help_text),"This command is used to %s the event handler for the selected services",(cmd==CMD_ENABLE_SVC_EVENT_HANDLER)?"enable":"disable");
		snprintf(action,sizeof(action),"%s the event handler for these services",(cmd==CMD_ENABLE_SVC_EVENT_HANDLER)?"Enable":"Disable");
		break;

	case CMD_ENABLE_HOST_CHECK:
	case CMD_DISABLE_HOST_CHECK:
		if (cmd==CMD_DISABLE_HOST_CHECK)
			snprintf(help_text,sizeof(help_text),"This command is used to temporarily prevent %s from actively checking the status of a particular host. If %s needs to check the status of this host, it will assume that it is in the same state that it was in before checks were disabled.", PROGRAM_NAME, PROGRAM_NAME);
		snprintf(action,sizeof(action),"%s active host checks",(cmd==CMD_ENABLE_HOST_CHECK)?"Enable":"Disable");
		break;

	case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
	case CMD_START_OBSESSING_OVER_SVC_CHECKS:
		if (cmd==CMD_START_OBSESSING_OVER_SVC_CHECKS)
			snprintf(help_text,sizeof(help_text),"This command is used to have %s start obsessing over service checks. Read the documentation on distributed monitoring for more information on this.", PROGRAM_NAME);
		snprintf(action,sizeof(action),"%s obsessing over service checks on a program-wide basis",(cmd==CMD_STOP_OBSESSING_OVER_SVC_CHECKS)?"Stop":"Start");
		break;

	case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
	case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
		snprintf(help_text,sizeof(help_text),"This command is used to remove an acknowledgement for %s problems. Once the acknowledgement is removed, notifications may start being "
				"sent out about the %s problem.",(cmd==CMD_REMOVE_HOST_ACKNOWLEDGEMENT)?"host":"service",(cmd==CMD_REMOVE_HOST_ACKNOWLEDGEMENT)?"host":"service");
		snprintf(action,sizeof(action),"Remove %s acknowledgements",(cmd==CMD_REMOVE_HOST_ACKNOWLEDGEMENT)?"host":"service");
		break;

	case CMD_SCHEDULE_HOST_DOWNTIME:
	case CMD_SCHEDULE_SVC_DOWNTIME:
		snprintf(help_text,sizeof(help_text),"This command is used to schedule downtime for these %s. During the specified downtime, %s will not send notifications out about the %s. "
			"When the scheduled downtime expires, %s will send out notifications for this %s as it normally would.	Scheduled downtimes are preserved "
			"across program shutdowns and restarts.",(cmd==CMD_SCHEDULE_HOST_DOWNTIME)?"hosts":"services",PROGRAM_NAME,(cmd==CMD_SCHEDULE_HOST_DOWNTIME)?"hosts":"services",PROGRAM_NAME,(cmd==CMD_SCHEDULE_HOST_DOWNTIME)?"hosts":"services");
		snprintf(action,sizeof(action),"Schedule downtime for these %s",(cmd==CMD_SCHEDULE_HOST_DOWNTIME)?"hosts":"services");
		break;

	case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
		snprintf(help_text,sizeof(help_text),"This command is used to schedule downtime for a particular host and all of its services.	During the specified downtime, %s will not send notifications out about the host. "
			"Normally, a host in downtime will not send alerts about any services in a failed state. This option will explicitly set downtime for all services for this host. "
			"When the scheduled downtime expires, %s will send out notifications for this host as it normally would. Scheduled downtimes are preserved "
			"across program shutdowns and restarts.",PROGRAM_NAME,PROGRAM_NAME);
		snprintf(action,sizeof(action),"Schedule downtime for all services for these hosts and the hosts themself");
		break;

	case CMD_PROCESS_HOST_CHECK_RESULT:
	case CMD_PROCESS_SERVICE_CHECK_RESULT:
		snprintf(help_text,sizeof(help_text),"This command is used to submit a passive check result for these %s. "
		"It is particularly useful for resetting security-related %s to %s states once they have been dealt with.",(cmd==CMD_PROCESS_HOST_CHECK_RESULT)?"hosts":"services",(cmd==CMD_PROCESS_HOST_CHECK_RESULT)?"hosts":"services",(cmd==CMD_PROCESS_HOST_CHECK_RESULT)?"UP":"OK");

		snprintf(action,sizeof(action),"Submit a passive check result for these %s",(cmd==CMD_PROCESS_HOST_CHECK_RESULT)?"hosts":"services");
		break;

	case CMD_ENABLE_HOST_FLAP_DETECTION:
	case CMD_DISABLE_HOST_FLAP_DETECTION:
		snprintf(action,sizeof(action),"%s flap detection for these hosts",(cmd==CMD_ENABLE_HOST_FLAP_DETECTION)?"Enable":"Disable");
		break;

	case CMD_ENABLE_SVC_FLAP_DETECTION:
	case CMD_DISABLE_SVC_FLAP_DETECTION:
		snprintf(action,sizeof(action),"%s flap detection for these services",(cmd==CMD_ENABLE_SVC_FLAP_DETECTION)?"Enable":"Disable");
		break;

	case CMD_ENABLE_FLAP_DETECTION:
	case CMD_DISABLE_FLAP_DETECTION:
		snprintf(action,sizeof(action),"%s flap detection for hosts and services on a program-wide basis",(cmd==CMD_ENABLE_FLAP_DETECTION)?"Enable":"Disable");
		break;

	case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		snprintf(action,sizeof(action),"%s notifications for all services in a particular hostgroup",(cmd==CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS)?"Enable":"Disable");
		break;

	case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		snprintf(action,sizeof(action),"%s notifications for all hosts in a particular hostgroup",(cmd==CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS)?"Enable":"Disable");
		break;

	case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
	case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
		snprintf(action,sizeof(action),"%s active checks of all services in a particular hostgroup",(cmd==CMD_ENABLE_HOSTGROUP_SVC_CHECKS)?"Enable":"Disable");
		break;

	case CMD_DEL_HOST_DOWNTIME:
	case CMD_DEL_SVC_DOWNTIME:
		snprintf(action,sizeof(action),"Cancel scheduled downtime for these %s",(cmd==CMD_DEL_HOST_DOWNTIME)?"hosts":"services");
		break;

	case CMD_ENABLE_FAILURE_PREDICTION:
	case CMD_DISABLE_FAILURE_PREDICTION:
		snprintf(action,sizeof(action),"%s failure prediction for hosts and service on a program-wide basis",(cmd==CMD_ENABLE_FAILURE_PREDICTION)?"Enable":"Disable");
		break;

	case CMD_ENABLE_PERFORMANCE_DATA:
	case CMD_DISABLE_PERFORMANCE_DATA:
		snprintf(action,sizeof(action),"%s performance data processing for hosts and services on a program-wide basis",(cmd==CMD_ENABLE_PERFORMANCE_DATA)?"Enable":"Disable");
		break;

	case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
	case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
		snprintf(action,sizeof(action),"Schedule downtime for all %s in a particular hostgroup",(cmd==CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME)?"hosts":"services");
		break;

	case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
	case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
		snprintf(action,sizeof(action),"%s accepting passive host checks on a program-wide basis",(cmd==CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS)?"Start":"Stop");
		break;

	case CMD_ENABLE_PASSIVE_HOST_CHECKS:
	case CMD_DISABLE_PASSIVE_HOST_CHECKS:
		snprintf(action,sizeof(action),"%s accepting passive checks for these hosts",(cmd==CMD_ENABLE_PASSIVE_HOST_CHECKS)?"Start":"Stop");
		break;

	case CMD_START_OBSESSING_OVER_HOST_CHECKS:
	case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:
		snprintf(action,sizeof(action),"%s obsessing over host checks on a program-wide basis",(cmd==CMD_START_OBSESSING_OVER_HOST_CHECKS)?"Start":"Stop");
		break;

	case CMD_START_OBSESSING_OVER_SVC:
	case CMD_STOP_OBSESSING_OVER_SVC:
		snprintf(action,sizeof(action),"%s obsessing over these services",(cmd==CMD_START_OBSESSING_OVER_SVC)?"Start":"Stop");
		break;

	case CMD_START_OBSESSING_OVER_HOST:
	case CMD_STOP_OBSESSING_OVER_HOST:
		snprintf(action,sizeof(action),"%s obsessing over these hosts",(cmd==CMD_START_OBSESSING_OVER_HOST)?"Start":"Stop");
		break;

	case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		snprintf(action,sizeof(action),"%s notifications for all services in a particular servicegroup",(cmd==CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS)?"Enable":"Disable");
		break;

	case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		snprintf(action,sizeof(action),"%s notifications for all hosts in a particular servicegroup",(cmd==CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS)?"Enable":"Disable");
		break;

	case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
	case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
		snprintf(action,sizeof(action),"%s active checks of all services in a particular servicegroup",(cmd==CMD_ENABLE_SERVICEGROUP_SVC_CHECKS)?"Enable":"Disable");
		break;

	case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
		snprintf(action,sizeof(action),"Schedule downtime for all hosts in a particular servicegroup");
		break;

	case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:
		snprintf(action,sizeof(action),"Schedule downtime for all services in a particular servicegroup");
		break;

	case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
	case CMD_SEND_CUSTOM_SVC_NOTIFICATION:
		snprintf(help_text,sizeof(help_text),"This command is used to send a custom notification about the specified %s.  Useful in emergencies when you need to notify admins of an issue regarding a monitored system or service.",(cmd==CMD_SEND_CUSTOM_HOST_NOTIFICATION)?"host":"service");
		snprintf(action,sizeof(action),"Send a custom %s notification",(cmd==CMD_SEND_CUSTOM_HOST_NOTIFICATION)?"host":"service");
		break;

	default:
		printf("<BR><DIV align='center'><DIV CLASS='errorBox'>\n");
		printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
		printf("<td CLASS='errorMessage'>Sorry Dave, I can't let you do that...</td></tr></table></DIV>\n");
		printf("<DIV CLASS='errorDescription'>Executing an unknown command? Shame on you!</DIV><br>");
		printf("</DIV>\n");
		printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
		return;
	}

	help_text[sizeof(help_text)-1]='\x0';
	action[sizeof(action)-1]='\x0';

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

	printf("<INPUT TYPE='HIDDEN' NAME='cmd_typ' VALUE='%d'><INPUT TYPE='HIDDEN' NAME='cmd_mod' VALUE='%d'>\n",cmd,CMDMODE_COMMIT);

	/* creating an extra table to make it compatible to IE6 & IE7 to have a nice frame around the form, damn it */
	printf("<TABLE CELLSPACING='0' CELLPADDING='0'><TR><TD CLASS='boxFrame BoxWidth'>\n");

	printf("<TABLE CELLSPACING='2' CELLPADDING='0' class='contentTable'>\n");

	printf("<tr CLASS='sectionHeader'><td COLSPAN='2'>Action</td></tr>\n");
	printf("<tr><td COLSPAN='2'>%s ",action);
	if (strlen(help_text) > 2)
		print_help_box(help_text);
	printf("</td></tr>\n");

	switch(cmd){

	case CMD_ADD_SVC_COMMENT:
	case CMD_ACKNOWLEDGE_SVC_PROBLEM:
	case CMD_ADD_HOST_COMMENT:
	case CMD_ACKNOWLEDGE_HOST_PROBLEM:

		if(cmd==CMD_ACKNOWLEDGE_SVC_PROBLEM || cmd==CMD_ADD_SVC_COMMENT)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER,cmd);
		print_form_element(PRINT_AUTHOR,cmd);
		print_form_element(PRINT_COMMENT_BOX,cmd);
		print_form_element(PRINT_PERSISTENT,cmd);

		if(cmd==CMD_ACKNOWLEDGE_HOST_PROBLEM || cmd==CMD_ACKNOWLEDGE_SVC_PROBLEM){
			print_form_element(PRINT_STICKY_ACK,cmd);
			print_form_element(PRINT_SEND_NOTFICATION,cmd);
		}

		break;

	case CMD_DEL_HOST_DOWNTIME:
	case CMD_DEL_SVC_DOWNTIME:
	case CMD_DEL_HOST_COMMENT:
	case CMD_DEL_SVC_COMMENT:

		if (cmd==CMD_DEL_HOST_COMMENT || cmd==CMD_DEL_SVC_COMMENT)
			print_object_list(PRINT_COMMENT_LIST);
		else
			print_object_list(PRINT_DOWNTIME_LIST);

		break;

	case CMD_DELAY_SVC_NOTIFICATION:
	case CMD_DELAY_HOST_NOTIFICATION:

		if(cmd==CMD_DELAY_SVC_NOTIFICATION)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER,cmd);
		print_form_element(PRINT_NOTIFICATION_DELAY,cmd);

		break;

	case CMD_SCHEDULE_SVC_CHECK:
	case CMD_SCHEDULE_HOST_CHECK:
	case CMD_SCHEDULE_HOST_SVC_CHECKS:

		if(cmd==CMD_SCHEDULE_SVC_CHECK)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER,cmd);
		print_form_element(PRINT_CHECK_TIME,cmd);
		print_form_element(PRINT_FORCE_CHECK,cmd);

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

		if(cmd==CMD_ENABLE_HOST_SVC_CHECKS || cmd==CMD_DISABLE_HOST_SVC_CHECKS || cmd==CMD_ENABLE_HOST_SVC_NOTIFICATIONS || cmd==CMD_DISABLE_HOST_SVC_NOTIFICATIONS || cmd==CMD_ENABLE_HOST_NOTIFICATIONS || cmd==CMD_DISABLE_HOST_NOTIFICATIONS){
			print_form_element(PRINT_COMMON_HEADER,cmd);
		}

		if(cmd==CMD_ENABLE_HOST_SVC_CHECKS || cmd==CMD_DISABLE_HOST_SVC_CHECKS || cmd==CMD_ENABLE_HOST_SVC_NOTIFICATIONS || cmd==CMD_DISABLE_HOST_SVC_NOTIFICATIONS){

			snprintf(help_text,sizeof(help_text),"This %s %s of the host too.",(cmd==CMD_ENABLE_HOST_SVC_CHECKS ||cmd==CMD_ENABLE_HOST_SVC_NOTIFICATIONS)?"enables":"disables",(cmd==CMD_ENABLE_HOST_SVC_CHECKS || cmd==CMD_DISABLE_HOST_SVC_CHECKS)?"checks":"notifications");
			help_text[sizeof(help_text)-1]='\x0';

			printf("<tr><td class=\"objectDescription descriptionleft\">%s For Host Too:",(cmd==CMD_ENABLE_HOST_SVC_CHECKS || cmd==CMD_ENABLE_HOST_SVC_NOTIFICATIONS)?"Enable":"Disable");
			print_help_box(help_text);
			printf("</td><td align=\"left\"><INPUT TYPE='checkbox' NAME='ahas'></td></tr>\n");
		}

		if(cmd==CMD_ENABLE_HOST_NOTIFICATIONS || cmd==CMD_DISABLE_HOST_NOTIFICATIONS){

			snprintf(help_text,sizeof(help_text),"%s notifications te be sent out to child hosts.",(cmd==CMD_ENABLE_HOST_NOTIFICATIONS)?"Enable":"Disable");
			help_text[sizeof(help_text)-1]='\x0';

			printf("<tr><td class=\"objectDescription descriptionleft\">%s Notifications For Child Hosts Too:",(cmd==CMD_ENABLE_HOST_NOTIFICATIONS)?"Enable":"Disable");
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
		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
		printf("<tr><td CLASS='objectDescription' colspan=2>There are no options for this command.<br>Click the 'Commit' button to submit the command.</td></tr>\n");
		break;

	case CMD_PROCESS_HOST_CHECK_RESULT:
	case CMD_PROCESS_SERVICE_CHECK_RESULT:

		if(cmd==CMD_PROCESS_SERVICE_CHECK_RESULT)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER,cmd);

		snprintf(help_text,sizeof(help_text),"Set the state which should be send to %s for this %s.",PROGRAM_NAME,(cmd==CMD_PROCESS_HOST_CHECK_RESULT)?"hosts":"services");
		help_text[sizeof(help_text)-1]='\x0';

		printf("<tr><td class=\"objectDescription descriptionleft\">Check Result:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">\n");
		printf("\t<SELECT NAME='plugin_state'>\n");
		if(cmd==CMD_PROCESS_SERVICE_CHECK_RESULT){
			printf("\t\t<OPTION VALUE=%d SELECTED>OK</OPTION>\n",STATE_OK);
			printf("\t\t<OPTION VALUE=%d>WARNING</OPTION>\n",STATE_WARNING);
			printf("\t\t<OPTION VALUE=%d>UNKNOWN</OPTION>\n",STATE_UNKNOWN);
			printf("\t\t<OPTION VALUE=%d>CRITICAL</OPTION>\n",STATE_CRITICAL);
		}else{
			printf("\t\t<OPTION VALUE=0 SELECTED>UP</OPTION>\n");
			printf("\t\t<OPTION VALUE=1>DOWN</OPTION>\n");
			printf("\t\t<OPTION VALUE=2>UNREACHABLE</OPTION>\n");
		}
		printf("\t</SELECT>\n");
		printf("</td></tr>\n");

		print_form_element(PRINT_CHECK_OUTPUT_BOX,cmd);
		print_form_element(PRINT_PERFORMANCE_DATA_BOX,cmd);

		break;

	case CMD_SCHEDULE_HOST_DOWNTIME:
	case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
	case CMD_SCHEDULE_SVC_DOWNTIME:

		if(cmd==CMD_SCHEDULE_SVC_DOWNTIME)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER,cmd);
		print_form_element(PRINT_AUTHOR,cmd);
		print_form_element(PRINT_COMMENT_BOX,cmd);

		snprintf(help_text,sizeof(help_text),"Define here if this downtime should get triggerd by another downtime of a particular host or service.",(cmd==CMD_PROCESS_HOST_CHECK_RESULT)?"host":"service");
		help_text[sizeof(help_text)-1]='\x0';

		printf("<tr id=\"trigger_select\"><td class=\"objectDescription descriptionleft\">Triggered By:");
		print_help_box(help_text);
		printf("</td><td align=\"left\">\n");
		printf("\t<SELECT name='trigger'>\n");
		printf("\t\t<OPTION VALUE='0'>N/A</OPTION>\n");

		for(temp_downtime=scheduled_downtime_list;temp_downtime!=NULL;temp_downtime=temp_downtime->next){
			if(temp_downtime->type!=HOST_DOWNTIME)
				continue;

			/* find the host... */
			 temp_host=find_host(temp_downtime->host_name);

			 /* make sure user has rights to view this host */
			if(is_authorized_for_host(temp_host,&current_authdata)==FALSE)
				continue;

			printf("\t\t<OPTION VALUE='%lu'>",temp_downtime->downtime_id);
			get_time_string(&temp_downtime->start_time,start_time,sizeof(start_time),SHORT_DATE_TIME);
			printf("ID: %lu, Host '%s' starting @ %s</OPTION>\n",temp_downtime->downtime_id,temp_downtime->host_name,start_time);
			found_trigger_objects=TRUE;
		}
		for(temp_downtime=scheduled_downtime_list;temp_downtime!=NULL;temp_downtime=temp_downtime->next){
			if(temp_downtime->type!=SERVICE_DOWNTIME)
				continue;

			printf("\t\t<OPTION VALUE='%lu'>",temp_downtime->downtime_id);
			get_time_string(&temp_downtime->start_time,start_time,sizeof(start_time),SHORT_DATE_TIME);
			printf("ID: %lu, Service '%s' on host '%s' starting @ %s</OPTION>\n",temp_downtime->downtime_id,temp_downtime->service_description,temp_downtime->host_name,start_time);
			found_trigger_objects=TRUE;
		}

		printf("\t</SELECT>\n");
		printf("</td></tr>\n");

		/* hide "Triggerd by" selction if nothing is found to get triggerd from */
		if(!found_trigger_objects)
			printf("<tr style=\"display:none;\"><td colspan=2><script language=\"JavaScript\">document.getElementById('trigger_select').style.display = 'none';</script></td></tr>\n");

		print_form_element(PRINT_START_TIME,cmd);
		print_form_element(PRINT_END_TIME,cmd);
		print_form_element(PRINT_FIXED_FLEXIBLE_TYPE,cmd);

		if(cmd==CMD_SCHEDULE_HOST_DOWNTIME){
			snprintf(help_text,sizeof(help_text),"Define here what should be done with the child hosts of these hosts.",(cmd==CMD_PROCESS_HOST_CHECK_RESULT)?"host":"service");
			help_text[sizeof(help_text)-1]='\x0';

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
		printf("<td><INPUT TYPE='HIDDEN' NAME='hostgroup' VALUE='%s'>%s</td></tr>\n",escape_string(hostgroup_name),escape_string(hostgroup_name));

		if(cmd==CMD_ENABLE_HOSTGROUP_SVC_CHECKS || cmd==CMD_DISABLE_HOSTGROUP_SVC_CHECKS || cmd==CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS || cmd==CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS){

			print_form_element(PRINT_COMMON_HEADER,cmd);

			printf("<tr><td class=\"objectDescription descriptionleft\">%s For Hosts Too:</td><td align=\"left\">\n",(cmd==CMD_ENABLE_HOSTGROUP_SVC_CHECKS || cmd==CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS)?"Enable":"Disable");
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
		printf("<td><INPUT TYPE='HIDDEN' NAME='servicegroup' VALUE='%s'>%s</td></tr>\n",escape_string(servicegroup_name),escape_string(servicegroup_name));

		if(cmd==CMD_ENABLE_SERVICEGROUP_SVC_CHECKS || cmd==CMD_DISABLE_SERVICEGROUP_SVC_CHECKS || cmd==CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS || cmd==CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS){

			print_form_element(PRINT_COMMON_HEADER,cmd);

			printf("<tr><td class=\"objectDescription descriptionleft\">%s For Hosts Too:</td><td align=\"left\">\n",(cmd==CMD_ENABLE_SERVICEGROUP_SVC_CHECKS || cmd==CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS)?"Enable":"Disable");
			printf("<INPUT TYPE='checkbox' NAME='ahas'></td></tr>\n");
		}
		break;

	case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
	case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
	case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
	case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:

		printf("<tr><td COLSPAN=\"2\">&nbsp;</td></tr>\n");
		printf("<tr class=\"statusEven\"><td width=\"50%%\" style=\"font-weight:bold;\">");
		if(cmd==CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME || cmd==CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME)
			printf("Hostgroup Name:</td><td><INPUT TYPE='HIDDEN' NAME='hostgroup' VALUE='%s'>%s</td></tr>\n",escape_string(hostgroup_name),escape_string(hostgroup_name));
		else
			printf("Servicegroup Name:</td><td><INPUT TYPE='HIDDEN' NAME='servicegroup' VALUE='%s'>%s</td></tr>\n",escape_string(servicegroup_name),escape_string(servicegroup_name));

		print_form_element(PRINT_COMMON_HEADER,cmd);
		print_form_element(PRINT_AUTHOR,cmd);
		print_form_element(PRINT_COMMENT_BOX,cmd);
		print_form_element(PRINT_START_TIME,cmd);
		print_form_element(PRINT_END_TIME,cmd);
		print_form_element(PRINT_FIXED_FLEXIBLE_TYPE,cmd);

		if(cmd==CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME || cmd==CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME){
			printf("<tr><td class=\"objectDescription descriptionleft\">Schedule Downtime For Hosts Too:</td><td align=\"left\">\n");
			printf("<INPUT TYPE='checkbox' NAME='ahas'></td></tr>\n");
		}
		break;

	case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
	case CMD_SEND_CUSTOM_SVC_NOTIFICATION:

		if(cmd==CMD_SEND_CUSTOM_SVC_NOTIFICATION)
			print_object_list(PRINT_SERVICE_LIST);
		else
			print_object_list(PRINT_HOST_LIST);

		print_form_element(PRINT_COMMON_HEADER,cmd);
		print_form_element(PRINT_AUTHOR,cmd);
		print_form_element(PRINT_COMMENT_BOX,cmd);
		print_form_element(PRINT_FORCE_NOTIFICATION,cmd);
		print_form_element(PRINT_BROADCAST_NOTIFICATION,cmd);

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

void commit_command_data(int cmd){
	char error_string[MAX_INPUT_BUFFER];
	service *temp_service;
	host *temp_host;
	hostgroup *temp_hostgroup;
	comment *temp_comment;
	scheduled_downtime *temp_downtime;
	servicegroup *temp_servicegroup=NULL;
	contact *temp_contact=NULL;
	int x=0;
	int e=0;
	short error_found=FALSE;
	short cmd_has_objects = FALSE;
	short row_color = 0;

	/* get authentication information */
	get_authentication_information(&current_authdata);

	/* allways set the first element to FALSE*/
	/* If there is a single COMMAND witch is not coverd correctly throught the following cases it won't get executed */
	is_authorized[x]=FALSE;

	/* get name to use for author */
	if(lock_author_names==TRUE){
		temp_contact=find_contact(current_authdata.username);
		if(temp_contact!=NULL && temp_contact->alias!=NULL)
			comment_author=temp_contact->alias;
		else
			comment_author=current_authdata.username;
	}

	switch(cmd){


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

		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {

			cmd_has_objects = TRUE;

			if (commands[x].host_name == NULL)
				continue;

			/* see if the user is authorized to issue a command... */
			is_authorized[x]=FALSE;
			if (cmd==CMD_ADD_HOST_COMMENT || cmd==CMD_ACKNOWLEDGE_HOST_PROBLEM || cmd==CMD_SEND_CUSTOM_HOST_NOTIFICATION) {
				temp_host=find_host(commands[x].host_name);
				if(is_authorized_for_host_commands(temp_host,&current_authdata)==TRUE)
					is_authorized[x]=TRUE;
			} else {
				temp_service=find_service(commands[x].host_name,commands[x].description);
				if(is_authorized_for_service_commands(temp_service,&current_authdata)==TRUE)
					is_authorized[x]=TRUE;
			}
		}
		break;

	case CMD_DEL_HOST_COMMENT:
	case CMD_DEL_SVC_COMMENT:

		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {

			cmd_has_objects = TRUE;

			if (multi_ids[x] == FALSE)
				continue;

			/* check the sanity of the comment id */
			if(multi_ids[x]==0) {
				error[e++].message=strdup("Comment id cannot be 0");
				continue;
			}

			/* find the comment */
			if(cmd==CMD_DEL_HOST_COMMENT)
				temp_comment=find_host_comment(multi_ids[x]);
			else
				temp_comment=find_service_comment(multi_ids[x]);

			/* see if the user is authorized to issue a command... */
			is_authorized[x]=FALSE;
			if (cmd==CMD_ADD_HOST_COMMENT || cmd==CMD_ACKNOWLEDGE_HOST_PROBLEM) {
				temp_host=find_host(temp_comment->host_name);
				if(is_authorized_for_host_commands(temp_host,&current_authdata)==TRUE)
					is_authorized[x]=TRUE;
			} else {
				temp_service=find_service(temp_comment->host_name,temp_comment->service_description);
				if(is_authorized_for_service_commands(temp_service,&current_authdata)==TRUE)
					is_authorized[x]=TRUE;
			}
		}

		/* free comment data */
		free_comment_data();

		break;

	case CMD_DEL_HOST_DOWNTIME:
	case CMD_DEL_SVC_DOWNTIME:

		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {

			cmd_has_objects = TRUE;

			if (multi_ids[x] == FALSE)
				continue;

			/* check the sanity of the downtime id */
			if(multi_ids[x]==0){
				error[e++].message=strdup("Downtime id cannot be 0");
				continue;
			}

			/* find the downtime entry */
			if(cmd==CMD_DEL_HOST_DOWNTIME)
				temp_downtime=find_host_downtime(multi_ids[x]);
			else
				temp_downtime=find_service_downtime(multi_ids[x]);

			/* see if the user is authorized to issue a command... */
			is_authorized[x]=FALSE;
			if(cmd==CMD_DEL_HOST_DOWNTIME && temp_downtime!=NULL){
				temp_host=find_host(temp_downtime->host_name);
				if(is_authorized_for_host_commands(temp_host,&current_authdata)==TRUE)
					is_authorized[x]=TRUE;
			}
			if(cmd==CMD_DEL_SVC_DOWNTIME && temp_downtime!=NULL){
				temp_service=find_service(temp_downtime->host_name,temp_downtime->service_description);
				if(is_authorized_for_service_commands(temp_service,&current_authdata)==TRUE)
					is_authorized[x]=TRUE;
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

		if(cmd==CMD_SCHEDULE_SVC_DOWNTIME) {
			/* make sure we have author and comment data */
			check_comment_sanity(&e);

			/* make sure we have start/end times for downtime */
			check_time_sanity(&e);

			/* clean up the comment data if scheduling downtime */
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		}

		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {

			cmd_has_objects = TRUE;

			if (commands[x].host_name == NULL)
				continue;

			is_authorized[x]=FALSE;
			temp_service=find_service(commands[x].host_name,commands[x].description);
			if(is_authorized_for_service_commands(temp_service,&current_authdata)==TRUE)
				is_authorized[x]=TRUE;
		}

		/* make sure we have passive check info (if necessary) */
		if(cmd==CMD_PROCESS_SERVICE_CHECK_RESULT && !strcmp(plugin_output,""))
			error[e++].message=strdup("Check output cannot be blank");

		/* make sure we have a notification delay (if necessary) */
		if(cmd==CMD_DELAY_SVC_NOTIFICATION && notification_delay<=0)
			error[e++].message=strdup("Notification delay must be greater than 0");

		/* make sure we have check time (if necessary) */
		if(cmd==CMD_SCHEDULE_SVC_CHECK && start_time==(time_t)0)
			error[e++].message=strdup("Start time must be non-zero or bad format has been submitted");

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

		/* see if the user is authorized to issue a command... */
		is_authorized[x]=FALSE;
		if(is_authorized_for_system_commands(&current_authdata)==TRUE)
			is_authorized[x]=TRUE;
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

		if(cmd==CMD_SCHEDULE_HOST_DOWNTIME||cmd==CMD_SCHEDULE_HOST_SVC_DOWNTIME) {
			/* make sure we have author and comment data */
			check_comment_sanity(&e);

			/* make sure we have start/end times for downtime */
			check_time_sanity(&e);

			/* clean up the comment data if scheduling downtime */
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		}

		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {

			cmd_has_objects = TRUE;

			if (commands[x].host_name == NULL)
				continue;

			/* see if the user is authorized to issue a command... */
			is_authorized[x]=FALSE;
			temp_host=find_host(commands[x].host_name);
			if(is_authorized_for_host_commands(temp_host,&current_authdata)==TRUE)
				is_authorized[x]=TRUE;
		}

		/* make sure we have a notification delay (if necessary) */
		if(cmd==CMD_DELAY_HOST_NOTIFICATION && notification_delay<=0)
			error[e++].message=strdup("Notification delay must be greater than 0");

		/* make sure we have check time (if necessary) */
		if((cmd==CMD_SCHEDULE_HOST_CHECK || cmd==CMD_SCHEDULE_HOST_SVC_CHECKS) && start_time==(time_t)0)
			error[e++].message=strdup("Start time must be non-zero or bad format has been submitted");

		/* make sure we have passive check info (if necessary) */
		if(cmd==CMD_PROCESS_HOST_CHECK_RESULT && !strcmp(plugin_output,""))
			error[e++].message=strdup("Check output cannot be blank");

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


		if(cmd==CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME || cmd==CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME \
		|| cmd==CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME || cmd==CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME){
			/* make sure we have author and comment data */
			check_comment_sanity(&e);

			/* make sure we have start/end times for downtime */
			check_time_sanity(&e);

			/* clean up the comment data if scheduling downtime */
			clean_comment_data(comment_author);
			clean_comment_data(comment_data);
		}

		/* see if the user is authorized to issue a command... */
		is_authorized[x]=FALSE;
		if(cmd==CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS	|| cmd==CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS || \
		   cmd==CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS || cmd==CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS || \
		   cmd==CMD_ENABLE_HOSTGROUP_SVC_CHECKS		|| cmd==CMD_DISABLE_HOSTGROUP_SVC_CHECKS || \
		   cmd==CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME	|| cmd==CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME ){
			temp_hostgroup=find_hostgroup(hostgroup_name);
			if(is_authorized_for_hostgroup(temp_hostgroup,&current_authdata)==TRUE)
				is_authorized[x]=TRUE;
		} else {
		//	if(cmd==CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME || cmd==CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME) {
			temp_servicegroup=find_servicegroup(servicegroup_name);
			if(is_authorized_for_servicegroup(temp_servicegroup,&current_authdata)==TRUE)
				is_authorized[x]=TRUE;
		}

		break;

	default:
		printf("<BR><DIV align='center'><DIV CLASS='errorBox'>\n");
		printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
		printf("<td CLASS='errorMessage'>Sorry Dave, I can't let you do that...</td></tr></table></DIV>\n");
		printf("<DIV CLASS='errorDescription'>Executing an unknown command? Shame on you!</DIV><br>");
		printf("</DIV>\n");
		printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
		return;
	}


	/*
	 * these are supposed to be implanted inside the
	 * completed commands shipped off to Icinga and
	 * must therefore never contain ';'
	 */
	for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
		if (commands[x].host_name == NULL)
			continue;

		if (strchr(commands[x].host_name, ';')) {
			snprintf(error_string,sizeof(error_string),"The hostname \"%s\" contains a semicolon",commands[x].host_name);
			error_string[sizeof(error_string)-1]='\x0';
			error[e++].message=(char *)strdup(error_string);
		}
		if (commands[x].description != NULL && strchr(commands[x].description, ';')) {
			snprintf(error_string,sizeof(error_string),"The service description \"%s\" on host \"%s\" contains a semicolon",commands[x].description,commands[x].host_name);
			error_string[sizeof(error_string)-1]='\x0';
			error[e++].message=strdup(error_string);
		}
	}
	if (hostgroup_name && strchr(hostgroup_name, ';'))
		error[e++].message=strdup("The hostgroup name contains a semicolon");
	if (servicegroup_name && strchr(servicegroup_name, ';'))
		error[e++].message=strdup("The servicegroup name  contains a semicolon");

	printf("<BR><DIV align='center'>\n");

	/* if Icinga isn't checking external commands, don't do anything... */
	if(check_external_commands==FALSE){
		if(content_type==WML_CONTENT)
			printf("<p>Error: %s is not checking external commands!</p>\n", PROGRAM_NAME);
		else{
			printf("<DIV CLASS='errorBox'>\n");
			printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
			printf("<td CLASS='errorMessage'>Sorry, but %s is currently not checking for external commands, so your command will not be committed!</td></tr></table></DIV>\n", PROGRAM_NAME);
			printf("<DIV CLASS='errorDescription'>Read the documentation for information on how to enable external commands...</DIV>\n");
			printf("</DIV>\n");
			printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
		}
		return;
	}

	/* to be safe, we are going to REQUIRE that the authentication functionality is enabled... */
	if(use_authentication==FALSE){
		if(content_type==WML_CONTENT)
			printf("<p>Error: Authentication is not enabled!</p>\n");
		else{
			printf("<DIV CLASS='errorBox'>\n");
			printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
			printf("<td CLASS='errorMessage'>Sorry Dave, I can't let you do that...</td></tr></table></DIV>\n");
			printf("<DIV CLASS='errorDescription'>");
			printf("It seems that you have chosen to not use the authentication functionality of the CGIs. ");
			printf("I don't want to be personally responsible for what may happen as a result of allowing unauthorized users to issue commands to %s, ", PROGRAM_NAME);
			printf("so you'll have to disable this safeguard if you are really stubborn and want to invite trouble. ");
			printf("Read the section on CGI authentication in the HTML documentation to learn how you can enable authentication and why you should want to.</DIV>\n");
			printf("</DIV>\n");
			printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
		}
		return;
	}

	/* Check if we found errors which preventing us from submiting the command */
	if(e>0) {
		printf("<DIV CLASS='errorBox'>\n");
		printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
		printf("<td CLASS='errorMessage'>Following errors occured.</td></tr></table></DIV>\n");
		printf("<table cellspacing=0 cellpadding=0 border=0 class='errorTable'>\n");
		for (e=0; e<NUMBER_OF_STRUCTS; e++) {
			if (error[e].message==NULL)
				continue;
			if(content_type==WML_CONTENT)
				printf("<p>Error: %s</p><BR>\n",error[e].message);
			else {
				printf("<tr><td class='errorString'>ERROR:</td><td class='errorContent'>%s</td></tr>\n",error[e].message);
			}
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
	if (cmd_has_objects == FALSE && is_authorized[0]==FALSE ) {
		if(content_type==WML_CONTENT)
			printf("<p>Error: You're not authorized to commit that command!</p>\n");
		else{
			printf("<DIV CLASS='errorBox'>\n");
			printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
			printf("<td CLASS='errorMessage'>Sorry, but you are not authorized to commit the specified command.</td></tr></table></DIV>\n");
			printf("<DIV CLASS='errorDescription'>Read the section of the documentation that deals with authentication and authorization in the CGIs for more information.</DIV>\n");
			printf("</DIV>\n");
			printf("<BR><DIV align='center'><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
		}
		return;
	}

	/* everything looks okay, so let's go ahead and commit the command... */
	/* commit the command */
	commit_command(cmd);

	/* for commands without objects get the first result*/
	if(cmd_has_objects == FALSE) {
		if (submit_result[0]==OK){
			if(content_type==WML_CONTENT)
				printf("<p>Your command was submitted sucessfully...</p>\n");
			else{
				printf("<DIV CLASS='successBox'>\n");
				printf("<DIV CLASS='successMessage'>Your command request was successfully submitted to %s for processing.<BR><BR>\n",PROGRAM_NAME);
				printf("Note: It may take a while before the command is actually processed.</DIV>\n");
				printf("</DIV>\n");
				printf("<BR><input type='submit' value='Done' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
			}
		}else{
			if(content_type==WML_CONTENT)
				printf("<p>An error occurred while committing your command!</p>\n");
			else{
				printf("<DIV CLASS='errorBox'>\n");
				printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
				printf("<td CLASS='errorMessage'>An error occurred while attempting to commit your command for processing.</td></tr></table></DIV>\n");
				printf("<DIV CLASS='errorDescription'>Unfortunately I can't determine the root cause of this problem.</DIV>\n");
				printf("</DIV>\n");
				printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
			}
		}
	} else {
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (cmd==CMD_DEL_HOST_COMMENT || cmd==CMD_DEL_SVC_COMMENT || cmd==CMD_DEL_HOST_DOWNTIME || cmd==CMD_DEL_SVC_DOWNTIME ) {
				if (multi_ids[x] == FALSE)
					continue;
			} else {
				if (commands[x].host_name == NULL)
					continue;
			}

			if (is_authorized[x] == FALSE || submit_result[x] == ERROR) {
				error_found=TRUE;
				break;
			}
		}

		if (error_found) {
			printf("<DIV CLASS='errorBox'>\n");
			printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
			printf("<td CLASS='errorMessage'>An error occurred while attempting to commit your command for processing.</td></tr></table></DIV>\n");
			printf("<DIV CLASS='errorDescription'>Not all commands could be send off successfully...</DIV>\n");
			printf("</DIV>\n");
		} else {
			printf("<DIV CLASS='successBox'>\n");
			printf("<DIV CLASS='successMessage'>Your command requests were successfully submitted to %s for processing.<BR><BR>\n",PROGRAM_NAME);
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

		printf("<TABLE CELLSPACING='0' CELLPADDING='0' ID='sumCommit' %s><TR><TD CLASS='boxFrame BoxWidth'>\n",(error_found)?"":"style='display:none;'");
		printf("<table cellspacing=2 cellpadding=0 border=0 class='contentTable'>\n");
		if (cmd==CMD_DEL_HOST_COMMENT || cmd==CMD_DEL_SVC_COMMENT)
			printf("<tr class='sumHeader'><td width='80%%'>Comment ID</td><td width='20%%'>Status</td></tr>\n");
		else if (cmd==CMD_DEL_HOST_DOWNTIME || cmd==CMD_DEL_SVC_DOWNTIME)
			printf("<tr class='sumHeader'><td width='80%%'>Downtime ID</td><td width='20%%'>Status</td></tr>\n");
		else
			printf("<tr class='sumHeader'><td width='40%%'>Host</td><td width='40%%'>Service</td><td width='20%%'>Status</td></tr>\n");

		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {

			if (cmd==CMD_DEL_HOST_COMMENT || cmd==CMD_DEL_SVC_COMMENT || cmd==CMD_DEL_HOST_DOWNTIME || cmd==CMD_DEL_SVC_DOWNTIME) {
				if (multi_ids[x] == FALSE)
					continue;
				row_color = (row_color==0)?1:0;
				printf("<tr class='status%s'><td>%lu</td><td>",(row_color==0)?"Even":"Odd ",multi_ids[x]);
			} else {
				if (commands[x].host_name == NULL)
					continue;
				row_color = (row_color==0)?1:0;

				printf("<tr class='status%s'><td>%s</td><td>%s</td><td>",(row_color==0)?"Even":"Odd ",commands[x].host_name,(commands[x].description != NULL)?commands[x].description:"N/A");
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

__attribute__((format(printf, 2, 3)))
static int cmd_submitf(int id, const char *fmt, ...){
	char cmd[MAX_EXTERNAL_COMMAND_LENGTH];
	const char *command;
	int len, len2;
	va_list ap;

	command = extcmd_get_name(id);
	/*
	 * We disallow sending 'CHANGE' commands from the cgi's
	 * until we do proper session handling to prevent cross-site
	 * request forgery
	 */
	if (!command || (strlen(command) > 6 && !memcmp("CHANGE", command, 6)))
		return ERROR;

	if(log_external_commands_user==TRUE){
		get_authentication_information(&current_authdata);
		len = snprintf(cmd, sizeof(cmd) - 1, "[%lu] %s;%s;", time(NULL), command, current_authdata.username);
	} else {
		len = snprintf(cmd, sizeof(cmd) - 1, "[%lu] %s;", time(NULL), command);
	}

	if (len < 0)
		return ERROR;

	if(fmt) {
		va_start(ap, fmt);
		len2 = vsnprintf(&cmd[len], sizeof(cmd) - len - 1, fmt, ap);
		va_end(ap);
		if (len2 < 0)
			return ERROR;
	}

	return write_command_to_file(cmd);
}

/* commits a command for processing */
int commit_command(int cmd){
	time_t current_time;
	time_t scheduled_time;
	time_t notification_time;
	int x = 0;

	/* get the current time */
	time(&current_time);

	/* get the scheduled time */
	scheduled_time=current_time+(schedule_delay*60);

	/* get the notification time */
	notification_time=current_time+(notification_delay*60);

	/* decide how to form the command line... */
	switch(cmd){

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
			submit_result[x] = cmd_submitf(cmd,NULL);
		break;

		/** simple host commands **/
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
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s",commands[x].host_name);
		}
		break;

		/** simple service commands **/
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
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%s",commands[x].host_name,commands[x].description);
		}
		break;

	case CMD_ADD_HOST_COMMENT:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%d;%s;%s",commands[x].host_name,persistent_comment,comment_author,comment_data);
		}
		break;

	case CMD_ADD_SVC_COMMENT:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%s;%d;%s;%s",commands[x].host_name,commands[x].description,persistent_comment,comment_author,comment_data);
		}
		break;

	case CMD_DEL_HOST_COMMENT:
	case CMD_DEL_SVC_COMMENT:
	case CMD_DEL_HOST_DOWNTIME:
	case CMD_DEL_SVC_DOWNTIME:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (multi_ids[x] == FALSE)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%lu",multi_ids[x]);
		}
		break;

	case CMD_DELAY_HOST_NOTIFICATION:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%lu",commands[x].host_name,notification_time);
		}
		break;

	case CMD_DELAY_SVC_NOTIFICATION:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%s;%lu",commands[x].host_name,commands[x].description,notification_time);
		}
		break;

	case CMD_SCHEDULE_SVC_CHECK:
	case CMD_SCHEDULE_FORCED_SVC_CHECK:
		if(force_check==TRUE)
			cmd=CMD_SCHEDULE_FORCED_SVC_CHECK;
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%s;%lu",commands[x].host_name,commands[x].description,start_time);
		}
		break;

	case CMD_DISABLE_NOTIFICATIONS:
	case CMD_ENABLE_NOTIFICATIONS:
	case CMD_SHUTDOWN_PROCESS:
	case CMD_RESTART_PROCESS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%lu",scheduled_time);
		break;

	case CMD_ENABLE_HOST_SVC_CHECKS:
	case CMD_DISABLE_HOST_SVC_CHECKS:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s",commands[x].host_name);
		}
		if(affect_host_and_services==TRUE){
			cmd = (cmd == CMD_ENABLE_HOST_SVC_CHECKS) ? CMD_ENABLE_HOST_CHECK : CMD_DISABLE_HOST_CHECK;
			for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
				if (commands[x].host_name == NULL)
					continue;
				if (is_authorized[x])
					submit_result[x] |= cmd_submitf(cmd,"%s",commands[x].host_name);
			}
		}
		break;

	case CMD_SCHEDULE_HOST_SVC_CHECKS:
		if (force_check == TRUE)
			cmd = CMD_SCHEDULE_FORCED_HOST_SVC_CHECKS;
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s;%lu",host_name,scheduled_time);
		break;

	case CMD_ENABLE_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOST_NOTIFICATIONS:
		if(propagate_to_children==TRUE)
			cmd = (cmd == CMD_ENABLE_HOST_NOTIFICATIONS) ? CMD_ENABLE_HOST_AND_CHILD_NOTIFICATIONS : CMD_DISABLE_HOST_AND_CHILD_NOTIFICATIONS;
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s",commands[x].host_name);
		}
		break;

	case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s",commands[x].host_name);
		}
		if(affect_host_and_services==TRUE){
			cmd = (cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS) ? CMD_ENABLE_HOST_NOTIFICATIONS : CMD_DISABLE_HOST_NOTIFICATIONS;
			for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
				if (commands[x].host_name == NULL)
					continue;
				if (is_authorized[x])
					submit_result[x] |= cmd_submitf(cmd,"%s",commands[x].host_name);
			}
		}
		break;

	case CMD_ACKNOWLEDGE_HOST_PROBLEM:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%d;%d;%d;%s;%s",commands[x].host_name,(sticky_ack==TRUE)?ACKNOWLEDGEMENT_STICKY:ACKNOWLEDGEMENT_NORMAL,send_notification,persistent_comment,comment_author,comment_data);
		}
		break;

	case CMD_ACKNOWLEDGE_SVC_PROBLEM:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%s;%d;%d;%d;%s;%s",commands[x].host_name,commands[x].description,(sticky_ack==TRUE)?ACKNOWLEDGEMENT_STICKY:ACKNOWLEDGEMENT_NORMAL,send_notification,persistent_comment,comment_author,comment_data);
		}
		break;

	case CMD_PROCESS_SERVICE_CHECK_RESULT:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%s;%d;%s|%s",commands[x].host_name,commands[x].description,plugin_state,plugin_output,performance_data);
		}
		break;

	case CMD_PROCESS_HOST_CHECK_RESULT:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%d;%s|%s",commands[x].host_name,plugin_state,plugin_output,performance_data);
		}
		break;

	case CMD_SCHEDULE_HOST_DOWNTIME:
		if(child_options==1)
			cmd = CMD_SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME;
		else if (child_options == 2)
			cmd = CMD_SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME;
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%lu;%lu;%d;%lu;%lu;%s;%s",commands[x].host_name,start_time,end_time,fixed,triggered_by,duration,comment_author,comment_data);
		}
		break;

	case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%lu;%lu;%d;%lu;%lu;%s;%s",commands[x].host_name,start_time,end_time,fixed,triggered_by,duration,comment_author,comment_data);
		}
		break;

	case CMD_SCHEDULE_SVC_DOWNTIME:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%s;%lu;%lu;%d;%lu;%lu;%s;%s",commands[x].host_name,commands[x].description,start_time,end_time,fixed,triggered_by,duration,comment_author,comment_data);
		}
		break;

	case CMD_SCHEDULE_HOST_CHECK:
		if (force_check == TRUE)
			cmd = CMD_SCHEDULE_FORCED_HOST_CHECK;
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%lu",commands[x].host_name,start_time);
		}
		break;

	case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%d;%s;%s",commands[x].host_name,(force_notification | broadcast_notification),comment_author,comment_data);
		}
		break;

	case CMD_SEND_CUSTOM_SVC_NOTIFICATION:
		for ( x = 0; x < NUMBER_OF_STRUCTS; x++ ) {
			if (commands[x].host_name == NULL)
				continue;
			if (is_authorized[x])
				submit_result[x] = cmd_submitf(cmd,"%s;%s;%d;%s;%s",commands[x].host_name,commands[x].description,(force_notification | broadcast_notification),comment_author,comment_data);
		}
		break;


		/***** HOSTGROUP COMMANDS *****/

	case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s",hostgroup_name);
		if(affect_host_and_services==TRUE){
			cmd = (cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS) ? CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS : CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS;
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(cmd,"%s",hostgroup_name);
		}
		break;

	case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s",hostgroup_name);
		break;

	case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
	case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s",hostgroup_name);
		if(affect_host_and_services==TRUE){
			cmd = (cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS) ? CMD_ENABLE_HOSTGROUP_HOST_CHECKS : CMD_DISABLE_HOSTGROUP_HOST_CHECKS;
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(cmd,"%s",hostgroup_name);
		}
		break;

	case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s;%lu;%lu;%d;0;%lu;%s;%s",hostgroup_name,start_time,end_time,fixed,duration,comment_author,comment_data);
		break;

	case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s;%lu;%lu;%d;0;%lu;%s;%s",hostgroup_name,start_time,end_time,fixed,duration,comment_author,comment_data);
		if(affect_host_and_services==TRUE) {
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME,"%s;%lu;%lu;%d;0;%lu;%s;%s",hostgroup_name,start_time,end_time,fixed,duration,comment_author,comment_data);
		}
		break;


		/***** SERVICEGROUP COMMANDS *****/

	case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s",servicegroup_name);
		if(affect_host_and_services==TRUE){
			cmd = (cmd == CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS) ? CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS : CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS;
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(cmd,"%s",servicegroup_name);
		}
		break;

	case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
	case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s",servicegroup_name);
		break;

	case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
	case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s",servicegroup_name);
		if(affect_host_and_services==TRUE){
			cmd = (cmd == CMD_ENABLE_SERVICEGROUP_SVC_CHECKS) ? CMD_ENABLE_SERVICEGROUP_HOST_CHECKS : CMD_DISABLE_SERVICEGROUP_HOST_CHECKS;
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(cmd,"%s",servicegroup_name);
		}
		break;

	case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s;%lu;%lu;%d;0;%lu;%s;%s",servicegroup_name,start_time,end_time,fixed,duration,comment_author,comment_data);
		break;

	case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:
		if (is_authorized[x])
			submit_result[x] = cmd_submitf(cmd,"%s;%lu;%lu;%d;0;%lu;%s;%s",servicegroup_name,start_time,end_time,fixed,duration,comment_author,comment_data);
		if(affect_host_and_services==TRUE) {
			if (is_authorized[x])
				submit_result[x] |= cmd_submitf(CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME, "%s;%lu;%lu;%d;0;%lu;%s;%s",servicegroup_name,start_time,end_time,fixed,duration,comment_author,comment_data);
		}
		break;

	default:
		submit_result[x] = ERROR;
		break;
	}

	return;
}

/* write a command entry to the command file */
int write_command_to_file(char *cmd){
	FILE *fp;
	struct stat statbuf;

	/*
	 * Commands are not allowed to have newlines in them, as
	 * that allows malicious users to hand-craft requests that
	 * bypass the access-restrictions.
	 */
	if (!cmd || !*cmd || strchr(cmd, '\n'))
		return ERROR;

	/* bail out if the external command file doesn't exist */
	if(stat(command_file,&statbuf)){

		if(content_type==WML_CONTENT)
			printf("<p>Error: Could not stat() external command file!</p>\n");
		else{
			printf("<DIV CLASS='errorBox'>\n");
			printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
			printf("<td CLASS='errorMessage'>Error: Could not stat() command file '%s'!</td></tr></table></DIV>\n",command_file);
			printf("<DIV CLASS='errorDescription'>The external command file may be missing, %s may not be running, and/or %s may not be checking external commands.</DIV>\n", PROGRAM_NAME, PROGRAM_NAME);
			printf("</DIV>\n");
			printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
		}

		return ERROR;
	}

	/* open the command for writing (since this is a pipe, it will really be appended) */
	fp=fopen(command_file,"w");
	if(fp==NULL){

		if(content_type==WML_CONTENT)
			printf("<p>Error: Could not open command file for update!</p>\n");
		else{
			printf("<DIV CLASS='errorBox'>\n");
			printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
			printf("<td CLASS='errorMessage'>Error: Could not open command file '%s' for update!</td></tr></table></DIV>\n",command_file);
			printf("<DIV CLASS='errorDescription'>The permissions on the external command file and/or directory may be incorrect. Read the FAQs on how to setup proper permissions.</DIV>\n");
			printf("</DIV>\n");
			printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-2);' class='submitButton'></DIV>\n");
		}

		return ERROR;
	}

	/* write the command to file */
	fprintf(fp, "%s\n", cmd);

	/* flush buffer */
	fflush(fp);

	fclose(fp);

	return OK;
}

/* strips out semicolons from comment data */
void clean_comment_data(char *buffer){
	int x;
	int y;

	y=(int)strlen(buffer);

	for(x=0;x<y;x++){
		if(buffer[x]==';' || buffer[x]=='\n')
			buffer[x]=' ';
	}

	return;
}

/* converts a time string to a UNIX timestamp, respecting the date_format option */
int string_to_time(char *buffer, time_t *t){
	struct tm lt;
	int ret=0;


	/* Initialize some variables just in case they don't get parsed
	   by the sscanf() call.  A better solution is to also check the
	   CGI input for validity, but this should suffice to prevent
	   strange problems if the input is not valid.
	   Jan 15 2003	Steve Bonds */
	lt.tm_mon=0;
	lt.tm_mday=1;
	lt.tm_year=1900;
	lt.tm_hour=0;
	lt.tm_min=0;
	lt.tm_sec=0;
	lt.tm_wday=0;
	lt.tm_yday=0;


	if(date_format==DATE_FORMAT_EURO)
		ret=sscanf(buffer,"%02d-%02d-%04d %02d:%02d:%02d",&lt.tm_mday,&lt.tm_mon,&lt.tm_year,&lt.tm_hour,&lt.tm_min,&lt.tm_sec);
	else if(date_format==DATE_FORMAT_ISO8601 || date_format==DATE_FORMAT_STRICT_ISO8601)
		ret=sscanf(buffer,"%04d-%02d-%02d%*[ T]%02d:%02d:%02d",&lt.tm_year,&lt.tm_mon,&lt.tm_mday,&lt.tm_hour,&lt.tm_min,&lt.tm_sec);
	else
		ret=sscanf(buffer,"%02d-%02d-%04d %02d:%02d:%02d",&lt.tm_mon,&lt.tm_mday,&lt.tm_year,&lt.tm_hour,&lt.tm_min,&lt.tm_sec);

	if (ret!=6)
		return ERROR;

	lt.tm_mon--;
	lt.tm_year-=1900;

	/* tell mktime() to try and compute DST automatically */
	lt.tm_isdst=-1;

	*t=mktime(&lt);

	return OK;
}

/* check if comment data is complete */
void check_comment_sanity(int *e){
	if(!strcmp(comment_author,""))
		error[(*e)++].message=strdup("Author name was not entered");
	if(!strcmp(comment_data,""))
		error[(*e)++].message=strdup("Comment data was not entered");

	return;
}

/* check if given sane values for times */
void check_time_sanity(int *e) {
	if (start_time==(time_t)0)
		error[(*e)++].message=strdup("Start time can't be zero or date format couldn't be recognized correctly");
	if (end_time==(time_t)0)
		error[(*e)++].message=strdup("End time can't be zero or date format couldn't be recognized correctly");
	if (end_time<start_time)
		error[(*e)++].message=strdup("End date before start date");

	return;
}
