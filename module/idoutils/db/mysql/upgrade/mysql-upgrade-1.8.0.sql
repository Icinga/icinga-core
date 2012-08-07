-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.8.0
--
-- -----------------------------------------
-- Copyright (c) 2012 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #905
-- -----------------------------------------

alter table icinga_programstatus add disable_notif_expire_time timestamp default '0000-00-00 00:00:00';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.8.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.8.0', modify_time=NOW();

