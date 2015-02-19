/*****************************************************************************
 *
 * LOGGING.H - Header file for logging functions
 *
 * Copyright (c) 2010-2011 Nagios Core Development Team and Community Contributors
 * Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
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
 *
 *****************************************************************************/


#ifndef INCLUDE_logging_h__
#define INCLUDE_logging_h__

#include "objects.h"

/******************* LOGGING TYPES ********************/

#define NSLOG_RUNTIME_ERROR		1
#define NSLOG_RUNTIME_WARNING		2

#define NSLOG_VERIFICATION_ERROR	4
#define NSLOG_VERIFICATION_WARNING	8

#define NSLOG_CONFIG_ERROR		16
#define NSLOG_CONFIG_WARNING		32

#define NSLOG_PROCESS_INFO		64
#define NSLOG_EVENT_HANDLER		128
/*#define NSLOG_NOTIFICATION		256*/	/* NOT USED ANYMORE - CAN BE REUSED */
#define NSLOG_EXTERNAL_COMMAND		512

#define NSLOG_HOST_UP      		1024
#define NSLOG_HOST_DOWN			2048
#define NSLOG_HOST_UNREACHABLE		4096

#define NSLOG_SERVICE_OK		8192
#define NSLOG_SERVICE_UNKNOWN		16384
#define NSLOG_SERVICE_WARNING		32768
#define NSLOG_SERVICE_CRITICAL		65536

#define NSLOG_PASSIVE_CHECK		131072

#define NSLOG_INFO_MESSAGE		262144

#define NSLOG_HOST_NOTIFICATION		524288
#define NSLOG_SERVICE_NOTIFICATION	1048576

/***************** DEBUGGING LEVELS *******************/

#define DEBUGL_ALL                      -1
#define DEBUGL_NONE                     0
#define DEBUGL_FUNCTIONS                1
#define DEBUGL_CONFIG                   2
#define DEBUGL_PROCESS                  4
#define DEBUGL_STATUSDATA               4
#define DEBUGL_RETENTIONDATA            4
#define DEBUGL_EVENTS                   8
#define DEBUGL_CHECKS                   16
#define DEBUGL_IPC                      16
#define DEBUGL_FLAPPING                 16
#define DEBUGL_EVENTHANDLERS            16
#define DEBUGL_PERFDATA                 16
#define DEBUGL_NOTIFICATIONS            32
#define DEBUGL_EVENTBROKER              64
#define DEBUGL_EXTERNALCOMMANDS         128
#define DEBUGL_COMMANDS                 256
#define DEBUGL_DOWNTIME                 512
#define DEBUGL_COMMENTS                 1024
#define DEBUGL_MACROS                   2048

#define DEBUGV_BASIC                    0
#define DEBUGV_MORE                     1
#define DEBUGV_MOST                     2


/**** Logging Functions ****/
/* __printf__ etc. are gnu specific,not usable with cc */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
void logit(int,int,const char *, ...)
	__attribute__((__format__(__printf__, 3, 4)));
int log_debug_info(int,int,const char *,...)
	__attribute__((__format__(__printf__, 3, 4)));
#else
	void logit(int,int,const char *, ...);
	int log_debug_info(int,int,const char *,...);
#endif /* gnu */

#ifndef NSCGI
int write_to_all_logs_with_host_service(char *, unsigned long, host *, service *);
int write_to_log_with_host_service(char *, unsigned long, time_t *, host *, service *);
int write_to_all_logs(char *,unsigned long);            /* writes a string to main log file and syslog facility */
int write_to_log(char *,unsigned long,time_t *);       	/* write a string to the main log file */
int write_to_syslog(char *,unsigned long);             	/* write a string to the syslog facility */
void write_to_console(char *);				/* write a string to the console */
void write_to_logs_and_console(char *, unsigned long, int);	/* write a string to all logs and the console */
int log_service_event(service *);			/* logs a service event */
int log_host_event(host *);				/* logs a host event */
int log_host_states(int,time_t *);	                /* logs initial/current host states */
int log_service_states(int,time_t *);                   /* logs initial/current service states */
int rotate_log_file(time_t);			     	/* rotates the main log file */
int write_log_file_info(time_t *); 			/* records log file/version info */
int open_debug_log(void);
int close_debug_log(void);
FILE *open_log_file(void);
int close_log_file(void);
int fix_log_file_owner(uid_t, gid_t);
int log_level(int, int);

#endif /* !NSCGI */

#endif

