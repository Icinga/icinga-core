-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.5.0
--
-- run it as icinga database user whithin  current directory 
-- sqlplus icinga@<instance> @ oracle-upgrade.1-5.0.sql
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

-- --------------------------------------------------------
-- alter performance data column
-- --------------------------------------------------------
/* change perfdata column on servicechecks to clob to allow unlimited performance data storage */
alter table servicechecks rename column perfdata to p_old;
alter table servicechecks add(perfdata clob) lob (perfdata) store as servicechecks_perf_lob(tablespace &LOBTBS);
update servicechecks set perfdata=p_old;
commit;
alter table servicechecks drop column p_old;

/* change perfdata column on hoststatus to clob to allow unlimited performance data storage */
alter table servicestatus rename column perfdata to p_old;
alter table servicestatus add(perfdata clob) lob (perfdata) store as servicestatus_perf_lob(tablespace &LOBTBS); 
update servicestatus set perfdata=p_old;
commit;
alter table servicestatus drop column p_old;

/* change perfdata column on hostchecks to clob to allow unlimited performance data storage */
alter table hostchecks rename column perfdata to p_old;
alter table hostchecks add(perfdata clob) lob (perfdata) store as hostchecks_perf_lob(tablespace &LOBTBS); 
update hostchecks set perfdata=p_old;
commit;
alter table hostchecks drop column p_old;

/* change perfdata column on hoststatus to clob to allow unlimited performance data storage */
alter table hoststatus rename column perfdata to p_old;
alter table hoststatus add(perfdata clob) lob (perfdata) store as hoststatus_perf_lob(tablespace &LOBTBS); 
update hoststatus set perfdata=p_old;
commit;
alter table hoststatus drop column p_old;

-- --------------------------------------------------------
-- change logentry_data column on logentries to clob 
-- --------------------------------------------------------
alter table logentries rename column logentry_data to d_old;
alter table logentries add(logentry_data clob) lob (logentry_data) store as logentries_data_lob(tablespace &LOBTBS); 
update logentries set logentry_data=d_old;
commit;
alter table logentries drop column d_old;

-- --------------------------------------------------------
-- add missed long_output column for eventhandler 
-- --------------------------------------------------------
alter table eventhandlers add (long_output clob) lob (long_output) store as eventhandlers_out_lob(tablespace &&LOBTBS);


-- -----------------------------------------
-- drop unique keys for multiline inserts
-- -----------------------------------------

ALTER TABLE configfilevariables DROP CONSTRAINT configfilevar_uq;
ALTER TABLE timeperiod_timeranges DROP CONSTRAINT timeperiod_timeranges_uq;
ALTER TABLE host_parenthosts DROP CONSTRAINT host_parenthosts_uq;
ALTER TABLE host_contactgroups DROP CONSTRAINT host_contactgroups_uq;
ALTER TABLE service_contactgroups DROP CONSTRAINT service_contactgroups_uq;
ALTER TABLE hostgroup_members DROP CONSTRAINT hostgroup_members_uq;
ALTER TABLE servicegroup_members DROP CONSTRAINT servicegroup_members_uq;
ALTER TABLE contactgroup_members DROP CONSTRAINT contactgroup_members_uq;
ALTER TABLE runtimevariables DROP CONSTRAINT runtimevariables_uq;

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

