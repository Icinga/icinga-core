ALTER TABLE `icinga_hostchecks` MODIFY COLUMN `long_output` TEXT NOT NULL;
ALTER TABLE `icinga_hoststatus` MODIFY COLUMN `long_output` TEXT NOT NULL;
ALTER TABLE `icinga_servicechecks` MODIFY COLUMN `long_output` TEXT NOT NULL;
ALTER TABLE `icinga_servicestatus` MODIFY COLUMN `long_output` TEXT NOT NULL;
ALTER TABLE `icinga_statehistory` MODIFY COLUMN `long_output` TEXT NOT NULL;
ALTER TABLE `icinga_eventhandlers` MODIFY COLUMN `long_output` TEXT NOT NULL;
ALTER TABLE `icinga_systemcommands` MODIFY COLUMN `long_output` TEXT NOT NULL;
ALTER TABLE `icinga_notifications` MODIFY COLUMN `long_output` TEXT NOT NULL;


ALTER TABLE `icinga_hostchecks` MODIFY COLUMN `perfdata` TEXT NULL;
ALTER TABLE `icinga_hoststatus` MODIFY COLUMN `perfdata` TEXT NULL;
ALTER TABLE `icinga_servicechecks` MODIFY COLUMN `perfdata` TEXT NULL;
ALTER TABLE `icinga_servicestatus` MODIFY COLUMN `perfdata` TEXT NULL;
