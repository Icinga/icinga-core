-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.6.0
--
-- run it as icinga database user whithin  current directory
-- sqlplus icinga@<instance> @ oracle-upgrade.1-6.0.sql
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------
set sqlprompt "&&_USER@&&_CONNECT_IDENTIFIER SQL>"
set pagesize 200;
set linesize 200;
set heading on;
set echo on;
set feedback on;

define ICINGA_VERSION=1.5.0

-- --------------------------------------------------------
-- warning:edit this script to define existing tablespaces
-- this particular step can be skipped safetly if no new table or index included
-- --------------------------------------------------------
/* set real TBS names on which you have quota, no checks are implemented!*/
define DATATBS='ICINGA_DATA1';
define LOBTBS='ICINGA_LOB1';
define IXTBS='ICINGA_IDX1';

/* script will be terminated on the first error */
whenever sqlerror exit failure
spool oracle-upgrade-&&ICINGA_VERSION..log

-- -----------------------------------------
-- add end_time for acknowledgments
-- -----------------------------------------
alter table acknowledgements add end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS');

-- --------------------------------------------------------

-- -----------------------------------------
-- Table structure for table timeperiods
-- -----------------------------------------

CREATE TABLE slahistory (
  id_id integer ,
  instance_id integer default 0 ,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  acknowledgement_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  object_id integer default 0 ,
  state integer default 0 ,
  state_type integer default 0 ,
  scheduled_downtime integer default 0
)tablespace &&DATATBS;

alter table slahistory add constraint slahistory_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;

-- -----------------------------------------
-- slahistory
-- -----------------------------------------

CREATE INDEX slahist_idx on slahistory(instance_id,object_id,start_time,end_time) tablespace &&IDXTBS;

CREATE SEQUENCE seq_slahistory
   start with 1
   increment by 1
   nocache nomaxvalue;

-- -----------------------------------------
-- Icinga Web Notifications
-- -----------------------------------------

CREATE INDEX notification_idx ON notifications(notification_type, object_id, start_time) tablespace &&IDXTBS;
CREATE INDEX notification_object_id_idx ON notifications(object_id) tablespace &&IDXTBS;
CREATE INDEX contact_notification_idx ON contactnotifications(notification_id, contact_object_id) tablespace &&IDXTBS;
CREATE INDEX contact_object_id_idx ON contacts(contact_object_id) tablespace &&IDXTBS;
CREATE INDEX contact_notif_meth_notif_idx ON contactnotificationmethods(contactnotification_id, command_object_id) tablespace &&IDXTBS;
CREATE INDEX command_object_idx ON commands(object_id) tablespace &&IDXTBS;
CREATE INDEX services_combined_object_idx ON services(service_object_id, host_object_id) tablespace &&IDXTBS;


-- -----------------------------------------
-- finally update dbversion
-- -----------------------------------------

MERGE INTO dbversion
	USING DUAL ON (name='idoutils')
	WHEN MATCHED THEN
		UPDATE SET version='&&ICINGA_VERSION'
	WHEN NOT MATCHED THEN
		INSERT (id, name, version) VALUES ('1', 'idoutils', '&&ICINGA_VERSION');

/* last check */
select object_name,object_type,status  from user_objects where status !='VALID';

/* goodbye */
spool off
exit;

