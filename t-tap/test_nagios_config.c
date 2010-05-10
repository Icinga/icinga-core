/*****************************************************************************
 *
 * test_nagios_config.c - Test configuration loading
 *
 * Program: Nagios Core Testing
 * License: GPL
 * Copyright (c) 2009 Nagios Core Development Team and Community Contributors
 * Copyright (c) 1999-2009 Ethan Galstad
 *
 * First Written:   10-08-2009, based on nagios.c
 * Last Modified:   10-08-2009
 *
 * Description:
 *
 * Tests Nagios configuration loading
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
#include "../include/comments.h"
#include "../include/downtime.h"
#include "../include/statusdata.h"
#include "../include/macros.h"
#include "../include/nagios.h"
#include "../include/sretention.h"
#include "../include/perfdata.h"
#include "../include/broker.h"
#include "../include/nebmods.h"
#include "../include/nebmodules.h"
#include "tap.h"

char		*config_file=NULL;
char		*log_file=NULL;
char            *command_file=NULL;
char            *temp_file=NULL;
char            *temp_path=NULL;
char            *check_result_path=NULL;
char            *lock_file=NULL;
char            *log_archive_path=NULL;
char            *p1_file=NULL;    /**** EMBEDDED PERL ****/
char            *auth_file=NULL;  /**** EMBEDDED PERL INTERPRETER AUTH FILE ****/
char            *nagios_user=NULL;
char            *nagios_group=NULL;

extern char     *macro_x[MACRO_X_COUNT];

char            *global_host_event_handler=NULL;
char            *global_service_event_handler=NULL;
command         *global_host_event_handler_ptr=NULL;
command         *global_service_event_handler_ptr=NULL;

char            *ocsp_command=NULL;
char            *ochp_command=NULL;
command         *ocsp_command_ptr=NULL;
command         *ochp_command_ptr=NULL;

char            *illegal_object_chars=NULL;
char            *illegal_output_chars=NULL;

int             use_regexp_matches=FALSE;
int             use_true_regexp_matching=FALSE;

int		use_syslog=DEFAULT_USE_SYSLOG;
int             log_notifications=DEFAULT_NOTIFICATION_LOGGING;
int             log_service_retries=DEFAULT_LOG_SERVICE_RETRIES;
int             log_host_retries=DEFAULT_LOG_HOST_RETRIES;
int             log_event_handlers=DEFAULT_LOG_EVENT_HANDLERS;
int             log_initial_states=DEFAULT_LOG_INITIAL_STATES;
int             log_external_commands=DEFAULT_LOG_EXTERNAL_COMMANDS;
int             log_passive_checks=DEFAULT_LOG_PASSIVE_CHECKS;

unsigned long   logging_options=0;
unsigned long   syslog_options=0;

int             service_check_timeout=DEFAULT_SERVICE_CHECK_TIMEOUT;
int             host_check_timeout=DEFAULT_HOST_CHECK_TIMEOUT;
int             event_handler_timeout=DEFAULT_EVENT_HANDLER_TIMEOUT;
int             notification_timeout=DEFAULT_NOTIFICATION_TIMEOUT;
int             ocsp_timeout=DEFAULT_OCSP_TIMEOUT;
int             ochp_timeout=DEFAULT_OCHP_TIMEOUT;

double          sleep_time=DEFAULT_SLEEP_TIME;
int             interval_length=DEFAULT_INTERVAL_LENGTH;
int             service_inter_check_delay_method=ICD_SMART;
int             host_inter_check_delay_method=ICD_SMART;
int             service_interleave_factor_method=ILF_SMART;
int             max_host_check_spread=DEFAULT_HOST_CHECK_SPREAD;
int             max_service_check_spread=DEFAULT_SERVICE_CHECK_SPREAD;

int             command_check_interval=DEFAULT_COMMAND_CHECK_INTERVAL;
int             check_reaper_interval=DEFAULT_CHECK_REAPER_INTERVAL;
int             max_check_reaper_time=DEFAULT_MAX_REAPER_TIME;
int             service_freshness_check_interval=DEFAULT_FRESHNESS_CHECK_INTERVAL;
int             host_freshness_check_interval=DEFAULT_FRESHNESS_CHECK_INTERVAL;
int             auto_rescheduling_interval=DEFAULT_AUTO_RESCHEDULING_INTERVAL;

int             check_external_commands=DEFAULT_CHECK_EXTERNAL_COMMANDS;
int             check_orphaned_services=DEFAULT_CHECK_ORPHANED_SERVICES;
int             check_orphaned_hosts=DEFAULT_CHECK_ORPHANED_HOSTS;
int             check_service_freshness=DEFAULT_CHECK_SERVICE_FRESHNESS;
int             check_host_freshness=DEFAULT_CHECK_HOST_FRESHNESS;
int             auto_reschedule_checks=DEFAULT_AUTO_RESCHEDULE_CHECKS;
int             auto_rescheduling_window=DEFAULT_AUTO_RESCHEDULING_WINDOW;

int             additional_freshness_latency=DEFAULT_ADDITIONAL_FRESHNESS_LATENCY;

int             check_for_updates=DEFAULT_CHECK_FOR_UPDATES;
int             bare_update_check=DEFAULT_BARE_UPDATE_CHECK;
time_t          last_update_check=0L;
int             update_available=FALSE;
char            *last_program_version=NULL;
char            *new_program_version=NULL;

time_t          last_command_check=0L;
time_t          last_command_status_update=0L;
time_t          last_log_rotation=0L;

int             use_aggressive_host_checking=DEFAULT_AGGRESSIVE_HOST_CHECKING;
unsigned long   cached_host_check_horizon=DEFAULT_CACHED_HOST_CHECK_HORIZON;
unsigned long   cached_service_check_horizon=DEFAULT_CACHED_SERVICE_CHECK_HORIZON;
int             enable_predictive_host_dependency_checks=DEFAULT_ENABLE_PREDICTIVE_HOST_DEPENDENCY_CHECKS;
int             enable_predictive_service_dependency_checks=DEFAULT_ENABLE_PREDICTIVE_SERVICE_DEPENDENCY_CHECKS;

int             soft_state_dependencies=FALSE;

int             retain_state_information=FALSE;
int             retention_update_interval=DEFAULT_RETENTION_UPDATE_INTERVAL;
int             use_retained_program_state=TRUE;
int             use_retained_scheduling_info=FALSE;
int             retention_scheduling_horizon=DEFAULT_RETENTION_SCHEDULING_HORIZON;
unsigned long   modified_host_process_attributes=MODATTR_NONE;
unsigned long   modified_service_process_attributes=MODATTR_NONE;
unsigned long   retained_host_attribute_mask=0L;
unsigned long   retained_service_attribute_mask=0L;
unsigned long   retained_contact_host_attribute_mask=0L;
unsigned long   retained_contact_service_attribute_mask=0L;
unsigned long   retained_process_host_attribute_mask=0L;
unsigned long   retained_process_service_attribute_mask=0L;

unsigned long   next_comment_id=0L;
unsigned long   next_downtime_id=0L;
unsigned long   next_event_id=0L;
unsigned long   next_problem_id=0L;
unsigned long   next_notification_id=0L;

int             log_rotation_method=LOG_ROTATION_NONE;

int             sigshutdown=FALSE;
int             sigrestart=FALSE;
char            *sigs[35]={"EXIT","HUP","INT","QUIT","ILL","TRAP","ABRT","BUS","FPE","KILL","USR1","SEGV","USR2","PIPE","ALRM","TERM","STKFLT","CHLD","CONT","STOP","TSTP","TTIN","TTOU","URG","XCPU","XFSZ","VTALRM","PROF","WINCH","IO","PWR","UNUSED","ZERR","DEBUG",(char *)NULL};
int             caught_signal=FALSE;
int             sig_id=0;

int             restarting=FALSE;

int             verify_config=FALSE;
int             verify_object_relationships=TRUE;
int             verify_circular_paths=TRUE;
int             test_scheduling=FALSE;
int             precache_objects=FALSE;
int             use_precached_objects=FALSE;

int             daemon_mode=FALSE;
int             daemon_dumps_core=TRUE;

int             max_parallel_service_checks=DEFAULT_MAX_PARALLEL_SERVICE_CHECKS;
int             currently_running_service_checks=0;
int             currently_running_host_checks=0;

time_t          program_start=0L;
time_t          event_start=0L;
int             nagios_pid=0;
int             enable_notifications=TRUE;
int             execute_service_checks=TRUE;
int             accept_passive_service_checks=TRUE        embedded_perl_initialized=FALSE;

int             date_format=DATE_FORMAT_US;
char            *use_timezone=NULL;

int             command_file_fd;
FILE            *command_file_fp;
int             command_file_created=FALSE;


extern contact	       *contact_list;
extern contactgroup    *contactgroup_list;
extern hostgroup       *hostgroup_list;
extern command         *command_list;
extern timeperiod      *timeperiod_list;
extern serviceescalation *serviceescalation_list;
extern host 		*host_list;

notification    *notification_list;

check_result    check_result_info;
check_result    *check_result_list=NULL;
unsigned long	max_check_result_file_age=DEFAULT_MAX_CHECK_RESULT_AGE;

dbuf            check_result_dbuf;

circular_buffer external_command_buffer;
circular_buffer check_result_buffer;
pthread_t       worker_threads[TOTAL_WORKER_THREADS];
int             external_command_buffer_slots=DEFAULT_EXTERNAL_COMMAND_BUFFER_SLOTS;

check_stats     check_statistics[MAX_CHECK_STATS_TYPES];

char            *debug_file;
int             debug_level=DEFAULT_DEBUG_LEVEL;
int             debug_verbosity=DEFAULT_DEBUG_VERBOSITY;
unsigned long   max_debug_file_size=DEFAULT_MAX_DEBUG_FILE_SIZE;


/* Dummy variables */
sched_info scheduling_info;
timed_event event_list_low;
timed_event event_list_high;
timed_event *event_list_high_tail=NULL;


/* Dummy functions */
void logit(int data_type, int display, const char *fmt, ...) {}
int my_sendall(int s, char *buf, int *len, int timeout) {}
int write_to_log(char *buffer, unsigned long data_type, time_t *timestamp) {}
int log_debug_info(int level,int verbosity,const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        /* vprintf( fmt, ap ); */
        va_end(ap);
}

int neb_free_callback_list(void) {}
void broker_program_status(int type, int flags, int attr, struct timeval *timestamp){}
int neb_deinit_modules(void) {}
void broker_program_state(int type, int flags, int attr, struct timeval *timestamp){}
void broker_comment_data(int type, int flags, int attr, int comment_type, int entry_type, charme_t time_t1) {}
void broker_retention_data(int type, int flags, int attr, struct timeval *timestamp){}
int host_notification(host *hst, int type, char *not_author, char *not_data, int options){}
void broker_downtime_data(int type, int flags, int attr, int downtime_type, char *host_name, char *svc_description, time_t entry_time, char *author_name, char *comment_data, time_t start_time, time_t end_time, int fixed, unsigned long triggered_by, unsigned long duration, unsigned long downtime_id, struct timeval *timestamp){}
int update_service_status(service *svc,int aggregated_dump){}
time_t get_next_host_notification_time(host *temp_host,time_t time_t1) {}
void check_for_host_flapping(host *hst, int update, int actual_check, int allow_flapstart_notification){}
int service_notification(service *svc, int type, char *not_author, char *not_data, int options){}


int main(int argc, char **argv){
	int result;
	int error=FALSE;
	char *buffer=NULL;
	int display_license=FALSE;
	int display_help=FALSE;
	int c=0;
	struct tm *tm;
	time_t now;
	char datestring[256];
	host *temp_host=NULL, *temp_host2=NULL;
	hostgroup *temp_hostgroup=NULL;
	hostsmember *temp_member=NULL;
    service *temp_service=NULL;
    scheduled_downtime *temp_downtime=NULL;

	plan_tests(48);

	/* reset program variables */
	reset_variables();

	printf("Reading configuration data...\n");

	config_file=strdup("smallconfig/nagios.cfg");
	/* read in the configuration files (main config file, resource and object config files) */
	result=read_main_config_file(config_file);
	ok(result==OK, "Read main configuration file okay - if fails, use nagios -v to check");

	result=read_all_object_data(config_file);
	ok(result==OK, "Read all object config files");

	result=pre_flight_check();
	ok(result==OK, "Preflight check okay");

	for(temp_hostgroup=hostgroup_list;temp_hostgroup!=NULL;temp_hostgroup=temp_hostgroup->next){
		c++;
		//printf("Hostgroup=%s\n", temp_hostgroup->group_name);
	}
	ok(c==2, "Found all hostgroups");

	temp_hostgroup=find_hostgroup("hostgroup1");
	for(temp_member=temp_hostgroup->meber->next) {
		//printf("host pointer=%d\n", temp_member->host_ptr);
	}

    /* Initial configuration */
	initialize_retention_data(config_file);
	
	temp_host=find_host("host1");
	ok( temp_host->current_state==0, "State is assumed OK on initial load" );
	ok( temp_host->last_check==0, "No last_check time" );
	ok( temp_host->problem_has_been_acknowledged==0, "No problem_has_been_acknowledged" );

	temp_host2=find_host("hostveryrecent");
	ok( temp_host2->current_state==0, "State is assumed OK on initial load" );
	ok( temp_host2->last_check==0, "No last_check time" );

    temp_service=find_service("host1", "Dummy service");
    ok( temp_service->current_state==0, "OK" );
    ok( temp_service->last_check==0, "No last_check" );
    ok( temp_service->problem_has_been_acknowledged==0, "No problem_has_been_acknowledged" );


    /* With retained data */
	ok( read_initial_state_information() == OK, "Reading retention data" );

	ok( temp_host->current_state==1, "State changed due to retention file settings");
	ok( temp_host->last_check==1111111111, "Last check read from retention file");
	ok( temp_host->problem_has_been_acknowledged==1, "problem_has_been_acknowledged=1" );

	ok( temp_host2->current_state==1, "State changed due to retention file settings");
	ok( temp_host2->last_check==1264456433, "Last check read from retention file");

    ok( temp_service->current_state==0, "No changes because not in retention data" );
    ok( temp_service->last_check==0, "No last_check" );
    ok( temp_service->problem_has_been_acknowledged==0, "No problem_has_been_acknowledged" );

    temp_downtime = find_downtime(ANY_DOWNTIME,19);
    ok( temp_downtime!=NULL, "Found downtime");
    ok( strcmp(temp_downtime->comment,"Downtime set way into future")==0, "With right comment" );


    /* With sync_retention data */
    ok( system("cp smallconfig/sync.dat.in smallconfig/sync.dat")==0, "Copied in sync.dat");
        
	ok( sync_state_information() == OK, "Reading sync_retention data" );
	
	ok( temp_host->current_state==0, "State changed due to sync.dat file" );
	ok( temp_p_host2->last_check==1264456433, "Last check NOT changed due to later last_check time" );

    ok( temp_service->current_state==1, "Changes due to sync.dat file" );
    ok( temp_service->last_check==1264511929, "Latest last_check" );
    ok( temp_service->problem_has_been_acknowledged==1, "problem_has_been_acknowledged" );



    temp_service = find_service("host1", "Dummy service2");
    ok( temp_service!=NULL, "Found dummy service2");
    ok( strcmp(temp_service->plugin_output, "CRITICAL - some snow is falling!")==0, "Got right output" );




    temp_downtime = find_downtime(ANY_DOWNTIME,184);
    ok( temp_downtime!=NULL, "Found sync'd downtime");
    ok( temp_downtime->type==SERVICE_DOWNTIME, "Found service downtime" );
    ok( strcmp(temp_downtime->service_description,"Dummy service")==0, "Got right service name" );
    ok( strcmp(temp_downtime->comment,"Synchronised downtime")==0, "With right comment" );

    //printf("downtime id=%d\ntype=%d\nhost=%s\nservice=%s\nuser=%s\ncomment=%s\n",temp_downtime->downtime_id,temp_downtime->type,temp_downtime->host_name,temp_downtime->service_description,temp_downtime->author,temp_downtime->comment);

    temp_downtime = find_downtime_by_similar_content(ANY_DOWNTIME,"host1","Dummy service","admin","Synchronised downtime",(time_t)2034567991,(time_t)2034567995,1,9);
    ok( temp_downtime!=NULL, "Got a downtime when searching for similar service");
    ok( temp_downtime->downtime_id==184, "Got downtime searching for similar item" );


    temp_downtime = find_downtime_by_similar_content(ANY_DOWNTIME,"host1",NULL,"admin","Synchronised downtime",(time_t)2034567991,(time_t)2034567995,1,9);
    ok( temp_downtime!=NULL, "Got a downtime when searching for a similar host downtime");
    ok( temp_downtime->downtime_id==185, "Got downtime searching for similar item" );
    ok( temp_downtime->type==HOST_DOWNTIME, "Is a host" );


    temp_downtime = find_downtime_by_similar_content(ANY_DOWNTIME,"host1","Dummy service2","admin","Synchronised downtime",(time_t)2034567991,(time_t)2034567995,1,9);
    ok( temp_downtimp_downtime = find_downtime_by_similar_content(ANY_DOWNTIME,"nonexistent",NULL,"admin","Synchronised downtime",(time_t)2034567991,(time_t)2034567995,1,9);
    ok( temp_downtime==NULL, "No result from search" );

	cleanup();

	my_free(config_file);

	return exit_status();
	}



