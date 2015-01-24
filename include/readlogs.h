/************************************************************************
 *
 * READLOGS.H - Header file for log reading functions
 *
 * Copyright (c) 1999-2008  Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2015 Icinga Development Team (http://www.icinga.org) 
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
 ************************************************************************/

/** @file readlogs.h
 *  @brief defines and structures which are used in combination with functions from readlogs.c
**/

#ifndef _READLOGS_H
#define _READLOGS_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @name LOG ENTRY TYPES
 @{**/
#define LOGENTRY_STARTUP			1
#define LOGENTRY_SHUTDOWN			2
#define LOGENTRY_RESTART			3
#define LOGENTRY_BAILOUT			4
#define LOGENTRY_LOG_ROTATION			5
#define LOGENTRY_ACTIVE_MODE			6
#define LOGENTRY_STANDBY_MODE			7
#define LOGENTRY_EXTERNAL_COMMAND		8
#define LOGENTRY_ERROR_COMMAND_EXECUTION	9

#define LOGENTRY_HOST_DOWN			10
#define LOGENTRY_HOST_UNREACHABLE		11
#define LOGENTRY_HOST_RECOVERY			12
#define LOGENTRY_HOST_UP			13
#define LOGENTRY_HOST_NOTIFICATION		14
#define LOGENTRY_HOST_EVENT_HANDLER		15

#define LOGENTRY_HOST_FLAPPING_STARTED		16
#define LOGENTRY_HOST_FLAPPING_STOPPED		17
#define LOGENTRY_HOST_FLAPPING_DISABLED		18
#define LOGENTRY_HOST_DOWNTIME_STARTED		19
#define LOGENTRY_HOST_DOWNTIME_STOPPED		20
#define LOGENTRY_HOST_DOWNTIME_CANCELLED	21

#define LOGENTRY_HOST_INITIAL_STATE		22
#define LOGENTRY_HOST_CURRENT_STATE		23

#define LOGENTRY_PASSIVE_HOST_CHECK		24

#define LOGENTRY_SERVICE_CRITICAL		25
#define LOGENTRY_SERVICE_WARNING		26
#define LOGENTRY_SERVICE_UNKNOWN		27
#define LOGENTRY_SERVICE_RECOVERY		28
#define LOGENTRY_SERVICE_OK			29
#define LOGENTRY_SERVICE_NOTIFICATION		30
#define LOGENTRY_SERVICE_EVENT_HANDLER		31

#define LOGENTRY_SERVICE_FLAPPING_STARTED	32
#define LOGENTRY_SERVICE_FLAPPING_STOPPED	33
#define LOGENTRY_SERVICE_FLAPPING_DISABLED	34
#define LOGENTRY_SERVICE_DOWNTIME_STARTED	35
#define LOGENTRY_SERVICE_DOWNTIME_STOPPED	36
#define LOGENTRY_SERVICE_DOWNTIME_CANCELLED	37

#define LOGENTRY_SERVICE_INITIAL_STATE		38
#define LOGENTRY_SERVICE_CURRENT_STATE		39

#define LOGENTRY_PASSIVE_SERVICE_CHECK		40

#define LOGENTRY_IDOMOD				41
#define LOGENTRY_NPCDMOD			42
#define LOGENTRY_AUTOSAVE			43
#define LOGENTRY_SYSTEM_WARNING			44

#define LOGENTRY_UNDEFINED			999
/** @}*/

/** @name LOG FILTER TYPES
 @{**/
#define LOGFILTER_INCLUDE			333
#define LOGFILTER_EXCLUDE			666
/** @}*/


/** @name READ LOG ENTRIES RETURN CODES
 @{**/

#define READLOG_OK		0
#define READLOG_ERROR_WARNING	1
#define READLOG_ERROR_FATAL	2
#define READLOG_ERROR_MEMORY	3
#define READLOG_ERROR_FILTER	4
/** @}*/


/** @brief log entry data struct
 *
 *  structure to hold single log entries @ref get_log_entries
**/
typedef struct logentry_struct {
	time_t	timestamp;		/**< timestamp of log entry date */
	int	type;			/**< type of log entry -> LOG ENTRY TYPES */
	char	*entry_text;		/**< the log text */
	struct	logentry_struct *next;	/**< next logentry_struct */
	}logentry;

/** @brief log entry filter struct
 *
 *  structure to hold log file filters @ref add_log_filter
**/
typedef struct logentry_filter {
	int	include;		/**< type of log entry which should be included -> LOG ENTRY TYPES */
	int	exclude;		/**< type of log entry which should be excluded -> LOG ENTRY TYPES */
	struct	logentry_filter *next;	/**< next logentry_filter */
	}logfilter;

/* for documentation on these functions see cgi/readlogs.c */
/** @name log reading
    @{ **/
int sort_icinga_logfiles_by_name(const void *a_in, const void *b_in);
int add_log_filter(logfilter **filter_list, int requested_filter, int include_exclude);
int get_log_entries(logentry **entry_list, logfilter **filter_list, char **error_text, char *search_string, int reverse, time_t ts_start, time_t ts_end);
void free_log_filters(logfilter **filter_list);
void free_log_entries(logentry **entry_list);
time_t get_backtrack_seconds(int backtrack_archives);
/**@}*/

#ifdef __cplusplus
}
#endif

#endif

