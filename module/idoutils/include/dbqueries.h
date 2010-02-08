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
int ido2db_query_insert_or_update_timedeventqueue_add(ndo2db_idi *idi, void **data);

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
int ido2db_query_insert_or_update_commentdata_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_commentdata_history_add(ndo2db_idi *idi, void **data);

/* DOWNTIME */
int ido2db_query_insert_or_update_downtimedata_scheduled_downtime_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_downtimedata_downtime_history_add(ndo2db_idi *idi, void **data);

/* PROGRAMSTATUS */
int ido2db_query_insert_or_update_programstatusdata_add(ndo2db_idi *idi, void **data);

/* HOSTSTATUS */
int ido2db_query_insert_or_update_hoststatusdata_add(ndo2db_idi *idi, void **data);

/* SERVICESTATUS */
int ido2db_query_insert_or_update_servicestatusdata_add(ndo2db_idi *idi, void **data);

/* CONTACTSTATUS */
int ido2db_query_insert_or_update_contactstatusdata_add(ndo2db_idi *idi, void **data);

/* CONFIGFILEVARIABLES */
int ido2db_query_insert_or_update_configfilevariables_add(ndo2db_idi *idi, void **data);

/* RUNTIMEVARIABLES */
int ido2db_query_insert_or_update_runtimevariables_add(ndo2db_idi *idi, void **data);

/* HOSTDEFINITION */
int ido2db_query_insert_or_update_hostdefinition_definition_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_hostdefinition_parenthosts_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_hostdefinition_contactgroups_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_hostdefinition_contacts_add(ndo2db_idi *idi, void **data);

/* HOSTGROUPDEFINITION */
int ido2db_query_insert_or_update_hostgroupdefinition_definition_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_hostgroupdefinition_hostgroupmembers_add(ndo2db_idi *idi, void **data);

/* SERVICEDEFINITION */
int ido2db_query_insert_or_update_servicedefinition_definition_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_servicedefinition_contactgroups_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_servicedefinition_contacts_add(ndo2db_idi *idi, void **data);

/* SERVICEGROUPDEFINITION */
int ido2db_query_insert_or_update_servicegroupdefinition_definition_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_servicegroupdefinition_members_add(ndo2db_idi *idi, void **data);

/* HOSTDEPENDENCYDEFINITION */
int ido2db_query_insert_or_update_hostdependencydefinition_definition_add(ndo2db_idi *idi, void **data);

/* SERVICEDEPENDENCYDEFINITION */
int ido2db_query_insert_or_update_servicedependencydefinition_definition_add(ndo2db_idi *idi, void **data);

/* HOSTESCALATIONDEFINITION */
int ido2db_query_insert_or_update_hostescalationdefinition_definition_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_hostescalationdefinition_contactgroups_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_hostescalationdefinition_contacts_add(ndo2db_idi *idi, void **data);

/* SERVICEESCALATIONDEFINITION */
int ido2db_query_insert_or_update_serviceescalationdefinition_definition_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_serviceescalationdefinition_contactgroups_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_serviceescalationdefinition_contacts_add(ndo2db_idi *idi, void **data);

/*  COMMANDDEFINITION */
int ido2db_query_insert_or_update_commanddefinition_definition_add(ndo2db_idi *idi, void **data);

/*  TIMEPERIODDEFINITION */
int ido2db_query_insert_or_update_timeperiodefinition_definition_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_timeperiodefinition_timeranges_add(ndo2db_idi *idi, void **data);

/* CONTACTDEFINITION */
int ido2db_query_insert_or_update_contactdefinition_definition_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_contactdefinition_addresses_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_contactdefinition_notificationcommands_add(ndo2db_idi *idi, void **data);

/* CUSTOMVARIABLES */
int ido2db_query_insert_or_update_save_custom_variables_customvariables_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_save_custom_variables_customvariablestatus_add(ndo2db_idi *idi, void **data);

/* CONTACTGROUPDEFINITION */
int ido2db_query_insert_or_update_contactgroupdefinition_definition_add(ndo2db_idi *idi, void **data);
int ido2db_query_insert_or_update_contactgroupdefinition_contactgroupmembers_add(ndo2db_idi *idi, void **data);

#endif
