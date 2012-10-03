/************************************************************************
 *
 * DBHANDLERS.H - IDO2DB DB Handler Include File
 * Copyright (c) 2005-2006 Ethan Galstad
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 ************************************************************************/

#ifndef _IDO2DB_DBHANDLERS_H
#define _IDO2DB_DBHANDLERS_H

#include "ido2db.h"

#define ICINGA_SIZEOF_ARRAY(var)       (sizeof(var)/sizeof(var[0]))

int ido2db_get_object_id(ido2db_idi *,int,char *,char *,unsigned long *);
int ido2db_get_object_id_with_insert(ido2db_idi *,int,char *,char *,unsigned long *);

int ido2db_get_cached_object_ids(ido2db_idi *);
int ido2db_get_cached_object_id(ido2db_idi *,int,char *,char *,unsigned long *);
int ido2db_add_cached_object_id(ido2db_idi *,int,char *,char *,unsigned long);
int ido2db_free_cached_object_ids(ido2db_idi *);

int ido2db_object_hashfunc(const char *,const char *,int);
int ido2db_compare_object_hashdata(const char *,const char *,const char *,const char *);

int ido2db_set_all_objects_as_inactive(ido2db_idi *);
int ido2db_set_object_as_active(ido2db_idi *,int,unsigned long);

int ido2db_handle_logentry(ido2db_idi *);
int ido2db_handle_processdata(ido2db_idi *);
int ido2db_handle_timedeventdata(ido2db_idi *);
int ido2db_handle_logdata(ido2db_idi *);
int ido2db_handle_systemcommanddata(ido2db_idi *);
int ido2db_handle_eventhandlerdata(ido2db_idi *);
int ido2db_handle_notificationdata(ido2db_idi *);
int ido2db_handle_contactnotificationdata(ido2db_idi *);
int ido2db_handle_contactnotificationmethoddata(ido2db_idi *);
int ido2db_handle_servicecheckdata(ido2db_idi *);
int ido2db_handle_hostcheckdata(ido2db_idi *);
int ido2db_handle_commentdata(ido2db_idi *);
int ido2db_handle_downtimedata(ido2db_idi *);
int ido2db_handle_flappingdata(ido2db_idi *);
int ido2db_handle_programstatusdata(ido2db_idi *);
int ido2db_handle_hoststatusdata(ido2db_idi *);
int ido2db_handle_servicestatusdata(ido2db_idi *);
int ido2db_handle_contactstatusdata(ido2db_idi *);
int ido2db_handle_adaptiveprogramdata(ido2db_idi *);
int ido2db_handle_adaptivehostdata(ido2db_idi *);
int ido2db_handle_adaptiveservicedata(ido2db_idi *);
int ido2db_handle_adaptivecontactdata(ido2db_idi *);
int ido2db_handle_externalcommanddata(ido2db_idi *);
int ido2db_handle_aggregatedstatusdata(ido2db_idi *);
int ido2db_handle_retentiondata(ido2db_idi *);
int ido2db_handle_acknowledgementdata(ido2db_idi *);
int ido2db_handle_statechangedata(ido2db_idi *);
int ido2db_handle_configfilevariables(ido2db_idi *,int);
int ido2db_handle_configvariables(ido2db_idi *);
int ido2db_handle_runtimevariables(ido2db_idi *);
int ido2db_handle_configdumpstart(ido2db_idi *);
int ido2db_handle_configdumpend(ido2db_idi *);
int ido2db_handle_hostdefinition(ido2db_idi *);
int ido2db_handle_hostgroupdefinition(ido2db_idi *);
int ido2db_handle_servicedefinition(ido2db_idi *);
int ido2db_handle_servicegroupdefinition(ido2db_idi *);
int ido2db_handle_hostdependencydefinition(ido2db_idi *);
int ido2db_handle_servicedependencydefinition(ido2db_idi *);
int ido2db_handle_hostescalationdefinition(ido2db_idi *);
int ido2db_handle_serviceescalationdefinition(ido2db_idi *);
int ido2db_handle_commanddefinition(ido2db_idi *);
int ido2db_handle_timeperiodefinition(ido2db_idi *);
int ido2db_handle_contactdefinition(ido2db_idi *);
int ido2db_handle_contactgroupdefinition(ido2db_idi *);
int ido2db_save_custom_variables(ido2db_idi *,int, unsigned long, char *, unsigned long);
int ido2db_handle_object_enable_disable(ido2db_idi *, int);

#endif
