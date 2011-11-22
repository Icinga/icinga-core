-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.6.0
--
-- -----------------------------------------
-- Copyright (c) 2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- add end_time for acknowledgements
-- -----------------------------------------

ALTER TABLE icinga_acknowledgements ADD end_time timestamp default '0000-00-00 00:00:00';

-- -----------------------------------------
-- extend conninfo.agent_version #2104
-- -----------------------------------------
ALTER TABLE icinga_conninfo modify agent_version varchar(16) default '';

-- -----------------------------------------
-- Table structure for table icinga_slahistory
-- -----------------------------------------

CREATE TABLE IF NOT EXISTS icinga_slahistory (
  slahistory_id serial,
  instance_id bigint unsigned default 0,
  start_time timestamp null default NULL,
  end_time timestamp null default NULL,
  acknowledgement_time timestamp null default NULL,
  object_id bigint unsigned default 0,
  state smallint default 0,
  state_type smallint default '0',
  scheduled_downtime tinyint(1) default 0,
  PRIMARY KEY (slahistory_id)
) ENGINE=InnoDB COMMENT='SLA statehistory';

-- -----------------------------------------
-- SLA statehistory
-- -----------------------------------------

CREATE INDEX slahist_i_id_o_id_s_ti_s_s_ti_e on icinga_slahistory(instance_id,object_id,start_time,end_time);

-- -----------------------------------------
-- Icinga Web Notifications
-- -----------------------------------------
CREATE INDEX notification_idx ON icinga_notifications(notification_type, object_id, start_time);
CREATE INDEX notification_object_id_idx ON icinga_notifications(object_id);
CREATE INDEX contact_notification_idx ON icinga_contactnotifications(notification_id, contact_object_id);
CREATE INDEX contacts_object_id_idx ON icinga_contacts(contact_object_id);
CREATE INDEX contact_notif_meth_notif_idx ON icinga_contactnotificationmethods(contactnotification_id, command_object_id);
CREATE INDEX command_object_idx ON icinga_commands(object_id);
CREATE INDEX services_combined_object_idx ON icinga_services(service_object_id, host_object_id);

-- --------------------------------------------------------
-- move datetimes to timestamps, storing all in UTC
-- implements #1954
-- https://dev.icinga.org/issues/1954
-- --------------------------------------------------------

alter table  icinga_acknowledgements  modify  entry_time  timestamp default 0;
alter table  icinga_commenthistory  modify  entry_time  timestamp default 0;
alter table  icinga_commenthistory  modify  comment_time  timestamp default 0;
alter table  icinga_commenthistory  modify  expiration_time  timestamp default 0;
alter table  icinga_commenthistory  modify  deletion_time  timestamp default 0;
alter table  icinga_comments  modify  entry_time  timestamp default 0;
alter table  icinga_comments  modify  comment_time  timestamp default 0;
alter table  icinga_comments  modify  expiration_time  timestamp default 0;
alter table  icinga_conninfo  modify  connect_time  timestamp default 0;
alter table  icinga_conninfo  modify  disconnect_time  timestamp default 0;
alter table  icinga_conninfo  modify  last_checkin_time  timestamp default 0;
alter table  icinga_conninfo  modify  data_start_time  timestamp default 0;
alter table  icinga_conninfo  modify  data_end_time  timestamp default 0;
alter table  icinga_contactnotificationmethods  modify  start_time  timestamp default 0;
alter table  icinga_contactnotificationmethods  modify  end_time  timestamp default 0;
alter table  icinga_contactnotifications  modify  start_time  timestamp default 0;
alter table  icinga_contactnotifications  modify  end_time  timestamp default 0;
alter table  icinga_contactstatus  modify  status_update_time  timestamp default 0;
alter table  icinga_contactstatus  modify  last_host_notification  timestamp default 0;
alter table  icinga_contactstatus  modify  last_service_notification  timestamp default 0;
alter table  icinga_customvariablestatus  modify  status_update_time  timestamp default 0;
alter table  icinga_downtimehistory  modify  entry_time  timestamp default 0;
alter table  icinga_downtimehistory  modify  scheduled_start_time  timestamp default 0;
alter table  icinga_downtimehistory  modify  scheduled_end_time  timestamp default 0;
alter table  icinga_downtimehistory  modify  actual_start_time  timestamp default 0;
alter table  icinga_downtimehistory  modify  actual_end_time  timestamp default 0;
alter table  icinga_eventhandlers  modify  start_time  timestamp default 0;
alter table  icinga_eventhandlers  modify  end_time  timestamp default 0;
alter table  icinga_externalcommands  modify  entry_time  timestamp default 0;
alter table  icinga_flappinghistory  modify  event_time  timestamp default 0;
alter table  icinga_flappinghistory  modify  comment_time  timestamp default 0;
alter table  icinga_hostchecks  modify  start_time  timestamp default 0;
alter table  icinga_hostchecks  modify  end_time  timestamp default 0;
alter table  icinga_hoststatus  modify  status_update_time  timestamp default 0;
alter table  icinga_hoststatus  modify  last_check  timestamp default 0;
alter table  icinga_hoststatus  modify  next_check  timestamp default 0;
alter table  icinga_hoststatus  modify  last_state_change  timestamp default 0;
alter table  icinga_hoststatus  modify  last_hard_state_change  timestamp default 0;
alter table  icinga_hoststatus  modify  last_time_up  timestamp default 0;
alter table  icinga_hoststatus  modify  last_time_down  timestamp default 0;
alter table  icinga_hoststatus  modify  last_time_unreachable  timestamp default 0;
alter table  icinga_hoststatus  modify  last_notification  timestamp default 0;
alter table  icinga_hoststatus  modify  next_notification  timestamp default 0;
alter table  icinga_logentries  modify  logentry_time  timestamp default 0;
alter table  icinga_logentries  modify  entry_time  timestamp default 0;
alter table  icinga_notifications  modify  start_time  timestamp default 0;
alter table  icinga_notifications  modify  end_time  timestamp default 0;
alter table  icinga_processevents  modify  event_time  timestamp default 0;
alter table  icinga_programstatus  modify  status_update_time  timestamp default 0;
alter table  icinga_programstatus  modify  program_start_time  timestamp default 0;
alter table  icinga_programstatus  modify  program_end_time  timestamp default 0;
alter table  icinga_programstatus  modify  last_command_check  timestamp default 0;
alter table  icinga_programstatus  modify  last_log_rotation  timestamp default 0;
alter table  icinga_scheduleddowntime  modify  entry_time  timestamp default 0;
alter table  icinga_scheduleddowntime  modify  scheduled_start_time  timestamp default 0;
alter table  icinga_scheduleddowntime  modify  scheduled_end_time  timestamp default 0;
alter table  icinga_scheduleddowntime  modify  actual_start_time  timestamp default 0;
alter table  icinga_servicechecks  modify  start_time  timestamp default 0;
alter table  icinga_servicechecks  modify  end_time  timestamp default 0;
alter table  icinga_servicestatus  modify  status_update_time  timestamp default 0;
alter table  icinga_servicestatus  modify  last_check  timestamp default 0;
alter table  icinga_servicestatus  modify  next_check  timestamp default 0;
alter table  icinga_servicestatus  modify  last_state_change  timestamp default 0;
alter table  icinga_servicestatus  modify  last_hard_state_change  timestamp default 0;
alter table  icinga_servicestatus  modify  last_time_ok  timestamp default 0;
alter table  icinga_servicestatus  modify  last_time_warning  timestamp default 0;
alter table  icinga_servicestatus  modify  last_time_unknown  timestamp default 0;
alter table  icinga_servicestatus  modify  last_time_critical  timestamp default 0;
alter table  icinga_servicestatus  modify  last_notification  timestamp default 0;
alter table  icinga_servicestatus  modify  next_notification  timestamp default 0;
alter table  icinga_statehistory  modify  state_time  timestamp default 0;
alter table  icinga_systemcommands  modify  start_time  timestamp default 0;
alter table  icinga_systemcommands  modify  end_time  timestamp default 0;
alter table  icinga_timedeventqueue  modify  queued_time  timestamp default 0;
alter table  icinga_timedeventqueue  modify  scheduled_time  timestamp default 0;
alter table  icinga_timedevents  modify  queued_time  timestamp default 0;
alter table  icinga_timedevents  modify  event_time  timestamp default 0;
alter table  icinga_timedevents  modify  scheduled_time  timestamp default 0;
alter table  icinga_timedevents  modify  deletion_time  timestamp default 0;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version) VALUES ('idoutils', '1.6.0') ON DUPLICATE KEY UPDATE version='1.6.0';

