/***********************************************************************
 *
 * READLOGS.C - Functions for reading Log files in Icinga CGIs
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

#include "../include/cgiutils.h"
#include "../include/readlogs.h"


/** Initialazing lists */
lifo		*lifo_list=NULL;
logfilter	*filter_list=NULL;
logentry 	*entry_list=NULL;

/** Initialazing some vars */
extern int      log_rotation_method=LOG_ROTATION_NONE;

time_t          this_scheduled_log_rotation=0L;
time_t          last_scheduled_log_rotation=0L;
time_t          next_scheduled_log_rotation=0L;

extern char     log_file[MAX_FILENAME_LENGTH];
extern char     log_archive_path[MAX_FILENAME_LENGTH];


/** Functions 
 *
 *
 */

/** Add Filter to the list of log filters
 *
 *  Only include OR exclude allowed. No combining for now.
 */
int add_log_filter(int requested_filter, int include_exclude) {
	logfilter *temp_filter=NULL;
	
	temp_filter=(logfilter *)malloc(sizeof(logfilter));
	if(temp_filter==NULL)
		return READLOG_ERROR_MEMORY;
	
	temp_filter->next=NULL;
	temp_filter->include=0;
	temp_filter->exclude=0;

	if (include_exclude==LOGFILTER_INCLUDE)
		temp_filter->include=requested_filter;		
	else if (include_exclude==LOGFILTER_EXCLUDE)
		temp_filter->exclude=requested_filter;
	else
		return READLOG_ERROR;

	if (filter_list==NULL)
		filter_list=temp_filter;
	else{
		temp_filter->next=filter_list;
		filter_list=temp_filter;
	}

	return READLOG_OK;
}

/** Read's a defined logfile
 *
 * Thats the actual function for reading entries.
 */

int get_log_entries(char *log_file, char *search_string, int reverse,time_t ts_start, time_t ts_end) {
	char *input=NULL;
	char *temp_buffer=NULL;
	int error=READLOG_OK;
	time_t timestamp;
	int type=0;
	mmapfile *thefile=NULL;
	logentry *temp_entry=NULL;
	int regex_i=0, i=0, len=0;
	char *search_regex=NULL;
	regex_t preg;
	logfilter *temp_filter;
	short keep_entry=TRUE;

	error=FALSE;

	if(reverse==TRUE){
		error=read_file_into_lifo(log_file);
		if(error!=LIFO_OK){
			if(error==LIFO_ERROR_MEMORY)
				error=READLOG_ERROR_MEMORY;
			else
				error=READLOG_ERROR;
			reverse=FALSE;
		}
	}

	if(reverse==FALSE){

		if((thefile=mmap_fopen(log_file))==NULL){
			//printf("error opening file: %s",log_file);
			error=READLOG_ERROR_NOFILE;
		}
	}

	if (error!=READLOG_OK)
		return error;

	if(search_string!=NULL){
		/* allocate for 3 extra chars, ^, $ and \0 */
		search_regex = malloc(sizeof(char) * (strlen(search_string) * 2 + 3));
		len=strlen(search_string);
		for (i=0;i<len;i++,regex_i++) {
			if(search_string[i]=='*') {
				search_regex[regex_i++]='.';
				search_regex[regex_i]='*';
			}else
				search_regex[regex_i]=search_string[i];
		}
		//search_regex[0]='^';
		//search_regex[regex_i++]='$';

		search_regex[regex_i]='\0';

		/* check and compile regex */
		if (regcomp(&preg,search_regex,REG_ICASE|REG_NOSUB) != 0 ) {
			regfree(&preg);
			return READLOG_ERROR_FILTER;
		}
	}

	if(error==FALSE){
		
		while(1){
			
			if(reverse==TRUE){
				if((input=pop_lifo())==NULL)
					break;
			}
			else if((input=mmap_fgets(thefile))==NULL)
				break;

			strip(input);
			
			if ((int)strlen(input)==0)
				continue;
			
			/* get timestamp */
			temp_buffer=strtok(input,"]");
			timestamp=(temp_buffer==NULL)?0L:strtoul(temp_buffer+1,NULL,10);

			/* skip line if out of time range */
			if ((ts_start>=0 && timestamp<ts_start) || (ts_end>=0 && timestamp>ts_end))
				continue;

			/* get log entry text */
			temp_buffer=strtok(NULL,"\n");
			
			if(search_string!=NULL){
				if (regexec(&preg,temp_buffer,0,NULL,0)==REG_NOMATCH)
					continue;
			}
			

			if(strstr(temp_buffer," starting..."))
				type=LOGENTRY_STARTUP;
			else if(strstr(temp_buffer," shutting down..."))
				type=LOGENTRY_SHUTDOWN;
			else if(strstr(temp_buffer,"Bailing out"))
				type=LOGENTRY_BAILOUT;
			else if(strstr(temp_buffer," restarting..."))
				type=LOGENTRY_RESTART;
			else if(strstr(temp_buffer,"HOST ALERT:") && strstr(temp_buffer,";DOWN;"))
				type=LOGENTRY_HOST_DOWN;
			else if(strstr(temp_buffer,"HOST ALERT:") && strstr(temp_buffer,";UNREACHABLE;"))
				type=LOGENTRY_HOST_UNREACHABLE;
			else if(strstr(temp_buffer,"HOST ALERT:") && strstr(temp_buffer,";RECOVERY;"))
				type=LOGENTRY_HOST_RECOVERY;
			else if(strstr(temp_buffer,"HOST ALERT:") && strstr(temp_buffer,";UP;"))
				type=LOGENTRY_HOST_UP;
			else if(strstr(temp_buffer,"HOST NOTIFICATION:"))
				type=LOGENTRY_HOST_NOTIFICATION;
			else if(strstr(temp_buffer,"SERVICE ALERT:") && strstr(temp_buffer,";CRITICAL;"))
				type=LOGENTRY_SERVICE_CRITICAL;
			else if(strstr(temp_buffer,"SERVICE ALERT:") && strstr(temp_buffer,";WARNING;"))
				type=LOGENTRY_SERVICE_WARNING;
			else if(strstr(temp_buffer,"SERVICE ALERT:") && strstr(temp_buffer,";UNKNOWN;"))
				type=LOGENTRY_SERVICE_UNKNOWN;
			else if(strstr(temp_buffer,"SERVICE ALERT:") && strstr(temp_buffer,";RECOVERY;"))
				type=LOGENTRY_SERVICE_RECOVERY;
			else if(strstr(temp_buffer,"SERVICE ALERT:") && strstr(temp_buffer,";OK;"))
				type=LOGENTRY_SERVICE_OK;
			else if(strstr(temp_buffer,"SERVICE NOTIFICATION:"))
				type=LOGENTRY_SERVICE_NOTIFICATION;
			else if(strstr(temp_buffer,"SERVICE EVENT HANDLER:"))
				type=LOGENTRY_SERVICE_EVENT_HANDLER;
			else if(strstr(temp_buffer,"HOST EVENT HANDLER:"))
				type=LOGENTRY_HOST_EVENT_HANDLER;
			else if(strstr(temp_buffer,"EXTERNAL COMMAND:"))
				type=LOGENTRY_EXTERNAL_COMMAND;
			else if(strstr(temp_buffer,"PASSIVE SERVICE CHECK:"))
				type=LOGENTRY_PASSIVE_SERVICE_CHECK;
			else if(strstr(temp_buffer,"PASSIVE HOST CHECK:"))
				type=LOGENTRY_PASSIVE_HOST_CHECK;
			else if(strstr(temp_buffer,"LOG ROTATION:"))
				type=LOGENTRY_LOG_ROTATION;
			else if(strstr(temp_buffer,"active mode..."))
				type=LOGENTRY_ACTIVE_MODE;
			else if(strstr(temp_buffer,"standby mode..."))
				type=LOGENTRY_STANDBY_MODE;
			else if(strstr(temp_buffer,"SERVICE FLAPPING ALERT:") && strstr(temp_buffer,";STARTED;"))
				type=LOGENTRY_SERVICE_FLAPPING_STARTED;
			else if(strstr(temp_buffer,"SERVICE FLAPPING ALERT:") && strstr(temp_buffer,";STOPPED;"))
				type=LOGENTRY_SERVICE_FLAPPING_STOPPED;
			else if(strstr(temp_buffer,"SERVICE FLAPPING ALERT:") && strstr(temp_buffer,";DISABLED;"))
				type=LOGENTRY_SERVICE_FLAPPING_DISABLED;
			else if(strstr(temp_buffer,"HOST FLAPPING ALERT:") && strstr(temp_buffer,";STARTED;"))
				type=LOGENTRY_HOST_FLAPPING_STARTED;
			else if(strstr(temp_buffer,"HOST FLAPPING ALERT:") && strstr(temp_buffer,";STOPPED;"))
				type=LOGENTRY_HOST_FLAPPING_STOPPED;
			else if(strstr(temp_buffer,"HOST FLAPPING ALERT:") && strstr(temp_buffer,";DISABLED;"))
				type=LOGENTRY_HOST_FLAPPING_DISABLED;
			else if(strstr(temp_buffer,"SERVICE DOWNTIME ALERT:") && strstr(temp_buffer,";STARTED;"))
				type=LOGENTRY_SERVICE_DOWNTIME_STARTED;
			else if(strstr(temp_buffer,"SERVICE DOWNTIME ALERT:") && strstr(temp_buffer,";STOPPED;"))
				type=LOGENTRY_SERVICE_DOWNTIME_STOPPED;
			else if(strstr(temp_buffer,"SERVICE DOWNTIME ALERT:") && strstr(temp_buffer,";CANCELLED;"))
				type=LOGENTRY_SERVICE_DOWNTIME_CANCELLED;
			else if(strstr(temp_buffer,"HOST DOWNTIME ALERT:") && strstr(temp_buffer,";STARTED;"))
				type=LOGENTRY_HOST_DOWNTIME_STARTED;
			else if(strstr(temp_buffer,"HOST DOWNTIME ALERT:") && strstr(temp_buffer,";STOPPED;"))
				type=LOGENTRY_HOST_DOWNTIME_STOPPED;
			else if(strstr(temp_buffer,"HOST DOWNTIME ALERT:") && strstr(temp_buffer,";CANCELLED;"))
				type=LOGENTRY_HOST_DOWNTIME_CANCELLED;
			else if (strstr(temp_buffer,"INITIAL SERVICE STATE:"))
				type=LOGENTRY_SERVICE_INITIAL_STATE;
			else if (strstr(temp_buffer,"INITIAL HOST STATE:"))
				type=LOGENTRY_HOST_INITIAL_STATE;
			else if (strstr(temp_buffer,"CURRENT SERVICE STATE:"))
				type=LOGENTRY_SERVICE_CURRENT_STATE;
			else if (strstr(temp_buffer,"CURRENT HOST STATE:"))
				type=LOGENTRY_HOST_CURRENT_STATE;
			else if(strstr(temp_buffer,"error executing command"))
				type=LOGENTRY_ERROR_COMMAND_EXECUTION;
			else if(strstr(temp_buffer,"idomod:"))
				type=LOGENTRY_IDOMOD;
			else if(strstr(temp_buffer,"npcdmod:"))
				type=LOGENTRY_NPCDMOD;
			else if(strstr(temp_buffer,"Auto-save of"))
				type=LOGENTRY_AUTOSAVE;
			else if(strstr(temp_buffer,"Warning:"))
				type=LOGENTRY_SYSTEM_WARNING;
			else
				type=LOGENTRY_UNDEFINED;

			
			if(filter_list!=NULL) {
				keep_entry=FALSE;
				for(temp_filter=filter_list;temp_filter!=NULL;temp_filter=temp_filter->next) {
					if(temp_filter->include!=0) {
						if(temp_filter->include==type) {
							keep_entry=TRUE;
							break;
						}
					}
					else if(temp_filter->exclude!=0) {
						if(temp_filter->exclude==type) {
							keep_entry=FALSE;
							break;
						} else
							keep_entry=TRUE;
					}
				}
				if (keep_entry==FALSE)
					continue;
			}

			/* initialzie */
			/* allocate memory for a new log entry */
			temp_entry=(logentry *)malloc(sizeof(logentry));
			if(temp_entry==NULL)
				return READLOG_ERROR_MEMORY;

			temp_entry->timestamp=0L;
			temp_entry->type=0;
			temp_entry->entry_text=NULL;
			temp_entry->next=NULL;


			temp_entry->timestamp=timestamp;
			temp_entry->type=type;
			temp_entry->entry_text=temp_buffer;
			
			temp_entry->next=entry_list;
			entry_list=temp_entry;

			i++;

		}

		if(reverse==FALSE)
			mmap_fclose(thefile);
		else
			free_lifo_memory();

		if(search_string!=NULL)
			regfree(&preg);
	}

	return READLOG_OK;
}


/**
 *   return next log entry
 */
logentry *next_log_entry(void) {
	logentry *temp_entry;
	logentry *next_entry;
	int temp_int=0;
	time_t temp_ts;

	if(entry_list==NULL || entry_list->entry_text==NULL)
		return NULL;

	temp_int=entry_list->type;
	temp_ts=entry_list->timestamp;

	temp_entry=(logentry *)malloc(sizeof(logentry));
	if(temp_entry==NULL)
		return NULL;
	
	temp_entry->entry_text=strndup(entry_list->entry_text,MAX_INPUT_BUFFER);
	temp_entry->type=temp_int;
	temp_entry->timestamp=temp_ts;

	next_entry=entry_list->next;
	entry_list=next_entry;

	return temp_entry;
}

/* frees all memory allocated to log_entries */
void free_log_entries(void){
	logentry *temp_entry;
	logentry *next_entry;

	if(entry_list==NULL)
		return;

	temp_entry=entry_list;
	while(temp_entry!=NULL){
		next_entry=temp_entry->next;
		if(entry_list->entry_text!=NULL)
			my_free(entry_list->entry_text);
		my_free(entry_list);
		temp_entry=next_entry;
	}

	return;
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
	lifo *next_lifo;
	char *buf;

	if(lifo_list==NULL || lifo_list->data==NULL)
		return NULL;

	buf=strdup(lifo_list->data);

	next_lifo=lifo_list->next;

	if(lifo_list->data!=NULL)
		free((void *)lifo_list->data);
	free((void *)lifo_list);

	lifo_list=next_lifo;

	return buf;
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
		else
			fclose(fd);
	}
	else
		fclose(fd);

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
			} else
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
			}else
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
