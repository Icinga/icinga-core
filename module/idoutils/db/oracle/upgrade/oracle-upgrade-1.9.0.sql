-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.9.0
--
-- run it as icinga database user from directory of this file
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
@../icinga_defines.sql

/* script will be terminated on the first error */
whenever sqlerror exit failure
spool oracle-upgrade-&&ICINGA_VERSION..log

-- -----------------------------------------
-- #3649 sla index
-- -----------------------------------------
CREATE INDEX sla_idx_sthist ON statehistory (object_id, state_time DESC) tablespace &&IDXTBS;
CREATE INDEX sla_idx_dohist ON downtimehistory (object_id, actual_start_time, actual_end_time) tablespace &&IDXTBS;
CREATE INDEX sla_idx_obj ON objects (objecttype_id, is_active, name1) tablespace &&IDXTBS;

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

