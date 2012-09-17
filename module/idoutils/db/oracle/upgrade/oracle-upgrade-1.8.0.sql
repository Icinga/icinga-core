-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.8.0
--
-- run it as icinga database user from directory of this file
-- sqlplus icinga@<instance> @ oracle-upgrade-1.8.0.sql
-- -----------------------------------------
-- Copyright (c) 2012 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------
set sqlprompt "&&_USER@&&_CONNECT_IDENTIFIER SQL>"
set pagesize 200;
set linesize 200;
set heading on;
set echo on;
set feedback on;

define ICINGA_VERSION=1.8.0

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
-- #905
-- -----------------------------------------
alter table programstatus add disable_notif_expire_time TIMESTAMP(0) WITH LOCAL TIME ZONE default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR');

-- -----------------------------------------
-- #2618
-- -----------------------------------------

CREATE INDEX cntgrpmbrs_cgid_coid ON contactgroup_members (contactgroup_id,contact_object_id) tablespace &&IDXTBS;
CREATE INDEX hstgrpmbrs_hgid_hoid ON hostgroup_members (hostgroup_id,host_object_id) tablespace &&IDXTBS;
CREATE INDEX hstcntgrps_hid_cgoid ON host_contactgroups (host_id,contactgroup_object_id) tablespace &&IDXTBS;
CREATE INDEX hstprnthsts_hid_phoid ON host_parenthosts (host_id,parent_host_object_id) tablespace &&IDXTBS;
CREATE INDEX runtimevars_iid_varn ON runtimevariables (instance_id,varname) tablespace &&IDXTBS;
CREATE INDEX sgmbrs_sgid_soid ON servicegroup_members (servicegroup_id,service_object_id) tablespace &&IDXTBS;
CREATE INDEX scgrps_sid_cgoid ON service_contactgroups (service_id,contactgroup_object_id) tablespace &&IDXTBS;
CREATE INDEX tperiod_tid_d_ss_es ON timeperiod_timeranges (timeperiod_id,day,start_sec,end_sec) tablespace &&IDXTBS;

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

