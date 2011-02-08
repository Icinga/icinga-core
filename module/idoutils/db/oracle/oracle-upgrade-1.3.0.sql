-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.3.0
--
-- add index for statehistory
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------


-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

ALTER TABLE dbversion ADD id number(11);
ALTER TABLE dbversion ADD CONSTRAINT PK_DBVERSION PRIMARY KEY (id);


MERGE INTO dbversion
USING DUAL ON (name='idoutils')
WHEN MATCHED THEN
UPDATE SET version='1.3.0'
WHEN NOT MATCHED THEN
INSERT (id, name, version) VALUES ('1', 'idoutils', '1.3.0');

-- -----------------------------------------
-- add index for statehistory
-- -----------------------------------------

CREATE INDEX statehist_i_id_o_id_s_ty_s_ti on statehistory(instance_id, object_id, state_type, state_time);

-- -----------------------------------------
-- add index for logentries
-- -----------------------------------------

CREATE INDEX loge_inst_id_time_idx on logentries (instance_id ASC, logentry_time DESC);


-- -----------------------------------------
-- fix NO_DATA_FOUND exception
-- -----------------------------------------

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



-- -----------------------------------------
-- drop unique keys for check history
-- -----------------------------------------

ALTER TABLE servicechecks DROP CONSTRAINT servicechecks;

ALTER TABLE hostchecks DROP CONSTRAINT hostchecks;


-- -----------------------------------------
-- drop unique keys for * contacts
-- -----------------------------------------

ALTER TABLE service_contacts DROP CONSTRAINT service_contacts;
ALTER TABLE host_contacts DROP CONSTRAINT host_contacts;


-- -----------------------------------------
-- add address6 column to hosts
-- -----------------------------------------

ALTER TABLE hosts ADD address6 varchar2(128);

