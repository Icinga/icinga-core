-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.7.0
--
-- -----------------------------------------
-- Copyright (c) 2012 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #2274
-- -----------------------------------------
create index statehist_state_idx on icinga_statehistory(object_id,state);

-- -----------------------------------------
-- #2203 cannot handle timstamp with timezone
-- -----------------------------------------

DROP FUNCTION from_unixtime(integer);
CREATE OR REPLACE FUNCTION from_unixtime(integer) RETURNS timestamp with time zone AS '
         SELECT to_timestamp($1) AS result
' LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION unix_timestamp(timestamp with time zone) RETURNS bigint AS '
        SELECT EXTRACT(EPOCH FROM $1)::bigint AS result;
' LANGUAGE 'SQL';

-- -----------------------------------------
-- #2537
-- -----------------------------------------

alter table icinga_downtimehistory add is_in_effect INTEGER  default 0;
alter table icinga_downtimehistory add trigger_time timestamp with time zone default '1970-01-01 00:00:00';
alter table icinga_scheduleddowntime add is_in_effect INTEGER  default 0;
alter table icinga_scheduleddowntime add trigger_time timestamp with time zone default '1970-01-01 00:00:00';


-- -----------------------------------------
-- #2562
-- -----------------------------------------

alter table icinga_dbversion add create_time timestamp with time zone default '1970-01-01 00:00:00';
alter table icinga_dbversion add modify_time timestamp with time zone default '1970-01-01 00:00:00';

CREATE OR REPLACE FUNCTION updatedbversion(version_i TEXT) RETURNS void AS $$
BEGIN
        IF EXISTS( SELECT * FROM icinga_dbversion WHERE name='idoutils')
        THEN
                UPDATE icinga_dbversion
                SET version=version_i, modify_time=NOW()
                WHERE name='idoutils';
        ELSE
                INSERT INTO icinga_dbversion (dbversion_id, name, version, create_time, modify_time) VALUES ('1', 'idoutils', version_i, NOW(), NOW());
        END IF;

        RETURN;
END;
$$ LANGUAGE plpgsql;
-- HINT: su - postgres; createlang plpgsql icinga;


-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.7.0');

