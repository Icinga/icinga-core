/************************************************************************
 *
 * IDO2DB.H - IDO2DB Include File
 * Copyright (c) 2005-2007 Ethan Galstad
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 ************************************************************************/

#ifndef _IDO2DB_IDO2DB_H
#define _IDO2DB_IDO2DB_H

#include "../../../include/config.h"
#include "utils.h"

/*************** RDBMS headers *************/

/* oracle */
#ifdef USE_ORACLE

#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#include <ocilib.h>
#define HAVE_CONFIG_H
#else
#include <ocilib.h>
#endif

#endif

/* pgsql */
#ifdef HAVE_LIBPQ_FE_H
#include <libpq-fe.h>
#endif

/* libdbi */
#ifdef HAVE_DBI_DBI_H
#include <dbi/dbi.h>
#endif


/*************** mbuf definitions *************/
#define IDO2DB_MBUF_CONTACTGROUP                        0
#define IDO2DB_MBUF_CONTACTGROUPMEMBER                  1
#define IDO2DB_MBUF_SERVICEGROUPMEMBER                  2
#define IDO2DB_MBUF_HOSTGROUPMEMBER                     3
#define IDO2DB_MBUF_SERVICENOTIFICATIONCOMMAND          4
#define IDO2DB_MBUF_HOSTNOTIFICATIONCOMMAND             5
#define IDO2DB_MBUF_CONTACTADDRESS                      6
#define IDO2DB_MBUF_TIMERANGE                           7
#define IDO2DB_MBUF_PARENTHOST                          8
#define IDO2DB_MBUF_CONFIGFILEVARIABLE                  9
#define IDO2DB_MBUF_CONFIGVARIABLE                      10
#define IDO2DB_MBUF_RUNTIMEVARIABLE                     11
#define IDO2DB_MBUF_CUSTOMVARIABLE                      12
#define IDO2DB_MBUF_CONTACT                             13

#define IDO2DB_MAX_MBUF_ITEMS                           14

#define IDO2DB_MAX_BUFLEN				16384
#define IDO2DB_MYSQL_MAX_TEXT_LEN		32768


/***************** structures *****************/

typedef struct ido2db_mbuf_struct{
	int used_lines;
	int allocated_lines;
	char **buffer;
        }ido2db_mbuf;


typedef struct ido2db_dbobject_struct{
	char *name1;
	char *name2;
	int object_type;
	/* ToDo Change object_id to unsigned long long */
	unsigned long object_id;
	struct ido2db_dbobject_struct *nexthash;
        }ido2db_dbobject;


typedef struct ido2db_dbconninfo_struct{
	int server_type;
	int connected;
	int error;
#ifdef USE_LIBDBI /* libdbi specific */
	dbi_conn dbi_conn;
	dbi_result dbi_result;
#endif

#ifdef USE_PGSQL /* pgsql specific */
	PGconn *pg_conn;
	PGresult *pg_result;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	OCI_Connection* oci_connection;
	OCI_Statement* oci_statement;
	OCI_Resultset* oci_resultset;
	/* for bind params keep prepared statements */
	OCI_Statement* oci_statement_objects_insert;
	OCI_Statement* oci_statement_logentries_insert;
	OCI_Statement* oci_statement_startup_clean;
	OCI_Statement* oci_statement_timedevents;
	OCI_Statement* oci_statement_timedevents_queue;
	OCI_Statement* oci_statement_timedeventqueue;
	OCI_Statement* oci_statement_hostchecks;
	OCI_Statement* oci_statement_hoststatus;
	OCI_Statement* oci_statement_servicechecks;
	OCI_Statement* oci_statement_servicestatus;
	OCI_Statement* oci_statement_contact_notificationcommands;
	OCI_Statement* oci_statement_programstatus;
	OCI_Statement* oci_statement_systemcommanddata;
	OCI_Statement* oci_statement_eventhandlerdata;
	OCI_Statement* oci_statement_notificationdata;
	OCI_Statement* oci_statement_contactnotificationdata;
	OCI_Statement* oci_statement_contactnotificationmethoddata;
	OCI_Statement* oci_statement_commentdata;
	OCI_Statement* oci_statement_commentdata_history;
	OCI_Statement* oci_statement_downtimedata_scheduled_downtime;
	OCI_Statement* oci_statement_downtimedata_downtime_history;
	OCI_Statement* oci_statement_contactstatusdata;
	OCI_Statement* oci_statement_configfilevariables;
	OCI_Statement* oci_statement_configfilevariables_insert;
	OCI_Statement* oci_statement_runtimevariables;
	OCI_Statement* oci_statement_hostdefinition_definition;
	OCI_Statement* oci_statement_hostdefinition_parenthosts;
	OCI_Statement* oci_statement_hostdefinition_contactgroups;
	OCI_Statement* oci_statement_hostdefinition_contacts;
	OCI_Statement* oci_statement_hostgroupdefinition_definition;
	OCI_Statement* oci_statement_hostgroupdefinition_hostgroupmembers;
	OCI_Statement* oci_statement_servicedefinition_definition;
	OCI_Statement* oci_statement_servicedefinition_contactgroups;
	OCI_Statement* oci_statement_servicedefinition_contacts;
	OCI_Statement* oci_statement_servicegroupdefinition_definition;
	OCI_Statement* oci_statement_servicegroupdefinition_members;
	OCI_Statement* oci_statement_hostdependencydefinition_definition;
	OCI_Statement* oci_statement_servicedependencydefinition_definition;
	OCI_Statement* oci_statement_hostescalationdefinition_definition;
	OCI_Statement* oci_statement_hostescalationdefinition_contactgroups;
	OCI_Statement* oci_statement_hostescalationdefinition_contacts;
	OCI_Statement* oci_statement_serviceescalationdefinition_definition;
	OCI_Statement* oci_statement_serviceescalationdefinition_contactgroups;
	OCI_Statement* oci_statement_serviceescalationdefinition_contacts;
	OCI_Statement* oci_statement_commanddefinition_definition;
	OCI_Statement* oci_statement_timeperiodefinition_definition;
	OCI_Statement* oci_statement_timeperiodefinition_timeranges;
	OCI_Statement* oci_statement_contactdefinition_definition;
	OCI_Statement* oci_statement_contactdefinition_addresses;
	OCI_Statement* oci_statement_contactdefinition_servicenotificationcommands;
	OCI_Statement* oci_statement_save_custom_variables_customvariables;
	OCI_Statement* oci_statement_save_custom_variables_customvariablestatus;
	OCI_Statement* oci_statement_contactgroupdefinition_definition;
	OCI_Statement* oci_statement_contactgroupdefinition_contactgroupmembers;
	OCI_Statement* oci_statement_process_events;
	OCI_Statement* oci_statement_flappinghistory;
	OCI_Statement* oci_statement_external_commands;
	OCI_Statement* oci_statement_acknowledgements;
	OCI_Statement* oci_statement_statehistory;
	OCI_Statement* oci_statement_instances;
	OCI_Statement* oci_statement_conninfo;
	/* retrieve object ids by name */
	OCI_Statement* oci_statement_objects_select_name1_name2;
	OCI_Statement* oci_statement_objects_select_cached;
	/* update */
	OCI_Statement* oci_statement_objects_update_inactive;
	OCI_Statement* oci_statement_objects_update_active;
	OCI_Statement* oci_statement_object_enable_disable;
	OCI_Statement* oci_statement_programstatus_update;
	OCI_Statement* oci_statement_timedevents_update;
	OCI_Statement* oci_statement_comment_history_update;
	OCI_Statement* oci_statement_downtimehistory_update_start;
	OCI_Statement* oci_statement_scheduleddowntime_update_start;
	OCI_Statement* oci_statement_downtimehistory_update_stop;
	OCI_Statement* oci_statement_conninfo_update;
	OCI_Statement* oci_statement_conninfo_update_checkin;
	/* select */
	OCI_Statement* oci_statement_logentries_select;
	OCI_Statement* oci_statement_instances_select;

	/* delete */
	OCI_Statement* oci_statement_timedeventqueue_delete;
	OCI_Statement* oci_statement_timedeventqueue_delete_more;
	OCI_Statement* oci_statement_comments_delete;
	OCI_Statement* oci_statement_downtime_delete;
	OCI_Statement* oci_statement_instances_delete;
	OCI_Statement* oci_statement_instances_delete_time;

	/* dbversion */
	OCI_Statement* oci_statement_dbversion_select;

        /* SLA */
	OCI_Statement* oci_statement_sla_services_select;
	OCI_Statement* oci_statement_sla_downtime_select;
	OCI_Statement* oci_statement_sla_history_select;
	OCI_Statement* oci_statement_sla_history_merge;
	OCI_Statement* oci_statement_sla_history_delete;

#endif /* Oracle ocilib specific */
	unsigned long instance_id;
	unsigned long conninfo_id;
	time_t latest_program_status_time;
	time_t latest_host_status_time;
	time_t latest_service_status_time;
	time_t latest_contact_status_time;
	time_t latest_queued_event_time;
	time_t latest_realtime_data_time;
	time_t latest_comment_time;
	int clean_event_queue;
	unsigned long last_notification_id;
	unsigned long last_contact_notification_id;
	unsigned long max_timedevents_age;
	unsigned long max_systemcommands_age;
	unsigned long max_servicechecks_age;
	unsigned long max_hostchecks_age;
	unsigned long max_eventhandlers_age;
	unsigned long max_externalcommands_age;
	unsigned long max_logentries_age;
	unsigned long max_acknowledgements_age;
	unsigned long max_notifications_age;
	unsigned long max_contactnotifications_age;
	unsigned long max_contactnotificationmethods_age;
	unsigned long trim_db_interval;
	unsigned long housekeeping_thread_startup_delay;
	unsigned long clean_realtime_tables_on_core_startup;
	unsigned long clean_config_tables_on_core_startup;
	unsigned long oci_errors_to_syslog;
	time_t last_table_trim_time;
	time_t last_logentry_time;
	char *last_logentry_data;
	char *dbversion;
	ido2db_dbobject **object_hashlist;
        }ido2db_dbconninfo;


typedef struct ido2db_input_data_info_struct{
	int protocol_version;
	int disconnect_client;
	int ignore_client_data;
	char *instance_name;
	char *agent_name;
	char *agent_version;
	char *disposition;
	char *connect_source;
	char *connect_type;
	int current_input_section;
	int current_input_data;
	/* ToDo change *_processed  to unsigned long long */
	unsigned long bytes_processed;
	unsigned long lines_processed;
	unsigned long entries_processed;
	unsigned long data_start_time;
	unsigned long data_end_time;
	int current_object_config_type;
	char **buffered_input;
	ido2db_mbuf mbuf[IDO2DB_MAX_MBUF_ITEMS];
	ido2db_dbconninfo dbinfo;
        }ido2db_idi;



/*************** DB server types ***************/
#define IDO2DB_DBSERVER_NONE                            0
#define IDO2DB_DBSERVER_MYSQL                           1
#define IDO2DB_DBSERVER_PGSQL                           2
#define IDO2DB_DBSERVER_DB2                             3
#define IDO2DB_DBSERVER_FIREBIRD                        4
#define IDO2DB_DBSERVER_FREETDS                         5
#define IDO2DB_DBSERVER_INGRES                          6
#define IDO2DB_DBSERVER_MSQL                            7
#define IDO2DB_DBSERVER_ORACLE                          8
#define IDO2DB_DBSERVER_SQLITE                          9
#define IDO2DB_DBSERVER_SQLITE3                         10

/*************** DBI Driver names **************/
#define IDO2DB_DBI_DRIVER_MYSQL				"mysql"
#define IDO2DB_DBI_DRIVER_PGSQL				"pgsql"
#define IDO2DB_DBI_DRIVER_DB2				"db2l"
#define IDO2DB_DBI_DRIVER_FIREBIRD			"firebird"
#define IDO2DB_DBI_DRIVER_FREETDS			"freetds"
#define IDO2DB_DBI_DRIVER_INGRES			"ingres"
#define IDO2DB_DBI_DRIVER_MSQL				"msql"
#define IDO2DB_DBI_DRIVER_ORACLE			"Oracle"
#define IDO2DB_DBI_DRIVER_SQLITE			"sqlite"
#define IDO2DB_DBI_DRIVER_SQLITE3			"sqlite3"

/*************** misc definitions **************/
#define IDO2DB_INPUT_BUFFER                             1024
/* #define IDO2DB_OBJECT_HASHSLOTS                         1024 */
#define IDO2DB_OBJECT_HASHSLOTS                         50240	/* Altinity patch: Spread the list of linked lists thinner */


/*********** types of input sections ***********/
#define IDO2DB_INPUT_SECTION_NONE                       0
#define IDO2DB_INPUT_SECTION_HEADER                     1
#define IDO2DB_INPUT_SECTION_FOOTER                     2
#define IDO2DB_INPUT_SECTION_DATA                       3


/************* types of input data *************/
#define IDO2DB_INPUT_DATA_NONE                          0

#define IDO2DB_INPUT_DATA_CONFIGDUMPSTART               1
#define IDO2DB_INPUT_DATA_CONFIGDUMPEND                 2

#define IDO2DB_INPUT_DATA_LOGENTRY                      10

#define IDO2DB_INPUT_DATA_PROCESSDATA                   20
#define IDO2DB_INPUT_DATA_TIMEDEVENTDATA                21
#define IDO2DB_INPUT_DATA_LOGDATA                       22
#define IDO2DB_INPUT_DATA_SYSTEMCOMMANDDATA             23
#define IDO2DB_INPUT_DATA_EVENTHANDLERDATA              24
#define IDO2DB_INPUT_DATA_NOTIFICATIONDATA              25
#define IDO2DB_INPUT_DATA_SERVICECHECKDATA              26
#define IDO2DB_INPUT_DATA_HOSTCHECKDATA                 27
#define IDO2DB_INPUT_DATA_COMMENTDATA                   28
#define IDO2DB_INPUT_DATA_DOWNTIMEDATA                  29
#define IDO2DB_INPUT_DATA_FLAPPINGDATA                  30
#define IDO2DB_INPUT_DATA_PROGRAMSTATUSDATA             31
#define IDO2DB_INPUT_DATA_HOSTSTATUSDATA                32
#define IDO2DB_INPUT_DATA_SERVICESTATUSDATA             33
#define IDO2DB_INPUT_DATA_ADAPTIVEPROGRAMDATA           34
#define IDO2DB_INPUT_DATA_ADAPTIVEHOSTDATA              35
#define IDO2DB_INPUT_DATA_ADAPTIVESERVICEDATA           36
#define IDO2DB_INPUT_DATA_EXTERNALCOMMANDDATA           37
#define IDO2DB_INPUT_DATA_AGGREGATEDSTATUSDATA          38
#define IDO2DB_INPUT_DATA_RETENTIONDATA                 39
#define IDO2DB_INPUT_DATA_CONTACTNOTIFICATIONDATA       40
#define IDO2DB_INPUT_DATA_CONTACTNOTIFICATIONMETHODDATA 41
#define IDO2DB_INPUT_DATA_ACKNOWLEDGEMENTDATA           42
#define IDO2DB_INPUT_DATA_STATECHANGEDATA               43
#define IDO2DB_INPUT_DATA_CONTACTSTATUSDATA             44
#define IDO2DB_INPUT_DATA_ADAPTIVECONTACTDATA           45

#define IDO2DB_INPUT_DATA_MAINCONFIGFILEVARIABLES       50
#define IDO2DB_INPUT_DATA_RESOURCECONFIGFILEVARIABLES   51
#define IDO2DB_INPUT_DATA_CONFIGVARIABLES               52
#define IDO2DB_INPUT_DATA_RUNTIMEVARIABLES              53

#define IDO2DB_INPUT_DATA_HOSTDEFINITION                61
#define IDO2DB_INPUT_DATA_HOSTGROUPDEFINITION           62
#define IDO2DB_INPUT_DATA_SERVICEDEFINITION             63
#define IDO2DB_INPUT_DATA_SERVICEGROUPDEFINITION        64
#define IDO2DB_INPUT_DATA_HOSTDEPENDENCYDEFINITION      65
#define IDO2DB_INPUT_DATA_SERVICEDEPENDENCYDEFINITION   66
#define IDO2DB_INPUT_DATA_HOSTESCALATIONDEFINITION      67
#define IDO2DB_INPUT_DATA_SERVICEESCALATIONDEFINITION   68
#define IDO2DB_INPUT_DATA_COMMANDDEFINITION             69
#define IDO2DB_INPUT_DATA_TIMEPERIODDEFINITION          70
#define IDO2DB_INPUT_DATA_CONTACTDEFINITION             71
#define IDO2DB_INPUT_DATA_CONTACTGROUPDEFINITION        72
#define IDO2DB_INPUT_DATA_HOSTEXTINFODEFINITION         73
#define IDO2DB_INPUT_DATA_SERVICEEXTINFODEFINITION      74

#define IDO2DB_INPUT_DATA_ENABLEOBJECT		        100
#define IDO2DB_INPUT_DATA_DISABLEOBJECT			101

/************* types of config data ************/
#define IDO2DB_CONFIGTYPE_ORIGINAL                      0
#define IDO2DB_CONFIGTYPE_RETAINED                      1



/************* debugging levels ****************/

#define IDO2DB_DEBUGL_ALL                      -1
#define IDO2DB_DEBUGL_NONE                     0
#define IDO2DB_DEBUGL_PROCESSINFO              1
#define IDO2DB_DEBUGL_SQL                      2

#define IDO2DB_DEBUGV_BASIC                    0
#define IDO2DB_DEBUGV_MORE		       1
#define IDO2DB_DEBUGV_MOST                     2

/************* default trim db interval ********/

#define DEFAULT_TRIM_DB_INTERVAL		3600

/* default housekeeping thread startup delay  **/

#define DEFAULT_HOUSEKEEPING_THREAD_STARTUP_DELAY 300

/************* oci errors to syslog ************/

#define DEFAULT_OCI_ERRORS_TO_SYSLOG 		1

/************* oracle trace level defaults ************/
#define DEFAULT_ORACLE_TRACE_LEVEL		4
#define ORACLE_TRACE_LEVEL_OFF 			0
/************* n worker threads ****************/

#define IDO2DB_CLEANER_THREADS			1
#define IDO2DB_WORKER_THREADS			1
#define IDO2DB_NR_OF_THREADS                   (IDO2DB_CLEANER_THREADS+IDO2DB_WORKER_THREADS)

#define IDO2DB_THREAD_POOL_CLEANER		0
#define IDO2DB_THREAD_POOL_WORKER		1

#define IDO2DB_DEFAULT_THREAD_STACK_SIZE	65536

/***************** functions *******************/

/* config */
int ido2db_process_arguments(int,char **);
int ido2db_process_config_var(char *);
int ido2db_process_config_file(char *);

/* init */
int ido2db_initialize_variables(void);
int ido2db_check_init_reqs(void);

/* daemonize */
int ido2db_drop_privileges(char *,char *);
int ido2db_daemonize(void);
int ido2db_cleanup_socket(void);
void ido2db_parent_sighandler(int);
void ido2db_child_sighandler(int);

/* cleanup */
int ido2db_free_program_memory(void);
int ido2db_free_input_memory(ido2db_idi *);
int ido2db_free_connection_memory(ido2db_idi *);

/* client connection */
int ido2db_wait_for_connections(void);
int ido2db_handle_client_connection(int);
int ido2db_idi_init(ido2db_idi *);
int ido2db_check_for_client_input(ido2db_idi *);
int ido2db_handle_client_input(ido2db_idi *,char *);

/* data handling */
int ido2db_start_input_data(ido2db_idi *);
int ido2db_end_input_data(ido2db_idi *);
int ido2db_add_input_data_item(ido2db_idi *,int,char *);
int ido2db_add_input_data_mbuf(ido2db_idi *,int,int,char *);

/* conversion */
/* ToDo : Add convert_to_unsigned_long_long needed for *_processed fields */
int ido2db_convert_standard_data_elements(ido2db_idi *,int *,int *,int *,struct timeval *);
int ido2db_convert_string_to_int(char *,int *);
int ido2db_convert_string_to_float(char *,float *);
int ido2db_convert_string_to_double(char *,double *);
int ido2db_convert_string_to_long(char *,long *);
int ido2db_convert_string_to_unsignedlong(char *,unsigned long *);
int ido2db_convert_string_to_timeval(char *,struct timeval *);

/* threads */
void *ido2db_thread_cleanup(void *);
void *ido2db_thread_worker(void *);
int ido2db_terminate_threads(void);
int terminate_worker_thread(void);
int terminate_cleanup_thread(void);

#endif
