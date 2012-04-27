-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.7.0
--
-- run it as icinga database user from directory of this file
-- sqlplus icinga@<instance> @ oracle-upgrade.1-7.0.sql
-- -----------------------------------------
-- Copyright (c) 2010-2012 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------
set sqlprompt "&&_USER@&&_CONNECT_IDENTIFIER SQL>"
set pagesize 200;
set linesize 200;
set heading on;
set echo on;
set feedback on;

define ICINGA_VERSION=1.7.0

-- --------------------------------------------------------
-- warning:edit this script to define existing tablespaces
-- this particular step can be skipped safetly if no new table or index included
-- --------------------------------------------------------
/* set real TBS names on which you have quota, no checks are implemented!*/
define DATATBS='ICINGA_DATA1';
define LOBTBS='ICINGA_LOB1';
define IDXTBS='ICINGA_IDX1';

/* load defines from file, if any */
@../icinga_defines.sql

/* script will be terminated on the first error */
whenever sqlerror exit failure
spool oracle-upgrade-&&ICINGA_VERSION..log

-- -----------------------------------------
-- index for BP Addon #2274
-- -----------------------------------------
create index statehistory_state_idx on statehistory(object_id,state)
tablespace &IDXTBS;

-- -----------------------------------------
-- enlarge FAILURE_PREDICTION_OPTIONS #2479
-- -----------------------------------------
alter table hosts modify (FAILURE_PREDICTION_OPTIONS varchar2(128));

-- -----------------------------------------
-- add default individual caching sizes to the sequences #2510
-- should be adjusted for your environment 
-- -----------------------------------------
alter sequence SEQ_ACKNOWLEDGEMENTS cache 10;
alter sequence SEQ_COMMANDS cache 5; 
alter sequence SEQ_COMMENTHISTORY cache 20; 
alter sequence SEQ_COMMENTS cache 20; 
alter sequence SEQ_CONFIGFILES cache 5; 
alter sequence SEQ_CONFIGFILEVARIABLES cache 10; 
alter sequence SEQ_CONNINFO nocache; 
alter sequence SEQ_CONTACTGROUPS nocache; 
alter sequence SEQ_CONTACTGROUP_MEMBERS cache 5; 
alter sequence SEQ_CONTACTNOTIFICATIONS cache 10; 
alter sequence SEQ_CONTACTNOTIFMETHODS cache 5; 
alter sequence SEQ_CONTACTS cache 20; 
alter sequence SEQ_CONTACTSTATUS cache 10; 
alter sequence SEQ_CONTACT_ADDRESSES cache 5; 
alter sequence SEQ_CONTACT_NOTIFCOMMANDS cache 5; 
alter sequence SEQ_CUSTOMVARIABLES cache 20; 
alter sequence SEQ_CUSTOMVARIABLESTATUS cache 20; 
alter sequence SEQ_DOWNTIMEHISTORY cache 10; 
alter sequence SEQ_EVENTHANDLERS cache 10; 
alter sequence SEQ_EXTERNALCOMMANDS cache 5; 
alter sequence SEQ_FLAPPINGHISTORY cache 20; 
alter sequence SEQ_HOSTCHECKS cache 100; 
alter sequence SEQ_HOSTDEPENDENCIES cache 5; 
alter sequence SEQ_HOSTESCALATIONS cache 20; 
alter sequence SEQ_HOSTESC_CONTACTGROUPS nocache; 
alter sequence SEQ_HOSTESC_CONTACTS cache 5; 
alter sequence SEQ_HOSTGROUPS nocache; 
alter sequence SEQ_HOSTGROUP_MEMBERS cache 5; 
alter sequence SEQ_HOSTS cache 20; 
alter sequence SEQ_HOSTSTATUS cache 20; 
alter sequence SEQ_HOST_CONTACTGROUPS nocache;
alter sequence SEQ_HOST_CONTACTS cache 5; 
alter sequence SEQ_HOST_PARENTHOSTS cache 5; 
alter sequence SEQ_INSTANCES nocache; 
alter sequence SEQ_LOGENTRIES cache 50;
alter sequence SEQ_NOTIFICATIONS cache 20; 
alter sequence SEQ_OBJECTS cache 20; 
alter sequence SEQ_PROCESSEVENTS cache 20;
alter sequence SEQ_PROGRAMSTATUS cache 50;
alter sequence SEQ_RUNTIMEVARIABLES cache 10; 
alter sequence SEQ_SCHEDULEDDOWNTIME cache 5;
alter sequence SEQ_SERVICECHECKS cache 100;
alter sequence SEQ_SERVICEDEPENDENCIES cache 10; 
alter sequence SEQ_SERVICEESCALATIONS cache 5; 
alter sequence SEQ_SERVICEESCCONTACTGROUPS nocache; 
alter sequence SEQ_SERVICEESC_CONTACTS cache 5; 
alter sequence SEQ_SERVICEGROUPS nocache; 
alter sequence SEQ_SERVICEGROUP_MEMBERS cache 5;
alter sequence SEQ_SERVICES cache 20; 
alter sequence SEQ_SERVICESTATUS cache 20;
alter sequence SEQ_SERVICE_CONTACTGROUPS nocache;
alter sequence SEQ_SERVICE_CONTACTS cache 5;
alter sequence SEQ_SLAHISTORY cache 20;
alter sequence SEQ_STATEHISTORY cache 50; 
alter sequence SEQ_SYSTEMCOMMANDS cache 5; 
alter sequence SEQ_TIMEDEVENTQUEUE cache 10;
alter sequence SEQ_TIMEDEVENTS cache 10; 
alter sequence SEQ_TIMEPERIODS nocache; 
alter sequence SEQ_TIMEP_TIMER cache 5;

-- -----------------------------------------
-- #2537
-- -----------------------------------------

alter table downtimehistory add is_in_effect integer default 0;
alter table downtimehistory add trigger_time TIMESTAMP(0) WITH LOCAL TIME ZONE default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR');
alter table scheduleddowntime add is_in_effect integer default 0;
alter table scheduleddowntime add trigger_time TIMESTAMP(0) WITH LOCAL TIME ZONE default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR');

-- -----------------------------------------
-- #2562
-- -----------------------------------------
alter table dbversion add create_time TIMESTAMP(0) WITH LOCAL TIME ZONE default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR');
alter table dbversion add modify_time TIMESTAMP(0) WITH LOCAL TIME ZONE default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR');

-- -----------------------------------------
-- finally update dbversion
-- -----------------------------------------

MERGE INTO dbversion
	USING DUAL ON (name='idoutils')
	WHEN MATCHED THEN
		UPDATE SET version='&&ICINGA_VERSION', modify_time=CURRENT_TIMESTAMP
	WHEN NOT MATCHED THEN
		INSERT (id, name, version, create_time, modify_time) VALUES ('1', 'idoutils', '&&ICINGA_VERSION', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);

/* last check */
select object_name,object_type,status  from user_objects where status !='VALID';

/* goodbye */
spool off
exit;

