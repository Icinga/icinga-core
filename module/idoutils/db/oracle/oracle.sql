/*
-- --------------------------------------------------------
-- oracle.sql
-- DB definition for Oracle
--
-- Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)
--
-- requires ocilib, oracle client (instantclient or full client/server) libs+sdk to work
-- requires Oracle 10+ on server, 
-- see documentation for details 
--
-- you need sys account to run included scripts for user and tablespace creation  
-- you *SHOULD* look into each scripts, if assingments will fit your needs before running it
-- CAUTION: THIS WILL DROP EXISTING USER AND TABLESPACE WITH SAME NAME
-- # export TWO_TASK=<instance>; sqlplus "sys as sysdba" @ oracle.sql
--
-- initial version: 2008-02-20 David Schmidt
-- 		    2011-01-17 Michael Friedrich <michael.friedrich(at)univie.ac.at>
-- current version: 2011-03-27 Thomas Dreßler
--
-- -- --------------------------------------------------------
*/
-- -----------------------------------------
-- set defines
-- EDIT THIS !
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
DEFINE ICINGA_USER=icinga14
DEFINE ICINGA_PASSWORD=icinga14
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
-- load current instance
-- -----------------------------------------
column inst new_value dbinst noprint;
SELECT instance_name inst  FROM v$instance;
-- -----------------------------------------
-- second connect may fail if instance_name != your tns name
-- replace &dbinst with real name 
-- -----------------------------------------
DEFINE myinst=&dbinst

-- -----------------------------------------
-- run user and tablespace creation
-- CAUTION: THIS WILL DROP EXISTING USER AND TABLESPACE WITH SAME NAME
-- -----------------------------------------
@create_oracle_sys.sql

-- -----------------------------------------
-- Connect to the newly created user 
-- and create icinga objects
-- -----------------------------------------
connect &&ICINGA_USER./&&ICINGA_PASSWORD@&&myinst
@create_icinga_objects_oracle.sql

/*goodbye */
exit;
