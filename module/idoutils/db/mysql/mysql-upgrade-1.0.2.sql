-- --------------------------------------------------------
-- mysql-upgrade-1.0.2.sql
-- DB definition for MySQL
-- 
-- Copyright (c) 2010 Icinga Development Team (http://www.icinga.org)
-- 
-- Changes:
-- no more binary casts, instead updating collation to case-sensitive 
--
-- -- --------------------------------------------------------

-- -----------------------------------------
-- change collation to case-sensitive
-- -----------------------------------------

ALTER TABLE `icinga_hosts` MODIFY COLUMN `display_name` varchar(64) character set latin1 collate latin1_general_cs NOT NULL default '';
ALTER TABLE `icinga_objects` MODIFY COLUMN `name1` varchar(128) character set latin1 collate latin1_general_cs NOT NULL default '';
ALTER TABLE `icinga_objects` MODIFY COLUMN `name2` varchar(128) character set latin1 collate latin1_general_cs default NULL;
ALTER TABLE `icinga_services` MODIFY COLUMN `display_name` varchar(64) character set latin1 collate latin1_general_cs NOT NULL default '';

