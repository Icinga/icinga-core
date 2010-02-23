/************************************************************************
 *
 * IDO2DB.H - IDO2DB Include File
 * Copyright (c) 2005-2007 Ethan Galstad
 * Copyright (c) 2009-2010 Icinga Development Team (http://www.icinga.org)
 *
 * Last Modified: 02-10-2010
 *
 ************************************************************************/

#ifndef _IDO2DB_NDO2DB_H
#define _IDO2DB_NDO2DB_H

#include "../../../include/config.h"
#include "utils.h"

#ifdef USE_ORACLE

#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#include <ocilib.h>
#define HAVE_CONFIG_H
#else
#include <ocilib.h>
#endif

#endif

/*************** mbuf definitions *************/
#define NDO2DB_MBUF_CONTACTGROUP                        0
#define NDO2DB_MBUF_CONTACTGROUPMEMBER                  1
#define NDO2DB_MBUF_SERVICEGROUPMEMBER                  2
#define NDO2DB_MBUF_HOSTGROUPMEMBER                     3
#define NDO2DB_MBUF_SERVICENOTIFICATIONCOMMAND          4
#define NDO2DB_MBUF_HOSTNOTIFICATIONCOMMAND             5
#define NDO2DB_MBUF_CONTACTADDRESS                      6
#define NDO2DB_MBUF_TIMERANGE                           7
#define NDO2DB_MBUF_PARENTHOST                          8
#define NDO2DB_MBUF_CONFIGFILEVARIABLE                  9
#define NDO2DB_MBUF_CONFIGVARIABLE                      10
#define NDO2DB_MBUF_RUNTIMEVARIABLE                     11
#define NDO2DB_MBUF_CUSTOMVARIABLE                      12
#define NDO2DB_MBUF_CONTACT                             13

#define NDO2DB_MAX_MBUF_ITEMS                           14


/***************** structures *****************/

typedef struct ndo2db_mbuf_struct{
	int used_lines;
	int allocated_lines;
	char **buffer;
        }ndo2db_mbuf;


typedef struct ndo2db_dbobject_struct{
	char *name1;
	char *name2;
	int object_type;
	unsigned long object_id;
	struct ndo2db_dbobject_struct *nexthash;
        }ndo2db_dbobject;


typedef struct ndo2db_dbconninfo_struct{
	int server_type;
	int connected;
	int error;
#ifndef USE_ORACLE /* libdbi specific */
	/* libdbi */
	dbi_conn dbi_conn;
	dbi_result dbi_result;
#else /* Oracle ocilib specific */
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
	/* well oh well */
	OCI_Statement* oci_statement_objects_select_name1_name2;
	OCI_Statement* oci_statement_objects_select_name1_null_name2;
	OCI_Statement* oci_statement_objects_select_name1_name2_null;
	OCI_Statement* oci_statement_objects_select_name1_null_name2_null;
	OCI_Statement* oci_statement_objects_select_cached;
	/* update */
	OCI_Statement* oci_statement_objects_update_inactive;
	OCI_Statement* oci_statement_objects_update_active;
	OCI_Statement* oci_statement_programstatus_update;
	OCI_Statement* oci_statement_timedevents_update;
	OCI_Statement* oci_statement_comment_history_update;
	OCI_Statement* oci_statement_downtimehistory_update_start;
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
	unsigned long trim_db_interval;
	time_t last_table_trim_time;
	time_t last_logentry_time;
	char *last_logentry_data;
	ndo2db_dbobject **object_hashlist;
        }ndo2db_dbconninfo;


typedef struct ndo2db_input_data_info_struct{
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
	unsigned long bytes_processed;
	unsigned long lines_processed;
	unsigned long entries_processed;
	unsigned long data_start_time;
	unsigned long data_end_time;
	int current_object_config_type;
	char **buffered_input;
	ndo2db_mbuf mbuf[NDO2DB_MAX_MBUF_ITEMS];
	ndo2db_dbconninfo dbinfo;
        }ndo2db_idi;



/*************** DB server types ***************/
#define NDO2DB_DBSERVER_NONE                            0
#define NDO2DB_DBSERVER_MYSQL                           1
#define NDO2DB_DBSERVER_PGSQL                           2
#define NDO2DB_DBSERVER_DB2                             3
#define NDO2DB_DBSERVER_FIREBIRD                        4
#define NDO2DB_DBSERVER_FREETDS                         5
#define NDO2DB_DBSERVER_INGRES                          6
#define NDO2DB_DBSERVER_MSQL                            7
#define NDO2DB_DBSERVER_ORACLE                          8
#define NDO2DB_DBSERVER_SQLITE                          9
#define NDO2DB_DBSERVER_SQLITE3                         10

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
#define NDO2DB_INPUT_BUFFER                             1024
#define NDO2DB_OBJECT_HASHSLOTS                         1024


/*********** types of input sections ***********/
#define NDO2DB_INPUT_SECTION_NONE                       0
#define NDO2DB_INPUT_SECTION_HEADER                     1
#define NDO2DB_INPUT_SECTION_FOOTER                     2
#define NDO2DB_INPUT_SECTION_DATA                       3


/************* types of input data *************/
#define NDO2DB_INPUT_DATA_NONE                          0

#define NDO2DB_INPUT_DATA_CONFIGDUMPSTART               1
#define NDO2DB_INPUT_DATA_CONFIGDUMPEND                 2

#define NDO2DB_INPUT_DATA_LOGENTRY                      10

#define NDO2DB_INPUT_DATA_PROCESSDATA                   20
#define NDO2DB_INPUT_DATA_TIMEDEVENTDATA                21
#define NDO2DB_INPUT_DATA_LOGDATA                       22
#define NDO2DB_INPUT_DATA_SYSTEMCOMMANDDATA             23
#define NDO2DB_INPUT_DATA_EVENTHANDLERDATA              24
#define NDO2DB_INPUT_DATA_NOTIFICATIONDATA              25
#define NDO2DB_INPUT_DATA_SERVICECHECKDATA              26
#define NDO2DB_INPUT_DATA_HOSTCHECKDATA                 27
#define NDO2DB_INPUT_DATA_COMMENTDATA                   28
#define NDO2DB_INPUT_DATA_DOWNTIMEDATA                  29
#define NDO2DB_INPUT_DATA_FLAPPINGDATA                  30
#define NDO2DB_INPUT_DATA_PROGRAMSTATUSDATA             31
#define NDO2DB_INPUT_DATA_HOSTSTATUSDATA                32
#define NDO2DB_INPUT_DATA_SERVICESTATUSDATA             33
#define NDO2DB_INPUT_DATA_ADAPTIVEPROGRAMDATA           34
#define NDO2DB_INPUT_DATA_ADAPTIVEHOSTDATA              35
#define NDO2DB_INPUT_DATA_ADAPTIVESERVICEDATA           36
#define NDO2DB_INPUT_DATA_EXTERNALCOMMANDDATA           37
#define NDO2DB_INPUT_DATA_AGGREGATEDSTATUSDATA          38
#define NDO2DB_INPUT_DATA_RETENTIONDATA                 39
#define NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONDATA       40
#define NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONMETHODDATA 41
#define NDO2DB_INPUT_DATA_ACKNOWLEDGEMENTDATA           42
#define NDO2DB_INPUT_DATA_STATECHANGEDATA               43
#define NDO2DB_INPUT_DATA_CONTACTSTATUSDATA             44
#define NDO2DB_INPUT_DATA_ADAPTIVECONTACTDATA           45

#define NDO2DB_INPUT_DATA_MAINCONFIGFILEVARIABLES       50
#define NDO2DB_INPUT_DATA_RESOURCECONFIGFILEVARIABLES   51
#define NDO2DB_INPUT_DATA_CONFIGVARIABLES               52
#define NDO2DB_INPUT_DATA_RUNTIMEVARIABLES              53

#define NDO2DB_INPUT_DATA_HOSTDEFINITION                61
#define NDO2DB_INPUT_DATA_HOSTGROUPDEFINITION           62
#define NDO2DB_INPUT_DATA_SERVICEDEFINITION             63
#define NDO2DB_INPUT_DATA_SERVICEGROUPDEFINITION        64
#define NDO2DB_INPUT_DATA_HOSTDEPENDENCYDEFINITION      65
#define NDO2DB_INPUT_DATA_SERVICEDEPENDENCYDEFINITION   66
#define NDO2DB_INPUT_DATA_HOSTESCALATIONDEFINITION      67
#define NDO2DB_INPUT_DATA_SERVICEESCALATIONDEFINITION   68
#define NDO2DB_INPUT_DATA_COMMANDDEFINITION             69
#define NDO2DB_INPUT_DATA_TIMEPERIODDEFINITION          70
#define NDO2DB_INPUT_DATA_CONTACTDEFINITION             71
#define NDO2DB_INPUT_DATA_CONTACTGROUPDEFINITION        72
#define NDO2DB_INPUT_DATA_HOSTEXTINFODEFINITION         73
#define NDO2DB_INPUT_DATA_SERVICEEXTINFODEFINITION      74


/************* types of config data ************/
#define NDO2DB_CONFIGTYPE_ORIGINAL                      0
#define NDO2DB_CONFIGTYPE_RETAINED                      1



/************* debugging levels ****************/

#define NDO2DB_DEBUGL_ALL                      -1
#define NDO2DB_DEBUGL_NONE                     0
#define NDO2DB_DEBUGL_PROCESSINFO              1
#define NDO2DB_DEBUGL_SQL                      2

#define NDO2DB_DEBUGV_BASIC                    0
#define NDO2DB_DEBUGV_MORE		       1
#define NDO2DB_DEBUGV_MOST                     2

/************* default trim db interval ********/

#define DEFAULT_TRIM_DB_INTERVAL 60

/***************** functions *******************/

int ndo2db_process_arguments(int,char **);

int ndo2db_process_config_var(char *);
int ndo2db_process_config_file(char *);

int ndo2db_initialize_variables(void);

int ndo2db_check_init_reqs(void);

int ndo2db_drop_privileges(char *,char *);
int ndo2db_daemonize(void);
int ndo2db_cleanup_socket(void);
void ndo2db_parent_sighandler(int);
void ndo2db_child_sighandler(int);

int ndo2db_free_program_memory(void);
int ndo2db_free_input_memory(ndo2db_idi *);
int ndo2db_free_connection_memory(ndo2db_idi *);

int ndo2db_wait_for_connections(void);
int ndo2db_handle_client_connection(int);
int ndo2db_idi_init(ndo2db_idi *);
int ndo2db_check_for_client_input(ndo2db_idi *,ndo_dbuf *, pthread_t *);
int ndo2db_handle_client_input(ndo2db_idi *,char *, pthread_t *);

int ndo2db_start_input_data(ndo2db_idi *);
int ndo2db_end_input_data(ndo2db_idi *);
int ndo2db_add_input_data_item(ndo2db_idi *,int,char *);
int ndo2db_add_input_data_mbuf(ndo2db_idi *,int,int,char *);

int ndo2db_convert_standard_data_elements(ndo2db_idi *,int *,int *,int *,struct timeval *);
int ndo2db_convert_string_to_int(char *,int *);
int ndo2db_convert_string_to_float(char *,float *);
int ndo2db_convert_string_to_double(char *,double *);
int ndo2db_convert_string_to_long(char *,long *);
int ndo2db_convert_string_to_unsignedlong(char *,unsigned long *);
int ndo2db_convert_string_to_timeval(char *,struct timeval *);

void *ido2db_thread_cleanup(void *);
static void *ido2db_thread_cleanup_exit_handler(void *);

#endif
