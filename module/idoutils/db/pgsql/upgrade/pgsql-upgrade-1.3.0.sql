-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.3.0
--
-- add index for statehistory
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

CREATE LANGUAGE plpgsql;

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

-- -----------------------------------------
-- add index for logentries
-- -----------------------------------------

CREATE INDEX loge_inst_id_time_idx on icinga_logentries (instance_id ASC, logentry_time DESC);


-- -----------------------------------------
-- drop unique keys for check history
-- -----------------------------------------

ALTER TABLE icinga_servicechecks DROP CONSTRAINT icinga_servicechecks_instance_id_key;

ALTER TABLE icinga_hostchecks DROP CONSTRAINT icinga_hostchecks_instance_id_key;

-- -----------------------------------------
-- drop unique keys for * contacts
-- -----------------------------------------

ALTER TABLE icinga_service_contacts DROP CONSTRAINT icinga_service_contacts_instance_id_key;
ALTER TABLE icinga_host_contacts DROP CONSTRAINT icinga_host_contacts_instance_id_key;


-- -----------------------------------------
-- add address6 column to hosts
-- -----------------------------------------

ALTER TABLE icinga_hosts ADD address6 TEXT NOT NULL default '';

