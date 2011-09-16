-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.6.0
--
-- -----------------------------------------
-- Copyright (c) 2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- add end_time for acknowledgements
-- -----------------------------------------

ALTER TABLE icinga_acknowledgements ADD end_time datetime default '0000-00-00 00:00:00';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version) VALUES ('idoutils', '1.5.0') ON DUPLICATE KEY UPDATE version='1.5.0';

