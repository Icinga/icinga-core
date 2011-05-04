/*
-- --------------------------------------------------------
-- oracle_move_icinga13_objects.sql
-- oracle schema update script for icinga V1.3.0 to V1.4.0
-- called by oracle-upgrade-1.4.0.sql
-- --------------------------------------------------------

-- Feature #1354 seperate Data, Index and Lobs https://dev.icinga.org/issues/1354
-- Feature #1355 drop unnessary constraints and rename remaining https://dev.icinga.org/issues/1355
-- alter sequences nocache

-- works with Oracle10+ and sqlplus
-- initial version: 2011-03-01 Thomas Dressler
-- current version: 2011-05-01 Thomas Dressler
--
-- this will ask you for the tablespace names unless you run it from oracle-upgrade-1.4.0.sql 
-- or defined it previous in defines (eg. define IDXTBS=<yourDATATBS> ....)
-- make sure you have quota on these tablespaces 
-- --------------------------------------------------------
*/
-- -----------------------------------------
-- set sqlplus parameter
-- -----------------------------------------
set sqlprompt "&&_USER@&&_CONNECT_IDENTIFIER SQL>"
set serveroutput on size 900000
set echo on
set feedback on



/* error handling and logging */
whenever sqlerror exit failure
spool move_icinga13_objects.log

/* 
-- --------------------------------------------------------
-- drop existing constraints (check, pk, unique) 
-- implements Feature #1354
-- https://dev.icinga.org/issues/1354
-- --------------------------------------------------------
*/
declare
cursor c is 
	select constraint_name,table_name from user_constraints i
	where table_name not like 'BIN$%' and constraint_type in ('C','P','U');
r c%rowtype;
s varchar2(2000);
begin
open c;
  loop
    fetch c into r;
    exit when c%notfound;
    s:='ALTER TABLE '||r.table_name|| ' drop constraint '||r.constraint_name;
    dbms_output.put_line(s);
    execute immediate s;
  end loop;
close c;
end;
/

-- --------------------------------------------------------
-- move icinga index/lobs to dedicated tablespace
-- implements Feature #1355
-- https://dev.icinga.org/issues/1355
-- --------------------------------------------------------

/* rebuild index in index tablespace */
declare
cursor c is select index_name,table_name from user_indexes where index_type ='NORMAL';
r c%rowtype;
s varchar2(2000);
begin
open c;
  loop
    fetch c into r;
    exit when c%notfound;
    s:='ALTER INDEX '||r.index_name|| ' rebuild tablespace &&IXTBS';
    dbms_output.put_line(s);
    execute immediate s;
  end loop;
close c;
end;
/

/* move lobs out of table segment */
ALTER TABLE hostchecks MOVE TABLESPACE &&DATATBS LOB (long_output) STORE AS hostchecks_out_lob(TABLESPACE &&LOBTBS);
ALTER TABLE hoststatus MOVE TABLESPACE &&DATATBS LOB (long_output) STORE AS hoststatus_out_lob(TABLESPACE &&LOBTBS);
ALTER TABLE notifications MOVE TABLESPACE &&DATATBS LOB (long_output) STORE AS notifications_out_lob(TABLESPACE &&LOBTBS);
ALTER TABLE servicechecks MOVE TABLESPACE &&DATATBS LOB (long_output) STORE AS servicechecks_out_lob(TABLESPACE &&LOBTBS);
ALTER TABLE servicestatus MOVE TABLESPACE &&DATATBS LOB (long_output) STORE AS servicestatus_out_lob(TABLESPACE &&LOBTBS);
ALTER TABLE statehistory MOVE TABLESPACE &&DATATBS LOB (long_output) STORE AS statehistory_out_lob(TABLESPACE &&LOBTBS);
ALTER TABLE systemcommands MOVE TABLESPACE &&DATATBS LOB (long_output) STORE AS systemcommands_out_lob(TABLESPACE &&LOBTBS);

/*recreate PK/UQ in index tablspace */
alter table acknowledgements add constraint acknowledgements_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table commands add constraint commands_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table commands add constraint  commands_uq UNIQUE (instance_id,object_id,config_type)using index tablespace &&IXTBS;
alter table commenthistory add constraint commenthist_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table commenthistory add constraint commenthist_uq  UNIQUE (instance_id,comment_time,internal_comment_id)using index tablespace &&IXTBS;
alter table comments add constraint comments_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table comments add constraint comments_uq  UNIQUE (instance_id,comment_time,internal_comment_id)using index tablespace &&IXTBS;
alter table configfiles add constraint configfiles_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table configfiles add constraint configfiles_uq  UNIQUE (instance_id,configfile_type,configfile_path)using index tablespace &&IXTBS;
alter table configfilevariables add constraint configfilevar_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table configfilevariables add constraint configfilevar_uq  UNIQUE (instance_id,configfile_id,varname)using index tablespace &&IXTBS;
alter table conninfo add constraint conninfo_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table contact_addresses add constraint contact_addresses_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table contact_addresses add CONSTRAINT contact_addresses_uq UNIQUE (contact_id,address_number)using index tablespace &&IXTBS;
alter table contact_notificationcommands add constraint contact_notifi_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table contact_notificationcommands add CONSTRAINT contact_notifi_uq UNIQUE (contact_id,notification_type,command_object_id,command_args)using index tablespace &&IXTBS;
alter table contactgroup_members add constraint contactgroup_members_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table contactgroup_members add CONSTRAINT contactgroup_members_uq UNIQUE (contactgroup_id,contact_object_id)using index tablespace &&IXTBS;
alter table contactgroups add constraint contactgroups_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table contactgroups add CONSTRAINT contactgroups_uq UNIQUE (instance_id,config_type,contactgroup_object_id)using index tablespace &&IXTBS;
alter table contactnotificationmethods add constraint contactnotifi_meth_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table contactnotificationmethods add CONSTRAINT contactnotifi_meth_uq UNIQUE (instance_id,contactnotification_id,start_time,start_time_usec)using index tablespace &&IXTBS;
alter table contactnotifications add constraint contactnotifi_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table contactnotifications add CONSTRAINT contactnotifi_uq UNIQUE (instance_id,contact_object_id,start_time,start_time_usec)using index tablespace &&IXTBS;
alter table contacts add constraint contacts_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table contacts add CONSTRAINT contacts_uq UNIQUE (instance_id,config_type,contact_object_id)using index tablespace &&IXTBS;
alter table contactstatus add constraint contactstatus_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table contactstatus add CONSTRAINT contactstatus_uq UNIQUE (contact_object_id)using index tablespace &&IXTBS;
alter table customvariables add constraint customvariables_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table customvariables add CONSTRAINT customvariables_uq UNIQUE (object_id,config_type,varname)using index tablespace &&IXTBS;
alter table customvariablestatus add constraint customvariablest_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table customvariablestatus add CONSTRAINT customvariablest_uq UNIQUE (object_id,varname)using index tablespace &&IXTBS;
alter table dbversion add constraint dbversion_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table dbversion add CONSTRAINT dbversion_uq UNIQUE (name)using index tablespace &&IXTBS;
alter table downtimehistory add constraint downtimehistory_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table downtimehistory add CONSTRAINT downtimehistory_uq UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)using index tablespace &&IXTBS;
alter table eventhandlers add constraint eventhandlers_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table eventhandlers add CONSTRAINT eventhandlers_uq UNIQUE (instance_id,object_id,start_time,start_time_usec)using index tablespace &&IXTBS;
alter table externalcommands add constraint externalcommands_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table flappinghistory add constraint flappinghistory_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table host_contactgroups add constraint host_contactgroups_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table host_contactgroups add CONSTRAINT host_contactgroups_uq UNIQUE (host_id,contactgroup_object_id)using index tablespace &&IXTBS;
alter table host_contacts add constraint host_contacts_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table host_parenthosts add constraint host_parenthosts_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table host_parenthosts add CONSTRAINT host_parenthosts_uq UNIQUE (host_id,parent_host_object_id)using index tablespace &&IXTBS;
alter table hostchecks add constraint hostchecks PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table hostdependencies add constraint hostdependencies_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table hostdependencies add CONSTRAINT hostdependencies_uq UNIQUE (instance_id,config_type,host_object_id,dependent_host_object_id,dependency_type,inherits_parent,fail_on_up,fail_on_down,fail_on_unreachable)using index tablespace &&IXTBS;
alter table hostescalation_contactgroups add constraint h_e_contactgroups_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table hostescalation_contactgroups add CONSTRAINT h_e_contactgroups_uq UNIQUE (hostescalation_id,contactgroup_object_id)using index tablespace &&IXTBS;
alter table hostescalation_contacts add constraint h_e_contacts_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table hostescalation_contacts add CONSTRAINT h_e_contacts_uq UNIQUE (instance_id,hostescalation_id,contact_object_id)using index tablespace &&IXTBS;
alter table hostescalations add constraint hostescalations_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table hostescalations add CONSTRAINT hostescalations_uq UNIQUE (instance_id,config_type,host_object_id,timeperiod_object_id,first_notification,last_notification)using index tablespace &&IXTBS;
alter table hostgroup_members add constraint hostgroup_members_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table hostgroup_members add CONSTRAINT hostgroup_members_uq UNIQUE (hostgroup_id,host_object_id)using index tablespace &&IXTBS;
alter table hostgroups add constraint hostgroups_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table hostgroups add CONSTRAINT hostgroups_uq UNIQUE (instance_id,hostgroup_object_id)using index tablespace &&IXTBS;
alter table hosts add constraint hosts_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table hosts add CONSTRAINT hosts_uq UNIQUE (instance_id,config_type,host_object_id)using index tablespace &&IXTBS;
alter table hoststatus add constraint hoststatus_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table hoststatus add CONSTRAINT hoststatus_uq UNIQUE (host_object_id)using index tablespace &&IXTBS;
alter table instances add constraint instances_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table logentries add constraint logentries_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table notifications add constraint notifications_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table notifications add CONSTRAINT notifications_uq UNIQUE (instance_id,object_id,start_time,start_time_usec)using index tablespace &&IXTBS;
alter table objects add constraint objects_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table processevents add constraint processevents_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table programstatus add constraint programstatus_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table programstatus add CONSTRAINT programstatus_uq UNIQUE (instance_id)using index tablespace &&IXTBS;
alter table runtimevariables add constraint runtimevariables_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table runtimevariables add CONSTRAINT runtimevariables_uq UNIQUE (instance_id,varname)using index tablespace &&IXTBS;
alter table scheduleddowntime add constraint scheduleddowntime_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table scheduleddowntime add CONSTRAINT scheduleddowntime_uq UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)using index tablespace &&IXTBS;
alter table service_contactgroups add constraint service_contactgroups_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table service_contactgroups add CONSTRAINT service_contactgroups_uq UNIQUE (service_id,contactgroup_object_id)using index tablespace &&IXTBS;
alter table service_contacts add constraint service_contacts_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table servicechecks add constraint servicechecks_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table servicedependencies add constraint servicedependencies_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table servicedependencies add CONSTRAINT servicedependencies_uq UNIQUE (instance_id,config_type,service_object_id,dependent_service_object_id,dependency_type,inherits_parent,fail_on_ok,fail_on_warning,fail_on_unknown,fail_on_critical)using index tablespace &&IXTBS;
alter table serviceescalationcontactgroups add constraint serv_esc_groups_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table serviceescalation_contacts add constraint serv_esc_contacts_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table serviceescalation_contacts add CONSTRAINT serv_esc_contacts_uq UNIQUE (instance_id,serviceescalation_id,contact_object_id)using index tablespace &&IXTBS;
alter table serviceescalations add constraint serviceescalations_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table serviceescalations add CONSTRAINT serviceescalations_uq UNIQUE (instance_id,config_type,service_object_id,timeperiod_object_id,first_notification,last_notification)using index tablespace &&IXTBS;
alter table servicegroup_members add constraint servicegroup_members_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table servicegroup_members add CONSTRAINT servicegroup_members_uq UNIQUE (servicegroup_id,service_object_id)using index tablespace &&IXTBS;
alter table servicegroups add constraint servicegroups_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table servicegroups add CONSTRAINT servicegroups_uq UNIQUE (instance_id,config_type,servicegroup_object_id)using index tablespace &&IXTBS;
alter table services add constraint services_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table services add CONSTRAINT services_uq UNIQUE (instance_id,config_type,service_object_id)using index tablespace &&IXTBS;
alter table servicestatus add constraint servicestatus_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table servicestatus add CONSTRAINT servicestatus_uq UNIQUE (service_object_id)using index tablespace &&IXTBS;
alter table statehistory add constraint statehistory_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table systemcommands add constraint systemcommands_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table systemcommands add CONSTRAINT systemcommands_uq UNIQUE (instance_id,start_time,start_time_usec)using index tablespace &&IXTBS;
alter table timedeventqueue add constraint timedeventqueue_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table timedevents add constraint timedevents_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table timedevents add CONSTRAINT timedevents_uq UNIQUE (instance_id,event_type,scheduled_time,object_id)using index tablespace &&IXTBS;
alter table timeperiod_timeranges add constraint timeperiod_timeranges_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table timeperiod_timeranges add CONSTRAINT timeperiod_timeranges_uq UNIQUE (timeperiod_id,day,start_sec,end_sec)using index tablespace &&IXTBS;
alter table timeperiods add constraint timeperiods_pk PRIMARY KEY  (id)using index tablespace &&IXTBS;
alter table timeperiods add CONSTRAINT timeperiods_uq UNIQUE (instance_id,config_type,timeperiod_object_id)using index tablespace &&IXTBS;

/* 
-- --------------------------------------------------------
-- set sequences nocache 
-- --------------------------------------------------------
*/
declare
cursor c is select sequence_name from user_sequences;
r c%rowtype;
s varchar2(2000);
begin
open c;
  loop
    fetch c into r;
    exit when c%notfound;
    s:='ALTER sequence '||r.sequence_name|| ' nocache';
    dbms_output.put_line(s);
    execute immediate s;
  end loop;
close c;
end;
/

spool off;
