/*****************************************************************************
 *
 * HISTORY.C - Icinga History CGI
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

/** @file history.c
 *  @brief cgi to browse through log history of a host/service
**/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"

#include "../include/getcgi.h"
#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/readlogs.h"


/** @name External vars
    @{ **/
extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_html_path[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];
extern char url_stylesheets_path[MAX_FILENAME_LENGTH];
extern char url_js_path[MAX_FILENAME_LENGTH];

extern int log_rotation_method;
extern int enable_splunk_integration;
extern int embedded;
extern int display_header;
extern int daemon_check;

extern logentry *entry_list;
/** @} */

/** @name Vars which are imported for cgiutils
 *  @warning these wars should be all extern, @n
 *	then they could get deleted, because they aren't used here.
 *	@n cgiutils.c , needs them
    @{ **/
int show_all_hostgroups=TRUE;
int show_all_servicegroups=TRUE;
char *host_filter=NULL;
char *hostgroup_name=NULL;
char *servicegroup_name=NULL;
char *service_filter=NULL;
/** @} */

/** @name Internal vars
    @{ **/
int log_archive=0;				/**< holds the archive id, which should be shown */
int display_type=DISPLAY_HOSTS;			/**< determine the view (host/service) */
int show_all_hosts=TRUE;			/**< if historical data is requested for all hosts */
int reverse=FALSE;				/**< determine if log should be viewed in reverse order */
int history_options=HISTORY_ALL;		/**< determines the type of historical data */
int state_options=STATE_ALL;			/**< the state of historical data */

int display_frills=TRUE;			/**< determine if icons should be shown in listing */
int display_timebreaks=TRUE;			/**< determine if time breaks should be shown */
int display_system_messages=TRUE;		/**< determine if system messages should be shown */
int display_flapping_alerts=TRUE;		/**< determine if flapping alerts should be shown */
int display_downtime_alerts=TRUE;		/**< determine if downtime alerts should be shown */

char *host_name="all";				/**< the requested host name */
char *service_desc="";				/**< the requested service name */
char log_file_to_use[MAX_FILENAME_LENGTH];	/**< the name of the logfile to read data from */

authdata current_authdata;			/**< struct to hold current authentication data */

int CGI_ID=HISTORY_CGI_ID;			/**< ID to identify the cgi for functions in cgiutils.c */
/** @} */

/** @brief displays the requested historical log entries
 *
 * Applies the requested filters, reads in all necessary log files
 * and afterwards showing each log entry.
**/
void show_history(void);

/** @brief Parses the requested GET/POST variables
 *  @return wether parsing was successful or not
 *	@retval TRUE
 *	@retval FALSE
 *
 *  @n This function parses the request and set's the necessary variables
**/
int process_cgivars(void);

/** @brief Yes we need a main function **/
int main(void){
	int result=OK;
	char temp_buffer[MAX_INPUT_BUFFER];
	char temp_buffer2[MAX_INPUT_BUFFER];

	/* get the variables passed to us */
	process_cgivars();

	/* reset internal CGI variables */
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
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=33%%>\n");

		if(display_type==DISPLAY_SERVICES)
			snprintf(temp_buffer,sizeof(temp_buffer)-1,"Service Alert History");
		else if(show_all_hosts==TRUE)
			snprintf(temp_buffer,sizeof(temp_buffer)-1,"Alert History");
		else
			snprintf(temp_buffer,sizeof(temp_buffer)-1,"Host Alert History");
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		display_info_table(temp_buffer,FALSE,&current_authdata, daemon_check);

		printf("<TABLE BORDER=1 CELLPADDING=0 CELLSPACING=0 CLASS='linkBox'>\n");
		printf("<TR><TD CLASS='linkBox'>\n");
		if(display_type==DISPLAY_HOSTS){
			printf("<A HREF='%s?host=%s'>View Status Detail For %s</A><BR />\n",STATUS_CGI,(show_all_hosts==TRUE)?"all":url_encode(host_name),(show_all_hosts==TRUE)?"All Hosts":"This Host");
			printf("<A HREF='%s?host=%s'>View Notifications For %s</A><BR />\n",NOTIFICATIONS_CGI,(show_all_hosts==TRUE)?"all":url_encode(host_name),(show_all_hosts==TRUE)?"All Hosts":"This Host");
#ifdef USE_TRENDS
			if(show_all_hosts==FALSE)
				printf("<A HREF='%s?host=%s'>View Trends For This Host</A>\n",TRENDS_CGI,url_encode(host_name));
#endif
		}else{
			printf("<A HREF='%s?host=%s&",NOTIFICATIONS_CGI,url_encode(host_name));
			printf("service=%s'>View Notifications For This Service</A><BR />\n",url_encode(service_desc));
#ifdef USE_TRENDS
			printf("<A HREF='%s?host=%s&",TRENDS_CGI,url_encode(host_name));
			printf("service=%s'>View Trends For This Service</A><BR />\n",url_encode(service_desc));
#endif
			printf("<A HREF='%s?host=%s'>View History For This Host</A>\n",HISTORY_CGI,url_encode(host_name));
		}
		printf("</TD></TR>\n");
		printf("</TABLE>\n");

		printf("</td>\n");

		/* middle column of top row */
		printf("<td align=center valign=top width=33%%>\n");

		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>\n");
		if(display_type==DISPLAY_SERVICES)
			printf("Service '%s' On Host '%s'",service_desc,host_name);
		else if(show_all_hosts==TRUE)
			printf("All Hosts and Services");
		else
			printf("Host '%s'",host_name);
		printf("</DIV>\n");
		printf("<BR />\n");

		snprintf(temp_buffer,sizeof(temp_buffer)-1,"%s?%shost=%s&type=%d&statetype=%d&",HISTORY_CGI,(reverse==TRUE)?"oldestfirst&":"",url_encode(host_name),history_options,state_options);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		if(display_type==DISPLAY_SERVICES){
			snprintf(temp_buffer2,sizeof(temp_buffer2)-1,"service=%s&",url_encode(service_desc));
			temp_buffer2[sizeof(temp_buffer2)-1]='\x0';
			strncat(temp_buffer,temp_buffer2,sizeof(temp_buffer)-strlen(temp_buffer)-1);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
		}
		display_nav_table(temp_buffer,log_archive);

		printf("</td>\n");

		/* right hand column of top row */
		printf("<td align=right valign=top width=33%%>\n");

		printf("<form method=\"GET\" action=\"%s\">\n",HISTORY_CGI);
		printf("<table border=0 CLASS='optBox'>\n");
		printf("<input type='hidden' name='host' value='%s'>\n",(show_all_hosts==TRUE)?"all":escape_string(host_name));
		if(display_type==DISPLAY_SERVICES)
			printf("<input type='hidden' name='service' value='%s'>\n",escape_string(service_desc));
		printf("<input type='hidden' name='archive' value='%d'>\n",log_archive);

		printf("<tr>\n");
		printf("<td align=left CLASS='optBoxItem'>State type options:</td>\n");
		printf("</tr>\n");

		printf("<tr>\n");
		printf("<td align=left CLASS='optBoxItem'><select name='statetype'>\n");
		printf("<option value=%d %s>All state types</option>\n",STATE_ALL,(state_options==STATE_ALL)?"selected":"");
		printf("<option value=%d %s>Soft states</option>\n",STATE_SOFT,(state_options==STATE_SOFT)?"selected":"");
		printf("<option value=%d %s>Hard states</option>\n",STATE_HARD,(state_options==STATE_HARD)?"selected":"");
		printf("</select></td>\n");
		printf("</tr>\n");

		printf("<tr>\n");
		printf("<td align=left CLASS='optBoxItem'>History detail level for ");
		if(display_type==DISPLAY_HOSTS)
			printf("%s host%s",(show_all_hosts==TRUE)?"all":"this",(show_all_hosts==TRUE)?"s":"");
		else
			printf("service");
		printf(":</td>\n");
		printf("</tr>\n");
		printf("<tr>\n");
		printf("<td align=left CLASS='optBoxItem'><select name='type'>\n");
		if(display_type==DISPLAY_HOSTS)
			printf("<option value=%d %s>All alerts</option>\n",HISTORY_ALL,(history_options==HISTORY_ALL)?"selected":"");
		printf("<option value=%d %s>All service alerts</option>\n",HISTORY_SERVICE_ALL,(history_options==HISTORY_SERVICE_ALL)?"selected":"");
		if(display_type==DISPLAY_HOSTS)
			printf("<option value=%d %s>All host alerts</option>\n",HISTORY_HOST_ALL,(history_options==HISTORY_HOST_ALL)?"selected":"");
		printf("<option value=%d %s>Service warning</option>\n",HISTORY_SERVICE_WARNING,(history_options==HISTORY_SERVICE_WARNING)?"selected":"");
		printf("<option value=%d %s>Service unknown</option>\n",HISTORY_SERVICE_UNKNOWN,(history_options==HISTORY_SERVICE_UNKNOWN)?"selected":"");
		printf("<option value=%d %s>Service critical</option>\n",HISTORY_SERVICE_CRITICAL,(history_options==HISTORY_SERVICE_CRITICAL)?"selected":"");
		printf("<option value=%d %s>Service recovery</option>\n",HISTORY_SERVICE_RECOVERY,(history_options==HISTORY_SERVICE_RECOVERY)?"selected":"");
		if(display_type==DISPLAY_HOSTS){
			printf("<option value=%d %s>Host down</option>\n",HISTORY_HOST_DOWN,(history_options==HISTORY_HOST_DOWN)?"selected":"");
			printf("<option value=%d %s>Host unreachable</option>\n",HISTORY_HOST_UNREACHABLE,(history_options==HISTORY_HOST_UNREACHABLE)?"selected":"");
			printf("<option value=%d %s>Host recovery</option>\n",HISTORY_HOST_RECOVERY,(history_options==HISTORY_HOST_RECOVERY)?"selected":"");
		}
		printf("</select></td>\n");
		printf("</tr>\n");

		printf("<tr>\n");
		printf("<td align=left valign=bottom CLASS='optBoxItem'><input type='checkbox' name='noflapping' %s> Hide Flapping Alerts</td>",(display_flapping_alerts==FALSE)?"checked":"");
		printf("</tr>\n");
		printf("<tr>\n");
		printf("<td align=left valign=bottom CLASS='optBoxItem'><input type='checkbox' name='nodowntime' %s> Hide Downtime Alerts</td>",(display_downtime_alerts==FALSE)?"checked":"");
		printf("</tr>\n");

		printf("<tr>\n");
		printf("<td align=left valign=bottom CLASS='optBoxItem'><input type='checkbox' name='nosystem' %s> Hide Process Messages</td>",(display_system_messages==FALSE)?"checked":"");
		printf("</tr>\n");
		printf("<tr>\n");
		printf("<td align=left valign=bottom CLASS='optBoxItem'><input type='checkbox' name='oldestfirst' %s> Older Entries First</td>",(reverse==TRUE)?"checked":"");
		printf("</tr>\n");

		printf("<tr>\n");
		printf("<td align=left CLASS='optBoxItem'><input type='submit' value='Update'></td>\n");
		printf("</tr>\n");

		/* display context-sensitive help */
		printf("<tr>\n");
		printf("<td align=right>\n");
		display_context_help(CONTEXTHELP_HISTORY);
		printf("</td>\n");
		printf("</tr>\n");

		printf("</table>\n");
		printf("</form>\n");

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");

	}

	/* display history */
	show_history();

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
		if(strlen(variables[x])>=MAX_INPUT_BUFFER-1)
			continue;

		/* we found the host argument */
		else if(!strcmp(variables[x],"host")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			if((host_name=(char *)strdup(variables[x]))==NULL)
				host_name="";
			strip_html_brackets(host_name);

			display_type=DISPLAY_HOSTS;

			if(!strcmp(host_name,"all"))
				show_all_hosts=TRUE;
			else
				show_all_hosts=FALSE;
		}

		/* we found the service argument */
		else if(!strcmp(variables[x],"service")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			if((service_desc=(char *)strdup(variables[x]))==NULL)
				service_desc="";
			strip_html_brackets(service_desc);

			display_type=DISPLAY_SERVICES;
		}


		/* we found the history type argument */
		else if(!strcmp(variables[x],"type")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			history_options=atoi(variables[x]);
		}

		/* we found the history state type argument */
		else if(!strcmp(variables[x],"statetype")){
			x++;
			if(variables[x]==NULL){
				error=TRUE;
				break;
			}

			state_options=atoi(variables[x]);
		}


		/* we found the log archive argument */
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
			reverse=TRUE;
		}

		/* we found the embed option */
		else if(!strcmp(variables[x],"embedded"))
			embedded=TRUE;

		/* we found the noheader option */
		else if(!strcmp(variables[x],"noheader"))
			display_header=FALSE;

		/* we found the nodaemoncheck option */
		else if(!strcmp(variables[x],"nodaemoncheck"))
			daemon_check=FALSE;

		/* we found the nofrills option */
		else if(!strcmp(variables[x],"nofrills"))
			display_frills=FALSE;

		/* we found the notimebreaks option */
		else if(!strcmp(variables[x],"notimebreaks"))
			display_timebreaks=FALSE;

		/* we found the no system messages option */
		else if(!strcmp(variables[x],"nosystem"))
			display_system_messages=FALSE;

		/* we found the no flapping alerts option */
		else if(!strcmp(variables[x],"noflapping"))
			display_flapping_alerts=FALSE;

		/* we found the no downtime alerts option */
		else if(!strcmp(variables[x],"nodowntime"))
			display_downtime_alerts=FALSE;
	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

void show_history(void){
	char image[MAX_INPUT_BUFFER];
	char image_alt[MAX_INPUT_BUFFER];
	char match1[MAX_INPUT_BUFFER];
	char match2[MAX_INPUT_BUFFER];
	char date_time[MAX_DATETIME_LENGTH];
	char *temp_buffer=NULL;
	char *entry_host_name=NULL;
	char *entry_service_desc=NULL;
	char error_text[MAX_INPUT_BUFFER]="";
	char last_message_date[MAX_INPUT_BUFFER]="";
	char current_message_date[MAX_INPUT_BUFFER]="";
	int found_line=FALSE;
	int system_message=FALSE;
	int display_line=FALSE;
	int history_type=SERVICE_HISTORY;
	int history_detail_type=HISTORY_SERVICE_CRITICAL;
	int status=0;
	host *temp_host=NULL;
	service *temp_service=NULL;
	logentry *temp_entry=NULL;
	struct tm *time_ptr=NULL;

	/* read log entries */
	status=get_log_entries(log_file_to_use,NULL,reverse,-1,-1);

	if (status==READLOG_ERROR_MEMORY) {
		printf("<P><DIV CLASS='warningMessage'>Run out of memory..., showing all I could gather!</DIV></P>");
	}

	if (status==READLOG_ERROR_NOFILE) {
		snprintf(error_text,sizeof(error_text),"Error: Could not open log file '%s' for reading!",log_file_to_use);
		error_text[sizeof(error_text)-1]='\x0';
		print_generic_error_message(error_text,NULL,0);
		/* free memory */
		free_log_entries();
		return;

	} else if (status==READLOG_OK) {

		printf("<P><DIV CLASS='logEntries'>\n");

		for(temp_entry=entry_list;temp_entry!=NULL;temp_entry=temp_entry->next) {

			strcpy(image,"");
			strcpy(image_alt,"");
			system_message=FALSE;

			switch(temp_entry->type){

				/* service state alerts */
				case LOGENTRY_SERVICE_CRITICAL:
				case LOGENTRY_SERVICE_WARNING:
				case LOGENTRY_SERVICE_UNKNOWN:
				case LOGENTRY_SERVICE_RECOVERY:
				case LOGENTRY_SERVICE_OK:

					history_type=SERVICE_HISTORY;

					/* get host and service names */
					temp_buffer=my_strtok(temp_entry->entry_text,":");
					temp_buffer=my_strtok(NULL,";");
					if(temp_buffer)
						entry_host_name=strdup(temp_buffer+1);
					else
						entry_host_name=NULL;

					temp_buffer=my_strtok(NULL,";");
					if(temp_buffer)
						entry_service_desc=strdup(temp_buffer);
					else
						entry_service_desc=NULL;

					if(temp_entry->type==LOGENTRY_SERVICE_CRITICAL){
						strcpy(image,CRITICAL_ICON);
						strcpy(image_alt,CRITICAL_ICON_ALT);
						history_detail_type=HISTORY_SERVICE_CRITICAL;
					}
					else if(temp_entry->type==LOGENTRY_SERVICE_WARNING){
						strcpy(image,WARNING_ICON);
						strcpy(image_alt,WARNING_ICON_ALT);
						history_detail_type=HISTORY_SERVICE_WARNING;
					}
					else if(temp_entry->type==LOGENTRY_SERVICE_UNKNOWN){
						strcpy(image,UNKNOWN_ICON);
						strcpy(image_alt,UNKNOWN_ICON_ALT);
						history_detail_type=HISTORY_SERVICE_UNKNOWN;
					}
					else if(temp_entry->type==LOGENTRY_SERVICE_RECOVERY || temp_entry->type==LOGENTRY_SERVICE_OK){
						strcpy(image,OK_ICON);
						strcpy(image_alt,OK_ICON_ALT);
						history_detail_type=HISTORY_SERVICE_RECOVERY;
					}
					break;

				/* service flapping alerts */
				case LOGENTRY_SERVICE_FLAPPING_STARTED:
				case LOGENTRY_SERVICE_FLAPPING_STOPPED:
				case LOGENTRY_SERVICE_FLAPPING_DISABLED:

					if(display_flapping_alerts==FALSE)
						continue;

					history_type=SERVICE_FLAPPING_HISTORY;

					/* get host and service names */
					temp_buffer=my_strtok(temp_entry->entry_text,":");
					temp_buffer=my_strtok(NULL,";");
					if(temp_buffer)
						entry_host_name=strdup(temp_buffer+1);
					else
						entry_host_name=NULL;
					temp_buffer=my_strtok(NULL,";");
					if(temp_buffer)
						entry_service_desc=strdup(temp_buffer);
					else
						entry_service_desc=NULL;

					strcpy(image,FLAPPING_ICON);

					if(temp_entry->type==LOGENTRY_SERVICE_FLAPPING_STARTED)
						strcpy(image_alt,"Service started flapping");
					else if(temp_entry->type==LOGENTRY_SERVICE_FLAPPING_STOPPED)
						strcpy(image_alt,"Service stopped flapping");
					else if(temp_entry->type==LOGENTRY_SERVICE_FLAPPING_DISABLED)
						strcpy(image_alt,"Service flap detection disabled");

					break;

				/* service downtime alerts */
				case LOGENTRY_SERVICE_DOWNTIME_STARTED:
				case LOGENTRY_SERVICE_DOWNTIME_STOPPED:
				case LOGENTRY_SERVICE_DOWNTIME_CANCELLED:

					if(display_downtime_alerts==FALSE)
						continue;

					history_type=SERVICE_DOWNTIME_HISTORY;

					/* get host and service names */
					temp_buffer=my_strtok(temp_entry->entry_text,":");
					temp_buffer=my_strtok(NULL,";");
					if(temp_buffer)
						entry_host_name=strdup(temp_buffer+1);
					else
						entry_host_name=NULL;
					temp_buffer=my_strtok(NULL,";");
					if(temp_buffer)
						entry_service_desc=strdup(temp_buffer);
					else
						entry_service_desc=NULL;

					strcpy(image,SCHEDULED_DOWNTIME_ICON);

					if(temp_entry->type==LOGENTRY_SERVICE_DOWNTIME_STARTED)
						strcpy(image_alt,"Service entered a period of scheduled downtime");
					else if(temp_entry->type==LOGENTRY_SERVICE_DOWNTIME_STOPPED)
						strcpy(image_alt,"Service exited from a period of scheduled downtime");
					else if(temp_entry->type==LOGENTRY_SERVICE_DOWNTIME_CANCELLED)
						strcpy(image_alt,"Service scheduled downtime has been cancelled");

					break;

				/* host state alerts */
				case LOGENTRY_HOST_DOWN:
				case LOGENTRY_HOST_UNREACHABLE:
				case LOGENTRY_HOST_RECOVERY:
				case LOGENTRY_HOST_UP:

					history_type=HOST_HISTORY;

					/* get host name */
					temp_buffer=my_strtok(temp_entry->entry_text,":");
					temp_buffer=my_strtok(NULL,";");
					if(temp_buffer)
						entry_host_name=strdup(temp_buffer+1);
					else
						entry_host_name=NULL;

					if(temp_entry->type==LOGENTRY_HOST_DOWN){
						strcpy(image,HOST_DOWN_ICON);
						strcpy(image_alt,HOST_DOWN_ICON_ALT);
						history_detail_type=HISTORY_HOST_DOWN;
					}
					else if(temp_entry->type==LOGENTRY_HOST_UNREACHABLE){
						strcpy(image,HOST_UNREACHABLE_ICON);
						strcpy(image_alt,HOST_UNREACHABLE_ICON_ALT);
						history_detail_type=HISTORY_HOST_UNREACHABLE;
					}
					else if(temp_entry->type==LOGENTRY_HOST_RECOVERY || temp_entry->type==LOGENTRY_HOST_UP){
						strcpy(image,HOST_UP_ICON);
						strcpy(image_alt,HOST_UP_ICON_ALT);
						history_detail_type=HISTORY_HOST_RECOVERY;
					}

					break;

				/* host flapping alerts */
				case LOGENTRY_HOST_FLAPPING_STARTED:
				case LOGENTRY_HOST_FLAPPING_STOPPED:
				case LOGENTRY_HOST_FLAPPING_DISABLED:

					if(display_flapping_alerts==FALSE)
						continue;

					history_type=HOST_FLAPPING_HISTORY;

					/* get host name */
					temp_buffer=my_strtok(temp_entry->entry_text,":");
					temp_buffer=my_strtok(NULL,";");
					if(temp_buffer)
						entry_host_name=strdup(temp_buffer+1);
					else
						entry_host_name=NULL;

					strcpy(image,FLAPPING_ICON);

					if(temp_entry->type==LOGENTRY_HOST_FLAPPING_STARTED)
						strcpy(image_alt,"Host started flapping");
					else if(temp_entry->type==LOGENTRY_HOST_FLAPPING_STOPPED)
						strcpy(image_alt,"Host stopped flapping");
					else if(temp_entry->type==LOGENTRY_HOST_FLAPPING_DISABLED)
						strcpy(image_alt,"Host flap detection disabled");

					break;

				/* host downtime alerts */
				case LOGENTRY_HOST_DOWNTIME_STARTED:
				case LOGENTRY_HOST_DOWNTIME_STOPPED:
				case LOGENTRY_HOST_DOWNTIME_CANCELLED:

					if(display_downtime_alerts==FALSE)
						continue;

					history_type=HOST_DOWNTIME_HISTORY;

					/* get host name */
					temp_buffer=my_strtok(temp_entry->entry_text,":");
					temp_buffer=my_strtok(NULL,";");
					if(temp_buffer)
						entry_host_name=strdup(temp_buffer+1);
					else
						entry_host_name=NULL;

					strcpy(image,SCHEDULED_DOWNTIME_ICON);

					if(temp_entry->type==LOGENTRY_HOST_DOWNTIME_STARTED)
						strcpy(image_alt,"Host entered a period of scheduled downtime");
					else if(temp_entry->type==LOGENTRY_HOST_DOWNTIME_STOPPED)
						strcpy(image_alt,"Host exited from a period of scheduled downtime");
					else if(temp_entry->type==LOGENTRY_HOST_DOWNTIME_CANCELLED)
						strcpy(image_alt,"Host scheduled downtime has been cancelled");

					break;


				/* program start */
				case LOGENTRY_STARTUP:
					if(display_system_messages==FALSE)
						continue;
					strcpy(image,START_ICON);
					strcpy(image_alt,START_ICON_ALT);
					system_message=TRUE;
					break;

				/* program termination */
				case LOGENTRY_SHUTDOWN:
				case LOGENTRY_BAILOUT:
					if(display_system_messages==FALSE)
						continue;
					strcpy(image,STOP_ICON);
					strcpy(image_alt,STOP_ICON_ALT);
					system_message=TRUE;
					break;

				/* program restart */
				case LOGENTRY_RESTART:
					if(display_system_messages==FALSE)
						continue;
					strcpy(image,RESTART_ICON);
					strcpy(image_alt,RESTART_ICON_ALT);
					system_message=TRUE;
					break;
			}

			image[sizeof(image)-1]='\x0';
			image_alt[sizeof(image_alt)-1]='\x0';

			/* get the timestamp */
			time_ptr=localtime(&temp_entry->timestamp);
			strftime(current_message_date,sizeof(current_message_date),"%B %d, %Y %H:00\n",time_ptr);
			current_message_date[sizeof(current_message_date)-1]='\x0';

			get_time_string(&temp_entry->timestamp,date_time,sizeof(date_time),SHORT_DATE_TIME);
			strip(date_time);

			if(strcmp(image,"")){

				display_line=FALSE;

				if(system_message==TRUE)
					display_line=TRUE;

				else if(display_type==DISPLAY_HOSTS){

					if(history_type==HOST_HISTORY || history_type==SERVICE_HISTORY){
						sprintf(match1," HOST ALERT: %s;",host_name);
						sprintf(match2," SERVICE ALERT: %s;",host_name);
					}
					else if(history_type==HOST_FLAPPING_HISTORY || history_type==SERVICE_FLAPPING_HISTORY){
						sprintf(match1," HOST FLAPPING ALERT: %s;",host_name);
						sprintf(match2," SERVICE FLAPPING ALERT: %s;",host_name);
					}
					else if(history_type==HOST_DOWNTIME_HISTORY || history_type==SERVICE_DOWNTIME_HISTORY){
						sprintf(match1," HOST DOWNTIME ALERT: %s;",host_name);
						sprintf(match2," SERVICE DOWNTIME ALERT: %s;",host_name);
					}

					if(show_all_hosts==TRUE)
						display_line=TRUE;
					else if(strstr(temp_entry->entry_text,match1))
						display_line=TRUE;
					else if(strstr(temp_entry->entry_text,match2))
						display_line=TRUE;

					if(display_line==TRUE){
						if(history_options==HISTORY_ALL)
							display_line=TRUE;
						else if(history_options==HISTORY_HOST_ALL && (history_type==HOST_HISTORY || history_type==HOST_FLAPPING_HISTORY || history_type==HOST_DOWNTIME_HISTORY))
							display_line=TRUE;
						else if(history_options==HISTORY_SERVICE_ALL && (history_type==SERVICE_HISTORY || history_type==SERVICE_FLAPPING_HISTORY || history_type==SERVICE_DOWNTIME_HISTORY))
							display_line=TRUE;
						else if((history_type==HOST_HISTORY || history_type==SERVICE_HISTORY) && (history_detail_type & history_options))
							display_line=TRUE;
						else
							display_line=FALSE;
					}

					/* check alert state types */
					if(display_line==TRUE && (history_type==HOST_HISTORY || history_type==SERVICE_HISTORY)){
						if(state_options==STATE_ALL)
							display_line=TRUE;
						else if((state_options & STATE_SOFT) && strstr(temp_buffer,";SOFT;"))
							display_line=TRUE;
						else if((state_options & STATE_HARD) && strstr(temp_buffer,";HARD;"))
							display_line=TRUE;
						else
							display_line=FALSE;
					}
				}

				else if(display_type==DISPLAY_SERVICES){

					if(history_type==SERVICE_HISTORY)
						sprintf(match1," SERVICE ALERT: %s;%s;",host_name,service_desc);
					else if(history_type==SERVICE_FLAPPING_HISTORY)
						sprintf(match1," SERVICE FLAPPING ALERT: %s;%s;",host_name,service_desc);
					else if(history_type==SERVICE_DOWNTIME_HISTORY)
						sprintf(match1," SERVICE DOWNTIME ALERT: %s;%s;",host_name,service_desc);

					if(strstr(temp_entry->entry_text,match1) && (history_type==SERVICE_HISTORY || history_type==SERVICE_FLAPPING_HISTORY || history_type==SERVICE_DOWNTIME_HISTORY))
						display_line=TRUE;

					if(display_line==TRUE){
						if(history_options==HISTORY_ALL || history_options==HISTORY_SERVICE_ALL)
							display_line=TRUE;
						else if(history_options & history_detail_type)
							display_line=TRUE;
						else
							display_line=FALSE;
					}

					/* check alert state type */
					if(display_line==TRUE && history_type==SERVICE_HISTORY){

						if(state_options==STATE_ALL)
							display_line=TRUE;
						else if((state_options & STATE_SOFT) && strstr(temp_buffer,";SOFT;"))
							display_line=TRUE;
						else if((state_options & STATE_HARD) && strstr(temp_buffer,";HARD;"))
							display_line=TRUE;
						else
							display_line=FALSE;
					}
				}

				/* make sure user is authorized to view this host or service information */
				if(system_message==FALSE){

					if(history_type==HOST_HISTORY || history_type==HOST_FLAPPING_HISTORY || history_type==HOST_DOWNTIME_HISTORY){
						temp_host=find_host(entry_host_name);
						if(is_authorized_for_host(temp_host,&current_authdata)==FALSE)
							display_line=FALSE;
					}else{
						temp_service=find_service(entry_host_name,entry_service_desc);
						if(is_authorized_for_service(temp_service,&current_authdata)==FALSE)
							display_line=FALSE;
					}
				}

				/* display the entry if we should... */
				if(display_line==TRUE){

					if(strcmp(last_message_date,current_message_date)!=0 && display_timebreaks==TRUE){
						printf("</DIV><BR CLEAR='all' />\n");
						printf("<DIV CLASS='dateTimeBreak'>\n");
						printf("<table border=0 width=95%%><tr>");
						printf("<td width=40%%><hr width=100%%></td>");
						printf("<td align=center CLASS='dateTimeBreak'>%s</td>",current_message_date);
						printf("<td width=40%%><hr width=100%%></td>");
						printf("</tr></table>\n");
						printf("</DIV>\n");
						printf("<BR CLEAR='all' /><DIV CLASS='logEntries'>\n");
						strncpy(last_message_date,current_message_date,sizeof(last_message_date));
						last_message_date[sizeof(last_message_date)-1]='\x0';
					}

					if(display_frills==TRUE)
						printf("<img align='left' src='%s%s' alt='%s' title='%s' />",url_images_path,image,image_alt,image_alt);
					printf("[%s] %s",date_time,html_encode(temp_entry->entry_text,FALSE));
					if(enable_splunk_integration==TRUE){
						printf("&nbsp;&nbsp;&nbsp;");
						display_splunk_generic_url(temp_entry->entry_text,2);
					}
					printf("<br clear='all' />\n");

					found_line=TRUE;
				}
			}

			/* free memory */
			free(entry_host_name);
			entry_host_name=NULL;
			free(entry_service_desc);
			entry_service_desc=NULL;
		}
	}

	free_log_entries();

	printf("</DIV></P>\n");

	if(found_line==FALSE){
		printf("<HR>\n");
		printf("<P><DIV CLASS='errorMessage' style='text-align:center'>No history information was found ");
		if(display_type==DISPLAY_HOSTS)
			printf("%s",(show_all_hosts==TRUE)?"":"for this host ");
		else
			printf("for this service ");
		printf("in %s log file</DIV></P>",(log_archive==0)?"the current":"this archived");
	}

	printf("<HR>\n");

	return;
}
