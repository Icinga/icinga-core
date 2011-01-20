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

ALTER TABLE icinga_dbversion ADD dbversion_id int(11) NOT NULL;
ALTER TABLE icinga_dbversion ADD PRIMARY KEY (dbversion_id);

ALTER TABLE icinga_dbversion ADD UNIQUE KEY `dbversion` (`name`);

INSERT INTO icinga_dbversion (name, version) VALUES ('idoutils', '1.3.0') ON DUPLICATE KEY UPDATE version='1.3.0';

-- -----------------------------------------
-- add index for statehistory
-- -----------------------------------------

CREATE INDEX statehist_i_id_o_id_s_ty_s_ti on icinga_statehistory(instance_id, object_id, state_type, state_time);

-- -----------------------------------------
-- drop unique keys for check history
-- -----------------------------------------

ALTER TABLE `icinga_servicechecks` DROP INDEX `instance_id`;

ALTER TABLE `icinga_hostchecks` DROP INDEX `instance_id`;


