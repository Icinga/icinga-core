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

ALTER TABLE icinga_dbversion ADD dbversion_id SERIAL;
ALTER TABLE icinga_dbversion ADD PRIMARY KEY (dbversion_id);

CREATE UNIQUE INDEX dbversion ON icinga_dbversion (name);

CREATE OR REPLACE FUNCTION updatedbversion(version_i TEXT) RETURNS void AS $$
BEGIN
	IF EXISTS( SELECT * FROM icinga_dbversion WHERE name='idoutils')
	THEN
		UPDATE icinga_dbversion
		SET version=version_i WHERE name='idoutils';
	ELSE
		INSERT INTO icinga_dbversion (dbversion_id, name, version) VALUES ('1', 'idoutils', version_i);
	END IF;

	RETURN;
END;
$$ LANGUAGE plpgsql;
-- HINT: su - postgres; createlang plpgsql icinga;

SELECT updatedbversion('1.3.0');

-- -----------------------------------------
-- add index for statehistory
-- -----------------------------------------

CREATE INDEX statehist_i_id_o_id_s_ty_s_ti on icinga_statehistory(instance_id, object_id, state_type, state_time);

