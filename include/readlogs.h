/************************************************************************
 *
 * READLOGS.H - Header file for log reading functions
 *
 * Copyright (c) 1999-2008  Ethan Galstad (egalstad@nagios.org)
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
 ************************************************************************/

#ifndef _READLOGS_H
#define _READLOGS_H

#ifdef __cplusplus
extern "C" {
#endif


/************************* LOG ENTRY TYPES ******************************/

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


/************************* LOG FILTER TYPES ******************************/

#define LOGFILTER_INCLUDE			333
#define LOGFILTER_EXCLUDE			666


/************************** LIFO RETURN CODES  ****************************/

#define LIFO_OK			0
#define LIFO_ERROR_MEMORY	1
#define LIFO_ERROR_FILE		2
#define LIFO_ERROR_DATA		3

/************************** RED LOG ENTRIES RETURN CODES  *****************/

#define READLOG_OK		0
#define READLOG_ERROR		1
#define READLOG_ERROR_MEMORY	2
#define READLOG_ERROR_NOFILE	3
#define READLOG_ERROR_FILTER	4


/*************************** DATA STRUCTURES  *****************************/

/* LIFO data structure */
typedef struct lifo_struct{
	char *data;
	struct lifo_struct *next;
        }lifo;


/* log entry data struct */
typedef struct logentry_struct {
	time_t	timestamp;
	int	type;
	char	*entry_text;
	struct	logentry_struct *next;
	}logentry;

/* log entry filter struct */
typedef struct logentry_filter {
	int	include;
	int	exclude;
	struct	logentry_filter *next;
	}logfilter;


/*************************** FUNCTIONS **************************************/
int read_file_into_lifo(char *);				/* LIFO functions */
void free_lifo_memory(void);
int push_lifo(char *);
char *pop_lifo(void);


int get_log_entries(char *, char *, int, time_t, time_t);			/* for reading and filtering logs */
int add_log_filter(int, int);
logentry *next_log_entry(void);
void free_log_entries(void);

void get_log_archive_to_use(int,char *,int);			/* determines the name of the log archive to use */
void determine_log_rotation_times(int);
int determine_archive_to_use_from_time(time_t);


#ifdef __cplusplus
}
#endif

#endif

