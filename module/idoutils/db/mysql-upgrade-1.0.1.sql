-- --------------------------------------------------------
-- mysql-upgrade-1.0.1.sql
-- DB definition for MySQL
-- 
-- modify command_line to carchar(1024)
-- sets index on several tables for improved delete
--
-- -- --------------------------------------------------------



-- modify command_line size to varchar(1024) instead of varchar(255)

ALTER TABLE `icinga_hostchecks` MODIFY COLUMN `command_line` varchar(1024) NOT NULL;
ALTER TABLE `icinga_servicechecks` MODIFY COLUMN `command_line` varchar(1024) NOT NULL;
ALTER TABLE `icinga_systemcommands` MODIFY COLUMN `command_line` varchar(1024) NOT NULL;
ALTER TABLE `icinga_eventhandlers` MODIFY COLUMN `command_line` varchar(1024) NOT NULL;


-- -----------------------------------------
-- add index
-- -----------------------------------------

-- for periodic delete 
-- instance_id and
-- TIMEDEVENTS => scheduled_time
-- SYSTEMCOMMANDS, SERVICECHECKS, HOSTCHECKS, EVENTHANDLERS  => start_time
-- EXTERNALCOMMANDS => entry_time

-- instance_id
CREATE INDEX instance_id_idx on icinga_timedevents(instance_id);
CREATE INDEX instance_id_idx on icinga_systemcommands(instance_id);
CREATE INDEX instance_id_idx on icinga_servicechecks(instance_id);
CREATE INDEX instance_id_idx on icinga_hostchecks(instance_id);
CREATE INDEX instance_id_idx on icinga_eventhandlers(instance_id);
CREATE INDEX instance_id_idx on icinga_externalcommands(instance_id);

-- time
CREATE INDEX time_id_idx on icinga_timedevents(scheduled_time);
CREATE INDEX time_id_idx on icinga_systemcommands(start_time);
CREATE INDEX time_id_idx on icinga_servicechecks(start_time);
CREATE INDEX time_id_idx on icinga_hostchecks(start_time);
CREATE INDEX time_id_idx on icinga_eventhandlers(start_time);
CREATE INDEX time_id_idx on icinga_externalcommands(entry_time);

-- for starting cleanup - referenced in dbhandler.c:882
-- instance_id only

-- realtime data
CREATE INDEX instance_id_idx on icinga_programstatus(instance_id);
CREATE INDEX instance_id_idx on icinga_hoststatus(instance_id);
CREATE INDEX instance_id_idx on icinga_servicestatus(instance_id);
CREATE INDEX instance_id_idx on icinga_contactstatus(instance_id);
CREATE INDEX instance_id_idx on icinga_timedeventqueue(instance_id);
CREATE INDEX instance_id_idx on icinga_comments(instance_id);
CREATE INDEX instance_id_idx on icinga_scheduleddowntime(instance_id);
CREATE INDEX instance_id_idx on icinga_runtimevariables(instance_id);
CREATE INDEX instance_id_idx on icinga_customvariablestatus(instance_id);

-- config data
CREATE INDEX instance_id_idx on icinga_configfiles(instance_id);
CREATE INDEX instance_id_idx on icinga_configfilevariables(instance_id);
CREATE INDEX instance_id_idx on icinga_customvariables(instance_id);
CREATE INDEX instance_id_idx on icinga_commands(instance_id);
CREATE INDEX instance_id_idx on icinga_timeperiods(instance_id);
CREATE INDEX instance_id_idx on icinga_timeperiod_timeranges(instance_id);
CREATE INDEX instance_id_idx on icinga_contactgroups(instance_id);
CREATE INDEX instance_id_idx on icinga_contactgroup_members(instance_id);
CREATE INDEX instance_id_idx on icinga_hostgroups(instance_id);
CREATE INDEX instance_id_idx on icinga_hostgroup_members(instance_id);
CREATE INDEX instance_id_idx on icinga_servicegroups(instance_id);
CREATE INDEX instance_id_idx on icinga_servicegroup_members(instance_id);
CREATE INDEX instance_id_idx on icinga_hostescalations(instance_id);
CREATE INDEX instance_id_idx on icinga_hostescalation_contacts(instance_id);
CREATE INDEX instance_id_idx on icinga_serviceescalations(instance_id);
CREATE INDEX instance_id_idx on icinga_serviceescalation_contacts(instance_id);
CREATE INDEX instance_id_idx on icinga_hostdependencies(instance_id);
CREATE INDEX instance_id_idx on icinga_contacts(instance_id);
CREATE INDEX instance_id_idx on icinga_contact_addresses(instance_id);
CREATE INDEX instance_id_idx on icinga_contact_notificationcommands(instance_id);
CREATE INDEX instance_id_idx on icinga_hosts(instance_id);
CREATE INDEX instance_id_idx on icinga_host_parenthosts(instance_id);
CREATE INDEX instance_id_idx on icinga_host_contacts(instance_id);
CREATE INDEX instance_id_idx on icinga_services(instance_id);
CREATE INDEX instance_id_idx on icinga_service_contacts(instance_id);
CREATE INDEX instance_id_idx on icinga_service_contactgroups(instance_id);
CREATE INDEX instance_id_idx on icinga_host_contactgroups(instance_id);
CREATE INDEX instance_id_idx on icinga_hostescalation_contactgroups(instance_id);
CREATE INDEX instance_id_idx on icinga_serviceescalation_contactgroups(instance_id);

