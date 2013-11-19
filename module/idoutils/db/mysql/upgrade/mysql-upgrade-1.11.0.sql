-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.11.0
--
-- -----------------------------------------
-- Copyright (c) 2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- config dump in progress
-- -----------------------------------------

ALTER TABLE icinga_programstatus ADD COLUMN config_dump_in_progress SMALLINT DEFAULT 0;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.11.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.11.0', modify_time=NOW();

