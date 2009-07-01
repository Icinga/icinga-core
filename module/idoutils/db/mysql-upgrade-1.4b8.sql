ALTER TABLE `icinga_hostchecks` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `icinga_hoststatus` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `icinga_servicechecks` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `icinga_servicestatus` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `icinga_statehistory` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;

ALTER TABLE `icinga_eventhandlers` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `icinga_systemcommands` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;
ALTER TABLE `icinga_notifications` ADD COLUMN `long_output` varchar(8192) NOT NULL default '' AFTER `output`;

