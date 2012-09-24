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
-- #2618
-- -----------------------------------------

CREATE INDEX cntgrpmbrs_cgid_coid ON icinga_contactgroup_members (contactgroup_id,contact_object_id);
CREATE INDEX hstgrpmbrs_hgid_hoid ON icinga_hostgroup_members (hostgroup_id,host_object_id);
CREATE INDEX hstcntgrps_hid_cgoid ON icinga_host_contactgroups (host_id,contactgroup_object_id);
CREATE INDEX hstprnthsts_hid_phoid ON icinga_host_parenthosts (host_id,parent_host_object_id);
CREATE INDEX runtimevars_iid_varn ON icinga_runtimevariables (instance_id,varname);
CREATE INDEX sgmbrs_sgid_soid ON icinga_servicegroup_members (servicegroup_id,service_object_id);
CREATE INDEX scgrps_sid_cgoid ON icinga_service_contactgroups (service_id,contactgroup_object_id);
CREATE INDEX tperiod_tid_d_ss_es ON icinga_timeperiod_timeranges (timeperiod_id,day,start_sec,end_sec);

-- -----------------------------------------
-- #3018
-- -----------------------------------------

ALTER TABLE icinga_acknowledgements DROP INDEX acknowledgement_id;
ALTER TABLE icinga_commands DROP INDEX command_id;
ALTER TABLE icinga_commenthistory DROP INDEX commenthistory_id;
ALTER TABLE icinga_comments DROP INDEX comment_id;
ALTER TABLE icinga_configfiles DROP INDEX configfile_id;
ALTER TABLE icinga_configfilevariables DROP INDEX configfilevariable_id;
ALTER TABLE icinga_conninfo DROP INDEX conninfo_id;
ALTER TABLE icinga_contactgroups DROP INDEX contactgroup_id;
ALTER TABLE icinga_contactgroup_members DROP INDEX contactgroup_member_id;
ALTER TABLE icinga_contactnotificationmethods DROP INDEX contactnotificationmethod_id;
ALTER TABLE icinga_contactnotifications DROP INDEX contactnotification_id;
ALTER TABLE icinga_contacts DROP INDEX contact_id;
ALTER TABLE icinga_contactstatus DROP INDEX contactstatus_id;
ALTER TABLE icinga_contact_addresses DROP INDEX contact_address_id;
ALTER TABLE icinga_contact_notificationcommands DROP INDEX contact_notificationcommand_id;
ALTER TABLE icinga_customvariables DROP INDEX customvariable_id;
ALTER TABLE icinga_customvariablestatus DROP INDEX customvariablestatus_id;
ALTER TABLE icinga_dbversion DROP INDEX dbversion_id;
ALTER TABLE icinga_downtimehistory DROP INDEX downtimehistory_id;
ALTER TABLE icinga_eventhandlers DROP INDEX eventhandler_id;
ALTER TABLE icinga_externalcommands DROP INDEX externalcommand_id;
ALTER TABLE icinga_flappinghistory DROP INDEX flappinghistory_id;
ALTER TABLE icinga_hostchecks DROP INDEX hostcheck_id;
ALTER TABLE icinga_hostdependencies DROP INDEX hostdependency_id;
ALTER TABLE icinga_hostescalations DROP INDEX hostescalation_id;
ALTER TABLE icinga_hostescalation_contactgroups DROP INDEX hostescalation_contactgroup_id;
ALTER TABLE icinga_hostescalation_contacts DROP INDEX hostescalation_contact_id;
ALTER TABLE icinga_hostgroups DROP INDEX hostgroup_id;
ALTER TABLE icinga_hostgroup_members DROP INDEX hostgroup_member_id;
ALTER TABLE icinga_hosts DROP INDEX host_id;
ALTER TABLE icinga_hoststatus DROP INDEX hoststatus_id;
ALTER TABLE icinga_host_contactgroups DROP INDEX host_contactgroup_id;
ALTER TABLE icinga_host_contacts DROP INDEX host_contact_id;
ALTER TABLE icinga_host_parenthosts DROP INDEX host_parenthost_id;
ALTER TABLE icinga_instances DROP INDEX instance_id;
ALTER TABLE icinga_logentries DROP INDEX logentry_id;
ALTER TABLE icinga_notifications DROP INDEX notification_id;
ALTER TABLE icinga_objects DROP INDEX object_id;
ALTER TABLE icinga_processevents DROP INDEX processevent_id;
ALTER TABLE icinga_programstatus DROP INDEX programstatus_id;
ALTER TABLE icinga_runtimevariables DROP INDEX runtimevariable_id;
ALTER TABLE icinga_scheduleddowntime DROP INDEX scheduleddowntime_id;
ALTER TABLE icinga_servicechecks DROP INDEX servicecheck_id;
ALTER TABLE icinga_servicedependencies DROP INDEX servicedependency_id;
ALTER TABLE icinga_serviceescalations DROP INDEX serviceescalation_id;
ALTER TABLE icinga_serviceescalation_contactgroups DROP INDEX serviceescalation_contactgroup_id;
ALTER TABLE icinga_serviceescalation_contacts DROP INDEX serviceescalation_contact_id;
ALTER TABLE icinga_servicegroups DROP INDEX servicegroup_id;
ALTER TABLE icinga_servicegroup_members DROP INDEX servicegroup_member_id;
ALTER TABLE icinga_services DROP INDEX service_id;
ALTER TABLE icinga_servicestatus DROP INDEX servicestatus_id;
ALTER TABLE icinga_service_contactgroups DROP INDEX service_contactgroup_id;
ALTER TABLE icinga_service_contacts DROP INDEX service_contact_id;
ALTER TABLE icinga_statehistory DROP INDEX statehistory_id;
ALTER TABLE icinga_systemcommands DROP INDEX systemcommand_id;
ALTER TABLE icinga_timedeventqueue DROP INDEX timedeventqueue_id;
ALTER TABLE icinga_timedevents DROP INDEX timedevent_id;
ALTER TABLE icinga_timeperiods DROP INDEX timeperiod_id;
ALTER TABLE icinga_timeperiod_timeranges DROP INDEX timeperiod_timerange_id;
ALTER TABLE icinga_slahistory DROP INDEX slahistory_id;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.8.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.8.0', modify_time=NOW();

