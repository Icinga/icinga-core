ALTER TABLE `icinga`.`icinga_hostchecks` MODIFY COLUMN `perfdata` VARCHAR(8192) NULL;
ALTER TABLE `icinga`.`icinga_hoststatus` MODIFY COLUMN `perfdata` VARCHAR(8192) NULL;
ALTER TABLE `icinga`.`icinga_servicechecks` MODIFY COLUMN `perfdata` VARCHAR(8192) NULL;
ALTER TABLE `icinga`.`icinga_servicestatus` MODIFY COLUMN `perfdata` VARCHAR(8192) NULL;
