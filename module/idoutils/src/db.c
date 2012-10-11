/***************************************************************
 * DB.C - Datatabase routines for IDO2DB daemon
 *
 * Copyright (c) 2005-2007 Ethan Galstad
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 **************************************************************/

/* include our project's header files */
#include "../../../include/config.h"
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ido2db.h"
#include "../include/dbhandlers.h"
#include "../include/db.h"
#include "../include/logging.h"

extern int errno;

//extern int ido2db_log_debug_info(int , int , const char *, ...);

int dummy;	/* reduce compiler warnings */

/* point to prepared statements after db initialize */
#ifdef USE_ORACLE
int ido2db_oci_prepared_statement_objects_insert(ido2db_idi *idi);
int ido2db_oci_prepared_statement_logentries_insert(ido2db_idi *idi);
int ido2db_oci_prepared_statement_timedevents_queue(ido2db_idi *idi);
int ido2db_oci_prepared_statement_timedeventqueue(ido2db_idi *idi);
int ido2db_oci_prepared_statement_timedevents(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hoststatus(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostchecks(ido2db_idi *idi);
int ido2db_oci_prepared_statement_servicestatus(ido2db_idi *idi);
int ido2db_oci_prepared_statement_servicechecks(ido2db_idi *idi);
int ido2db_oci_prepared_statement_contact_notificationcommands(ido2db_idi *idi);
int ido2db_oci_prepared_statement_programstatus(ido2db_idi *idi);
int ido2db_oci_prepared_statement_systemcommanddata(ido2db_idi *idi);
int ido2db_oci_prepared_statement_eventhandlerdata(ido2db_idi *idi);
int ido2db_oci_prepared_statement_notificationdata(ido2db_idi *idi);
int ido2db_oci_prepared_statement_contactnotificationdata(ido2db_idi *idi);
int ido2db_oci_prepared_statement_contactnotificationmethoddata(ido2db_idi *idi);
int ido2db_oci_prepared_statement_commentdata(ido2db_idi *idi);
int ido2db_oci_prepared_statement_commentdata_history(ido2db_idi *idi);
int ido2db_oci_prepared_statement_downtimedata_scheduled_downtime(ido2db_idi *idi);
int ido2db_oci_prepared_statement_downtimedata_downtime_history(ido2db_idi *idi);
int ido2db_oci_prepared_statement_contactstatusdata(ido2db_idi *idi);
int ido2db_oci_prepared_statement_configfilevariables(ido2db_idi *idi);
int ido2db_oci_prepared_statement_runtimevariables(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostdefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostdefinition_parenthosts(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostdefinition_contactgroups(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostdefinition_contacts(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostgroupdefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostgroupdefinition_hostgroupmembers(ido2db_idi *idi);
int ido2db_oci_prepared_statement_servicedefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_servicedefinition_contactgroups(ido2db_idi *idi);
int ido2db_oci_prepared_statement_servicedefinition_contacts(ido2db_idi *idi);
int ido2db_oci_prepared_statement_servicegroupdefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_servicegroupdefinition_members(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostdependencydefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_servicedependencydefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostescalationdefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostescalationdefinition_contactgroups(ido2db_idi *idi);
int ido2db_oci_prepared_statement_hostescalationdefinition_contacts(ido2db_idi *idi);
int ido2db_oci_prepared_statement_serviceescalationdefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_serviceescalationdefinition_contactgroups(ido2db_idi *idi);
int ido2db_oci_prepared_statement_serviceescalationdefinition_contacts(ido2db_idi *idi);
int ido2db_oci_prepared_statement_timeperiodefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_timeperiodefinition_timeranges(ido2db_idi *idi);
int ido2db_oci_prepared_statement_contactdefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_contactdefinition_addresses(ido2db_idi *idi);
int ido2db_oci_prepared_statement_contactdefinition_servicenotificationcommands(ido2db_idi *idi);
int ido2db_oci_prepared_statement_save_custom_variables_customvariables(ido2db_idi *idi);
int ido2db_oci_prepared_statement_save_custom_variables_customvariablestatus(ido2db_idi *idi);
int ido2db_oci_prepared_statement_contactgroupdefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_contactgroupdefinition_contactgroupmembers(ido2db_idi *idi);
int ido2db_oci_prepared_statement_commanddefinition_definition(ido2db_idi *idi);
int ido2db_oci_prepared_statement_process_events(ido2db_idi *idi);
int ido2db_oci_prepared_statement_configfilevariables_insert(ido2db_idi *idi);
int ido2db_oci_prepared_statement_flappinghistory(ido2db_idi *idi);
int ido2db_oci_prepared_statement_external_commands(ido2db_idi *idi);
int ido2db_oci_prepared_statement_acknowledgements(ido2db_idi *idi);
int ido2db_oci_prepared_statement_statehistory(ido2db_idi *idi);
int ido2db_oci_prepared_statement_instances(ido2db_idi *idi);
int ido2db_oci_prepared_statement_conninfo(ido2db_idi *idi);

/* select stuff */
int ido2db_oci_prepared_statement_objects_select_name1_name2(ido2db_idi *idi);
int ido2db_oci_prepared_statement_objects_select_cached(ido2db_idi *idi);
int ido2db_oci_prepared_statement_logentries_select(ido2db_idi *idi);
int ido2db_oci_prepared_statement_instances_select(ido2db_idi *idi);

/* update stuff */
int ido2db_oci_prepared_statement_objects_update_inactive(ido2db_idi *idi);
int ido2db_oci_prepared_statement_objects_update_active(ido2db_idi *idi);
int ido2db_oci_prepared_statement_object_enable_disable(ido2db_idi *idi);
int ido2db_oci_prepared_statement_programstatus_update(ido2db_idi *idi);
int ido2db_oci_prepared_statement_timedevents_update(ido2db_idi *idi);
int ido2db_oci_prepared_statement_comment_history_update(ido2db_idi *idi);
int ido2db_oci_prepared_statement_downtimehistory_update_start(ido2db_idi *idi);
int ido2db_oci_prepared_statement_downtimehistory_update_stop(ido2db_idi *idi);
int ido2db_oci_prepared_statement_conninfo_update(ido2db_idi *idi);
int ido2db_oci_prepared_statement_conninfo_update_checkin(ido2db_idi *idi);
int ido2db_oci_prepared_statement_scheduleddowntime_update_start(ido2db_idi *idi);

/* delete stuff */
int ido2db_oci_prepared_statement_timedeventqueue_delete(ido2db_idi *idi);
int ido2db_oci_prepared_statement_timedeventqueue_delete_more(ido2db_idi *idi);
int ido2db_oci_prepared_statement_comments_delete(ido2db_idi *idi);
int ido2db_oci_prepared_statement_downtime_delete(ido2db_idi *idi);
int ido2db_oci_prepared_statement_instances_delete(ido2db_idi *idi);
int ido2db_oci_prepared_statement_instances_delete_time(ido2db_idi *idi);

/* db version stuff */
int ido2db_oci_prepared_statement_dbversion_select(ido2db_idi *idi);

/* SLA */
int ido2db_oci_prepared_statement_sla_services_select(ido2db_idi *idi);
int ido2db_oci_prepared_statement_sla_history_select(ido2db_idi *idi);
int ido2db_oci_prepared_statement_sla_downtime_select(ido2db_idi *idi);
int ido2db_oci_prepared_statement_sla_history_merge(ido2db_idi *idi);
int ido2db_oci_prepared_statement_sla_history_delete(ido2db_idi *idi);

#endif

extern ido2db_dbconfig ido2db_db_settings;
extern time_t ido2db_db_last_checkin_time;

extern char *libdbi_driver_dir;

char *ido2db_db_rawtablenames[IDO2DB_MAX_DBTABLES] = {
	"instances",
	"conninfo",
	"objects",
	"objecttypes",
	"logentries",
	"systemcommands",
	"eventhandlers",
	"servicechecks",
	"hostchecks",
	"programstatus",
	"externalcommands",
	"servicestatus",
	"hoststatus",
	"processevents",
	"timedevents",
	"timedeventqueue",
	"flappinghistory",
	"commenthistory",
	"comments",
	"notifications",
	"contactnotifications",
	"contactnotificationmethods",
	"acknowledgements",
	"statehistory",
	"downtimehistory",
	"scheduleddowntime",
	"configfiles",
	"configfilevariables",
	"runtimevariables",
	"contactstatus",
	"customvariablestatus",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"commands",
	"timeperiods",
	"timeperiod_timeranges",
	"contactgroups",
	"contactgroup_members",
	"hostgroups",
	"hostgroup_members",
	"servicegroups",
	"servicegroup_members",
	"hostescalations",
	"hostescalation_contacts",
	"serviceescalations",
	"serviceescalation_contacts",
	"hostdependencies",
	"servicedependencies",
	"contacts",
	"contact_addresses",
	"contact_notificationcommands",
	"hosts",
	"host_parenthosts",
	"host_contacts",
	"services",
	"service_contacts",
	"customvariables",
	"host_contactgroups",
	"service_contactgroups",
	"hostescalation_contactgroups",
#ifdef USE_ORACLE /* Oracle ocilib specific */
	"serviceescalationcontactgroups",
#else /* everything else will be libdbi */
	"serviceescalation_contactgroups",
#endif /* Oracle ocilib specific */
	"dbversion",
	"slahistory"
};

char *ido2db_db_tablenames[IDO2DB_MAX_DBTABLES];

/*
 #define DEBUG_IDO2DB_QUERIES 1
 */

/****************************************************************************/
/* CONNECTION FUNCTIONS                                                     */
/****************************************************************************/


/************************************/
/* initialize database structures   */
/************************************/
int ido2db_db_init(ido2db_idi *idi) {
	register int x;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_init() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* initialize db server type */
	idi->dbinfo.server_type = ido2db_db_settings.server_type;

	/* initialize table names */
	for (x = 0; x < IDO2DB_MAX_DBTABLES; x++) {

		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
		case IDO2DB_DBSERVER_PGSQL:
		case IDO2DB_DBSERVER_DB2:
		case IDO2DB_DBSERVER_FIREBIRD:
		case IDO2DB_DBSERVER_FREETDS:
		case IDO2DB_DBSERVER_INGRES:
		case IDO2DB_DBSERVER_MSQL:
			if ((ido2db_db_tablenames[x] = (char *) malloc(strlen(ido2db_db_rawtablenames[x]) + ((ido2db_db_settings.dbprefix == NULL) ? 0 : strlen(ido2db_db_settings.dbprefix)) + 1)) == NULL)
				return IDO_ERROR;

			sprintf(ido2db_db_tablenames[x], "%s%s", (ido2db_db_settings.dbprefix == NULL) ? "" : ido2db_db_settings.dbprefix, ido2db_db_rawtablenames[x]);
			break;
		case IDO2DB_DBSERVER_ORACLE:
#ifdef USE_ORACLE /* Oracle ocilib specific */
			/* don't allow user to set table prefix for oracle */
			if ((ido2db_db_tablenames[x] = strdup(ido2db_db_rawtablenames[x])) == NULL)
				return IDO_ERROR;

#endif /* Oracle ocilib specific */
			break;
		case IDO2DB_DBSERVER_SQLITE:
		case IDO2DB_DBSERVER_SQLITE3:
			if ((ido2db_db_tablenames[x] = (char *) malloc(strlen(ido2db_db_rawtablenames[x]) + ((ido2db_db_settings.dbprefix == NULL) ? 0 : strlen(ido2db_db_settings.dbprefix)) + 1)) == NULL)
				return IDO_ERROR;

			sprintf(ido2db_db_tablenames[x], "%s%s", (ido2db_db_settings.dbprefix == NULL) ? "" : ido2db_db_settings.dbprefix, ido2db_db_rawtablenames[x]);
			break;
		default:
			break;
		}
	}

	/* initialize other variables */
	idi->dbinfo.connected = IDO_FALSE;
	idi->dbinfo.error = IDO_FALSE;
	idi->dbinfo.instance_id = 0L;
	idi->dbinfo.conninfo_id = 0L;
	idi->dbinfo.latest_program_status_time = (time_t) 0L;
	idi->dbinfo.latest_host_status_time = (time_t) 0L;
	idi->dbinfo.latest_service_status_time = (time_t) 0L;
	idi->dbinfo.latest_queued_event_time = (time_t) 0L;
	idi->dbinfo.latest_realtime_data_time = (time_t) 0L;
	idi->dbinfo.latest_comment_time = (time_t) 0L;
	idi->dbinfo.clean_event_queue = IDO_FALSE;
	idi->dbinfo.last_notification_id = 0L;
	idi->dbinfo.last_contact_notification_id = 0L;
	idi->dbinfo.max_timedevents_age = ido2db_db_settings.max_timedevents_age;
	idi->dbinfo.max_systemcommands_age = ido2db_db_settings.max_systemcommands_age;
	idi->dbinfo.max_servicechecks_age = ido2db_db_settings.max_servicechecks_age;
	idi->dbinfo.max_hostchecks_age = ido2db_db_settings.max_hostchecks_age;
	idi->dbinfo.max_eventhandlers_age = ido2db_db_settings.max_eventhandlers_age;
	idi->dbinfo.max_externalcommands_age = ido2db_db_settings.max_externalcommands_age;
	idi->dbinfo.max_logentries_age = ido2db_db_settings.max_logentries_age;
	idi->dbinfo.max_acknowledgements_age = ido2db_db_settings.max_acknowledgements_age;
	idi->dbinfo.max_notifications_age = ido2db_db_settings.max_notifications_age;
	idi->dbinfo.max_contactnotifications_age = ido2db_db_settings.max_contactnotifications_age;
	idi->dbinfo.max_contactnotificationmethods_age = ido2db_db_settings.max_contactnotificationmethods_age;
	idi->dbinfo.trim_db_interval = ido2db_db_settings.trim_db_interval;
	idi->dbinfo.housekeeping_thread_startup_delay = ido2db_db_settings.housekeeping_thread_startup_delay;
	idi->dbinfo.last_table_trim_time = (time_t) 0L;
	idi->dbinfo.last_logentry_time = (time_t) 0L;
	idi->dbinfo.last_logentry_data = NULL;
	idi->dbinfo.object_hashlist = NULL;

	/* initialize db structures, etc. */
#ifdef USE_LIBDBI /* everything else will be libdbi */

	if (dbi_initialize(libdbi_driver_dir) == -1) {
		syslog(LOG_USER | LOG_INFO, "Error: dbi_initialize() failed\n");
		return IDO_ERROR;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

	/* check if config matches */
	if (idi->dbinfo.server_type != IDO2DB_DBSERVER_PGSQL) {
		syslog(LOG_USER | LOG_INFO, "Error: ido2db.cfg not correctly configured. Please recheck for PGSQL!\n");
		return IDO_ERROR;
	}

	idi->dbinfo.pg_conn = NULL;
	idi->dbinfo.pg_result = NULL;

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if config matches */
	if (idi->dbinfo.server_type != IDO2DB_DBSERVER_ORACLE) {
		syslog(LOG_USER | LOG_INFO, "Error: ido2db.cfg not correctly configured. Please recheck for Oracle!\n");
		return IDO_ERROR;
	}

	/* register error handler and init oci */
	if (!OCI_Initialize(ido2db_ocilib_err_handler, NULL, OCI_ENV_THREADED|OCI_ENV_CONTEXT)) {
		syslog(LOG_USER | LOG_INFO, "Error:  OCI_Initialize() failed\n");
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "OCI_Initialize() failed\n");
		return IDO_ERROR;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "OCI_Initialize() ok\n");

#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_init() end\n");
	return IDO_OK;
}

/************************************/
/* clean up database structures     */
/************************************/
int ido2db_db_deinit(ido2db_idi *idi) {
	register int x;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_deinit() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* free table names */
	for (x = 0; x < IDO2DB_MAX_DBTABLES; x++) {
		if (ido2db_db_tablenames[x])
			free(ido2db_db_tablenames[x]);
		ido2db_db_tablenames[x] = NULL;
	}

	/* free cached object ids */
	ido2db_free_cached_object_ids(idi);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_deinit() end\n");
	return IDO_OK;
}

/************************************/
/* connects to the database server  */
/************************************/

int ido2db_db_is_connected(ido2db_idi *idi) {

#ifdef USE_LIBDBI
	if (!dbi_conn_ping(idi->dbinfo.dbi_conn))
		return IDO_FALSE;
#endif

#ifdef USE_PGSQL
	if (PQstatus(idi->dbinfo.pg_conn) != CONNECTION_OK)
		return IDO_FALSE;
#endif

#ifdef USE_ORACLE
	if (!idi->dbinfo.oci_connection) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_is_connected() Warning:Null Connection handle \n");
		return IDO_FALSE;
	}

	if (!OCI_IsConnected(idi->dbinfo.oci_connection)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_is_connected() ->not connected\n");
		return IDO_FALSE;
	}
#endif

	return IDO_TRUE;
}

int ido2db_db_reconnect(ido2db_idi *idi) {

	int result = IDO_OK;

	/* check connection */
	if (ido2db_db_is_connected(idi) == IDO_FALSE)
		idi->dbinfo.connected = IDO_FALSE;

	/* try to reconnect... */
	if (idi->dbinfo.connected == IDO_FALSE) {
		if (ido2db_db_connect(idi) == IDO_ERROR) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_reconnect(): failed.\n");
			syslog(LOG_USER | LOG_INFO, "Error: Could not reconnect to database!");
			return IDO_ERROR;
		}
		result = ido2db_db_hello(idi);
	}

	return result;
}


int ido2db_db_connect(ido2db_idi *idi) {
	int result = IDO_OK;
#ifdef USE_PGSQL /* pgsql */
	char *temp_port = NULL;
#endif
#ifdef USE_LIBDBI
	const char *dbi_error;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_connect() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* we're already connected... (and we don't wanna double check with ido2db_db_is_connected) */
	if (idi->dbinfo.connected == IDO_TRUE) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "\tido2db_db_connect(): already connected. Dropping out.\n");
		return IDO_OK;
	}

#ifdef USE_LIBDBI /* Oracle ocilib specific */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_MYSQL);
		break;
	case IDO2DB_DBSERVER_PGSQL:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_PGSQL);
		break;
	case IDO2DB_DBSERVER_DB2:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_DB2);
		break;
	case IDO2DB_DBSERVER_FIREBIRD:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_FIREBIRD);
		break;
	case IDO2DB_DBSERVER_FREETDS:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_FREETDS);
		break;
	case IDO2DB_DBSERVER_INGRES:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_INGRES);
		break;
	case IDO2DB_DBSERVER_MSQL:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_MSQL);
		break;
	case IDO2DB_DBSERVER_ORACLE:
		break;
	case IDO2DB_DBSERVER_SQLITE:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_SQLITE);
		break;
	case IDO2DB_DBSERVER_SQLITE3:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_SQLITE3);
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_connect() pgsql start somewhere\n");
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* for ocilib there is no such statement next below */

#endif /* Oracle ocilib specific */


#ifdef USE_LIBDBI /* everything else will be libdbi */

	/* Check if the dbi connection was created successful */
	if (idi->dbinfo.dbi_conn == NULL) {
		dbi_conn_error(idi->dbinfo.dbi_conn, &dbi_error);
		syslog(LOG_USER | LOG_INFO, "Error: Could  not dbi_conn_new(): %s", dbi_error);
		result = IDO_ERROR;
		idi->disconnect_client = IDO_TRUE;
		return IDO_ERROR;
	}

	/* decide wether to set host and port, or not ... drivers will use socket otherwise */
        switch (idi->dbinfo.server_type) {
        case IDO2DB_DBSERVER_MYSQL:
		if (ido2db_db_settings.host != NULL)
			dbi_conn_set_option(idi->dbinfo.dbi_conn, "host", ido2db_db_settings.host);
		if (ido2db_db_settings.port != 0)
			dbi_conn_set_option_numeric(idi->dbinfo.dbi_conn, "port", (int)ido2db_db_settings.port);
                break;
        case IDO2DB_DBSERVER_PGSQL:
		if (ido2db_db_settings.host != NULL)
			dbi_conn_set_option(idi->dbinfo.dbi_conn, "host", ido2db_db_settings.host);
		if (ido2db_db_settings.port != 0)
			dbi_conn_set_option_numeric(idi->dbinfo.dbi_conn, "port", (int)ido2db_db_settings.port);
                break;
        default:
		if (ido2db_db_settings.host != NULL)
			dbi_conn_set_option(idi->dbinfo.dbi_conn, "host", ido2db_db_settings.host);
		if (ido2db_db_settings.port != 0)
			dbi_conn_set_option_numeric(idi->dbinfo.dbi_conn, "port", (int)ido2db_db_settings.port);
                break;
        }

	dbi_conn_set_option(idi->dbinfo.dbi_conn, "username", ido2db_db_settings.username);
	dbi_conn_set_option(idi->dbinfo.dbi_conn, "password", ido2db_db_settings.password);
	dbi_conn_set_option(idi->dbinfo.dbi_conn, "dbname", ido2db_db_settings.dbname);
	dbi_conn_set_option(idi->dbinfo.dbi_conn, "encoding", "auto");

	if (ido2db_db_settings.dbsocket != NULL) {
		/* a local db socket was desired, drop db_port settings in case */
		dbi_conn_clear_option(idi->dbinfo.dbi_conn, "port");

		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			dbi_conn_set_option(idi->dbinfo.dbi_conn, "mysql_unix_socket", ido2db_db_settings.dbsocket);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* override the port as stated in libdbi-driver docs */
			dbi_conn_set_option(idi->dbinfo.dbi_conn, "port", ido2db_db_settings.dbsocket);
			break;
		default:
			break;
		}
	}

	if (dbi_conn_connect(idi->dbinfo.dbi_conn) != 0) {
		dbi_conn_error(idi->dbinfo.dbi_conn, &dbi_error);
		syslog(LOG_USER | LOG_INFO, "Error: Could not connect to %s database: %s", ido2db_db_settings.dbserver, dbi_error);
		result = IDO_ERROR;
		idi->disconnect_client = IDO_TRUE;
	} else {
		idi->dbinfo.connected = IDO_TRUE;
		syslog(LOG_USER | LOG_INFO, "Successfully connected to %s database", ido2db_db_settings.dbserver);
	}
#endif

#ifdef USE_PGSQL /* pgsql */

	dummy = asprintf(&temp_port, "%d", ido2db_db_settings.port);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_connect() pgsql start\n");

	/* check if config matches */
	if (idi->dbinfo.server_type != IDO2DB_DBSERVER_PGSQL) {
		syslog(LOG_USER | LOG_INFO, "Error: ido2db.cfg not correctly configured. Please recheck for PGSQL!\n");
		return IDO_ERROR;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_connect() pgsql trying to connect host: %s, port: %s, dbname: %s, user: %s, pass: %s\n", ido2db_db_settings.host, temp_port, ido2db_db_settings.dbname, ido2db_db_settings.username, ido2db_db_settings.password);

	/* create db connection instantly */
	idi->dbinfo.pg_conn = PQsetdbLogin(ido2db_db_settings.host,
	                                   "",/*temp_port,*/
	                                   "", /* pgoptions */
	                                   "", /* pgtty*/
	                                   ido2db_db_settings.dbname,
	                                   ido2db_db_settings.username,
	                                   ido2db_db_settings.password);
	free(temp_port);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_connect() pgsql segfault?\n");

	if (PQstatus(idi->dbinfo.pg_conn) != CONNECTION_OK) {
		syslog(LOG_USER | LOG_INFO, "Error: Could not connect to pgsql database: %s", PQerrorMessage(idi->dbinfo.pg_conn));
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "Error: Could not connect to %s database\n", ido2db_db_settings.dbserver);
		result = IDO_ERROR;
		idi->disconnect_client = IDO_TRUE;
	} else {
		idi->dbinfo.connected = IDO_TRUE;
		syslog(LOG_USER | LOG_INFO, "Successfully connected to pgsql database");
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "Successfully connected to %s database\n", ido2db_db_settings.dbserver);
	}
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if config matches */
	if (idi->dbinfo.server_type != IDO2DB_DBSERVER_ORACLE) {
		syslog(LOG_USER | LOG_INFO, "Error: ido2db.cfg not correctly configured. Please recheck for Oracle!\n");
		return IDO_ERROR;
	}

	/* create db connection instantly */
	idi->dbinfo.oci_connection = OCI_ConnectionCreate((mtext *)ido2db_db_settings.dbname, (mtext *)ido2db_db_settings.username, (mtext *)ido2db_db_settings.password, OCI_SESSION_DEFAULT);

	if (idi->dbinfo.oci_connection == NULL) {
		syslog(LOG_USER | LOG_INFO, "Error: Could not connect to oracle database: %s", OCI_ErrorGetString(OCI_GetLastError()));
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "Error: Could not connect to %s database\n", ido2db_db_settings.dbserver);
		idi->dbinfo.connected = IDO_FALSE;
		idi->disconnect_client = IDO_TRUE;
		return IDO_ERROR;
	} else {
		idi->dbinfo.connected = IDO_TRUE;
		syslog(LOG_USER | LOG_INFO, "Successfully connected to oracle database");
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "Successfully connected to database '%s'@'%s'"
		                      " (server version %i.%i.%i, client version [%u], ocilib version %u.%u.%u) \n",
		                      ido2db_db_settings.username,
		                      ido2db_db_settings.dbname,
		                      OCI_GetServerMajorVersion(idi->dbinfo.oci_connection),
		                      OCI_GetServerMinorVersion(idi->dbinfo.oci_connection),
		                      OCI_GetServerRevisionVersion(idi->dbinfo.oci_connection),
		                      OCI_GetVersionConnection(idi->dbinfo.oci_connection),
		                      OCILIB_MAJOR_VERSION,
		                      OCILIB_MINOR_VERSION,
		                      OCILIB_REVISION_VERSION);
	}

	/* initialize prepared statements */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_connect() prepare statements start\n");
	/* object inserts */
	if (ido2db_oci_prepared_statement_objects_insert(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_insert() failed\n");
		return IDO_ERROR;
	}

	/* logentries */
	if (ido2db_oci_prepared_statement_logentries_insert(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_logentries_insert() failed\n");
		return IDO_ERROR;
	}

	/* timed events */
	if (ido2db_oci_prepared_statement_timedevents_queue(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() failed\n");
		return IDO_ERROR;
	}

	if (ido2db_oci_prepared_statement_timedevents(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents() failed\n");
		return IDO_ERROR;
	}

	if (ido2db_oci_prepared_statement_timedeventqueue(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedeventqueue() failed\n");
		return IDO_ERROR;
	}

	/* hoststatus/check */
	if (ido2db_oci_prepared_statement_hoststatus(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hoststatus() failed\n");
		return IDO_ERROR;
	}

	if (ido2db_oci_prepared_statement_hostchecks(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostchecks() failed\n");
		return IDO_ERROR;
	}

	/* servicestatus/check */
	if (ido2db_oci_prepared_statement_servicestatus(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicestatus() failed\n");
		return IDO_ERROR;
	}

	if (ido2db_oci_prepared_statement_servicechecks(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicechecks() failed\n");
		return IDO_ERROR;
	}

	/* contactnotifications */
	if (ido2db_oci_prepared_statement_contact_notificationcommands(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contact_notificationcommands() failed\n");
		return IDO_ERROR;
	}

	/* programstatus */
	if (ido2db_oci_prepared_statement_programstatus(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_programstatus() failed\n");
		return IDO_ERROR;
	}

	/* systemcommand */
	if (ido2db_oci_prepared_statement_systemcommanddata(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_systemcommanddata() failed\n");
		return IDO_ERROR;
	}

	/* commanddefinition_definition */
	if (ido2db_oci_prepared_statement_commanddefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commanddefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* eventhandler */
	if (ido2db_oci_prepared_statement_eventhandlerdata(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_eventhandlerdata() failed\n");
		return IDO_ERROR;
	}

	/* notificationdata */
	if (ido2db_oci_prepared_statement_notificationdata(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_notificationdata() failed\n");
		return IDO_ERROR;
	}

	/* contactnotificationdata */
	if (ido2db_oci_prepared_statement_contactnotificationdata(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactnotificationdata() failed\n");
		return IDO_ERROR;
	}

	/* contactnotificationmethoddata */
	if (ido2db_oci_prepared_statement_contactnotificationmethoddata(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactnotificationmethoddata() failed\n");
		return IDO_ERROR;
	}

	/* commentdata */
	if (ido2db_oci_prepared_statement_commentdata(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commentdata() failed\n");
		return IDO_ERROR;
	}

	/* commentdata history */
	if (ido2db_oci_prepared_statement_commentdata_history(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commentdata_history() failed\n");
		return IDO_ERROR;
	}

	/* downtimedata scheduled_downtime */
	if (ido2db_oci_prepared_statement_downtimedata_scheduled_downtime(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_downtimedata_scheduled_downtime() failed\n");
		return IDO_ERROR;
	}

	/* downtimedata downtime_history */
	if (ido2db_oci_prepared_statement_downtimedata_downtime_history(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_downtimedata_downtime_history() failed\n");
		return IDO_ERROR;
	}

	/* contactstatusdata */
	if (ido2db_oci_prepared_statement_contactstatusdata(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactstatusdata() failed\n");
		return IDO_ERROR;
	}

	/* configfilevariables */
	if (ido2db_oci_prepared_statement_configfilevariables(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_configfilevariables() failed\n");
		return IDO_ERROR;
	}

	if (ido2db_oci_prepared_statement_configfilevariables_insert(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_configfilevariables_insert() failed\n");
		return IDO_ERROR;
	}

	/* runtimevariables */
	if (ido2db_oci_prepared_statement_runtimevariables(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_runtimevariables() failed\n");
		return IDO_ERROR;
	}

	/* hostdefinition_definition */
	if (ido2db_oci_prepared_statement_hostdefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostdefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* hostdefinition_parenthosts */
	if (ido2db_oci_prepared_statement_hostdefinition_parenthosts(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostdefinition_parenthosts() failed\n");
		return IDO_ERROR;
	}

	/* hostdefinition_contactgroups */
	if (ido2db_oci_prepared_statement_hostdefinition_contactgroups(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostdefinition_contactgroups() failed\n");
		return IDO_ERROR;
	}

	/* hostdefinition_contacts */
	if (ido2db_oci_prepared_statement_hostdefinition_contacts(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostdefinition_contacts() failed\n");
		return IDO_ERROR;
	}

	/* hostgroupdefinition_definition */
	if (ido2db_oci_prepared_statement_hostgroupdefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostgroupdefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* hostgroupdefinition_hostgroupmembers */
	if (ido2db_oci_prepared_statement_hostgroupdefinition_hostgroupmembers(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostgroupdefinition_hostgroupmembers() failed\n");
		return IDO_ERROR;
	}

	/* servicedefinition_definition */
	if (ido2db_oci_prepared_statement_servicedefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicedefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* servicedefinition_contactgroups */
	if (ido2db_oci_prepared_statement_servicedefinition_contactgroups(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicedefinition_contactgroups() failed\n");
		return IDO_ERROR;
	}

	/* servicedefinition_contacts */
	if (ido2db_oci_prepared_statement_servicedefinition_contacts(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicedefinition_contacts() failed\n");
		return IDO_ERROR;
	}

	/* servicegroupdefinition_definition */
	if (ido2db_oci_prepared_statement_servicegroupdefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicegroupdefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* servicegroupdefinition_members */
	if (ido2db_oci_prepared_statement_servicegroupdefinition_members(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicegroupdefinition_members() failed\n");
		return IDO_ERROR;
	}

	/* hostdependencydefinition_definition */
	if (ido2db_oci_prepared_statement_hostdependencydefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostdependencydefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* servicedependencydefinition_definition */
	if (ido2db_oci_prepared_statement_servicedependencydefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicedependencydefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* hostescalationdefinition_definition */
	if (ido2db_oci_prepared_statement_hostescalationdefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostescalationdefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* hostescalationdefinition_contactgroups */
	if (ido2db_oci_prepared_statement_hostescalationdefinition_contactgroups(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostescalationdefinition_contactgroups() failed\n");
		return IDO_ERROR;
	}

	/* hostescalationdefinition_contacts */
	if (ido2db_oci_prepared_statement_hostescalationdefinition_contacts(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostescalationdefinition_contacts() failed\n");
		return IDO_ERROR;
	}

	/* serviceescalationdefinition_definition */
	if (ido2db_oci_prepared_statement_serviceescalationdefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_serviceescalationdefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* serviceescalationdefinition_contactgroups */
	if (ido2db_oci_prepared_statement_serviceescalationdefinition_contactgroups(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_serviceescalationdefinition_contactgroups() failed\n");
		return IDO_ERROR;
	}

	/* serviceescalationdefinition_contacts */
	if (ido2db_oci_prepared_statement_serviceescalationdefinition_contacts(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_serviceescalationdefinition_contacts() failed\n");
		return IDO_ERROR;
	}

	/* timeperiodefinition_definition */
	if (ido2db_oci_prepared_statement_timeperiodefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timeperiodefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* timeperiodefinition_timeranges */
	if (ido2db_oci_prepared_statement_timeperiodefinition_timeranges(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timeperiodefinition_timeranges() failed\n");
		return IDO_ERROR;
	}

	/* contactdefinition_definition */
	if (ido2db_oci_prepared_statement_contactdefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactdefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* contactdefinition_addresses */
	if (ido2db_oci_prepared_statement_contactdefinition_addresses(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactdefinition_addresses() failed\n");
		return IDO_ERROR;
	}

	/* contactdefinition_servicenotificationcommands */
	if (ido2db_oci_prepared_statement_contactdefinition_servicenotificationcommands(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactdefinition_servicenotificationcommands() failed\n");
		return IDO_ERROR;
	}

	/* save_custom_variables_customvariables */
	if (ido2db_oci_prepared_statement_save_custom_variables_customvariables(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_save_custom_variables_customvariables() failed\n");
		return IDO_ERROR;
	}

	/* save_custom_variables_customvariablestatus */
	if (ido2db_oci_prepared_statement_save_custom_variables_customvariablestatus(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_save_custom_variables_customvariablestatus() failed\n");
		return IDO_ERROR;
	}

	/* contactgroupdefinition_definition */
	if (ido2db_oci_prepared_statement_contactgroupdefinition_definition(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactgroupdefinition_definition() failed\n");
		return IDO_ERROR;
	}

	/* contactgroupdefinition_contactgroupmembers */
	if (ido2db_oci_prepared_statement_contactgroupdefinition_contactgroupmembers(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactgroupdefinition_contactgroupmembers() failed\n");
		return IDO_ERROR;
	}

	/* process_events */
	if (ido2db_oci_prepared_statement_process_events(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_process_events() failed\n");
		return IDO_ERROR;
	}

	/* flapping history */
	if (ido2db_oci_prepared_statement_flappinghistory(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_flappinghistory() failed\n");
		return IDO_ERROR;
	}

	/* external commands */
	if (ido2db_oci_prepared_statement_external_commands(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_external_commands() failed\n");
		return IDO_ERROR;
	}

	/* acknowledgements */
	if (ido2db_oci_prepared_statement_acknowledgements(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_acknowledgements() failed\n");
		return IDO_ERROR;
	}

	/* statehistory */
	if (ido2db_oci_prepared_statement_statehistory(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_statehistory() failed\n");
		return IDO_ERROR;
	}

	/* instances */
	if (ido2db_oci_prepared_statement_instances(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_instances() failed\n");
		return IDO_ERROR;
	}

	/*  conninfo */
	if (ido2db_oci_prepared_statement_conninfo(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_connfino() failed\n");
		return IDO_ERROR;
	}

	/* objects select */
	if (ido2db_oci_prepared_statement_objects_select_name1_name2(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* objects select cached */
	if (ido2db_oci_prepared_statement_objects_select_cached(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* objects update inactive */
	if (ido2db_oci_prepared_statement_objects_update_inactive(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_update_inactive() failed\n");
		return IDO_ERROR;
	}

	/*  objects update active */
	if (ido2db_oci_prepared_statement_objects_update_active(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_update_active() failed\n");
		return IDO_ERROR;
	}

        /* objects enable disable */
        if (ido2db_oci_prepared_statement_object_enable_disable(idi) == IDO_ERROR) {
                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_object_enable_disable() failed\n");
                return IDO_ERROR;
        }

	/* logentries select  */
	if (ido2db_oci_prepared_statement_logentries_select(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_logentries_select() failed\n");
		return IDO_ERROR;
	}

	/* programstatus update */
	if (ido2db_oci_prepared_statement_programstatus_update(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_programstatus_update() failed\n");
		return IDO_ERROR;
	}

	/* timedevents update */
	if (ido2db_oci_prepared_statement_timedevents_update(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_update() failed\n");
		return IDO_ERROR;
	}

	/* timdeventqueue delete */
	if (ido2db_oci_prepared_statement_timedeventqueue_delete(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* timdeventqueue delete more*/
	if (ido2db_oci_prepared_statement_timedeventqueue_delete_more(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* comment history update  */
	if (ido2db_oci_prepared_statement_comment_history_update(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* comments delete */
	if (ido2db_oci_prepared_statement_comments_delete(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* downtimehistory_update start */
	if (ido2db_oci_prepared_statement_downtimehistory_update_start(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* scheduleddowntime_update start */
	if (ido2db_oci_prepared_statement_scheduleddowntime_update_start(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* downtimehistory_update stop */
	if (ido2db_oci_prepared_statement_downtimehistory_update_stop(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* downtime delete */
	if (ido2db_oci_prepared_statement_downtime_delete(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* instances select */
	if (ido2db_oci_prepared_statement_instances_select(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* conninfo update */
	if (ido2db_oci_prepared_statement_conninfo_update(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* conninfo update */
	if (ido2db_oci_prepared_statement_conninfo_update_checkin(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() failed\n");
		return IDO_ERROR;
	}

	/* instances delete */
	if (ido2db_oci_prepared_statement_instances_delete(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_instances_delete() failed\n");
		return IDO_ERROR;
	}

	/* instances delete time */
	if (ido2db_oci_prepared_statement_instances_delete_time(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_instances_delete_time() failed\n");
		return IDO_ERROR;
	}


	/* dbversion */
	if (ido2db_oci_prepared_statement_dbversion_select(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_dbversion_select() failed\n");
		return IDO_ERROR;
	}

	/* SLA */
	if (ido2db_oci_prepared_statement_sla_services_select(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_sla_services_select() failed\n");
		return IDO_ERROR;
	}

	if (ido2db_oci_prepared_statement_sla_downtime_select(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_sla_downtime_select() failed\n");
		return IDO_ERROR;
	}

	if (ido2db_oci_prepared_statement_sla_history_select(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_sla_history_select() failed\n");
		return IDO_ERROR;
	}

	if (ido2db_oci_prepared_statement_sla_history_merge(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_sla_history_insert() failed\n");
		return IDO_ERROR;
	}

	if (ido2db_oci_prepared_statement_sla_history_delete(idi) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_sla_history_delete() failed\n");
		return IDO_ERROR;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_dbversion_select() prepare statements end\n");

	/* start oracle trace if activated */
	ido2db_oci_set_trace_event(idi->dbinfo.oci_connection, ido2db_db_settings.oracle_trace_level);
#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_connect(%d) end\n", result);
	return result;
}

/****************************************/
/* disconnects from the database server */
/****************************************/
int ido2db_db_disconnect(ido2db_idi *idi) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_disconnect() start\n");

	if (idi == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_disconnect() Error:Connection structure is null\n");
#ifdef USE_ORACLE
		OCI_Cleanup();
#endif
		return IDO_ERROR;
	}


	/* we're not connected... */
	if (ido2db_db_is_connected(idi) == IDO_FALSE) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_disconnect() already disconnected\n");
#ifdef USE_ORACLE
		OCI_Cleanup();
#endif
		return IDO_OK;
	}


#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_conn_close(idi->dbinfo.dbi_conn);
	dbi_shutdown();

	syslog(LOG_USER | LOG_INFO, "Successfully disconnected from %s database", ido2db_db_settings.dbserver);
#endif

#ifdef USE_PGSQL /* pgsql */

	PQfinish(idi->dbinfo.pg_conn);

	syslog(LOG_USER | LOG_INFO, "Successfully disconnected from %s database", ido2db_db_settings.dbserver);

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_disconnect() free statements\n");
	/**
	 *  close prepared statements
	 *  2011-06-19 only if handle is set
	 * */
	//ido2db_oci_statement_free(idi->dbinfo.oci_statement, "oci_statement");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_timedevents, "oci_statement_objects_update_inactive");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_timedevents_queue, "oci_statement_timedevents_queue");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_timedeventqueue, "oci_statement_timedeventqueue");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostchecks, "oci_statement_hostchecks");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hoststatus, "oci_statement_hoststatus");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_servicechecks, "oci_statement_servicechecks");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_servicestatus, "oci_statement_servicestatus");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_contact_notificationcommands, "oci_statement_contact_notificationcommands");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_objects_insert, "oci_statement_objects_insert");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_logentries_insert, "oci_statement_logentries_insert");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_programstatus, "oci_statement_programstatus");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_systemcommanddata, "oci_statement_systemcommanddata");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_eventhandlerdata, "oci_statement_eventhandlerdata");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_notificationdata, "oci_statement_notificationdata");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_contactnotificationdata, "oci_statement_contactnotificationdata");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_contactnotificationmethoddata, "oci_statement_contactnotificationmethoddata");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_commentdata, "oci_statement_commentdata");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_commentdata_history, "oci_statement_commentdata_history");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, "oci_statement_downtimedata_scheduled_downtime");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_downtimedata_downtime_history, "oci_statement_downtimedata_downtime_history");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_contactstatusdata, "oci_statement_contactstatusdata");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_configfilevariables, "oci_statement_configfilevariables");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_configfilevariables_insert, "oci_statement_configfilevariables_insert");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_runtimevariables, "oci_statement_runtimevariables");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostdefinition_definition, "oci_statement_hostdefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostdefinition_parenthosts, "oci_statement_hostdefinition_parenthosts");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostdefinition_contactgroups, "oci_statement_hostdefinition_contactgroups");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostdefinition_contacts, "oci_statement_hostdefinition_contacts");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostgroupdefinition_definition, "oci_statement_hostgroupdefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, "oci_statement_hostgroupdefinition_hostgroupmembers");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_servicedefinition_definition, "oci_statement_servicedefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_servicedefinition_contactgroups, "oci_statement_servicedefinition_contactgroups");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_servicedefinition_contacts, "oci_statement_servicedefinition_contacts");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_servicegroupdefinition_definition, "oci_statement_servicegroupdefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_servicegroupdefinition_members, "oci_statement_servicegroupdefinition_members");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostdependencydefinition_definition, "oci_statement_hostdependencydefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_servicedependencydefinition_definition, "oci_statement_servicedependencydefinition_definition");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostescalationdefinition_definition, "oci_statement_hostescalationdefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostescalationdefinition_contactgroups, "oci_statement_hostescalationdefinition_contactgroups");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_hostescalationdefinition_contacts, "oci_statement_hostescalationdefinition_contacts");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, "oci_statement_serviceescalationdefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_serviceescalationdefinition_contactgroups, "oci_statement_serviceescalationdefinition_contactgroups");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_serviceescalationdefinition_contacts, "oci_statement_serviceescalationdefinition_contacts");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_commanddefinition_definition, "oci_statement_commanddefinition_definition");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_timeperiodefinition_definition, "oci_statement_timeperiodefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, "oci_statement_timeperiodefinition_timeranges");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_contactdefinition_definition, "oci_statement_contactdefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_contactdefinition_addresses, "oci_statement_contactdefinition_addresses");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands, "oci_statement_contactdefinition_servicenotificationcommands");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_save_custom_variables_customvariables, "oci_statement_save_custom_variables_customvariables");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, "oci_statement_save_custom_variables_customvariablestatus");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_contactgroupdefinition_definition, "oci_statement_contactgroupdefinition_definition");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, "oci_statement_contactgroupdefinition_contactgroupmembers");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_process_events, "oci_statement_process_events");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_flappinghistory, "oci_statement_flappinghistory");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_external_commands, "oci_statement_external_commands");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_acknowledgements, "oci_statement_acknowledgements");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_statehistory, "oci_statement_statehistory");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_instances, "oci_statement_instances");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_conninfo, "oci_statement_conninfo");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_objects_select_name1_name2, "oci_statement_objects_select_name1_name2");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_objects_select_cached, "oci_statement_objects_select_cached");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_objects_update_inactive, "oci_statement_objects_update_inactive");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_objects_update_active, "oci_statement_objects_update_active");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_object_enable_disable, "oci_statement_object_enable_disable");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_logentries_select, "oci_statement_logentries_select");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_programstatus_update, "oci_statement_programstatus_update");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_timedevents_update, "oci_statement_timedevents_update");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_timedeventqueue_delete, "oci_statement_timedeventqueue_delete");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_timedeventqueue_delete_more, "oci_statement_timedeventqueue_delete_more");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_comment_history_update, "oci_statement_comment_history_update");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_comments_delete, "oci_statement_comments_delete");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_downtimehistory_update_start, "oci_statement_downtimehistory_update_start");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_scheduleddowntime_update_start, "oci_statement_scheduleddowntime_update_start");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_downtimehistory_update_stop, "oci_statement_downtimehistory_update_stop");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_downtime_delete, "oci_statement_downtime_delete");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_instances_select, "oci_statement_instances_select");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_conninfo_update, "oci_statement_conninfo_update");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_conninfo_update_checkin, "oci_statement_conninfo_update_checkin");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_instances_delete, "oci_statement_instances_delete");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_instances_delete_time, "oci_statement_instances_delete_time");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_dbversion_select, "oci_statement_dbversion_select");

	ido2db_oci_statement_free(idi->dbinfo.oci_statement_sla_services_select, "oci_statement_sla_services_select");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_sla_downtime_select, "oci_statement_sla_downtime_select");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_sla_history_select, "oci_statement_sla_history_select");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_sla_history_merge, "oci_statement_sla_history_merge");
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_sla_history_delete, "oci_statement_sla_history_delete");

	syslog(LOG_USER | LOG_INFO, "Successfully freed prepared statements");

	/* close db connection */
	OCI_ConnectionFree(idi->dbinfo.oci_connection);
	OCI_Cleanup();
	syslog(LOG_USER | LOG_INFO, "Successfully disconnected from oracle database");

#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_disconnect() stop\n");
	return IDO_OK;
}


/************************************/
/* post-connect routines            */
/************************************/

int ido2db_db_version_check(ido2db_idi *idi) {
	char *buf;
	char *name;
	int result;
	void *data[1];
	/*
	#ifdef USE_ORACLE
		char *dbversion;
		dbversion=NULL;
	#endif
	*/

	buf = NULL;
	name = NULL;
	result= IDO_OK;


	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check () start \n");

	name = strdup("idoutils");
	data[0] = (void *) &name;

#ifdef USE_LIBDBI

	if (asprintf(&buf, "SELECT version FROM %s WHERE name='%s'", ido2db_db_tablenames[IDO2DB_DBTABLE_DBVERSION], name) == -1)
		buf = NULL;

	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {

		if (idi->dbinfo.dbi_result != NULL) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
				idi->dbinfo.dbversion = strdup(dbi_result_get_string(idi->dbinfo.dbi_result, "version"));
			}
		}
	}

	free(buf);
#endif

#ifdef USE_ORACLE


	if (!OCI_BindString(idi->dbinfo.oci_statement_dbversion_select, MT(":X1"), *(char **) data[0], 0)) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_dbversion_select)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check () \n");
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_dbversion_select);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check() query ok\n");

	if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check() fetchnext ok\n");
		idi->dbinfo.dbversion = strdup(OCI_GetString(idi->dbinfo.oci_resultset, 1));
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check(version=%s)\n", idi->dbinfo.dbversion);
	} else {
		idi->dbinfo.dbversion = NULL;
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check() fetchnext not ok\n");
	}


#endif

#ifdef USE_PGSQL

	//FIXME
#endif

	free(name);

	/* check dbversion against program version */
	if (idi->dbinfo.dbversion == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check() dbversion is NULL\n");
		syslog(LOG_ERR, "Error: DB Version cannot be retrieved. Please check the upgrade docs and verify the db schema!");
		return IDO_ERROR;
	}
	if (strcmp(idi->dbinfo.dbversion, IDO_SCHEMA_VERSION) != 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check() db version %s does not match schema version %s\n", idi->dbinfo.dbversion, IDO_SCHEMA_VERSION);
		syslog(LOG_ERR, "Error: DB Version %s does not match needed schema version %s. Please check the upgrade docs!", idi->dbinfo.dbversion, IDO_SCHEMA_VERSION);
		return IDO_ERROR;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check () end\n");

	return result;

}

int ido2db_db_hello(ido2db_idi *idi) {
#ifdef USE_LIBDBI
	char *buf = NULL;
	char *buf1 = NULL;
	char *ts = NULL;
#endif
	int result = IDO_OK;
	int have_instance = IDO_FALSE;
	time_t current_time;

#ifdef USE_ORACLE
	//unsigned long n_zero = 0;
	void *data[9];
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() start\n");

	/* make sure we have an instance name */
	if (idi->instance_name == NULL)
		idi->instance_name = strdup("default");


	result = ido2db_db_version_check(idi);

	if (result == IDO_ERROR) {
                syslog(LOG_USER | LOG_INFO, "Error: DB Version Check against %s database query failed! Please check %s database configuration and schema!", ido2db_db_settings.dbserver, ido2db_db_settings.dbserver);
                syslog(LOG_USER | LOG_INFO, "Exiting ...");

                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_version_check() query against existing instance not possible, cleaning up and exiting\n");

		return IDO_ERROR;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */

	/* get existing instance */
	if (asprintf(&buf, "SELECT instance_id FROM %s WHERE instance_name='%s'",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_INSTANCES], idi->instance_name)
	        == -1)
		buf = NULL;

	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {

		if (idi->dbinfo.dbi_result != NULL) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
				idi->dbinfo.instance_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "instance_id");
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(instance_id=%lu)\n", idi->dbinfo.instance_id);
				have_instance = IDO_TRUE;
			}
		}
	} else {
		/* there was an error with the initial db handshake, bail out */
		/* could be missing database, missing db schema */
		syslog(LOG_USER | LOG_INFO, "Error: Initial %s database query failed! Please check %s database configuration and schema!", ido2db_db_settings.dbserver, ido2db_db_settings.dbserver);
		syslog(LOG_USER | LOG_INFO, "Exiting ...");

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() query against existing instance not possible, cleaning up and exiting\n");

		/* bail out, but do not exit the child yet */
		return IDO_ERROR;
	}

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
	free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */
	//FIXME

	syslog(LOG_USER | LOG_INFO, "Error: Initial pgsql database query failed! Please check pgsql database configuration and schema!");
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* get existing instance */
	data[0] = (void *) &idi->instance_name;

	if (!OCI_BindString(idi->dbinfo.oci_statement_instances_select, MT(":X1"), *(char **) data[0], 0)) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_instances_select)) {
		/* there was an error with the initial db handshake, bail out */
		/* could be missing database, missing db schema */
		syslog(LOG_USER | LOG_INFO, "Error: Initial oracle database query failed! Please check oracle database configuration and schema!");
		syslog(LOG_USER | LOG_INFO, "Exiting ...");

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() query against existing instance not possible, cleaning up and exiting\n");

		/* bail out, but do not exit the child yet */
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_instances_select);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() query ok\n");

	if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() fetchnext ok\n");
		idi->dbinfo.instance_id	= OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 1);

		if (idi->dbinfo.instance_id == 0) {
			have_instance = IDO_FALSE;
		} else {
			have_instance = IDO_TRUE;
		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() fetchnext not ok\n");
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(instance_id=%lu)\n", idi->dbinfo.instance_id);


#endif /* Oracle ocilib specific */


	/* insert new instance if necessary */
	if (have_instance == IDO_FALSE) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(instance_id=%lu)->insert new db entry\n", idi->dbinfo.instance_id);
#ifdef USE_LIBDBI /* everything else will be libdbi */
		if (asprintf(&buf, "INSERT INTO %s (instance_name) VALUES ('%s')", ido2db_db_tablenames[IDO2DB_DBTABLE_INSTANCES], idi->instance_name) == -1)
			buf = NULL;
		if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {

			switch (idi->dbinfo.server_type) {
			case IDO2DB_DBSERVER_MYSQL:
				/* mysql doesn't use sequences */
				idi->dbinfo.instance_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
				break;
			case IDO2DB_DBSERVER_PGSQL:
				/* depending on tableprefix/tablename a sequence will be used */
				if (asprintf(&buf1, "%s_instance_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_INSTANCES]) == -1)
					buf1 = NULL;

				idi->dbinfo.instance_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(%s=%lu) instance_id\n", buf1, idi->dbinfo.instance_id);
				free(buf1);
				break;
			case IDO2DB_DBSERVER_DB2:
				break;
			case IDO2DB_DBSERVER_FIREBIRD:
				break;
			case IDO2DB_DBSERVER_FREETDS:
				break;
			case IDO2DB_DBSERVER_INGRES:
				break;
			case IDO2DB_DBSERVER_MSQL:
				break;
			case IDO2DB_DBSERVER_ORACLE:
				break;
			case IDO2DB_DBSERVER_SQLITE:
				break;
			case IDO2DB_DBSERVER_SQLITE3:
				break;
			default:
				break;
			}
		}
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;

		free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		data[0] = (void *) &idi->instance_name;

		if (!OCI_BindString(idi->dbinfo.oci_statement_instances, MT(":X1"), *(char **) data[0], 0)) {
			return IDO_ERROR;
		}

		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_instances)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_instances() execute error\n");
			return IDO_ERROR;
		}

		OCI_Commit(idi->dbinfo.oci_connection);

		idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_instances);

		if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
			idi->dbinfo.instance_id = OCI_GetInt(idi->dbinfo.oci_resultset, 1);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(%lu) instance_id\n", idi->dbinfo.instance_id);
		} else {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() instance_id could not be fetched\n");
		}

		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	}

#ifdef USE_LIBDBI
	/* set session time zone to utc */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() set session timezone\n");
	switch (idi->dbinfo.server_type) {
	       case IDO2DB_DBSERVER_MYSQL:
		       if (asprintf(&buf, "SET SESSION TIME_ZONE='+00:00'")==-1) buf=NULL;
		       break;
	       case IDO2DB_DBSERVER_PGSQL:
		       if (asprintf(&buf, "SET SESSION TIMEZONE='UTC'")==-1) buf=NULL;
		       break;
           default:
	           break;
	}
	if (buf !=NULL) {
		//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(timezone) execute %s\n",buf);
		if (ido2db_db_query(idi, buf)== IDO_OK){
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(timezone) UTC set OK\n");
		}else{
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(timezone) Error: TimeZone Set to UTC failed\n");
		}
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);
	}else{
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(timezone) Error: buffer allocation failed\n");
	}

#endif

	/* record initial connection information */
#ifdef USE_LIBDBI /* everything else will be libdbi */
	//FIXME why we need ts here?
	ts = ido2db_db_timet_to_sql(idi, idi->data_start_time);

	if (asprintf(&buf, "INSERT INTO %s (instance_id, connect_time, last_checkin_time, bytes_processed, lines_processed, entries_processed, "
			"agent_name, agent_version, disposition, connect_source, connect_type, data_start_time) "
			"VALUES (%lu, NOW(), NOW(), '0', '0', '0', '%s', '%s', '%s', '%s', '%s', NOW())",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONNINFO],
	             idi->dbinfo.instance_id, idi->agent_name, idi->agent_version,
	             idi->disposition, idi->connect_source, idi->connect_type) == -1)
		buf = NULL;

	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {

		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			idi->dbinfo.conninfo_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(%lu) conninfo_id\n", idi->dbinfo.conninfo_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf1, "%s_conninfo_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONNINFO]) == -1)
				buf1 = NULL;

			idi->dbinfo.conninfo_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(%s=%lu) conninfo_id\n", buf1, idi->dbinfo.conninfo_id);
			free(buf1);
			break;
		case IDO2DB_DBSERVER_DB2:
			break;
		case IDO2DB_DBSERVER_FIREBIRD:
			break;
		case IDO2DB_DBSERVER_FREETDS:
			break;
		case IDO2DB_DBSERVER_INGRES:
			break;
		case IDO2DB_DBSERVER_MSQL:
			break;
		case IDO2DB_DBSERVER_ORACLE:
			break;
		case IDO2DB_DBSERVER_SQLITE:
			break;
		case IDO2DB_DBSERVER_SQLITE3:
			break;
		default:
			break;
		}
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}

	free(buf);
	free(ts);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	data[0] = (void *) &idi->dbinfo.instance_id;
	/* zero already set in sql
	data[1] = (void *) &n_zero;
	data[2] = (void *) &n_zero;
	data[3] = (void *) &n_zero;
	*/
	data[4] = (void *) &idi->agent_name;
	data[5] = (void *) &idi->agent_version;
	data[6] = (void *) &idi->disposition;
	data[7] = (void *) &idi->connect_source;
	data[8] = (void *) &idi->connect_type;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	/*
	if(!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if(!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if(!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	    */
	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[5] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[6] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X7") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X7"), *(char **) data[6], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[7] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X8") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X8"), *(char **) data[7], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[8] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X9") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X9"), *(char **) data[8], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_conninfo)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_conninfo() execute error\n");
		return IDO_ERROR;
	}
	OCI_Commit(idi->dbinfo.oci_connection);

	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_conninfo);

	if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
		idi->dbinfo.conninfo_id = OCI_GetInt(idi->dbinfo.oci_resultset, 1);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello(%lu) conninfo_id\n", idi->dbinfo.conninfo_id);
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() conninfo_id could not be fetched\n");
	}

	/* do not free statement yet! */
	/* set oracle session application info fields and Timezone*/
	ido2db_oci_set_session_info(idi->dbinfo.oci_connection, idi->agent_name);
#endif /* Oracle ocilib specific */


	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() get cached object ids\n");
	/* get cached object ids... */
	ido2db_get_cached_object_ids(idi);

	/* get latest times from various tables... */
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_program_status_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTSTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_host_status_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICESTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_service_status_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTSTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_contact_status_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTQUEUE], "queued_time", (unsigned long *) &idi->dbinfo.latest_queued_event_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS], "entry_time", (unsigned long *) &idi->dbinfo.latest_comment_time);

	/* calculate time of latest realtime data */
	idi->dbinfo.latest_realtime_data_time = (time_t) 0L;
	if (idi->dbinfo.latest_program_status_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_program_status_time;
	if (idi->dbinfo.latest_host_status_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_host_status_time;
	if (idi->dbinfo.latest_service_status_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_service_status_time;
	if (idi->dbinfo.latest_contact_status_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_contact_status_time;
	if (idi->dbinfo.latest_queued_event_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_queued_event_time;

	/* get current time */
	/* make sure latest time stamp isn't in the future - this will cause problems if a backwards system time change occurs */
	time(&current_time);
	if (idi->dbinfo.latest_realtime_data_time > current_time)
		idi->dbinfo.latest_realtime_data_time = current_time;

	/* set flags to clean event queue, etc. */
	idi->dbinfo.clean_event_queue = IDO_TRUE;

	/* set misc data */
	idi->dbinfo.last_notification_id = 0L;
	idi->dbinfo.last_contact_notification_id = 0L;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_hello() end\n");
	return result;
}

/************************************/
/* threading post-connect routines  */
/************************************/
int ido2db_thread_db_hello(ido2db_idi *idi) {
#ifdef USE_LIBDBI
	char *buf = NULL;
	char *buf1 = NULL;
	char *ts = NULL;
#endif
	int result = IDO_OK;
	int have_instance = IDO_FALSE;
	time_t current_time;

#ifdef USE_ORACLE
	unsigned long n_zero = 0;
	void *data[9];
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() start\n");

	/* make sure we have an instance name */
	if (idi->instance_name == NULL)
		idi->instance_name = strdup("default");

#ifdef USE_LIBDBI /* everything else will be libdbi */

	/* get existing instance */
	if (asprintf(&buf, "SELECT instance_id FROM %s WHERE instance_name='%s'",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_INSTANCES], idi->instance_name)
	        == -1)
		buf = NULL;

	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {

		if (idi->dbinfo.dbi_result != NULL) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
				idi->dbinfo.instance_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "instance_id");
				have_instance = IDO_TRUE;
			}
		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() query against existing instance not possible, cleaning up and exiting\n");

		/* bail out, but do not exit the child yet - not allowed as thread */
		return IDO_ERROR;
	}

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
	free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* get existing instance */
	data[0] = (void *) &idi->instance_name;

	if (!OCI_BindString(idi->dbinfo.oci_statement_instances_select, MT(":X1"), *(char **) data[0], 0)) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_instances_select)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() query against existing instance not possible, cleaning up and exiting\n");

		/* bail out, but do not exit the child yet - not allowed as thread */
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_instances_select);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() query ok\n");

	if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() fetchnext ok\n");
		idi->dbinfo.instance_id = OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 1);

		if (idi->dbinfo.instance_id == 0) {
			have_instance = IDO_FALSE;
		} else {
			have_instance = IDO_TRUE;
		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() fetchnext not ok\n");
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello(instance_id=%lu)\n", idi->dbinfo.instance_id);


#endif /* Oracle ocilib specific */


	/* check if instance found */
	if (have_instance == IDO_FALSE) {
		return IDO_ERROR;
	}

#ifdef USE_LIBDBI
	/* set session time zone to utc */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() set session timezone\n");
	switch (idi->dbinfo.server_type) {
		   case IDO2DB_DBSERVER_MYSQL:
			   if (asprintf(&buf, "SET SESSION TIME_ZONE='+00:00'")==-1) buf=NULL;
			   break;
		   case IDO2DB_DBSERVER_PGSQL:
			   if (asprintf(&buf, "SET SESSION TIMEZONE='UTC'")==-1) buf=NULL;
			   break;
		   default:
					break;
	}
	if (buf !=NULL) {
		//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello(timezone) execute %s\n",buf);
		if (ido2db_db_query(idi, buf)== IDO_OK){
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello(timezone) UTC set OK\n");
		}else{
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello(timezone) Error: TimeZone Set to UTC failed\n");
		}
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);
	}else{
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello(timezone) Error: buffer allocation failed\n");
	}

#endif

	/* record initial connection information */
#ifdef USE_LIBDBI /* everything else will be libdbi */
	//Fixme No ts needed!
	ts = ido2db_db_timet_to_sql(idi, idi->data_start_time);

	if (asprintf(&buf, "INSERT INTO %s (instance_id, connect_time, last_checkin_time, bytes_processed, lines_processed, entries_processed, "
	      		"agent_name, agent_version, disposition, connect_source, connect_type, data_start_time) "
	       		"VALUES (%lu, NOW(), NOW(), '0', '0', '0', '%s', '%s', '%s', '%s', '%s', NOW())",	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONNINFO],
	       	idi->dbinfo.instance_id, idi->agent_name, idi->agent_version,
	        idi->disposition, idi->connect_source, idi->connect_type) == -1)
		buf = NULL;

	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {

		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			idi->dbinfo.conninfo_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello(%lu) conninfo_id\n", idi->dbinfo.conninfo_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf1, "%s_conninfo_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONNINFO]) == -1)
				buf1 = NULL;

			idi->dbinfo.conninfo_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello(%s=%lu) conninfo_id\n", buf1, idi->dbinfo.conninfo_id);
			free(buf1);
			break;
		case IDO2DB_DBSERVER_DB2:
			break;
		case IDO2DB_DBSERVER_FIREBIRD:
			break;
		case IDO2DB_DBSERVER_FREETDS:
			break;
		case IDO2DB_DBSERVER_INGRES:
			break;
		case IDO2DB_DBSERVER_MSQL:
			break;
		case IDO2DB_DBSERVER_ORACLE:
			break;
		case IDO2DB_DBSERVER_SQLITE:
			break;
		case IDO2DB_DBSERVER_SQLITE3:
			break;
		default:
			break;
		}
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}

	free(buf);
	free(ts);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	n_zero = 0;

	data[0] = (void *) &idi->dbinfo.instance_id;
	/* already in sql
	data[1] = (void *) &n_zero;
	data[2] = (void *) &n_zero;
	data[3] = (void *) &n_zero;
	*/
	data[4] = (void *) &idi->agent_name;
	data[5] = (void *) &idi->agent_version;
	data[6] = (void *) &idi->disposition;
	data[7] = (void *) &idi->connect_source;
	data[8] = (void *) &idi->connect_type;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	/*
	if(!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo, MT(":X2"), (uint *) data[1])) {
	        return IDO_ERROR;
	}
	if(!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo, MT(":X3"), (uint *) data[2])) {
	        return IDO_ERROR;
	}
	if(!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo, MT(":X4"), (uint *) data[3])) {
	        return IDO_ERROR;
	}
	*/
	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[5] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[6] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X7") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X7"), *(char **) data[6], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[7] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X8") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X8"), *(char **) data[7], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[8] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_conninfo, ":X9") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_conninfo, MT(":X9"), *(char **) data[8], 0)) {
			return IDO_ERROR;
		}
	}


	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_conninfo)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_conninfo() execute error\n");
		return IDO_ERROR;
	}
	OCI_Commit(idi->dbinfo.oci_connection);

	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_conninfo);

	if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
		idi->dbinfo.conninfo_id = OCI_GetInt(idi->dbinfo.oci_resultset, 1);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello(%lu) conninfo_id\n", idi->dbinfo.conninfo_id);
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() conninfo_id could not be fetched\n");
	}

	/* do not free statement yet! */

	/* set oracle session application info fields and Timezone */
	ido2db_oci_set_session_info(idi->dbinfo.oci_connection, idi->agent_name);

#endif /* Oracle ocilib specific */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() get cached object ids\n");
	/* get cached object ids... */
	ido2db_get_cached_object_ids(idi);

	/* get latest times from various tables... */
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_program_status_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTSTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_host_status_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICESTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_service_status_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTSTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_contact_status_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTQUEUE], "queued_time", (unsigned long *) &idi->dbinfo.latest_queued_event_time);
	ido2db_db_get_latest_data_time(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS], "entry_time", (unsigned long *) &idi->dbinfo.latest_comment_time);

	/* calculate time of latest realtime data */
	idi->dbinfo.latest_realtime_data_time = (time_t) 0L;
	if (idi->dbinfo.latest_program_status_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_program_status_time;
	if (idi->dbinfo.latest_host_status_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_host_status_time;
	if (idi->dbinfo.latest_service_status_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_service_status_time;
	if (idi->dbinfo.latest_contact_status_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_contact_status_time;
	if (idi->dbinfo.latest_queued_event_time > idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time = idi->dbinfo.latest_queued_event_time;

	/* get current time */
	/* make sure latest time stamp isn't in the future - this will cause problems if a backwards system time change occurs */
	time(&current_time);
	if (idi->dbinfo.latest_realtime_data_time > current_time)
		idi->dbinfo.latest_realtime_data_time = current_time;

	/* set flags to clean event queue, etc. */
	idi->dbinfo.clean_event_queue = IDO_TRUE;

	/* set misc data */
	idi->dbinfo.last_notification_id = 0L;
	idi->dbinfo.last_contact_notification_id = 0L;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_thread_db_hello() end\n");
	return result;
}




/************************************/
/* pre-disconnect routines          */
/************************************/
int ido2db_db_goodbye(ido2db_idi *idi) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char *buf = NULL;
	char *ts = NULL;
#endif

#ifdef USE_ORACLE
	big_int bytes_processed;
	big_int lines_processed;
	big_int entries_processed;
	void *data[5];
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_goodbye() start\n");

#ifdef USE_LIBDBI /* everything else will be libdbi */
	ts = ido2db_db_timet_to_sql(idi, idi->data_end_time);

	/* record last connection information */
	if (asprintf(&buf, "UPDATE %s SET disconnect_time=NOW(), last_checkin_time=NOW(), data_end_time=%s, bytes_processed=%lu, lines_processed=%lu, entries_processed=%lu WHERE conninfo_id=%lu",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONNINFO], ts,
	             idi->bytes_processed, idi->lines_processed, idi->entries_processed,
	             idi->dbinfo.conninfo_id) == -1)
		buf = NULL;

	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;

	free(buf);
	free(ts);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	bytes_processed = idi->bytes_processed;
	lines_processed = idi->lines_processed;
	entries_processed = idi->entries_processed;
	data[0] = (void *) &idi->data_end_time;
	data[1] = (void *) &bytes_processed;
	data[2] = (void *) &lines_processed;
	data[3] = (void *) &entries_processed;
	data[4] = (void *) &idi->dbinfo.conninfo_id;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo_update, MT(":X1"), (uint *) data[0])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedBigInt(idi->dbinfo.oci_statement_conninfo_update, MT(":X2"), (big_uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedBigInt(idi->dbinfo.oci_statement_conninfo_update, MT(":X3"), (big_uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedBigInt(idi->dbinfo.oci_statement_conninfo_update, MT(":X4"), (big_uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo_update, MT(":X5"), (uint *) data[4])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_conninfo_update)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_conninfo_update() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_goodbye() end\n");
	return result;
}

/************************************/
/* checking routines                */
/************************************/
int ido2db_db_checkin(ido2db_idi *idi) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif

#ifdef USE_ORACLE
	big_int bytes_processed;
	big_int lines_processed;
	big_int entries_processed;
	void *data[4];
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_checkin() start\n");

	/* record last connection information */
#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (asprintf(&buf, "UPDATE %s SET last_checkin_time=NOW(), bytes_processed=%lu, lines_processed=%lu, entries_processed=%lu WHERE conninfo_id=%lu",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONNINFO],
	             idi->bytes_processed, idi->lines_processed, idi->entries_processed,
	             idi->dbinfo.conninfo_id) == -1)
		buf = NULL;

	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
	free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	bytes_processed = idi->bytes_processed;
	lines_processed = idi->lines_processed;
	entries_processed = idi->entries_processed;

	data[0] = (void *) &bytes_processed;
	data[1] = (void *) &lines_processed;
	data[2] = (void *) &entries_processed;
	data[3] = (void *) &idi->dbinfo.conninfo_id;

	if (!OCI_BindUnsignedBigInt(idi->dbinfo.oci_statement_conninfo_update_checkin, MT(":X1"), (big_uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedBigInt(idi->dbinfo.oci_statement_conninfo_update_checkin, MT(":X2"), (big_uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedBigInt(idi->dbinfo.oci_statement_conninfo_update_checkin, MT(":X3"), (big_uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_conninfo_update_checkin, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_conninfo_update_checkin)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_conninfo_update_checkin() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

#endif /* Oracle ocilib specific */


	time(&ido2db_db_last_checkin_time);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_checkin() end\n");
	return result;
}

/****************************************************************************/
/* MISC FUNCTIONS                                                           */
/****************************************************************************/

/***************************************/
/* escape a string for a SQL statement */
/***************************************/
char *ido2db_db_escape_string(ido2db_idi *idi, char *buf) {
	register int x, y, z;
	char *newbuf = NULL;

	if (idi == NULL || buf == NULL)
		return NULL;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_escape_string('%s') start\n", buf);

	/* init variables to make compilers happy */
	x=0;
	y=0;

	z = strlen(buf);
	if ((newbuf = (char *) malloc((z * 2) + 1)) == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_escape_string('%s') Error: memory allocation failed \n", buf);
		return NULL;
	}

#ifdef USE_ORACLE
	/* oracle doesnt need escaping because of bind variables,
	 * but we need to allocate the buffer #2534
	 * */
	strcpy(newbuf,buf);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_escape_string oracle changed string ('%s')\n", newbuf);
	return newbuf;
#endif


	/* escape characters */
#ifdef USE_LIBDBI /* everything else will be libdbi */
	/* allocate space for the new string */


	for (x = 0, y = 0; x < z; x++) {

		if (idi->dbinfo.server_type == IDO2DB_DBSERVER_MYSQL) {

			if (buf[x] == '\'' || buf[x] == '\"' || buf[x] == '*' || buf[x] == '\\' || buf[x] == '$' || buf[x] == '?' || buf[x] == '.' || buf[x] == '^' || buf[x] == '+' || buf[x] == '[' || buf[x] == ']' || buf[x] == '(' || buf[x] == ')')
				newbuf[y++] = '\\';
		} else if (idi->dbinfo.server_type == IDO2DB_DBSERVER_PGSQL) {

			if (buf[x] == '\'' || buf[x] == '\\' || buf[x] == '\0')
				newbuf[y++] = '\\';
		} else {

			if (buf[x] == '\'')
				newbuf[y++] = '\'';

		}

		newbuf[y++] = buf[x];
	}

	/* terminate escape string */
	newbuf[y] = '\0';

#endif

#ifdef USE_PGSQL /* pgsql */

	for (x = 0, y = 0; x < z; x++) {

		if (buf[x] == '\'' || buf[x] == '[' || buf[x] == ']' || buf[x] == '(' || buf[x] == ')')
			newbuf[y++] = '\\';

		newbuf[y++] = buf[x];
	}

	/* terminate escape string */
	newbuf[y] = '\0';

#endif

	/* terminate escape string */
	newbuf[y] = '\0';
	if (strcmp(buf, newbuf) != 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_escape_string changed string ('%s')\n", newbuf);
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_escape_string end\n");
	return newbuf;
}

/*************************************************************/
/* SQL query conversion of time_t format to date/time format */
/*************************************************************/
char *ido2db_db_timet_to_sql(ido2db_idi *idi, time_t t) {

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/**
	* oracle doesn't use this function currently
		*/
	return NULL;
#else /* Oracle ocilib specific */
	char *buf = NULL;
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_timet_to_sql(%lu) start\n", (unsigned long)t);

	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		/* mysql from_unixtime treats 0 as 'Out of range value for column '...' at row 1'
		 * which basically is a mess, when doing updates at all. in order to stay sane, we
		 * set the value explicitely to NULL. mysql, you suck hard.
		 */
		if (t == 0) {
			if (asprintf(&buf, "FROM_UNIXTIME(NULL)") == -1)
			buf = NULL;
		} else {
			if (asprintf(&buf, "FROM_UNIXTIME(%lu)", (unsigned long) t) == -1)
				buf = NULL;
		}
		break;
	case IDO2DB_DBSERVER_PGSQL:
		/* from_unixtime is a PL/SQL function (defined in db/pgsql.sql) */
		if (asprintf(&buf, "FROM_UNIXTIME(%lu)", (unsigned long) t) == -1)
			buf = NULL;
		break;
	case IDO2DB_DBSERVER_DB2:
		break;
	case IDO2DB_DBSERVER_FIREBIRD:
		break;
	case IDO2DB_DBSERVER_FREETDS:
		break;
	case IDO2DB_DBSERVER_INGRES:
		break;
	case IDO2DB_DBSERVER_MSQL:
		break;
	case IDO2DB_DBSERVER_ORACLE:



		break;
	case IDO2DB_DBSERVER_SQLITE:
		break;
	case IDO2DB_DBSERVER_SQLITE3:
		break;
	default:
		break;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_timet_to_sql(%s) end\n", buf);

	return buf;
#endif
}

/*************************************************************/
/* SQL query conversion of date/time format to time_t format */
/*************************************************************/
char *ido2db_db_sql_to_timet(ido2db_idi *idi, char *field) {
#ifdef USE_ORACLE /* Oracle ocilib specific */
	/**
	 * oracle doesn't use this function currently
	*/
	return NULL;
#else/* all others */
	char *buf = NULL;
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_sql_to_timet(%s) start\n", field);

	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		if (asprintf(&buf, "UNIX_TIMESTAMP(%s)", (field == NULL) ? "" : field) == -1)
			buf = NULL;
		break;
	case IDO2DB_DBSERVER_PGSQL:
		/* unix_timestamp is a PL/SQL function (defined in db/pgsql.sql) */
		if (asprintf(&buf, "UNIX_TIMESTAMP(%s)", (field == NULL) ? "" : field) == -1)
			buf = NULL;
		break;
	case IDO2DB_DBSERVER_DB2:
		break;
	case IDO2DB_DBSERVER_FIREBIRD:
		break;
	case IDO2DB_DBSERVER_FREETDS:
		break;
	case IDO2DB_DBSERVER_INGRES:
		break;
	case IDO2DB_DBSERVER_MSQL:
		break;
	case IDO2DB_DBSERVER_ORACLE:


		break;
	case IDO2DB_DBSERVER_SQLITE:
		break;
	case IDO2DB_DBSERVER_SQLITE3:
		break;
	default:
		break;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_sql_to_timet(%s) end\n", buf);

	return buf;
#endif
}

/************************************/
/* executes a SQL statement         */
/************************************/
int ido2db_db_query(ido2db_idi *idi, char *buf) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	const char *error_msg;
#endif

#ifdef USE_ORACLE
	int oci_res = 0;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_query() start\n");

	if (idi == NULL || buf == NULL)
		return IDO_ERROR;

	/* if we're not connected, try and reconnect... */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;


#ifdef DEBUG_IDO2DB_QUERIES
	printf("%s\n\n", buf);
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 0, "%s\n", buf);

#ifdef USE_LIBDBI /* everything else will be libdbi */

	/* send query */
	idi->dbinfo.dbi_result = dbi_conn_query(idi->dbinfo.dbi_conn, buf);

	if (idi->dbinfo.dbi_result == NULL) {
		dbi_conn_error(idi->dbinfo.dbi_conn, &error_msg);

		syslog(LOG_USER | LOG_INFO, "Error: database query failed for '%s' - '%s'\n", buf, error_msg);

		ido2db_handle_db_error(idi);
		result = IDO_ERROR;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

	idi->dbinfo.pg_result = PQexec(idi->dbinfo.pg_conn, buf);

	if (PQresultStatus(idi->dbinfo.pg_result) != PGRES_COMMAND_OK) {
		syslog(LOG_USER | LOG_INFO, "Error: database query failed for '%s': %s\n", buf, PQerrorMessage(idi->dbinfo.pg_conn));
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "Error: database query failed for '%s': %s\n", buf, PQerrorMessage(idi->dbinfo.pg_conn));

		ido2db_handle_db_error(idi);
		result = IDO_ERROR;
	}
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	oci_res = 0;

	/* create statement handler */
	idi->dbinfo.oci_statement = OCI_StatementCreate(idi->dbinfo.oci_connection);

	/* execute query in one go */
	oci_res = OCI_ExecuteStmt(idi->dbinfo.oci_statement, MT(buf));

	/*  get result set */
	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement);

	/* check for errors */
	if (!oci_res) {

		syslog(LOG_USER | LOG_INFO, "Error: database query failed for '%s'\n", buf);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "Error: database query failed for: '%s'\n", buf);

		ido2db_handle_db_error(idi);
		result = IDO_ERROR;
	}

	/* since we do not want to set auto commit to true, we do it manually */
	OCI_Commit(idi->dbinfo.oci_connection);

#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_query(%d) end\n", result);
	return result;
}

/****************************************/
/* frees memory associated with a query */
/****************************************/
int ido2db_db_free_query(ido2db_idi *idi) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_free_query() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* ToDo: Examine where this function is called
	 * Not freeing the query may result in memory leaks
	 **/

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_free_query() end\n");
	return IDO_OK;
}

/************************************/
/* handles SQL query errors         */
/************************************/
int ido2db_handle_db_error(ido2db_idi *idi) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_db_error() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* we're not currently connected... */
	if (ido2db_db_is_connected(idi) == IDO_TRUE)
		return IDO_OK;

	ido2db_db_disconnect(idi);

	idi->disconnect_client = IDO_TRUE;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_db_error() end\n");
	return IDO_OK;
}

/**********************************************************/
/* clears data from a given table (current instance only) */
/**********************************************************/
int ido2db_db_clear_table(ido2db_idi *idi, char *table_name) {
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif
	int result = IDO_OK;
#ifdef USE_ORACLE
	void *data[2];
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_clear_table(%s) start\n", table_name);

	if (idi == NULL || table_name == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_clear_table(%s) Error:IDI or table name NULL\n", table_name);
		return IDO_ERROR;
	}


#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (asprintf(&buf, "DELETE FROM %s WHERE instance_id=%lu", table_name, idi->dbinfo.instance_id) == -1)
		buf = NULL;

	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
	free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* procedure approach */
	data[0] = (void *) &table_name;
	data[1] = (void *) &idi->dbinfo.instance_id;

	if (!OCI_BindString(idi->dbinfo.oci_statement_instances_delete, MT(":X1"), *(char **) data[0], 0)) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_instances_delete, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}

	if (!OCI_Execute(idi->dbinfo.oci_statement_instances_delete)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_instances_delete() execute error\n");
		return IDO_ERROR;
	}

	OCI_Commit(idi->dbinfo.oci_connection);

#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_clear_table(%s) end\n", table_name);
	return result;
}

/**************************************************/
/* gets latest data time value from a given table */
/**************************************************/
int ido2db_db_get_latest_data_time(ido2db_idi *idi, char *table_name, char *field_name, unsigned long *t) {
	char *buf = NULL;
	char * fname = "ido2db_db_get_latest_data_time";
	int result = IDO_OK;
#ifdef USE_ORACLE
	unsigned int instance_id;
#endif

	*t = (time_t) 0L;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() start\n", fname);

	if (idi == NULL || table_name == NULL || field_name == NULL || t == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() Error:at least one parameter is NULL\n", fname);
		return IDO_ERROR;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */


	if (asprintf(&buf, "SELECT %s AS latest_time FROM %s WHERE instance_id=%lu ORDER BY %s DESC LIMIT 1 OFFSET 0",
	             field_name, table_name, idi->dbinfo.instance_id, field_name) == -1)
		buf = NULL;

	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {
		if (idi->dbinfo.dbi_result != NULL) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
				*t = dbi_result_get_datetime(idi->dbinfo.dbi_result, "latest_time");
			}
		}
	}

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	instance_id = idi->dbinfo.instance_id;

	idi->dbinfo.oci_statement = OCI_StatementCreate(idi->dbinfo.oci_connection);
	if (asprintf(&buf, "select max(localts2unixts(%s)) from %s where instance_id=:ID", field_name, table_name) == -1) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() Error:Memory allocation SQL failed\n", fname);
		free(buf);
		return IDO_ERROR;
	}
	if (!(OCI_Prepare(idi->dbinfo.oci_statement, buf))) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() Error:Prepare SQL [ %s ] failed\n", fname, buf);
		ido2db_oci_statement_free(idi->dbinfo.oci_statement, fname);
		free(buf);
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s() SQL [ %s ] prepared\n", fname, buf);
	if (!(OCI_BindUnsignedInt(idi->dbinfo.oci_statement, MT(":ID"), &instance_id))) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() Error:Bind instance_ID(%llu) failed\n", fname, instance_id);
		ido2db_oci_statement_free(idi->dbinfo.oci_statement, fname);
		free(buf);
		return IDO_ERROR;
	}
	if ((ido2db_oci_execute_out(idi->dbinfo.oci_statement, fname)) == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() Error:execute failed\n", fname);
		ido2db_oci_statement_free(idi->dbinfo.oci_statement, fname);
		free(buf);
		return IDO_ERROR;
	}
	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement);

	if (idi->dbinfo.oci_resultset != NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s query ok\n", fname);
		/* check if next row */
		if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
			*t = OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 1);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() latest=%lu\n", fname, *t);
		} else {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() Nothing to fetch\n", fname);
		}
	}


#endif /* Oracle ocilib specific */

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif
#ifdef USE_ORACLE /* Oracle ocilib specific */
	ido2db_oci_statement_free(idi->dbinfo.oci_statement, fname);
#endif /* Oracle ocilib specific */

	free(buf);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() end\n", fname);
	return result;
}

/*******************************************/
/* trim/delete old data from a given table */
/*******************************************/
int ido2db_db_trim_data_table(ido2db_idi *idi, char *table_name, char *field_name, unsigned long t) {
	char *buf = NULL;
	char *ts[1];
	int result = IDO_OK;

#ifdef USE_ORACLE
	void *data[4];
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_trim_data_table() start, time=%lu\n", t);

	if (idi == NULL || table_name == NULL || field_name == NULL)
		return IDO_ERROR;

	ts[0] = ido2db_db_timet_to_sql(idi, (time_t) t);

#ifdef USE_LIBDBI /* everything else will be libdbi */

	if (asprintf(&buf, "DELETE FROM %s WHERE instance_id=%lu AND %s<%s",
	             table_name, idi->dbinfo.instance_id, field_name, ts[0]) == -1)
		buf = NULL;

	result = ido2db_db_query(idi, buf);
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	data[0] = (void *) &table_name;
	data[1] = (void *) &idi->dbinfo.instance_id;
	data[2] = (void *) &field_name;
	data[3] = (void *) &t;

	if (!OCI_BindString(idi->dbinfo.oci_statement_instances_delete_time, MT(":X1"), *(char **) data[0], 0)) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_instances_delete_time, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindString(idi->dbinfo.oci_statement_instances_delete_time, MT(":X3"), *(char **) data[2], 0)) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_instances_delete_time, MT(":X4"), (uint *) data[3])) { // unixtimestamp instead of time2sql
		return IDO_ERROR;
	}

	if (!OCI_Execute(idi->dbinfo.oci_statement_instances_delete_time)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_instances_delete_time() execute error\n");
		return IDO_ERROR;
	}

	OCI_Commit(idi->dbinfo.oci_connection);
#endif /* Oracle ocilib specific */

	free(buf);
	free(ts[0]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_trim_data_table(%s => %s: %lu) end\n", table_name, field_name, t);
	return result;
}

/***********************************************/
/* performs some periodic table maintenance... */
/***********************************************/
int ido2db_db_perform_maintenance(ido2db_idi *idi) {
	time_t current_time;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_perform_maintenance() start\n");

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_perform_maintenance() max_logentries_age=%lu, max_ack_age=%lu\n", idi->dbinfo.max_logentries_age, idi->dbinfo.max_logentries_age);

	/* get the current time */
	time(&current_time);

	/* trim tables */
	if (((unsigned long) current_time - idi->dbinfo.trim_db_interval) > (unsigned long) idi->dbinfo.last_table_trim_time) {
		if (idi->dbinfo.max_timedevents_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTS], "scheduled_time", (time_t)((unsigned long) current_time - idi->dbinfo.max_timedevents_age));
		if (idi->dbinfo.max_systemcommands_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SYSTEMCOMMANDS], "start_time", (time_t)((unsigned long) current_time - idi->dbinfo.max_systemcommands_age));
		if (idi->dbinfo.max_servicechecks_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECHECKS], "start_time", (time_t)((unsigned long) current_time - idi->dbinfo.max_servicechecks_age));
		if (idi->dbinfo.max_hostchecks_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCHECKS], "start_time", (time_t)((unsigned long) current_time - idi->dbinfo.max_hostchecks_age));
		if (idi->dbinfo.max_eventhandlers_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_EVENTHANDLERS], "start_time", (time_t)((unsigned long) current_time - idi->dbinfo.max_eventhandlers_age));
		if (idi->dbinfo.max_externalcommands_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_EXTERNALCOMMANDS], "entry_time", (time_t)((unsigned long)current_time - idi->dbinfo.max_externalcommands_age));
		if (idi->dbinfo.max_logentries_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_LOGENTRIES], "logentry_time", (time_t)((unsigned long)current_time - idi->dbinfo.max_logentries_age));
		if (idi->dbinfo.max_acknowledgements_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_ACKNOWLEDGEMENTS], "entry_time", (time_t)((unsigned long)current_time - idi->dbinfo.max_acknowledgements_age));
		if (idi->dbinfo.max_notifications_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS], "start_time", (time_t)((unsigned long) current_time - idi->dbinfo.max_notifications_age));
		if (idi->dbinfo.max_contactnotifications_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS], "start_time", (time_t)((unsigned long) current_time - idi->dbinfo.max_contactnotifications_age));
		if (idi->dbinfo.max_contactnotificationmethods_age > 0L)
			ido2db_db_trim_data_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS], "start_time", (time_t)((unsigned long) current_time - idi->dbinfo.max_contactnotificationmethods_age));
		idi->dbinfo.last_table_trim_time = current_time;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_db_perform_maintenance() end\n");
	return IDO_OK;
}

/************************************/
/* check database driver (libdbi)   */
/************************************/

#ifdef USE_LIBDBI /* everything else will be libdbi */

int ido2db_check_dbd_driver(void) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_check_dbd_driver() start\n");

	dbi_initialize(libdbi_driver_dir);

	switch (ido2db_db_settings.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		if ((dbi_driver_open(IDO2DB_DBI_DRIVER_MYSQL)) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	case IDO2DB_DBSERVER_PGSQL:
		if (dbi_driver_open(IDO2DB_DBI_DRIVER_PGSQL) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	case IDO2DB_DBSERVER_DB2:
		if (dbi_driver_open(IDO2DB_DBI_DRIVER_DB2) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	case IDO2DB_DBSERVER_FIREBIRD:
		if (dbi_driver_open(IDO2DB_DBI_DRIVER_FIREBIRD) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	case IDO2DB_DBSERVER_FREETDS:
		if (dbi_driver_open(IDO2DB_DBI_DRIVER_FREETDS) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	case IDO2DB_DBSERVER_INGRES:
		if (dbi_driver_open(IDO2DB_DBI_DRIVER_INGRES) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	case IDO2DB_DBSERVER_MSQL:
		if (dbi_driver_open(IDO2DB_DBI_DRIVER_MSQL) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	case IDO2DB_DBSERVER_ORACLE:

		if (dbi_driver_open(IDO2DB_DBI_DRIVER_ORACLE) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	case IDO2DB_DBSERVER_SQLITE:
		if (dbi_driver_open(IDO2DB_DBI_DRIVER_SQLITE) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	case IDO2DB_DBSERVER_SQLITE3:
		if (dbi_driver_open(IDO2DB_DBI_DRIVER_SQLITE3) == NULL) {
			dbi_shutdown();
			return IDO_FALSE;
		}
		break;
	default:
		dbi_shutdown();
		return IDO_FALSE;
		break;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_check_dbd_driver() end\n");
	return IDO_TRUE;
}
#endif

/************************************/
/* error handler (ocilib)           */
/************************************/


#ifdef USE_ORACLE /* Oracle ocilib specific */

void ido2db_ocilib_err_handler(OCI_Error *err) {
	OCI_Statement *st = NULL;
//	OCI_Error *arrerr;
	const mtext *sql = "";
	unsigned int  err_type;
	const char * err_msg;
	char * errt_msg = NULL;
	char * buf = NULL;
	char * binds = NULL;
//	int arrerrcount=0;
//	int arrsize=0;
	unsigned int err_pos = 0;
	err_type = OCI_ErrorGetType(err);
	err_msg  = OCI_ErrorGetString(err);
	st = OCI_ErrorGetStatement(err);
	switch (err_type) {
	case OCI_ERR_WARNING:
		errt_msg = strdup("OCIWARN");
		break;
	case OCI_ERR_ORACLE :
		errt_msg = strdup("OCIERROR");
		break;
	case OCI_ERR_OCILIB :
		errt_msg = strdup("OCILIB");
		break;
	default:
		errt_msg = strdup("OCIUNKNOWN");
	}
	if (st) {
		sql = OCI_GetSql(st);
		err_pos = OCI_GetSqlErrorPos(st);
		/* todo check array error implementation. OCI_BatchErrorCount throws errors */
		/*		arrsize=OCI_BindArrayGetSize(st);
				if (arrsize>0) {
					arrerrcount=OCI_GetBatchErrorCount(st);
					if (arrerrcount>0){
						ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%d Batch Errors detected\n",arrerrcount);
						arrerr=OCI_GetBatchError(st);
						while (arrerr){
							ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "Error at row %d : %s\n", OCI_ErrorGetRow(arrerr), OCI_ErrorGetString(arrerr));
							arrerr = OCI_GetBatchError(st);
						}
					}
				}
		*/
		if (OCI_GetBindCount(st) > 0) {
			binds = malloc(OCI_VARCHAR_SIZE * 16);
			if (binds == NULL) {
				binds = strdup("Error:Memory Allocation Error");
			} else {
				ido2db_oci_print_binds(st, sizeof(binds), (char **)binds);
				asprintf(&buf, "%s - MSG %s at pos %u in QUERY '%s' -->%s",
				         errt_msg, err_msg, err_pos, sql, binds);
				free(binds);
			}

		} else {
			asprintf(&buf, "%s - MSG %s at pos %u in QUERY '%s'",
			         errt_msg, err_msg, err_pos, sql);
		}
	} else {
		asprintf(&buf, "%s - MSG %s",
		         errt_msg, err_msg);
	}
	if (ido2db_db_settings.oci_errors_to_syslog == IDO_TRUE) {
		syslog(LOG_USER | LOG_INFO, "%s\n", buf);
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s\n", buf);
	free(buf);
}

/**
 * retrieving last sequence value for ids
 * This is multiprocess save because query should only get values
 * the same session increased just before
 *
 * @param idi structure
 * @param sequence_name
 * @return ulong currval for named sequence or zero on error
 */
unsigned long ido2db_oci_sequence_lastid(ido2db_idi *idi, char *seq_name) {

	unsigned long insert_id = 0L;
	char *buf = NULL;
	OCI_Statement *st=NULL;
	OCI_Resultset *rs=NULL;
	char * fname = "ido2db_oci_sequence_lastid";
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() start\n", fname);
	if (idi == NULL || seq_name == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() Error:At least one parameter is NULL\n", fname);
		return insert_id;
	}
	/* check connection */
	if (idi->dbinfo.oci_connection == NULL) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s Error:No Connection\n", fname);
			return insert_id;
	}

	/* create new handle */
	st = OCI_StatementCreate(idi->dbinfo.oci_connection);
	if (st == NULL) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s Error:Create Statement\n", fname);
			return insert_id;
	}
	/* prepare sql.
	 * unfortunally seqname is in the dynmaic part so statements are changed
	 * but the numer of seq is limited and cannot poison sga
	 * */
	if (asprintf(&buf, "SELECT %s.currval from dual",seq_name) == -1) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s Error:memory allocation failed\n", fname);
			buf = NULL;
			return insert_id;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s query: %s\n", fname, buf);
	if (!OCI_Prepare(st, MT(buf))) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s Error:prepare failed failed\n", fname);
			if (buf) free(buf);
			OCI_StatementFree(st);
			return insert_id;
	}

	free(buf);
	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() prepared\n",fname);
	/* execute statement */
	if (!OCI_Execute(st)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() execute error\n",fname);
			OCI_StatementFree(st);
			return insert_id;
	}
	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s() executed\n",fname);
	/*  get result set */
	rs = OCI_GetResultset(st);

	if (rs != NULL) {
		/* check if next row */
		if (OCI_FetchNext(rs)) {
			insert_id = OCI_GetUnsignedInt(rs, 1);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s(%s) id=%lu\n", fname, seq_name, insert_id);
		}
	}
	if (insert_id == 0L) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s(%s) Warning:No Result\n", fname, seq_name);
	}
	OCI_StatementFree(st);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s(%s) end\n", fname, seq_name);
	return insert_id;
}


/****************************************************************************/
/* PREPARED STATEMENTS                                                      */
/****************************************************************************/

/************************************/
/* INSTANCES                        */
/************************************/


int ido2db_oci_prepared_statement_instances(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_name) "
	             "VALUES (seq_instances.nextval, :X1) "
	             "RETURNING id INTO :id",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_INSTANCES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_instances = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_instances, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_instances, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		OCI_RegisterInt(idi->dbinfo.oci_statement_instances, ":id");
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_instances_select(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "SELECT id FROM %s WHERE instance_name=:X1",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_INSTANCES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_instances_select = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_instances_select, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_instances_select, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_dbversion_select(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_dbversion_select() start\n");

	if (asprintf(&buf, "SELECT version FROM %s WHERE name=:X1",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_DBVERSION]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_dbversion_select() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_dbversion_select = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_dbversion_select, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_dbversion_select, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_dbversion_select() end\n");

	return IDO_OK;
}


/************************************/
/* CONNINFO                         */
/************************************/


int ido2db_oci_prepared_statement_conninfo(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s "
	             "(id, instance_id, connect_time, last_checkin_time, "
	             "bytes_processed, lines_processed, entries_processed, "
	             "agent_name, agent_version, disposition, connect_source, "
	             "connect_type, data_start_time) "
	             "VALUES (seq_conninfo.nextval, :X1, LOCALTIMESTAMP, LOCALTIMESTAMP, "
	             "0, 0, 0, "
	             ":X5, :X6, :X7, :X8, "
	             ":X9, LOCALTIMESTAMP) RETURNING id INTO :id",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONNINFO]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_conninfo = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_conninfo, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_conninfo, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		OCI_RegisterUnsignedInt(idi->dbinfo.oci_statement_conninfo, ":id");
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_conninfo_update(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_conninfo_update() start\n");

	if (asprintf(&buf, "UPDATE %s SET disconnect_time=LOCALTIMESTAMP, "
	             "last_checkin_time=LOCALTIMESTAMP, "
	             "data_end_time=unixts2localts(:X1), "
	             "bytes_processed=:X2, "
	             "lines_processed=:X3, "
	             "entries_processed=:X4 WHERE id=:X5",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONNINFO]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_conninfo_update() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_conninfo_update = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_conninfo_update, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_conninfo_update, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_conninfo_update_checkin(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "UPDATE %s SET last_checkin_time=LOCALTIMESTAMP, "
	             "bytes_processed=:X1, "
	             "lines_processed=:X2, "
	             "entries_processed=:X3 WHERE id=:X4",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONNINFO]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_conninfo_update_checkin = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_conninfo_update_checkin, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_conninfo_update_checkin, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* OBJECTS INSERT                   */
/************************************/

int ido2db_oci_prepared_statement_objects_insert(ido2db_idi *idi) {

	char *buf = NULL;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_insert() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, objecttype_id, name1, name2) "
	             "VALUES (seq_objects.nextval, :X1, :X2, :X3, :X4) "
	             "RETURNING id INTO :id",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_insert() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {
		idi->dbinfo.oci_statement_objects_insert = OCI_StatementCreate(idi->dbinfo.oci_connection);
		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_objects_insert, 1);
		if (!OCI_Prepare(idi->dbinfo.oci_statement_objects_insert, MT(buf))) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_insert() parse failed\n");
			free(buf);
			return IDO_ERROR;
		}

		//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_insert() after rebind\n");
		OCI_RegisterUnsignedBigInt(idi->dbinfo.oci_statement_objects_insert, MT(":id"));
		//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_insert() after register\n");
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_insert() No Connection\n");
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_objects_insert() end\n");

	return IDO_OK;
}


/************************************/
/* OBJECTS SELECT                   */
/************************************/


int ido2db_oci_prepared_statement_objects_select_cached(ido2db_idi *idi) {


	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "SELECT id, objecttype_id, name1, name2 FROM %s WHERE instance_id=:X1",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_objects_select_cached = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_objects_select_cached, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_objects_select_cached, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_objects_select_name1_name2(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "SELECT id FROM %s "
	             "WHERE instance_id=:X1 "
	             "AND objecttype_id=:X2 "
	             "AND (((name1 is null) and (:X3 is null)) or (name1=:X3)) "
	             "AND (((name2 is null) and (:X4 is null)) or (name2=:X4))",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_objects_select_name1_name2 = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_objects_select_name1_name2, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_objects_select_name1_name2, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

/************************************/
/* OBJECTS UPDATE                   */
/************************************/


int ido2db_oci_prepared_statement_objects_update_inactive(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "UPDATE %s SET is_active=0 WHERE instance_id=:X2",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_objects_update_inactive = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_objects_update_inactive, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_objects_update_inactive, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_objects_update_active(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "UPDATE %s SET is_active=1 "
	             "WHERE instance_id=:X2 AND objecttype_id=:X3 AND id=:X4",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_objects_update_active = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_objects_update_active, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_objects_update_active, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_object_enable_disable(ido2db_idi *idi) {

        char *buf = NULL;

        //ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

        if (asprintf(&buf,
                     "UPDATE %s SET is_active=:X1"
                     "WHERE instance_id=:X2 "
                     "AND (((name1 is null) and (:X3 is null)) or (name1=:X3)) "
                     "AND (((name2 is null) and (:X4 is null)) or (name2=:X4))",
                     ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS]) == -1) {
                buf = NULL;
        }

        //ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

        if (idi->dbinfo.oci_connection) {

                idi->dbinfo.oci_statement_object_enable_disable = OCI_StatementCreate(idi->dbinfo.oci_connection);

                /* allow rebinding values */
                OCI_AllowRebinding(idi->dbinfo.oci_statement_object_enable_disable, 1);

                if (!OCI_Prepare(idi->dbinfo.oci_statement_object_enable_disable, MT(buf))) {
                        free(buf);
                        return IDO_ERROR;
                }
        } else {
                free(buf);
                return IDO_ERROR;
        }
        free(buf);

        //ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

        return IDO_OK;
}


/************************************/
/* LOGENTRIES                       */
/************************************/


int ido2db_oci_prepared_statement_logentries_insert(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_logentries_insert() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, logentry_time, "
	             "entry_time, entry_time_usec, logentry_type, "
	             "logentry_data,realtime_data, inferred_data_extracted) "
	             "VALUES (seq_logentries.nextval, :X1, unixts2localts(:X2), unixts2localts(:X3), "
	             ":X4, :X5, :X6, :X7, :X8)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_LOGENTRIES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_logentries_insert() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_logentries_insert = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_logentries_insert, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_logentries_insert, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_logentries_insert() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_logentries_select(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "SELECT id FROM %s "
	             "WHERE instance_id=:X1 "
	             "AND logentry_time=unixts2localts(:X2) "
	             "AND logentry_data=:X3",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_LOGENTRIES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_logentries_select = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_logentries_select, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_logentries_select, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}



/************************************/
/* PROCESSEVENTS                    */
/************************************/


int ido2db_oci_prepared_statement_process_events(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, event_type, event_time, "
	             "event_time_usec, process_id, program_name, program_version, program_date) "
	             "VALUES (seq_processevents.nextval, :X1, :X2, unixts2localts(:X3) , "
	             ":X4, :X5, :X6, :X7, :X8)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_PROCESSEVENTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_process_events = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_process_events, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_process_events, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* FLAPPINGHISTORY                  */
/************************************/


int ido2db_oci_prepared_statement_flappinghistory(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_id, event_time, event_time_usec, "
	             "event_type, reason_type, flapping_type, object_id, "
	             "percent_state_change, low_threshold, high_threshold, "
	             "comment_time, internal_comment_id) "
	             "VALUES (seq_flappinghistory.nextval, :X1, unixts2localts(:X2) , :X3, "
	             ":X4, :X5, :X6, :X7, :X8, :X9, :X10, "
	             "unixts2localts(:X11), :X12)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_FLAPPINGHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_flappinghistory = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_flappinghistory, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_flappinghistory, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

/************************************/
/* EXTERNALCOMMANDS                 */
/************************************/


int ido2db_oci_prepared_statement_external_commands(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, command_type, entry_time, "
	             "command_name, command_args) "
	             "VALUES (seq_externalcommands.nextval, :X1, :X2, unixts2localts(:X3), "
	             ":X4, :X5)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_EXTERNALCOMMANDS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_external_commands = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_external_commands, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_external_commands, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

/************************************/
/* ACKNOWLEDGMENTS                  */
/************************************/


int ido2db_oci_prepared_statement_acknowledgements(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, entry_time, entry_time_usec, "
	             "acknowledgement_type, object_id, state, author_name, comment_data, "
	             "is_sticky, persistent_comment, notify_contacts, end_time) "
	             "VALUES (seq_acknowledgements.nextval, :X1, unixts2localts(:X2), :X3, "
	             ":X4, :X5, :X6, :X7, :X8, :X9, :X10, :X11, unixts2localts(:X12))",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_ACKNOWLEDGEMENTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_acknowledgements = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_acknowledgements, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_acknowledgements, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

/************************************/
/* STATEHISTORY                     */
/************************************/


int ido2db_oci_prepared_statement_statehistory(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, state_time, state_time_usec, "
	             "object_id, state_change, state, state_type, "
	             "current_check_attempt, max_check_attempts, last_state, "
	             "last_hard_state, output, long_output) "
	             "VALUES (seq_statehistory.nextval, :X1, unixts2localts(:X2), :X3, "
	             ":X4, :X5, :X6, :X7, "
	             ":X8, :X9, :X10, :X11, :X12, :X13)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_STATEHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_statehistory = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_statehistory, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_statehistory, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}



/************************************/
/* TIMED EVENTS                     */
/************************************/

int ido2db_oci_prepared_statement_timedevents_queue(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() start\n");

	if (asprintf(&buf, "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND event_type=:X2 "
	             "AND scheduled_time=unixts2localts(:X5) "
	             "AND object_id=:X7) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET queued_time=unixts2localts(:X3), "
	             "queued_time_usec=:X4, "
	             "recurring_event=:X6 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, event_type, queued_time, queued_time_usec, "
	             "scheduled_time, recurring_event, object_id) "
	             "VALUES (seq_timedevents.nextval, :X1, :X2, unixts2localts(:X3), :X4, "
	             "unixts2localts(:X5), :X6, :X7)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_timedevents_queue = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_timedevents_queue, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_timedevents_queue, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_timedeventqueue(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedeventqueue() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND event_type=:X2 "
	             "AND scheduled_time=unixts2localts(:X5)"
	             "AND object_id=:X7) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET queued_time=unixts2localts(:X3), "
	             "queued_time_usec=:X4, "
	             "recurring_event=:X6 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, event_type, "
	             "queued_time, queued_time_usec, scheduled_time, "
	             "recurring_event, object_id) "
	             "VALUES (seq_timedeventqueue.nextval, :X1, :X2, "
	             "unixts2localts(:X3), :X4, unixts2localts(:X5), :X6, :X7)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTQUEUE]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedeventqueue() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_timedeventqueue = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_timedeventqueue, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_timedeventqueue, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedeventqueue() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_timedevents(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND event_type=:X2 "
	             "AND scheduled_time=unixts2localts(:X5) "
	             "AND object_id=:X7) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET event_time=unixts2localts(:X3), "
	             "event_time_usec=:X4, "
	             "recurring_event=:X6 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, event_type, "
	             "event_time, event_time_usec, scheduled_time, "
	             "recurring_event, object_id) "
	             "VALUES (seq_timedevents.nextval, :X1, :X2, "
	             "unixts2localts(:X3), :X4, unixts2localts(:X5), :X6, :X7)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_timedevents = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_timedevents, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_timedevents, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_timedevents_update(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "UPDATE %s SET deletion_time=unixts2localts(:X1), "
	             "deletion_time_usec=:X2 "
	             "WHERE instance_id=:X3 "
	             "AND event_type=:X4 "
	             "AND scheduled_time=unixts2localts(:X5) "
	             "AND recurring_event=:X6 "
	             "AND object_id=:X7",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_timedevents_update = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_timedevents_update, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_timedevents_update, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* HOSTSTATUS/CHECK                 */
/************************************/

int ido2db_oci_prepared_statement_hoststatus(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hoststatus() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL ON (host_object_id=:X2) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET instance_id=:X1, status_update_time=unixts2localts(:X3), "
	             "output=:X4, long_output=:X5u, perfdata=:X6u, "
	             "current_state=:X7, has_been_checked=:X8, should_be_scheduled=:X9, "
	             "current_check_attempt=:X10, max_check_attempts=:X11, "
	             "last_check=unixts2localts(:X12), next_check=unixts2localts(:X13), check_type=:X14, "
	             "last_state_change=unixts2localts(:X15), last_hard_state_change=unixts2localts(:X16), "
	             "last_hard_state=:X17, last_time_up=unixts2localts(:X18), "
	             "last_time_down=unixts2localts(:X19), last_time_unreachable=unixts2localts(:X20), "
	             "state_type=:X21, last_notification=unixts2localts(:X22), "
	             "next_notification=unixts2localts(:X23), no_more_notifications=:X24, "
	             "notifications_enabled=:X25, problem_has_been_acknowledged=:X26, "
	             "acknowledgement_type=:X27, current_notification_number=:X28, "
	             "passive_checks_enabled=:X29, active_checks_enabled=:X30, "
	             "event_handler_enabled=:X31, flap_detection_enabled=:X32, is_flapping=:X33, "
	             "percent_state_change=:X34, latency=:X35, execution_time=:X36, "
	             "scheduled_downtime_depth=:X37, failure_prediction_enabled=:X38, "
	             "process_performance_data=:X39, obsess_over_host=:X40, "
	             "modified_host_attributes=:X41, event_handler=:X42, "
	             "check_command=:X43, normal_check_interval=:X44, "
	             "retry_check_interval=:X45, check_timeperiod_object_id=:X46 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, host_object_id, status_update_time, output, long_output, perfdata, "
	             "current_state, has_been_checked, should_be_scheduled, current_check_attempt, "
	             "max_check_attempts, last_check, next_check, check_type, "
	             "last_state_change, last_hard_state_change, last_hard_state, "
	             "last_time_up, last_time_down, last_time_unreachable, "
	             "state_type, last_notification, next_notification, "
	             "no_more_notifications, notifications_enabled, problem_has_been_acknowledged, "
	             "acknowledgement_type, current_notification_number, passive_checks_enabled, "
	             "active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, "
	             "percent_state_change, latency, execution_time, scheduled_downtime_depth, "
	             "failure_prediction_enabled, process_performance_data, obsess_over_host, "
	             "modified_host_attributes, event_handler, check_command, normal_check_interval, "
	             "retry_check_interval, check_timeperiod_object_id) "
	             "VALUES (seq_hoststatus.nextval, :X1, :X2, unixts2localts(:X3), :X4, :X5i, :X6i, "
	             ":X7, :X8, :X9, :X10, "
	             ":X11, unixts2localts(:X12), unixts2localts(:X13), :X14, "
	             "unixts2localts(:X15), unixts2localts(:X16), :X17, "
	             "unixts2localts(:X18), unixts2localts(:X19), unixts2localts(:X20), "
	             ":X21, unixts2localts(:X22), unixts2localts(:X23), "
	             ":X24, :X25, :X26, "
	             ":X27, :X28, :X29, "
	             ":X30, :X31, :X32, :X33, "
	             ":X34, :X35, :X36, :X37, "
	             ":X38, :X39, :X40, "
	             ":X41, :X42, :X43, :X44, "
	             ":X45, :X46)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTSTATUS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hoststatus() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hoststatus = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_hoststatus, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hoststatus, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hoststatus() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_hostchecks(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostchecks() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, command_object_id, command_args, command_line, "
	             "instance_id, host_object_id, check_type, is_raw_check, "
	             "current_check_attempt, max_check_attempts, state, state_type, "
	             "start_time, start_time_usec, end_time, end_time_usec, timeout, "
	             "early_timeout, execution_time, latency, "
	             "return_code, output, long_output, perfdata) "
	             "VALUES (seq_hostchecks.nextval, :X1, :X2, :X3, "
	             ":X4, :X5, :X6, :X7, "
	             ":X8, :X9, :X10, :X11, "
	             "unixts2localts(:X12), :X13, unixts2localts(:X14), :X15, :X16, "
	             ":X17, :X18, :X19, "
	             ":X20, :X21, :X22, :X23)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCHECKS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostchecks() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostchecks = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostchecks, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostchecks, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_hostchecks() end\n");

	return IDO_OK;
}

/************************************/
/* SERVICESTATUS/CHECK              */
/************************************/

int ido2db_oci_prepared_statement_servicestatus(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicestatus() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL ON (service_object_id=:X2) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET instance_id=:X1, status_update_time=unixts2localts(:X3), "
	             "output=:X4, long_output=:X5u, perfdata=:X6u, current_state=:X7, "
	             "has_been_checked=:X8, should_be_scheduled=:X9, "
	             "current_check_attempt=:X10, max_check_attempts=:X11, "
	             "last_check=unixts2localts(:X12), next_check=unixts2localts(:X13), "
	             "check_type=:X14, last_state_change=unixts2localts(:X15), "
	             "last_hard_state_change=unixts2localts(:X16), last_hard_state=:X17, "
	             "last_time_ok=unixts2localts(:X18), last_time_warning=unixts2localts(:X19), "
	             "last_time_unknown=unixts2localts(:X20), last_time_critical=unixts2localts(:X21), "
	             "state_type=:X22, last_notification=unixts2localts(:X23), "
	             "next_notification=unixts2localts(:X24), no_more_notifications=:X25, "
	             "notifications_enabled=:X26, problem_has_been_acknowledged=:X27, "
	             "acknowledgement_type=:X28, current_notification_number=:X29, "
	             "passive_checks_enabled=:X30, active_checks_enabled=:X31, "
	             "event_handler_enabled=:X32, flap_detection_enabled=:X33, "
	             "is_flapping=:X34, percent_state_change=:X35, latency=:X36, "
	             "execution_time=:X37, scheduled_downtime_depth=:X38, "
	             "failure_prediction_enabled=:X39, process_performance_data=:X40, "
	             "obsess_over_service=:X41, modified_service_attributes=:X42, "
	             "event_handler=:X43, check_command=:X44, normal_check_interval=:X45, "
	             "retry_check_interval=:X46, check_timeperiod_object_id=:X47 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT "
	             "(id, instance_id, service_object_id, status_update_time, "
	             "output, long_output, perfdata, current_state, "
	             "has_been_checked, should_be_scheduled, current_check_attempt, "
	             "max_check_attempts, last_check, next_check, check_type, last_state_change, "
	             "last_hard_state_change, last_hard_state, last_time_ok, "
	             "last_time_warning, last_time_unknown, last_time_critical, "
	             "state_type, last_notification, next_notification, no_more_notifications, "
	             "notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, "
	             "current_notification_number, passive_checks_enabled, active_checks_enabled, "
	             "event_handler_enabled, flap_detection_enabled, is_flapping, "
	             "percent_state_change, latency, execution_time, scheduled_downtime_depth, "
	             "failure_prediction_enabled, process_performance_data, obsess_over_service, "
	             "modified_service_attributes, event_handler, check_command, "
	             "normal_check_interval, retry_check_interval, check_timeperiod_object_id) "
	             "VALUES "
	             "(seq_servicestatus.nextval, :X1, :X2, unixts2localts(:X3), "
	             ":X4, :X5i, :X6i, :X7, :X8, :X9, :X10, :X11, "
	             "unixts2localts(:X12), unixts2localts(:X13), :X14, unixts2localts(:X15), "
	             "unixts2localts(:X16), :X17, unixts2localts(:X18), "
	             "unixts2localts(:X19), unixts2localts(:X20), unixts2localts(:X21), "
	             ":X22, unixts2localts(:X23), unixts2localts(:X24), :X25, "
	             ":X26, :X27, :X28, :X29, :X30, :X31, :X32, :X33, "
	             ":X34, :X35, :X36, :X37, :X38, :X39, :X40, :X41, :X42, "
	             ":X43, :X44, :X45, :X46, :X47)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICESTATUS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicestatus() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_servicestatus = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_servicestatus, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_servicestatus, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicestatus() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_servicechecks(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicechecks() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, service_object_id, check_type, "
	             "current_check_attempt, max_check_attempts, state, "
	             "state_type, start_time, start_time_usec, end_time, "
	             "end_time_usec, timeout, early_timeout, "
	             "execution_time, latency, return_code, output, "
	             "command_object_id, command_args, command_line,"
	             "long_output, perfdata ) "
	             "VALUES (seq_servicechecks.nextval, :X1, :X2, :X3, "
	             ":X4, :X5, :X6, "
	             ":X7, unixts2localts(:X8), :X9, unixts2localts(:X10), "
	             ":X11, :X12, :X13, "
	             ":X14, :X15, :X16, :X17, "
	             ":X20, :X21, :X22,"
	             ":X18, :X19 )",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECHECKS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicechecks() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_servicechecks = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_servicechecks, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_servicechecks, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_servicechecks() end\n");

	return IDO_OK;
}

/************************************/
/* CONTACTNOTIFICATION              */
/************************************/

int ido2db_oci_prepared_statement_contact_notificationcommands(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contact_notificationcommands() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND contact_id=:X2 "
	             "AND notification_type=:X3 "
	             "AND command_object_id=:X4) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET command_args=:X5 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, contact_id, "
	             "notification_type, command_object_id, command_args) "
	             "VALUES (seq_contact_notifcommands.nextval, :X1, :X2, :X3, :X4, :X5)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contact_notificationcommands() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_contact_notificationcommands = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_contact_notificationcommands, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_contact_notificationcommands, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contact_notificationcommands() end\n");

	return IDO_OK;
}

/************************************/
/* PROGRAMSTATUS                    */
/************************************/

int ido2db_oci_prepared_statement_programstatus(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_programstatus() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET status_update_time=unixts2localts(:X2), "
	             "program_start_time=unixts2localts(:X3) , "
	             "is_currently_running=1, process_id=:X4, "
	             "daemon_mode=:X5, "
	             "last_command_check=unixts2localts(:X6), "
	             "last_log_rotation=unixts2localts(:X7), "
	             "notifications_enabled=:X8, active_service_checks_enabled=:X9, "
	             "passive_service_checks_enabled=:X10, active_host_checks_enabled=:X11, "
	             "passive_host_checks_enabled=:X12, event_handlers_enabled=:X13, "
	             "flap_detection_enabled=:X14, failure_prediction_enabled=:X15, "
	             "process_performance_data=:X16, obsess_over_hosts=:X17, "
	             "obsess_over_services=:X18, modified_host_attributes=:X19, "
	             "modified_service_attributes=:X20, global_host_event_handler=:X21, "
	             "global_service_event_handler=:X22, disable_notif_expire_time=unixts2localts(:X23) "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, status_update_time, "
	             "program_start_time, is_currently_running, "
	             "process_id, daemon_mode, last_command_check, "
	             "last_log_rotation, notifications_enabled, "
	             "active_service_checks_enabled, passive_service_checks_enabled, "
	             "active_host_checks_enabled, passive_host_checks_enabled, "
	             "event_handlers_enabled, flap_detection_enabled, "
	             "failure_prediction_enabled, process_performance_data, "
	             "obsess_over_hosts, obsess_over_services, "
	             "modified_host_attributes, modified_service_attributes, "
	             "global_host_event_handler, global_service_event_handler, "
		     "disable_notif_expire_time) "
	             "VALUES (seq_programstatus.nextval, :X1, unixts2localts(:X2) , "
	             "unixts2localts(:X3) , '1', :X4, :X5, "
	             "unixts2localts(:X6), unixts2localts(:X7) , :X8, :X9, :X10, :X11, "
	             ":X12, :X13, :X14, :X15, :X16, :X17, :X18, :X19, :X20, :X21, :X22, unixts2localts(:X23) )",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_programstatus() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_programstatus = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_programstatus, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_programstatus, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_programstatus() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_programstatus_update(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "UPDATE %s SET program_end_time=unixts2localts(:X1), "
	             "is_currently_running=:X2 "
	             "WHERE instance_id=:X3",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_programstatus_update = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_programstatus_update, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_programstatus_update, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* SYSTEMCOMMANDS                   */
/************************************/


int ido2db_oci_prepared_statement_systemcommanddata(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_systemcommanddata() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND start_time=unixts2localts(:X2) "
	             "AND start_time_usec=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET end_time=unixts2localts(:X4), end_time_usec=:X5, "
	             "command_line=:X6, timeout=:X7, early_timeout=:X8, "
	             "execution_time=:X9, return_code=:X10, output=:X11, long_output=:X12u "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, start_time, start_time_usec, "
	             "end_time, end_time_usec, command_line, timeout, early_timeout, "
	             "execution_time, return_code, output, long_output) "
	             "VALUES (seq_systemcommands.nextval, :X1, unixts2localts(:X2), :X3, "
	             "unixts2localts(:X4), :X5, :X6, :X7, :X8, "
	             ":X9, :X10, :X11, :X12i)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SYSTEMCOMMANDS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_systemcommanddata() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_systemcommanddata = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_systemcommanddata, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_systemcommanddata, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_systemcommanddata() end\n");

	return IDO_OK;
}

/************************************/
/* EVENTHANLDERS                    */
/************************************/


int ido2db_oci_prepared_statement_eventhandlerdata(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_eventhandlerdata() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND start_time=unixts2localts(:X6)"
	             "AND start_time_usec=:X7) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET eventhandler_type=:X2, object_id=:X3, "
	             "state=:X4, state_type=:X5, end_time=unixts2localts(:X8), "
	             "end_time_usec=:X9, command_object_id=:X10, "
	             "command_args=:X11, command_line=:X12, "
	             "timeout=:X13, early_timeout=:X14, execution_time=:X15, "
	             "return_code=:X16, output=:X17, long_output=:X18u "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, eventhandler_type, object_id, "
	             "state, state_type, start_time, start_time_usec, "
	             "end_time, end_time_usec, command_object_id, "
	             "command_args, command_line, "
	             "timeout, early_timeout, execution_time, "
	             "return_code, output, long_output) "
	             "VALUES (seq_eventhandlers.nextval, :X1, :X2, "
	             ":X3, :X4, :X5, unixts2localts(:X6), :X7, "
	             "unixts2localts(:X8), :X9, :X10, :X11, :X12, "
	             ":X13, :X14, :X15, :X16, :X17, :X18i)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_EVENTHANDLERS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_eventhandlerdata() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_eventhandlerdata = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_eventhandlerdata, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_eventhandlerdata, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_eventhandlerdata() end\n");

	return IDO_OK;
}



/************************************/
/* NOTIFICATIONS                    */
/************************************/


int ido2db_oci_prepared_statement_notificationdata(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_notificationdata() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND start_time=unixts2localts(:X4) "
	             "AND start_time_usec=:X5 "
	             "AND object_id=:X8) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET notification_type=:X2, notification_reason=:X3, "
	             "end_time=unixts2localts(:X6), end_time_usec=:X7, state=:X9, "
	             "output=:X10, long_output=:X11u, escalated=:X12, contacts_notified=:X13 "
	             "WHEN NOT MATCHED "
	             "THEN INSERT (id, instance_id, notification_type, notification_reason, "
	             "start_time, start_time_usec, end_time, end_time_usec, object_id, state, "
	             "output, long_output, escalated, contacts_notified) "
	             "VALUES (seq_notifications.nextval, :X1, :X2, :X3, "
	             "unixts2localts(:X4), :X5, unixts2localts(:X6), :X7, :X8, :X9, "
	             ":X10, :X11i, :X12, :X13)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_notificationdata() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_notificationdata = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_notificationdata, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_notificationdata, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_notificationdata() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_contactnotificationdata(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactnotificationdata() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND contact_object_id=:X7 "
	             "AND start_time=unixts2localts(:X3) "
	             "AND start_time_usec=:X4) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET notification_id=:X2, "
	             "end_time= unixts2localts(:X5), "
	             "end_time_usec=:X6 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, notification_id, "
	             "start_time, start_time_usec, "
	             "end_time, end_time_usec, contact_object_id) "
	             "VALUES (seq_contactnotifications.nextval, :X1, :X2, "
	             "unixts2localts(:X3), :X4, unixts2localts(:X5), :X6, :X7)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactnotificationdata() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_contactnotificationdata = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_contactnotificationdata, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_contactnotificationdata, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactnotificationdata() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_contactnotificationmethoddata(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactnotificationmethoddata() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND contactnotification_id=:X2 "
	             "AND start_time=unixts2localts(:X3)"
	             "AND start_time_usec=:X4) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET end_time=unixts2localts(:X5), "
	             "end_time_usec=:X6, command_object_id=:X7, command_args=:X8 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, contactnotification_id, "
	             "start_time, start_time_usec, "
	             "end_time, end_time_usec, command_object_id, command_args) "
	             "VALUES (seq_contactnotifmethods.nextval, :X1, :X2, "
	             "unixts2localts(:X3), :X4, "
	             "unixts2localts(:X5), :X6, :X7, :X8)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactnotificationmethoddata() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_contactnotificationmethoddata = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_contactnotificationmethoddata, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_contactnotificationmethoddata, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactnotificationmethoddata() end\n");

	return IDO_OK;
}




/************************************/
/* COMMENTS                         */
/************************************/


int ido2db_oci_prepared_statement_commentdata(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commentdata() start\n");

	if (asprintf(&buf, "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X3 "
	             "AND comment_time=unixts2localts(:X7) "
	             "AND internal_comment_id=:X8) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET comment_type=:X4, entry_type=:X5, "
	             "object_id=:X6, author_name=:X9, comment_data=:X10, "
	             "is_persistent=:X11, comment_source=:X12, "
	             "expires=:X13, expiration_time=unixts2localts(:X14) "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, entry_time, entry_time_usec, instance_id, "
	             "comment_type, entry_type, object_id, "
	             "comment_time, internal_comment_id, author_name, "
	             "comment_data, is_persistent, comment_source, "
	             "expires, expiration_time) "
	             "VALUES (seq_comments.nextval, unixts2localts(:X1), :X2, :X3, "
	             ":X4, :X5, :X6, "
	             "unixts2localts(:X7), :X8, :X9, "
	             ":X10, :X11, :X12, "
	             ":X13, unixts2localts(:X14))",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commentdata() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_commentdata = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_commentdata, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_commentdata, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commentdata() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_commentdata_history(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commentdata_history() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X3 "
	             "AND comment_time=unixts2localts(:X7) "
	             "AND internal_comment_id=:X8) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET comment_type=:X4, entry_type=:X5, object_id=:X6, "
	             "author_name=:X9, comment_data=:X10, is_persistent=:X11, "
	             "comment_source=:X12, expires=:X13, "
	             "expiration_time=unixts2localts(:X14) "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, entry_time, entry_time_usec, "
	             "instance_id, comment_type, entry_type, object_id, "
	             "comment_time, internal_comment_id, author_name, "
	             "comment_data, is_persistent, comment_source, "
	             "expires, expiration_time) "
	             "VALUES (seq_commenthistory.nextval, unixts2localts(:X1), :X2, "
	             ":X3, :X4, :X5, :X6, "
	             "unixts2localts(:X7), :X8, :X9, "
	             ":X10, :X11, :X12, "
	             ":X13, unixts2localts(:X14))",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commentdata_history() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_commentdata_history = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_commentdata_history, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_commentdata_history, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commentdata_history() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_comment_history_update(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "UPDATE %s SET deletion_time=unixts2localts(:X1), "
	             "deletion_time_usec=:X2 "
	             "WHERE instance_id=:X3 "
	             "AND comment_time=unixts2localts(:X4) "
	             "AND internal_comment_id=:X5",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_comment_history_update = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_comment_history_update, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_comment_history_update, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

/************************************/
/* DOWNTIME                         */
/************************************/


int ido2db_oci_prepared_statement_downtimedata_scheduled_downtime(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_downtimedata_scheduled_downtime() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND object_id=:X3 "
	             "AND entry_time=unixts2localts(:X4) "
	             "AND internal_downtime_id=:X7) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET downtime_type=:X2, author_name=:X5, "
	             "comment_data=:X6, triggered_by_id=:X8, "
	             "is_fixed=:X9, duration=:X10, "
	             "scheduled_start_time=unixts2localts(:X11) , "
	             "scheduled_end_time=unixts2localts(:X12), "
		     "is_in_effect=:X13, trigger_time=unixts2localts(:X14) "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, downtime_type, object_id, "
	             "entry_time, author_name, comment_data, "
	             "internal_downtime_id, triggered_by_id, "
	             "is_fixed, duration, scheduled_start_time, scheduled_end_time, "
		     "is_in_effect, trigger_time) "
	             "VALUES (seq_scheduleddowntime.nextval, :X1, :X2, :X3, "
	             "unixts2localts(:X4), :X5, :X6, "
	             ":X7, :X8, :X9, :X10, unixts2localts(:X11),unixts2localts(:X12), "
		     ":X13, unixts2localts(:X14))",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_scheduled_downtime() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_downtimedata_scheduled_downtime = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_downtimedata_scheduled_downtime() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_downtimedata_downtime_history(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_downtimedata_downtime_history() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND object_id=:X3 "
	             "AND entry_time=unixts2localts(:X4) "
	             "AND internal_downtime_id=:X7) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET downtime_type=:X2, author_name=:X5, "
	             "comment_data=:X6, triggered_by_id=:X8, is_fixed=:X9, "
	             "duration=:X10, scheduled_start_time=unixts2localts(:X11), "
	             "scheduled_end_time=unixts2localts(:X12), "
		     "is_in_effect=:X13, trigger_time=unixts2localts(:X14) "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, downtime_type, object_id, "
	             "entry_time, author_name, comment_data, internal_downtime_id, "
	             "triggered_by_id, is_fixed, duration, "
	             "scheduled_start_time, scheduled_end_time, "
		     "is_in_effect, trigger_time)"
	             "VALUES (seq_downtimehistory.nextval, :X1, :X2, :X3, "
	             "unixts2localts(:X4), :X5, :X6, :X7, :X8, :X9, :X10, "
	             "unixts2localts(:X11), unixts2localts(:X12), "
		     ":X13, unixts2localts(:X14))",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_downtime_history() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_downtimedata_downtime_history = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_downtimedata_downtime_history, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_downtimedata_downtime_history() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_downtimehistory_update_start(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "UPDATE %s SET actual_start_time=unixts2localts(:X1) , "
	             "actual_start_time_usec=:X2, was_started=:X3, "
		     "is_in_effect=:X10, trigger_time=unixts2localts(:X11) "
	             "WHERE instance_id=:X4 "
	             "AND downtime_type=:X5 "
	             "AND object_id=:X6 "
	             "AND entry_time=unixts2localts(:X7) "
	             "AND scheduled_start_time=unixts2localts(:X8) "
	             "AND scheduled_end_time=unixts2localts(:X9) "
	             "AND was_started=0",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_downtimehistory_update_start = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_downtimehistory_update_start, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_scheduleddowntime_update_start(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "UPDATE %s SET actual_start_time=unixts2localts(:X1), "
	             "actual_start_time_usec=:X2, was_started=:X3, "
		     "is_in_effect=:X10, trigger_time=unixts2localts(:X11) "
	             "WHERE instance_id=:X4 "
	             "AND downtime_type=:X5 "
	             "AND object_id=:X6 "
	             "AND entry_time=unixts2localts(:X7) "
	             "AND scheduled_start_time=unixts2localts(:X8) "
	             "AND scheduled_end_time=unixts2localts(:X9) "
	             "AND was_started=0",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_scheduleddowntime_update_start = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_scheduleddowntime_update_start, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_downtimehistory_update_stop(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "UPDATE %s SET "
	             "actual_end_time=unixts2localts(:X1) , "
	             "actual_end_time_usec=:X2, was_cancelled=:X3, "
		     "is_in_effect=:X10, trigger_time=unixts2localts(:X11) "
	             "WHERE instance_id=:X4 "
	             "AND downtime_type=:X5 "
	             "AND object_id=:X6 "
	             "AND entry_time= unixts2localts(:X7) "
	             "AND scheduled_start_time=unixts2localts(:X8) "
	             "AND scheduled_end_time=unixts2localts(:X9) ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_downtimehistory_update_stop = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_downtimehistory_update_stop, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

/************************************/
/* CONTACTSTATUS                    */
/************************************/


int ido2db_oci_prepared_statement_contactstatusdata(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (contact_object_id=:X2) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET instance_id=:X1, status_update_time=unixts2localts(:X3), "
	             "host_notifications_enabled=:X4, service_notifications_enabled=:X5, "
	             "last_host_notification=unixts2localts(:X6), "
	             "last_service_notification=unixts2localts(:X7), modified_attributes=:X8, "
	             "modified_host_attributes=:X9, modified_service_attributes=:X10 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, contact_object_id, status_update_time, "
	             "host_notifications_enabled, service_notifications_enabled, "
	             "last_host_notification, last_service_notification, "
	             "modified_attributes, modified_host_attributes, "
	             "modified_service_attributes) "
	             "VALUES (seq_contactstatus.nextval, :X1, :X2, unixts2localts(:X3), "
	             ":X4, :X5,unixts2localts(:X6), unixts2localts(:X7) , "
	             ":X8, :X9, :X10)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTSTATUS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_contactstatusdata() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_contactstatusdata = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_contactstatusdata, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_contactstatusdata, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}



/************************************/
/* VARIABLES                        */
/************************************/


int ido2db_oci_prepared_statement_configfilevariables(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET configfile_type=:X2, configfile_path=:X3 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, configfile_type, configfile_path) "
	             "VALUES (seq_configfiles.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_configfilevariables = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_configfilevariables, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_configfilevariables, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_configfilevariables_insert(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_configfilevariables_insert() start\n");
	/*
	       if(asprintf(&buf,
	       		"MERGE INTO %s USING DUAL "
	       		"ON (instance_id=:X1 AND configfile_id=:X2 AND varname=:X3) "
		       "WHEN MATCHED THEN "
			       "UPDATE SET varvalue=:X4 "
		       "WHEN NOT MATCHED THEN "
			       "INSERT (id, instance_id, configfile_id, varname, varvalue) "
	       			"VALUES (seq_configfilevariables.nextval, :X1, :X2, :X3, :X4)",
	               ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILEVARIABLES]) == -1) {
	                       buf = NULL;
	       }
	*/
	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, configfile_id, varname, varvalue) "
	             "VALUES (seq_configfilevariables.nextval, :X1, :X2, :X3, :X4)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILEVARIABLES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_configvariables_insert() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_configfilevariables_insert = OCI_StatementCreate(idi->dbinfo.oci_connection);
		OCI_AllowRebinding(idi->dbinfo.oci_statement_configfilevariables_insert, 1);
		if (!OCI_Prepare(idi->dbinfo.oci_statement_configfilevariables_insert, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
		 * array count cannot exceed this value after initial set here, but be lower.
		 * See ocilib doc module "binding variables and arrays"
		 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_configfilevariables_insert, OCI_BINDARRAY_MAX_SIZE);
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_configfilevariables_insert() end\n");

	return IDO_OK;
}



int ido2db_oci_prepared_statement_runtimevariables(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_runtimevariables() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, varname, varvalue) "
	             "VALUES (seq_runtimevariables.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_RUNTIMEVARIABLES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_runtimevariables() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_runtimevariables = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_runtimevariables, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_runtimevariables, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
		 * array count cannot exceed this value after initial set here, but be lower.
		 * See ocilib doc module "binding variables and arrays"
		 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_runtimevariables, OCI_BINDARRAY_MAX_SIZE);
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_runtimevaribles() end\n");

	return IDO_OK;
}


/************************************/
/* HOSTDEFINITION                   */
/************************************/


int ido2db_oci_prepared_statement_hostdefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 AND config_type=:X2 AND host_object_id=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET alias=:X4, display_name=:X5, address=:X6, "
	             "check_command_object_id=:X7, check_command_args=:X8, "
	             "eventhandler_command_object_id=:X9, eventhandler_command_args=:X10, "
	             "check_timeperiod_object_id=:X11, notif_timeperiod_object_id=:X12, "
	             "failure_prediction_options=:X13, check_interval=:X14, "
	             "retry_interval=:X15, max_check_attempts=:X16, "
	             "first_notification_delay=:X17, notification_interval=:X18, "
	             "notify_on_down=:X19, notify_on_unreachable=:X20, "
	             "notify_on_recovery=:X21, notify_on_flapping=:X22, "
	             "notify_on_downtime=:X23, stalk_on_up=:X24, stalk_on_down=:X25, "
	             "stalk_on_unreachable=:X26, flap_detection_enabled=:X27, "
	             "flap_detection_on_up=:X28, flap_detection_on_down=:X29, "
	             "flap_detection_on_unreachable=:X30, low_flap_threshold=:X31, "
	             "high_flap_threshold=:X32, process_performance_data=:X33, "
	             "freshness_checks_enabled=:X34, freshness_threshold=:X35, "
	             "passive_checks_enabled=:X36, event_handler_enabled=:X37, "
	             "active_checks_enabled=:X38, retain_status_information=:X39, "
	             "retain_nonstatus_information=:X40, notifications_enabled=:X41, "
	             "obsess_over_host=:X42, failure_prediction_enabled=:X43, "
	             "notes=:X44, notes_url=:X45, action_url=:X46, icon_image=:X47, "
	             "icon_image_alt=:X48, vrml_image=:X49, statusmap_image=:X50, "
	             "have_2d_coords=:X51, x_2d=:X52, y_2d=:X53, have_3d_coords=:X54, "
	             "x_3d=:X55, y_3d=:X56, z_3d=:X57, address6=:X58 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, host_object_id, alias, "
	             "display_name, address, check_command_object_id, check_command_args, "
	             "eventhandler_command_object_id, eventhandler_command_args, "
	             "check_timeperiod_object_id, notif_timeperiod_object_id, "
	             "failure_prediction_options, check_interval, "
	             "retry_interval, max_check_attempts, first_notification_delay, "
	             "notification_interval, notify_on_down, notify_on_unreachable, "
	             "notify_on_recovery, notify_on_flapping, notify_on_downtime, "
	             "stalk_on_up, stalk_on_down, stalk_on_unreachable, "
	             "flap_detection_enabled, flap_detection_on_up, "
	             "flap_detection_on_down, flap_detection_on_unreachable, "
	             "low_flap_threshold, high_flap_threshold, process_performance_data, "
	             "freshness_checks_enabled, freshness_threshold, passive_checks_enabled, "
	             "event_handler_enabled, active_checks_enabled, retain_status_information, "
	             "retain_nonstatus_information, notifications_enabled, obsess_over_host, "
	             "failure_prediction_enabled, notes, notes_url, action_url, "
	             "icon_image, icon_image_alt, vrml_image, statusmap_image, "
	             "have_2d_coords, x_2d, y_2d, have_3d_coords, x_3d, y_3d, z_3d, address6) "
	             "VALUES (seq_hosts.nextval, :X1, :X2, :X3, :X4, :X5, :X6, :X7, :X8, :X9, :X10, "
	             ":X11, :X12, :X13, :X14, :X15, :X16, :X17, :X18, :X19, :X20, :X21, "
	             ":X22, :X23, :X24, :X25, :X26, :X27, :X28, :X29, :X30, :X31, :X32, "
	             ":X33, :X34, :X35, :X36, :X37, :X38, :X39, :X40, :X41, :X42, :X43, "
	             ":X44, :X45, :X46, :X47, :X48, :X49, :X50, :X51, :X52, :X53, :X54, "
	             ":X55, :X56, :X57, :X58)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostdefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostdefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostdefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_hostdefinition_parenthosts(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "INSERT INTO %s (id, instance_id, host_id, parent_host_object_id) "
	             "VALUES (seq_host_parenthosts.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTPARENTHOSTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostdefinition_parenthosts = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostdefinition_parenthosts, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostdefinition_parenthosts, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
				 * array count cannot exceed this value after initial set here, but be lower.
				 * See ocilib doc module "binding variables and arrays"
				 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostdefinition_parenthosts, OCI_BINDARRAY_MAX_SIZE);
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_hostdefinition_contactgroups(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_id, host_id, contactgroup_object_id) "
	             "VALUES (seq_host_contactgroups.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTGROUPS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostdefinition_contactgroups = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostdefinition_contactgroups, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostdefinition_contactgroups, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
				 * array count cannot exceed this value after initial set here, but be lower.
				 * See ocilib doc module "binding variables and arrays"
				 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostdefinition_contactgroups, OCI_BINDARRAY_MAX_SIZE);

	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_hostdefinition_contacts(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_id, host_id, contact_object_id) "
	             "VALUES (seq_host_contacts.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostdefinition_contacts = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostdefinition_contacts, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostdefinition_contacts, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
				 * array count cannot exceed this value after initial set here, but be lower.
				 * See ocilib doc module "binding variables and arrays"
				 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostdefinition_contacts, OCI_BINDARRAY_MAX_SIZE);

	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* HOSTGROUPDEFINITION              */
/************************************/


int ido2db_oci_prepared_statement_hostgroupdefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:x1 AND hostgroup_object_id=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET config_type=:X2, alias=:X4 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, hostgroup_object_id, alias) "
	             "VALUES (seq_hostgroups.nextval, :X1, :X2, :X3, :X4)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostgroupdefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostgroupdefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostgroupdefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_hostgroupdefinition_hostgroupmembers(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_id, hostgroup_id, host_object_id) "
	             "VALUES (seq_hostgroup_members.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPMEMBERS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
		 		 * array count cannot exceed this value after initial set here, but be lower.
		 		 * See ocilib doc module "binding variables and arrays"
		 		 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, OCI_BINDARRAY_MAX_SIZE);

	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* SERVICEDEFINITION                */
/************************************/


int ido2db_oci_prepared_statement_servicedefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND config_type=:X2 "
	             "AND service_object_id=:X4) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET host_object_id=:X3, "
	             "display_name=:X5, check_command_object_id=:X6, "
	             "check_command_args=:X7, eventhandler_command_object_id=:X8, "
	             "eventhandler_command_args=:X9, check_timeperiod_object_id=:X10, "
	             "notif_timeperiod_object_id=:X11, failure_prediction_options=:X12, "
	             "check_interval=:X13, retry_interval=:X14, "
	             "max_check_attempts=:X15, first_notification_delay=:X16, "
	             "notification_interval=:X17, notify_on_warning=:X18, "
	             "notify_on_unknown=:X19, notify_on_critical=:X20, "
	             "notify_on_recovery=:X21, notify_on_flapping=:X22, "
	             "notify_on_downtime=:X23, stalk_on_ok=:X24, "
	             "stalk_on_warning=:X25, stalk_on_unknown=:X26, "
	             "stalk_on_critical=:X27, is_volatile=:X28, "
	             "flap_detection_enabled=:X29, flap_detection_on_ok=:X30, "
	             "flap_detection_on_warning=:X31, flap_detection_on_unknown=:X32, "
	             "flap_detection_on_critical=:X33, "
	             "low_flap_threshold=:X34, high_flap_threshold=:X35, "
	             "process_performance_data=:X36, freshness_checks_enabled=:X37, "
	             "freshness_threshold=:X38, passive_checks_enabled=:X39, "
	             "event_handler_enabled=:X40, active_checks_enabled=:X41, "
	             "retain_status_information=:X42, retain_nonstatus_information=:X43, "
	             "notifications_enabled=:X44, obsess_over_service=:X45, "
	             "failure_prediction_enabled=:X46, notes=:X47, "
	             "notes_url=:X48, action_url=:X49, icon_image=:X50, icon_image_alt=:X51 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, host_object_id, service_object_id, "
	             "display_name, check_command_object_id, check_command_args, "
	             "eventhandler_command_object_id, eventhandler_command_args, "
	             "check_timeperiod_object_id, notif_timeperiod_object_id, "
	             "failure_prediction_options, check_interval, retry_interval, "
	             "max_check_attempts, first_notification_delay, "
	             "notification_interval, notify_on_warning, notify_on_unknown, "
	             "notify_on_critical, notify_on_recovery, notify_on_flapping, "
	             "notify_on_downtime, stalk_on_ok, stalk_on_warning, stalk_on_unknown, "
	             "stalk_on_critical, is_volatile, flap_detection_enabled, "
	             "flap_detection_on_ok, flap_detection_on_warning, "
	             "flap_detection_on_unknown, flap_detection_on_critical, "
	             "low_flap_threshold, high_flap_threshold, process_performance_data, "
	             "freshness_checks_enabled, freshness_threshold, passive_checks_enabled, "
	             "event_handler_enabled, active_checks_enabled, retain_status_information, "
	             "retain_nonstatus_information, notifications_enabled, obsess_over_service, "
	             "failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt) "
	             "VALUES (seq_services.nextval, :X1, :X2, :X3, :X4, :X5, :X6, :X7, :X8, :X9, :X10, :X11, "
	             ":X12, :X13, :X14, :X15, :X16, :X17, :X18, :X19, :X20, :X21, :X22, :X23, "
	             ":X24, :X25, :X26, :X27, :X28, :X29, :X30, :X31, :X32, :X33, :X34, :X35, "
	             ":X36, :X37, :X38, :X39, :X40, :X41, :X42, :X43, :X44, :X45, :X46, :X47, "
	             ":X48, :X49, :X50, :X51)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_servicedefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_servicedefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_servicedefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_servicedefinition_contactgroups(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_id, service_id, contactgroup_object_id) "
	             "VALUES (seq_service_contactgroups.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTGROUPS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_servicedefinition_contactgroups = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_servicedefinition_contactgroups, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_servicedefinition_contactgroups, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
		 		 * array count cannot exceed this value after initial set here, but be lower.
		 		 * See ocilib doc module "binding variables and arrays"
		 		 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_servicedefinition_contactgroups, OCI_BINDARRAY_MAX_SIZE);

	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_servicedefinition_contacts(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_id, service_id, contact_object_id) "
	             "VALUES (seq_service_contacts.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_servicedefinition_contacts = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_servicedefinition_contacts, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_servicedefinition_contacts, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
		 		 * array count cannot exceed this value after initial set here, but be lower.
		 		 * See ocilib doc module "binding variables and arrays"
		 		 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_servicedefinition_contacts, OCI_BINDARRAY_MAX_SIZE);

	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* SERVICEGROUPDEFINITION           */
/************************************/


int ido2db_oci_prepared_statement_servicegroupdefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND config_type=:X2 "
	             "AND servicegroup_object_id=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET alias=:X4 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, "
	             "servicegroup_object_id, alias) "
	             "VALUES (seq_servicegroups.nextval, :X1, :X2, :X3, :X4)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_servicegroupdefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_servicegroupdefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_servicegroupdefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_servicegroupdefinition_members(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_id, servicegroup_id, service_object_id) "
	             "VALUES (seq_servicegroup_members.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPMEMBERS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_servicegroupdefinition_members = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_servicegroupdefinition_members, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_servicegroupdefinition_members, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
				 * array count cannot exceed this value after initial set here, but be lower.
				 * See ocilib doc module "binding variables and arrays"
				 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_servicegroupdefinition_members, OCI_BINDARRAY_MAX_SIZE);

	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}



/************************************/
/* DEPENDENCYDEFINITION             */
/************************************/


int ido2db_oci_prepared_statement_hostdependencydefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND config_type=:X2 "
	             "AND host_object_id=:X3 "
	             "AND dependent_host_object_id=:X4 "
	             "AND dependency_type=:X5 "
	             "AND inherits_parent=:X6 "
	             "AND fail_on_up=:X8 "
	             "AND fail_on_down=:X9 "
	             "AND fail_on_unreachable=:X10) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET timeperiod_object_id=:X7 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, host_object_id, "
	             "dependent_host_object_id, dependency_type, "
	             "inherits_parent, timeperiod_object_id, fail_on_up, "
	             "fail_on_down, fail_on_unreachable) "
	             "VALUES (seq_hostdependencies.nextval, :X1, :X2, :X3, "
	             ":X4, :X5, :X6, :X7, :X8, :X9, :X10)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTDEPENDENCIES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostdependencydefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostdependencydefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_servicedependencydefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND config_type=:X2 "
	             "AND service_object_id=:X3 "
	             "AND dependent_service_object_id=:X4 "
	             "AND dependency_type=:X5 "
	             "AND inherits_parent=:X6 "
	             "AND fail_on_ok=:X8 "
	             "AND fail_on_warning=:X9 "
	             "AND fail_on_unknown=:X10 "
	             "AND fail_on_critical=:X11) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET timeperiod_object_id=:X7 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, service_object_id, "
	             "dependent_service_object_id, dependency_type, "
	             "inherits_parent, timeperiod_object_id, "
	             "fail_on_ok, fail_on_warning, fail_on_unknown, "
	             "fail_on_critical) "
	             "VALUES (seq_servicedependencies.nextval, :X1, :X2, :X3, "
	             ":X4, :X5, :X6, :X7, :X8, :X9, :X10, :X11)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEDEPENDENCIES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_servicedependencydefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_servicedependencydefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* HOSTESCALATIONDEFINITION         */
/************************************/


int ido2db_oci_prepared_statement_hostescalationdefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND config_type=:X2 "
	             "AND host_object_id=:X3 "
	             "AND timeperiod_object_id=:X4 "
	             "AND first_notification=:X5 "
	             "AND last_notification=:X6) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET notification_interval=:X7, "
	             "escalate_on_recovery=:X8, escalate_on_down=:X9, "
	             "escalate_on_unreachable=:X10 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id,instance_id, config_type, host_object_id, "
	             "timeperiod_object_id, first_notification, last_notification, "
	             "notification_interval, escalate_on_recovery, escalate_on_down, "
	             "escalate_on_unreachable) "
	             "VALUES (seq_hostescalations.nextval, :X1, :X2, :X3, "
	             ":X4, :X5, :X6, "
	             ":X7, :X8, :X9, "
	             ":X10)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostescalationdefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostescalationdefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_hostescalationdefinition_contactgroups(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (hostescalation_id=:X2 AND contactgroup_object_id=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET instance_id=:X1 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, hostescalation_id, contactgroup_object_id) "
	             "VALUES (seq_hostesc_contactgroups.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostescalationdefinition_contactgroups = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostescalationdefinition_contactgroups, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostescalationdefinition_contactgroups, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_hostescalationdefinition_contacts(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET hostescalation_id=:X2, contact_object_id=:X3 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, hostescalation_id, contact_object_id) "
	             "VALUES (seq_hostesc_contacts.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_hostescalationdefinition_contacts = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_hostescalationdefinition_contacts, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_hostescalationdefinition_contacts, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* SERVICEESCALATIONDEFINITION      */
/************************************/


int ido2db_oci_prepared_statement_serviceescalationdefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 "
	             "AND config_type=:X2 "
	             "AND service_object_id=:X3 "
	             "AND timeperiod_object_id=:X4 "
	             "AND first_notification=:X5 "
	             "AND last_notification=:X6) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET notification_interval=:X7, escalate_on_recovery=:X8, "
	             "escalate_on_warning=:X9, escalate_on_unknown=:X10, "
	             "escalate_on_critical=:X11 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, service_object_id, "
	             "timeperiod_object_id, first_notification, last_notification, "
	             "notification_interval, escalate_on_recovery, "
	             "escalate_on_warning, escalate_on_unknown, escalate_on_critical) "
	             "VALUES (seq_serviceescalations.nextval, :X1, :X2, :X3, "
	             ":X4, :X5, :X6, "
	             ":X7, :X8, "
	             ":X9, :X10, :X11)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_serviceescalationdefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_serviceescalationdefinition_contactgroups(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (serviceescalation_id=:X2 AND contactgroup_object_id=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET instance_id=:X1 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, serviceescalation_id, contactgroup_object_id) "
	             "VALUES (seq_serviceesccontactgroups.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_serviceescalationdefinition_contactgroups = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_serviceescalationdefinition_contactgroups, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_serviceescalationdefinition_contactgroups, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_serviceescalationdefinition_contacts(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET serviceescalation_id=:X2, contact_object_id=:X3 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, serviceescalation_id, contact_object_id) "
	             "VALUES (seq_serviceesc_contacts.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_serviceescalationdefinition_contacts = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_serviceescalationdefinition_contacts, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_serviceescalationdefinition_contacts, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* COMMANDDEFINITION                */
/************************************/


int ido2db_oci_prepared_statement_commanddefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commanddefinition_definition() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 AND object_id=:X2 AND config_type=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET command_line=:X4 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, object_id, config_type, command_line) "
	             "VALUES (seq_commands.nextval, :X1, :X2, :X3, :X4)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_COMMANDS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_commanddefinition_definition() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_commanddefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_commanddefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_commanddefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(nddefinition_definition) end\n");

	return IDO_OK;
}


/************************************/
/* TIMEPERIODEFINTION               */
/************************************/


int ido2db_oci_prepared_statement_timeperiodefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 AND config_type=:X2 AND timeperiod_object_id=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET alias=:X4 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, timeperiod_object_id, alias) "
	             "VALUES (seq_timeperiods.nextval, :X1, :X2, :X3, :X4)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_timeperiodefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);


		OCI_AllowRebinding(idi->dbinfo.oci_statement_timeperiodefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_timeperiodefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_timeperiodefinition_timeranges(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_id, timeperiod_id, day, start_sec, end_sec) "
	             "VALUES (seq_timep_timer.nextval, :X1, :X2, :X3, :X4, :X5)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODTIMERANGES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_timeperiodefinition_timeranges = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
				 * array count cannot exceed this value after initial set here, but be lower.
				 * See ocilib doc module "binding variables and arrays"
				 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, OCI_BINDARRAY_MAX_SIZE);
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* CONTACTDEFINTION                 */
/************************************/


int ido2db_oci_prepared_statement_contactdefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 AND config_type=:X2 AND contact_object_id=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET alias=:X4, email_address=:X5, pager_address=:X6, "
	             "host_timeperiod_object_id=:X7, service_timeperiod_object_id=:X8, "
	             "host_notifications_enabled=:X9, service_notifications_enabled=:X10, "
	             "can_submit_commands=:X11, notify_service_recovery=:X12, "
	             "notify_service_warning=:X13, notify_service_unknown=:X14, "
	             "notify_service_critical=:X15, notify_service_flapping=:X16, "
	             "notify_service_downtime=:X17, notify_host_recovery=:X18, "
	             "notify_host_down=:X19, notify_host_unreachable=:X20, "
	             "notify_host_flapping=:X21, notify_host_downtime=:X22 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, contact_object_id, "
	             "alias, email_address, pager_address, "
	             "host_timeperiod_object_id, service_timeperiod_object_id, "
	             "host_notifications_enabled, service_notifications_enabled, "
	             "can_submit_commands, notify_service_recovery, "
	             "notify_service_warning, notify_service_unknown, "
	             "notify_service_critical, notify_service_flapping, "
	             "notify_service_downtime, notify_host_recovery, "
	             "notify_host_down, notify_host_unreachable, "
	             "notify_host_flapping, notify_host_downtime) "
	             "VALUES (seq_contacts.nextval, :X1, :X2, :X3, "
	             ":X4, :X5, :X6, :X7, :X8, :X9, :X10, :X11, :X12, :X13, :X14,"
	             " :X15, :X16, :X17, :X18, :X19, :X20, :X21, :X22)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_contactdefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_contactdefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_contactdefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_contactdefinition_addresses(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "MERGE INTO %s USING DUAL "
	             "ON (contact_id=:X1 AND address_number=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET instance_id=:X2, address=:X4 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, contact_id, address_number, address) "
	             "VALUES (seq_contact_addresses.nextval, :X1, :X2, :X3, :X4)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTADDRESSES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_contactdefinition_addresses = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_contactdefinition_addresses, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_contactdefinition_addresses, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_contactdefinition_servicenotificationcommands(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 AND contact_id=:X2 "
	             "AND notification_type=:X3 AND command_object_id=:X4) "
	             "WHEN MATCHED THEN UPDATE SET command_args=:X5 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, contact_id, notification_type, "
	             "command_object_id, command_args) "
	             "VALUES (seq_contact_notifcommands.nextval, :X1, :X2, :X3, "
	             ":X4, :X5)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* CUSTOMVARIABLES                  */
/************************************/


int ido2db_oci_prepared_statement_save_custom_variables_customvariables(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (object_id=:X2 AND varname=:X5) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET instance_id=:X1, config_type=:X3, has_been_modified=:X4, "
	             "varvalue=:X6 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, object_id, config_type, has_been_modified, "
	             "varname, varvalue) "
	             "VALUES (seq_customvariables.nextval, :X1, :X2, :X3, :X4, "
	             ":X5, :X6)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_save_custom_variables_customvariables = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_save_custom_variables_customvariables, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_save_custom_variables_customvariables, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_save_custom_variables_customvariablestatus(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (object_id=:X2 AND varname=:X5) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET instance_id=:X1, "
	             "status_update_time= unixts2localts(:X3) , "
	             "has_been_modified=:X4, varvalue=:X6 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, object_id, status_update_time, "
	             "has_been_modified, varname, varvalue) "
	             "VALUES "
	             "(seq_customvariablestatus.nextval, :X1, :X2, unixts2localts(:X3), "
	             ":X4, :X5, :X6)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* CONTACTGROUPDEFINITION           */
/************************************/


int ido2db_oci_prepared_statement_contactgroupdefinition_definition(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "MERGE INTO %s USING DUAL "
	             "ON (instance_id=:X1 AND config_type=:X2 AND contactgroup_object_id=:X3) "
	             "WHEN MATCHED THEN "
	             "UPDATE SET alias=:X4 "
	             "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, config_type, contactgroup_object_id, alias) "
	             "VALUES "
	             "(seq_contactgroups.nextval, :X1, :X2, :X3, :X4)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_contactgroupdefinition_definition = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_contactgroupdefinition_definition, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_contactgroupdefinition_definition, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_contactgroupdefinition_contactgroupmembers(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "INSERT INTO %s (id, instance_id, contactgroup_id, contact_object_id) "
	             "VALUES "
	             "(seq_contactgroup_members.nextval, :X1, :X2, :X3)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPMEMBERS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers = OCI_StatementCreate(idi->dbinfo.oci_connection);

		OCI_AllowRebinding(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
		/* we will use bind arrays to improve performance
				 * array count cannot exceed this value after initial set here, but be lower.
				 * See ocilib doc module "binding variables and arrays"
				 */
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, OCI_BINDARRAY_MAX_SIZE);
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


/************************************/
/* DELETE                           */
/************************************/


int ido2db_oci_prepared_statement_timedeventqueue_delete(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, ""
	             "DELETE FROM %s "
	             "WHERE instance_id=:X1 "
	             "AND scheduled_time<= unixts2localts(:X2)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTQUEUE]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_timedeventqueue_delete = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_timedeventqueue_delete, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_timedeventqueue_delete, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_timedeventqueue_delete_more(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "DELETE FROM %s "
	             "WHERE instance_id=:X1 "
	             "AND event_type=:X2 "
	             "AND scheduled_time= unixts2localts(:X3) "
	             "AND recurring_event=:X4 "
	             "AND object_id=:X5",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTQUEUE]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_timedeventqueue_delete_more = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_timedeventqueue_delete_more, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_timedeventqueue_delete_more, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_comments_delete(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "DELETE FROM %s "
	             "WHERE instance_id=:X1 "
	             "AND comment_time=unixts2localts(:X2) "
	             "AND internal_comment_id=:X3",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_comments_delete = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_comments_delete, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_comments_delete, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_downtime_delete(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "DELETE FROM %s "
	             "WHERE instance_id=:X1 "
	             "AND downtime_type=:X2 "
	             "AND object_id=:X3 "
	             "AND entry_time= unixts2localts(:X4) "
	             "AND scheduled_start_time=unixts2localts(:X5) "
	             "AND scheduled_end_time=unixts2localts(:X6)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_downtime_delete = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_downtime_delete, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_downtime_delete, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}


int ido2db_oci_prepared_statement_instances_delete(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "BEGIN clean_table_by_instance(:X1, :X2); END;") == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_instances_delete = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_instances_delete, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_instances_delete, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_instances_delete_time(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf, "BEGIN clean_table_by_instance_time(:X1, :X2, :X3, :X4); END;") == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_instances_delete_time = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_instances_delete_time, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_instances_delete_time, MT(buf))) {
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_sla_history_merge(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
			"MERGE INTO %s USING DUAL "
	             "ON (id=:X1) "
	        "WHEN MATCHED THEN "
	             "UPDATE SET start_time=unixts2localts(:X3), "
	             "end_time=unixts2localts(:X4), "
	             "acknowledgement_time=unixts2localts(:X5), "
	             "state=:X7, state_type=:X8, scheduled_downtime=:X9 "
	        "WHEN NOT MATCHED THEN "
	             "INSERT (id, instance_id, start_time, end_time, "
		     	 "acknowledgement_time, object_id, state, "
		     	 "state_type, scheduled_downtime) "
	             "VALUES (seq_slahistory.nextval, :X2, "
		     	 "unixts2localts(:X3), unixts2localts(:X4), unixts2localts(:X5), "
	             ":X6, :X7, :X8, :X9)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SLAHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_history_merge) query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_sla_history_merge = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_sla_history_merge, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_sla_history_merge, MT(buf))) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_history_merge) ERROR\n");
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_history_merge) end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_sla_history_delete(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "DELETE FROM %s "
	             "WHERE id=:X1",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SLAHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_sla_history_delete = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_sla_history_delete, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_sla_history_delete, MT(buf))) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_history_delete) ERROR\n");
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_history_delete) end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_sla_services_select(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "SELECT service_object_id FROM %s "
	             "WHERE instance_id = :X1 AND host_object_id = :X2",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_sla_services_select = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_sla_services_select, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_sla_services_select, MT(buf))) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_services_select) ERROR\n");
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_services_select) end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_sla_history_select(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
	             "SELECT id AS slahistory_id,"
		     "localts2unixts(start_time) AS start_time, "
		     "localts2unixts(end_time) AS end_time, "
		     "localts2unixts(acknowledgement_time) AS acknowledgement_time,"
		     "state, state_type, scheduled_downtime "
		     "FROM %s "
		     "WHERE instance_id = :X1 AND object_id = :X2 AND "
		     "((start_time > unixts2localts(:X3) AND start_time < unixts2localts(:X4)) OR "
		     " (end_time > unixts2localts(:X3) AND end_time < unixts2localts(:X4)) OR "
		     " (start_time < unixts2localts(:X3) AND end_time > unixts2localts(:X4)) OR "
		     " (end_time = unixts2localts(0)))",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SLAHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_sla_history_select = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_sla_history_select, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_sla_history_select, MT(buf))) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_history_select) ERROR\n");
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_history_select) end\n");

	return IDO_OK;
}

int ido2db_oci_prepared_statement_sla_downtime_select(ido2db_idi *idi) {

	char *buf = NULL;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() start\n");

	if (asprintf(&buf,
		     "SELECT id AS downtimehistory_id,"
		     "localts2unixts(actual_start_time) AS actual_start_time, "
		     "localts2unixts(actual_end_time) AS actual_end_time,"
		     "localts2unixts(scheduled_start_time) AS scheduled_start_time, "
		     "localts2unixts(scheduled_end_time) AS scheduled_end_time, "
		     "is_fixed, duration "
		     "FROM %s "
		     "WHERE instance_id = :X1 AND object_id = :X2 AND "
		     "((actual_start_time > unixts2localts(:X3) AND actual_start_time < unixts2localts(:X4)) OR "
		     " (actual_end_time > unixts2localts(:X3) AND actual_end_time < unixts2localts(:X4)) OR "
		     " (actual_start_time < unixts2localts(:X3) AND actual_end_time > unixts2localts(:X4)) OR "
		     " (actual_end_time = unixts2localts(0)))",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]) == -1) {
		buf = NULL;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_() query: %s\n", buf);

	if (idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_sla_downtime_select = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_sla_downtime_select, 1);

		if (!OCI_Prepare(idi->dbinfo.oci_statement_sla_downtime_select, MT(buf))) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_downtime_select) ERROR\n");
			free(buf);
			return IDO_ERROR;
		}
	} else {
		free(buf);
		return IDO_ERROR;
	}
	free(buf);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_(sla_downtime_select) end\n");

	return IDO_OK;
}


/****************************************************************************/
/* MISC FUNCTIONS                                                           */
/****************************************************************************/

/************************************/
/* NULL BINDING                     */
/************************************/

int ido2db_oci_prepared_statement_bind_null_param(OCI_Statement *oci_statement_null, char *param_name) {

	char *oci_tmp;

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_bind_null_param() start\n");
	//syslog(LOG_USER | LOG_INFO, "bind null param %s\n", param_name);

	dummy = asprintf(&oci_tmp, "a"); /* just malloc sth that ocilib is happy */

	if (param_name == NULL)
		return IDO_ERROR;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_bind_null_param() param=%s\n", param_name);

	/* bind to dummy value */
	if (!OCI_BindString(oci_statement_null, MT(param_name), oci_tmp, 0)) {
		return IDO_ERROR;
	}

	/* free dummy */
	free(oci_tmp);

	/* get bind ptr, set to NULL */
	if (!OCI_BindSetNull(OCI_GetBind2(oci_statement_null, MT(param_name)))) {
		return IDO_ERROR;
	}

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_bind_null_param() end\n");

	return IDO_OK;
}
/**
 * set oracle session trace EVENT
 * @param OCI_CONNECTION
 * @param unsigned long oracle trace level for alter session set event
 * @returns boolean
 */
int ido2db_oci_set_trace_event(OCI_Connection *cn, unsigned int trace_level) {
	char * fname = "ido2db_oci_set_trace_event";
	OCI_Statement *st;
	int ret;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:set trace level event to %d\n", fname, trace_level);
	if (!(cn)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:No Connection\n", fname);
		return IDO_ERROR;
	}

	/* call stored procedure to handle trace events */
	st = OCI_StatementCreate(cn);
	if (!(OCI_Prepare(st, "begin "
	                  "set_trace_event(:level);"
	                  "end;"))) {
		OCI_StatementFree(st);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Prepare failed:\n", fname);
		return IDO_ERROR;
	}

	/* bind level parameter */
	if (!(OCI_BindUnsignedInt(st, MT(":level"), &trace_level))) {
		OCI_StatementFree(st);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Bind failed:\n", fname);
		return IDO_ERROR;
	}
	/* execute statement and log output */
	ret = ido2db_oci_execute_out(st, fname);
	if (ret != IDO_OK) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Event set failed\n", fname);
	}
	ido2db_oci_statement_free(st, fname);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s end\n", fname);
	return ret;
}
/**
 * executes a statement and handle DBMS_OUTPUT
 * @param OCI_Statement statement to execute
 * @return IDO_OK
 */
int ido2db_oci_execute_out(OCI_Statement *st, char * fname) {
	const dtext *p;
	int ret;
	char *binds = NULL;
	OCI_Connection *cn;

	/* check parameter */
	if (!st) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_execute_out: %s No valid statement handle supplied\n", fname);
		return IDO_ERROR;
	}
	if (!fname) {
		fname = strdup("ido2db_oci_execute_out");
	}
	/* get current connection */
	cn = OCI_StatementGetConnection(st);

	/* print binds in Level SQL */
	binds = malloc(OCI_VARCHAR_SIZE * 4);
	if (binds) {
		ido2db_oci_print_binds(st, sizeof(binds), (char **)binds);
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s Binds:%s\n", fname, binds);
		free(binds);
	}
	/**
	 * enable dbms_output, execute statement and retrieve dbms_output lines
	 */
	OCI_ServerEnableOutput(cn , OCI_OUTPUT_BUFFER_SIZE, 1, 2000);
	ret = OCI_Execute(st);
	while ((p = OCI_ServerGetOutput(cn))) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s DBMSOUT:%s\n", fname, p);
	}
	/* return execute status */
	if (!ret) {
		return IDO_ERROR;
	}
	return IDO_OK;
}
/**
 * set oracle session application info details and Time Zone
 * @param OCI_Connection
 * @param action string (agent_name)
 * @return IDO_OK
 */
int ido2db_oci_set_session_info(OCI_Connection *cn, char * action) {
	/* set oracle session application info module*/
	char * fname = "ido2db_oci_set_session_info";
	char * module = "IDO2DB";
	char * app_info;
	OCI_Statement *st;
	int ret;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Set Session Info to %s\n", fname, action);
	if (!(cn)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:No Connection\n", fname);
		return IDO_ERROR;
	}

	st = OCI_StatementCreate(cn);
	if (!(OCI_Prepare(st,
	                  "begin "
	                  "dbms_application_info.set_module(:module,:action);"
	                  "end;"))) {
		OCI_StatementFree(st);
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s:Prepare failed:\n", fname);
		return IDO_ERROR;
	}
	/* bind module parameter */
	if (!(OCI_BindString(st, MT(":module"), module, 0))) {
		OCI_StatementFree(st);
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s:Bind module failed:\n", fname);
		return IDO_ERROR;
	}
	app_info = action ? strdup(action) : "";
	/* bind action parameter */
	if (!(OCI_BindString(st, MT(":action"), app_info, 0))) {
		OCI_StatementFree(st);
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s:Bind action failed:\n", fname);
		return IDO_ERROR;
	}
	/* execute statement */
	ret = ido2db_oci_execute_out(st, fname);
	if (ret != IDO_OK) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s: Session Info Module execute failed\n", fname);
	}
	ido2db_oci_statement_free(st,fname);

	/* set session time zone to UTC */
	st=OCI_StatementCreate(cn);
	if (OCI_ExecuteStmt(st,"alter session set time_zone='UTC'")) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s: TimeZone Set to UTC OK\n",fname);
	}else{
		 ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s Error: TimeZone Set to UTC failed\n",fname);
	}
	ido2db_oci_statement_free(st,fname);

	if (app_info) free(app_info);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s end\n", fname);
	return ret;
}
/**
 * print bind names and values
 * @param OCI_Connection
 * @param buffer size
 * @param buffer pointer
 */
void ido2db_oci_print_binds(OCI_Statement *st, int bsize, char ** outp) {
	OCI_Bind *bn;
	char *text = NULL;
	char *val = NULL;
	char *fmt = NULL;
	const mtext * name;
	unsigned int type = 0;
	unsigned int subtype = 0;
	void * data;
	OCI_Date *dt;
	OCI_Timestamp *ts;
	OCI_Interval *inv;
	OCI_File *file;
	unsigned int size = 0;
	unsigned int count = 0;
	unsigned int i;

	if (st == NULL) return;
	text = malloc(OCI_VARCHAR_SIZE + 20);
	val = malloc(OCI_VARCHAR_SIZE + 5);
	fmt = malloc(OCI_STR_SIZE + 5);
	if (text == NULL || val == NULL || fmt == NULL) {
		if (text) free(text);
		if (val) free(val);
		if (fmt) free(fmt);
		strcpy((char *)outp, "Memory Allocation Error!");
		return;
	}
	/* get bind count */
	count = OCI_GetBindCount(st);
	if (count == 0) return;
	sprintf(text, "%u BindVars", count);
	i = OCI_BindArrayGetSize(st);
	if (i > 1) {
		sprintf(text, "%s, Array %u Rows, Binds for first row", text, i);
	}
	strcat(text, " -->");
	strcpy((char *)outp, text);
	/* loop through bind vars */
	for (i = 1; i <= count; i++) {
		/* get bind parameter */
		bn = OCI_GetBind(st, i);
		name = OCI_BindGetName(bn);
		type = OCI_BindGetType(bn);
		subtype = OCI_BindGetSubtype(bn);
		data = OCI_BindGetData(bn);
		size = OCI_BindGetDataSize(bn);
		/*
		 * different handling per data type
		 * build string values
		 * type in 'fmt'
		 * values in 'val'
		*/
		strcpy(val, "");
		switch (type) {
		case OCI_CDT_NUMERIC : //short, int, long long, double
			switch (subtype) {
			case OCI_NUM_SHORT:
				strcpy(fmt, "Short");
				sprintf(val, "'%i'", *(short *) data);
				break;
			case OCI_NUM_INT:
				strcpy(fmt, "Int");
				sprintf(val, "'%d'", *(int *) data);
				break;
			case OCI_NUM_BIGINT:
				strcpy(fmt, "BigInt");
				sprintf(val, "'%lld'", *(long long *) data);
				break;
			case OCI_NUM_USHORT:
				strcpy(fmt, "ushort");
				sprintf(val, "'%u'", *(unsigned short *) data);
				break;
			case OCI_NUM_UINT:
				strcpy(fmt, "uInt");
				sprintf(val, "'%u'", *(unsigned int *) data);
				break;
			case OCI_NUM_BIGUINT:
				strcpy(fmt, "Big uInt");
				sprintf(val, "'%llu'", *(unsigned long long *) data);
				break;
			case OCI_NUM_DOUBLE:
				strcpy(fmt, "Double");
				sprintf(val, "'%f'", *(double *) data);
				break;
			}
			break;

		case OCI_CDT_DATETIME : //OCI_Date *
			strcpy(fmt, "Date");
			dt = (OCI_Date *)data;
			OCI_DateToText(dt, "YYYY-MM-DD HH24:MI:SS", OCI_VARCHAR_SIZE, text);
			sprintf(val, "'%s'", text);
			break;
		case OCI_CDT_TEXT : //dtext *
			strncpy(text, (char *)data, OCI_VARCHAR_SIZE);
			sprintf(val, "'%s'", text);
			if (strlen(val) < strlen(data) + 2) {
				strcat(val, "...");
			}
			sprintf(fmt, "Text Size:%u", (unsigned int)strlen(data));
			break;
		case OCI_CDT_LONG : //OCI_Long *
			switch (subtype) {
			}
			strcpy(fmt, "Long");
			sprintf(val, "(n/a) Size: %u", size);
			break;
		case OCI_CDT_CURSOR : //OCI_Statement *
			strcpy(fmt, "Cursor");
			sprintf(val, "(n/a)");
			break;
		case OCI_CDT_LOB : //OCI_Lob *
			switch (subtype) {
			case OCI_BLOB:
				strcpy(fmt, "BLob");
				sprintf(val, "(n/a) Size: %u", size);
				break;
			case OCI_CLOB:
				strcpy(fmt, "CLob");
				sprintf(val, "(n/a) Size: %u", size);
				break;
			case OCI_NCLOB:
				strcpy(fmt, "NCLob");
				sprintf(val, "(n/a) Size: %u", size);
				break;
			}
			break;
		case OCI_CDT_FILE : //OCI_File *
			switch (subtype) {
			case OCI_BFILE:
				strcpy(fmt, "BFile");
				break;
			case OCI_CFILE:
				strcpy(fmt, "CFile");
				break;
			}
			file = (OCI_File *)data;
			strncpy(text, (char *)OCI_FileGetName(file), OCI_VARCHAR_SIZE);
			sprintf(val, "(FileName: '%s')", text);
			break;
		case OCI_CDT_TIMESTAMP : //OCI_Timestamp *
			switch (subtype) {
			case OCI_TIMESTAMP:
				strcpy(fmt, "Timestamp");
				break;
			case OCI_TIMESTAMP_TZ:
				strcpy(fmt, "TimeStamp TZ");
				break;
			case OCI_TIMESTAMP_LTZ:
				strcpy(fmt, "TimeStamp LTZ");
				break;
			}
			ts = (OCI_Timestamp *)data;
			OCI_TimestampToText(ts, "YYYY-MM-DD HH24:MI:SS FF6", OCI_VARCHAR_SIZE, text, 6);
			sprintf(val, "'%s'", text);
			break;
		case OCI_CDT_INTERVAL : //OCI_Interval *
			switch (subtype) {
			case OCI_INTERVAL_YM:
				strcpy(fmt, "Interval YM");
				break;
			case OCI_INTERVAL_DS:
				strcpy(fmt, "Interval DS");
				break;
			}
			inv = (OCI_Interval *)data;
			OCI_IntervalToText(inv, 10, 6, OCI_VARCHAR_SIZE, text);
			sprintf(val, "'%s'", text);
			break;
		case OCI_CDT_RAW : //void *
			strcpy(fmt, "RAW");
			sprintf(val, "(n/a) Size: %u", size);
			break;
		case OCI_CDT_OBJECT : //OCI_Object *
			strcpy(fmt, "Object");
			sprintf(val, "(n/a)");
			break;
		case OCI_CDT_COLLECTION : //OCI_Coll *
			strcpy(fmt, "Collection");
			sprintf(val, "(n/a)");
			break;
		case OCI_CDT_REF : //OCI_Ref *
			strcpy(fmt, "Ref");
			sprintf(val, "(n/a)");
			break;
			//case OCI_CDT_UNKNOWN: //not known
		default:
			strcpy(fmt, "Unknown");
			break;
		}//switch

		sprintf(text, "[Name:'%s',Type:%s,Val:%s]", name, fmt, val);
		/* add to provided buffer, short strings is needed */
		strncat((char *)outp, text, bsize - strlen((char*)outp) - 15);
		if (strlen((char *)outp) > bsize - 14) {
			/* buffer full, notice this and exit */
			strcat((char *)outp, "...->shorted!");
			break;
		}

	}//for
	free(text);
	free(val);
	free(fmt);
}

/**
 * check cleanup statement if handle is set and free it
 * @param st Statement handle variable
 * @param statement name
 * @return void
 */
void ido2db_oci_statement_free(OCI_Statement *st, char * stname) {
	char * fname = "ido2db_oci_statement_free";
	if (st == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Statement %s is null\n", fname, stname);
	} else {
		if (OCI_StatementGetConnection(st)) {
			OCI_StatementFree(st);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Statement %s freed\n", fname, stname);
		} else {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Statement %s invalid connection\n", fname, stname);
		}
	}
}

/**
 * load clob and bind
 * @param st Statement handle variable
 * @param bind variable name
 * @param text data to load
 * @param inout pointor to OCI_LOB structure
 * @return boolean
 */

int ido2db_oci_bind_clob(OCI_Statement *st, char * bindname, char * text, OCI_Lob ** lobp) {
	char * fname = "ido2db_oci_bind_clob";
	unsigned long len = 0;
	unsigned long slen = 0;
	unsigned int cb = 0; //char byte
	unsigned int bb = 0; //bin�r byte
	unsigned long rc = 0; //recent position
	const int chunk = OCI_LOB_CHUNK_SIZE; //max chunk size
	char * buffer =NULL; //temporary buffer
	int code=0;

	if (st == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Statement handle is null\n", fname);
		return IDO_ERROR;
	}
	if (bindname == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:bindname is null\n", fname);
		return IDO_ERROR;
	}
	if (lobp == NULL || *lobp == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:LobPointer is null\n", fname);
		return IDO_ERROR;
	}

	if (!OCI_BindLob(st, MT(bindname), *lobp)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s:Bind %s failed\n", fname, bindname);
		return IDO_ERROR;
	}


	len = text ? strlen(text) : 0;
	slen= text ? ido2db_oci_StringUTF8Length(text):0;
	if (len == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s:Bind %s,Null or empty\n", fname, bindname);
		OCI_BindSetNull(OCI_GetBind2(st, bindname));
		return IDO_OK;
	}

	buffer = malloc(chunk+1);
	if (buffer == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Buffer allocation error\n", fname);
		return IDO_ERROR;
	}
	//reset lob
	rc=0; //stream position
	OCI_LobTruncate(*lobp,rc);
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s:Bind %s,%lu Bytes requested\n", fname, bindname, len);

	while ((len - rc) > 0) {

		if ((len - rc) > chunk) {
			bb = chunk; //buffer full size
		} else {
			bb = len - rc; //remaining bytes
		}
		if (bb < 1) break; //should not happens
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s:Bind %s,Next chunk %lu bytes from pos %lu (total %lu) \n", fname, bindname,bb,rc, len);

		memset(buffer,0,chunk+1);
		memcpy(buffer,text+rc,bb);
		cb=0;//reset character pointer, we will work with bytes
		if (OCI_LobWrite2(*lobp, buffer, &cb, &bb)) {
			rc = rc + bb;
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s:Bind %s,%d Bytes=%d Chars (total %lu/%lu)   appended\n", fname, bindname, bb, cb,rc, len);
		} else {
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "%s:Bind %s,Last append failed: %d Bytes=%d Chars(total %lu/%lu):'%s'\n", fname, bindname, bb, cb, rc, len,buffer);
			code=1; //this is an error
			break;
		}

	}
	//final length check
	cb = OCI_LobGetLength(*lobp);
	if (cb == slen) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Bind %s,Lob successfully written:%lu bytes=%lu chars \n", fname, bindname, len,slen);
	} else {
		//final size differs, but only an error if write failed
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "%s:Bind %s,Lob write warning:len %lu, but lob size %lu bytes=%lu chars (last chunk '%s')\n", fname, bindname, len,slen, cb,buffer);
	}
	free(buffer);
	if (code == 1) return IDO_ERROR;
	return IDO_OK;
}
/* UTF8 ready string length function,
 * taken from http://sourceforge.net/projects/orclib/forums/forum/470801/topic/5000325
 */
int ido2db_oci_StringUTF8Length
(
    const char *str
)
{
    int size = 0;

    while (*str)
    {
        if ((*str & 0xc0) != 0x80)
        {
            size++;
        }

        str++;
    }

    return size;
}
/*
int ido2db_oci_bind_bigint(OCI_Statement *st, char * bindname, void ** data) {
#ifdef OCI_BIG_UINT_ENABLED
#endif
}
*/
#endif /* Oracle ocilib specific */

