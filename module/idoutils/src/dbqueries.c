/***************************************************************
 * DBQUERIES.C - Data Query handler routines for IDO2DB daemon
 *
 * Copyright (c) 2009-2015 Icinga Development Team (http://www.icinga.org)
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
#include "../include/dbqueries.h"
#include "../include/logging.h"

/* Icinga header files */
#include "../../../include/icinga.h"
#include "../../../include/broker.h"
#include "../../../include/comments.h"

extern int errno;

extern char *ido2db_db_tablenames[IDO2DB_MAX_DBTABLES];

/****************************************************************************/
/* INSERT QUERIES                                                           */
/****************************************************************************/

/****************************************************************************/
/* DELETE QUERIES                                                           */
/****************************************************************************/

/****************************************************************************/
/* INSERT/UPDATE/MERGE QUERIES                                              */
/****************************************************************************/


/************************************/
/* SYSTEMCOMMANDDATA                */
/************************************/

int ido2db_query_insert_or_update_systemcommanddata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long systemcommand_id;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	OCI_Lob *lob_oi;
	OCI_Lob *lob_ou;
	OCI_Lob *lob_loi;
	OCI_Lob *lob_lou;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		/* truncate long_output #2342 */
		if (*(char **) data[11] != NULL) {
			if (strlen(*(char **) data[11]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[11])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata_add() Warning:long_output truncated\n");
				if ((*(char **) data[11])[strlen(*(char **) data[11]) - 1] == '\\') {
					(*(char **) data[11])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}

                asprintf(&query1, "UPDATE %s SET end_time=%s, end_time_usec=%lu, command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s' WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SYSTEMCOMMANDS],
                                 *(char **) data[3],             /* update start */
                                 *(unsigned long *) data[4],
                                 *(char **) data[5],
                                 *(int *) data[6],
                                 *(int *) data[7],
                                 *(double *) data[8],
                                 *(int *) data[9],
                                 *(char **) data[10],
                                 *(char **) data[11],             /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[1],
                                 *(unsigned long *) data[2]      /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT systemcommand_id FROM %s WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SYSTEMCOMMANDS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[1],
                                 *(unsigned long *) data[2]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                systemcommand_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "systemcommand_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {

	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, start_time, start_time_usec, end_time, end_time_usec, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %s, %lu, %s, %lu, '%s', %d, %d, %lf, %d, '%s', '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_SYSTEMCOMMANDS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(char **) data[1],
                                         *(unsigned long*) data[2],
                                         *(char **) data[3],
                                         *(unsigned long *) data[4],
                                         *(char **) data[5],
                                         *(int *) data[6],
                                         *(int *) data[7],
                                         *(double *) data[8],
                                         *(int *) data[9],
                                         *(char **) data[10],
                                         *(char **) data[11]            /* insert end */
                                        );
                	        /* send query to db */
                        	result = ido2db_db_query(idi, query2);
	                        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET end_time=%s, end_time_usec=%lu, command_line=E'%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output=E'%s', long_output=E'%s' WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SYSTEMCOMMANDS],
		                 *(char **) data[3],             /* update start */
		                 *(unsigned long *) data[4],
		                 *(char **) data[5],
		                 *(int *) data[6],
		                 *(int *) data[7],
		                 *(double *) data[8],
		                 *(int *) data[9],
		                 *(char **) data[10],
		                 *(char **) data[11],             /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(char **) data[1],
		                 *(unsigned long *) data[2]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, start_time, start_time_usec, end_time, end_time_usec, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %s, %lu, %s, %lu, E'%s', %d, %d, %lf, %d, E'%s', E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SYSTEMCOMMANDS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(char **) data[1],
			                 *(unsigned long*) data[2],
			                 *(char **) data[3],
			                 *(unsigned long *) data[4],
			                 *(char **) data[5],
			                 *(int *) data[6],
			                 *(int *) data[7],
			                 *(double *) data[8],
			                 *(int *) data[9],
			                 *(char **) data[10],
			                 *(char **) data[11]            /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_systemcommanddata, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_systemcommanddata, MT(":X2"), (uint *) data[12])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_systemcommanddata, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_systemcommanddata, MT(":X4"), (uint *) data[13])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_systemcommanddata, MT(":X5"), (uint *) data[4])) {
		return IDO_ERROR;
	}
	if (*(char **) data[5] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_systemcommanddata, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit commandline size #3324 */
		if ( strlen(*(char **)data[5])  > OCI_COMMAND_LINE_SIZE ) {
			(*(char **)data[5])[OCI_COMMAND_LINE_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_systemcommand() output shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_systemcommanddata, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_systemcommanddata, MT(":X7"), (int *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_systemcommanddata, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_systemcommanddata, MT(":X9"), (double *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_systemcommanddata, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}

	//bind clob 2 times,once for update, once for insert to make oracle happy and avoid ora-600 because of double binding
	lob_oi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_ou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_loi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_lou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata() bind clob\n");
	result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_systemcommanddata, ":X11i", *(char **)data[10], &lob_oi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_systemcommanddata, ":X11u", *(char **)data[10], &lob_ou);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_systemcommanddata, ":X12i", *(char **)data[11], &lob_loi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_systemcommanddata, ":X12u", *(char **)data[11], &lob_lou);
	if (result == IDO_OK) {
		/* execute statement */
		result = OCI_Execute(idi->dbinfo.oci_statement_systemcommanddata) ? IDO_OK : IDO_ERROR;
		if (result == IDO_OK) {
			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata() executed\n");
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata() execute error\n");
		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata() bind clob error\n");
	}

	//free lobs
	if (lob_ou) OCI_LobFree(lob_ou);
	if (lob_oi) OCI_LobFree(lob_oi);
	if (lob_lou) OCI_LobFree(lob_lou);
	if (lob_loi) OCI_LobFree(lob_loi);


	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata_add() end\n");

	return result;
}


/************************************/
/* EVENTHANDLER                     */
/************************************/

int ido2db_query_insert_or_update_eventhandlerdata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long eventhandler_id;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	OCI_Lob *lob_oi;
	OCI_Lob *lob_ou;
	OCI_Lob *lob_loi;
	OCI_Lob *lob_lou;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		/* truncate long_output #2342 */
		if (*(char **) data[17] != NULL) {
			if (strlen(*(char **) data[17]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[17])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata_add() Warning:long_output truncated\n");
				if ((*(char **) data[17])[strlen(*(char **) data[17]) - 1] == '\\') {
					(*(char **) data[17])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}

                asprintf(&query1, "UPDATE %s SET eventhandler_type=%d, object_id=%lu, state=%d, state_type=%d, end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s', command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s' WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_EVENTHANDLERS],
                                 *(int *) data[1],               /* update start */
                                 *(unsigned long *) data[2],
                                 *(int *) data[3],
                                 *(int *) data[4],
                                 *(char **) data[7],
                                 *(unsigned long *) data[8],
                                 *(unsigned long *) data[9],
                                 *(char **) data[10],
                                 *(char **) data[11],
                                 *(int *) data[12],
                                 *(int *) data[13],
                                 *(double *) data[14],
                                 *(int *) data[15],
                                 *(char **) data[16],
                                 *(char **) data[17],            /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[5],
                                 *(unsigned long *) data[6]      /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT eventhandler_id FROM %s WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_EVENTHANDLERS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[5],
                                 *(unsigned long *) data[6]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                eventhandler_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "eventhandler_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, eventhandler_type, object_id, state, state_type, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %d, %lu, %d, %d, %s, %lu, %s, %lu, %lu, '%s', '%s', %d, %d, %lf, %d, '%s', '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_EVENTHANDLERS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(int *) data[3],
                                         *(int *) data[4],
                                         *(char **) data[5],
                                         *(unsigned long *) data[6],
                                         *(char **) data[7],
                                         *(unsigned long *) data[8],
                                         *(unsigned long *) data[9],
                                         *(char **) data[10],
                                         *(char **) data[11],
                                         *(int *) data[12],
                                         *(int *) data[13],
                                         *(double *) data[14],
                                         *(int *) data[15],
                                         *(char **) data[16],
                                         *(char **) data[17]            /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET eventhandler_type=%d, object_id=%lu, state=%d, state_type=%d, end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args=E'%s', command_line=E'%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output=E'%s', long_output=E'%s' WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_EVENTHANDLERS],
		                 *(int *) data[1],               /* update start */
		                 *(unsigned long *) data[2],
		                 *(int *) data[3],
		                 *(int *) data[4],
		                 *(char **) data[7],
		                 *(unsigned long *) data[8],
		                 *(unsigned long *) data[9],
		                 *(char **) data[10],
		                 *(char **) data[11],
		                 *(int *) data[12],
		                 *(int *) data[13],
		                 *(double *) data[14],
		                 *(int *) data[15],
		                 *(char **) data[16],
		                 *(char **) data[17],            /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(char **) data[5],
		                 *(unsigned long *) data[6]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, eventhandler_type, object_id, state, state_type, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %d, %lu, %d, %d, %s, %lu, %s, %lu, %lu, E'%s', E'%s', %d, %d, %lf, %d, E'%s', E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_EVENTHANDLERS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(int *) data[3],
			                 *(int *) data[4],
			                 *(char **) data[5],
			                 *(unsigned long *) data[6],
			                 *(char **) data[7],
			                 *(unsigned long *) data[8],
			                 *(unsigned long *) data[9],
			                 *(char **) data[10],
			                 *(char **) data[11],
			                 *(int *) data[12],
			                 *(int *) data[13],
			                 *(double *) data[14],
			                 *(int *) data[15],
			                 *(char **) data[16],
			                 *(char **) data[17]            /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X4"), (int *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X6"), (uint *) data[18])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X8"), (uint *) data[19])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X9"), (uint *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X10"), (uint *) data[9])) {
		return IDO_ERROR;
	}
	if (*(char **) data[10] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_eventhandlerdata, ":X11") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit command_args size #3324 */
		if ( strlen(*(char **)data[10])  > OCI_COMMAND_ARG_SIZE ) {
			(*(char **)data[10])[OCI_COMMAND_ARG_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_eventhandler() command_args shorted\n");
		}

		if (!OCI_BindString(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X11"), *(char **) data[10], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[11] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_eventhandlerdata, ":X12") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit commandline size #3324 */
		if ( strlen(*(char **)data[11])  > OCI_COMMAND_LINE_SIZE ) {
			(*(char **)data[11])[OCI_COMMAND_LINE_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_eventhandler() commandline shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X12"), *(char **) data[11], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X13"), (int *) data[12])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X14"), (int *) data[13])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X15"), (double *) data[14])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_eventhandlerdata, MT(":X16"), (int *) data[15])) {
		return IDO_ERROR;
	}

	//bind clobs
	lob_oi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_ou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_loi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_lou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata() bind clob\n");
	result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_eventhandlerdata, ":X17i",*(char **)data[16], &lob_oi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_eventhandlerdata, ":X17u",*(char **)data[16], &lob_ou);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_eventhandlerdata, ":X18i",*(char **)data[17], &lob_loi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_eventhandlerdata, ":X18u",*(char **)data[17], &lob_lou);
	if (result == IDO_OK) {
		/* execute statement */
		result = OCI_Execute(idi->dbinfo.oci_statement_eventhandlerdata) ? IDO_OK : IDO_ERROR;
		if (result == IDO_OK) {
			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata() executed\n");
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata() execute error\n");

		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata() bind clob error\n");
	}
	//free lobs
	if (lob_oi !=NULL) OCI_LobFree(lob_oi);
	if (lob_ou !=NULL) OCI_LobFree(lob_ou);
	if (lob_loi !=NULL) OCI_LobFree(lob_loi);
	if (lob_lou !=NULL) OCI_LobFree(lob_lou);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata_add() end\n");

	return result;
}


/************************************/
/* NOTIFICATIONS                    */
/************************************/

int ido2db_query_insert_or_update_notificationdata_add(ido2db_idi *idi, void **data, int type) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
//        unsigned long notification_id;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
	OCI_Lob *lob_oi;
	OCI_Lob *lob_ou;
	OCI_Lob *lob_loi;
	OCI_Lob *lob_lou;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		/* truncate long_output #2342 */
		if (*(char **) data[10] != NULL) {
			if (strlen(*(char **) data[10]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[10])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata_add() Warning:long_output truncated\n");
				if ((*(char **) data[10])[strlen(*(char **) data[10]) - 1] == '\\') {
					(*(char **) data[10])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}

                asprintf(&query1, "UPDATE %s SET notification_type=%d, notification_reason=%d, end_time=%s, end_time_usec=%lu, state=%d, output='%s', long_output='%s', escalated=%d, contacts_notified=%d  WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu AND object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS],
                                 *(int *) data[1],               /* update start */
                                 *(int *) data[2],
                                 *(char **) data[5],
                                 *(unsigned long *) data[6],
                                 *(int *) data[8],
                                 *(char **) data[9],
                                 *(char **) data[10],
                                 *(int *) data[11],
                                 *(int *) data[12],               /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[3],
                                 *(unsigned long *) data[4],
                                 *(unsigned long *) data[7]      /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT notification_id FROM %s WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu AND object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[3],
                                 *(unsigned long *) data[4],
                                 *(unsigned long *) data[7]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                notification_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "notification_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
	                        asprintf(&query2, "INSERT INTO %s (instance_id, notification_type, notification_reason, start_time, start_time_usec, end_time, end_time_usec, object_id, state, output, long_output, escalated, contacts_notified) VALUES (%lu, %d, %d, %s, %lu, %s, %lu, %lu, %d, '%s', '%s', %d, %d)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(int *) data[2],
                                         *(char **) data[3],
                                         *(unsigned long *) data[4],
                                         *(char **) data[5],
                                         *(unsigned long *) data[6],
                                         *(unsigned long *) data[7],
                                         *(int *) data[8],
                                         *(char **) data[9],
                                         *(char **) data[10],
                                         *(int *) data[11],
                                         *(int *) data[12]               /* insert end */
                                        );
        	                /* send query to db */
                	        result = ido2db_db_query(idi, query2);
                        	free(query2);

			        /* save the notification id for later use... */
			        if (type == NEBTYPE_NOTIFICATION_START)
			                idi->dbinfo.last_notification_id = 0L;
			        if (result == IDO_OK && type == NEBTYPE_NOTIFICATION_START) {
		                        /* mysql doesn't use sequences */
		                        idi->dbinfo.last_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata(%lu) last_notification_id\n", idi->dbinfo.last_notification_id);
				}

			        dbi_result_free(idi->dbinfo.dbi_result);
			        idi->dbinfo.dbi_result = NULL;
			}
                } else {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* if we actually did an update, we cannot just call dbi_conn_sequence_last, but rather select the id we updated */
                        asprintf(&query, "SELECT notification_id FROM %s WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu AND object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[3],
                                 *(unsigned long *) data[4],
                                 *(unsigned long *) data[7]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                idi->dbinfo.last_notification_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "notification_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata(%lu) last_notification_id\n", idi->dbinfo.last_notification_id);
                                        } 

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
				dbi_result_free(idi->dbinfo.dbi_result);
				idi->dbinfo.dbi_result = NULL;

			}
                        free(query);
		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET notification_type=%d, notification_reason=%d, end_time=%s, end_time_usec=%lu, state=%d, output=E'%s', long_output=E'%s', escalated=%d, contacts_notified=%d  WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu AND object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS],
		                 *(int *) data[1],               /* update start */
		                 *(int *) data[2],
		                 *(char **) data[5],
		                 *(unsigned long *) data[6],
		                 *(int *) data[8],
		                 *(char **) data[9],
		                 *(char **) data[10],
		                 *(int *) data[11],
		                 *(int *) data[12],               /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(char **) data[3],
		                 *(unsigned long *) data[4],
		                 *(unsigned long *) data[7]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, notification_type, notification_reason, start_time, start_time_usec, end_time, end_time_usec, object_id, state, output, long_output, escalated, contacts_notified) VALUES (%lu, %d, %d, %s, %lu, %s, %lu, %lu, %d, E'%s', E'%s', %d, %d)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(int *) data[2],
			                 *(char **) data[3],
			                 *(unsigned long *) data[4],
			                 *(char **) data[5],
			                 *(unsigned long *) data[6],
			                 *(unsigned long *) data[7],
			                 *(int *) data[8],
			                 *(char **) data[9],
			                 *(char **) data[10],
			                 *(int *) data[11],
			                 *(int *) data[12]               /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

		        /* save the notification id for later use... */
		        if (type == NEBTYPE_NOTIFICATION_START)
		                idi->dbinfo.last_notification_id = 0L;
		        if (result == IDO_OK && type == NEBTYPE_NOTIFICATION_START) {
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf, "%s_notification_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS]) == -1)
                	                buf = NULL;

	                        idi->dbinfo.last_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
	                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata(%s=%lu) last_notification_id\n", buf, idi->dbinfo.last_notification_id);
	                        free(buf);
			}

		        dbi_result_free(idi->dbinfo.dbi_result);
		        idi->dbinfo.dbi_result = NULL;

                } else {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

                        /* if we actually did an update, we cannot just call dbi_conn_sequence_last, but rather select the id we updated */
                        asprintf(&query, "SELECT notification_id FROM %s WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu AND object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_NOTIFICATIONS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[3],
                                 *(unsigned long *) data[4],
                                 *(unsigned long *) data[7]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                idi->dbinfo.last_notification_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "notification_id");
	                        		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata(%lu) last_notification_id\n", idi->dbinfo.last_notification_id);
                                        } 

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;

                        }
                        free(query);
                }
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_notificationdata, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_notificationdata, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_notificationdata, MT(":X3"), (int *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_notificationdata, MT(":X4"), (uint *) data[13])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_notificationdata, MT(":X5"), (uint *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_notificationdata, MT(":X6"), (uint *) data[14])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_notificationdata, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_notificationdata, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_notificationdata, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}

	if (!OCI_BindInt(idi->dbinfo.oci_statement_notificationdata, MT(":X12"), (int *) data[11])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_notificationdata, MT(":X13"), (int *) data[12])) {
		return IDO_ERROR;
	}

	/* bind clobs */
	lob_oi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_ou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_loi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_lou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata() bind clob");
	result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_notificationdata, ":X10i", *(char **)data[9], &lob_oi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_notificationdata, ":X11i", *(char **)data[10], &lob_loi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_notificationdata, ":X10u", *(char **)data[9], &lob_ou);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_notificationdata, ":X11u", *(char **)data[10], &lob_lou);
	if (result == IDO_OK) {
		/* execute statement */
		result = OCI_Execute(idi->dbinfo.oci_statement_notificationdata) ? IDO_OK : IDO_ERROR;
		if (result == IDO_OK) {
			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata() executed\n");
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata() execute error\n");

		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata() clob bind error\n");
	}

        /* save the notification id for later use... */
        if (type == NEBTYPE_NOTIFICATION_START)
                idi->dbinfo.last_notification_id = 0L;
        if (result == IDO_OK && type == NEBTYPE_NOTIFICATION_START) {

                if (asprintf(&seq_name, "seq_notifications") == -1)
                        seq_name = NULL;

		/* this hopefully works if we update the colum as well */
                idi->dbinfo.last_notification_id = ido2db_oci_sequence_lastid(idi, seq_name);
                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_notificationdata(%lu) last_notification_id\n", idi->dbinfo.last_notification_id);
                free(seq_name);
	}

	/* free lobs */
	if (lob_oi != NULL) OCI_LobFree(lob_oi);
	if (lob_ou != NULL) OCI_LobFree(lob_ou);
	if (lob_loi != NULL) OCI_LobFree(lob_loi);
	if (lob_lou != NULL) OCI_LobFree(lob_lou);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata_add() end\n");

	return result;
}


/************************************/
/* CONTACTNOTIFICATIONS             */
/************************************/

int ido2db_query_insert_or_update_contactnotificationdata_add(ido2db_idi *idi, void **data, int type) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
//        unsigned long contactnotification_id;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET notification_id=%lu, end_time=%s, end_time_usec=%lu WHERE instance_id=%lu AND contact_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
                                 *(unsigned long *) data[1],     /* update start */
                                 *(char **) data[4],
                                 *(unsigned long *) data[5],     /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[6],
                                 *(char **) data[2],
                                 *(unsigned long *) data[3]      /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT contactnotification_id FROM %s WHERE instance_id=%lu AND contact_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[6],
                                 *(char **) data[2],
                                 *(unsigned long *) data[3]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                contactnotification_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contactnotification_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, notification_id, start_time, start_time_usec, end_time, end_time_usec, contact_object_id) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(char **) data[2],
                                         *(unsigned long *) data[3],
                                         *(char **) data[4],
                                         *(unsigned long *) data[5],
                                         *(unsigned long *) data[6]      /* insert end */
                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
                	        free(query2);

			        /* save the contact notification id for later use... */
			        if (type == NEBTYPE_CONTACTNOTIFICATION_START)
			                idi->dbinfo.last_contact_notification_id = 0L;
			        if (result == IDO_OK && type == NEBTYPE_CONTACTNOTIFICATION_START) {
		                        /* mysql doesn't use sequences */
		                        idi->dbinfo.last_contact_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata(%lu) contactnotification_id\n", idi->dbinfo.last_contact_notification_id);
				}

                		dbi_result_free(idi->dbinfo.dbi_result);
	                	idi->dbinfo.dbi_result = NULL;
			}
                } else {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;

			/* hey, update happened, select the id we just updated */
                        asprintf(&query, "SELECT contactnotification_id FROM %s WHERE instance_id=%lu AND contact_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[6],
                                 *(char **) data[2],
                                 *(unsigned long *) data[3]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                idi->dbinfo.last_contact_notification_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contactnotification_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata(%lu) contactnotification_id\n", idi->dbinfo.last_contact_notification_id);
                                        } 

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
				}
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
			}
                        free(query);
		}

                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET notification_id=%lu, end_time=%s, end_time_usec=%lu WHERE instance_id=%lu AND contact_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
		                 *(unsigned long *) data[1],     /* update start */
		                 *(char **) data[4],
		                 *(unsigned long *) data[5],     /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(unsigned long *) data[6],
		                 *(char **) data[2],
		                 *(unsigned long *) data[3]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, notification_id, start_time, start_time_usec, end_time, end_time_usec, contact_object_id) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(char **) data[2],
			                 *(unsigned long *) data[3],
			                 *(char **) data[4],
			                 *(unsigned long *) data[5],
			                 *(unsigned long *) data[6]      /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

		        /* save the contact notification id for later use... */
		        if (type == NEBTYPE_CONTACTNOTIFICATION_START)
		                idi->dbinfo.last_contact_notification_id = 0L;
		        if (result == IDO_OK && type == NEBTYPE_CONTACTNOTIFICATION_START) {
	                        /* depending on tableprefix/tablename a sequence will be used */
	                        if (asprintf(&buf, "%s_contactnotification_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS]) == -1)
        	                        buf = NULL;

	                        idi->dbinfo.last_contact_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
	                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata(%s=%lu) contactnotification_id\n", buf, idi->dbinfo.last_contact_notification_id);
	                        free(buf);
			}

                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

                } else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

                        /* hey, update happened, select the id we just updated */
                        asprintf(&query, "SELECT contactnotification_id FROM %s WHERE instance_id=%lu AND contact_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[6],
                                 *(char **) data[2],
                                 *(unsigned long *) data[3]      /* unique constraint end */
                                );
        
                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                idi->dbinfo.last_contact_notification_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contactnotification_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata(%lu) contactnotification_id\n", idi->dbinfo.last_contact_notification_id);
                                        } 

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
			}
                        free(query);
                }

		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationdata, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationdata, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationdata, MT(":X3"), (uint *) data[7])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationdata, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationdata, MT(":X5"), (uint *) data[8])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationdata, MT(":X6"), (uint *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationdata, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_contactnotificationdata)) {
		syslog(LOG_USER | LOG_INFO, "contactnotificationdata execute\n");
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_contactnotificationdata() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

        /* save the contact notification id for later use... */
        if (type == NEBTYPE_CONTACTNOTIFICATION_START)
                idi->dbinfo.last_contact_notification_id = 0L;
        if (result == IDO_OK && type == NEBTYPE_CONTACTNOTIFICATION_START) {
                if (asprintf(&seq_name, "seq_contactnotifications") == -1)
                        seq_name = NULL;
                idi->dbinfo.last_contact_notification_id = ido2db_oci_sequence_lastid(idi, seq_name);
                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactnotificationdata(%lu) \n", idi->dbinfo.last_contact_notification_id);
                free(seq_name);
	}

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationdata_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_contactnotificationmethoddata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long contactnotificationmethod_id;
        int mysql_update = FALSE;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationmethoddata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s' WHERE instance_id=%lu AND contactnotification_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS],
                                 *(char **) data[4],             /* update start */
                                 *(unsigned long *) data[5],
                                 *(unsigned long *) data[6],
                                 (*(char **) data[7] == NULL) ? "" : *(char **) data[7],       /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(char **) data[2],
                                 *(unsigned long *) data[3]      /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT contactnotificationmethod_id FROM %s WHERE instance_id=%lu AND contactnotification_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(char **) data[2],
                                 *(unsigned long *) data[3]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                contactnotificationmethod_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contactnotificationmethod_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, contactnotification_id, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(char **) data[2],
                                         *(unsigned long *) data[3],
                                         *(char **) data[4],
                                         *(unsigned long *) data[5],
                                         *(unsigned long *) data[6],
                                         (*(char **) data[7] == NULL) ? "" : *(char **) data[7]       /* insert end */
                                        );
                        	/* send query to db */
                	        result = ido2db_db_query(idi, query2);
	                        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args=E'%s' WHERE instance_id=%lu AND contactnotification_id=%lu AND start_time=%s AND start_time_usec=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS],
		                 *(char **) data[4],             /* update start */
		                 *(unsigned long *) data[5],
		                 *(unsigned long *) data[6],
		                 (*(char **) data[7] == NULL) ? "" : *(char **) data[7],       /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(unsigned long *) data[1],
		                 *(char **) data[2],
		                 *(unsigned long *) data[3]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, contactnotification_id, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(char **) data[2],
			                 *(unsigned long *) data[3],
			                 *(char **) data[4],
			                 *(unsigned long *) data[5],
			                 *(unsigned long *) data[6],
			                 (*(char **) data[7] == NULL) ? "" : *(char **) data[7]       /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationmethoddata, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationmethoddata, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationmethoddata, MT(":X3"), (uint *) data[8])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationmethoddata, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationmethoddata, MT(":X5"), (uint *) data[9])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationmethoddata, MT(":X6"), (uint *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactnotificationmethoddata, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (*(char **) data[7] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_contactnotificationmethoddata, ":X8") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit command_arg size #3324 */
		if ( strlen(*(char **)data[7])  > OCI_COMMAND_ARG_SIZE ) {
			(*(char **)data[7])[OCI_COMMAND_ARG_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_contactnotificationmethods() command_args shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_contactnotificationmethoddata, MT(":X8"), *(char **) data[7], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_contactnotificationmethoddata)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_contactnotificationmethoddata() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationmethoddata_add() end\n");

	return result;
}


/************************************/
/* SERVICECHECKS                    */
/************************************/

int ido2db_query_insert_servicecheckdata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char * query1 = NULL;
#endif
#ifdef USE_ORACLE
	OCI_Lob *lob_o;
	OCI_Lob *lob_l;
	OCI_Lob *lob_p;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicecheckdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		/* truncate long_output #2342 */
		if (*(char **) data[17] != NULL) {
			if (strlen(*(char **) data[17]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[17])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicecheckdata_add() Warning:long_output truncated\n");
				if ((*(char **) data[17])[strlen(*(char **) data[17]) - 1] == '\\') {
					(*(char **) data[17])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicecheckdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}
		if (*(char **) data[18] != NULL) {
			if (strlen(*(char **) data[18]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[18])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicecheckdata_add() Warning:perfdata truncated\n");
				if ((*(char **) data[18])[strlen(*(char **) data[18]) - 1] == '\\') {
					(*(char **) data[18])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicecheckdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}
                asprintf(&query1,
                		"INSERT INTO %s (instance_id, service_object_id, check_type, "
                		"current_check_attempt, max_check_attempts, state, state_type, "
                		"start_time, start_time_usec, end_time, end_time_usec, timeout, "
                		"early_timeout, execution_time, latency, return_code, output, "
                		"long_output, perfdata, command_object_id, command_args, command_line) "
                		"VALUES (%lu, %lu, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, '%s', '%s', '%s', %lu, '%s', '%s')",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECHECKS],
                                 *(unsigned long *) data[0],     /* insert start */
                                 *(unsigned long *) data[1],
                                 *(int *) data[2],
                                 *(int *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5],
                                 *(int *) data[6],
                                 *(char **) data[7],
                                 *(unsigned long *) data[8],
                                 *(char **) data[9],
                                 *(unsigned long *) data[10],
                                 *(int *) data[11],
                                 *(int *) data[12],
                                 *(double *) data[13],
                                 *(double *) data[14],
                                 *(int *) data[15],
                                 *(char **) data[16],
                                 *(char **) data[17],
                                 *(char **) data[18],
                                 *(unsigned long *) data[19],
                                 *(char **) data[20],
                                 *(char **) data[21]            /* insert end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;
	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "INSERT INTO %s (instance_id, service_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata, command_object_id, command_args, command_line) VALUES (%lu, %lu, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, E'%s', E'%s', E'%s', %lu, E'%s', E'%s')",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECHECKS],
		                 *(unsigned long *) data[0],     /* insert start */
		                 *(unsigned long *) data[1],
		                 *(int *) data[2],
		                 *(int *) data[3],
		                 *(int *) data[4],
		                 *(int *) data[5],
		                 *(int *) data[6],
		                 *(char **) data[7],
		                 *(unsigned long *) data[8],
		                 *(char **) data[9],
		                 *(unsigned long *) data[10],
		                 *(int *) data[11],
		                 *(int *) data[12],
		                 *(double *) data[13],
		                 *(double *) data[14],
		                 *(int *) data[15],
		                 *(char **) data[16],
		                 *(char **) data[17],
		                 *(char **) data[18],
		                 *(unsigned long *) data[19],
		                 *(char **) data[20],
		                 *(char **) data[21]     	/* insert end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* bind params to prepared statement */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicechecks, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicechecks, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicechecks, MT(":X3"), (int *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicechecks, MT(":X4"), (int *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicechecks, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicechecks, MT(":X6"), (int *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicechecks, MT(":X7"), (int *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicechecks, MT(":X8"), (uint *) data[22])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicechecks, MT(":X9"), (uint *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicechecks, MT(":X10"), (uint *) data[23])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicechecks, MT(":X11"), (uint *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicechecks, MT(":X12"), (int *) data[11])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicechecks, MT(":X13"), (int *) data[12])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicechecks, MT(":X14"), (double *) data[13])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicechecks, MT(":X15"), (double *) data[14])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicechecks, MT(":X16"), (int *) data[15])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicechecks, MT(":X20"), (uint *) data[19])) {
			return IDO_ERROR;
	}

	if (*(char **) data[20] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicechecks, ":X21") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit command_arg size #3324 */
		if ( strlen(*(char **)data[20])  > OCI_COMMAND_ARG_SIZE ) {
			(*(char **)data[20])[OCI_COMMAND_ARG_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicechecks() command_args shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicechecks, MT(":X21"), *(char **) data[20], 0)) {
			return IDO_ERROR;
		}
	}

	if (*(char **) data[21] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicechecks, ":X22") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit command_line size #3324 */
		if ( strlen(*(char **)data[21])  > OCI_COMMAND_LINE_SIZE ) {
			(*(char **)data[21])[OCI_COMMAND_LINE_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicechecks() command_line shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicechecks, MT(":X22"), *(char **) data[21], 0)) {
			return IDO_ERROR;
		}
	}
	//bind clob
	lob_o = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_l = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_p = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicechecks() bind clob");
	result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_servicechecks, ":X17", *(char **)data[16], &lob_o);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_servicechecks, ":X18", *(char **)data[17], &lob_l);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_servicechecks, ":X19", *(char **)data[18], &lob_p);
	if (result == IDO_OK) {
		/* execute statement */
		result = OCI_Execute(idi->dbinfo.oci_statement_servicechecks) ? IDO_OK : IDO_ERROR;
		if (result == IDO_OK) {
			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicechecks() executed\n");
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicechecks() execute error\n");
		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicechecks() clob bind error\n");
	}

	//free lobs
	if (lob_o != NULL) OCI_LobFree(lob_o);
	if (lob_l != NULL) OCI_LobFree(lob_l);
	if (lob_p != NULL) OCI_LobFree(lob_p);
	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicecheckdata_add() end\n");

	return result;
}


/************************************/
/* HOSTCHECKS                       */
/************************************/

int ido2db_query_insert_hostcheckdata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char * query1 = NULL;
#endif
#ifdef USE_ORACLE
	OCI_Lob *lob_o;
	OCI_Lob *lob_l;
	OCI_Lob *lob_p;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostcheckdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		/* truncate long_output #2342 */
		if (*(char **) data[21] != NULL) {
			if (strlen(*(char **) data[21]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[21])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostcheckdata_add() Warning:long_output truncated\n");
				if ((*(char **) data[21])[strlen(*(char **) data[21]) - 1] == '\\') {
					(*(char **) data[21])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostcheckdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}
		if (*(char **) data[22] != NULL) {
			if (strlen(*(char **) data[22]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[22])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostcheckdata_add() Warning:perfdata truncated\n");
				if ((*(char **) data[22])[strlen(*(char **) data[22]) - 1] == '\\') {
					(*(char **) data[22])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostcheckdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}
                asprintf(&query1, "INSERT INTO %s (command_object_id, command_args, command_line, "
                				"instance_id, host_object_id, check_type, is_raw_check, current_check_attempt, "
                				"max_check_attempts, state, state_type, start_time, start_time_usec, end_time, "
                				"end_time_usec, timeout, early_timeout, execution_time, latency, return_code, "
                				"output, long_output, perfdata) "
                				"VALUES (%lu, '%s', '%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, '%s', '%s', '%s')",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCHECKS],
                                 *(unsigned long *) data[0],     /* insert start */
                                 *(char **) data[1],
                                 *(char **) data[2],
                                 *(unsigned long *) data[3],
                                 *(unsigned long *) data[4],
                                 *(int *) data[5],
                                 *(int *) data[6],
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9],
                                 *(int *) data[10],
                                 *(char **) data[11],
                                 *(unsigned long *) data[12],
                                 *(char **) data[13],
                                 *(unsigned long *) data[14],
                                 *(int *) data[15],
                                 *(int *) data[16],
                                 *(double *) data[17],
                                 *(double *) data[18],
                                 *(int *) data[19],
                                 *(char **) data[20],
                                 *(char **) data[21],
                                 *(char **) data[22]            /* insert end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;
	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "INSERT INTO %s (command_object_id, command_args, command_line, instance_id, host_object_id, check_type, is_raw_check, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata) VALUES (%lu, E'%s', E'%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, E'%s', E'%s', E'%s')",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCHECKS],
		                 *(unsigned long *) data[0],     /* insert start */
		                 *(char **) data[1],
		                 *(char **) data[2],
		                 *(unsigned long *) data[3],
		                 *(unsigned long *) data[4],
		                 *(int *) data[5],
		                 *(int *) data[6],
		                 *(int *) data[7],
		                 *(int *) data[8],
		                 *(int *) data[9],
		                 *(int *) data[10],
		                 *(char **) data[11],
		                 *(unsigned long *) data[12],
		                 *(char **) data[13],
		                 *(unsigned long *) data[14],
		                 *(int *) data[15],
		                 *(int *) data[16],
		                 *(double *) data[17],
		                 *(double *) data[18],
		                 *(int *) data[19],
		                 *(char **) data[20],
		                 *(char **) data[21],
		                 *(char **) data[22]            /* insert end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* bind params to prepared statement */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostchecks, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (*(char **) data[1] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostchecks, ":X2") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit command_arg size #3324 */
		if ( strlen(*(char **)data[1])  > OCI_COMMAND_ARG_SIZE ) {
				(*(char **)data[1])[OCI_COMMAND_ARG_SIZE] = '\0';
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostchecks() command_args shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostchecks, MT(":X2"), *(char **) data[1], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[2] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostchecks, ":X3") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit commandline size #3324 */
		if ( strlen(*(char **)data[2])  > OCI_COMMAND_LINE_SIZE ) {
					(*(char **)data[2])[OCI_COMMAND_LINE_SIZE] = '\0';
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostchecks() commandline shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostchecks, MT(":X3"), *(char **) data[2], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostchecks, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostchecks, MT(":X5"), (uint *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostchecks, MT(":X6"), (int *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostchecks, MT(":X7"), (int *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostchecks, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostchecks, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostchecks, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostchecks, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostchecks, MT(":X12"), (uint *) data[23])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostchecks, MT(":X13"), (uint *) data[12])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostchecks, MT(":X14"), (uint *) data[24])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostchecks, MT(":X15"), (uint *) data[14])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostchecks, MT(":X16"), (int *) data[15])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostchecks, MT(":X17"), (int *) data[16])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostchecks, MT(":X18"), (double *) data[17])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostchecks, MT(":X19"), (double *) data[18])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostchecks, MT(":X20"), (int *) data[19])) {
		return IDO_ERROR;
	}


	//bind clob
	lob_o = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_l = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_p = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostchecks() bind clobs");
	result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_hostchecks, ":X21", *(char **)data[20], &lob_o);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_hostchecks, ":X22", *(char **)data[21], &lob_l);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_hostchecks, ":X23", *(char **)data[22], &lob_p);
	if (result == IDO_OK) {
		/* execute statement */
		result = OCI_Execute(idi->dbinfo.oci_statement_hostchecks) ? IDO_OK : IDO_ERROR;
		if (result == IDO_OK) {
			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostchecks() executed\n");
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostchecks() execute error\n");

		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostchecks() bind clob error\n");
	}

	//free lobs
	if (lob_o !=NULL) OCI_LobFree(lob_o);
	if (lob_l !=NULL) OCI_LobFree(lob_l);
	if (lob_p !=NULL) OCI_LobFree(lob_p);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostcheckdata_add() end\n");

	return result;
}


/************************************/
/* COMMENTS                         */
/************************************/

int ido2db_query_insert_or_update_commentdata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char * query = NULL;
	char * query1 = NULL;
	char * query2 = NULL;
	int mysql_update = FALSE;
//	unsigned long comment_id = 0L;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commentdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET comment_type=%d, entry_type=%d, object_id=%lu, author_name='%s', comment_data='%s', is_persistent=%d, comment_source=%d, expires=%d, expiration_time=%s WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
				ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS],
				*(int *) data[3],               /* update start */
				*(int *) data[4],
				*(unsigned long *) data[5],
				*(char **) data[8],
				*(char **) data[9],
				*(int *) data[10],
				*(int *) data[11],
				*(int *) data[12],
				*(char **) data[13],            /* end end */
				*(unsigned long *) data[2],     /* unique constraint start */
				*(char **) data[6],
				*(unsigned long *) data[7]      /* unique constraint end */
				);
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commentdata_add() dbi_result_get_numrows_affected=%lu\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

		/* we can't use INSERT ... ON DUPLICATE KEY UPDATE ... anymore, as MySQL flagged that unsafe
		 * for more than one unique constraint (haha, primary key, suckers!) on replication starting
		 * with 5.5.24 - see #3008 for more info. That's also true for REPLACE INTO ... SELECT ...
		 * Since comments can be loaded and added with the same data by the core, we cannot just try
		 * update and then insert, we need to explicitely select and then decide what to do.
		 */

		/* it seems we did not affect anything, but the libdbi mysql driver is entirely broken
		 * and might return 0 in any case, so we need to fire another select statement just to
		 * be sure about it.
		 */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

	                asprintf(&query, "SELECT comment_id FROM %s WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
					ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS],
					*(unsigned long *) data[2],     /* unique constraint start */
					*(char **) data[6],
					*(unsigned long *) data[7]      /* unique constraint end */
					);

	                /* send query to db */
			if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
				if (idi->dbinfo.dbi_result != NULL) {
					if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//						comment_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "comment_id");
						mysql_update = TRUE;
					} else {
						mysql_update = FALSE;
					}

					dbi_result_free(idi->dbinfo.dbi_result);
					idi->dbinfo.dbi_result = NULL;
				}
			}
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                	free(query);

			if (mysql_update == FALSE) {
	                        /* no data found, insert it new */
        	                asprintf(&query2, "INSERT INTO %s (entry_time, entry_time_usec, instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time) VALUES (%s, %lu, %lu, %d, %d, %lu, %s, %lu, '%s', '%s', %d, %d, %d, %s)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS],
                                         *(char **) data[0],             /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2],
                                         *(int *) data[3],
                                         *(int *) data[4],
                                         *(unsigned long *) data[5],
                                         *(char **) data[6],
                                         *(unsigned long *) data[7],
                                         *(char **) data[8],
                                         *(char **) data[9],
                                         *(int *) data[10],
                                         *(int *) data[11],
                                         *(int *) data[12],
                                         *(char **) data[13]            /* insert end */
                                        );
                	        /* send query to db */
                        	result = ido2db_db_query(idi, query2);
	                        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET comment_type=%d, entry_type=%d, object_id=%lu, author_name=E'%s', comment_data=E'%s', is_persistent=%d, comment_source=%d, expires=%d, expiration_time=%s WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS],
		                 *(int *) data[3],               /* update start */
		                 *(int *) data[4],
		                 *(unsigned long *) data[5],
		                 *(char **) data[8],
		                 *(char **) data[9],
		                 *(int *) data[10],
		                 *(int *) data[11],
		                 *(int *) data[12],
		                 *(char **) data[13],            /* end end */
		                 *(unsigned long *) data[2],     /* unique constraint start */
		                 *(char **) data[6],
		                 *(unsigned long *) data[7]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (entry_time, entry_time_usec, instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time) VALUES (%s, %lu, %lu, %d, %d, %lu, %s, %lu, E'%s', E'%s', %d, %d, %d, %s)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTS],
			                 *(char **) data[0],             /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2],
			                 *(int *) data[3],
			                 *(int *) data[4],
			                 *(unsigned long *) data[5],
			                 *(char **) data[6],
			                 *(unsigned long *) data[7],
			                 *(char **) data[8],
			                 *(char **) data[9],
			                 *(int *) data[10],
			                 *(int *) data[11],
			                 *(int *) data[12],
			                 *(char **) data[13]            /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata, MT(":X1"), (uint *) data[14])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata, MT(":X4"), (int *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata, MT(":X6"), (uint *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata, MT(":X7"), (uint *) data[15])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}
	if (*(char **) data[8] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_commentdata, ":X9") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_commentdata, MT(":X9"), *(char **) data[8], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[9] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_commentdata, ":X10") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_commentdata, MT(":X10"), *(char **) data[9], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata, MT(":X12"), (int *) data[11])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata, MT(":X13"), (int *) data[12])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata, MT(":X14"), (uint *) data[16])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_commentdata)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_commentdata() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commentdata_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_commentdata_history_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char * query = NULL;
	char * query1 = NULL;
	char * query2 = NULL;
	int mysql_update = FALSE;
//	unsigned long comment_id = 0L;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commentdata_history_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		asprintf(&query1, "UPDATE %s SET comment_type=%d, entry_type=%d, object_id=%lu, author_name='%s', comment_data='%s', is_persistent=%d, comment_source=%d, expires=%d, expiration_time=%s WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
				ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTHISTORY],
				*(int *) data[3],               /* update start */
				*(int *) data[4],
				*(unsigned long *) data[5],
				*(char **) data[8],
				*(char **) data[9],
				*(int *) data[10],
				*(int *) data[11],
				*(int *) data[12],
				*(char **) data[13],            /* end end */
				*(unsigned long *) data[2],     /* unique constraint start */
				*(char **) data[6],
				*(unsigned long *) data[7]      /* unique constraint end */
				);
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commentdata_history_add() dbi_result_get_numrows_affected=%lu\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

                /* it seems we did not affect anything, but the libdbi mysql driver is entirely broken
                 * and might return 0 in any case, so we need to fire another select statement just to
                 * be sure about it.
                 */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

	                asprintf(&query, "SELECT commenthistory_id FROM %s WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
				ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTHISTORY],
				*(unsigned long *) data[2],     /* unique constraint start */
				*(char **) data[6],
				*(unsigned long *) data[7]      /* unique constraint end */
				);

        	        /* send query to db */
			if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
				if (idi->dbinfo.dbi_result != NULL) {
					if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//						comment_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "comment_id");
						mysql_update = TRUE;
					} else {
						mysql_update = FALSE;
					}

					dbi_result_free(idi->dbinfo.dbi_result);
					idi->dbinfo.dbi_result = NULL;
				}
			}
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
        	        free(query);

			if (mysql_update == FALSE) {

        	                /* no data provided, insert one */
	                        asprintf(&query2, "INSERT INTO %s (entry_time, entry_time_usec, instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time) VALUES (%s, %lu, %lu, %d, %d, %lu, %s, %lu, '%s', '%s', %d, %d, %d, %s)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTHISTORY],
                                         *(char **) data[0],             /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2],
                                         *(int *) data[3],
                                         *(int *) data[4],
                                         *(unsigned long *) data[5],
                                         *(char **) data[6],
                                         *(unsigned long *) data[7],
                                         *(char **) data[8],
                                         *(char **) data[9],
                                         *(int *) data[10],
                                         *(int *) data[11],
                                         *(int *) data[12],
                                         *(char **) data[13]            /* insert end */
                                        );
                	        /* send query to db */
                        	result = ido2db_db_query(idi, query2);
	                        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET comment_type=%d, entry_type=%d, object_id=%lu, author_name=E'%s', comment_data=E'%s', is_persistent=%d, comment_source=%d, expires=%d, expiration_time=%s WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTHISTORY],
		                 *(int *) data[3],               /* update start */
		                 *(int *) data[4],
		                 *(unsigned long *) data[5],
		                 *(char **) data[8],
		                 *(char **) data[9],
		                 *(int *) data[10],
		                 *(int *) data[11],
		                 *(int *) data[12],
		                 *(char **) data[13],            /* end end */
		                 *(unsigned long *) data[2],     /* unique constraint start */
		                 *(char **) data[6],
		                 *(unsigned long *) data[7]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (entry_time, entry_time_usec, instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time) VALUES (%s, %lu, %lu, %d, %d, %lu, %s, %lu, E'%s', E'%s', %d, %d, %d, %s)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_COMMENTHISTORY],
			                 *(char **) data[0],             /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2],
			                 *(int *) data[3],
			                 *(int *) data[4],
			                 *(unsigned long *) data[5],
			                 *(char **) data[6],
			                 *(unsigned long *) data[7],
			                 *(char **) data[8],
			                 *(char **) data[9],
			                 *(int *) data[10],
			                 *(int *) data[11],
			                 *(int *) data[12],
			                 *(char **) data[13]            /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X1"), (uint *) data[14])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X4"), (int *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X6"), (uint *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X7"), (uint *) data[15])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}
	if (*(char **) data[8] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_commentdata_history, ":X9") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_commentdata_history, MT(":X9"), *(char **) data[8], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[9] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_commentdata_history, ":X10") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_commentdata_history, MT(":X10"), *(char **) data[9], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X12"), (int *) data[11])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X13"), (int *) data[12])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commentdata_history, MT(":X14"), (uint *) data[16])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_commentdata_history)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_commentdata_history() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commentdata_history_add() end\n");

	return result;
}


/************************************/
/* DOWNTIME                         */
/************************************/

int ido2db_query_insert_or_update_downtimedata_scheduled_downtime_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long scheduleddowntime_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_scheduled_downtime_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s "
                                                "SET downtime_type=%d, author_name='%s', comment_data='%s', triggered_by_id=%lu, is_fixed=%d, duration=%lu, scheduled_start_time=%s, scheduled_end_time=%s, is_in_effect=%d, trigger_time=%s "
                                                "WHERE instance_id=%lu AND object_id=%lu AND entry_time=%s AND internal_downtime_id=%lu"
                                 ,ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]
                                 ,*(int *) data[1]               /* update start */
                                 ,*(char **) data[4]
                                 ,*(char **) data[5]
                                 ,*(unsigned long *) data[7]
                                 ,*(int *) data[8]
                                 ,*(unsigned long *) data[9]
                                 ,*(char **) data[10]
                                 ,*(char **) data[11]
                                 ,*(int *) data[15]
                                 ,*(char **) data[16]            /* update end */
                                 ,*(unsigned long *) data[0]     /* unique constraint start */
                                 ,*(unsigned long *) data[2]
                                 ,*(char **) data[3]
                                 ,*(unsigned long *) data[6]      /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_scheduled_downtime_add() dbi_result_get_numrows_affected=%lu\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT scheduleddowntime_id FROM %s WHERE instance_id=%lu AND object_id=%lu AND entry_time=%s AND internal_downtime_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]
                                 ,*(unsigned long *) data[0]     /* unique constraint start */
                                 ,*(unsigned long *) data[2]
                                 ,*(char **) data[3]
                                 ,*(unsigned long *) data[6]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                scheduleddowntime_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "scheduleddowntime_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);

                        if (mysql_update == FALSE) {

                        	/* try insert instead */
	                        asprintf(&query2, "INSERT INTO %s "
                                                        "(instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time, is_in_effect, trigger_time) "
                                                        "VALUES (%lu, %d, %lu, %s, '%s', '%s', %lu, %lu, %d, %lu, %s, %s, %d, %s)"
                                         ,ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]
                                         ,*(unsigned long *) data[0]     /* insert start */
                                         ,*(int *) data[1]
                                         ,*(unsigned long *) data[2]
                                         ,*(char **) data[3]
                                         ,*(char **) data[4]
                                         ,*(char **) data[5]
                                         ,*(unsigned long *) data[6]
                                         ,*(unsigned long *) data[7]
                                         ,*(int *) data[8]
                                         ,*(unsigned long *) data[9]
                                         ,*(char **) data[10]
                                         ,*(char **) data[11]
                                         ,*(int *) data[15]
                                         ,*(char **) data[16]            /* insert end */
                                        );
        	                /* send query to db */
                	        result = ido2db_db_query(idi, query2);
                        	free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s "
						"SET downtime_type=%d, author_name=E'%s', comment_data=E'%s', triggered_by_id=%lu, is_fixed=%d, duration=%lu, scheduled_start_time=%s, scheduled_end_time=%s, is_in_effect=%d, trigger_time=%s "
						"WHERE instance_id=%lu AND object_id=%lu AND entry_time=%s AND internal_downtime_id=%lu"
		                 ,ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]
		                 ,*(int *) data[1]               /* update start */
		                 ,*(char **) data[4]
		                 ,*(char **) data[5]
		                 ,*(unsigned long *) data[7]
		                 ,*(int *) data[8]
		                 ,*(unsigned long *) data[9]
		                 ,*(char **) data[10]
		                 ,*(char **) data[11]
				 ,*(int *) data[15]
				 ,*(char **) data[16]		 /* update end */
		                 ,*(unsigned long *) data[0]     /* unique constraint start */
		                 ,*(unsigned long *) data[2]
		                 ,*(char **) data[3]
		                 ,*(unsigned long *) data[6]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s "
							"(instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time, is_in_effect, trigger_time) "
							"VALUES (%lu, %d, %lu, %s, E'%s', E'%s', %lu, %lu, %d, %lu, %s, %s, %d, %s)"
			                 ,ido2db_db_tablenames[IDO2DB_DBTABLE_SCHEDULEDDOWNTIME]
			                 ,*(unsigned long *) data[0]     /* insert start */
			                 ,*(int *) data[1]
			                 ,*(unsigned long *) data[2]
			                 ,*(char **) data[3]
			                 ,*(char **) data[4]
			                 ,*(char **) data[5]
			                 ,*(unsigned long *) data[6]
			                 ,*(unsigned long *) data[7]
			                 ,*(int *) data[8]
			                 ,*(unsigned long *) data[9]
			                 ,*(char **) data[10]
					 ,*(char **) data[11]
				 	 ,*(int *) data[15]
					 ,*(char **) data[16]		 /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X4"), (uint *) data[12])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[5] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X10"), (uint *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X11"), (uint *) data[13])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X12"), (uint *) data[14])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X13"), (int *) data[15])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime, MT(":X14"), (uint *) data[17])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_downtimedata_scheduled_downtime)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_downtimedata_scheduled_downtime() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_scheduled_downtime_add() end\n");

	return result;
}

int ido2db_query_insert_or_update_downtimedata_downtime_history_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long downtimehistory_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_downtime_history_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s "
                                                "SET downtime_type=%d, author_name='%s', comment_data='%s', triggered_by_id=%lu, is_fixed=%d, duration=%lu, scheduled_start_time=%s, scheduled_end_time=%s, is_in_effect=%d, trigger_time=%s "
                                                "WHERE instance_id=%lu AND object_id=%lu AND entry_time=%s AND internal_downtime_id=%lu"
                                 ,ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]
                                 ,*(int *) data[1]               /* update start */
                                 ,*(char **) data[4]
                                 ,*(char **) data[5]
                                 ,*(unsigned long *) data[7]
                                 ,*(int *) data[8]
                                 ,*(unsigned long *) data[9]
                                 ,*(char **) data[10]
                                 ,*(char **) data[11]
                                 ,*(int *) data[15] /* is_in_effect */
                                 ,*(char **) data[16] /* trigger_time as sql string */
                                                                /* update end */
                                 ,*(unsigned long *) data[0]     /* unique constraint start */
                                 ,*(unsigned long *) data[2]
                                 ,*(char **) data[3]
                                 ,*(unsigned long *) data[6]      /* unique constraint end */
                                );

                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_downtime_history_add() dbi_result_get_numrows_affected=%lu\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT downtimehistory_id FROM %s WHERE instance_id=%lu AND object_id=%lu AND entry_time=%s AND internal_downtime_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]
                                 ,*(unsigned long *) data[0]     /* unique constraint start */
                                 ,*(unsigned long *) data[2]
                                 ,*(char **) data[3]
                                 ,*(unsigned long *) data[6]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                downtimehistory_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "downtimehistory_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);

                        if (mysql_update == FALSE) {
                        	/* try insert instead */
	                        asprintf(&query2, "INSERT INTO %s "
                                                        "(instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time, is_in_effect, trigger_time) "
                                                        "VALUES (%lu, %d, %lu, %s, '%s', '%s', %lu, %lu, %d, %lu, %s, %s, %d, %s)"
                                         ,ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]
                                         ,*(unsigned long *) data[0]     /* insert start */
                                         ,*(int *) data[1]
                                         ,*(unsigned long *) data[2]
                                         ,*(char **) data[3]
                                         ,*(char **) data[4]
                                         ,*(char **) data[5]
                                         ,*(unsigned long *) data[6]
                                         ,*(unsigned long *) data[7]
                                         ,*(int *) data[8]
                                         ,*(unsigned long *) data[9]
                                         ,*(char **) data[10]
                                         ,*(char **) data[11]
                                         ,*(int *) data[15] /* is_in_effect */
                                         ,*(char **) data[16] /* trigger_time as sql string */

                                         /* insert end */
                                        );
        	                /* send query to db */
                	        result = ido2db_db_query(idi, query2);
                        	free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s "
						"SET downtime_type=%d, author_name=E'%s', comment_data=E'%s', triggered_by_id=%lu, is_fixed=%d, duration=%lu, scheduled_start_time=%s, scheduled_end_time=%s, is_in_effect=%d, trigger_time=%s "
						"WHERE instance_id=%lu AND object_id=%lu AND entry_time=%s AND internal_downtime_id=%lu"
		                 ,ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]
		                 ,*(int *) data[1]               /* update start */
		                 ,*(char **) data[4]
		                 ,*(char **) data[5]
		                 ,*(unsigned long *) data[7]
		                 ,*(int *) data[8]
		                 ,*(unsigned long *) data[9]
		                 ,*(char **) data[10]
		                 ,*(char **) data[11]
				 ,*(int *) data[15] /* is_in_effect */
				 ,*(char **) data[16] /* trigger_time as sql string */
				 				/* update end */
		                 ,*(unsigned long *) data[0]     /* unique constraint start */
		                 ,*(unsigned long *) data[2]
		                 ,*(char **) data[3]
		                 ,*(unsigned long *) data[6]      /* unique constraint end */
		                );

		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s "
							"(instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time, is_in_effect, trigger_time) "
							"VALUES (%lu, %d, %lu, %s, E'%s', E'%s', %lu, %lu, %d, %lu, %s, %s, %d, %s)"
			                 ,ido2db_db_tablenames[IDO2DB_DBTABLE_DOWNTIMEHISTORY]
					 ,*(unsigned long *) data[0]     /* insert start */
			                 ,*(int *) data[1]
			                 ,*(unsigned long *) data[2]
			                 ,*(char **) data[3]
			                 ,*(char **) data[4]
			                 ,*(char **) data[5]
			                 ,*(unsigned long *) data[6]
			                 ,*(unsigned long *) data[7]
			                 ,*(int *) data[8]
			                 ,*(unsigned long *) data[9]
			                 ,*(char **) data[10]
			                 ,*(char **) data[11]             
				 	 ,*(int *) data[15] /* is_in_effect */
					 ,*(char **) data[16] /* trigger_time as sql string */

					 /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X4"), (uint *) data[12])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_downtimedata_downtime_history, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[5] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_downtimedata_downtime_history, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X10"), (uint *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X11"), (uint *) data[13])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X12"), (uint *) data[14])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X13"), (int *) data[15])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_downtimedata_downtime_history, MT(":X14"), (uint *) data[17])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_downtimedata_downtime_history)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_downtimedata_downtime_history() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_downtime_history_add() end\n");

	return result;
}



/************************************/
/* PROGRAMSTATUS                    */
/************************************/

int ido2db_query_insert_or_update_programstatusdata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char * query = NULL;
	char * query1 = NULL;
	char * query2 = NULL;
//	unsigned long programstatus_id;
	int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_programstatusdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET status_update_time=%s, program_start_time=%s, is_currently_running=1, "
                                                "process_id=%lu, daemon_mode=%d, last_command_check=%s, "
                                                "last_log_rotation=%s, notifications_enabled=%d, active_service_checks_enabled=%d, "
                                                "passive_service_checks_enabled=%d, active_host_checks_enabled=%d, passive_host_checks_enabled=%d, "
                                                "event_handlers_enabled=%d, flap_detection_enabled=%d, failure_prediction_enabled=%d, "
                                                "process_performance_data=%d, obsess_over_hosts=%d, obsess_over_services=%d, "
                                                "modified_host_attributes=%lu, modified_service_attributes=%lu, global_host_event_handler='%s', "
                                                "global_service_event_handler='%s', disable_notif_expire_time=%s, program_version='%s' "
                                                "WHERE instance_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS],
                                 *(char **) data[1],             /* update start */
                                 *(char **) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(char **) data[5],
                                 *(char **) data[6],
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9],
                                 *(int *) data[10],
                                 *(int *) data[11],
                                 *(int *) data[12],
                                 *(int *) data[13],
                                 *(int *) data[14],
                                 *(int *) data[15],
                                 *(int *) data[16],
                                 *(int *) data[17],
                                 *(unsigned long *) data[18],
                                 *(unsigned long *) data[19],
                                 *(char **) data[20],
                                 *(char **) data[21],
                                 *(char **) data[26],
                                 *(char **) data[28],            /* update end */
                                 *(unsigned long *) data[0]      /* unique constraint start/end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_programstatusdata_add() dbi_result_get_numrows_affected=%lu\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT programstatus_id FROM %s WHERE instance_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS],
                                 *(unsigned long *) data[0]      /* unique constraint start/end */
                                );
                        
                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                programstatus_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "programstatus_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }
                                         
                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


			if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, status_update_time, program_start_time, "
                                                        "is_currently_running, process_id, daemon_mode, "
                                                        "last_command_check, last_log_rotation, notifications_enabled, "
                                                        "active_service_checks_enabled, passive_service_checks_enabled, active_host_checks_enabled, "
                                                        "passive_host_checks_enabled, event_handlers_enabled, flap_detection_enabled, "
                                                        "failure_prediction_enabled, process_performance_data, obsess_over_hosts, "
                                                        "obsess_over_services, modified_host_attributes, modified_service_attributes, "
                                                        "global_host_event_handler, global_service_event_handler, disable_notif_expire_time, program_version) "
                                                        "VALUES (%lu, %s, %s, '1', %lu, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lu, %lu, '%s', '%s', %s, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(char **) data[1],
                                         *(char **) data[2],
                                         *(unsigned long *) data[3],
                                         *(int *) data[4],
                                         *(char **) data[5],
                                         *(char **) data[6],
                                         *(int *) data[7],
                                         *(int *) data[8],
                                         *(int *) data[9],
                                         *(int *) data[10],
                                         *(int *) data[11],
                                         *(int *) data[12],
                                         *(int *) data[13],
                                         *(int *) data[14],
                                         *(int *) data[15],
                                         *(int *) data[16],
                                         *(int *) data[17],
                                         *(unsigned long *) data[18],
                                         *(unsigned long *) data[19],
                                         *(char **) data[20],
                                         *(char **) data[21],
                                         *(char **) data[26],
                                         *(char **) data[28]             /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET status_update_time=%s, program_start_time=%s, is_currently_running=1, "
						"process_id=%lu, daemon_mode=%d, last_command_check=%s, "
						"last_log_rotation=%s, notifications_enabled=%d, active_service_checks_enabled=%d, "
						"passive_service_checks_enabled=%d, active_host_checks_enabled=%d, passive_host_checks_enabled=%d, "
						"event_handlers_enabled=%d, flap_detection_enabled=%d, failure_prediction_enabled=%d, "
						"process_performance_data=%d, obsess_over_hosts=%d, obsess_over_services=%d, "
						"modified_host_attributes=%lu, modified_service_attributes=%lu, global_host_event_handler=E'%s', "
						"global_service_event_handler=E'%s', disable_notif_expire_time=%s, program_version='%s' "
						"WHERE instance_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS],
		                 *(char **) data[1],             /* update start */
		                 *(char **) data[2],
		                 *(unsigned long *) data[3],
		                 *(int *) data[4],
		                 *(char **) data[5],
		                 *(char **) data[6],
		                 *(int *) data[7],
		                 *(int *) data[8],
		                 *(int *) data[9],
		                 *(int *) data[10],
		                 *(int *) data[11],
		                 *(int *) data[12],
		                 *(int *) data[13],
		                 *(int *) data[14],
		                 *(int *) data[15],
		                 *(int *) data[16],
		                 *(int *) data[17],
		                 *(unsigned long *) data[18],
		                 *(unsigned long *) data[19],
		                 *(char **) data[20],
		                 *(char **) data[21],
		                 *(char **) data[26],
		                 *(char **) data[28],		 /* update end */
		                 *(unsigned long *) data[0]      /* unique constraint start/end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, status_update_time, program_start_time, "
							"is_currently_running, process_id, daemon_mode, "
							"last_command_check, last_log_rotation, notifications_enabled, "
							"active_service_checks_enabled, passive_service_checks_enabled, active_host_checks_enabled, "
							"passive_host_checks_enabled, event_handlers_enabled, flap_detection_enabled, "
							"failure_prediction_enabled, process_performance_data, obsess_over_hosts, "
							"obsess_over_services, modified_host_attributes, modified_service_attributes, "
							"global_host_event_handler, global_service_event_handler, disable_notif_expire_time, program_version) "
							"VALUES (%lu, %s, %s, '1', %lu, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lu, %lu, E'%s', E'%s', %s, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_PROGRAMSTATUS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(char **) data[1],
			                 *(char **) data[2],
			                 *(unsigned long *) data[3],
			                 *(int *) data[4],
			                 *(char **) data[5],
			                 *(char **) data[6],
			                 *(int *) data[7],
			                 *(int *) data[8],
			                 *(int *) data[9],
			                 *(int *) data[10],
			                 *(int *) data[11],
			                 *(int *) data[12],
			                 *(int *) data[13],
			                 *(int *) data[14],
			                 *(int *) data[15],
			                 *(int *) data[16],
			                 *(int *) data[17],
			                 *(unsigned long *) data[18],
			                 *(unsigned long *) data[19],
			                 *(char **) data[20],
			                 *(char **) data[21],
			                 *(char **) data[26],
			                 *(char **) data[28]             /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* bind params to prepared statement */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus, MT(":X2"), (uint *) data[22])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus, MT(":X3"), (uint *) data[23])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus, MT(":X6"), (uint *) data[24])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus, MT(":X7"), (uint *) data[25])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X12"), (int *) data[11])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X13"), (int *) data[12])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X14"), (int *) data[13])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X15"), (int *) data[14])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X16"), (int *) data[15])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X17"), (int *) data[16])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_programstatus, MT(":X18"), (int *) data[17])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus, MT(":X19"), (uint *) data[18])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus, MT(":X20"), (uint *) data[19])) {
		return IDO_ERROR;
	}
	if (*(char **) data[20] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_programstatus, ":X21") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_programstatus, MT(":X21"), *(char **) data[20], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[21] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_programstatus, ":X22") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_programstatus, MT(":X22"), *(char **) data[21], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_programstatus, MT(":X23"), (uint *) data[27])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
        if (*(char **) data[28] == NULL) {
                if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_programstatus, ":X24") == IDO_ERROR) {
                        return IDO_ERROR;
                }
        } else {
                if (!OCI_BindString(idi->dbinfo.oci_statement_programstatus, MT(":X24"), *(char **) data[28], 0)) {
                        return IDO_ERROR;
                }
        }


	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_programstatus)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_programstatus() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_programstatusdata_add() end\n");

	return result;
}


/************************************/
/* HOSTSTATUS                       */
/************************************/

int ido2db_query_insert_or_update_hoststatusdata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char * query = NULL;
	char * query1 = NULL;
	char * query2 = NULL;
//	unsigned long hoststatus_id;
	int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	OCI_Lob *lob_oi;
	OCI_Lob *lob_ou;
	OCI_Lob *lob_loi;
	OCI_Lob *lob_lou;
	OCI_Lob *lob_pi;
	OCI_Lob *lob_pu;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;


	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		/* truncate long_output #2342 */
		if (*(char **) data[4] != NULL) {
			if (strlen(*(char **) data[4]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[4])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() Warning:long_output truncated\n");
				if ((*(char **) data[4])[strlen(*(char **) data[4]) - 1] == '\\') {
					(*(char **) data[4])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}
		if (*(char **) data[5] != NULL) {
			if (strlen(*(char **) data[5]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[5])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() Warning:perfdata truncated\n");
				if ((*(char **) data[5])[strlen(*(char **) data[5]) - 1] == '\\') {
					(*(char **) data[5])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}
	        asprintf(&query1, "UPDATE %s SET instance_id=%lu, host_object_id=%lu, status_update_time=%s, output='%s', long_output='%s', perfdata='%s', current_state=%d, has_been_checked=%d, should_be_scheduled=%d, current_check_attempt=%d, max_check_attempts=%d, last_check=%s, next_check=%s, check_type=%d, last_state_change=%s, last_hard_state_change=%s, last_hard_state=%d, last_time_up=%s, last_time_down=%s, last_time_unreachable=%s, state_type=%d, last_notification=%s, next_notification=%s, no_more_notifications=%d, notifications_enabled=%d, problem_has_been_acknowledged=%d, acknowledgement_type=%d, current_notification_number=%d, passive_checks_enabled=%d, active_checks_enabled=%d, event_handler_enabled=%d, flap_detection_enabled=%d, is_flapping=%d, percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth=%d, failure_prediction_enabled=%d, process_performance_data=%d, obsess_over_host=%d, modified_host_attributes=%lu, event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id=%lu WHERE host_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTSTATUS],
                                 *(unsigned long *) data[0],     /* update start */
                                 *(unsigned long *) data[1],
                                 *(char **) data[2],
                                 *(char **) data[3],
                                 *(char **) data[4],
                                 *(char **) data[5],
                                 *(int *) data[6],
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9],
                                 *(int *) data[10],
                                 *(char **) data[11],
                                 *(char **) data[12],
                                 *(int *) data[13],
                                 *(char **) data[14],
                                 *(char **) data[15],
                                 *(int *) data[16],
                                 *(char **) data[17],
                                 *(char **) data[18],
                                 *(char **) data[19],
                                 *(int *) data[20],
                                 *(char **) data[21],
                                 *(char **) data[22],
                                 *(int *) data[23],
                                 *(int *) data[24],
                                 *(int *) data[25],
                                 *(int *) data[26],
                                 *(int *) data[27],
                                 *(int *) data[28],
                                 *(int *) data[29],
                                 *(int *) data[30],
                                 *(int *) data[31],
                                 *(int *) data[32],
                                 *(double *) data[33],
                                 *(double *) data[34],
                                 *(double *) data[35],
                                 *(int *) data[36],
                                 *(int *) data[37],
                                 *(int *) data[38],
                                 *(int *) data[39],
                                 *(unsigned long *) data[40],
                                 *(char **) data[41],
                                 *(char **) data[42],
                                 *(double *) data[43],
                                 *(double *) data[44],
                                 *(unsigned long *) data[45],     /* update end */
                                 *(unsigned long *) data[1]     /* unique constraint start/end */
                                );
	        /* send query to db */
        	result = ido2db_db_query(idi, query1);
	        free(query1);
                
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() dbi_result_get_numrows_affected=%lu\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			asprintf(&query, "SELECT hoststatus_id FROM %s WHERE host_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTSTATUS],
                                *(unsigned long *) data[1]     /* unique constraint start/end */
                                );

	                /* send query to db */
	                if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
        	                if (idi->dbinfo.dbi_result != NULL) {
                	                if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                        	                hoststatus_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hoststatus_id");
                                	        mysql_update = TRUE;
	                                } else {
        	                                mysql_update = FALSE;
                	                }

	                                dbi_result_free(idi->dbinfo.dbi_result);
        	                        idi->dbinfo.dbi_result = NULL;
	                        }
	                }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
        	        free(query);

			if (mysql_update == FALSE) {
                        	/* try insert instead */
	                        asprintf(&query2, "INSERT INTO %s (instance_id, host_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_up, last_time_down, last_time_unreachable, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_host, modified_host_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (%lu, %lu, %s, '%s', '%s', '%s', %d, %d, %d, %d, %d, %s, %s, %d, %s, %s, %d, %s, %s, %s, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %lf, %d, %d, %d, %d, %lu, '%s', '%s', %lf, %lf, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTSTATUS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(char **) data[2],
                                         *(char **) data[3],
                                         *(char **) data[4],
                                         *(char **) data[5],
                                         *(int *) data[6],
                                         *(int *) data[7],
                                         *(int *) data[8],
                                         *(int *) data[9],
                                         *(int *) data[10],
                                         *(char **) data[11],
                                         *(char **) data[12],
                                         *(int *) data[13],
                                         *(char **) data[14],
                                         *(char **) data[15],
                                         *(int *) data[16],
                                         *(char **) data[17],
                                         *(char **) data[18],
                                         *(char **) data[19],
                                         *(int *) data[20],
                                         *(char **) data[21],
                                         *(char **) data[22],
                                         *(int *) data[23],
                                         *(int *) data[24],
                                         *(int *) data[25],
                                         *(int *) data[26],
                                         *(int *) data[27],
                                         *(int *) data[28],
                                         *(int *) data[29],
                                         *(int *) data[30],
                                         *(int *) data[31],
                                         *(int *) data[32],
                                         *(double *) data[33],
                                         *(double *) data[34],
                                         *(double *) data[35],
                                         *(int *) data[36],
                                         *(int *) data[37],
                                         *(int *) data[38],
                                         *(int *) data[39],
                                         *(unsigned long *) data[40],
                                         *(char **) data[41],
                                         *(char **) data[42],
                                         *(double *) data[43],
                                         *(double *) data[44],
                                         *(unsigned long *) data[45]     /* insert end */
                                        );
        	                /* send query to db */
                	        result = ido2db_db_query(idi, query2);
                        	free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;


	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu, host_object_id=%lu, status_update_time=%s, output=E'%s', long_output=E'%s', perfdata=E'%s', current_state=%d, has_been_checked=%d, should_be_scheduled=%d, current_check_attempt=%d, max_check_attempts=%d, last_check=%s, next_check=%s, check_type=%d, last_state_change=%s, last_hard_state_change=%s, last_hard_state=%d, last_time_up=%s, last_time_down=%s, last_time_unreachable=%s, state_type=%d, last_notification=%s, next_notification=%s, no_more_notifications=%d, notifications_enabled=%d, problem_has_been_acknowledged=%d, acknowledgement_type=%d, current_notification_number=%d, passive_checks_enabled=%d, active_checks_enabled=%d, event_handler_enabled=%d, flap_detection_enabled=%d, is_flapping=%d, percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth=%d, failure_prediction_enabled=%d, process_performance_data=%d, obsess_over_host=%d, modified_host_attributes=%lu, event_handler=E'%s', check_command=E'%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id=%lu WHERE host_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTSTATUS],
		                 *(unsigned long *) data[0],     /* update start */
		                 *(unsigned long *) data[1],
		                 *(char **) data[2],
		                 *(char **) data[3],
		                 *(char **) data[4],
		                 *(char **) data[5],
		                 *(int *) data[6],
		                 *(int *) data[7],
		                 *(int *) data[8],
		                 *(int *) data[9],
		                 *(int *) data[10],
		                 *(char **) data[11],
		                 *(char **) data[12],
		                 *(int *) data[13],
		                 *(char **) data[14],
		                 *(char **) data[15],
		                 *(int *) data[16],
		                 *(char **) data[17],
		                 *(char **) data[18],
		                 *(char **) data[19],
		                 *(int *) data[20],
		                 *(char **) data[21],
		                 *(char **) data[22],
		                 *(int *) data[23],
		                 *(int *) data[24],
		                 *(int *) data[25],
		                 *(int *) data[26],
		                 *(int *) data[27],
		                 *(int *) data[28],
		                 *(int *) data[29],
		                 *(int *) data[30],
		                 *(int *) data[31],
		                 *(int *) data[32],
		                 *(double *) data[33],
		                 *(double *) data[34],
		                 *(double *) data[35],
		                 *(int *) data[36],
		                 *(int *) data[37],
		                 *(int *) data[38],
		                 *(int *) data[39],
		                 *(unsigned long *) data[40],
		                 *(char **) data[41],
		                 *(char **) data[42],
		                 *(double *) data[43],
		                 *(double *) data[44],
		                 *(unsigned long *) data[45],     /* update end */
		                 *(unsigned long *) data[1]     /* unique constraint start/end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, host_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_up, last_time_down, last_time_unreachable, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_host, modified_host_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (%lu, %lu, %s, E'%s', E'%s', E'%s', %d, %d, %d, %d, %d, %s, %s, %d, %s, %s, %d, %s, %s, %s, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %lf, %d, %d, %d, %d, %lu, E'%s', E'%s', %lf, %lf, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTSTATUS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(char **) data[2],
			                 *(char **) data[3],
			                 *(char **) data[4],
			                 *(char **) data[5],
			                 *(int *) data[6],
			                 *(int *) data[7],
			                 *(int *) data[8],
			                 *(int *) data[9],
			                 *(int *) data[10],
			                 *(char **) data[11],
			                 *(char **) data[12],
			                 *(int *) data[13],
			                 *(char **) data[14],
			                 *(char **) data[15],
			                 *(int *) data[16],
			                 *(char **) data[17],
			                 *(char **) data[18],
			                 *(char **) data[19],
			                 *(int *) data[20],
			                 *(char **) data[21],
			                 *(char **) data[22],
			                 *(int *) data[23],
			                 *(int *) data[24],
			                 *(int *) data[25],
			                 *(int *) data[26],
			                 *(int *) data[27],
			                 *(int *) data[28],
			                 *(int *) data[29],
			                 *(int *) data[30],
			                 *(int *) data[31],
			                 *(int *) data[32],
			                 *(double *) data[33],
			                 *(double *) data[34],
			                 *(double *) data[35],
			                 *(int *) data[36],
			                 *(int *) data[37],
			                 *(int *) data[38],
			                 *(int *) data[39],
			                 *(unsigned long *) data[40],
			                 *(char **) data[41],
			                 *(char **) data[42],
			                 *(double *) data[43],
			                 *(double *) data[44],
			                 *(unsigned long *) data[45]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* bind params to prepared statement */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X3"), (uint *) data[46])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}

	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X7"), (int *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X12"), (uint *) data[47])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X13"), (uint *) data[48])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X14"), (int *) data[13])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X15"), (uint *) data[49])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X16"), (uint *) data[50])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X17"), (int *) data[16])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X18"), (uint *) data[51])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X19"), (uint *) data[52])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X20"), (uint *) data[53])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X21"), (int *) data[20])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X22"), (uint *) data[54])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X23"), (uint *) data[55])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X24"), (int *) data[23])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X25"), (int *) data[24])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X26"), (int *) data[25])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X27"), (int *) data[26])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X28"), (int *) data[27])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X29"), (int *) data[28])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X30"), (int *) data[29])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X31"), (int *) data[30])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X32"), (int *) data[31])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X33"), (int *) data[32])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hoststatus, MT(":X34"), (double *) data[33])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hoststatus, MT(":X35"), (double *) data[34])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hoststatus, MT(":X36"), (double *) data[35])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X37"), (int *) data[36])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X38"), (int *) data[37])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X39"), (int *) data[38])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hoststatus, MT(":X40"), (int *) data[39])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X41"), (uint *) data[40])) {
		return IDO_ERROR;
	}
	if (*(char **) data[41] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hoststatus, ":X42") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hoststatus, MT(":X42"), *(char **) data[41], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[42] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hoststatus, ":X43") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hoststatus, MT(":X43"), *(char **) data[42], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hoststatus, MT(":X44"), (double *) data[43])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hoststatus, MT(":X45"), (double *) data[44])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hoststatus, MT(":X46"), (uint *) data[45])) {
		return IDO_ERROR;
	}
	//bind clob 2 times,once for update, once for insert to make oracle happy and avoid ora-600 because of double binding
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatus() bind clob\n");
	lob_oi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_ou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_loi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_lou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_pi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_pu = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);

	result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_hoststatus, ":X4i", *(char **)data[3], &lob_oi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_hoststatus, ":X4u", *(char **)data[3], &lob_ou);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_hoststatus, ":X5i", *(char **)data[4], &lob_loi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_hoststatus, ":X5u", *(char **)data[4], &lob_lou);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_hoststatus, ":X6i", *(char **)data[5], &lob_pi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_hoststatus, ":X6u", *(char **)data[5], &lob_pu);
	if (result == IDO_OK) {
		/* execute statement */
		result = OCI_Execute(idi->dbinfo.oci_statement_hoststatus) ? IDO_OK : IDO_ERROR;
		if (result == IDO_OK) {
			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatus()  executed\n");
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatus()  execute error\n");

		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatus()  clob bind error\n");
	}
	//free lobs
	if (lob_ou != NULL) OCI_LobFree(lob_ou);
	if (lob_oi != NULL) OCI_LobFree(lob_oi);
	if (lob_lou != NULL) OCI_LobFree(lob_lou);
	if (lob_loi != NULL) OCI_LobFree(lob_loi);
	if (lob_pu != NULL) OCI_LobFree(lob_pu);
	if (lob_pi != NULL) OCI_LobFree(lob_pi);
	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() end\n");

	return result;
}

/************************************/
/* SERVICESTATUS                    */
/************************************/

int ido2db_query_insert_or_update_servicestatusdata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
	char * query = NULL;
	char * query1 = NULL;
	char * query2 = NULL;
//	unsigned long servicestatus_id;
	int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	OCI_Lob *lob_oi;
	OCI_Lob *lob_ou;
	OCI_Lob *lob_loi;
	OCI_Lob *lob_lou;
	OCI_Lob *lob_pi;
	OCI_Lob *lob_pu;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatusdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		/* truncate long_output #2342 */
		if (*(char **) data[4] != NULL) {
			if (strlen(*(char **) data[4]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[4])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatusdata_add() Warning:long_output truncated\n");
				if ((*(char **) data[4])[strlen(*(char **) data[4]) - 1] == '\\') {
					(*(char **) data[4])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatusdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}	
		if (*(char **) data[5] != NULL) {
			if (strlen(*(char **) data[5]) > IDO2DB_MYSQL_MAX_TEXT_LEN ) {
				(*(char **) data[5])[IDO2DB_MYSQL_MAX_TEXT_LEN]=0;
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatusdata_add() Warning:perfdata truncated\n");
				if ((*(char **) data[5])[strlen(*(char **) data[5]) - 1] == '\\') {
					(*(char **) data[5])[IDO2DB_MYSQL_MAX_TEXT_LEN - 1]=0;
					ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatusdata_add() Warning:backslash detected at the end, truncating a bit more\n");
				}
			}
		}
                asprintf(&query1, "UPDATE %s SET instance_id=%lu, service_object_id=%lu, status_update_time=%s, output='%s', long_output='%s', perfdata='%s', current_state=%d, has_been_checked=%d, should_be_scheduled=%d, current_check_attempt=%d, max_check_attempts=%d, last_check=%s, next_check=%s, check_type=%d, last_state_change=%s, last_hard_state_change=%s, last_hard_state=%d, last_time_ok=%s, last_time_warning=%s, last_time_unknown=%s, last_time_critical=%s, state_type=%d, last_notification=%s, next_notification=%s, no_more_notifications=%d, notifications_enabled=%d, problem_has_been_acknowledged=%d, acknowledgement_type=%d, current_notification_number=%d, passive_checks_enabled=%d, active_checks_enabled=%d, event_handler_enabled=%d, flap_detection_enabled=%d, is_flapping=%d, percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth=%d, failure_prediction_enabled=%d, process_performance_data=%d, obsess_over_service=%d, modified_service_attributes=%lu, event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id=%lu WHERE service_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICESTATUS],
                                 *(unsigned long *) data[0],     /* update start */
                                 *(unsigned long *) data[1],
                                 *(char **) data[2],
                                 *(char **) data[3],
                                 *(char **) data[4],
                                 *(char **) data[5],
                                 *(int *) data[6],
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9],
                                 *(int *) data[10],
                                 *(char **) data[11],
                                 *(char **) data[12],
                                 *(int *) data[13],
                                 *(char **) data[14],
                                 *(char **) data[15],
                                 *(int *) data[16],
                                 *(char **) data[17],
                                 *(char **) data[18],
                                 *(char **) data[19],
                                 *(char **) data[20],
                                 *(int *) data[21],
                                 *(char **) data[22],
                                 *(char **) data[23],
                                 *(int *) data[24],
                                 *(int *) data[25],
                                 *(int *) data[26],
                                 *(int *) data[27],
                                 *(int *) data[28],
                                 *(int *) data[29],
                                 *(int *) data[30],
                                 *(int *) data[31],
                                 *(int *) data[32],
                                 *(int *) data[33],
                                 *(double *) data[34],
                                 *(double *) data[35],
                                 *(double *) data[36],
                                 *(int *) data[37],
                                 *(int *) data[38],
                                 *(int *) data[39],
                                 *(int *) data[40],
                                 *(unsigned long *) data[41],
                                 *(char **) data[42],
                                 *(char **) data[43],
                                 *(double *) data[44],
                                 *(double *) data[45],
                                 *(unsigned long *) data[46],     /* update end */
                                 *(unsigned long *) data[1]     /* unique constraint start/end */
                                );

                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatusdata_add() dbi_result_get_numrows_affected=%lu\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


	                asprintf(&query, "SELECT servicestatus_id FROM %s WHERE service_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICESTATUS],
                                 *(unsigned long *) data[1]     /* unique constraint start/end */
                                );

	                /* send query to db */
	                if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
	                        if (idi->dbinfo.dbi_result != NULL) {
	                                if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//	                                        servicestatus_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "servicestatus_id");
	                                        mysql_update = TRUE;
	                                } else {
	                                        mysql_update = FALSE;
	                                }

	                                dbi_result_free(idi->dbinfo.dbi_result);
	                                idi->dbinfo.dbi_result = NULL;
	                        }
	                }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
        	        free(query);

			if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, service_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_ok, last_time_warning, last_time_unknown, last_time_critical, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_service, modified_service_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (%lu, %lu, %s, '%s', '%s', '%s', %d, %d, %d, %d, %d, %s, %s, %d, %s, %s, %d, %s, %s, %s, %s, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%lf', '%lf', '%lf', %d, %d, %d, %d, %lu, '%s', '%s', '%lf', '%lf', %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICESTATUS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(char **) data[2],
                                         *(char **) data[3],
                                         *(char **) data[4],
                                         *(char **) data[5],
                                         *(int *) data[6],
                                         *(int *) data[7],
                                         *(int *) data[8],
                                         *(int *) data[9],
                                         *(int *) data[10],
                                         *(char **) data[11],
                                         *(char **) data[12],
                                         *(int *) data[13],
                                         *(char **) data[14],
                                         *(char **) data[15],
                                         *(int *) data[16],
                                         *(char **) data[17],
                                         *(char **) data[18],
                                         *(char **) data[19],
                                         *(char **) data[20],
                                         *(int *) data[21],
                                         *(char **) data[22],
                                         *(char **) data[23],
                                         *(int *) data[24],
                                         *(int *) data[25],
                                         *(int *) data[26],
                                         *(int *) data[27],
                                         *(int *) data[28],
                                         *(int *) data[29],
                                         *(int *) data[30],
                                         *(int *) data[31],
                                         *(int *) data[32],
                                         *(int *) data[33],
                                         *(double *) data[34],
                                         *(double *) data[35],
                                         *(double *) data[36],
                                         *(int *) data[37],
                                         *(int *) data[38],
                                         *(int *) data[39],
                                         *(int *) data[40],
                                         *(unsigned long *) data[41],
                                         *(char **) data[42],
                                         *(char **) data[43],
                                         *(double *) data[44],
                                         *(double *) data[45],
                                         *(unsigned long *) data[46]     /* insert end */
                                        );
                	        /* send query to db */
                        	result = ido2db_db_query(idi, query2);
	                        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu, service_object_id=%lu, status_update_time=%s, output=E'%s', long_output=E'%s', perfdata=E'%s', current_state=%d, has_been_checked=%d, should_be_scheduled=%d, current_check_attempt=%d, max_check_attempts=%d, last_check=%s, next_check=%s, check_type=%d, last_state_change=%s, last_hard_state_change=%s, last_hard_state=%d, last_time_ok=%s, last_time_warning=%s, last_time_unknown=%s, last_time_critical=%s, state_type=%d, last_notification=%s, next_notification=%s, no_more_notifications=%d, notifications_enabled=%d, problem_has_been_acknowledged=%d, acknowledgement_type=%d, current_notification_number=%d, passive_checks_enabled=%d, active_checks_enabled=%d, event_handler_enabled=%d, flap_detection_enabled=%d, is_flapping=%d, percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth=%d, failure_prediction_enabled=%d, process_performance_data=%d, obsess_over_service=%d, modified_service_attributes=%lu, event_handler=E'%s', check_command=E'%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id=%lu WHERE service_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICESTATUS],
		                 *(unsigned long *) data[0],     /* update start */
		                 *(unsigned long *) data[1],
		                 *(char **) data[2],
		                 *(char **) data[3],
		                 *(char **) data[4],
		                 *(char **) data[5],
		                 *(int *) data[6],
		                 *(int *) data[7],
		                 *(int *) data[8],
		                 *(int *) data[9],
		                 *(int *) data[10],
		                 *(char **) data[11],
		                 *(char **) data[12],
		                 *(int *) data[13],
		                 *(char **) data[14],
		                 *(char **) data[15],
		                 *(int *) data[16],
		                 *(char **) data[17],
		                 *(char **) data[18],
		                 *(char **) data[19],
		                 *(char **) data[20],
		                 *(int *) data[21],
		                 *(char **) data[22],
		                 *(char **) data[23],
		                 *(int *) data[24],
		                 *(int *) data[25],
		                 *(int *) data[26],
		                 *(int *) data[27],
		                 *(int *) data[28],
		                 *(int *) data[29],
		                 *(int *) data[30],
		                 *(int *) data[31],
		                 *(int *) data[32],
		                 *(int *) data[33],
		                 *(double *) data[34],
		                 *(double *) data[35],
		                 *(double *) data[36],
		                 *(int *) data[37],
		                 *(int *) data[38],
		                 *(int *) data[39],
		                 *(int *) data[40],
		                 *(unsigned long *) data[41],
		                 *(char **) data[42],
		                 *(char **) data[43],
		                 *(double *) data[44],
		                 *(double *) data[45],
		                 *(unsigned long *) data[46],     /* update end */
		                 *(unsigned long *) data[1]     /* unique constraint start/end */
		                );

		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, service_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_ok, last_time_warning, last_time_unknown, last_time_critical, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_service, modified_service_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (%lu, %lu, %s, E'%s', E'%s', E'%s', %d, %d, %d, %d, %d, %s, %s, %d, %s, %s, %d, %s, %s, %s, %s, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%lf', '%lf', '%lf', %d, %d, %d, %d, %lu, E'%s', E'%s', '%lf', '%lf', %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICESTATUS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(char **) data[2],
			                 *(char **) data[3],
			                 *(char **) data[4],
			                 *(char **) data[5],
			                 *(int *) data[6],
			                 *(int *) data[7],
			                 *(int *) data[8],
			                 *(int *) data[9],
			                 *(int *) data[10],
			                 *(char **) data[11],
			                 *(char **) data[12],
			                 *(int *) data[13],
			                 *(char **) data[14],
			                 *(char **) data[15],
			                 *(int *) data[16],
			                 *(char **) data[17],
			                 *(char **) data[18],
			                 *(char **) data[19],
			                 *(char **) data[20],
			                 *(int *) data[21],
			                 *(char **) data[22],
			                 *(char **) data[23],
			                 *(int *) data[24],
			                 *(int *) data[25],
			                 *(int *) data[26],
			                 *(int *) data[27],
			                 *(int *) data[28],
			                 *(int *) data[29],
			                 *(int *) data[30],
			                 *(int *) data[31],
			                 *(int *) data[32],
			                 *(int *) data[33],
			                 *(double *) data[34],
			                 *(double *) data[35],
			                 *(double *) data[36],
			                 *(int *) data[37],
			                 *(int *) data[38],
			                 *(int *) data[39],
			                 *(int *) data[40],
			                 *(unsigned long *) data[41],
			                 *(char **) data[42],
			                 *(char **) data[43],
			                 *(double *) data[44],
			                 *(double *) data[45],
			                 *(unsigned long *) data[46]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* bind params to prepared statement */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X3"), (uint *) data[47])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}

	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X7"), (int *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X12"), (uint *) data[48])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X13"), (uint *) data[49])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X14"), (int *) data[13])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X15"), (uint *) data[50])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X16"), (uint *) data[51])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X17"), (int *) data[16])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X18"), (uint *) data[52])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X19"), (uint *) data[53])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X20"), (uint *) data[54])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X21"), (uint *) data[55])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X22"), (int *) data[21])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X23"), (uint *) data[56])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X24"), (uint *) data[57])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X25"), (int *) data[24])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X26"), (int *) data[25])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X27"), (int *) data[26])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X28"), (int *) data[27])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X29"), (int *) data[28])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X30"), (int *) data[29])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X31"), (int *) data[30])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X32"), (int *) data[31])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X33"), (int *) data[32])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X34"), (int *) data[33])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicestatus, MT(":X35"), (double *) data[34])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicestatus, MT(":X36"), (double *) data[35])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicestatus, MT(":X37"), (double *) data[36])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X38"), (int *) data[37])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X39"), (int *) data[38])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X40"), (int *) data[39])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicestatus, MT(":X41"), (int *) data[40])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X42"), (uint *) data[41])) {
		return IDO_ERROR;
	}
	if (*(char **) data[42] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicestatus, ":X43") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicestatus, MT(":X43"), *(char **) data[42], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[43] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicestatus, ":X44") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicestatus, MT(":X44"), *(char **) data[43], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicestatus, MT(":X45"), (double *) data[44])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicestatus, MT(":X46"), (double *) data[45])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicestatus, MT(":X47"), (uint *) data[46])) {
		return IDO_ERROR;
	}
	//bind clob 2 times,once for update, once for insert to make oracle happy and avoid ora-600 because of double binding
	lob_oi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_ou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_loi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_lou = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_pi = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);
	lob_pu = OCI_LobCreate(idi->dbinfo.oci_connection, OCI_CLOB);

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatus() bind clobs\n");
	result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_servicestatus, ":X4i", *(char **)data[3], &lob_oi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_servicestatus, ":X4u", *(char **)data[3], &lob_ou);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_servicestatus, ":X5i", *(char **)data[4], &lob_loi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_servicestatus, ":X5u", *(char **)data[4], &lob_lou);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_servicestatus, ":X6i", *(char **)data[5], &lob_pi);
	if (result == IDO_OK) result = ido2db_oci_bind_clob(idi->dbinfo.oci_statement_servicestatus, ":X6u", *(char **)data[5], &lob_pu);

	if (result == IDO_OK) {
		/* execute statement */
		result = OCI_Execute(idi->dbinfo.oci_statement_servicestatus) ? IDO_OK : IDO_ERROR;
		if (result == IDO_OK) {
			/* commit statement */
			OCI_Commit(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatus() executed\n");
		} else {
			OCI_Rollback(idi->dbinfo.oci_connection);
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatus() execute error\n");
		}
	} else {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatus() clob bind error\n");
	}
	//free lobs
	if (lob_ou != NULL) OCI_LobFree(lob_ou);
	if (lob_oi != NULL) OCI_LobFree(lob_oi);
	if (lob_lou != NULL) OCI_LobFree(lob_lou);
	if (lob_loi != NULL) OCI_LobFree(lob_loi);
	if (lob_pu != NULL) OCI_LobFree(lob_pu);
	if (lob_pi != NULL) OCI_LobFree(lob_pi);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatusdata_add() end\n");

	return result;
}


/************************************/
/* CONTACTSTATUS                    */
/************************************/

int ido2db_query_insert_or_update_contactstatusdata_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long contactstatus_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactstatusdata_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu, status_update_time=%s, host_notifications_enabled=%d, service_notifications_enabled=%d, last_host_notification=%s, last_service_notification=%s, modified_attributes=%lu, modified_host_attributes=%lu, modified_service_attributes=%lu WHERE contact_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTSTATUS],
                                 *(unsigned long *) data[0],     /* update start */
                                 *(char **) data[2],
                                 *(int *) data[3],
                                 *(int *) data[4],
                                 *(char **) data[5],
                                 *(char **) data[6],
                                 *(unsigned long *) data[7],
                                 *(unsigned long *) data[8],
                                 *(unsigned long *) data[9],     /* update end */
                                 *(unsigned long *) data[1]     /* unique constraint start/end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT contactstatus_id FROM %s WHERE contact_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTSTATUS],
                                 *(unsigned long *) data[1]     /* unique constraint start/end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                contactstatus_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contactstatus_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, contact_object_id, status_update_time, host_notifications_enabled, service_notifications_enabled, last_host_notification, last_service_notification, modified_attributes, modified_host_attributes, modified_service_attributes) VALUES (%lu, %lu, %s, %d, %d, %s, %s, %lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTSTATUS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(char **) data[2],
                                         *(int *) data[3],
                                         *(int *) data[4],
                                         *(char **) data[5],
                                         *(char **) data[6],
                                         *(unsigned long *) data[7],
                                         *(unsigned long *) data[8],
                                         *(unsigned long *) data[9]     /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu, status_update_time=%s, host_notifications_enabled=%d, service_notifications_enabled=%d, last_host_notification=%s, last_service_notification=%s, modified_attributes=%lu, modified_host_attributes=%lu, modified_service_attributes=%lu WHERE contact_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTSTATUS],
		                 *(unsigned long *) data[0],     /* update start */
		                 *(char **) data[2],
		                 *(int *) data[3],
		                 *(int *) data[4],
		                 *(char **) data[5],
		                 *(char **) data[6],
		                 *(unsigned long *) data[7],
		                 *(unsigned long *) data[8],
		                 *(unsigned long *) data[9],     /* update end */
		                 *(unsigned long *) data[1]     /* unique constraint start/end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, contact_object_id, status_update_time, host_notifications_enabled, service_notifications_enabled, last_host_notification, last_service_notification, modified_attributes, modified_host_attributes, modified_service_attributes) VALUES (%lu, %lu, %s, %d, %d, %s, %s, %lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTSTATUS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(char **) data[2],
			                 *(int *) data[3],
			                 *(int *) data[4],
			                 *(char **) data[5],
			                 *(char **) data[6],
			                 *(unsigned long *) data[7],
			                 *(unsigned long *) data[8],
			                 *(unsigned long *) data[9]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X3"), (uint *) data[10])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X4"), (int *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X6"), (uint *) data[11])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X7"), (uint *) data[12])) { /* unixtimestamp instead of time2sql */
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X9"), (uint *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactstatusdata, MT(":X10"), (uint *) data[9])) {
		return IDO_ERROR;
	}


	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_contactstatusdata)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_contactstatusdata() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactstatusdata_add() end\n");

	return result;
}


/************************************/
/* CONFIGFILEVARIABLES              */
/************************************/

int ido2db_query_insert_or_update_configfilevariables_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf1 = NULL;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_configfilevariables_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu, configfile_type=%d, configfile_path='%s' WHERE instance_id=%lu AND configfile_type=%d AND configfile_path='%s'",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES],
                                 *(unsigned long *) data[0],     /* update start */
                                 *(int *) data[1],
                                 *(char **) data[2],              /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(char **) data[2]             /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_configfilevariables_add(%lu) update rows affected\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT configfile_id FROM %s WHERE instance_id=%lu AND configfile_type=%d AND configfile_path='%s'",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(char **) data[2]             /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
						/* this condition should never happen, as libdbi UPDATE and affected rows
						 * should take care of it. it seems that newer mysql versions got problems
						 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
						 * as fallback here
						 */
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "configfile_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, configfile_type, configfile_path) VALUES (%lu, %d, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(char **) data[2]             /* insert end */
                                        );
                	        /* send query to db */
                        	result = ido2db_db_query(idi, query2);
	                        free(query2);

			        if (result == IDO_OK) {
			                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables"
		                                      " preceding ido2db_query_insert_or_update_configfilevariables_add OK \n");
		                        /* mysql doesn't use sequences */
                		        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables(%lu) configfile_id\n", *id);
				}

                		dbi_result_free(idi->dbinfo.dbi_result);
	                	idi->dbinfo.dbi_result = NULL;
			}
                } else {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;

			/* we did an update, get the id */
                        asprintf(&query, "SELECT configfile_id FROM %s WHERE instance_id=%lu AND configfile_type=%d AND configfile_path='%s'",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(char **) data[2]             /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "configfile_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables(%lu) configfile_id\n", *id);
                                        } 

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        } 
			else {
				dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
			}
                        free(query);

		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu, configfile_type=%d, configfile_path=E'%s' WHERE instance_id=%lu AND configfile_type=%d AND configfile_path=E'%s'",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES],
		                 *(unsigned long *) data[0],     /* update start */
		                 *(int *) data[1],
		                 *(char **) data[2],              /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(int *) data[1],
		                 *(char **) data[2]             /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_configfilevariables_add(%lu) update rows affected\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, configfile_type, configfile_path) VALUES (%lu, %d, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(char **) data[2]             /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

		        if (result == IDO_OK) {
		                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables"
                		                      " preceding ido2db_query_insert_or_update_configfilevariables_add OK \n");
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf1, "%s_configfile_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES]) == -1)
                	                buf1 = NULL;

                        	*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
	                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables(%s=%lu) configfile_id\n", buf1, *id);
        	                free(buf1);
			}

                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
                } else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

                        /* we did an update, get the id */
                        asprintf(&query, "SELECT configfile_id FROM %s WHERE instance_id=%lu AND configfile_type=%d AND configfile_path='%s'",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONFIGFILES],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(char **) data[2]             /* unique constraint end */
                                );
                                 
                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "configfile_id");
	                        		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables(%lu) configfile_id\n", *id);
                                        } 
                
                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);

                }

		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_configfilevariables, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_configfilevariables, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (*(char **) data[2] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_configfilevariables, ":X3") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_configfilevariables, MT(":X3"), *(char **) data[2], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_configfilevariables)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_configfilevariables() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);


	/* retrieve last inserted configfile_id */
	if (asprintf(&seq_name, "seq_configfiles") == -1)
                seq_name = NULL;
	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_configfilevariables(id %lu) \n", *id);
        free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_configfilevariables_add() end\n");

	return result;
}


/************************************/
/* RUNTIMEVARIABLES                 */
/************************************/

int ido2db_query_insert_or_update_runtimevariables_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long runtimevariable_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_runtimevariables_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET varvalue='%s' WHERE instance_id=%lu AND varname='%s'",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_RUNTIMEVARIABLES],
                                 *(char **) data[2],             /* update start/end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[1]             /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT runtimevariable_id FROM %s WHERE instance_id=%lu AND varname='%s'",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_RUNTIMEVARIABLES],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(char **) data[1]             /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                runtimevariable_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "runtimevariable_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, varname, varvalue) VALUES (%lu, '%s', '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_RUNTIMEVARIABLES],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(char **) data[1],
                                         *(char **) data[2]             /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);	
        	                free(query2);
			}
                }
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET varvalue=E'%s' WHERE instance_id=%lu AND varname=E'%s'",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_RUNTIMEVARIABLES],
		                 *(char **) data[2],             /* update start/end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(char **) data[1]		/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, varname, varvalue) VALUES (%lu, E'%s', E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_RUNTIMEVARIABLES],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(char **) data[1],
			                 *(char **) data[2]             /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_runtimevariables, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (*(char **) data[1] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_runtimevariables, ":X2") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_runtimevariables, MT(":X2"), *(char **) data[1], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[2] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_runtimevariables, ":X3") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_runtimevariables, MT(":X3"), *(char **) data[2], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_runtimevariables)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_runtimevariables() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_runtimevariables_add() end\n");

	return result;
}


/************************************/
/* HOSTDEFINITION                   */
/************************************/

int ido2db_query_insert_or_update_hostdefinition_definition_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
	int do_insert = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		if (!idi->tables_cleared) {
			asprintf(&query1, "UPDATE %s SET alias='%s', display_name='%s', address='%s', check_command_object_id=%lu, check_command_args='%s', eventhandler_command_object_id=%lu, eventhandler_command_args='%s', check_timeperiod_object_id=%lu, notification_timeperiod_object_id=%lu, failure_prediction_options='%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_down=%d, notify_on_unreachable=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_up=%d, stalk_on_down=%d, stalk_on_unreachable=%d, flap_detection_enabled=%d, flap_detection_on_up=%d, flap_detection_on_down=%d, flap_detection_on_unreachable=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_host=%d, failure_prediction_enabled=%d, notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s', vrml_image='%s', statusmap_image='%s', have_2d_coords=%d, x_2d=%d, y_2d=%d, have_3d_coords=%d, x_3d=%lf, y_3d=%lf, z_3d=%lf, address6='%s' WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu",
					 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS],
					 *(char **) data[3],             /* update start */
					 *(char **) data[4],
					 *(char **) data[5],
					 *(unsigned long *) data[6],
					 (*(char **) data[7] == NULL) ? "" : *(char **) data[7],
					 *(unsigned long *) data[8],
					 (*(char **) data[9] == NULL) ? "" : *(char **) data[9],
					 *(unsigned long *) data[10],
					 *(unsigned long *) data[11],
					 *(char **) data[12],
					 *(double *) data[13],
					 *(double *) data[14],
					 *(int *) data[15],
					 *(double *) data[16],
					 *(double *) data[17],
					 *(int *) data[18],
					 *(int *) data[19],
					 *(int *) data[20],
					 *(int *) data[21],
					 *(int *) data[22],
					 *(int *) data[23],
					 *(int *) data[24],
					 *(int *) data[25],
					 *(int *) data[26],
					 *(int *) data[27],
					 *(int *) data[28],
					 *(int *) data[29],
					 *(double *) data[30],
					 *(double *) data[31],
					 *(int *) data[32],
					 *(int *) data[33],
					 *(int *) data[34],
					 *(int *) data[35],
					 *(int *) data[36],
					 *(int *) data[37],
					 *(int *) data[38],
					 *(int *) data[39],
					 *(int *) data[40],
					 *(int *) data[41],
					 *(int *) data[42],
					 *(char **) data[43],
					 *(char **) data[44],
					 *(char **) data[45],
					 *(char **) data[46],
					 *(char **) data[47],
					 *(char **) data[48],
					 *(char **) data[49],
					 *(int *) data[50],
					 *(int *) data[51],
					 *(int *) data[52],
					 *(int *) data[53],
					 *(double *) data[54],
					 *(double *) data[55],
					 *(double *) data[56],
					 *(char **) data[57],           /* update end */
					 *(unsigned long *) data[0],     /* unique constraint start */
					 *(int *) data[1],
					 *(unsigned long *) data[2]      /* unique constraint end */
					);
			/* send query to db */
			result = ido2db_db_query(idi, query1);
			free(query1);

			/* check result if update was ok */
			if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) > 0) {
				dbi_result_free(idi->dbinfo.dbi_result);
				idi->dbinfo.dbi_result = NULL;

				/* we actually did an update, select the host id */
				asprintf(&query, "SELECT host_id FROM %s WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu",
					ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS],
					 *(unsigned long *) data[0],     /* unique constraint start */
					 *(int *) data[1],
					 *(unsigned long *) data[2]      /* unique constraint end */
					);

				/* send query to db */
				if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
					if (idi->dbinfo.dbi_result != NULL) {
						if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
							*id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "host_id");
							ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition(%lu) host_id\n", *id);
						}

						dbi_result_free(idi->dbinfo.dbi_result);
						idi->dbinfo.dbi_result = NULL;
					}
				}
				else {
					dbi_result_free(idi->dbinfo.dbi_result);
					idi->dbinfo.dbi_result = NULL;
				}
				free(query);

			} else {
				dbi_result_free(idi->dbinfo.dbi_result);
				idi->dbinfo.dbi_result = NULL;

				asprintf(&query, "SELECT host_id FROM %s WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu",
					ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS],
					 *(unsigned long *) data[0],     /* unique constraint start */
					 *(int *) data[1],
					 *(unsigned long *) data[2]      /* unique constraint end */
					);

				/* send query to db */
				if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
					if (idi->dbinfo.dbi_result != NULL) {
						if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
							/* this condition should never happen, as libdbi UPDATE and affected rows
							 * should take care of it. it seems that newer mysql versions got problems
							 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
							 * as fallback here
							 */
							*id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "host_id");
							do_insert = FALSE;
						} else {
							do_insert = TRUE;
						}

						dbi_result_free(idi->dbinfo.dbi_result);
						idi->dbinfo.dbi_result = NULL;
					}
				}
				else {
					dbi_result_free(idi->dbinfo.dbi_result);
					idi->dbinfo.dbi_result = NULL;
				}

				free(query);
			}
		} else {
			do_insert = IDO_TRUE;
		}

		if (do_insert) {
			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, alias, display_name, address, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_down, notify_on_unreachable, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_up, stalk_on_down, stalk_on_unreachable, flap_detection_enabled, flap_detection_on_up, flap_detection_on_down, flap_detection_on_unreachable, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_host, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt, vrml_image, statusmap_image, have_2d_coords, x_2d, y_2d, have_3d_coords, x_3d, y_3d, z_3d, address6) VALUES (%lu, %d, %lu, '%s', '%s', '%s', %lu, '%s', %lu, '%s', %lu, %lu, '%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %d, %d, %d, %lf, %lf, %lf, '%s')",
				 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS],
				 *(unsigned long *) data[0],     /* insert start */
				 *(int *) data[1],
				 *(unsigned long *) data[2],
				 *(char **) data[3],
				 *(char **) data[4],
				 *(char **) data[5],
				 *(unsigned long *) data[6],
				 (*(char **) data[7] == NULL) ? "" : *(char **) data[7],
				 *(unsigned long *) data[8],
				 (*(char **) data[9] == NULL) ? "" : *(char **) data[9],
				 *(unsigned long *) data[10],
				 *(unsigned long *) data[11],
				 *(char **) data[12],
				 *(double *) data[13],
				 *(double *) data[14],
				 *(int *) data[15],
				 *(double *) data[16],
				 *(double *) data[17],
				 *(int *) data[18],
				 *(int *) data[19],
				 *(int *) data[20],
				 *(int *) data[21],
				 *(int *) data[22],
				 *(int *) data[23],
				 *(int *) data[24],
				 *(int *) data[25],
				 *(int *) data[26],
				 *(int *) data[27],
				 *(int *) data[28],
				 *(int *) data[29],
				 *(double *) data[30],
				 *(double *) data[31],
				 *(int *) data[32],
				 *(int *) data[33],
				 *(int *) data[34],
				 *(int *) data[35],
				 *(int *) data[36],
				 *(int *) data[37],
				 *(int *) data[38],
				 *(int *) data[39],
				 *(int *) data[40],
				 *(int *) data[41],
				 *(int *) data[42],
				 *(char **) data[43],
				 *(char **) data[44],
				 *(char **) data[45],
				 *(char **) data[46],
				 *(char **) data[47],
				 *(char **) data[48],
				 *(char **) data[49],
				 *(int *) data[50],
				 *(int *) data[51],
				 *(int *) data[52],
				 *(int *) data[53],
				 *(double *) data[54],
				 *(double *) data[55],
				 *(double *) data[56],
				 *(char **) data[57]           /* insert end */
				);
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
				/* mysql doesn't use sequences */
				*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition(%lu) host_id\n", *id);
			}

			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET alias=E'%s', display_name=E'%s', address=E'%s', check_command_object_id=%lu, check_command_args=E'%s', eventhandler_command_object_id=%lu, eventhandler_command_args=E'%s', check_timeperiod_object_id=%lu, notification_timeperiod_object_id=%lu, failure_prediction_options=E'%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_down=%d, notify_on_unreachable=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_up=%d, stalk_on_down=%d, stalk_on_unreachable=%d, flap_detection_enabled=%d, flap_detection_on_up=%d, flap_detection_on_down=%d, flap_detection_on_unreachable=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_host=%d, failure_prediction_enabled=%d, notes=E'%s', notes_url=E'%s', action_url=E'%s', icon_image=E'%s', icon_image_alt=E'%s', vrml_image=E'%s', statusmap_image=E'%s', have_2d_coords=%d, x_2d=%d, y_2d=%d, have_3d_coords=%d, x_3d=%lf, y_3d=%lf, z_3d=%lf, address6=E'%s' WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS],
		                 *(char **) data[3],             /* update start */
		                 *(char **) data[4],
		                 *(char **) data[5],
		                 *(unsigned long *) data[6],
		                 (*(char **) data[7] == NULL) ? "" : *(char **) data[7],
		                 *(unsigned long *) data[8],
		                 (*(char **) data[9] == NULL) ? "" : *(char **) data[9],
		                 *(unsigned long *) data[10],
		                 *(unsigned long *) data[11],
		                 *(char **) data[12],
		                 *(double *) data[13],
		                 *(double *) data[14],
		                 *(int *) data[15],
		                 *(double *) data[16],
		                 *(double *) data[17],
		                 *(int *) data[18],
		                 *(int *) data[19],
		                 *(int *) data[20],
		                 *(int *) data[21],
		                 *(int *) data[22],
		                 *(int *) data[23],
		                 *(int *) data[24],
		                 *(int *) data[25],
		                 *(int *) data[26],
		                 *(int *) data[27],
		                 *(int *) data[28],
		                 *(int *) data[29],
		                 *(double *) data[30],
		                 *(double *) data[31],
		                 *(int *) data[32],
		                 *(int *) data[33],
		                 *(int *) data[34],
		                 *(int *) data[35],
		                 *(int *) data[36],
		                 *(int *) data[37],
		                 *(int *) data[38],
		                 *(int *) data[39],
		                 *(int *) data[40],
		                 *(int *) data[41],
		                 *(int *) data[42],
		                 *(char **) data[43],
		                 *(char **) data[44],
		                 *(char **) data[45],
		                 *(char **) data[46],
		                 *(char **) data[47],
		                 *(char **) data[48],
		                 *(char **) data[49],
		                 *(int *) data[50],
		                 *(int *) data[51],
		                 *(int *) data[52],
		                 *(int *) data[53],
		                 *(double *) data[54],
		                 *(double *) data[55],
		                 *(double *) data[56],
		                 *(char **) data[57],           /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[2]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, alias, display_name, address, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_down, notify_on_unreachable, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_up, stalk_on_down, stalk_on_unreachable, flap_detection_enabled, flap_detection_on_up, flap_detection_on_down, flap_detection_on_unreachable, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_host, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt, vrml_image, statusmap_image, have_2d_coords, x_2d, y_2d, have_3d_coords, x_3d, y_3d, z_3d, address6) VALUES (%lu, %d, %lu, E'%s', E'%s', E'%s', %lu, E'%s', %lu, E'%s', %lu, %lu, E'%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, E'%s', E'%s', E'%s', E'%s', E'%s', E'%s', E'%s', %d, %d, %d, %d, %lf, %lf, %lf, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(char **) data[3],
			                 *(char **) data[4],
			                 *(char **) data[5],
			                 *(unsigned long *) data[6],
			                 (*(char **) data[7] == NULL) ? "" : *(char **) data[7],
			                 *(unsigned long *) data[8],
			                 (*(char **) data[9] == NULL) ? "" : *(char **) data[9],
			                 *(unsigned long *) data[10],
			                 *(unsigned long *) data[11],
			                 *(char **) data[12],
			                 *(double *) data[13],
			                 *(double *) data[14],
			                 *(int *) data[15],
			                 *(double *) data[16],
			                 *(double *) data[17],
			                 *(int *) data[18],
			                 *(int *) data[19],
			                 *(int *) data[20],
			                 *(int *) data[21],
			                 *(int *) data[22],
			                 *(int *) data[23],
			                 *(int *) data[24],
			                 *(int *) data[25],
			                 *(int *) data[26],
			                 *(int *) data[27],
			                 *(int *) data[28],
			                 *(int *) data[29],
			                 *(double *) data[30],
			                 *(double *) data[31],
			                 *(int *) data[32],
			                 *(int *) data[33],
			                 *(int *) data[34],
			                 *(int *) data[35],
			                 *(int *) data[36],
			                 *(int *) data[37],
			                 *(int *) data[38],
			                 *(int *) data[39],
			                 *(int *) data[40],
			                 *(int *) data[41],
			                 *(int *) data[42],
			                 *(char **) data[43],
			                 *(char **) data[44],
			                 *(char **) data[45],
			                 *(char **) data[46],
			                 *(char **) data[47],
			                 *(char **) data[48],
			                 *(char **) data[49],
			                 *(int *) data[50],
			                 *(int *) data[51],
			                 *(int *) data[52],
			                 *(int *) data[53],
			                 *(double *) data[54],
			                 *(double *) data[55],
			                 *(double *) data[56],
			                 *(char **) data[57]           /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

		        if (result == IDO_OK) {
	                        /* depending on tableprefix/tablename a sequence will be used */
	                        if (asprintf(&buf, "%s_host_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS]) == -1)
        	                        buf = NULL;

	                        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
	                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition(%s=%lu) host_id\n", buf, *id);
        	                free(buf);
			}

                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
		}
                else { 
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

                        /* we actually did an update, select the host id */
                        asprintf(&query, "SELECT host_id FROM %s WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "host_id");
                                                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinitio(%lu) host_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);

                }
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (*(char **) data[3] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X4") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X4"), *(char **) data[3], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[5] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (*(char **) data[7] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X8") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit command_arg size #3324 */
		if ( strlen(*(char **)data[7])  > OCI_COMMAND_ARG_SIZE ) {
			(*(char **)data[7])[OCI_COMMAND_ARG_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_hostdefinition() check_command_args shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X8"), *(char **) data[7], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X9"), (uint *) data[8])) {
		return IDO_ERROR;
	}
	if (*(char **) data[9] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X10") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X10"), *(char **) data[9], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X11"), (uint *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X12"), (uint *) data[11])) {
		return IDO_ERROR;
	}
	if (*(char **) data[12] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X13") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X13"), *(char **) data[12], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X14"), (double *) data[13])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X15"), (double *) data[14])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X16"), (int *) data[15])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X17"), (double *) data[16])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X18"), (double *) data[17])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X19"), (int *) data[18])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X20"), (int *) data[19])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X21"), (int *) data[20])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X22"), (int *) data[21])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X23"), (int *) data[22])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X24"), (int *) data[23])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X25"), (int *) data[24])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X26"), (int *) data[25])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X27"), (int *) data[26])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X28"), (int *) data[27])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X29"), (int *) data[28])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X30"), (int *) data[29])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X31"), (double *) data[30])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X32"), (double *) data[31])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X33"), (int *) data[32])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X34"), (int *) data[33])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X35"), (int *) data[34])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X36"), (int *) data[35])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X37"), (int *) data[36])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X38"), (int *) data[37])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X39"), (int *) data[38])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X40"), (int *) data[39])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X41"), (int *) data[40])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X42"), (int *) data[41])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X43"), (int *) data[42])) {
		return IDO_ERROR;
	}
	if (*(char **) data[43] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X44") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X44"), *(char **) data[43], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[44] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X45") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X45"), *(char **) data[44], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[45] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X46") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X46"), *(char **) data[45], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[46] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X47") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X47"), *(char **) data[46], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[47] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X48") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X48"), *(char **) data[47], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[48] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X49") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X49"), *(char **) data[48], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[49] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X50") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X50"), *(char **) data[49], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X51"), (int *) data[50])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X52"), (int *) data[51])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X53"), (int *) data[52])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X54"), (int *) data[53])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X55"), (double *) data[54])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X56"), (double *) data[55])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X57"), (double *) data[56])) {
		return IDO_ERROR;
	}
	if (*(char **) data[57] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostdefinition_definition, ":X58") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostdefinition_definition, MT(":X58"), *(char **) data[57], 0)) {
			return IDO_ERROR;
		}
	}


	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_hostdefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_hostdefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	if (asprintf(&seq_name, "seq_hosts") == -1)
		seq_name = NULL;

	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostdefinition(%lu) \n", *id);
	free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_definition_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_hostdefinition_parenthosts_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long host_parenthost_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_parenthosts_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE host_id=%lu AND parent_host_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTPARENTHOSTS],
                                 *(unsigned long *) data[0],     /* update start/end */
                                 *(unsigned long *) data[1],            /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;

                        asprintf(&query, "SELECT host_parenthost_id FROM %s WHERE host_id=%lu AND parent_host_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTPARENTHOSTS],
                                 *(unsigned long *) data[1],            /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                host_parenthost_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "host_parenthost_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, host_id, parent_host_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTPARENTHOSTS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]     /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE host_id=%lu AND parent_host_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTPARENTHOSTS],
		                 *(unsigned long *) data[0],     /* update start/end */
		                 *(unsigned long *) data[1],    	/* unique constraint start */
		                 *(unsigned long *) data[2]	/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, host_id, parent_host_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTPARENTHOSTS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_parenthosts, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_parenthosts, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_parenthosts, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_hostdefinition_parenthosts)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_hostdefinition_parenthosts() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_parenthosts_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_hostdefinition_contactgroups_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long host_contactgroup_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_contactgroups_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE host_id=%lu AND contactgroup_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTGROUPS],
                                 *(unsigned long *) data[0],     /* update start/end */
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */ 
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT host_contactgroup_id FROM %s WHERE host_id=%lu AND contactgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTGROUPS],
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                host_contactgroup_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "host_contactgroup_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, host_id, contactgroup_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTGROUPS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]     /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE host_id=%lu AND contactgroup_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTGROUPS],
		                 *(unsigned long *) data[0],     /* update start/end */
		                 *(unsigned long *) data[1],     /* unique constraint start */
		                 *(unsigned long *) data[2]     /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, host_id, contactgroup_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTCONTACTGROUPS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_contactgroups, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_contactgroups, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdefinition_contactgroups, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_hostdefinition_contactgroups)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_hostdefinition_contactgroups() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_contactgroups_add() end\n");

	return result;
}


/************************************/
/* HOSTGROUPDEFINITION              */
/************************************/

int ido2db_query_insert_or_update_hostgroupdefinition_definition_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostgroupdefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET config_type=%d, alias='%s' WHERE instance_id=%lu AND hostgroup_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS],
                                 *(int *) data[1],              /* update start */
                                 *(char **) data[3],             /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT hostgroup_id FROM %s WHERE instance_id=%lu AND hostgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                /* this condition should never happen, as libdbi UPDATE and affected rows
                                                 * should take care of it. it seems that newer mysql versions got problems
                                                 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
                                                 * as fallback here
                                                 */
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostgroup_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, hostgroup_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(char **) data[3]             /* insert end */
                                        );
                	        /* send query to db */
                        	result = ido2db_db_query(idi, query2);
	                        free(query2);

			        if (result == IDO_OK) {
		                        /* mysql doesn't use sequences */
		                        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition(%lu) hostgroup_id\n", *id);
				}

                		dbi_result_free(idi->dbinfo.dbi_result);
	                	idi->dbinfo.dbi_result = NULL;
			}
                }
		else {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;
		
			/* we hit an update, fetch the id */
                        asprintf(&query, "SELECT hostgroup_id FROM %s WHERE instance_id=%lu AND hostgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostgroup_id");
			                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition(%lu) hostgroup_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                		dbi_result_free(idi->dbinfo.dbi_result);
	                	idi->dbinfo.dbi_result = NULL;
			}
                        free(query);
		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET config_type=%d, alias=E'%s' WHERE instance_id=%lu AND hostgroup_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS],
		                 *(int *) data[1],		/* update start */
		                 *(char **) data[3],             /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(unsigned long *) data[2]	/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, hostgroup_object_id, alias) VALUES (%lu, %d, %lu, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(char **) data[3]             /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf, "%s_hostgroup_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS]) == -1)
                	                buf = NULL;

	                        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
        	                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition(%s=%lu) hostgroup_id\n", buf, *id);
                	        free(buf);
			}

                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
		}
                else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

                        /* we hit an update, fetch the id */
                        asprintf(&query, "SELECT hostgroup_id FROM %s WHERE instance_id=%lu AND hostgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostgroup_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition(%lu) hostgroup_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);
                }
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostgroupdefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostgroupdefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostgroupdefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (*(char **) data[3] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_hostgroupdefinition_definition, ":X4") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_hostgroupdefinition_definition, MT(":X4"), *(char **) data[3], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_hostgroupdefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_hostgroupdefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	if (asprintf(&seq_name, "seq_hostgroups") == -1)
		seq_name = NULL;

	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostgroupdefinition(%lu) hostgroup_id\n", *id);
	free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostgroupdefinition_definition_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_hostgroupdefinition_hostgroupmembers_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long hostgroup_member_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostgroupdefinition_hostgroupmembers_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE hostgroup_id=%lu AND host_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPMEMBERS],
                                 *(unsigned long *) data[0],     /* update start/end */
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]      /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT hostgroup_member_id FROM %s WHERE hostgroup_id=%lu AND host_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPMEMBERS],
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                hostgroup_member_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostgroup_member_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, hostgroup_id, host_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPMEMBERS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]     /* insert end */
                                        );
                	        /* send query to db */
                        	result = ido2db_db_query(idi, query2);
	                        free(query2);
			}
                }
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE hostgroup_id=%lu AND host_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPMEMBERS],
		                 *(unsigned long *) data[0],     /* update start/end */
		                 *(unsigned long *) data[1],     /* unique constraint start */
		                 *(unsigned long *) data[2]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, hostgroup_id, host_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTGROUPMEMBERS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */
	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_hostgroupdefinition_hostgroupmembers)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_hostgroupdefinition_hostgroupmembers() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostgroupdefinition_hostgroupmembers_add() end\n");

	return result;
}


/************************************/
/* SERVICEDEFINITION                */
/************************************/

int ido2db_query_insert_or_update_servicedefinition_definition_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
        int do_insert = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
		if (!idi->tables_cleared) {
			asprintf(&query1, "UPDATE %s SET host_object_id=%lu, display_name='%s', check_command_object_id=%lu, check_command_args='%s', eventhandler_command_object_id=%lu, eventhandler_command_args='%s', check_timeperiod_object_id=%lu, notification_timeperiod_object_id=%lu, failure_prediction_options='%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_warning=%d, notify_on_unknown=%d, notify_on_critical=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_ok=%d, stalk_on_warning=%d, stalk_on_unknown=%d, stalk_on_critical=%d, is_volatile=%d, flap_detection_enabled=%d, flap_detection_on_ok=%d, flap_detection_on_warning=%d, flap_detection_on_unknown=%d, flap_detection_on_critical=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_service=%d, failure_prediction_enabled=%d, notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s' WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu",
					 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES],
					 *(unsigned long *) data[2],    /* update start */
					 *(char **) data[4],
					 *(unsigned long *) data[5],
					 (*(char **) data[6] == NULL) ? "" : *(char **) data[6],
					 *(unsigned long *) data[7],
					 (*(char **) data[8] == NULL) ? "" : *(char **) data[8],
					 *(unsigned long *) data[9],
					 *(unsigned long *) data[10],
					 *(char **) data[11],
					 *(double *) data[12],
					 *(double *) data[13],
					 *(int *) data[14],
					 *(double *) data[15],
					 *(double *) data[16],
					 *(int *) data[17],
					 *(int *) data[18],
					 *(int *) data[19],
					 *(int *) data[20],
					 *(int *) data[21],
					 *(int *) data[22],
					 *(int *) data[23],
					 *(int *) data[24],
					 *(int *) data[25],
					 *(int *) data[26],
					 *(int *) data[27],
					 *(int *) data[28],
					 *(int *) data[29],
					 *(int *) data[30],
					 *(int *) data[31],
					 *(int *) data[32],
					 *(double *) data[33],
					 *(double *) data[34],
					 *(int *) data[35],
					 *(int *) data[36],
					 *(int *) data[37],
					 *(int *) data[38],
					 *(int *) data[39],
					 *(int *) data[40],
					 *(int *) data[41],
					 *(int *) data[42],
					 *(int *) data[43],
					 *(int *) data[44],
					 *(int *) data[45],
					 *(char **) data[46],
					 *(char **) data[47],
					 *(char **) data[48],
					 *(char **) data[49],
					 *(char **) data[50],            /* update end */
					 *(unsigned long *) data[0],     /* unique constraint start */
					 *(int *) data[1],
					 *(unsigned long *) data[3]      /* unique constraint end */
					);
			/* send query to db */
			result = ido2db_db_query(idi, query1);
			free(query1);

			/* check result if update was ok */
			if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) > 0) {
				dbi_result_free(idi->dbinfo.dbi_result);
				idi->dbinfo.dbi_result = NULL;

				/* we hit an update, fetch the id */
				asprintf(&query, "SELECT service_id FROM %s WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu",
					ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES],
					 *(unsigned long *) data[0],     /* unique constraint start */
					 *(int *) data[1],
					 *(unsigned long *) data[3]      /* unique constraint end */
					);

				/* send query to db */
				if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
					if (idi->dbinfo.dbi_result != NULL) {
						if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
							*id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "service_id");
							ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition(%lu) service_id\n", *id);
						}

						dbi_result_free(idi->dbinfo.dbi_result);
						idi->dbinfo.dbi_result = NULL;
					}
				}
				else {
					dbi_result_free(idi->dbinfo.dbi_result);
					idi->dbinfo.dbi_result = NULL;
				}
				free(query);
			} else {
				dbi_result_free(idi->dbinfo.dbi_result);
				idi->dbinfo.dbi_result = NULL;

				asprintf(&query, "SELECT service_id FROM %s WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu",
					ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES],
					 *(unsigned long *) data[0],     /* unique constraint start */
					 *(int *) data[1],
					 *(unsigned long *) data[3]      /* unique constraint end */
					);

				/* send query to db */
				if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
					if (idi->dbinfo.dbi_result != NULL) {
						if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
							/* this condition should never happen, as libdbi UPDATE and affected rows
							 * should take care of it. it seems that newer mysql versions got problems
							 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
							 * as fallback here
							 */
							*id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "service_id");
							do_insert = FALSE;
						} else {
							do_insert = TRUE;
						}

						dbi_result_free(idi->dbinfo.dbi_result);
						idi->dbinfo.dbi_result = NULL;
					}
				}
				else {
					dbi_result_free(idi->dbinfo.dbi_result);
					idi->dbinfo.dbi_result = NULL;
				}
				free(query);
			}
		} else {
			do_insert = IDO_TRUE;
		}

		if (do_insert) {
			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, service_object_id, display_name, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_warning, notify_on_unknown, notify_on_critical, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_ok, stalk_on_warning, stalk_on_unknown, stalk_on_critical, is_volatile, flap_detection_enabled, flap_detection_on_ok, flap_detection_on_warning, flap_detection_on_unknown, flap_detection_on_critical, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_service, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt) VALUES (%lu, %d, %lu, %lu, '%s', %lu, '%s', %lu, '%s', %lu, %lu, '%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', '%s')",
				 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES],
				 *(unsigned long *) data[0],     /* insert start */
				 *(int *) data[1],
				 *(unsigned long *) data[2],
				 *(unsigned long *) data[3],
				 *(char **) data[4],
				 *(unsigned long *) data[5],
				 (*(char **) data[6] == NULL) ? "" : *(char **) data[6],
				 *(unsigned long *) data[7],
				 (*(char **) data[8] == NULL) ? "" : *(char **) data[8],
				 *(unsigned long *) data[9],
				 *(unsigned long *) data[10],
				 *(char **) data[11],
				 *(double *) data[12],
				 *(double *) data[13],
				 *(int *) data[14],
				 *(double *) data[15],
				 *(double *) data[16],
				 *(int *) data[17],
				 *(int *) data[18],
				 *(int *) data[19],
				 *(int *) data[20],
				 *(int *) data[21],
				 *(int *) data[22],
				 *(int *) data[23],
				 *(int *) data[24],
				 *(int *) data[25],
				 *(int *) data[26],
				 *(int *) data[27],
				 *(int *) data[28],
				 *(int *) data[29],
				 *(int *) data[30],
				 *(int *) data[31],
				 *(int *) data[32],
				 *(double *) data[33],
				 *(double *) data[34],
				 *(int *) data[35],
				 *(int *) data[36],
				 *(int *) data[37],
				 *(int *) data[38],
				 *(int *) data[39],
				 *(int *) data[40],
				 *(int *) data[41],
				 *(int *) data[42],
				 *(int *) data[43],
				 *(int *) data[44],
				 *(int *) data[45],
				 *(char **) data[46],
				 *(char **) data[47],
				 *(char **) data[48],
				 *(char **) data[49],
				 *(char **) data[50]            /* insert end */
				);
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
				/* mysql doesn't use sequences */
				*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition(%lu) service_id\n", *id);
			}

			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET host_object_id=%lu, display_name=E'%s', check_command_object_id=%lu, check_command_args=E'%s', eventhandler_command_object_id=%lu, eventhandler_command_args=E'%s', check_timeperiod_object_id=%lu, notification_timeperiod_object_id=%lu, failure_prediction_options=E'%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_warning=%d, notify_on_unknown=%d, notify_on_critical=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_ok=%d, stalk_on_warning=%d, stalk_on_unknown=%d, stalk_on_critical=%d, is_volatile=%d, flap_detection_enabled=%d, flap_detection_on_ok=%d, flap_detection_on_warning=%d, flap_detection_on_unknown=%d, flap_detection_on_critical=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_service=%d, failure_prediction_enabled=%d, notes=E'%s', notes_url=E'%s', action_url=E'%s', icon_image=E'%s', icon_image_alt=E'%s' WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES],
		                 *(unsigned long *) data[2], 	/* update start */
		                 *(char **) data[4],
		                 *(unsigned long *) data[5],
		                 (*(char **) data[6] == NULL) ? "" : *(char **) data[6],
		                 *(unsigned long *) data[7],
		                 (*(char **) data[8] == NULL) ? "" : *(char **) data[8],
		                 *(unsigned long *) data[9],
		                 *(unsigned long *) data[10],
		                 *(char **) data[11],
		                 *(double *) data[12],
		                 *(double *) data[13],
		                 *(int *) data[14],
		                 *(double *) data[15],
		                 *(double *) data[16],
		                 *(int *) data[17],
		                 *(int *) data[18],
		                 *(int *) data[19],
		                 *(int *) data[20],
		                 *(int *) data[21],
		                 *(int *) data[22],
		                 *(int *) data[23],
		                 *(int *) data[24],
		                 *(int *) data[25],
		                 *(int *) data[26],
		                 *(int *) data[27],
		                 *(int *) data[28],
		                 *(int *) data[29],
		                 *(int *) data[30],
		                 *(int *) data[31],
		                 *(int *) data[32],
		                 *(double *) data[33],
		                 *(double *) data[34],
		                 *(int *) data[35],
		                 *(int *) data[36],
		                 *(int *) data[37],
		                 *(int *) data[38],
		                 *(int *) data[39],
		                 *(int *) data[40],
		                 *(int *) data[41],
		                 *(int *) data[42],
		                 *(int *) data[43],
		                 *(int *) data[44],
		                 *(int *) data[45],
		                 *(char **) data[46],
		                 *(char **) data[47],
		                 *(char **) data[48],
		                 *(char **) data[49],
		                 *(char **) data[50],            /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[3]      /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, service_object_id, display_name, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_warning, notify_on_unknown, notify_on_critical, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_ok, stalk_on_warning, stalk_on_unknown, stalk_on_critical, is_volatile, flap_detection_enabled, flap_detection_on_ok, flap_detection_on_warning, flap_detection_on_unknown, flap_detection_on_critical, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_service, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt) VALUES (%lu, %d, %lu, %lu, E'%s', %lu, E'%s', %lu, E'%s', %lu, %lu, E'%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, E'%s', E'%s', E'%s', E'%s', E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(unsigned long *) data[3],
			                 *(char **) data[4],
			                 *(unsigned long *) data[5],
			                 (*(char **) data[6] == NULL) ? "" : *(char **) data[6],
			                 *(unsigned long *) data[7],
			                 (*(char **) data[8] == NULL) ? "" : *(char **) data[8],
			                 *(unsigned long *) data[9],
			                 *(unsigned long *) data[10],
			                 *(char **) data[11],
			                 *(double *) data[12],
			                 *(double *) data[13],
			                 *(int *) data[14],
			                 *(double *) data[15],
			                 *(double *) data[16],
			                 *(int *) data[17],
			                 *(int *) data[18],
			                 *(int *) data[19],
			                 *(int *) data[20],
			                 *(int *) data[21],
			                 *(int *) data[22],
			                 *(int *) data[23],
			                 *(int *) data[24],
			                 *(int *) data[25],
			                 *(int *) data[26],
			                 *(int *) data[27],
			                 *(int *) data[28],
			                 *(int *) data[29],
			                 *(int *) data[30],
			                 *(int *) data[31],
			                 *(int *) data[32],
			                 *(double *) data[33],
			                 *(double *) data[34],
			                 *(int *) data[35],
			                 *(int *) data[36],
			                 *(int *) data[37],
			                 *(int *) data[38],
			                 *(int *) data[39],
			                 *(int *) data[40],
			                 *(int *) data[41],
			                 *(int *) data[42],
			                 *(int *) data[43],
			                 *(int *) data[44],
			                 *(int *) data[45],
			                 *(char **) data[46],
			                 *(char **) data[47],
			                 *(char **) data[48],
			                 *(char **) data[49],
			                 *(char **) data[50]            /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf, "%s_service_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES]) == -1)
                	                buf = NULL;

                        	*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
	                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition(%s=%lu) service_id\n", buf, *id);
        	                free(buf);
			}

                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
		}
                else { 
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

                        /* we hit an update, fetch the id */
                        asprintf(&query, "SELECT service_id FROM %s WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICES],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[3]      /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "service_id");
                                                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition(%lu) service_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);
                }

		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicedefinition_definition, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X6"), (uint *) data[5])) {
		return IDO_ERROR;
	}
	if (*(char **) data[6] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicedefinition_definition, ":X7") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit command_arg size #3324 */
		if ( strlen(*(char **)data[6])  > OCI_COMMAND_ARG_SIZE ) {
				(*(char **)data[6])[OCI_COMMAND_ARG_SIZE] = '\0';
				ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_servicedefinition() check_command_args shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X7"), *(char **) data[6], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}
	if (*(char **) data[8] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicedefinition_definition, ":X9") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit command_arg size #3324 */
		if ( strlen(*(char **)data[8])  > OCI_COMMAND_ARG_SIZE ) {
			(*(char **)data[8])[OCI_COMMAND_ARG_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_servicedefinition_definition() eventhandler_command_args shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X9"), *(char **) data[8], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X10"), (uint *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X11"), (uint *) data[10])) {
		return IDO_ERROR;
	}
	if (*(char **) data[11] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicedefinition_definition, ":X12") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X12"), *(char **) data[11], 0)) {
			return IDO_ERROR;
		}
	}


	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X13"), (double *) data[12])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X14"), (double *) data[13])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X15"), (uint *) data[14])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X16"), (double *) data[15])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X17"), (double *) data[16])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X18"), (int *) data[17])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X19"), (int *) data[18])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X20"), (int *) data[19])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X21"), (int *) data[20])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X22"), (int *) data[21])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X23"), (int *) data[22])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X24"), (int *) data[23])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X25"), (int *) data[24])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X26"), (int *) data[25])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X27"), (int *) data[26])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X28"), (int *) data[27])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X29"), (int *) data[28])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X30"), (int *) data[29])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X31"), (int *) data[30])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X32"), (int *) data[31])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X33"), (int *) data[32])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X34"), (double *) data[33])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X35"), (double *) data[34])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X36"), (int *) data[35])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X37"), (int *) data[36])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X38"), (int *) data[37])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X39"), (int *) data[38])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X40"), (int *) data[39])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X41"), (int *) data[40])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X42"), (int *) data[41])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X43"), (int *) data[42])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X44"), (int *) data[43])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X45"), (int *) data[44])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X46"), (int *) data[45])) {
		return IDO_ERROR;
	}
	if (*(char **) data[46] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicedefinition_definition, ":X47") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X47"), *(char **) data[46], 0)) {
			return IDO_ERROR;
		}
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_definition_add() here5\n");
	if (*(char **) data[47] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicedefinition_definition, ":X48") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X48"), *(char **) data[47], 0)) {
			return IDO_ERROR;
		}
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_definition_add() here6\n");
	if (*(char **) data[48] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicedefinition_definition, ":X49") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X49"), *(char **) data[48], 0)) {
			return IDO_ERROR;
		}
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_definition_add() here7\n");
	if (*(char **) data[49] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicedefinition_definition, ":X50") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X50"), *(char **) data[49], 0)) {
			return IDO_ERROR;
		}
	}
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_definition_add() here8\n");
	if (*(char **) data[50] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicedefinition_definition, ":X51") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicedefinition_definition, MT(":X51"), *(char **) data[50], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_servicedefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_servicedefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	if (asprintf(&seq_name, "seq_services") == -1)
		seq_name = NULL;

	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicedefinition(%lu) service_id\n", *id);
	free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_definition_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_servicedefinition_contactgroups_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long service_contactgroup_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_contactgroups_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE service_id=%lu AND contactgroup_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTGROUPS],
                                 *(unsigned long *) data[0],     /* update start/end */
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT service_contactgroup_id FROM %s WHERE service_id=%lu AND contactgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTGROUPS],
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                service_contactgroup_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "service_contactgroup_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, service_id, contactgroup_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTGROUPS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]     /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE service_id=%lu AND contactgroup_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTGROUPS],
		                 *(unsigned long *) data[0],     /* update start/end */
		                 *(unsigned long *) data[1],     /* unique constraint start */
		                 *(unsigned long *) data[2]     /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, service_id, contactgroup_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICECONTACTGROUPS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_contactgroups, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_contactgroups, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedefinition_contactgroups, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_servicedefinition_contactgroups)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_servicedefinition_contactgroups() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_contactgroups_add() end\n");

	return result;
}


/************************************/
/* SERVICEGROUPDEFINITION           */
/************************************/

int ido2db_query_insert_or_update_servicegroupdefinition_definition_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicegroupdefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET alias='%s' WHERE instance_id=%lu AND config_type=%d AND servicegroup_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS],
                                 *(char **) data[3],            /* update start/end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT servicegroup_id FROM %s WHERE instance_id=%lu AND config_type=%d AND servicegroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                /* this condition should never happen, as libdbi UPDATE and affected rows
                                                 * should take care of it. it seems that newer mysql versions got problems
                                                 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
                                                 * as fallback here
                                                 */
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "servicegroup_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, servicegroup_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(char **) data[3]             /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);

				if (result == IDO_OK) {
		                        /* mysql doesn't use sequences */
                		        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition(%lu) group_id\n", *id);
				}

                        	dbi_result_free(idi->dbinfo.dbi_result);
	                        idi->dbinfo.dbi_result = NULL;
			}
                }
		else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

			/* we hit an update, fetch the id */
                        asprintf(&query, "SELECT servicegroup_id FROM %s WHERE instance_id=%lu AND config_type=%d AND servicegroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "servicegroup_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition(%lu) group_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                        	dbi_result_free(idi->dbinfo.dbi_result);
	                        idi->dbinfo.dbi_result = NULL;
			}
                        free(query);
		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET alias=E'%s' WHERE instance_id=%lu AND config_type=%d AND servicegroup_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS],
		                 *(char **) data[3],		/* update start/end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[2]	/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, servicegroup_object_id, alias) VALUES (%lu, %d, %lu, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(char **) data[3]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf, "%s_servicegroup_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS]) == -1)
                	                buf = NULL;

                        	*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition(%s=%lu) group_id\n", buf, *id);
                        	free(buf);
			}

                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
		}
                else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

                        /* we hit an update, fetch the id */
                        asprintf(&query, "SELECT servicegroup_id FROM %s WHERE instance_id=%lu AND config_type=%d AND servicegroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                                 
                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "servicegroup_id");
                                                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition(%lu) group_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);
                }
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicegroupdefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicegroupdefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicegroupdefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (*(char **) data[3] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_servicegroupdefinition_definition, ":X3") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_servicegroupdefinition_definition, MT(":X4"), *(char **) data[3], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_servicegroupdefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_servicegroupdefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	if (asprintf(&seq_name, "seq_servicegroups") == -1)
		seq_name = NULL;

	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_servicegroupdefinition(%lu) group_id\n", *id);
	free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicegroupdefinition_definition_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_servicegroupdefinition_members_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long servicegroup_member_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicegroupdefinition_members_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE servicegroup_id=%lu AND service_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPMEMBERS],
                                 *(unsigned long *) data[0],     /* update start/end */
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT servicegroup_member_id FROM %s WHERE servicegroup_id=%lu AND service_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPMEMBERS],
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                servicegroup_member_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "servicegroup_member_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, servicegroup_id, service_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPMEMBERS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]     /* insert end */

                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
	                        free(query2);
			}
                }
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE servicegroup_id=%lu AND service_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPMEMBERS],
		                 *(unsigned long *) data[0],     /* update start/end */
		                 *(unsigned long *) data[1],     /* unique constraint start */
		                 *(unsigned long *) data[2]     /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, servicegroup_id, service_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEGROUPMEMBERS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]     /* insert end */

			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicegroupdefinition_members, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicegroupdefinition_members, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicegroupdefinition_members, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_servicegroupdefinition_members)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_servicegroupdefinition_members() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicegroupdefinition_members_add() end\n");

	return result;
}


/************************************/
/* HOSTDEPENDENCIES                 */
/************************************/

int ido2db_query_insert_or_update_hostdependencydefinition_definition_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long hostdependency_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdependencydefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET timeperiod_object_id=%lu WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND dependent_host_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_up=%d AND fail_on_down=%d AND fail_on_unreachable=%d",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTDEPENDENCIES],
                                 *(unsigned long *) data[6],    /* update start/end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5],
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9]               /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT hostdependency_id FROM %s WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND dependent_host_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_up=%d AND fail_on_down=%d AND fail_on_unreachable=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTDEPENDENCIES],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5],
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9]               /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                hostdependency_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostdependency_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, dependent_host_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_up, fail_on_down, fail_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTDEPENDENCIES],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(unsigned long *) data[3],
                                         *(int *) data[4],
                                         *(int *) data[5],
                                         *(unsigned long *) data[6],
                                         *(int *) data[7],
                                         *(int *) data[8],
                                         *(int *) data[9]               /* insert end */
                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
                	        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET timeperiod_object_id=%lu WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND dependent_host_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_up=%d AND fail_on_down=%d AND fail_on_unreachable=%d",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTDEPENDENCIES],
		                 *(unsigned long *) data[6],	/* update start/end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[2],
		                 *(unsigned long *) data[3],
		                 *(int *) data[4],
		                 *(int *) data[5],
		                 *(int *) data[7],
		                 *(int *) data[8],
		                 *(int *) data[9]               /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, dependent_host_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_up, fail_on_down, fail_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTDEPENDENCIES],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(unsigned long *) data[3],
			                 *(int *) data[4],
			                 *(int *) data[5],
			                 *(unsigned long *) data[6],
			                 *(int *) data[7],
			                 *(int *) data[8],
			                 *(int *) data[9]               /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X6"), (int *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostdependencydefinition_definition, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_hostdependencydefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_hostdependencydefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdependencydefinition_definition_add() end\n");

	return result;
}


/************************************/
/* SERVICEDEPENDENCYDEFINITION      */
/************************************/

int ido2db_query_insert_or_update_servicedependencydefinition_definition_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long servicedependency_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedependencydefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET timeperiod_object_id=%lu WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND dependent_service_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_ok=%d AND fail_on_warning=%d AND fail_on_unknown=%d AND fail_on_critical=%d",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEDEPENDENCIES],
                                 *(unsigned long *) data[6],    /* update start/end*/
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5],
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9],
                                 *(int *) data[10]              /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT servicedependency_id FROM %s WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND dependent_service_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_ok=%d AND fail_on_warning=%d AND fail_on_unknown=%d AND fail_on_critical=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEDEPENDENCIES],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5],
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9],
                                 *(int *) data[10]              /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                servicedependency_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "servicedependency_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, service_object_id, dependent_service_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_ok, fail_on_warning, fail_on_unknown, fail_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d, %d)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEDEPENDENCIES],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(unsigned long *) data[3],
                                         *(int *) data[4],
                                         *(int *) data[5],
                                         *(unsigned long *) data[6],
                                         *(int *) data[7],
                                         *(int *) data[8],
                                         *(int *) data[9],
                                         *(int *) data[10]              /* insert end */
                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
                	        free(query2);  
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET timeperiod_object_id=%lu WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND dependent_service_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_ok=%d AND fail_on_warning=%d AND fail_on_unknown=%d AND fail_on_critical=%d",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEDEPENDENCIES],
		                 *(unsigned long *) data[6],	/* update start/end*/
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[2],
		                 *(unsigned long *) data[3],
		                 *(int *) data[4],
		                 *(int *) data[5],
		                 *(int *) data[7],
		                 *(int *) data[8],
		                 *(int *) data[9],
		                 *(int *) data[10]              /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, service_object_id, dependent_service_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_ok, fail_on_warning, fail_on_unknown, fail_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d, %d)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEDEPENDENCIES],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(unsigned long *) data[3],
			                 *(int *) data[4],
			                 *(int *) data[5],
			                 *(unsigned long *) data[6],
			                 *(int *) data[7],
			                 *(int *) data[8],
			                 *(int *) data[9],
			                 *(int *) data[10]              /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X6"), (int *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_servicedependencydefinition_definition, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_servicedependencydefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_servicedependencydefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedependencydefinition_definition_add() end\n");

	return result;
}


/************************************/
/* HOSTESCALATIONDEFINITION         */
/************************************/

int ido2db_query_insert_or_update_hostescalationdefinition_definition_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET notification_interval=%lf, escalate_on_recovery=%d, escalate_on_down=%d, escalate_on_unreachable=%d WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS],
                                 *(double *) data[6],           /* update start */
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9],              /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5]               /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT hostescalation_id FROM %s WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5]               /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                /* this condition should never happen, as libdbi UPDATE and affected rows
                                                 * should take care of it. it seems that newer mysql versions got problems
                                                 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
                                                 * as fallback here
                                                 */
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostescalation_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_down, escalate_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(unsigned long *) data[3],
                                         *(int *) data[4],
                                         *(int *) data[5],
                                         *(double *) data[6],
                                         *(int *) data[7],
                                         *(int *) data[8],
                                         *(int *) data[9]               /* insert end */
                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
                	        free(query2);

				if (result == IDO_OK) {
		                        /* mysql doesn't use sequences */
		                        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition(%lu) escalation_id\n", *id);
				}

	                        dbi_result_free(idi->dbinfo.dbi_result);
        	                idi->dbinfo.dbi_result = NULL;
			}
                }
		else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

			/* we hit an update, fetch the id */
                        asprintf(&query, "SELECT hostescalation_id FROM %s WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5]               /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostescalation_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition(%lu) escalation_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                        	dbi_result_free(idi->dbinfo.dbi_result);
	                        idi->dbinfo.dbi_result = NULL;
			}
                        free(query);
		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET notification_interval=%lf, escalate_on_recovery=%d, escalate_on_down=%d, escalate_on_unreachable=%d WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS],
		                 *(double *) data[6],		/* update start */
		                 *(int *) data[7],
		                 *(int *) data[8],
		                 *(int *) data[9],		/* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[2],
		                 *(unsigned long *) data[3],
		                 *(int *) data[4],
		                 *(int *) data[5]		/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_down, escalate_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(unsigned long *) data[3],
			                 *(int *) data[4],
			                 *(int *) data[5],
			                 *(double *) data[6],
			                 *(int *) data[7],
			                 *(int *) data[8],
			                 *(int *) data[9]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf, "%s_hostescalation_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS]) == -1)
                	                buf = NULL;

                        	*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
	                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition(%s=%lu) escalation_id\n", buf, *id);
        	                free(buf);
			}

                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
		}
                else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

                        /* we hit an update, fetch the id */
                        asprintf(&query, "SELECT hostescalation_id FROM %s WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5]               /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostescalation_id");
                                                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition(%lu) escalation_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);
                }
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X6"), (int *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X7"), (double *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_hostescalationdefinition_definition, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_hostescalationdefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_hostescalationdefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	if (asprintf(&seq_name, "seq_hostescalations") == -1)
		seq_name = NULL;

	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_hostescalationdefinition(%lu) escalation_id\n", *id);
	free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_definition_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_hostescalationdefinition_contactgroups_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long hostescalation_contactgroup_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_contactgroups_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE hostescalation_id=%lu AND contactgroup_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS],
                                 *(unsigned long *) data[0],     /* update start/end */
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT hostescalation_contactgroup_id FROM %s WHERE hostescalation_id=%lu AND contactgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS],
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                hostescalation_contactgroup_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostescalation_contactgroup_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, hostescalation_id, contactgroup_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]     /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE hostescalation_id=%lu AND contactgroup_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS],
		                 *(unsigned long *) data[0],     /* update start/end */
		                 *(unsigned long *) data[1],     /* unique constraint start */
		                 *(unsigned long *) data[2]     /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, hostescalation_id, contactgroup_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostescalationdefinition_contactgroups, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostescalationdefinition_contactgroups, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostescalationdefinition_contactgroups, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_hostescalationdefinition_contactgroups)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_hostescalationdefinition_contactgroups() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_contactgroups_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_hostescalationdefinition_contacts_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long hostescalation_contact_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_contacts_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu, hostescalation_id=%lu, contact_object_id=%lu WHERE instance_id=%lu AND hostescalation_id=%lu AND contact_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTS],
                                 *(unsigned long *) data[0],     /* update start */
                                 *(unsigned long *) data[1],
                                 *(unsigned long *) data[2],     /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;

                        asprintf(&query, "SELECT hostescalation_contact_id FROM %s WHERE instance_id=%lu AND hostescalation_id=%lu AND contact_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                hostescalation_contact_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "hostescalation_contact_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, hostescalation_id, contact_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]     /* insert end */
                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
                	        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu, hostescalation_id=%lu, contact_object_id=%lu WHERE instance_id=%lu AND hostescalation_id=%lu AND contact_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTS],
		                 *(unsigned long *) data[0],     /* update start */
		                 *(unsigned long *) data[1],
		                 *(unsigned long *) data[2],     /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(unsigned long *) data[1],
		                 *(unsigned long *) data[2]     /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, hostescalation_id, contact_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_HOSTESCALATIONCONTACTS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostescalationdefinition_contacts, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostescalationdefinition_contacts, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_hostescalationdefinition_contacts, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_hostescalationdefinition_contacts)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_hostescalationdefinition_contacts() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_contacts_add() end\n");

	return result;
}


/************************************/
/* SERVICEESCALATIONDEFINITION      */
/************************************/

int ido2db_query_insert_or_update_serviceescalationdefinition_definition_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET notification_interval=%lf, escalate_on_recovery=%d, escalate_on_warning=%d, escalate_on_unknown=%d, escalate_on_critical=%d WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS],
                                 *(double *) data[6],           /* update start */
                                 *(int *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9],
                                 *(int *) data[10],             /* update end */
                                 *(unsigned long *) data[0],            /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5]               /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;

                        asprintf(&query, "SELECT serviceescalation_id FROM %s WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS],
                                 *(unsigned long *) data[0],            /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5]               /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                /* this condition should never happen, as libdbi UPDATE and affected rows
                                                 * should take care of it. it seems that newer mysql versions got problems
                                                 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
                                                 * as fallback here
                                                 */
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "serviceescalation_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, service_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_warning, escalate_on_unknown, escalate_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d, %d)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(unsigned long *) data[3],
                                         *(int *) data[4],
                                         *(int *) data[5],
                                         *(double *) data[6],
                                         *(int *) data[7],
                                         *(int *) data[8],
                                         *(int *) data[9],
                                         *(int *) data[10]              /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);

				if (result == IDO_OK) {
		                        /* mysql doesn't use sequences */
                		       	*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_serviceescalationdefinition(%lu) escalation_id\n", *id);
				}

	                        dbi_result_free(idi->dbinfo.dbi_result);
        	                idi->dbinfo.dbi_result = NULL;
			}
                }
		else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

			/* we hit an update, fetch the id */
                        asprintf(&query, "SELECT serviceescalation_id FROM %s WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS],
                                 *(unsigned long *) data[0],            /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5]               /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "serviceescalation_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_serviceescalationdefinition(%lu) escalation_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                        	dbi_result_free(idi->dbinfo.dbi_result);
	                        idi->dbinfo.dbi_result = NULL;
			}
                        free(query);
		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET notification_interval=%lf, escalate_on_recovery=%d, escalate_on_warning=%d, escalate_on_unknown=%d, escalate_on_critical=%d WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS],
		                 *(double *) data[6],		/* update start */
		                 *(int *) data[7],
		                 *(int *) data[8],
		                 *(int *) data[9],
		                 *(int *) data[10],		/* update end */
		                 *(unsigned long *) data[0],    	/* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[2],
		                 *(unsigned long *) data[3],
		                 *(int *) data[4],
		                 *(int *) data[5]		/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, service_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_warning, escalate_on_unknown, escalate_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d, %d)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(unsigned long *) data[3],
			                 *(int *) data[4],
			                 *(int *) data[5],
			                 *(double *) data[6],
			                 *(int *) data[7],
			                 *(int *) data[8],
			                 *(int *) data[9],
			                 *(int *) data[10]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf, "%s_serviceescalation_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS]) == -1)
                	                buf = NULL;

                        	*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
	                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_serviceescalationdefinition(%s=%lu) escalation_id\n", buf, *id);
        	                free(buf);
			}

                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
		}
                else { 
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

                        /* we hit an update, fetch the id */
                        asprintf(&query, "SELECT serviceescalation_id FROM %s WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONS],
                                 *(unsigned long *) data[0],            /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2],
                                 *(unsigned long *) data[3],
                                 *(int *) data[4],
                                 *(int *) data[5]               /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "serviceescalation_id");
                                                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_serviceescalationdefinition(%lu) escalation_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);
                }
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X5"), (int *) data[4])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X6"), (int *) data[5])) {
		return IDO_ERROR;
	}
	if (!OCI_BindDouble(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X7"), (double *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X8"), (int *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_serviceescalationdefinition_definition, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_serviceescalationdefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_serviceescalationdefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	if (asprintf(&seq_name, "seq_serviceescalations") == -1)
		seq_name = NULL;

	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_serviceescalationdefinition(%lu) escalation_id\n", *id);
	free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_definition_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_serviceescalationdefinition_contactgroups_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long serviceescalation_contactgroup_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_contactgroups_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE serviceescalation_id=%lu AND contactgroup_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS],
                                 *(unsigned long *) data[0],     /* update start/end */
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT serviceescalation_contactgroup_id FROM %s WHERE serviceescalation_id=%lu AND contactgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS],
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                serviceescalation_contactgroup_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "serviceescalation_contactgroup_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, serviceescalation_id, contactgroup_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]    /* insert start */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE serviceescalation_id=%lu AND contactgroup_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS],
		                 *(unsigned long *) data[0],     /* update start/end */
		                 *(unsigned long *) data[1],     /* unique constraint start */
		                 *(unsigned long *) data[2]     /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, serviceescalation_id, contactgroup_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]    /* insert start */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_serviceescalationdefinition_contactgroups, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_serviceescalationdefinition_contactgroups, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_serviceescalationdefinition_contactgroups, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_serviceescalationdefinition_contactgroups)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_serviceescalationdefinition_contactgroups() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_contactgroups_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_serviceescalationdefinition_contacts_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long serviceescalation_contact_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_contacts_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu, serviceescalation_id=%lu, contact_object_id=%lu WHERE instance_id=%lu AND serviceescalation_id=%lu AND contact_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS],
                                 *(unsigned long *) data[0],     /* update start */
                                 *(unsigned long *) data[1],
                                 *(unsigned long *) data[2],     /* update start */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT serviceescalation_contact_id FROM %s WHERE instance_id=%lu AND serviceescalation_id=%lu AND contact_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                serviceescalation_contact_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "serviceescalation_contact_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, serviceescalation_id, contact_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]     /* insert end */
                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
                	        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu, serviceescalation_id=%lu, contact_object_id=%lu WHERE instance_id=%lu AND serviceescalation_id=%lu AND contact_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS],
		                 *(unsigned long *) data[0],     /* update start */
		                 *(unsigned long *) data[1],
		                 *(unsigned long *) data[2],     /* update start */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(unsigned long *) data[1],
		                 *(unsigned long *) data[2]     /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, serviceescalation_id, contact_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_serviceescalationdefinition_contacts, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_serviceescalationdefinition_contacts, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_serviceescalationdefinition_contacts, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_serviceescalationdefinition_contacts)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_serviceescalationdefinition_contacts() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_contacts_add() end\n");

	return result;
}


/************************************/
/*  COMMANDDEFINITION               */
/************************************/

int ido2db_query_insert_or_update_commanddefinition_definition_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long command_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commanddefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET command_line='%s' WHERE instance_id=%lu AND object_id=%lu AND config_type=%d",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_COMMANDS],
                                 *(char **) data[3],            /* update start/end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(int *) data[2]               /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT command_id FROM %s WHERE instance_id=%lu AND object_id=%lu AND config_type=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_COMMANDS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(int *) data[2]               /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                command_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "command_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, object_id, config_type, command_line) VALUES (%lu, %lu, %d, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_COMMANDS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(int *) data[2],
                                         *(char **) data[3]             /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET command_line=E'%s' WHERE instance_id=%lu AND object_id=%lu AND config_type=%d",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_COMMANDS],
		                 *(char **) data[3],		/* update start/end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(unsigned long *) data[1],
		                 *(int *) data[2]		/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, object_id, config_type, command_line) VALUES (%lu, %lu, %d, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_COMMANDS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(int *) data[2],
			                 *(char **) data[3]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commanddefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_commanddefinition_definition, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_commanddefinition_definition, MT(":X3"), (int *) data[2])) {
		return IDO_ERROR;
	}
	if (*(char **) data[3] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_commanddefinition_definition, ":X4") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit commandline size #3324 */
		if ( strlen(*(char **)data[3])  > OCI_COMMAND_LINE_SIZE ) {
			(*(char **)data[3])[OCI_COMMAND_LINE_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_commanddefinition() commandline shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_commanddefinition_definition, MT(":X4"), *(char **) data[3], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_commanddefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */

#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commanddefinition_definition_add() end\n");

	return result;
}


/************************************/
/*  TIMEPERIODDEFINITION            */
/************************************/

int ido2db_query_insert_or_update_timeperiodefinition_definition_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timeperiodefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET alias='%s' WHERE instance_id=%lu AND config_type=%d AND timeperiod_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS],
                                 *(char **) data[3],            /* update start/end */
                                 *(unsigned long *) data[0],            /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;

                        asprintf(&query, "SELECT timeperiod_id FROM %s WHERE instance_id=%lu AND config_type=%d AND timeperiod_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS],
                                 *(unsigned long *) data[0],            /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                /* this condition should never happen, as libdbi UPDATE and affected rows
                                                 * should take care of it. it seems that newer mysql versions got problems
                                                 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
                                                 * as fallback here
                                                 */
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "timeperiod_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, timeperiod_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(char **) data[3]             /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);

				if (result == IDO_OK) {
		                        /* mysql doesn't use sequences */
                		        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition(%lu) timeperiod_id\n", *id);
				}

                        	dbi_result_free(idi->dbinfo.dbi_result);
	                        idi->dbinfo.dbi_result = NULL;
			}
                }
		else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

			/* we hit an update, fetch the id */                
                        asprintf(&query, "SELECT timeperiod_id FROM %s WHERE instance_id=%lu AND config_type=%d AND timeperiod_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS],
                                 *(unsigned long *) data[0],            /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "timeperiod_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition(%lu) timeperiod_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                        	dbi_result_free(idi->dbinfo.dbi_result);
	                        idi->dbinfo.dbi_result = NULL;
			}
                        free(query);
		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET alias=E'%s' WHERE instance_id=%lu AND config_type=%d AND timeperiod_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS],
		                 *(char **) data[3],		/* update start/end */
		                 *(unsigned long *) data[0],    	/* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[2]	/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, timeperiod_object_id, alias) VALUES (%lu, %d, %lu, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(char **) data[3]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf, "%s_timeperiod_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS]) == -1)
                	                buf = NULL;

	                        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
        	                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition(%s=%lu) timeperiod_id\n", buf, *id);
                	        free(buf);
			}

                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
		}
                else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

			/* we hit an update, fetch the id */                
                        asprintf(&query, "SELECT timeperiod_id FROM %s WHERE instance_id=%lu AND config_type=%d AND timeperiod_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODS],
                                 *(unsigned long *) data[0],            /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                                 
                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "timeperiod_id");
                                                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition(%lu) timeperiod_id\n", *id);
                                        }
                        
                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);
                }
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timeperiodefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_timeperiodefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timeperiodefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (*(char **) data[3] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_timeperiodefinition_definition, ":X3") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_timeperiodefinition_definition, MT(":X4"), *(char **) data[3], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_timeperiodefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_timeperiodefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	if (asprintf(&seq_name, "seq_timeperiods") == -1)
		seq_name = NULL;

	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_timeperiodefinition(%lu) timeperiod_id\n", *id);
	free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timeperiodefinition_definition_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_timeperiodefinition_timeranges_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long timeperiod_timerange_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timeperiodefinition_timeranges_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE timeperiod_id=%lu AND day=%d AND start_sec=%lu AND end_sec=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODTIMERANGES],
                                 *(unsigned long *) data[0],     /* update start/end */
                                 *(unsigned long *) data[1],    /* unique constraint start */
                                 *(int *) data[2],
                                 *(unsigned long *) data[3],
                                 *(unsigned long *) data[4]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;

                        asprintf(&query, "SELECT timeperiod_timerange_id FROM %s WHERE timeperiod_id=%lu AND day=%d AND start_sec=%lu AND end_sec=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODTIMERANGES],
                                 *(unsigned long *) data[1],    /* unique constraint start */
                                 *(int *) data[2],
                                 *(unsigned long *) data[3],
                                 *(unsigned long *) data[4]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                timeperiod_timerange_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "timeperiod_timerange_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, timeperiod_id, day, start_sec, end_sec) VALUES (%lu, %lu, %d, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODTIMERANGES],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(int *) data[2],
                                         *(unsigned long *) data[3],
                                         *(unsigned long *) data[4]     /* insert end */
                                        );
                	        /* send query to db */
                        	result = ido2db_db_query(idi, query2);
	                        free(query2);
			}
                }
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE timeperiod_id=%lu AND day=%d AND start_sec=%lu AND end_sec=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODTIMERANGES],
		                 *(unsigned long *) data[0],     /* update start/end */
		                 *(unsigned long *) data[1],	/* unique constraint start */
		                 *(int *) data[2],
		                 *(unsigned long *) data[3],
		                 *(unsigned long *) data[4]	/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, timeperiod_id, day, start_sec, end_sec) VALUES (%lu, %lu, %d, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_TIMEPERIODTIMERANGES],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(int *) data[2],
			                 *(unsigned long *) data[3],
			                 *(unsigned long *) data[4]	/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X3"), (int *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_timeperiodefinition_timeranges, MT(":X5"), (uint *) data[4])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_timeperiodefinition_timeranges)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_timeperiodefinition_timeranges() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timeperiodefinition_timeranges_add() end\n");

	return result;
}


/************************************/
/* CONTACTDEFINITION                */
/************************************/

int ido2db_query_insert_or_update_contactdefinition_definition_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET alias='%s', email_address='%s', pager_address='%s', host_timeperiod_object_id=%lu, service_timeperiod_object_id=%lu, host_notifications_enabled=%d, service_notifications_enabled=%d, can_submit_commands=%d, notify_service_recovery=%d, notify_service_warning=%d, notify_service_unknown=%d, notify_service_critical=%d, notify_service_flapping=%d, notify_service_downtime=%d, notify_host_recovery=%d, notify_host_down=%d, notify_host_unreachable=%d, notify_host_flapping=%d, notify_host_downtime=%d WHERE instance_id=%lu AND config_type=%d AND contact_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS],
                                 *(char **) data[3],            /* update start */
                                 *(char **) data[4],
                                 *(char **) data[5],
                                 *(unsigned long *) data[6],
                                 *(unsigned long *) data[7],
                                 *(int *) data[8],
                                 *(int *) data[9],
                                 *(int *) data[10],
                                 *(int *) data[11],
                                 *(int *) data[12],
                                 *(int *) data[13],
                                 *(int *) data[14],
                                 *(int *) data[15],
                                 *(int *) data[16],
                                 *(int *) data[17],
                                 *(int *) data[18],
                                 *(int *) data[19],
                                 *(int *) data[20],
                                 *(int *) data[21],              /* update end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT contact_id FROM %s WHERE instance_id=%lu AND config_type=%d AND contact_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                /* this condition should never happen, as libdbi UPDATE and affected rows
                                                 * should take care of it. it seems that newer mysql versions got problems
                                                 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
                                                 * as fallback here
                                                 */
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contact_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, contact_object_id, alias, email_address, pager_address, host_timeperiod_object_id, service_timeperiod_object_id, host_notifications_enabled, service_notifications_enabled, can_submit_commands, notify_service_recovery, notify_service_warning, notify_service_unknown, notify_service_critical, notify_service_flapping, notify_service_downtime, notify_host_recovery, notify_host_down, notify_host_unreachable, notify_host_flapping, notify_host_downtime) VALUES (%lu, %d, %lu, '%s', '%s', '%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(char **) data[3],
                                         *(char **) data[4],
                                         *(char **) data[5],
                                         *(unsigned long *) data[6],
                                         *(unsigned long *) data[7],
                                         *(int *) data[8],
                                         *(int *) data[9],
                                         *(int *) data[10],
                                         *(int *) data[11],
                                         *(int *) data[12],
                                         *(int *) data[13],
                                         *(int *) data[14],
                                         *(int *) data[15],
                                         *(int *) data[16],
                                         *(int *) data[17],
                                         *(int *) data[18],
                                         *(int *) data[19],
                                         *(int *) data[20],
                                         *(int *) data[21]              /* insert end */
                                        );
                        	/* send query to db */
                	        result = ido2db_db_query(idi, query2);
	                        free(query2);

				if (result == IDO_OK) {
		                        /* mysql doesn't use sequences */
                		        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition(%lu) contact_id\n", *id);
				}

	                        dbi_result_free(idi->dbinfo.dbi_result);
        	                idi->dbinfo.dbi_result = NULL;
			}
                }
		else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

			/* we hit an update, fetch the id */
                        asprintf(&query, "SELECT contact_id FROM %s WHERE instance_id=%lu AND config_type=%d AND contact_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contact_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition(%lu) contact_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                        	dbi_result_free(idi->dbinfo.dbi_result);
	                        idi->dbinfo.dbi_result = NULL;
			}
                        free(query);

		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET alias=E'%s', email_address=E'%s', pager_address=E'%s', host_timeperiod_object_id=%lu, service_timeperiod_object_id=%lu, host_notifications_enabled=%d, service_notifications_enabled=%d, can_submit_commands=%d, notify_service_recovery=%d, notify_service_warning=%d, notify_service_unknown=%d, notify_service_critical=%d, notify_service_flapping=%d, notify_service_downtime=%d, notify_host_recovery=%d, notify_host_down=%d, notify_host_unreachable=%d, notify_host_flapping=%d, notify_host_downtime=%d WHERE instance_id=%lu AND config_type=%d AND contact_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS],
		                 *(char **) data[3],		/* update start */
		                 *(char **) data[4],
		                 *(char **) data[5],
		                 *(unsigned long *) data[6],
		                 *(unsigned long *) data[7],
		                 *(int *) data[8],
		                 *(int *) data[9],
		                 *(int *) data[10],
		                 *(int *) data[11],
		                 *(int *) data[12],
		                 *(int *) data[13],
		                 *(int *) data[14],
		                 *(int *) data[15],
		                 *(int *) data[16],
		                 *(int *) data[17],
		                 *(int *) data[18],
		                 *(int *) data[19],
		                 *(int *) data[20],
		                 *(int *) data[21],              /* update end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[2]	/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, contact_object_id, alias, email_address, pager_address, host_timeperiod_object_id, service_timeperiod_object_id, host_notifications_enabled, service_notifications_enabled, can_submit_commands, notify_service_recovery, notify_service_warning, notify_service_unknown, notify_service_critical, notify_service_flapping, notify_service_downtime, notify_host_recovery, notify_host_down, notify_host_unreachable, notify_host_flapping, notify_host_downtime) VALUES (%lu, %d, %lu, E'%s', E'%s', E'%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(char **) data[3],
			                 *(char **) data[4],
			                 *(char **) data[5],
			                 *(unsigned long *) data[6],
			                 *(unsigned long *) data[7],
			                 *(int *) data[8],
			                 *(int *) data[9],
			                 *(int *) data[10],
			                 *(int *) data[11],
			                 *(int *) data[12],
			                 *(int *) data[13],
			                 *(int *) data[14],
			                 *(int *) data[15],
			                 *(int *) data[16],
			                 *(int *) data[17],
			                 *(int *) data[18],
			                 *(int *) data[19],
			                 *(int *) data[20],
			                 *(int *) data[21]              /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf, "%s_contact_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS]) == -1)
                	                buf = NULL;

                        	*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
	                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition(ido2db_idi *idi)(%s=%lu) contact_id\n", buf, *id);
	                        free(buf);
			}

			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
		}
                else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
                
                        /* we hit an update, fetch the id */
                        asprintf(&query, "SELECT contact_id FROM %s WHERE instance_id=%lu AND config_type=%d AND contact_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contact_id");
                                                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition(%lu) contact_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);

                }
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (*(char **) data[3] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_contactdefinition_definition, ":X4") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X4"), *(char **) data[3], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_contactdefinition_definition, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[5] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_contactdefinition_definition, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X7"), (uint *) data[6])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X8"), (uint *) data[7])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X9"), (int *) data[8])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X10"), (int *) data[9])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X11"), (int *) data[10])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X12"), (int *) data[11])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X13"), (int *) data[12])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X14"), (int *) data[13])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X15"), (int *) data[14])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X16"), (int *) data[15])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X17"), (int *) data[16])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X18"), (int *) data[17])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X19"), (int *) data[18])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X20"), (int *) data[19])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X21"), (int *) data[20])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_definition, MT(":X22"), (int *) data[21])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_contactdefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_contactdefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	if (asprintf(&seq_name, "seq_contacts") == -1)
	seq_name = NULL;

	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactdefinition(%lu) contact_id\n", *id);
	free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_definition_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_contactdefinition_addresses_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long contact_address_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_addresses_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu, address='%s' WHERE contact_id=%lu AND address_number=%d",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTADDRESSES],
                                 *(unsigned long *) data[0],     /* update start */
                                 *(char **) data[3],            /* update end */
                                 *(unsigned long *) data[1],    /* unique constraint start */
                                 *(int *) data[2]               /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT contact_address_id FROM %s WHERE contact_id=%lu AND address_number=%d",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTADDRESSES],
                                 *(unsigned long *) data[1],    /* unique constraint start */
                                 *(int *) data[2]               /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                contact_address_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contact_address_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, contact_id, address_number, address) VALUES (%lu, %lu, %d, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTADDRESSES],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(int *) data[2],
                                         *(char **) data[3]             /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu, address=E'%s' WHERE contact_id=%lu AND address_number=%d",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTADDRESSES],
		                 *(unsigned long *) data[0],     /* update start */
		                 *(char **) data[3],		/* update end */
		                 *(unsigned long *) data[1],	/* unique constraint start */
		                 *(int *) data[2]		/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, contact_id, address_number, address) VALUES (%lu, %lu, %d, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTADDRESSES],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(int *) data[2],
			                 *(char **) data[3]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactdefinition_addresses, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactdefinition_addresses, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_addresses, MT(":X3"), (int *) data[2])) {
		return IDO_ERROR;
	}
	if (*(char **) data[3] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_contactdefinition_addresses, ":X4") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_contactdefinition_addresses, MT(":X4"), *(char **) data[3], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_contactdefinition_addresses)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_contactdefinition_addresses() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_addresses_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_contactdefinition_notificationcommands_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long contact_notificationcommand_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_notificationcommands_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET command_args='%s' WHERE instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4],                /* update start/end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(int *) data[2],
                                 *(unsigned long *) data[3]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;

                        asprintf(&query, "SELECT contact_notificationcommand_id FROM %s WHERE instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(int *) data[2],
                                 *(unsigned long *) data[3]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                contact_notificationcommand_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contact_notificationcommand_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(int *) data[2],
                                         *(unsigned long *) data[3],
                                         (*(char **) data[4] == NULL) ? "" : *(char **) data[4]         /* insert end */
                                        );
                	        /* send query to db */
                        	result = ido2db_db_query(idi, query2);
	                        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET command_args=E'%s' WHERE instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
		                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4],		/* update start/end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(unsigned long *) data[1],
		                 *(int *) data[2],
		                 *(unsigned long *) data[3]	/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(int *) data[2],
			                 *(unsigned long *) data[3],
			                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* bind params to prepared statement */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contact_notificationcommands, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contact_notificationcommands, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contact_notificationcommands, MT(":X3"), (int *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contact_notificationcommands, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_contact_notificationcommands, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		/* limit command_arg size #3324 */
		if ( strlen(*(char **)data[4])  > OCI_COMMAND_ARG_SIZE ) {
			(*(char **)data[4])[OCI_COMMAND_ARG_SIZE] = '\0';
			ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_contact_notification() command_args shorted\n");
		}
		if (!OCI_BindString(idi->dbinfo.oci_statement_contact_notificationcommands, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_contact_notificationcommands)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_notificationcommands_add() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinitionnotificationcommands_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_contactdefinition_servicenotificationcommands_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long contact_notificationcommand_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_servicenotificationcommands_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET command_args='%s' WHERE instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                 *(char **) data[4],            /* update start/end */
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(int *) data[2],
                                 *(unsigned long *) data[3]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT contact_notificationcommand_id FROM %s WHERE instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                 *(unsigned long *) data[0],     /* unique constraint start */
                                 *(unsigned long *) data[1],
                                 *(int *) data[2],
                                 *(unsigned long *) data[3]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                contact_notificationcommand_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contact_notificationcommand_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(int *) data[2],
                                         *(unsigned long *) data[3],
                                         *(char **) data[4]             /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);
			}
                }
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET command_args=E'%s' WHERE instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
		                 *(char **) data[4],		/* update start/end */
		                 *(unsigned long *) data[0],     /* unique constraint start */
		                 *(unsigned long *) data[1],
		                 *(int *) data[2],
		                 *(unsigned long *) data[3]	/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(int *) data[2],
			                 *(unsigned long *) data[3],
			                 *(char **) data[4]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands, MT(":X3"), (int *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands, MT(":X4"), (uint *) data[3])) {
		return IDO_ERROR;
	}
	if (!OCI_BindString(idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands, MT(":X5"), *(char **) data[4], 0)) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_contactdefinition_servicenotificationcommands)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_contactdefinition_servicenotificationcommands() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_servicenotificationcommands_add() end\n");

	return result;
}


/************************************/
/* CUSTOMVARIABLES                  */
/************************************/

int ido2db_query_insert_or_update_save_custom_variables_customvariables_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long customvariable_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_save_custom_variables_customvariables_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu, config_type=%d, has_been_modified=%d, varvalue='%s' WHERE object_id=%lu AND varname='%s'",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLES],
                                 *(unsigned long *) data[0],     /* update start */
                                 *(int *) data[2],
                                 *(int *) data[3],
                                 (*(char **) data[5] == NULL) ? "" : *(char **) data[5],                /* update end */
                                 *(unsigned long *) data[1],    /* unique constraint start */
                                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4]         /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT customvariable_id FROM %s WHERE object_id=%lu AND varname='%s'",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLES],
                                 *(unsigned long *) data[1],    /* unique constraint start */
                                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4]         /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                customvariable_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "customvariable_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);

                        if (mysql_update == FALSE) {

	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %d, %d, '%s', '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLES],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(int *) data[2],
                                         *(int *) data[3],
                                         (*(char **) data[4] == NULL) ? "" : *(char **) data[4],
                                         (*(char **) data[5] == NULL) ? "" : *(char **) data[5]         /* insert end */
                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
                	        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu, config_type=%d, has_been_modified=%d, varvalue=E'%s' WHERE object_id=%lu AND varname=E'%s'",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLES],
		                 *(unsigned long *) data[0],     /* update start */
		                 *(int *) data[2],
		                 *(int *) data[3],
		                 (*(char **) data[5] == NULL) ? "" : *(char **) data[5],		/* update end */
		                 *(unsigned long *) data[1],	/* unique constraint start */
		                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4]		/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %d, %d, E'%s', E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLES],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(int *) data[2],
			                 *(int *) data[3],
			                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4],
			                 (*(char **) data[5] == NULL) ? "" : *(char **) data[5]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_save_custom_variables_customvariables, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_save_custom_variables_customvariables, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_save_custom_variables_customvariables, MT(":X3"), (int *) data[2])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_save_custom_variables_customvariables, MT(":X4"), (int *) data[3])) {
		return IDO_ERROR;
	}
	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_save_custom_variables_customvariables, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_save_custom_variables_customvariables, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[5] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_save_custom_variables_customvariables, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_save_custom_variables_customvariables, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}


	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_save_custom_variables_customvariables)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_save_custom_variables_customvariables() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_save_custom_variables_customvariables_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_save_custom_variables_customvariablestatus_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long customvariablestatus_id;
        int mysql_update = FALSE;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_save_custom_variables_customvariablestatus_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu, status_update_time=%s, has_been_modified=%d, varvalue='%s' WHERE object_id=%lu AND varname='%s'",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
                                 *(unsigned long *) data[0],     /* update start */
                                 *(char **) data[2],
                                 *(int *) data[3],
                                 (*(char **) data[5] == NULL) ? "" : *(char **) data[5],       /* update end */
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4] /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT customvariablestatus_id FROM %s WHERE object_id=%lu AND varname='%s'",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
                                 *(unsigned long *) data[1],     /* unique constraint start */
                                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4] /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                customvariablestatus_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "customvariablestatus_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);

                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, object_id, status_update_time, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %s, %d, '%s', '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(char **) data[2],
                                         *(int *) data[3],
                                         (*(char **) data[4] == NULL) ? "" : *(char **) data[4],
                                         (*(char **) data[5] == NULL) ? "" : *(char **) data[5]       /* insert end */
                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
                	        free(query2);
			}
                }
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu, status_update_time=%s, has_been_modified=%d, varvalue=E'%s' WHERE object_id=%lu AND varname=E'%s'",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
		                 *(unsigned long *) data[0],     /* update start */
		                 *(char **) data[2],
		                 *(int *) data[3],
		                 (*(char **) data[5] == NULL) ? "" : *(char **) data[5],       /* update end */
		                 *(unsigned long *) data[1],     /* unique constraint start */
		                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4] /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, object_id, status_update_time, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %s, %d, E'%s', E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(char **) data[2],
			                 *(int *) data[3],
			                 (*(char **) data[4] == NULL) ? "" : *(char **) data[4],
			                 (*(char **) data[5] == NULL) ? "" : *(char **) data[5]       /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		/* free last dbi_result */
		dbi_result_free(idi->dbinfo.dbi_result);
		idi->dbinfo.dbi_result = NULL;
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	/* we need to check if time was provided, and then explicitely bind value to NULL */
	if ((*(uint *) data[6]) < 0) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, ":X3") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else { /* fine */
		if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, MT(":X3"), (uint *) data[6])) { /* unixtimestamp instead of time2sql */
			return IDO_ERROR;
		}
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, MT(":X4"), (int *) data[3])) {
		return IDO_ERROR;
	}

	if (*(char **) data[4] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, ":X5") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, MT(":X5"), *(char **) data[4], 0)) {
			return IDO_ERROR;
		}
	}
	if (*(char **) data[5] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, ":X6") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus, MT(":X6"), *(char **) data[5], 0)) {
			return IDO_ERROR;
		}
	}


	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_save_custom_variables_customvariablestatus)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_save_custom_variables_customvariablestatus() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_save_custom_variables_customvariablestatus_add() end\n");

	return result;
}


/************************************/
/* CONTACTGROUPDEFINITION           */
/************************************/

int ido2db_query_insert_or_update_contactgroupdefinition_definition_add(ido2db_idi *idi, void **data, unsigned long *id) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
	char * buf = NULL;
        int mysql_update = FALSE;
#endif
#ifdef USE_ORACLE
	char * seq_name = NULL;
#endif

	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactgroupdefinition_definition_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET alias='%s' WHERE instance_id=%lu AND config_type=%d AND contactgroup_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS],
                                 *(char **) data[3],            /* update start/end */
                                 *(unsigned long *) data[0],    /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT contactgroup_id FROM %s WHERE instance_id=%lu AND config_type=%d AND contactgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS],
                                 *(unsigned long *) data[0],    /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                /* this condition should never happen, as libdbi UPDATE and affected rows
                                                 * should take care of it. it seems that newer mysql versions got problems
                                                 * with libdbi (https://dev.icinga.org/issues/3728) so we return the selected id
                                                 * as fallback here
                                                 */
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contactgroup_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, contactgroup_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(int *) data[1],
                                         *(unsigned long *) data[2],
                                         *(char **) data[3]             /* insert end */
                                        );
                	        /* send query to db */
	                        result = ido2db_db_query(idi, query2);
        	                free(query2);

				if (result == IDO_OK) {
		                        /* mysql doesn't use sequences */
                		        *id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
		                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition(%lu) group_id\n", *id);
				}

        	                dbi_result_free(idi->dbinfo.dbi_result);
                	        idi->dbinfo.dbi_result = NULL;
			}
                }
		else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;

			/* we hit an update, fetch the id */
                        asprintf(&query, "SELECT contactgroup_id FROM %s WHERE instance_id=%lu AND config_type=%d AND contactgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS],
                                 *(unsigned long *) data[0],    /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contactgroup_id");
		                        	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition(%lu) group_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                        	dbi_result_free(idi->dbinfo.dbi_result);
	                        idi->dbinfo.dbi_result = NULL;
			}
                        free(query);
		}
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET alias=E'%s' WHERE instance_id=%lu AND config_type=%d AND contactgroup_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS],
		                 *(char **) data[3],		/* update start/end */
		                 *(unsigned long *) data[0], 	/* unique constraint start */
		                 *(int *) data[1],
		                 *(unsigned long *) data[2]	/* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, config_type, contactgroup_object_id, alias) VALUES (%lu, %d, %lu, E'%s')",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(int *) data[1],
			                 *(unsigned long *) data[2],
			                 *(char **) data[3]		/* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);

			if (result == IDO_OK) {
	                        /* depending on tableprefix/tablename a sequence will be used */
        	                if (asprintf(&buf, "%s_contactgroup_id_seq", ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS]) == -1)
                	                buf = NULL;

                        	*id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf);
	                        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition(%s=%lu) group_id\n", buf, *id);
	                        free(buf);
			}

			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
		}
                else {
                        dbi_result_free(idi->dbinfo.dbi_result);
                        idi->dbinfo.dbi_result = NULL;
                
                        /* we hit an update, fetch the id */
                        asprintf(&query, "SELECT contactgroup_id FROM %s WHERE instance_id=%lu AND config_type=%d AND contactgroup_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPS],
                                 *(unsigned long *) data[0],    /* unique constraint start */
                                 *(int *) data[1],
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                                 
                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
                                                *id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contactgroup_id");
                                                ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition(%lu) group_id\n", *id);
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
			else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);
                }
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactgroupdefinition_definition, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindInt(idi->dbinfo.oci_statement_contactgroupdefinition_definition, MT(":X2"), (int *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactgroupdefinition_definition, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}
	if (*(char **) data[3] == NULL) {
		if (ido2db_oci_prepared_statement_bind_null_param(idi->dbinfo.oci_statement_contactgroupdefinition_definition, ":X4") == IDO_ERROR) {
			return IDO_ERROR;
		}
	} else {
		if (!OCI_BindString(idi->dbinfo.oci_statement_contactgroupdefinition_definition, MT(":X4"), *(char **) data[3], 0)) {
			return IDO_ERROR;
		}
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_contactgroupdefinition_definition)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_contactgroupdefinition_definition() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	asprintf(&seq_name, "seq_contactgroups");
	*id = ido2db_oci_sequence_lastid(idi, seq_name);
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_handle_contactgroupdefinition(%lu) group_id\n", *id);
	free(seq_name);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactgroupdefinition_definition_add() end\n");

	return result;
}


int ido2db_query_insert_or_update_contactgroupdefinition_contactgroupmembers_add(ido2db_idi *idi, void **data) {
	int result = IDO_OK;
#ifdef USE_LIBDBI
        char * query = NULL;
        char * query1 = NULL;
        char * query2 = NULL;
//        unsigned long contactgroup_member_id;
        int mysql_update = FALSE;
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactgroupdefinition_contactgroupmembers_add() start\n");

	if (idi == NULL)
		return IDO_ERROR;

	if (idi->dbinfo.connected == IDO_FALSE)
		return IDO_ERROR;

#ifdef USE_LIBDBI /* everything else will be libdbi */
	switch (idi->dbinfo.server_type) {
	case IDO2DB_DBSERVER_MYSQL:
                asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE contactgroup_id=%lu AND contact_object_id=%lu",
                                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPMEMBERS],
                                 *(unsigned long *) data[0],     /* update start/end */
                                 *(unsigned long *) data[1],            /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );
                /* send query to db */
                result = ido2db_db_query(idi, query1);
                free(query1);

                /* check result if update was ok */
                if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                	dbi_result_free(idi->dbinfo.dbi_result);
                	idi->dbinfo.dbi_result = NULL;


                        asprintf(&query, "SELECT contactgroup_member_id FROM %s WHERE contactgroup_id=%lu AND contact_object_id=%lu",
                                ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPMEMBERS],
                                 *(unsigned long *) data[1],            /* unique constraint start */
                                 *(unsigned long *) data[2]     /* unique constraint end */
                                );

                        /* send query to db */
                        if ((result = ido2db_db_query(idi, query)) == IDO_OK) {
                                if (idi->dbinfo.dbi_result != NULL) {
                                        if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
//                                                contactgroup_member_id = dbi_result_get_ulonglong(idi->dbinfo.dbi_result, "contactgroup_member_id");
                                                mysql_update = TRUE;
                                        } else {
                                                mysql_update = FALSE;
                                        }

                                        dbi_result_free(idi->dbinfo.dbi_result);
                                        idi->dbinfo.dbi_result = NULL;
                                }
                        }
                        else {
                                dbi_result_free(idi->dbinfo.dbi_result);
                                idi->dbinfo.dbi_result = NULL;
                        }
                        free(query);


                        if (mysql_update == FALSE) {
	                        /* try insert instead */
        	                asprintf(&query2, "INSERT INTO %s (instance_id, contactgroup_id, contact_object_id) VALUES (%lu, %lu, %lu)",
                                         ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPMEMBERS],
                                         *(unsigned long *) data[0],     /* insert start */
                                         *(unsigned long *) data[1],
                                         *(unsigned long *) data[2]     /* insert end */
                                        );
	                        /* send query to db */
        	                result = ido2db_db_query(idi, query2);
                	        free(query2);
			}
                }
                break;

	case IDO2DB_DBSERVER_PGSQL:
		asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE contactgroup_id=%lu AND contact_object_id=%lu",
		                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPMEMBERS],
		                 *(unsigned long *) data[0],     /* update start/end */
		                 *(unsigned long *) data[1],    	/* unique constraint start */
		                 *(unsigned long *) data[2]     /* unique constraint end */
		                );
		/* send query to db */
		result = ido2db_db_query(idi, query1);
		free(query1);

		/* check result if update was ok */
		if (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;

			/* try insert instead */
			asprintf(&query2, "INSERT INTO %s (instance_id, contactgroup_id, contact_object_id) VALUES (%lu, %lu, %lu)",
			                 ido2db_db_tablenames[IDO2DB_DBTABLE_CONTACTGROUPMEMBERS],
			                 *(unsigned long *) data[0],     /* insert start */
			                 *(unsigned long *) data[1],
			                 *(unsigned long *) data[2]     /* insert end */
			                );
			/* send query to db */
			result = ido2db_db_query(idi, query2);
			free(query2);
		}
		break;
	default:
		break;
	}
#endif

#ifdef USE_PGSQL /* pgsql */

#endif

#ifdef USE_ORACLE /* Oracle ocilib specific */

	/* check if we lost connection, and reconnect */
	if (ido2db_db_reconnect(idi, IDO_TRUE) == IDO_ERROR)
		return IDO_ERROR;

	/* use prepared statements and ocilib */
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, MT(":X1"), (uint *) data[0])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, MT(":X2"), (uint *) data[1])) {
		return IDO_ERROR;
	}
	if (!OCI_BindUnsignedInt(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers, MT(":X3"), (uint *) data[2])) {
		return IDO_ERROR;
	}

	/* execute statement */
	if (!OCI_Execute(idi->dbinfo.oci_statement_contactgroupdefinition_contactgroupmembers)) {
		ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_contactgroupdefinition_contactgroupmembers() execute error\n");
		return IDO_ERROR;
	}

	/* commit statement */
	OCI_Commit(idi->dbinfo.oci_connection);

	/* do not free statement yet! */
#endif
	ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactgroupdefinition_contactgroupmembers_add() end\n");

	return result;
}




