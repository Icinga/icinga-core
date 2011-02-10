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

extern char   main_config_file[MAX_FILENAME_LENGTH];
extern char   url_html_path[MAX_FILENAME_LENGTH];
extern char   url_images_path[MAX_FILENAME_LENGTH];
extern char   url_stylesheets_path[MAX_FILENAME_LENGTH];
extern char   url_js_path[MAX_FILENAME_LENGTH];

extern int    log_rotation_method;
extern int    enable_splunk_integration;
extern int    showlog_initial_states;
extern int    showlog_current_states;

int display_type=DISPLAY_HOSTS;
int show_all_hosts=TRUE;
int show_all_hostgroups=TRUE;
int show_all_servicegroups=TRUE;

char *host_name=NULL;
char *host_filter=NULL;
char *hostgroup_name=NULL;
char *servicegroup_name=NULL;
char *service_desc=NULL;
char *service_filter=NULL;

int process_cgivars(void);

authdata current_authdata;

int display_log(void);

char log_file_to_use[MAX_FILENAME_LENGTH]="";
int log_archive=0;

int use_lifo=TRUE;

extern int embedded;
extern int display_header;
int display_frills=TRUE;
int display_timebreaks=TRUE;
extern int daemon_check;

int CGI_ID=SHOWLOG_CGI_ID;

int main(void){
	int result=OK;
	char temp_buffer[MAX_INPUT_BUFFER];


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
		snprintf(temp_buffer,sizeof(temp_buffer)-1,"%s?%s",SHOWLOG_CGI,(use_lifo==FALSE)?"oldestfirst&":"");
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		display_nav_table(temp_buffer,log_archive);
		printf("</td>\n");

		/* right hand column of top row */
		printf("<td align=right valign=top width=33%%>\n");

		printf("<table border=0 cellspacing=0 cellpadding=0 CLASS='optBox'>\n");
		printf("<form method='GET' action='%s'>\n",SHOWLOG_CGI);
		printf("<input type='hidden' name='archive' value='%d'>\n",log_archive);
		printf("<tr>");
		printf("<td align=left valign=bottom CLASS='optBoxItem'><input type='checkbox' name='oldestfirst' %s> Older Entries First:</td>",(use_lifo==FALSE)?"checked":"");
		printf("</tr>\n");
		printf("<tr>");
		printf("<td align=left valign=bottom CLASS='optBoxItem'><input type='submit' value='Update'></td>\n");
		printf("</tr>\n");

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


	/* display the contents of the log file */
	display_log();

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

		/* we found the order argument */
		else if(!strcmp(variables[x],"oldestfirst")){
			use_lifo=FALSE;
		        }

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
int display_log(void){
	char *input=NULL;
	char image[MAX_INPUT_BUFFER];
	char image_alt[MAX_INPUT_BUFFER];
	time_t t;
	char *temp_buffer=NULL;
	char date_time[MAX_DATETIME_LENGTH];
	int error=FALSE;
	mmapfile *thefile=NULL;
	char last_message_date[MAX_INPUT_BUFFER]="";
	char current_message_date[MAX_INPUT_BUFFER]="";
	char error_text[MAX_INPUT_BUFFER]="";
	struct tm *time_ptr=NULL;


	/* check to see if the user is authorized to view the log file */
	if(is_authorized_for_system_information(&current_authdata)==FALSE){
		print_generic_error_message("It appears as though you do not have permission to view the log file...","If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.",0);
		return ERROR;
	}

	error=FALSE;

	if(use_lifo==TRUE){
		error=read_file_into_lifo(log_file_to_use);
		if(error!=LIFO_OK){
			if(error==LIFO_ERROR_MEMORY){
				printf("<P><DIV CLASS='warningMessage'>Not enough memory to reverse log file - displaying log in natural order...</DIV></P>");
				error=FALSE;
			        }
			else
				error=TRUE;
			use_lifo=FALSE;
		        }
		else
			error=FALSE;
	        }

	if(use_lifo==FALSE){

		if((thefile=mmap_fopen(log_file_to_use))==NULL){
			snprintf(error_text,sizeof(error_text),"Error: Could not open log file '%s' for reading!",log_file_to_use);
			error_text[sizeof(error_text)-1]='\x0';
			print_generic_error_message(error_text,NULL,0);
			error=TRUE;
		}
	}

	if(error==FALSE){

		printf("<P><DIV CLASS='logEntries'>\n");
		
		while(1){

			free(input);

			if(use_lifo==TRUE){
				if((input=pop_lifo())==NULL)
					break;
			        }
			else if((input=mmap_fgets(thefile))==NULL)
				break;

			strip(input);

			if ( showlog_initial_states==FALSE && (strstr(input,"INITIAL SERVICE STATE:") || strstr(input,"INITIAL HOST STATE:"))){
				continue;
			}

			if ( showlog_current_states==FALSE && (strstr(input,"CURRENT SERVICE STATE:") || strstr(input,"CURRENT HOST STATE:"))){
				continue;
			}

			if(strstr(input," starting...")){
				strcpy(image,START_ICON);
				strcpy(image_alt,START_ICON_ALT);
			        }
			else if(strstr(input," shutting down...")){
				strcpy(image,STOP_ICON);
				strcpy(image_alt,STOP_ICON_ALT);
			        }
			else if(strstr(input,"Bailing out")){
				strcpy(image,STOP_ICON);
				strcpy(image_alt,STOP_ICON_ALT);
			        }
			else if(strstr(input," restarting...")){
				strcpy(image,RESTART_ICON);
				strcpy(image_alt,RESTART_ICON_ALT);
			        }
			else if(strstr(input,"HOST ALERT:") && strstr(input,";DOWN;")){
				strcpy(image,HOST_DOWN_ICON);
				strcpy(image_alt,HOST_DOWN_ICON_ALT);
			        }
			else if(strstr(input,"HOST ALERT:") && strstr(input,";UNREACHABLE;")){
				strcpy(image,HOST_UNREACHABLE_ICON);
				strcpy(image_alt,HOST_UNREACHABLE_ICON_ALT);
			        }
			else if(strstr(input,"HOST ALERT:") && (strstr(input,";RECOVERY;") || strstr(input,";UP;"))){
				strcpy(image,HOST_UP_ICON);
				strcpy(image_alt,HOST_UP_ICON_ALT);
			        }
			else if(strstr(input,"HOST NOTIFICATION:")){
				strcpy(image,HOST_NOTIFICATION_ICON);
				strcpy(image_alt,HOST_NOTIFICATION_ICON_ALT);
			        }
			else if(strstr(input,"SERVICE ALERT:") && strstr(input,";CRITICAL;")){
				strcpy(image,CRITICAL_ICON);
				strcpy(image_alt,CRITICAL_ICON_ALT);
			        }
			else if(strstr(input,"SERVICE ALERT:") && strstr(input,";WARNING;")){
				strcpy(image,WARNING_ICON);
				strcpy(image_alt,WARNING_ICON_ALT);
			        }
			else if(strstr(input,"SERVICE ALERT:") && strstr(input,";UNKNOWN;")){
				strcpy(image,UNKNOWN_ICON);
				strcpy(image_alt,UNKNOWN_ICON_ALT);
			        }
			else if(strstr(input,"SERVICE ALERT:") && (strstr(input,";RECOVERY;") || strstr(input,";OK;"))){
				strcpy(image,OK_ICON);
				strcpy(image_alt,OK_ICON_ALT);
			        }
			else if(strstr(input,"SERVICE NOTIFICATION:")){
				strcpy(image,NOTIFICATION_ICON);
				strcpy(image_alt,NOTIFICATION_ICON_ALT);
			        }
			else if(strstr(input,"SERVICE EVENT HANDLER:")){
				strcpy(image,SERVICE_EVENT_ICON);
				strcpy(image_alt,SERVICE_EVENT_ICON_ALT);
			        }
			else if(strstr(input,"HOST EVENT HANDLER:")){
				strcpy(image,HOST_EVENT_ICON);
				strcpy(image_alt,HOST_EVENT_ICON_ALT);
			        }
			else if(strstr(input,"EXTERNAL COMMAND:")){
				strcpy(image,EXTERNAL_COMMAND_ICON);
				strcpy(image_alt,EXTERNAL_COMMAND_ICON_ALT);
			        }
			else if(strstr(input,"PASSIVE SERVICE CHECK:")){
				strcpy(image,PASSIVE_ICON);
				strcpy(image_alt,"Passive Service Check");
			        }
			else if(strstr(input,"PASSIVE HOST CHECK:")){
				strcpy(image,PASSIVE_ICON);
				strcpy(image_alt,"Passive Host Check");
			        }
			else if(strstr(input,"LOG ROTATION:")){
				strcpy(image,LOG_ROTATION_ICON);
				strcpy(image_alt,LOG_ROTATION_ICON_ALT);
			        }
			else if(strstr(input,"active mode...")){
				strcpy(image,ACTIVE_ICON);
				strcpy(image_alt,ACTIVE_ICON_ALT);
			        }
			else if(strstr(input,"standby mode...")){
				strcpy(image,STANDBY_ICON);
				strcpy(image_alt,STANDBY_ICON_ALT);
			        }
			else if(strstr(input,"SERVICE FLAPPING ALERT:") && strstr(input,";STARTED;")){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Service started flapping");
			        }
			else if(strstr(input,"SERVICE FLAPPING ALERT:") && strstr(input,";STOPPED;")){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Service stopped flapping");
			        }
			else if(strstr(input,"SERVICE FLAPPING ALERT:") && strstr(input,";DISABLED;")){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Service flap detection disabled");
			        }
			else if(strstr(input,"HOST FLAPPING ALERT:") && strstr(input,";STARTED;")){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Host started flapping");
			        }
			else if(strstr(input,"HOST FLAPPING ALERT:") && strstr(input,";STOPPED;")){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Host stopped flapping");
			        }
			else if(strstr(input,"HOST FLAPPING ALERT:") && strstr(input,";DISABLED;")){
				strcpy(image,FLAPPING_ICON);
				strcpy(image_alt,"Host flap detection disabled");
			        }
			else if(strstr(input,"SERVICE DOWNTIME ALERT:") && strstr(input,";STARTED;")){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Service entered a period of scheduled downtime");
			        }
			else if(strstr(input,"SERVICE DOWNTIME ALERT:") && strstr(input,";STOPPED;")){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Service exited a period of scheduled downtime");
			        }
			else if(strstr(input,"SERVICE DOWNTIME ALERT:") && strstr(input,";CANCELLED;")){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Service scheduled downtime has been cancelled");
			        }
			else if(strstr(input,"HOST DOWNTIME ALERT:") && strstr(input,";STARTED;")){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Host entered a period of scheduled downtime");
			        }
			else if(strstr(input,"HOST DOWNTIME ALERT:") && strstr(input,";STOPPED;")){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Host exited a period of scheduled downtime");
			        }
			else if(strstr(input,"HOST DOWNTIME ALERT:") && strstr(input,";CANCELLED;")){
				strcpy(image,SCHEDULED_DOWNTIME_ICON);
				strcpy(image_alt,"Host scheduled downtime has been cancelled");
			        }
			else{
				strcpy(image,INFO_ICON);
				strcpy(image_alt,INFO_ICON_ALT);
			        }

			temp_buffer=strtok(input,"]");
			t=(temp_buffer==NULL)?0L:strtoul(temp_buffer+1,NULL,10);

			time_ptr=localtime(&t);
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

			get_time_string(&t,date_time,(int)sizeof(date_time),SHORT_DATE_TIME);
			strip(date_time);

			temp_buffer=strtok(NULL,"\n");

			if(display_frills==TRUE)
				printf("<img align='left' src='%s%s' alt='%s' title='%s'>",url_images_path,image,image_alt,image_alt);
			printf("[%s] %s",date_time,(temp_buffer==NULL)?"":html_encode(temp_buffer,FALSE));
			if(enable_splunk_integration==TRUE){
				printf("&nbsp;&nbsp;&nbsp;");
				display_splunk_generic_url(temp_buffer,2);
				}
			printf("<br clear='all'>\n");
		        }

		printf("</DIV></P>\n");
		printf("<HR>\n");

		free(input);

		if(use_lifo==FALSE)
			mmap_fclose(thefile);
	        }

	if(use_lifo==TRUE)
		free_lifo_memory();

	return OK;
        }

