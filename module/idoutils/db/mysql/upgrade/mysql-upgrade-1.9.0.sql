-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.9.0
--
-- -----------------------------------------
-- Copyright (c) 2012-2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #3810 - executed command
-- -----------------------------------------

ALTER TABLE icinga_hoststatus ADD executed_command TEXT;
ALTER TABLE icinga_servicestatus ADD executed_command TEXT;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.9.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.9.0', modify_time=NOW();

