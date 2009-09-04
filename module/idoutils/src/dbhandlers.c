/***************************************************************
 * DBHANDLERS.C - Data handler routines for IDO2DB daemon
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
#include "../include/db.h"
#include "../include/dbhandlers.h"
#include "../include/dbqueries.h"

/* Nagios header files */
#include "../../../include/icinga.h"
#include "../../../include/broker.h"
#include "../../../include/comments.h"

extern int errno;

extern char *ndo2db_db_tablenames[NDO2DB_MAX_DBTABLES];

/****************************************************************************/
/* OBJECT ROUTINES                                                          */
/****************************************************************************/

int ndo2db_get_object_id(ndo2db_idi *idi, int object_type, char *n1, char *n2, unsigned long *object_id) {
	int result = NDO_OK;
	int x = 0;
	unsigned long cached_object_id = 0L;
	int found_object = NDO_FALSE;
	char *name1 = NULL;
	char *name2 = NULL;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *es[2];

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_object_id() start\n");

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
	        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_object_id() return null names\n");
		return NDO_OK;
	}

	/* see if the object already exists in cached lookup table */
	if (ndo2db_get_cached_object_id(idi, object_type, name1, name2, &cached_object_id) == NDO_OK) {
		*object_id = cached_object_id;
		ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_object_id(%lu) return cached object\n", *object_id);
		return NDO_OK;
	}

	if (name1 == NULL) {
		es[0] = NULL;
		if (asprintf(&buf1, "name1 IS NULL") == -1)
			buf1 = NULL;
	} else {
		es[0] = ndo2db_db_escape_string(idi, name1);
		if (asprintf(&buf1, "name1='%s'", es[0]) == -1)
			buf1 = NULL;
	}

	if (name2 == NULL) {
		es[1] = NULL;
		if (asprintf(&buf2, "name2 IS NULL") == -1)
			buf2 = NULL;
	} else {
		es[1] = ndo2db_db_escape_string(idi, name2);
		if (asprintf(&buf2, "name2='%s'", es[1]) == -1)
			buf2 = NULL;
	}

	if (asprintf(
			&buf,
			"SELECT * FROM %s WHERE instance_id='%lu' AND objecttype_id='%d' AND %s AND %s",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS],
			idi->dbinfo.instance_id, object_type, buf1, buf2) == -1)
		buf = NULL;
	if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {
		if (idi->dbinfo.dbi_result != NULL) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result))
				*object_id = dbi_result_get_ulong(idi->dbinfo.dbi_result, "object_id");
				

				dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
		}
	}

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	/* free memory */
	free(buf1);
	free(buf2);

	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	if (found_object == NDO_FALSE)
		result = NDO_ERROR;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_object_id(%lu) end\n", *object_id);

	return result;
}

int ndo2db_get_object_id_with_insert(ndo2db_idi *idi, int object_type, char *n1, char *n2, unsigned long *object_id) {
	int result = NDO_OK;
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *tmp = NULL;
	char *name1 = NULL;
	char *name2 = NULL;
	char *es[2];

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_object_id_with_insert() start\n");

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
		return NDO_OK;
	}

	/* object already exists */
	if ((result = ndo2db_get_object_id(idi, object_type, name1, name2,
			object_id)) == NDO_OK)
		return NDO_OK;

	if (name1 != NULL) {
		es[0] = ndo2db_db_escape_string(idi, name1);
		if (asprintf(&buf1, ", '%s'", es[0]) == -1)
			buf1 = NULL;
	} else
		es[0] = NULL;
	if (name2 != NULL) {
		es[1] = ndo2db_db_escape_string(idi, name2);
		if (asprintf(&buf2, "'%s'", es[1]) == -1)
			buf2 = NULL;
	} else
		es[1] = NULL;

	if (asprintf(&buf,
			"INSERT INTO %s (instance_id, objecttype_id, name1, name2) VALUES ('%lu', '%d' %s, %s)",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS],
			idi->dbinfo.instance_id, object_type, (buf1 == NULL) ? "NULL" : buf1,
			(buf2 == NULL) ? "NULL" : buf2) == -1)
		buf = NULL;
	if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                *object_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_object_id_with_insert(%lu) object_id\n", *object_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&tmp, "%s_object_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS]) == -1)
                                        tmp = NULL;

                                *object_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, tmp);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_object_id_with_insert(%s=%lu) object_id\n", tmp, *object_id);
                                free(tmp);
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
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
        }

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	/* cache object id for later lookups */
	ndo2db_add_cached_object_id(idi, object_type, name1, name2, *object_id);

	/* free memory */
	free(buf1);
	free(buf2);

        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_object_id_with_insert(%lu) end\n", *object_id);

	return result;
}

int ndo2db_get_cached_object_ids(ndo2db_idi *idi) {
	int result = NDO_OK;
	unsigned long object_id = 0L;
	int objecttype_id = 0;
	char *buf = NULL;

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_cached_object_ids() start\n");

	/* find all the object definitions we already have */
	if (asprintf(
			&buf,
			"SELECT object_id, objecttype_id, name1, name2 FROM %s WHERE instance_id='%lu'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS],
			idi->dbinfo.instance_id) == -1)
		buf = NULL;

	if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {
		while (idi->dbinfo.dbi_result) {
			if (dbi_result_next_row(idi->dbinfo.dbi_result)) {
				object_id = dbi_result_get_uint(idi->dbinfo.dbi_result,
						"object_id");
				objecttype_id = dbi_result_get_int(idi->dbinfo.dbi_result,
						"objecttype_id");
				ndo2db_add_cached_object_id(idi, objecttype_id,
						dbi_result_get_string_copy(idi->dbinfo.dbi_result,
								"name1"), dbi_result_get_string_copy(
								idi->dbinfo.dbi_result, "name2"), object_id);
			}
			dbi_result_free(idi->dbinfo.dbi_result);
			idi->dbinfo.dbi_result = NULL;
		}
	}
	free(buf);
        
	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_cached_object_ids(%lu) end\n", object_id);
	
	return result;
}

int ndo2db_get_cached_object_id(ndo2db_idi *idi, int object_type, char *name1,
		char *name2, unsigned long *object_id) {
	int result = NDO_ERROR;
	int hashslot = 0;
	int compare = 0;
	ndo2db_dbobject *temp_object = NULL;
	int y = 0;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_cached_object_id() start\n");

	hashslot = ndo2db_object_hashfunc(name1, name2, NDO2DB_OBJECT_HASHSLOTS);
#ifdef NDO2DB_DEBUG_CACHING
	printf("OBJECT LOOKUP: type=%d, name1=%s, name2=%s\n",object_type,(name1==NULL)?"NULL":name1,(name2==NULL)?"NULL":name2);
#endif

	if (idi->dbinfo.object_hashlist == NULL)
		return NDO_ERROR;

	for (temp_object = idi->dbinfo.object_hashlist[hashslot], y = 0; temp_object
			!= NULL; temp_object = temp_object->nexthash, y++) {
#ifdef NDO2DB_DEBUG_CACHING
		printf("OBJECT LOOKUP LOOPING [%d][%d]: type=%d, id=%lu, name1=%s, name2=%s\n",hashslot,y,temp_object->object_type,temp_object->object_id,(temp_object->name1==NULL)?"NULL":temp_object->name1,(temp_object->name2==NULL)?"NULL":temp_object->name2);
#endif
		compare = ndo2db_compare_object_hashdata(temp_object->name1,
				temp_object->name2, name1, name2);
		if (compare == 0 && temp_object->object_type == object_type)
			break;
	}

	/* we have a match! */
	if (temp_object && (ndo2db_compare_object_hashdata(temp_object->name1,
			temp_object->name2, name1, name2) == 0) && temp_object->object_type
			== object_type) {
#ifdef NDO2DB_DEBUG_CACHING
		printf("OBJECT CACHE HIT [%d][%d]: type=%d, id=%lu, name1=%s, name2=%s\n",hashslot,y,object_type,temp_object->object_id,(name1==NULL)?"NULL":name1,(name2==NULL)?"NULL":name2);
#endif
		*object_id = temp_object->object_id;
		result = NDO_OK;
	}
#ifdef NDO2DB_DEBUG_CACHING
	else {
		printf("OBJECT CACHE MISS: type=%d, name1=%s, name2=%s\n",object_type,(name1==NULL)?"NULL":name1,(name2==NULL)?"NULL":name2);
	}
#endif

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_get_cached_object_id(%lu) end\n", *object_id);

	return result;
}

int ndo2db_add_cached_object_id(ndo2db_idi *idi, int object_type, char *n1, char *n2, unsigned long object_id) {
	int result = NDO_OK;
	ndo2db_dbobject *temp_object = NULL;
	ndo2db_dbobject *lastpointer = NULL;
	ndo2db_dbobject *new_object = NULL;
	int x = 0;
	int y = 0;
	int hashslot = 0;
	int compare = 0;
	char *name1 = NULL;
	char *name2 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_add_cached_object_id() start\n");

	/* make sure empty strings are set to null */
	name1 = n1;
	name2 = n2;
	if (name1 && !strcmp(name1, ""))
		name1 = NULL;
	if (name2 && !strcmp(name2, ""))
		name2 = NULL;

	/* null names mean no object id, so don't cache */
	if (name1 == NULL && name2 == NULL) {
		return NDO_OK;
	}

#ifdef NDO2DB_DEBUG_CACHING
	printf("OBJECT CACHE ADD: type=%d, id=%lu, name1=%s, name2=%s\n",object_type,object_id,(name1==NULL)?"NULL":name1,(name2==NULL)?"NULL":name2);
#endif

	/* initialize hash list if necessary */
	if (idi->dbinfo.object_hashlist == NULL) {

		idi->dbinfo.object_hashlist = (ndo2db_dbobject **) malloc(
				sizeof(ndo2db_dbobject *) * NDO2DB_OBJECT_HASHSLOTS);
		if (idi->dbinfo.object_hashlist == NULL)
			return NDO_ERROR;

		for (x = 0; x < NDO2DB_OBJECT_HASHSLOTS; x++)
			idi->dbinfo.object_hashlist[x] = NULL;
	}

	/* allocate and populate new object */
	if ((new_object = (ndo2db_dbobject *) malloc(sizeof(ndo2db_dbobject)))
			==NULL)
		return NDO_ERROR;
	new_object->object_type = object_type;
	new_object->object_id = object_id;
	new_object->name1 = NULL;
	if (name1)
		new_object->name1 = strdup(name1);
	new_object->name2 = NULL;
	if (name2)
		new_object->name2 = strdup(name2);

	hashslot = ndo2db_object_hashfunc(new_object->name1, new_object->name2,
			NDO2DB_OBJECT_HASHSLOTS);

	lastpointer = NULL;
	for (temp_object = idi->dbinfo.object_hashlist[hashslot], y = 0; temp_object
			!= NULL; temp_object = temp_object->nexthash, y++) {
		compare = ndo2db_compare_object_hashdata(temp_object->name1,
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

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_add_cached_object_id() end\n");

	return result;
}

int ndo2db_object_hashfunc(const char *name1, const char *name2, int hashslots) {
	unsigned int i, result;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_object_hashfunc() start\n");

	result = 0;
	if (name1)
		for (i = 0; i < strlen(name1); i++)
			result += name1[i];

	if (name2)
		for (i = 0; i < strlen(name2); i++)
			result += name2[i];

	result = result % hashslots;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_object_hashfunc() end\n");

	return result;
}

int ndo2db_compare_object_hashdata(const char *val1a, const char *val1b,
		const char *val2a, const char *val2b) {
	int result = 0;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_compare_object_hashdata() start\n");

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

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_compare_object_hashdata() end\n");

	return result;
}

int ndo2db_free_cached_object_ids(ndo2db_idi *idi) {
	int x = 0;
	ndo2db_dbobject *temp_object = NULL;
	ndo2db_dbobject *next_object = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_free_cached_object_ids() start\n");

	if (idi == NULL)
		return NDO_OK;

	if (idi->dbinfo.object_hashlist) {

		for (x = 0; x < NDO2DB_OBJECT_HASHSLOTS; x++) {
			for (temp_object = idi->dbinfo.object_hashlist[x]; temp_object
					!=NULL; temp_object = next_object) {
				next_object = temp_object->nexthash;
				free(temp_object->name1);
				free(temp_object->name2);
				free(temp_object);
			}
		}

		free(idi->dbinfo.object_hashlist);
		idi->dbinfo.object_hashlist = NULL;
	}

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_free_cached_object_ids() end\n");
	
	return NDO_OK;
}

int ndo2db_set_all_objects_as_inactive(ndo2db_idi *idi) {
	int result = NDO_OK;
	char *buf = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_set_all_objects_as_inactive() start\n");

	/* mark all objects as being inactive */
	if (asprintf(&buf, "UPDATE %s SET is_active='0' WHERE instance_id='%lu'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS],
			idi->dbinfo.instance_id) == -1)
		buf = NULL;

	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_set_all_objects_as_inactive() end\n");

	return result;
}

int ndo2db_set_object_as_active(ndo2db_idi *idi, int object_type,
		unsigned long object_id) {
	int result = NDO_OK;
	char *buf = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_set_object_as_active() start\n");

	/* mark the object as being active */
	if (asprintf(
			&buf,
			"UPDATE %s SET is_active='1' WHERE instance_id='%lu' AND objecttype_id='%d' AND object_id='%lu'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS],
			idi->dbinfo.instance_id, object_type, object_id) == -1)
		buf = NULL;

	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_set_object_as_active() end\n");

	return result;
}

/****************************************************************************/
/* ARCHIVED LOG DATA HANDLER                                                */
/****************************************************************************/

int ndo2db_handle_logentry(ndo2db_idi *idi) {
	char *ptr = NULL;
	char *buf = NULL;
	char *es[1];
	time_t etime = 0L;
	char *ts[1];
	unsigned long type = 0L;
	int result = NDO_OK;
	int duplicate_record = NDO_FALSE;
	int len = 0;
	int x = 0;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_logentry() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* break log entry in pieces */
	if ((ptr = strtok(idi->buffered_input[NDO_DATA_LOGENTRY], "]")) == NULL)
		return NDO_ERROR;
	if ((ndo2db_convert_string_to_unsignedlong(ptr + 1,
			(unsigned long *) &etime)) == NDO_ERROR)
		return NDO_ERROR;
	ts[0] = ndo2db_db_timet_to_sql(idi, etime);
	if ((ptr = strtok(NULL, "\x0")) == NULL)
		return NDO_ERROR;
	es[0] = ndo2db_db_escape_string(idi, (ptr + 1));

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
	if (asprintf(
			&buf,
			"SELECT * FROM %s WHERE instance_id='%lu' AND logentry_time=%s AND logentry_data='%s'",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_LOGENTRIES],
			idi->dbinfo.instance_id, ts[0], es[0]) == -1)
		buf = NULL;
	if ((result = ndo2db_db_query(idi, buf)) == NDO_OK) {
			if (idi->dbinfo.dbi_result != NULL) {
				if (dbi_result_next_row(idi->dbinfo.dbi_result) != 0)
					duplicate_record = NDO_TRUE;
			}
		}

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	/*if(duplicate_record==NDO_TRUE && idi->last_logentry_time!=etime){*/
	/*if(duplicate_record==NDO_TRUE && strcmp((es[0]==NULL)?"":es[0],idi->dbinfo.last_logentry_data)){*/
	if (duplicate_record == NDO_TRUE) {
#ifdef NDO2DB_DEBUG
		printf("IGNORING DUPLICATE LOG RECORD!\n");
#endif
		return NDO_OK;
	}

	/* save entry to db */
	if (asprintf(
			&buf,
			"INSERT INTO %s (instance_id, logentry_time, entry_time, entry_time_usec, logentry_type, logentry_data, realtime_data, inferred_data_extracted) VALUES ('%lu', %s, %s, '0', '%lu', '%s', '0', '0')",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_LOGENTRIES],
			idi->dbinfo.instance_id, ts[0], ts[0], type, (es[0] == NULL) ? ""
					: es[0]) == -1)
		buf = NULL;
	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	/* record timestamp of last log entry */
	idi->dbinfo.last_logentry_time = etime;

	/* save last log entry (for detecting duplicates) */
	if (idi->dbinfo.last_logentry_data)
		free(idi->dbinfo.last_logentry_data);
	idi->dbinfo.last_logentry_data = strdup((es[0] == NULL) ? "" : es[0]);

	/* free memory */
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
                free(ts[x]);

	/* TODO - further processing of log entry to expand archived data... */

        ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_logentry() end\n");

	return result;
}

/****************************************************************************/
/* REALTIME DATA HANDLERS                                                   */
/****************************************************************************/

int ndo2db_handle_processdata(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long process_id;
	int result = NDO_OK;
	char *ts[1];
	char *es[3];
	int x = 0;
	char *buf = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_processdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* convert vars */
	result = ndo2db_convert_string_to_unsignedlong(
			idi->buffered_input[NDO_DATA_PROCESSID], &process_id);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);

	es[0] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_PROGRAMNAME]);
	es[1] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_PROGRAMVERSION]);
	es[2] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_PROGRAMDATE]);

	/* save entry to db */
	if (asprintf(
			&buf,
			"INSERT INTO %s (instance_id, event_type, event_time, event_time_usec, process_id, program_name, program_version, program_date) VALUES ('%lu', '%d', %s, '%lu', '%lu', '%s', '%s', '%s')",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_PROCESSEVENTS],
			idi->dbinfo.instance_id, type, ts[0], tstamp.tv_usec, process_id,
			es[0], es[1], es[2]) == -1)
		buf = NULL;
	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	/* MORE PROCESSING.... */

	/* if process is starting up, clearstatus data, event queue, etc. */
	if (type == NEBTYPE_PROCESS_PRELAUNCH && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		/* clear realtime data */
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTSTATUS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICESTATUS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTSTATUS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SCHEDULEDDOWNTIME]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_RUNTIMEVARIABLES]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS]);

		/* clear config data */
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILES]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILEVARIABLES]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMANDS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODTIMERANGES]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPMEMBERS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPMEMBERS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPMEMBERS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTDEPENDENCIES]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEDEPENDENCIES]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTADDRESSES]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTPARENTHOSTS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICES]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTGROUPS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTGROUPS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS]);
		ndo2db_db_clear_table(idi, ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS]);


		/* flag all objects as being inactive */
		ndo2db_set_all_objects_as_inactive(idi);

#ifdef BAD_IDEA
		/* record a fake log entry to indicate that Icinga is starting - this normally occurs during the module's "blackout period" */
		if(asprintf(&buf,"INSERT INTO %s (instance_id, logentry_time, logentry_type, logentry_data) VALUES ('%lu', %s, '%lu', 'Icinga %s starting... (PID=%lu)')"
						,ndo2db_db_tablenames[NDO2DB_DBTABLE_LOGENTRIES]
						,idi->dbinfo.instance_id
						,ts[0]
						,NSLOG_PROCESS_INFO
						,es[1]
						,process_id
				)==-1)
		buf=NULL;
		result=ndo2db_db_query(idi,buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
#endif
	}

	/* if process is shutting down or restarting, update process status data */
	if ((type == NEBTYPE_PROCESS_SHUTDOWN || type == NEBTYPE_PROCESS_RESTART)
			&& tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		if (asprintf(
				&buf,
				"UPDATE %s SET program_end_time=%s, is_currently_running='0' WHERE instance_id='%lu'",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS], ts[0],
				idi->dbinfo.instance_id) == -1)
			buf = NULL;
		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_processdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_timedeventdata(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int event_type = 0;
	unsigned long run_time = 0L;
	int recurring_event = 0;
	unsigned long object_id = 0L;
	int result = NDO_OK;
	char *ts[2];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_timedeventdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* convert vars */
	result = ndo2db_convert_string_to_int(
			idi->buffered_input[NDO_DATA_EVENTTYPE], &event_type);
	result = ndo2db_convert_string_to_int(
			idi->buffered_input[NDO_DATA_RECURRING], &recurring_event);
	result = ndo2db_convert_string_to_unsignedlong(
			idi->buffered_input[NDO_DATA_RUNTIME], &run_time);

	/* skip sleep events.... */
	if (type == NEBTYPE_TIMEDEVENT_SLEEP) {

		/* we could do some maintenance here if we wanted.... */

		return NDO_OK;
	}

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, run_time);

	/* get the object id (if applicable) */
	if (event_type == EVENT_SERVICE_CHECK || (event_type
			== EVENT_SCHEDULED_DOWNTIME
			&& idi->buffered_input[NDO_DATA_SERVICE] != NULL && strcmp(
			idi->buffered_input[NDO_DATA_SERVICE], "")))
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOST],
				idi->buffered_input[NDO_DATA_SERVICE], &object_id);
	if (event_type == EVENT_HOST_CHECK || (event_type
			== EVENT_SCHEDULED_DOWNTIME
			&& (idi->buffered_input[NDO_DATA_SERVICE] == NULL || !strcmp(
					idi->buffered_input[NDO_DATA_SERVICE], ""))))
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST,
				idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);

	/* HISTORICAL TIMED EVENTS */

	/* save a record of timed events that get added */
	if (type == NEBTYPE_TIMEDEVENT_ADD) {

		void *data[7];
		data[0] = (void *) &idi->dbinfo.instance_id; 
		data[1] = (void *) &event_type;
		data[2] = (void *) &ts[0];
		data[3] = (void *) &tstamp.tv_usec;
		data[4] = (void *) &ts[1];
		data[5] = (void *) &recurring_event;
		data[6] = (void *) &object_id;

		result = ido2db_query_insert_or_update_timedevent_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save a record of timed events that get executed.... */
	if (type == NEBTYPE_TIMEDEVENT_EXECUTE) {

		/* save entry to db */
                void *data[7];
                data[0] = (void *) &idi->dbinfo.instance_id;
                data[1] = (void *) &event_type;
                data[2] = (void *) &ts[0];
                data[3] = (void *) &tstamp.tv_usec;
                data[4] = (void *) &ts[1];
                data[5] = (void *) &recurring_event;
                data[6] = (void *) &object_id;

                result = ido2db_query_insert_or_update_timedevents_execute_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save a record of timed events that get removed.... */
	if (type == NEBTYPE_TIMEDEVENT_REMOVE) {

		/* save entry to db */
		if (asprintf(
				&buf,
				"UPDATE %s SET deletion_time=%s, deletion_time_usec='%lu' WHERE instance_id='%lu' AND event_type='%d' AND scheduled_time=%s AND recurring_event='%d' AND object_id='%lu'",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS], ts[0],
				tstamp.tv_usec, idi->dbinfo.instance_id, event_type, ts[1],
				recurring_event, object_id) == -1)
			buf = NULL;

		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* CURRENT TIMED EVENTS */

	/* remove (probably) expired events from the queue if client just connected */
	if (idi->dbinfo.clean_event_queue == NDO_TRUE && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		idi->dbinfo.clean_event_queue = NDO_FALSE;

		/* clear old entries from db */
		if (asprintf(
				&buf,
				"DELETE FROM %s WHERE instance_id='%lu' AND scheduled_time<=%s",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE],
				idi->dbinfo.instance_id, ts[0]) == -1)
			buf = NULL;
		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* ADD QUEUED TIMED EVENTS */
	if (type == NEBTYPE_TIMEDEVENT_ADD && tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db */
		if (asprintf(
				&buf,
				"INSERT INTO %s (instance_id, event_type, queued_time, queued_time_usec, scheduled_time, recurring_event, object_id) VALUES ('%lu', '%d', %s, '%lu', %s, '%d', '%lu')",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE],
				idi->dbinfo.instance_id, event_type, ts[0], tstamp.tv_usec,
				ts[1], recurring_event, object_id) == -1)
			buf = NULL;
		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* REMOVE QUEUED TIMED EVENTS */
	if ((type == NEBTYPE_TIMEDEVENT_REMOVE || type
			== NEBTYPE_TIMEDEVENT_EXECUTE) && tstamp.tv_sec
			>= idi->dbinfo.latest_realtime_data_time) {

		/* clear entry from db */
		if (asprintf(
				&buf,
				"DELETE FROM %s WHERE instance_id='%lu' AND event_type='%d' AND scheduled_time=%s AND recurring_event='%d' AND object_id='%lu'",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE],
				idi->dbinfo.instance_id, event_type, ts[1], recurring_event,
				object_id) == -1)
			buf = NULL;
		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);

		/* if we are executing a low-priority event, remove older events from the queue, as we know they've already been executed */
		/* THIS IS A HACK!  It shouldn't be necessary, but for some reason it is...  Otherwise not all events are removed from the queue. :-( */
		if (type == NEBTYPE_TIMEDEVENT_EXECUTE && (event_type
				== EVENT_SERVICE_CHECK || event_type == EVENT_HOST_CHECK)) {

			/* clear entries from db */
			if (asprintf(
					&buf,
					"DELETE FROM %s WHERE instance_id='%lu' AND scheduled_time<%s",
					ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE],
					idi->dbinfo.instance_id, ts[1]) == -1)
				buf = NULL;
			result = ndo2db_db_query(idi, buf);

			dbi_result_free(idi->dbinfo.dbi_result);
			free(buf);
		}

	}

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_timedeventdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_logdata(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	time_t etime = 0L;
	unsigned long letype = 0L;
	int result = NDO_OK;
	char *ts[2];
	char *es[1];
	char *buf = NULL;
	int len = 0;
	int x = 0;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_logdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* convert data */
	result = ndo2db_convert_string_to_unsignedlong(
			idi->buffered_input[NDO_DATA_LOGENTRYTYPE], &letype);
	result = ndo2db_convert_string_to_unsignedlong(
			idi->buffered_input[NDO_DATA_LOGENTRYTIME],
			(unsigned long *) &etime);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, etime);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_LOGENTRY]);

	/* strip newline chars from end */
	len = strlen(es[0]);
	for (x = len - 1; x >= 0; x--) {
		if (es[0][x] == '\n')
			es[0][x] = '\x0';
		else
			break;
	}

	/* save entry to db */
	if (asprintf(
			&buf,
			"INSERT INTO %s (instance_id, logentry_time, entry_time, entry_time_usec, logentry_type, logentry_data, realtime_data, inferred_data_extracted) VALUES ('%lu', %s, %s, '%lu', '%lu', '%s', '1', '1')",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_LOGENTRIES],
			idi->dbinfo.instance_id, ts[1], ts[0], tstamp.tv_usec, letype,
			es[0]) == -1)
		buf = NULL;
	result = ndo2db_db_query(idi, buf);
	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_logdata() start\n");

	return NDO_OK;
}

int ndo2db_handle_systemcommanddata(ndo2db_idi *idi) {
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
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_systemcommanddata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* covert vars */
	result = ndo2db_convert_string_to_int( idi->buffered_input[NDO_DATA_TIMEOUT], &timeout);
	result = ndo2db_convert_string_to_int( idi->buffered_input[NDO_DATA_EARLYTIMEOUT], &early_timeout);
	result = ndo2db_convert_string_to_int( idi->buffered_input[NDO_DATA_RETURNCODE], &return_code);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME], &execution_time);
	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME], &start_time);
	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME], &end_time);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMANDLINE]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_OUTPUT]);
	es[2] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_LONGOUTPUT]);

	ts[0] = ndo2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* save entry to db */
        void *data[11];
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

        result = ido2db_query_insert_or_update_systemcommanddata_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

        /* free memory */
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
                free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_systemcommanddata() end\n");

	return NDO_OK;
}

int ndo2db_handle_eventhandlerdata(ndo2db_idi *idi) {
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
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_eventhandlerdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* covert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTHANDLERTYPE], &eventhandler_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE], &state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE], &state_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT], &timeout);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT], &early_timeout);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE], &return_code);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME], &execution_time);
	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME], &start_time);
	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME], &end_time);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMANDARGS]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMANDLINE]);
	es[2] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_OUTPUT]);
	es[3] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_LONGOUTPUT]);

	ts[0] = ndo2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the object id */
	if (eventhandler_type == SERVICE_EVENTHANDLER || eventhandler_type == GLOBAL_SERVICE_EVENTHANDLER)
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOST], idi->buffered_input[NDO_DATA_SERVICE], &object_id);
	else
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST, idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);

	/* get the command id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND, idi->buffered_input[NDO_DATA_COMMANDNAME], NULL, &command_id);

	/* save entry to db */
        void *data[18];
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

        result = ido2db_query_insert_or_update_eventhandlerdata_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

        /* free memory */
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
                free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_eventhandlerdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_notificationdata(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *ts[2];
	char *es[2];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_notificationdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONTYPE], &notification_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONREASON], &notification_reason);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE], &state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATED], &escalated);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CONTACTSNOTIFIED], &contacts_notified);

	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME], &start_time);
	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME], &end_time);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_OUTPUT]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_LONGOUTPUT]);

	ts[0] = ndo2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the object id */
	if (notification_type == SERVICE_NOTIFICATION)
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOST], idi->buffered_input[NDO_DATA_SERVICE], &object_id);
	if (notification_type == HOST_NOTIFICATION)
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST, idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);

	/* save entry to db */
        void *data[13];
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

        result = ido2db_query_insert_or_update_notificationdata_add(idi, data);

	/* save the notification id for later use... */
	if (type == NEBTYPE_NOTIFICATION_START)
		idi->dbinfo.last_notification_id = 0L;
	if (result == NDO_OK && type == NEBTYPE_NOTIFICATION_START) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                idi->dbinfo.last_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_notificationdata(%lu) last_notification_id\n", idi->dbinfo.last_notification_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_notification_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_NOTIFICATIONS]) == -1)
                                        buf1 = NULL;

                                idi->dbinfo.last_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_notificationdata(%s=%lu) last_notification_id\n", buf1, idi->dbinfo.last_notification_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
        }

	dbi_result_free(idi->dbinfo.dbi_result);

        /* free memory */
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
                free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_notificationdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_contactnotificationdata(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long contact_id = 0L;
	struct timeval start_time;
	struct timeval end_time;
	int result = NDO_OK;
	char *ts[2];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactnotificationdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* convert vars */

	result = ndo2db_convert_string_to_timeval(
			idi->buffered_input[NDO_DATA_STARTTIME], &start_time);
	result = ndo2db_convert_string_to_timeval(
			idi->buffered_input[NDO_DATA_ENDTIME], &end_time);

	ts[0] = ndo2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the contact id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_CONTACT,
			idi->buffered_input[NDO_DATA_CONTACTNAME], NULL, &contact_id);

	/* save entry to db */
        void *data[7];
        data[0] = (void *) &idi->dbinfo.instance_id;
        data[1] = (void *) &idi->dbinfo.last_notification_id;
        data[2] = (void *) &ts[0];
        data[3] = (void *) &start_time.tv_usec;
        data[4] = (void *) &ts[1];
        data[5] = (void *) &end_time.tv_usec;
        data[6] = (void *) &contact_id;

        result = ido2db_query_insert_or_update_contactnotificationdata_add(idi, data);

	/* save the contact notification id for later use... */
	if (type == NEBTYPE_CONTACTNOTIFICATION_START)
		idi->dbinfo.last_contact_notification_id = 0L;
	if (result == NDO_OK && type == NEBTYPE_CONTACTNOTIFICATION_START) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                idi->dbinfo.last_contact_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactnotificationdata(%lu) contactnotification_id\n", idi->dbinfo.last_contact_notification_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_contactnotification_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONS]) == -1)
                                        buf1 = NULL;

                                idi->dbinfo.last_contact_notification_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactnotificationdata(%s=%lu) contactnotification_id\n", buf1, idi->dbinfo.last_contact_notification_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactnotificationdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_contactnotificationmethoddata(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long command_id = 0L;
	struct timeval start_time;
	struct timeval end_time;
	int result = NDO_OK;
	char *ts[2];
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactnotificationmethoddata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* convert vars */

	result = ndo2db_convert_string_to_timeval(
			idi->buffered_input[NDO_DATA_STARTTIME], &start_time);
	result = ndo2db_convert_string_to_timeval(
			idi->buffered_input[NDO_DATA_ENDTIME], &end_time);

	es[0] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_COMMANDARGS]);

	ts[0] = ndo2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the command id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND,
			idi->buffered_input[NDO_DATA_COMMANDNAME], NULL, &command_id);

	/* save entry to db */
        void *data[7];
        data[0] = (void *) &idi->dbinfo.instance_id;
        data[1] = (void *) &idi->dbinfo.last_contact_notification_id;
        data[2] = (void *) &ts[0];
        data[3] = (void *) &start_time.tv_usec;
        data[4] = (void *) &ts[1];
        data[5] = (void *) &end_time.tv_usec;
        data[6] = (void *) &command_id;

        result = ido2db_query_insert_or_update_contactnotificationmethoddata_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactnotificationmethoddata() end\n");

	return NDO_OK;
}

int ndo2db_handle_servicecheckdata(ndo2db_idi *idi) {
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
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2=NULL;
	char *buf3=NULL;

	int x = 0;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicecheckdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* only process some types of service checks... */
	if (type != NEBTYPE_SERVICECHECK_INITIATE && type
			!= NEBTYPE_SERVICECHECK_PROCESSED)
		return NDO_OK;

	/* skip precheck events - they aren't useful to us */
	if (type == NEBTYPE_SERVICECHECK_ASYNC_PRECHECK)
		return NDO_OK;

	/* covert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE], &check_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT], &current_check_attempt);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS], &max_check_attempts);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE], &state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE], &state_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT], &timeout);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT], &early_timeout);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE], &return_code);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME], &execution_time);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY], &latency);
	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME], &start_time);
	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME], &end_time);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMANDARGS]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMANDLINE]);
	es[2] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_OUTPUT]);
	es[3] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_LONGOUTPUT]);
	es[4] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_PERFDATA]);

	ts[0] = ndo2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE,
			idi->buffered_input[NDO_DATA_HOST],
			idi->buffered_input[NDO_DATA_SERVICE], &object_id);

	/* get the command id */
	if (idi->buffered_input[NDO_DATA_COMMANDNAME] != NULL && strcmp(
			idi->buffered_input[NDO_DATA_COMMANDNAME], ""))
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_COMMAND,
				idi->buffered_input[NDO_DATA_COMMANDNAME], NULL, &command_id);
	else
		command_id = 0L;

	/* save entry to db */
        void *data[19];
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

        result = ido2db_query_insert_or_update_servicecheckdata_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

        /* free memory */
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
                free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicecheckdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_hostcheckdata(ndo2db_idi *idi) {
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
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	int x = 0;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostcheckdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* only process finished host checks... */
	/*
	 if(type!=NEBTYPE_HOSTCHECK_PROCESSED)
	 return NDO_OK;
	 */

	/* skip precheck events - they aren't useful to us */
	if (type == NEBTYPE_HOSTCHECK_ASYNC_PRECHECK || type
			== NEBTYPE_HOSTCHECK_SYNC_PRECHECK)
		return NDO_OK;

	/* covert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE], &check_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT], &current_check_attempt);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS], &max_check_attempts);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE], &state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE], &state_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT], &timeout);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT], &early_timeout);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE], &return_code);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME], &execution_time);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY], &latency);
	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME], &start_time);
	result = ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME], &end_time);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMANDARGS]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMANDLINE]);
	es[2] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_OUTPUT]);
	es[3] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_LONGOUTPUT]);
	es[4] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_PERFDATA]);

	ts[0] = ndo2db_db_timet_to_sql(idi, start_time.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, end_time.tv_sec);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST,
			idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);

	/* get the command id */
	if (idi->buffered_input[NDO_DATA_COMMANDNAME] != NULL && strcmp(
			idi->buffered_input[NDO_DATA_COMMANDNAME], ""))
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_COMMAND,
				idi->buffered_input[NDO_DATA_COMMANDNAME], NULL, &command_id);
	else
		command_id = 0L;

	/* is this a raw check? */
	if (type == NEBTYPE_HOSTCHECK_RAW_START || type
			== NEBTYPE_HOSTCHECK_RAW_END)
		is_raw_check = 1;
	else
		is_raw_check = 0;

	/* save entry to db */
        void *data[23];
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
        data[22] = (void *) &es[3];

        result = ido2db_query_insert_or_update_hostcheckdata_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

        /* free memory */
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
                free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostcheckdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_commentdata(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *ts[3];
	char *es[2];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_commentdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_COMMENTTYPE], &comment_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ENTRYTYPE], &entry_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PERSISTENT], &is_persistent);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SOURCE], &comment_source);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EXPIRES], &expires);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_COMMENTID], &internal_comment_id);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENTRYTIME], &comment_time);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_EXPIRATIONTIME], &expire_time);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_AUTHORNAME]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMENT]);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, comment_time);
	ts[2] = ndo2db_db_timet_to_sql(idi, expire_time);

	/* get the object id */
	if (comment_type == SERVICE_COMMENT)
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOST],
				idi->buffered_input[NDO_DATA_SERVICE], &object_id);
	if (comment_type == HOST_COMMENT)
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST,
				idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);

	/* ADD HISTORICAL COMMENTS */
	/* save a record of comments that get added (or get loaded and weren't previously recorded).... */
	if (type == NEBTYPE_COMMENT_ADD || type == NEBTYPE_COMMENT_LOAD) {

		/* save entry to db */
	        void *data[14];
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

	        result = ido2db_query_insert_or_update_commentdata_add(idi, data, ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTHISTORY]);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* UPDATE HISTORICAL COMMENTS */
	/* mark records that have been deleted */
	if (type == NEBTYPE_COMMENT_DELETE) {

		/* update db entry */
		if (asprintf(
				&buf,
				"UPDATE %s SET deletion_time=%s, deletion_time_usec='%lu' WHERE instance_id='%lu' AND comment_time=%s AND internal_comment_id='%lu'",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTHISTORY], ts[0],
				tstamp.tv_usec, idi->dbinfo.instance_id, ts[1],
				internal_comment_id) == -1)
			buf = NULL;
		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* ADD CURRENT COMMENTS */
	if ((type == NEBTYPE_COMMENT_ADD || type == NEBTYPE_COMMENT_LOAD)
			&& tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db */
                void *data[14];
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

                result = ido2db_query_insert_or_update_commentdata_add(idi, data, ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTS]);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* REMOVE CURRENT COMMENTS */
	if (type == NEBTYPE_COMMENT_DELETE && tstamp.tv_sec
			>= idi->dbinfo.latest_realtime_data_time) {

		/* clear entry from db */
		if (asprintf(
				&buf,
				"DELETE FROM %s WHERE instance_id='%lu' AND comment_time=%s AND internal_comment_id='%lu'",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTS],
				idi->dbinfo.instance_id, ts[1], internal_comment_id) == -1)
			buf = NULL;
		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_commentdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_downtimedata(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *ts[4];
	char *es[2];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_downtimedata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_DOWNTIMETYPE], &downtime_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FIXED], &fixed);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_DURATION], &duration);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_DOWNTIMEID], &internal_downtime_id);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_TRIGGEREDBY], &triggered_by);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENTRYTIME], &entry_time);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_STARTTIME], &start_time);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENDTIME], &end_time);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_AUTHORNAME]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMENT]);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, entry_time);
	ts[2] = ndo2db_db_timet_to_sql(idi, start_time);
	ts[3] = ndo2db_db_timet_to_sql(idi, end_time);

	/* get the object id */
	if (downtime_type == SERVICE_DOWNTIME)
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOST],
				idi->buffered_input[NDO_DATA_SERVICE], &object_id);
	if (downtime_type == HOST_DOWNTIME)
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST,
				idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);

	/* HISTORICAL DOWNTIME */

	/* save a record of scheduled downtime that gets added (or gets loaded and wasn't previously recorded).... */
	if (type == NEBTYPE_DOWNTIME_ADD || type == NEBTYPE_DOWNTIME_LOAD) {

		/* save entry to db */
	        void *data[12];
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

	        result = ido2db_query_insert_or_update_downtimedata_add(idi, data, ndo2db_db_tablenames[NDO2DB_DBTABLE_DOWNTIMEHISTORY]);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save a record of scheduled downtime that starts */
	if (type == NEBTYPE_DOWNTIME_START) {

		/* save entry to db */
		if (asprintf(
				&buf,
				"UPDATE %s SET actual_start_time=%s, actual_start_time_usec='%lu', was_started='%d' WHERE instance_id='%lu' AND downtime_type='%d' AND object_id='%lu' AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_DOWNTIMEHISTORY], ts[0],
				tstamp.tv_usec, 1, idi->dbinfo.instance_id, downtime_type,
				object_id, ts[1], ts[2], ts[3]) == -1)
			buf = NULL;

		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* save a record of scheduled downtime that ends */
	if (type == NEBTYPE_DOWNTIME_STOP) {

		/* save entry to db */
		if (asprintf(
				&buf,
				"UPDATE %s SET actual_end_time=%s, actual_end_time_usec='%lu', was_cancelled='%d' WHERE instance_id='%lu' AND downtime_type='%d' AND object_id='%lu' AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_DOWNTIMEHISTORY], ts[0],
				tstamp.tv_usec, (attr == NEBATTR_DOWNTIME_STOP_CANCELLED) ? 1
						: 0, idi->dbinfo.instance_id, downtime_type, object_id,
				ts[1], ts[2], ts[3]) == -1)
			buf = NULL;

		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* CURRENT DOWNTIME */

	/* save a record of scheduled downtime that gets added (or gets loaded and wasn't previously recorded).... */
	if ((type == NEBTYPE_DOWNTIME_ADD || type == NEBTYPE_DOWNTIME_LOAD)
			&& tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db */
                void *data[12];
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

                result = ido2db_query_insert_or_update_downtimedata_add(idi, data, ndo2db_db_tablenames[NDO2DB_DBTABLE_SCHEDULEDDOWNTIME]);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save a record of scheduled downtime that starts */
	if (type == NEBTYPE_DOWNTIME_START && tstamp.tv_sec
			>= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db */
		if (asprintf(
				&buf,
				"UPDATE %s SET actual_start_time=%s, actual_start_time_usec='%lu', was_started='%d' WHERE instance_id='%lu' AND downtime_type='%d' AND object_id='%lu' AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_SCHEDULEDDOWNTIME], ts[0],
				tstamp.tv_usec, 1, idi->dbinfo.instance_id, downtime_type,
				object_id, ts[1], ts[2], ts[3]) == -1)
			buf = NULL;

		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* remove completed or deleted downtime */
	if ((type == NEBTYPE_DOWNTIME_STOP || type == NEBTYPE_DOWNTIME_DELETE)
			&& tstamp.tv_sec >= idi->dbinfo.latest_realtime_data_time) {

		/* save entry to db */
		if (asprintf(
				&buf,
				"DELETE FROM %s WHERE instance_id='%lu' AND downtime_type='%d' AND object_id='%lu' AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_SCHEDULEDDOWNTIME],
				idi->dbinfo.instance_id, downtime_type, object_id, ts[1],
				ts[2], ts[3]) == -1)
			buf = NULL;

		result = ndo2db_db_query(idi, buf);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
	}

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_downtimedata() end\n");

	return NDO_OK;
}

int ndo2db_handle_flappingdata(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *ts[2];
	char *buf = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_flappingdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* convert vars */
	result = ndo2db_convert_string_to_int( idi->buffered_input[NDO_DATA_FLAPPINGTYPE], &flapping_type);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_PERCENTSTATECHANGE], &percent_state_change);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LOWTHRESHOLD], &low_threshold);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HIGHTHRESHOLD], &high_threshold);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_COMMENTTIME], &comment_time);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_COMMENTID], &internal_comment_id);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, comment_time);

	/* get the object id (if applicable) */
	if (flapping_type == SERVICE_FLAPPING)
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOST],
				idi->buffered_input[NDO_DATA_SERVICE], &object_id);
	if (flapping_type == HOST_FLAPPING)
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST,
				idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);

	/* save entry to db */
	if (asprintf(
			&buf,
			"INSERT INTO %s (instance_id, event_time, event_time_usec, event_type, reason_type, flapping_type, object_id, percent_state_change, low_threshold, high_threshold, comment_time, internal_comment_id) VALUES ('%lu', %s, '%lu', '%d', '%d', '%d', '%lu', '%lf', '%lf', '%lf', %s, '%lu')",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_FLAPPINGHISTORY],
			idi->dbinfo.instance_id, ts[0], tstamp.tv_usec, type, attr,
			flapping_type, object_id, percent_state_change, low_threshold,
			high_threshold, ts[1], internal_comment_id) == -1)
		buf = NULL;
	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_flappingdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_programstatusdata(ndo2db_idi *idi) {
	int type, flags, attr;
	int x = 0;
	struct timeval tstamp;
	unsigned long program_start_time = 0L;
	unsigned long process_id = 0L;
	int daemon_mode = 0;
	unsigned long last_command_check = 0L;
	unsigned long last_log_rotation = 0L;
	int notifications_enabled = 0;
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
	char *ts[4];
	char *es[2];
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_programstatusdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* covert vars */
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_PROGRAMSTARTTIME], &program_start_time);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_PROCESSID], &process_id);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_DAEMONMODE], &daemon_mode);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTCOMMANDCHECK], &last_command_check);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTLOGROTATION], &last_log_rotation);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONSENABLED], &notifications_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVESERVICECHECKSENABLED], &active_service_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVESERVICECHECKSENABLED], &passive_service_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVEHOSTCHECKSENABLED], &active_host_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVEHOSTCHECKSENABLED], &passive_host_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTHANDLERSENABLED], &event_handlers_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSPERFORMANCEDATA], &process_performance_data);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERHOSTS], &obsess_over_hosts);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERSERVICES], &obsess_over_services);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDHOSTATTRIBUTES], &modified_host_attributes);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDSERVICEATTRIBUTES], &modified_service_attributes);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_GLOBALHOSTEVENTHANDLER]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_GLOBALSERVICEEVENTHANDLER]);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, program_start_time);
	ts[2] = ndo2db_db_timet_to_sql(idi, last_command_check);
	ts[3] = ndo2db_db_timet_to_sql(idi, last_log_rotation);

        void *data[22];
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

	/* save entry to db */
        result = ido2db_query_insert_or_update_programstatusdata_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_programstatusdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_hoststatusdata(ndo2db_idi *idi) {
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
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	unsigned long object_id = 0L;
	unsigned long check_timeperiod_object_id = 0L;
	int x = 0;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hoststatusdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* covert vars */
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTHOSTCHECK], &last_check);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_NEXTHOSTCHECK], &next_check);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTSTATECHANGE], &last_state_change);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTHARDSTATECHANGE], &last_hard_state_change);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEUP], &last_time_up);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEDOWN], &last_time_down);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEUNREACHABLE], &last_time_unreachable);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTHOSTNOTIFICATION], &last_notification);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_NEXTHOSTNOTIFICATION], &next_notification);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDHOSTATTRIBUTES], &modified_host_attributes);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_PERCENTSTATECHANGE], &percent_state_change);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY], &latency);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME], &execution_time);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTSTATE], &current_state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HASBEENCHECKED], &has_been_checked);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SHOULDBESCHEDULED], &should_be_scheduled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT], &current_check_attempt);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS], &max_check_attempts);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE], &check_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTHARDSTATE], &last_hard_state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE], &state_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOMORENOTIFICATIONS], &no_more_notifications);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONSENABLED], &notifications_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROBLEMHASBEENACKNOWLEDGED], &problem_has_been_acknowledged);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACKNOWLEDGEMENTTYPE], &acknowledgement_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTNOTIFICATIONNUMBER], &current_notification_number);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVEHOSTCHECKSENABLED], &passive_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVEHOSTCHECKSENABLED], &active_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTHANDLERENABLED], &event_handler_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ISFLAPPING], &is_flapping);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SCHEDULEDDOWNTIMEDEPTH], &scheduled_downtime_depth);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSPERFORMANCEDATA], &process_performance_data);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERHOST], &obsess_over_host);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_NORMALCHECKINTERVAL], &normal_check_interval);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_RETRYCHECKINTERVAL], &retry_check_interval);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_OUTPUT]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_LONGOUTPUT]);
	es[2] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_PERFDATA]);
	es[3] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_EVENTHANDLER]);
	es[4] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_CHECKCOMMAND]);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, last_check);
	ts[2] = ndo2db_db_timet_to_sql(idi, next_check);
	ts[3] = ndo2db_db_timet_to_sql(idi, last_state_change);
	ts[4] = ndo2db_db_timet_to_sql(idi, last_hard_state_change);
	ts[5] = ndo2db_db_timet_to_sql(idi, last_time_up);
	ts[6] = ndo2db_db_timet_to_sql(idi, last_time_down);
	ts[7] = ndo2db_db_timet_to_sql(idi, last_time_unreachable);
	ts[8] = ndo2db_db_timet_to_sql(idi, last_notification);
	ts[9] = ndo2db_db_timet_to_sql(idi, next_notification);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST, idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[NDO_DATA_HOSTCHECKPERIOD], NULL, &check_timeperiod_object_id);

	/* save entry to db */
        void *data[46];
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

        result = ido2db_query_insert_or_update_hoststatusdata_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

	/* save custom variables to db */
	result=ndo2db_save_custom_variables(idi,NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS,object_id,ts[0]);

        /* free memory */
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
                free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hoststatusdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_servicestatusdata(ndo2db_idi *idi) {
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
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	unsigned long object_id = 0L;
	unsigned long check_timeperiod_object_id = 0L;
	int x = 0;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicestatusdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* covert vars */
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTSERVICECHECK], &last_check);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_NEXTSERVICECHECK], &next_check);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTSTATECHANGE], &last_state_change);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTHARDSTATECHANGE], &last_hard_state_change);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEOK], &last_time_ok);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEWARNING], &last_time_warning);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEUNKNOWN], &last_time_unknown);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMECRITICAL], &last_time_critical);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTSERVICENOTIFICATION], &last_notification);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_NEXTSERVICENOTIFICATION], &next_notification);
	result = ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDSERVICEATTRIBUTES], &modified_service_attributes);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_PERCENTSTATECHANGE], &percent_state_change);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY], &latency);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME], &execution_time);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTSTATE], &current_state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HASBEENCHECKED], &has_been_checked);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SHOULDBESCHEDULED], &should_be_scheduled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT], &current_check_attempt);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS], &max_check_attempts);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE], &check_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTHARDSTATE], &last_hard_state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE], &state_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOMORENOTIFICATIONS], &no_more_notifications);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONSENABLED], &notifications_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROBLEMHASBEENACKNOWLEDGED], &problem_has_been_acknowledged);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACKNOWLEDGEMENTTYPE], &acknowledgement_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTNOTIFICATIONNUMBER], &current_notification_number);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVESERVICECHECKSENABLED], &passive_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVESERVICECHECKSENABLED], &active_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTHANDLERENABLED], &event_handler_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ISFLAPPING], &is_flapping);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SCHEDULEDDOWNTIMEDEPTH], &scheduled_downtime_depth);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSPERFORMANCEDATA], &process_performance_data);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERSERVICE], &obsess_over_service);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_NORMALCHECKINTERVAL], &normal_check_interval);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_RETRYCHECKINTERVAL], &retry_check_interval);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_OUTPUT]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_LONGOUTPUT]);
	es[2] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_PERFDATA]);
	es[3] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_EVENTHANDLER]);
	es[4] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_CHECKCOMMAND]);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, last_check);
	ts[2] = ndo2db_db_timet_to_sql(idi, next_check);
	ts[3] = ndo2db_db_timet_to_sql(idi, last_state_change);
	ts[4] = ndo2db_db_timet_to_sql(idi, last_hard_state_change);
	ts[5] = ndo2db_db_timet_to_sql(idi, last_time_ok);
	ts[6] = ndo2db_db_timet_to_sql(idi, last_time_warning);
	ts[7] = ndo2db_db_timet_to_sql(idi, last_time_unknown);
	ts[8] = ndo2db_db_timet_to_sql(idi, last_time_critical);
	ts[9] = ndo2db_db_timet_to_sql(idi, last_notification);
	ts[10] = ndo2db_db_timet_to_sql(idi, next_notification);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE,
			idi->buffered_input[NDO_DATA_HOST],
			idi->buffered_input[NDO_DATA_SERVICE], &object_id);
	result = ndo2db_get_object_id_with_insert(idi,
			NDO2DB_OBJECTTYPE_TIMEPERIOD,
			idi->buffered_input[NDO_DATA_SERVICECHECKPERIOD], NULL,
			&check_timeperiod_object_id);

	/* save entry to db */
        void *data[47];
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

        result = ido2db_query_insert_or_update_servicestatusdata_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
		free(es[x]);

	/* save custom variables to db */
	result=ndo2db_save_custom_variables(idi,NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS,object_id,ts[0]);

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicestatusdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_contactstatusdata(ndo2db_idi *idi) {
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
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	unsigned long object_id = 0L;
	int x = 0;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactstatusdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* covert vars */
	result = ndo2db_convert_string_to_unsignedlong( idi->buffered_input[NDO_DATA_LASTHOSTNOTIFICATION], &last_host_notification);
	result = ndo2db_convert_string_to_unsignedlong( idi->buffered_input[NDO_DATA_LASTSERVICENOTIFICATION], &last_service_notification);
	result = ndo2db_convert_string_to_unsignedlong( idi->buffered_input[NDO_DATA_MODIFIEDCONTACTATTRIBUTES], &modified_attributes);
	result = ndo2db_convert_string_to_unsignedlong( idi->buffered_input[NDO_DATA_MODIFIEDHOSTATTRIBUTES], &modified_host_attributes);
	result = ndo2db_convert_string_to_unsignedlong( idi->buffered_input[NDO_DATA_MODIFIEDSERVICEATTRIBUTES], &modified_service_attributes);
	result = ndo2db_convert_string_to_int( idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONSENABLED], &host_notifications_enabled);
	result = ndo2db_convert_string_to_int( idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONSENABLED], &service_notifications_enabled);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);
	ts[1] = ndo2db_db_timet_to_sql(idi, last_host_notification);
	ts[2] = ndo2db_db_timet_to_sql(idi, last_service_notification);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_CONTACT, idi->buffered_input[NDO_DATA_CONTACTNAME], NULL, &object_id);

	/* save entry to db */
        void *data[10];
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

        result = ido2db_query_insert_or_update_contactstatusdata_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

	/* save custom variables to db */
	result=ndo2db_save_custom_variables(idi,NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS,object_id,ts[0]);

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactstatusdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_adaptiveprogramdata(ndo2db_idi *idi) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_adaptiveprogramdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* IGNORED */
	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_adaptiveprogramdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_adaptivehostdata(ndo2db_idi *idi) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_adaptivehostdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* IGNORED */
	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_adaptivehostdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_adaptiveservicedata(ndo2db_idi *idi) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_adaptiveservicedata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* IGNORED */
	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_adaptiveservicedata() end\n");

	return NDO_OK;
}

int ndo2db_handle_adaptivecontactdata(ndo2db_idi *idi) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_adaptivecontactdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* IGNORED */
	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_adaptivecontactdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_externalcommanddata(ndo2db_idi *idi) {
	int type, flags, attr;
	int x = 0;
	struct timeval tstamp;
	char *ts = NULL;
	char *es[2];
	int command_type = 0;
	unsigned long entry_time = 0L;
	char *buf = NULL;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_externalcommanddata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* only handle start events */
	if (type != NEBTYPE_EXTERNALCOMMAND_START)
		return NDO_OK;

	/* covert vars */
	result = ndo2db_convert_string_to_int(
			idi->buffered_input[NDO_DATA_COMMANDTYPE], &command_type);
	result = ndo2db_convert_string_to_unsignedlong(
			idi->buffered_input[NDO_DATA_ENTRYTIME], &entry_time);

	es[0] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_COMMANDSTRING]);
	es[1] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_COMMANDARGS]);

	ts = ndo2db_db_timet_to_sql(idi, entry_time);

	/* save entry to db */
	if (asprintf(
			&buf,
			"INSERT INTO %s (instance_id, command_type, entry_time, command_name, command_args) VALUES ('%lu', '%d', %s, '%s', '%s')",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_EXTERNALCOMMANDS],
			idi->dbinfo.instance_id, command_type, ts, es[0], es[1]) == -1)
		buf = NULL;
	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

	/* free memory */
	free(ts);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_externalcommanddata() end\n");

	return NDO_OK;
}

int ndo2db_handle_aggregatedstatusdata(ndo2db_idi *idi) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_aggregatedstatusdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* IGNORED */

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_aggregatedstatusdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_retentiondata(ndo2db_idi *idi) {

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_retentiondata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* IGNORED */

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_retentiondata() end\n");

	return NDO_OK;
}

int ndo2db_handle_acknowledgementdata(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int acknowledgement_type = 0;
	int state = 0;
	int is_sticky = 0;
	int persistent_comment = 0;
	int notify_contacts = 0;
	unsigned long object_id = 0L;
	int result = NDO_OK;
	char *ts[1];
	char *es[2];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_acknowledgementdata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACKNOWLEDGEMENTTYPE], &acknowledgement_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE], &state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STICKY], &is_sticky);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PERSISTENT], &persistent_comment);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYCONTACTS], &notify_contacts);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_AUTHORNAME]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_COMMENT]);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);

	/* get the object id */
	if (acknowledgement_type == SERVICE_ACKNOWLEDGEMENT)
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOST], idi->buffered_input[NDO_DATA_SERVICE], &object_id);
	if (acknowledgement_type == HOST_ACKNOWLEDGEMENT)
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST, idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);

	/* save entry to db */
	/* NOTE Primary Key and only unique key is auto_increment thus ON DUPLICATE KEY will not occur ever */

        /* the data part of the INSERT statement */
        if(asprintf(&buf1,"(instance_id, entry_time, entry_time_usec, acknowledgement_type, object_id, state, author_name, comment_data, is_sticky, persistent_comment, notify_contacts) VALUES (%lu, %s, %lu, %d, %lu, %d, '%s', '%s', %d, %d, %d)"
		    ,idi->dbinfo.instance_id
		    ,ts[0]
		    ,tstamp.tv_usec
		    ,acknowledgement_type
		    ,object_id
		    ,state
		    ,es[0]
		    ,es[1]
		    ,is_sticky
		    ,persistent_comment
		    ,notify_contacts
                   )==-1)
                buf1=NULL;

        if(asprintf(&buf,"INSERT INTO %s %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_ACKNOWLEDGEMENTS]
                    ,buf1
                   )==-1)
                buf=NULL;
        free(buf1);

	result = ndo2db_db_query(idi, buf1);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);
	free(buf1);

	/* free memory */
	for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
		free(ts[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_acknowledgementdata() end\n");

	return NDO_OK;
}

int ndo2db_handle_statechangedata(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *ts[1];
	char *es[2];
	char *buf = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_statechangedata() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* only process completed state changes */
	if (type != NEBTYPE_STATECHANGE_END)
		return NDO_OK;

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATECHANGETYPE], &statechange_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATECHANGE], &state_change_occurred);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE], &state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE], &state_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT], &current_attempt);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS], &max_attempts);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTHARDSTATE], &last_hard_state);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTSTATE], &last_state);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_OUTPUT]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_LONGOUTPUT]);

	ts[0] = ndo2db_db_timet_to_sql(idi, tstamp.tv_sec);

	/* get the object id */
	if (statechange_type == SERVICE_STATECHANGE)
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOST],
				idi->buffered_input[NDO_DATA_SERVICE], &object_id);
	else
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST,
				idi->buffered_input[NDO_DATA_HOST], NULL, &object_id);

	/* save entry to db */
	if (asprintf(
			&buf,
			"INSERT INTO %s (instance_id, state_time, state_time_usec, object_id, state_change, state, state_type, current_check_attempt, max_check_attempts, last_state, last_hard_state, output, long_output) VALUES ('%lu', %s, '%lu', '%lu', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%s', '%s')",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_STATEHISTORY],
			idi->dbinfo.instance_id, ts[0], tstamp.tv_usec, object_id,
			state_change_occurred, state, state_type, current_attempt,
			max_attempts, last_state, last_hard_state, es[0], es[1]) == -1)
		buf = NULL;

	result = ndo2db_db_query(idi, buf);

	dbi_result_free(idi->dbinfo.dbi_result);
	free(buf);

        /* free memory */
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(es); x++)
                free(es[x]);
        for (x = 0; x < NAGIOS_SIZEOF_ARRAY(ts); x++)
                free(ts[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_statechangedata() end\n");

	return NDO_OK;
}

/****************************************************************************/
/* VARIABLE DATA HANDLERS                                                   */
/****************************************************************************/

int ndo2db_handle_configfilevariables(ndo2db_idi *idi, int configfile_type) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long configfile_id = 0L;
	int result = NDO_OK;
	char *es[3];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	char *varname = NULL;
	char *varvalue = NULL;
	ndo2db_mbuf mbuf;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_configfilevariables() start\n");
	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL, 0, "HANDLE_CONFIGFILEVARS [1]\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL, 0, "HANDLE_CONFIGFILEVARS [2]\n");
	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL, 0, "TSTAMP: %lu   LATEST: %lu\n", tstamp.tv_sec, idi->dbinfo.latest_realtime_data_time);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL, 0, "HANDLE_CONFIGFILEVARS [3]\n");

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_CONFIGFILENAME]);

	/* add config file to db */
        void *data[3];
        data[0] = (void *) &idi->dbinfo.instance_id;
        data[1] = (void *) &configfile_type;
        data[2] = (void *) &es[0];

        result = ido2db_query_insert_or_update_configfilevariables_add(idi, data);

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                configfile_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_configfilevariables(%lu) configfilevariables_id\n", configfile_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_configfilevariable_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILEVARIABLES]) == -1)
                                        buf1 = NULL;

                                configfile_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_configfilevariables(%s=%lu) configfilevariables_id\n", buf1, configfile_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	free(es[0]);

	/* save config file variables to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONFIGFILEVARIABLE];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get var name/val pair */
		varname = strtok(mbuf.buffer[x], "=");
		varvalue = strtok(NULL, "\x0");

		es[1] = ndo2db_db_escape_string(idi, varname);
		es[2] = ndo2db_db_escape_string(idi, varvalue);

		if (asprintf(
				&buf,
				"(instance_id, configfile_id, varname, varvalue) VALUES ('%lu', '%lu', '%s', '%s')",
				idi->dbinfo.instance_id, configfile_id, es[1], es[2]) == -1)
			buf = NULL;

		if (asprintf(&buf1, "INSERT INTO %s %s",
				ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILEVARIABLES], buf)
				== -1)
			buf1 = NULL;
		result = ndo2db_db_query(idi, buf1);

		dbi_result_free(idi->dbinfo.dbi_result);
		free(buf);
		free(buf1);

		free(es[1]);
		free(es[2]);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_configfilevariables() end\n");
	return NDO_OK;
}

int ndo2db_handle_configvariables(ndo2db_idi *idi) {

	if (idi == NULL)
		return NDO_ERROR;

	return NDO_OK;
}

int ndo2db_handle_runtimevariables(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int result = NDO_OK;
	char *es[2];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	char *varname = NULL;
	char *varvalue = NULL;
	ndo2db_mbuf mbuf;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_runtimevariables() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* save config file variables to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_RUNTIMEVARIABLE];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get var name/val pair */
		varname = strtok(mbuf.buffer[x], "=");
		varvalue = strtok(NULL, "\x0");

		es[0] = ndo2db_db_escape_string(idi, varname);
		es[1] = ndo2db_db_escape_string(idi, varvalue);
		
		/* save entry to db */
	        void *data[3];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &es[0];
	        data[2] = (void *) &es[1];

	        result = ido2db_query_insert_or_update_runtimevariables_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);

		free(es[0]);
		free(es[1]);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_runtimevariables() end\n");

	return NDO_OK;
}

/****************************************************************************/
/* OBJECT DEFINITION DATA HANDLERS                                          */
/****************************************************************************/

int ndo2db_handle_configdumpstart(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	int result = NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_configdumpstart() start\n");

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* set config dump type */
	if (idi->buffered_input[NDO_DATA_CONFIGDUMPTYPE] != NULL && !strcmp(
			idi->buffered_input[NDO_DATA_CONFIGDUMPTYPE],
			NDO_API_CONFIGDUMP_RETAINED))
		idi->current_object_config_type = 1;
	else
		idi->current_object_config_type = 0;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_configdumpstart() end\n");

	return NDO_OK;
}

int ndo2db_handle_configdumpend(ndo2db_idi *idi) {

	return NDO_OK;
}

int ndo2db_handle_hostdefinition(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *es[13];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	ndo2db_mbuf mbuf;
	char *cmdptr = NULL;
	char *argptr = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostdefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HOSTCHECKINTERVAL], &check_interval);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HOSTRETRYINTERVAL], &retry_interval);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTMAXCHECKATTEMPTS], &max_check_attempts);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_FIRSTNOTIFICATIONDELAY], &first_notification_delay);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONINTERVAL], &notification_interval);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTDOWN], &notify_on_down);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTUNREACHABLE], &notify_on_unreachable);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTRECOVERY],	&notify_on_recovery);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTFLAPPING],	&notify_on_flapping);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTDOWNTIME],	&notify_on_downtime);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKHOSTONUP], &stalk_on_up);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKHOSTONDOWN], &stalk_on_down);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKHOSTONUNREACHABLE], &stalk_on_unreachable);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTFLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONUP], &flap_detection_on_up);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONDOWN], &flap_detection_on_down);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONUNREACHABLE], &flap_detection_on_unreachable);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSHOSTPERFORMANCEDATA],	&process_performance_data);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTFRESHNESSCHECKSENABLED],	&freshness_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTFRESHNESSTHRESHOLD], &freshness_threshold);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVEHOSTCHECKSENABLED], &passive_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTEVENTHANDLERENABLED], &event_handler_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVEHOSTCHECKSENABLED], &active_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETAINHOSTSTATUSINFORMATION], &retain_status_information);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETAINHOSTNONSTATUSINFORMATION], &retain_nonstatus_information);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONSENABLED], &notifications_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERHOST], &obsess_over_host);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTFAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LOWHOSTFLAPTHRESHOLD], &low_flap_threshold);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HIGHHOSTFLAPTHRESHOLD], &high_flap_threshold);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HAVE2DCOORDS], &have_2d_coords);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_X2D], &x_2d);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_Y3D], &y_2d);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HAVE3DCOORDS], &have_3d_coords);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_X3D], &x_3d);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_Y3D], &y_3d);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_Z3D], &z_3d);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_HOSTADDRESS]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS]);

	/* get the check command */
	cmdptr = strtok(idi->buffered_input[NDO_DATA_HOSTCHECKCOMMAND], "!");
	argptr = strtok(NULL, "\x0");
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &check_command_id);
	es[2] = ndo2db_db_escape_string(idi, argptr);

	/* get the event handler command */
	cmdptr = strtok(idi->buffered_input[NDO_DATA_HOSTEVENTHANDLER], "!");
	argptr = strtok(NULL, "\x0");
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &eventhandler_command_id);
	es[3] = ndo2db_db_escape_string(idi, argptr);

	es[4] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_NOTES]);
	es[5] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_NOTESURL]);
	es[6] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_ACTIONURL]);
	es[7] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_ICONIMAGE]);
	es[8] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_ICONIMAGEALT]);
	es[9] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_VRMLIMAGE]);
	es[10] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_STATUSMAPIMAGE]);
	es[11] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_DISPLAYNAME]);
	es[12] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_HOSTALIAS]);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST, idi->buffered_input[NDO_DATA_HOSTNAME], NULL, &object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi, NDO2DB_OBJECTTYPE_HOST, object_id);

	/* get the timeperiod ids */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[NDO_DATA_HOSTCHECKPERIOD], NULL, &check_timeperiod_id);
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONPERIOD], NULL, &notification_timeperiod_id);

	/* add definition to db */

	/* prepare var handler for data array */
	for(x = 0; x < 13; x++) {
		if(es[x] == NULL) {
			es[x] = "";
		}
	}	

	/* save entry to db */
        void *data[57];
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

        result = ido2db_query_insert_or_update_hostdefinition_definition_add(idi, data);

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                host_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostdefinitio(%lu) host_id\n", host_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_host_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTS]) == -1)
                                        buf1 = NULL;

                                host_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostdefinitio(%s=%lu) host_id\n", buf1, host_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	for (x = 0; x < 13; x++) {
		/* before we've prepared NULL values with "", but string literals cannot be free'd! */
		if(es[x] == "") 
			continue;
		free(es[x]);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostdefinition() free es\n");

	/* save parent hosts to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_PARENTHOST];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST,
				mbuf.buffer[x], NULL, &member_id);

		/* save entry to db */
	        void *data[3];
	        data[0] = (void *) &idi->dbinfo.instance_id;
        	data[1] = (void *) &host_id;
	        data[2] = (void *) &member_id;

	        result = ido2db_query_insert_or_update_hostdefinition_parenthosts_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save contact groups to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACTGROUP];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_CONTACTGROUP, mbuf.buffer[x], NULL,
				&member_id);

                /* save entry to db */
                void *data[3];
                data[0] = (void *) &idi->dbinfo.instance_id;
                data[1] = (void *) &host_id;
                data[2] = (void *) &member_id;

                result = ido2db_query_insert_or_update_hostdefinition_contactgroups_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save contacts to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACT];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		/* NOTE: UPDATE senseless upon unique constraint - FIXME */
                /* save entry to db */
                void *data[3];
                data[0] = (void *) &idi->dbinfo.instance_id;
                data[1] = (void *) &host_id;
                data[2] = (void *) &member_id;

		result = ido2db_query_insert_or_update_hostdefinition_contacts_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save custom variables to db */
	result=ndo2db_save_custom_variables(idi,NDO2DB_DBTABLE_CUSTOMVARIABLES,object_id,NULL);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostdefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_hostgroupdefinition(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long group_id = 0L;
	unsigned long member_id = 0L;
	int result = NDO_OK;
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	ndo2db_mbuf mbuf;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostgroupdefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_HOSTGROUPALIAS]);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOSTGROUP, idi->buffered_input[NDO_DATA_HOSTGROUPNAME], NULL, &object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi, NDO2DB_OBJECTTYPE_HOSTGROUP, object_id);

	/* add definition to db */
        void *data[4];
        data[0] = (void *) &idi->dbinfo.instance_id;
        data[1] = (void *) &idi->current_object_config_type;
        data[2] = (void *) &object_id;
        data[3] = (void *) &es[0];

        result = ido2db_query_insert_or_update_hostgroupdefinition_definition_add(idi, data);

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostgroupdefinition(%lu) hostgroup_id\n", group_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_hostgroup_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPS]) == -1)
                                        buf1 = NULL;

                                group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostgroupdefinition(%s=%lu) hostgroup_id\n", buf1, group_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	free(es[0]);

	/* save hostgroup members to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_HOSTGROUPMEMBER];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST,
				mbuf.buffer[x], NULL, &member_id);

	        /* save entry to db */
		void *data[3];
        	data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &group_id;
	        data[2] = (void *) &member_id;

	        result = ido2db_query_insert_or_update_hostgroupdefinition_hostgroupmembers_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostgroupdefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_servicedefinition(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *es[9];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	ndo2db_mbuf mbuf;
	char *cmdptr = NULL;
	char *argptr = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicedefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_SERVICECHECKINTERVAL], &check_interval);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_SERVICERETRYINTERVAL], &retry_interval);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXSERVICECHECKATTEMPTS], &max_check_attempts);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_FIRSTNOTIFICATIONDELAY], &first_notification_delay);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONINTERVAL], &notification_interval);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEWARNING], &notify_on_warning);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEUNKNOWN], &notify_on_unknown);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICECRITICAL], &notify_on_critical);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICERECOVERY], &notify_on_recovery);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEFLAPPING], &notify_on_flapping);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEDOWNTIME], &notify_on_downtime);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKSERVICEONOK], &stalk_on_ok);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKSERVICEONWARNING], &stalk_on_warning);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKSERVICEONUNKNOWN], &stalk_on_unknown);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKSERVICEONCRITICAL], &stalk_on_critical);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEISVOLATILE], &is_volatile);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEFLAPDETECTIONENABLED], &flap_detection_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONOK], &flap_detection_on_ok);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONWARNING], &flap_detection_on_warning);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONUNKNOWN], &flap_detection_on_unknown);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONCRITICAL], &flap_detection_on_critical);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSSERVICEPERFORMANCEDATA], &process_performance_data);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEFRESHNESSCHECKSENABLED], &freshness_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEFRESHNESSTHRESHOLD], &freshness_threshold);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVESERVICECHECKSENABLED], &passive_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEEVENTHANDLERENABLED], &event_handler_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVESERVICECHECKSENABLED], &active_checks_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETAINSERVICESTATUSINFORMATION], &retain_status_information);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETAINSERVICENONSTATUSINFORMATION], &retain_nonstatus_information);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONSENABLED], &notifications_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERSERVICE], &obsess_over_service);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEFAILUREPREDICTIONENABLED], &failure_prediction_enabled);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LOWSERVICEFLAPTHRESHOLD], &low_flap_threshold);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HIGHSERVICEFLAPTHRESHOLD], &high_flap_threshold);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS]);

	/* get the check command */
	cmdptr = strtok(idi->buffered_input[NDO_DATA_SERVICECHECKCOMMAND], "!");
	argptr = strtok(NULL, "\x0");
	
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &check_command_id);
	
	es[1] = ndo2db_db_escape_string(idi, argptr);

	/* get the event handler command */
	cmdptr = strtok(idi->buffered_input[NDO_DATA_SERVICEEVENTHANDLER], "!");
	argptr = strtok(NULL, "\x0");
	
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &eventhandler_command_id);

	es[2] = ndo2db_db_escape_string(idi, argptr);
	es[3] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_NOTES]);
	es[4] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_NOTESURL]);
	es[5] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_ACTIONURL]);
	es[6] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_ICONIMAGE]);
	es[7] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_ICONIMAGEALT]);
	es[8] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_DISPLAYNAME]);

	/* get the object ids */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOSTNAME], idi->buffered_input[NDO_DATA_SERVICEDESCRIPTION], &object_id);
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST, idi->buffered_input[NDO_DATA_HOSTNAME], NULL, &host_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi, NDO2DB_OBJECTTYPE_SERVICE, object_id);

	/* get the timeperiod ids */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[NDO_DATA_SERVICECHECKPERIOD], NULL, &check_timeperiod_id);
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONPERIOD], NULL, &notification_timeperiod_id);

	/* add definition to db */

        /* prepare var handler for data array */
        for(x = 0; x < 9; x++) {
                if(es[x] == NULL) {
                        es[x] = "";
                }
        }

	/* save entry to db */
        void *data[51];
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

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                service_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicedefinition(%lu) service_id\n", service_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_service_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICES]) == -1)
                                        buf1 = NULL;

                                service_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicedefinition(%s=%lu) service_id\n", buf1, service_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }

	}

	dbi_result_free(idi->dbinfo.dbi_result);

	for (x = 0; x < 9; x++) {
                /* before we've prepared NULL values with "", but string literals cannot be free'd! */
                if(es[x] == "")
                        continue;
		free(es[x]);
	}

	/* save contact groups to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACTGROUP];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_CONTACTGROUP, mbuf.buffer[x], NULL,
				&member_id);

                /* save entry to db */
                void *data[3];
                data[0] = (void *) &idi->dbinfo.instance_id;
                data[1] = (void *) &service_id;
                data[2] = (void *) &member_id;

                result = ido2db_query_insert_or_update_servicedefinition_contactgroups_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save contacts to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACT];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		/* NOTE: UPDATE senseless upon unique constraint - FIXME */
		/* save entry to db */
	        void *data[3];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &service_id;
	        data[2] = (void *) &member_id;
		
		result = ido2db_query_insert_or_update_servicedefinition_contacts_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save custom variables to db */
	result=ndo2db_save_custom_variables(idi,NDO2DB_DBTABLE_CUSTOMVARIABLES,object_id,NULL);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicedefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_servicegroupdefinition(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long group_id = 0L;
	unsigned long member_id = 0L;
	int result = NDO_OK;
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	ndo2db_mbuf mbuf;
	char *hptr = NULL;
	char *sptr = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicegroupdefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_SERVICEGROUPALIAS]);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi,
			NDO2DB_OBJECTTYPE_SERVICEGROUP,
			idi->buffered_input[NDO_DATA_SERVICEGROUPNAME], NULL, &object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi, NDO2DB_OBJECTTYPE_SERVICEGROUP, object_id);

	/* add definition to db */
        void *data[4];
        data[0] = (void *) &idi->dbinfo.instance_id;
        data[1] = (void *) &idi->current_object_config_type;
        data[2] = (void *) &object_id;
        data[3] = (void *) &es[0];

        result = ido2db_query_insert_or_update_servicegroupdefinition_definition_add(idi, data);

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicegroupdefinition(%lu) group_id\n", group_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_servicegroup_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPS]) == -1)
                                        buf1 = NULL;

                                group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicegroupdefinition(%s=%lu) group_id\n", buf1, group_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	free(es[0]);

	/* save members to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_SERVICEGROUPMEMBER];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* split the host/service name */
		hptr = strtok(mbuf.buffer[x], ";");
		sptr = strtok(NULL, "\x0");

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_SERVICE, hptr, sptr, &member_id);

		/* save entry to db */
	        void *data[3];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &group_id;
	        data[2] = (void *) &member_id;

		result = ido2db_query_insert_or_update_servicegroupdefinition_members_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicegroupdefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_hostdependencydefinition(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostdependencydefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_DEPENDENCYTYPE], &dependency_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_INHERITSPARENT], &inherits_parent);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONUP], &fail_on_up);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONDOWN], &fail_on_down);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONUNREACHABLE], &fail_on_unreachable);

	/* get the object ids */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST, idi->buffered_input[NDO_DATA_HOSTNAME], NULL, &object_id);
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST, idi->buffered_input[NDO_DATA_DEPENDENTHOSTNAME], NULL, &dependent_object_id);
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[NDO_DATA_DEPENDENCYPERIOD], NULL, &timeperiod_object_id);

	/* add definition to db */
        void *data[10];
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
	dbi_result_free(idi->dbinfo.dbi_result);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostdependencydefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_servicedependencydefinition(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicedependencydefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_DEPENDENCYTYPE], &dependency_type);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_INHERITSPARENT], &inherits_parent);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONOK], &fail_on_ok);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONWARNING], &fail_on_warning);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONUNKNOWN], &fail_on_unknown);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONCRITICAL], &fail_on_critical);

	/* get the object ids */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOSTNAME], idi->buffered_input[NDO_DATA_SERVICEDESCRIPTION], &object_id);
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_DEPENDENTHOSTNAME], idi->buffered_input[NDO_DATA_DEPENDENTSERVICEDESCRIPTION], &dependent_object_id);
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[NDO_DATA_DEPENDENCYPERIOD], NULL, &timeperiod_object_id);

	/* add definition to db */
        void *data[11];
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
	dbi_result_free(idi->dbinfo.dbi_result);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicedependencydefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_hostescalationdefinition(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	ndo2db_mbuf mbuf;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostescalationdefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FIRSTNOTIFICATION], &first_notification);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTNOTIFICATION], &last_notification);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_NOTIFICATIONINTERVAL], &notification_interval);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONRECOVERY], &escalate_recovery);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONDOWN], &escalate_down);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONUNREACHABLE], &escalate_unreachable);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST, idi->buffered_input[NDO_DATA_HOSTNAME], NULL, &object_id);

	/* get the timeperiod id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[NDO_DATA_ESCALATIONPERIOD], NULL, &timeperiod_id);

	/* add definition to db */
        void *data[10];
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

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                escalation_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostescalationdefinition(%lu) escalation_id\n", escalation_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_hostescalation_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONS]) == -1)
                                        buf1 = NULL;

                                escalation_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostescalationdefinition(%s=%lu) escalation_id\n", buf1, escalation_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	/* save contact groups to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACTGROUP];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_CONTACTGROUP, mbuf.buffer[x], NULL,
				&member_id);

		/* save entry to db */
	        void *data[3];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &escalation_id;
	        data[2] = (void *) &member_id;

	        result = ido2db_query_insert_or_update_hostescalationdefinition_contactgroups_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save contacts to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACT];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		/* NOTE: UPDATE senseless upon unique constraint - FIXME */
		/* save entry tp db */
	        void *data[3];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &escalation_id;
	        data[2] = (void *) &member_id;

	        result = ido2db_query_insert_or_update_hostescalationdefinition_contacts_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_hostescalationdefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_serviceescalationdefinition(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	ndo2db_mbuf mbuf;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicetescalationdefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FIRSTNOTIFICATION], &first_notification);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTNOTIFICATION], &last_notification);
	result = ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_NOTIFICATIONINTERVAL], &notification_interval);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONRECOVERY], &escalate_recovery);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONWARNING], &escalate_warning);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONUNKNOWN], &escalate_unknown);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONCRITICAL], &escalate_critical);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE, idi->buffered_input[NDO_DATA_HOSTNAME], idi->buffered_input[NDO_DATA_SERVICEDESCRIPTION], &object_id);

	/* get the timeperiod id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, idi->buffered_input[NDO_DATA_ESCALATIONPERIOD], NULL, &timeperiod_id);

	/* add definition to db */
        void *data[11];
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

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                escalation_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_serviceescalationdefinition(%lu) escalation_id\n", escalation_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_serviceescalation_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONS]) == -1)
                                        buf1 = NULL;

                                escalation_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_serviceescalationdefinition(%s=%lu) escalation_id\n", buf1, escalation_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	/* save contact groups to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACTGROUP];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_CONTACTGROUP, mbuf.buffer[x], NULL,
				&member_id);

		/* save entry to db */
	        void *data[3];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &escalation_id;
	        data[2] = (void *) &member_id;

		result = ido2db_query_insert_or_update_serviceescalationdefinition_contactgroups_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	/* save contacts to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACT];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		/* NOTE: UPDATE senseless upon unique constraint - FIXME */
		/* save entry to db */
	        void *data[3];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &escalation_id;
	        data[2] = (void *) &member_id;

	        result = ido2db_query_insert_or_update_serviceescalationdefinition_contacts_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_servicetescalationdefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_commanddefinition(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	int result = NDO_OK;
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_commanddefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_COMMANDLINE]);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND,
			idi->buffered_input[NDO_DATA_COMMANDNAME], NULL, &object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi, NDO2DB_OBJECTTYPE_COMMAND, object_id);

	/* add definition to db */
        void *data[4];
        data[0] = (void *) &idi->dbinfo.instance_id;
        data[1] = (void *) &object_id;
        data[2] = (void *) &idi->current_object_config_type;
        data[3] = (void *) &es[0];

        result = ido2db_query_insert_or_update_commanddefinition_definition_add(idi, data);
	dbi_result_free(idi->dbinfo.dbi_result);

	for (x = 0; x < 1; x++)
		free(es[x]);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_commanddefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_timeperiodefinition(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	ndo2db_mbuf mbuf;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_timeperiodefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_TIMEPERIODALIAS]);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi,
			NDO2DB_OBJECTTYPE_TIMEPERIOD,
			idi->buffered_input[NDO_DATA_TIMEPERIODNAME], NULL, &object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, object_id);

	/* add definition to db */
        void *data[4];
        data[0] = (void *) &idi->dbinfo.instance_id;
        data[1] = (void *) &idi->current_object_config_type;
        data[2] = (void *) &object_id;
        data[3] = (void *) &es[0];

        result = ido2db_query_insert_or_update_timeperiodefinition_definition_add(idi, data);

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                timeperiod_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_timeperiodefinition(%lu) timeperiod_id\n", timeperiod_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_timeperiod_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODS]) == -1)
                                        buf1 = NULL;

                                timeperiod_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_timeperiodefinition(%s=%lu) timeperiod_id\n", buf1, timeperiod_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	free(es[0]);

	/* save timeranges to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_TIMERANGE];
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

		/* save entry to db */	
	        void *data[5];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &timeperiod_id;
	        data[2] = (void *) &day;
	        data[3] = (void *) &start_sec;
	        data[4] = (void *) &end_sec;

	        result = ido2db_query_insert_or_update_timeperiodefinition_timeranges_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_timeperiodefinition() end\n");

	return NDO_OK;
}

int ndo2db_handle_contactdefinition(ndo2db_idi *idi) {
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
	int result = NDO_OK;
	char *es[3];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	ndo2db_mbuf mbuf;
	char *numptr = NULL;
	char *addressptr = NULL;
	int address_number = 0;
	char *cmdptr = NULL;
	char *argptr = NULL;

	int tmp1 = HOST_NOTIFICATION;
	int tmp2 = SERVICE_NOTIFICATION;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactdefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONSENABLED], &host_notifications_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONSENABLED], &service_notifications_enabled);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CANSUBMITCOMMANDS], &can_submit_commands);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEWARNING], &notify_service_warning);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEUNKNOWN], &notify_service_unknown);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICECRITICAL], &notify_service_critical);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICERECOVERY], &notify_service_recovery);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEFLAPPING], &notify_service_flapping);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEDOWNTIME], &notify_service_downtime);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTDOWN], &notify_host_down);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTUNREACHABLE], &notify_host_unreachable);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTRECOVERY], &notify_host_recovery);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTFLAPPING],	&notify_host_flapping);
	result = ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTDOWNTIME], &notify_host_downtime);

	es[0] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_CONTACTALIAS]);
	es[1] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_EMAILADDRESS]);
	es[2] = ndo2db_db_escape_string(idi, idi->buffered_input[NDO_DATA_PAGERADDRESS]);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_CONTACT,
			idi->buffered_input[NDO_DATA_CONTACTNAME], NULL, &contact_id);

	/* get the timeperiod ids */
	result = ndo2db_get_object_id_with_insert(idi,
			NDO2DB_OBJECTTYPE_TIMEPERIOD,
			idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONPERIOD], NULL,
			&host_timeperiod_id);
	result = ndo2db_get_object_id_with_insert(idi,
			NDO2DB_OBJECTTYPE_TIMEPERIOD,
			idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONPERIOD], NULL,
			&service_timeperiod_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi, NDO2DB_OBJECTTYPE_CONTACT, contact_id);

	/* add definition to db */
        void *data[22];
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

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                contact_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactdefinition(ndo2db_idi *idi)(%lu) contact_id\n", contact_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_contact_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTS]) == -1)
                                        buf1 = NULL;

                                contact_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactdefinition(ndo2db_idi *idi)(%s=%lu) contact_id\n", buf1, contact_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	for (x = 0; x < 3; x++)
		free(es[x]);

	/* save addresses to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACTADDRESS];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		numptr = strtok(mbuf.buffer[x], ":");
		addressptr = strtok(NULL, "\x0");

		if (numptr == NULL || addressptr == NULL)
			continue;

		address_number = atoi(numptr);
		es[0] = ndo2db_db_escape_string(idi, addressptr);

		/* save entry to db */
	        void *data[4];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &contact_id;
	        data[2] = (void *) &address_number;
	        data[3] = (void *) &es[0];

	        result = ido2db_query_insert_or_update_contactdefinition_addresses_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);

		free(es[0]);
	}

	/* save host notification commands to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACTADDRESS];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		cmdptr = strtok(mbuf.buffer[x], "!");
		argptr = strtok(NULL, "\x0");

		if (numptr == NULL)
		//if (cmdptr == NULL || argptr == NULL)
			continue;

		/* find the command */
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &command_id);

		es[0] = ndo2db_db_escape_string(idi, argptr);

		if(es[0] == NULL) {
			es[0] = "";
		}

		/* save entry to db */
	        void *data[5];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &contact_id;
	        data[2] = (void *) &tmp1;
	        data[3] = (void *) &command_id;
	        data[4] = (void *) &es[0];

	        result = ido2db_query_insert_or_update_contactdefinition_hostnotificationcommands_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);
	
		if(es[0] != "") {
			free(es[0]);
		}
	}

	/* save service notification commands to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACTADDRESS];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		cmdptr = strtok(mbuf.buffer[x], "!");
		argptr = strtok(NULL, "\x0");

		if (numptr == NULL)
		//if (cmdptr == NULL || argptr == NULL)
			continue;

		/* find the command */
		result = ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND, cmdptr, NULL, &command_id);

		es[0] = ndo2db_db_escape_string(idi, argptr);

                if(es[0] == NULL) {
                        es[0] = "";
                }

		/* save entry to db */

	        void *data[5];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &contact_id;
	        data[2] = (void *) &tmp2;
	        data[3] = (void *) &command_id;
	        data[4] = (void *) &es[0];

	        result = ido2db_query_insert_or_update_contactdefinition_servicenotificationcommands_add(idi, data);
		dbi_result_free(idi->dbinfo.dbi_result);

		if(es[0] != "") {
			free(es[0]);
		}
	}

	/* save custom variables to db */
	result=ndo2db_save_custom_variables(idi,NDO2DB_DBTABLE_CUSTOMVARIABLES,contact_id,NULL);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactdefinition() end\n");

	return NDO_OK;
}

int ndo2db_save_custom_variables(ndo2db_idi *idi,int table_idx, int o_id, char *ts ){
	char *buf=NULL;
	char *buf1=NULL;
	char *buf2=NULL;
	char *buf3=NULL;
	ndo2db_mbuf mbuf;
	char *es[1];
	char *ptr1=NULL;
	char *ptr2=NULL;
	char *ptr3=NULL;
	int result=NDO_OK;
	int has_been_modified=0;
	int x=0;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_save_custom_variables() start\n");

	/* save custom variables to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CUSTOMVARIABLE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		if((ptr1=strtok(mbuf.buffer[x],":"))==NULL)
			continue;

		es[0]=strdup(ptr1);

		if((ptr2=strtok(NULL,":"))==NULL)
			continue;

		has_been_modified=atoi(ptr2);
		ptr3=strtok(NULL,"\n");
		buf1=strdup((ptr3==NULL)?"":ptr3);
		es[1]=ndo2db_db_escape_string(idi,buf1);
		free(buf1);

		if(es[0] == NULL) {
			es[0] = "";
		}
		if(es[1] == NULL) {
			es[1] = "";
		}

		if (table_idx==NDO2DB_DBTABLE_CUSTOMVARIABLES) {

			/* save entry to db */
		        void *data[6];
		        data[0] = (void *) &idi->dbinfo.instance_id;
		        data[1] = (void *) &o_id;
		        data[2] = (void *) &idi->current_object_config_type;
		        data[3] = (void *) &has_been_modified;
		        data[4] = (void *) &es[0];
		        data[5] = (void *) &es[1];

		        result = ido2db_query_insert_or_update_save_custom_variables_customvariables_add(idi, data);
	                dbi_result_free(idi->dbinfo.dbi_result);

		}
		if (table_idx==NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS) {

			/* save entry to db */
		        void *data[6];
		        data[0] = (void *) &idi->dbinfo.instance_id;
		        data[1] = (void *) &o_id;
		        data[2] = (void *) &ts[0];
		        data[3] = (void *) &has_been_modified;
		        data[4] = (void *) &es[0];
		        data[5] = (void *) &es[1];

		        result = ido2db_query_insert_or_update_save_custom_variables_customvariablestatus_add(idi, data);
			dbi_result_free(idi->dbinfo.dbi_result);
		}

		free(es[0]);
		free(es[1]);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_save_custom_variables() end\n");

	return result;

}

int ndo2db_handle_contactgroupdefinition(ndo2db_idi *idi) {
	int type, flags, attr;
	struct timeval tstamp;
	unsigned long object_id = 0L;
	unsigned long group_id = 0L;
	unsigned long member_id = 0L;
	int result = NDO_OK;
	char *es[1];
	int x = 0;
	char *buf = NULL;
	char *buf1 = NULL;
	char *buf2 = NULL;
	char *buf3 = NULL;
	ndo2db_mbuf mbuf;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactgroupdefinition() start\n");

	if (idi == NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result = ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr,
			&tstamp);

	/* don't store old data */
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0] = ndo2db_db_escape_string(idi,
			idi->buffered_input[NDO_DATA_CONTACTGROUPALIAS]);

	/* get the object id */
	result = ndo2db_get_object_id_with_insert(idi,
			NDO2DB_OBJECTTYPE_CONTACTGROUP,
			idi->buffered_input[NDO_DATA_CONTACTGROUPNAME], NULL, &object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi, NDO2DB_OBJECTTYPE_CONTACTGROUP, object_id);

	/* add definition to db */
        void *data[4];
        data[0] = (void *) &idi->dbinfo.instance_id;
        data[1] = (void *) &idi->current_object_config_type;
        data[2] = (void *) &object_id;
        data[3] = (void *) &es[0];

        result = ido2db_query_insert_or_update_contactgroupdefinition_definition_add(idi, data);

	if (result == NDO_OK) {

                switch (idi->dbinfo.server_type) {
                        case NDO2DB_DBSERVER_MYSQL:
                                /* mysql doesn't use sequences */
                                group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, NULL);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactgroupdefinition(%lu) group_id\n", group_id);
                                break;
                        case NDO2DB_DBSERVER_PGSQL:
                                /* depending on tableprefix/tablename a sequence will be used */
                                if(asprintf(&buf1, "%s_contactgroup_id_seq", ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPS]) == -1)
                                        buf1 = NULL;

                                group_id = dbi_conn_sequence_last(idi->dbinfo.dbi_conn, buf1);
                                ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactgroupdefinition(%s=%lu) group_id\n", buf1, group_id);
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
#ifdef USE_ORACLE
#endif
                                break;
                        case NDO2DB_DBSERVER_SQLITE:
                                break;
                        case NDO2DB_DBSERVER_SQLITE3:
                                break;
                        default:
                                break;
                }
	}

	dbi_result_free(idi->dbinfo.dbi_result);

	free(es[0]);

	/* save contact group members to db */
	mbuf = idi->mbuf[NDO2DB_MBUF_CONTACTGROUPMEMBER];
	for (x = 0; x < mbuf.used_lines; x++) {

		if (mbuf.buffer[x] == NULL)
			continue;

		/* get the object id of the member */
		result = ndo2db_get_object_id_with_insert(idi,
				NDO2DB_OBJECTTYPE_CONTACT, mbuf.buffer[x], NULL, &member_id);

		/* save entry to db */
	        void *data[3];
	        data[0] = (void *) &idi->dbinfo.instance_id;
	        data[1] = (void *) &group_id;
	        data[2] = (void *) &member_id;

	        result = ido2db_query_insert_or_update_contactgroupdefinition_contactgroupmembers_add(idi, data);		
		dbi_result_free(idi->dbinfo.dbi_result);
	}

	ndo2db_log_debug_info(NDO2DB_DEBUGL_PROCESSINFO, 2, "ndo2db_handle_contactgroupdefinition() end\n");

	return NDO_OK;
}
