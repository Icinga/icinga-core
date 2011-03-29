-- pgsql-upgrade-1.0.3.sql
-- upgrade path for Icinga IDOUtils 1.0.3
--
-- modify varchar(n) to text
-- -----------------------------------------
-- Copyright (c) 2010 Icinga Development Team (http://www.icinga.org)
--
-- Initial Revision: 2010-07-20 Michael Friedrich <michael.friedrich(at)univie.ac.at>
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- varchar(n) to text
-- -----------------------------------------

ALTER TABLE icinga_acknowledgements ALTER COLUMN author_name TYPE TEXT;
ALTER TABLE icinga_acknowledgements ALTER COLUMN comment_data TYPE TEXT;

ALTER TABLE icinga_commands ALTER COLUMN command_line TYPE TEXT;

ALTER TABLE icinga_commenthistory ALTER COLUMN author_name TYPE TEXT;
ALTER TABLE icinga_commenthistory ALTER COLUMN comment_data TYPE TEXT;

ALTER TABLE icinga_comments ALTER COLUMN author_name TYPE TEXT;
ALTER TABLE icinga_comments ALTER COLUMN comment_data TYPE TEXT;

ALTER TABLE icinga_configfiles ALTER COLUMN configfile_path TYPE TEXT;

ALTER TABLE icinga_configfilevariables ALTER COLUMN varname TYPE TEXT;
ALTER TABLE icinga_configfilevariables ALTER COLUMN varvalue TYPE TEXT;

ALTER TABLE icinga_conninfo ALTER COLUMN agent_name TYPE TEXT;
ALTER TABLE icinga_conninfo ALTER COLUMN agent_version TYPE TEXT;
ALTER TABLE icinga_conninfo ALTER COLUMN disposition TYPE TEXT;
ALTER TABLE icinga_conninfo ALTER COLUMN connect_source TYPE TEXT;
ALTER TABLE icinga_conninfo ALTER COLUMN connect_type TYPE TEXT;

ALTER TABLE icinga_contactgroups ALTER COLUMN alias TYPE TEXT;

ALTER TABLE icinga_contactnotificationmethods ALTER COLUMN command_args TYPE TEXT;

ALTER TABLE icinga_contacts ALTER COLUMN alias TYPE TEXT;
ALTER TABLE icinga_contacts ALTER COLUMN email_address TYPE TEXT;
ALTER TABLE icinga_contacts ALTER COLUMN pager_address TYPE TEXT;

ALTER TABLE icinga_contact_addresses ALTER COLUMN address TYPE TEXT;

ALTER TABLE icinga_contact_notificationcommands ALTER COLUMN command_args TYPE TEXT;

ALTER TABLE icinga_customvariables ALTER COLUMN varname TYPE TEXT;
ALTER TABLE icinga_customvariables ALTER COLUMN varvalue TYPE TEXT;

ALTER TABLE icinga_customvariablestatus ALTER COLUMN varname TYPE TEXT;
ALTER TABLE icinga_customvariablestatus ALTER COLUMN varvalue TYPE TEXT;

ALTER TABLE icinga_dbversion ALTER COLUMN name TYPE TEXT;
ALTER TABLE icinga_dbversion ALTER COLUMN version TYPE TEXT;

ALTER TABLE icinga_downtimehistory ALTER COLUMN author_name TYPE TEXT;
ALTER TABLE icinga_downtimehistory ALTER COLUMN comment_data TYPE TEXT;

ALTER TABLE icinga_eventhandlers ALTER COLUMN command_args TYPE TEXT;
ALTER TABLE icinga_eventhandlers ALTER COLUMN command_line TYPE TEXT;
ALTER TABLE icinga_eventhandlers ALTER COLUMN output TYPE TEXT;
ALTER TABLE icinga_eventhandlers ALTER COLUMN long_output TYPE TEXT;

ALTER TABLE icinga_externalcommands ALTER COLUMN command_name TYPE TEXT;
ALTER TABLE icinga_externalcommands ALTER COLUMN command_args TYPE TEXT;

ALTER TABLE icinga_hostchecks ALTER COLUMN command_args TYPE TEXT;
ALTER TABLE icinga_hostchecks ALTER COLUMN command_line TYPE TEXT;
ALTER TABLE icinga_hostchecks ALTER COLUMN output TYPE TEXT;
ALTER TABLE icinga_hostchecks ALTER COLUMN long_output TYPE TEXT;
ALTER TABLE icinga_hostchecks ALTER COLUMN perfdata TYPE TEXT;

ALTER TABLE icinga_hostgroups ALTER COLUMN alias TYPE TEXT;

ALTER TABLE icinga_hosts ALTER COLUMN alias TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN display_name TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN address TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN check_command_args TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN eventhandler_command_args TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN failure_prediction_options TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN notes TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN notes_url TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN action_url TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN icon_image TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN icon_image_alt TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN vrml_image TYPE TEXT;
ALTER TABLE icinga_hosts ALTER COLUMN statusmap_image TYPE TEXT;

ALTER TABLE icinga_hoststatus ALTER COLUMN output TYPE TEXT;
ALTER TABLE icinga_hoststatus ALTER COLUMN long_output TYPE TEXT;
ALTER TABLE icinga_hoststatus ALTER COLUMN perfdata TYPE TEXT;
ALTER TABLE icinga_hoststatus ALTER COLUMN event_handler TYPE TEXT;
ALTER TABLE icinga_hoststatus ALTER COLUMN check_command TYPE TEXT;

ALTER TABLE icinga_instances ALTER COLUMN instance_name TYPE TEXT;
ALTER TABLE icinga_instances ALTER COLUMN instance_description TYPE TEXT;

ALTER TABLE icinga_logentries ALTER COLUMN logentry_data TYPE TEXT;

ALTER TABLE icinga_notifications ALTER COLUMN output TYPE TEXT;
ALTER TABLE icinga_notifications ALTER COLUMN long_output TYPE TEXT;

ALTER TABLE icinga_objects ALTER COLUMN name1 TYPE TEXT;
ALTER TABLE icinga_objects ALTER COLUMN name2 TYPE TEXT;

ALTER TABLE icinga_processevents ALTER COLUMN program_name TYPE TEXT;
ALTER TABLE icinga_processevents ALTER COLUMN program_version TYPE TEXT;
ALTER TABLE icinga_processevents ALTER COLUMN program_date TYPE TEXT;

ALTER TABLE icinga_programstatus ALTER COLUMN global_host_event_handler TYPE TEXT;
ALTER TABLE icinga_programstatus ALTER COLUMN global_service_event_handler TYPE TEXT;

ALTER TABLE icinga_runtimevariables ALTER COLUMN varname TYPE TEXT;
ALTER TABLE icinga_runtimevariables ALTER COLUMN varvalue TYPE TEXT;

ALTER TABLE icinga_scheduleddowntime ALTER COLUMN author_name TYPE TEXT;
ALTER TABLE icinga_scheduleddowntime ALTER COLUMN comment_data TYPE TEXT;

ALTER TABLE icinga_servicechecks ALTER COLUMN command_args TYPE TEXT;
ALTER TABLE icinga_servicechecks ALTER COLUMN command_line TYPE TEXT;
ALTER TABLE icinga_servicechecks ALTER COLUMN output TYPE TEXT;
ALTER TABLE icinga_servicechecks ALTER COLUMN long_output TYPE TEXT;
ALTER TABLE icinga_servicechecks ALTER COLUMN perfdata TYPE TEXT;

ALTER TABLE icinga_servicegroups ALTER COLUMN alias TYPE TEXT;

ALTER TABLE icinga_services ALTER COLUMN display_name TYPE TEXT;
ALTER TABLE icinga_services ALTER COLUMN check_command_args TYPE TEXT;
ALTER TABLE icinga_services ALTER COLUMN eventhandler_command_args TYPE TEXT;
ALTER TABLE icinga_services ALTER COLUMN failure_prediction_options TYPE TEXT;
ALTER TABLE icinga_services ALTER COLUMN notes TYPE TEXT;
ALTER TABLE icinga_services ALTER COLUMN notes_url TYPE TEXT;
ALTER TABLE icinga_services ALTER COLUMN action_url TYPE TEXT;
ALTER TABLE icinga_services ALTER COLUMN icon_image TYPE TEXT;
ALTER TABLE icinga_services ALTER COLUMN icon_image_alt TYPE TEXT;

ALTER TABLE icinga_servicestatus ALTER COLUMN output TYPE TEXT;
ALTER TABLE icinga_servicestatus ALTER COLUMN long_output TYPE TEXT;
ALTER TABLE icinga_servicestatus ALTER COLUMN perfdata TYPE TEXT;
ALTER TABLE icinga_servicestatus ALTER COLUMN event_handler TYPE TEXT;
ALTER TABLE icinga_servicestatus ALTER COLUMN check_command TYPE TEXT;

ALTER TABLE icinga_statehistory ALTER COLUMN output TYPE TEXT;
ALTER TABLE icinga_statehistory ALTER COLUMN long_output TYPE TEXT;

ALTER TABLE icinga_systemcommands ALTER COLUMN command_line TYPE TEXT;
ALTER TABLE icinga_systemcommands ALTER COLUMN output TYPE TEXT;
ALTER TABLE icinga_systemcommands ALTER COLUMN long_output TYPE TEXT;

ALTER TABLE icinga_timeperiods ALTER COLUMN alias TYPE TEXT;

