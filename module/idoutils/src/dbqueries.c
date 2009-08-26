/***************************************************************
 * DBQUERIES.C - Data Query handler routines for IDO2DB daemon
 *
 * Copyright (c) 2009 Icinga Development Team (http://www.icinga.org)
 *
 * Last Modified: 07-25-2009
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

/* for result checking */
#include <dbi/dbi-dev.h>

extern int errno;

extern char *ndo2db_db_tablenames[NDO2DB_MAX_DBTABLES];

/****************************************************************************/
/* OBJECT ROUTINES                                                          */
/****************************************************************************/

/*
NOTE using libdbi:
result is stored in idi->dbinfo.dbi_result
which consists of the structure dbi_result_t * defined in libdbi/include/dbi/dbi-dev.h
for checking if UPDATE was successful try
        unsigned long long numrows_matched; 
        unsigned long long numrows_affected;
*/

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
	char * query1;
	char * query2;

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
					*(int *) data[3],
					*(int *) data[5]		/* update end */
			);
			/* send query to db */
			result = ndo2db_db_query(idi, query1);	
	                break;
        	case NDO2DB_DBSERVER_PGSQL:
			asprintf(&query1, "UPDATE %s queued_time=%s, queued_time_usec=%lu, recurring_event=%d WHERE instance_id=%lu AND event_type=%d AND scheduled_time=%s AND object_id=%lu",
					ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
                                        *(char **) data[2],		/* update start */
                                        *(int *) data[3],
                                        *(int *) data[5],		/* update end */
					*(unsigned long *) data[0],	/* unique constraint start */
					*(int *) data[1],
					*(char **) data[4],
					*(unsigned long *) data[6]	/* unique constraint end */
			);
			/* send query to db */
			result = ndo2db_db_query(idi, query1);		
		
			/* check result if update was ok */
			if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) { /* recast from void * */
				/* try insert instead */
				asprintf(&query2, "INSERT INTO %s (instance_id, event_type, queued_time, queued_time_usec, scheduled_time, recurring_event, object_id) VALUES ('%lu', '%d', '%s', '%lu', '%s', '%d', '%lu')",
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
#endif
        	        break;
	        case NDO2DB_DBSERVER_SQLITE:
        	        break;
	        case NDO2DB_DBSERVER_SQLITE3:
        	        break;
	        default:
        	        break;
        }

	free(query1);
	free(query2);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timedevents_add() end\n"); 

	return result;
}

int ido2db_query_insert_or_update_timedevents_execute_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

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
                                        *(int *) data[3],
                                        *(int *) data[5]                /* update end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s event_time=%s, event_time_usec=%lu, recurring_event=%d WHERE instance_id=%lu AND event_type=%d AND scheduled_time=%s AND object_id=%lu",
                                        ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],
                                        *(char **) data[2],             /* update start */
                                        *(int *) data[3],
                                        *(int *) data[5],               /* update end */
                                        *(unsigned long *) data[0],     /* unique constraint start */
                                        *(int *) data[1],
                                        *(char **) data[4],
                                        *(unsigned long *) data[6]      /* unique constraint end */
                        );
                        /* send query to db */
                        result = ndo2db_db_query(idi, query1);

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, event_type, event_time, event_time_usec, scheduled_time, recurring_event, object_id) VALUES ('%lu', '%d', '%s', '%lu', '%s', '%d', '%lu')",
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timedevents_execute() end\n");

        return result;
}


/************************************/
/* SYSTEMCOMMANDDATA                */
/************************************/

int ido2db_query_insert_or_update_systemcommanddata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s end_time=%s, end_time_usec=%lu, command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s' WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_systemcommanddata_add() end\n");

        return result;
}


/************************************/
/* EVENTHANDLER                     */
/************************************/

int ido2db_query_insert_or_update_eventhandlerdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, eventhandler_type, object_id, state, state_type, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args, command_line, timeout, early_timeout, execution_time, return_code, output) VALUES (%lu, %d, %lu, %d, %d, %s, %lu, %s, %lu, %lu, '%s', '%s', %d, %d, %lf, %d, '%s', '%s') ON DUPLICATE KEY UPDATE eventhandler_type=%d, object_id=%lu, state=%d, state_type=%d, end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s', command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s'",
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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s eventhandler_type=%d, object_id=%lu, state=%d, state_type=%d, end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s', command_line='%s', timeout=%d, early_timeout=%d, execution_time=%lf, return_code=%d, output='%s', long_output='%s' WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, eventhandler_type, object_id, state, state_type, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args, command_line, timeout, early_timeout, execution_time, return_code, output) VALUES (%lu, %d, %lu, %d, %d, %s, %lu, %s, %lu, %lu, '%s', '%s', %d, %d, %lf, %d, '%s')",
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_eventhandlerdata_add() end\n");

        return result;
}


/************************************/
/* NOTIFICATIONS                    */
/************************************/

int ido2db_query_insert_or_update_notificationdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s notification_type=%d, notification_reason=%d, end_time=%s, end_time_usec=%lu, state=%d, output='%s', long_output='%s', escalated=%d, contacts_notified=%d  WHERE instance_id=%lu AND start_time=%s AND start_time_usec=%lu AND object_id=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_notificationdata_add() end\n");

        return result;
}


/************************************/
/* CONTACTNOTIFICATIONS             */
/************************************/

int ido2db_query_insert_or_update_contactnotificationdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s notification_id=%lu, end_time=%s, end_time_usec=%lu WHERE instance_id=%lu AND contact_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationdata_add() end\n");

        return result;
}


int ido2db_query_insert_or_update_contactnotificationmethoddata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s end_time=%s, end_time_usec=%lu, command_object_id=%lu, command_args='%s' WHERE instance_id=%lu AND contactnotification_id=%lu AND start_time=%s AND start_time_usec=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_contactnotificationmethoddata_add() end\n");

        return result;
}


/************************************/
/* SERVICECHECKS                    */
/************************************/

int ido2db_query_insert_or_update_servicecheckdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicecheckdata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, service_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata) VALUES ('%lu', '%lu', '%d', '%d', '%d', '%d', '%d', '%s', '%lu', '%s', '%lu', '%d', '%d', '%lf', '%lf', '%d', '%s', '%s', '%s') ON DUPLICATE KEY UPDATE check_type='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s'",
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
                                        *(char **) data[18],     	/* insert end */
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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s check_type='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s' WHERE instance_id=%lu AND service_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (instance_id, service_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata) VALUES ('%lu', '%lu', '%d', '%d', '%d', '%d', '%d', '%s', '%lu', '%s', '%lu', '%d', '%d', '%lf', '%lf', '%d', '%s', '%s', '%s')",
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
                                        *(char **) data[18]            /* insert end */
                                );
                                /* send query to db */
                                result = ndo2db_db_query(idi, query2);
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_servicecheckdata_add() end\n");

        return result;
}


/************************************/
/* HOSTCHECKS                       */
/************************************/

int ido2db_query_insert_or_update_hostcheckdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s check_type='%d', is_raw_check='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s' WHERE instance_id=%lu AND host_object_id=%lu AND start_time=%s AND start_time_usec=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
                                /* try insert instead */
                                asprintf(&query2, "INSERT INTO %s (command_object_id, command_args, command_line, instance_id, host_object_id, check_type, is_raw_check, current_check_attempt, max_check_attempts, state, state_type, start_time, start_time_usec, end_time, end_time_usec, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata) VALUES (%lu, '%s', '%s', %lu, %lu, %d, %d, %d, %d, %d, %d, %s, %lu, %s, %lu, %d, %d, %lf, %lf, %d, '%s', i'%s', '%s')",
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hostcheckdata_add() end\n");

        return result;
}


/************************************/
/* COMMENTS                         */
/************************************/

int ido2db_query_insert_or_update_commentdata_add(ndo2db_idi *idi, void **data, char *table_name) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s comment_type=%d, entry_type=%d, object_id=%lu, author_name='%s', comment_data='%s', is_persistent=%d, comment_source=%d, expires=%d, expiration_time=%s WHERE instance_id=%lu AND comment_time=%s AND internal_comment_id=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_commentdata_add() end\n");

        return result;
}


/************************************/
/* DOWNTIME                         */
/************************************/

int ido2db_query_insert_or_update_downtimedata_add(ndo2db_idi *idi, void **data, char *table_name) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_add() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;

        switch (idi->dbinfo.server_type) {
                case NDO2DB_DBSERVER_MYSQL:
                        asprintf(&query1, "INSERT INTO %s (instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time) VALUES (%lu, %d, %lu, %s, '%s', '%s', %lu, %lu, %d, %lu, %s, %s) ON DUPLICATE KEY UPDATEdowntime_type=%d, author_name='%s', comment_data='%s', triggered_by_id=%lu, is_fixed=%d, duration=%lu, scheduled_start_time=%s, scheduled_end_time=%s ",
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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s downtime_type=%d, author_name='%s', comment_data='%s', triggered_by_id=%lu, is_fixed=%d, duration=%lu, scheduled_start_time=%s, scheduled_end_time=%s WHERE instance_id=%lu AND object_id=%lu AND entry_time=%s AND internal_downtime_id=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_downtimedata_add() end\n");

        return result;
}


/************************************/
/* PROGRAMSTATUS                    */
/************************************/

int ido2db_query_insert_or_update_programstatusdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s status_update_time=%s, program_start_time=%s, is_currently_running=1, process_id=%lu, daemon_mode=%d, last_command_check=%s, last_log_rotation=%s, notifications_enabled=%d, active_service_checks_enabled=%d, passive_service_checks_enabled=%d, active_host_checks_enabled=%d, passive_host_checks_enabled=%d, event_handlers_enabled=%d, flap_detection_enabled=%d, failure_prediction_enabled=%d, process_performance_data=%d, obsess_over_hosts=%d, obsess_over_services=%d, modified_host_attributes=%lu, modified_service_attributes=%lu, global_host_event_handler='%s', global_service_event_handler='%s' WHERE instance_id=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_programstatusdata_add() end\n");

        return result;
}


/************************************/
/* HOSTSTATUS                       */
/************************************/

int ido2db_query_insert_or_update_hoststatusdata_add(ndo2db_idi *idi, void **data) {
        int result = NDO_OK;
        const char *dbi_error;
        char * query1;
        char * query2;

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
                        break;
                case NDO2DB_DBSERVER_PGSQL:
                        asprintf(&query1, "UPDATE %s instance_id='%lu', host_object_id='%lu', status_update_time=%s, output='%s', long_output='%s', perfdata='%s', current_state='%d', has_been_checked='%d', should_be_scheduled='%d', current_check_attempt='%d', max_check_attempts='%d', last_check=%s, next_check=%s, check_type='%d', last_state_change=%s, last_hard_state_change=%s, last_hard_state='%d', last_time_up=%s, last_time_down=%s, last_time_unreachable=%s, state_type='%d', last_notification=%s, next_notification=%s, no_more_notifications='%d', notifications_enabled='%d', problem_has_been_acknowledged='%d', acknowledgement_type='%d', current_notification_number='%d', passive_checks_enabled='%d', active_checks_enabled='%d', event_handler_enabled='%d', flap_detection_enabled='%d', is_flapping='%d', percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_host='%d', modified_host_attributes='%lu', event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id='%lu' WHERE host_object_id=%lu",
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

                        /* check result if update was ok */
                        if(((dbi_result_t *)(idi->dbinfo.dbi_result))->numrows_matched == 0) {
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
#endif
                        break;
                case NDO2DB_DBSERVER_SQLITE:
                        break;
                case NDO2DB_DBSERVER_SQLITE3:
                        break;
                default:
                        break;
        }

        free(query1);
        free(query2);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_hoststatusdata_add() end\n");

        return result;
}


