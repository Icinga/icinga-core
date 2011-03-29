-- --------------------------------------------------------
-- mysql-upgrade-1.0.2.sql
-- DB definition for MySQL
--
-- modify display_name to varchar(255)
--
-- -- --------------------------------------------------------

ALTER TABLE `icinga_hosts` MODIFY COLUMN `display_name` varchar(255) NOT NULL;
ALTER TABLE `icinga_services` MODIFY COLUMN `display_name` varchar(255) NOT NULL;
ALTER TABLE `icinga_configfilevariables` MODIFY COLUMN `varvalue` varchar(1024) NOT NULL;


