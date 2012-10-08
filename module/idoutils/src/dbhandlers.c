/***************************************************************
 * DBHANDLERS.C - Data handler routines for IDO2DB daemon
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
#include "../include/db.h"
#include "../include/dbhandlers.h"
#include "../include/dbqueries.h"
#include "../include/sla.h"
#include "../include/logging.h"

/* Icinga header files */
#include "../../../include/icinga.h"
#include "../../../include/broker.h"
#include "../../../include/comments.h"

extern int errno;

extern char *ido2db_db_tablenames[IDO2DB_MAX_DBTABLES];

extern ido2db_dbconfig ido2db_db_settings; /* for tables cleanup settings */

extern int enable_sla;

int dummy;	/* reduce compiler warnings */

/****************************************************************************/
/* OBJECT ROUTINES                                                          */
/****************************************************************************/

int ido2db_get_object_id(ido2db_idi *idi, int object_type, char *n1, char *n2, unsigned long *object_id) {
	int result = IDO_OK;
	int x = 0;
	unsigned long cached_object_id = 0L;
	char *name1 = NULL;
	char *name2 = NULL;
#ifdef USE_LIBDBI
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
#endif
#ifdef USE_ORACLE
	void *data[4];
#endif
	char *es[2];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id() start\n");

	/* make sure empty strings are set to null */
	name1 = n1;
	name2 = n2;
	if (name1 && !strcmp(name1, ""))
		name1 = NULL;
	if (name2 && !strcmp(name2, ""))
		name2 = NULL;

	/* null names mean no object id */
	if (name1 == NULL && name2 == NULL) {
		*object_id = 0L;
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id() null names=ID 0:shouldnt not happen\n");
		return IDO_OK;
	}

	/* see if the object already exists in cached lookup table */
	if (ido2db_get_cached_object_id(idi, object_type, name1, name2, &cached_object_id) == IDO_OK) {
		*object_id = cached_object_id;
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id(%lu) return cached object\n", *object_id);
		return IDO_OK;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */

	/* normal parts for !oracle */

	if (name1 == NULL) {
		es[0] = NULL;
		if (asprintf(&buf1, "name1 IS NULL") == -1)
			buf1 = NULL;
	} else {
		es[0] = ido2db_db_escape_string(idi, name1);
		switch (idi->dbinfo.server_type) {
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
		case IDO2DB_DBSERVER_PGSQL:
			/* Postgres does case sensitive compare  */
			if (asprintf(&buf1, "name1=E'%s'", es[0]) == -1)
				buf1 = NULL;
			break;
		default:
			/* William Preston: mysql does case sensitive compare
			 * IF the collation is changed to latin1_general_cs */
			if (asprintf(&buf1, "name1='%s'", es[0]) == -1)
				buf1 = NULL;
			break;
		}
	}

	if (name2 == NULL) {
		es[1] = NULL;
		if (asprintf(&buf2, "name2 IS NULL") == -1)
			buf2 = NULL;
	} else {
		es[1] = ido2db_db_escape_string(idi, name2);
		switch (idi->dbinfo.server_type) {
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
                case IDO2DB_DBSERVER_PGSQL:
                        /* Postgres does case sensitive compare  */
                        if (asprintf(&buf2, "name2=E'%s'", es[1]) == -1)
                                buf2 = NULL;
                        break;
		default:
			/* William Preston: mysql does case sensitive compare
			 * IF the collation is changed to latin1_general_cs */
			if (asprintf(&buf2, "name2='%s'", es[1]) == -1)
				buf2 = NULL;
			break;
		}
	}

	if (asprintf(&buf, "SELECT object_id FROM %s WHERE instance_id=%lu AND objecttype_id=%d AND %s AND %s", ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS], idi->dbinfo.instance_id, object_type, buf1, buf2) == -1)
		buf = NULL;

	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {
		if (idi->dbinfo.dbi_result != NULL) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
				*object_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "object_id");
			} else {
				result = IDO_ERROR;
			}

			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
		}
	}

	free(buf);
	free(buf1);
	free(buf2);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;
	/**
	 * #1655 new prepared statement can handle both, null and values
	 */
	es[0] = ido2db_db_escape_string(idi, name1);
	es[1] = ido2db_db_escape_string(idi, name2);

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &object_type;
	data[2] = (void *) &es[0];
	data[3] = (void *) &es[1];


	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_objects_select_name1_name2, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_objects_select_name1_name2, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	/* check if name1/2 would be NULL, explicitely bind that to NULL so that the IS NULL from the selects match */
	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind name1\n");
	if (name1 == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_objects_select_name1_name2, ":X3") == IDO_ERROR) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id Bind name1=null failed\n");
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_objects_select_name1_name2, MT(":X3"), *(char **) data[2], 0)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id Bind name1=%s failed\n", (es[0]==NULL)?"(null)":es[0]);
			return IDO_ERROR;
		}
	}
	// Name2
	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind name2\n");
	if (name2 == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_objects_select_name1_name2, ":X4") == IDO_ERROR) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id Bind name2=null failed\n");
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_objects_select_name1_name2, MT(":X4"), *(char **) data[3], 0)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id Bind name2=%s failed\n", (es[1]==NULL)?"(null)":es[1]);
			return IDO_ERROR;
		}
	}


	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_objects_select_name1_name2)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_objects_select_name1_name2(%s,%s) execute error\n", (es[0]==NULL)?"(null)":es[0], (es[1]==NULL)?"(null)":es[1]);
		return IDO_ERROR;
	}

	OCI_Commit(idi->dbinfo.oci_connection);
	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_objects_select_name1_name2);
	if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
		*object_id = OCI_GetUnsignedInt2(idi->dbinfo.oci_resultset, MT("id"));
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_objects_select_name1_name2(%s,%s) object id=%lu selected\n", (es[0]==NULL)?"(null)":es[0], (es[1]==NULL)?"(null)":es[1], *object_id);
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_objects_select_name1_name2(%s,%s) object id could not be found\n", (es[0]==NULL)?"(null)":es[0], (es[1]==NULL)?"(null)":es[1]);
		result = IDO_ERROR;
	}

	/* do not free statement yet! */



#endif /* Oracle ocilib specific */

	/* free memory */
	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_objects_select_() before free\n");
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	//ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_objects_select_() free of es\n");

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id(%lu) end\n", *object_id);

	return result;
}

int ido2db_get_object_id_with_insert(ido2db_idi *idi, int object_type, char *n1, char *n2, unsigned long *object_id) {
	int result = IDO_OK;
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
#ifdef USE_LIBDBI
	char *tmp = NULL;
#endif
	char *name1 = NULL;
	char *name2 = NULL;
	char *es[2];
#ifdef USE_ORACLE
	void *data[4];
	char *fname = "get_object_id_with_insert";
	const char *sql = NULL;
	OCI_Statement *st;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() start\n");

	/* make sure empty strings are set to null */
	name1 = n1;
	name2 = n2;
	if (name1 && !strcmp(name1, ""))
		name1 = NULL;
	if (name2 && !strcmp(name2, ""))
		name2 = NULL;

	/* null names mean no object id */
	if (name1 == NULL && name2 == NULL) {
		*object_id = 0L;
		return IDO_OK;
	}

	/* object already exists */
	if ((result = ido2db_get_object_id(idi, object_type, name1, name2,
	                                   object_id)) == IDO_OK)
		return IDO_OK;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (name1 != NULL) {
		tmp = ido2db_db_escape_string(idi, name1);
                switch (idi->dbinfo.server_type) {
                case IDO2DB_DBSERVER_PGSQL:
			dummy = asprintf(&es[0], "E'%s'", tmp);
                        break;
                default:
			dummy = asprintf(&es[0], "'%s'", tmp);
                        break;
                }
		if (tmp) {
			free(tmp);
			tmp = NULL;
		}
	} else
		dummy = asprintf(&es[0], "NULL");

	if (name2 != NULL) {
		tmp = ido2db_db_escape_string(idi, name2);
                switch (idi->dbinfo.server_type) {
                case IDO2DB_DBSERVER_PGSQL:
                        dummy = asprintf(&es[1], "E'%s'", tmp);
                        break;
                default:
                        dummy = asprintf(&es[1], "'%s'", tmp);
                        break;
                }
		if (tmp) {
			free(tmp);
			tmp = NULL;
		}
	} else
		dummy = asprintf(&es[1], "NULL");

	if (asprintf(&buf,
	             "INSERT INTO %s (instance_id, objecttype_id, name1, name2) VALUES (%lu, %d, %s, %s)",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS],
	             idi->dbinfo.instance_id, object_type, es[0],
	             es[1]) == -1)
		buf = NULL;
	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {

		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			*object_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert(%lu) object_id\n", *object_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&tmp, "%s_object_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS]) == -1)
				tmp = NULL;

			*object_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, tmp);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert(%s=%lu) object_id\n", tmp, *object_id);
			free(tmp);
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

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;

	es[0] = ido2db_db_escape_string(idi, name1);
	es[1] = ido2db_db_escape_string(idi, name2);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() name1=%s, name2=%s\n", (es[0]==NULL)?"(null)":es[0], (es[1]==NULL)?"(null)":es[1]);

#ifdef DEBUG_IDO2DB
	syslog(LOG_USER | LOG_INFO, "name1=%s, name2=%s\n", (es[0]==NULL)?"(null)":es[0], (es[1]==NULL)?"(null)":es[1]);
#endif

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &object_type;
	data[2] = (void *) &es[0];
	data[3] = (void *) &es[1];

	/* FIXME: OCILIB claimed this already prepared statement is not prepared ,
	 * but no prepare error occured and statement sql is available with statement handle
	 * (https://dev.icinga.org/issues/1638)
	 *
	 * this bad workaround prepares new statement handle at every call
	 * */
	st = OCI_CreateStatement(idi->dbinfo.oci_connection);
	sql = OCI_GetSql(idi->dbinfo.oci_statement_objects_insert);
	if (!OCI_Prepare(st, MT(sql))) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() extra prepare failed\n");
		return IDO_ERROR;
	}

	OCI_RegisterUnsignedInt(st, MT(":id"));
	//free old and reassign
	ido2db_oci_statement_free(idi->dbinfo.oci_statement_objects_insert, fname);
	idi->dbinfo.oci_statement_objects_insert = st;

	/* --end workaround-- */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind instance id %lu\n", idi->dbinfo.instance_id);
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_objects_insert, MT(":X1"), (uint *) data[0])) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind instance id failed\n");
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind type\n");
	if (!OCI_BindInt(idi->dbinfo.oci_statement_objects_insert, MT(":X2"), (int *) data[1])) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind type failed\n");
		return IDO_ERROR;
	}

	/* check if name1/2 would be NULL, explicitely bind that to NULL so that the IS NULL from the selects match */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind name1\n");
	if (name1 == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind name1=0\n");
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_objects_insert, ":X3") == IDO_ERROR) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind name1=null failed\n");
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_objects_insert, MT(":X3"), *(char **) data[2], 0)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind name1=%s failed\n", (es[0]==NULL)?"(null)":es[0]);
			return IDO_ERROR;
		}
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind name2\n");
	if (name2 == NULL) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() name2=0\n");
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_objects_insert, ":X4") == IDO_ERROR) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind name2=null failed\n");
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_objects_insert, MT(":X4"), *(char **) data[3], 0)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() Bind name2=%s failed\n", (es[1]==NULL)?"(null)":es[1]);
			return IDO_ERROR;
		}
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() execute\n");
	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_objects_insert)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() execute error\n");
		return IDO_ERROR;
	}

	OCI_Commit(idi->dbinfo.oci_connection);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() get rs\n");
	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_objects_insert);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() fetch\n");
	if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
		*object_id = OCI_GetUnsignedInt(idi->dbinfo.oci_resultset, 1);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() OK with inserted object id=%lu\n", *object_id);
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert() inserted object id could not be fetched\n");
	}


	/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	free(buf);

	/* cache object id for later lookups */
	ido2db_add_cached_object_id(idi, object_type, name1, name2, *object_id);

	/* free memory */
	free(buf1);
	free(buf2);

	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_object_id_with_insert(object id=%lu) end\n", *object_id);
	return result;
}

int ido2db_get_cached_object_ids(ido2db_idi *idi) {
	int result = IDO_OK;
	unsigned long object_id = 0L;
	int objecttype_id = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif

#ifdef USE_ORACLE
	char *tmp1 = NULL;
	char *tmp2 = NULL;
	void *data[1];
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_cached_object_ids() start\n");

	/* find all the object definitions we already have */
#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (asprintf(&buf, "SELECT object_id, objecttype_id, name1, name2 FROM %s WHERE instance_id=%lu", ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS], idi->dbinfo.instance_id) == -1)
		buf = NULL;

	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {
		while (idi->dbinfo.dbi_result) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
				object_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result,
				                                     "object_id");
				objecttype_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result,
				                "objecttype_id");
				ido2db_add_cached_object_id(idi, objecttype_id,
				                            dbi_result_get_string_copy(idi->dbinfo.dbi_result,
				                                    "name1"), dbi_result_get_string_copy(
				                                idi->dbinfo.dbi_result, "name2"), object_id);
			}
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
		}
	}

	free(buf);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;


	data[0] = (void *) &idi->dbinfo.instance_id;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_objects_select_cached, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_objects_select_cached)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_objects_select_cached() execute error\n");
		return IDO_ERROR;
	}

	OCI_Commit(idi->dbinfo.oci_connection);
	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_objects_select_cached);

	if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_cached_object_ids() fetchnext ok\n");
		object_id = OCI_GetUnsignedInt2(idi->dbinfo.oci_resultset, MT("id"));
		objecttype_id = OCI_GetUnsignedInt2(idi->dbinfo.oci_resultset, MT("objecttype_id"));

		/* dirty little hack for mtext* <-> char* */
		if (asprintf(&tmp1, "%s", OCI_GetString2(idi->dbinfo.oci_resultset, MT("name1"))) == -1)
			tmp1 = NULL;
		if (asprintf(&tmp2, "%s", OCI_GetString2(idi->dbinfo.oci_resultset, MT("name2"))) == -1)
			tmp2 = NULL;

		ido2db_add_cached_object_id(idi, objecttype_id, tmp1, tmp2, object_id);

	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_cached_object_ids() fetchnext ok\n\n");
	}


	/* do not free statement yet! */

	free(tmp1);
	free(tmp2);

#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_cached_object_ids(%lu) end\n", object_id);

	return result;
}

int ido2db_get_cached_object_id(ido2db_idi *idi, int object_type, char *name1,
                                char *name2, unsigned long *object_id) {
	int result = IDO_ERROR;
	int hashslot = 0;
	int compare = 0;
	ido2db_dbobject *temp_object = NULL;
	int y = 0;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_cached_object_id() start\n");

	hashslot = ido2db_object_hashfunc(name1, name2, IDO2DB_OBJECT_HASHSLOTS);
#ifdef IDO2DB_DEBUG_CACHING
	printf("OBJECT LOOKUP: type=%d, name1=%s, name2=%s\n", object_type, (name1 == NULL) ? "NULL" : name1, (name2 == NULL) ? "NULL" : name2);
#endif

	if (idi->dbinfo.object_hashlist == NULL)
		return IDO_ERROR;

	for (temp_object = idi->dbinfo.object_hashlist[hashslot], y = 0; temp_object
	        != NULL; temp_object = temp_object->nexthash, y++) {
#ifdef IDO2DB_DEBUG_CACHING
		printf("OBJECT LOOKUP LOOPING [%d][%d]: type=%d, id=%lu, name1=%s, name2=%s\n", hashslot, y, temp_object->object_type, temp_object->object_id, (temp_object->name1 == NULL) ? "NULL" : temp_object->name1, (temp_object->name2 == NULL) ? "NULL" : temp_object->name2);
#endif
		compare = ido2db_compare_object_hashdata(temp_object->name1,
		          temp_object->name2, name1, name2);
		if (compare == 0 && temp_object->object_type == object_type)
			break;
	}

	/* we have a match! */
	if (temp_object && (ido2db_compare_object_hashdata(temp_object->name1,
	                    temp_object->name2, name1, name2) == 0) && temp_object->object_type
	        == object_type) {
#ifdef IDO2DB_DEBUG_CACHING
		printf("OBJECT CACHE HIT [%d][%d]: type=%d, id=%lu, name1=%s, name2=%s\n", hashslot, y, object_type, temp_object->object_id, (name1 == NULL) ? "NULL" : name1, (name2 == NULL) ? "NULL" : name2);
#endif
		*object_id = temp_object->object_id;
		result = IDO_OK;
	}
#ifdef IDO2DB_DEBUG_CACHING
	else {
		printf("OBJECT CACHE MISS: type=%d, name1=%s, name2=%s\n", object_type, (name1 == NULL) ? "NULL" : name1, (name2 == NULL) ? "NULL" : name2);
	}
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_get_cached_object_id(%lu) end\n", *object_id);

	return result;
}

int ido2db_add_cached_object_id(ido2db_idi *idi, int object_type, char *n1, char *n2, unsigned long object_id) {
	int result = IDO_OK;
	ido2db_dbobject *temp_object = NULL;
	ido2db_dbobject *lastpointer = NULL;
	ido2db_dbobject *new_object = NULL;
	int x = 0;
	int y = 0;
	int hashslot = 0;
	int compare = 0;
	char *name1 = NULL;
	char *name2 = NULL;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_add_cached_object_id() start\n");

	/* make sure empty strings are set to null */
	name1 = n1;
	name2 = n2;
	if (name1 && !strcmp(name1, ""))
		name1 = NULL;
	if (name2 && !strcmp(name2, ""))
		name2 = NULL;

	/* null names mean no object id, so don't cache */
	if (name1 == NULL && name2 == NULL) {
		return IDO_OK;
	}

#ifdef IDO2DB_DEBUG_CACHING
	printf("OBJECT CACHE ADD: type=%d, id=%lu, name1=%s, name2=%s\n", object_type, object_id, (name1 == NULL) ? "NULL" : name1, (name2 == NULL) ? "NULL" : name2);
#endif

	/* initialize hash list if necessary */
	if (idi->dbinfo.object_hashlist == NULL) {

		idi->dbinfo.object_hashlist = (ido2db_dbobject **) malloc(
		                                  sizeof(ido2db_dbobject *) * IDO2DB_OBJECT_HASHSLOTS);
		if (idi->dbinfo.object_hashlist == NULL)
			return IDO_ERROR;

		for (x = 0; x < IDO2DB_OBJECT_HASHSLOTS; x++)
			idi->dbinfo.object_hashlist[x] = NULL;
	}

	/* allocate and populate new object */
	if ((new_object = (ido2db_dbobject *) malloc(sizeof(ido2db_dbobject)))
	        == NULL)
		return IDO_ERROR;
	new_object->object_type = object_type;
	new_object->object_id = object_id;
	new_object->name1 = NULL;
	if (name1)
		new_object->name1 = strdup(name1);
	new_object->name2 = NULL;
	if (name2)
		new_object->name2 = strdup(name2);

	hashslot = ido2db_object_hashfunc(new_object->name1, new_object->name2,
	                                  IDO2DB_OBJECT_HASHSLOTS);

	lastpointer = NULL;
	for (temp_object = idi->dbinfo.object_hashlist[hashslot], y = 0; temp_object
	        != NULL; temp_object = temp_object->nexthash, y++) {
		compare = ido2db_compare_object_hashdata(temp_object->name1,
		          temp_object->name2, new_object->name1, new_object->name2);
		if (compare < 0)
			break;
		lastpointer = temp_object;
	}

	if (lastpointer)
		lastpointer->nexthash = new_object;
	else
		idi->dbinfo.object_hashlist[hashslot] = new_object;
	new_object->nexthash = temp_object;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_add_cached_object_id() end\n");

	return result;
}

/*
 * hash functions
 */
static unsigned long ido2db_object_sdbm(const char *str) {
        unsigned long hash = 0;
        int c;

        while ((c = *str++) != '\0')
                hash = c + (hash << 6) + (hash << 16) - hash;

        return hash;
}

int ido2db_object_hashfunc(const char *name1, const char *name2, int hashslots) {
        unsigned int result = 0;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_object_hashfunc() start\n");

        if (name1)
                result += ido2db_object_sdbm(name1);

        if (name2)
                result += ido2db_object_sdbm(name2);

        result = result % hashslots;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_object_hashfunc() end\n");

        return result;
}

int ido2db_compare_object_hashdata(const char *val1a, const char *val1b,
                                   const char *val2a, const char *val2b) {
	int result = 0;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_compare_object_hashdata() start\n");

	/* check first name */
	if (val1a == NULL && val2a == NULL)
		result = 0;
	else if (val1a == NULL)
		result = 1;
	else if (val2a == NULL)
		result = -1;
	else
		result = strcmp(val1a, val2a);

	/* check second name if necessary */
	if (result == 0) {
		if (val1b == NULL && val2b == NULL)
			result = 0;
		else if (val1b == NULL)
			result = 1;
		else if (val2b == NULL)
			result = -1;
		else
			return strcmp(val1b, val2b);
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_compare_object_hashdata() end\n");

	return result;
}

/*
 * free cached object ids
 */
int ido2db_free_cached_object_ids(ido2db_idi *idi) {
	int x = 0;
	ido2db_dbobject *temp_object = NULL;
	ido2db_dbobject *next_object = NULL;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_free_cached_object_ids() start\n");

	if (idi == NULL)
		return IDO_OK;

	if (idi->dbinfo.object_hashlist) {

		for (x = 0; x < IDO2DB_OBJECT_HASHSLOTS; x++) {
			for (temp_object = idi->dbinfo.object_hashlist[x]; temp_object
			        != NULL; temp_object = next_object) {
				next_object = temp_object->nexthash;
				free(temp_object->name1);
				free(temp_object->name2);
				free(temp_object);
			}
		}

		free(idi->dbinfo.object_hashlist);
		idi->dbinfo.object_hashlist = NULL;
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_free_cached_object_ids() end\n");

	return IDO_OK;
}


int ido2db_set_all_objects_as_inactive(ido2db_idi *idi) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif

#ifdef USE_ORACLE
	void *data[2];
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_set_all_objects_as_inactive() start\n");

	/* mark all objects as being inactive */
#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (asprintf(&buf, "UPDATE %s SET is_active='0' WHERE instance_id=%lu",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS],
	             idi->dbinfo.instance_id) == -1)
		buf = NULL;

	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
	free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


	data[0] = (void *) &idi->dbinfo.instance_id;
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_objects_update_inactive, MT(":X2"), (uint *) data[0])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_objects_update_inactive)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_objects_update_inactive() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_set_all_objects_as_inactive() end\n");

	return result;
}

int ido2db_set_object_as_active(ido2db_idi *idi, int object_type,
                                unsigned long object_id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif

#ifdef USE_ORACLE

	void *data[3];
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_set_object_as_active() start\n");

	/* mark the object as being active */
#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (asprintf(
	            &buf,
	            "UPDATE %s SET is_active='1' WHERE instance_id=%lu AND objecttype_id=%d AND object_id=%lu",
	            ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS],
	            idi->dbinfo.instance_id, object_type, object_id) == -1)
		buf = NULL;

	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
	free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;


	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &object_type;
	data[2] = (void *) &object_id;


	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_objects_update_active, MT(":X2"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_objects_update_active, MT(":X3"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_objects_update_active, MT(":X4"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_objects_update_active)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_objects_update_active() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_set_object_as_active() end\n");

	return result;
}

int ido2db_handle_object_enable_disable(ido2db_idi *idi, int enable) {
	int result = IDO_OK;
        char *name1 = NULL;
        char *name2 = NULL;
#ifdef USE_LIBDBI
        char *buf = NULL;
        char *buf1 = NULL;
        char *buf2 = NULL;
#endif
#ifdef USE_ORACLE
        void *data[4];
#endif
        char *es[2];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_object_enable_disable(enable=%d) start\n", enable);

	if (enable < 0 || enable > 1)
		return IDO_ERROR;

	name1 = idi->buffered_input[IDO_DATA_HOST];
	name2 = idi->buffered_input[IDO_DATA_SERVICE];

	es[0] = ido2db_db_escape_string(idi, name1);
	es[1] = ido2db_db_escape_string(idi, name2);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_object_enable_disable() name1='%s', name2='%s'\n", es[0] == NULL ? "(null)" : es[0], es[1] == NULL ? "(null)" : es[1]);

#ifdef USE_LIBDBI /* everything else will be libdbi */

	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_PGSQL:
		if (asprintf(&buf1, "name1=E'%s'", es[0]) == -1)
			buf1 = NULL;
		break;
	default:
		if (asprintf(&buf1, "name1='%s'", es[0]) == -1)
			buf1 = NULL;
		break;
	}

        if (es[1] == NULL) {
                if (asprintf(&buf2, "name2 IS NULL") == -1)
                        buf2 = NULL;
        } else {
                switch (idi->dbinfo.server_type) {
                case IDO2DB_DBSERVER_PGSQL:
                        if (asprintf(&buf2, "name2=E'%s'", es[1]) == -1)
                                buf2 = NULL;
                        break;
                default:
                        if (asprintf(&buf2, "name2='%s'", es[1]) == -1)
                                buf2 = NULL;
                        break;
                }
	}

        if (asprintf(&buf, "UPDATE %s SET is_active=%d WHERE instance_id=%lu AND %s AND %s"
			,ido2db_db_tablenames[IDO2DB_DBTABLE_OBJECTS]
			,enable
              		,idi->dbinfo.instance_id
			,buf1
			,buf2
			) == -1
		)
                buf = NULL;

        result = ido2db_db_query(idi, buf);

        dbi_result_free(idi->dbinfo.dbi_result);
        idi->dbinfo.dbi_result = NULL;
        free(buf);
        free(buf1);
        free(buf2);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

        data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &enable;
	data[2] = (void *) &es[0];
	data[3] = (void *) &es[1];

        if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_object_enable_disable, MT(":X1"), (uint *) data[0])) {
                return IDO_ERROR;
        }

        if (!OCI_BindInt(idi->dbinfo.oci_statement_object_enable_disable, MT(":X2"), (int *) data[1])) {
                return IDO_ERROR;
        }

        /* check if name1/2 would be NULL, explicitely bind that to NULL so that the IS NULL from the selects match */
        if (name1 == NULL) {
                if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_object_enable_disable, ":X3") == IDO_ERROR) {
                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, " Bind name1=null failed\n");
                        return IDO_ERROR;
                }
        } else {
                if (!OCI_BindString(idi->dbinfo.oci_statement_object_enable_disable, MT(":X3"), *(char **) data[2], 0)) {
                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, " Bind name1=%s failed\n", (es[0]==NULL)?"(null)":es[0]);
                        return IDO_ERROR;
                }
        }
        // Name2
        if (name2 == NULL) {
                if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_object_enable_disable, ":X4") == IDO_ERROR) {
                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, " Bind name2=null failed\n");
                        return IDO_ERROR;
                }
        } else {
                if (!OCI_BindString(idi->dbinfo.oci_statement_object_enable_disable, MT(":X4"), *(char **) data[3], 0)) {
                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, " Bind name2=%s failed\n", (es[1]==NULL)?"(null)":es[1]);
                        return IDO_ERROR;
                }
        }

        /* execute statement */
        if (!OCI_Execute(idi->dbinfo.oci_statement_object_enable_disable)) {
                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_objects_enable_disable() execute error\n");
                return IDO_ERROR;
        }

        /* commit statement */
        OCI_Commit(idi->dbinfo.oci_connection);

        /* do not free statement yet! */

#endif /* Oracle ocilib specific */


	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_object_enable_disable() end\n");

	return result;

}



/****************************************************************************/
/* ARCHIVED LOG DATA HANDLER                                                */
/****************************************************************************/

int ido2db_handle_logentry(ido2db_idi *idi) {
	char *ptr = NULL;
	char *buf = NULL;
	char *es[1];
	time_t etime = 0L;
	char *ts[1];
	unsigned long type = 0L;
	int result = IDO_OK;
	int duplicate_record = IDO_FALSE;
	int len = 0;
	int x = 0;

#ifdef USE_ORACLE
	int n_zero = 0L;
	void *data[8];
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_logentry() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* break log entry in pieces */
	if ((ptr = strtok(idi->buffered_input[IDO_DATA_LOGENTRY], "]")) == NULL)
		return IDO_ERROR;
	if ((ido2db_convert_string_to_unsignedlong(ptr + 1,
	        (unsigned long *) &etime)) == IDO_ERROR)
		return IDO_ERROR;
	ts[0] = ido2db_db_timet_to_sql(idi, etime);
	if ((ptr = strtok(NULL, "\x0")) == NULL)
		return IDO_ERROR;
	es[0] = ido2db_db_escape_string(idi, (ptr + 1));

	/* strip newline chars from end */
	len = strlen(es[0]);
	for (x = len - 1; x >= 0; x--) {
		if (es[0][x] == '\n')
			es[0][x] = '\x0';
		else
			break;
	}

	/* what type of log entry is this? */
	type = 0;

	/* make sure we aren't importing a duplicate log entry... */
#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_PGSQL:
		if (asprintf(&buf, "SELECT logentry_id FROM %s WHERE instance_id=%lu AND logentry_time=%s AND logentry_data=E'%s'", ido2db_db_tablenames[IDO2DB_DBTABLE_LOGENTRIES], idi->dbinfo.instance_id, ts[0], es[0]) == -1)
			buf = NULL;
		break;
	default:
		if (asprintf(&buf, "SELECT logentry_id FROM %s WHERE instance_id=%lu AND logentry_time=%s AND logentry_data='%s'", ido2db_db_tablenames[IDO2DB_DBTABLE_LOGENTRIES], idi->dbinfo.instance_id, ts[0], es[0]) == -1)
			buf = NULL;
		break;
	}

	if ((result = ido2db_db_query(idi, buf)) == IDO_OK) {
		if (idi->dbinfo.dbi_result != NULL) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result) != 0)
				duplicate_record = IDO_TRUE;
		}
	}

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
	free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &etime;
	data[2] = (void *) &es[0];

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_select, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_select, MT(":X2"), (uint *) data[1])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindString(idi->dbinfo.oci_statement_logentries_select, MT(":X3"), *(char **) data[2], 0)) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_logentries_select)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_() execute error\n");
		return IDO_ERROR;
	}

	OCI_Commit(idi->dbinfo.oci_connection);
	idi->dbinfo.oci_resultset = OCI_GetResultset(idi->dbinfo.oci_statement_logentries_select);

	if (OCI_FetchNext(idi->dbinfo.oci_resultset)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_logentry() fetchnext ok\n");
		duplicate_record = IDO_TRUE;
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_logentry() fetchnext not ok\n");
	}


	/* do not free statement yet! */

#endif /* Oracle ocilib specific */


	/*if(duplicate_record==IDO_TRUE && idi->last_logentry_time!=etime){*/
	/*if(duplicate_record==IDO_TRUE && strcmp((es[0]==NULL)?"":es[0],idi->dbinfo.last_logentry_data)){*/
	if (duplicate_record == IDO_TRUE) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_logentry() ignoring duplicate log record\n");
#ifdef IDO2DB_DEBUG
		printf("IGNORING DUPLICATE LOG RECORD!\n");
#endif
		return IDO_OK;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	/* save entry to db */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_PGSQL:
	        if (asprintf(
	                    &buf,
	                    "INSERT INTO %s (instance_id, logentry_time, entry_time, entry_time_usec, logentry_type, logentry_data, realtime_data, inferred_data_extracted) VALUES (%lu, %s, %s, '0', %lu, E'%s', '0', '0')",
	                    ido2db_db_tablenames[IDO2DB_DBTABLE_LOGENTRIES],
	                    idi->dbinfo.instance_id, ts[0], ts[0], type, (es[0] == NULL) ? ""
	                    : es[0]) == -1)
	                buf = NULL;
		break;
	default:
	        if (asprintf(
	                    &buf,
        	            "INSERT INTO %s (instance_id, logentry_time, entry_time, entry_time_usec, logentry_type, logentry_data, realtime_data, inferred_data_extracted) VALUES (%lu, %s, %s, '0', %lu, '%s', '0', '0')",
	                    ido2db_db_tablenames[IDO2DB_DBTABLE_LOGENTRIES],
	                    idi->dbinfo.instance_id, ts[0], ts[0], type, (es[0] == NULL) ? ""
	                    : es[0]) == -1)
	                buf = NULL;
		break;
	}

	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;

	/* set only values needed */
	n_zero = 0L;

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &etime;
	data[2] = (void *) &etime;
	data[3] = (void *) &n_zero;
	data[4] = (void *) &type;
	data[5] = (void *) &es[0];
	data[6] = (void *) &n_zero;
	data[7] = (void *) &n_zero;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindString(idi->dbinfo.oci_statement_logentries_insert, MT(":X2"), *(char **) data[1], 0)) {
		return IDO_ERROR;
	}
	if (!OCI_BindString(idi->dbinfo.oci_statement_logentries_insert, MT(":X3"), *(char **) data[2], 0)) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X5"), (uint *) data[4])) {
		return IDO_ERROR;
	}

	if (es[0] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_logentries_insert, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_logentries_insert, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_logentries_insert)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_logentries_insert() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	free(buf);

	/* record timestamp of last log entry */
	idi->dbinfo.last_logentry_time = etime;

	/* save last log entry (for detecting duplicates) */
	if (idi->dbinfo.last_logentry_data)
		free(idi->dbinfo.last_logentry_data);
	idi->dbinfo.last_logentry_data = strdup((es[0] == NULL) ? "" : es[0]);

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++) {
		free(es[x]);
	}
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	/* TODO - further processing of log entry to expand archived data... */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_logentry() end\n");

	return result;
}

/****************************************************************************/
/* REALTIME DATA HANDLERS                                                   */
/****************************************************************************/

int ido2db_handle_processdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long process_id;
	int result = IDO_OK;
	char *ts[1];
	char *es[3];
	int x = 0;
	char *buf = NULL;
#ifdef USE_ORACLE
	void *data[8];
	unsigned long is_currently_running = 0L;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_processdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* convert vars */
	result = ido2db_convert_string_to_unsignedlong(
	             idi->buffered_input[IDO_DATA_PROCESSID], &process_id);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_PROGRAMNAME]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_PROGRAMVERSION]);
	es[2] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_PROGRAMDATE]);

	/* save entry to db */
#ifdef USE_LIBDBI /* everything else will be libdbi */
        switch (idi->dbinfo.server_type) {
        case IDO2DB_DBSERVER_PGSQL:
	        if (asprintf(&buf, "INSERT INTO %s (instance_id, event_type, event_time, event_time_usec, process_id, program_name, program_version, program_date) VALUES (%lu, %d, %s, %lu, %lu, E'%s', E'%s', E'%s')",
	                     ido2db_db_tablenames[IDO2DB_DBTABLE_PROCESSEVENTS],
	                     idi->dbinfo.instance_id, type, ts[0], tstamp.tv_usec, process_id,
	                     es[0], es[1], es[2]) == -1)
	                buf = NULL;
                break;
        default:
	        if (asprintf(&buf, "INSERT INTO %s (instance_id, event_type, event_time, event_time_usec, process_id, program_name, program_version, program_date) VALUES (%lu, %d, %s, %lu, %lu, '%s', '%s', '%s')",
                	     ido2db_db_tablenames[IDO2DB_DBTABLE_PROCESSEVENTS],
        	             idi->dbinfo.instance_id, type, ts[0], tstamp.tv_usec, process_id,
	                     es[0], es[1], es[2]) == -1)
        	        buf = NULL;
                break;
        }

	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &type;
	data[2] = (void *) &tstamp.tv_sec;
	data[3] = (void *) &tstamp.tv_usec;
	data[4] = (void *) &process_id;
	data[5] = (void *) &es[0];
	data[6] = (void *) &es[1];
	data[7] = (void *) &es[2];

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_process_events, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_process_events, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_process_events, MT(":X3"), (uint *) data[2])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_process_events, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_process_events, MT(":X5"), (uint *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindString(idi->dbinfo.oci_statement_process_events, MT(":X6"), *(char **) data[5], 0)) {
		return IDO_ERROR;
	}
	if (!OCI_BindString(idi->dbinfo.oci_statement_process_events, MT(":X7"), *(char **) data[6], 0)) {
		return IDO_ERROR;
	}
	if (!OCI_BindString(idi->dbinfo.oci_statement_process_events, MT(":X8"), *(char **) data[7], 0)) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_process_events)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_process_events() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	free(buf);

	/* MORE PROCESSING.... */

	/* if process is starting up, clearstatus data, event queue, etc. */
	if (type == NEBTYPE_PROCESS_PRELAUNCH && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		if (ido2db_db_settings.clean_realtime_tables_on_core_startup == IDO_TRUE) { /* only if desired */

			/* clear realtime data */
			/* don't clear necessary status tables on restart/reload of the core, as Icinga Web
			   won't show any data then */
			/* ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTSTATUS]); */
			/* ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICESTATUS]); */
			/* ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]); */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTSTATUS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTQUEUE]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_RUNTIMEVARIABLES]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS]);
		}

		if (ido2db_db_settings.clean_config_tables_on_core_startup == IDO_TRUE) { /* only if desired */
			/* clear config data */

			/* host definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS]); /* IDO2DB_OBJECTTYPE_HOST */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTPARENTHOSTS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTGROUPS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTS]);

			/* hostgroup definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS]); /* IDO2DB_OBJECTTYPE_HOSTGROUP */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPMEMBERS]);


			/* service definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES]); /* IDO2DB_OBJECTTYPE_SERVICE */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTGROUPS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTS]);

			/* servicegroup definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS]); /* IDO2DB_OBJECTTYPE_SERVICEGROUP */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPMEMBERS]);

			/* hostdependency definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTDEPENDENCIES]);

			/* servicedependency definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEDEPENDENCIES]);

			/* hostescalation definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTS]);

			/* serviceescalation definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS]);

			/* command definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_COMMANDS]); /* IDO2DB_OBJECTTYPE_COMMAND */

			/* timeperiod definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS]); /* IDO2DB_OBJECTTYPE_TIMEPERIOD */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODTIMERANGES]);

			/* contact definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS]); /* IDO2DB_OBJECTTYPE_CONTACT */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTADDRESSES]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS]); /* both host and service */

			/* contactgroup definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS]); /* IDO2DB_OBJECTTYPE_CONTACTGROUP */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPMEMBERS]);

			/* configfile definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES]);
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILEVARIABLES]);

			/* customvariable definition */
			ido2db_db_clear_table(idi, ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLES]);


		}

		/* flag all objects as being inactive */
		/* if the core starts up, the fresh config is being pushed
		           into ido2db, marking actual config object ids as active. */
		ido2db_set_all_objects_as_inactive(idi);
	}

	/* if process is shutting down or restarting, update process status data */
	if ((type == NEBTYPE_PROCESS_SHUTDOWN || type == NEBTYPE_PROCESS_RESTART)
	        && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		if (asprintf(&buf, "UPDATE %s SET program_end_time=%s, is_currently_running='0' WHERE instance_id=%lu",
		             ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS], ts[0],
		             idi->dbinfo.instance_id) == -1)
			buf = NULL;
		result = ido2db_db_query(idi, buf);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		data[0] = (void *) &tstamp.tv_sec;
		data[1] = (void *) &is_currently_running;
		data[2] = (void *) &idi->dbinfo.instance_id;

		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus_update, MT(":X1"), (uint *) data[0])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus_update, MT(":X2"), (uint *) data[1])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus_update, MT(":X3"), (uint *) data[2])) {
			return IDO_ERROR;
		}

		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_programstatus_update)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_programstatus_update() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */


#endif /* Oracle ocilib specific */

	}

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_processdata() end\n");

	return IDO_OK;
}

int ido2db_handle_timedeventdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int event_type = 0;
	unsigned long run_time = 0L;
	int recurring_event = 0;
	unsigned long object_id = 0L;
	int result = IDO_OK;
	char *ts[2];
	int x = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif
	void *data[9];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timedeventdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EVENTTYPE], &event_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_RECURRING], &recurring_event);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_RUNTIME], &run_time);

	/* skip sleep events.... */
	if (type == NEBTYPE_TIMEDEVENT_SLEEP) {

		/* we could do some maintenance here if we wanted.... */
		return IDO_OK;
	}

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, run_time);

	/* get the object id (if applicable) */
	if (event_type == EVENT_SERVICE_CHECK || (event_type == EVENT_SCHEDULED_DOWNTIME && idi->buffered_input[IDO_DATA_SERVICE] != NULL && strcmp(idi->buffered_input[IDO_DATA_SERVICE], "")))
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOST], idi->buffered_input[IDO_DATA_SERVICE], &object_id);

	if (event_type == EVENT_HOST_CHECK || (event_type == EVENT_SCHEDULED_DOWNTIME && (idi->buffered_input[IDO_DATA_SERVICE] == NULL || !strcmp(idi->buffered_input[IDO_DATA_SERVICE], ""))))
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);

	/* HISTORICAL TIMED EVENTS */

	/* save a record of timed events that get added */
	if (type == NEBTYPE_TIMEDEVENT_ADD) {

		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &event_type;
		data[2] = (void *) &ts[0];
		data[3] = (void *) &tstamp.tv_usec;
		data[4] = (void *) &ts[1];
		data[5] = (void *) &recurring_event;
		data[6] = (void *) &object_id;
		/* add unixtime for bind params */
		data[7] = (void *) &tstamp.tv_sec;
		data[8] = (void *) &run_time;


		result = ido2db_query_insert_or_update_timedevent_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* do not free if prepared statement */

#endif /* Oracle ocilib specific */
	}

	/* save a record of timed events that get executed.... */
	if (type == NEBTYPE_TIMEDEVENT_EXECUTE) {

		/* save entry to db */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &event_type;
		data[2] = (void *) &ts[0];
		data[3] = (void *) &tstamp.tv_usec;
		data[4] = (void *) &ts[1];
		data[5] = (void *) &recurring_event;
		data[6] = (void *) &object_id;
		/* add unixtime for bind params */
		data[7] = (void *) &tstamp.tv_sec;
		data[8] = (void *) &run_time;

		result = ido2db_query_insert_or_update_timedevents_execute_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* do not free if prepared statement */

#endif /* Oracle ocilib specific */
	}

	/* save a record of timed events that get removed.... */
	if (type == NEBTYPE_TIMEDEVENT_REMOVE) {

		/* save entry to db */
#ifdef USE_LIBDBI /* everything else will be libdbi */
		if (asprintf(&buf, "UPDATE %s SET deletion_time=%s, deletion_time_usec=%lu WHERE instance_id=%lu AND event_type=%d AND scheduled_time=%s AND recurring_event=%d AND object_id=%lu",
		             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTS], ts[0],
		             tstamp.tv_usec, idi->dbinfo.instance_id, event_type, ts[1],
		             recurring_event, object_id) == -1)
			buf = NULL;

		result = ido2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		data[0] = (void *) &tstamp.tv_sec;
		data[1] = (void *) &tstamp.tv_usec;
		data[2] = (void *) &idi->dbinfo.instance_id;
		data[3] = (void *) &event_type;
		data[4] = (void *) &run_time;
		data[5] = (void *) &recurring_event;
		data[6] = (void *) &object_id;

		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedevents_update, MT(":X1"), (uint *) data[0])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedevents_update, MT(":X2"), (uint *) data[1])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedevents_update, MT(":X3"), (uint *) data[2])) {
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_timedevents_update, MT(":X4"), (int *) data[3])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedevents_update, MT(":X5"), (uint *) data[4])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_timedevents_update, MT(":X6"), (int *) data[5])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedevents_update, MT(":X7"), (uint *) data[6])) {
			return IDO_ERROR;
		}

		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_timedevents_update)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_timedevents_update() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */


#endif /* Oracle ocilib specific */

	}

	/* CURRENT TIMED EVENTS */

	/* remove (probably) expired events from the queue if client just connected */
	if (idi->dbinfo.clean_event_queue == IDO_TRUE && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		idi->dbinfo.clean_event_queue = IDO_FALSE;

		/* clear old entries from db */
#ifdef USE_LIBDBI /* everything else will be libdbi */

		if (asprintf(&buf, "DELETE FROM %s WHERE instance_id=%lu AND scheduled_time<=%s",
		             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTQUEUE],
		             idi->dbinfo.instance_id, ts[0]) == -1)
			buf = NULL;
		result = ido2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &tstamp.tv_sec;

		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedeventqueue_delete, MT(":X1"), (uint *) data[0])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedeventqueue_delete, MT(":X2"), (uint *) data[1])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}

		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_timedeventqueue_delete)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_timedeventqueue_delete() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */


#endif /* Oracle ocilib specific */

	}

	/* ADD QUEUED TIMED EVENTS */
	if (type == NEBTYPE_TIMEDEVENT_ADD && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db by update or insert */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &event_type;
		data[2] = (void *) &ts[0];
		data[3] = (void *) &tstamp.tv_usec;
		data[4] = (void *) &ts[1];
		data[5] = (void *) &recurring_event;
		data[6] = (void *) &object_id;
		/* add unixtime for bind params */
		data[7] = (void *) &tstamp.tv_sec;
		data[8] = (void *) &run_time;


		result = ido2db_query_insert_or_update_timedeventqueue_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* do not free if prepared statement */

#endif /* Oracle ocilib specific */
	}

	/* REMOVE QUEUED TIMED EVENTS */
	if ((type == NEBTYPE_TIMEDEVENT_REMOVE || type
	        == NEBTYPE_TIMEDEVENT_EXECUTE) && tstamp.tv_sec
	        >= idi->dbinfo.latest_realtime_data_time) {

		/* clear entry from db */
#ifdef USE_LIBDBI /* everything else will be libdbi */

		if (asprintf(&buf, "DELETE FROM %s WHERE instance_id=%lu AND event_type=%d AND scheduled_time=%s AND recurring_event=%d AND object_id=%lu",
		             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTQUEUE],
		             idi->dbinfo.instance_id, event_type, ts[1], recurring_event,
		             object_id) == -1)
			buf = NULL;
		result = ido2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &event_type;
		data[2] = (void *) &run_time;
		data[3] = (void *) &recurring_event;
		data[4] = (void *) &object_id;

		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedeventqueue_delete_more, MT(":X1"), (uint *) data[0])) {
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_timedeventqueue_delete_more, MT(":X2"), (int *) data[1])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedeventqueue_delete_more, MT(":X3"), (uint *) data[2])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_timedeventqueue_delete_more, MT(":X4"), (int *) data[3])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedeventqueue_delete_more, MT(":X5"), (uint *) data[4])) {
			return IDO_ERROR;
		}

		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_timedeventqueue_delete_more)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_timedeventqueue_delete_more() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */


#endif /* Oracle ocilib specific */

		/* if we are executing a low-priority event, remove older events from the queue, as we know they've already been executed */
		/* THIS IS A HACK!  It shouldn't be necessary, but for some reason it is...  Otherwise not all events are removed from the queue. :-( */
		if (type == NEBTYPE_TIMEDEVENT_EXECUTE && (event_type == EVENT_SERVICE_CHECK || event_type == EVENT_HOST_CHECK)) {

			/* clear entries from db */
#ifdef USE_LIBDBI /* everything else will be libdbi */

			if (asprintf(&buf, "DELETE FROM %s WHERE instance_id=%lu AND scheduled_time<%s",
			             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEDEVENTQUEUE],
			             idi->dbinfo.instance_id, ts[1]) == -1)
				buf = NULL;
			result = ido2db_db_query(idi, buf);

			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
			free(buf);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

			/* check if we lost connection, and reconnect */
			if (ido2db_db_reconnect(idi) == IDO_ERROR)
				return IDO_ERROR;

			data[0] = (void *) &idi->dbinfo.instance_id;
			data[1] = (void *) &run_time;

			if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedeventqueue_delete, MT(":X1"), (uint *) data[0])) {
				return IDO_ERROR;
			}
			if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timedeventqueue_delete, MT(":X2"), (uint *) data[1])) { /* unixtimestamp instead of time2sql */
				return IDO_ERROR;
			}

			/* execute statement */
			if (!OCI_Execute(idi->dbinfo.oci_statement_timedeventqueue_delete)) {
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_timedeventqueue_delete() execute error\n");
				return IDO_ERROR;
			}

			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);

			/* do not free statement yet! */

#endif /* Oracle ocilib specific */

		}

	}

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timedeventdata() end\n");

	return IDO_OK;
}

int ido2db_handle_logdata(ido2db_idi *idi) {
	int result = IDO_OK;
	int type, flags, attr;
	struct timeval tstamp;
	time_t etime = 0L;
	unsigned long letype = 0L;
	char *ts[2];
	char *es[1];
	char *buf = NULL;
	int len = 0;
	int x = 0;

#ifdef USE_ORACLE
	unsigned long n_one = 1L;
	void *data[8];
	OCI_Lob * lob_i;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_logdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* convert data */
	result = ido2db_convert_string_to_unsignedlong(
	             idi->buffered_input[IDO_DATA_LOGENTRYTYPE], &letype);
	result = ido2db_convert_string_to_unsignedlong(
	             idi->buffered_input[IDO_DATA_LOGENTRYTIME],
	             (unsigned long *) &etime);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, etime);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_LOGENTRY]);

	/* strip newline chars from end */
	len = strlen(es[0]);
	for (x = len - 1; x >= 0; x--) {
		if (es[0][x] == '\n')
			es[0][x] = '\x0';
		else
			break;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	/* save entry to db */
        switch (idi->dbinfo.server_type) {
        case IDO2DB_DBSERVER_PGSQL:
	        if (asprintf(&buf, "INSERT INTO %s (instance_id, logentry_time, entry_time, entry_time_usec, logentry_type, logentry_data, realtime_data, inferred_data_extracted) VALUES (%lu, %s, %s, %lu, %lu, E'%s', '1', '1')",
	                     ido2db_db_tablenames[IDO2DB_DBTABLE_LOGENTRIES],
	                     idi->dbinfo.instance_id, ts[1], ts[0], tstamp.tv_usec, letype,
	                     es[0]) == -1)
	                buf = NULL;
                break;
        default:
	        if (asprintf(&buf, "INSERT INTO %s (instance_id, logentry_time, entry_time, entry_time_usec, logentry_type, logentry_data, realtime_data, inferred_data_extracted) VALUES (%lu, %s, %s, %lu, %lu, '%s', '1', '1')",
	                     ido2db_db_tablenames[IDO2DB_DBTABLE_LOGENTRIES],
	                     idi->dbinfo.instance_id, ts[1], ts[0], tstamp.tv_usec, letype,
	                     es[0]) == -1)
	                buf = NULL;
                break;
        }

	result = ido2db_db_query(idi, buf);
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &etime;
	data[2] = (void *) &tstamp.tv_sec;
	data[3] = (void *) &tstamp.tv_usec;
	data[4] = (void *) &letype;
	data[5] = (void *) &es[0];
	data[6] = (void *) &n_one;
	data[7] = (void *) &n_one;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_logdata() data array\n");

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X2"), (uint *) data[1])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X3"), (uint *) data[2])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X5"), (uint *) data[4])) {
		return IDO_ERROR;
	}

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_logentries_insert, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}

	//bind clob
	lob_i = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_logentries_insert() bind clob\n");
	result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_logentries_insert, ":X6", *(char **)data[5], &lob_i);
	if (result == IDO_OK) {
		/* execute statement */
		result = OCI_Execute(idi->dbinfo.oci_statement_logentries_insert) ? IDO_OK : IDO_ERROR;
		if (result == IDO_OK) {
			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_logentries_insert() executed\n");
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_logentries_insert() execute error\n");

		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_logentries_insert() bind clob error\n");
	}
	//free lobs
	if (lob_i) OCI_LobFree(lob_i);

	/* do not free statement yet! */
#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_logdata() query ok\n");

	free(buf);

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_logdata() end\n");

	return result;
}

int ido2db_handle_systemcommanddata(ido2db_idi *idi) {
	int type, flags, attr;
	int x = 0;
	struct timeval tstamp;
	struct timeval start_time;
	struct timeval end_time;
	int timeout = 0;
	int early_timeout = 0;
	double execution_time = 0.0;
	int return_code = 0;
	char *ts[2];
	char *es[3];
	int result = IDO_OK;
	void *data[14];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_systemcommanddata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* covert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_TIMEOUT], &timeout);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EARLYTIMEOUT], &early_timeout);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_RETURNCODE], &return_code);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_EXECUTIONTIME], &execution_time);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_STARTTIME], &start_time);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_ENDTIME], &end_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDLINE]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_OUTPUT]);
	es[2] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_LONGOUTPUT]);

	ts[0] = ido2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &ts[0];
	data[2] = (void *) &start_time.tv_usec;
	data[3] = (void *) &ts[1];
	data[4] = (void *) &end_time.tv_usec;
	data[5] = (void *) &es[0];
	data[6] = (void *) &timeout;
	data[7] = (void *) &early_timeout;
	data[8] = (void *) &execution_time;
	data[9] = (void *) &return_code;
	data[10] = (void *) &es[1];
	data[11] = (void *) &es[2];
	/* bind params */
	data[12] = (void *) &start_time.tv_sec;
	data[13] = (void *) &end_time.tv_sec;

	result = ido2db_query_insert_or_update_systemcommanddata_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_systemcommanddata() end\n");

	return IDO_OK;
}

int ido2db_handle_eventhandlerdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	char *ts[2];
	char *es[4];
	int x = 0;
	int eventhandler_type = 0;
	int state = 0;
	int state_type = 0;
	struct timeval start_time;
	struct timeval end_time;
	int timeout = 0;
	int early_timeout = 0;
	double execution_time = 0.0;
	int return_code = 0;
	unsigned long object_id = 0L;
	unsigned long command_id = 0L;
	int result = IDO_OK;
	void *data[20];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_eventhandlerdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* covert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EVENTHANDLERTYPE], &eventhandler_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATE], &state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATETYPE], &state_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_TIMEOUT], &timeout);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EARLYTIMEOUT], &early_timeout);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_RETURNCODE], &return_code);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_EXECUTIONTIME], &execution_time);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_STARTTIME], &start_time);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_ENDTIME], &end_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDARGS]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDLINE]);
	es[2] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_OUTPUT]);
	es[3] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_LONGOUTPUT]);

	ts[0] = ido2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the object id */
	if (eventhandler_type == SERVICE_EVENTHANDLER || eventhandler_type == GLOBAL_SERVICE_EVENTHANDLER)
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOST], idi->buffered_input[IDO_DATA_SERVICE], &object_id);
	else
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);

	/* get the command id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, idi->buffered_input[IDO_DATA_COMMANDNAME], NULL, &command_id);

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &eventhandler_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &state;
	data[4] = (void *) &state_type;
	data[5] = (void *) &ts[0];
	data[6] = (void *) &start_time.tv_usec;
	data[7] = (void *) &ts[1];
	data[8] = (void *) &end_time.tv_usec;
	data[9] = (void *) &command_id;
	data[10] = (void *) &es[0];
	data[11] = (void *) &es[1];
	data[12] = (void *) &timeout;
	data[13] = (void *) &early_timeout;
	data[14] = (void *) &execution_time;
	data[15] = (void *) &return_code;
	data[16] = (void *) &es[2];
	data[17] = (void *) &es[3];
	/* bind params */
	data[18] = (void *) &start_time.tv_sec;
	data[19] = (void *) &end_time.tv_sec;

	result = ido2db_query_insert_or_update_eventhandlerdata_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */


	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_eventhandlerdata() end\n");

	return IDO_OK;
}

int ido2db_handle_notificationdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int notification_type = 0;
	int notification_reason = 0;
	unsigned long object_id = 0L;
	struct timeval start_time;
	struct timeval end_time;
	int state = 0;
	int escalated = 0;
	int contacts_notified = 0;
	int result = IDO_OK;
	char *ts[2];
	char *es[2];
	int x = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif
#ifdef USE_ORACLE
	char *seq_name = NULL;
#endif

	void *data[15];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFICATIONTYPE], &notification_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFICATIONREASON], &notification_reason);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATE], &state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ESCALATED], &escalated);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CONTACTSNOTIFIED], &contacts_notified);

	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_STARTTIME], &start_time);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_ENDTIME], &end_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_OUTPUT]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_LONGOUTPUT]);

	ts[0] = ido2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the object id */
	if (notification_type == SERVICE_NOTIFICATION)
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOST], idi->buffered_input[IDO_DATA_SERVICE], &object_id);
	if (notification_type == HOST_NOTIFICATION)
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &notification_type;
	data[2] = (void *) &notification_reason;
	data[3] = (void *) &ts[0];
	data[4] = (void *) &start_time.tv_sec;
	data[5] = (void *) &ts[1];
	data[6] = (void *) &end_time.tv_usec;
	data[7] = (void *) &object_id;
	data[8] = (void *) &state;
	data[9] = (void *) &es[0];
	data[10] = (void *) &es[1];
	data[11] = (void *) &escalated;
	data[12] = (void *) &contacts_notified;
	/* bind params */
	data[13] = (void *) &start_time.tv_sec;
	data[14] = (void *) &end_time.tv_sec;

	result = ido2db_query_insert_or_update_notificationdata_add(idi, data);

	/* save the notification id for later use... */
	if (type == NEBTYPE_NOTIFICATION_START)
		idi->dbinfo.last_notification_id = 0L;
	if (result == IDO_OK && type == NEBTYPE_NOTIFICATION_START) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			idi->dbinfo.last_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata(%lu) last_notification_id\n", idi->dbinfo.last_notification_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_notification_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS]) == -1)
				buf = NULL;

			idi->dbinfo.last_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata(%s=%lu) last_notification_id\n", buf, idi->dbinfo.last_notification_id);
			free(buf);
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

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_notifications") == -1)
			seq_name = NULL;
		idi->dbinfo.last_notification_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata(%lu) last_notification_id\n", idi->dbinfo.last_notification_id);
		free(seq_name);

#endif /* Oracle ocilib specific */
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata() end\n");

	return IDO_OK;
}

int ido2db_handle_contactnotificationdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long contact_id = 0L;
	struct timeval start_time;
	struct timeval end_time;
	int result = IDO_OK;
	char *ts[2];
	int x = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif
#ifdef USE_ORACLE
	char *seq_name = NULL;
#endif
	void *data[9];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* convert vars */

	result = ido2db_convert_string_to_timeval(
	             idi->buffered_input[IDO_DATA_STARTTIME], &start_time);
	result = ido2db_convert_string_to_timeval(
	             idi->buffered_input[IDO_DATA_ENDTIME], &end_time);

	ts[0] = ido2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the contact id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_CONTACT,
	         idi->buffered_input[IDO_DATA_CONTACTNAME], NULL, &contact_id);

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->dbinfo.last_notification_id;
	data[2] = (void *) &ts[0];
	data[3] = (void *) &start_time.tv_usec;
	data[4] = (void *) &ts[1];
	data[5] = (void *) &end_time.tv_usec;
	data[6] = (void *) &contact_id;
	/* bind params */
	data[7] = (void *) &start_time.tv_sec;
	data[8] = (void *) &end_time.tv_sec;

	result = ido2db_query_insert_or_update_contactnotificationdata_add(idi, data);

	/* save the contact notification id for later use... */
	if (type == NEBTYPE_CONTACTNOTIFICATION_START)
		idi->dbinfo.last_contact_notification_id = 0L;
	if (result == IDO_OK && type == NEBTYPE_CONTACTNOTIFICATION_START) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			idi->dbinfo.last_contact_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata(%lu) contactnotification_id\n", idi->dbinfo.last_contact_notification_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_contactnotification_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS]) == -1)
				buf = NULL;

			idi->dbinfo.last_contact_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata(%s=%lu) contactnotification_id\n", buf, idi->dbinfo.last_contact_notification_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_contactnotifications") == -1)
			seq_name = NULL;
		idi->dbinfo.last_contact_notification_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata(%lu) \n", idi->dbinfo.last_contact_notification_id);
		free(seq_name);

#endif /* Oracle ocilib specific */
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata() end\n");

	return IDO_OK;
}

int ido2db_handle_contactnotificationmethoddata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long command_id = 0L;
	struct timeval start_time;
	struct timeval end_time;
	int result = IDO_OK;
	char *ts[2];
	char *es[1];
	int x = 0;
	void *data[10];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationmethoddata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* convert vars */

	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_STARTTIME], &start_time);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_ENDTIME], &end_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDARGS]);

	ts[0] = ido2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the command id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND,
	         idi->buffered_input[IDO_DATA_COMMANDNAME], NULL, &command_id);

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->dbinfo.last_contact_notification_id;
	data[2] = (void *) &ts[0];
	data[3] = (void *) &start_time.tv_usec;
	data[4] = (void *) &ts[1];
	data[5] = (void *) &end_time.tv_usec;
	data[6] = (void *) &command_id;
	data[7] = (void *) &es[0];
	/* bind params */
	data[8] = (void *) &start_time.tv_sec;
	data[9] = (void *) &end_time.tv_sec;

	result = ido2db_query_insert_or_update_contactnotificationmethoddata_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationmethoddata() end\n");

	return IDO_OK;
}

int ido2db_handle_servicecheckdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	char *ts[2];
	char *es[5];
	int check_type = 0;
	struct timeval start_time;
	struct timeval end_time;
	int current_check_attempt = 0;
	int max_check_attempts = 0;
	int state = 0;
	int state_type = 0;
	int timeout = 0;
	int early_timeout = 0;
	double execution_time = 0.0;
	double latency = 0.0;
	int return_code = 0;
	unsigned long object_id = 0L;
	unsigned long command_id = 0L;
	void *data[24];

	int x = 0;
	int result = IDO_OK;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicecheckdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* only process finished service checks... */
	if (type != NEBTYPE_SERVICECHECK_PROCESSED)
		return IDO_OK;

	/* only process some types of service checks... */
	if (type != NEBTYPE_SERVICECHECK_INITIATE && type != NEBTYPE_SERVICECHECK_PROCESSED)
		return IDO_OK;

	/* skip precheck events - they aren't useful to us */
	if (type == NEBTYPE_SERVICECHECK_ASYNC_PRECHECK)
		return IDO_OK;

	/* covert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CHECKTYPE], &check_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CURRENTCHECKATTEMPT], &current_check_attempt);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_MAXCHECKATTEMPTS], &max_check_attempts);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATE], &state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATETYPE], &state_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_TIMEOUT], &timeout);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EARLYTIMEOUT], &early_timeout);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_RETURNCODE], &return_code);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_EXECUTIONTIME], &execution_time);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_LATENCY], &latency);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_STARTTIME], &start_time);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_ENDTIME], &end_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDARGS]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDLINE]);
	es[2] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_OUTPUT]);
	es[3] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_LONGOUTPUT]);
	es[4] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_PERFDATA]);

	ts[0] = ido2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE,
	         idi->buffered_input[IDO_DATA_HOST],
	         idi->buffered_input[IDO_DATA_SERVICE], &object_id);

	/* get the command id */
	if (idi->buffered_input[IDO_DATA_COMMANDNAME] != NULL && strcmp(idi->buffered_input[IDO_DATA_COMMANDNAME], ""))
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, idi->buffered_input[IDO_DATA_COMMANDNAME], NULL, &command_id);
	else
		command_id = 0L;

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &object_id;
	data[2] = (void *) &check_type;
	data[3] = (void *) &current_check_attempt;
	data[4] = (void *) &max_check_attempts;
	data[5] = (void *) &state;
	data[6] = (void *) &state_type;
	data[7] = (void *) &ts[0];
	data[8] = (void *) &start_time.tv_usec;
	data[9] = (void *) &ts[1];
	data[10] = (void *) &end_time.tv_usec;
	data[11] = (void *) &timeout;
	data[12] = (void *) &early_timeout;
	data[13] = (void *) &execution_time;
	data[14] = (void *) &latency;
	data[15] = (void *) &return_code;
	data[16] = (void *) &es[2];
	data[17] = (void *) &es[3];
	data[18] = (void *) &es[4];
	data[19] = (void *) &command_id;
	data[20] = (void *) &es[0];
	data[21] = (void *) &es[1];
	/* add unixtime for bind params */
	data[22] = (void *) &start_time.tv_sec;
	data[23] = (void *) &end_time.tv_sec;

	result = ido2db_query_insert_servicecheckdata_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* do not free if prepared statement */

#endif /* Oracle ocilib specific */

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicecheckdata() end\n");

	return IDO_OK;
}

int ido2db_handle_hostcheckdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	char *ts[2];
	char *es[5];
	int check_type = 0;
	int is_raw_check = 0;
	struct timeval start_time;
	struct timeval end_time;
	int current_check_attempt = 0;
	int max_check_attempts = 0;
	int state = 0;
	int state_type = 0;
	int timeout = 0;
	int early_timeout = 0;
	double execution_time = 0.0;
	double latency = 0.0;
	int return_code = 0;
	unsigned long object_id = 0L;
	unsigned long command_id = 0L;
	int x = 0;
	int result = IDO_OK;
	void *data[25];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostcheckdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* only process finished host checks... */
	if (type != NEBTYPE_HOSTCHECK_PROCESSED)
		return IDO_OK;

	/* skip precheck events - they aren't useful to us */
	if (type == NEBTYPE_HOSTCHECK_ASYNC_PRECHECK || type == NEBTYPE_HOSTCHECK_SYNC_PRECHECK)
		return IDO_OK;

	/* covert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CHECKTYPE], &check_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CURRENTCHECKATTEMPT], &current_check_attempt);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_MAXCHECKATTEMPTS], &max_check_attempts);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATE], &state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATETYPE], &state_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_TIMEOUT], &timeout);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EARLYTIMEOUT], &early_timeout);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_RETURNCODE], &return_code);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_EXECUTIONTIME], &execution_time);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_LATENCY], &latency);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_STARTTIME], &start_time);
	result = ido2db_convert_string_to_timeval(idi->buffered_input[IDO_DATA_ENDTIME], &end_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDARGS]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDLINE]);
	es[2] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_OUTPUT]);
	es[3] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_LONGOUTPUT]);
	es[4] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_PERFDATA]);

	ts[0] = ido2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);

	/* get the command id */
	if (idi->buffered_input[IDO_DATA_COMMANDNAME] != NULL && strcmp(idi->buffered_input[IDO_DATA_COMMANDNAME], ""))
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, idi->buffered_input[IDO_DATA_COMMANDNAME], NULL, &command_id);
	else
		command_id = 0L;

	/* is this a raw check? */
	if (type == NEBTYPE_HOSTCHECK_RAW_START || type == NEBTYPE_HOSTCHECK_RAW_END)
		is_raw_check = 1;
	else
		is_raw_check = 0;

	/* save entry to db */
	data[0] = (void *) &command_id;
	data[1] = (void *) &es[0];
	data[2] = (void *) &es[1];
	data[3] = (void *) &idi->dbinfo.instance_id;
	data[4] = (void *) &object_id;
	data[5] = (void *) &check_type;
	data[6] = (void *) &is_raw_check;
	data[7] = (void *) &current_check_attempt;
	data[8] = (void *) &max_check_attempts;
	data[9] = (void *) &state;
	data[10] = (void *) &state_type;
	data[11] = (void *) &ts[0];
	data[12] = (void *) &start_time.tv_usec;
	data[13] = (void *) &ts[1];
	data[14] = (void *) &end_time.tv_usec;
	data[15] = (void *) &timeout;
	data[16] = (void *) &early_timeout;
	data[17] = (void *) &execution_time;
	data[18] = (void *) &latency;
	data[19] = (void *) &return_code;
	data[20] = (void *) &es[2];
	data[21] = (void *) &es[3];
	data[22] = (void *) &es[4];
	/* add unixtime for bind params */
	data[23] = (void *) &start_time.tv_sec;
	data[24] = (void *) &end_time.tv_sec;

	result = ido2db_query_insert_hostcheckdata_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* do not free if prepared statement */

#endif /* Oracle ocilib specific */

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostcheckdata() end\n");

	return IDO_OK;
}

int ido2db_handle_commentdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int comment_type = 0;
	int entry_type = 0;
	unsigned long object_id = 0L;
	unsigned long comment_time = 0L;
	unsigned long internal_comment_id = 0L;
	int is_persistent = 0;
	int comment_source = 0;
	int expires = 0;
	unsigned long expire_time = 0L;
	int result = IDO_OK;
	char *ts[3];
	char *es[2];
	int x = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_commentdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_COMMENTTYPE], &comment_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ENTRYTYPE], &entry_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PERSISTENT], &is_persistent);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SOURCE], &comment_source);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EXPIRES], &expires);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_COMMENTID], &internal_comment_id);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_ENTRYTIME], &comment_time);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_EXPIRATIONTIME], &expire_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_AUTHORNAME]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMENT]);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, comment_time);
	ts[2] = ido2db_db_timet_to_sql(idi, expire_time);

	/* get the object id */
	if (comment_type == SERVICE_COMMENT)
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOST], idi->buffered_input[IDO_DATA_SERVICE], &object_id);

	if (comment_type == HOST_COMMENT)
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);

	/* ADD HISTORICAL COMMENTS */
	/* save a record of comments that get added (or get loaded and weren't previously recorded).... */
	if (type == NEBTYPE_COMMENT_ADD || type == NEBTYPE_COMMENT_LOAD) {

		/* save entry to db */
		void *data[17];
		data[0] = (void *) &ts[0];
		data[1] = (void *) &tstamp.tv_usec;
		data[2] = (void *) &idi->dbinfo.instance_id;
		data[3] = (void *) &comment_type;
		data[4] = (void *) &entry_type;
		data[5] = (void *) &object_id;
		data[6] = (void *) &ts[1];
		data[7] = (void *) &internal_comment_id;
		data[8] = (void *) &es[0];
		data[9] = (void *) &es[1];
		data[10] = (void *) &is_persistent;
		data[11] = (void *) &comment_source;
		data[12] = (void *) &expires;
		data[13] = (void *) &ts[2];
		/* bind params */
		data[14] = &tstamp.tv_sec;
		data[15] = &comment_time;
		data[16] = &expire_time;

		result = ido2db_query_insert_or_update_commentdata_history_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	}

	/* UPDATE HISTORICAL COMMENTS */
	/* mark records that have been deleted */
	if (type == NEBTYPE_COMMENT_DELETE) {

		/* update db entry */
#ifdef USE_LIBDBI /* everything else will be libdbi */

		if (asprintf(
		            &buf,
		            "UPDATE %s SET deletion_time=%s, deletion_time_usec=%lu WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
		            ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTHISTORY], ts[0],
		            tstamp.tv_usec, idi->dbinfo.instance_id, ts[1],
		            internal_comment_id) == -1)
			buf = NULL;
		result = ido2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		void *data[5];

		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		data[0] = (void *) &tstamp.tv_sec;
		data[1] = (void *) &tstamp.tv_usec;
		data[2] = (void *) &idi->dbinfo.instance_id;
		data[3] = (void *) &comment_time;
		data[4] = (void *) &internal_comment_id;

		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_comment_history_update, MT(":X1"), (uint *) data[0])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_comment_history_update, MT(":X2"), (uint *) data[1])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_comment_history_update, MT(":X3"), (uint *) data[2])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_comment_history_update, MT(":X4"), (uint *) data[3])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_comment_history_update, MT(":X5"), (uint *) data[4])) {
			return IDO_ERROR;
		}

		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_comment_history_update)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_comment_history_update() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	}

	/* ADD CURRENT COMMENTS */
	if ((type == NEBTYPE_COMMENT_ADD || type == NEBTYPE_COMMENT_LOAD)
	        && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db */
		void *data[17];
		data[0] = (void *) &ts[0];
		data[1] = (void *) &tstamp.tv_usec;
		data[2] = (void *) &idi->dbinfo.instance_id;
		data[3] = (void *) &comment_type;
		data[4] = (void *) &entry_type;
		data[5] = (void *) &object_id;
		data[6] = (void *) &ts[1];
		data[7] = (void *) &internal_comment_id;
		data[8] = (void *) &es[0];
		data[9] = (void *) &es[1];
		data[10] = (void *) &is_persistent;
		data[11] = (void *) &comment_source;
		data[12] = (void *) &expires;
		data[13] = (void *) &ts[2];
		/* bind params */
		data[14] = &tstamp.tv_sec;
		data[15] = &comment_time;
		data[16] = &expire_time;

		result = ido2db_query_insert_or_update_commentdata_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	}

	/* REMOVE CURRENT COMMENTS */
	if (type == NEBTYPE_COMMENT_DELETE && tstamp.tv_sec
	        >= idi->dbinfo.latest_realtime_data_time) {

		/* clear entry from db */
#ifdef USE_LIBDBI /* everything else will be libdbi */

		if (asprintf(
		            &buf,
		            "DELETE FROM %s WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
		            ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS],
		            idi->dbinfo.instance_id, ts[1], internal_comment_id) == -1)
			buf = NULL;
		result = ido2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		void *data[3];
		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &comment_time;
		data[2] = (void *) &internal_comment_id;

		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_comments_delete, MT(":X1"), (uint *) data[0])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_comments_delete, MT(":X2"), (uint *) data[1])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_comments_delete, MT(":X3"), (uint *) data[2])) {
			return IDO_ERROR;
		}

		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_comments_delete)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_comments_delete() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */


#endif /* Oracle ocilib specific */

	}

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_commentdata() end\n");

	return IDO_OK;
}

int ido2db_handle_downtimedata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int downtime_type = 0;
	int fixed = 0;
	unsigned long duration = 0L;
	unsigned long internal_downtime_id = 0L;
	unsigned long triggered_by = 0L;
	unsigned long entry_time = 0L;
	unsigned long start_time = 0L;
	unsigned long end_time = 0L;
	unsigned long object_id = 0L;
	int is_in_effect = 0L;
	unsigned long trigger_time = 0L;
	int result = IDO_OK;
	char *ts[5];
	char *es[2];
	int x = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif

#ifdef USE_ORACLE
	unsigned long was_started = 1;
#endif
	void *data[18];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_downtimedata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_DOWNTIMETYPE], &downtime_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FIXED], &fixed);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_DOWNTIMEISINEFFECT], &is_in_effect);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_DURATION], &duration);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_DOWNTIMEID], &internal_downtime_id);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_TRIGGEREDBY], &triggered_by);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_ENTRYTIME], &entry_time);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_STARTTIME], &start_time);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_ENDTIME], &end_time);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_DOWNTIMETRIGGERTIME], &trigger_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_AUTHORNAME]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMENT]);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, entry_time);
	ts[2] = ido2db_db_timet_to_sql(idi, start_time);
	ts[3] = ido2db_db_timet_to_sql(idi, end_time);
	ts[4] = ido2db_db_timet_to_sql(idi, trigger_time);

	/* get the object id */
	if (downtime_type == SERVICE_DOWNTIME)
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOST],
		         idi->buffered_input[IDO_DATA_SERVICE], &object_id);
	if (downtime_type == HOST_DOWNTIME)
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST,
		         idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);

	/* HISTORICAL DOWNTIME */

	/* save a record of scheduled downtime that gets added (or gets loaded and wasn't previously recorded).... */
	if (type == NEBTYPE_DOWNTIME_ADD || type == NEBTYPE_DOWNTIME_LOAD) {

		/* save entry to db */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &downtime_type;
		data[2] = (void *) &object_id;
		data[3] = (void *) &ts[1];
		data[4] = (void *) &es[0];
		data[5] = (void *) &es[1];
		data[6] = (void *) &internal_downtime_id;
		data[7] = (void *) &triggered_by;
		data[8] = (void *) &fixed;
		data[9] = (void *) &duration;
		data[10] = (void *) &ts[2];
		data[11] = (void *) &ts[3];
		/* bind params */
		data[12] = (void *) &entry_time;
		data[13] = (void *) &start_time;
		data[14] = (void *) &end_time;
		/* is_in_effect + trigger_time as char* and ulong */
		data[15] = (void *) &is_in_effect;
		data[16] = (void *) &ts[4];
		data[17] = (void *) &trigger_time;


		result = ido2db_query_insert_or_update_downtimedata_downtime_history_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

		if (enable_sla)
			sla_process_downtime_history(idi, object_id, start_time, end_time);
	}

	/* save a record of scheduled downtime that starts */
	if (type == NEBTYPE_DOWNTIME_START) {
		/* start means downtime in effect as well, fake it */
		is_in_effect = 1;

		if (enable_sla)
			sla_process_downtime(idi, object_id, tstamp.tv_sec, type);

		/* save entry to db */
#ifdef USE_LIBDBI /* everything else will be libdbi */

		if (asprintf(&buf,
		             "UPDATE %s SET "
			     "actual_start_time=%s, actual_start_time_usec=%lu, was_started=%d, is_in_effect=%d, trigger_time=%s "
			     "WHERE instance_id=%lu AND downtime_type=%d AND object_id=%lu AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s AND was_started=0"
		             , ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]
			     , ts[0], tstamp.tv_usec, 1, is_in_effect, ts[4]
			     , idi->dbinfo.instance_id, downtime_type, object_id, ts[1], ts[2], ts[3]
			     ) == -1)
			buf = NULL;

		result = ido2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		was_started = 1;
		data[0] = (void *) &tstamp.tv_sec;
		data[1] = (void *) &tstamp.tv_usec;
		data[2] = (void *) &was_started;
		data[3] = (void *) &idi->dbinfo.instance_id;
		data[4] = (void *) &downtime_type;
		data[5] = (void *) &object_id;
		data[6] = (void *) &entry_time;
		data[7] = (void *) &start_time;
		data[8] = (void *) &end_time;
		/* is_in_effect + trigger_time as char* and ulong */
		data[9] = (void *) &is_in_effect;
		data[10] = (void *) &trigger_time;

		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X1"), (uint *) data[0])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X2"), (uint *) data[1])) {
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X3"), (int *) data[2])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X4"), (uint *) data[3])) {
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X5"), (int *) data[4])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X6"), (uint *) data[5])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X7"), (uint *) data[6])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X8"), (uint *) data[7])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X9"), (uint *) data[8])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X10"), (int *) data[9])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_start, MT(":X11"), (uint *) data[10])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}


		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_downtimehistory_update_start)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_downtimehistory_update_start() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */


#endif /* Oracle ocilib specific */

	}

	/* save a record of scheduled downtime that ends */
	if (type == NEBTYPE_DOWNTIME_STOP) {
		if (enable_sla)
			sla_process_downtime(idi, object_id, tstamp.tv_sec, type);

		/* save entry to db */
#ifdef USE_LIBDBI /* everything else will be libdbi */

		if (asprintf(
		            &buf,
		            "UPDATE %s "
			    "SET actual_end_time=%s, actual_end_time_usec=%lu, was_cancelled=%d, is_in_effect=%d, trigger_time=%s "
			    "WHERE instance_id=%lu AND downtime_type=%d AND object_id=%lu AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s"
		            ,ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]
			    , ts[0], tstamp.tv_usec, (attr == NEBATTR_DOWNTIME_STOP_CANCELLED) ? 1 : 0, is_in_effect, ts[4]
			    , idi->dbinfo.instance_id, downtime_type, object_id, ts[1], ts[2], ts[3]
			    ) == -1)
			buf = NULL;

		result = ido2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		int was_cancelled;

		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		if (attr == NEBATTR_DOWNTIME_STOP_CANCELLED) {
			was_cancelled = 1;
		} else {
			was_cancelled = 0;
		}

		data[0] = (void *) &tstamp.tv_sec;
		data[1] = (void *) &tstamp.tv_usec;
		data[2] = (void *) &was_cancelled;
		data[3] = (void *) &idi->dbinfo.instance_id;
		data[4] = (void *) &downtime_type;
		data[5] = (void *) &object_id;
		data[6] = (void *) &entry_time;
		data[7] = (void *) &start_time;
		data[8] = (void *) &end_time;
                /* is_in_effect + trigger_time */
                data[9] = (void *) &is_in_effect;
                data[10] = (void *) &trigger_time;

		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X1"), (uint *) data[0])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X2"), (uint *) data[1])) {
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X3"), (int *) data[2])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X4"), (uint *) data[3])) {
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X5"), (int *) data[4])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X6"), (uint *) data[5])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X7"), (uint *) data[6])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X8"), (uint *) data[7])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X9"), (uint *) data[8])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X10"), (int *) data[9])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimehistory_update_stop, MT(":X11"), (uint *) data[10])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}


		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_downtimehistory_update_stop)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_downtimehistory_update_stop() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	}

	/* CURRENT DOWNTIME */

	/* save a record of scheduled downtime that gets added (or gets loaded and wasn't previously recorded).... */
	if ((type == NEBTYPE_DOWNTIME_ADD || type == NEBTYPE_DOWNTIME_LOAD)
	        && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &downtime_type;
		data[2] = (void *) &object_id;
		data[3] = (void *) &ts[1];
		data[4] = (void *) &es[0];
		data[5] = (void *) &es[1];
		data[6] = (void *) &internal_downtime_id;
		data[7] = (void *) &triggered_by;
		data[8] = (void *) &fixed;
		data[9] = (void *) &duration;
		data[10] = (void *) &ts[2];
		data[11] = (void *) &ts[3];
		/* bind params */
		data[12] = (void *) &entry_time;
		data[13] = (void *) &start_time;
		data[14] = (void *) &end_time;
                /* is_in_effect + trigger_time as char* and ulong */
                data[15] = (void *) &is_in_effect;
                data[16] = (void *) &ts[4];
                data[17] = (void *) &trigger_time;

		result = ido2db_query_insert_or_update_downtimedata_scheduled_downtime_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	}

	/* save a record of scheduled downtime that starts */
	if (type == NEBTYPE_DOWNTIME_START && tstamp.tv_sec
	        >= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db */
#ifdef USE_LIBDBI /* everything else will be libdbi */

		if (asprintf(&buf,
		             "UPDATE %s "
			     "SET actual_start_time=%s, actual_start_time_usec=%lu, was_started=%d, is_in_effect=%d, trigger_time=%s "
			     "WHERE instance_id=%lu AND downtime_type=%d AND object_id=%lu AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s AND was_started=0"
		             , ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]
			     , ts[0], tstamp.tv_usec, 1, is_in_effect, ts[4]
			     , idi->dbinfo.instance_id, downtime_type, object_id, ts[1], ts[2], ts[3]
			     ) == -1)
			buf = NULL;

		result = ido2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);

#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		was_started = 1;
		data[0] = (void *) &tstamp.tv_sec;
		data[1] = (void *) &tstamp.tv_usec;
		data[2] = (void *) &was_started;
		data[3] = (void *) &idi->dbinfo.instance_id;
		data[4] = (void *) &downtime_type;
		data[5] = (void *) &object_id;
		data[6] = (void *) &entry_time;
		data[7] = (void *) &start_time;
		data[8] = (void *) &end_time;
                /* is_in_effect + trigger_time */
                data[9] = (void *) &is_in_effect;
                data[10] = (void *) &trigger_time;


		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X1"), (uint *) data[0])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X2"), (uint *) data[1])) {
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X3"), (int *) data[2])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X4"), (uint *) data[3])) {
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X5"), (int *) data[4])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X6"), (uint *) data[5])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X7"), (uint *) data[6])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X8"), (uint *) data[7])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X9"), (uint *) data[8])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X10"), (int *) data[9])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_scheduleddowntime_update_start, MT(":X11"), (uint *) data[10])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}


		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_scheduleddowntime_update_start)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_scheduleddowntime_update_start() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */



#endif /* Oracle ocilib specific */

	}

	/* remove completed or deleted downtime */
	if ((type == NEBTYPE_DOWNTIME_STOP || type == NEBTYPE_DOWNTIME_DELETE)
	        && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db */
#ifdef USE_LIBDBI /* everything else will be libdbi */

		if (asprintf(&buf,
		             "DELETE FROM %s WHERE instance_id=%lu AND downtime_type=%d AND object_id=%lu AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s",
		             ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME],
		             idi->dbinfo.instance_id, downtime_type, object_id, ts[1],
		             ts[2], ts[3]) == -1)
			buf = NULL;

		result = ido2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		free(buf);
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* check if we lost connection, and reconnect */
		if (ido2db_db_reconnect(idi) == IDO_ERROR)
			return IDO_ERROR;

		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &downtime_type;
		data[2] = (void *) &object_id;
		data[3] = (void *) &entry_time;
		data[4] = (void *) &start_time;
		data[5] = (void *) &end_time;

		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtime_delete, MT(":X1"), (uint *) data[0])) {
			return IDO_ERROR;
		}
		if (!OCI_BindInt(idi->dbinfo.oci_statement_downtime_delete, MT(":X2"), (int *) data[1])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtime_delete, MT(":X3"), (uint *) data[2])) {
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtime_delete, MT(":X4"), (uint *) data[3])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtime_delete, MT(":X5"), (uint *) data[4])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtime_delete, MT(":X6"), (uint *) data[5])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}

		/* execute statement */
		if (!OCI_Execute(idi->dbinfo.oci_statement_downtime_delete)) {
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_downtime_delete() execute error\n");
			return IDO_ERROR;
		}

		/* commit statement */
		OCI_Commit(idi->dbinfo.oci_connection);

		/* do not free statement yet! */


#endif /* Oracle ocilib specific */


	}

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_downtimedata() end\n");

	return IDO_OK;
}

int ido2db_handle_flappingdata(ido2db_idi *idi) {
	int type, flags, attr;
	int x = 0;
	struct timeval tstamp;
	int flapping_type = 0;
	unsigned long object_id = 0L;
	double percent_state_change = 0.0;
	double low_threshold = 0.0;
	double high_threshold = 0.0;
	unsigned long comment_time = 0L;
	unsigned long internal_comment_id = 0L;
	int result = IDO_OK;
	char *ts[2];
	char *buf = NULL;

#ifdef USE_ORACLE
	void *data[12];
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_flappingdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPPINGTYPE], &flapping_type);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_PERCENTSTATECHANGE], &percent_state_change);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_LOWTHRESHOLD], &low_threshold);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_HIGHTHRESHOLD], &high_threshold);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_COMMENTTIME], &comment_time);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_COMMENTID], &internal_comment_id);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, comment_time);

	/* get the object id (if applicable) */
	if (flapping_type == SERVICE_FLAPPING)
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOST],
		         idi->buffered_input[IDO_DATA_SERVICE], &object_id);
	if (flapping_type == HOST_FLAPPING)
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST,
		         idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);

	/* save entry to db */
#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (asprintf(
	            &buf,
	            "INSERT INTO %s (instance_id, event_time, event_time_usec, event_type, reason_type, flapping_type, object_id, percent_state_change, low_threshold, high_threshold, comment_time, internal_comment_id) VALUES (%lu, %s, %lu, %d, %d, %d, %lu, '%lf', '%lf', '%lf', %s, %lu)",
	            ido2db_db_tablenames[IDO2DB_DBTABLE_FLAPPINGHISTORY],
	            idi->dbinfo.instance_id, ts[0], tstamp.tv_usec, type, attr,
	            flapping_type, object_id, percent_state_change, low_threshold,
	            high_threshold, ts[1], internal_comment_id) == -1)
		buf = NULL;
	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &tstamp.tv_sec;
	data[2] = (void *) &tstamp.tv_usec;
	data[3] = (void *) &type;
	data[4] = (void *) &attr;
	data[5] = (void *) &flapping_type;
	data[6] = (void *) &object_id;
	data[7] = (void *) &percent_state_change;
	data[8] = (void *) &low_threshold;
	data[9] = (void *) &high_threshold;
	data[10] = (void *) &comment_time;
	data[11] = (void *) &internal_comment_id;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_flappinghistory, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_flappinghistory, MT(":X2"), (uint *) data[1])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_flappinghistory, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_flappinghistory, MT(":X4"), (int *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_flappinghistory, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_flappinghistory, MT(":X6"), (int *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_flappinghistory, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_flappinghistory, MT(":X8"), (double *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_flappinghistory, MT(":X9"), (double *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_flappinghistory, MT(":X10"), (double *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_flappinghistory, MT(":X11"), (uint *) data[10])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_flappinghistory, MT(":X12"), (uint *) data[11])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_flappinghistory)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_flappinghistory() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */



#endif /* Oracle ocilib specific */

	free(buf);

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_flappingdata() end\n");

	return IDO_OK;
}

int ido2db_handle_programstatusdata(ido2db_idi *idi) {
	int type, flags, attr;
	int x = 0;
	struct timeval tstamp;
	unsigned long program_start_time = 0L;
	unsigned long process_id = 0L;
	int daemon_mode = 0;
	unsigned long last_command_check = 0L;
	unsigned long last_log_rotation = 0L;
	int notifications_enabled = 0;
	unsigned long disable_notifications_expire_time = 0L;
	int active_service_checks_enabled = 0;
	int passive_service_checks_enabled = 0;
	int active_host_checks_enabled = 0;
	int passive_host_checks_enabled = 0;
	int event_handlers_enabled = 0;
	int flap_detection_enabled = 0;
	int failure_prediction_enabled = 0;
	int process_performance_data = 0;
	int obsess_over_hosts = 0;
	int obsess_over_services = 0;
	unsigned long modified_host_attributes = 0L;
	unsigned long modified_service_attributes = 0L;
	char *ts[5];
	char *es[2];
	int result = IDO_OK;
	void *data[28];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_programstatusdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_programstatusdata() %lu < %lu\n", tstamp.tv_sec, idi->dbinfo.latest_realtime_data_time);
		return IDO_OK;
	}

	/* covert vars */
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_PROGRAMSTARTTIME], &program_start_time);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_PROCESSID], &process_id);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_DAEMONMODE], &daemon_mode);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTCOMMANDCHECK], &last_command_check);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTLOGROTATION], &last_log_rotation);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFICATIONSENABLED], &notifications_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ACTIVESERVICECHECKSENABLED], &active_service_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PASSIVESERVICECHECKSENABLED], &passive_service_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ACTIVEHOSTCHECKSENABLED], &active_host_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PASSIVEHOSTCHECKSENABLED], &passive_host_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EVENTHANDLERSENABLED], &event_handlers_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PROCESSPERFORMANCEDATA], &process_performance_data);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_OBSESSOVERHOSTS], &obsess_over_hosts);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_OBSESSOVERSERVICES], &obsess_over_services);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_MODIFIEDHOSTATTRIBUTES], &modified_host_attributes);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_MODIFIEDSERVICEATTRIBUTES], &modified_service_attributes);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_DISABLED_NOTIFICATIONS_EXPIRE_TIME], &disable_notifications_expire_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_GLOBALHOSTEVENTHANDLER]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_GLOBALSERVICEEVENTHANDLER]);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, program_start_time);
	ts[2] = ido2db_db_timet_to_sql(idi, last_command_check);
	ts[3] = ido2db_db_timet_to_sql(idi, last_log_rotation);
	ts[4] = ido2db_db_timet_to_sql(idi, disable_notifications_expire_time);

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &ts[0];
	data[2] = (void *) &ts[1];
	data[3] = (void *) &process_id;
	data[4] = (void *) &daemon_mode;
	data[5] = (void *) &ts[2];
	data[6] = (void *) &ts[3];
	data[7] = (void *) &notifications_enabled;
	data[8] = (void *) &active_service_checks_enabled;
	data[9] = (void *) &passive_service_checks_enabled;
	data[10] = (void *) &active_host_checks_enabled;
	data[11] = (void *) &passive_host_checks_enabled;
	data[12] = (void *) &event_handlers_enabled;
	data[13] = (void *) &flap_detection_enabled;
	data[14] = (void *) &failure_prediction_enabled;
	data[15] = (void *) &process_performance_data;
	data[16] = (void *) &obsess_over_hosts;
	data[17] = (void *) &obsess_over_services;
	data[18] = (void *) &modified_host_attributes;
	data[19] = (void *) &modified_service_attributes;
	data[20] = (void *) &es[0];
	data[21] = (void *) &es[1];
	/* add unixtime for bind params */
	data[22] = (void *) &tstamp.tv_sec;
	data[23] = (void *) &program_start_time;
	data[24] = (void *) &last_command_check;
	data[25] = (void *) &last_log_rotation;
	/* disabled notifications expiry */
	data[26] = (void *) &ts[4];
	data[27] = (void *) &disable_notifications_expire_time;

	/* save entry to db */
	result = ido2db_query_insert_or_update_programstatusdata_add(idi, data);

	if (result == IDO_ERROR) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_programstatusdata() error\n");
		return result;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* do not free if prepared statement */

#endif /* Oracle ocilib specific */

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_programstatusdata() end\n");

	return IDO_OK;
}

int ido2db_handle_hoststatusdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long last_check = 0L;
	unsigned long next_check = 0L;
	unsigned long last_state_change = 0L;
	unsigned long last_hard_state_change = 0L;
	unsigned long last_time_up = 0L;
	unsigned long last_time_down = 0L;
	unsigned long last_time_unreachable = 0L;
	unsigned long last_notification = 0L;
	unsigned long next_notification = 0L;
	unsigned long modified_host_attributes = 0L;
	double percent_state_change = 0.0;
	double latency = 0.0;
	double execution_time = 0.0;
	int current_state = 0;
	int has_been_checked = 0;
	int should_be_scheduled = 0;
	int current_check_attempt = 0;
	int max_check_attempts = 0;
	int check_type = 0;
	int last_hard_state = 0;
	int state_type = 0;
	int no_more_notifications = 0;
	int notifications_enabled = 0;
	int problem_has_been_acknowledged = 0;
	int acknowledgement_type = 0;
	int current_notification_number = 0;
	int passive_checks_enabled = 0;
	int active_checks_enabled = 0;
	int event_handler_enabled;
	int flap_detection_enabled = 0;
	int is_flapping = 0;
	int scheduled_downtime_depth = 0;
	int failure_prediction_enabled = 0;
	int process_performance_data;
	int obsess_over_host = 0;
	double normal_check_interval = 0.0;
	double retry_check_interval = 0.0;
	char *ts[10];
	char *es[5];
	unsigned long object_id = 0L;
	unsigned long check_timeperiod_object_id = 0L;
	int x = 0;
	int result = IDO_OK;
	void *data[56];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hoststatusdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* covert vars */
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTHOSTCHECK], &last_check);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_NEXTHOSTCHECK], &next_check);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTSTATECHANGE], &last_state_change);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTHARDSTATECHANGE], &last_hard_state_change);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTTIMEUP], &last_time_up);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTTIMEDOWN], &last_time_down);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTTIMEUNREACHABLE], &last_time_unreachable);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTHOSTNOTIFICATION], &last_notification);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_NEXTHOSTNOTIFICATION], &next_notification);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_MODIFIEDHOSTATTRIBUTES], &modified_host_attributes);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_PERCENTSTATECHANGE], &percent_state_change);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_LATENCY], &latency);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_EXECUTIONTIME], &execution_time);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CURRENTSTATE], &current_state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HASBEENCHECKED], &has_been_checked);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SHOULDBESCHEDULED], &should_be_scheduled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CURRENTCHECKATTEMPT], &current_check_attempt);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_MAXCHECKATTEMPTS], &max_check_attempts);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CHECKTYPE], &check_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_LASTHARDSTATE], &last_hard_state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATETYPE], &state_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOMORENOTIFICATIONS], &no_more_notifications);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFICATIONSENABLED], &notifications_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PROBLEMHASBEENACKNOWLEDGED], &problem_has_been_acknowledged);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ACKNOWLEDGEMENTTYPE], &acknowledgement_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CURRENTNOTIFICATIONNUMBER], &current_notification_number);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PASSIVEHOSTCHECKSENABLED], &passive_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ACTIVEHOSTCHECKSENABLED], &active_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EVENTHANDLERENABLED], &event_handler_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ISFLAPPING], &is_flapping);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SCHEDULEDDOWNTIMEDEPTH], &scheduled_downtime_depth);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PROCESSPERFORMANCEDATA], &process_performance_data);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_OBSESSOVERHOST], &obsess_over_host);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_NORMALCHECKINTERVAL], &normal_check_interval);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_RETRYCHECKINTERVAL], &retry_check_interval);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_OUTPUT]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_LONGOUTPUT]);
	es[2] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_PERFDATA]);
	es[3] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_EVENTHANDLER]);
	es[4] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_CHECKCOMMAND]);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, last_check);
	ts[2] = ido2db_db_timet_to_sql(idi, next_check);
	ts[3] = ido2db_db_timet_to_sql(idi, last_state_change);
	ts[4] = ido2db_db_timet_to_sql(idi, last_hard_state_change);
	ts[5] = ido2db_db_timet_to_sql(idi, last_time_up);
	ts[6] = ido2db_db_timet_to_sql(idi, last_time_down);
	ts[7] = ido2db_db_timet_to_sql(idi, last_time_unreachable);
	ts[8] = ido2db_db_timet_to_sql(idi, last_notification);
	ts[9] = ido2db_db_timet_to_sql(idi, next_notification);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_HOSTCHECKPERIOD], NULL, &check_timeperiod_object_id);

	if (enable_sla)
		sla_process_acknowledgement(idi, object_id, tstamp.tv_sec, problem_has_been_acknowledged);

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &object_id;
	data[2] = (void *) &ts[0];
	data[3] = (void *) &es[0];
	data[4] = (void *) &es[1];
	data[5] = (void *) &es[2];
	data[6] = (void *) &current_state;
	data[7] = (void *) &has_been_checked;
	data[8] = (void *) &should_be_scheduled;
	data[9] = (void *) &current_check_attempt;
	data[10] = (void *) &max_check_attempts;
	data[11] = (void *) &ts[1];
	data[12] = (void *) &ts[2];
	data[13] = (void *) &check_type;
	data[14] = (void *) &ts[3];
	data[15] = (void *) &ts[4];
	data[16] = (void *) &last_hard_state;
	data[17] = (void *) &ts[5];
	data[18] = (void *) &ts[6];
	data[19] = (void *) &ts[7];
	data[20] = (void *) &state_type;
	data[21] = (void *) &ts[8];
	data[22] = (void *) &ts[9];
	data[23] = (void *) &no_more_notifications;
	data[24] = (void *) &notifications_enabled;
	data[25] = (void *) &problem_has_been_acknowledged;
	data[26] = (void *) &acknowledgement_type;
	data[27] = (void *) &current_notification_number;
	data[28] = (void *) &passive_checks_enabled;
	data[29] = (void *) &active_checks_enabled;
	data[30] = (void *) &event_handler_enabled;
	data[31] = (void *) &flap_detection_enabled;
	data[32] = (void *) &is_flapping;
	data[33] = (void *) &percent_state_change;
	data[34] = (void *) &latency;
	data[35] = (void *) &execution_time;
	data[36] = (void *) &scheduled_downtime_depth;
	data[37] = (void *) &failure_prediction_enabled;
	data[38] = (void *) &process_performance_data;
	data[39] = (void *) &obsess_over_host;
	data[40] = (void *) &modified_host_attributes;
	data[41] = (void *) &es[3];
	data[42] = (void *) &es[4];
	data[43] = (void *) &normal_check_interval;
	data[44] = (void *) &retry_check_interval;
	data[45] = (void *) &check_timeperiod_object_id;
	/* add unixtime for bind params */
	data[46] = (void *) &tstamp.tv_sec;
	data[47] = (void *) &last_check;
	data[48] = (void *) &next_check;
	data[49] = (void *) &last_state_change;
	data[50] = (void *) &last_hard_state_change;
	data[51] = (void *) &last_time_up;
	data[52] = (void *) &last_time_down;
	data[53] = (void *) &last_time_unreachable;
	data[54] = (void *) &last_notification;
	data[55] = (void *) &next_notification;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hoststatusdata() LongLen:%d\n", strlen(es[1]));
	result = ido2db_query_insert_or_update_hoststatusdata_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* do not free if prepared statement */

#endif /* Oracle ocilib specific */

	/* save custom variables to db */
	result = ido2db_save_custom_variables(idi, IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS, object_id, ts[0], tstamp.tv_sec);

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hoststatusdata() end\n");

	return IDO_OK;
}

int ido2db_handle_servicestatusdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long last_check = 0L;
	unsigned long next_check = 0L;
	unsigned long last_state_change = 0L;
	unsigned long last_hard_state_change = 0L;
	unsigned long last_time_ok = 0L;
	unsigned long last_time_warning = 0L;
	unsigned long last_time_unknown = 0L;
	unsigned long last_time_critical = 0L;
	unsigned long last_notification = 0L;
	unsigned long next_notification = 0L;
	unsigned long modified_service_attributes = 0L;
	double percent_state_change = 0.0;
	double latency = 0.0;
	double execution_time = 0.0;
	int current_state = 0;
	int has_been_checked = 0;
	int should_be_scheduled = 0;
	int current_check_attempt = 0;
	int max_check_attempts = 0;
	int check_type = 0;
	int last_hard_state = 0;
	int state_type = 0;
	int no_more_notifications = 0;
	int notifications_enabled = 0;
	int problem_has_been_acknowledged = 0;
	int acknowledgement_type = 0;
	int current_notification_number = 0;
	int passive_checks_enabled = 0;
	int active_checks_enabled = 0;
	int event_handler_enabled;
	int flap_detection_enabled = 0;
	int is_flapping = 0;
	int scheduled_downtime_depth = 0;
	int failure_prediction_enabled = 0;
	int process_performance_data;
	int obsess_over_service = 0;
	double normal_check_interval = 0.0;
	double retry_check_interval = 0.0;
	char *ts[11];
	char *es[5];
	unsigned long object_id = 0L;
	unsigned long check_timeperiod_object_id = 0L;
	int x = 0;
	int result = IDO_OK;
	void *data[58];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicestatusdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* covert vars */
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTSERVICECHECK], &last_check);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_NEXTSERVICECHECK], &next_check);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTSTATECHANGE], &last_state_change);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTHARDSTATECHANGE], &last_hard_state_change);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTTIMEOK], &last_time_ok);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTTIMEWARNING], &last_time_warning);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTTIMEUNKNOWN], &last_time_unknown);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTTIMECRITICAL], &last_time_critical);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTSERVICENOTIFICATION], &last_notification);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_NEXTSERVICENOTIFICATION], &next_notification);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_MODIFIEDSERVICEATTRIBUTES], &modified_service_attributes);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_PERCENTSTATECHANGE], &percent_state_change);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_LATENCY], &latency);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_EXECUTIONTIME], &execution_time);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CURRENTSTATE], &current_state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HASBEENCHECKED], &has_been_checked);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SHOULDBESCHEDULED], &should_be_scheduled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CURRENTCHECKATTEMPT], &current_check_attempt);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_MAXCHECKATTEMPTS], &max_check_attempts);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CHECKTYPE], &check_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_LASTHARDSTATE], &last_hard_state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATETYPE], &state_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOMORENOTIFICATIONS], &no_more_notifications);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFICATIONSENABLED], &notifications_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PROBLEMHASBEENACKNOWLEDGED], &problem_has_been_acknowledged);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ACKNOWLEDGEMENTTYPE], &acknowledgement_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CURRENTNOTIFICATIONNUMBER], &current_notification_number);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PASSIVESERVICECHECKSENABLED], &passive_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ACTIVESERVICECHECKSENABLED], &active_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_EVENTHANDLERENABLED], &event_handler_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ISFLAPPING], &is_flapping);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SCHEDULEDDOWNTIMEDEPTH], &scheduled_downtime_depth);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PROCESSPERFORMANCEDATA], &process_performance_data);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_OBSESSOVERSERVICE], &obsess_over_service);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_NORMALCHECKINTERVAL], &normal_check_interval);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_RETRYCHECKINTERVAL], &retry_check_interval);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_OUTPUT]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_LONGOUTPUT]);
	es[2] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_PERFDATA]);
	es[3] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_EVENTHANDLER]);
	es[4] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_CHECKCOMMAND]);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, last_check);
	ts[2] = ido2db_db_timet_to_sql(idi, next_check);
	ts[3] = ido2db_db_timet_to_sql(idi, last_state_change);
	ts[4] = ido2db_db_timet_to_sql(idi, last_hard_state_change);
	ts[5] = ido2db_db_timet_to_sql(idi, last_time_ok);
	ts[6] = ido2db_db_timet_to_sql(idi, last_time_warning);
	ts[7] = ido2db_db_timet_to_sql(idi, last_time_unknown);
	ts[8] = ido2db_db_timet_to_sql(idi, last_time_critical);
	ts[9] = ido2db_db_timet_to_sql(idi, last_notification);
	ts[10] = ido2db_db_timet_to_sql(idi, next_notification);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE,
	         idi->buffered_input[IDO_DATA_HOST],
	         idi->buffered_input[IDO_DATA_SERVICE], &object_id);
	result = ido2db_get_object_id_with_insert(idi,
	         IDO2DB_OBJECTTYPE_TIMEPERIOD,
	         idi->buffered_input[IDO_DATA_SERVICECHECKPERIOD], NULL,
	         &check_timeperiod_object_id);

	if (enable_sla)
		sla_process_acknowledgement(idi, object_id, tstamp.tv_sec, problem_has_been_acknowledged);

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &object_id;
	data[2] = (void *) &ts[0];
	data[3] = (void *) &es[0];
	data[4] = (void *) &es[1];
	data[5] = (void *) &es[2];
	data[6] = (void *) &current_state;
	data[7] = (void *) &has_been_checked;
	data[8] = (void *) &should_be_scheduled;
	data[9] = (void *) &current_check_attempt;
	data[10] = (void *) &max_check_attempts;
	data[11] = (void *) &ts[1];
	data[12] = (void *) &ts[2];
	data[13] = (void *) &check_type;
	data[14] = (void *) &ts[3];
	data[15] = (void *) &ts[4];
	data[16] = (void *) &last_hard_state;
	data[17] = (void *) &ts[5];
	data[18] = (void *) &ts[6];
	data[19] = (void *) &ts[7];
	data[20] = (void *) &ts[8];
	data[21] = (void *) &state_type;
	data[22] = (void *) &ts[9];
	data[23] = (void *) &ts[10];
	data[24] = (void *) &no_more_notifications;
	data[25] = (void *) &notifications_enabled;
	data[26] = (void *) &problem_has_been_acknowledged;
	data[27] = (void *) &acknowledgement_type;
	data[28] = (void *) &current_notification_number;
	data[29] = (void *) &passive_checks_enabled;
	data[30] = (void *) &active_checks_enabled;
	data[31] = (void *) &event_handler_enabled;
	data[32] = (void *) &flap_detection_enabled;
	data[33] = (void *) &is_flapping;
	data[34] = (void *) &percent_state_change;
	data[35] = (void *) &latency;
	data[36] = (void *) &execution_time;
	data[37] = (void *) &scheduled_downtime_depth;
	data[38] = (void *) &failure_prediction_enabled;
	data[39] = (void *) &process_performance_data;
	data[40] = (void *) &obsess_over_service;
	data[41] = (void *) &modified_service_attributes;
	data[42] = (void *) &es[3];
	data[43] = (void *) &es[4];
	data[44] = (void *) &normal_check_interval;
	data[45] = (void *) &retry_check_interval;
	data[46] = (void *) &check_timeperiod_object_id;
	/* add unixtime for bind params */
	data[47] = (void *) &tstamp.tv_sec;
	data[48] = (void *) &last_check;
	data[49] = (void *) &next_check;
	data[50] = (void *) &last_state_change;
	data[51] = (void *) &last_hard_state_change;
	data[52] = (void *) &last_time_ok;
	data[53] = (void *) &last_time_warning;
	data[54] = (void *) &last_time_unknown;
	data[55] = (void *) &last_time_critical;
	data[56] = (void *) &last_notification;
	data[57] = (void *) &next_notification;

	result = ido2db_query_insert_or_update_servicestatusdata_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* do not free if prepared statement */

#endif /* Oracle ocilib specific */

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	/* save custom variables to db */
	result = ido2db_save_custom_variables(idi, IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS, object_id, ts[0], tstamp.tv_sec);

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicestatusdata() end\n");

	return IDO_OK;
}

int ido2db_handle_contactstatusdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long last_host_notification = 0L;
	unsigned long last_service_notification = 0L;
	unsigned long modified_attributes = 0L;
	unsigned long modified_host_attributes = 0L;
	unsigned long modified_service_attributes = 0L;
	int host_notifications_enabled = 0;
	int service_notifications_enabled = 0;
	char *ts[3];
	unsigned long object_id = 0L;
	int x = 0;
	int result = IDO_OK;
	void *data[13];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactstatusdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* covert vars */
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTHOSTNOTIFICATION], &last_host_notification);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_LASTSERVICENOTIFICATION], &last_service_notification);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_MODIFIEDCONTACTATTRIBUTES], &modified_attributes);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_MODIFIEDHOSTATTRIBUTES], &modified_host_attributes);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_MODIFIEDSERVICEATTRIBUTES], &modified_service_attributes);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HOSTNOTIFICATIONSENABLED], &host_notifications_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SERVICENOTIFICATIONSENABLED], &service_notifications_enabled);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, last_host_notification);
	ts[2] = ido2db_db_timet_to_sql(idi, last_service_notification);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_CONTACT, idi->buffered_input[IDO_DATA_CONTACTNAME], NULL, &object_id);

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &object_id;
	data[2] = (void *) &ts[0];
	data[3] = (void *) &host_notifications_enabled;
	data[4] = (void *) &service_notifications_enabled;
	data[5] = (void *) &ts[1];
	data[6] = (void *) &ts[2];
	data[7] = (void *) &modified_attributes;
	data[8] = (void *) &modified_host_attributes;
	data[9] = (void *) &modified_service_attributes;
	/* bind params */
	data [10] = (void *) &tstamp.tv_sec;
	data [11] = (void *) &last_host_notification;
	data [12] = (void *) &last_service_notification;

	result = ido2db_query_insert_or_update_contactstatusdata_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	/* save custom variables to db */
	result = ido2db_save_custom_variables(idi, IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS, object_id, ts[0], tstamp.tv_sec);

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactstatusdata() end\n");

	return IDO_OK;
}

int ido2db_handle_adaptiveprogramdata(ido2db_idi *idi) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_adaptiveprogramdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* IGNORED */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_adaptiveprogramdata() end\n");

	return IDO_OK;
}

int ido2db_handle_adaptivehostdata(ido2db_idi *idi) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_adaptivehostdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* IGNORED */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_adaptivehostdata() end\n");

	return IDO_OK;
}

int ido2db_handle_adaptiveservicedata(ido2db_idi *idi) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_adaptiveservicedata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* IGNORED */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_adaptiveservicedata() end\n");

	return IDO_OK;
}

int ido2db_handle_adaptivecontactdata(ido2db_idi *idi) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_adaptivecontactdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* IGNORED */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_adaptivecontactdata() end\n");

	return IDO_OK;
}

int ido2db_handle_externalcommanddata(ido2db_idi *idi) {
	int type, flags, attr;
	int x = 0;
	struct timeval tstamp;
	char *ts = NULL;
	char *es[2];
	int command_type = 0;
	unsigned long entry_time = 0L;
	char *buf = NULL;
	int result = IDO_OK;

#ifdef USE_ORACLE
	void *data[5];
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_externalcommanddata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* only handle start events */
	if (type != NEBTYPE_EXTERNALCOMMAND_START)
		return IDO_OK;

	/* covert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_COMMANDTYPE], &command_type);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_ENTRYTIME], &entry_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDSTRING]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDARGS]);

	ts = ido2db_db_timet_to_sql(idi, entry_time);

	/* save entry to db */
#ifdef USE_LIBDBI /* everything else will be libdbi */
        switch (idi->dbinfo.server_type) {
        case IDO2DB_DBSERVER_PGSQL:
	        if (asprintf(&buf, "INSERT INTO %s (instance_id, command_type, entry_time, command_name, command_args) VALUES (%lu, %d, %s, E'%s', E'%s')",
	                     ido2db_db_tablenames[IDO2DB_DBTABLE_EXTERNALCOMMANDS],
	                     idi->dbinfo.instance_id, command_type, ts, es[0], es[1]) == -1)
	                buf = NULL;
                break;
        default:
	        if (asprintf(&buf, "INSERT INTO %s (instance_id, command_type, entry_time, command_name, command_args) VALUES (%lu, %d, %s, '%s', '%s')",
	                     ido2db_db_tablenames[IDO2DB_DBTABLE_EXTERNALCOMMANDS],
	                     idi->dbinfo.instance_id, command_type, ts, es[0], es[1]) == -1)
	                buf = NULL;
                break;
        }

	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &command_type;
	data[2] = (void *) &entry_time;
	data[3] = (void *) &es[0];
	data[4] = (void *) &es[1];

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_external_commands, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_external_commands, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_external_commands, MT(":X3"), (uint *) data[2])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (es[0] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_external_commands, ":X4") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_external_commands, MT(":X4"), *(char **) data[3], 0)) {
			return IDO_ERROR;
		}
	}
	if (es[1] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_external_commands, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_external_commands, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_external_commands)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_external_commands() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	free(buf);

	/* free memory */
	free(ts);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_externalcommanddata() end\n");

	return IDO_OK;
}

int ido2db_handle_aggregatedstatusdata(ido2db_idi *idi) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_aggregatedstatusdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* IGNORED */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_aggregatedstatusdata() end\n");

	return IDO_OK;
}

int ido2db_handle_retentiondata(ido2db_idi *idi) {

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_retentiondata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* IGNORED */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_retentiondata() end\n");

	return IDO_OK;
}

int ido2db_handle_acknowledgementdata(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int acknowledgement_type = 0;
	int state = 0;
	int is_sticky = 0;
	int persistent_comment = 0;
	int notify_contacts = 0;
	unsigned long object_id = 0L;
	int result = IDO_OK;
	unsigned long end_time;
	char *ts[2];
	char *es[2];
	int x = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
	char *buf1 = NULL;
#endif

#ifdef USE_ORACLE
	void *data[12];
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_acknowledgementdata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ACKNOWLEDGEMENTTYPE], &acknowledgement_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATE], &state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STICKY], &is_sticky);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PERSISTENT], &persistent_comment);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYCONTACTS], &notify_contacts);
	result = ido2db_convert_string_to_unsignedlong(idi->buffered_input[IDO_DATA_END_TIME], &end_time);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_AUTHORNAME]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMENT]);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ido2db_db_timet_to_sql(idi, end_time);

	/* get the object id */
	if (acknowledgement_type == SERVICE_ACKNOWLEDGEMENT)
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOST], idi->buffered_input[IDO_DATA_SERVICE], &object_id);
	if (acknowledgement_type == HOST_ACKNOWLEDGEMENT)
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);

	/* save entry to db */
	/* NOTE Primary Key and only unique key is auto_increment thus ON DUPLICATE KEY will not occur ever */

#ifdef USE_LIBDBI /* everything else will be libdbi */
	/* the data part of the INSERT statement */
        switch (idi->dbinfo.server_type) {
        case IDO2DB_DBSERVER_PGSQL:
	        if (asprintf(&buf1, "(instance_id, entry_time, entry_time_usec, acknowledgement_type, object_id, state, author_name, comment_data, is_sticky, persistent_comment, notify_contacts, end_time) VALUES (%lu, %s, %lu, %d, %lu, %d, E'%s', E'%s', %d, %d, %d, %s)"
	                     , idi->dbinfo.instance_id
	                     , ts[0]
	                     , tstamp.tv_usec
	                     , acknowledgement_type
	                     , object_id
	                     , state
	                     , es[0]
	                     , es[1]
	                     , is_sticky
	                     , persistent_comment
	                     , notify_contacts
        	             , ts[1]
	                    ) == -1)
	                buf1 = NULL;
                break;
        default:
	        if (asprintf(&buf1, "(instance_id, entry_time, entry_time_usec, acknowledgement_type, object_id, state, author_name, comment_data, is_sticky, persistent_comment, notify_contacts, end_time) VALUES (%lu, %s, %lu, %d, %lu, %d, '%s', '%s', %d, %d, %d, %s)"
	                     , idi->dbinfo.instance_id
	                     , ts[0]
	                     , tstamp.tv_usec
	                     , acknowledgement_type
	                     , object_id
	                     , state
	                     , es[0]
	                     , es[1]
	                     , is_sticky
	                     , persistent_comment
	                     , notify_contacts
	                     , ts[1]
	                    ) == -1)
	                buf1 = NULL;
                break;
        }

	if (asprintf(&buf, "INSERT INTO %s %s"
	             , ido2db_db_tablenames[IDO2DB_DBTABLE_ACKNOWLEDGEMENTS]
	             , buf1
	            ) == -1)
		buf = NULL;

	free(buf1);

	result = ido2db_db_query(idi, buf);
	free(buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &tstamp.tv_sec;
	data[2] = (void *) &tstamp.tv_usec;
	data[3] = (void *) &acknowledgement_type;
	data[4] = (void *) &object_id;
	data[5] = (void *) &state;
	data[6] = (void *) &es[0];
	data[7] = (void *) &es[1];
	data[8] = (void *) &is_sticky;
	data[9] = (void *) &persistent_comment;
	data[10] = (void *) &notify_contacts;
	data[11] = (void *) &end_time;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X2"), (uint *) data[1])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X4"), (int *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X5"), (uint *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X6"), (int *) data[5])) {
		return IDO_ERROR;
	}
	if (es[0] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_acknowledgements, ":X7") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_acknowledgements, MT(":X7"), *(char **) data[6], 0)) {
			return IDO_ERROR;
		}
	}
	if (es[1] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_acknowledgements, ":X8") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_acknowledgements, MT(":X8"), *(char **) data[7], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_acknowledgements, MT(":X12"), (uint *) data[11])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_acknowledgements)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_acknowledgements() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_acknowledgementdata() end\n");

	return IDO_OK;
}

int ido2db_handle_statechangedata(ido2db_idi *idi) {
	int result = IDO_OK;
	int type, flags, attr;
	int x = 0;
	struct timeval tstamp;
	int statechange_type = 0;
	int state_change_occurred = 0;
	int state = 0;
	int state_type = 0;
	int current_attempt = 0;
	int max_attempts = 0;
	int last_state = -1;
	int last_hard_state = -1;
	unsigned long object_id = 0L;
	char *ts[1];
	char *es[2];
	char *buf = NULL;

#ifdef USE_ORACLE
	void *data[13];
	OCI_Lob *lob_i;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_statechangedata() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* only process completed state changes */
	if (type != NEBTYPE_STATECHANGE_END)
		return IDO_OK;

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATECHANGETYPE], &statechange_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATECHANGE], &state_change_occurred);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATE], &state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STATETYPE], &state_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CURRENTCHECKATTEMPT], &current_attempt);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_MAXCHECKATTEMPTS], &max_attempts);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_LASTHARDSTATE], &last_hard_state);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_LASTSTATE], &last_state);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_OUTPUT]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_LONGOUTPUT]);

	ts[0] = ido2db_db_timet_to_sql(idi, tstamp.tv_sec);

	/* get the object id */
	if (statechange_type == SERVICE_STATECHANGE)
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOST],
		         idi->buffered_input[IDO_DATA_SERVICE], &object_id);
	else
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST,
		         idi->buffered_input[IDO_DATA_HOST], NULL, &object_id);

	if (enable_sla)
		sla_process_statechange(idi, object_id, tstamp.tv_sec,
		    tstamp.tv_sec, &state, &state_type, NULL);

	/* save entry to db */
#ifdef USE_LIBDBI /* everything else will be libdbi */
        switch (idi->dbinfo.server_type) {
        case IDO2DB_DBSERVER_PGSQL:
	        if (asprintf(
        	            &buf,
	                    "INSERT INTO %s (instance_id, state_time, state_time_usec, object_id, state_change, state, state_type, current_check_attempt, max_check_attempts, last_state, last_hard_state, output, long_output) VALUES (%lu, %s, %lu, %lu, %d, %d, %d, %d, %d, %d, %d, E'%s', E'%s')",
	                    ido2db_db_tablenames[IDO2DB_DBTABLE_STATEHISTORY],
	                    idi->dbinfo.instance_id, ts[0], tstamp.tv_usec, object_id,
	                    state_change_occurred, state, state_type, current_attempt,
	                    max_attempts, last_state, last_hard_state, es[0], es[1]) == -1)
	                buf = NULL;
                break;
        default:
	        if (asprintf(
	                    &buf,
	                    "INSERT INTO %s (instance_id, state_time, state_time_usec, object_id, state_change, state, state_type, current_check_attempt, max_check_attempts, last_state, last_hard_state, output, long_output) VALUES (%lu, %s, %lu, %lu, %d, %d, %d, %d, %d, %d, %d, '%s', '%s')",
	                    ido2db_db_tablenames[IDO2DB_DBTABLE_STATEHISTORY],
	                    idi->dbinfo.instance_id, ts[0], tstamp.tv_usec, object_id,
	                    state_change_occurred, state, state_type, current_attempt,
	                    max_attempts, last_state, last_hard_state, es[0], es[1]) == -1)
	                buf = NULL;
                break;
        }

	result = ido2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi) == IDO_ERROR)
		return IDO_ERROR;

	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &tstamp.tv_sec;
	data[2] = (void *) &tstamp.tv_usec;
	data[3] = (void *) &object_id;
	data[4] = (void *) &state_change_occurred;
	data[5] = (void *) &state;
	data[6] = (void *) &state_type;
	data[7] = (void *) &current_attempt;
	data[8] = (void *) &max_attempts;
	data[9] = (void *) &last_state;
	data[10] = (void *) &last_hard_state;
	data[11] = (void *) &es[0];
	data[12] = (void *) &es[1];

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_statehistory, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_statehistory, MT(":X2"), (uint *) data[1])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_statehistory, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_statehistory, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_statehistory, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_statehistory, MT(":X6"), (int *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_statehistory, MT(":X7"), (int *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_statehistory, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_statehistory, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_statehistory, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_statehistory, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}
	if (es[0] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_statehistory, ":X12") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_statehistory, MT(":X12"), *(char **) data[11], 0)) {
			return IDO_ERROR;
		}
	}

	//bind clob
	lob_i = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_statehistory() bind clob\n");
	result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_statehistory, ":X13", *(char **)data[12], &lob_i);
	if (result == IDO_OK) {
		/* execute statement */
		result = OCI_Execute(idi->dbinfo.oci_statement_statehistory) ? IDO_OK : IDO_ERROR;
		if (result == IDO_OK) {
			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_statehistory() executed\n");
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_statehistory() execute error\n");

		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_statehistory() bind clob error\n");
	}
	//free lobs
	if (lob_i) OCI_LobFree(lob_i);
	/* do not free statement yet! */

#endif /* Oracle ocilib specific */

	free(buf);

	/* free memory */
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);
	for (x = 0; x < ICINGA_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_statechangedata() end\n");

	return result;
}

/****************************************************************************/
/* VARIABLE DATA HANDLERS                                                   */
/****************************************************************************/

int ido2db_handle_configfilevariables(ido2db_idi *idi, int configfile_type) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long configfile_id = 0L;
	int result = IDO_OK;
	char *es[3];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *varname = NULL;
	char *varvalue = NULL;
	ido2db_mbuf mbuf;
#ifdef USE_ORACLE
	char *seq_name = NULL;
	int len = 0;
	/* definitions for array binding */
	const int CONFIG_VARNAME_SIZE = 64; /* table configfilevariables column varname length */
	const int CONFIG_VARVALUE_SIZE = 2048; /* table configfilevariables column varvalue length */
	int  arrsize = 0;
	int count = 0;
	big_uint  *instid_arr;
	big_uint  *fileid_arr;
	char * name_arr;
	char * val_arr;

#endif
	void *data[4];
	int first;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables() start\n");
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 0, "HANDLE_CONFIGFILEVARS [1]\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 0, "HANDLE_CONFIGFILEVARS [2]\n");
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 0, "TSTAMP: %lu   LATEST: %lu\n", tstamp.tv_sec, idi->dbinfo.latest_realtime_data_time);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 0, "HANDLE_CONFIGFILEVARS [3]\n");

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_CONFIGFILENAME]);

	/* add config file to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &configfile_type;
	data[2] = (void *) &es[0];

	result = ido2db_query_insert_or_update_configfilevariables_add(idi, data);
	free(es[0]);
	if (result == IDO_OK) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables"
		                      " preceeding ido2db_query_insert_or_update_configfilevariables_add OK \n");
#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			configfile_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables(%lu) configfilevariables_id\n", configfile_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf1, "%s_configfile_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES]) == -1)
				buf1 = NULL;

			configfile_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables(%s=%lu) configfilevariables_id\n", buf1, configfile_id);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		/* retrieve last inserted configfile_id */
		if (asprintf(&seq_name, "seq_configfiles") == -1)
			seq_name = NULL;
		configfile_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables(id %lu) \n", configfile_id);
		free(seq_name);

#endif /* Oracle ocilib specific */

	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables"
		                      " preceeding ido2db_query_insert_or_update_configfilevariables_add ERROR \n");
		return IDO_ERROR;
	}


#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

	/* save config file variables to db */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables_elements()  start\n");

#ifdef USE_LIBDBI
	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, configfile_id, varname, varvalue) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILEVARIABLES]
	            ) == -1)
		buf1 = NULL;
#endif


#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* allocate array buffers based an maximal expected sizes*/
	instid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	fileid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	/* string array buffer must be continous memory */
	name_arr = (char *) malloc(OCI_BINDARRAY_MAX_SIZE * (CONFIG_VARNAME_SIZE + 1));
	val_arr = (char *) malloc(OCI_BINDARRAY_MAX_SIZE * (CONFIG_VARVALUE_SIZE + 1));

	if ((instid_arr == NULL) || (fileid_arr == NULL) || (name_arr == NULL) || (val_arr == NULL)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_configfilevariables_elements()"
		                      "  ArrayVars Alloc ERROR, exit\n");
		if (instid_arr) free(instid_arr);
		if (fileid_arr) free(fileid_arr);
		if (name_arr) free(name_arr);
		if (val_arr) free(val_arr);
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_configfilevariables_elements()"
	                      "  ArrayVars OK\n");
	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_configfilevariables_insert, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_configfilevariables_insert, MT(":X2"), fileid_arr, 0);
	OCI_BindArrayOfStrings(idi->dbinfo.oci_statement_configfilevariables_insert, MT(":X3"), (char *)name_arr, CONFIG_VARNAME_SIZE, 0);
	OCI_BindArrayOfStrings(idi->dbinfo.oci_statement_configfilevariables_insert, MT(":X4"), (char *)val_arr, CONFIG_VARVALUE_SIZE, 0);
	arrsize = 0;

#endif /* Oracle ocilib specific */


	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_CONFIGFILEVARIABLE];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get var name/val pair */
		varname = strtok(mbuf.buffer[x], "=");
		varvalue = strtok(NULL, "\x0");

		es[1] = ido2db_db_escape_string(idi, varname);
		es[2] = ido2db_db_escape_string(idi, varvalue);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI /* everything else will be libdbi */
                switch (idi->dbinfo.server_type) {
                case IDO2DB_DBSERVER_PGSQL:
	                if (asprintf(&buf1, "%s%s(%lu, %lu, E'%s', E'%s')",
	                             buf1,
	                             (first == 1 ? "" : ","),
	                             idi->dbinfo.instance_id,
	                             configfile_id,
	                             es[1],
	                             es[2]
	                            ) == -1)
	                        buf1 = NULL;
                        break;
                default:
	                if (asprintf(&buf1, "%s%s(%lu, %lu, '%s', '%s')",
        	                     buf1,
	                             (first == 1 ? "" : ","),
	                             idi->dbinfo.instance_id,
	                             configfile_id,
	                             es[1],
	                             es[2]
	                            ) == -1)
	                        buf1 = NULL;
                        break;
                }
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables_elements"
		                      "(Pos %lu) Id=%lu File=%lu %s=%s\n",
		                      arrsize + count, idi->dbinfo.instance_id, configfile_id, es[1], es[2]);
		/* copy instanceid and configfile_id to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		fileid_arr[arrsize] = configfile_id;
		/* take care about string len not exceeding varname column length */
		len = (strlen(es[1]) < CONFIG_VARNAME_SIZE) ? strlen(es[1]) : CONFIG_VARNAME_SIZE;
		es[1][len+1] = 0; //make sure string is terminated
		/* copy to string array buffer at the right position */
		strcpy(&name_arr[arrsize*(CONFIG_VARNAME_SIZE+1)], es[1]);
		/* take care about string len not exceeding varvalue column length */
		len = (strlen(es[1]) < CONFIG_VARVALUE_SIZE) ? strlen(es[2]) : CONFIG_VARVALUE_SIZE;
		es[2][len+1] = 0; //make sure string is terminated
		/* copy to string array buffer at the right position */
		strcpy(&val_arr[arrsize*(CONFIG_VARVALUE_SIZE+1)], es[2]);
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		free(es[1]);
		free(es[2]);
		first = 0;
#ifdef USE_ORACLE /* Oracle ocilib specific */
		//ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "Pos:%d of %d\n",arrsize,OCI_BINDARRAY_MAX_SIZE);
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			//ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "Do Commit:%d of %d\n",arrsize,OCI_BINDARRAY_MAX_SIZE);
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_configfilevariables_insert, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_configfilevariables_insert)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_configfilevariables_insert);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_configfilevariables_elements"
				                      "(File %lu):so far %d items committed\n", configfile_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_configfilevariables_elements"
				                      "(File %lu) ERROR:Rollback %d items\n", configfile_id, arrsize);
				free(instid_arr);
				free(fileid_arr);
				free(name_arr);
				free(val_arr);
				return IDO_ERROR;
			}

		}


#endif /* Oracle ocilib specific */
	}//for


#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remainig entries*/
	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_configfilevariables_insert, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_configfilevariables_insert)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_configfilevariables_insert);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_configfilevariables_elements"
			                      "(File %lu) %d items finished\n", configfile_id, count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_configfilevariables"
			                      "(File %lu) ERROR:Rollback %d items\n", configfile_id, arrsize);
		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_configfilevariables"
		                      "(File %lu) Warning:No items storable\n", configfile_id);
	}

	//cleanup array buffers
	free(name_arr);
	free(val_arr);
	free(instid_arr);
	free(fileid_arr);

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables() end\n");
	return IDO_OK;
}

int ido2db_handle_configvariables(ido2db_idi *idi) {

	if (idi == NULL)
		return IDO_ERROR;

	return IDO_OK;
}

int ido2db_handle_runtimevariables(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int result = IDO_OK;
	char *es[2];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *varname = NULL;
	char *varvalue = NULL;
	ido2db_mbuf mbuf;
	int first;
#ifdef USE_ORACLE
	int len = 0;
	/* definitions for array binding */
	const int CONFIG_VARNAME_SIZE = 64; /* table runtimefilevariables column varname length */
	const int CONFIG_VARVALUE_SIZE = 1024; /* table runtimevariables column varvalue length */
	int  arrsize = 0;
	int count = 0;
	big_uint  *instid_arr;
	char * name_arr;
	char * val_arr;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_runtimevariables() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;


#ifdef USE_LIBDBI

	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, varname, varvalue) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_RUNTIMEVARIABLES]
	            ) == -1)
		buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* allocate array buffers based an maximal expected sizes*/
	instid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	/* string array buffer must be continous memory */
	name_arr = (char *) malloc(OCI_BINDARRAY_MAX_SIZE * (CONFIG_VARNAME_SIZE + 1));
	val_arr = (char *) malloc(OCI_BINDARRAY_MAX_SIZE * (CONFIG_VARVALUE_SIZE + 1));

	if ((instid_arr == NULL) || (name_arr == NULL) || (val_arr == NULL)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_runtimevariables"
		                      " ArrayVars Alloc ERROR, exit\n");
		if (instid_arr) free(instid_arr);
		if (name_arr) free(name_arr);
		if (val_arr) free(val_arr);
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_runtimevariables()"
	                      "  ArrayVars OK\n");
	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_runtimevariables, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfStrings(idi->dbinfo.oci_statement_runtimevariables, MT(":X2"), (char *)name_arr, CONFIG_VARNAME_SIZE, 0);
	OCI_BindArrayOfStrings(idi->dbinfo.oci_statement_runtimevariables, MT(":X3"), (char *)val_arr, CONFIG_VARVALUE_SIZE, 0);
	arrsize = 0;


#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_RUNTIMEVARIABLE];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get var name/val pair */
		varname = strtok(mbuf.buffer[x], "=");
		varvalue = strtok(NULL, "\x0");

		es[0] = ido2db_db_escape_string(idi, varname);
		es[1] = ido2db_db_escape_string(idi, varvalue);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
                switch (idi->dbinfo.server_type) {
                case IDO2DB_DBSERVER_PGSQL:
	                if (asprintf(&buf1, "%s%s(%lu, E'%s', E'%s')",
	                             buf1,
	                             (first == 1 ? "" : ","),
	                             idi->dbinfo.instance_id,
	                             es[0],
	                             es[1]
	                            ) == -1)
	                        buf1 = NULL;
                        break;
                default:
	                if (asprintf(&buf1, "%s%s(%lu, '%s', '%s')",
	                             buf1,
	                             (first == 1 ? "" : ","),
	                             idi->dbinfo.instance_id,
	                             es[0],
	                             es[1]
	                            ) == -1)
	                        buf1 = NULL;
                        break;
                }
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_runtimevariables"
		                      "(Pos %lu) Id=%lu %s=%s\n",
		                      arrsize + count, idi->dbinfo.instance_id, es[0], es[1]);
		/* copy instanceid and configfile_id to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;

		/* take care about string len not exceeding varname column length */
		len = (strlen(es[0]) < CONFIG_VARNAME_SIZE) ? strlen(es[0]) : CONFIG_VARNAME_SIZE;
		es[0][len+1] = 0; //make sure string is terminated
		/* copy to string array buffer at the right position */
		strcpy(&name_arr[arrsize*(CONFIG_VARNAME_SIZE+1)], es[0]);
		/* take care about string len not exceeding varvalue column length */
		len = (strlen(es[1]) < CONFIG_VARVALUE_SIZE) ? strlen(es[1]) : CONFIG_VARVALUE_SIZE;
		es[1][len+1] = 0; //make sure string is terminated
		/* copy to string array buffer at the right position */
		strcpy(&val_arr[arrsize*(CONFIG_VARVALUE_SIZE+1)], es[1]);
		arrsize++;

#endif /* Oracle ocilib specific */

		free(es[0]);
		free(es[1]);

		free(buf);
		first = 0;
#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_runtimevariables, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_runtimevariables)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_runtimevariables);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_runtimevariables():"
				                      "so far %d items committed\n", count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_runtimevariables()"
				                      "ERROR:Rollback %d items\n", arrsize);
				free(instid_arr);
				free(name_arr);
				free(val_arr);
				return IDO_ERROR;
			}

		}
#endif /* Oracle ocilib specific */

	}//for
#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remainig entries*/
	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_runtimevariables, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_runtimevariables)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_runtimevariables);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_runtimevariables()"
			                      " %d items finished\n", count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_runtimevariables()"
			                      "ERROR:Rollback %d items\n", arrsize);
		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_runtimevariables() Warning:No variables storable\n");
	}
	//cleanup array buffers
	free(name_arr);
	free(val_arr);
	free(instid_arr);

#endif /* Oracle ocilib specific */



#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif


	if (buf1 != NULL) free(buf1);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_runtimevariables() end\n");
	return IDO_OK;
}

/****************************************************************************/
/* OBJECT DEFINITION DATA HANDLERS                                          */
/****************************************************************************/

int ido2db_handle_configdumpstart(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int result = IDO_OK;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configdumpstart() start\n");

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* set config dump type */
	if (idi->buffered_input[IDO_DATA_CONFIGDUMPTYPE] != NULL && !strcmp(
	            idi->buffered_input[IDO_DATA_CONFIGDUMPTYPE],
	            IDO_API_CONFIGDUMP_RETAINED))
		idi->current_object_config_type = 1;
	else
		idi->current_object_config_type = 0;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configdumpstart() end\n");

	return IDO_OK;
}

int ido2db_handle_configdumpend(ido2db_idi *idi) {

	return IDO_OK;
}

int ido2db_handle_hostdefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long check_timeperiod_id = 0L;
	unsigned long notification_timeperiod_id = 0L;
	unsigned long check_command_id = 0L;
	unsigned long eventhandler_command_id = 0L;
	double check_interval = 0.0;
	double retry_interval = 0.0;
	int max_check_attempts = 0;
	double first_notification_delay = 0.0;
	double notification_interval = 0.0;
	int notify_on_down = 0;
	int notify_on_unreachable = 0;
	int notify_on_recovery = 0;
	int notify_on_flapping = 0;
	int notify_on_downtime = 0;
	int stalk_on_up = 0;
	int stalk_on_down = 0;
	int stalk_on_unreachable = 0;
	int flap_detection_enabled = 0;
	int flap_detection_on_up = 0;
	int flap_detection_on_down = 0;
	int flap_detection_on_unreachable = 0;
	int process_performance_data = 0;
	int freshness_checks_enabled = 0;
	int freshness_threshold = 0;
	int passive_checks_enabled = 0;
	int event_handler_enabled = 0;
	int active_checks_enabled = 0;
	int retain_status_information = 0;
	int retain_nonstatus_information = 0;
	int notifications_enabled = 0;
	int obsess_over_host = 0;
	int failure_prediction_enabled = 0;
	double low_flap_threshold = 0.0;
	double high_flap_threshold = 0.0;
	int have_2d_coords = 0;
	int x_2d = 0;
	int y_2d = 0;
	int have_3d_coords = 0;
	double x_3d = 0.0;
	double y_3d = 0.0;
	double z_3d = 0.0;
	unsigned long host_id = 0L;
	unsigned long member_id = 0L;
	int result = IDO_OK;
	char *es[14];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;

	ido2db_mbuf mbuf;
	char *cmdptr = NULL;
	char *argptr = NULL;
#ifdef USE_ORACLE
	char *seq_name = NULL;
	/* definitions for array binding */
	int  arrsize = 0;
	int count = 0;
	big_uint  *instid_arr;
	big_uint  *hostid_arr;
	big_uint  *memberid_arr;

#endif
	void *data[58];
	int first;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* convert vars */
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_HOSTCHECKINTERVAL], &check_interval);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_HOSTRETRYINTERVAL], &retry_interval);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HOSTMAXCHECKATTEMPTS], &max_check_attempts);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_FIRSTNOTIFICATIONDELAY], &first_notification_delay);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_HOSTNOTIFICATIONINTERVAL], &notification_interval);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTDOWN], &notify_on_down);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTUNREACHABLE], &notify_on_unreachable);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTRECOVERY],	&notify_on_recovery);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTFLAPPING],	&notify_on_flapping);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTDOWNTIME],	&notify_on_downtime);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STALKHOSTONUP], &stalk_on_up);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STALKHOSTOIDOWN], &stalk_on_down);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STALKHOSTONUNREACHABLE], &stalk_on_unreachable);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HOSTFLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONONUP], &flap_detection_on_up);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONOIDOWN], &flap_detection_on_down);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONONUNREACHABLE], &flap_detection_on_unreachable);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PROCESSHOSTPERFORMANCEDATA],	&process_performance_data);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HOSTFRESHNESSCHECKSENABLED],	&freshness_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HOSTFRESHNESSTHRESHOLD], &freshness_threshold);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PASSIVEHOSTCHECKSENABLED], &passive_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HOSTEVENTHANDLERENABLED], &event_handler_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ACTIVEHOSTCHECKSENABLED], &active_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_RETAINHOSTSTATUSINFORMATION], &retain_status_information);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_RETAINHOSTNONSTATUSINFORMATION], &retain_nonstatus_information);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HOSTNOTIFICATIONSENABLED], &notifications_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_OBSESSOVERHOST], &obsess_over_host);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HOSTFAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_LOWHOSTFLAPTHRESHOLD], &low_flap_threshold);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_HIGHHOSTFLAPTHRESHOLD], &high_flap_threshold);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HAVE2DCOORDS], &have_2d_coords);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_X2D], &x_2d);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_Y2D], &y_2d);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HAVE3DCOORDS], &have_3d_coords);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_X3D], &x_3d);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_Y3D], &y_3d);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_Z3D], &z_3d);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_HOSTADDRESS]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_HOSTFAILUREPREDICTIONOPTIONS]);

	/* get the check command */
	cmdptr = strtok(idi->buffered_input[IDO_DATA_HOSTCHECKCOMMAND], "!");
	argptr = strtok(NULL, "\x0");
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &check_command_id);
	es[2] = ido2db_db_escape_string(idi, argptr);

	/* get the event handler command */
	cmdptr = strtok(idi->buffered_input[IDO_DATA_HOSTEVENTHANDLER], "!");
	argptr = strtok(NULL, "\x0");
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &eventhandler_command_id);
	es[3] = ido2db_db_escape_string(idi, argptr);

	es[4] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_NOTES]);
	es[5] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_NOTESURL]);
	es[6] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_ACTIONURL]);
	es[7] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_ICONIMAGE]);
	es[8] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_ICONIMAGEALT]);
	es[9] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_VRMLIMAGE]);
	es[10] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_STATUSMAPIMAGE]);
	es[11] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_DISPLAYNAME]);
	es[12] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_HOSTALIAS]);
	es[13] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_HOSTADDRESS6]);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOSTNAME], NULL, &object_id);

	/* flag the object as being active */
	ido2db_set_object_as_active(idi, IDO2DB_OBJECTTYPE_HOST, object_id);

	/* get the timeperiod ids */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_HOSTCHECKPERIOD], NULL, &check_timeperiod_id);
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_HOSTNOTIFICATIONPERIOD], NULL, &notification_timeperiod_id);

	/* add definition to db */

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &es[12];
	data[4] = (void *) &es[11];
	data[5] = (void *) &es[0];
	data[6] = (void *) &check_command_id;
	data[7] = (void *) &es[2];
	data[8] = (void *) &eventhandler_command_id;
	data[9] = (void *) &es[3];
	data[10] = (void *) &check_timeperiod_id;
	data[11] = (void *) &notification_timeperiod_id;
	data[12] = (void *) &es[1];
	data[13] = (void *) &check_interval;
	data[14] = (void *) &retry_interval;
	data[15] = (void *) &max_check_attempts;
	data[16] = (void *) &first_notification_delay;
	data[17] = (void *) &notification_interval;
	data[18] = (void *) &notify_on_down;
	data[19] = (void *) &notify_on_unreachable;
	data[20] = (void *) &notify_on_recovery;
	data[21] = (void *) &notify_on_flapping;
	data[22] = (void *) &notify_on_downtime;
	data[23] = (void *) &stalk_on_up;
	data[24] = (void *) &stalk_on_down;
	data[25] = (void *) &stalk_on_unreachable;
	data[26] = (void *) &flap_detection_enabled;
	data[27] = (void *) &flap_detection_on_up;
	data[28] = (void *) &flap_detection_on_down;
	data[29] = (void *) &flap_detection_on_unreachable;
	data[30] = (void *) &low_flap_threshold;
	data[31] = (void *) &high_flap_threshold;
	data[32] = (void *) &process_performance_data;
	data[33] = (void *) &freshness_checks_enabled;
	data[34] = (void *) &freshness_threshold;
	data[35] = (void *) &passive_checks_enabled;
	data[36] = (void *) &event_handler_enabled;
	data[37] = (void *) &active_checks_enabled;
	data[38] = (void *) &retain_status_information;
	data[39] = (void *) &retain_nonstatus_information;
	data[40] = (void *) &notifications_enabled;
	data[41] = (void *) &obsess_over_host;
	data[42] = (void *) &failure_prediction_enabled;
	data[43] = (void *) &es[4];
	data[44] = (void *) &es[5];
	data[45] = (void *) &es[6];
	data[46] = (void *) &es[7];
	data[47] = (void *) &es[8];
	data[48] = (void *) &es[9];
	data[49] = (void *) &es[10];
	data[50] = (void *) &have_2d_coords;
	data[51] = (void *) &x_2d;
	data[52] = (void *) &y_2d;
	data[53] = (void *) &have_3d_coords;
	data[54] = (void *) &x_3d;
	data[55] = (void *) &y_3d;
	data[56] = (void *) &z_3d;
	data[57] = (void *) &es[13]; /* HOSTADDRESS6 */

	result = ido2db_query_insert_or_update_hostdefinition_definition_add(idi, data);

	if (result == IDO_OK) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			host_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinitio(%lu) host_id\n", host_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_host_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS]) == -1)
				buf = NULL;

			host_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinitio(%s=%lu) host_id\n", buf, host_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_hosts") == -1)
			seq_name = NULL;
		host_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition(%lu) \n", host_id);
		free(seq_name);

#endif /* Oracle ocilib specific */
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() "
		                      "preceeding ido2db_query_insert_or_update_hostdefinition_definition_add ERROR \n");
		return IDO_ERROR;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++) {
		free(es[x]);
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() free es\n");

	/* save parent hosts to db */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() parent_hosts start\n");

#ifdef USE_LIBDBI

	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, host_id, parent_host_object_id) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTPARENTHOSTS]
	            ) == -1)
		buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* allocate array buffers based an maximal expected sizes*/
	instid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	hostid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	memberid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));

	if ((instid_arr == NULL) || (hostid_arr == NULL) || (memberid_arr == NULL)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition() parent"
		                      "  ArrayVars Alloc ERROR, exit\n");
		if (instid_arr) free(instid_arr);
		if (hostid_arr) free(hostid_arr);
		if (memberid_arr) free(memberid_arr);
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition() parent"
	                      " ArrayVars OK\n");
	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostdefinition_parenthosts, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostdefinition_parenthosts, MT(":X2"), hostid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostdefinition_parenthosts, MT(":X3"), memberid_arr, 0);
	arrsize = 0;

#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_PARENTHOST];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST,
		         mbuf.buffer[x], NULL, &member_id);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
		if (asprintf(&buf1, "%s%s(%lu,%lu,%lu)",
		             buf1,
		             (first == 1 ? "" : ","),
		             idi->dbinfo.instance_id,
		             host_id,
		             member_id
		            ) == -1)
			buf1 = NULL;
#endif
#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition parent "
		                      "(Pos %lu) Id=%lu host=%lu member=%lu\n",
		                      arrsize + count, idi->dbinfo.instance_id, host_id, member_id);
		/* copy ids to to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		hostid_arr[arrsize] = host_id;
		memberid_arr[arrsize] = member_id;
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		first = 0;

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostdefinition_parenthosts, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_hostdefinition_parenthosts)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_hostdefinition_parenthosts);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition parent "
				                      "(hostid %lu):so far %d items committed\n", host_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition parent "
				                      "(hostid %lu) ERROR:Rollback %d items\n", host_id, arrsize);
				arrsize = 0;
			}

		}
#endif /* Oracle ocilib specific */


	}//for

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */



#ifdef USE_LIBDBI /* everything else will be libdbi */

	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remaining entries*/

	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostdefinition_parenthosts, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_hostdefinition_parenthosts)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_hostdefinition_parenthosts);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition parent"
			                      "(hostid %lu): %d items finished\n", host_id, count);

			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition parent"
			                      "(hostid %lu) ERROR:Rollback %d items \n", host_id, arrsize);

		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition parent"
		                      "(hostid %lu) Warning:No members storable\n", host_id);

	}

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() parent_hosts end\n");
	/* save contact groups to db */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() contactgroups start\n");
#ifdef USE_LIBDBI

	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, host_id, contactgroup_object_id) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTGROUPS]
	            ) == -1)
		buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostdefinition_contactgroups, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostdefinition_contactgroups, MT(":X2"), hostid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostdefinition_contactgroups, MT(":X3"), memberid_arr, 0);
	arrsize = 0;

#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACTGROUP];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_CONTACTGROUP, mbuf.buffer[x], NULL,
		         &member_id);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
		if (asprintf(&buf1, "%s%s(%lu,%lu,%lu)",
		             buf1,
		             (first == 1 ? "" : ","),
		             idi->dbinfo.instance_id,
		             host_id,
		             member_id
		            ) == -1)
			buf1 = NULL;
#endif
#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition contactgroups "
		                      "(Pos %lu) Id=%lu host=%lu member=%lu\n",
		                      arrsize + count, idi->dbinfo.instance_id, host_id, member_id);
		/* copy ids to to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		hostid_arr[arrsize] = host_id;
		memberid_arr[arrsize] = member_id;
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		first = 0;
#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostdefinition_contactgroups, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_hostdefinition_contactgroups)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_hostdefinition_contactgroups);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contactgroups"
				                      "(hostid %lu):so far %d items committed\n", host_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contactgroups"
				                      "(hostid %lu) ERROR:Rollback %d items\n", host_id, arrsize);
				arrsize = 0;
			}

		}
#endif /* Oracle ocilib specific */

	}//for

#ifdef USE_ORACLE /* Oracle ocilib specific */



#endif /* Oracle ocilib specific */



#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remaining entries*/

	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostdefinition_contactgroups, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_hostdefinition_contactgroups)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_hostdefinition_contactgroups);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contactgroups"
			                      "(hostid %lu): %d items finished\n", host_id, count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contactgroups"
			                      "(hostid %lu) ERROR:Rollback %d items \n", host_id, arrsize);

		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contactgroups"
		                      "(hostid %lu) Warning:No contactgroups storable\n", host_id);

	}

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() contactsgroups end\n");

	/* save contacts to db */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() host_contacts start\n");

#ifdef USE_LIBDBI
	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id,host_id,contact_object_id) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTS]
	            ) == -1)
		buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostdefinition_contacts, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostdefinition_contacts, MT(":X2"), hostid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostdefinition_contacts, MT(":X3"), memberid_arr, 0);
	arrsize = 0;

#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACT];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
		if (asprintf(&buf1, "%s%s(%lu,%lu,%lu)",
		             buf1,
		             (first == 1 ? "" : ","),
		             idi->dbinfo.instance_id,
		             host_id,
		             member_id
		            ) == -1)
			buf1 = NULL;
#endif
#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition contacts "
		                      "(Pos %lu) Id=%lu host=%lu member=%lu\n",
		                      arrsize + count, idi->dbinfo.instance_id, host_id, member_id);
		/* copy ids to to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		hostid_arr[arrsize] = host_id;
		memberid_arr[arrsize] = member_id;
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		first = 0;
#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostdefinition_contacts, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_hostdefinition_contacts)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_hostdefinition_contacts);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contacts"
				                      "(hostid %lu):so far %d items committed\n", host_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contacts"
				                      "(hostid %lu) ERROR:Rollback %d items\n", host_id, arrsize);
				arrsize = 0;
			}

		}
#endif /* Oracle ocilib specific */
	}//for

#ifdef USE_ORACLE /* Oracle ocilib specific */
#endif /* Oracle ocilib specific */


#ifdef USE_LIBDBI /* everything else will be libdbi */

	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remaining entries*/

	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostdefinition_contacts, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_hostdefinition_contacts)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_hostdefinition_contacts);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contacts"
			                      "(hostid %lu): %d items finished\n", host_id, count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contacts"
			                      "(hostid %lu) ERROR:Rollback %d items \n", host_id, arrsize);

		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition contacts"
		                      "(hostid %lu) Warning:No contacts storable\n", host_id);

	}

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() host_contacts end\n");

#ifdef USE_ORACLE
	//cleanup array buffers
	free(hostid_arr);
	free(instid_arr);
	free(memberid_arr);
#endif

	/* save custom variables to db */
	result = ido2db_save_custom_variables(idi, IDO2DB_DBTABLE_CUSTOMVARIABLES, object_id, NULL, -1);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_hostgroupdefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long group_id = 0L;
	unsigned long member_id = 0L;
	int result = IDO_OK;
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	ido2db_mbuf mbuf;
#ifdef USE_ORACLE
	char *seq_name = NULL;
	/* definitions for array binding */
	int  arrsize = 0;
	int count = 0;
	big_uint  *instid_arr;
	big_uint  *groupid_arr;
	big_uint  *memberid_arr;

#endif
	void *data[4];
	int first;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_HOSTGROUPALIAS]);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOSTGROUP, idi->buffered_input[IDO_DATA_HOSTGROUPNAME], NULL, &object_id);

	/* flag the object as being active */
	ido2db_set_object_as_active(idi, IDO2DB_OBJECTTYPE_HOSTGROUP, object_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &es[0];

	result = ido2db_query_insert_or_update_hostgroupdefinition_definition_add(idi, data);

	if (result == IDO_OK) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition(%lu) hostgroup_id\n", group_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_hostgroup_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS]) == -1)
				buf = NULL;

			group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition(%s=%lu) hostgroup_id\n", buf, group_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_hostgroups") == -1)
			seq_name = NULL;
		group_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition(%lu) hostgroup_id\n", group_id);
		free(seq_name);

#endif /* Oracle ocilib specific */
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition() "
		                      "preeceding ido2db_query_insert_or_update_hostgroupdefinition_definition_add ERROR \n");
		return IDO_ERROR;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	free(es[0]);

	/* save hostgroup members to db */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition() members start\n");

#ifdef USE_LIBDBI

	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, hostgroup_id, host_object_id) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPMEMBERS]
	            ) == -1)
		buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* allocate array buffers based an maximal expected sizes*/
	instid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	groupid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	memberid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));

	if ((instid_arr == NULL) || (groupid_arr == NULL) || (memberid_arr == NULL)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostgroupdefinition() members"
		                      "  ArrayVars Alloc ERROR, exit\n");
		if (instid_arr) free(instid_arr);
		if (groupid_arr) free(groupid_arr);
		if (memberid_arr) free(memberid_arr);
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostdefinition() members ArrayVars OK\n");
	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, MT(":X2"), groupid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, MT(":X3"), memberid_arr, 0);
	arrsize = 0;

#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_HOSTGROUPMEMBER];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST,
		         mbuf.buffer[x], NULL, &member_id);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
		if (asprintf(&buf1, "%s%s(%lu,%lu,%lu)",
		             buf1,
		             (first == 1 ? "" : ","),
		             idi->dbinfo.instance_id,
		             group_id,
		             member_id
		            ) == -1)
			buf1 = NULL;
#endif
#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition members "
		                      "(Pos %lu) Id=%lu group=%lu member=%lu\n",
		                      arrsize + count, idi->dbinfo.instance_id, group_id, member_id);
		/* copy ids to to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		groupid_arr[arrsize] = group_id;
		memberid_arr[arrsize] = member_id;
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		first = 0;

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostgroupdefinition members "
				                      "(groupid %lu):so far %d items committed\n", group_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostgroupdefinition members"
				                      "(groupid %lu) ERROR:Rollback %d items\n", group_id, arrsize);
				arrsize = 0;
			}

		}
#endif /* Oracle ocilib specific */

	}//for
#ifdef USE_ORACLE /* Oracle ocilib specific */

#endif /* Oracle ocilib specific */


#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remaining entries*/

	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostgroupdefinition members"
			                      "(groupid %lu): %d items finished\n", group_id, count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostgroupdefinition members"
			                      "groupid %lu) ERROR:Rollback %d items \n", group_id, arrsize);

		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_hostgroupdefinition members"
		                      "(grupidid %lu) Warning:No members storable\n", group_id);
	}

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);

#ifdef USE_ORACLE
	//cleanup array buffers
	free(groupid_arr);
	free(instid_arr);
	free(memberid_arr);
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_servicedefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long host_id = 0L;
	unsigned long check_timeperiod_id = 0L;
	unsigned long notification_timeperiod_id = 0L;
	unsigned long check_command_id = 0L;
	unsigned long eventhandler_command_id = 0L;
	double check_interval = 0.0;
	double retry_interval = 0.0;
	int max_check_attempts = 0;
	double first_notification_delay = 0.0;
	double notification_interval = 0.0;
	int notify_on_warning = 0;
	int notify_on_unknown = 0;
	int notify_on_critical = 0;
	int notify_on_recovery = 0;
	int notify_on_flapping = 0;
	int notify_on_downtime = 0;
	int stalk_on_ok = 0;
	int stalk_on_warning = 0;
	int stalk_on_unknown = 0;
	int stalk_on_critical = 0;
	int is_volatile = 0;
	int flap_detection_enabled = 0;
	int flap_detection_on_ok = 0;
	int flap_detection_on_warning = 0;
	int flap_detection_on_unknown = 0;
	int flap_detection_on_critical = 0;
	int process_performance_data = 0;
	int freshness_checks_enabled = 0;
	int freshness_threshold = 0;
	int passive_checks_enabled = 0;
	int event_handler_enabled = 0;
	int active_checks_enabled = 0;
	int retain_status_information = 0;
	int retain_nonstatus_information = 0;
	int notifications_enabled = 0;
	int obsess_over_service = 0;
	int failure_prediction_enabled = 0;
	double low_flap_threshold = 0.0;
	double high_flap_threshold = 0.0;
	unsigned long service_id = 0L;
	unsigned long member_id = 0L;
	int result = IDO_OK;
	char *es[9];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;

	ido2db_mbuf mbuf;
	char *cmdptr = NULL;
	char *argptr = NULL;
#ifdef USE_ORACLE
	char *seq_name = NULL;
	/* definitions for array binding */
	int  arrsize = 0;
	int count = 0;
	big_uint  *instid_arr;
	big_uint  *serviceid_arr;
	big_uint  *memberid_arr;

#endif
	void *data[51];
	int first;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* convert vars */
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_SERVICECHECKINTERVAL], &check_interval);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_SERVICERETRYINTERVAL], &retry_interval);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_MAXSERVICECHECKATTEMPTS], &max_check_attempts);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_FIRSTNOTIFICATIONDELAY], &first_notification_delay);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_SERVICENOTIFICATIONINTERVAL], &notification_interval);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICEWARNING], &notify_on_warning);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICEUNKNOWN], &notify_on_unknown);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICECRITICAL], &notify_on_critical);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICERECOVERY], &notify_on_recovery);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICEFLAPPING], &notify_on_flapping);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICEDOWNTIME], &notify_on_downtime);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STALKSERVICEONOK], &stalk_on_ok);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STALKSERVICEONWARNING], &stalk_on_warning);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STALKSERVICEONUNKNOWN], &stalk_on_unknown);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_STALKSERVICEONCRITICAL], &stalk_on_critical);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SERVICEISVOLATILE], &is_volatile);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SERVICEFLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONONOK], &flap_detection_on_ok);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONONWARNING], &flap_detection_on_warning);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONONUNKNOWN], &flap_detection_on_unknown);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FLAPDETECTIONONCRITICAL], &flap_detection_on_critical);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PROCESSSERVICEPERFORMANCEDATA], &process_performance_data);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SERVICEFRESHNESSCHECKSENABLED], &freshness_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SERVICEFRESHNESSTHRESHOLD], &freshness_threshold);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_PASSIVESERVICECHECKSENABLED], &passive_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SERVICEEVENTHANDLERENABLED], &event_handler_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ACTIVESERVICECHECKSENABLED], &active_checks_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_RETAINSERVICESTATUSINFORMATION], &retain_status_information);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_RETAINSERVICENONSTATUSINFORMATION], &retain_nonstatus_information);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SERVICENOTIFICATIONSENABLED], &notifications_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_OBSESSOVERSERVICE], &obsess_over_service);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SERVICEFAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_LOWSERVICEFLAPTHRESHOLD], &low_flap_threshold);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_HIGHSERVICEFLAPTHRESHOLD], &high_flap_threshold);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS]);

	/* get the check command */
	cmdptr = strtok(idi->buffered_input[IDO_DATA_SERVICECHECKCOMMAND], "!");
	argptr = strtok(NULL, "\x0");

	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &check_command_id);

	es[1] = ido2db_db_escape_string(idi, argptr);

	/* get the event handler command */
	cmdptr = strtok(idi->buffered_input[IDO_DATA_SERVICEEVENTHANDLER], "!");
	argptr = strtok(NULL, "\x0");

	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &eventhandler_command_id);

	es[2] = ido2db_db_escape_string(idi, argptr);
	es[3] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_NOTES]);
	es[4] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_NOTESURL]);
	es[5] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_ACTIONURL]);
	es[6] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_ICONIMAGE]);
	es[7] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_ICONIMAGEALT]);
	es[8] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_DISPLAYNAME]);

	/* get the object ids */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOSTNAME], idi->buffered_input[IDO_DATA_SERVICEDESCRIPTION], &object_id);
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOSTNAME], NULL, &host_id);

	/* flag the object as being active */
	ido2db_set_object_as_active(idi, IDO2DB_OBJECTTYPE_SERVICE, object_id);

	/* get the timeperiod ids */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_SERVICECHECKPERIOD], NULL, &check_timeperiod_id);
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_SERVICENOTIFICATIONPERIOD], NULL, &notification_timeperiod_id);

	/* add definition to db */

	/* save entry to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &host_id;
	data[3] = (void *) &object_id;
	data[4] = (void *) &es[8];
	data[5] = (void *) &check_command_id;
	data[6] = (void *) &es[1];
	data[7] = (void *) &eventhandler_command_id;
	data[8] = (void *) &es[2];
	data[9] = (void *) &check_timeperiod_id;
	data[10] = (void *) &notification_timeperiod_id;
	data[11] = (void *) &es[0];
	data[12] = (void *) &check_interval;
	data[13] = (void *) &retry_interval;
	data[14] = (void *) &max_check_attempts;
	data[15] = (void *) &first_notification_delay;
	data[16] = (void *) &notification_interval;
	data[17] = (void *) &notify_on_warning;
	data[18] = (void *) &notify_on_unknown;
	data[19] = (void *) &notify_on_critical;
	data[20] = (void *) &notify_on_recovery;
	data[21] = (void *) &notify_on_flapping;
	data[22] = (void *) &notify_on_downtime;
	data[23] = (void *) &stalk_on_ok;
	data[24] = (void *) &stalk_on_warning;
	data[25] = (void *) &stalk_on_unknown;
	data[26] = (void *) &stalk_on_critical;
	data[27] = (void *) &is_volatile;
	data[28] = (void *) &flap_detection_enabled;
	data[29] = (void *) &flap_detection_on_ok;
	data[30] = (void *) &flap_detection_on_warning;
	data[31] = (void *) &flap_detection_on_unknown;
	data[32] = (void *) &flap_detection_on_critical;
	data[33] = (void *) &low_flap_threshold;
	data[34] = (void *) &high_flap_threshold;
	data[35] = (void *) &process_performance_data;
	data[36] = (void *) &freshness_checks_enabled;
	data[37] = (void *) &freshness_threshold;
	data[38] = (void *) &passive_checks_enabled;
	data[39] = (void *) &event_handler_enabled;
	data[40] = (void *) &active_checks_enabled;
	data[41] = (void *) &retain_status_information;
	data[42] = (void *) &retain_nonstatus_information;
	data[43] = (void *) &notifications_enabled;
	data[44] = (void *) &obsess_over_service;
	data[45] = (void *) &failure_prediction_enabled;
	data[46] = (void *) &es[3];
	data[47] = (void *) &es[4];
	data[48] = (void *) &es[5];
	data[49] = (void *) &es[6];
	data[50] = (void *) &es[7];

	result = ido2db_query_insert_or_update_servicedefinition_definition_add(idi, data);

	if (result == IDO_OK) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			service_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition(%lu) service_id\n", service_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_service_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES]) == -1)
				buf = NULL;

			service_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition(%s=%lu) service_id\n", buf, service_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_services") == -1)
			seq_name = NULL;
		service_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition(%lu) service_id\n", service_id);
		free(seq_name);

#endif /* Oracle ocilib specific */
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition() "
		                      "preceeding ido2db_query_insert_or_update_servicedefinition_definition_add ERROR \n");
		return IDO_ERROR;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++) {
		free(es[x]);
	}

	/* save contact groups to db */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition() contactgroups start\n");

#ifdef USE_LIBDBI

	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, service_id, contactgroup_object_id) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTGROUPS]
	            ) == -1)
		buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* allocate array buffers based an maximal expected sizes*/
	instid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	serviceid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	memberid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));

	if ((instid_arr == NULL) || (serviceid_arr == NULL) || (memberid_arr == NULL)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition() contactgroups"
		                      "  ArrayVars Alloc ERROR, exit\n");
		if (instid_arr) free(instid_arr);
		if (serviceid_arr) free(serviceid_arr);
		if (memberid_arr) free(memberid_arr);
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition() contactgroups"
	                      " ArrayVars OK\n");
	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_servicedefinition_contactgroups, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_servicedefinition_contactgroups, MT(":X2"), serviceid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_servicedefinition_contactgroups, MT(":X3"), memberid_arr, 0);
	arrsize = 0;


#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACTGROUP];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_CONTACTGROUP, mbuf.buffer[x], NULL,
		         &member_id);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
		if (asprintf(&buf1, "%s%s(%lu,%lu,%lu)",
		             buf1,
		             (first == 1 ? "" : ","),
		             idi->dbinfo.instance_id,
		             service_id,
		             member_id
		            ) == -1)
			buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition contactgroups "
		                      "(Pos %lu) Id=%lu service=%lu member=%lu\n",
		                      arrsize + count, idi->dbinfo.instance_id, service_id, member_id);
		/* copy ids to to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		serviceid_arr[arrsize] = service_id;
		memberid_arr[arrsize] = member_id;
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		first = 0;
#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_servicedefinition_contactgroups, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_servicedefinition_contactgroups)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_servicedefinition_contactgroups);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contactgroups"
				                      "(serviceid %lu):so far %d items committed\n", service_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contactgroups"
				                      "(hostid %lu) ERROR:Rollback %d items\n", service_id, arrsize);
				arrsize = 0;
			}

		}
#endif /* Oracle ocilib specific */

	}//for
#ifdef USE_ORACLE /* Oracle ocilib specific */

#endif /* Oracle ocilib specific */


#ifdef USE_LIBDBI /* everything else will be libdbi */

	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif
#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remaining entries*/

	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_servicedefinition_contactgroups, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_servicedefinition_contactgroups)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_servicedefinition_contactgroups);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contactgroups"
			                      "(hostid %lu): %d items finished\n", service_id, count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contactgroups"
			                      "(hostid %lu) ERROR:Rollback %d items \n", service_id, arrsize);

		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contactgroups"
		                      "(hostid %lu) Warning:No contactgroups storable\n", service_id);

	}

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition() contactsgroups end\n");


	/* save contacts to db */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition() service_contacts start\n");

#ifdef USE_LIBDBI
	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, service_id, contact_object_id) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTS]
	            ) == -1)
		buf1 = NULL;
#endif


#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_servicedefinition_contacts, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_servicedefinition_contacts, MT(":X2"), serviceid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_servicedefinition_contacts, MT(":X3"), memberid_arr, 0);
	arrsize = 0;


#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACT];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
		if (asprintf(&buf1, "%s%s(%lu,%lu,%lu)",
		             buf1,
		             (first == 1 ? "" : ","),
		             idi->dbinfo.instance_id,
		             service_id,
		             member_id
		            ) == -1)
			buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition contacts "
		                      "(Pos %lu) Id=%lu service=%lu member=%lu\n",
		                      arrsize + count, idi->dbinfo.instance_id, service_id, member_id);
		/* copy ids to to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		serviceid_arr[arrsize] = service_id;
		memberid_arr[arrsize] = member_id;
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		first = 0;
#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_servicedefinition_contacts, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_servicedefinition_contacts)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_servicedefinition_contacts);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contacts"
				                      "(hostid %lu):so far %d items committed\n", service_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contacts"
				                      "(hostid %lu) ERROR:Rollback %d items\n", service_id, arrsize);
				arrsize = 0;
			}

		}
#endif /* Oracle ocilib specific */
	}//for

#ifdef USE_ORACLE /* Oracle ocilib specific */
#endif /* Oracle ocilib specific */



#ifdef USE_ORACLE /* Oracle ocilib specific */

#endif /* Oracle ocilib specific */



#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif
#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remaining entries*/

	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_servicedefinition_contacts, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_servicedefinition_contacts)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_servicedefinition_contacts);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contacts"
			                      "(hostid %lu): %d items finished\n", service_id, count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contacts"
			                      "(hostid %lu) ERROR:Rollback %d items \n", service_id, arrsize);

		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition contacts"
		                      "(hostid %lu) Warning:No contacts storable\n", service_id);

	}

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);

#ifdef USE_ORACLE
	//cleanup array buffers
	free(serviceid_arr);
	free(instid_arr);
	free(memberid_arr);
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition() service_contacts end\n");

	/* save custom variables to db */
	result = ido2db_save_custom_variables(idi, IDO2DB_DBTABLE_CUSTOMVARIABLES, object_id, NULL, -1);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_servicegroupdefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long group_id = 0L;
	unsigned long member_id = 0L;
	int result = IDO_OK;
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	ido2db_mbuf mbuf;
	char *hptr = NULL;
	char *sptr = NULL;
#ifdef USE_ORACLE
	char *seq_name = NULL;
	/* definitions for array binding */
	int  arrsize = 0;
	int count = 0;
	big_uint  *instid_arr;
	big_uint  *groupid_arr;
	big_uint  *memberid_arr;
#endif
	void *data[4];
	int first;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_SERVICEGROUPALIAS]);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICEGROUP, idi->buffered_input[IDO_DATA_SERVICEGROUPNAME], NULL, &object_id);

	/* flag the object as being active */
	ido2db_set_object_as_active(idi, IDO2DB_OBJECTTYPE_SERVICEGROUP, object_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &es[0];

	result = ido2db_query_insert_or_update_servicegroupdefinition_definition_add(idi, data);

	if (result == IDO_OK) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition(%lu) group_id\n", group_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_servicegroup_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS]) == -1)
				buf = NULL;

			group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition(%s=%lu) group_id\n", buf, group_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_servicegroups") == -1)
			seq_name = NULL;
		group_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition(%lu) group_id\n", group_id);
		free(seq_name);

#endif /* Oracle ocilib specific */
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition() "
		                      "preeceding ido2db_query_insert_or_update_servicegroupdefinition_definition_add ERROR \n");
		return IDO_ERROR;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	free(es[0]);

	/* save members to db */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition() "
	                      "members  start\n");

#ifdef USE_LIBDBI

	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, servicegroup_id, service_object_id) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPMEMBERS]
	            ) == -1)
		buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* allocate array buffers based an maximal expected sizes*/
	instid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	groupid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	memberid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));

	if ((instid_arr == NULL) || (groupid_arr == NULL) || (memberid_arr == NULL)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicegroupdefinition() "
		                      "members  ArrayVars Alloc ERROR, exit\n");
		if (instid_arr) free(instid_arr);
		if (groupid_arr) free(groupid_arr);
		if (memberid_arr) free(memberid_arr);
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicedefinition()"
	                      " members ArrayVars OK\n");
	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_servicegroupdefinition_members, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_servicegroupdefinition_members, MT(":X2"), groupid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_servicegroupdefinition_members, MT(":X3"), memberid_arr, 0);
	arrsize = 0;

#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_SERVICEGROUPMEMBER];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* split the host/service name */
		hptr = strtok(mbuf.buffer[x], ";");
		sptr = strtok(NULL, "\x0");

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_SERVICE, hptr, sptr, &member_id);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
		if (asprintf(&buf1, "%s%s(%lu,%lu,%lu)",
		             buf1,
		             (first == 1 ? "" : ","),
		             idi->dbinfo.instance_id,
		             group_id,
		             member_id
		            ) == -1)
			buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition members "
		                      "(Pos %lu) Id=%lu group=%lu member=%lu\n",
		                      arrsize + count, idi->dbinfo.instance_id, group_id, member_id);
		/* copy ids to to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		groupid_arr[arrsize] = group_id;
		memberid_arr[arrsize] = member_id;
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		first = 0;

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_servicegroupdefinition_members, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_servicegroupdefinition_members)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_servicegroupdefinition_members);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicegroupdefinition members "
				                      "(groupid %lu):so far %d items committed\n", group_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicegroupdefinition members"
				                      "(groupid %lu) ERROR:Rollback %d items\n", group_id, arrsize);
				arrsize = 0;
			}

		}
#endif /* Oracle ocilib specific */

	}//for
#ifdef USE_ORACLE /* Oracle ocilib specific */

#endif /* Oracle ocilib specific */

#ifdef USE_LIBDBI /* everything else will be libdbi */

	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif
#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remaining entries*/

	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_servicegroupdefinition_members, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_servicegroupdefinition_members)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_servicegroupdefinition_members);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicegroupdefinition members"
			                      "(groupid %lu): %d items finished\n", group_id, count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicegroupdefinition members"
			                      "groupid %lu) ERROR:Rollback %d items \n", group_id, arrsize);

		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_servicegroupdefinition members"
		                      "(groupid %lu) Warning:No members storable\n", group_id);
	}

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);

#ifdef USE_ORACLE
	//cleanup array buffers
	free(groupid_arr);
	free(instid_arr);
	free(memberid_arr);
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_hostdependencydefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long dependent_object_id = 0L;
	unsigned long timeperiod_object_id = 0L;
	int dependency_type = 0;
	int inherits_parent = 0;
	int fail_on_up = 0;
	int fail_on_down = 0;
	int fail_on_unreachable = 0;
	int result = IDO_OK;
	void *data[10];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdependencydefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_DEPENDENCYTYPE], &dependency_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_INHERITSPARENT], &inherits_parent);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILONUP], &fail_on_up);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILOIDOWN], &fail_on_down);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILONUNREACHABLE], &fail_on_unreachable);

	/* get the object ids */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOSTNAME], NULL, &object_id);
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_DEPENDENTHOSTNAME], NULL, &dependent_object_id);
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_DEPENDENCYPERIOD], NULL, &timeperiod_object_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &dependent_object_id;
	data[4] = (void *) &dependency_type;
	data[5] = (void *) &inherits_parent;
	data[6] = (void *) &timeperiod_object_id;
	data[7] = (void *) &fail_on_up;
	data[8] = (void *) &fail_on_down;
	data[9] = (void *) &fail_on_unreachable;

	result = ido2db_query_insert_or_update_hostdependencydefinition_definition_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdependencydefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_servicedependencydefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long dependent_object_id = 0L;
	unsigned long timeperiod_object_id = 0L;
	int dependency_type = 0;
	int inherits_parent = 0;
	int fail_on_ok = 0;
	int fail_on_warning = 0;
	int fail_on_unknown = 0;
	int fail_on_critical = 0;
	int result = IDO_OK;
	void *data[11];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedependencydefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_DEPENDENCYTYPE], &dependency_type);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_INHERITSPARENT], &inherits_parent);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILONOK], &fail_on_ok);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILONWARNING], &fail_on_warning);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILONUNKNOWN], &fail_on_unknown);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FAILONCRITICAL], &fail_on_critical);

	/* get the object ids */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOSTNAME], idi->buffered_input[IDO_DATA_SERVICEDESCRIPTION], &object_id);
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_DEPENDENTHOSTNAME], idi->buffered_input[IDO_DATA_DEPENDENTSERVICEDESCRIPTION], &dependent_object_id);
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_DEPENDENCYPERIOD], NULL, &timeperiod_object_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &dependent_object_id;
	data[4] = (void *) &dependency_type;
	data[5] = (void *) &inherits_parent;
	data[6] = (void *) &timeperiod_object_id;
	data[7] = (void *) &fail_on_ok;
	data[8] = (void *) &fail_on_warning;
	data[9] = (void *) &fail_on_unknown;
	data[10] = (void *) &fail_on_critical;

	result = ido2db_query_insert_or_update_servicedependencydefinition_definition_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedependencydefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_hostescalationdefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long timeperiod_id = 0L;
	unsigned long escalation_id = 0L;
	unsigned long member_id = 0L;
	int first_notification = 0;
	int last_notification = 0;
	double notification_interval = 0.0;
	int escalate_recovery = 0;
	int escalate_down = 0;
	int escalate_unreachable = 0;
	int result = IDO_OK;
	int x = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif
	ido2db_mbuf mbuf;
#ifdef USE_ORACLE
	char *seq_name = NULL;
#endif
	void *data[10];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FIRSTNOTIFICATION], &first_notification);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_LASTNOTIFICATION], &last_notification);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_NOTIFICATIONINTERVAL], &notification_interval);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ESCALATEONRECOVERY], &escalate_recovery);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ESCALATEOIDOWN], &escalate_down);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ESCALATEONUNREACHABLE], &escalate_unreachable);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_HOST, idi->buffered_input[IDO_DATA_HOSTNAME], NULL, &object_id);

	/* get the timeperiod id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_ESCALATIONPERIOD], NULL, &timeperiod_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &timeperiod_id;
	data[4] = (void *) &first_notification;
	data[5] = (void *) &last_notification;
	data[6] = (void *) &notification_interval;
	data[7] = (void *) &escalate_recovery;
	data[8] = (void *) &escalate_down;
	data[9] = (void *) &escalate_unreachable;

	result = ido2db_query_insert_or_update_hostescalationdefinition_definition_add(idi, data);

	if (result == IDO_OK) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			escalation_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition(%lu) escalation_id\n", escalation_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_hostescalation_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS]) == -1)
				buf = NULL;

			escalation_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition(%s=%lu) escalation_id\n", buf, escalation_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_hostescalations") == -1)
			seq_name = NULL;
		escalation_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition(%lu) escalation_id\n", escalation_id);
		free(seq_name);

#endif /* Oracle ocilib specific */
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	/* save contact groups to db */
	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACTGROUP];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_CONTACTGROUP, mbuf.buffer[x], NULL,
		         &member_id);

		/* save entry to db */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &escalation_id;
		data[2] = (void *) &member_id;

		result = ido2db_query_insert_or_update_hostescalationdefinition_contactgroups_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	}

	/* save contacts to db */
	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACT];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		/* save entry tp db */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &escalation_id;
		data[2] = (void *) &member_id;

		result = ido2db_query_insert_or_update_hostescalationdefinition_contacts_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_serviceescalationdefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long timeperiod_id = 0L;
	unsigned long escalation_id = 0L;
	unsigned long member_id = 0L;
	int first_notification = 0;
	int last_notification = 0;
	double notification_interval = 0.0;
	int escalate_recovery = 0;
	int escalate_warning = 0;
	int escalate_unknown = 0;
	int escalate_critical = 0;
	int result = IDO_OK;
	int x = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif
	ido2db_mbuf mbuf;
#ifdef USE_ORACLE
	char *seq_name = NULL;
#endif
	void *data[11];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicetescalationdefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_FIRSTNOTIFICATION], &first_notification);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_LASTNOTIFICATION], &last_notification);
	result = ido2db_convert_string_to_double(idi->buffered_input[IDO_DATA_NOTIFICATIONINTERVAL], &notification_interval);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ESCALATEONRECOVERY], &escalate_recovery);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ESCALATEONWARNING], &escalate_warning);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ESCALATEONUNKNOWN], &escalate_unknown);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_ESCALATEONCRITICAL], &escalate_critical);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[IDO_DATA_HOSTNAME], idi->buffered_input[IDO_DATA_SERVICEDESCRIPTION], &object_id);

	/* get the timeperiod id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_ESCALATIONPERIOD], NULL, &timeperiod_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &timeperiod_id;
	data[4] = (void *) &first_notification;
	data[5] = (void *) &last_notification;
	data[6] = (void *) &notification_interval;
	data[7] = (void *) &escalate_recovery;
	data[8] = (void *) &escalate_warning;
	data[9] = (void *) &escalate_unknown;
	data[10] = (void *) &escalate_critical;

	result = ido2db_query_insert_or_update_serviceescalationdefinition_definition_add(idi, data);

	if (result == IDO_OK) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			escalation_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_serviceescalationdefinition(%lu) escalation_id\n", escalation_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_serviceescalation_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS]) == -1)
				buf = NULL;

			escalation_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_serviceescalationdefinition(%s=%lu) escalation_id\n", buf, escalation_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_serviceescalations") == -1)
			seq_name = NULL;
		escalation_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_serviceescalationdefinition(%lu) escalation_id\n", escalation_id);
		free(seq_name);

#endif /* Oracle ocilib specific */
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	/* save contact groups to db */
	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACTGROUP];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_CONTACTGROUP, mbuf.buffer[x], NULL,
		         &member_id);

		/* save entry to db */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &escalation_id;
		data[2] = (void *) &member_id;

		result = ido2db_query_insert_or_update_serviceescalationdefinition_contactgroups_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	}

	/* save contacts to db */
	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACT];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		/* save entry to db */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &escalation_id;
		data[2] = (void *) &member_id;

		result = ido2db_query_insert_or_update_serviceescalationdefinition_contacts_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicetescalationdefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_commanddefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	int result = IDO_OK;
	char *es[1];
	int x = 0;
	void *data[4];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_commanddefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_COMMANDLINE]);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, idi->buffered_input[IDO_DATA_COMMANDNAME], NULL, &object_id);

	/* flag the object as being active */
	ido2db_set_object_as_active(idi, IDO2DB_OBJECTTYPE_COMMAND, object_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &object_id;
	data[2] = (void *) &idi->current_object_config_type;
	data[3] = (void *) &es[0];

	result = ido2db_query_insert_or_update_commanddefinition_definition_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_commanddefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_timeperiodefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long timeperiod_id = 0L;
	char *dayptr = NULL;
	char *startptr = NULL;
	char *endptr = NULL;
	int day = 0;
	unsigned long start_sec = 0L;
	unsigned long end_sec = 0L;
	int result = IDO_OK;
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	ido2db_mbuf mbuf;
#ifdef USE_ORACLE
	char *seq_name = NULL;
	/* definitions for array binding */
	int  arrsize = 0;
	int count = 0;
	big_uint  *instid_arr;
	big_uint  *timeid_arr;
	int  *day_arr;
	big_uint  *startsec_arr;
	big_uint  *endsec_arr;
#endif
	void *data[5];
	int first;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr,
	         &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_TIMEPERIODALIAS]);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[IDO_DATA_TIMEPERIODNAME], NULL, &object_id);

	/* flag the object as being active */
	ido2db_set_object_as_active(idi, IDO2DB_OBJECTTYPE_TIMEPERIOD, object_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &es[0];

	result = ido2db_query_insert_or_update_timeperiodefinition_definition_add(idi, data);
	free(es[0]);
	if (result == IDO_OK) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition() "
		                      "preceeding do2db_query_insert_or_update_timeperiodefinition_definition_add OK \n");
#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			timeperiod_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition(%lu) timeperiod_id\n", timeperiod_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_timeperiod_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS]) == -1)
				buf = NULL;

			timeperiod_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition(%s=%lu) timeperiod_id\n", buf, timeperiod_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_timeperiods") == -1)
			seq_name = NULL;
		timeperiod_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition(%lu) "
		                      "timeperiod_id\n", timeperiod_id);
		free(seq_name);
#endif /* Oracle ocilib specific */

	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition() "
		                      "preceeding do2db_query_insert_or_update_timeperiodefinition_definition_add "
		                      "ERROR \n");
		return IDO_ERROR;
	}


#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */



	/* save timeranges to db */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition() "
	                      "timeranges start\n");

#ifdef USE_LIBDBI

	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, timeperiod_id, day, start_sec, end_sec) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODTIMERANGES]
	            ) == -1)
		buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* allocate array buffers based an maximal expected sizes*/
	instid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	timeid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	day_arr = (int *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(int));
	startsec_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	endsec_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));

	if ((instid_arr == NULL) || (timeid_arr == NULL) || (day_arr == NULL) || (startsec_arr == NULL) || (endsec_arr == NULL)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_timeperiodefinition_timeranges()"
		                      "  ArrayVars Alloc ERROR, exit\n");
		if (instid_arr) free(instid_arr);
		if (timeid_arr) free(timeid_arr);
		if (day_arr) free(day_arr);
		if (startsec_arr) free(startsec_arr);
		if (endsec_arr) free(endsec_arr);
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_timeperiodefinition_timeranges()"
	                      " ArrayVars OK\n");
	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X2"), timeid_arr, 0);
	OCI_BindArrayOfInts(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X3"), (int *)day_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X4"), startsec_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X5"), endsec_arr, 0);
	arrsize = 0;

#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_TIMERANGE];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;
		/* get var name/val pair */
		dayptr = strtok(mbuf.buffer[x], ":");
		startptr = strtok(NULL, "-");
		endptr = strtok(NULL, "\x0");

		if (startptr == NULL || endptr == NULL)
			continue;

		day = atoi(dayptr);
		start_sec = strtoul(startptr, NULL, 0);
		end_sec = strtoul(endptr, NULL, 0);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
		if (asprintf(&buf1, "%s%s(%lu,%lu,%d,%lu,%lu)",
		             buf1,
		             (first == 1 ? "" : ","),
		             idi->dbinfo.instance_id,
		             timeperiod_id,
		             day,
		             start_sec,
		             end_sec
		            ) == -1)
			buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition_timeranges"
		                      "(Pos %lu) Id=%lu period=%lu day=%lu,start=%lu,end=%lu\n",
		                      arrsize + count, idi->dbinfo.instance_id, timeperiod_id, day, start_sec, end_sec);
		/* copy instanceid and configfile_id to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		timeid_arr[arrsize] = timeperiod_id;
		day_arr[arrsize] = day;
		startsec_arr[arrsize] = start_sec;
		endsec_arr[arrsize] = end_sec;
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		first = 0;
#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_timeperiodefinition_timeranges)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_configfilevariables_insert);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_timeperiodefinition_timeranges"
				                      "(Period %lu):so far %d items committed\n", timeperiod_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_timeperiodefinition_timeranges"
				                      "(Period %lu) ERROR:Rollback %d items\n", timeperiod_id, arrsize);
				free(day_arr);
				free(startsec_arr);
				free(endsec_arr);
				free(instid_arr);
				free(timeid_arr);
				return IDO_ERROR;
			}

		}


#endif /* Oracle ocilib specific */
	}//for


#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remaining entries*/

	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_timeperiodefinition_timeranges)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_timeperiodefinition_timeranges);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_timeperiodefinition_timeranges"
			                      "(period %lu) %d items finished\n", timeperiod_id, count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_timeperiodefinition_timeranges"
			                      "(period %lu) ERROR:Rollback %d items\n", timeperiod_id, arrsize);
		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_timeperiodefinition_timeranges"
		                      "(period %lu) Warning:No ranges storable\n", timeperiod_id);
	}
	//cleanup array buffers
	free(day_arr);
	free(startsec_arr);
	free(endsec_arr);
	free(instid_arr);
	free(timeid_arr);

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition() end\n");

	return IDO_OK;
}

int ido2db_handle_contactdefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long contact_id = 0L;
	unsigned long host_timeperiod_id = 0L;
	unsigned long service_timeperiod_id = 0L;
	int host_notifications_enabled = 0;
	int service_notifications_enabled = 0;
	int can_submit_commands = 0;
	int notify_service_recovery = 0;
	int notify_service_warning = 0;
	int notify_service_unknown = 0;
	int notify_service_critical = 0;
	int notify_service_flapping = 0;
	int notify_service_downtime = 0;
	int notify_host_recovery = 0;
	int notify_host_down = 0;
	int notify_host_unreachable = 0;
	int notify_host_flapping = 0;
	int notify_host_downtime = 0;
	unsigned long command_id = 0L;
	int result = IDO_OK;
	char *es[3];
	int x = 0;
#ifdef USE_LIBDBI
	char *buf = NULL;
#endif
	ido2db_mbuf mbuf;
	char *numptr = NULL;
	char *addressptr = NULL;
	int address_number = 0;
	char *cmdptr = NULL;
	char *argptr = NULL;
#ifdef USE_ORACLE
	char *seq_name = NULL;
#endif
	void *data[22];

	int tmp1 = HOST_NOTIFICATION;
	int tmp2 = SERVICE_NOTIFICATION;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	/* convert vars */
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_HOSTNOTIFICATIONSENABLED], &host_notifications_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_SERVICENOTIFICATIONSENABLED], &service_notifications_enabled);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_CANSUBMITCOMMANDS], &can_submit_commands);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICEWARNING], &notify_service_warning);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICEUNKNOWN], &notify_service_unknown);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICECRITICAL], &notify_service_critical);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICERECOVERY], &notify_service_recovery);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICEFLAPPING], &notify_service_flapping);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYSERVICEDOWNTIME], &notify_service_downtime);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTDOWN], &notify_host_down);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTUNREACHABLE], &notify_host_unreachable);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTRECOVERY], &notify_host_recovery);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTFLAPPING],	&notify_host_flapping);
	result = ido2db_convert_string_to_int(idi->buffered_input[IDO_DATA_NOTIFYHOSTDOWNTIME], &notify_host_downtime);

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_CONTACTALIAS]);
	es[1] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_EMAILADDRESS]);
	es[2] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_PAGERADDRESS]);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_CONTACT,
	         idi->buffered_input[IDO_DATA_CONTACTNAME], NULL, &contact_id);

	/* get the timeperiod ids */
	result = ido2db_get_object_id_with_insert(idi,
	         IDO2DB_OBJECTTYPE_TIMEPERIOD,
	         idi->buffered_input[IDO_DATA_HOSTNOTIFICATIONPERIOD], NULL,
	         &host_timeperiod_id);
	result = ido2db_get_object_id_with_insert(idi,
	         IDO2DB_OBJECTTYPE_TIMEPERIOD,
	         idi->buffered_input[IDO_DATA_SERVICENOTIFICATIONPERIOD], NULL,
	         &service_timeperiod_id);

	/* flag the object as being active */
	ido2db_set_object_as_active(idi, IDO2DB_OBJECTTYPE_CONTACT, contact_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &contact_id;
	data[3] = (void *) &es[0];
	data[4] = (void *) &es[1];
	data[5] = (void *) &es[2];
	data[6] = (void *) &host_timeperiod_id;
	data[7] = (void *) &service_timeperiod_id;
	data[8] = (void *) &host_notifications_enabled;
	data[9] = (void *) &service_notifications_enabled;
	data[10] = (void *) &can_submit_commands;
	data[11] = (void *) &notify_service_recovery;
	data[12] = (void *) &notify_service_warning;
	data[13] = (void *) &notify_service_unknown;
	data[14] = (void *) &notify_service_critical;
	data[15] = (void *) &notify_service_flapping;
	data[16] = (void *) &notify_service_downtime;
	data[17] = (void *) &notify_host_recovery;
	data[18] = (void *) &notify_host_down;
	data[19] = (void *) &notify_host_unreachable;
	data[20] = (void *) &notify_host_flapping;
	data[21] = (void *) &notify_host_downtime;

	result = ido2db_query_insert_or_update_contactdefinition_definition_add(idi, data);

	if (result == IDO_OK) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			contact_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition(ido2db_idi *idi)(%lu) contact_id\n", contact_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_contact_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS]) == -1)
				buf = NULL;

			contact_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition(ido2db_idi *idi)(%s=%lu) contact_id\n", buf, contact_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (asprintf(&seq_name, "seq_contacts") == -1)
			seq_name = NULL;
		contact_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition(ido2db_idi *idi)(%lu) contact_id\n", contact_id);
		free(seq_name);
#endif /* Oracle ocilib specific */
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	for (x = 0; x < ICINGA_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	/* save addresses to db */
	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACTADDRESS];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		numptr = strtok(mbuf.buffer[x], ":");
		addressptr = strtok(NULL, "\x0");

		if (numptr == NULL || addressptr == NULL)
			continue;

		address_number = atoi(numptr);
		es[0] = ido2db_db_escape_string(idi, addressptr);

		/* save entry to db */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &contact_id;
		data[2] = (void *) &address_number;
		data[3] = (void *) &es[0];

		result = ido2db_query_insert_or_update_contactdefinition_addresses_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

		free(es[0]);
	}

	/* save host notification commands to db */
	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACTADDRESS];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		cmdptr = strtok(mbuf.buffer[x], "!");
		argptr = strtok(NULL, "\x0");

		if (numptr == NULL)
			//if (cmdptr == NULL || argptr == NULL)
			continue;

		/* find the command */
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &command_id);

		es[0] = ido2db_db_escape_string(idi, argptr);

		/* save entry to db */
		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &contact_id;
		data[2] = (void *) &tmp1;
		data[3] = (void *) &command_id;
		data[4] = (void *) &es[0];

		result = ido2db_query_insert_or_update_contactdefinition_notificationcommands_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* do not free if prepared statement */

#endif /* Oracle ocilib specific */

		free(es[0]);
	}

	/* save service notification commands to db */
	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACTADDRESS];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		cmdptr = strtok(mbuf.buffer[x], "!");
		argptr = strtok(NULL, "\x0");

		if (numptr == NULL)
			//if (cmdptr == NULL || argptr == NULL)
			continue;

		/* find the command */
		result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &command_id);

		es[0] = ido2db_db_escape_string(idi, argptr);

		/* save entry to db */

		data[0] = (void *) &idi->dbinfo.instance_id;
		data[1] = (void *) &contact_id;
		data[2] = (void *) &tmp2;
		data[3] = (void *) &command_id;
		data[4] = (void *) &es[0];

		result = ido2db_query_insert_or_update_contactdefinition_notificationcommands_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

		/* do not free if prepared statement */

#endif /* Oracle ocilib specific */

		free(es[0]);
	}

	/* save custom variables to db */
	result = ido2db_save_custom_variables(idi, IDO2DB_DBTABLE_CUSTOMVARIABLES, contact_id, NULL, -1);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition() end\n");

	return IDO_OK;
}

int ido2db_save_custom_variables(ido2db_idi *idi, int table_idx, unsigned long o_id, char *ts, unsigned long tstamp) {
	char *buf = NULL;
	ido2db_mbuf mbuf;
	char *es[2];
	char *ptr1 = NULL;
	char *ptr2 = NULL;
	char *ptr3 = NULL;
	int result = IDO_OK;
	int has_been_modified = 0;
	int x = 0;
	void *data[7];

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_save_custom_variables() start\n");

	/* save custom variables to db */
	mbuf = idi->mbuf[IDO2DB_MBUF_CUSTOMVARIABLE];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		if ((ptr1 = strtok(mbuf.buffer[x], ":")) == NULL)
			continue;

		es[0] = strdup(ptr1);

		if ((ptr2 = strtok(NULL, ":")) == NULL)
			continue;

		has_been_modified = atoi(ptr2);
		ptr3 = strtok(NULL, "\n");
		buf = strdup((ptr3 == NULL) ? "" : ptr3);
		es[1] = ido2db_db_escape_string(idi, buf);
		free(buf);

		if (ts == NULL) {
			if (asprintf(&ts, "NULL") == -1)
				;
		}

		if (table_idx == IDO2DB_DBTABLE_CUSTOMVARIABLES) {

			/* save entry to db */
			data[0] = (void *) &idi->dbinfo.instance_id;
			data[1] = (void *) &o_id;
			data[2] = (void *) &idi->current_object_config_type;
			data[3] = (void *) &has_been_modified;
			data[4] = (es[0] == NULL) ? NULL : (void *) &es[0];
			data[5] = (es[1] == NULL) ? NULL : (void *) &es[1];

			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_save_custom_variables() instance_id=%lu, object_id=%lu, config_type=%d, modified=%d, varname=%s, varvalue=%s\n", idi->dbinfo.instance_id, o_id, idi->current_object_config_type, has_been_modified, es[0], es[1]);

			result = ido2db_query_insert_or_update_save_custom_variables_customvariables_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

		}
		if (table_idx == IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS) {

			/* save entry to db */
			data[0] = (void *) &idi->dbinfo.instance_id;
			data[1] = (void *) &o_id;
			data[2] = (void *) &ts;
			data[3] = (void *) &has_been_modified;
			data[4] = (es[0] == NULL) ? NULL : (void *) &es[0];
			data[5] = (es[1] == NULL) ? NULL : (void *) &es[1];
			/* wtf is ts doing here? */
			data[6] = (void *) &tstamp;

			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_save_custom_variablestatus() instance_id=%lu, object_id=%lu, ts=%s, modified=%d, varname=%s, varvalue=%s\n", idi->dbinfo.instance_id, o_id, ts, has_been_modified, es[0], es[1]);

			result = ido2db_query_insert_or_update_save_custom_variables_customvariablestatus_add(idi, data);

#ifdef USE_LIBDBI /* everything else will be libdbi */
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

		}

		free(es[0]);
		free(es[1]);
	}

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_save_custom_variables() end\n");

	return result;

}

int ido2db_handle_contactgroupdefinition(ido2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long group_id = 0L;
	unsigned long member_id = 0L;
	int result = IDO_OK;
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	ido2db_mbuf mbuf;
#ifdef USE_ORACLE
	char *seq_name = NULL;
	/* definitions for array binding */
	int  arrsize = 0;
	int count = 0;
	big_uint  *instid_arr;
	big_uint  *groupid_arr;
	big_uint  *memberid_arr;
#endif
	void *data[4];
	int first;

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	/* convert timestamp, etc */
	result = ido2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return IDO_OK;

	es[0] = ido2db_db_escape_string(idi, idi->buffered_input[IDO_DATA_CONTACTGROUPALIAS]);

	/* get the object id */
	result = ido2db_get_object_id_with_insert(idi, IDO2DB_OBJECTTYPE_CONTACTGROUP, idi->buffered_input[IDO_DATA_CONTACTGROUPNAME], NULL, &object_id);

	/* flag the object as being active */
	ido2db_set_object_as_active(idi, IDO2DB_OBJECTTYPE_CONTACTGROUP, object_id);

	/* add definition to db */
	data[0] = (void *) &idi->dbinfo.instance_id;
	data[1] = (void *) &idi->current_object_config_type;
	data[2] = (void *) &object_id;
	data[3] = (void *) &es[0];

	result = ido2db_query_insert_or_update_contactgroupdefinition_definition_add(idi, data);

	if (result == IDO_OK) {

#ifdef USE_LIBDBI /* everything else will be libdbi */
		switch (idi->dbinfo.server_type) {
		case IDO2DB_DBSERVER_MYSQL:
			/* mysql doesn't use sequences */
			group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition(%lu) group_id\n", group_id);
			break;
		case IDO2DB_DBSERVER_PGSQL:
			/* depending on tableprefix/tablename a sequence will be used */
			if (asprintf(&buf, "%s_contactgroup_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS]) == -1)
				buf = NULL;

			group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition(%s=%lu) group_id\n", buf, group_id);
			free(buf);
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
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		dummy = asprintf(&seq_name, "seq_contactgroups");
		group_id = ido2db_oci_sequence_lastid(idi, seq_name);
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition(%lu) group_id\n", group_id);
		free(seq_name);

#endif /* Oracle ocilib specific */
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactroupdefinition() "
		                      "preeceding ido2db_query_insert_or_update_contactgroupdefinition_definition_add ERROR \n");
		return IDO_ERROR;
	}

#ifdef USE_LIBDBI /* everything else will be libdbi */
	dbi_result_free(idi->dbinfo.dbi_result);
	idi->dbinfo.dbi_result = NULL;
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */


#endif /* Oracle ocilib specific */

	free(es[0]);

	/* save contact group members to db */
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_()  start\n");

#ifdef USE_LIBDBI

	/* build a multiple insert value array */
	if (asprintf(&buf1, "INSERT INTO %s (instance_id, contactgroup_id, contact_object_id) VALUES ",
	             ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPMEMBERS]
	            ) == -1)
		buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* allocate array buffers based an maximal expected sizes*/
	instid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	groupid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));
	memberid_arr = (big_uint *) malloc(OCI_BINDARRAY_MAX_SIZE * sizeof(big_uint));

	if ((instid_arr == NULL) || (groupid_arr == NULL) || (memberid_arr == NULL)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_contactgroupdefinition() members"
		                      "  ArrayVars Alloc ERROR, exit\n");
		if (instid_arr) free(instid_arr);
		if (groupid_arr) free(groupid_arr);
		if (memberid_arr) free(memberid_arr);
		return IDO_ERROR;
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_contactgroupdefinition() members ArrayVars OK\n");
	/* bind arrays to statement */
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, MT(":X1"), instid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, MT(":X2"), groupid_arr, 0);
	OCI_BindArrayOfUnsignedBigInts(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, MT(":X3"), memberid_arr, 0);
	arrsize = 0;

#endif /* Oracle ocilib specific */

	first = 1;

	mbuf = idi->mbuf[IDO2DB_MBUF_CONTACTGROUPMEMBER];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ido2db_get_object_id_with_insert(idi,
		         IDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		buf = buf1; /* save this pointer for later free'ing */

#ifdef USE_LIBDBI
		if (asprintf(&buf1, "%s%s(%lu,%lu,%lu)",
		             buf1,
		             (first == 1 ? "" : ","),
		             idi->dbinfo.instance_id,
		             group_id,
		             member_id
		            ) == -1)
			buf1 = NULL;
#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition members "
		                      "(Pos %lu) Id=%lu group=%lu member=%lu\n",
		                      arrsize + count, idi->dbinfo.instance_id, group_id, member_id);
		/* copy ids to to array */
		instid_arr[arrsize] = idi->dbinfo.instance_id;
		groupid_arr[arrsize] = group_id;
		memberid_arr[arrsize] = member_id;
		arrsize++;
#endif /* Oracle ocilib specific */

		free(buf);
		first = 0;

#ifdef USE_ORACLE /* Oracle ocilib specific */
		if (arrsize == OCI_BINDARRAY_MAX_SIZE) {
			/* array limit being exceeded, need to push to database before continue*/
			OCI_BindArraySetSize(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, arrsize);
			if (OCI_Execute(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers)) {
				count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_contactgroupdefinition members "
				                      "(groupid %lu):so far %d items committed\n", group_id, count);
				arrsize = 0;
				OCI_Commit(idi->dbinfo.oci_connection);
			} else {
				/* execute error occured, need rollback and exit */
				OCI_Rollback(idi->dbinfo.oci_connection);
				ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_contactgroupdefinition members"
				                      "(groupid %lu) ERROR:Rollback %d items\n", group_id, arrsize);
				arrsize = 0;
			}

		}
#endif /* Oracle ocilib specific */

	}//for
#ifdef USE_ORACLE /* Oracle ocilib specific */

#endif /* Oracle ocilib specific */



#ifdef USE_LIBDBI /* everything else will be libdbi */
	if (first == 0) {
		result = ido2db_db_query(idi, buf1);
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
	}
#endif


#ifdef USE_PGSQL /* pgsql */

#endif
#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* store remaining entries*/

	if (arrsize > 0) {
		OCI_BindArraySetSize(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, arrsize);
		if (OCI_Execute(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers)) {
			count += OCI_GetAffectedRows(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_contactgroupdefinition members"
			                      "(groupid %lu): %d items finished\n", group_id, count);
			OCI_Commit(idi->dbinfo.oci_connection);
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_contactgroupdefinition members"
			                      "groupid %lu) ERROR:Rollback %d items \n", group_id, arrsize);

		}
	}
	if (count == 0) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_SQL, 2, "ido2db_handle_contactgroupdefinition members"
		                      "(groupid %lu) Warning:No members storable\n", group_id);
	}

#endif /* Oracle ocilib specific */
	if (buf1 != NULL) free(buf1);

#ifdef USE_ORACLE
	//cleanup array buffers
	free(groupid_arr);
	free(instid_arr);
	free(memberid_arr);
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition() end\n");

	return IDO_OK;
}
