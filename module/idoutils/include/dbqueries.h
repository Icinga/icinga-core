/************************************************************************
 *
 * DBQUERIES.H - IDO2DB DB QUERY Handler Include File
 *
 * Copyright (c) 2009 Icinga Development Team (http://www.icinga.org)
 *
 * Last Modified: 07-25-2009
 ************************************************************************/

#ifndef _IDO2DB_DBQUERIES_H
#define _IDO2DB_DBQUERIES_H

#include "ido2db.h"

#define NAGIOS_SIZEOF_ARRAY(var)       (sizeof(var)/sizeof(var[0]))

/* TIMEDEVENTS */
int ido2db_query_insert_or_update_timedevents_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_timedevents_execute_add(ndo2db_idi *idi, void **data);

/* SYSTEMCOMMANDS */
int ido2db_query_insert_or_update_systemcommanddata_add(ndo2db_idi *idi, void **data);

/* EVENTHANDLER */
int ido2db_query_insert_or_update_eventhandlerdata_add(ndo2db_idi *idi, void **data);

/* NOTIFICATIONS */
int ido2db_query_insert_or_update_notificationdata_add(ndo2db_idi *idi, void **data);

/* CONTACTNOTIFICATIONS */
int ido2db_query_insert_or_update_contactnotificationdata_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_contactnotificationmethoddata_add(ndo2db_idi *idi, void **data);

/* SERVICECHECKS */
int ido2db_query_insert_or_update_servicecheckdata_add(ndo2db_idi *idi, void **data);

/* HOSTCHECKS */
int ido2db_query_insert_or_update_hostcheckdata_add(ndo2db_idi *idi, void **data);

/* COMMENTS */
int ido2db_query_insert_or_update_commentdata_add(ndo2db_idi *idi, void **data, char *table_name);

/* DOWNTIME */
int ido2db_query_insert_or_update_downtimedata_add(ndo2db_idi *idi, void **data, char *table_name);

/* PROGRAMSTATUS */
int ido2db_query_insert_or_update_programstatusdata_add(ndo2db_idi *idi, void **data);

/* HOSTSTATUS */
int ido2db_query_insert_or_update_hoststatusdata_add(ndo2db_idi *idi, void **data);

/* SERVICESTATUS */
int ido2db_query_insert_or_update_servicestatusdata_add(ndo2db_idi *idi, void **data);

#endif
