/************************************************************************
 *
 * DB.H - IDO Database Include File
 * Copyright (c) 2005-2006 Ethan Galstad
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 ************************************************************************/

#ifndef _IDO2DB_DB_H
#define _IDO2DB_DB_H

#include "../../../include/config.h"
#include "ido2db.h"

typedef struct ido2db_dbconfig_struct{
	int server_type;
	int port;
	char *host;
	char *username;
	char *password;
	char *dbname;
	char *dbprefix;
	char *dbserver;
	char *dbsocket;
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
	unsigned int oracle_trace_level;
        }ido2db_dbconfig;

/*************** DB server types ***************/

#define IDO2DB_DBTABLE_INSTANCES                      0
#define IDO2DB_DBTABLE_CONNINFO                       1
#define IDO2DB_DBTABLE_OBJECTS                        2
#define IDO2DB_DBTABLE_OBJECTTYPES                    3
#define IDO2DB_DBTABLE_LOGENTRIES                     4
#define IDO2DB_DBTABLE_SYSTEMCOMMANDS                 5
#define IDO2DB_DBTABLE_EVENTHANDLERS                  6
#define IDO2DB_DBTABLE_SERVICECHECKS                  7
#define IDO2DB_DBTABLE_HOSTCHECKS                     8
#define IDO2DB_DBTABLE_PROGRAMSTATUS                  9
#define IDO2DB_DBTABLE_EXTERNALCOMMANDS               10
#define IDO2DB_DBTABLE_SERVICESTATUS                  11
#define IDO2DB_DBTABLE_HOSTSTATUS                     12
#define IDO2DB_DBTABLE_PROCESSEVENTS                  13
#define IDO2DB_DBTABLE_TIMEDEVENTS                    14
#define IDO2DB_DBTABLE_TIMEDEVENTQUEUE                15
#define IDO2DB_DBTABLE_FLAPPINGHISTORY                16
#define IDO2DB_DBTABLE_COMMENTHISTORY                 17
#define IDO2DB_DBTABLE_COMMENTS                       18
#define IDO2DB_DBTABLE_NOTIFICATIONS                  19
#define IDO2DB_DBTABLE_CONTACTNOTIFICATIONS           20
#define IDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS     21
#define IDO2DB_DBTABLE_ACKNOWLEDGEMENTS               22
#define IDO2DB_DBTABLE_STATEHISTORY                   23
#define IDO2DB_DBTABLE_DOWNTIMEHISTORY                24
#define IDO2DB_DBTABLE_SCHEDULEDDOWNTIME              25
#define IDO2DB_DBTABLE_CONFIGFILES                    26
#define IDO2DB_DBTABLE_CONFIGFILEVARIABLES            27
#define IDO2DB_DBTABLE_RUNTIMEVARIABLES               28
#define IDO2DB_DBTABLE_CONTACTSTATUS                  29
#define IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS           30
#define IDO2DB_DBTABLE_RESERVED31                     31
#define IDO2DB_DBTABLE_RESERVED32                     32
#define IDO2DB_DBTABLE_RESERVED33                     33
#define IDO2DB_DBTABLE_RESERVED34                     34
#define IDO2DB_DBTABLE_RESERVED35                     35
#define IDO2DB_DBTABLE_RESERVED36                     36
#define IDO2DB_DBTABLE_RESERVED37                     37
#define IDO2DB_DBTABLE_RESERVED38                     38
#define IDO2DB_DBTABLE_RESERVED39                     39

#define IDO2DB_DBTABLE_COMMANDS                       40
#define IDO2DB_DBTABLE_TIMEPERIODS                    41
#define IDO2DB_DBTABLE_TIMEPERIODTIMERANGES           42
#define IDO2DB_DBTABLE_CONTACTGROUPS                  43
#define IDO2DB_DBTABLE_CONTACTGROUPMEMBERS            44
#define IDO2DB_DBTABLE_HOSTGROUPS                     45
#define IDO2DB_DBTABLE_HOSTGROUPMEMBERS               46
#define IDO2DB_DBTABLE_SERVICEGROUPS                  47
#define IDO2DB_DBTABLE_SERVICEGROUPMEMBERS            48
#define IDO2DB_DBTABLE_HOSTESCALATIONS                49
#define IDO2DB_DBTABLE_HOSTESCALATIONCONTACTS         50
#define IDO2DB_DBTABLE_SERVICEESCALATIONS             51
#define IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS      52
#define IDO2DB_DBTABLE_HOSTDEPENDENCIES               53
#define IDO2DB_DBTABLE_SERVICEDEPENDENCIES            54
#define IDO2DB_DBTABLE_CONTACTS                       55
#define IDO2DB_DBTABLE_CONTACTADDRESSES               56
#define IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS    57
#define IDO2DB_DBTABLE_HOSTS                          58
#define IDO2DB_DBTABLE_HOSTPARENTHOSTS                59
#define IDO2DB_DBTABLE_HOSTCONTACTS                   60
#define IDO2DB_DBTABLE_SERVICES                       61
#define IDO2DB_DBTABLE_SERVICECONTACTS                62
#define IDO2DB_DBTABLE_CUSTOMVARIABLES                63
#define IDO2DB_DBTABLE_HOSTCONTACTGROUPS              64
#define IDO2DB_DBTABLE_SERVICECONTACTGROUPS           65
#define IDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS    66
#define IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS 67
#define IDO2DB_DBTABLE_DBVERSION                      68
#define IDO2DB_DBTABLE_SLAHISTORY                     69

#define IDO2DB_MAX_DBTABLES                           70


/**************** Object types *****************/

#define IDO2DB_OBJECTTYPE_HOST                1
#define IDO2DB_OBJECTTYPE_SERVICE             2
#define IDO2DB_OBJECTTYPE_HOSTGROUP           3
#define IDO2DB_OBJECTTYPE_SERVICEGROUP        4
#define IDO2DB_OBJECTTYPE_HOSTESCALATION      5
#define IDO2DB_OBJECTTYPE_SERVICEESCALATION   6
#define IDO2DB_OBJECTTYPE_HOSTDEPENDENCY      7
#define IDO2DB_OBJECTTYPE_SERVICEDEPENDENCY   8
#define IDO2DB_OBJECTTYPE_TIMEPERIOD          9
#define IDO2DB_OBJECTTYPE_CONTACT             10
#define IDO2DB_OBJECTTYPE_CONTACTGROUP        11
#define IDO2DB_OBJECTTYPE_COMMAND             12



extern char *ido2db_db_tablenames[IDO2DB_MAX_DBTABLES];

int ido2db_db_init(ido2db_idi *);
int ido2db_db_deinit(ido2db_idi *);

int ido2db_db_connect(ido2db_idi *);
int ido2db_db_is_connected(ido2db_idi *);
int ido2db_db_reconnect(ido2db_idi *);
int ido2db_db_disconnect(ido2db_idi *);

int ido2db_db_hello(ido2db_idi *);
int ido2db_thread_db_hello(ido2db_idi *);
int ido2db_db_goodbye(ido2db_idi *);
int ido2db_db_checkin(ido2db_idi *);

char *ido2db_db_escape_string(ido2db_idi *,char *);
char *ido2db_db_timet_to_sql(ido2db_idi *,time_t);
char *ido2db_db_sql_to_timet(ido2db_idi *,char *);
int ido2db_db_query(ido2db_idi *,char *);
int ido2db_db_free_query(ido2db_idi *);
int ido2db_handle_db_error(ido2db_idi *);

int ido2db_db_clear_table(ido2db_idi *,char *);
int ido2db_db_get_latest_data_time(ido2db_idi *,char *,char *,unsigned long *);
int ido2db_db_perform_maintenance(ido2db_idi *);
int ido2db_db_trim_data_table(ido2db_idi *,char *,char *,unsigned long);

#ifdef USE_ORACLE /* Oracle ocilib specific */
#define OCI_VARCHAR_SIZE 4096 /* max allowed string size for varchar2 (+1) */
#define OCI_STR_SIZE 256 /* default small string buffer size */
#define OCI_BINDARRAY_MAX_SIZE 5000 /* default array buffer and commit size for bulk ops */
#define OCI_OUTPUT_BUFFER_SIZE 32000 /* Buffer size for dbms_output calls */
#define OCI_LOB_CHUNK_SIZE 2048 /* Buffer size for LOB operations */

void ido2db_ocilib_err_handler(OCI_Error *);
unsigned long ido2db_oci_sequence_lastid(ido2db_idi *, char *);
int ido2db_oci_prepared_statement_bind_null_param(OCI_Statement *, char *);
int ido2db_oci_bind_clob(OCI_Statement *st, char * bindname, char * text,OCI_Lob **);
int ido2db_oci_set_trace_event(OCI_Connection *,unsigned int);
int ido2db_oci_execute_out(OCI_Statement *,char *);
int ido2db_oci_set_session_info(OCI_Connection *, char *);
void ido2db_oci_print_binds(OCI_Statement *,int,char **);
void ido2db_oci_statement_free(OCI_Statement *,char *);
/* Helper */
int ido2db_oci_StringUTF8Length(const char *str);
#endif /* Oracle ocilib specific */

#endif
