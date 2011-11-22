-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.6.0
--
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- add end_time for acknowledgements
-- -----------------------------------------

ALTER TABLE icinga_acknowledgements ADD end_time timestamp with time zone default '1970-01-01 00:00:00';


-- --------------------------------------------------------

-- -----------------------------------------
-- Table structure for table icinga_slahistory
-- -----------------------------------------
                                                                                
CREATE TABLE icinga_slahistory (
  slahistory_id serial,
  instance_id bigint default 0,
  start_time timestamp default '1970-01-01 00:00:00',
  end_time timestamp default '1970-01-01 00:00:00',
  acknowledgement_time timestamp default '1970-01-01 00:00:00',
  object_id bigint default 0,
  state INTEGER default 0,
  state_type INTEGER default '0',
  scheduled_downtime INTEGER default 0,
  CONSTRAINT PK_slahistory_id PRIMARY KEY (slahistory_id)
) ;

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

-- -----------------------------------------
-- modify timestamp to use time zone, #2106
-- https://dev.icinga.org/issues/2106
-- -----------------------------------------

alter table icinga_acknowledgements
        alter column entry_time set data type timestamp with time zone
        using timestamp with time zone 'epoch' + extract(epoch from entry_time) * interval '1 second';


alter table icinga_commenthistory
        alter column entry_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from entry_time) * interval '1 second',
        alter column comment_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from comment_time) * interval '1 second',
        alter column expiration_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from expiration_time) * interval '1 second',
        alter column deletion_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from deletion_time) * interval '1 second';

alter table icinga_comments 
        alter column entry_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from entry_time) * interval '1 second',
        alter column comment_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from comment_time) * interval '1 second',
        alter column expiration_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from expiration_time) * interval '1 second';

alter table icinga_conninfo 
        alter column connect_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from connect_time) * interval '1 second',
        alter column disconnect_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from disconnect_time) * interval '1 second',
        alter column last_checkin_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_checkin_time) * interval '1 second',
        alter column data_start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from data_start_time) * interval '1 second',
        alter column data_end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from data_end_time) * interval '1 second';

alter table icinga_contactnotificationmethods
        alter column start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from start_time) * interval '1 second',
        alter column end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from end_time) * interval '1 second';

alter table icinga_contactnotifications
        alter column start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from start_time) * interval '1 second',
        alter column end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from end_time) * interval '1 second';

alter table icinga_contactstatus 
        alter column status_update_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from status_update_time) * interval '1 second',
        alter column last_host_notification set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_host_notification) * interval '1 second',
        alter column last_service_notification set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_service_notification) * interval '1 second';

alter table icinga_customvariablestatus
        alter column status_update_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from status_update_time) * interval '1 second';

alter table icinga_downtimehistory
        alter column entry_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from entry_time) * interval '1 second',
        alter column scheduled_start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from scheduled_start_time) * interval '1 second',
        alter column scheduled_end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from scheduled_end_time) * interval '1 second',
        alter column actual_start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from actual_start_time) * interval '1 second',
        alter column actual_end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from actual_end_time) * interval '1 second';

alter table icinga_eventhandlers
        alter column start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from start_time) * interval '1 second',
        alter column end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from end_time) * interval '1 second';

alter table icinga_externalcommands
        alter column entry_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from entry_time) * interval '1 second';

alter table icinga_flappinghistory
        alter column event_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from event_time) * interval '1 second',
        alter column comment_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from comment_time) * interval '1 second';

alter table icinga_hostchecks
        alter column start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from start_time) * interval '1 second',
        alter column end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from end_time) * interval '1 second';

alter table icinga_hoststatus 
        alter column status_update_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from status_update_time) * interval '1 second',
        alter column last_check set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_check) * interval '1 second',
        alter column next_check set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from next_check) * interval '1 second',
        alter column last_state_change set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_state_change) * interval '1 second',
        alter column last_hard_state_change set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_hard_state_change) * interval '1 second',
        alter column last_time_up set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_time_up) * interval '1 second',
        alter column last_time_down set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_time_down) * interval '1 second',
        alter column last_time_unreachable set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_time_unreachable) * interval '1 second',
        alter column last_notification set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_notification) * interval '1 second',
        alter column next_notification set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from next_notification) * interval '1 second';

alter table icinga_logentries
        alter column logentry_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from logentry_time) * interval '1 second',
        alter column entry_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from entry_time) * interval '1 second';

alter table icinga_notifications
        alter column start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from start_time) * interval '1 second',
        alter column end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from end_time) * interval '1 second';

alter table icinga_processevents 
        alter column event_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from event_time) * interval '1 second';

alter table icinga_programstatus
        alter column status_update_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from status_update_time) * interval '1 second',
        alter column program_start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from program_start_time) * interval '1 second',
        alter column program_end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from program_end_time) * interval '1 second',
        alter column last_command_check set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_command_check) * interval '1 second',
        alter column last_log_rotation set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_log_rotation) * interval '1 second';

alter table icinga_scheduleddowntime
        alter column entry_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from entry_time) * interval '1 second',
        alter column scheduled_start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from scheduled_start_time) * interval '1 second',
        alter column scheduled_end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from scheduled_end_time) * interval '1 second',
        alter column actual_start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from actual_start_time) * interval '1 second';

alter table icinga_servicechecks
        alter column start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from start_time) * interval '1 second',
        alter column end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from end_time) * interval '1 second';

alter table icinga_servicestatus 
        alter column status_update_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from status_update_time) * interval '1 second',
        alter column last_check set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_check) * interval '1 second',
        alter column next_check set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from next_check) * interval '1 second',
        alter column last_state_change set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_state_change) * interval '1 second',
        alter column last_hard_state_change set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_hard_state_change) * interval '1 second',
        alter column last_time_ok set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_time_ok) * interval '1 second',
        alter column last_time_warning set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_time_warning) * interval '1 second',
        alter column last_time_unknown set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_time_unknown) * interval '1 second',
        alter column last_time_critical set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_time_critical) * interval '1 second',
        alter column last_notification set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from last_notification) * interval '1 second',
        alter column next_notification set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from next_notification) * interval '1 second';

alter table icinga_statehistory
        alter column state_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from state_time) * interval '1 second';

alter table icinga_systemcommands
        alter column start_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from start_time) * interval '1 second',
        alter column end_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from end_time) * interval '1 second';

alter table icinga_timedeventqueue
        alter column queued_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from queued_time) * interval '1 second',
        alter column scheduled_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from scheduled_time) * interval '1 second';

alter table icinga_timedevents
        alter column queued_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from queued_time) * interval '1 second',
        alter column event_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from event_time) * interval '1 second',
        alter column scheduled_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from scheduled_time) * interval '1 second',
        alter column deletion_time set data type timestamp with time zone
	        using timestamp with time zone 'epoch' + extract(epoch from deletion_time) * interval '1 second';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.6.0');

