/*
-- --------------------------------------------------------
-- alter_icinga13_numbers.sql
-- oracle schema update script for icinga V1.3.0 to V1.4.0
-- called by oracle-upgrade-1.4.0.sql
-- --------------------------------------------------------
-- remove number limitations
-- fixes Bug #1173: int(11) to small for some of ido tables https://dev.icinga.org/issues/1173
--       Bug #1401: integer not big enough for bytes_processed https://dev.icinga.org/issues/1401
-- --------------------------------------------------------
-- works with Oracle10+ and sqlplus
-- 
-- initial version: 2011-03-01 Thomas Dressler
-- current version: 2011-05-01 Thomas Dressler
-- --------------------------------------------------------
*/
-- -----------------------------------------
-- set sqlplus parameter
-- -----------------------------------------
set sqlprompt "&&_USER@&&_CONNECT_IDENTIFIER SQL>"
set serveroutput on size 100000
set pagesize 200;
set linesize 200;
set heading off;
set echo off;
set feedback off;

/* error handling and logging */
whenever sqlerror exit failure
spool alter_numbers_sql.sql;

-- -----------------------------------------
-- Drop function based index 
-- cannot alter tables with index on this (mostly DESC Index)
-- -----------------------------------------
select 'Drop index '||index_name||';' from user_indexes where INDEX_TYPE='FUNCTION-BASED NORMAL'; 

-- -----------------------------------------
-- Prepare Alter Table script for each number column and run it
-- -----------------------------------------
select 'ALTER TABLE '||table_name ||' modify ('|| column_name||' '|| decode(data_scale,0,'INTEGER','NUMBER')||');' from user_tab_columns where data_type='NUMBER';
spool off;
set heading on;
set echo on;
set feedback on;
spool alter_icinga13_numbers.log;
--run created sql file
@alter_numbers_sql.sql;
spool off;
