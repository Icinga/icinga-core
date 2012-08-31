/***************************************************************
 * SLA.C - Data Query handler routines for IDO2DB daemon
 *
 * Copyright (c) 2011 Icinga Development Team (http://www.icinga.org)
 *
 **************************************************************/

#include <stddef.h>

/* include our project's header files */
#include "../../../include/config.h"
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ido2db.h"
#include "../include/db.h"
#include "../include/sla.h"
#include "../include/logging.h"

/* Icinga header files */
#include "../../../include/icinga.h"
#include "../../../include/broker.h"
#include "../../../include/comments.h"

extern int enable_sla;

/**
 * Allocates and initializes a new SLA state history entry.
 *
 * @param instance_id the Icinga instance ID for this state entry
 * @param object_id the service/host this state entry belongs to
 * @return a new SLA state history entry
 */
sla_state_t *sla_alloc_state(unsigned long instance_id,
                             unsigned long object_id) {
	sla_state_t *state;

	state = (sla_state_t *)malloc(sizeof(sla_state_t));

	if (state == NULL)
		return NULL;

	memset(state, 0, sizeof(sla_state_t));

	state->instance_id = instance_id;
	state->start_time = time(NULL);
	state->object_id = object_id;

	return state;
}

/**
 * Frees a state history entry.
 *
 * @param state the state history entry
 */
void sla_free_state(sla_state_t *state) {
	free(state);
}

/**
 * Reallocates and initializes a list of SLA state history entries.
 * @param ptr the old SLA state list
 * @param count the number of state history entries to allocate
 * @return a list of SLA state history entries, NULL on failure
 */
sla_state_list_t *sla_realloc_state_list(sla_state_list_t *ptr,
        unsigned int count) {
	sla_state_list_t *list;
	int changed_count, changed_offset;

	list = (sla_state_list_t *)realloc(ptr,
	                                   offsetof(sla_state_list_t, states) +
	                                   count * sizeof(sla_state_t));

	if (list == NULL)
		return NULL;

	if (ptr != NULL) {
		changed_count =
		    (count > list->count) ? count - list->count : 0;
		changed_offset = list->count - changed_count + 1;
	} else {
		changed_count = count;
		changed_offset = 0;
	}

	list->count = count;
	memset(&(list->states[changed_offset]), 0, changed_count *
	       sizeof(sla_state_t));

	return list;
}

/**
 * Allocates and initializes a list of SLA state history entries.
 * @param count the number of state history entries to allocate
 * @return a list of SLA state history entries, NULL on failure
 */
sla_state_list_t *sla_alloc_state_list(unsigned int count) {
	return sla_realloc_state_list(NULL, count);
}

/**
 * Frees a SLA state history list
 * @param list the list that is to be freed
 */
void sla_free_state_list(sla_state_list_t *list) {
	free(list);
}

/**
 * Queries the database for SLA state history entries for the specified
 * host/service and start/end time.
 *
 * @param idi the IDO database connection
 * @param instance_id the Icinga instance ID for the state entries
 * @param object_id the host/service the state entries belong to
 * @param start_time the start time of the search interval
 * @param end_time the end time of the search interval
 * @param states an SLA state list, set to NULL on failure
 * @return 0 on success, negative value on failure
 */
int sla_query_states(ido2db_idi *idi, unsigned long object_id,
                     time_t start_time, time_t end_time, sla_state_list_t **list) {
	int count, i;
	sla_state_t *state;
#ifndef USE_ORACLE
	char *start_time_str, *end_time_str;
	char *start_time_sql, *end_time_sql, *ack_time_sql;
	char *query;
	int rc;
#else /* !USE_ORACLE */
	uint data[4];
#endif /* !USE_ORACLE */

	if (idi == NULL || idi->dbinfo.connected == IDO_FALSE ||
	        list == NULL)
		return -1;

#ifdef USE_LIBDBI
	start_time_str = ido2db_db_timet_to_sql(idi, start_time);

	if (start_time_str == NULL)
		return -1;

	end_time_str = ido2db_db_timet_to_sql(idi, end_time);

	if (end_time_str == NULL) {
		free(start_time_str);
		return -1;
	}

	start_time_sql = ido2db_db_sql_to_timet(idi,
	                                        "start_time");

	if (start_time_sql == NULL) {
		free(start_time_str);
		free(end_time_str);
		return -1;
	}

	end_time_sql = ido2db_db_sql_to_timet(idi,
	                                      "end_time");

	if (end_time_sql == NULL) {
		free(start_time_str);
		free(end_time_str);
		free(start_time_sql);
		return -1;
	}

	ack_time_sql = ido2db_db_sql_to_timet(idi,
	                                      "acknowledgement_time");

	if (ack_time_sql == NULL) {
		free(start_time_str);
		free(end_time_str);
		free(start_time_sql);
		free(end_time_sql);
		return -1;
	}

	rc = asprintf(&query, "SELECT slahistory_id, "
	              "%s AS start_time, %s AS end_time, %s AS acknowledgement_time, "
	              "state, state_type, scheduled_downtime "
	              "FROM %s "
	              "WHERE instance_id = '%lu' AND object_id = '%lu' AND "
	              "((start_time > %s AND start_time < %s) OR "
	              " (end_time > %s AND end_time < %s) OR "
	              " (start_time < %s AND end_time > %s) OR "
	              " (end_time IS NULL))",
	              start_time_sql, end_time_sql, ack_time_sql,
	              ido2db_db_tablenames[IDO2DB_DBTABLE_SLAHISTORY],
	              idi->dbinfo.instance_id, object_id,
	              start_time_str, end_time_str,
	              start_time_str, end_time_str,
	              start_time_str, end_time_str);

	free(start_time_str);
	free(end_time_str);
	free(start_time_sql);
	free(end_time_sql);
	free(ack_time_sql);

	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "sla_query_states(): %s\n",
	                      query);

	rc = ido2db_db_query(idi, query);

	free(query);

	if (rc != IDO_OK || idi->dbinfo.dbi_result == NULL) {
		*list = NULL;
		return -1;
	}

	count = dbi_result_get_numrows(idi->dbinfo.dbi_result);
	*list = sla_alloc_state_list(count);

	if (*list == NULL) {
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		return -1;
	}

	for (i = 0; i < count; i++) {
		if (!dbi_result_next_row(idi->dbinfo.dbi_result)) {
			sla_free_state_list(*list);
			*list = NULL;
			return -1;
		}

		state = &((*list)->states[i]);

		state->persistent = IDO_TRUE;
		state->instance_id = idi->dbinfo.instance_id;
		state->object_id = object_id;

		state->slahistory_id = dbi_result_get_ulonglong(
		                           idi->dbinfo.dbi_result, "slahistory_id");
		state->start_time = (time_t)dbi_result_get_ulonglong(
		                        idi->dbinfo.dbi_result, "start_time");
		state->end_time = (time_t)dbi_result_get_ulonglong(
		                      idi->dbinfo.dbi_result, "end_time");
		state->acknowledgement_time = (time_t)dbi_result_get_ulonglong(
		                                  idi->dbinfo.dbi_result, "acknowledgement_time");
		state->state = dbi_result_get_int(idi->dbinfo.dbi_result,
		                                  "state");
		state->state_type = dbi_result_get_int(idi->dbinfo.dbi_result,
		                                       "state_type");
		state->scheduled_downtime = (int)dbi_result_get_int(
		                                idi->dbinfo.dbi_result, "scheduled_downtime");
	}

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif /* USE_LIBDBI */

#ifdef USE_PGSQL

#endif /* USE_PGSQL */

#ifdef USE_ORACLE
	data[0] = idi->dbinfo.instance_id;
	data[1] = object_id;
	data[2] = start_time;
	data[3] = end_time;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_select,
	                         MT(":X1"), (uint *) & (data[0]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_select,
	                         MT(":X2"), (uint *) & (data[1]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_select,
	                         MT(":X3"), (uint *) & (data[2]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_select,
	                         MT(":X4"), (uint *) & (data[3]))) {
		return -1;
	}

	if (!OCI_Execute(idi->dbinfo.oci_statement_sla_history_select)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
		                      "sla_query_states() execute error\n");
		return -1;
	}

	OCI_Commit(idi->dbinfo.oci_connection);

	idi->dbinfo.oci_resultset = OCI_GetResultset(
	                                idi->dbinfo.oci_statement_sla_history_select);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                      "sla_query_states() query ok\n");

	count = OCI_GetRowCount(idi->dbinfo.oci_resultset);
	*list = sla_alloc_state_list(count);

	if (*list == NULL)
		return -1;

	for (i = 0; i < count; i++) {
		if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
			                      "sla_query_states() fetchnext not ok\n");

			sla_free_state_list(*list);
			*list = NULL;
			return -1;
		}

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
		                      "sla_query_states() fetchnext ok\n");

		state = &((*list)->states[i]);

		state->persistent = IDO_TRUE;
		state->instance_id = idi->dbinfo.instance_id;
		state->object_id = object_id;

		state->slahistory_id = OCI_GetUnsignedInt(
		                           idi->dbinfo.oci_resultset, 1);
		state->start_time = OCI_GetUnsignedInt(
		                        idi->dbinfo.oci_resultset, 2);
		state->end_time = OCI_GetUnsignedInt(idi->dbinfo.oci_resultset,
		                                     3);
		state->acknowledgement_time = OCI_GetUnsignedInt(
		                                  idi->dbinfo.oci_resultset, 4);
		state->state = OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 5);
		state->state_type = OCI_GetUnsignedInt(
		                        idi->dbinfo.oci_resultset, 6);
		state->scheduled_downtime = OCI_GetUnsignedInt(
		                                idi->dbinfo.oci_resultset, 7);
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                      "sla_query_states(instance_id=%lu,"
	                      " object_id=%llu, start_time=%llu"
	                      ", end_time=%llu)\n", idi->dbinfo.instance_id,
	                      object_id, start_time, end_time);
#endif /* USE_ORACLE */

	return 0;
}

/**
 * Saves an SLA state history entry.
 *
 * @param idi the IDO database connection
 * @param state the state history entry that is to be saved
 * @return 0 on success, negative value on failure
 */
int sla_save_state(ido2db_idi *idi, sla_state_t *state) {
#ifndef USE_ORACLE
	char *query;
	int rc;
	char *start_time_str = NULL, *end_time_str = NULL, *ack_time_str = NULL;
#else /* !USE_ORACLE */
	uint data[9];
#endif /* !USE_ORACLE */

	if (idi == NULL || state == NULL || idi->dbinfo.connected == IDO_FALSE)
		return -1;

#ifdef USE_LIBDBI
	if (state->start_time != 0) {
		start_time_str = ido2db_db_timet_to_sql(idi, state->start_time);

		if (start_time_str == NULL)
			return -1;
	}

	if (state->end_time != 0) {
		end_time_str = ido2db_db_timet_to_sql(idi, state->end_time);

		if (end_time_str == NULL) {
			free(start_time_str);

			return -1;
		}
	}

	if (state->acknowledgement_time != 0) {
		ack_time_str = ido2db_db_timet_to_sql(idi,
		                                      state->acknowledgement_time);

		if (ack_time_str == NULL) {
			free(start_time_str);
			free(end_time_str);

			return -1;
		}
	}

	if (state->persistent) {
		rc = asprintf(&query, "UPDATE %s "
		              "SET start_time = %s, "
		              "end_time = %s, "
		              "acknowledgement_time = %s, "
		              "state = %d, state_type = %d, "
		              "scheduled_downtime = %d "
		              "WHERE slahistory_id = '%lu'",
		              ido2db_db_tablenames[IDO2DB_DBTABLE_SLAHISTORY],
		              (start_time_str != NULL) ? start_time_str : "NULL",
		              (end_time_str != NULL) ? end_time_str : "NULL",
		              (ack_time_str != NULL) ? ack_time_str : "NULL",
		              state->state, state->state_type,
		              (int)state->scheduled_downtime,
		              state->slahistory_id);

		free(start_time_str);
		free(end_time_str);
		free(ack_time_str);

		if (rc < 0)
			return -1;
	} else {
		rc = asprintf(&query, "INSERT INTO %s "
		              "(instance_id, "
		              " start_time, "
		              " end_time, "
		              " acknowledgement_time, "
		              " object_id, state, "
		              " state_type, scheduled_downtime) "
		              "VALUES "
		              "('%lu', %s, %s, "
		              " %s, '%lu', '%d', "
		              " '%d', '%d')",
		              ido2db_db_tablenames[IDO2DB_DBTABLE_SLAHISTORY],
		              state->instance_id,
		              (start_time_str != NULL) ? start_time_str : "NULL",
		              (end_time_str != NULL) ? end_time_str : "NULL",
		              (ack_time_str != NULL) ? ack_time_str : "NULL",
		              state->object_id, state->state,
		              state->state_type, (int)state->scheduled_downtime);

		free(start_time_str);
		free(end_time_str);
		free(ack_time_str);

		if (rc < 0)
			return -1;
	}

	rc = ido2db_db_query(idi, query);

	free(query);

	if (rc != IDO_OK)
		return -1;

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif /* USE_LIBDBI */

#ifdef USE_PGSQL

#endif /* USE_PGSQL */

#ifdef USE_ORACLE
	if (!state->persistent)
		state->slahistory_id = 0;

	data[0] = state->slahistory_id;
	data[1] = state->instance_id;
	data[2] = state->start_time;
	data[3] = state->end_time;
	data[4] = state->acknowledgement_time;
	data[5] = state->object_id;
	data[6] = state->state;
	data[7] = state->state_type;
	data[8] = state->scheduled_downtime;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_merge,
	                         MT(":X1"), (uint *) & (data[0]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_merge,
	                         MT(":X2"), (uint *) & (data[1]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_merge,
	                         MT(":X3"), (uint *) & (data[2]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_merge,
	                         MT(":X4"), (uint *) & (data[3]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_merge,
	                         MT(":X5"), (uint *) & (data[4]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_merge,
	                         MT(":X6"), (uint *) & (data[5]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_merge,
	                         MT(":X7"), (uint *) & (data[6]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_merge,
	                         MT(":X8"), (uint *) & (data[7]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_merge,
	                         MT(":X9"), (uint *) & (data[8]))) {
		return -1;
	}

	if (!OCI_Execute(idi->dbinfo.oci_statement_sla_history_merge)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
		                      "sla_save_state() execute error\n");
		return -1;
	}

	OCI_Commit(idi->dbinfo.oci_connection);
#endif /* USE_ORACLE */

	/* TODO: insert/update DB */
	state->persistent = IDO_TRUE;

	return 0;
}

/**
 * Deletes an SLA state history entry from the database. Note that the passed
 * struct still needs to be freed by the caller.
 *
 * @param idi the IDO database connection
 * @param state the state history entry that is to be deleted
 * @return 0 on success, negative value on failure
 */
int sla_delete_state(ido2db_idi *idi, sla_state_t *state) {
#ifndef USE_ORACLE
	char *query;
	int rc;
#else /* !USE_ORACLE */
	uint data[1];
#endif /* !USE_ORACLE */

	if (idi == NULL || state == NULL || idi->dbinfo.connected == IDO_FALSE)
		return -1;

	/* deleting a non-persistent entry is a no-op */
	if (!state->persistent)
		return 0;

#ifdef USE_LIBDBI
	rc = asprintf(&query, "DELETE FROM %s WHERE slahistory_id = '%lu'",
	              ido2db_db_tablenames[IDO2DB_DBTABLE_SLAHISTORY],
	              state->slahistory_id);

	if (rc < 0)
		return -1;

	rc = ido2db_db_query(idi, query);

	free(query);

	if (rc != IDO_OK)
		return -1;

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif /* USE_LIBDBI */

#ifdef USE_PGSQL /* USE_PGSQL */

#endif

#ifdef USE_ORACLE
	data[0] = state->slahistory_id;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_history_delete,
	                         MT(":X1"), (uint *) & (data[0]))) {
		return -1;
	}

	if (!OCI_Execute(idi->dbinfo.oci_statement_sla_history_delete)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
		                      "sla_delete_state() execute error\n");
		return -1;
	}

	OCI_Commit(idi->dbinfo.oci_connection);
#endif /* USE_ORACLE */

	state->persistent = IDO_FALSE;

	return 0;
}

/**
 * Allocates and initializes a new downtime entry.
 *
 * @param instance_id the Icinga instance ID for this downtime entry
 * @param object_id the service/host this downtime entry belongs to
 * @return a new SLA state history entry
 */
sla_downtime_t *sla_alloc_downtime(unsigned long instance_id,
                                   unsigned long object_id) {
	sla_downtime_t *downtime;

	downtime = (sla_downtime_t *)malloc(sizeof(sla_downtime_t));

	if (downtime == NULL)
		return NULL;

	memset(downtime, 0, sizeof(sla_downtime_t));

	downtime->instance_id = instance_id;
	downtime->object_id = object_id;

	return downtime;
}

/**
 * Frees a downtime entry.
 *
 * @param state the downtime entry
 */
void sla_free_downtime(sla_downtime_t *downtime) {
	free(downtime);
}

/**
 * Allocates and initializes a list of downtime entries.
 * @param count the number of downtime entries to allocate
 * @return a list of downtime entries, NULL on failure
 */
sla_downtime_list_t *sla_alloc_downtime_list(unsigned int count) {
	sla_downtime_list_t *list;

	list = (sla_downtime_list_t *)malloc(
	           offsetof(sla_downtime_list_t, downtimes) +
	           count * sizeof(sla_downtime_t));

	if (list == NULL)
		return NULL;

	list->count = count;
	memset(&(list->downtimes), 0, count * sizeof(sla_downtime_t));

	return list;
}

/**
 * Frees a downtime list.
 * @param list the list that is to be freed
 */
void sla_free_downtime_list(sla_downtime_list_t *list) {
	free(list);
}

/**
 * Checks whether there was scheduled downtime for the specified host/service.
 *
 * @param idi the IDO database connection
 * @param instance_id the Icinga instance ID for the state entries
 * @param object_id the host/service the state entries belong to
 * @param timestamp the timestamp to check
 * @return 1 if the host/service had scheduled downtime, 0 otherwise
 */
int sla_query_downtime(ido2db_idi *idi, unsigned long object_id,
                       time_t start_time, time_t end_time, sla_downtime_list_t **list) {
	int count, i;
	sla_downtime_t *downtime;
#ifndef USE_ORACLE
	char *start_time_str, *end_time_str;
	char *scheduled_start_sql, *scheduled_end_sql;
	char *actual_start_sql, *actual_end_sql;
	char *query;
	int rc;
#else /* USE_ORACLE */
	uint data[4];
#endif /* USE_ORACLE */

	if (idi == NULL || idi->dbinfo.connected == IDO_FALSE ||
	        list == NULL)
		return -1;

#ifdef USE_LIBDBI
	start_time_str = ido2db_db_timet_to_sql(idi, start_time);

	if (start_time_str == NULL)
		return -1;

	end_time_str = ido2db_db_timet_to_sql(idi, end_time);

	if (end_time_str == NULL) {
		free(start_time_str);
		return -1;
	}

	scheduled_start_sql = ido2db_db_sql_to_timet(idi,
	                      "scheduled_start_time");

	if (scheduled_start_sql == NULL) {
		free(start_time_str);
		free(end_time_str);
		return -1;
	}

	scheduled_end_sql = ido2db_db_sql_to_timet(idi,
	                    "scheduled_end_time");

	if (scheduled_end_sql == NULL) {
		free(start_time_str);
		free(end_time_str);
		free(scheduled_start_sql);
		return -1;
	}

	actual_start_sql = ido2db_db_sql_to_timet(idi,
	                   "actual_start_time");

	if (actual_start_sql == NULL) {
		free(start_time_str);
		free(end_time_str);
		free(scheduled_start_sql);
		free(scheduled_end_sql);
		return -1;
	}

	actual_end_sql = ido2db_db_sql_to_timet(idi,
	                                        "actual_end_time");

	if (actual_end_sql == NULL) {
		free(start_time_str);
		free(end_time_str);
		free(scheduled_start_sql);
		free(scheduled_end_sql);
		free(actual_start_sql);
		return -1;
	}

	rc = asprintf(&query, "SELECT downtimehistory_id, "
	              "%s AS actual_start_time, %s AS actual_end_time, "
	              "%s AS scheduled_start_time, %s AS scheduled_end_time, "
	              "is_fixed, duration "
	              "FROM %s "
	              "WHERE instance_id = '%lu' AND object_id = '%lu' AND "
	              "((actual_start_time > %s AND actual_start_time < %s) OR "
	              " (actual_end_time > %s AND actual_end_time < %s) OR "
	              " (actual_start_time < %s AND actual_end_time > %s) OR "
	              " (actual_end_time IS NULL))",
	              actual_start_sql, actual_end_sql,
	              scheduled_start_sql, scheduled_end_sql,
	              ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY],
	              idi->dbinfo.instance_id, object_id,
	              start_time_str, end_time_str,
	              start_time_str, end_time_str,
	              start_time_str, end_time_str);

	free(start_time_str);
	free(end_time_str);
	free(actual_start_sql);
	free(actual_end_sql);
	free(scheduled_start_sql);
	free(scheduled_end_sql);

	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2,
	                      "sla_query_downtime(): %s\n", query);

	rc = ido2db_db_query(idi, query);

	free(query);

	if (rc != IDO_OK || idi->dbinfo.dbi_result == NULL) {
		*list = NULL;
		return -1;
	}

	count = dbi_result_get_numrows(idi->dbinfo.dbi_result);
	*list = sla_alloc_downtime_list(count);

	if (*list == NULL) {
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		return -1;
	}

	for (i = 0; i < count; i++) {
		if (!dbi_result_next_row(idi->dbinfo.dbi_result)) {
			sla_free_downtime_list(*list);
			*list = NULL;
			return -1;
		}

		downtime = &((*list)->downtimes[i]);

		downtime->instance_id = idi->dbinfo.instance_id;
		downtime->object_id = object_id;

		downtime->downtimehistory_id = dbi_result_get_ulonglong(
		                                   idi->dbinfo.dbi_result, "downtimehistory_id");
		downtime->is_fixed = dbi_result_get_int(
		                         idi->dbinfo.dbi_result, "is_fixed");
		downtime->duration = dbi_result_get_int(
		                         idi->dbinfo.dbi_result, "duration");
		downtime->actual_start_time = (time_t)dbi_result_get_ulonglong(
		                                  idi->dbinfo.dbi_result, "actual_start_time");
		downtime->actual_end_time = (time_t)dbi_result_get_ulonglong(
		                                idi->dbinfo.dbi_result, "actual_end_time");
		downtime->scheduled_start_time = (time_t)dbi_result_get_ulonglong(
		                                     idi->dbinfo.dbi_result, "scheduled_start_time");
		downtime->scheduled_end_time = (time_t)dbi_result_get_ulonglong(
		                                   idi->dbinfo.dbi_result, "scheduled_end_time");
	}

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif /* USE_LIBDBI */

#ifdef USE_PGSQL

#endif /* USE_PGSQL */

#ifdef USE_ORACLE
	data[0] = idi->dbinfo.instance_id;
	data[1] = object_id;
	data[2] = start_time;
	data[3] = end_time;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_downtime_select,
	                         MT(":X1"), (uint *) & (data[0]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_downtime_select,
	                         MT(":X2"), (uint *) & (data[1]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_downtime_select,
	                         MT(":X3"), (uint *) & (data[2]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_downtime_select,
	                         MT(":X4"), (uint *) & (data[3]))) {
		return -1;
	}

	if (!OCI_Execute(idi->dbinfo.oci_statement_sla_downtime_select)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
		                      "sla_query_downtime() execute error\n");
		return -1;
	}

	OCI_Commit(idi->dbinfo.oci_connection);

	idi->dbinfo.oci_resultset = OCI_GetResultset(
	                                idi->dbinfo.oci_statement_sla_downtime_select);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                      "sla_query_downtime() query ok\n");

	count = OCI_GetRowCount(idi->dbinfo.oci_resultset);
	*list = sla_alloc_downtime_list(count);

	if (*list == NULL)
		return -1;

	for (i = 0; i < count; i++) {
		if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
			                      "sla_query_downtime() fetchnext not ok\n");

			sla_free_downtime_list(*list);
			*list = NULL;
			return -1;
		}

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
		                      "sla_query_downtime() fetchnext ok\n");

		downtime = &((*list)->downtimes[i]);

		downtime->instance_id = idi->dbinfo.instance_id;
		downtime->object_id = object_id;

		downtime->downtimehistory_id =
		    OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 1);
		downtime->actual_start_time =
		    OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 2);
		downtime->actual_end_time =
		    OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 3);
		downtime->scheduled_start_time =
		    OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 4);
		downtime->scheduled_end_time =
		    OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 5);
		downtime->is_fixed =
		    OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 6);
		downtime->duration =
		    OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 7);
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                      "sla_query_downtime(instance_id=%lu, object_id=%llu,"
	                      " start_time=%llu, end_time=%llu)\n", idi->dbinfo.instance_id,
	                      object_id, start_time, end_time);
#endif /* USE_ORACLE */

	return 0;
}

/**
 * Splits the specified states into multiple states in order to update
 * their 'scheduled_downtime' attribute.
 *
 * @param state_list
 * @param downtime_list
 * @return 0 on success, negative value on failure
 */
int sla_apply_downtime(ido2db_idi *idi, sla_state_list_t **state_list_ptr,
                       sla_downtime_list_t *downtime_list) {
	sla_state_list_t *state_list = *state_list_ptr;
	sla_state_t *state, *clone_state;
	sla_downtime_t *downtime;
	int i, k;
	time_t dt_start_time, dt_end_time;

	for (i = 0; i < state_list->count; i++) {
		state = &(state_list->states[i]);

		/* ignore states that aren't over yet */
		if (state->end_time == 0)
			continue;

		for (k = 0; k < downtime_list->count; k++) {
			downtime = &(downtime_list->downtimes[k]);

			/*
			 * Ignore downtime that hasn't started yet or
			 * isn't over already.
			 */
			if (downtime->actual_start_time == 0 ||
			        downtime->actual_end_time == 0) {
				continue;
			}

			if (downtime->is_fixed) {
				dt_start_time = downtime->scheduled_start_time;
				dt_end_time = downtime->scheduled_end_time;
			} else {
				dt_start_time = downtime->actual_start_time;
				dt_end_time = dt_start_time +
				              downtime->duration;
			}

			/*
			 * is the downtime outside of the state's
			 * time interval
			 */
			if (dt_start_time > state->end_time ||
			        dt_end_time < state->start_time) {
				continue;
			}

			/* is the state fully contained in the downtime? */
			if (dt_start_time <= state->start_time &&
			        dt_end_time >= state->end_time) {
				state->scheduled_downtime = IDO_TRUE;
				sla_save_state(idi, state);

				continue;
			}

			state_list = sla_realloc_state_list(state_list,
			                                    state_list->count + 1);

			if (state_list == NULL)
				return -1;

			*state_list_ptr = state_list;
			state = &(state_list->states[i]);

			clone_state =
			    &(state_list->states[state_list->count - 1]);

			memcpy(clone_state, state, sizeof(sla_state_t));
			clone_state->slahistory_id = 0;
			clone_state->persistent = IDO_FALSE;

			if (state->start_time <= dt_start_time) {
				state->scheduled_downtime = IDO_TRUE;
				state->end_time = dt_start_time;
				clone_state->start_time = dt_start_time;
			} else {
				state->end_time = dt_end_time;
				clone_state->start_time = state->end_time;
			}

			/*
			 * if the cloned state happens to intersect
			 * with another segment of downtime the for
			 * loop will take care of that later on.
			 */
			clone_state->scheduled_downtime = IDO_FALSE;

			sla_save_state(idi, clone_state);
		}

		sla_save_state(idi, state);
	}

	return 0;
}

/**
 * Queries the database for dependent services of parent_object_id. A list
 * of services is placed in service_object_ids and the total number of
 * services is returned. The returned list must be passed to free() when it's
 * no longer needed.
 *
 * @param idi the database connection
 * @param parent_object_id the parent object ID (usually a host)
 * @param service_object_ids the returned list of dependent services
 * @return the number of services or a negative value in case of an error
 */
static int sla_query_dependent_services(ido2db_idi *idi,
                                        unsigned long parent_object_id, int **service_object_ids) {
	int count, i;
#ifndef USE_ORACLE
	char *query;
	int rc;
#else /* !USE_ORACLE */
	uint data[2];
#endif /* !USE_ORACLE */

	if (idi == NULL || idi->dbinfo.connected == IDO_FALSE ||
	        service_object_ids == NULL)
		return -1;

#ifdef USE_LIBDBI

	rc = asprintf(&query, "SELECT service_object_id "
	              "FROM %s "
	              "WHERE instance_id = '%lu' AND host_object_id = '%lu'",
	              ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES],
	              idi->dbinfo.instance_id, parent_object_id);

	if (rc < 0) {
		*service_object_ids = NULL;
		return -1;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2,
	                      "sla_query_dependent_services(): %s\n", query);

	rc = ido2db_db_query(idi, query);

	free(query);

	if (rc != IDO_OK || idi->dbinfo.dbi_result == NULL) {
		free(*service_object_ids);
		*service_object_ids = NULL;
		return -1;
	}

	count = dbi_result_get_numrows(idi->dbinfo.dbi_result);
	*service_object_ids = malloc(count * sizeof(int));

	if (*service_object_ids == NULL) {
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		return -1;
	}

	for (i = 0; i < count; i++) {
		if (!dbi_result_next_row(idi->dbinfo.dbi_result)) {
			free(*service_object_ids);
			*service_object_ids = NULL;
			return -1;
		}

		(*service_object_ids)[i] = dbi_result_get_ulonglong(
		                               idi->dbinfo.dbi_result, "service_object_id");
	}

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif /* USE_LIBDBI */

#ifdef USE_PGSQL

#endif /* USE_PGSQL */

#ifdef USE_ORACLE
	data[0] = idi->dbinfo.instance_id;
	data[1] = parent_object_id;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_services_select,
	                         MT(":X1"), (uint *) & (data[0]))) {
		return -1;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_sla_services_select,
	                         MT(":X2"), (uint *) & (data[1]))) {
		return -1;
	}

	if (!OCI_Execute(idi->dbinfo.oci_statement_sla_services_select)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
		                      "sla_query_dependent_services() execute error\n");
		return -1;
	}

	OCI_Commit(idi->dbinfo.oci_connection);

	idi->dbinfo.oci_resultset =
	    OCI_GetResultset(idi->dbinfo.oci_statement_sla_services_select);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                      "sla_query_dependent_services() query ok\n");

	count = OCI_GetRowCount(idi->dbinfo.oci_resultset);
	*service_object_ids = malloc(count * sizeof(int));

	if (*service_object_ids == NULL)
		return -1;

	for (i = 0; i < count; i++) {
		if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
			                      "sla_query_dependent_services()"
			                      " fetchnext not ok\n");

			free(*service_object_ids);
			*service_object_ids = NULL;
			return -1;
		}

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
		                      "sla_query_dependent_services() fetchnext ok\n");

		(*service_object_ids)[i] =
		    OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 1);
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                      "sla_query_dependent_services(instance_id=%lu,"
	                      " parent_object_id=%llu)\n", idi->dbinfo.instance_id,
	                      parent_object_id);
#endif /* USE_ORACLE */

	return count;
}

/**
 * Updates the SLA state history when a host/service state changes.
 *
 * @return 0 on success, negative value on failure
 */
static int sla_process_statechange_one(ido2db_idi *idi, unsigned long object_id,
                                       time_t start_time, time_t end_time, const int *pstate_value,
                                       const int *pstate_type, const int *pdowntime) {
	sla_state_t *state, *previous_state, *new_state;
	sla_state_list_t *state_list;
	sla_downtime_list_t *downtime_list;
	time_t earliest_start_time;
	int rc, i;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                       "sla_process_statechange(%p, %lu, %lu, %lu, "
	                       "%p, %p)\n", idi, object_id, start_time,
	                       end_time, pstate_value, pstate_type);

	rc = sla_query_states(idi, object_id, start_time, end_time,
	                      &state_list);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                     "sla_query_states(): %d\n", rc);

	if (rc < 0)
		return rc;

	/* there should only ever be at most one result */
	if (state_list->count > 1)
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
		                      "Error: more than one state entry with "
		                      "end_time set to NULL.");

	previous_state = NULL;

	earliest_start_time = start_time;

	for (i = 0; i < state_list->count; i++) {
		state = &(state_list->states[i]);

		if (state->end_time == 0 && (previous_state == NULL ||
		                             previous_state->start_time > state->state))
			previous_state = state;

		if (state->start_time < earliest_start_time)
			earliest_start_time = state->start_time;

		/* Only update end time when the state has changed. */
		if ((pstate_value != NULL && pstate_type != NULL) ||
		        pdowntime != NULL)
			state->end_time = end_time;
	}

	rc = sla_query_downtime(idi, object_id, earliest_start_time, end_time,
	                        &downtime_list);

	if (rc >= 0) {
		/*
		 * The call to sla_apply_downtime also takes care of saving the
		 * changed SLA states (if necessary).
		 */
		sla_apply_downtime(idi, &state_list, downtime_list);

		sla_free_downtime_list(downtime_list);
	}

	if (pstate_value == NULL && pstate_type == NULL && pdowntime == NULL) {
		sla_free_state_list(state_list);

		return 0;
	}

	new_state = sla_alloc_state(idi->dbinfo.instance_id, object_id);

	if (new_state == NULL) {
		sla_free_state_list(state_list);
		return -1;
	}

	if (previous_state == NULL &&
	        (pstate_value == NULL || pstate_value == NULL))
		return -1;

	new_state->start_time = end_time;

	if (pstate_value != NULL && pstate_type != NULL) {
		new_state->state = *pstate_value;
		new_state->state_type = *pstate_type;
	} else {
		new_state->state = previous_state->state;
		new_state->state_type = previous_state->state_type;
	}

	if (previous_state != NULL && new_state->state != STATE_OK)
		new_state->acknowledgement_time =
		    previous_state->acknowledgement_time;

	if (pdowntime != NULL)
		new_state->scheduled_downtime = *pdowntime;
	else if (previous_state != NULL)
		/*
		 * TODO: query downtimes, just to double-check whether the
		 * value is right?
		 */
		new_state->scheduled_downtime =
		    previous_state->scheduled_downtime;

	sla_save_state(idi, new_state);

	sla_free_state_list(state_list);

	sla_free_state(new_state);

	return 0;
}

/**
 * Updates the SLA state history when a host/service state changes. Also updates
 * the state for dependent services if the passed object_id is a host.
 *
 * @return 0 on success, negative value on failure
 */
int sla_process_statechange(ido2db_idi *idi, unsigned long object_id,
                            time_t start_time, time_t end_time, const int *pstate_value,
                            const int *pstate_type, const int *pdowntime) {
	int *dependent_services = NULL;
	int rc, i, service_state;
	int *pservice_state = NULL;

	/* Propagate host state changes to dependent services. */
	if (pstate_value != NULL) {
		if (*pstate_value == 0)
			service_state = STATE_OK;
		else
			service_state = STATE_CRITICAL;

		pservice_state = &service_state;
	}

	rc = sla_query_dependent_services(idi, object_id,
	                                  &dependent_services);

	if (rc >= 0) {
		for (i = 0; i < rc; i++) {
			/*
			 * TODO: rather than setting services to UP
			 * when the host is available again we should
			 * use the service's state before the downtime
			 * event.
			 */
			(void) sla_process_statechange_one(idi,
			                                   dependent_services[i], start_time, end_time,
			                                   pservice_state, pstate_type, pdowntime);
		}

		free(dependent_services);
	}

	return sla_process_statechange_one(idi, object_id, start_time, end_time,
	                                   pstate_value, pstate_type, pdowntime);

}

/**
 * Updates the SLA state history when a state is acknowledged/unacknowledged.
 * @return 0 on success, negative value on failure
 */
int sla_process_acknowledgement(ido2db_idi *idi, unsigned long object_id,
                                time_t state_time, int is_acknowledged) {
	sla_state_t *state;
	sla_state_list_t *state_list;
	int rc, i;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                      "sla_process_acknowledgement(%p, %lu, %lu, %d)\n",
	                      idi, object_id, state_time, is_acknowledged);

	rc = sla_query_states(idi, object_id, state_time, state_time,
	                      &state_list);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2,
	                      "sla_query_states(): %d\n", rc);

	if (rc < 0)
		return rc;

	for (i = 0; i < state_list->count; i++) {
		state = &(state_list->states[i]);

		if (is_acknowledged) {
			if ((state_time < state->acknowledgement_time) ||
			        (state->acknowledgement_time == 0))
				state->acknowledgement_time = state_time;
		} else {
			state->acknowledgement_time = 0;
		}

		sla_save_state(idi, state);
	}

	sla_free_state_list(state_list);

	return 0;
}

/**
 * Updates the SLA state history when a scheduled downtime is started or ends.
 * @return 0 on success, negative value on failure
 */
int sla_process_downtime(ido2db_idi *idi, unsigned long object_id,
                         time_t state_time, int event_type) {
	int downtime;

	downtime = (event_type == NEBTYPE_DOWNTIME_START);

	return sla_process_statechange(idi, object_id, state_time, state_time,
	                               NULL, NULL, &downtime);
}

int sla_process_downtime_history(ido2db_idi *idi, unsigned long object_id,
                                 time_t start_time, time_t end_time) {
	return sla_process_statechange_one(idi, object_id, start_time, end_time,
	                                   NULL, NULL, NULL);
}
