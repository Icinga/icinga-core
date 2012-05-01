-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.7.0
--
-- -----------------------------------------
-- Copyright (c) 2012 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #2274
-- -----------------------------------------
create index statehist_state_idx on icinga_statehistory(object_id,state);

-- -----------------------------------------
-- #2479
-- -----------------------------------------
alter table icinga_hosts modify FAILURE_PREDICTION_OPTIONS varchar(128) ;

-- -----------------------------------------
-- #2537
-- -----------------------------------------

alter table icinga_downtimehistory add is_in_effect smallint default 0;
alter table icinga_downtimehistory add trigger_time timestamp  default '0000-00-00 00:00:00';
alter table icinga_scheduleddowntime add is_in_effect smallint default 0;
alter table icinga_scheduleddowntime add trigger_time timestamp  default '0000-00-00 00:00:00';

-- -----------------------------------------
-- #2181
-- -----------------------------------------

-- acknowledgements
alter table icinga_acknowledgements modify comment_data TEXT;

-- commands
alter table icinga_commands modify command_line TEXT;

-- commenthistory
alter table icinga_commenthistory modify comment_data TEXT;

-- comments
alter table icinga_comments modify comment_data TEXT;

-- configfilevariables
alter table icinga_configfilevariables modify varvalue TEXT;

-- contactgroups
alter table icinga_contactgroups modify alias TEXT;

-- contactnotificationmethods
alter table icinga_contactnotificationmethods modify command_args TEXT;

-- customvariables
alter table icinga_customvariables modify varvalue TEXT;

-- customvariablestatus
alter table icinga_customvariablestatus modify varvalue TEXT;

-- downtimehistory
alter table icinga_downtimehistory modify comment_data TEXT;

-- eventhandlers
alter table icinga_eventhandlers modify command_args TEXT;
alter table icinga_eventhandlers modify command_line TEXT;
alter table icinga_eventhandlers modify output TEXT;

-- externalcommands
alter table icinga_externalcommands modify command_args TEXT;

-- hostchecks
alter table icinga_hostchecks modify command_args TEXT;
alter table icinga_hostchecks modify command_line TEXT;
alter table icinga_hostchecks modify output TEXT;

-- hostgroups
alter table icinga_hostgroups modify alias TEXT;

-- hosts
alter table icinga_hosts modify check_command_args TEXT;
alter table icinga_hosts modify eventhandler_command_args TEXT;
alter table icinga_hosts modify notes TEXT;
alter table icinga_hosts modify notes_url TEXT;
alter table icinga_hosts modify action_url TEXT;
alter table icinga_hosts modify icon_image TEXT;
alter table icinga_hosts modify icon_image_alt TEXT;
alter table icinga_hosts modify vrml_image TEXT;
alter table icinga_hosts modify statusmap_image TEXT;

-- hoststatus
alter table icinga_hoststatus modify output TEXT;
alter table icinga_hoststatus modify event_handler TEXT;
alter table icinga_hoststatus modify check_command TEXT;

-- logentries
alter table icinga_logentries modify logentry_data TEXT;

-- notifications
alter table icinga_notifications modify output TEXT;

-- programstatus
alter table icinga_programstatus modify global_host_event_handler TEXT;
alter table icinga_programstatus modify global_service_event_handler TEXT;

-- runtimevariables
alter table icinga_runtimevariables modify varvalue TEXT;

-- scheduleddowntime
alter table icinga_scheduleddowntime modify comment_data TEXT;

-- servicechecks
alter table icinga_servicechecks modify command_args TEXT;
alter table icinga_servicechecks modify command_line TEXT;
alter table icinga_servicechecks modify output TEXT;

-- servicegroups
alter table icinga_servicegroups modify alias TEXT;

-- services
alter table icinga_services modify check_command_args TEXT;
alter table icinga_services modify eventhandler_command_args TEXT;
alter table icinga_services modify notes TEXT;
alter table icinga_services modify notes_url TEXT;
alter table icinga_services modify action_url TEXT;
alter table icinga_services modify icon_image TEXT;
alter table icinga_services modify icon_image_alt TEXT;

-- servicestatus
alter table icinga_servicestatus modify output TEXT;
alter table icinga_servicestatus modify event_handler TEXT;
alter table icinga_servicestatus modify check_command TEXT;

-- statehistory
alter table icinga_statehistory modify output TEXT;

-- systemcommands
alter table icinga_systemcommands modify command_line TEXT;
alter table icinga_systemcommands modify output TEXT;

-- timeperiods
alter table icinga_timeperiods modify alias TEXT;

-- -----------------------------------------
-- #2562
-- -----------------------------------------

alter table icinga_dbversion add create_time timestamp NOT NULL DEFAULT '0000-00-00 00:00:00';
alter table icinga_dbversion add modify_time timestamp NOT NULL DEFAULT '0000-00-00 00:00:00';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.7.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.7.0', modify_time=NOW();

