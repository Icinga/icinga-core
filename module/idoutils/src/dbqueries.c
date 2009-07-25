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
	char * query1;
	char * query2;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timedevents() start\n");

        if (idi == NULL)
                return NDO_ERROR;

        if (idi->dbinfo.connected == NDO_FALSE)
                return NDO_ERROR;
	
	/* check data array length */


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
			if(result == NDO_ERROR) {
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

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_query_insert_or_update_timedevents() end\n"); 

	return result;
}



