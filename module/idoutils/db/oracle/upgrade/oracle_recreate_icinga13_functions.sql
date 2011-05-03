/*
-- --------------------------------------------------------
-- recreate_icinga13_functions.sql
-- oracle schema update script for icinga V1.3.0 to V1.4.0
-- called by oracle-upgrade-1.4.0.sql
-- --------------------------------------------------------
-- fixes or changes pl/sql functions
-- works with Oracle10+ and sqlplus
--
-- initial version: 2011-04-03 Thomas Dressler
--
-- --------------------------------------------------------
*/
-- -----------------------------------------
-- set sqlplus parameter
-- -----------------------------------------
set sqlprompt "&&_USER@&&_CONNECT_IDENTIFIER SQL>"
set serveroutput on size 20000
set echo on
set feedback on



/* error handling and logging */
whenever sqlerror exit failure
spool recreate_icinga13_functions.log

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

/*
-- --------------------------------------------------------
-- add NO_DATA_FOUND exception handler #
-- https://dev.icinga.org/issues/1363
-- --------------------------------------------------------
*/
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
        /* surpress ORA-01403 exception if nothing to delete */
        exception
                when NO_DATA_FOUND then null;
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
        /* surpress ORA-01403 exception if nothing to delete */
        exception
                when NO_DATA_FOUND then null;
END;
/

spool off;
