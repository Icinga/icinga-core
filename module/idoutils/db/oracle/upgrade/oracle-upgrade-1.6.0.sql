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

define ICINGA_VERSION=1.6.0

-- --------------------------------------------------------
-- warning:edit this script to define existing tablespaces
-- this particular step can be skipped safetly if no new table or index included
-- --------------------------------------------------------
/* set real TBS names on which you have quota, no checks are implemented!*/
define DATATBS='ICINGA_DATA1';
define LOBTBS='ICINGA_LOB1';
define IDXTBS='ICINGA_IDX1';

/* script will be terminated on the first error */
whenever sqlerror exit failure
spool oracle-upgrade-&&ICINGA_VERSION..log

-- -----------------------------------------
-- add end_time for acknowledgments
-- -----------------------------------------
alter table acknowledgements add end_time TIMESTAMP(0) WITH LOCAL TIME ZONE default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR'); 

-- -----------------------------------------
-- extend conninfo.agent_version #2104
-- -----------------------------------------
alter table conninfo modify (agent_version varchar2(16));

-- --------------------------------------------------------

-- -----------------------------------------
-- Table structure for table slahistory
-- -----------------------------------------

CREATE TABLE slahistory (
  id integer ,
  instance_id integer default 0 ,
  start_time TIMESTAMP(0) WITH LOCAL TIME ZONE default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') ,
  end_time TIMESTAMP(0) WITH LOCAL TIME ZONE default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') ,
  acknowledgement_time TIMESTAMP(0) WITH LOCAL TIME ZONE default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') ,
  object_id integer default 0 ,
  state integer default 0 ,
  state_type integer default 0 ,
  scheduled_downtime integer default 0
)tablespace &&DATATBS;

alter table slahistory add constraint slahistory_pk PRIMARY KEY  (id)
	using index tablespace &&IDXTBS;

-- -----------------------------------------
-- slahistory index
-- -----------------------------------------

CREATE INDEX slahist_idx on slahistory(instance_id,object_id,start_time,end_time) tablespace &&IDXTBS;
-- -----------------------------------------
-- slahistory sequence
-- -----------------------------------------

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



-- --------------------------------------------------------
-- move dates to timestamps, storing all in UTC
-- implements #1954
-- https://dev.icinga.org/issues/1954
-- note:functions unixts2date and date2unixts are not longer used, 
-- but not removed 
-- --------------------------------------------------------

-- --------------------------------------------------------
-- unix UTC timestamp to oracle TIMESTAMP function
-- --------------------------------------------------------

CREATE OR REPLACE FUNCTION unixts2localts( n_seconds   IN    integer) return timestamp
IS
        unix_start  timestamp    := TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR');
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
-- oracle LOCAL TIMESTAMP to unix timestamp function 
-- --------------------------------------------------------

CREATE OR REPLACE FUNCTION localts2unixts( ts in TIMESTAMP ) RETURN    INTEGER
IS
        unix_start TIMESTAMP    := TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR');
        n_seconds   integer;
        unix_max    INTEGER  := 2145916799;
        unix_min    INTEGER     := -2114380800;
        diff        interval day(9) to second(0);

BEGIN
	if ts is null then
          return 0;
  	end if;
  	diff:=ts-unix_start;
	n_seconds:=floor(extract(second from diff))
    		+extract(minute from diff)*60 
    		+extract(hour from diff)*3600 
    		+extract(day from diff)*86400;
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
        || '<unixts2localts('
        || p_time
        || ')';
        EXECUTE IMMEDIATE v_stmt_str;
        exception when NO_DATA_FOUND then null;
END;
/

-- --------------------------------------------------------
-- alter table columns from date to timestamp(0) with local timezone
-- it asumes you are running this script with your local timezone so 
-- does not change the values, but will respect this when using UTC
-- --------------------------------------------------------
ALTER TABLE LOGENTRIES modify ( LOGENTRY_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE LOGENTRIES modify ( ENTRY_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE    
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE NOTIFICATIONS modify ( START_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE NOTIFICATIONS modify ( END_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE   
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE PROCESSEVENTS modify ( EVENT_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE PROGRAMSTATUS modify ( STATUS_UPDATE_TIME TIMESTAMP(0) WITH LOCAL   
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE PROGRAMSTATUS modify ( PROGRAM_START_TIME TIMESTAMP(0) WITH LOCAL   
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE PROGRAMSTATUS modify ( PROGRAM_END_TIME TIMESTAMP(0) WITH LOCAL TIME
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE PROGRAMSTATUS modify ( LAST_COMMAND_CHECK TIMESTAMP(0) WITH LOCAL   
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE PROGRAMSTATUS modify ( LAST_LOG_ROTATION TIMESTAMP(0) WITH LOCAL    
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE SCHEDULEDDOWNTIME modify ( ENTRY_TIME TIMESTAMP(0) WITH LOCAL TIME  
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE SCHEDULEDDOWNTIME modify ( SCHEDULED_START_TIME TIMESTAMP(0) WITH   
LOCAL TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );  
ALTER TABLE SCHEDULEDDOWNTIME modify ( SCHEDULED_END_TIME TIMESTAMP(0) WITH     
LOCAL TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );  
ALTER TABLE SCHEDULEDDOWNTIME modify ( ACTUAL_START_TIME TIMESTAMP(0) WITH LOCAL
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE SERVICECHECKS modify ( START_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE SERVICECHECKS modify ( END_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE   
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE SERVICESTATUS modify ( STATUS_UPDATE_TIME TIMESTAMP(0) WITH LOCAL   
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE SERVICESTATUS modify ( LAST_CHECK TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE SERVICESTATUS modify ( NEXT_CHECK TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE SERVICESTATUS modify ( LAST_STATE_CHANGE TIMESTAMP(0) WITH LOCAL    
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE SERVICESTATUS modify ( LAST_HARD_STATE_CHANGE TIMESTAMP(0) WITH     
LOCAL TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );  
ALTER TABLE SERVICESTATUS modify ( LAST_TIME_OK TIMESTAMP(0) WITH LOCAL TIME    
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE SERVICESTATUS modify ( LAST_TIME_WARNING TIMESTAMP(0) WITH LOCAL    
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE SERVICESTATUS modify ( LAST_TIME_UNKNOWN TIMESTAMP(0) WITH LOCAL    
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE SERVICESTATUS modify ( LAST_TIME_CRITICAL TIMESTAMP(0) WITH LOCAL   
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE SERVICESTATUS modify ( LAST_NOTIFICATION TIMESTAMP(0) WITH LOCAL    
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE SERVICESTATUS modify ( NEXT_NOTIFICATION TIMESTAMP(0) WITH LOCAL    
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE STATEHISTORY modify ( STATE_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE  
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE SYSTEMCOMMANDS modify ( START_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE SYSTEMCOMMANDS modify ( END_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE  
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE TIMEDEVENTQUEUE modify ( QUEUED_TIME TIMESTAMP(0) WITH LOCAL TIME   
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE TIMEDEVENTQUEUE modify ( SCHEDULED_TIME TIMESTAMP(0) WITH LOCAL TIME
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE TIMEDEVENTS modify ( QUEUED_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE  
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE TIMEDEVENTS modify ( EVENT_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE   
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE TIMEDEVENTS modify ( SCHEDULED_TIME TIMESTAMP(0) WITH LOCAL TIME    
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE TIMEDEVENTS modify ( DELETION_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE ACKNOWLEDGEMENTS modify ( ENTRY_TIME TIMESTAMP(0) WITH LOCAL TIME   
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE COMMENTHISTORY modify ( ENTRY_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE COMMENTHISTORY modify ( COMMENT_TIME TIMESTAMP(0) WITH LOCAL TIME   
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE COMMENTHISTORY modify ( EXPIRATION_TIME TIMESTAMP(0) WITH LOCAL TIME
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE COMMENTHISTORY modify ( DELETION_TIME TIMESTAMP(0) WITH LOCAL TIME  
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE COMMENTS modify ( ENTRY_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE      
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE COMMENTS modify ( COMMENT_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE    
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE COMMENTS modify ( EXPIRATION_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE CONNINFO modify ( CONNECT_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE    
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE CONNINFO modify ( DISCONNECT_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE CONNINFO modify ( LAST_CHECKIN_TIME TIMESTAMP(0) WITH LOCAL TIME    
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE CONNINFO modify ( DATA_START_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE CONNINFO modify ( DATA_END_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE   
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE CONTACTNOTIFICATIONMETHODS modify ( START_TIME TIMESTAMP(0) WITH    
LOCAL TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );  
ALTER TABLE CONTACTNOTIFICATIONMETHODS modify ( END_TIME TIMESTAMP(0) WITH LOCAL
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE CONTACTNOTIFICATIONS modify ( START_TIME TIMESTAMP(0) WITH LOCAL    
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE CONTACTNOTIFICATIONS modify ( END_TIME TIMESTAMP(0) WITH LOCAL TIME 
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE CONTACTSTATUS modify ( STATUS_UPDATE_TIME TIMESTAMP(0) WITH LOCAL   
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE CONTACTSTATUS modify ( LAST_HOST_NOTIFICATION TIMESTAMP(0) WITH     
LOCAL TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );  
ALTER TABLE CONTACTSTATUS modify ( LAST_SERVICE_NOTIFICATION TIMESTAMP(0) WITH  
LOCAL TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );  
ALTER TABLE CUSTOMVARIABLESTATUS modify ( STATUS_UPDATE_TIME TIMESTAMP(0) WITH  
LOCAL TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );  
ALTER TABLE DOWNTIMEHISTORY modify ( ENTRY_TIME TIMESTAMP(0) WITH LOCAL TIME    
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE DOWNTIMEHISTORY modify ( SCHEDULED_START_TIME TIMESTAMP(0) WITH     
LOCAL TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );  
ALTER TABLE DOWNTIMEHISTORY modify ( SCHEDULED_END_TIME TIMESTAMP(0) WITH LOCAL 
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE DOWNTIMEHISTORY modify ( ACTUAL_START_TIME TIMESTAMP(0) WITH LOCAL  
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE DOWNTIMEHISTORY modify ( ACTUAL_END_TIME TIMESTAMP(0) WITH LOCAL    
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE EVENTHANDLERS modify ( START_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE 
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE EVENTHANDLERS modify ( END_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE   
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE EXTERNALCOMMANDS modify ( ENTRY_TIME TIMESTAMP(0) WITH LOCAL TIME   
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE FLAPPINGHISTORY modify ( EVENT_TIME TIMESTAMP(0) WITH LOCAL TIME    
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE FLAPPINGHISTORY modify ( COMMENT_TIME TIMESTAMP(0) WITH LOCAL TIME  
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE HOSTCHECKS modify ( START_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE    
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE HOSTCHECKS modify ( END_TIME TIMESTAMP(0) WITH LOCAL TIME ZONE      
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE HOSTSTATUS modify ( STATUS_UPDATE_TIME TIMESTAMP(0) WITH LOCAL TIME 
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE HOSTSTATUS modify ( LAST_CHECK TIMESTAMP(0) WITH LOCAL TIME ZONE    
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE HOSTSTATUS modify ( NEXT_CHECK TIMESTAMP(0) WITH LOCAL TIME ZONE    
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE HOSTSTATUS modify ( LAST_STATE_CHANGE TIMESTAMP(0) WITH LOCAL TIME  
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE HOSTSTATUS modify ( LAST_HARD_STATE_CHANGE TIMESTAMP(0) WITH LOCAL  
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE HOSTSTATUS modify ( LAST_TIME_UP TIMESTAMP(0) WITH LOCAL TIME ZONE  
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE HOSTSTATUS modify ( LAST_TIME_DOWN TIMESTAMP(0) WITH LOCAL TIME ZONE
default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );                   
ALTER TABLE HOSTSTATUS modify ( LAST_TIME_UNREACHABLE TIMESTAMP(0) WITH LOCAL   
TIME ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );        
ALTER TABLE HOSTSTATUS modify ( LAST_NOTIFICATION TIMESTAMP(0) WITH LOCAL TIME  
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             
ALTER TABLE HOSTSTATUS modify ( NEXT_NOTIFICATION TIMESTAMP(0) WITH LOCAL TIME  
ZONE  default TO_TIMESTAMP_TZ('01.01.1970 UTC','DD.MM.YYYY TZR') );             


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

