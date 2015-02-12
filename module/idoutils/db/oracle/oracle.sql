/*
-- --------------------------------------------------------
-- oracle.sql
-- DB definition for Oracle
--
-- Copyright (c) 2009-2015 Icinga Development Team (http://www.icinga.org)
--
-- requires ocilib, oracle client (instantclient or full client/server) libs+sdk to work
-- requires Oracle 10+ on server, 
-- see documentation for details 
--
-- you need sys account to run included scripts for user and tablespace creation  
-- you *SHOULD* look into each scripts, if assingments will fit your needs before running it
-- CAUTION: THIS WILL DROP EXISTING USER AND TABLESPACE WITH SAME NAME
-- # export TWO_TASK=<instance>; cd <this_sql_dir>; sqlplus "sys as sysdba" @ oracle.sql
--
-- -- --------------------------------------------------------
*/

DEFINE ICINGA_VERSION=1.13.0

-- -----------------------------------------
-- set defines
-- EDIT THIS or icinga_defines.sql
-- -----------------------------------------

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
set pagesize 200;
set linesize 200;
set heading on;
set echo on;
set feedback on;
SET ESCAPE \



-- -----------------------------------------
-- Create icinga objects
-- -----------------------------------------
@create_icinga_objects_oracle.sql

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------
INSERT INTO dbversion (id, name, version, create_time, modify_time) VALUES ('1', 'idoutils', '&&ICINGA_VERSION', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);
commit;
/*goodbye */
exit;
