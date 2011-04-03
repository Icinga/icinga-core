-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.4.0
--
-- run it as icinga database user whithin  current directory 
-- sqlplus icinga@<instance> @ oracle-upgrade.1-4.0.sql
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

define ICINGA_VERSION=1.4.0

-- --------------------------------------------------------
-- move icinga index/lobs to dedicated tablespace
-- implements Feature #1355
-- https://dev.icinga.org/issues/1355
-- warning:edit this script to define existing tablespaces
-- this particular step can be skipped safetly
-- --------------------------------------------------------
/* set real TBS names on which you have quota, no checks are implemented!*/
define DATATBS='ICINGA_DATA';
define LOBTBS='ICINGA_DATA';
define IXTBS='ICINGA_IDX';
@move_icinga13_objects.sql

-- --------------------------------------------------------
-- remove number limitations
-- fixes Bug #1173: int(11) to small for some of ido tables
-- https://dev.icinga.org/issues/1173
-- --------------------------------------------------------
@alter_icinga13_numbers.sql

/* script will be terminated on the first error */
whenever sqlerror exit failure
spool oracle-upgrade-&&ICINGA_VERSION..log

-- --------------------------------------------------------
-- redefine unix time functions
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


-- -----------------------------------------
-- finally update dbversion
-- -----------------------------------------

MERGE INTO dbversion
	USING DUAL ON (name='idoutils')
	WHEN MATCHED THEN
		UPDATE SET version='1.4.0'
	WHEN NOT MATCHED THEN
		INSERT (id, name, version) VALUES ('1', 'idoutils', '&&ICINGA_VERSION');

/* last check */
select object_name,object_type,status  from user_objects where status !='VALID';

/* goodbye */
spool off 
exit;

