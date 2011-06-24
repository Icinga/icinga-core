/*
-- --------------------------------------------------------
-- create_oracle_sys.sql
-- Create icinga tablespace and user (SYS User part)
-- called and defines set from oracle.sql
--
-- Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)
--
-- works with Oracle10+ and sqlplus
-- for because of grants on v$ views this must run as sys 
-- initial version: 2011-03-07 Thomas Dressler
-- current version: 2011-06-10 
-- --------------------------------------------------------
*/
-- -----------------------------------------
-- set defines
-- EDIT icinga_defines.sql!
-- -----------------------------------------

/*
filesystems to use for distributing index and data. In low frequency environments
this can be the same. trailing slash is mandantory
*/
DEFINE DATAFS=./
DEFINE IDXFS=./
DEFINE LOBFS=./
/*
icinga tablespaces and user must fit definitions in create_icinga_objects_oracle.sql
*/

DEFINE DATATBS=ICINGA_DATA1
DEFINE IDXTBS=ICINGA_IDX1
DEFINE LOBTBS=ICINGA_LOB1
DEFINE ICINGA_USER=icinga
DEFINE ICINGA_PASSWORD=icinga

/*
overwrite previous defines if needed
*/
@icinga_defines.sql
-- -----------------------------------------
-- set sqlplus parameter
-- -----------------------------------------
set sqlprompt "&&_USER@&&_CONNECT_IDENTIFIER SQL>"
set serveroutput on
set echo on
set feedback on

/* logging */
spool create_oracle_sys.log
whenever sqlerror continue
-- -----------------------------------------
-- drop existing user if any
-- -----------------------------------------
prompt "nonexistent user" error on the next statement can be ignored
drop user &&ICINGA_USER cascade;

-- -----------------------------------------
-- drop existing icinga tablespaces if any
-- -----------------------------------------
prompt "nonexistent tablespace" errors on the next statement can be ignored
 DROP TABLESPACE &&DATATBS including contents and datafiles;
 DROP TABLESPACE &&IDXTBS including contents and datafiles;
 DROP TABLESPACE &&LOBTBS including contents and datafiles;

 /* from now we will exit on all errors */
whenever sqlerror exit failure

-- -----------------------------------------
-- Create new tablespaces
-- -----------------------------------------

/* create tablespaces */
CREATE TABLESPACE &DATATBS DATAFILE '&&DATAFS.&DATATBS..dbf' 
	SIZE 50M AUTOEXTEND ON NEXT 50M MAXSIZE 2G 
	LOGGING EXTENT MANAGEMENT LOCAL SEGMENT SPACE MANAGEMENT AUTO;
CREATE TABLESPACE &IDXTBS DATAFILE '&&IDXFS.&IDXTBS..dbf' 
	SIZE 50M AUTOEXTEND ON NEXT 50M MAXSIZE 2G 
	LOGGING EXTENT MANAGEMENT LOCAL SEGMENT SPACE MANAGEMENT AUTO;
CREATE TABLESPACE &LOBTBS DATAFILE '&&LOBFS.&LOBTBS..dbf' 
	SIZE 50M AUTOEXTEND ON NEXT 50M MAXSIZE 2G 
	LOGGING EXTENT MANAGEMENT LOCAL SEGMENT SPACE MANAGEMENT AUTO;

-- -----------------------------------------
-- Create new user and grant rights on System and Quotas
-- -----------------------------------------

/* create user, must not exists before */
create user &ICINGA_USER identified by &ICINGA_PASSWORD 
	default tablespace &DATATBS 
	temporary tablespace temp;

/* assing tablespace quotas */
alter user &&ICINGA_USER quota unlimited on &&DATATBS;
alter user &&ICINGA_USER quota unlimited on &&IDXTBS;
alter user &&ICINGA_USER quota unlimited on &&LOBTBS;

/* object rights */
grant connect, 
	create table,
	create procedure,
	create trigger, 
	create sequence, 
	create synonym,
	create view,
	create type,
	alter session
to &&ICINGA_USER;
/* monitoring views,must be grantet from SYS  */
grant select on v_$session to &&ICINGA_USER;
grant select on v_$process to &&ICINGA_USER;
grant select on v_$sesstat to &&ICINGA_USER;
grant select on v_$mystat to &&ICINGA_USER;
grant select on v_$statname to &&ICINGA_USER;

prompt system prepared, run now create_icinga_objects_oracle.sql as &&ICINGA_USER
spool off;
exit;


