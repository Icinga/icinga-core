/***********************************************************************
 *
 * CGIUTILS.C - Common utilities for Icinga CGIs
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
 ***********************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/locations.h"
#include "../include/objects.h"
#include "../include/statusdata.h"

#include "../include/cgiutils.h"

char            main_config_file[MAX_FILENAME_LENGTH];
char            log_file[MAX_FILENAME_LENGTH];
char            log_archive_path[MAX_FILENAME_LENGTH];
char            command_file[MAX_FILENAME_LENGTH];

char            physical_html_path[MAX_FILENAME_LENGTH];
char            physical_images_path[MAX_FILENAME_LENGTH];
char            physical_ssi_path[MAX_FILENAME_LENGTH];
char            url_html_path[MAX_FILENAME_LENGTH];
char            url_docs_path[MAX_FILENAME_LENGTH];
char            url_context_help_path[MAX_FILENAME_LENGTH];
char            url_images_path[MAX_FILENAME_LENGTH];
char            url_logo_images_path[MAX_FILENAME_LENGTH];
char            url_stylesheets_path[MAX_FILENAME_LENGTH];
char            url_js_path[MAX_FILENAME_LENGTH];
char            url_media_path[MAX_FILENAME_LENGTH];

char            *service_critical_sound=NULL;
char            *service_warning_sound=NULL;
char            *service_unknown_sound=NULL;
char            *host_down_sound=NULL;
char            *host_unreachable_sound=NULL;
char            *normal_sound=NULL;
char            *statusmap_background_image=NULL;
char            *statuswrl_include=NULL;

char            *illegal_output_chars=NULL;

char            *http_charset=NULL;

char            *notes_url_target=NULL;
char            *action_url_target=NULL;

char            *ping_syntax=NULL;

char		*csv_delimiter=CSV_DELIMITER;
char		*csv_data_enclosure=CSV_DATA_ENCLOSURE;

char            nagios_check_command[MAX_INPUT_BUFFER]="";
char            nagios_process_info[MAX_INPUT_BUFFER]="";
int             nagios_process_state=STATE_OK;

int             enable_splunk_integration=FALSE;
char            *splunk_url=NULL;
int             lock_author_names=TRUE;

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

int             check_external_commands=0;

int             log_external_commands_user=FALSE;

int             date_format=DATE_FORMAT_US;

int             log_rotation_method=LOG_ROTATION_NONE;

time_t          this_scheduled_log_rotation=0L;
time_t          last_scheduled_log_rotation=0L;
time_t          next_scheduled_log_rotation=0L;

int             use_authentication=TRUE;

int             interval_length=60;

int             show_context_help=FALSE;
int		show_all_services_host_is_authorized_for=TRUE;

int             use_pending_states=TRUE;

int             host_status_has_been_read=FALSE;
int             service_status_has_been_read=FALSE;
int             program_status_has_been_read=FALSE;

int             refresh_rate=DEFAULT_REFRESH_RATE;

int             escape_html_tags=FALSE;

int             persistent_ack_comments=FALSE;

int             use_ssl_authentication=FALSE;

int             default_statusmap_layout_method=0;
int             default_statuswrl_layout_method=0;

int		color_transparency_index_r=255;
int		color_transparency_index_g=255;
int		color_transparency_index_b=255;

int		status_show_long_plugin_output=FALSE;
int		tac_show_only_hard_state=FALSE;
int		showlog_initial_states=TRUE;
int		showlog_current_states=TRUE;
int		tab_friendly_titles=FALSE;
int		add_notif_num_hard=0;
int		add_notif_num_soft=0;

extern hostgroup       *hostgroup_list;
extern contactgroup    *contactgroup_list;
extern command         *command_list;
extern timeperiod      *timeperiod_list;
extern contact         *contact_list;
extern serviceescalation *serviceescalation_list;

extern hoststatus      *hoststatus_list;
extern servicestatus   *servicestatus_list;

lifo            *lifo_list=NULL;

char encoded_url_string[2][MAX_INPUT_BUFFER]; // 2 to be able use url_encode twice
char *encoded_html_string=NULL;

#ifdef HAVE_TZNAME
#ifdef CYGWIN
extern char     *_tzname[2] __declspec(dllimport);
#else
extern char     *tzname[2];
#endif
#endif

int content_type=HTML_CONTENT;
int embedded=FALSE;
int display_header=TRUE;
int refresh=TRUE;
int daemon_check=TRUE;

extern char alert_message;
extern char *host_name;
extern char *host_filter;
extern char *hostgroup_name;
extern char *service_desc;
extern char *servicegroup_name;
extern char *service_filter;
extern int host_alert;
extern int show_all_hosts;
extern int show_all_hostgroups;
extern int show_all_servicegroups;
extern int display_type;
extern int overview_columns;
extern int max_grid_width;
extern int group_style_type;
extern int navbar_search;


/*
 * These function stubs allow us to compile a lot of the
 * source-files common to cgi's and daemon without adding
 * a whole bunch of #ifdef's everywhere. Note that we can't
 * have them as macros, since the goal is to compile the
 * source-files once. A decent linker will make the call
 * a no-op anyway, so it's not a big issue
 */
void logit(int data_type, int display, const char *fmt, ...) { return; }
int log_debug_info(int leve, int verbosity, const char *fmt, ...) { return 0; }

/**********************************************************
 ***************** CLEANUP FUNCTIONS **********************
 **********************************************************/

/* reset all variables used by the CGIs */
void reset_cgi_vars(void){

	strcpy(main_config_file,"");

	strcpy(physical_html_path,"");
	strcpy(physical_images_path,"");
	strcpy(physical_ssi_path,"");

	strcpy(url_html_path,"");
	strcpy(url_docs_path,"");
	strcpy(url_context_help_path,"");
	strcpy(url_stylesheets_path,"");
	strcpy(url_js_path,"");
	strcpy(url_media_path,"");
	strcpy(url_images_path,"");

	strcpy(log_file,"");
	strcpy(log_archive_path,DEFAULT_LOG_ARCHIVE_PATH);
	if(log_archive_path[strlen(log_archive_path)-1]!='/' && strlen(log_archive_path)<sizeof(log_archive_path)-2)
		strcat(log_archive_path,"/");
	strcpy(command_file,get_cmd_file_location());

	strcpy(nagios_check_command,"");
	strcpy(nagios_process_info,"");
	nagios_process_state=STATE_OK;

	log_rotation_method=LOG_ROTATION_NONE;

	use_authentication=TRUE;

	interval_length=60;

	refresh_rate=DEFAULT_REFRESH_RATE;

	default_statusmap_layout_method=0;
	default_statusmap_layout_method=0;

	service_critical_sound=NULL;
	service_warning_sound=NULL;
	service_unknown_sound=NULL;
	host_down_sound=NULL;
	host_unreachable_sound=NULL;
	normal_sound=NULL;

	my_free(http_charset);
	http_charset = strdup(DEFAULT_HTTP_CHARSET);

	statusmap_background_image=NULL;
	color_transparency_index_r=255;
	color_transparency_index_g=255;
	color_transparency_index_b=255;
	statuswrl_include=NULL;

	ping_syntax=NULL;

	return;
        }



/* free all memory for object definitions */
void free_memory(void){

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
	free(statuswrl_include);
	free(ping_syntax);

	return;
        }




/**********************************************************
 *************** CONFIG FILE FUNCTIONS ********************
 **********************************************************/

/* read the CGI config file location from an environment variable */
char * get_cgi_config_location(void){
        static char *cgiloc=NULL;

        if(!cgiloc){
                cgiloc=getenv("ICINGA_CGI_CONFIG");
        	if(!cgiloc){
			/* stay compatible */
                	cgiloc=getenv("NAGIOS_CGI_CONFIG");
                	if(!cgiloc){
                        	cgiloc=DEFAULT_CGI_CONFIG_FILE;
			}
		}
	}

        return cgiloc;
}


/* read the command file location from an environment variable */
char * get_cmd_file_location(void){
        static char *cmdloc=NULL;

        if(!cmdloc){
                cmdloc=getenv("ICINGA_COMMAND_FILE");
        	if(!cmdloc){
			/* stay compatible */
                	cmdloc=getenv("NAGIOS_COMMAND_FILE");
	                if(!cmdloc){
        	                cmdloc=DEFAULT_COMMAND_FILE;
			}
                }
	}
        return cmdloc;
}

/*read the CGI configuration file */
int read_cgi_config_file(char *filename){
	char *input=NULL;
	mmapfile *thefile;
	char *var=NULL;
	char *val=NULL;


	if((thefile=mmap_fopen(filename))==NULL)
		return ERROR;

	while(1){

		/* free memory */
		free(input);

		/* read the next line */
		if((input=mmap_fgets_multiline(thefile))==NULL)
			break;

		strip(input);

		var=strtok(input,"=");
		val=strtok(NULL,"\n");

		if(var==NULL || val==NULL)
			continue;

		if(!strcmp(var,"main_config_file")){
			strncpy(main_config_file,val,sizeof(main_config_file));
			main_config_file[sizeof(main_config_file)-1]='\x0';
			strip(main_config_file);
		        }

		else if(!strcmp(var,"show_context_help"))
			show_context_help=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"show_all_services_host_is_authorized_for"))
			show_all_services_host_is_authorized_for=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"use_pending_states"))
			use_pending_states=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"use_authentication"))
			use_authentication=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"nagios_check_command")){
			strncpy(nagios_check_command,val,sizeof(nagios_check_command));
			nagios_check_command[sizeof(nagios_check_command)-1]='\x0';
			strip(nagios_check_command);
		        }

		else if(!strcmp(var,"refresh_rate"))
			refresh_rate=atoi(val);

		else if(!strcmp(var,"physical_html_path")){
			strncpy(physical_html_path,val,sizeof(physical_html_path));
			physical_html_path[sizeof(physical_html_path)-1]='\x0';
			strip(physical_html_path);
			if(physical_html_path[strlen(physical_html_path)-1]!='/' && (strlen(physical_html_path) < sizeof(physical_html_path)-1))
				strcat(physical_html_path,"/");

			snprintf(physical_images_path,sizeof(physical_images_path),"%simages/",physical_html_path);
			physical_images_path[sizeof(physical_images_path)-1]='\x0';

			snprintf(physical_ssi_path,sizeof(physical_images_path),"%sssi/",physical_html_path);
			physical_ssi_path[sizeof(physical_ssi_path)-1]='\x0';
		        }

		else if(!strcmp(var,"url_html_path")){

			strncpy(url_html_path,val,sizeof(url_html_path));
			url_html_path[sizeof(url_html_path)-1]='\x0';

			strip(url_html_path);
			if(url_html_path[strlen(url_html_path)-1]!='/' && (strlen(url_html_path) < sizeof(url_html_path)-1))
				strcat(url_html_path,"/");

			snprintf(url_docs_path,sizeof(url_docs_path),"%sdocs/",url_html_path);
			url_docs_path[sizeof(url_docs_path)-1]='\x0';

			snprintf(url_context_help_path,sizeof(url_context_help_path),"%scontexthelp/",url_html_path);
			url_context_help_path[sizeof(url_context_help_path)-1]='\x0';

			snprintf(url_images_path,sizeof(url_images_path),"%simages/",url_html_path);
			url_images_path[sizeof(url_images_path)-1]='\x0';

			snprintf(url_logo_images_path,sizeof(url_logo_images_path),"%slogos/",url_images_path);
			url_logo_images_path[sizeof(url_logo_images_path)-1]='\x0';

			/*
			snprintf(url_stylesheets_path,sizeof(url_stylesheets_path),"%sstylesheets/",url_html_path);
			url_stylesheets_path[sizeof(url_stylesheets_path)-1]='\x0';
			*/

			snprintf(url_js_path,sizeof(url_js_path),"%sjs/",url_html_path);
			url_js_path[sizeof(url_js_path)-1]='\x0';

			snprintf(url_media_path,sizeof(url_media_path),"%smedia/",url_html_path);
			url_media_path[sizeof(url_media_path)-1]='\x0';
		        }

		else if(!strcmp(var,"url_stylesheets_path")){

                        strncpy(url_stylesheets_path,val,sizeof(url_stylesheets_path));
                        url_stylesheets_path[sizeof(url_stylesheets_path)-1]='\x0';

                        strip(url_stylesheets_path);
                        if(url_stylesheets_path[strlen(url_stylesheets_path)-1]!='/' && (strlen(url_stylesheets_path) < sizeof(url_stylesheets_path)-1))
                                strcat(url_stylesheets_path,"/");

			}
		else if(!strcmp(var,"service_critical_sound"))
			service_critical_sound=strdup(val);

		else if(!strcmp(var,"service_warning_sound"))
			service_warning_sound=strdup(val);

		else if(!strcmp(var,"service_unknown_sound"))
			service_unknown_sound=strdup(val);

		else if(!strcmp(var,"host_down_sound"))
			host_down_sound=strdup(val);

		else if(!strcmp(var,"host_unreachable_sound"))
			host_unreachable_sound=strdup(val);

		else if(!strcmp(var,"normal_sound"))
			normal_sound=strdup(val);

		else if(!strcmp(var,"statusmap_background_image"))
			statusmap_background_image=strdup(val);

		else if(!strcmp(var,"color_transparency_index_r"))
			color_transparency_index_r=atoi(val);

		else if(!strcmp(var,"color_transparency_index_g"))
			color_transparency_index_g=atoi(val);

		else if(!strcmp(var,"color_transparency_index_b"))
			color_transparency_index_b=atoi(val);

		else if(!strcmp(var,"default_statusmap_layout"))
			default_statusmap_layout_method=atoi(val);

		else if(!strcmp(var,"default_statuswrl_layout"))
			default_statuswrl_layout_method=atoi(val);

		else if(!strcmp(var,"statuswrl_include"))
			statuswrl_include=strdup(val);

		else if(!strcmp(var,"ping_syntax"))
			ping_syntax=strdup(val);

		else if(!strcmp(var,"action_url_target"))
			action_url_target=strdup(val);

		else if(!strcmp(var,"illegal_macro_output_chars"))
			illegal_output_chars=strdup(val);

		else if(!strcmp(var,"http_charset"))
			http_charset=strdup(val);

		else if(!strcmp(var,"notes_url_target"))
			notes_url_target=strdup(val);

		else if(!strcmp(var,"enable_splunk_integration"))
			enable_splunk_integration=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"splunk_url"))
			splunk_url=strdup(val);

		else if(!strcmp(var,"escape_html_tags"))
			escape_html_tags=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"persistent_ack_comments"))
			persistent_ack_comments=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"lock_author_names"))
			lock_author_names=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"use_ssl_authentication"))
			use_ssl_authentication=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"status_show_long_plugin_output"))
			status_show_long_plugin_output=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"tac_show_only_hard_state"))
			tac_show_only_hard_state=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"showlog_initial_state") || !strcmp(var,"showlog_initial_states"))
			showlog_initial_states=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"showlog_current_state") || !strcmp(var,"showlog_current_states"))
			showlog_current_states=(atoi(val)>0)?TRUE:FALSE;

		else if(!strcmp(var,"tab_friendly_titles"))
			tab_friendly_titles=(atoi(val)>0)?TRUE:FALSE;

 		else if(!strcmp(var,"add_notif_num_hard"))
			add_notif_num_hard=atoi(val);

		else if(!strcmp(var,"add_notif_num_soft"))
			add_notif_num_soft=atoi(val);

		else if(!strcmp(var,"csv_delimiter"))
			csv_delimiter=strdup(val);

		else if(!strcmp(var,"csv_data_enclosure"))
			csv_data_enclosure=strdup(val);

		}

	/* free memory and close the file */
	free(input);
	mmap_fclose(thefile);

	/* check if stylesheet path was set */
	if(!strcmp(url_stylesheets_path,"")){
		snprintf(url_stylesheets_path,sizeof(url_stylesheets_path),"%sstylesheets/",url_html_path);
		url_stylesheets_path[sizeof(url_stylesheets_path)-1]='\x0';
	}

	if(!strcmp(main_config_file,""))
		return ERROR;
	else
		return OK;
}




/* read the main configuration file */
int read_main_config_file(char *filename){
	char *input=NULL;
	char *temp_buffer;
	mmapfile *thefile;


	if((thefile=mmap_fopen(filename))==NULL)
		return ERROR;

	while(1){

		/* free memory */
		free(input);

		/* read the next line */
		if((input=mmap_fgets_multiline(thefile))==NULL)
			break;

		strip(input);

		if(strstr(input,"interval_length=")==input){
			temp_buffer=strtok(input,"=");
			temp_buffer=strtok(NULL,"\x0");
			interval_length=(temp_buffer==NULL)?60:atoi(temp_buffer);
		        }

		else if(strstr(input,"log_file=")==input){
			temp_buffer=strtok(input,"=");
			temp_buffer=strtok(NULL,"\x0");
			strncpy(log_file,(temp_buffer==NULL)?"":temp_buffer,sizeof(log_file));
			log_file[sizeof(log_file)-1]='\x0';
			strip(log_file);
		        }

		else if(strstr(input,"log_archive_path=")==input){
			temp_buffer=strtok(input,"=");
			temp_buffer=strtok(NULL,"\n");
			strncpy(log_archive_path,(temp_buffer==NULL)?"":temp_buffer,sizeof(log_archive_path));
			log_archive_path[sizeof(log_archive_path)-1]='\x0';
			strip(physical_html_path);
			if(log_archive_path[strlen(log_archive_path)-1]!='/' && (strlen(log_archive_path) < sizeof(log_archive_path)-1))
				strcat(log_archive_path,"/");
		        }

		else if(strstr(input,"log_rotation_method=")==input){
			temp_buffer=strtok(input,"=");
			temp_buffer=strtok(NULL,"\x0");
			if(temp_buffer==NULL)
				log_rotation_method=LOG_ROTATION_NONE;
			else if(!strcmp(temp_buffer,"h"))
				log_rotation_method=LOG_ROTATION_HOURLY;
			else if(!strcmp(temp_buffer,"d"))
				log_rotation_method=LOG_ROTATION_DAILY;
			else if(!strcmp(temp_buffer,"w"))
				log_rotation_method=LOG_ROTATION_WEEKLY;
			else if(!strcmp(temp_buffer,"m"))
				log_rotation_method=LOG_ROTATION_MONTHLY;
		        }

		else if(strstr(input,"command_file=")==input){
			temp_buffer=strtok(input,"=");
			temp_buffer=strtok(NULL,"\x0");
			strncpy(command_file,(temp_buffer==NULL)?"":temp_buffer,sizeof(command_file));
			command_file[sizeof(command_file)-1]='\x0';
			strip(command_file);
		        }

		else if(strstr(input,"check_external_commands=")==input){
			temp_buffer=strtok(input,"=");
			temp_buffer=strtok(NULL,"\x0");
			check_external_commands=(temp_buffer==NULL)?0:atoi(temp_buffer);
		        }

                else if(strstr(input,"log_external_commands_user=")==input){
                        temp_buffer=strtok(input,"=");
                        temp_buffer=strtok(NULL,"\x0");
                        log_external_commands_user=(temp_buffer==NULL)?0:atoi(temp_buffer);
                        }

		else if(strstr(input,"date_format=")==input){
			temp_buffer=strtok(input,"=");
			temp_buffer=strtok(NULL,"\x0");
			if(temp_buffer==NULL)
				date_format=DATE_FORMAT_US;
			else if(!strcmp(temp_buffer,"euro"))
				date_format=DATE_FORMAT_EURO;
			else if(!strcmp(temp_buffer,"iso8601"))
				date_format=DATE_FORMAT_ISO8601;
			else if(!strcmp(temp_buffer,"strict-iso8601"))
				date_format=DATE_FORMAT_STRICT_ISO8601;
			else
				date_format=DATE_FORMAT_US;
		        }
               }

	/* free memory and close the file */
	free(input);
	mmap_fclose(thefile);

	return OK;
        }



/* read all object definitions */
int read_all_object_configuration_data(char *config_file,int options){
	int result=OK;

	/* read in all external config data of the desired type(s) */
	result=read_object_config_data(config_file,options,FALSE,FALSE);

	return result;
        }


/* read all status data */
int read_all_status_data(char *config_file,int options){
	int result=OK;

	/* don't duplicate things we've already read in */
	if(program_status_has_been_read==TRUE && (options & READ_PROGRAM_STATUS))
		options-=READ_PROGRAM_STATUS;
	if(host_status_has_been_read==TRUE && (options & READ_HOST_STATUS))
		options-=READ_HOST_STATUS;
	if(service_status_has_been_read==TRUE && (options & READ_SERVICE_STATUS))
		options-=READ_SERVICE_STATUS;

	/* bail out if we've already read what we need */
	if(options<=0)
		return OK;

	/* read in all external status data */
	result=read_status_data(config_file,options);

	/* mark what items we've read in... */
	if(options & READ_PROGRAM_STATUS)
		program_status_has_been_read=TRUE;
	if(options & READ_HOST_STATUS)
		host_status_has_been_read=TRUE;
	if(options & READ_SERVICE_STATUS)
		service_status_has_been_read=TRUE;

	return result;
        }


/**********************************************************
 ******************* LIFO FUNCTIONS ***********************
 **********************************************************/

/* reads contents of file into the lifo struct */
int read_file_into_lifo(char *filename){
	char *input=NULL;
	mmapfile *thefile;
	int lifo_result;

	if((thefile=mmap_fopen(filename))==NULL)
		return LIFO_ERROR_FILE;

	while(1){

		free(input);

		if((input=mmap_fgets(thefile))==NULL)
			break;

		lifo_result=push_lifo(input);

		if(lifo_result!=LIFO_OK){
			free_lifo_memory();
			free(input);
			mmap_fclose(thefile);
			return lifo_result;
		        }
	        }

	mmap_fclose(thefile);

	return LIFO_OK;
        }


/* frees all memory allocated to lifo */
void free_lifo_memory(void){
	lifo *temp_lifo;
	lifo *next_lifo;

	if(lifo_list==NULL)
		return;

	temp_lifo=lifo_list;
	while(temp_lifo!=NULL){
		next_lifo=temp_lifo->next;
		if(temp_lifo->data!=NULL)
			free((void *)temp_lifo->data);
		free((void *)temp_lifo);
		temp_lifo=next_lifo;
	        }

	return;
        }


/* adds an item to lifo */
int push_lifo(char *buffer){
	lifo *temp_lifo;

	temp_lifo=(lifo *)malloc(sizeof(lifo));
	if(temp_lifo==NULL)
		return LIFO_ERROR_MEMORY;

	if(buffer==NULL)
		temp_lifo->data=(char *)strdup("");
	else
		temp_lifo->data=(char *)strdup(buffer);
	if(temp_lifo->data==NULL){
		free(temp_lifo);
		return LIFO_ERROR_MEMORY;
	        }

	/* add item to front of lifo... */
	temp_lifo->next=lifo_list;
	lifo_list=temp_lifo;

	return LIFO_OK;
        }



/* returns/clears an item from lifo */
char *pop_lifo(void){
	lifo *temp_lifo;
	char *buf;

	if(lifo_list==NULL || lifo_list->data==NULL)
		return NULL;

	buf=strdup(lifo_list->data);

	temp_lifo=lifo_list->next;

	if(lifo_list->data!=NULL)
		free((void *)lifo_list->data);
	free((void *)lifo_list);

	lifo_list=temp_lifo;

	return buf;
        }

/**********************************************************
 *************** COMMON HEADER AND FOOTER *****************
 **********************************************************/

void document_header(int cgi_id, int use_stylesheet){
	char date_time[MAX_DATETIME_LENGTH];
	char *cgi_name=NULL;
	char *cgi_css=NULL;
	char *cgi_title=NULL;
	char *cgi_body_class=NULL;
	time_t expire_time;
	time_t current_time;

	switch(cgi_id) {
		case STATUS_CGI_ID:
			cgi_name        = STATUS_CGI;
			cgi_css         = STATUS_CSS;
			cgi_title       = "Current Network Status";
			cgi_body_class  = "status";
			break;
		case AVAIL_CGI_ID:
			cgi_name        = AVAIL_CGI;
			cgi_css         = AVAIL_CSS;
			cgi_title       = "Availability";
			cgi_body_class  = "avail";
			break;
		case CMD_CGI_ID:
			cgi_name        = CMD_CGI;
			cgi_css         = CMD_CSS;
			cgi_title       = "External Command Interface";
			cgi_body_class  = "cmd";
			refresh         = FALSE;
			break;
                case CONFIG_CGI_ID:
                        cgi_name        = CONFIG_CGI;
                        cgi_css         = CONFIG_CSS;
                        cgi_title       = "Configuration";
                        cgi_body_class  = "config";
                        break;
                case EXTINFO_CGI_ID:
                        cgi_name        = EXTINFO_CGI;
                        cgi_css         = EXTINFO_CSS;
                        cgi_title       = "Extended Information";
                        cgi_body_class  = "extinfo";
                        break;
                case HISTOGRAM_CGI_ID:
                        cgi_name        = HISTOGRAM_CGI;
                        cgi_css         = HISTOGRAM_CSS;
                        cgi_title       = "Histogram";
                        cgi_body_class  = "histogram";
                        break;
                case HISTORY_CGI_ID:
                        cgi_name        = HISTORY_CGI;
                        cgi_css         = HISTORY_CSS;
                        cgi_title       = "History";
                        cgi_body_class  = "history";
                        break;
                case NOTIFICATIONS_CGI_ID:
                        cgi_name        = NOTIFICATIONS_CGI;
                        cgi_css         = NOTIFICATIONS_CSS;
                        cgi_title       = "Alert Notifications";
                        cgi_body_class  = "notifications";
                        break;
                case OUTAGES_CGI_ID:
                        cgi_name        = OUTAGES_CGI;
                        cgi_css         = OUTAGES_CSS;
                        cgi_title       = "Network Outages";
                        cgi_body_class  = "outages";
                        break;
                case SHOWLOG_CGI_ID:
                        cgi_name        = SHOWLOG_CGI;
                        cgi_css         = SHOWLOG_CSS;
                        cgi_title       = "Log File";
                        cgi_body_class  = "showlog";
                        break;
                case STATUSMAP_CGI_ID:
                        cgi_name        = STATUSMAP_CGI;
                        cgi_css         = STATUSMAP_CSS;
                        cgi_title       = "Network Map";
                        cgi_body_class  = "statusmap";
                        break;
                case SUMMARY_CGI_ID:
                        cgi_name        = SUMMARY_CGI;
                        cgi_css         = SUMMARY_CSS;
                        cgi_title       = "Event Summary";
                        cgi_body_class  = "summary";
                        break;
                case TAC_CGI_ID:
                        cgi_name        = TAC_CGI;
                        cgi_css         = TAC_CSS;
                        cgi_title       = "Tactical Monitoring Overview";
                        cgi_body_class  = "tac";
                        break;
                case TRENDS_CGI_ID:
                        cgi_name        = TRENDS_CGI;
                        cgi_css         = TRENDS_CSS;
                        cgi_title       = "Trends";
                        cgi_body_class  = "trends";
                        break;
                case ERROR_CGI_ID:
                        cgi_name        = "";
                        cgi_css         = CMD_CSS;
                        cgi_title       = "ERROR";
                        cgi_body_class  = "error";
                        break;
        }

	if(content_type==WML_CONTENT){
                /* used by cmd.cgi */
		printf("Content-type: text/vnd.wap.wml; charset=\"%s\"\r\n\r\n", http_charset);

		printf("<?xml version=\"1.0\"?>\n");
		printf("<!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\">\n");

		printf("<wml>\n");

		printf("<card id='card1' title='Command Results'>\n");

		return;
	}

	if(cgi_id!=ERROR_CGI_ID){
		printf("Cache-Control: no-store\r\n");
		printf("Pragma: no-cache\r\n");

		if(refresh)
			printf("Refresh: %d\r\n",refresh_rate);

		get_time_string(&current_time,date_time,(int)sizeof(date_time),HTTP_DATE_TIME);
		printf("Last-Modified: %s\r\n",date_time);

		expire_time=(time_t)0L;
		get_time_string(&expire_time,date_time,(int)sizeof(date_time),HTTP_DATE_TIME);
		printf("Expires: %s\r\n",date_time);
	}

	if(cgi_id==STATUSWRL_CGI_ID) {
		printf("Content-Type: x-world/x-vrml\r\n\r\n");
		return;
	}
	if(cgi_id==STATUSWML_CGI_ID) {

		printf("Content-type: text/vnd.wap.wml; charset=\"%s\"\r\n\r\n", http_charset);

		printf("<?xml version=\"1.0\"?>\n");
		printf("<!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\">\n");

		printf("<wml>\n");

		printf("<head>\n");
		printf("<meta forua=\"true\" http-equiv=\"Cache-Control\" content=\"max-age=0\"/>\n");
		printf("</head>\n");

		return;
	}
	if(content_type==IMAGE_CONTENT) {
		printf("Content-Type: image/png\r\n\r\n");
		return;
	}

	if(content_type==CSV_CONTENT) {
		printf("Content-type: text/plain; charset=\"%s\"\r\n\r\n", http_charset);
		return;
	}

	if(cgi_id!=ERROR_CGI_ID){
		// send HTML CONTENT
		printf("Content-type: text/html; charset=\"%s\"\r\n\r\n", http_charset);
	}

	if(embedded==TRUE)
		return;

	printf("<html>\n");
	printf("<head>\n");
	printf("<link rel=\"shortcut icon\" href=\"%sfavicon.ico\" type=\"image/ico\">\n",url_images_path);
	printf("<META HTTP-EQUIV='Pragma' CONTENT='no-cache'>\n");
	printf("<title>\n");

	if(cgi_id == STATUS_CGI_ID){
		if (tab_friendly_titles){
			if ((display_type==DISPLAY_HOSTS)&&(!show_all_hosts)&&host_name&&(*host_name!='\0'))
				printf ("[%s]\n",html_encode(host_name,FALSE));
			else if ((display_type==DISPLAY_HOSTGROUPS)&&(!show_all_hostgroups)&&hostgroup_name &&(*hostgroup_name!='\0'))
				printf ("{%s}\n",html_encode(hostgroup_name,FALSE));
			else if ((display_type==DISPLAY_SERVICEGROUPS)&&(!show_all_servicegroups)&&servicegroup_name &&(*servicegroup_name!='\0'))
				printf ("(%s)\n",html_encode(servicegroup_name,FALSE));
			else
				printf("%s\n",cgi_title);
		}
		else printf("%s\n",cgi_title);
	} else if(cgi_id == EXTINFO_CGI_ID){
		if (tab_friendly_titles){
			if ((display_type==DISPLAY_HOST_INFO)&&host_name&&(*host_name!='\0'))
				printf ("[%s]\n",html_encode(host_name,FALSE));
			else if ((display_type==DISPLAY_SERVICE_INFO)&&service_desc&&(*service_desc!='\0')){
				printf ("%s\n",service_desc);
				if (host_name&&(*host_name!='\0'))
					printf ("@ %s\n",html_encode(host_name,FALSE));
			}
			else if ((display_type==DISPLAY_HOSTGROUP_INFO)&&hostgroup_name&&(*hostgroup_name!='\0'))
				printf ("{%s}\n",html_encode(hostgroup_name,FALSE));
			else if ((display_type==DISPLAY_SERVICEGROUP_INFO)&&servicegroup_name &&(*servicegroup_name!='\0'))
				printf ("(%s)\n",html_encode(servicegroup_name,FALSE));
			else
				printf("%s\n",cgi_title);
		}
		else printf("%s\n",cgi_title);
	} else {
		printf("%s\n",cgi_title);
	}
	printf("</title>\n");

	if(use_stylesheet){
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n",url_stylesheets_path,COMMON_CSS);
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n",url_stylesheets_path,cgi_css);
	}

	if(cgi_id == STATUS_CGI_ID || cgi_id == EXTINFO_CGI_ID) {
		/* JavaScript for dropdown menu WITH images */
		printf("<script type='text/javascript' src='%s%s'></script>\n",url_js_path,JQUERY_MAIN_JS);
		printf("<script type='text/javascript' src='%s%s'></script>\n",url_js_path,JQUERY_DD_JS);

		/* This CSS IS needed for proper dropdown menu's (bypass the use_stylesheets above, who does without anyway?) */
		printf("<link rel='stylesheet' type='text/css' href='%s%s'/>\n",url_stylesheets_path,JQUERY_DD_CSS);

		/* functions to handle the checkboxes and dropdown menus */
		printf("<script type='text/javascript' src='%s%s'></script>\n",url_js_path,CHECKBOX_FUNCTIONS_JS);
	}

	if(cgi_id==STATUSMAP_CGI_ID || cgi_id==TRENDS_CGI_ID) {
		/* write JavaScript code for popup window */
		write_popup_code(cgi_id);
	}

	printf("</head>\n");

	if(cgi_id==STATUSMAP_CGI_ID)
		printf("<body CLASS='%s' name='mappage' id='mappage'>\n",cgi_body_class);
	else if(cgi_id==TAC_CGI_ID)
		printf("<body CLASS='%s' marginwidth=2 marginheight=2 topmargin=0 leftmargin=0 rightmargin=0>\n",cgi_body_class);
	else
		printf("<body CLASS='%s'>\n",cgi_body_class);

	/* include user SSI header */
	include_ssi_files(cgi_name,SSI_HEADER);

        /* this line was also in histogram.c, is this necessary??? */
	if(cgi_id==HISTOGRAM_CGI_ID || cgi_id==STATUSMAP_CGI_ID || cgi_id==TRENDS_CGI_ID)
		printf("<div id=\"popup\" style=\"position:absolute; z-index:1; visibility: hidden\"></div>\n");

	if(cgi_id == STATUS_CGI_ID) {
		/* Set everything in a form, so checkboxes can be searched after and checked. */
		printf("<form name='tableform' id='tableform'>\n");
		printf("<input type=hidden name=hiddenforcefield><input type=hidden name=hiddencmdfield><input type=hidden name=buttonValidChoice><input type=hidden name=buttonCheckboxChecked>\n");

		/* Print out the activator for the dropdown (which must be between the body tags */
/*		printf("<script language='javascript'>");
		printf("$(document).ready(function(e) {");
		printf("try {");
		printf("$('body select').msDropDown();");
		printf("} catch(e) {");
		printf("alert(e.message);");
		printf("}");
		printf("});");
		printf("</script>\n");
*/
		/* Javascript lib to show tooltips */
	}

	if(cgi_id == STATUS_CGI_ID || cgi_id == CMD_CGI_ID) {
		printf("\n<script type='text/javascript' src='%s%s'>\n<!-- SkinnyTip (c) Elliott Brueggeman -->\n</script>\n",url_js_path,SKINNYTIP_JS);
		printf("<div id='tiplayer' style='position:absolute; visibility:hidden; z-index:1000;'></div>\n");
	}

	return;
}


void document_footer(int cgi_id){
	char *cgi_name=NULL;

	switch(cgi_id) {
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

	if(embedded || content_type!=HTML_CONTENT)
		return;

	if(content_type==WML_CONTENT){
		/* used by cmd.cgi */
		printf("</card>\n");
		printf("</wml>\n");
		return;
	}

	if(cgi_id==STATUSWML_CGI_ID) {
		printf("</wml>\n");
		return;
	}

	if(cgi_id == STATUS_CGI_ID) {
		/* Close the form */
		printf("</form>\n");
	}

	/* include user SSI footer */
	include_ssi_files(cgi_name,SSI_FOOTER);

	printf("</body>\n");
	printf("</html>\n");

	return;
}

/* write JavaScript code an layer for popup window */
void write_popup_code(int cgi_id){
	char *border_color="#000000";
	char *background_color="#ffffcc";
	int border=1;
	int padding=3;
	int x_offset=3;
	int y_offset=3;

	printf("<SCRIPT LANGUAGE='JavaScript'>\n");
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
	printf("table += \"<table bgcolor='%s' border=%d cellpadding=%d cellspacing=0>\";\n",background_color,border,padding);
	printf("table += \"<tr><td>\";\n");
	printf("table += \"<table cellspacing=0 cellpadding=%d>\";\n",padding);
	printf("table += \"<tr><td bgcolor='%s' class='popupText'>\" + text + \"</td></tr>\";\n",background_color);
	printf("table += \"</table></td></tr></table>\"\n");
	printf("document.popup.innerHTML = table;\n");
	
	if (cgi_id==STATUSMAP_CGI_ID) {
	        printf("document.popup.style.left = document.body.scrollLeft + %d;\n",x_offset);
	        printf("document.popup.style.top = document.body.scrollTop + %d;\n",y_offset);
        } else if (cgi_id==TRENDS_CGI_ID){
	        printf("document.popup.style.left = (document.all ? eventObj.x : eventObj.layerX) + %d;\n",x_offset);
	        printf("document.popup.style.top  = (document.all ? eventObj.y : eventObj.layerY) + %d;\n",y_offset);
	}

	printf("document.popup.style.visibility = \"visible\";\n");
	printf("} \n");


	printf("else{\n");
	printf("table += \"<table cellpadding=%d border=%d cellspacing=0 bordercolor='%s'>\";\n",padding,border,border_color);
	printf("table += \"<tr><td bgcolor='%s' class='popupText'>\" + text + \"</td></tr></table>\";\n",background_color);
	printf("document.popup.document.open();\n");
	printf("document.popup.document.write(table);\n");
	printf("document.popup.document.close();\n");

	/* set x coordinate */
	printf("document.popup.left = eventObj.layerX + %d;\n",x_offset);
	
	/* make sure we don't overlap the right side of the screen */
	printf("if(document.popup.left + document.popup.document.width + %d > window.innerWidth) document.popup.left = window.innerWidth - document.popup.document.width - %d - 16;\n",x_offset,x_offset);
		
	/* set y coordinate */
	printf("document.popup.top  = eventObj.layerY + %d;\n",y_offset);
	
	/* make sure we don't overlap the bottom edge of the screen */
	printf("if(document.popup.top + document.popup.document.height + %d > window.innerHeight) document.popup.top = window.innerHeight - document.popup.document.height - %d - 16;\n",y_offset,y_offset);
		
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
char *unescape_newlines(char *rawbuf){
	register int x,y;

	for(x=0,y=0;rawbuf[x]!=(char)'\x0';x++){

		if(rawbuf[x]=='\\'){

			/* unescape newlines */
			if(rawbuf[x+1]=='n'){
				rawbuf[y++]='\n';
				x++;
				}

			/* unescape backslashes and other stuff */
			if(rawbuf[x+1]!='\x0'){
				rawbuf[y++]=rawbuf[x+1];
				x++;
				}

			}
		else
			rawbuf[y++]=rawbuf[x];
		}
	rawbuf[y]='\x0';

	return rawbuf;
	}

/* escapes newlines in a string */
char *escape_newlines(char *rawbuf) {
	char *newbuf=NULL;
	register int x,y;

	if(rawbuf==NULL)
		return NULL;

	/* allocate enough memory to escape all chars if necessary */
	if((newbuf=malloc((strlen(rawbuf)*2)+1))==NULL)
		return NULL;

	for(x=0,y=0;rawbuf[x]!=(char)'\x0';x++){

		/* escape backslashes */
		if(rawbuf[x]=='\\'){
			newbuf[y++]='\\';
			newbuf[y++]='\\';
			}

		/* escape newlines */
		else if(rawbuf[x]=='\n'){
			newbuf[y++]='\\';
			newbuf[y++]='n';
			}

		else
			newbuf[y++]=rawbuf[x];
		}
	newbuf[y]='\x0';

	return newbuf;
}


/* strips HTML and bad stuff from plugin output */
void sanitize_plugin_output(char *buffer){
	int x=0;
	int y=0;
	int in_html=FALSE;
	char *new_buffer;

	if(buffer==NULL)
		return;

	new_buffer=strdup(buffer);
	if(new_buffer==NULL)
		return;

	/* check each character */
	for(x=0,y=0;buffer[x]!='\x0';x++){

		/* we just started an HTML tag */
		if(buffer[x]=='<'){
			in_html=TRUE;
			continue;
		        }

		/* end of an HTML tag */
		else if(buffer[x]=='>'){
			in_html=FALSE;
			continue;
		        }

		/* skip everything inside HTML tags */
		else if(in_html==TRUE)
			continue;

		/* strip single and double quotes */
		else if(buffer[x]=='\'' || buffer[x]=='\"')
			new_buffer[y++]=' ';

		/* strip semicolons (replace with colons) */
		else if(buffer[x]==';')
			new_buffer[y++]=':';

		/* strip pipe and ampersand */
		else if(buffer[x]=='&' || buffer[x]=='|')
			new_buffer[y++]=' ';

		/* normal character */
		else
			new_buffer[y++]=buffer[x];
	        }

	/* terminate sanitized buffer */
	new_buffer[y++]='\x0';

	/* copy the sanitized buffer back to the original */
	strcpy(buffer,new_buffer);

	/* free memory allocated to the new buffer */
	free(new_buffer);

	return;
        }



/* get date/time string */
void get_time_string(time_t *raw_time,char *buffer,int buffer_length,int type){
	time_t t;
	struct tm *tm_ptr=NULL;
	int hour=0;
	int minute=0;
	int second=0;
	int month=0;
	int day=0;
	int year=0;
	char *weekdays[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	char *months[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	char *tzone="";

	if(raw_time==NULL)
		time(&t);
	else
		t=*raw_time;

	if(type==HTTP_DATE_TIME)
		tm_ptr=gmtime(&t);
	else
		tm_ptr=localtime(&t);

	hour=tm_ptr->tm_hour;
	minute=tm_ptr->tm_min;
	second=tm_ptr->tm_sec;
	month=tm_ptr->tm_mon+1;
	day=tm_ptr->tm_mday;
	year=tm_ptr->tm_year+1900;

#ifdef HAVE_TM_ZONE
	tzone=(char *)tm_ptr->tm_zone;
#else
	tzone=(tm_ptr->tm_isdst)?tzname[1]:tzname[0];
#endif

	/* ctime() style */
	if(type==LONG_DATE_TIME)
		snprintf(buffer,buffer_length,"%s %s %d %02d:%02d:%02d %s %d",weekdays[tm_ptr->tm_wday],months[tm_ptr->tm_mon],day,hour,minute,second,tzone,year);

	/* short style */
	else if(type==SHORT_DATE_TIME){
		if(date_format==DATE_FORMAT_EURO)
			snprintf(buffer,buffer_length,"%02d-%02d-%04d %02d:%02d:%02d",tm_ptr->tm_mday,tm_ptr->tm_mon+1,tm_ptr->tm_year+1900,tm_ptr->tm_hour,tm_ptr->tm_min,tm_ptr->tm_sec);
		else if(date_format==DATE_FORMAT_ISO8601 || date_format==DATE_FORMAT_STRICT_ISO8601)
			snprintf(buffer,buffer_length,"%04d-%02d-%02d%c%02d:%02d:%02d",tm_ptr->tm_year+1900,tm_ptr->tm_mon+1,tm_ptr->tm_mday,(date_format==DATE_FORMAT_STRICT_ISO8601)?'T':' ',tm_ptr->tm_hour,tm_ptr->tm_min,tm_ptr->tm_sec);
		else
			snprintf(buffer,buffer_length,"%02d-%02d-%04d %02d:%02d:%02d",tm_ptr->tm_mon+1,tm_ptr->tm_mday,tm_ptr->tm_year+1900,tm_ptr->tm_hour,tm_ptr->tm_min,tm_ptr->tm_sec);
	        }

	/* short date */
	else if(type==SHORT_DATE){
		if(date_format==DATE_FORMAT_EURO)
			snprintf(buffer,buffer_length,"%02d-%02d-%04d",day,month,year);
		else if(date_format==DATE_FORMAT_ISO8601 || date_format==DATE_FORMAT_STRICT_ISO8601)
			snprintf(buffer,buffer_length,"%04d-%02d-%02d",year,month,day);
		else
			snprintf(buffer,buffer_length,"%02d-%02d-%04d",month,day,year);
	        }

	/* expiration date/time for HTTP headers */
	else if(type==HTTP_DATE_TIME)
		snprintf(buffer,buffer_length,"%s, %02d %s %d %02d:%02d:%02d GMT",weekdays[tm_ptr->tm_wday],day,months[tm_ptr->tm_mon],year,hour,minute,second);

	/* short time */
	else
		snprintf(buffer,buffer_length,"%02d:%02d:%02d",hour,minute,second);

	buffer[buffer_length-1]='\x0';

	return;
        }


/* get time string for an interval of time */
void get_interval_time_string(double time_units,char *buffer,int buffer_length){
	unsigned long total_seconds;
	int hours=0;
	int minutes=0;
	int seconds=0;

	total_seconds=(unsigned long)(time_units*interval_length);
	hours=(int)total_seconds/3600;
	total_seconds%=3600;
	minutes=(int)total_seconds/60;
	total_seconds%=60;
	seconds=(int)total_seconds;
	snprintf(buffer,buffer_length,"%dh %dm %ds",hours,minutes,seconds);
	buffer[buffer_length-1]='\x0';

	return;
        }


/* encodes a string in proper URL format */
char * url_encode(char *input){
	int len,output_len;
	int x,y;
	char temp_expansion[4];
	static int i = 0;
	char* str = encoded_url_string[i];

	/* initialize return string */
	strcpy(str,"");

	if(input==NULL)
		return str;

	len=(int)strlen(input);
	output_len=(int)sizeof(encoded_url_string[0]);

	str[0]='\x0';

	for(x=0,y=0;x<=len && y<output_len-1;x++){

		/* end of string */
		if((char)input[x]==(char)'\x0'){
			str[y]='\x0';
			break;
		        }

		/* alpha-numeric characters and a few other characters don't get encoded */
		else if(((char)input[x]>='0' && (char)input[x]<='9') || ((char)input[x]>='A' && (char)input[x]<='Z') || ((char)input[x]>=(char)'a' && (char)input[x]<=(char)'z') || (char)input[x]==(char)'.' || (char)input[x]==(char)'-' || (char)input[x]==(char)'_'){
			str[y]=input[x];
			y++;
		        }

		/* high bit characters don't get encoded */
		else if((unsigned char)input[x]>=0x7f){
			str[y]=input[x];
			y++;
		}

		/* spaces are pluses */
		else if((char)input[x]==(char)' '){
			str[y]='+';
			y++;
		        }

		/* anything else gets represented by its hex value */
		else{
			str[y]='\x0';
			if((int)strlen(str)<(output_len-3)){
				sprintf(temp_expansion,"%%%02X",(unsigned int)input[x]);
				strcat(str,temp_expansion);
				y+=3;
			        }
		        }
	        }

	str[sizeof(encoded_url_string[0])-1]='\x0';

	return str;
        }



/* escapes a string used in HTML */
char * html_encode(char *input, int escape_newlines){
	int len,output_len;
	int x,y;
	char temp_expansion[10];

	/* we need up to six times the space to do the conversion */
	len=(int)strlen(input);
	output_len=len*6;
	if((encoded_html_string=(char *)malloc(output_len+1))==NULL)
		return "";

	strcpy(encoded_html_string,"");

	for(x=0,y=0;x<=len;x++){

		/* end of string */
		if((char)input[x]==(char)'\x0'){
			encoded_html_string[y]='\x0';
			break;
		        }

		/* alpha-numeric characters and spaces don't get encoded */
		else if(((char)input[x]==(char)' ') || ((char)input[x]>='0' && (char)input[x]<='9') || ((char)input[x]>='A' && (char)input[x]<='Z') || ((char)input[x]>=(char)'a' && (char)input[x]<=(char)'z'))
			encoded_html_string[y++]=input[x];

		/* newlines turn to <BR> tags */
		else if(escape_newlines==TRUE && (char)input[x]==(char)'\n'){
			strcpy(&encoded_html_string[y],"<BR>");
			y+=4;
			}
		else if(escape_newlines==TRUE && (char)input[x]==(char)'\\' && (char)input[x+1]==(char)'n'){
			strcpy(&encoded_html_string[y],"<BR>");
			y+=4;
			x++;
			}

		/* TODO - strip all but allowed HTML tags out... */

		else if((char)input[x]==(char)'<'){

			if(escape_html_tags==FALSE)
				encoded_html_string[y++]=input[x];
			else{
				encoded_html_string[y]='\x0';
				if((int)strlen(encoded_html_string)<(output_len-4)){
					strcat(encoded_html_string,"&lt;");
					y+=4;
					}
				}
		        }

		else if((char)input[x]==(char)'>'){

			if(escape_html_tags==FALSE)
				encoded_html_string[y++]=input[x];
			else{
				encoded_html_string[y]='\x0';
				if((int)strlen(encoded_html_string)<(output_len-4)){
					strcat(encoded_html_string,"&gt;");
					y+=4;
					}
				}
		        }

		/* high bit chars don't get encoded, so we won't be breaking utf8 characters */
		else if ((unsigned char)input[x] >= 0x7f)
			encoded_html_string[y++]=input[x];

		/* for simplicity, all other chars represented by their numeric value */
		else{
			if(escape_html_tags==FALSE)
				encoded_html_string[y++]=input[x];
			else{
				encoded_html_string[y]='\x0';
				sprintf(temp_expansion,"&#%d;",(unsigned char)input[x]);
				if((int)strlen(encoded_html_string)<(output_len-strlen(temp_expansion))){
					strcat(encoded_html_string,temp_expansion);
					y+=strlen(temp_expansion);
					}
				}
		        }
	        }

	encoded_html_string[y++]='\x0';

	return encoded_html_string;
        }



/* strip > and < from string */
void strip_html_brackets(char *buffer){
	register int x;
	register int y;
	register int z;

	if(buffer==NULL || buffer[0]=='\x0')
		return;

	/* remove all occurances in string */
	z=(int)strlen(buffer);
	for(x=0,y=0;x<z;x++){
		if(buffer[x]=='<' || buffer[x]=='>')
			continue;
		buffer[y++]=buffer[x];
	        }
	buffer[y++]='\x0';

	return;
	}



/* escape string for html form usage */
char * escape_string(char *input){
	int len,output_len;
	int x,y;
	char temp_expansion[10];

	/* we need up to six times the space to do the conversion */
	len=(int)strlen(input);
	output_len=len*6;
	if((encoded_html_string=(char *)malloc(output_len+1))==NULL)
		return "";

	strcpy(encoded_html_string,"");

	for(x=0,y=0;x<=len;x++){

		/* end of string */
		if((char)input[x]==(char)'\x0'){
			encoded_html_string[y]='\x0';
			break;
			}

		/* alpha-numeric characters don't get encoded */
		else if(((char)input[x]>='0' && (char)input[x]<='9') || ((char)input[x]>='A' && (char)input[x]<='Z') || ((char)input[x]>=(char)'a' && (char)input[x]<=(char)'z'))
			encoded_html_string[y++]=input[x];

		/* spaces, hyphens, periods, underscores and colons don't get encoded */
		else if(((char)input[x]==(char)' ') || ((char)input[x]==(char)'-') || ((char)input[x]==(char)'.') || ((char)input[x]==(char)'_') || ((char)input[x]==(char)':'))
			encoded_html_string[y++]=input[x];

		/* high bit characters don't get encoded */
		else if((unsigned char)input[x]>=0x7f)
			encoded_html_string[y++]=input[x];

		/* for simplicity, all other chars represented by their numeric value */
		else{
			encoded_html_string[y]='\x0';
			sprintf(temp_expansion,"&#%d;",(unsigned char)input[x]);
			if((int)strlen(encoded_html_string)<(output_len-strlen(temp_expansion))){
				strcat(encoded_html_string,temp_expansion);
				y+=strlen(temp_expansion);
				}
			}
	        }

	encoded_html_string[y++]='\x0';

	return encoded_html_string;
        }



/* determines the log file we should use (from current time) */
void get_log_archive_to_use(int archive,char *buffer,int buffer_length){
	struct tm *t;
	FILE *fd;

	/* determine the time at which the log was rotated for this archive # */
	determine_log_rotation_times(archive);

	/* if we're not rotating the logs or if we want the current log, use the main one... */
	if(log_rotation_method==LOG_ROTATION_NONE || archive<=0){
		strncpy(buffer,log_file,buffer_length);
		buffer[buffer_length-1]='\x0';
		return;
	        }

	t=localtime(&this_scheduled_log_rotation);

	/* use the time that the log rotation occurred to figure out the name of the log file */
	snprintf(buffer,buffer_length,"%sicinga-%02d-%02d-%d-%02d.log",log_archive_path, t->tm_mon+1, t->tm_mday, t->tm_year+1900, t->tm_hour);
	buffer[buffer_length-1]='\x0';

	/* check if a icinga named archive logfile already exist. Otherwise change back to nagios syntax */
	if((fd = fopen(buffer, "r")) == NULL){
		snprintf(buffer,buffer_length,"%snagios-%02d-%02d-%d-%02d.log",log_archive_path,t->tm_mon+1,t->tm_mday,t->tm_year+1900,t->tm_hour);
		buffer[buffer_length-1]='\x0';

		/* 06-02-2010 Michael Friedrich
		   Yeah, and if no log has been written, nagios- will fail with the wrong error message
		   leading the user to the assumption that the logfile is not even created - if the logfile
		   was not rotated by the core after this date */
		if((fd = fopen(buffer, "r")) == NULL){
			snprintf(buffer,buffer_length,"%sicinga-%02d-%02d-%d-%02d.log",log_archive_path, t->tm_mon+1, t->tm_mday, t->tm_year+1900, t->tm_hour);
			buffer[buffer_length-1]='\x0';
		}
		else {
			fclose(fd);
		}
	}
	else {
		fclose(fd);
	}

	return;
        }



/* determines log archive to use, given a specific time */
int determine_archive_to_use_from_time(time_t target_time){
	time_t current_time;
	int current_archive=0;

	/* if log rotation is disabled, we don't have archives */
	if(log_rotation_method==LOG_ROTATION_NONE)
		return 0;

	/* make sure target time is rational */
	current_time=time(NULL);
	if(target_time>=current_time)
		return 0;

	/* backtrack through archives to find the one we need for this time */
	/* start with archive of 1, subtract one when we find the right time period to compensate for current (non-rotated) log */
	for(current_archive=1;;current_archive++){

		/* determine time at which the log rotation occurred for this archive number */
		determine_log_rotation_times(current_archive);

		/* if the target time falls within the times encompassed by this archive, we have the right archive! */
		if(target_time>=this_scheduled_log_rotation)
			return current_archive-1;
	        }

	return 0;
        }



/* determines the log rotation times - past, present, future */
void determine_log_rotation_times(int archive){
	struct tm *t;
	int current_month;
	int is_dst_now=FALSE;
	time_t current_time;

	/* negative archive numbers don't make sense */
	/* if archive=0 (current log), this_scheduled_log_rotation time is set to next rotation time */
	if(archive<0)
		return;

	time(&current_time);
	t=localtime(&current_time);
	is_dst_now=(t->tm_isdst>0)?TRUE:FALSE;
	t->tm_min=0;
	t->tm_sec=0;


	switch(log_rotation_method){

	case LOG_ROTATION_HOURLY:
		this_scheduled_log_rotation=mktime(t);
		this_scheduled_log_rotation=(time_t)(this_scheduled_log_rotation-((archive-1)*3600));
		last_scheduled_log_rotation=(time_t)(this_scheduled_log_rotation-3600);
		break;

	case LOG_ROTATION_DAILY:
		t->tm_hour=0;
		this_scheduled_log_rotation=mktime(t);
		this_scheduled_log_rotation=(time_t)(this_scheduled_log_rotation-((archive-1)*86400));
		last_scheduled_log_rotation=(time_t)(this_scheduled_log_rotation-86400);
		break;

	case LOG_ROTATION_WEEKLY:
		t->tm_hour=0;
		this_scheduled_log_rotation=mktime(t);
		this_scheduled_log_rotation=(time_t)(this_scheduled_log_rotation-(86400*t->tm_wday));
		this_scheduled_log_rotation=(time_t)(this_scheduled_log_rotation-((archive-1)*604800));
		last_scheduled_log_rotation=(time_t)(this_scheduled_log_rotation-604800);
		break;

	case LOG_ROTATION_MONTHLY:

		t=localtime(&current_time);
		t->tm_mon++;
		t->tm_mday=1;
		t->tm_hour=0;
		t->tm_min=0;
		t->tm_sec=0;
		for(current_month=0;current_month<=archive;current_month++){
			if(t->tm_mon==0){
				t->tm_mon=11;
				t->tm_year--;
			        }
			else
				t->tm_mon--;
		        }
		last_scheduled_log_rotation=mktime(t);

		t=localtime(&current_time);
		t->tm_mon++;
		t->tm_mday=1;
		t->tm_hour=0;
		t->tm_min=0;
		t->tm_sec=0;
		for(current_month=0;current_month<archive;current_month++){
			if(t->tm_mon==0){
				t->tm_mon=11;
				t->tm_year--;
			        }
			else
				t->tm_mon--;
		        }
		this_scheduled_log_rotation=mktime(t);

		break;
	default:
		break;
	        }

	/* adjust this rotation time for daylight savings time */
	t=localtime(&this_scheduled_log_rotation);
	if(t->tm_isdst>0 && is_dst_now==FALSE)
		this_scheduled_log_rotation=(time_t)(this_scheduled_log_rotation-3600);
	else if(t->tm_isdst==0 && is_dst_now==TRUE)
		this_scheduled_log_rotation=(time_t)(this_scheduled_log_rotation+3600);

	/* adjust last rotation time for daylight savings time */
	t=localtime(&last_scheduled_log_rotation);
	if(t->tm_isdst>0 && is_dst_now==FALSE)
		last_scheduled_log_rotation=(time_t)(last_scheduled_log_rotation-3600);
	else if(t->tm_isdst==0 && is_dst_now==TRUE)
		last_scheduled_log_rotation=(time_t)(last_scheduled_log_rotation+3600);

	return;
        }




/**********************************************************
 *************** COMMON HTML FUNCTIONS ********************
 **********************************************************/

void display_info_table(char *title,int refresh, authdata *current_authdata, int daemon_check){
	time_t current_time;
	char date_time[MAX_DATETIME_LENGTH];
	int result;

	/* read program status */
	result=read_all_status_data(get_cgi_config_location(),READ_PROGRAM_STATUS);

	printf("<TABLE CLASS='infoBox' BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD CLASS='infoBox'>\n");
	printf("<DIV CLASS='infoBoxTitle'>%s</DIV>\n",title);

	time(&current_time);
	get_time_string(&current_time,date_time,(int)sizeof(date_time),LONG_DATE_TIME);

	printf("Last Updated: %s<BR>\n",date_time);

	/* decide if refresh is paused or not */
	if(refresh==TRUE) {
		/* if refresh, add paused query to url and set location.href */
		printf("Updated every %d seconds <small>[<a href=\"javascript:window.location.href += ((window.location.toString().indexOf('?') != -1) ? '&' : '?') + 'paused'\">pause</a>]</small><br>\n",refresh_rate);
	} else {
		/* if no refresh, remove the paused query from url and set location.href */
		printf("Update is paused <small>[<a href=\"javascript:window.location.href = window.location.href.replace(/[\?&]paused/,'')\">continue</a>]</small><br>\n");
	}

	printf("%s %s - <A HREF='http://www.icinga.org' TARGET='_new' CLASS='homepageURL'>www.icinga.org</A><BR>\n", PROGRAM_NAME, PROGRAM_VERSION);

	/* We shouldn't forget from where we come */
	printf("(Credits to: Nagios&reg; - <A HREF='http://www.nagios.org' TARGET='_new' CLASS='homepageURL'>www.nagios.org</A>)<BR>");

	if(current_authdata!=NULL)
		printf("Logged in as <i>%s</i><BR>\n",(!strcmp(current_authdata->username,""))?"?":current_authdata->username);

	if(nagios_process_state!=STATE_OK)
		printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: Monitoring process may not be running!<BR>Click <A HREF='%s?type=%d'>here</A> for more info.</DIV>",EXTINFO_CGI,DISPLAY_PROCESS_INFO);

	if(result==ERROR && daemon_check == TRUE)
		printf("<DIV CLASS='infoBoxBadProcStatus'>Warning: Could not read program status information!</DIV>");

	else{
		if(enable_notifications==FALSE)
			printf("<DIV CLASS='infoBoxBadProcStatus'>- Notifications are disabled</DIV>");

		if(execute_service_checks==FALSE)
			printf("<DIV CLASS='infoBoxBadProcStatus'>- Service checks are disabled</DIV>");
	        }

	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	return;
        }



void display_nav_table(char *url,int archive){
	char date_time[MAX_DATETIME_LENGTH];
	char archive_file[MAX_INPUT_BUFFER];
	char *archive_basename;

	if(log_rotation_method!=LOG_ROTATION_NONE){
		printf("<table border=0 cellspacing=0 cellpadding=0 CLASS='navBox'>\n");
		printf("<tr>\n");
		printf("<td align=center valign=center CLASS='navBoxItem'>\n");
		if(archive==0){
			printf("Latest Archive<br>");
			printf("<a href='%sarchive=1'><img src='%s%s' border=0 alt='Latest Archive' title='Latest Archive'></a>",url,url_images_path,LEFT_ARROW_ICON);
		        }
		else{
			printf("Earlier Archive<br>");
			printf("<a href='%sarchive=%d'><img src='%s%s' border=0 alt='Earlier Archive' title='Earlier Archive'></a>",url,archive+1,url_images_path,LEFT_ARROW_ICON);
		        }
		printf("</td>\n");

		printf("<td width=15></td>\n");

		printf("<td align=center CLASS='navBoxDate'>\n");
		printf("<DIV CLASS='navBoxTitle'>Log File Navigation</DIV>\n");
		get_time_string(&last_scheduled_log_rotation,date_time,(int)sizeof(date_time),LONG_DATE_TIME);
		printf("%s",date_time);
		printf("<br>to<br>");
		if(archive==0)
			printf("Present..");
		else{
			get_time_string(&this_scheduled_log_rotation,date_time,(int)sizeof(date_time),LONG_DATE_TIME);
			printf("%s",date_time);
		        }
		printf("</td>\n");

		printf("<td width=15></td>\n");
		if(archive!=0){

			printf("<td align=center valign=center CLASS='navBoxItem'>\n");
			if(archive==1){
				printf("Current Log<br>");
				printf("<a href='%s'><img src='%s%s' border=0 alt='Current Log' title='Current Log'></a>",url,url_images_path,RIGHT_ARROW_ICON);
			        }
			else{
				printf("More Recent Archive<br>");
				printf("<a href='%sarchive=%d'><img src='%s%s' border=0 alt='More Recent Archive' title='More Recent Archive'></a>",url,archive-1,url_images_path,RIGHT_ARROW_ICON);
			        }
			printf("</td>\n");
		        }
		else
			printf("<td><img src='%s%s' border=0 width=75 height=1></td>\n",url_images_path,EMPTY_ICON);

		printf("</tr>\n");

		printf("</table>\n");
	        }

	/* get archive to use */
	get_log_archive_to_use(archive,archive_file,sizeof(archive_file)-1);

	/* cut the pathname for security, and the remaining slash for clarity */
	archive_basename=(char *)&archive_file;
	if(strrchr((char *)&archive_basename,'/')!=NULL)
		archive_basename=strrchr((char *)&archive_file,'/')+1;

	/* now it's safe to print the filename */
	printf("<BR><DIV CLASS='navBoxFile'>File: %s</DIV>\n",archive_basename);

	return;
        }



/* prints the additional notes or action url for a hostgroup (with macros substituted) */
void print_extra_hostgroup_url(char *group_name, char *url){
	char input_buffer[MAX_INPUT_BUFFER]="";
	char output_buffer[MAX_INPUT_BUFFER]="";
	char *temp_buffer;
	int in_macro=FALSE;
	hostgroup *temp_hostgroup=NULL;

	if(group_name==NULL || url==NULL)
		return;

	temp_hostgroup=find_hostgroup(group_name);
	if(temp_hostgroup==NULL){
		printf("%s",url);
		return;
	        }

	strncpy(input_buffer,url,sizeof(input_buffer)-1);
	input_buffer[sizeof(input_buffer)-1]='\x0';

	for(temp_buffer=my_strtok(input_buffer,"$");temp_buffer!=NULL;temp_buffer=my_strtok(NULL,"$")){

		if(in_macro==FALSE){
			if(strlen(output_buffer)+strlen(temp_buffer)<sizeof(output_buffer)-1){
				strncat(output_buffer,temp_buffer,sizeof(output_buffer)-strlen(output_buffer)-1);
				output_buffer[sizeof(output_buffer)-1]='\x0';
			        }
			in_macro=TRUE;
			}
		else{

			if(strlen(output_buffer)+strlen(temp_buffer) < sizeof(output_buffer)-1){

				if(!strcmp(temp_buffer,"HOSTGROUPNAME"))
					strncat(output_buffer,url_encode(temp_hostgroup->group_name),sizeof(output_buffer)-strlen(output_buffer)-1);
			        }

			in_macro=FALSE;
		        }
	        }

	printf("%s",output_buffer);

	return;
        }



/* prints the additional notes or action url for a servicegroup (with macros substituted) */
void print_extra_servicegroup_url(char *group_name, char *url){
	char input_buffer[MAX_INPUT_BUFFER]="";
	char output_buffer[MAX_INPUT_BUFFER]="";
	char *temp_buffer;
	int in_macro=FALSE;
	servicegroup *temp_servicegroup=NULL;

	if(group_name==NULL || url==NULL)
		return;

	temp_servicegroup=find_servicegroup(group_name);
	if(temp_servicegroup==NULL){
		printf("%s",url);
		return;
	        }

	strncpy(input_buffer,url,sizeof(input_buffer)-1);
	input_buffer[sizeof(input_buffer)-1]='\x0';

	for(temp_buffer=my_strtok(input_buffer,"$");temp_buffer!=NULL;temp_buffer=my_strtok(NULL,"$")){

		if(in_macro==FALSE){
			if(strlen(output_buffer)+strlen(temp_buffer)<sizeof(output_buffer)-1){
				strncat(output_buffer,temp_buffer,sizeof(output_buffer)-strlen(output_buffer)-1);
				output_buffer[sizeof(output_buffer)-1]='\x0';
			        }
			in_macro=TRUE;
			}
		else{

			if(strlen(output_buffer)+strlen(temp_buffer) < sizeof(output_buffer)-1){

				if(!strcmp(temp_buffer,"SERVICEGROUPNAME"))
					strncat(output_buffer,url_encode(temp_servicegroup->group_name),sizeof(output_buffer)-strlen(output_buffer)-1);
			        }

			in_macro=FALSE;
		        }
	        }

	printf("%s",output_buffer);

	return;
        }



/* include user-defined SSI footers or headers */
void include_ssi_files(char *cgi_name, int type){
	char common_ssi_file[MAX_INPUT_BUFFER];
	char cgi_ssi_file[MAX_INPUT_BUFFER];
	char raw_cgi_name[MAX_INPUT_BUFFER];
	char *stripped_cgi_name;
	int x;

	/* common header or footer */
	snprintf(common_ssi_file,sizeof(common_ssi_file)-1,"%scommon-%s.ssi",physical_ssi_path,(type==SSI_HEADER)?"header":"footer");
	common_ssi_file[sizeof(common_ssi_file)-1]='\x0';

	/* CGI-specific header or footer */
	strncpy(raw_cgi_name,cgi_name,sizeof(raw_cgi_name)-1);
	raw_cgi_name[sizeof(raw_cgi_name)-1]='\x0';
	stripped_cgi_name=strtok(raw_cgi_name,".");
	snprintf(cgi_ssi_file,sizeof(cgi_ssi_file)-1,"%s%s-%s.ssi",physical_ssi_path,(stripped_cgi_name==NULL)?"":stripped_cgi_name,(type==SSI_HEADER)?"header":"footer");
	cgi_ssi_file[sizeof(cgi_ssi_file)-1]='\x0';
	for(x=0;x<strlen(cgi_ssi_file);x++)
		cgi_ssi_file[x]=tolower(cgi_ssi_file[x]);

	if(type==SSI_HEADER){
		printf("\n<!-- Produced by %s (http://www.%s.org).\nCopyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)\nCopyright (c) 2009-2011 Icinga Development Team -->\n", PROGRAM_NAME, PROGRAM_NAME_LC);
		include_ssi_file(common_ssi_file);
		include_ssi_file(cgi_ssi_file);
	        }
	else{
		include_ssi_file(cgi_ssi_file);
		include_ssi_file(common_ssi_file);
		printf("\n<!-- Produced by %s (http://www.%s.org).\nCopyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)\nCopyright (c) 2009-2011 Icinga Development Team -->\n", PROGRAM_NAME, PROGRAM_NAME_LC);
	        }

	return;
        }



/* include user-defined SSI footer or header */
void include_ssi_file(char *filename){
	char buffer[MAX_INPUT_BUFFER];
	FILE *fp;
        struct stat stat_result;
        int call_return;

        /* if file is executable, we want to run it rather than print it */
        call_return=stat(filename,&stat_result);

	/* file is executable */
	if(call_return==0 && (stat_result.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))){

		/* must flush output stream first so that output
		   from script shows up in correct place. Other choice
		   is to open program under pipe and copy the data from
		   the program to our output stream.
		*/
		fflush(stdout);

		/* ignore return status from system call. */
		call_return=system(filename);

		return;
	        }

	/* an error occurred trying to stat() the file */
	else if(call_return!=0){

		/* Handle error conditions. Assume that standard posix error codes and errno are available. If not, comment this section out. */
		switch(errno){
		case ENOTDIR: /* - A component of the path is not a directory. */
		case ELOOP: /* Too many symbolic links encountered while traversing the path. */
		case EFAULT: /* Bad address. */
		case ENOMEM: /* Out of memory (i.e. kernel memory). */
		case ENAMETOOLONG: /* File name too long. */
			printf("<br /> A stat call returned %d while looking for the file %s.<br />", errno, filename);
			return;
		case EACCES: /* Permission denied. -- The file should be accessible by nagios. */
			printf("<br /> A stat call returned a permissions error(%d) while looking for the file %s.<br />", errno, filename);
			return;
		case ENOENT: /* A component of the path file_name does not exist, or the path is an empty string. Just return if the file doesn't exist. */
			return;
		default:
			return;
		        }
	        }

	fp=fopen(filename,"r");
	if(fp==NULL)
		return;

	/* print all lines in the SSI file */
	while(fgets(buffer,(int)(sizeof(buffer)-1),fp)!=NULL)
		printf("%s",buffer);

	fclose(fp);

	return;
        }


/* displays an error if CGI config file could not be read */
void cgi_config_file_error(char *config_file){

	printf("<H1>Whoops!</H1>\n");

	printf("<P><STRONG><FONT COLOR='RED'>Error: Could not open CGI config file '%s' for reading!</FONT></STRONG></P>\n",config_file);

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
void main_config_file_error(char *config_file){

	printf("<H1>Whoops!</H1>\n");

	printf("<P><STRONG><FONT COLOR='RED'>Error: Could not open main config file '%s' for reading!</FONT></STRONG></P>\n",config_file);

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


/* displays an error if object data could not be read */
void object_data_error(void){

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
void status_data_error(void){

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


void print_error(char *config_file, int error_type){

	/* if cgi.cfg is missing, we don't know which fancy style to use, take our own */
	if(error_type!=ERROR_CGI_CFG_FILE){
	        document_header(ERROR_CGI_ID,TRUE);
	}

         /* Giving credits to stop.png image source */
        printf("\n<!-- Image \"stop.png\" has been taken from \"http://fedoraproject.org/wiki/Template:Admon/caution\" -->\n\n");

        printf("<BR><DIV align='center'><DIV CLASS='errorBox'>\n");
	if(error_type==ERROR_CGI_CFG_FILE){
	        printf("<DIV style='font-family:  Helvetica, serif; background-color: #fff; color: #000; font-size: 8pt; text-align:left; font-weight: bold; margin:1em; border:1px red solid; background-color: #FFE5E5;' CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55 valign=top></td>");
	} else {
	        printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55 valign=top><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);
	}
        printf("<td class='errorDescription'>");

	switch(error_type){
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
	}

        printf("</td></tr></table></DIV>\n");
        printf("</DIV>\n");

	return;
}

/* displays context-sensitive help window */
void display_context_help(char *chid){
	char *icon=CONTEXT_HELP_ICON1;

	if(show_context_help==FALSE)
		return;

	/* change icon if necessary */
	if(!strcmp(chid,CONTEXTHELP_TAC))
		icon=CONTEXT_HELP_ICON2;

	printf("<a href='%s%s.html' target='cshw' onClick='javascript:window.open(\"%s%s.html\",\"cshw\",\"width=550,height=600,toolbar=0,location=0,status=0,resizable=1,scrollbars=1\");return true'><img src='%s%s' border=0 alt='Display context-sensitive help for this screen' title='Display context-sensitive help for this screen'></a>\n",url_context_help_path,chid,url_context_help_path,chid,url_images_path,icon);

	return;
        }



void display_splunk_host_url(host *hst){

	if(enable_splunk_integration==FALSE)
		return;
	if(hst==NULL)
		return;

	printf("<a href='%s?q=search %s' target='_blank'><img src='%s%s' alt='Splunk It' title='Splunk It' border='0'></a>\n",splunk_url,url_encode(hst->name),url_images_path,SPLUNK_SMALL_WHITE_ICON);

	return;
	}



void display_splunk_service_url(service *svc){

	if(enable_splunk_integration==FALSE)
		return;
	if(svc==NULL)
		return;

	printf("<a href='%s?q=search %s%%20",splunk_url,url_encode(svc->host_name));
	printf("%s' target='_blank'><img src='%s%s' alt='Splunk It' title='Splunk It' border='0'></a>\n",url_encode(svc->description),url_images_path,SPLUNK_SMALL_WHITE_ICON);

	return;
	}



void display_splunk_generic_url(char *buf, int icon){
	char *newbuf=NULL;

	if(enable_splunk_integration==FALSE)
		return;
	if(buf==NULL)
		return;

	if((newbuf=(char *)strdup(buf))==NULL)
		return;

	strip_splunk_query_terms(newbuf);

	printf("<a href='%s?q=search %s' target='_blank'>",splunk_url,url_encode(newbuf));

	if(icon>0)
		printf("<img src='%s%s' alt='Splunk It' title='Splunk It' border='0'>",url_images_path,(icon==1)?SPLUNK_SMALL_WHITE_ICON:SPLUNK_SMALL_BLACK_ICON);
	printf("</a>\n");

	free(newbuf);

	return;
	}


/* strip quotes and from string */
void strip_splunk_query_terms(char *buffer){
	register int x;
	register int y;
	register int z;

	if(buffer==NULL || buffer[0]=='\x0')
		return;

	/* remove all occurances in string */
	z=(int)strlen(buffer);
	for(x=0,y=0;x<z;x++){
		if(buffer[x]=='\'' || buffer[x]=='\"' || buffer[x]==';' || buffer[x]==':' || buffer[x]==',' || buffer[x]=='-' || buffer[x]=='=')
			buffer[y++]=' ';
		else
			buffer[y++]=buffer[x];
	        }
	buffer[y++]='\x0';

	return;
	}

void print_generic_error_message(char *title, char *text, int returnlevels) {

	if(content_type==WML_CONTENT) {
		printf("<p>");

		if(title!=NULL && title[0]!='\x0')
			printf("%s",title);
		if(text!=NULL && text[0]!='\x0')
			printf("<br>%s",text);

		printf("</p>\n");
	} else {
		printf("<BR><DIV align='center'><DIV CLASS='errorBox'>\n");
		printf("<DIV CLASS='errorMessage'><table cellspacing=0 cellpadding=0 border=0><tr><td width=55><img src=\"%s%s\" border=0></td>",url_images_path,CMD_STOP_ICON);

		if(title!=NULL && title[0]!='\x0')
			printf("<td CLASS='errorMessage'>%s</td></tr></table></DIV>\n",title);

		if(text!=NULL && text[0]!='\x0')
			printf("<DIV CLASS='errorDescription'>%s</DIV><br>",text);

		printf("</DIV>\n");

		if(returnlevels!=0)
			printf("<BR><input type='submit' value='Get me out of here' onClick='window.history.go(-%d);' class='submitButton'></DIV>\n",returnlevels);
	}

	return;
}

