/*
-- --------------------------------------------------------
-- create_icinga_objects_oracle.sql
-- icinga DB object definition for Oracle
-- called and defines set from oracle.sql
--
-- Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)
--
-- initial version: 2008-02-20 David Schmidt
--                  2011-01-17 Michael Friedrich <michael.friedrich(at)univie.ac.at>
-- current version: 2011-06-10 Thomas Dressler
-- -- --------------------------------------------------------
*/
-- -----------------------------------------
-- set sqlplus parameter
-- -----------------------------------------
define ICINGA_VERSION=1.5.0

set sqlprompt "&&_USER@&&_CONNECT_IDENTIFIER SQL>"
/* drop all objects  if called seperately*/
set pagesize 200;
set linesize 200;
set heading off;
set echo off;
set feedback off;
--skip on error
whenever sqlerror resume next
-- -----------------------------------------
-- drop ALL existing schema objects (should none be there, anyway)
-- -----------------------------------------
--create object list to drop
spool drop_objects.sql;
select 'drop '||object_type||' '||object_name||' cascade constraints;' from user_objects where object_type='TABLE';
select 'drop '||object_type||' '||object_name||';' from user_objects where object_type not in ('TABLE','INDEX','TRIGGER','PACKAGE BODY','LOB');
prompt;
prompt PURGE RECYCLEBIN;;
prompt select * from user_objects;;
prompt spool off;;
spool off;
set heading on;
set echo on;
set feedback on;
--run drop script
spool drop_objects.log;
@drop_objects.sql;

-- -----------------------------------------
-- Create new schema objects for icinga ido2db
-- Functions, Tables, Constraints, Indexes, Sequencen
-- -----------------------------------------

-- terminates immediately on any error
whenever sqlerror exit failure
-- logging
spool create_icinga_objects_oracle.log
prompt Installing Icinga Objects Version &&ICINGA_VERSION

-- --------------------------------------------------------
-- unix timestamp to oracle date function
-- --------------------------------------------------------

CREATE OR REPLACE FUNCTION unixts2date( n_seconds   IN    integer) RETURN    DATE
IS
        unix_start  DATE    := TO_DATE('01.01.1970','DD.MM.YYYY');
        unix_max    INTEGER  := 2145916799;
        unix_min    INTEGER     := -2114380800;

BEGIN
				 if n_seconds is null then
          return unix_start;
        end if;
        IF n_seconds > unix_max THEN
                RAISE_APPLICATION_ERROR( -20901, 'UNIX timestamp too large for 32 bit limit' );
        ELSIF n_seconds < unix_min THEN
                RAISE_APPLICATION_ERROR( -20902, 'UNIX timestamp too small for 32 bit limit' );
       END IF;
       RETURN unix_start + NUMTODSINTERVAL( n_seconds, 'SECOND' );
/* no exception handling, all errors goes to application */        
END;
/

-- --------------------------------------------------------
-- oracle date to unix timestamp function
-- --------------------------------------------------------

CREATE OR REPLACE FUNCTION date2unixts( d in date) RETURN    INTEGER
IS
        unix_start  DATE    := TO_DATE('01.01.1970','DD.MM.YYYY');
        n_seconds   integer;
        unix_max    INTEGER  := 2145916799;
        unix_min    INTEGER     := -2114380800;

BEGIN
				if d is null then
          return 0;
        end if;
        
				n_seconds:=(d-unix_start)*60*60*24;
        IF n_seconds > unix_max THEN
                RAISE_APPLICATION_ERROR( -20901, 'UNIX timestamp too large for 32 bit limit' );
        ELSIF n_seconds < unix_min THEN
                RAISE_APPLICATION_ERROR( -20902, 'UNIX timestamp too small for 32 bit limit' );
        END IF;
        return n_seconds;
/* no exception handling, all errors goes to application */
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
	exception when NO_DATA_FOUND then null;
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
	|| '<unixts2date('
	|| p_time
	|| ')';
        EXECUTE IMMEDIATE v_stmt_str;
	exception when NO_DATA_FOUND then null;
END;
/


-- --------------------------------------------------------
-- set trace event procedure
-- --------------------------------------------------------
CREATE or replace procedure set_trace_event(trace_level integer) 
/*
requires explicit alter session privilege
0 - pseudo level TRACE OFF
1 – standard SQL trace no, no wait events, or bind variables.
4 – Bind variables only
8 – Wait events only
12 – Bind Variables and Wait Events
*/
  IS
    mysid integer;
    text varchar2(200);
    output varchar(200);
    mypid integer;
    myfile varchar2(255);
    no_table EXCEPTION;
    no_rights EXCEPTION;
    invalid_name exception;
    invalid_level Exception;
  
  PRAGMA EXCEPTION_INIT(no_table, -942);
  PRAGMA EXCEPTION_INIT(invalid_name, -904);
  PRAGMA EXCEPTION_INIT(no_rights, -1031);
  BEGIN
    mysid:=0;
    mypid:=0;
    /* get own sid */
    select sys_context('USERENV','SID') into mysid from dual;
    /*check trace level*/
    if trace_level not in (0,1,4,8,12) then
      raise invalid_level;
    end if;
    if trace_level=0 then
      text:='ALTER SESSION SET EVENTS ''10046 TRACE NAME CONTEXT OFF''';
      output:='Session trace event set off for SID '||to_char(mysid);
    else
      text:='ALTER SESSION SET EVENTS ''10046 TRACE NAME CONTEXT FOREVER, LEVEL '||to_char( trace_level)||' ''';
      output:='Session trace event set to level '||to_char(trace_level)|| ' for SID '||to_char(mysid);
    end if;
    --dbms_output.put_line('Execute:'||text);
    execute immediate text;   
    dbms_output.put_line(output);
    /* optional */
    
    if trace_level>0 then
        text:='select p.spid  from v$process p,v$session s where s.paddr=p.addr and s.sid='||to_char(mysid);
        --dbms_output.put_line('Execute:'||text);
        EXECUTE IMMEDIATE text  into mypid;
        output:='Tracefile:<user_dump_dest>/<inst>_ora_'||to_char(mypid)||'.trc';
        text:='select p.tracefile from v$process p,v$session s where s.paddr=p.addr and s.sid='||to_char(mysid) ;
        --dbms_output.put_line('Execute:'||text);
        begin
          EXECUTE IMMEDIATE text into myfile;
          output:='Tracefile:'||myfile;
        exception
        when invalid_name then
          null;
          dbms_output.put_line('Tracefile field not available, guess name' );
        when others then
           dbms_output.put_line(sqlerrm);
        end;
        dbms_output.put_line(output);
    end if;
    exception
    /* surpress errors*/
    when no_rights then
      /* ora 1031 indicates no alter session priviledge */
      dbms_output.put_line('Error: No "Alter session" right');
    when invalid_level then
      dbms_output.put_line('Error:Only levels 0,1,4,8,12 are valid');
    when no_table then
        /* Ora 942 indicatin no access to v$view */
        dbms_output.put_line('Warning:No access to v$session and/or v$process');
    when others then
        dbms_output.put_line('Warning:Cannot get ProcessID:'||sqlerrm);      
END set_trace_event;
/

-- --------------------------------------------------------
-- database table creation: icinga
-- --------------------------------------------------------

--
-- Table structure for table acknowledgements
--

CREATE TABLE acknowledgements (
  id integer ,
  instance_id integer default 0 ,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  entry_time_usec integer default 0 ,
  acknowledgement_type integer default 0 ,
  object_id integer default 0 ,
  state integer default 0 ,
  author_name varchar2(64),
  comment_data varchar2(2048),
  is_sticky integer default 0 ,
  persistent_comment integer default 0 ,
  notify_contacts integer default 0 
)tablespace &&DATATBS ;

alter table acknowledgements add constraint acknowledgements_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
-- --------------------------------------------------------

-- 
-- Table structure for table commands
-- 

CREATE TABLE commands (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  object_id integer default 0 ,
  command_line varchar2(2048)
) tablespace &&DATATBS;

alter table commands add constraint commands_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table commands add constraint  commands_uq UNIQUE (instance_id,object_id,config_type)
	using index tablespace &&IDXTBS;
-- --------------------------------------------------------

-- 
-- Table structure for table commenthistory
-- 

CREATE TABLE commenthistory (
  id integer ,
  instance_id integer default 0 ,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  entry_time_usec integer default 0 ,
  comment_type integer default 0 ,
  entry_type integer default 0 ,
  object_id integer default 0 ,
  comment_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  internal_comment_id integer default 0 ,
  author_name varchar2(64),
  comment_data varchar2(2048),
  is_persistent integer default 0 ,
  comment_source integer default 0 ,
  expires integer default 0 ,
  expiration_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  deletion_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  deletion_time_usec integer default 0 
)tablespace &&DATATBS;

alter table commenthistory add constraint commenthist_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table commenthistory add constraint commenthist_uq  UNIQUE (instance_id,comment_time,internal_comment_id)
	using index tablespace &&IDXTBS;
-- --------------------------------------------------------

-- 
-- Table structure for table comments
-- 

CREATE TABLE comments (
  id integer ,
  instance_id integer default 0 ,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  entry_time_usec integer default 0 ,
  comment_type integer default 0 ,
  entry_type integer default 0 ,
  object_id integer default 0 ,
  comment_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  internal_comment_id integer default 0 ,
  author_name varchar2(64),
  comment_data varchar2(2048),
  is_persistent integer default 0 ,
  comment_source integer default 0 ,
  expires integer default 0 ,
  expiration_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') 
)tablespace &&DATATBS;

alter table comments add constraint comments_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table comments add constraint comments_uq  UNIQUE (instance_id,comment_time,internal_comment_id)
	using index tablespace &&IDXTBS;
	

-- --------------------------------------------------------

-- 
-- Table structure for table configfiles
-- 

CREATE TABLE configfiles (
  id integer ,
  instance_id integer default 0 ,
  configfile_type integer default 0 ,
  configfile_path varchar2(1024)
)tablespace &&DATATBS;

alter table configfiles add constraint configfiles_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table configfiles add constraint configfiles_uq  UNIQUE (instance_id,configfile_type,configfile_path)
	using index tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table configfilevariables
-- 

CREATE TABLE configfilevariables (
  id integer ,
  instance_id integer default 0 ,
  configfile_id integer default 0 ,
  varname varchar2(64),
  varvalue varchar2(2048)
)tablespace &&DATATBS;

alter table configfilevariables add constraint configfilevar_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table conninfo
-- 

CREATE TABLE conninfo (
  id integer ,
  instance_id integer default 0 ,
  agent_name varchar2(32),
  agent_version varchar2(8),
  disposition varchar2(16),
  connect_source varchar2(16),
  connect_type varchar2(16),
  connect_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  disconnect_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_checkin_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  data_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  data_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  bytes_processed integer default 0 ,
  lines_processed integer default 0 ,
  entries_processed integer default 0 
)tablespace &&DATATBS;

alter table conninfo add constraint conninfo_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table contact_addresses
-- 

CREATE TABLE contact_addresses (
  id integer ,
  instance_id integer default 0 ,
  contact_id integer default 0 ,
  address_number integer default 0 ,
  address varchar2(1024)
)tablespace &&DATATBS;

alter table contact_addresses add constraint contact_addresses_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table contact_addresses add CONSTRAINT contact_addresses_uq UNIQUE (contact_id,address_number)
	using index tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table contact_notificationcommands
-- 
CREATE TABLE contact_notificationcommands (
  id integer ,
  instance_id integer default 0 ,
  contact_id integer default 0 ,
  notification_type integer default 0 ,
  command_object_id integer default 0 ,
  command_args varchar2(1024)
)
tablespace &&DATATBS;

alter table contact_notificationcommands add constraint contact_notifi_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table contact_notificationcommands add CONSTRAINT contact_notifi_uq UNIQUE (contact_id,notification_type,command_object_id,command_args)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table contactgroup_members
-- 

CREATE TABLE contactgroup_members (
  id integer ,
  instance_id integer default 0 ,
  contactgroup_id integer default 0 ,
  contact_object_id integer default 0   
)tablespace &&DATATBS;

alter table contactgroup_members add constraint contactgroup_members_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table contactgroups
-- 

CREATE TABLE contactgroups (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  contactgroup_object_id integer default 0 ,
  alias varchar2(1024)  
)tablespace &&DATATBS;
alter table contactgroups add constraint contactgroups_pk PRIMARY KEY  (id)
using index tablespace &&IDXTBS;
alter table contactgroups add CONSTRAINT contactgroups_uq UNIQUE (instance_id,config_type,contactgroup_object_id)
	using index tablespace &&IDXTBS;
-- --------------------------------------------------------

-- 
-- Table structure for table contactnotificationmethods
-- 

CREATE TABLE contactnotificationmethods (
  id integer ,
  instance_id integer default 0 ,
  contactnotification_id integer default 0 ,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  start_time_usec integer default 0 ,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  end_time_usec integer default 0 ,
  command_object_id integer default 0 ,
  command_args varchar2(1024)  
)tablespace &&DATATBS;

alter table contactnotificationmethods add constraint contactnotifi_meth_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table contactnotificationmethods add CONSTRAINT contactnotifi_meth_uq UNIQUE (instance_id,contactnotification_id,start_time,start_time_usec)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table contactnotifications
-- 

CREATE TABLE contactnotifications (
  id integer ,
  instance_id integer default 0 ,
  notification_id integer default 0 ,
  contact_object_id integer default 0 ,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  start_time_usec integer default 0 ,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  end_time_usec integer default 0 
)tablespace &&DATATBS;

alter table contactnotifications add constraint contactnotifi_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table contactnotifications add CONSTRAINT contactnotifi_uq UNIQUE (instance_id,contact_object_id,start_time,start_time_usec)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table contacts
-- 

CREATE TABLE contacts (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  contact_object_id integer default 0 ,
  alias varchar2(64),
  email_address varchar2(1024),
  pager_address varchar2(64),
  host_timeperiod_object_id integer default 0 ,
  service_timeperiod_object_id integer default 0 ,
  host_notifications_enabled integer default 0 ,
  service_notifications_enabled integer default 0 ,
  can_submit_commands integer default 0 ,
  notify_service_recovery integer default 0 ,
  notify_service_warning integer default 0 ,
  notify_service_unknown integer default 0 ,
  notify_service_critical integer default 0 ,
  notify_service_flapping integer default 0 ,
  notify_service_downtime integer default 0 ,
  notify_host_recovery integer default 0 ,
  notify_host_down integer default 0 ,
  notify_host_unreachable integer default 0 ,
  notify_host_flapping integer default 0 ,
  notify_host_downtime integer default 0 
)tablespace &&DATATBS;

alter table contacts add constraint contacts_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table contacts add CONSTRAINT contacts_uq UNIQUE (instance_id,config_type,contact_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table contactstatus
-- 

CREATE TABLE contactstatus (
  id integer ,
  instance_id integer default 0 ,
  contact_object_id integer default 0 ,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  host_notifications_enabled integer default 0 ,
  service_notifications_enabled integer default 0 ,
  last_host_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_service_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  modified_attributes integer default 0 ,
  modified_host_attributes integer default 0 ,
  modified_service_attributes integer default 0 
)tablespace &&DATATBS;

alter table contactstatus add constraint contactstatus_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table contactstatus add CONSTRAINT contactstatus_uq UNIQUE (contact_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table customvariables
-- 

CREATE TABLE customvariables (
  id integer ,
  instance_id integer default 0 ,
  object_id integer default 0 ,
  config_type integer default 0 ,
  has_been_modified integer default 0 ,
  varname varchar2(1024),
  varvalue varchar2(1024)
)tablespace &&DATATBS;

alter table customvariables add constraint customvariables_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table customvariables add CONSTRAINT customvariables_uq UNIQUE (object_id,config_type,varname)
	using index tablespace &&IDXTBS;

CREATE INDEX customvariables_idx ON customvariables(varname) 
	tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table customvariablestatus
-- 

CREATE TABLE customvariablestatus (
  id integer ,
  instance_id integer default 0 ,
  object_id integer default 0 ,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  has_been_modified integer default 0 ,
  varname varchar2(1024),
  varvalue varchar2(1024)
)tablespace &&DATATBS;

alter table customvariablestatus add constraint customvariablest_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table customvariablestatus add CONSTRAINT customvariablest_uq UNIQUE (object_id,varname)
	using index tablespace &&IDXTBS;

CREATE INDEX customvariablest_idx ON customvariablestatus(varname)
	tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table dbversion
-- 

CREATE TABLE dbversion (
  id integer ,
  name varchar2(10),
  version varchar2(10)
)tablespace &&DATATBS;
alter table dbversion add constraint dbversion_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table dbversion add CONSTRAINT dbversion_uq UNIQUE (name)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table downtimehistory
-- 

CREATE TABLE downtimehistory (
  id integer ,
  instance_id integer default 0 ,
  downtime_type integer default 0 ,
  object_id integer default 0 ,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  author_name varchar2(64),
  comment_data varchar2(2048),
  internal_downtime_id integer default 0 ,
  triggered_by_id integer default 0 ,
  is_fixed integer default 0 ,
  duration integer default 0 ,
  scheduled_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  scheduled_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  was_started integer default 0 ,
  actual_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  actual_start_time_usec integer default 0 ,
  actual_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  actual_end_time_usec integer default 0 ,
  was_cancelled integer default 0 
)tablespace &&DATATBS;

alter table downtimehistory add constraint downtimehistory_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table downtimehistory add CONSTRAINT downtimehistory_uq UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table eventhandlers
-- 

CREATE TABLE eventhandlers (
  id integer ,
  instance_id integer default 0 ,
  eventhandler_type integer default 0 ,
  object_id integer default 0 ,
  state integer default 0 ,
  state_type integer default 0 ,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  start_time_usec integer default 0 ,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  end_time_usec integer default 0 ,
  command_object_id integer default 0 ,
  command_args varchar2(1024),
  command_line varchar2(2048),
  timeout integer default 0 ,
  early_timeout integer default 0 ,
  execution_time number default 0 ,
  return_code integer default 0 ,
  output varchar2(2048),
  long_output clob
)
lob (long_output) store as eventhandlers_out_lob(tablespace &&LOBTBS)
tablespace &&DATATBS;

alter table eventhandlers add constraint eventhandlers_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table eventhandlers add CONSTRAINT eventhandlers_uq UNIQUE (instance_id,object_id,start_time,start_time_usec)
	using index tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table externalcommands
-- 

CREATE TABLE externalcommands (
  id integer ,
  instance_id integer default 0 ,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  command_type integer default 0 ,
  command_name varchar2(128),
  command_args varchar2(1024)
)tablespace &&DATATBS;

alter table externalcommands add constraint externalcommands_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table flappinghistory
-- 

CREATE TABLE flappinghistory (
  id integer ,
  instance_id integer default 0 ,
  event_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  event_time_usec integer default 0 ,
  event_type integer default 0 ,
  reason_type integer default 0 ,
  flapping_type integer default 0 ,
  object_id integer default 0 ,
  percent_state_change number default 0 ,
  low_threshold number default 0 ,
  high_threshold number default 0 ,
  comment_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  internal_comment_id integer default 0 
)
tablespace &&DATATBS;

alter table flappinghistory add constraint flappinghistory_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table host_contactgroups
-- 

CREATE TABLE host_contactgroups (
  id integer ,
  instance_id integer default 0 ,
  host_id integer default 0 ,
  contactgroup_object_id integer default 0 
)tablespace &&DATATBS;

alter table host_contactgroups add constraint host_contactgroups_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table host_contacts
-- 

CREATE TABLE host_contacts (
  id integer ,
  instance_id integer default 0 ,
  host_id integer default 0 ,
  contact_object_id integer default 0 
)tablespace &&DATATBS;

alter table host_contacts add constraint host_contacts_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table host_parenthosts
-- 

CREATE TABLE host_parenthosts (
  id integer ,
  instance_id integer default 0 ,
  host_id integer default 0 ,
  parent_host_object_id integer default 0 
)tablespace &&DATATBS;

alter table host_parenthosts add constraint host_parenthosts_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table hostchecks
-- 

CREATE TABLE hostchecks (
  id integer ,
  instance_id integer default 0 ,
  host_object_id integer default 0 ,
  check_type integer default 0 ,
  is_raw_check integer default 0 ,
  current_check_attempt integer default 0 ,
  max_check_attempts integer default 0 ,
  state integer default 0 ,
  state_type integer default 0 ,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  start_time_usec integer default 0 ,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  end_time_usec integer default 0 ,
  command_object_id integer default 0 ,
  command_args varchar2(1024),
  command_line varchar2(2048),
  timeout integer default 0 ,
  early_timeout integer default 0 ,
  execution_time number default 0 ,
  latency number default 0 ,
  return_code integer default 0 ,
  output varchar2(2048),
  long_output clob,
  perfdata clob)
  lob (long_output) store as hostchecks_out_lob(tablespace &&LOBTBS)
  lob (perfdata) store as hostchecks_perf_lob(tablespace &&LOBTBS)
  tablespace &&DATATBS;

alter table hostchecks add constraint hostchecks PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table hostdependencies
-- 

CREATE TABLE hostdependencies (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  host_object_id integer default 0 ,
  dependent_host_object_id integer default 0 ,
  dependency_type integer default 0 ,
  inherits_parent integer default 0 ,
  timeperiod_object_id integer default 0 ,
  fail_on_up integer default 0 ,
  fail_on_down integer default 0 ,
  fail_on_unreachable integer default 0 )
  tablespace &&DATATBS;

alter table hostdependencies add constraint hostdependencies_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table hostdependencies add CONSTRAINT hostdependencies_uq UNIQUE (instance_id,config_type,host_object_id,dependent_host_object_id,dependency_type,inherits_parent,fail_on_up,fail_on_down,fail_on_unreachable)
	using index tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table hostescalation_contactgroups
-- 

CREATE TABLE hostescalation_contactgroups (
  id integer ,
  instance_id integer default 0 ,
  hostescalation_id integer default 0 ,
  contactgroup_object_id integer default 0 
)tablespace &&DATATBS;

alter table hostescalation_contactgroups add constraint h_e_contactgroups_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table hostescalation_contactgroups add CONSTRAINT h_e_contactgroups_uq UNIQUE (hostescalation_id,contactgroup_object_id)
	using index tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table hostescalation_contacts
-- 

CREATE TABLE hostescalation_contacts (
  id integer ,
  instance_id integer default 0 ,
  hostescalation_id integer default 0 ,
  contact_object_id integer default 0 
)tablespace &&DATATBS;

alter table hostescalation_contacts add constraint h_e_contacts_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table hostescalation_contacts add CONSTRAINT h_e_contacts_uq UNIQUE (instance_id,hostescalation_id,contact_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table hostescalations
-- 

CREATE TABLE hostescalations (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  host_object_id integer default 0 ,
  timeperiod_object_id integer default 0 ,
  first_notification integer default 0 ,
  last_notification integer default 0 ,
  notification_interval number default 0 ,
  escalate_on_recovery integer default 0 ,
  escalate_on_down integer default 0 ,
  escalate_on_unreachable integer default 0 
)tablespace &&DATATBS;

alter table hostescalations add constraint hostescalations_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table hostescalations add CONSTRAINT hostescalations_uq UNIQUE (instance_id,config_type,host_object_id,timeperiod_object_id,first_notification,last_notification)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table hostgroup_members
-- 

CREATE TABLE hostgroup_members (
  id integer ,
  instance_id integer default 0 ,
  hostgroup_id integer default 0 ,
  host_object_id integer default 0 
)tablespace &&DATATBS;

alter table hostgroup_members add constraint hostgroup_members_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table hostgroups
-- 

CREATE TABLE hostgroups (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  hostgroup_object_id integer default 0 ,
  alias varchar2(1024)
)tablespace &&DATATBS;

alter table hostgroups add constraint hostgroups_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table hostgroups add CONSTRAINT hostgroups_uq UNIQUE (instance_id,hostgroup_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table hosts
-- 

CREATE TABLE hosts (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  host_object_id integer default 0 ,
  alias varchar2(1024),
  display_name varchar2(1024),
  address varchar2(128),
  address6 varchar2(128),
  check_command_object_id integer default 0 ,
  check_command_args varchar2(1024),
  eventhandler_command_object_id integer default 0 ,
  eventhandler_command_args varchar2(1024),
  notif_timeperiod_object_id integer default 0 , 
  check_timeperiod_object_id integer default 0 ,
  failure_prediction_options varchar2(64),
  check_interval number default 0 ,
  retry_interval number default 0 ,
  max_check_attempts integer default 0 ,
  first_notification_delay number default 0 ,
  notification_interval number default 0 ,
  notify_on_down integer default 0 ,
  notify_on_unreachable integer default 0 ,
  notify_on_recovery integer default 0 ,
  notify_on_flapping integer default 0 ,
  notify_on_downtime integer default 0 ,
  stalk_on_up integer default 0 ,
  stalk_on_down integer default 0 ,
  stalk_on_unreachable integer default 0 ,
  flap_detection_enabled integer default 0 ,
  flap_detection_on_up integer default 0 ,
  flap_detection_on_down integer default 0 ,
  flap_detection_on_unreachable integer default 0 ,
  low_flap_threshold number default 0 ,
  high_flap_threshold number default 0 ,
  process_performance_data integer default 0 ,
  freshness_checks_enabled integer default 0 ,
  freshness_threshold integer default 0 ,
  passive_checks_enabled integer default 0 ,
  event_handler_enabled integer default 0 ,
  active_checks_enabled integer default 0 ,
  retain_status_information integer default 0 ,
  retain_nonstatus_information integer default 0 ,
  notifications_enabled integer default 0 ,
  obsess_over_host integer default 0 ,
  failure_prediction_enabled integer default 0 ,
  notes varchar2(1024),
  notes_url varchar2(1024),
  action_url varchar2(1024),
  icon_image varchar2(1024),
  icon_image_alt varchar2(1024),
  vrml_image varchar2(1024),
  statusmap_image varchar2(1024),
  have_2d_coords integer default 0 ,
  x_2d integer default 0 ,
  y_2d integer default 0 ,
  have_3d_coords integer default 0 ,
  x_3d number default 0 ,
  y_3d number default 0 ,
  z_3d number default 0 
)tablespace &&DATATBS;

alter table hosts add constraint hosts_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table hosts add CONSTRAINT hosts_uq UNIQUE (instance_id,config_type,host_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table hoststatus
-- 

CREATE TABLE hoststatus (
  id integer ,
  instance_id integer default 0 ,
  host_object_id integer default 0 ,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  output varchar2(2048),
  long_output clob,
  perfdata clob,
  current_state integer default 0 ,
  has_been_checked integer default 0 ,
  should_be_scheduled integer default 0 ,
  current_check_attempt integer default 0 ,
  max_check_attempts integer default 0 ,
  last_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  next_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  check_type integer default 0 ,
  last_state_change date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_hard_state_change date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_hard_state integer default 0 ,
  last_time_up date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_time_down date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_time_unreachable date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  state_type integer default 0 ,
  last_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  next_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  no_more_notifications integer default 0 ,
  notifications_enabled integer default 0 ,
  problem_has_been_acknowledged integer default 0 ,
  acknowledgement_type integer default 0 ,
  current_notification_number integer default 0 ,
  passive_checks_enabled integer default 0 ,
  active_checks_enabled integer default 0 ,
  event_handler_enabled integer default 0 ,
  flap_detection_enabled integer default 0 ,
  is_flapping integer default 0 ,
  percent_state_change number default 0 ,
  latency number default 0 ,
  execution_time number default 0 ,
  scheduled_downtime_depth integer default 0 ,
  failure_prediction_enabled integer default 0 ,
  process_performance_data integer default 0 ,
  obsess_over_host integer default 0 ,
  modified_host_attributes integer default 0 ,
  event_handler varchar2(1024),
  check_command varchar2(1024),
  normal_check_interval number default 0 ,
  retry_check_interval number default 0 ,
  check_timeperiod_object_id integer default 0 
)
lob (long_output) store as hoststatus_out_lob(tablespace &&LOBTBS)
lob (perfdata) store as hoststatus_perf_lob(tablespace &&LOBTBS)
tablespace &&DATATBS;

alter table hoststatus add constraint hoststatus_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table hoststatus add CONSTRAINT hoststatus_uq UNIQUE (host_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table instances
-- 

CREATE TABLE instances (
  id integer ,
  instance_name varchar2(64),
  instance_description varchar2(128)
)tablespace &&DATATBS;

alter table instances add constraint instances_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table logentries
-- 

CREATE TABLE logentries (
  id integer ,
  instance_id integer default 0 ,
  logentry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  entry_time_usec integer default 0 ,
  logentry_type integer default 0 ,
  logentry_data clob,
  realtime_data integer default 0 ,
  inferred_data_extracted integer default 0 
)
lob (logentry_data) store as logentries_data_lob(tablespace &&LOBTBS)
tablespace &&DATATBS;

alter table logentries add constraint logentries_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table notifications
-- 

CREATE TABLE notifications (
  id integer ,
  instance_id integer default 0 ,
  notification_type integer default 0 ,
  notification_reason integer default 0 ,
  object_id integer default 0 ,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  start_time_usec integer default 0 ,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  end_time_usec integer default 0 ,
  state integer default 0 ,
  output varchar2(2048),
  long_output clob,
  escalated integer default 0 ,
  contacts_notified integer default 0 
)
lob (long_output) store as notific_out_lob(tablespace &&LOBTBS)
tablespace &&DATATBS;

alter table notifications add constraint notifications_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table notifications add CONSTRAINT notifications_uq UNIQUE (instance_id,object_id,start_time,start_time_usec)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table objects
-- 

CREATE TABLE objects (
  id integer ,
  instance_id integer default 0 ,
  objecttype_id integer default 0 ,
  name1 varchar2(128),
  name2 varchar2(128),
  is_active integer default 0 
)tablespace &&DATATBS;

alter table objects add constraint objects_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;


CREATE INDEX objects_idx ON objects(objecttype_id,name1,name2)
	tablespace &&IDXTBS;

-- --------------------------------------------------------

-- 
-- Table structure for table processevents
-- 

CREATE TABLE processevents (
  id integer ,
  instance_id integer default 0 ,
  event_type integer default 0 ,
  event_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  event_time_usec integer default 0 ,
  process_id integer default 0 ,
  program_name varchar2(16),
  program_version varchar2(20),
  program_date varchar2(10)
)tablespace &&DATATBS;

alter table processevents add constraint processevents_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table programstatus
-- 

CREATE TABLE programstatus (
  id integer ,
  instance_id integer default 0 ,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  program_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  program_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  is_currently_running integer default 0 ,
  process_id integer default 0 ,
  daemon_mode integer default 0 ,
  last_command_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_log_rotation date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  notifications_enabled integer default 0 ,
  active_service_checks_enabled integer default 0 ,
  passive_service_checks_enabled integer default 0 ,
  active_host_checks_enabled integer default 0 ,
  passive_host_checks_enabled integer default 0 ,
  event_handlers_enabled integer default 0 ,
  flap_detection_enabled integer default 0 ,
  failure_prediction_enabled integer default 0 ,
  process_performance_data integer default 0 ,
  obsess_over_hosts integer default 0 ,
  obsess_over_services integer default 0 ,
  modified_host_attributes integer default 0 ,
  modified_service_attributes integer default 0 ,
  global_host_event_handler varchar2(1024),
  global_service_event_handler varchar2(1024)
)tablespace &&DATATBS;

alter table programstatus add constraint programstatus_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table programstatus add CONSTRAINT programstatus_uq UNIQUE (instance_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table runtimevariables
-- 

CREATE TABLE runtimevariables (
  id integer ,
  instance_id integer default 0 ,
  varname varchar2(64),
  varvalue varchar2(1024)
)tablespace &&DATATBS;

alter table runtimevariables add constraint runtimevariables_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table scheduleddowntime
-- 

CREATE TABLE scheduleddowntime (
  id integer ,
  instance_id integer default 0 ,
  downtime_type integer default 0 ,
  object_id integer default 0 ,
  entry_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  author_name varchar2(64),
  comment_data varchar2(2048),
  internal_downtime_id integer default 0 ,
  triggered_by_id integer default 0 ,
  is_fixed integer default 0 ,
  duration integer default 0 ,
  scheduled_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  scheduled_end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  was_started integer default 0 ,
  actual_start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  actual_start_time_usec integer default 0 
)tablespace &&DATATBS;

alter table scheduleddowntime add constraint scheduleddowntime_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table scheduleddowntime add CONSTRAINT scheduleddowntime_uq UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table service_contactgroups
-- 

CREATE TABLE service_contactgroups (
  id integer ,
  instance_id integer default 0 ,
  service_id integer default 0 ,
  contactgroup_object_id integer default 0 
)tablespace &&DATATBS;

alter table service_contactgroups add constraint service_contactgroups_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table service_contacts
-- 

CREATE TABLE service_contacts (
  id integer ,
  instance_id integer default 0 ,
  service_id integer default 0 ,
  contact_object_id integer default 0 
)tablespace &&DATATBS;

alter table service_contacts add constraint service_contacts_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table servicechecks
-- 

CREATE TABLE servicechecks (
  id integer ,
  instance_id integer default 0 ,
  service_object_id integer default 0 ,
  check_type integer default 0 ,
  current_check_attempt integer default 0 ,
  max_check_attempts integer default 0 ,
  state integer default 0 ,
  state_type integer default 0 ,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  start_time_usec integer default 0 ,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  end_time_usec integer default 0 ,
  command_object_id integer default 0 ,
  command_args varchar2(1024),
  command_line varchar2(2048),
  timeout integer default 0 ,
  early_timeout integer default 0 ,
  execution_time number default 0 ,
  latency number default 0 ,
  return_code integer default 0 ,
  output varchar2(2048),
  long_output clob,
  perfdata clob
)
lob (long_output) store as servicechecks_out_lob(tablespace &&LOBTBS)
lob (perfdata) store as servicechecks_perf_lob(tablespace &&LOBTBS)
tablespace &&DATATBS;

alter table servicechecks add constraint servicechecks_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table servicedependencies
-- 

CREATE TABLE servicedependencies (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  service_object_id integer default 0 ,
  dependent_service_object_id integer default 0 ,
  dependency_type integer default 0 ,
  inherits_parent integer default 0 ,
  timeperiod_object_id integer default 0 ,
  fail_on_ok integer default 0 ,
  fail_on_warning integer default 0 ,
  fail_on_unknown integer default 0 ,
  fail_on_critical integer default 0 
)tablespace &&DATATBS;

alter table servicedependencies add constraint servicedependencies_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table servicedependencies add CONSTRAINT servicedependencies_uq UNIQUE (instance_id,config_type,service_object_id,dependent_service_object_id,dependency_type,inherits_parent,fail_on_ok,fail_on_warning,fail_on_unknown,fail_on_critical)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table serviceescalationcontactgroups
-- 

CREATE TABLE serviceescalationcontactgroups (
  id integer ,
  instance_id integer default 0 ,
  serviceescalation_id integer default 0 ,
  contactgroup_object_id integer default 0 
)tablespace &&DATATBS;

alter table serviceescalationcontactgroups add constraint serv_esc_groups_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table serviceescalation_contacts
-- 

CREATE TABLE serviceescalation_contacts (
  id integer ,
  instance_id integer default 0 ,
  serviceescalation_id integer default 0 ,
  contact_object_id integer default 0 
)tablespace &&DATATBS;

alter table serviceescalation_contacts add constraint serv_esc_contacts_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table serviceescalation_contacts add CONSTRAINT serv_esc_contacts_uq UNIQUE (instance_id,serviceescalation_id,contact_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table serviceescalations
-- 

CREATE TABLE serviceescalations (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  service_object_id integer default 0 ,
  timeperiod_object_id integer default 0 ,
  first_notification integer default 0 ,
  last_notification integer default 0 ,
  notification_interval number default 0 ,
  escalate_on_recovery integer default 0 ,
  escalate_on_warning integer default 0 ,
  escalate_on_unknown integer default 0 ,
  escalate_on_critical integer default 0 
)tablespace &&DATATBS;
alter table serviceescalations add constraint serviceescalations_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table serviceescalations add CONSTRAINT serviceescalations_uq UNIQUE (instance_id,config_type,service_object_id,timeperiod_object_id,first_notification,last_notification)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table servicegroup_members
-- 

CREATE TABLE servicegroup_members (
  id integer ,
  instance_id integer default 0 ,
  servicegroup_id integer default 0 ,
  service_object_id integer default 0 
)tablespace &&DATATBS;
alter table servicegroup_members add constraint servicegroup_members_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table servicegroups
-- 

CREATE TABLE servicegroups (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  servicegroup_object_id integer default 0 ,
  alias varchar2(1024)
)tablespace &&DATATBS;
alter table servicegroups add constraint servicegroups_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table servicegroups add CONSTRAINT servicegroups_uq UNIQUE (instance_id,config_type,servicegroup_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table services
-- 

CREATE TABLE services (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  host_object_id integer default 0 ,
  service_object_id integer default 0 ,
  display_name varchar2(1024),
  check_command_object_id integer default 0 ,
  check_command_args varchar2(1024),
  eventhandler_command_object_id integer default 0 ,
  eventhandler_command_args varchar2(1024),
  notif_timeperiod_object_id integer default 0 ,
  check_timeperiod_object_id integer default 0 ,
  failure_prediction_options varchar2(64),
  check_interval number default 0 ,
  retry_interval number default 0 ,
  max_check_attempts integer default 0 ,
  first_notification_delay number default 0 ,
  notification_interval number default 0 ,
  notify_on_warning integer default 0 ,
  notify_on_unknown integer default 0 ,
  notify_on_critical integer default 0 ,
  notify_on_recovery integer default 0 ,
  notify_on_flapping integer default 0 ,
  notify_on_downtime integer default 0 ,
  stalk_on_ok integer default 0 ,
  stalk_on_warning integer default 0 ,
  stalk_on_unknown integer default 0 ,
  stalk_on_critical integer default 0 ,
  is_volatile integer default 0 ,
  flap_detection_enabled integer default 0 ,
  flap_detection_on_ok integer default 0 ,
  flap_detection_on_warning integer default 0 ,
  flap_detection_on_unknown integer default 0 ,
  flap_detection_on_critical integer default 0 ,
  low_flap_threshold number default 0 ,
  high_flap_threshold number default 0 ,
  process_performance_data integer default 0 ,
  freshness_checks_enabled integer default 0 ,
  freshness_threshold integer default 0 ,
  passive_checks_enabled integer default 0 ,
  event_handler_enabled integer default 0 ,
  active_checks_enabled integer default 0 ,
  retain_status_information integer default 0 ,
  retain_nonstatus_information integer default 0 ,
  notifications_enabled integer default 0 ,
  obsess_over_service integer default 0 ,
  failure_prediction_enabled integer default 0 ,
  notes varchar2(1024),
  notes_url varchar2(1024),
  action_url varchar2(1024),
  icon_image varchar2(1024),
  icon_image_alt varchar2(1024)
)tablespace &&DATATBS;
alter table services add constraint services_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table services add CONSTRAINT services_uq UNIQUE (instance_id,config_type,service_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table servicestatus
-- 

CREATE TABLE servicestatus (
  id integer ,
  instance_id integer default 0 ,
  service_object_id integer default 0 ,
  status_update_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  output varchar2(2048),
  long_output clob,
  perfdata clob,
  current_state integer default 0 ,
  has_been_checked integer default 0 ,
  should_be_scheduled integer default 0 ,
  current_check_attempt integer default 0 ,
  max_check_attempts integer default 0 ,
  last_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  next_check date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  check_type integer default 0 ,
  last_state_change date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_hard_state_change date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_hard_state integer default 0 ,
  last_time_ok date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_time_warning date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_time_unknown date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  last_time_critical date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  state_type integer default 0 ,
  last_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  next_notification date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  no_more_notifications integer default 0 ,
  notifications_enabled integer default 0 ,
  problem_has_been_acknowledged integer default 0 ,
  acknowledgement_type integer default 0 ,
  current_notification_number integer default 0 ,
  passive_checks_enabled integer default 0 ,
  active_checks_enabled integer default 0 ,
  event_handler_enabled integer default 0 ,
  flap_detection_enabled integer default 0 ,
  is_flapping integer default 0 ,
  percent_state_change number default 0 ,
  latency number default 0 ,
  execution_time number default 0 ,
  scheduled_downtime_depth integer default 0 ,
  failure_prediction_enabled integer default 0 ,
  process_performance_data integer default 0 ,
  obsess_over_service integer default 0 ,
  modified_service_attributes integer default 0 ,
  event_handler varchar2(1024),
  check_command varchar2(1024),
  normal_check_interval number default 0 ,
  retry_check_interval number default 0 ,
  check_timeperiod_object_id integer default 0 
)
lob (long_output) store as servicestatus_out_lob(tablespace &&LOBTBS)
lob (perfdata) store as servicestatus_perf_lob(tablespace &&LOBTBS)
tablespace &&DATATBS;

alter table servicestatus add constraint servicestatus_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table servicestatus add CONSTRAINT servicestatus_uq UNIQUE (service_object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table statehistory
-- 

CREATE TABLE statehistory (
  id integer ,
  instance_id integer default 0 ,
  state_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  state_time_usec integer default 0 ,
  object_id integer default 0 ,
  state_change integer default 0 ,
  state integer default 0 ,
  state_type integer default 0 ,
  current_check_attempt integer default 0 ,
  max_check_attempts integer default 0 ,
  last_state integer default -1 ,
  last_hard_state integer default -1 ,
  output varchar2(2048),
  long_output clob
)
lob (long_output) store as statehistory_out_lob(tablespace &&LOBTBS)
tablespace &&DATATBS;
alter table statehistory add constraint statehistory_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table systemcommands
-- 

CREATE TABLE systemcommands (
  id integer ,
  instance_id integer default 0 ,
  start_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  start_time_usec integer default 0 ,
  end_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  end_time_usec integer default 0 ,
  command_line varchar2(2048),
  timeout integer default 0 ,
  early_timeout integer default 0 ,
  execution_time number default 0 ,
  return_code integer default 0 ,
  output varchar2(2048),
  long_output clob
)
lob (long_output) store as systemcommands_out_lob(tablespace &&LOBTBS)
tablespace &&DATATBS;

alter table systemcommands add constraint systemcommands_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table systemcommands add CONSTRAINT systemcommands_uq UNIQUE (instance_id,start_time,start_time_usec)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table timedeventqueue
-- 

CREATE TABLE timedeventqueue (
  id integer ,
  instance_id integer default 0 ,
  event_type integer default 0 ,
  queued_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  queued_time_usec integer default 0 ,
  scheduled_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  recurring_event integer default 0 ,
  object_id integer default 0 
)tablespace &&DATATBS;
alter table timedeventqueue add constraint timedeventqueue_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;



-- --------------------------------------------------------

-- 
-- Table structure for table timedevents
-- 

CREATE TABLE timedevents (
  id integer ,
  instance_id integer default 0 ,
  event_type integer default 0 ,
  queued_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  queued_time_usec integer default 0 ,
  event_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  event_time_usec integer default 0 ,
  scheduled_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  recurring_event integer default 0 ,
  object_id integer default 0 ,
  deletion_time date default TO_DATE('1970-01-01 00:00:00','YYYY-MM-DD HH24:MI:SS') ,
  deletion_time_usec integer default 0 
)tablespace &&DATATBS;
alter table timedevents add constraint timedevents_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table timedevents add CONSTRAINT timedevents_uq UNIQUE (instance_id,event_type,scheduled_time,object_id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table timeperiod_timeranges
-- 

CREATE TABLE timeperiod_timeranges (
  id integer ,
  instance_id integer default 0 ,
  timeperiod_id integer default 0 ,
  day integer default 0 ,
  start_sec integer default 0 ,
  end_sec integer default 0 
)tablespace &&DATATBS;
alter table timeperiod_timeranges add constraint timeperiod_timeranges_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;


-- --------------------------------------------------------

-- 
-- Table structure for table timeperiods
-- 

CREATE TABLE timeperiods (
  id integer ,
  instance_id integer default 0 ,
  config_type integer default 0 ,
  timeperiod_object_id integer default 0 ,
  alias varchar2(1024)
)tablespace &&DATATBS;
alter table timeperiods add constraint timeperiods_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;
alter table timeperiods add CONSTRAINT timeperiods_uq UNIQUE (instance_id,config_type,timeperiod_object_id)
	using index tablespace &&IDXTBS;



-- -----------------------------------------
-- add index (delete)
-- -----------------------------------------

-- for periodic delete 
-- instance_id and
-- TIMEDEVENTS => scheduled_time
-- SYSTEMCOMMANDS, SERVICECHECKS, HOSTCHECKS, EVENTHANDLERS  => start_time
-- EXTERNALCOMMANDS => entry_time

-- instance_id
CREATE INDEX timedevents_i_id_idx on timedevents(instance_id) tablespace &&IDXTBS;
CREATE INDEX timedeventq_i_id_idx on timedeventqueue(instance_id) tablespace &&IDXTBS;
CREATE INDEX systemcommands_i_id_idx on systemcommands(instance_id) tablespace &&IDXTBS;
CREATE INDEX servicechecks_i_id_idx on servicechecks(instance_id)tablespace &&IDXTBS;
CREATE INDEX hostchecks_i_id_idx on hostchecks(instance_id) tablespace &&IDXTBS;
CREATE INDEX eventhandlers_i_id_idx on eventhandlers(instance_id) tablespace &&IDXTBS;
CREATE INDEX externalcommands_i_id_idx on externalcommands(instance_id) tablespace &&IDXTBS;

-- time
CREATE INDEX timedevents_time_id_idx on timedevents(scheduled_time) tablespace &&IDXTBS;
CREATE INDEX timedeventq_time_id_idx on timedeventqueue(scheduled_time) tablespace &&IDXTBS;
CREATE INDEX systemcommands_time_id_idx on systemcommands(start_time) tablespace &&IDXTBS;
CREATE INDEX servicechecks_time_id_idx on servicechecks(start_time) tablespace &&IDXTBS;
CREATE INDEX hostchecks_time_id_idx on hostchecks(start_time) tablespace &&IDXTBS;
CREATE INDEX eventhandlers_time_id_idx on eventhandlers(start_time) tablespace &&IDXTBS;
CREATE INDEX externalcommands_time_id_idx on externalcommands(entry_time) tablespace &&IDXTBS;



-- realtime data
-- CREATE INDEX programstatus_i_id_idx on programstatus(instance_id); -- unique constraint
CREATE INDEX hoststatus_i_id_idx on hoststatus(instance_id) tablespace &&IDXTBS;
CREATE INDEX servicestatus_i_id_idx on servicestatus(instance_id) tablespace &&IDXTBS;
CREATE INDEX contactstatus_i_id_idx on contactstatus(instance_id) tablespace &&IDXTBS;
-- CREATE INDEX timedeventqueue_i_id_idx on timedeventqueue(instance_id); -- defined adobe
CREATE INDEX comments_i_id_idx on comments(instance_id) tablespace &&IDXTBS;
CREATE INDEX scheduleddowntime_i_id_idx on scheduleddowntime(instance_id) tablespace &&IDXTBS;
CREATE INDEX runtimevariables_i_id_idx on runtimevariables(instance_id) tablespace &&IDXTBS;
CREATE INDEX customvariablestatus_i_id_idx on customvariablestatus(instance_id) tablespace &&IDXTBS;

-- config data
CREATE INDEX configfiles_i_id_idx on configfiles(instance_id) tablespace &&IDXTBS;
CREATE INDEX configfilevariables_i_id_idx on configfilevariables(instance_id)tablespace &&IDXTBS;
CREATE INDEX customvariables_i_id_idx on customvariables(instance_id) tablespace &&IDXTBS;
CREATE INDEX commands_i_id_idx on commands(instance_id) tablespace &&IDXTBS;
CREATE INDEX timeperiods_i_id_idx on timeperiods(instance_id) tablespace &&IDXTBS;
CREATE INDEX timeperiod_timeranges_i_id_idx on timeperiod_timeranges(instance_id) tablespace &&IDXTBS;
CREATE INDEX contactgroups_i_id_idx on contactgroups(instance_id) tablespace &&IDXTBS;
CREATE INDEX contactgroup_members_i_id_idx on contactgroup_members(instance_id) tablespace &&IDXTBS;
CREATE INDEX hostgroups_i_id_idx on hostgroups(instance_id) tablespace &&IDXTBS;
CREATE INDEX hostgroup_members_i_id_idx on hostgroup_members(instance_id) tablespace &&IDXTBS;
CREATE INDEX servicegroups_i_id_idx on servicegroups(instance_id) tablespace &&IDXTBS;
CREATE INDEX servicegroup_members_i_id_idx on servicegroup_members(instance_id) tablespace &&IDXTBS;
CREATE INDEX hostesc_i_id_idx on hostescalations(instance_id) tablespace &&IDXTBS;
CREATE INDEX hostesc_contacts_i_id_idx on hostescalation_contacts(instance_id) tablespace &&IDXTBS;
CREATE INDEX serviceesc_i_id_idx on serviceescalations(instance_id) tablespace &&IDXTBS;
CREATE INDEX serviceesc_contacts_i_id_idx on serviceescalation_contacts(instance_id) tablespace &&IDXTBS;
CREATE INDEX hostdependencies_i_id_idx on hostdependencies(instance_id) tablespace &&IDXTBS;
CREATE INDEX contacts_i_id_idx on contacts(instance_id) tablespace &&IDXTBS;
CREATE INDEX contact_addresses_i_id_idx on contact_addresses(instance_id) tablespace &&IDXTBS;
CREATE INDEX contact_notifcommands_i_id_idx on contact_notificationcommands(instance_id) tablespace &&IDXTBS;
CREATE INDEX hosts_i_id_idx on hosts(instance_id) tablespace &&IDXTBS;
CREATE INDEX host_parenthosts_i_id_idx on host_parenthosts(instance_id) tablespace &&IDXTBS;
CREATE INDEX host_contacts_i_id_idx on host_contacts(instance_id) tablespace &&IDXTBS;
CREATE INDEX services_i_id_idx on services(instance_id) tablespace &&IDXTBS;
CREATE INDEX service_contacts_i_id_idx on service_contacts(instance_id) tablespace &&IDXTBS;
CREATE INDEX service_contactgroups_i_id_idx on service_contactgroups(instance_id) tablespace &&IDXTBS;
CREATE INDEX host_contactgroups_i_id_idx on host_contactgroups(instance_id) tablespace &&IDXTBS;
CREATE INDEX hostesc_cgroups_i_id_idx on hostescalation_contactgroups(instance_id) tablespace &&IDXTBS;
CREATE INDEX serviceesc_cgroups_i_id_idx on serviceescalationcontactgroups(instance_id) tablespace &&IDXTBS;


-- -----------------------------------------
-- more index stuff (WHERE clauses)
-- -----------------------------------------

-- hosts
CREATE INDEX hosts_host_object_id_idx on hosts(host_object_id) tablespace &&IDXTBS;

-- hoststatus
CREATE INDEX hoststatus_stat_upd_time_idx on hoststatus(status_update_time) tablespace &&IDXTBS;
CREATE INDEX hoststatus_current_state_idx on hoststatus(current_state) tablespace &&IDXTBS;
CREATE INDEX hoststatus_check_type_idx on hoststatus(check_type) tablespace &&IDXTBS;
CREATE INDEX hoststatus_state_type_idx on hoststatus(state_type) tablespace &&IDXTBS;
CREATE INDEX hoststatus_last_state_chg_idx on hoststatus(last_state_change) tablespace &&IDXTBS;
CREATE INDEX hoststatus_notif_enabled_idx on hoststatus(notifications_enabled) tablespace &&IDXTBS;
CREATE INDEX hoststatus_problem_ack_idx on hoststatus(problem_has_been_acknowledged) tablespace &&IDXTBS;
CREATE INDEX hoststatus_act_chks_en_idx on hoststatus(active_checks_enabled) tablespace &&IDXTBS;
CREATE INDEX hoststatus_pas_chks_en_idx on hoststatus(passive_checks_enabled) tablespace &&IDXTBS;
CREATE INDEX hoststatus_event_hdl_en_idx on hoststatus(event_handler_enabled) tablespace &&IDXTBS;
CREATE INDEX hoststatus_flap_det_en_idx on hoststatus(flap_detection_enabled) tablespace &&IDXTBS;
CREATE INDEX hoststatus_is_flapping_idx on hoststatus(is_flapping) tablespace &&IDXTBS;
CREATE INDEX hoststatus_p_state_chg_idx on hoststatus(percent_state_change) tablespace &&IDXTBS;
CREATE INDEX hoststatus_latency_idx on hoststatus(latency) tablespace &&IDXTBS;
CREATE INDEX hoststatus_ex_time_idx on hoststatus(execution_time) tablespace &&IDXTBS;
CREATE INDEX hoststatus_sch_downt_d_idx on hoststatus(scheduled_downtime_depth) tablespace &&IDXTBS;

-- services
CREATE INDEX services_host_object_id_idx on services(host_object_id) tablespace &&IDXTBS;

--servicestatus
CREATE INDEX srvcstatus_stat_upd_time_idx on servicestatus(status_update_time) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_current_state_idx on servicestatus(current_state) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_check_type_idx on servicestatus(check_type) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_state_type_idx on servicestatus(state_type) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_last_state_chg_idx on servicestatus(last_state_change) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_notif_enabled_idx on servicestatus(notifications_enabled) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_problem_ack_idx on servicestatus(problem_has_been_acknowledged) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_act_chks_en_idx on servicestatus(active_checks_enabled) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_pas_chks_en_idx on servicestatus(passive_checks_enabled) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_event_hdl_en_idx on servicestatus(event_handler_enabled) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_flap_det_en_idx on servicestatus(flap_detection_enabled) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_is_flapping_idx on servicestatus(is_flapping) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_p_state_chg_idx on servicestatus(percent_state_change) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_latency_idx on servicestatus(latency) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_ex_time_idx on servicestatus(execution_time) tablespace &&IDXTBS;
CREATE INDEX srvcstatus_sch_downt_d_idx on servicestatus(scheduled_downtime_depth) tablespace &&IDXTBS;

-- timedeventqueue
CREATE INDEX timed_e_q_event_type_idx on timedeventqueue(event_type) tablespace &&IDXTBS;
-- CREATE INDEX timed_e_q_sched_time_idx on timedeventqueue(scheduled_time); -- defined above
CREATE INDEX timed_e_q_object_id_idx on timedeventqueue(object_id) tablespace &&IDXTBS;
CREATE INDEX timed_e_q_rec_ev_id_idx on timedeventqueue(recurring_event) tablespace &&IDXTBS;

-- timedevents
CREATE INDEX timed_e_event_type_idx on timedevents(event_type) tablespace &&IDXTBS;
--CREATE INDEX timed_e_sched_time_idx on timedevents(scheduled_time); --already set for delete
CREATE INDEX timed_e_object_id_idx on timedevents(object_id) tablespace &&IDXTBS;
CREATE INDEX timed_e_rec_ev_idx on timedevents(recurring_event) tablespace &&IDXTBS;

-- hostchecks
CREATE INDEX hostchks_h_obj_id_idx on hostchecks(host_object_id) tablespace &&IDXTBS;

-- servicechecks
CREATE INDEX servicechks_s_obj_id_idx on servicechecks(service_object_id) tablespace &&IDXTBS;

-- objects
CREATE INDEX objects_objtype_id_idx ON objects(objecttype_id) tablespace &&IDXTBS;
CREATE INDEX objects_name1_idx ON objects(name1) tablespace &&IDXTBS;
CREATE INDEX objects_name2_idx ON objects(name2) tablespace &&IDXTBS;
CREATE INDEX objects_inst_id_idx ON objects(instance_id) tablespace &&IDXTBS;

-- logentries
-- #236
CREATE INDEX loge_time_idx on logentries(logentry_time) tablespace &&IDXTBS;

-- statehistory
CREATE INDEX statehist_i_id_o_id_s_ty_s_ti on statehistory(instance_id, object_id, state_type, state_time) tablespace &&IDXTBS;

-- -----------------------------------------
-- sequences
-- -----------------------------------------

CREATE SEQUENCE seq_acknowledgements
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_commands
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_commenthistory
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_comments
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_configfiles
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_configfilevariables
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_conninfo
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_contact_addresses
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_contact_notifcommands
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_contactgroup_members
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_contactgroups
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_contactnotifmethods
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_contactnotifications
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_contacts
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_contactstatus
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_customvariables
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_customvariablestatus
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_downtimehistory
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_eventhandlers
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_externalcommands
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_flappinghistory
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_host_contactgroups
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_host_contacts
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_host_parenthosts
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_hostchecks
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_hostdependencies
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_hostesc_contactgroups
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_hostesc_contacts
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_hostescalations
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_hostgroup_members
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_hostgroups
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_hosts
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_hoststatus
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_instances
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_logentries
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_notifications
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_objects
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_processevents
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_programstatus
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_runtimevariables
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_scheduleddowntime
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_service_contactgroups
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_service_contacts
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_servicechecks
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_servicedependencies
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_serviceesccontactgroups
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_serviceesc_contacts
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_serviceescalations
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_servicegroup_members
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_servicegroups
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_services
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_servicestatus
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_statehistory
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_systemcommands
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_timedeventqueue
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_timedevents
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_timep_timer
   start with 1
   increment by 1
   nocache nomaxvalue;

CREATE SEQUENCE seq_timeperiods
   start with 1
   increment by 1
   nocache nomaxvalue;
 

/* final check */
select object_name,object_type,status  from user_objects where status !='VALID';

/* goodbye */
prompt Object creation finished
spool off;


