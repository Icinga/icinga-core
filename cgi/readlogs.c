/***********************************************************************
 *
 * READLOGS.C - Functions for reading Log files in Icinga CGIs
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
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

/** @file readlogs.c
 *  @brief functions to read and filter log files in normal and reverse order
**/


#include "../include/cgiutils.h"
#include "../include/readlogs.h"

/** @brief file name data struct
 *
 *  structure to hold single file name for file list array
**/
struct file_data {
	char	*file_name;				/**< holds file name */
};

/** @name external vars
    @{ **/
extern char	log_file[MAX_FILENAME_LENGTH];		/**< the full file name of the main icinga log file */
extern char	log_archive_path[MAX_FILENAME_LENGTH];	/**< the full path to the archived log files */
/** @} */


/** @brief sort helper function for icinga logfile sorting
 *  @param [in] a_in file a to compare
 *  @param [in] b_in file b to compare
 *  @return wether file a is newer or older then file b, based on filename
 *	@retval positive or negative
 *  @author Ricardo Bartels
**/
int sort_icinga_logfiles_by_name(const void *a_in, const void *b_in) {
	char date_a[11] = "";
	char date_b[11] = "";

	struct file_data *a = (struct file_data *)a_in;
	struct file_data *b = (struct file_data *)b_in;

	if (a->file_name == NULL || b->file_name == NULL)
		return 0;

	// year
	date_a[0]  = a->file_name[13];
	date_a[1]  = a->file_name[14];
	date_a[2]  = a->file_name[15];
	date_a[3]  = a->file_name[16];
	//month
	date_a[4]  = a->file_name[7];
	date_a[5]  = a->file_name[8];
	// day
	date_a[6]  = a->file_name[10];
	date_a[7]  = a->file_name[11];
	// hour
	date_a[8]  = a->file_name[18];
	date_a[9]  = a->file_name[19];
	date_a[10] = 0;

	// year
	date_b[0]  = b->file_name[13];
	date_b[1]  = b->file_name[14];
	date_b[2]  = b->file_name[15];
	date_b[3]  = b->file_name[16];
	// month
	date_b[4]  = b->file_name[7];
	date_b[5]  = b->file_name[8];
	// day
	date_b[6]  = b->file_name[10];
	date_b[7]  = b->file_name[11];
	// hour
	date_b[8]  = b->file_name[18];
	date_b[9]  = b->file_name[19];
	date_b[10] = 0;

	// return compared values
	return (int)(atoi(date_b) - atoi(date_a));
}

/** @brief Add Filter to the list of log filters
 *  @param [out] filter_list a list of filters of type logfilter struct where requested filter got added
 *  @param [in] requested_filter the id of the log entry you want to filter for
 *  @param [in] include_exclude type of filter
 *	@arg LOGFILTER_INCLUDE
 *	@arg LOGFILTER_EXCLUDE
 *  @return wether adding filter was successful or not (see readlogs.h)
 *	@retval READLOG_OK
 *	@retval READLOG_ERROR_FATAL
 *	@retval READLOG_ERROR_MEMORY
 *  @warning at the moment can be only one type of filters for all elements,
 *	     only include for all OR exclude for all.
 *	     Combining is not available at the moment
 *  @author Ricardo Bartels
 *
 *  With this function you can add filters before reading the actual log entries
 *  from file. This will prevent allocating memory for log entries we don't need
 *  anyway. Be aware that you can use only one type of filtering (include or exclude).
 *  If you want to have all entries except a view, then use the filter with exclude.
 *  If you need just a few defined ones (like in notifications.c) then use include.
 *
 * - LOGFILTER_INCLUDE keeps the log entries specified and throws out the rest
 * - LOGFILTER_EXCLUDE keeps all log entries except the ones which are specified
**/
int add_log_filter(logfilter **new_filter, int requested_filter, int include_exclude) {
	logfilter *temp_filter = NULL;

	temp_filter = (logfilter *)malloc(sizeof(logfilter));
	if (temp_filter == NULL)
		return READLOG_ERROR_MEMORY;

	/* initialize filter */
	temp_filter->next = NULL;
	temp_filter->include = 0;
	temp_filter->exclude = 0;

	if (include_exclude == LOGFILTER_INCLUDE)
		temp_filter->include = requested_filter;
	else if (include_exclude == LOGFILTER_EXCLUDE)
		temp_filter->exclude = requested_filter;
	else {
		my_free(temp_filter);
		return READLOG_ERROR_FATAL;
	}

	if (*new_filter == NULL)
		*new_filter = temp_filter;
	else {
		temp_filter->next = *new_filter;
		*new_filter = temp_filter;
	}

	return READLOG_OK;
}


/** @brief Read's log data for defined timerange and stores the entries into entry_list struct
 *  @param [out] entry_list returns a filled entry list of requested log data
 *  @param [in] filter_list a list of filters of type logfilter struct
 *  @param [out] error_text returns a error string in case of an error execpt on READLOG_ERROR_MEMORY
 *  @param [in] search_string a string you are searching for
 *		Set to NULL to disable search function
 *  @param [in] reverse this bool defines which order the log entries should return
 *  @param [in] ts_start defines the start timestamp for log entries
 *	@arg >=0 means unix timestamp
 *  @param [in] ts_end defines the end timestamp for log entries
 *	@arg >=0 means unix timestamp
 *  @return
 *	@retval READLOG_OK
 *	@retval READLOG_ERROR_WARNING
 *	@retval READLOG_ERROR_FATAL
 *	@retval READLOG_ERROR_MEMORY
 *	@retval READLOG_ERROR_FILTER
 *  @author Ricardo Bartels
 *
 *  This functions reads a  \c log_file and and try's (if set) to filter for a search string.
 *  This search string uses regular expressions. The reverse option defines if you want
 *  have your log entries returned in normal or revers order. Normal order for returning
 *  would be from the newest entry to the oldest. You can also set a time "window". This
 *  defines if you want to exclude entries which are outside of these "window". Then only
 *  entries will be returned which are between start and end. Very useful if user has all
 *  entries in one log file.
**/
int get_log_entries(logentry **entry_list, logfilter **filter_list, char **error_text, char *search_string, int reverse, time_t ts_start, time_t ts_end) {
	char *input = NULL;
	char *temp_buffer = NULL;
	char *search_regex = NULL;
	char log_file_name[MAX_FILENAME_LENGTH];
	char ts_buffer[16];
	int type = 0;
	int regex_i = 0, i = 0, len = 0;
	int file_num = 1;
	int file = 0;
	int in_range = FALSE;
	int return_val = READLOG_OK;
	int data_found = FALSE;
	int dummy;
	short keep_entry = TRUE;
	time_t timestamp = 0L;
	time_t last_timestamp = 0L;
	mmapfile *thefile = NULL;
	logentry *temp_entry = NULL;
	logentry *last_entry = NULL;
	regex_t preg;
	logfilter *temp_filter;
	DIR *dirp;
	struct dirent *dptr;
	struct file_data files[10000];

	/* empty error_text */
	if (*error_text != NULL)
		my_free(*error_text);

	/* bail out if one timestamp is negative */
	if (ts_start < 0 || ts_end < 0) {
		*error_text = strdup("start or end timestamp are invalid. Check submited date information");
		return READLOG_ERROR_FATAL;
	}

	/* check if search_string is set */
	if (search_string != NULL) {

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

		/* check and compile regex, return error on failure */
		if (regcomp(&preg, search_regex, REG_ICASE | REG_NOSUB) != 0) {
			regfree(&preg);
			my_free(search_regex);
			*error_text = strdup("It seems like that reagular expressions don't like what you searched for. Please change your search string.");
			return READLOG_ERROR_FATAL;
		}

		my_free(search_regex);
	}

	/* initialize file data array */
	for (i=0;i<10000;i++)
		files[i].file_name = NULL;

	/* try to open log_archive_path, return if it fails */
	if ((dirp=opendir(log_archive_path)) == NULL){

		if (search_string != NULL)
			regfree(&preg);

		dummy = asprintf(&temp_buffer, "Unable to open \"log_archive_path\" -> \"%s\"!!!", log_archive_path);
		*error_text = strdup(temp_buffer);
		my_free(temp_buffer);

		return READLOG_ERROR_FATAL;

	} else {

		/* read every dir entry */
		while ((dptr=readdir(dirp)) != NULL) {

			/* filter dir for icinga / nagios log files */
			if ((strncmp("icinga-",dptr->d_name,7) == 0 || strncmp("nagios-",dptr->d_name,7) == 0 ) && strstr(dptr->d_name, ".log"))
				files[file_num++].file_name = strdup(dptr->d_name);
		}
		closedir(dirp);
	}

	/* sort log files, newest first */
	qsort(files, sizeof(files) / sizeof(struct file_data) , sizeof(struct file_data), sort_icinga_logfiles_by_name);

	/* define which log files to use */
	for (i=0; i< file_num; i++) {

		/* first log file is always the current log file */
		if (i == 0) {
			strncpy(log_file_name, log_file, sizeof(log_file_name) -1);
			log_file_name[sizeof(log_file_name)-1] = '\x0';

		/* return full path of logfile and store first timestamp of last file */
		} else {
			snprintf(log_file_name, sizeof(log_file_name) -1, "%s%s",log_archive_path, files[i].file_name);
			log_file_name[sizeof(log_file_name)-1] = '\x0';

			last_timestamp = timestamp;
		}

		/* free file entry and set to NULL. if valid file is found, entry gets refilled */
		my_free(files[i].file_name);

		/* we found data and we are out of range again, file must be older then ts_start. stop checking files */
		if (data_found == TRUE && in_range == FALSE)
			continue;

		/* try to open log file, or throw error and try next log file */
		if((file=open(log_file_name, O_RDONLY)) < -1) {

			if (*error_text == NULL) {
				dummy = asprintf(&temp_buffer, "Unable to open log file \"%s\" !!!", log_file_name);
				*error_text = strdup(temp_buffer);
				my_free(temp_buffer);
			}

			return_val = READLOG_ERROR_WARNING;

			continue;
		}

		/* read first 16 bytes to get first timestamp, or throw error if data is not 16 bytes log (empty file) */
		if(read(file,ts_buffer,16) != 16) {

			if (*error_text == NULL) {
				dummy = asprintf(&temp_buffer, "Log file \"%s\" invalid! No timestamp found within first 16 bytes!", log_file_name);
				*error_text = strdup(temp_buffer);
				my_free(temp_buffer);
			}

			return_val = READLOG_ERROR_WARNING;

			close(file);
			continue;
		}

		close(file);

		/* get first timestamp */
		temp_buffer = strtok(ts_buffer, "]");
		timestamp = (temp_buffer == NULL) ? 0L : strtoul(temp_buffer + 1, NULL, 10);


		/* if first (oldest) timestamp in file is newer then ts_end, skip file */
		if (timestamp > ts_end)
			continue;

		in_range = TRUE;

		/* the priviouse file holds range for ts_start */
		if (last_timestamp != 0L && last_timestamp < ts_start)
			in_range = FALSE;

		/* keep file if in range */
		if(in_range == TRUE) {
			files[i].file_name = strdup(log_file_name);
			data_found = TRUE;
		}
	}

	/* read all log files we found earlier in reverse order, starting with the oldest */
	for (i=file_num; i >= 0; i--) {

		/* if file name is empty try next file */
		if (files[i].file_name == NULL)
			continue;

		/* try to open log file */
		if ((thefile = mmap_fopen(files[i].file_name)) == NULL)
			continue;

		while (1) {

			/* free memory */
			my_free(input);

			if ((input = mmap_fgets(thefile)) == NULL)
				break;

			strip(input);

			if ((int)strlen(input) == 0)
				continue;

			/* get timestamp */
			temp_buffer = strtok(input, "]");
			timestamp = (temp_buffer == NULL) ? 0L : strtoul(temp_buffer + 1, NULL, 10);

			/* skip line if out of range */
			if ((ts_end >= 0 && timestamp > ts_end) || (ts_start >= 0 && timestamp < ts_start))
				continue;

			/* get log entry text */
			temp_buffer = strtok(NULL, "\n");

			/* if we search for something, check if it entry matches search_string */
			if (search_string != NULL) {
				if (regexec(&preg, temp_buffer, 0, NULL, 0) == REG_NOMATCH)
					continue;
			}

			/* categorize log entry */
			if (strstr(temp_buffer, " starting..."))
				type = LOGENTRY_STARTUP;
			else if (strstr(temp_buffer, " shutting down..."))
				type = LOGENTRY_SHUTDOWN;
			else if (strstr(temp_buffer, "Bailing out"))
				type = LOGENTRY_BAILOUT;
			else if (strstr(temp_buffer, " restarting..."))
				type = LOGENTRY_RESTART;
			else if (strstr(temp_buffer, "HOST ALERT:") && strstr(temp_buffer, ";DOWN;"))
				type = LOGENTRY_HOST_DOWN;
			else if (strstr(temp_buffer, "HOST ALERT:") && strstr(temp_buffer, ";UNREACHABLE;"))
				type = LOGENTRY_HOST_UNREACHABLE;
			else if (strstr(temp_buffer, "HOST ALERT:") && strstr(temp_buffer, ";RECOVERY;"))
				type = LOGENTRY_HOST_RECOVERY;
			else if (strstr(temp_buffer, "HOST ALERT:") && strstr(temp_buffer, ";UP;"))
				type = LOGENTRY_HOST_UP;
			else if (strstr(temp_buffer, "HOST NOTIFICATION:"))
				type = LOGENTRY_HOST_NOTIFICATION;
			else if (strstr(temp_buffer, "SERVICE ALERT:") && strstr(temp_buffer, ";CRITICAL;"))
				type = LOGENTRY_SERVICE_CRITICAL;
			else if (strstr(temp_buffer, "SERVICE ALERT:") && strstr(temp_buffer, ";WARNING;"))
				type = LOGENTRY_SERVICE_WARNING;
			else if (strstr(temp_buffer, "SERVICE ALERT:") && strstr(temp_buffer, ";UNKNOWN;"))
				type = LOGENTRY_SERVICE_UNKNOWN;
			else if (strstr(temp_buffer, "SERVICE ALERT:") && strstr(temp_buffer, ";RECOVERY;"))
				type = LOGENTRY_SERVICE_RECOVERY;
			else if (strstr(temp_buffer, "SERVICE ALERT:") && strstr(temp_buffer, ";OK;"))
				type = LOGENTRY_SERVICE_OK;
			else if (strstr(temp_buffer, "SERVICE NOTIFICATION:"))
				type = LOGENTRY_SERVICE_NOTIFICATION;
			else if (strstr(temp_buffer, "SERVICE EVENT HANDLER:"))
				type = LOGENTRY_SERVICE_EVENT_HANDLER;
			else if (strstr(temp_buffer, "HOST EVENT HANDLER:"))
				type = LOGENTRY_HOST_EVENT_HANDLER;
			else if (strstr(temp_buffer, "EXTERNAL COMMAND:"))
				type = LOGENTRY_EXTERNAL_COMMAND;
			else if (strstr(temp_buffer, "PASSIVE SERVICE CHECK:"))
				type = LOGENTRY_PASSIVE_SERVICE_CHECK;
			else if (strstr(temp_buffer, "PASSIVE HOST CHECK:"))
				type = LOGENTRY_PASSIVE_HOST_CHECK;
			else if (strstr(temp_buffer, "LOG ROTATION:"))
				type = LOGENTRY_LOG_ROTATION;
			else if (strstr(temp_buffer, "active mode..."))
				type = LOGENTRY_ACTIVE_MODE;
			else if (strstr(temp_buffer, "standby mode..."))
				type = LOGENTRY_STANDBY_MODE;
			else if (strstr(temp_buffer, "SERVICE FLAPPING ALERT:") && strstr(temp_buffer, ";STARTED;"))
				type = LOGENTRY_SERVICE_FLAPPING_STARTED;
			else if (strstr(temp_buffer, "SERVICE FLAPPING ALERT:") && strstr(temp_buffer, ";STOPPED;"))
				type = LOGENTRY_SERVICE_FLAPPING_STOPPED;
			else if (strstr(temp_buffer, "SERVICE FLAPPING ALERT:") && strstr(temp_buffer, ";DISABLED;"))
				type = LOGENTRY_SERVICE_FLAPPING_DISABLED;
			else if (strstr(temp_buffer, "HOST FLAPPING ALERT:") && strstr(temp_buffer, ";STARTED;"))
				type = LOGENTRY_HOST_FLAPPING_STARTED;
			else if (strstr(temp_buffer, "HOST FLAPPING ALERT:") && strstr(temp_buffer, ";STOPPED;"))
				type = LOGENTRY_HOST_FLAPPING_STOPPED;
			else if (strstr(temp_buffer, "HOST FLAPPING ALERT:") && strstr(temp_buffer, ";DISABLED;"))
				type = LOGENTRY_HOST_FLAPPING_DISABLED;
			else if (strstr(temp_buffer, "SERVICE DOWNTIME ALERT:") && strstr(temp_buffer, ";STARTED;"))
				type = LOGENTRY_SERVICE_DOWNTIME_STARTED;
			else if (strstr(temp_buffer, "SERVICE DOWNTIME ALERT:") && strstr(temp_buffer, ";STOPPED;"))
				type = LOGENTRY_SERVICE_DOWNTIME_STOPPED;
			else if (strstr(temp_buffer, "SERVICE DOWNTIME ALERT:") && strstr(temp_buffer, ";CANCELLED;"))
				type = LOGENTRY_SERVICE_DOWNTIME_CANCELLED;
			else if (strstr(temp_buffer, "HOST DOWNTIME ALERT:") && strstr(temp_buffer, ";STARTED;"))
				type = LOGENTRY_HOST_DOWNTIME_STARTED;
			else if (strstr(temp_buffer, "HOST DOWNTIME ALERT:") && strstr(temp_buffer, ";STOPPED;"))
				type = LOGENTRY_HOST_DOWNTIME_STOPPED;
			else if (strstr(temp_buffer, "HOST DOWNTIME ALERT:") && strstr(temp_buffer, ";CANCELLED;"))
				type = LOGENTRY_HOST_DOWNTIME_CANCELLED;
			else if (strstr(temp_buffer, "INITIAL SERVICE STATE:"))
				type = LOGENTRY_SERVICE_INITIAL_STATE;
			else if (strstr(temp_buffer, "INITIAL HOST STATE:"))
				type = LOGENTRY_HOST_INITIAL_STATE;
			else if (strstr(temp_buffer, "CURRENT SERVICE STATE:"))
				type = LOGENTRY_SERVICE_CURRENT_STATE;
			else if (strstr(temp_buffer, "CURRENT HOST STATE:"))
				type = LOGENTRY_HOST_CURRENT_STATE;
			else if (strstr(temp_buffer, "error executing command"))
				type = LOGENTRY_ERROR_COMMAND_EXECUTION;
			else if (strstr(temp_buffer, "idomod:"))
				type = LOGENTRY_IDOMOD;
			else if (strstr(temp_buffer, "npcdmod:"))
				type = LOGENTRY_NPCDMOD;
			else if (strstr(temp_buffer, "Auto-save of"))
				type = LOGENTRY_AUTOSAVE;
			else if (strstr(temp_buffer, "Warning:"))
				type = LOGENTRY_SYSTEM_WARNING;
			else
				type = LOGENTRY_UNDEFINED;

			/* apply filters */
			if (*filter_list != NULL) {
				keep_entry = FALSE;
				for (temp_filter = *filter_list; temp_filter != NULL; temp_filter = temp_filter->next) {
					if (temp_filter->include != 0) {
						if (temp_filter->include == type) {
							keep_entry = TRUE;
							break;
						}
					} else if (temp_filter->exclude != 0) {
						if (temp_filter->exclude == type) {
							keep_entry = FALSE;
							break;
						} else
							keep_entry = TRUE;
					}
				}
				if (keep_entry == FALSE)
					continue;
			}

			/* initialzie */
			/* allocate memory for a new log entry */
			temp_entry = (logentry *)malloc(sizeof(logentry));
			if (temp_entry == NULL) {

				mmap_fclose(thefile);
				return READLOG_ERROR_MEMORY;
			}

			temp_entry->timestamp = 0L;
			temp_entry->type = 0;
			temp_entry->entry_text = NULL;
			temp_entry->next = NULL;


			temp_entry->timestamp = timestamp;
			temp_entry->type = type;
			temp_entry->entry_text = strdup(temp_buffer);

			if (reverse == TRUE) {
				if (*entry_list == NULL) {
					*entry_list = temp_entry;
					last_entry = *entry_list;
				} else {
					last_entry->next = temp_entry;
					last_entry = temp_entry;
				}
			} else {
				temp_entry->next = *entry_list;
				*entry_list = temp_entry;
			}
		}

		mmap_fclose(thefile);
	}

	for (i=0; i< file_num;i++)
		my_free(files[i].file_name);

	if (search_string != NULL)
		regfree(&preg);

	return return_val;
}

/** @brief frees all memory allocated to list of log filters in memory
 *  @author Ricardo Bartels
**/
void free_log_filters(logfilter **filter_list) {
	logfilter *temp_filter = NULL;
	logfilter *next_filter = NULL;

	for (temp_filter = *filter_list; temp_filter != NULL;) {
		next_filter = temp_filter->next;
		my_free(temp_filter);
		temp_filter = next_filter;
	}

	*filter_list = NULL;

	return;
}

/** @brief frees all memory allocated to list of log entries in memory
 *  @author Ricardo Bartels
**/
void free_log_entries(logentry **entry_list) {
	logentry *temp_entry;
	logentry *next_entry;

	for (temp_entry = *entry_list; temp_entry != NULL;) {
		next_entry = temp_entry->next;
		if (temp_entry->entry_text != NULL)
			my_free(temp_entry->entry_text);
		my_free(temp_entry);
		temp_entry = next_entry;
	}

	*entry_list = NULL;

	return;
}
