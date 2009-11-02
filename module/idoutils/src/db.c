/***************************************************************
 * DB.C - Datatabase routines for NDO2DB daemon
 *
 * Copyright (c) 2005-2007 Ethan Galstad
 * Copyright (c) 2009 Icinga Development Team (http://www.icinga.org)
 *
 * Last Modified: 08-31-2009
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

extern int errno;

extern int ndo2db_log_debug_info(int , int , const char *, ...);

/* point to prepared statements after db initialize */
#ifdef USE_ORACLE
int ido2db_oci_prepared_statement_timedevents_queue(ndo2db_idi *idi);
#endif

extern ndo2db_dbconfig ndo2db_db_settings;
extern time_t ndo2db_db_last_checkin_time;

char *ndo2db_db_rawtablenames[NDO2DB_MAX_DBTABLES]={
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
	"serviceescalationcontactgroups"
#else /* everything else will be libdbi */
	"serviceescalation_contactgroups"
#endif /* Oracle ocilib specific */
	};

char *ndo2db_db_tablenames[NDO2DB_MAX_DBTABLES];

/*
 #define DEBUG_NDO2DB_QUERIES 1
 */

/****************************************************************************/
/* CONNECTION FUNCTIONS                                                     */
/****************************************************************************/


/************************************/
/* initialize database structures   */
/************************************/
int ndo2db_db_init(ndo2db_idi *idi) {
	register int x;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_init() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* initialize db server type */
	idi->dbinfo.server_type = ndo2db_db_settings.server_type;

	/* initialize table names */
	for (x = 0; x < NDO2DB_MAX_DBTABLES; x++) {

	        switch (idi->dbinfo.server_type) {
        		case NDO2DB_DBSERVER_MYSQL:
		        case NDO2DB_DBSERVER_PGSQL:
		        case NDO2DB_DBSERVER_DB2:
		        case NDO2DB_DBSERVER_FIREBIRD:
		        case NDO2DB_DBSERVER_FREETDS:
		        case NDO2DB_DBSERVER_INGRES:
		        case NDO2DB_DBSERVER_MSQL:
		                if ((ndo2db_db_tablenames[x] = (char *) malloc(strlen(ndo2db_db_rawtablenames[x]) + ((ndo2db_db_settings.dbprefix==NULL) ? 0 : strlen(ndo2db_db_settings.dbprefix)) + 1))==NULL)
                		        return NDO_ERROR;
				
				sprintf(ndo2db_db_tablenames[x], "%s%s", (ndo2db_db_settings.dbprefix==NULL) ? "" : ndo2db_db_settings.dbprefix,ndo2db_db_rawtablenames[x]);
				break;
		        case NDO2DB_DBSERVER_ORACLE:
#ifdef USE_ORACLE /* Oracle ocilib specific */
				/* don't allow user to set table prefix for oracle */
        		        if ((ndo2db_db_tablenames[x] = (char *) malloc(strlen(ndo2db_db_rawtablenames[x])))==NULL)
                        		return NDO_ERROR;

				sprintf(ndo2db_db_tablenames[x], "%s", ndo2db_db_rawtablenames[x]);
#endif /* Oracle ocilib specific */
		                break;
		        case NDO2DB_DBSERVER_SQLITE:
		        case NDO2DB_DBSERVER_SQLITE3:
		                if ((ndo2db_db_tablenames[x] = (char *) malloc(strlen(ndo2db_db_rawtablenames[x]) + ((ndo2db_db_settings.dbprefix==NULL) ? 0 : strlen(ndo2db_db_settings.dbprefix)) + 1))==NULL)
                		        return NDO_ERROR;
		                
				sprintf(ndo2db_db_tablenames[x], "%s%s", (ndo2db_db_settings.dbprefix==NULL) ? "" : ndo2db_db_settings.dbprefix,ndo2db_db_rawtablenames[x]);
				break;
		        default:
                		break;
        	}
	}

	/* initialize other variables */
	idi->dbinfo.connected = NDO_FALSE;
	idi->dbinfo.error = NDO_FALSE;
	idi->dbinfo.instance_id = 0L;
	idi->dbinfo.conninfo_id = 0L;
	idi->dbinfo.latest_program_status_time = (time_t) 0L;
	idi->dbinfo.latest_host_status_time = (time_t) 0L;
	idi->dbinfo.latest_service_status_time = (time_t) 0L;
	idi->dbinfo.latest_queued_event_time = (time_t) 0L;
	idi->dbinfo.latest_realtime_data_time = (time_t) 0L;
	idi->dbinfo.latest_comment_time = (time_t) 0L;
	idi->dbinfo.clean_event_queue = NDO_FALSE;
	idi->dbinfo.last_notification_id = 0L;
	idi->dbinfo.last_contact_notification_id = 0L;
	idi->dbinfo.max_timedevents_age = ndo2db_db_settings.max_timedevents_age;
	idi->dbinfo.max_systemcommands_age = ndo2db_db_settings.max_systemcommands_age;
	idi->dbinfo.max_servicechecks_age = ndo2db_db_settings.max_servicechecks_age;
	idi->dbinfo.max_hostchecks_age = ndo2db_db_settings.max_hostchecks_age;
	idi->dbinfo.max_eventhandlers_age = ndo2db_db_settings.max_eventhandlers_age;
	idi->dbinfo.max_externalcommands_age=ndo2db_db_settings.max_externalcommands_age;
	idi->dbinfo.trim_db_interval=ndo2db_db_settings.trim_db_interval;
	idi->dbinfo.last_table_trim_time = (time_t) 0L;
	idi->dbinfo.last_logentry_time = (time_t) 0L;
	idi->dbinfo.last_logentry_data = NULL;
	idi->dbinfo.object_hashlist = NULL;

	/* initialize db structures, etc. */
#ifndef USE_ORACLE /* everything else will be libdbi */

	if (dbi_initialize(NULL) == -1) {
		syslog(LOG_USER | LOG_INFO, "Error: dbi_initialize() failed\n");
		return NDO_ERROR;
	}
#else /* Oracle ocilib specific */

	/* register error handler and init oci */
	if(!OCI_Initialize(ido2db_ocilib_err_handler, NULL, OCI_ENV_DEFAULT)) {
		syslog(LOG_USER | LOG_INFO, "Error:  OCI_Initialize() failed\n");
		ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "OCI_Initialize() failed\n");
		return NDO_ERROR;
	} 

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "OCI_Initialize() ok\n");

#endif /* Oracle ocilib specific */

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_init() end\n");
	return NDO_OK;
}

/************************************/
/* clean up database structures     */
/************************************/
int ndo2db_db_deinit(ndo2db_idi *idi) {
	register int x;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_deinit() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* free table names */
	for (x = 0; x < NDO2DB_MAX_DBTABLES; x++) {
		if (ndo2db_db_tablenames[x])
			free(ndo2db_db_tablenames[x]);
		ndo2db_db_tablenames[x] = NULL;
	}

	/* free cached object ids */
	ndo2db_free_cached_object_ids(idi);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_deinit() end\n");
	return NDO_OK;
}

/************************************/
/* connects to the database server  */
/************************************/
int ndo2db_db_connect(ndo2db_idi *idi) {
	int result = NDO_OK;
	const char *dbi_error;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_connect() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* we're already connected... */
	if (idi->dbinfo.connected == NDO_TRUE)
		return NDO_OK;

	switch (idi->dbinfo.server_type) {
	case NDO2DB_DBSERVER_MYSQL:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_MYSQL);
		break;
	case NDO2DB_DBSERVER_PGSQL:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_PGSQL);
		break;
	case NDO2DB_DBSERVER_DB2:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_DB2);
		break;
	case NDO2DB_DBSERVER_FIREBIRD:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_FIREBIRD);
		break;
	case NDO2DB_DBSERVER_FREETDS:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_FREETDS);
		break;
	case NDO2DB_DBSERVER_INGRES:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_INGRES);
		break;
	case NDO2DB_DBSERVER_MSQL:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_MSQL);
		break;
	case NDO2DB_DBSERVER_ORACLE:
#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* for ocilib there is no such statement next below */

#endif /* Oracle ocilib specific */
		break;
	case NDO2DB_DBSERVER_SQLITE:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_SQLITE);
		break;
	case NDO2DB_DBSERVER_SQLITE3:
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_SQLITE3);
		break;
	default:
		break;
	}

	/* Check if the dbi connection was created successful */

#ifndef USE_ORACLE /* everything else will be libdbi */
	if (idi->dbinfo.dbi_conn == NULL) {
		dbi_conn_error(idi->dbinfo.dbi_conn, &dbi_error);
		syslog(LOG_USER | LOG_INFO, "Error: Could  not dbi_conn_new(): %s", dbi_error);
		result = NDO_ERROR;
		idi->disconnect_client = NDO_TRUE;
		return NDO_ERROR;
	}

	dbi_conn_set_option(idi->dbinfo.dbi_conn, "host", ndo2db_db_settings.host);
	dbi_conn_set_option(idi->dbinfo.dbi_conn, "username", ndo2db_db_settings.username);
	dbi_conn_set_option(idi->dbinfo.dbi_conn, "password", ndo2db_db_settings.password);
	dbi_conn_set_option(idi->dbinfo.dbi_conn, "dbname", ndo2db_db_settings.dbname);
	dbi_conn_set_option(idi->dbinfo.dbi_conn, "encoding", "auto");

	if (dbi_conn_connect(idi->dbinfo.dbi_conn) != 0) {
		dbi_conn_error(idi->dbinfo.dbi_conn, &dbi_error);
		syslog(LOG_USER | LOG_INFO, "Error: Could not connect to database: %s", dbi_error);
		result = NDO_ERROR;
		idi->disconnect_client = NDO_TRUE;
	} else {
		idi->dbinfo.connected = NDO_TRUE;
		syslog(LOG_USER | LOG_INFO, "Successfully connected to database");
	}
#else /* Oracle ocilib specific */

	/* create db connection instantly */

	idi->dbinfo.oci_connection = OCI_ConnectionCreate((mtext *)ndo2db_db_settings.dbname, (mtext *)ndo2db_db_settings.username, (mtext *)ndo2db_db_settings.password, OCI_SESSION_DEFAULT);
	
	if(idi->dbinfo.oci_connection == NULL) {
		syslog(LOG_USER | LOG_INFO, "Error: Could not connect to oracle database: %s", OCI_ErrorGetString(OCI_GetLastError()));
		ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "Error: Could not connect to oracle database\n");
                result = NDO_ERROR;
                idi->disconnect_client = NDO_TRUE;
        } else {
                idi->dbinfo.connected = NDO_TRUE;
                syslog(LOG_USER | LOG_INFO, "Successfully connected to oracle database");
		ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "Successfully connected to oracle database\n");
	}

        /* initialize prepared statements */
        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() called\n");

        if(ido2db_oci_prepared_statement_timedevents_queue(idi) == NDO_ERROR) {
                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() failed\n");
                return NDO_ERROR;
        }
        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() call done\n");

#endif /* Oracle ocilib specific */

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_connect(%d) end\n", result);
	return result;
}


/****************************************/
/* disconnects from the database server */
/****************************************/
int ndo2db_db_disconnect(ndo2db_idi *idi) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_disconnect() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* we're not connected... */
	if (idi->dbinfo.connected == NDO_FALSE)
		return NDO_OK;

#ifndef USE_ORACLE /* everything else will be libdbi */
	dbi_conn_close(idi->dbinfo.dbi_conn);
	dbi_shutdown();

	syslog(LOG_USER | LOG_INFO, "Successfully disconnected from database");
#else /* Oracle ocilib specific */

	/* close prepared statements */
	OCI_StatementFree(idi->dbinfo.oci_statement_timedevents);
	OCI_StatementFree(idi->dbinfo.oci_statement_timedevents_queue);
	OCI_StatementFree(idi->dbinfo.oci_statement_hostchecks);
	OCI_StatementFree(idi->dbinfo.oci_statement_hoststatus);
	OCI_StatementFree(idi->dbinfo.oci_statement_servicechecks);
	OCI_StatementFree(idi->dbinfo.oci_statement_servicestatus);


	/* close db connection */
	OCI_ConnectionFree(idi->dbinfo.oci_connection);
	OCI_Cleanup();
	syslog(LOG_USER | LOG_INFO, "Successfully disconnected from oracle database");

#endif /* Oracle ocilib specific */

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_disconnect() stop\n");
	return NDO_OK;
}

/************************************/
/* post-connect routines            */
/************************************/
int ndo2db_db_hello(ndo2db_idi *idi) {
	char *buf = NULL;
	char *buf1 = NULL;
	char *ts = NULL;
	int result = NDO_OK;
	int have_instance = NDO_FALSE;
	time_t current_time;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello() start\n");

	/* make sure we have an instance name */
	if (idi->instance_name == NULL)
		idi->instance_name = strdup("default");

#ifndef USE_ORACLE /* everything else will be libdbi */

	/* get existing instance */
	if (asprintf(&buf, "SELECT instance_id FROM %s WHERE instance_name='%s'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_INSTANCES], idi->instance_name)
			== -1)
		buf = NULL;

	if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {

		if (idi->dbinfo.dbi_result != NULL) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
				idi->dbinfo.instance_id = dbi_result_get_uint(idi->dbinfo.dbi_result, "instance_id");
				have_instance = NDO_TRUE;
			}
		}
	}
#else /* Oracle ocilib specific */

        /* get existing instance */
        if (asprintf(&buf, "SELECT id FROM %s WHERE instance_name='%s'", ndo2db_db_tablenames[NDO2DB_DBTABLE_INSTANCES], idi->instance_name) == -1)
                buf = NULL;

        if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {

		ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello() query ok\n");
			
		if(OCI_FetchNext(idi->dbinfo.oci_resultset)) {
			ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello() fetchnext ok\n");
			idi->dbinfo.instance_id	= OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 1);

			if(idi->dbinfo.instance_id == 0) {
				have_instance = NDO_FALSE;
			} else {
				have_instance = NDO_TRUE;
			}		
		} else {
			ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello() fetchnext not ok\n");
		}
		ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello(instance_id=%lu)\n", idi->dbinfo.instance_id);
        }

#endif /* Oracle ocilib specific */
	
	else {
		ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello() existing instance not found, cleaning up and exiting\n");
		/* cleanup the socket */
		ndo2db_cleanup_socket();

		/* free memory */
		ndo2db_free_program_memory();

		_exit(0);
	}

#ifndef USE_ORACLE /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
#else /* Oracle ocilib specific */

	OCI_StatementFree(idi->dbinfo.oci_statement);

#endif /* Oracle ocilib specific */
	
	free(buf);

	/* insert new instance if necessary */
	if (have_instance == NDO_FALSE) {
		if (asprintf(&buf, "INSERT INTO %s (instance_name) VALUES ('%s')", ndo2db_db_tablenames[NDO2DB_DBTABLE_INSTANCES], idi->instance_name) == -1)
			buf = NULL;
		if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {
	                switch (idi->dbinfo.server_type) {
        	                case NDO2DB_DBSERVER_MYSQL:
                	                /* mysql doesn't use sequences */
	                                idi->dbinfo.instance_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
	                                break;
	                        case NDO2DB_DBSERVER_PGSQL:
	                                /* depending on tableprefix/tablename a sequence will be used */
	                                if(asprintf(&buf1, "%s_instance_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_INSTANCES]) == -1)
        	                                buf1 = NULL;
	
	                                idi->dbinfo.instance_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
        	                        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello(%s=%lu) instance_id\n", buf1, idi->dbinfo.instance_id);
	                                free(buf1);
        	                        break;
	                        case NDO2DB_DBSERVER_DB2:
	                                break;
	                        case NDO2DB_DBSERVER_FIREBIRD:
	                                break;
	                        case NDO2DB_DBSERVER_FREETDS:
	                                break;
	                        case NDO2DB_DBSERVER_INGRES:
	                                break;
	                        case NDO2DB_DBSERVER_MSQL:
	                                break;
	                        case NDO2DB_DBSERVER_ORACLE:
#ifdef USE_ORACLE /* Oracle ocilib specific */

					idi->dbinfo.instance_id = ido2db_ocilib_insert_id(idi);
					ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello(%lu) instance_id\n", idi->dbinfo.instance_id);

#endif /* Oracle ocilib specific */
        	                        break;
	                        case NDO2DB_DBSERVER_SQLITE:
	                                break;
	                        case NDO2DB_DBSERVER_SQLITE3:
	                                break;
        	                default:
	                                break;
        	        }
		}

#ifndef USE_ORACLE /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
#else /* Oracle ocilib specific */

		 OCI_StatementFree(idi->dbinfo.oci_statement);

#endif /* Oracle ocilib specific */

		free(buf);
	}

	ts = ndo2db_db_timet_to_sql(idi, idi->data_start_time);

	/* record initial connection information */
#ifndef USE_ORACLE /* everything else will be libdbi */
	if (asprintf(&buf, "INSERT INTO %s (instance_id, connect_time, last_checkin_time, bytes_processed, lines_processed, entries_processed, agent_name, agent_version, disposition, connect_source, connect_type, data_start_time) VALUES ('%lu', NOW(), NOW(), '0', '0', '0', '%s', '%s', '%s', '%s', '%s', NOW())",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO],
			idi->dbinfo.instance_id, idi->agent_name, idi->agent_version,
			idi->disposition, idi->connect_source, idi->connect_type) == -1)
		buf = NULL;

#else /* Oracle ocilib specific */
	if (asprintf(&buf, "INSERT INTO %s (instance_id, connect_time, last_checkin_time, bytes_processed, lines_processed, entries_processed, agent_name, agent_version, disposition, connect_source, connect_type, data_start_time) VALUES ('%lu', SYSDATE, SYSDATE, '0', '0', '0', '%s', '%s', '%s', '%s', '%s', SYSDATE)",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO],
			idi->dbinfo.instance_id, idi->agent_name, idi->agent_version,
			idi->disposition, idi->connect_source, idi->connect_type) == -1)
		buf = NULL;

#endif /* Oracle ocilib specific */

	if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {

	        switch (idi->dbinfo.server_type) {
        	        case NDO2DB_DBSERVER_MYSQL:
				/* mysql doesn't use sequences */
				idi->dbinfo.conninfo_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
				ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello(%lu) conninfo_id\n", idi->dbinfo.conninfo_id);
        	                break;
	                case NDO2DB_DBSERVER_PGSQL:
				/* depending on tableprefix/tablename a sequence will be used */
				if(asprintf(&buf1, "%s_conninfo_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO]) == -1)
					buf1 = NULL;

				idi->dbinfo.conninfo_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
				ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello(%s=%lu) conninfo_id\n", buf1, idi->dbinfo.conninfo_id);
				free(buf1);
                	        break;
	                case NDO2DB_DBSERVER_DB2:
        	                break;
	                case NDO2DB_DBSERVER_FIREBIRD:
        	                break;
	                case NDO2DB_DBSERVER_FREETDS:
        	                break;
	                case NDO2DB_DBSERVER_INGRES:
        	                break;
	                case NDO2DB_DBSERVER_MSQL:
        	                break;
	                case NDO2DB_DBSERVER_ORACLE:
#ifdef USE_ORACLE /* Oracle ocilib specific */

                                idi->dbinfo.conninfo_id = ido2db_ocilib_insert_id(idi);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello(%lu) conninfo_id\n", idi->dbinfo.conninfo_id);

#endif /* Oracle ocilib specific */
        	                break;
	                case NDO2DB_DBSERVER_SQLITE:
        	                break;
	                case NDO2DB_DBSERVER_SQLITE3:
        	                break;
	                default:
        	                break;
        	}
	}

#ifndef USE_ORACLE /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
#else /* Oracle ocilib specific */

	OCI_StatementFree(idi->dbinfo.oci_statement);

#endif /* Oracle ocilib specific */

	free(buf);
	free(ts);

	/* get cached object ids... */
	ndo2db_get_cached_object_ids(idi);

	/* get latest times from various tables... */
	ndo2db_db_get_latest_data_time(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_program_status_time);
	ndo2db_db_get_latest_data_time(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTSTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_host_status_time);
	ndo2db_db_get_latest_data_time(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICESTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_service_status_time);
	ndo2db_db_get_latest_data_time(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTSTATUS], "status_update_time", (unsigned long *) &idi->dbinfo.latest_contact_status_time);
	ndo2db_db_get_latest_data_time(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE], "queued_time", (unsigned long *) &idi->dbinfo.latest_queued_event_time);
	ndo2db_db_get_latest_data_time(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTS], "entry_time", (unsigned long *) &idi->dbinfo.latest_comment_time);

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
	idi->dbinfo.clean_event_queue = NDO_TRUE;

	/* set misc data */
	idi->dbinfo.last_notification_id = 0L;
	idi->dbinfo.last_contact_notification_id = 0L;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello() end\n");
	return result;
}

/************************************/
/* pre-disconnect routines          */
/************************************/
int ndo2db_db_goodbye(ndo2db_idi *idi) {
	int result = NDO_OK;
	char *buf = NULL;
	char *ts = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_goodbye() start\n");

	ts = ndo2db_db_timet_to_sql(idi, idi->data_end_time);

	/* record last connection information */
#ifndef USE_ORACLE /* everything else will be libdbi */
	if (asprintf(&buf, "UPDATE %s SET disconnect_time=NOW(), last_checkin_time=NOW(), data_end_time=%s, bytes_processed='%lu', lines_processed='%lu', entries_processed='%lu' WHERE conninfo_id='%lu'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO], ts,
			idi->bytes_processed, idi->lines_processed, idi->entries_processed,
			idi->dbinfo.conninfo_id) == -1)
		buf = NULL;

#else /* Oracle ocilib specific */
	if (asprintf(&buf, "UPDATE %s SET disconnect_time=SYSDATE, last_checkin_time=SYSDATE, data_end_time=%s, bytes_processed='%lu', lines_processed='%lu', entries_processed='%lu' WHERE id='%lu'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO], ts,
			idi->bytes_processed, idi->lines_processed, idi->entries_processed,
			idi->dbinfo.conninfo_id) == -1)
		buf = NULL;        

#endif /* Oracle ocilib specific */

	result = ndo2db_db_query(idi, buf);

#ifndef USE_ORACLE /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
#else /* Oracle ocilib specific */

	 OCI_StatementFree(idi->dbinfo.oci_statement);

#endif /* Oracle ocilib specific */

	free(buf);
	free(ts);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_goodbye() end\n");
	return result;
}

/************************************/
/* checking routines                */
/************************************/
int ndo2db_db_checkin(ndo2db_idi *idi) {
	int result = NDO_OK;
	char *buf = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_checkin() start\n");

	/* record last connection information */
#ifndef USE_ORACLE /* everything else will be libdbi */
	if (asprintf(&buf, "UPDATE %s SET last_checkin_time=NOW(), bytes_processed='%lu', lines_processed='%lu', entries_processed='%lu' WHERE conninfo_id='%lu'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO],
			idi->bytes_processed, idi->lines_processed, idi->entries_processed,
			idi->dbinfo.conninfo_id) == -1)
		buf = NULL;

#else /* Oracle ocilib specific */
	if (asprintf(&buf, "UPDATE %s SET last_checkin_time=SYSDATE, bytes_processed='%lu', lines_processed='%lu', entries_processed='%lu' WHERE id='%lu'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO],
			idi->bytes_processed, idi->lines_processed, idi->entries_processed,
			idi->dbinfo.conninfo_id) == -1)
		buf = NULL;

#endif /* Oracle ocilib specific */

	result = ndo2db_db_query(idi, buf);

#ifndef USE_ORACLE /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
#else /* Oracle ocilib specific */

	 OCI_StatementFree(idi->dbinfo.oci_statement);

#endif /* Oracle ocilib specific */

	free(buf);

	time(&ndo2db_db_last_checkin_time);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_checkin() end\n");
	return result;
}

/****************************************************************************/
/* MISC FUNCTIONS                                                           */
/****************************************************************************/

/***************************************/
/* escape a string for a SQL statement */
/***************************************/
char *ndo2db_db_escape_string(ndo2db_idi *idi, char *buf) {
	register int x, y, z;
	char *newbuf = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_escape_string(%s) start\n", buf);

	if (idi == NULL || buf == NULL)
		return NULL;

	z = strlen(buf);

	/* allocate space for the new string */
	if ((newbuf = (char *) malloc((z * 2) + 1)) == NULL)
		return NULL;

	/* escape characters */
	for (x = 0, y = 0; x < z; x++) {

        	switch (idi->dbinfo.server_type) {
	                case NDO2DB_DBSERVER_MYSQL:
				if (buf[x] == '\'' || buf[x] == '\"' || buf[x] == '*' || buf[x]	== '\\' || 
					buf[x] == '$' || buf[x] == '?' || buf[x] == '.'	|| 
					buf[x] == '^' || buf[x] == '+' || buf[x] == '['	|| 
					buf[x] == ']' || buf[x] == '(' || buf[x] == ')')
					newbuf[y++] = '\\';
				break;
			case NDO2DB_DBSERVER_PGSQL:
				if (buf[x] == '\'' || buf[x] == '[' || 
					buf[x] == ']' || buf[x] == '(' || buf[x] == ')')
					newbuf[y++] = '\\';
	                        break;
        	        case NDO2DB_DBSERVER_DB2:
	                        break;
	                case NDO2DB_DBSERVER_FIREBIRD:
	                        break;
	                case NDO2DB_DBSERVER_FREETDS:
	                        break;
	                case NDO2DB_DBSERVER_INGRES:
	                        break;
	                case NDO2DB_DBSERVER_MSQL:
	                        break;
	                case NDO2DB_DBSERVER_ORACLE:

#ifdef USE_ORACLE /* Oracle ocilib specific */
				if(buf[x]=='\'' )
					newbuf[y++]='\'';
#endif /* Oracle ocilib specific */

        	                break;
	                case NDO2DB_DBSERVER_SQLITE:
	                        break;
	                case NDO2DB_DBSERVER_SQLITE3:
	                        break;
	                default:
	                        break;
		}

		newbuf[y++] = buf[x];
	}

	/* terminate escape string */
	newbuf[y] = '\0';

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_escape_string(%s) end\n", newbuf);
	return newbuf;
}

/*************************************************************/
/* SQL query conversion of time_t format to date/time format */
/*************************************************************/
char *ndo2db_db_timet_to_sql(ndo2db_idi *idi, time_t t) {
	char *buf = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_timet_to_sql() start\n");

	switch (idi->dbinfo.server_type) {
		case NDO2DB_DBSERVER_MYSQL:
			asprintf(&buf, "FROM_UNIXTIME(%lu)", (unsigned long) t);
			break;
		case NDO2DB_DBSERVER_PGSQL:
			/* from_unixtime is a PL/SQL function (defined in db/pgsql.sql) */
			asprintf(&buf, "FROM_UNIXTIME(%lu)", (unsigned long) t);
			break;
                case NDO2DB_DBSERVER_DB2:
                        break;
                case NDO2DB_DBSERVER_FIREBIRD:
                        break;
                case NDO2DB_DBSERVER_FREETDS:
                        break;
                case NDO2DB_DBSERVER_INGRES:
                        break;
                case NDO2DB_DBSERVER_MSQL:
                        break;
                case NDO2DB_DBSERVER_ORACLE:

#ifdef USE_ORACLE /* Oracle ocilib specific */
                        /* unixts2date is a PL/SQL function (defined in db/oracle.sql) */
                        asprintf(&buf,"(SELECT unixts2date(%lu) FROM DUAL)",(unsigned long)t);
#endif /* Oracle ocilib specific */

                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
	} 

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_timet_to_sql() end\n");

	return buf;
}

/*************************************************************/
/* SQL query conversion of date/time format to time_t format */
/*************************************************************/
char *ndo2db_db_sql_to_timet(ndo2db_idi *idi, char *field) {
	char *buf = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_sql_to_timet() start\n");

	switch (idi->dbinfo.server_type) { 
		case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&buf,"UNIX_TIMESTAMP(%s)",(field==NULL)?"":field);
			break;
                case NDO2DB_DBSERVER_PGSQL:
                        /* unix_timestamp is a PL/SQL function (defined in db/pgsql.sql) */
                        asprintf(&buf,"UNIX_TIMESTAMP(%s)",(field==NULL)?"":field);
			break;
                case NDO2DB_DBSERVER_DB2:
                        break;
                case NDO2DB_DBSERVER_FIREBIRD:
                        break;
                case NDO2DB_DBSERVER_FREETDS:
                        break;
                case NDO2DB_DBSERVER_INGRES:
                        break;
                case NDO2DB_DBSERVER_MSQL:
                        break;
                case NDO2DB_DBSERVER_ORACLE:

#ifdef USE_ORACLE /* Oracle ocilib specific */
			asprintf(&buf,"((SELECT ((SELECT %s FROM %%s) - TO_DATE('01-01-1970 00:00:00','dd-mm-yyyy hh24:mi:ss')) * 86400) FROM DUAL)",(field==NULL)?"":field);
#endif/* Oracle ocilib specific */

                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        } 
	
	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_sql_to_timet() end\n");

	return buf;
}

/************************************/
/* executes a SQL statement         */
/************************************/
int ndo2db_db_query(ndo2db_idi *idi, char *buf) {
	int result = NDO_OK;
	const char *error_msg;

	//ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_query(%s) start\n", buf);
	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_query() start\n");

	if (idi == NULL || buf == NULL)
		return NDO_ERROR;

	/* if we're not connected, try and reconnect... */
	if (idi->dbinfo.connected == NDO_FALSE) {
		if (ndo2db_db_connect(idi) == NDO_ERROR)
			return NDO_ERROR;
		ndo2db_db_hello(idi);
	}

#ifdef DEBUG_NDO2DB_QUERIES
	printf("%s\n\n",buf);
#endif

	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL, 0, "%s\n", buf);

#ifndef USE_ORACLE /* everything else will be libdbi */
	idi->dbinfo.dbi_result = dbi_conn_query(idi->dbinfo.dbi_conn, buf);

	if (idi->dbinfo.dbi_result == NULL){
		dbi_conn_error(idi->dbinfo.dbi_conn, &error_msg);

		syslog(LOG_USER | LOG_INFO, "Error: database query failed for '%s' - '%s'\n", buf, error_msg);

		ndo2db_handle_db_error(idi);
		result = NDO_ERROR;
	}
#else /* Oracle ocilib specific */

	int oci_res = 0;

        /* create statement handler */
        idi->dbinfo.oci_statement = OCI_StatementCreate(idi->dbinfo.oci_connection);

	/* execute query in one go */
	oci_res = OCI_ExecuteStmt(idi->dbinfo.oci_statement, MT(buf));

	/*  get result set */
	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement);

	/* check for errors */
	if(!oci_res) {
		
		syslog(LOG_USER | LOG_INFO, "Error: database query failed for '%s'\n", buf);
		ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "Error: database query failed for: '%s'\n", buf);

		ndo2db_handle_db_error(idi);
		result = NDO_ERROR;

	}

	/* since we do not want to set auto commit to true, we do it manually */
	OCI_Commit(idi->dbinfo.oci_connection);


#endif /* Oracle ocilib specific */

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_query(%d) end\n", result);
	return result;
}

//FIXME
//define query-function for bind params


/****************************************/
/* frees memory associated with a query */
/****************************************/
int ndo2db_db_free_query(ndo2db_idi *idi) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_free_query() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* ToDo: Examine where this function is called
	 * Not freeing the query may result in memory leaks
	 **/

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_free_query() end\n");
	return NDO_OK;
}

/************************************/
/* handles SQL query errors         */
/************************************/
int ndo2db_handle_db_error(ndo2db_idi *idi) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_db_error() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* we're not currently connected... */
	if (idi->dbinfo.connected == NDO_FALSE)
		return NDO_OK;

	ndo2db_db_disconnect(idi);

	idi->disconnect_client = NDO_TRUE;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_db_error() end\n");
	return NDO_OK;
}

/**********************************************************/
/* clears data from a given table (current instance only) */
/**********************************************************/
int ndo2db_db_clear_table(ndo2db_idi *idi, char *table_name) {
	char *buf = NULL;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_clear_table() start\n");

	if (idi == NULL || table_name == NULL)
		return NDO_ERROR;

	if (asprintf(&buf, "DELETE FROM %s WHERE instance_id='%lu'", table_name, idi->dbinfo.instance_id) == -1)
		buf = NULL;

	result = ndo2db_db_query(idi, buf);

#ifndef USE_ORACLE /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
#else /* Oracle ocilib specific */

	OCI_StatementFree(idi->dbinfo.oci_statement);

#endif /* Oracle ocilib specific */

	free(buf);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_clear_table() end\n");
	return result;
}

/**************************************************/
/* gets latest data time value from a given table */
/**************************************************/
int ndo2db_db_get_latest_data_time(ndo2db_idi *idi, char *table_name, char *field_name, unsigned long *t) {
	char *buf = NULL;
	char *ts[1];
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_get_latest_data_time() start\n");

	if (idi == NULL || table_name == NULL || field_name == NULL || t == NULL)
		return NDO_ERROR;

	*t = (time_t) 0L;
	ts[0] = ndo2db_db_sql_to_timet(idi, field_name);

#ifndef USE_ORACLE /* everything else will be libdbi */

	if (asprintf(&buf,"SELECT %s AS latest_time FROM %s WHERE instance_id='%lu' ORDER BY %s DESC LIMIT 1 OFFSET 0",
			field_name, table_name, idi->dbinfo.instance_id, field_name) == -1)
		buf = NULL;

	if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {
		if (idi->dbinfo.dbi_result != NULL) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result)){
				// ndo2db_convert_string_to_unsignedlong(dbi_result_get_string_copy(idi->dbinfo.dbi_result, ts[0]), t);
				*t = dbi_result_get_datetime(idi->dbinfo.dbi_result, "latest_time");
			}
		}
	}

#else /* Oracle ocilib specific */

        if( asprintf(&buf,"SELECT ( ( ( SELECT * FROM ( SELECT %s FROM %s WHERE instance_id='%lu' ORDER BY %s DESC) WHERE ROWNUM = 1 ) - to_date( '01-01-1970 00:00:00','dd-mm-yyyy hh24:mi:ss' )) * 86400) AS latest_time FROM DUAL"
                    ,(field_name==NULL)?"":field_name
                    ,table_name
                    ,idi->dbinfo.instance_id
                    ,field_name
                   )==-1)
                buf=NULL;

        if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {

                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_get_latest_data_time() query ok\n");
                if (idi->dbinfo.oci_resultset != NULL) {
                        /* check if next row */
                        if(OCI_FetchNext(idi->dbinfo.oci_resultset)) {
                                *t = OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 1);
				ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_get_latest_data_time(%lu)\n", *t);
                        }
                }
        }

#endif /* Oracle ocilib specific */

#ifndef USE_ORACLE /* everything else will be libdbi */
        dbi_result_free(idi->dbinfo.dbi_result);
#else /* Oracle ocilib specific */

	OCI_StatementFree(idi->dbinfo.oci_statement);

#endif /* Oracle ocilib specific */

	free(buf);
        free(ts[0]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_get_latest_data_time() end\n");
	return result;
}

/*******************************************/
/* trim/delete old data from a given table */
/*******************************************/
int ndo2db_db_trim_data_table(ndo2db_idi *idi, char *table_name, char *field_name, unsigned long t) {
	char *buf = NULL;
	char *ts[1];
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_trim_data_table() start\n");

	if (idi == NULL || table_name == NULL || field_name == NULL)
		return NDO_ERROR;

	ts[0] = ndo2db_db_timet_to_sql(idi, (time_t) t);

#ifndef USE_ORACLE /* everything else will be libdbi */

	if (asprintf(&buf, "DELETE FROM %s WHERE instance_id='%lu' AND %s<%s",
			table_name, idi->dbinfo.instance_id, field_name, ts[0]) == -1)
		buf = NULL;

	result = ndo2db_db_query(idi, buf);

#else /* Oracle ocilib specific */

	//FIXME - use special plsql query with commits
        if (asprintf(&buf, "DELETE FROM %s WHERE instance_id='%lu' AND %s<%s",
                        table_name, idi->dbinfo.instance_id, field_name, ts[0]) == -1)
                buf = NULL;

        result = ndo2db_db_query(idi, buf);

#endif /* Oracle ocilib specific */

#ifndef USE_ORACLE /* everything else will be libdbi */
        dbi_result_free(idi->dbinfo.dbi_result);
#else /* Oracle ocilib specific */

	OCI_StatementFree(idi->dbinfo.oci_statement);

#endif /* Oracle ocilib specific */

	free(buf);
        free(ts[0]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_trim_data_table() end\n");
	return result;
}

/***********************************************/
/* performs some periodic table maintenance... */
/***********************************************/
int ndo2db_db_perform_maintenance(ndo2db_idi *idi) {
	time_t current_time;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_perform_maintenance() start\n");

	/* get the current time */
	time(&current_time);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_perform_maintenance() max_timedevents_age=%lu, max_systemcommands_age=%lu, max_servicechecks_age=%lu, max_hostchecks_age=%lu, max_eventhandlers_age=%lu, max_externalcommands_age=%lu, trim_db_interval=%lu\n", idi->dbinfo.max_timedevents_age, idi->dbinfo.max_systemcommands_age, idi->dbinfo.max_servicechecks_age, idi->dbinfo.max_hostchecks_age, idi->dbinfo.max_eventhandlers_age, idi->dbinfo.max_externalcommands_age, idi->dbinfo.trim_db_interval);

	/* trim tables */
	if (((unsigned long) current_time - idi->dbinfo.trim_db_interval) > (unsigned long) idi->dbinfo.last_table_trim_time) {
		if (idi->dbinfo.max_timedevents_age > 0L)
			ndo2db_db_trim_data_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS], "scheduled_time", (time_t) ((unsigned long) current_time - idi->dbinfo.max_timedevents_age));
		if (idi->dbinfo.max_systemcommands_age > 0L)
			ndo2db_db_trim_data_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SYSTEMCOMMANDS], "start_time", (time_t) ((unsigned long) current_time - idi->dbinfo.max_systemcommands_age));
		if (idi->dbinfo.max_servicechecks_age > 0L)
			ndo2db_db_trim_data_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECHECKS], "start_time", (time_t) ((unsigned long) current_time - idi->dbinfo.max_servicechecks_age));
		if (idi->dbinfo.max_hostchecks_age > 0L)
			ndo2db_db_trim_data_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCHECKS], "start_time", (time_t) ((unsigned long) current_time - idi->dbinfo.max_hostchecks_age));
		if (idi->dbinfo.max_eventhandlers_age > 0L)
			ndo2db_db_trim_data_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_EVENTHANDLERS], "start_time", (time_t) ((unsigned long) current_time - idi->dbinfo.max_eventhandlers_age));
		if(idi->dbinfo.max_externalcommands_age>0L)
			ndo2db_db_trim_data_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_EXTERNALCOMMANDS],"entry_time",(time_t)((unsigned long)current_time - idi->dbinfo.max_externalcommands_age));
		idi->dbinfo.last_table_trim_time = current_time;
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_perform_maintenance() end\n");
	return NDO_OK;
}

/************************************/
/* check database driver (libdbi)   */
/************************************/

int ido2db_check_dbd_driver(void) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_check_dbd_driver() start\n");

	dbi_initialize(NULL);

	switch (ndo2db_db_settings.server_type) {
		case NDO2DB_DBSERVER_MYSQL:
			if ( (dbi_driver_open(IDO2DB_DBI_DRIVER_MYSQL)) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
			break;
		case NDO2DB_DBSERVER_PGSQL:
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_PGSQL) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
			break;
		case NDO2DB_DBSERVER_DB2:
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_DB2) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
			break;
		case NDO2DB_DBSERVER_FIREBIRD:
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_FIREBIRD) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
			break;
		case NDO2DB_DBSERVER_FREETDS:
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_FREETDS) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
			break;
		case NDO2DB_DBSERVER_INGRES:
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_INGRES) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
			break;
		case NDO2DB_DBSERVER_MSQL:
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_MSQL) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
			break;
		case NDO2DB_DBSERVER_ORACLE:

#ifndef USE_ORACLE /* everything else will be libdbi */
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_ORACLE) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
#else /* Oracle ocilib specific */
			/* unused */
#endif /* Oracle ocilib specific */

			break;
		case NDO2DB_DBSERVER_SQLITE:
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_SQLITE) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
			break;
		case NDO2DB_DBSERVER_SQLITE3:
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_SQLITE3) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
			break;
		default:
			dbi_shutdown();
			return NDO_FALSE;
			break;
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_check_dbd_driver() end\n");
	return NDO_TRUE;
}


/************************************/
/* error handler (ocilib)           */
/************************************/


#ifdef USE_ORACLE /* Oracle ocilib specific */

void ido2db_ocilib_err_handler(OCI_Error *err) {

	if (OCI_ErrorGetType(err) == OCI_ERR_ORACLE) {
		const mtext *sql = OCI_GetSql(OCI_ErrorGetStatement(err));

		if (sql != NULL) {
			syslog(LOG_USER | LOG_INFO, "ERROR: QUERY '%s'\n", sql);
			ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ERROR: QUERY '%s'\n", sql);
		}
	}

	syslog(LOG_USER | LOG_INFO, "ERROR: MSG '%s'\n", OCI_ErrorGetString(err));
	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ERROR: MSG '%s'\n", OCI_ErrorGetString(err));
}

/* use defined triggers/sequences to emulate last insert id in oracle */
unsigned long ido2db_ocilib_insert_id(ndo2db_idi *idi) {

	unsigned long insert_id = 0;

        OCI_ExecuteStmt(idi->dbinfo.oci_statement, MT("SELECT AUTOINCREMENT.CURRVAL FROM DUAL"));
	OCI_Commit(idi->dbinfo.oci_connection);

        /*  get result set */
        idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement);

	if(idi->dbinfo.oci_resultset == NULL) {


	} else {

		/* check if next row */
                if(OCI_FetchNext(idi->dbinfo.oci_resultset)) {
                	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_db_hello() nextrow ok\n");
                        insert_id = OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 1);
                }
	}


	return insert_id;
}


/****************************************************************************/
/* PREPARED STATEMENTS                                                      */
/****************************************************************************/

int ido2db_oci_prepared_statement_timedevents_queue(ndo2db_idi *idi) {

        char *buf = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() start\n");

        if(asprintf(&buf, "MERGE INTO %s USING DUAL ON (instance_id=:X1 AND event_type=:X2 AND scheduled_time=(SELECT unixts2date(:X5) FROM DUAL) AND object_id=:X6) WHEN MATCHED THEN UPDATE SET queued_time=(SELECT unixts2date(:X3) FROM DUAL), queued_time_usec=:X4, recurring_event=:X6 WHEN NOT MATCHED THEN INSERT (instance_id, event_type, queued_time, queued_time_usec, scheduled_time, recurring_event, object_id) VALUES (:X1, :X2, (SELECT unixts2date(:X3) FROM DUAL), :X4, (SELECT unixts2date(:X5) FROM DUAL), :X6, :X7)",
                ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS]) == -1) {
                        buf = NULL;
        }

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() query: %s\n", buf);

        /* bind to timedevents_queue statement */
	if(idi->dbinfo.oci_connection) {

		idi->dbinfo.oci_statement_timedevents_queue = OCI_StatementCreate(idi->dbinfo.oci_connection);

		/* allow rebinding values */
		OCI_AllowRebinding(idi->dbinfo.oci_statement_timedevents_queue, 1);
		
	        if(!OCI_Prepare(idi->dbinfo.oci_statement_timedevents_queue, MT(buf))) {
        	        free(buf);
                	return NDO_ERROR;
	        }
	} else {
		free(buf);
		return NDO_ERROR;
	}
        free(buf);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_oci_prepared_statement_timedevents_queue() end\n");

        return NDO_OK;
}


#endif /* Oracle ocilib specific */
