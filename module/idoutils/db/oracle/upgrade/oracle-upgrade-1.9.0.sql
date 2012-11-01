-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.9.0
--
-- run it as icinga database user from whithin current directory
-- sqlplus icinga@<instance> @ oracle-upgrade-1.9.0.sql

-- -----------------------------------------
-- Copyright (c) 2012-2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------
set sqlprompt "&&_USER@&&_CONNECT_IDENTIFIER SQL>"
set pagesize 200;
set linesize 200;
set heading on;
set echo on;
set feedback on;

define ICINGA_VERSION=1.9.0

-- --------------------------------------------------------
-- warning: edit this script to define existing tablespaces
-- this particular step can be skipped safely if no new
-- table or index included
-- --------------------------------------------------------
/* set real TBS names on which you have quota, no checks are implemented!*/
define DATATBS='ICINGA_DATA1';
define LOBTBS='ICINGA_LOB1';
define IDXTBS='ICINGA_IDX1';

/* load defines from file, if any */
@icinga_defines.sql

/* script will be terminated on the first error */
whenever sqlerror exit failure
spool oracle-upgrade-&&ICINGA_VERSION..log

-- -----------------------------------------
-- #3649 sla index
-- -----------------------------------------
CREATE INDEX sla_idx_sthist ON statehistory (object_id, state_time DESC) tablespace &&IDXTBS;
CREATE INDEX sla_idx_dohist ON downtimehistory (object_id, actual_start_time, actual_end_time) tablespace &&IDXTBS;
CREATE INDEX sla_idx_obj ON objects (objecttype_id, is_active, name1) tablespace &&IDXTBS;

-- --------------------------------------------------------
-- alter output column to clob  to allow unlimited storage of output data#3412
-- --------------------------------------------------------
/* eventhandler */
alter table eventhandlers rename column output to o_old;
alter table eventhandlers add(output clob) lob (output) store as eventhandler_outp_lob(tablespace &LOBTBS);
update eventhandlers set output=o_old;
commit;
alter table eventhandlers drop column o_old;

/* hostchecks */
alter table hostchecks rename column output to o_old;
alter table hostchecks add(output clob) lob (output) store as hostchecks_outp_lob(tablespace &LOBTBS);
update hostchecks set output=o_old;
commit;
alter table hostchecks drop column o_old;

/* hoststatus */
alter table hoststatus rename column output to o_old;
alter table hoststatus add(output clob) lob (output) store as hoststatus_outp_lob(tablespace &LOBTBS);
update hoststatus set output=o_old;
commit;
alter table hoststatus drop column o_old;

/* notifications */
alter table notifications rename column output to o_old;
alter table notifications add(output clob) lob (output) store as notifications_outp_lob(tablespace &LOBTBS);
update notifications set output=o_old;
commit;
alter table notifications drop column o_old;

/* servicechecks */
alter table servicechecks rename column output to o_old;
alter table servicechecks add(output clob) lob (output) store as servicechecks_outp_lob(tablespace &LOBTBS);
update servicechecks set output=o_old;
commit;
alter table servicechecks drop column o_old;

/* servicestatus */
alter table servicestatus rename column output to o_old;
alter table servicestatus add(output clob) lob (output) store as servicestatus_outp_lob(tablespace &LOBTBS);
update servicestatus set output=o_old;
commit;
alter table servicestatus drop column o_old;

/* statehistory */
alter table statehistory rename column output to o_old;
alter table statehistory add(output clob) lob (output) store as statehistory_outp_lob(tablespace &LOBTBS);
update statehistory set output=o_old;
commit;
alter table statehistory drop column o_old;

/* systemcommands */
alter table systemcommands rename column output to o_old;
alter table systemcommands add(output clob) lob (output) store as systemcommands_outp_lob(tablespace &LOBTBS);
update systemcommands set output=o_old;
commit;
alter table systemcommands drop column o_old;

-- -----------------------------------------
-- finally update dbversion
-- -----------------------------------------

MERGE INTO dbversion
	USING DUAL ON (name='idoutils')
	WHEN MATCHED THEN
		UPDATE SET version='&&ICINGA_VERSION', modify_time=CURRENT_TIMESTAMP
	WHEN NOT MATCHED THEN
		INSERT (id, name, version, create_time, modify_time) VALUES ('1', 'idoutils', '&&ICINGA_VERSION', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);
commit;

/* last check */
select object_name,object_type,status  from user_objects where status !='VALID';

/* goodbye */
spool off 
exit;

