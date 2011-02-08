-- --------------------------------------------------------
-- oracle.sql
-- DB definition for Oracle
--
-- Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)
--
-- requires ocilib, oracle (instantclient) libs+sdk to work
-- specify oracle (instantclient) libs+sdk in ocilib configure
-- ./configure \
--	--with-oracle-headers-path=/opt/oracle/product/instantclient/instantclient_11_1/sdk/include \
--	--with-oracle-lib-path=/opt/oracle/product/instantclient/instantclient_11_1/
--
-- enable ocilib in Icinga with
-- ./configure --enable-idoutils --enable--oracle
--
-- copy to $ORACLE_HOME
-- # sqlplus username/password
-- SQL> @oracle.sql
--
-- Hints:
-- * set open_cursors to an appropriate value, not the default 50
--   http://www.orafaq.com/node/758
-- * if you are going into performance issues, consider setting commit_write to nowait
--
-- Example:
-- open_cursors=1000
-- commit_write='batch,nowait'
--
--
-- initial version: 2008-02-20 David Schmidt
-- current version: 2011-01-17 Michael Friedrich <michael.friedrich(at)univie.ac.at>
--
-- -- --------------------------------------------------------

-- set escape character
SET ESCAPE \

-- --------------------------------------------------------
-- unix timestamp 2 oradate function
-- --------------------------------------------------------

CREATE OR REPLACE FUNCTION unixts2date( n_seconds   IN    PLS_INTEGER)
        RETURN    DATE
IS
        unix_start  DATE    := TO_DATE('01.01.1970','DD.MM.YYYY');
        unix_max    PLS_INTEGER  := 2145916799;
        unix_min    PLS_INTEGER     := -2114380800;

BEGIN

        IF n_seconds > unix_max THEN
                RAISE_APPLICATION_ERROR( -20901, 'UNIX timestamp too large for 32 bit limit' );
        ELSIF n_seconds < unix_min THEN
                RAISE_APPLICATION_ERROR( -20901, 'UNIX timestamp too small for 32 bit limit' );
        ELSE
                RETURN unix_start + NUMTODSINTERVAL( n_seconds, 'SECOND' );
        END IF;

EXCEPTION
        WHEN NO_DATA_FOUND THEN
                RETURN TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS');
        WHEN OTHERS THEN
                RAISE;
END;
/

-- --------------------------------------------------------
-- cleaning procedures
-- --------------------------------------------------------

-- will be called during startup maintenance
CREATE OR REPLACE PROCEDURE clean_table_by_instance
     (p_table_name IN varchar2, p_id IN number )
     IS
     	v_stmt_str varchar2(200);
BEGIN
	v_stmt_str := 'DELETE FROM '
	|| p_table_name
	|| ' WHERE instance_id='
	|| p_id;
	EXECUTE IMMEDIATE v_stmt_str;
END;
/


-- will be called during periodic maintenance
CREATE OR REPLACE PROCEDURE clean_table_by_instance_time
     (p_table_name IN varchar2, p_id IN number, p_field_name IN varchar2, p_time IN number)
     IS
        v_stmt_str varchar2(200);
BEGIN
        v_stmt_str := 'DELETE FROM '
        || p_table_name
        || ' WHERE instance_id='
        || p_id
	|| ' AND '
	|| p_field_name
	|| '<(SELECT unixts2date('
	|| p_time
	|| ') FROM DUAL)';
        EXECUTE IMMEDIATE v_stmt_str;
END;
/



-- --------------------------------------------------------
-- database table creation: icinga
-- --------------------------------------------------------

--
-- Table structure for table acknowledgements
--

CREATE TABLE acknowledgements (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  entry_time_usec number(11) default 0 NOT NULL,
  acknowledgement_type number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  state number(6) default 0 NOT NULL,
  author_name varchar2(64),
  comment_data varchar2(1024),
  is_sticky number(6) default 0 NOT NULL,
  persistent_comment number(6) default 0 NOT NULL,
  notify_contacts number(6) default 0 NOT NULL,
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table commands
-- 

CREATE TABLE commands (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  command_line varchar2(1024),
  PRIMARY KEY  (id),
  CONSTRAINT commands UNIQUE (instance_id,object_id,config_type)
);

-- --------------------------------------------------------

-- 
-- Table structure for table commenthistory
-- 

CREATE TABLE commenthistory (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  entry_time_usec number(11) default 0 NOT NULL,
  comment_type number(6) default 0 NOT NULL,
  entry_type number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  comment_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  internal_comment_id number(11) default 0 NOT NULL,
  author_name varchar2(64),
  comment_data varchar2(1024),
  is_persistent number(6) default 0 NOT NULL,
  comment_source number(6) default 0 NOT NULL,
  expires number(6) default 0 NOT NULL,
  expiration_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  deletion_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  deletion_time_usec number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT commenthistory UNIQUE (instance_id,comment_time,internal_comment_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table comments
-- 

CREATE TABLE comments (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  entry_time_usec number(11) default 0 NOT NULL,
  comment_type number(6) default 0 NOT NULL,
  entry_type number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  comment_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  internal_comment_id number(11) default 0 NOT NULL,
  author_name varchar2(64),
  comment_data varchar2(1024),
  is_persistent number(6) default 0 NOT NULL,
  comment_source number(6) default 0 NOT NULL,
  expires number(6) default 0 NOT NULL,
  expiration_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT comments UNIQUE (instance_id,comment_time,internal_comment_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table configfiles
-- 

CREATE TABLE configfiles (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  configfile_type number(6) default 0 NOT NULL,
  configfile_path varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT configfiles UNIQUE (instance_id,configfile_type,configfile_path)
);

-- --------------------------------------------------------

-- 
-- Table structure for table configfilevariables
-- 

CREATE TABLE configfilevariables (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  configfile_id number(11) default 0 NOT NULL,
  varname varchar2(64),
  varvalue varchar2(1024),
  PRIMARY KEY  (id),
  CONSTRAINT configfilevariables UNIQUE (instance_id,configfile_id,varname)
);

-- --------------------------------------------------------

-- 
-- Table structure for table conninfo
-- 

CREATE TABLE conninfo (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  agent_name varchar2(32),
  agent_version varchar2(8),
  disposition varchar2(16),
  connect_source varchar2(16),
  connect_type varchar2(16),
  connect_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  disconnect_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_checkin_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  data_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  data_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  bytes_processed number(11) default 0 NOT NULL,
  lines_processed number(11) default 0 NOT NULL,
  entries_processed number(11) default 0 NOT NULL,
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table contact_addresses
-- 

CREATE TABLE contact_addresses (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  contact_id number(11) default 0 NOT NULL,
  address_number number(6) default 0 NOT NULL,
  address varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT contact_addresses UNIQUE (contact_id,address_number)
);

-- --------------------------------------------------------

-- 
-- Table structure for table contact_notificationcommands
-- 
CREATE TABLE contact_notificationcommands (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  contact_id number(11) default 0 NOT NULL,
  notification_type number(6) default 0 NOT NULL,
  command_object_id number(11) default 0 NOT NULL,
  command_args varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT contact_notificationcommands UNIQUE (contact_id,notification_type,command_object_id,command_args)
);

-- --------------------------------------------------------

-- 
-- Table structure for table contactgroup_members
-- 

CREATE TABLE contactgroup_members (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  contactgroup_id number(11) default 0 NOT NULL,
  contact_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT contactgroup_members UNIQUE (contactgroup_id,contact_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table contactgroups
-- 

CREATE TABLE contactgroups (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  contactgroup_object_id number(11) default 0 NOT NULL,
  alias varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT contactgroups UNIQUE (instance_id,config_type,contactgroup_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table contactnotificationmethods
-- 

CREATE TABLE contactnotificationmethods (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  contactnotification_id number(11) default 0 NOT NULL,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  start_time_usec number(11) default 0 NOT NULL,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  end_time_usec number(11) default 0 NOT NULL,
  command_object_id number(11) default 0 NOT NULL,
  command_args varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT contactnotificationmethods UNIQUE (instance_id,contactnotification_id,start_time,start_time_usec)
);

-- --------------------------------------------------------

-- 
-- Table structure for table contactnotifications
-- 

CREATE TABLE contactnotifications (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  notification_id number(11) default 0 NOT NULL,
  contact_object_id number(11) default 0 NOT NULL,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  start_time_usec number(11) default 0 NOT NULL,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  end_time_usec number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT contactnotifications UNIQUE (instance_id,contact_object_id,start_time,start_time_usec)
);

-- --------------------------------------------------------

-- 
-- Table structure for table contacts
-- 

CREATE TABLE contacts (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  contact_object_id number(11) default 0 NOT NULL,
  alias varchar2(64),
  email_address varchar2(255),
  pager_address varchar2(64),
  host_timeperiod_object_id number(11) default 0 NOT NULL,
  service_timeperiod_object_id number(11) default 0 NOT NULL,
  host_notifications_enabled number(6) default 0 NOT NULL,
  service_notifications_enabled number(6) default 0 NOT NULL,
  can_submit_commands number(6) default 0 NOT NULL,
  notify_service_recovery number(6) default 0 NOT NULL,
  notify_service_warning number(6) default 0 NOT NULL,
  notify_service_unknown number(6) default 0 NOT NULL,
  notify_service_critical number(6) default 0 NOT NULL,
  notify_service_flapping number(6) default 0 NOT NULL,
  notify_service_downtime number(6) default 0 NOT NULL,
  notify_host_recovery number(6) default 0 NOT NULL,
  notify_host_down number(6) default 0 NOT NULL,
  notify_host_unreachable number(6) default 0 NOT NULL,
  notify_host_flapping number(6) default 0 NOT NULL,
  notify_host_downtime number(6) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT contacts UNIQUE (instance_id,config_type,contact_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table contactstatus
-- 

CREATE TABLE contactstatus (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  contact_object_id number(11) default 0 NOT NULL,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  host_notifications_enabled number(6) default 0 NOT NULL,
  service_notifications_enabled number(6) default 0 NOT NULL,
  last_host_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_service_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  modified_attributes number(11) default 0 NOT NULL,
  modified_host_attributes number(11) default 0 NOT NULL,
  modified_service_attributes number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT contactstatus UNIQUE (contact_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table customvariables
-- 

CREATE TABLE customvariables (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  has_been_modified number(6) default 0 NOT NULL,
  varname varchar2(255),
  varvalue varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT customvariables UNIQUE (object_id,config_type,varname)
);
CREATE INDEX customvariables_i ON customvariables(varname);

-- --------------------------------------------------------

-- 
-- Table structure for table customvariablestatus
-- 

CREATE TABLE customvariablestatus (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  has_been_modified number(6) default 0 NOT NULL,
  varname varchar2(255),
  varvalue varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT customvariablestatus UNIQUE (object_id,varname)
);
CREATE INDEX customvariablestatus_i ON customvariablestatus(varname);

-- --------------------------------------------------------

-- 
-- Table structure for table dbversion
-- 

CREATE TABLE dbversion (
  id number(11) NOT NULL,
  name varchar2(10),
  version varchar2(10),
  PRIMARY KEY (id),
  CONSTRAINT dbversion UNIQUE (name)
);

-- --------------------------------------------------------

-- 
-- Table structure for table downtimehistory
-- 

CREATE TABLE downtimehistory (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  downtime_type number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  author_name varchar2(64),
  comment_data varchar2(1024),
  internal_downtime_id number(11) default 0 NOT NULL,
  triggered_by_id number(11) default 0 NOT NULL,
  is_fixed number(6) default 0 NOT NULL,
  duration number(6) default 0 NOT NULL,
  scheduled_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  scheduled_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  was_started number(6) default 0 NOT NULL,
  actual_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  actual_start_time_usec number(11) default 0 NOT NULL,
  actual_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  actual_end_time_usec number(11) default 0 NOT NULL,
  was_cancelled number(6) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT downtimehistory UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table eventhandlers
-- 

CREATE TABLE eventhandlers (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  eventhandler_type number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  state number(6) default 0 NOT NULL,
  state_type number(6) default 0 NOT NULL,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  start_time_usec number(11) default 0 NOT NULL,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  end_time_usec number(11) default 0 NOT NULL,
  command_object_id number(11) default 0 NOT NULL,
  command_args varchar2(255),
  command_line varchar2(1024),
  timeout number(6) default 0 NOT NULL,
  early_timeout number(6) default 0 NOT NULL,
  execution_time number default 0 NOT NULL,
  return_code number(6) default 0 NOT NULL,
  output varchar2(1024),
  PRIMARY KEY  (id),
  CONSTRAINT eventhandlers UNIQUE (instance_id,object_id,start_time,start_time_usec)
);

-- --------------------------------------------------------

-- 
-- Table structure for table externalcommands
-- 

CREATE TABLE externalcommands (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  command_type number(6) default 0 NOT NULL,
  command_name varchar2(128),
  command_args varchar2(255),
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table flappinghistory
-- 

CREATE TABLE flappinghistory (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  event_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  event_time_usec number(11) default 0 NOT NULL,
  event_type number(6) default 0 NOT NULL,
  reason_type number(6) default 0 NOT NULL,
  flapping_type number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  percent_state_change number default 0 NOT NULL,
  low_threshold number default 0 NOT NULL,
  high_threshold number default 0 NOT NULL,
  comment_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  internal_comment_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table host_contactgroups
-- 

CREATE TABLE host_contactgroups (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  host_id number(11) default 0 NOT NULL,
  contactgroup_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT host_contactgroups UNIQUE (host_id,contactgroup_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table host_contacts
-- 

CREATE TABLE host_contacts (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  host_id number(11) default 0 NOT NULL,
  contact_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table host_parenthosts
-- 

CREATE TABLE host_parenthosts (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  host_id number(11) default 0 NOT NULL,
  parent_host_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT host_parenthosts UNIQUE (host_id,parent_host_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table hostchecks
-- 

CREATE TABLE hostchecks (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  host_object_id number(11) default 0 NOT NULL,
  check_type number(6) default 0 NOT NULL,
  is_raw_check number(6) default 0 NOT NULL,
  current_check_attempt number(6) default 0 NOT NULL,
  max_check_attempts number(6) default 0 NOT NULL,
  state number(6) default 0 NOT NULL,
  state_type number(6) default 0 NOT NULL,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  start_time_usec number(11) default 0 NOT NULL,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  end_time_usec number(11) default 0 NOT NULL,
  command_object_id number(11) default 0 NOT NULL,
  command_args varchar2(255),
  command_line varchar2(1024),
  timeout number(6) default 0 NOT NULL,
  early_timeout number(6) default 0 NOT NULL,
  execution_time number default 0 NOT NULL,
  latency number default 0 NOT NULL,
  return_code number(6) default 0 NOT NULL,
  output varchar2(1024),
  long_output clob,
  perfdata varchar2(1024),
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table hostdependencies
-- 

CREATE TABLE hostdependencies (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  host_object_id number(11) default 0 NOT NULL,
  dependent_host_object_id number(11) default 0 NOT NULL,
  dependency_type number(6) default 0 NOT NULL,
  inherits_parent number(6) default 0 NOT NULL,
  timeperiod_object_id number(11) default 0 NOT NULL,
  fail_on_up number(6) default 0 NOT NULL,
  fail_on_down number(6) default 0 NOT NULL,
  fail_on_unreachable number(6) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT hostdependencies UNIQUE (instance_id,config_type,host_object_id,dependent_host_object_id,dependency_type,inherits_parent,fail_on_up,fail_on_down,fail_on_unreachable)
);

-- --------------------------------------------------------

-- 
-- Table structure for table hostescalation_contactgroups
-- 

CREATE TABLE hostescalation_contactgroups (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  hostescalation_id number(11) default 0 NOT NULL,
  contactgroup_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT hostescalation_contactgroups UNIQUE (hostescalation_id,contactgroup_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table hostescalation_contacts
-- 

CREATE TABLE hostescalation_contacts (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  hostescalation_id number(11) default 0 NOT NULL,
  contact_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT hostescalation_contacts UNIQUE (instance_id,hostescalation_id,contact_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table hostescalations
-- 

CREATE TABLE hostescalations (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  host_object_id number(11) default 0 NOT NULL,
  timeperiod_object_id number(11) default 0 NOT NULL,
  first_notification number(6) default 0 NOT NULL,
  last_notification number(6) default 0 NOT NULL,
  notification_interval number default 0 NOT NULL,
  escalate_on_recovery number(6) default 0 NOT NULL,
  escalate_on_down number(6) default 0 NOT NULL,
  escalate_on_unreachable number(6) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT hostescalations UNIQUE (instance_id,config_type,host_object_id,timeperiod_object_id,first_notification,last_notification)
);

-- --------------------------------------------------------

-- 
-- Table structure for table hostgroup_members
-- 

CREATE TABLE hostgroup_members (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  hostgroup_id number(11) default 0 NOT NULL,
  host_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT hostgroup_members UNIQUE (hostgroup_id,host_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table hostgroups
-- 

CREATE TABLE hostgroups (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  hostgroup_object_id number(11) default 0 NOT NULL,
  alias varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT hostgroups UNIQUE (instance_id,hostgroup_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table hosts
-- 

CREATE TABLE hosts (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  host_object_id number(11) default 0 NOT NULL,
  alias varchar2(255),
  display_name varchar2(255),
  address varchar2(128),
  address6 varchar2(128),
  check_command_object_id number(11) default 0 NOT NULL,
  check_command_args varchar2(255),
  eventhandler_command_object_id number(11) default 0 NOT NULL,
  eventhandler_command_args varchar2(255),
  notif_timeperiod_object_id number(11) default 0 NOT NULL, 
  check_timeperiod_object_id number(11) default 0 NOT NULL,
  failure_prediction_options varchar2(64),
  check_interval number default 0 NOT NULL,
  retry_interval number default 0 NOT NULL,
  max_check_attempts number(6) default 0 NOT NULL,
  first_notification_delay number default 0 NOT NULL,
  notification_interval number default 0 NOT NULL,
  notify_on_down number(6) default 0 NOT NULL,
  notify_on_unreachable number(6) default 0 NOT NULL,
  notify_on_recovery number(6) default 0 NOT NULL,
  notify_on_flapping number(6) default 0 NOT NULL,
  notify_on_downtime number(6) default 0 NOT NULL,
  stalk_on_up number(6) default 0 NOT NULL,
  stalk_on_down number(6) default 0 NOT NULL,
  stalk_on_unreachable number(6) default 0 NOT NULL,
  flap_detection_enabled number(6) default 0 NOT NULL,
  flap_detection_on_up number(6) default 0 NOT NULL,
  flap_detection_on_down number(6) default 0 NOT NULL,
  flap_detection_on_unreachable number(6) default 0 NOT NULL,
  low_flap_threshold number default 0 NOT NULL,
  high_flap_threshold number default 0 NOT NULL,
  process_performance_data number(6) default 0 NOT NULL,
  freshness_checks_enabled number(6) default 0 NOT NULL,
  freshness_threshold number(6) default 0 NOT NULL,
  passive_checks_enabled number(6) default 0 NOT NULL,
  event_handler_enabled number(6) default 0 NOT NULL,
  active_checks_enabled number(6) default 0 NOT NULL,
  retain_status_information number(6) default 0 NOT NULL,
  retain_nonstatus_information number(6) default 0 NOT NULL,
  notifications_enabled number(6) default 0 NOT NULL,
  obsess_over_host number(6) default 0 NOT NULL,
  failure_prediction_enabled number(6) default 0 NOT NULL,
  notes varchar2(255),
  notes_url varchar2(255),
  action_url varchar2(255),
  icon_image varchar2(255),
  icon_image_alt varchar2(255),
  vrml_image varchar2(255),
  statusmap_image varchar2(255),
  have_2d_coords number(6) default 0 NOT NULL,
  x_2d number(6) default 0 NOT NULL,
  y_2d number(6) default 0 NOT NULL,
  have_3d_coords number(6) default 0 NOT NULL,
  x_3d number default 0 NOT NULL,
  y_3d number default 0 NOT NULL,
  z_3d number default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT hosts UNIQUE (instance_id,config_type,host_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table hoststatus
-- 

CREATE TABLE hoststatus (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  host_object_id number(11) default 0 NOT NULL,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  output varchar2(1024),
  long_output clob,
  perfdata varchar2(1024),
  current_state number(6) default 0 NOT NULL,
  has_been_checked number(6) default 0 NOT NULL,
  should_be_scheduled number(6) default 0 NOT NULL,
  current_check_attempt number(6) default 0 NOT NULL,
  max_check_attempts number(6) default 0 NOT NULL,
  last_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  next_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  check_type number(6) default 0 NOT NULL,
  last_state_change date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_hard_state_change date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_hard_state number(6) default 0 NOT NULL,
  last_time_up date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_time_down date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_time_unreachable date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  state_type number(6) default 0 NOT NULL,
  last_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  next_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  no_more_notifications number(6) default 0 NOT NULL,
  notifications_enabled number(6) default 0 NOT NULL,
  problem_has_been_acknowledged number(6) default 0 NOT NULL,
  acknowledgement_type number(6) default 0 NOT NULL,
  current_notification_number number(6) default 0 NOT NULL,
  passive_checks_enabled number(6) default 0 NOT NULL,
  active_checks_enabled number(6) default 0 NOT NULL,
  event_handler_enabled number(6) default 0 NOT NULL,
  flap_detection_enabled number(6) default 0 NOT NULL,
  is_flapping number(6) default 0 NOT NULL,
  percent_state_change number default 0 NOT NULL,
  latency number default 0 NOT NULL,
  execution_time number default 0 NOT NULL,
  scheduled_downtime_depth number(6) default 0 NOT NULL,
  failure_prediction_enabled number(6) default 0 NOT NULL,
  process_performance_data number(6) default 0 NOT NULL,
  obsess_over_host number(6) default 0 NOT NULL,
  modified_host_attributes number(11) default 0 NOT NULL,
  event_handler varchar2(255),
  check_command varchar2(255),
  normal_check_interval number default 0 NOT NULL,
  retry_check_interval number default 0 NOT NULL,
  check_timeperiod_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT hoststatus UNIQUE (host_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table instances
-- 

CREATE TABLE instances (
  id number(11) NOT NULL,
  instance_name varchar2(64),
  instance_description varchar2(128),
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table logentries
-- 

CREATE TABLE logentries (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  logentry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  entry_time_usec number(11) default 0 NOT NULL,
  logentry_type number(11) default 0 NOT NULL,
  logentry_data varchar2(1024),
  realtime_data number(6) default 0 NOT NULL,
  inferred_data_extracted number(6) default 0 NOT NULL,
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table notifications
-- 

CREATE TABLE notifications (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  notification_type number(6) default 0 NOT NULL,
  notification_reason number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  start_time_usec number(11) default 0 NOT NULL,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  end_time_usec number(11) default 0 NOT NULL,
  state number(6) default 0 NOT NULL,
  output varchar2(1024),
  long_output clob,
  escalated number(6) default 0 NOT NULL,
  contacts_notified number(6) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT notifications UNIQUE (instance_id,object_id,start_time,start_time_usec)
);

-- --------------------------------------------------------

-- 
-- Table structure for table objects
-- 

CREATE TABLE objects (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  objecttype_id number(6) default 0 NOT NULL,
  name1 varchar2(128),
  name2 varchar2(128),
  is_active number(6) default 0 NOT NULL,
  PRIMARY KEY  (id)
);
CREATE INDEX objects_i ON objects(objecttype_id,name1,name2);

-- --------------------------------------------------------

-- 
-- Table structure for table processevents
-- 

CREATE TABLE processevents (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  event_type number(6) default 0 NOT NULL,
  event_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  event_time_usec number(11) default 0 NOT NULL,
  process_id number(11) default 0 NOT NULL,
  program_name varchar2(16),
  program_version varchar2(20),
  program_date varchar2(10),
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table programstatus
-- 

CREATE TABLE programstatus (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  program_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  program_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  is_currently_running number(6) default 0 NOT NULL,
  process_id number(11) default 0 NOT NULL,
  daemon_mode number(6) default 0 NOT NULL,
  last_command_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_log_rotation date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  notifications_enabled number(6) default 0 NOT NULL,
  active_service_checks_enabled number(6) default 0 NOT NULL,
  passive_service_checks_enabled number(6) default 0 NOT NULL,
  active_host_checks_enabled number(6) default 0 NOT NULL,
  passive_host_checks_enabled number(6) default 0 NOT NULL,
  event_handlers_enabled number(6) default 0 NOT NULL,
  flap_detection_enabled number(6) default 0 NOT NULL,
  failure_prediction_enabled number(6) default 0 NOT NULL,
  process_performance_data number(6) default 0 NOT NULL,
  obsess_over_hosts number(6) default 0 NOT NULL,
  obsess_over_services number(6) default 0 NOT NULL,
  modified_host_attributes number(11) default 0 NOT NULL,
  modified_service_attributes number(11) default 0 NOT NULL,
  global_host_event_handler varchar2(255),
  global_service_event_handler varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT programstatus UNIQUE (instance_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table runtimevariables
-- 

CREATE TABLE runtimevariables (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  varname varchar2(64),
  varvalue varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT runtimevariables UNIQUE (instance_id,varname)
);

-- --------------------------------------------------------

-- 
-- Table structure for table scheduleddowntime
-- 

CREATE TABLE scheduleddowntime (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  downtime_type number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  author_name varchar2(64),
  comment_data varchar2(1024),
  internal_downtime_id number(11) default 0 NOT NULL,
  triggered_by_id number(11) default 0 NOT NULL,
  is_fixed number(6) default 0 NOT NULL,
  duration number(6) default 0 NOT NULL,
  scheduled_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  scheduled_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  was_started number(6) default 0 NOT NULL,
  actual_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  actual_start_time_usec number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT scheduleddowntime UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table service_contactgroups
-- 

CREATE TABLE service_contactgroups (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  service_id number(11) default 0 NOT NULL,
  contactgroup_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT service_contactgroups UNIQUE (service_id,contactgroup_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table service_contacts
-- 

CREATE TABLE service_contacts (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  service_id number(11) default 0 NOT NULL,
  contact_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table servicechecks
-- 

CREATE TABLE servicechecks (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  service_object_id number(11) default 0 NOT NULL,
  check_type number(6) default 0 NOT NULL,
  current_check_attempt number(6) default 0 NOT NULL,
  max_check_attempts number(6) default 0 NOT NULL,
  state number(6) default 0 NOT NULL,
  state_type number(6) default 0 NOT NULL,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  start_time_usec number(11) default 0 NOT NULL,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  end_time_usec number(11) default 0 NOT NULL,
  command_object_id number(11) default 0 NOT NULL,
  command_args varchar2(255),
  command_line varchar2(1024),
  timeout number(6) default 0 NOT NULL,
  early_timeout number(6) default 0 NOT NULL,
  execution_time number default 0 NOT NULL,
  latency number default 0 NOT NULL,
  return_code number(6) default 0 NOT NULL,
  output varchar2(1024),
  long_output clob,
  perfdata varchar2(1024),
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table servicedependencies
-- 

CREATE TABLE servicedependencies (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  service_object_id number(11) default 0 NOT NULL,
  dependent_service_object_id number(11) default 0 NOT NULL,
  dependency_type number(6) default 0 NOT NULL,
  inherits_parent number(6) default 0 NOT NULL,
  timeperiod_object_id number(11) default 0 NOT NULL,
  fail_on_ok number(6) default 0 NOT NULL,
  fail_on_warning number(6) default 0 NOT NULL,
  fail_on_unknown number(6) default 0 NOT NULL,
  fail_on_critical number(6) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT servicedependencies UNIQUE (instance_id,config_type,service_object_id,dependent_service_object_id,dependency_type,inherits_parent,fail_on_ok,fail_on_warning,fail_on_unknown,fail_on_critical)
);

-- --------------------------------------------------------

-- 
-- Table structure for table serviceescalationcontactgroups
-- 

CREATE TABLE serviceescalationcontactgroups (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  serviceescalation_id number(11) default 0 NOT NULL,
  contactgroup_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT serviceescalationcontactgroups UNIQUE (serviceescalation_id,contactgroup_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table serviceescalation_contacts
-- 

CREATE TABLE serviceescalation_contacts (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  serviceescalation_id number(11) default 0 NOT NULL,
  contact_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT serviceescalation_contacts UNIQUE (instance_id,serviceescalation_id,contact_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table serviceescalations
-- 

CREATE TABLE serviceescalations (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  service_object_id number(11) default 0 NOT NULL,
  timeperiod_object_id number(11) default 0 NOT NULL,
  first_notification number(6) default 0 NOT NULL,
  last_notification number(6) default 0 NOT NULL,
  notification_interval number default 0 NOT NULL,
  escalate_on_recovery number(6) default 0 NOT NULL,
  escalate_on_warning number(6) default 0 NOT NULL,
  escalate_on_unknown number(6) default 0 NOT NULL,
  escalate_on_critical number(6) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT serviceescalations UNIQUE (instance_id,config_type,service_object_id,timeperiod_object_id,first_notification,last_notification)
);

-- --------------------------------------------------------

-- 
-- Table structure for table servicegroup_members
-- 

CREATE TABLE servicegroup_members (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  servicegroup_id number(11) default 0 NOT NULL,
  service_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT servicegroup_members UNIQUE (servicegroup_id,service_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table servicegroups
-- 

CREATE TABLE servicegroups (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  servicegroup_object_id number(11) default 0 NOT NULL,
  alias varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT servicegroups UNIQUE (instance_id,config_type,servicegroup_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table services
-- 

CREATE TABLE services (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  host_object_id number(11) default 0 NOT NULL,
  service_object_id number(11) default 0 NOT NULL,
  display_name varchar2(255),
  check_command_object_id number(11) default 0 NOT NULL,
  check_command_args varchar2(255),
  eventhandler_command_object_id number(11) default 0 NOT NULL,
  eventhandler_command_args varchar2(255),
  notif_timeperiod_object_id number(11) default 0 NOT NULL,
  check_timeperiod_object_id number(11) default 0 NOT NULL,
  failure_prediction_options varchar2(64),
  check_interval number default 0 NOT NULL,
  retry_interval number default 0 NOT NULL,
  max_check_attempts number(6) default 0 NOT NULL,
  first_notification_delay number default 0 NOT NULL,
  notification_interval number default 0 NOT NULL,
  notify_on_warning number(6) default 0 NOT NULL,
  notify_on_unknown number(6) default 0 NOT NULL,
  notify_on_critical number(6) default 0 NOT NULL,
  notify_on_recovery number(6) default 0 NOT NULL,
  notify_on_flapping number(6) default 0 NOT NULL,
  notify_on_downtime number(6) default 0 NOT NULL,
  stalk_on_ok number(6) default 0 NOT NULL,
  stalk_on_warning number(6) default 0 NOT NULL,
  stalk_on_unknown number(6) default 0 NOT NULL,
  stalk_on_critical number(6) default 0 NOT NULL,
  is_volatile number(6) default 0 NOT NULL,
  flap_detection_enabled number(6) default 0 NOT NULL,
  flap_detection_on_ok number(6) default 0 NOT NULL,
  flap_detection_on_warning number(6) default 0 NOT NULL,
  flap_detection_on_unknown number(6) default 0 NOT NULL,
  flap_detection_on_critical number(6) default 0 NOT NULL,
  low_flap_threshold number default 0 NOT NULL,
  high_flap_threshold number default 0 NOT NULL,
  process_performance_data number(6) default 0 NOT NULL,
  freshness_checks_enabled number(6) default 0 NOT NULL,
  freshness_threshold number(6) default 0 NOT NULL,
  passive_checks_enabled number(6) default 0 NOT NULL,
  event_handler_enabled number(6) default 0 NOT NULL,
  active_checks_enabled number(6) default 0 NOT NULL,
  retain_status_information number(6) default 0 NOT NULL,
  retain_nonstatus_information number(6) default 0 NOT NULL,
  notifications_enabled number(6) default 0 NOT NULL,
  obsess_over_service number(6) default 0 NOT NULL,
  failure_prediction_enabled number(6) default 0 NOT NULL,
  notes varchar2(255),
  notes_url varchar2(255),
  action_url varchar2(255),
  icon_image varchar2(255),
  icon_image_alt varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT services UNIQUE (instance_id,config_type,service_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table servicestatus
-- 

CREATE TABLE servicestatus (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  service_object_id number(11) default 0 NOT NULL,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  output varchar2(1024),
  long_output clob,
  perfdata varchar2(1024),
  current_state number(6) default 0 NOT NULL,
  has_been_checked number(6) default 0 NOT NULL,
  should_be_scheduled number(6) default 0 NOT NULL,
  current_check_attempt number(6) default 0 NOT NULL,
  max_check_attempts number(6) default 0 NOT NULL,
  last_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  next_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  check_type number(6) default 0 NOT NULL,
  last_state_change date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_hard_state_change date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_hard_state number(6) default 0 NOT NULL,
  last_time_ok date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_time_warning date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_time_unknown date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  last_time_critical date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  state_type number(6) default 0 NOT NULL,
  last_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  next_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  no_more_notifications number(6) default 0 NOT NULL,
  notifications_enabled number(6) default 0 NOT NULL,
  problem_has_been_acknowledged number(6) default 0 NOT NULL,
  acknowledgement_type number(6) default 0 NOT NULL,
  current_notification_number number(6) default 0 NOT NULL,
  passive_checks_enabled number(6) default 0 NOT NULL,
  active_checks_enabled number(6) default 0 NOT NULL,
  event_handler_enabled number(6) default 0 NOT NULL,
  flap_detection_enabled number(6) default 0 NOT NULL,
  is_flapping number(6) default 0 NOT NULL,
  percent_state_change number default 0 NOT NULL,
  latency number default 0 NOT NULL,
  execution_time number default 0 NOT NULL,
  scheduled_downtime_depth number(6) default 0 NOT NULL,
  failure_prediction_enabled number(6) default 0 NOT NULL,
  process_performance_data number(6) default 0 NOT NULL,
  obsess_over_service number(6) default 0 NOT NULL,
  modified_service_attributes number(11) default 0 NOT NULL,
  event_handler varchar2(255),
  check_command varchar2(255),
  normal_check_interval number default 0 NOT NULL,
  retry_check_interval number default 0 NOT NULL,
  check_timeperiod_object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT servicestatus UNIQUE (service_object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table statehistory
-- 

CREATE TABLE statehistory (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  state_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  state_time_usec number(11) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  state_change number(6) default 0 NOT NULL,
  state number(6) default 0 NOT NULL,
  state_type number(6) default 0 NOT NULL,
  current_check_attempt number(6) default 0 NOT NULL,
  max_check_attempts number(6) default 0 NOT NULL,
  last_state number(6) default -1 NOT NULL,
  last_hard_state number(6) default -1 NOT NULL,
  output varchar2(1024),
  long_output clob,
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table systemcommands
-- 

CREATE TABLE systemcommands (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  start_time_usec number(11) default 0 NOT NULL,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  end_time_usec number(11) default 0 NOT NULL,
  command_line varchar2(1024),
  timeout number(6) default 0 NOT NULL,
  early_timeout number(6) default 0 NOT NULL,
  execution_time number default 0 NOT NULL,
  return_code number(6) default 0 NOT NULL,
  output varchar2(1024),
  long_output clob,
  PRIMARY KEY  (id),
  CONSTRAINT systemcommands UNIQUE (instance_id,start_time,start_time_usec)
);

-- --------------------------------------------------------

-- 
-- Table structure for table timedeventqueue
-- 

CREATE TABLE timedeventqueue (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  event_type number(6) default 0 NOT NULL,
  queued_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  queued_time_usec number(11) default 0 NOT NULL,
  scheduled_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  recurring_event number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  PRIMARY KEY  (id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table timedevents
-- 

CREATE TABLE timedevents (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  event_type number(6) default 0 NOT NULL,
  queued_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  queued_time_usec number(11) default 0 NOT NULL,
  event_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  event_time_usec number(11) default 0 NOT NULL,
  scheduled_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  recurring_event number(6) default 0 NOT NULL,
  object_id number(11) default 0 NOT NULL,
  deletion_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') NOT NULL,
  deletion_time_usec number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT timedevents UNIQUE (instance_id,event_type,scheduled_time,object_id)
);

-- --------------------------------------------------------

-- 
-- Table structure for table timeperiod_timeranges
-- 

CREATE TABLE timeperiod_timeranges (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  timeperiod_id number(11) default 0 NOT NULL,
  day number(6) default 0 NOT NULL,
  start_sec number(11) default 0 NOT NULL,
  end_sec number(11) default 0 NOT NULL,
  PRIMARY KEY  (id),
  CONSTRAINT timeperiod_timeranges UNIQUE (timeperiod_id,day,start_sec,end_sec)
);

-- --------------------------------------------------------

-- 
-- Table structure for table timeperiods
-- 

CREATE TABLE timeperiods (
  id number(11) NOT NULL,
  instance_id number(11) default 0 NOT NULL,
  config_type number(6) default 0 NOT NULL,
  timeperiod_object_id number(11) default 0 NOT NULL,
  alias varchar2(255),
  PRIMARY KEY  (id),
  CONSTRAINT timeperiods UNIQUE (instance_id,config_type,timeperiod_object_id)
);

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------
INSERT INTO dbversion (id, name, version) VALUES ('1', 'idoutils', '1.3.0');


-- -----------------------------------------
-- add index (delete)
-- -----------------------------------------

-- for periodic delete 
-- instance_id and
-- TIMEDEVENTS => scheduled_time
-- SYSTEMCOMMANDS, SERVICECHECKS, HOSTCHECKS, EVENTHANDLERS  => start_time
-- EXTERNALCOMMANDS => entry_time

-- instance_id
CREATE INDEX timedevents_i_id_idx on timedevents(instance_id);
CREATE INDEX timedeventq_i_id_idx on timedeventqueue(instance_id);
CREATE INDEX systemcommands_i_id_idx on systemcommands(instance_id);
CREATE INDEX servicechecks_i_id_idx on servicechecks(instance_id);
CREATE INDEX hostchecks_i_id_idx on hostchecks(instance_id);
CREATE INDEX eventhandlers_i_id_idx on eventhandlers(instance_id);
CREATE INDEX externalcommands_i_id_idx on externalcommands(instance_id);

-- time
CREATE INDEX timedevents_time_id_idx on timedevents(scheduled_time);
CREATE INDEX timedeventq_time_id_idx on timedeventqueue(scheduled_time);
CREATE INDEX systemcommands_time_id_idx on systemcommands(start_time);
CREATE INDEX servicechecks_time_id_idx on servicechecks(start_time);
CREATE INDEX hostchecks_time_id_idx on hostchecks(start_time);
CREATE INDEX eventhandlers_time_id_idx on eventhandlers(start_time);
CREATE INDEX externalcommands_time_id_idx on externalcommands(entry_time);


-- for starting cleanup - referenced in dbhandler.c:882
-- instance_id only

-- realtime data
-- CREATE INDEX programstatus_i_id_idx on programstatus(instance_id); -- unique constraint
CREATE INDEX hoststatus_i_id_idx on hoststatus(instance_id);
CREATE INDEX servicestatus_i_id_idx on servicestatus(instance_id);
CREATE INDEX contactstatus_i_id_idx on contactstatus(instance_id);
-- CREATE INDEX timedeventqueue_i_id_idx on timedeventqueue(instance_id); -- defined adobe
CREATE INDEX comments_i_id_idx on comments(instance_id);
CREATE INDEX scheduleddowntime_i_id_idx on scheduleddowntime(instance_id);
CREATE INDEX runtimevariables_i_id_idx on runtimevariables(instance_id);
CREATE INDEX customvariablestatus_i_id_idx on customvariablestatus(instance_id);

-- config data
CREATE INDEX configfiles_i_id_idx on configfiles(instance_id);
CREATE INDEX configfilevariables_i_id_idx on configfilevariables(instance_id);
CREATE INDEX customvariables_i_id_idx on customvariables(instance_id);
CREATE INDEX commands_i_id_idx on commands(instance_id);
CREATE INDEX timeperiods_i_id_idx on timeperiods(instance_id);
CREATE INDEX timeperiod_timeranges_i_id_idx on timeperiod_timeranges(instance_id);
CREATE INDEX contactgroups_i_id_idx on contactgroups(instance_id);
CREATE INDEX contactgroup_members_i_id_idx on contactgroup_members(instance_id);
CREATE INDEX hostgroups_i_id_idx on hostgroups(instance_id);
CREATE INDEX hostgroup_members_i_id_idx on hostgroup_members(instance_id);
CREATE INDEX servicegroups_i_id_idx on servicegroups(instance_id);
CREATE INDEX servicegroup_members_i_id_idx on servicegroup_members(instance_id);
CREATE INDEX hostesc_i_id_idx on hostescalations(instance_id);
CREATE INDEX hostesc_contacts_i_id_idx on hostescalation_contacts(instance_id);
CREATE INDEX serviceesc_i_id_idx on serviceescalations(instance_id);
CREATE INDEX serviceesc_contacts_i_id_idx on serviceescalation_contacts(instance_id);
CREATE INDEX hostdependencies_i_id_idx on hostdependencies(instance_id);
CREATE INDEX contacts_i_id_idx on contacts(instance_id);
CREATE INDEX contact_addresses_i_id_idx on contact_addresses(instance_id);
CREATE INDEX contact_notifcommands_i_id_idx on contact_notificationcommands(instance_id);
CREATE INDEX hosts_i_id_idx on hosts(instance_id);
CREATE INDEX host_parenthosts_i_id_idx on host_parenthosts(instance_id);
CREATE INDEX host_contacts_i_id_idx on host_contacts(instance_id);
CREATE INDEX services_i_id_idx on services(instance_id);
CREATE INDEX service_contacts_i_id_idx on service_contacts(instance_id);
CREATE INDEX service_contactgroups_i_id_idx on service_contactgroups(instance_id);
CREATE INDEX host_contactgroups_i_id_idx on host_contactgroups(instance_id);
CREATE INDEX hostesc_cgroups_i_id_idx on hostescalation_contactgroups(instance_id);
CREATE INDEX serviceesc_cgroups_i_id_idx on serviceescalationcontactgroups(instance_id);


-- -----------------------------------------
-- more index stuff (WHERE clauses)
-- -----------------------------------------

-- hosts
CREATE INDEX hosts_host_object_id_idx on hosts(host_object_id);

-- hoststatus
CREATE INDEX hoststatus_stat_upd_time_idx on hoststatus(status_update_time);
CREATE INDEX hoststatus_current_state_idx on hoststatus(current_state);
CREATE INDEX hoststatus_check_type_idx on hoststatus(check_type);
CREATE INDEX hoststatus_state_type_idx on hoststatus(state_type);
CREATE INDEX hoststatus_last_state_chg_idx on hoststatus(last_state_change);
CREATE INDEX hoststatus_notif_enabled_idx on hoststatus(notifications_enabled);
CREATE INDEX hoststatus_problem_ack_idx on hoststatus(problem_has_been_acknowledged);
CREATE INDEX hoststatus_act_chks_en_idx on hoststatus(active_checks_enabled);
CREATE INDEX hoststatus_pas_chks_en_idx on hoststatus(passive_checks_enabled);
CREATE INDEX hoststatus_event_hdl_en_idx on hoststatus(event_handler_enabled);
CREATE INDEX hoststatus_flap_det_en_idx on hoststatus(flap_detection_enabled);
CREATE INDEX hoststatus_is_flapping_idx on hoststatus(is_flapping);
CREATE INDEX hoststatus_p_state_chg_idx on hoststatus(percent_state_change);
CREATE INDEX hoststatus_latency_idx on hoststatus(latency);
CREATE INDEX hoststatus_ex_time_idx on hoststatus(execution_time);
CREATE INDEX hoststatus_sch_downt_d_idx on hoststatus(scheduled_downtime_depth);

-- services
CREATE INDEX services_host_object_id_idx on services(host_object_id);

--servicestatus
CREATE INDEX srvcstatus_stat_upd_time_idx on servicestatus(status_update_time);
CREATE INDEX srvcstatus_current_state_idx on servicestatus(current_state);
CREATE INDEX srvcstatus_check_type_idx on servicestatus(check_type);
CREATE INDEX srvcstatus_state_type_idx on servicestatus(state_type);
CREATE INDEX srvcstatus_last_state_chg_idx on servicestatus(last_state_change);
CREATE INDEX srvcstatus_notif_enabled_idx on servicestatus(notifications_enabled);
CREATE INDEX srvcstatus_problem_ack_idx on servicestatus(problem_has_been_acknowledged);
CREATE INDEX srvcstatus_act_chks_en_idx on servicestatus(active_checks_enabled);
CREATE INDEX srvcstatus_pas_chks_en_idx on servicestatus(passive_checks_enabled);
CREATE INDEX srvcstatus_event_hdl_en_idx on servicestatus(event_handler_enabled);
CREATE INDEX srvcstatus_flap_det_en_idx on servicestatus(flap_detection_enabled);
CREATE INDEX srvcstatus_is_flapping_idx on servicestatus(is_flapping);
CREATE INDEX srvcstatus_p_state_chg_idx on servicestatus(percent_state_change);
CREATE INDEX srvcstatus_latency_idx on servicestatus(latency);
CREATE INDEX srvcstatus_ex_time_idx on servicestatus(execution_time);
CREATE INDEX srvcstatus_sch_downt_d_idx on servicestatus(scheduled_downtime_depth);

-- timedeventqueue
CREATE INDEX timed_e_q_event_type_idx on timedeventqueue(event_type);
-- CREATE INDEX timed_e_q_sched_time_idx on timedeventqueue(scheduled_time); -- defined above
CREATE INDEX timed_e_q_object_id_idx on timedeventqueue(object_id);
CREATE INDEX timed_e_q_rec_ev_id_idx on timedeventqueue(recurring_event);

-- timedevents
CREATE INDEX timed_e_event_type_idx on timedevents(event_type);
--CREATE INDEX timed_e_sched_time_idx on timedevents(scheduled_time); --already set for delete
CREATE INDEX timed_e_object_id_idx on timedevents(object_id);
CREATE INDEX timed_e_rec_ev_idx on timedevents(recurring_event);

-- hostchecks
CREATE INDEX hostchks_h_obj_id_idx on hostchecks(host_object_id);

-- servicechecks
CREATE INDEX servicechks_s_obj_id_idx on servicechecks(service_object_id);

-- objects
CREATE INDEX objects_objtype_id_idx ON objects(objecttype_id);
CREATE INDEX objects_name1_idx ON objects(name1);
CREATE INDEX objects_name2_idx ON objects(name2);
CREATE INDEX objects_inst_id_idx ON objects(instance_id);

-- instances
-- CREATE INDEX instances_name_idx on instances(instance_name);

-- logentries
-- CREATE INDEX loge_instance_id_idx on logentries(instance_id);
-- #236
CREATE INDEX loge_time_idx on logentries(logentry_time);
-- CREATE INDEX loge_data_idx on logentries(logentry_data);
CREATE INDEX loge_inst_id_time_idx on logentries (instance_id ASC, logentry_time DESC);

-- commenthistory
-- CREATE INDEX c_hist_instance_id_idx on logentries(instance_id);
-- CREATE INDEX c_hist_c_time_idx on logentries(comment_time);
-- CREATE INDEX c_hist_i_c_id_idx on logentries(internal_comment_id);

-- downtimehistory
-- CREATE INDEX d_t_hist_nstance_id_idx on downtimehistory(instance_id);
-- CREATE INDEX d_t_hist_type_idx on downtimehistory(downtime_type);
-- CREATE INDEX d_t_hist_object_id_idx on downtimehistory(object_id);
-- CREATE INDEX d_t_hist_entry_time_idx on downtimehistory(entry_time);
-- CREATE INDEX d_t_hist_sched_start_idx on downtimehistory(scheduled_start_time);
-- CREATE INDEX d_t_hist_sched_end_idx on downtimehistory(scheduled_end_time);

-- scheduleddowntime
-- CREATE INDEX sched_d_t_downtime_type_idx on scheduleddowntime(downtime_type);
-- CREATE INDEX sched_d_t_object_id_idx on scheduleddowntime(object_id);
-- CREATE INDEX sched_d_t_entry_time_idx on scheduleddowntime(entry_time);
-- CREATE INDEX sched_d_t_start_time_idx on scheduleddowntime(scheduled_start_time);
-- CREATE INDEX sched_d_t_end_time_idx on scheduleddowntime(scheduled_end_time);

-- statehistory
CREATE INDEX statehist_i_id_o_id_s_ty_s_ti on statehistory(instance_id, object_id, state_type, state_time);

-- -----------------------------------------
-- triggers/sequences
-- -----------------------------------------

CREATE SEQUENCE seq_acknowledgements
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_commands
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_commenthistory
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_comments
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_configfiles
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_configfilevariables
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_conninfo
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_contact_addresses
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_contact_notifcommands
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_contactgroup_members
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_contactgroups
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_contactnotifmethods
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_contactnotifications
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_contacts
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_contactstatus
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_customvariables
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_customvariablestatus
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_downtimehistory
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_eventhandlers
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_externalcommands
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_flappinghistory
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_host_contactgroups
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_host_contacts
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_host_parenthosts
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_hostchecks
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_hostdependencies
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_hostesc_contactgroups
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_hostesc_contacts
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_hostescalations
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_hostgroup_members
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_hostgroups
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_hosts
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_hoststatus
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_instances
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_logentries
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_notifications
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_objects
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_processevents
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_programstatus
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_runtimevariables
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_scheduleddowntime
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_service_contactgroups
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_service_contacts
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_servicechecks
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_servicedependencies
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_serviceesccontactgroups
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_serviceesc_contacts
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_serviceescalations
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_servicegroup_members
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_servicegroups
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_services
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_servicestatus
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_statehistory
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_systemcommands
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_timedeventqueue
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_timedevents
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_timep_timer
   start with 1
   increment by 1
   nomaxvalue;

CREATE SEQUENCE seq_timeperiods
   start with 1
   increment by 1
   nomaxvalue;

