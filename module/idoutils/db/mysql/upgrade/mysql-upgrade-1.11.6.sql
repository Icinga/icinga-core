-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.11.6
--
-- -----------------------------------------
-- Copyright (c) 2014 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------


ALTER TABLE `icinga_customvariables` MODIFY COLUMN `varname` varchar(255) character set latin1 collate latin1_general_cs default NULL;
ALTER TABLE `icinga_customvariablestatus` MODIFY COLUMN `varname` varchar(255) character set latin1 collate latin1_general_cs default NULL;


-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.11.6', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.11.6', modify_time=NOW();

