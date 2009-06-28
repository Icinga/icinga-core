/***************************************************************
 * DB.C - Datatabase routines for NDO2DB daemon
 *
 * Copyright (c) 2005-2007 Ethan Galstad
 * Copyright (c) 2009 Icinga Development Team (http://www.icinga.org)
 *
 * Last Modified: 07-12-2009
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
	"serviceescalation_contactgroups"
	};

char *ndo2db_db_tablenames[NDO2DB_MAX_DBTABLES];

/*
 #define DEBUG_NDO2DB_QUERIES 1
 */

/****************************************************************************/
/* CONNECTION FUNCTIONS                                                     */
/****************************************************************************/

/* initialize database structures */
int ndo2db_db_init(ndo2db_idi *idi) {
	register int x;

	if (idi == NULL)
		return NDO_ERROR;

	/* initialize db server type */
	idi->dbinfo.server_type = ndo2db_db_settings.server_type;

	/* initialize table names */
	for (x = 0; x < NDO2DB_MAX_DBTABLES; x++) {
		if ((ndo2db_db_tablenames[x] = (char *) malloc(strlen(ndo2db_db_rawtablenames[x]) + ((ndo2db_db_settings.dbprefix==NULL) ? 0 : strlen(ndo2db_db_settings.dbprefix)) + 1))==NULL)
			return NDO_ERROR;
		sprintf(ndo2db_db_tablenames[x], "%s%s", (ndo2db_db_settings.dbprefix==NULL) ? "" : ndo2db_db_settings.dbprefix,ndo2db_db_rawtablenames[x]);
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
	idi->dbinfo.last_table_trim_time = (time_t) 0L;
	idi->dbinfo.last_logentry_time = (time_t) 0L;
	idi->dbinfo.last_logentry_data = NULL;
	idi->dbinfo.object_hashlist = NULL;

	/* initialize db structures, etc. */
	if (dbi_initialize(NULL) == -1) {
		syslog(LOG_USER | LOG_INFO, "Error: dbi_initialize() failed\n");
		return NDO_ERROR;
	}

	return NDO_OK;
}

/* clean up database structures */
int ndo2db_db_deinit(ndo2db_idi *idi) {
	register int x;

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

	return NDO_OK;
}

/* connects to the database server */
int ndo2db_db_connect(ndo2db_idi *idi) {
	int result = NDO_OK;
	const char *dbi_error;

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
		idi->dbinfo.dbi_conn = dbi_conn_new(IDO2DB_DBI_DRIVER_ORACLE);
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
		syslog(LOG_USER | LOG_INFO,
				"Successfully connected to database");
	}

	return result;
}

/* disconnects from the database server */
int ndo2db_db_disconnect(ndo2db_idi *idi) {

	if (idi == NULL)
		return NDO_ERROR;

	/* we're not connected... */
	if (idi->dbinfo.connected == NDO_FALSE)
		return NDO_OK;

	dbi_conn_close(idi->dbinfo.dbi_conn);
	dbi_shutdown();

	syslog(LOG_USER | LOG_INFO, "Successfully disconnected from database");

	return NDO_OK;
}

/* post-connect routines */
int ndo2db_db_hello(ndo2db_idi *idi) {
	char *buf = NULL;
	char *ts = NULL;
	int result = NDO_OK;
	int have_instance = NDO_FALSE;
	time_t current_time;

	/* make sure we have an instance name */
	if (idi->instance_name == NULL)
		idi->instance_name = strdup("default");

	/* get existing instance */
	if (asprintf(&buf, "SELECT instance_id FROM %s WHERE instance_name='%s'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_INSTANCES], idi->instance_name)
			== -1)
		buf = NULL;

	if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {
			if (idi->dbinfo.dbi_result != NULL) {
				if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
					idi->dbinfo.instance_id = dbi_result_get_uint(
							idi->dbinfo.dbi_result, "instance_id");
					have_instance = NDO_TRUE;
				}
			}
	}
	else {
		/* cleanup the socket */
		ndo2db_cleanup_socket();

		/* free memory */
		ndo2db_free_program_memory();

		_exit(0);
	}
	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	/* insert new instance if necessary */
	if (have_instance == NDO_FALSE) {
		if (asprintf(&buf, "INSERT INTO %s (instance_name) VALUES ('%s')",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_INSTANCES],
				idi->instance_name) == -1)
			buf = NULL;
		if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {
			idi->dbinfo.instance_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		}
		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	ts = ndo2db_db_timet_to_sql(idi, idi->data_start_time);

	/* record initial connection information */
	if (asprintf(
			&buf,
			"INSERT INTO %s "
			"(instance_id, connect_time, last_checkin_time, bytes_processed, lines_processed, entries_processed, agent_name, agent_version, disposition, connect_source, connect_type, data_start_time) "
			"VALUES ('%lu', NOW(), NOW(), '0', '0', '0', '%s', '%s', '%s', '%s', '%s', NOW())",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO],
			idi->dbinfo.instance_id, idi->agent_name, idi->agent_version,
			idi->disposition, idi->connect_source, idi->connect_type) == -1)
		buf = NULL;
	if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {
		/* This might be 0 (zero) in some cases */
		idi->dbinfo.conninfo_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);

		/* ToDo: 	Check if dbinfo.conninfo_id is zero and find another way to get the
		 * 				last inserted ID
		 */
	}

	dbi_result_free(idi->dbinfo.dbi_result);
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

	return result;
}

/* pre-disconnect routines */
int ndo2db_db_goodbye(ndo2db_idi *idi) {
	int result = NDO_OK;
	char *buf = NULL;
	char *ts = NULL;

	ts = ndo2db_db_timet_to_sql(idi, idi->data_end_time);

	/* record last connection information */
	if (asprintf(&buf, "UPDATE %s SET "
		"disconnect_time=NOW(), "
		"last_checkin_time=NOW(), "
		"data_end_time=%s, "
		"bytes_processed='%lu', "
		"lines_processed='%lu', "
		"entries_processed='%lu' "
		"WHERE conninfo_id='%lu'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO], ts,
			idi->bytes_processed, idi->lines_processed, idi->entries_processed,
			idi->dbinfo.conninfo_id) == -1)
		buf = NULL;
	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	free(ts);

	return result;
}

/* checking routines */
int ndo2db_db_checkin(ndo2db_idi *idi) {
	int result = NDO_OK;
	char *buf = NULL;

	/* record last connection information */
	if (asprintf(
			&buf,
			"UPDATE %s SET last_checkin_time=NOW(), bytes_processed='%lu', lines_processed='%lu', entries_processed='%lu' WHERE conninfo_id='%lu'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO],
			idi->bytes_processed, idi->lines_processed, idi->entries_processed,
			idi->dbinfo.conninfo_id) == -1)
		buf = NULL;
	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	time(&ndo2db_db_last_checkin_time);

	return result;
}

/****************************************************************************/
/* MISC FUNCTIONS                                                           */
/****************************************************************************/

/* escape a string for a SQL statement */
char *ndo2db_db_escape_string(ndo2db_idi *idi, char *buf) {
	register int x, y, z;
	char *newbuf = NULL;

	if (idi == NULL || buf == NULL)
		return NULL;

	z = strlen(buf);

	/* allocate space for the new string */
	if ((newbuf = (char *) malloc((z * 2) + 1)) == NULL)
		return NULL;

	/* escape characters */
	for (x = 0, y = 0; x < z; x++) {

		if (idi->dbinfo.server_type == NDO2DB_DBSERVER_MYSQL) {
			if (buf[x] == '\'' || buf[x] == '\"' || buf[x] == '*' || buf[x]
					== '\\' || buf[x] == '$' || buf[x] == '?' || buf[x] == '.'
					|| buf[x] == '^' || buf[x] == '+' || buf[x] == '['
					|| buf[x] == ']' || buf[x] == '(' || buf[x] == ')')
				newbuf[y++] = '\\';
		} else if (idi->dbinfo.server_type == NDO2DB_DBSERVER_PGSQL) {
			if (!(isspace(buf[x]) || isalnum(buf[x]) || (buf[x] == '_')))
				newbuf[y++] = '\\';
		}

		newbuf[y++] = buf[x];
	}

	/* terminate escape string */
	newbuf[y] = '\0';

	return newbuf;
}

/* SQL query conversion of time_t format to date/time format */
char *ndo2db_db_timet_to_sql(ndo2db_idi *idi, time_t t) {
	char *buf = NULL;

	asprintf(&buf, "FROM_UNIXTIME(%lu)", (unsigned long) t);

	return buf;
}

/* SQL query conversion of date/time format to time_t format */
char *ndo2db_db_sql_to_timet(ndo2db_idi *idi, char *field) {
	char *buf = NULL;

	asprintf(&buf, "UNIX_TIMESTAMP(%s)", (field == NULL) ? "" : field);

	return buf;
}

/* executes a SQL statement */
int ndo2db_db_query(ndo2db_idi *idi, char *buf) {
	int result = NDO_OK;
	const char *error_msg;

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

	idi->dbinfo.dbi_result = dbi_conn_query(idi->dbinfo.dbi_conn, buf);

	if (idi->dbinfo.dbi_result == NULL){
		dbi_conn_error(idi->dbinfo.dbi_conn, &error_msg);

		syslog(LOG_USER | LOG_INFO, "Error: database query failed for '%s' - '%s'\n", buf, error_msg);

		ndo2db_handle_db_error(idi);
		result = NDO_ERROR;
	}

	return result;
}

/* frees memory associated with a query */
int ndo2db_db_free_query(ndo2db_idi *idi) {

	if (idi == NULL)
		return NDO_ERROR;

	/* ToDo: Examine where this function is called
	 * Not freeing the query may result in memory leaks
	 **/

	return NDO_OK;
}

/* handles SQL query errors */
int ndo2db_handle_db_error(ndo2db_idi *idi) {

	if (idi == NULL)
		return NDO_ERROR;

	/* we're not currently connected... */
	if (idi->dbinfo.connected == NDO_FALSE)
		return NDO_OK;

	ndo2db_db_disconnect(idi);

	idi->disconnect_client = NDO_TRUE;

	return NDO_OK;
}

/* clears data from a given table (current instance only) */
int ndo2db_db_clear_table(ndo2db_idi *idi, char *table_name) {
	char *buf = NULL;
	int result = NDO_OK;

	if (idi == NULL || table_name == NULL)
		return NDO_ERROR;

	if (asprintf(&buf, "DELETE FROM %s WHERE instance_id='%lu'", table_name, idi->dbinfo.instance_id) == -1)
		buf = NULL;

	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	return result;
}

/* gets latest data time value from a given table */
int ndo2db_db_get_latest_data_time(ndo2db_idi *idi, char *table_name, char *field_name, unsigned long *t) {
	char *buf = NULL;
	char *ts[1];
	int result = NDO_OK;

	if (idi == NULL || table_name == NULL || field_name == NULL || t == NULL)
		return NDO_ERROR;

	*t = (time_t) 0L;
	ts[0] = ndo2db_db_sql_to_timet(idi, field_name);

	if (asprintf(
			&buf,
			"SELECT %s AS latest_time FROM %s WHERE instance_id='%lu' ORDER BY %s DESC LIMIT 1 OFFSET 0",
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

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);
	free(ts[0]);

	return result;
}

/* trim/delete old data from a given table */
int ndo2db_db_trim_data_table(ndo2db_idi *idi, char *table_name, char *field_name, unsigned long t) {
	char *buf = NULL;
	char *ts[1];
	int result = NDO_OK;

	if (idi == NULL || table_name == NULL || field_name == NULL)
		return NDO_ERROR;

	ts[0] = ndo2db_db_timet_to_sql(idi, (time_t) t);

	if (asprintf(&buf, "DELETE FROM %s WHERE instance_id='%lu' AND %s<%s",
			table_name, idi->dbinfo.instance_id, field_name, ts[0]) == -1)
		buf = NULL;

	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);
	free(ts[0]);

	return result;
}

/* performs some periodic table maintenance... */
int ndo2db_db_perform_maintenance(ndo2db_idi *idi) {
	time_t current_time;

	/* get the current time */
	time(&current_time);

	/* trim tables */
	if (((unsigned long) current_time - 60) > (unsigned long) idi->dbinfo.last_table_trim_time) {
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
		idi->dbinfo.last_table_trim_time = current_time;
	}

	return NDO_OK;
}

int ido2db_check_dbd_driver(void) {

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
			if (dbi_driver_open(IDO2DB_DBI_DRIVER_ORACLE) == NULL){
				dbi_shutdown();
				return NDO_FALSE;
				}
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
			break;
	}

	return NDO_TRUE;
}
