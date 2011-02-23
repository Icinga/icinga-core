/*****************************************************************************
 *
 * SHOWLOG.C - Icinga Log File CGI
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)
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
 *
 *****************************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"

#include "../include/getcgi.h"
#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/readlogs.h"

extern char   main_config_file[MAX_FILENAME_LENGTH];
extern char   url_html_path[MAX_FILENAME_LENGTH];
extern char   url_images_path[MAX_FILENAME_LENGTH];
extern char   url_stylesheets_path[MAX_FILENAME_LENGTH];
extern char   url_js_path[MAX_FILENAME_LENGTH];

extern int    log_rotation_method;
extern int    enable_splunk_integration;
extern int    showlog_initial_states;
extern int    showlog_current_states;
extern int    escape_html_tags;

int display_type=DISPLAY_HOSTS;
int show_all_hosts=TRUE;
int show_all_hostgroups=TRUE;
int show_all_servicegroups=TRUE;

char *host_name=NULL;
char *filter=NULL;
char *hostgroup_name=NULL;
char *servicegroup_name=NULL;
char *service_desc=NULL;
char *service_filter=NULL;

int process_cgivars(void);

authdata current_authdata;

int display_log(void);

char log_file_to_use[MAX_FILENAME_LENGTH]="";
int log_archive=0;

int reverse=FALSE;

extern int embedded;
extern int display_header;
int display_frills=TRUE;
int display_timebreaks=TRUE;
extern int daemon_check;

int CGI_ID=SHOWLOG_CGI_ID;

void display_logentries();


char *search=NULL;

int main(void){
	int result=OK;
	char temp_buffer[MAX_INPUT_BUFFER];
	char temp_buffer2[MAX_INPUT_BUFFER];

	/* get the CGI variables passed in the URL */
	process_cgivars();

	/* reset internal variables */
	reset_cgi_vars();

	/* read the CGI configuration file */
	result=read_cgi_config_file(get_cgi_config_location());
	if(result==ERROR){
		document_header(CGI_ID,FALSE);
		print_error(get_cgi_config_location(), ERROR_CGI_CFG_FILE);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read the main configuration file */
	result=read_main_config_file(main_config_file);
	if(result==ERROR){
		document_header(CGI_ID,FALSE);
		print_error(main_config_file, ERROR_CGI_MAIN_CFG);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read all object configuration data */
	result=read_all_object_configuration_data(main_config_file,READ_ALL_OBJECT_DATA);
	if(result==ERROR){
		document_header(CGI_ID,FALSE);
		print_error(NULL, ERROR_CGI_OBJECT_DATA);
		document_footer(CGI_ID);
		return ERROR;
	}

	document_header(CGI_ID,TRUE);

	/* get authentication information */
	get_authentication_information(&current_authdata);

	/* determine what log file we should be using */
	get_log_archive_to_use(log_archive,log_file_to_use,(int)sizeof(log_file_to_use));

	if(display_header==TRUE){

		/* begin top table */
		printf("<table border=0 width=100%% cellpadding=0 cellspacing=0>\n");
		printf("<tr>\n");

		/* left column of top table - info box */
		printf("<td align=left valign=top width=33%%>\n");
		display_info_table((log_rotation_method==LOG_ROTATION_NONE || log_archive==0)?"Current Event Log":"Archived Event Log",FALSE,&current_authdata, daemon_check);
		printf("</td>\n");

		/* middle column of top table - log file navigation options */
		printf("<td align=center valign=top width=33%%>\n");
		
		if (search!=NULL) {
			snprintf(temp_buffer2,sizeof(temp_buffer2)-1,"search=%s&",url_encode(search));
			temp_buffer2[sizeof(temp_buffer2)-1]='\x0';
		}
		snprintf(temp_buffer,sizeof(temp_buffer)-1,"%s?%s%s",SHOWLOG_CGI,(reverse==TRUE)?"oldestfirst&":"",(search!=NULL)?temp_buffer2:"");
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		display_nav_table(temp_buffer,log_archive);
		
		printf("</td>\n");

		/* right hand column of top row */
		printf("<td align=right valign=top width=33%%>\n");

		printf("<table border=0 cellspacing=0 cellpadding=0 CLASS='optBox'>\n");
		printf("<form method='GET' action='%s'>\n",SHOWLOG_CGI);
		printf("<input type='hidden' name='archive' value='%d'>\n",log_archive);
		printf("<tr>");
		printf("<td align=left valign=bottom CLASS='optBoxItem'><input type='checkbox' name='oldestfirst' %s> Older Entries First:</td>",(reverse==TRUE)?"checked":"");
		printf("</tr>\n");
		printf("<tr>");
		printf("<td align=left valign=bottom CLASS='optBoxItem'><input type='submit' value='Update'></td>\n");
		printf("</tr>\n");

		/* search box */
		//escape_html_tags=TRUE;
		//printf("<tr><td><br><input type='text' name='search' size='20' class='NavBarSearchItem' value='%s'>&nbsp;<input type='submit' value='find'></td></tr>",(search==NULL)?"":html_encode(search,TRUE));

		/* display context-sensitive help */
		printf("<tr>\n");
		printf("<td align=right>\n");
		display_context_help(CONTEXTHELP_LOG);
		printf("</td>\n");
		printf("</tr>\n");

		printf("</form>\n");
		printf("</table>\n");

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");
		printf("</p>\n");

	}

	/* check to see if the user is authorized to view the log file */
	if(is_authorized_for_system_information(&current_authdata)==FALSE){
		print_generic_error_message("It appears as though you do not have permission to view the log file...","If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.",0);
		return ERROR;
	}

	/* display the contents of the log file */
	display_logentries();

	document_footer(CGI_ID);

	/* free allocated memory */
	free_memory();
	
	return OK;
}

int process_cgivars(void){
	char **variables;
	int error=FALSE;
	int x;

	variables=getcgivars();

	for(x=0;variables[x]!=NULL;x++){

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if(strlen(variables[x])>=MAX_INPUT_BUFFER-1){
			continue;
		        }

		/* we found the archive argument */
		else if(!strcmp(variables[x],"archive")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			        }

			log_archive=atoi(variables[x]);
			if(log_archive<0)
				log_archive=0;
		        }

		else if(!strcmp(variables[x],"search")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			search=strdup(variables[x]);
			strip_html_brackets(search);
			
			if(strlen(search)==0)
				search=NULL;
		}

		/* we found the order argument */
		else if(!strcmp(variables[x],"oldestfirst"))
			reverse=TRUE;

		/* we found the embed option */
		else if(!strcmp(variables[x],"embedded"))
			embedded=TRUE;

		/* we found the noheader option */
		else if(!strcmp(variables[x],"noheader"))
			display_header=FALSE;

		/* we found the nofrills option */
		else if (!strcmp(variables[x],"nofrills"))
			display_frills=FALSE;

		/* we found the notimebreaks option */
		else if(!strcmp(variables[x],"notimebreaks"))
			display_timebreaks=FALSE;

		/* we found the nodaemoncheck option */
		else if(!strcmp(variables[x],"nodaemoncheck"))
			daemon_check=FALSE;

		/* we received an invalid argument */
		else
			error=TRUE;
	
	        }

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}


/* display the contents of the log file */
void display_logentries() {
	char image[MAX_INPUT_BUFFER];
	char image_alt[MAX_INPUT_BUFFER];
	char last_message_date[MAX_INPUT_BUFFER]="";
	char current_message_date[MAX_INPUT_BUFFER]="";
	char date_time[MAX_DATETIME_LENGTH];
	char error_text[MAX_INPUT_BUFFER]="";
	int status=0;
	struct tm *time_ptr=NULL;
	logentry *temp_entry=NULL;
	
	/*
	if (search!=NULL) {
		escape_html_tags=TRUE;
		printf("<P><H2><DIV style='text-align:center'>Filter: \"%s\"</DIV></H2></P>",html_encode(search,TRUE));
	}*/
	
	if (showlog_initial_states==FALSE) {
		add_log_filter(LOGENTRY_SERVICE_INITIAL_STATE,LOGFILTER_EXCLUDE);
		add_log_filter(LOGENTRY_HOST_INITIAL_STATE,LOGFILTER_EXCLUDE);
	}
	if (showlog_current_states==FALSE) {
		add_log_filter(LOGENTRY_SERVICE_CURRENT_STATE,LOGFILTER_EXCLUDE);
		add_log_filter(LOGENTRY_HOST_CURRENT_STATE,LOGFILTER_EXCLUDE);
	}
	
	status=get_log_entries(log_file_to_use,search,reverse);

	if (status==READLOG_ERROR_MEMORY) {
		printf("<P><DIV CLASS='warningMessage'>Run out of memory..., showing all I could get!</DIV></P>");
	}
	if (status==READLOG_ERROR_NOFILE) {
		snprintf(error_text,sizeof(error_text),"Error: Could not open log file '%s' for reading!",log_file_to_use);
		error_text[sizeof(error_text)-1]='\x0';
		print_generic_error_message(error_text,NULL,0);
	}
	if (status==READLOG_ERROR_FILTER) {
		print_generic_error_message("It seems like that reagular expressions don't like waht you searched for. Please change your search string.",NULL,0);
	}
	else if (status==READLOG_OK) {

		printf("<P><DIV CLASS='logEntries'>\n");

		for(temp_entry=next_log_entry();temp_entry!=NULL;temp_entry=next_log_entry()) {

			if(temp_entry->type==LOGENTRY_STARTUP){
				strcpy(image,START_ICON);
				strcpy(image_alt,START_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_SHUTDOWN || temp_entry->type==LOGENTRY_BAILOUT){
				strcpy(image,STOP_ICON);
				strcpy(image_alt,STOP_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_RESTART){
				strcpy(image,RESTART_ICON);
				strcpy(image_alt,RESTART_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_HOST_DOWN){
				strcpy(image,HOST_DOWN_ICON);
				strcpy(image_alt,HOST_DOWN_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_HOST_UNREACHABLE){
				strcpy(image,HOST_UNREACHABLE_ICON);
				strcpy(image_alt,HOST_UNREACHABLE_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_HOST_RECOVERY || temp_entry->type==LOGENTRY_HOST_UP){
				strcpy(image,HOST_UP_ICON);
				strcpy(image_alt,HOST_UP_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_HOST_NOTIFICATION){
				strcpy(image,HOST_NOTIFICATION_ICON);
				strcpy(image_alt,HOST_NOTIFICATION_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_CRITICAL){
				strcpy(image,CRITICAL_ICON);
				strcpy(image_alt,CRITICAL_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_WARNING){
				strcpy(image,WARNING_ICON);
				strcpy(image_alt,WARNING_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_UNKNOWN){
				strcpy(image,UNKNOWN_ICON);
				strcpy(image_alt,UNKNOWN_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_RECOVERY || temp_entry->type==LOGENTRY_SERVICE_OK){
				strcpy(image,OK_ICON);
				strcpy(image_alt,OK_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_NOTIFICATION){
				strcpy(image,NOTIFICATION_ICON);
				strcpy(image_alt,NOTIFICATION_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_EVENT_HANDLER){
				strcpy(image,SERVICE_EVENT_ICON);
				strcpy(image_alt,SERVICE_EVENT_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_HOST_EVENT_HANDLER){
				strcpy(image,HOST_EVENT_ICON);
				strcpy(image_alt,HOST_EVENT_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_EXTERNAL_COMMAND){
				strcpy(image,EXTERNAL_COMMAND_ICON);
				strcpy(image_alt,EXTERNAL_COMMAND_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_PASSIVE_SERVICE_CHECK){
				strcpy(image,PASSIVE_ICON);
				strcpy(image_alt,"Passive Service Check");
			}
			else if(temp_entry->type==LOGENTRY_PASSIVE_HOST_CHECK){
				strcpy(image,PASSIVE_ICON);
				strcpy(image_alt,"Passive Host Check");
			}
			else if(temp_entry->type==LOGENTRY_LOG_ROTATION){
				strcpy(image,LOG_ROTATION_ICON);
				strcpy(image_alt,LOG_ROTATION_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_ACTIVE_MODE){
				strcpy(image,ACTIVE_ICON);
				strcpy(image_alt,ACTIVE_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_STANDBY_MODE){
				strcpy(image,STANDBY_ICON);
				strcpy(image_alt,STANDBY_ICON_ALT);
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_FLAPPING_STARTED){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Service started flapping");
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_FLAPPING_STOPPED){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Service stopped flapping");
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_FLAPPING_DISABLED){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Service flap detection disabled");
			}
			else if(temp_entry->type==LOGENTRY_HOST_FLAPPING_STARTED){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Host started flapping");
			}
			else if(temp_entry->type==LOGENTRY_HOST_FLAPPING_STOPPED){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Host stopped flapping");
			}
			else if(temp_entry->type==LOGENTRY_HOST_FLAPPING_DISABLED){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Host flap detection disabled");
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_DOWNTIME_STARTED){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Service entered a period of scheduled downtime");
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_DOWNTIME_STOPPED){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Service exited a period of scheduled downtime");
			}
			else if(temp_entry->type==LOGENTRY_SERVICE_DOWNTIME_CANCELLED){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Service scheduled downtime has been cancelled");
			}
			else if(temp_entry->type==LOGENTRY_HOST_DOWNTIME_STARTED){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Host entered a period of scheduled downtime");
			}
			else if(temp_entry->type==LOGENTRY_HOST_DOWNTIME_STOPPED){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Host exited a period of scheduled downtime");
			}
			else if(temp_entry->type==LOGENTRY_HOST_DOWNTIME_CANCELLED){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Host scheduled downtime has been cancelled");
			}
			else{
				strcpy(image,INFO_ICON);
				strcpy(image_alt,INFO_ICON_ALT);
			}

			time_ptr=localtime(&temp_entry->timestamp);
			strftime(current_message_date,sizeof(current_message_date),"%B %d, %Y %H:00\n",time_ptr);
			current_message_date[sizeof(current_message_date)-1]='\x0';

			if(strcmp(last_message_date,current_message_date)!=0 && display_timebreaks==TRUE){
				printf("<BR CLEAR='all'>\n");
				printf("<DIV CLASS='dateTimeBreak'>\n");
				printf("<table border=0 width=95%%><tr>");
				printf("<td width=40%%><hr width=100%%></td>");
				printf("<td align=center CLASS='dateTimeBreak'>%s</td>",current_message_date);
				printf("<td width=40%%><hr width=100%%></td>");
				printf("</tr></table>\n");
				printf("</DIV>\n");
				printf("<BR CLEAR='all'><DIV CLASS='logEntries'>\n");
				strncpy(last_message_date,current_message_date,sizeof(last_message_date));
				last_message_date[sizeof(last_message_date)-1]='\x0';
			}

			get_time_string(&temp_entry->timestamp,date_time,(int)sizeof(date_time),SHORT_DATE_TIME);
			strip(date_time);

			if(display_frills==TRUE)
				printf("<img align='left' src='%s%s' alt='%s' title='%s'>",url_images_path,image,image_alt,image_alt);
			printf("[%s] %s",date_time,(temp_entry->entry_text==NULL)?"":html_encode(temp_entry->entry_text,FALSE));
			if(enable_splunk_integration==TRUE){
				printf("&nbsp;&nbsp;&nbsp;");
				display_splunk_generic_url(temp_entry->entry_text,2);
			}
			printf("<br clear='all'>\n");

			my_free(temp_entry->entry_text);
			my_free(temp_entry);
		}
				
		printf("</DIV></P>\n");
		printf("<HR>\n");

		free_log_entries();
	}
	return;
}
