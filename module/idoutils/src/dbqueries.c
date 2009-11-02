/***************************************************************
 * DBQUERIES.C - Data Query handler routines for IDO2DB daemon
 *
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
#include "../include/db.h"
#include "../include/dbqueries.h"

/* Icinga header files */
#include "../../../include/icinga.h"
#include "../../../include/broker.h"
#include "../../../include/comments.h"

extern int errno;

extern char *ndo2db_db_tablenames[NDO2DB_MAX_DBTABLES];

/****************************************************************************/
/* OBJECT ROUTINES                                                          */
/****************************************************************************/

/****************************************************************************/
/* INSERT QUERIES                                                          */
/****************************************************************************/

/****************************************************************************/
/* DELETE QUERIES                                                          */
/****************************************************************************/

/****************************************************************************/
/* INSERT/UPDATE/MERGE QUERIES                                                          */
/****************************************************************************/


/************************************/
/* TIMEDEVENTS                      */
/************************************/

int ido2db_query_insert_or_update_timedevent_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
	char * query1 = NULL;
	char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timedevents_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;
	
        switch (idi->dbinfo.server_type) {
	        case NDO2DB_DBSERVER_MYSQL:
			asprintf(&query1, "INSERT INTO %s (instance_id, event_type, queued_time, queued_time_usec, scheduled_time, recurring_event, object_id) VALUES (%lu, %d, %s, %lu, %s, %d, %lu) ON DUPLICATE KEY UPDATE queued_time=%s, queued_time_usec=%lu, recurring_event=%d", 
					ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
					*(unsigned long *) data[0], 	/* insert start */
					*(int *) data[1],
					*(char **) data[2],
					*(unsigned long *) data[3],
					*(char **) data[4],
					*(int *) data[5],
					*(unsigned long *) data[6], 	/* insert end */
					*(char **) data[2],		/* update start */
					*(unsigned long *) data[3],
					*(int *) data[5]		/* update end */
			);
			/* send query to db */
			result = ndo2db_db_query(idi, query1);	
			free(query1);
	                break;
        	case NDO2DB_DBSERVER_PGSQL:
			asprintf(&query1, "UPDATE %s SET queued_time=%s, queued_time_usec=%lu, recurring_event=%d WHERE instance_id=%lu AND event_type=%d AND scheduled_time=%s AND object_id=%lu",
					ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
                                        *(char **) data[2],		/* update start */
                                        *(unsigned long *) data[3],
                                        *(int *) data[5],		/* update end */
					*(unsigned long *) data[0],	/* unique constraint start */
					*(int *) data[1],
					*(char **) data[4],
					*(unsigned long *) data[6]	/* unique constraint end */
			);

			/* send query to db */
			result = ndo2db_db_query(idi, query1);		
			free(query1);

			ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timedevents_add(%lu) update rows matched\n", (dbi_result_get_numrows_affected(idi->dbinfo.dbi_result)));
			/* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) { 
				/* try insert instead */
				asprintf(&query2, "INSERT INTO %s (instance_id, event_type, queued_time, queued_time_usec, scheduled_time, recurring_event, object_id) VALUES ('%lu', '%d', %s, '%lu', %s, '%d', '%lu')",
					ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(int *) data[5],
                                        *(unsigned long *) data[6]     /* insert end */
				);
				/* send query to db */
				result = ndo2db_db_query(idi, query2);
				free(query2);
			}		
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
#ifdef USE_ORACLE
			/* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND event_type=%d AND scheduled_time=%s AND object_id=%lu) WHEN MATCHED THEN UPDATE SET queued_time=%s, queued_time_usec=%lu, recurring_event=%d WHEN NOT MATCHED THEN INSERT (instance_id, event_type, queued_time, queued_time_usec, scheduled_time, recurring_event, object_id) VALUES ('%lu', '%d', %s, '%lu', %s, '%d', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
					*(unsigned long *) data[0],	/* unique constraint start */
					*(int *) data[1],
					*(char **) data[4],
					*(unsigned long *) data[6],	/* unique constraint end */
                                        *(char **) data[2],		/* update start */
                                        *(unsigned long *) data[3],
                                        *(int *) data[5],		/* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(int *) data[5],
                                        *(unsigned long *) data[6]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
        	        break;
	        case NDO2DB_DBSERVER_SQLITE:
        	        break;
	        case NDO2DB_DBSERVER_SQLITE3:
        	        break;
	        default:
        	        break;
        }

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timedevents_add() end\n"); 

	return result;
}

int ido2db_query_insert_or_update_timedevents_execute_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timedevents_execute() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, event_type, event_time, event_time_usec, scheduled_time, recurring_event, object_id) VALUES (%lu, %d, %s, %lu, %s, %d, %lu) ON DUPLICATE KEY UPDATE event_time=%s, event_time_usec=%lu, recurring_event=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(int *) data[5],
                                        *(unsigned long *) data[6],     /* insert end */
                                        *(char **) data[2],             /* update start */
                                        *(unsigned long *) data[3],
                                        *(int *) data[5]                /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET event_time=%s, event_time_usec=%lu, recurring_event=%d WHERE instance_id=%lu AND event_type=%d AND scheduled_time=%s AND object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
                                        *(char **) data[2],             /* update start */
                                        *(unsigned *) data[3],
                                        *(int *) data[5],               /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(char **) data[4],
                                        *(unsigned long *) data[6]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, event_type, event_time, event_time_usec, scheduled_time, recurring_event, object_id) VALUES ('%lu', '%d', %s, '%lu', %s, '%d', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(int *) data[5],
                                        *(unsigned long *) data[6]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND event_type=%d AND scheduled_time=%s AND object_id=%lu) WHEN MATCHED THEN UPDATE SET event_time=%s, event_time_usec=%lu, recurring_event=%d WHEN NOT MATCHED THEN INSERT (instance_id, event_type, event_time, event_time_usec, scheduled_time, recurring_event, object_id) VALUES ('%lu', '%d', %s, '%lu', %s, '%d', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(char **) data[4],
                                        *(unsigned long *) data[6],      /* unique constraint end */
                                        *(char **) data[2],             /* update start */
                                        *(unsigned long *) data[3],
                                        *(int *) data[5],               /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(int *) data[5],
                                        *(unsigned long *) data[6]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timedevents_execute() end\n");

        return result;
}


/************************************/
/* SYSTEMCOMMANDDATA                */
/************************************/

int ido2db_query_insert_or_update_systemcommanddata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, start_time, start_time_usec, end_time, end_time_usec, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %s, %lu, %s, %lu, '%s', %d, %d, %lf, %d, '%s', '%s') ON DUPLICATE KEY UPDATE end_time=%s, end_time_usec=%lu, command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SYSTEMCOMMANDS],
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
                                        *(char **) data[11],     	/* insert end */
                                        *(char **) data[3],             /* update start */
                                        *(unsigned long *) data[4],
                                        *(char **) data[5],
                                        *(int *) data[6],
                                        *(int *) data[7],
                                        *(double *) data[8],
                                        *(int *) data[9],
                                        *(char **) data[10],             
                                        *(char **) data[11]             /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET end_time=%s, end_time_usec=%lu, command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s' WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SYSTEMCOMMANDS],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, start_time, start_time_usec, end_time, end_time_usec, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %s, %lu, %s, %lu, '%s', %d, %d, %lf, %d, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SYSTEMCOMMANDS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND start_time=%s AND start_time_usec=%lu) WHEN MATCHED THEN UPDATE SET end_time=%s, end_time_usec=%lu, command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s' WHEN NOT MATCHED THEN INSERT (instance_id, start_time, start_time_usec, end_time, end_time_usec, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %s, %lu, %s, %lu, '%s', %d, %d, %lf, %d, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SYSTEMCOMMANDS],
					*(unsigned long *) data[0],     /* unique constraint start */
                                        *(char **) data[1],
                                        *(unsigned long *) data[2],      /* unique constraint end */
                                        *(char **) data[3],             /* update start */
                                        *(unsigned long *) data[4],
                                        *(char **) data[5],
                                        *(int *) data[6],
                                        *(int *) data[7],
                                        *(double *) data[8],
                                        *(int *) data[9],
                                        *(char **) data[10],                                                  
                                        *(char **) data[11],             /* update end */  
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata_add() end\n");

        return result;
}


/************************************/
/* EVENTHANDLER                     */
/************************************/

int ido2db_query_insert_or_update_eventhandlerdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, eventhandler_type, object_id, state, state_type, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %d, %lu, %d, %d, %s, %lu, %s, %lu, %lu, '%s', '%s', %d, %d, %lf, %d, '%s', '%s') ON DUPLICATE KEY UPDATE eventhandler_type=%d, object_id=%lu, state=%d, state_type=%d, end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s', command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_EVENTHANDLERS],
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
                                        *(char **) data[17],		/* insert end */
                                        *(int *) data[1],		/* update start */
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
                                        *(char **) data[17]            /* update end */
			);
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET eventhandler_type=%d, object_id=%lu, state=%d, state_type=%d, end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s', command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s' WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_EVENTHANDLERS],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, eventhandler_type, object_id, state, state_type, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %d, %lu, %d, %d, %s, %lu, %s, %lu, %lu, '%s', '%s', %d, %d, %lf, %d, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_EVENTHANDLERS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND start_time=%s AND start_time_usec=%lu) WHEN MATCHED THEN UPDATE SET eventhandler_type=%d, object_id=%lu, state=%d, state_type=%d, end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s', command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s' WHEN NOT MATCHED THEN INSERT (instance_id, eventhandler_type, object_id, state, state_type, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (%lu, %d, %lu, %d, %d, %s, %lu, %s, %lu, %lu, '%s', '%s', %d, %d, %lf, %d, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_EVENTHANDLERS],
					*(unsigned long *) data[0],     /* unique constraint start */
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],      /* unique constraint end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata_add() end\n");

        return result;
}


/************************************/
/* NOTIFICATIONS                    */
/************************************/

int ido2db_query_insert_or_update_notificationdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, notification_type, notification_reason, start_time, start_time_usec, end_time, end_time_usec, object_id, state, output, long_output, escalated, contacts_notified) VALUES (%lu, %d, %d, %s, %lu, %s, %lu, %lu, %d, '%s', '%s', %d, %d) ON DUPLICATE KEY UPDATE notification_type=%d, notification_reason=%d, end_time=%s, end_time_usec=%lu, state=%d, output='%s', long_output='%s', escalated=%d, contacts_notified=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_NOTIFICATIONS],
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
                                        *(int *) data[12],  		/* insert end */
                                        *(int *) data[1],               /* update start */
                                        *(int *) data[2],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(int *) data[8],
                                        *(char **) data[9],
                                        *(char **) data[10],
                                        *(int *) data[11],
                                        *(int *) data[12]               /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET notification_type=%d, notification_reason=%d, end_time=%s, end_time_usec=%lu, state=%d, output='%s', long_output='%s', escalated=%d, contacts_notified=%d  WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu AND object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_NOTIFICATIONS],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, notification_type, notification_reason, start_time, start_time_usec, end_time, end_time_usec, object_id, state, output, long_output, escalated, contacts_notified) VALUES (%lu, %d, %d, %s, %lu, %s, %lu, %lu, %d, '%s', '%s', %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_NOTIFICATIONS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND start_time=%s AND start_time_usec=%lu AND object_id=%lu) WHEN MATCHED THEN UPDATE SET notification_type=%d, notification_reason=%d, end_time=%s, end_time_usec=%lu, state=%d, output='%s', long_output='%s', escalated=%d, contacts_notified=%d WHEN NOT MATCHED THEN INSERT (instance_id, notification_type, notification_reason, start_time, start_time_usec, end_time, end_time_usec, object_id, state, output, long_output, escalated, contacts_notified) VALUES (%lu, %d, %d, %s, %lu, %s, %lu, %lu, %d, '%s', '%s', %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_NOTIFICATIONS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(char **) data[3],
                                        *(unsigned long *) data[4],
                                        *(unsigned long *) data[7],      /* unique constraint end */
                                        *(int *) data[1],               /* update start */
                                        *(int *) data[2],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(int *) data[8],
                                        *(char **) data[9],
                                        *(char **) data[10],
                                        *(int *) data[11],
                                        *(int *) data[12],               /* update end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata_add() end\n");

        return result;
}


/************************************/
/* CONTACTNOTIFICATIONS             */
/************************************/

int ido2db_query_insert_or_update_contactnotificationdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, notification_id, start_time, start_time_usec, end_time, end_time_usec, contact_object_id) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu) ON DUPLICATE KEY UPDATE notification_id=%lu, end_time=%s, end_time_usec=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],     
                                        *(unsigned long *) data[6],     /* insert end */
                                        *(unsigned long *) data[1],     /* update start */
                                        *(char **) data[4],
                                        *(unsigned long *) data[5]      /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET notification_id=%lu, end_time=%s, end_time_usec=%lu WHERE instance_id=%lu AND contact_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
                                        *(unsigned long *) data[1],     /* update start */
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],     /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[6],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, notification_id, start_time, start_time_usec, end_time, end_time_usec, contact_object_id) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],     
                                        *(unsigned long *) data[6]      /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND contact_object_id=%lu AND start_time=%s AND start_time_usec=%lu) WHEN MATCHED THEN UPDATE SET notification_id=%lu, end_time=%s, end_time_usec=%lu WHEN NOT MATCHED THEN INSERT (instance_id, notification_id, start_time, start_time_usec, end_time, end_time_usec, contact_object_id) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[6],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],      /* unique constraint end */
                                        *(unsigned long *) data[1],     /* update start */
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],     /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],     
                                        *(unsigned long *) data[6]      /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationdata_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_contactnotificationmethoddata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationmethoddata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, contactnotification_id, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu, '%s') ON DUPLICATE KEY UPDATE end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],
                                        *(unsigned long *) data[6],     
                                        *(char **) data[7],     	/* insert end */
                                        *(char **) data[4],     	/* update start */
                                        *(unsigned long *) data[5],
                                        *(unsigned long *) data[6],
                                        *(char **) data[7]      	/* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s' WHERE instance_id=%lu AND contactnotification_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS],
                                        *(char **) data[4],             /* update start */
                                        *(unsigned long *) data[5],
                                        *(unsigned long *) data[6],
                                        *(char **) data[7],             /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, contactnotification_id, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],
                                        *(unsigned long *) data[6],     
                                        *(char **) data[7]             /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND contactnotification_id=%lu AND start_time=%s AND start_time_usec=%lu) WHEN MATCHED THEN UPDATE SET end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s' WHEN NOT MATCHED THEN INSERT (instance_id, contactnotification_id, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args) VALUES (%lu, %lu, %s, %lu, %s, %lu, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],      /* unique constraint end */
                                        *(char **) data[4],             /* update start */
                                        *(unsigned long *) data[5],
                                        *(unsigned long *) data[6],
                                        *(char **) data[7],             /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],
                                        *(unsigned long *) data[6],     
                                        *(char **) data[7]             /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationmethoddata_add() end\n");

        return result;
}


/************************************/
/* SERVICECHECKS                    */
/************************************/

int ido2db_query_insert_or_update_servicecheckdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicecheckdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, service_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata, command_object_id, command_args, command_line) VALUES (%lu, %lu, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, '%s', '%s', '%s', %lu, '%s', '%s') ON DUPLICATE KEY UPDATE check_type='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECHECKS],
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
                                        *(char **) data[21],     	/* insert end */
                                        *(int *) data[2],               /* update start */
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
                                        *(char **) data[18]            /* updapte end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET check_type='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s' WHERE instance_id=%lu AND service_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECHECKS],
                                        *(int *) data[2],               /* update start */
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
                                        *(char **) data[18],            /* updapte end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned *) data[1],
                                        *(char **) data[7],
                                        *(unsigned long *) data[8]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, service_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata, command_object_id, command_args, command_line) VALUES (%lu, %lu, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, '%s', '%s', '%s', %lu, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECHECKS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND service_object_id=%lu AND start_time=%s AND start_time_usec=%lu) WHEN MATCHED THEN UPDATE SET check_type='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s' WHEN NOT MATCHED THEN INSERT (instance_id, service_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata, command_object_id, command_args, command_line) VALUES (%lu, %lu, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, '%s', '%s', '%s', %lu, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECHECKS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned *) data[1],
                                        *(char **) data[7],
                                        *(unsigned long *) data[8],      /* unique constraint end */
                                        *(int *) data[2],               /* update start */
                                        *(int *) data[3],
                                        *(int *) data[4],
                                        *(int *) data[5],
                                        *(int *) data[6],
                                        *(char **) data[9],
                                        *(unsigned long *) data[10],
                                        *(int *) data[11],
                                        *(int *) data[12],
                                        *(double *) data[13],
                                        *(double *) data[14],
                                        *(int *) data[15],
                                        *(char **) data[16],
                                        *(char **) data[17],
                                        *(char **) data[18],            /* updapte end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicecheckdata_add() end\n");

        return result;
}


/************************************/
/* HOSTCHECKS                       */
/************************************/

int ido2db_query_insert_or_update_hostcheckdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostcheckdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (command_object_id, command_args, command_line, instance_id, host_object_id, check_type, is_raw_check, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata) VALUES (%lu, '%s', '%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, '%s', '%s', '%s') ON DUPLICATE KEY UPDATE check_type='%d', is_raw_check='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCHECKS],
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
                                        *(char **) data[22],		/* insert end */
                                        *(int *) data[5],		/* update start */
                                        *(int *) data[6],
                                        *(int *) data[7],
                                        *(int *) data[8],
                                        *(int *) data[9],
                                        *(int *) data[10],
                                        *(char **) data[13],
                                        *(unsigned long *) data[14],
                                        *(int *) data[15],
                                        *(int *) data[16],
                                        *(double *) data[17],
                                        *(double *) data[18],
                                        *(int *) data[19],
                                        *(char **) data[20],
                                        *(char **) data[21],
                                        *(char **) data[22]            /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET check_type='%d', is_raw_check='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s' WHERE instance_id=%lu AND host_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCHECKS],
                                        *(int *) data[5],               /* update start */
                                        *(int *) data[6],
                                        *(int *) data[7],
                                        *(int *) data[8],
                                        *(int *) data[9],
                                        *(int *) data[10],
                                        *(char **) data[13],
                                        *(unsigned long *) data[14],
                                        *(int *) data[15],
                                        *(int *) data[16], 
                                        *(double *) data[17],
                                        *(double *) data[18],
                                        *(int *) data[19],
                                        *(char **) data[20],
                                        *(char **) data[21],
                                        *(char **) data[22],            /* update end */
                                        *(unsigned long *) data[3],     /* unique constraint start */
                                        *(unsigned long *) data[4],
                                        *(char **) data[11],
                                        *(unsigned long *) data[12]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (command_object_id, command_args, command_line, instance_id, host_object_id, check_type, is_raw_check, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata) VALUES (%lu, '%s', '%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, '%s', '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCHECKS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND host_object_id=%lu AND start_time=%s AND start_time_usec=%lu) WHEN MATCHED THEN UPDATE SET check_type='%d', is_raw_check='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s' WHEN NOT MATCHED THEN INSERT (command_object_id, command_args, command_line, instance_id, host_object_id, check_type, is_raw_check, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata) VALUES (%lu, '%s', '%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, '%s', '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCHECKS],
                                        *(unsigned long *) data[3],     /* unique constraint start */
                                        *(unsigned long *) data[4],
                                        *(char **) data[11],
                                        *(unsigned long *) data[12],      /* unique constraint end */
                                        *(int *) data[5],               /* update start */
                                        *(int *) data[6],
                                        *(int *) data[7],
                                        *(int *) data[8],
                                        *(int *) data[9],
                                        *(int *) data[10],
                                        *(char **) data[13],
                                        *(unsigned long *) data[14],
                                        *(int *) data[15],
                                        *(int *) data[16], 
                                        *(double *) data[17],
                                        *(double *) data[18],
                                        *(int *) data[19],
                                        *(char **) data[20],
                                        *(char **) data[21],
                                        *(char **) data[22],            /* update end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostcheckdata_add() end\n");

        return result;
}


/************************************/
/* COMMENTS                         */
/************************************/

int ido2db_query_insert_or_update_commentdata_add(ndo2db_idi *idi, void **data, char *table_name) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commentdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (entry_time, entry_time_usec, instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time) VALUES (%s, %lu, %lu, %d, %d, %lu, %s, %lu, '%s', '%s', %d, %d, %d, %s) ON DUPLICATE KEY UPDATE comment_type=%d, entry_type=%d, object_id=%lu, author_name='%s', comment_data='%s', is_persistent=%d, comment_source=%d, expires=%d, expiration_time=%s",
                                        table_name,
                                        *(char **) data[0],     	/* insert start */
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
                                        *(char **) data[13],     	/* insert end */
                                        *(int *) data[3],		/* update start */
                                        *(int *) data[4],
                                        *(unsigned long *) data[5],
                                        *(char **) data[8],
                                        *(char **) data[9],
                                        *(int *) data[10],
                                        *(int *) data[11],
                                        *(int *) data[12],
                                        *(char **) data[13]             /* end end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET comment_type=%d, entry_type=%d, object_id=%lu, author_name='%s', comment_data='%s', is_persistent=%d, comment_source=%d, expires=%d, expiration_time=%s WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
                                        table_name,
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (entry_time, entry_time_usec, instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time) VALUES (%s, %lu, %lu, %d, %d, %lu, %s, %lu, '%s', '%s', %d, %d, %d, %s)",
                                        table_name,
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu) WHEN MATCHED THEN UPDATE SET comment_type=%d, entry_type=%d, object_id=%lu, author_name='%s', comment_data='%s', is_persistent=%d, comment_source=%d, expires=%d, expiration_time=%s WHEN NOT MATCHED THEN INSERT (entry_time, entry_time_usec, instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time) VALUES (%s, %lu, %lu, %d, %d, %lu, %s, %lu, '%s', '%s', %d, %d, %d, %s)",
                                        table_name,
                                        *(unsigned long *) data[2],     /* unique constraint start */
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],      /* unique constraint end */
                                        *(int *) data[3],               /* update start */
                                        *(int *) data[4],
                                        *(unsigned long *) data[5],
                                        *(char **) data[8],
                                        *(char **) data[9],
                                        *(int *) data[10],
                                        *(int *) data[11],
                                        *(int *) data[12],
                                        *(char **) data[13],            /* end end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commentdata_add() end\n");

        return result;
}


/************************************/
/* DOWNTIME                         */
/************************************/

int ido2db_query_insert_or_update_downtimedata_add(ndo2db_idi *idi, void **data, char *table_name) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time) VALUES (%lu, %d, %lu, %s, '%s', '%s', %lu, %lu, %d, %lu, %s, %s) ON DUPLICATE KEY UPDATE downtime_type=%d, author_name='%s', comment_data='%s', triggered_by_id=%lu, is_fixed=%d, duration=%lu, scheduled_start_time=%s, scheduled_end_time=%s ",
                                        table_name,
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(unsigned long *) data[7],
                                        *(int *) data[8],
                                        *(unsigned long *) data[9],
                                        *(char **) data[10],
                                        *(char **) data[11],     	/* insert end */
                                        *(int *) data[1],               /* update start */
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[7],
                                        *(int *) data[8],
                                        *(unsigned long *) data[9],
                                        *(char **) data[10],
                                        *(char **) data[11]             /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET downtime_type=%d, author_name='%s', comment_data='%s', triggered_by_id=%lu, is_fixed=%d, duration=%lu, scheduled_start_time=%s, scheduled_end_time=%s WHERE instance_id=%lu AND object_id=%lu AND entry_time=%s AND internal_downtime_id=%lu",
                                        table_name,
                                        *(int *) data[1],               /* update start */
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[7],
                                        *(int *) data[8],
                                        *(unsigned long *) data[9],
                                        *(char **) data[10],
                                        *(char **) data[11],            /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],
                                        *(unsigned long *) data[6]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time) VALUES (%lu, %d, %lu, %s, '%s', '%s', %lu, %lu, %d, %lu, %s, %s)",
                                        table_name,
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(unsigned long *) data[7],
                                        *(int *) data[8],
                                        *(unsigned long *) data[9],
                                        *(char **) data[10],
                                        *(char **) data[11]             /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND object_id=%lu AND entry_time=%s AND internal_downtime_id=%lu) WHEN MATCHED THEN UPDATE SET downtime_type=%d, author_name='%s', comment_data='%s', triggered_by_id=%lu, is_fixed=%d, duration=%lu, scheduled_start_time=%s, scheduled_end_time=%s WHEN NOT MATCHED THEN INSERT (instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time) VALUES (%lu, %d, %lu, %s, '%s', '%s', %lu, %lu, %d, %lu, %s, %s)",
                                        table_name,
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],
                                        *(unsigned long *) data[6],      /* unique constraint end */
                                        *(int *) data[1],               /* update start */
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[7],
                                        *(int *) data[8],
                                        *(unsigned long *) data[9],
                                        *(char **) data[10],
                                        *(char **) data[11],            /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(unsigned long *) data[7],
                                        *(int *) data[8],
                                        *(unsigned long *) data[9],
                                        *(char **) data[10],
                                        *(char **) data[11]             /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_add() end\n");

        return result;
}


/************************************/
/* PROGRAMSTATUS                    */
/************************************/

int ido2db_query_insert_or_update_programstatusdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_programstatusdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, status_update_time, program_start_time, is_currently_running, process_id, daemon_mode, last_command_check, last_log_rotation, notifications_enabled, active_service_checks_enabled, passive_service_checks_enabled, active_host_checks_enabled, passive_host_checks_enabled, event_handlers_enabled, flap_detection_enabled, failure_prediction_enabled, process_performance_data, obsess_over_hosts, obsess_over_services, modified_host_attributes, modified_service_attributes, global_host_event_handler, global_service_event_handler) VALUES (%lu, %s, %s, '1', %lu, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lu, %lu, '%s', '%s') ON DUPLICATE KEY UPDATE status_update_time=%s, program_start_time=%s, is_currently_running=1, process_id=%lu, daemon_mode=%d, last_command_check=%s, last_log_rotation=%s, notifications_enabled=%d, active_service_checks_enabled=%d, passive_service_checks_enabled=%d, active_host_checks_enabled=%d, passive_host_checks_enabled=%d, event_handlers_enabled=%d, flap_detection_enabled=%d, failure_prediction_enabled=%d, process_performance_data=%d, obsess_over_hosts=%d, obsess_over_services=%d, modified_host_attributes=%lu, modified_service_attributes=%lu, global_host_event_handler='%s', global_service_event_handler='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS],
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
                                        *(char **) data[21],		/* insert end */
                                        *(char **) data[1],		/* update start */
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
                                        *(char **) data[21]            /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET status_update_time=%s, program_start_time=%s, is_currently_running=1, process_id=%lu, daemon_mode=%d, last_command_check=%s, last_log_rotation=%s, notifications_enabled=%d, active_service_checks_enabled=%d, passive_service_checks_enabled=%d, active_host_checks_enabled=%d, passive_host_checks_enabled=%d, event_handlers_enabled=%d, flap_detection_enabled=%d, failure_prediction_enabled=%d, process_performance_data=%d, obsess_over_hosts=%d, obsess_over_services=%d, modified_host_attributes=%lu, modified_service_attributes=%lu, global_host_event_handler='%s', global_service_event_handler='%s' WHERE instance_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS],
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
                                        *(char **) data[21],            /* update end */
                                        *(unsigned long *) data[0]      /* unique constraint start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, status_update_time, program_start_time, is_currently_running, process_id, daemon_mode, last_command_check, last_log_rotation, notifications_enabled, active_service_checks_enabled, passive_service_checks_enabled, active_host_checks_enabled, passive_host_checks_enabled, event_handlers_enabled, flap_detection_enabled, failure_prediction_enabled, process_performance_data, obsess_over_hosts, obsess_over_services, modified_host_attributes, modified_service_attributes, global_host_event_handler, global_service_event_handler) VALUES (%lu, %s, %s, '1', %lu, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lu, %lu, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS],
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
                                        *(char **) data[21]             /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu) WHEN MATCHED THEN UPDATE SET status_update_time=%s, program_start_time=%s, is_currently_running=1, process_id=%lu, daemon_mode=%d, last_command_check=%s, last_log_rotation=%s, notifications_enabled=%d, active_service_checks_enabled=%d, passive_service_checks_enabled=%d, active_host_checks_enabled=%d, passive_host_checks_enabled=%d, event_handlers_enabled=%d, flap_detection_enabled=%d, failure_prediction_enabled=%d, process_performance_data=%d, obsess_over_hosts=%d, obsess_over_services=%d, modified_host_attributes=%lu, modified_service_attributes=%lu, global_host_event_handler='%s', global_service_event_handler='%s' WHEN NOT MATCHED THEN INSERT (instance_id, status_update_time, program_start_time, is_currently_running, process_id, daemon_mode, last_command_check, last_log_rotation, notifications_enabled, active_service_checks_enabled, passive_service_checks_enabled, active_host_checks_enabled, passive_host_checks_enabled, event_handlers_enabled, flap_detection_enabled, failure_prediction_enabled, process_performance_data, obsess_over_hosts, obsess_over_services, modified_host_attributes, modified_service_attributes, global_host_event_handler, global_service_event_handler) VALUES (%lu, %s, %s, '1', %lu, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lu, %lu, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS],
                                        *(unsigned long *) data[0],      /* unique constraint start/end */
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
                                        *(char **) data[21],            /* update end */
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
                                        *(char **) data[21]             /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_programstatusdata_add() end\n");

        return result;
}


/************************************/
/* HOSTSTATUS                       */
/************************************/

int ido2db_query_insert_or_update_hoststatusdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, host_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_up, last_time_down, last_time_unreachable, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_host, modified_host_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (%lu, %lu, %s, '%s', '%s', '%s', %d, %d, %d, %d, %d, %s, %s, %d, %s, %s, %d, %s, %s, %s, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %lf, %d, %d, %d, %d, %lu, '%s', '%s', %lf, %lf, %lu) ON DUPLICATE KEY UPDATE instance_id='%lu', host_object_id='%lu', status_update_time=%s, output='%s', long_output='%s', perfdata='%s', current_state='%d', has_been_checked='%d', should_be_scheduled='%d', current_check_attempt='%d', max_check_attempts='%d', last_check=%s, next_check=%s, check_type='%d', last_state_change=%s, last_hard_state_change=%s, last_hard_state='%d', last_time_up=%s, last_time_down=%s, last_time_unreachable=%s, state_type='%d', last_notification=%s, next_notification=%s, no_more_notifications='%d', notifications_enabled='%d', problem_has_been_acknowledged='%d', acknowledgement_type='%d', current_notification_number='%d', passive_checks_enabled='%d', active_checks_enabled='%d', event_handler_enabled='%d', flap_detection_enabled='%d', is_flapping='%d', percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_host='%d', modified_host_attributes='%lu', event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTSTATUS],
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
                                        *(unsigned long *) data[45],     /* insert end */
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
                                        *(unsigned long *) data[45]     /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%lu', host_object_id='%lu', status_update_time=%s, output='%s', long_output='%s', perfdata='%s', current_state='%d', has_been_checked='%d', should_be_scheduled='%d', current_check_attempt='%d', max_check_attempts='%d', last_check=%s, next_check=%s, check_type='%d', last_state_change=%s, last_hard_state_change=%s, last_hard_state='%d', last_time_up=%s, last_time_down=%s, last_time_unreachable=%s, state_type='%d', last_notification=%s, next_notification=%s, no_more_notifications='%d', notifications_enabled='%d', problem_has_been_acknowledged='%d', acknowledgement_type='%d', current_notification_number='%d', passive_checks_enabled='%d', active_checks_enabled='%d', event_handler_enabled='%d', flap_detection_enabled='%d', is_flapping='%d', percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_host='%d', modified_host_attributes='%lu', event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id='%lu' WHERE host_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTSTATUS],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
                        if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, host_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_up, last_time_down, last_time_unreachable, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_host, modified_host_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (%lu, %lu, %s, '%s', '%s', '%s', %d, %d, %d, %d, %d, %s, %s, %d, %s, %s, %d, %s, %s, %s, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %lf, %d, %d, %d, %d, %lu, '%s', '%s', %lf, %lf, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTSTATUS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (host_object_id=%lu) WHEN MATCHED THEN UPDATE SET instance_id='%lu', status_update_time=%s, output='%s', long_output='%s', perfdata='%s', current_state='%d', has_been_checked='%d', should_be_scheduled='%d', current_check_attempt='%d', max_check_attempts='%d', last_check=%s, next_check=%s, check_type='%d', last_state_change=%s, last_hard_state_change=%s, last_hard_state='%d', last_time_up=%s, last_time_down=%s, last_time_unreachable=%s, state_type='%d', last_notification=%s, next_notification=%s, no_more_notifications='%d', notifications_enabled='%d', problem_has_been_acknowledged='%d', acknowledgement_type='%d', current_notification_number='%d', passive_checks_enabled='%d', active_checks_enabled='%d', event_handler_enabled='%d', flap_detection_enabled='%d', is_flapping='%d', percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_host='%d', modified_host_attributes='%lu', event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, host_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_up, last_time_down, last_time_unreachable, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_host, modified_host_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (%lu, %lu, %s, '%s', '%s', '%s', %d, %d, %d, %d, %d, %s, %s, %d, %s, %s, %d, %s, %s, %s, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %lf, %d, %d, %d, %d, %lu, '%s', '%s', %lf, %lf, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTSTATUS],
                                        *(unsigned long *) data[1],     /* unique constraint start/end */
                                        *(unsigned long *) data[0],     /* update start */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() end\n");

        return result;
}

/************************************/
/* SERVICESTATUS                    */
/************************************/

int ido2db_query_insert_or_update_servicestatusdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatusdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, service_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_ok, last_time_warning, last_time_unknown, last_time_critical, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_service, modified_service_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES ('%lu', '%lu', %s, '%s', '%s', '%s', '%d', '%d', '%d', '%d', '%d', %s, %s, '%d', %s, %s, '%d', %s, %s, %s, %s, '%d', %s, %s, '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%lf', '%lf', '%lf', '%d', '%d', '%d', '%d', '%lu', '%s', '%s', '%lf', '%lf', '%lu') ON DUPLICATE KEY UPDATE instance_id='%lu', service_object_id='%lu', status_update_time=%s, output='%s', long_output='%s', perfdata='%s', current_state='%d', has_been_checked='%d', should_be_scheduled='%d', current_check_attempt='%d', max_check_attempts='%d', last_check=%s, next_check=%s, check_type=%d, last_state_change=%s, last_hard_state_change=%s, last_hard_state='%d', last_time_ok=%s, last_time_warning=%s, last_time_unknown=%s, last_time_critical=%s, state_type='%d', last_notification=%s, next_notification=%s, no_more_notifications='%d', notifications_enabled='%d', problem_has_been_acknowledged='%d', acknowledgement_type='%d', current_notification_number='%d', passive_checks_enabled='%d', active_checks_enabled='%d', event_handler_enabled='%d', flap_detection_enabled='%d', is_flapping='%d', percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_service='%d', modified_service_attributes='%lu', event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICESTATUS],
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
                                        *(unsigned long *) data[46],     /* insert end */
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
                                        *(unsigned long *) data[46]     /* update end */                       
			);
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%lu', service_object_id='%lu', status_update_time=%s, output='%s', long_output='%s', perfdata='%s', current_state='%d', has_been_checked='%d', should_be_scheduled='%d', current_check_attempt='%d', max_check_attempts='%d', last_check=%s, next_check=%s, check_type='%d', last_state_change=%s, last_hard_state_change=%s, last_hard_state='%d', last_time_ok=%s, last_time_warning=%s, last_time_unknown=%s, last_time_critical=%s, state_type='%d', last_notification=%s, next_notification=%s, no_more_notifications='%d', notifications_enabled='%d', problem_has_been_acknowledged='%d', acknowledgement_type='%d', current_notification_number='%d', passive_checks_enabled='%d', active_checks_enabled='%d', event_handler_enabled='%d', flap_detection_enabled='%d', is_flapping='%d', percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_service='%d', modified_service_attributes='%lu', event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id='%lu' WHERE service_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICESTATUS],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, service_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_ok, last_time_warning, last_time_unknown, last_time_critical, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_service, modified_service_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES ('%lu', '%lu', %s, '%s', '%s', '%s', '%d', '%d', '%d', '%d', '%d', %s, %s, '%d', %s, %s, '%d', %s, %s, %s, %s, '%d', %s, %s, '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%lf', '%lf', '%lf', '%d', '%d', '%d', '%d', '%lu', '%s', '%s', '%lf', '%lf', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICESTATUS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (service_object_id='%lu') WHEN MATCHED THEN UPDATE SET instance_id='%lu', status_update_time=%s, output='%s', long_output='%s', perfdata='%s', current_state='%d', has_been_checked='%d', should_be_scheduled='%d', current_check_attempt='%d', max_check_attempts='%d', last_check=%s, next_check=%s, check_type='%d', last_state_change=%s, last_hard_state_change=%s, last_hard_state='%d', last_time_ok=%s, last_time_warning=%s, last_time_unknown=%s, last_time_critical=%s, state_type='%d', last_notification=%s, next_notification=%s, no_more_notifications='%d', notifications_enabled='%d', problem_has_been_acknowledged='%d', acknowledgement_type='%d', current_notification_number='%d', passive_checks_enabled='%d', active_checks_enabled='%d', event_handler_enabled='%d', flap_detection_enabled='%d', is_flapping='%d', percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_service='%d', modified_service_attributes='%lu', event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, service_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_ok, last_time_warning, last_time_unknown, last_time_critical, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_service, modified_service_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES ('%lu', '%lu', %s, '%s', '%s', '%s', '%d', '%d', '%d', '%d', '%d', %s, %s, '%d', %s, %s, '%d', %s, %s, %s, %s, '%d', %s, %s, '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%lf', '%lf', '%lf', '%d', '%d', '%d', '%d', '%lu', '%s', '%s', '%lf', '%lf', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICESTATUS],
                                        *(unsigned long *) data[1],     /* unique constraint start/end */
                                        *(unsigned long *) data[0],     /* update start */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicestatusdata_add() end\n");

        return result;
}


/************************************/
/* CONTACTSTATUS                    */
/************************************/

int ido2db_query_insert_or_update_contactstatusdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactstatusdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, contact_object_id, status_update_time, host_notifications_enabled, service_notifications_enabled, last_host_notification, last_service_notification, modified_attributes, modified_host_attributes, modified_service_attributes) VALUES (%lu, %lu, %s, %d, %d, %s, %s, %lu, %lu, %lu) ON DUPLICATE KEY UPDATE instance_id=%lu, status_update_time=%s, host_notifications_enabled=%d, service_notifications_enabled=%d, last_host_notification=%s, last_service_notification=%s, modified_attributes=%lu, modified_host_attributes=%lu, modified_service_attributes=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTSTATUS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(int *) data[3],
                                        *(int *) data[4],
                                        *(char **) data[5],
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],
                                        *(unsigned long *) data[8],
                                        *(unsigned long *) data[9],     /* insert end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(char **) data[2],
                                        *(int *) data[3],
                                        *(int *) data[4],
                                        *(char **) data[5],
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],
                                        *(unsigned long *) data[8],
                                        *(unsigned long *) data[9]     /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id=%lu, status_update_time=%s, host_notifications_enabled=%d, service_notifications_enabled=%d, last_host_notification=%s, last_service_notification=%s, modified_attributes=%lu, modified_host_attributes=%lu, modified_service_attributes=%lu WHERE contact_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTSTATUS],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, contact_object_id, status_update_time, host_notifications_enabled, service_notifications_enabled, last_host_notification, last_service_notification, modified_attributes, modified_host_attributes, modified_service_attributes) VALUES (%lu, %lu, %s, %d, %d, %s, %s, %lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTSTATUS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (contact_object_id=%lu) WHEN MATCHED THEN UPDATE SET instance_id=%lu, status_update_time=%s, host_notifications_enabled=%d, service_notifications_enabled=%d, last_host_notification=%s, last_service_notification=%s, modified_attributes=%lu, modified_host_attributes=%lu, modified_service_attributes=%lu WHEN NOT MATCHED THEN INSERT (instance_id, contact_object_id, status_update_time, host_notifications_enabled, service_notifications_enabled, last_host_notification, last_service_notification, modified_attributes, modified_host_attributes, modified_service_attributes) VALUES (%lu, %lu, %s, %d, %d, %s, %s, %lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTSTATUS],
                                        *(unsigned long *) data[1],     /* unique constraint start/end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(char **) data[2],
                                        *(int *) data[3],
                                        *(int *) data[4],
                                        *(char **) data[5],
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],
                                        *(unsigned long *) data[8],
                                        *(unsigned long *) data[9],     /* update end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactstatusdata_add() end\n");

        return result;
}


/************************************/
/* CONFIGFILEVARIABLES              */
/************************************/

int ido2db_query_insert_or_update_configfilevariables_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_configfilevariables_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, configfile_type, configfile_path) VALUES (%lu, %d, '%s') ON DUPLICATE KEY UPDATE instance_id='%lu', configfile_type='%d', configfile_path='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(char **) data[2],		/* insert end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(int *) data[1],
                                        *(char **) data[2]		/* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%lu', configfile_type='%d', configfile_path='%s' WHERE instance_id=%lu AND configfile_type=%d AND configfile_path='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILES],
                                        *(unsigned long *) data[0],     /* update start */
                                        *(int *) data[1],
                                        *(char **) data[2],              /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(char **) data[2]             /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
			
			ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_configfilevariables_add(%lu) update rows affected\n", dbi_result_get_numrows_affected(idi->dbinfo.dbi_result));

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, configfile_type, configfile_path) VALUES (%lu, %d, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(char **) data[2]             /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */

                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu) WHEN MATCHED THEN UPDATE SET configfile_type='%d', configfile_path='%s' WHEN NOT MATCHED THEN INSERT (instance_id, configfile_type, configfile_path) VALUES (%lu, %d, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILES],
                                        *(unsigned long *) data[0],     /* unique constraint start/end */
                                        *(int *) data[1],		/* update start */
                                        *(char **) data[2],             /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(char **) data[2]             /* insert end */
                        );

                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_configfilevariables_add() end\n");

        return result;
}


/************************************/
/* RUNTIMEVARIABLES                 */
/************************************/

int ido2db_query_insert_or_update_runtimevariables_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_runtimevariables_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, varname, varvalue) VALUES (%lu, '%s', '%s') ON DUPLICATE KEY UPDATE varvalue='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_RUNTIMEVARIABLES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(char **) data[1],
                                        *(char **) data[2],		/* insert end */
                                        *(char **) data[2]             /* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET varvalue='%s' WHERE instance_id=%lu AND varname='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_RUNTIMEVARIABLES],
                                        *(char **) data[2],             /* update start/end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(char **) data[1]		/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, varname, varvalue) VALUES (%lu, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_RUNTIMEVARIABLES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(char **) data[1],
                                        *(char **) data[2]             /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND varname='%s') WHEN MATCHED THEN UPDATE SET varvalue='%s' WHEN NOT MATCHED THEN INSERT (instance_id, varname, varvalue) VALUES (%lu, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_RUNTIMEVARIABLES],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(char **) data[1],		/* unique constraint end */
                                        *(char **) data[2],             /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(char **) data[1],
                                        *(char **) data[2]             /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_runtimevariables_add() end\n");

        return result;
}


/************************************/
/* HOSTDEFINITION                   */
/************************************/

int ido2db_query_insert_or_update_hostdefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, host_object_id, alias, display_name, address, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_down, notify_on_unreachable, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_up, stalk_on_down, stalk_on_unreachable, flap_detection_enabled, flap_detection_on_up, flap_detection_on_down, flap_detection_on_unreachable, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_host, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt, vrml_image, statusmap_image, have_2d_coords, x_2d, y_2d, have_3d_coords, x_3d, y_3d, z_3d) VALUES (%lu, %d, %lu, '%s', '%s', '%s', %lu, '%s', %lu, '%s', %lu, %lu, '%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %d, %d, %d, %lf, %lf, %lf) ON DUPLICATE KEY UPDATE alias='%s', display_name='%s', address='%s', check_command_object_id=%lu, check_command_args='%s', eventhandler_command_object_id=%lu, eventhandler_command_args='%s', check_timeperiod_object_id=%lu, notification_timeperiod_object_id=%lu, failure_prediction_options='%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_down=%d, notify_on_unreachable=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_up=%d, stalk_on_down=%d, stalk_on_unreachable=%d, flap_detection_enabled=%d, flap_detection_on_up=%d, flap_detection_on_down=%d, flap_detection_on_unreachable=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_host=%d, failure_prediction_enabled=%d, notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s', vrml_image='%s', statusmap_image='%s', have_2d_coords=%d, x_2d=%d, y_2d=%d, have_3d_coords=%d, x_3d=%lf, y_3d=%lf, z_3d=%lf",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(char **) data[7],
                                        *(unsigned long *) data[8],
                                        *(char **) data[9],
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
                                        *(double *) data[56],		/* insert end */
                                        *(char **) data[3],		/* update start */
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(char **) data[7],
                                        *(unsigned long *) data[8],
                                        *(char **) data[9],
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
                                        *(double *) data[56]           /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET alias='%s', display_name='%s', address='%s', check_command_object_id=%lu, check_command_args='%s', eventhandler_command_object_id=%lu, eventhandler_command_args='%s', check_timeperiod_object_id=%lu, notification_timeperiod_object_id=%lu, failure_prediction_options='%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_down=%d, notify_on_unreachable=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_up=%d, stalk_on_down=%d, stalk_on_unreachable=%d, flap_detection_enabled=%d, flap_detection_on_up=%d, flap_detection_on_down=%d, flap_detection_on_unreachable=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_host=%d, failure_prediction_enabled=%d, notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s', vrml_image='%s', statusmap_image='%s', have_2d_coords=%d, x_2d=%d, y_2d=%d, have_3d_coords=%d, x_3d=%lf, y_3d=%lf, z_3d=%lf WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTS],
                                        *(char **) data[3],             /* update start */
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(char **) data[7],
                                        *(unsigned long *) data[8],
                                        *(char **) data[9],
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
                                        *(double *) data[56],           /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, alias, display_name, address, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_down, notify_on_unreachable, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_up, stalk_on_down, stalk_on_unreachable, flap_detection_enabled, flap_detection_on_up, flap_detection_on_down, flap_detection_on_unreachable, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_host, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt, vrml_image, statusmap_image, have_2d_coords, x_2d, y_2d, have_3d_coords, x_3d, y_3d, z_3d) VALUES (%lu, %d, %lu, '%s', '%s', '%s', %lu, '%s', %lu, '%s', %lu, %lu, '%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %d, %d, %d, %lf, %lf, %lf)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(char **) data[7],
                                        *(unsigned long *) data[8],
                                        *(char **) data[9],
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
                                        *(double *) data[56]           /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND config_type=%d AND host_object_id=%lu) WHEN MATCHED THEN UPDATE SET alias='%s', display_name='%s', address='%s', check_command_object_id=%lu, check_command_args='%s', eventhandler_command_object_id=%lu, eventhandler_command_args='%s', check_timeperiod_object_id=%lu, notif_timeperiod_object_id=%lu, failure_prediction_options='%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_down=%d, notify_on_unreachable=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_up=%d, stalk_on_down=%d, stalk_on_unreachable=%d, flap_detection_enabled=%d, flap_detection_on_up=%d, flap_detection_on_down=%d, flap_detection_on_unreachable=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_host=%d, failure_prediction_enabled=%d, notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s', vrml_image='%s', statusmap_image='%s', have_2d_coords=%d, x_2d=%d, y_2d=%d, have_3d_coords=%d, x_3d=%lf, y_3d=%lf, z_3d=%lf WHEN NOT MATCHED THEN INSERT (instance_id, config_type, host_object_id, alias, display_name, address, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notif_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_down, notify_on_unreachable, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_up, stalk_on_down, stalk_on_unreachable, flap_detection_enabled, flap_detection_on_up, flap_detection_on_down, flap_detection_on_unreachable, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_host, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt, vrml_image, statusmap_image, have_2d_coords, x_2d, y_2d, have_3d_coords, x_3d, y_3d, z_3d) VALUES (%lu, %d, %lu, '%s', '%s', '%s', %lu, '%s', %lu, '%s', %lu, %lu, '%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %d, %d, %d, %lf, %lf, %lf)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],      /* unique constraint end */
                                        *(char **) data[3],             /* update start */
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(char **) data[7],
                                        *(unsigned long *) data[8],
                                        *(char **) data[9],
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
                                        *(double *) data[56],           /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5],
                                        *(unsigned long *) data[6],
                                        *(char **) data[7],
                                        *(unsigned long *) data[8],
                                        *(char **) data[9],
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
                                        *(double *) data[56]           /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_definition_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_hostdefinition_parenthosts_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_parenthosts_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, host_id, parent_host_object_id) VALUES (%lu, %lu, %lu) ON DUPLICATE KEY UPDATE instance_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTPARENTHOSTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0]      /* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE host_id=%lu AND parent_host_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTPARENTHOSTS],
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[1],    	/* unique constraint start */
                                        *(unsigned long *) data[2]	/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, host_id, parent_host_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTPARENTHOSTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (host_id=%lu AND parent_host_object_id=%lu) WHEN MATCHED THEN UPDATE SET instance_id=%lu WHEN NOT MATCHED THEN INSERT (instance_id, host_id, parent_host_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTPARENTHOSTS],
                                        *(unsigned long *) data[1],    	/* unique constraint start */
                                        *(unsigned long *) data[2],	/* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_parenthosts_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_hostdefinition_contactgroups_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_contactgroups_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, host_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu') ON DUPLICATE KEY UPDATE instance_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0]     	/* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%lu' WHERE host_id='%lu' AND contactgroup_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2]     /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, host_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (host_id='%lu' AND contactgroup_object_id='%lu') WHEN MATCHED THEN UPDATE SET instance_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, host_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTGROUPS],
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2],     /* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_contactgroups_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_hostdefinition_contacts_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_contacts_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, host_id, contact_object_id) VALUES (%lu, %lu, %lu) ON DUPLICATE KEY UPDATE instance_id='%d', host_id='%lu', contact_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(unsigned long *) data[1],
                                        *(unsigned long *) data[2]      /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%d', host_id='%lu', contact_object_id='%lu' WHERE instance_id=%lu AND host_id=%lu AND contact_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTS],
                                        *(unsigned long *) data[0],     /* update start */
                                        *(unsigned long *) data[1],
                                        *(unsigned long *) data[2],     /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],
                                        *(unsigned long *) data[2]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, host_id, contact_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu) WHEN MATCHED THEN UPDATE SET host_id='%lu', contact_object_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, host_id, contact_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTS],
                                        *(unsigned long *) data[0],     /* unique constraint start/end */
                                        *(unsigned long *) data[1],	/* update start */
                                        *(unsigned long *) data[2],     /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdefinition_contacts_add() end\n");

        return result;
}


/************************************/
/* HOSTGROUPDEFINITION              */
/************************************/

int ido2db_query_insert_or_update_hostgroupdefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostgroupdefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, hostgroup_object_id, alias) VALUES (%lu, %d, %lu, '%s') ON DUPLICATE KEY UPDATE config_type=%d, alias='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],		/* insert end */
                                        *(int *) data[1],		/* update start */
                                        *(char **) data[3]             /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET config_type=%d, alias='%s' WHERE instance_id=%lu AND hostgroup_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPS],
                                        *(int *) data[1],		/* update start */
                                        *(char **) data[3],             /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[2]	/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, hostgroup_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3]             /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND hostgroup_object_id=%lu) WHEN MATCHED THEN UPDATE SET config_type=%d, alias='%s' WHEN NOT MATCHED THEN INSERT (instance_id, config_type, hostgroup_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[2],	/* unique constraint end */
                                        *(int *) data[1],		/* update start */
                                        *(char **) data[3],             /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3]             /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostgroupdefinition_definition_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_hostgroupdefinition_hostgroupmembers_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostgroupdefinition_hostgroupmembers_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, hostgroup_id, host_object_id) VALUES (%lu, %lu, %lu) ON DUPLICATE KEY UPDATE instance_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPMEMBERS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0]     /* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE hostgroup_id=%lu AND host_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPMEMBERS],
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, hostgroup_id, host_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPMEMBERS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (hostgroup_id=%lu AND host_object_id=%lu) WHEN MATCHED THEN UPDATE SET instance_id=%lu WHEN NOT MATCHED THEN INSERT (instance_id, hostgroup_id, host_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPMEMBERS],
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2],      /* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostgroupdefinition_hostgroupmembers_add() end\n");

        return result;
}


/************************************/
/* SERVICEDEFINITION                */
/************************************/

int ido2db_query_insert_or_update_servicedefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, host_object_id, service_object_id, display_name, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_warning, notify_on_unknown, notify_on_critical, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_ok, stalk_on_warning, stalk_on_unknown, stalk_on_critical, is_volatile, flap_detection_enabled, flap_detection_on_ok, flap_detection_on_warning, flap_detection_on_unknown, flap_detection_on_critical, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_service, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt) VALUES (%lu, %d, %lu, %lu, '%s', %lu, '%s', %lu, '%s', %lu, %lu, '%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', '%s') ON DUPLICATE KEY UPDATE host_object_id=%lu, display_name='%s', check_command_object_id=%lu, check_command_args='%s', eventhandler_command_object_id=%lu, eventhandler_command_args='%s', check_timeperiod_object_id=%lu, notification_timeperiod_object_id=%lu, failure_prediction_options='%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_warning=%d, notify_on_unknown=%d, notify_on_critical=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_ok=%d, stalk_on_warning=%d, stalk_on_unknown=%d, stalk_on_critical=%d, is_volatile=%d, flap_detection_enabled=%d, flap_detection_on_ok=%d, flap_detection_on_warning=%d, flap_detection_on_unknown=%d, flap_detection_on_critical=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_service=%d, failure_prediction_enabled=%d, notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],
                                        *(char **) data[8],
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
                                        *(char **) data[50],		/* insert end */
                                        *(unsigned long *) data[2],	/* update start */
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],
                                        *(char **) data[8],
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
                                        *(char **) data[50]            /* update end */

                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET host_object_id=%lu, display_name='%s', check_command_object_id=%lu, check_command_args='%s', eventhandler_command_object_id=%lu, eventhandler_command_args='%s', check_timeperiod_object_id=%lu, notification_timeperiod_object_id=%lu, failure_prediction_options='%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_warning=%d, notify_on_unknown=%d, notify_on_critical=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_ok=%d, stalk_on_warning=%d, stalk_on_unknown=%d, stalk_on_critical=%d, is_volatile=%d, flap_detection_enabled=%d, flap_detection_on_ok=%d, flap_detection_on_warning=%d, flap_detection_on_unknown=%d, flap_detection_on_critical=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_service=%d, failure_prediction_enabled=%d, notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s' WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICES],
                                        *(unsigned long *) data[2], 	/* update start */
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],
                                        *(char **) data[8],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, service_object_id, display_name, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_warning, notify_on_unknown, notify_on_critical, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_ok, stalk_on_warning, stalk_on_unknown, stalk_on_critical, is_volatile, flap_detection_enabled, flap_detection_on_ok, flap_detection_on_warning, flap_detection_on_unknown, flap_detection_on_critical, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_service, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt) VALUES (%lu, %d, %lu, %lu, '%s', %lu, '%s', %lu, '%s', %lu, %lu, '%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],
                                        *(char **) data[8],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND config_type=%d AND service_object_id=%lu) WHEN MATCHED THEN UPDATE SET host_object_id=%lu, display_name='%s', check_command_object_id=%lu, check_command_args='%s', eventhandler_command_object_id=%lu, eventhandler_command_args='%s', check_timeperiod_object_id=%lu, notif_timeperiod_object_id=%lu, failure_prediction_options='%s', check_interval=%lf, retry_interval=%lf, max_check_attempts=%d, first_notification_delay=%lf, notification_interval=%lf, notify_on_warning=%d, notify_on_unknown=%d, notify_on_critical=%d, notify_on_recovery=%d, notify_on_flapping=%d, notify_on_downtime=%d, stalk_on_ok=%d, stalk_on_warning=%d, stalk_on_unknown=%d, stalk_on_critical=%d, is_volatile=%d, flap_detection_enabled=%d, flap_detection_on_ok=%d, flap_detection_on_warning=%d, flap_detection_on_unknown=%d, flap_detection_on_critical=%d, low_flap_threshold=%lf, high_flap_threshold=%lf, process_performance_data=%d, freshness_checks_enabled=%d, freshness_threshold=%d, passive_checks_enabled=%d, event_handler_enabled=%d, active_checks_enabled=%d, retain_status_information=%d, retain_nonstatus_information=%d, notifications_enabled=%d, obsess_over_service=%d, failure_prediction_enabled=%d, notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s' WHEN NOT MATCHED THEN INSERT (instance_id, config_type, host_object_id, service_object_id, display_name, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notif_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_warning, notify_on_unknown, notify_on_critical, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_ok, stalk_on_warning, stalk_on_unknown, stalk_on_critical, is_volatile, flap_detection_enabled, flap_detection_on_ok, flap_detection_on_warning, flap_detection_on_unknown, flap_detection_on_critical, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_service, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt) VALUES (%lu, %d, %lu, %lu, '%s', %lu, '%s', %lu, '%s', %lu, %lu, '%s', %lf, %lf, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICES],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[3],      /* unique constraint end */
                                        *(unsigned long *) data[2], 	/* update start */
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],
                                        *(char **) data[8],
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
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],
                                        *(unsigned long *) data[5],
                                        *(char **) data[6],
                                        *(unsigned long *) data[7],
                                        *(char **) data[8],
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_definition_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_servicedefinition_contactgroups_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_contactgroups_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, service_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu') ON DUPLICATE KEY UPDATE instance_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0]     /* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%lu' WHERE service_id='%lu' AND contactgroup_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2]     /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, service_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (service_id='%lu' AND contactgroup_object_id='%lu') WHEN MATCHED THEN UPDATE SET instance_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, service_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTGROUPS],
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2],     /* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_contactgroups_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_servicedefinition_contacts_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_contacts_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, service_id, contact_object_id) VALUES (%lu, %lu, %lu) ON DUPLICATE KEY UPDATE instance_id='%lu', service_id='%lu', contact_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%lu', service_id='%lu', contact_object_id='%lu' WHERE instance_id=%lu AND service_id=%lu AND contact_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTS],
                                        *(unsigned long *) data[0],     /* update start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, service_id, contact_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu) WHEN MATCHED THEN UPDATE SET service_id='%lu', contact_object_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, service_id, contact_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTS],
                                        *(unsigned long *) data[0],     /* unique constraint start/end */
                                        *(unsigned long *) data[1],    /* update start */ 
                                        *(unsigned long *) data[2],     /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedefinition_contacts_add() end\n");

        return result;
}


/************************************/
/* SERVICEGROUPDEFINITION           */
/************************************/

int ido2db_query_insert_or_update_servicegroupdefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicegroupdefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, servicegroup_object_id, alias) VALUES (%lu, %d, %lu, '%s') ON DUPLICATE KEY UPDATE alias='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],		/* insert end */
                                        *(char **) data[3]		/* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET alias='%s' WHERE instance_id=%lu AND config_type=%d AND servicegroup_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPS],
                                        *(char **) data[3],		/* update start/end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2]	/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, servicegroup_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3]		/* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND config_type=%d AND servicegroup_object_id=%lu) WHEN MATCHED THEN UPDATE SET alias='%s' WHEN NOT MATCHED THEN INSERT (instance_id, config_type, servicegroup_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],	/* unique constraint end */
                                        *(char **) data[3],		/* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3]		/* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicegroupdefinition_definition_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_servicegroupdefinition_members_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicegroupdefinition_members_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, servicegroup_id, service_object_id) VALUES (%lu, %lu, %lu) ON DUPLICATE KEY UPDATE instance_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPMEMBERS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0]     /* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE servicegroup_id=%lu AND service_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPMEMBERS],
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2]     /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, servicegroup_id, service_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPMEMBERS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */

                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (servicegroup_id=%lu AND service_object_id=%lu) WHEN MATCHED THEN UPDATE SET instance_id=%lu WHEN NOT MATCHED THEN INSERT (instance_id, servicegroup_id, service_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPMEMBERS],
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2],     /* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicegroupdefinition_members_add() end\n");

        return result;
}


/************************************/
/* HOSTDEPENDENCIES                 */
/************************************/

int ido2db_query_insert_or_update_hostdependencydefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdependencydefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, host_object_id, dependent_host_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_up, fail_on_down, fail_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d) ON DUPLICATE KEY UPDATE timeperiod_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTDEPENDENCIES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(int *) data[4],
                                        *(int *) data[5],
                                        *(unsigned long *) data[6],
                                        *(int *) data[7],
                                        *(int *) data[8],
                                        *(int *) data[9],		/* insert end */
                                        *(unsigned long *) data[6]	/* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET timeperiod_object_id=%lu WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND dependent_host_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_up=%d AND fail_on_down=%d AND fail_on_unreachable=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTDEPENDENCIES],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, dependent_host_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_up, fail_on_down, fail_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTDEPENDENCIES],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND config_type=%d AND host_object_id=%lu AND dependent_host_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_up=%d AND fail_on_down=%d AND fail_on_unreachable=%d) WHEN MATCHED THEN UPDATE SET timeperiod_object_id=%lu WHEN NOT MATCHED THEN INSERT (instance_id, config_type, host_object_id, dependent_host_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_up, fail_on_down, fail_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTDEPENDENCIES],
					*(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(int *) data[4],
                                        *(int *) data[5],
                                        *(int *) data[7],
                                        *(int *) data[8],
                                        *(int *) data[9],               /* unique constraint end */
                                        *(unsigned long *) data[6],	/* update start/end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostdependencydefinition_definition_add() end\n");

        return result;
}


/************************************/
/* SERVICEDEPENDENCYDEFINITION      */
/************************************/

int ido2db_query_insert_or_update_servicedependencydefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedependencydefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, service_object_id, dependent_service_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_ok, fail_on_warning, fail_on_unknown, fail_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d, %d) ON DUPLICATE KEY UPDATE timeperiod_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEDEPENDENCIES],
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
                                        *(int *) data[10],		/* insert end */
                                        *(unsigned long *) data[6]	/* update start/end*/
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET timeperiod_object_id=%lu WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND dependent_service_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_ok=%d AND fail_on_warning=%d AND fail_on_unknown=%d AND fail_on_critical=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEDEPENDENCIES],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, service_object_id, dependent_service_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_ok, fail_on_warning, fail_on_unknown, fail_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEDEPENDENCIES],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (nstance_id=%lu AND config_type=%d AND service_object_id=%lu AND dependent_service_object_id=%lu AND dependency_type=%d AND inherits_parent=%d AND fail_on_ok=%d AND fail_on_warning=%d AND fail_on_unknown=%d AND fail_on_critical=%d) WHEN MATCHED THEN UPDATE SET timeperiod_object_id=%lu WHEN NOT MATCHED THEN INSERT (instance_id, config_type, service_object_id, dependent_service_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_ok, fail_on_warning, fail_on_unknown, fail_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lu, %d, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEDEPENDENCIES],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(int *) data[4],
                                        *(int *) data[5],
                                        *(int *) data[7],
                                        *(int *) data[8],
                                        *(int *) data[9],
                                        *(int *) data[10],              /* unique constraint end */
                                        *(unsigned long *) data[6],	/* update start/end*/
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicedependencydefinition_definition_add() end\n");

        return result;
}


/************************************/
/* HOSTESCALATIONDEFINITION         */
/************************************/

int ido2db_query_insert_or_update_hostescalationdefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, host_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_down, escalate_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d) ON DUPLICATE KEY UPDATE notification_interval=%lf, escalate_on_recovery=%d, escalate_on_down=%d, escalate_on_unreachable=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(int *) data[4],
                                        *(int *) data[5],
                                        *(double *) data[6],
                                        *(int *) data[7],
                                        *(int *) data[8],
                                        *(int *) data[9],		/* insert end */
                                        *(int *) data[7],		/* update start */
                                        *(int *) data[8],
                                        *(int *) data[9]		/* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET notification_interval=%lf, escalate_on_recovery=%d, escalate_on_down=%d, escalate_on_unreachable=%d WHERE instance_id=%lu AND config_type=%d AND host_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONS],
                                        *(int *) data[7],		/* update start */
                                        *(int *) data[8],
                                        *(int *) data[9],		/* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(int *) data[4],
                                        *(int *) data[5],
                                        *(double *) data[6]		/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, host_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_down, escalate_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND config_type=%d AND host_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d) WHEN MATCHED THEN UPDATE SET notification_interval=%lf, escalate_on_recovery=%d, escalate_on_down=%d, escalate_on_unreachable=%d WHEN NOT MATCHED THEN INSERT (instance_id, config_type, host_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_down, escalate_on_unreachable) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(int *) data[4],
                                        *(int *) data[5],
                                        *(double *) data[6],		/* unique constraint end */
                                        *(int *) data[7],		/* update start */
                                        *(int *) data[8],
                                        *(int *) data[9],		/* update end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_definition_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_hostescalationdefinition_contactgroups_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_contactgroups_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, hostescalation_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu') ON DUPLICATE KEY UPDATE instance_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0]     /* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%lu' WHERE hostescalation_id='%lu' AND contactgroup_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2]     /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, hostescalation_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (hostescalation_id='%lu' AND contactgroup_object_id='%lu') WHEN MATCHED THEN UPDATE SET instance_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, hostescalation_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS],
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2],     /* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_contactgroups_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_hostescalationdefinition_contacts_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_contacts_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, hostescalation_id, contact_object_id) VALUES (%lu, %lu, %lu) ON DUPLICATE KEY UPDATE instance_id='%d', hostescalation_id='%lu', contact_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%d', hostescalation_id='%lu', contact_object_id='%lu' WHERE instance_id=%lu AND hostescalation_id=%lu AND contact_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTS],
                                        *(unsigned long *) data[0],     /* update start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, hostescalation_id, contact_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu) WHEN MATCHED THEN UPDATE SET hostescalation_id='%lu', contact_object_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, hostescalation_id, contact_object_id) VALUES (%lu, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTS],
                                        *(unsigned long *) data[0],     /* unique constraint start/end */
                                        *(unsigned long *) data[1],    /* update start */ 
                                        *(unsigned long *) data[2],     /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostescalationdefinition_contacts_add() end\n");

        return result;
}


/************************************/
/* SERVICEESCALATIONDEFINITION      */
/************************************/

int ido2db_query_insert_or_update_serviceescalationdefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, service_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_warning, escalate_on_unknown, escalate_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d, %d) ON DUPLICATE KEY UPDATE notification_interval=%lf, escalate_on_recovery=%d, escalate_on_warning=%d, escalate_on_unknown=%d, escalate_on_critical=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONS],
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
                                        *(int *) data[10],		/* insert end */
                                        *(double *) data[6],		/* update start */
                                        *(int *) data[7],
                                        *(int *) data[8],
                                        *(int *) data[9],
                                        *(int *) data[10]		/* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET notification_interval=%lf, escalate_on_recovery=%d, escalate_on_warning=%d, escalate_on_unknown=%d, escalate_on_critical=%d WHERE instance_id=%lu AND config_type=%d AND service_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONS],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, service_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_warning, escalate_on_unknown, escalate_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND config_type=%d AND service_object_id=%lu AND timeperiod_object_id=%lu AND first_notification=%d AND last_notification=%d) WHEN MATCHED THEN UPDATE SET notification_interval=%lf, escalate_on_recovery=%d, escalate_on_warning=%d, escalate_on_unknown=%d, escalate_on_critical=%d WHEN NOT MATCHED THEN INSERT (instance_id, config_type, service_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_warning, escalate_on_unknown, escalate_on_critical) VALUES (%lu, %d, %lu, %lu, %d, %d, %lf, %d, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONS],
                                        *(unsigned long *) data[0],    	/* unique constraint start */ 
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(unsigned long *) data[3],
                                        *(int *) data[4],
                                        *(int *) data[5],		/* unique constraint end */
                                        *(double *) data[6],		/* update start */
                                        *(int *) data[7],
                                        *(int *) data[8],
                                        *(int *) data[9],
                                        *(int *) data[10],		/* update end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_definition_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_serviceescalationdefinition_contactgroups_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_contactgroups_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, serviceescalation_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu') ON DUPLICATE KEY UPDATE instance_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert start */
                                        *(unsigned long *) data[0]     /* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%lu' WHERE serviceescalation_id='%lu' AND contactgroup_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2]     /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, serviceescalation_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]    /* insert start */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (serviceescalation_id='%lu' AND contactgroup_object_id='%lu') WHEN MATCHED THEN UPDATE SET instance_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, serviceescalation_id, contactgroup_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS],
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(unsigned long *) data[2],     /* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]    /* insert start */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_contactgroups_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_serviceescalationdefinition_contacts_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_contacts_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, serviceescalation_id, contact_object_id) VALUES ('%lu', '%lu', '%lu') ON DUPLICATE KEY UPDATE instance_id='%d', serviceescalation_id='%lu', contact_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert start */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* update start */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%d', serviceescalation_id='%lu', contact_object_id='%lu' WHERE instance_id='%d' AND serviceescalation_id='%lu' AND contact_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS],
                                        *(unsigned long *) data[0],     /* update start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* update start */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, serviceescalation_id, contact_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        
			asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id='%d') WHEN MATCHED THEN UPDATE SET serviceescalation_id='%lu', contact_object_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, serviceescalation_id, contact_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS],
                                        *(unsigned long *) data[0],     /* unique constraint start/end */
                                        *(unsigned long *) data[1],	/* update start */
                                        *(unsigned long *) data[2],     /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(unsigned long *) data[2]     /* insert end */
                        );

                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_serviceescalationdefinition_contacts_add() end\n");

        return result;
}


/************************************/
/*  COMMANDDEFINITION               */
/************************************/

int ido2db_query_insert_or_update_commanddefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commanddefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, object_id, config_type, command_line) VALUES (%lu, %lu, %d, '%s') ON DUPLICATE KEY UPDATE command_line='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMANDS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(char **) data[3],		/* insert end */
                                        *(char **) data[3]		/* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET command_line='%s' WHERE instance_id=%lu AND object_id=%lu AND config_type=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMANDS],
                                        *(char **) data[3],		/* update start/end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2]		/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, object_id, config_type, command_line) VALUES (%lu, %lu, %d, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMANDS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(char **) data[3]		/* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND object_id=%lu AND config_type=%d) WHEN MATCHED THEN UPDATE SET command_line='%s' WHEN NOT MATCHED THEN INSERT (instance_id, object_id, config_type, command_line) VALUES (%lu, %lu, %d, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMANDS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],		/* unique constraint end */
                                        *(char **) data[3],		/* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(char **) data[3]		/* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commanddefinition_definition_add() end\n");

        return result;
}


/************************************/
/*  TIMEPERIODDEFINITION            */
/************************************/

int ido2db_query_insert_or_update_timeperiodefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timeperiodefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, timeperiod_object_id, alias) VALUES (%lu, %d, %lu, '%s') ON DUPLICATE KEY UPDATE alias='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],		/* insert end */
                                        *(char **) data[3]		/* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET alias='%s' WHERE instance_id=%lu AND config_type=%d AND timeperiod_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODS],
                                        *(char **) data[3],		/* update start/end */
                                        *(unsigned long *) data[0],    	/* unique constraint start */ 
                                        *(int *) data[1],
                                        *(unsigned long *) data[2]	/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, timeperiod_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3]		/* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND config_type=%d AND timeperiod_object_id=%lu) WHEN MATCHED THEN UPDATE SET alias='%s' WHEN NOT MATCHED THEN INSERT (instance_id, config_type, timeperiod_object_id, alias) VALUES (%lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODS],
                                        *(unsigned long *) data[0],    	/* unique constraint start */ 
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],	/* unique constraint end */
                                        *(char **) data[3],		/* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3]		/* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timeperiodefinition_definition_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_timeperiodefinition_timeranges_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timeperiodefinition_timeranges_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, timeperiod_id, day, start_sec, end_sec) VALUES (%lu, %lu, %d, %lu, %lu) ON DUPLICATE KEY UPDATE instance_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODTIMERANGES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(unsigned long *) data[4],	/* insert end */
                                        *(unsigned long *) data[0]     /* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id=%lu WHERE timeperiod_id=%lu AND day=%d AND start_sec=%lu AND end_sec=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODTIMERANGES],
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[1],	/* unique constraint start */
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(unsigned long *) data[4]	/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, timeperiod_id, day, start_sec, end_sec) VALUES (%lu, %lu, %d, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODTIMERANGES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(unsigned long *) data[4]	/* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (timeperiod_id=%lu AND day=%d AND start_sec=%lu AND end_sec=%lu) WHEN MATCHED THEN UPDATE SET instance_id=%lu WHEN NOT MATCHED THEN INSERT (instance_id, timeperiod_id, day, start_sec, end_sec) VALUES (%lu, %lu, %d, %lu, %lu)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODTIMERANGES],
                                        *(unsigned long *) data[1],	/* unique constraint start */
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(unsigned long *) data[4],	/* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(unsigned long *) data[4]	/* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timeperiodefinition_timeranges_add() end\n");

        return result;
}


/************************************/
/* CONTACTDEFINITION                */
/************************************/

int ido2db_query_insert_or_update_contactdefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, contact_object_id, alias, email_address, pager_address, host_timeperiod_object_id, service_timeperiod_object_id, host_notifications_enabled, service_notifications_enabled, can_submit_commands, notify_service_recovery, notify_service_warning, notify_service_unknown, notify_service_critical, notify_service_flapping, notify_service_downtime, notify_host_recovery, notify_host_down, notify_host_unreachable, notify_host_flapping, notify_host_downtime) VALUES (%lu, %d, %lu, '%s', '%s', '%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d) ON DUPLICATE KEY UPDATE alias='%s', email_address='%s', pager_address='%s', host_timeperiod_object_id=%lu, service_timeperiod_object_id=%lu, host_notifications_enabled=%d, service_notifications_enabled=%d, can_submit_commands=%d, notify_service_recovery=%d, notify_service_warning=%d, notify_service_unknown=%d, notify_service_critical=%d, notify_service_flapping=%d, notify_service_downtime=%d, notify_host_recovery=%d, notify_host_down=%d, notify_host_unreachable=%d, notify_host_flapping=%d, notify_host_downtime=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTS],
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
                                        *(int *) data[21],		/* insert end */
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
                                        *(int *) data[21]              /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET alias='%s', email_address='%s', pager_address='%s', host_timeperiod_object_id=%lu, service_timeperiod_object_id=%lu, host_notifications_enabled=%d, service_notifications_enabled=%d, can_submit_commands=%d, notify_service_recovery=%d, notify_service_warning=%d, notify_service_unknown=%d, notify_service_critical=%d, notify_service_flapping=%d, notify_service_downtime=%d, notify_host_recovery=%d, notify_host_down=%d, notify_host_unreachable=%d, notify_host_flapping=%d, notify_host_downtime=%d WHERE instance_id=%lu AND config_type=%d AND contact_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTS],
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
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, contact_object_id, alias, email_address, pager_address, host_timeperiod_object_id, service_timeperiod_object_id, host_notifications_enabled, service_notifications_enabled, can_submit_commands, notify_service_recovery, notify_service_warning, notify_service_unknown, notify_service_critical, notify_service_flapping, notify_service_downtime, notify_host_recovery, notify_host_down, notify_host_unreachable, notify_host_flapping, notify_host_downtime) VALUES (%lu, %d, %lu, '%s', '%s', '%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTS],
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
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND config_type=%d AND contact_object_id=%lu) WHEN MATCHED THEN UPDATE SET alias='%s', email_address='%s', pager_address='%s', host_timeperiod_object_id=%lu, service_timeperiod_object_id=%lu, host_notifications_enabled=%d, service_notifications_enabled=%d, can_submit_commands=%d, notify_service_recovery=%d, notify_service_warning=%d, notify_service_unknown=%d, notify_service_critical=%d, notify_service_flapping=%d, notify_service_downtime=%d, notify_host_recovery=%d, notify_host_down=%d, notify_host_unreachable=%d, notify_host_flapping=%d, notify_host_downtime=%d WHEN NOT MATCHED THEN INSERT (instance_id, config_type, contact_object_id, alias, email_address, pager_address, host_timeperiod_object_id, service_timeperiod_object_id, host_notifications_enabled, service_notifications_enabled, can_submit_commands, notify_service_recovery, notify_service_warning, notify_service_unknown, notify_service_critical, notify_service_flapping, notify_service_downtime, notify_host_recovery, notify_host_down, notify_host_unreachable, notify_host_flapping, notify_host_downtime) VALUES (%lu, %d, %lu, '%s', '%s', '%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],	/* unique constraint end */
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
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_definition_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_contactdefinition_addresses_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_addresses_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, contact_id, address_number, address) VALUES (%lu, %lu, %d, '%s') ON DUPLICATE KEY UPDATE instance_id=%lu, address='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTADDRESSES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(char **) data[3],		/* insert end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(char **) data[3]		/* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id=%lu, address='%s' WHERE contact_id=%lu AND address_number=%d",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTADDRESSES],
                                        *(unsigned long *) data[0],     /* update start */
                                        *(char **) data[3],		/* update end */
                                        *(unsigned long *) data[1],	/* unique constraint start */
                                        *(int *) data[2]		/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, contact_id, address_number, address) VALUES (%lu, %lu, %d, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTADDRESSES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(char **) data[3]		/* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (contact_id=%lu AND address_number=%d) WHEN MATCHED THEN UPDATE SET instance_id=%lu, address='%s' WHEN NOT MATCHED THEN INSERT (instance_id, contact_id, address_number, address) VALUES (%lu, %lu, %d, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTADDRESSES],
                                        *(unsigned long *) data[1],	/* unique constraint start */
                                        *(int *) data[2],		/* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(char **) data[3],             /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(char **) data[3]		/* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_addresses_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_contactdefinition_hostnotificationcommands_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_hostnotificationcommands_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, '%s') ON DUPLICATE KEY UPDATE command_args='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],		/* insert end */
                                        *(char **) data[4]		/* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET command_args='%s' WHERE instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                        *(char **) data[4],		/* update start/end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3]	/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4]		/* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu) WHEN MATCHED THEN UPDATE SET command_args='%s' WHEN NOT MATCHED THEN INSERT (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],	/* unique constraint end */
                                        *(char **) data[4],		/* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4]              /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_hostnotificationcommands_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_contactdefinition_servicenotificationcommands_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_servicenotificationcommands_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, '%s') ON DUPLICATE KEY UPDATE command_args='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4],		/* insert end */
                                        *(char **) data[4]		/* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET command_args='%s' WHERE instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                        *(char **) data[4],		/* update start/end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3]	/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4]		/* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id=%lu AND contact_id=%lu AND notification_type=%d AND command_object_id=%lu) WHEN MATCHED THEN UPDATE SET command_args='%s' WHEN NOT MATCHED THEN INSERT (instance_id, contact_id, notification_type, command_object_id, command_args) VALUES (%lu, %lu, %d, %lu, '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS],
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],	/* unique constraint end */
                                        *(char **) data[4],		/* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(unsigned long *) data[3],
                                        *(char **) data[4]		/* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactdefinition_servicenotificationcommands_add() end\n");

        return result;
}


/************************************/
/* CUSTOMVARIABLES                  */
/************************************/

int ido2db_query_insert_or_update_save_custom_variables_customvariables_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_save_custom_variables_customvariables_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %d, %d, '%s', '%s') ON DUPLICATE KEY UPDATE instance_id=%lu, config_type=%d, has_been_modified=%d, varvalue='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(int *) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5],		/* insert end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(int *) data[2],
                                        *(int *) data[3],
                                        *(char **) data[5]		/* update end */

                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id=%lu, config_type=%d, has_been_modified=%d, varvalue='%s' WHERE object_id=%lu AND varname='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES],
                                        *(unsigned long *) data[0],     /* update start */
                                        *(int *) data[2],
                                        *(int *) data[3],
                                        *(char **) data[5],		/* update end */
                                        *(unsigned long *) data[1],	/* unique constraint start */
                                        *(char **) data[4]		/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %d, %d, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(int *) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5]		/* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (object_id=%lu AND varname='%s') WHEN MATCHED THEN UPDATE SET instance_id=%lu, config_type=%d, has_been_modified=%d, varvalue='%s' WHEN NOT MATCHED THEN INSERT (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %d, %d, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES],
                                        *(unsigned long *) data[1],	/* unique constraint start */
                                        *(char **) data[4],		/* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(int *) data[2],
                                        *(int *) data[3],
                                        *(char **) data[5],		/* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(int *) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5]		/* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_save_custom_variables_customvariables_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_save_custom_variables_customvariablestatus_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_save_custom_variables_customvariablestatus_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, object_id, status_update_time, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %s, %d, '%s', '%s') ON DUPLICATE KEY UPDATE instance_id=%lu, status_update_time=%s, has_been_modified=%d, varvalue='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(char **) data[2],
                                        *(int *) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5],             /* insert end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(char **) data[2],
                                        *(int *) data[3],
                                        *(char **) data[5]              /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id=%lu, status_update_time=%s, has_been_modified=%d, varvalue='%s' WHERE object_id=%lu AND varname='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
                                        *(unsigned long *) data[0],     /* update start */
                                        *(char **) data[2],
                                        *(int *) data[3],
                                        *(char **) data[5],             /* update end */
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(char **) data[4]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, object_id, status_update_time, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %s, %d, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(int *) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5]             /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (object_id=%lu AND varname='%s') WHEN MATCHED THEN UPDATE SET instance_id=%lu, status_update_time=%s, has_been_modified=%d, varvalue='%s' WHEN NOT MATCHED THEN INSERT (instance_id, object_id, status_update_time, has_been_modified, varname, varvalue) VALUES (%lu, %lu, %s, %d, '%s', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
                                        *(unsigned long *) data[1],     /* unique constraint start */
                                        *(char **) data[4],      /* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start */
                                        *(char **) data[2],
                                        *(int *) data[3],
                                        *(char **) data[5],             /* update end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],
                                        *(int *) data[2],
                                        *(int *) data[3],
                                        *(char **) data[4],
                                        *(char **) data[5]             /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_save_custom_variables_customvariablestatus_add() end\n");

        return result;
}


/************************************/
/* CONTACTGROUPDEFINITION           */
/************************************/

int ido2db_query_insert_or_update_contactgroupdefinition_definition_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactgroupdefinition_definition_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, config_type, contactgroup_object_id, alias) VALUES ('%lu', '%d', '%lu', '%s') ON DUPLICATE KEY UPDATE alias='%s'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3],		/* insert end */
                                        *(char **) data[3]		/* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET alias='%s' WHERE instance_id='%lu' AND config_type='%d' AND contactgroup_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPS],
                                        *(char **) data[3],		/* update start/end */
                                        *(unsigned long *) data[0], 	/* unique constraint start */    
                                        *(int *) data[1],
                                        *(unsigned long *) data[2]	/* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, config_type, contactgroup_object_id, alias) VALUES ('%lu', '%d', '%lu', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3]		/* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (instance_id='%lu' AND config_type='%d' AND contactgroup_object_id='%lu') WHEN MATCHED THEN UPDATE SET alias='%s' WHEN NOT MATCHED THEN INSERT (instance_id, config_type, contactgroup_object_id, alias) VALUES ('%lu', '%d', '%lu', '%s')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPS],
                                        *(unsigned long *) data[0], 	/* unique constraint start */    
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],	/* unique constraint end */
                                        *(char **) data[3],		/* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(int *) data[1],
                                        *(unsigned long *) data[2],
                                        *(char **) data[3]		/* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactgroupdefinition_definition_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_contactgroupdefinition_contactgroupmembers_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1 = NULL;
        char * query2 = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactgroupdefinition_contactgroupmembers_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, contactgroup_id, contact_object_id) VALUES ('%lu', '%lu', '%lu') ON DUPLICATE KEY UPDATE instance_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPMEMBERS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2],     /* insert end */
                                        *(unsigned long *) data[0]     /* update start/end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s SET instance_id='%lu' WHERE contactgroup_id='%lu' AND contact_object_id='%lu'",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPMEMBERS],
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[1],    	/* unique constraint start */ 
                                        *(unsigned long *) data[2]     /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
			free(query1);

                        /* check result if update was ok */
			if(dbi_result_get_numrows_affected(idi->dbinfo.dbi_result) == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, contactgroup_id, contact_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPMEMBERS],
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
				free(query2);
                        }
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
#ifdef USE_ORACLE
                        /* use prepared statements and ocilib */
                        asprintf(&query1, "MERGE INTO %s USING DUAL ON (contactgroup_id='%lu' AND contact_object_id='%lu') WHEN MATCHED THEN UPDATE SET instance_id='%lu' WHEN NOT MATCHED THEN INSERT (instance_id, contactgroup_id, contact_object_id) VALUES ('%lu', '%lu', '%lu')",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPMEMBERS],
                                        *(unsigned long *) data[1],    	/* unique constraint start */ 
                                        *(unsigned long *) data[2],     /* unique constraint end */
                                        *(unsigned long *) data[0],     /* update start/end */
                                        *(unsigned long *) data[0],     /* insert start */
                                        *(unsigned long *) data[1],     
                                        *(unsigned long *) data[2]     /* insert end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        free(query1);
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactgroupdefinition_contactgroupmembers_add() end\n");

        return result;
}




