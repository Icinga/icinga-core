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

ALTER TABLE icinga_acknowledgements ADD end_time datetime default '0000-00-00 00:00:00';

-- -----------------------------------------
-- Table structure for table icinga_slahistory
-- -----------------------------------------

CREATE TABLE IF NOT EXISTS icinga_slahistory (
  slahistory_id serial,
  instance_id bigint unsigned default 0,
  start_time datetime default '0000-00-00 00:00:00',
  end_time datetime default '0000-00-00 00:00:00',
  acknowledgement_time datetime default NULL,
  object_id bigint unsigned default 0,
  state smallint default 0,
  state_type smallint default '0',
  scheduled_downtime tinyint(1) default 0,
  PRIMARY KEY (slahistory_id),
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
CREATE INDEX notification_idx ON icinga_contactnotifications(notification_id, contact_object_id);
CREATE INDEX contacts_object_id_idx ON icinga_contacts(contact_object_id);
CREATE INDEX notification_idx ON icinga_contactnotificationmethods(contactnotification_id, command_object_id);
CREATE INDEX command_object_idx ON icinga_commands(object_id);                         
CREATE INDEX services_combined_idx ON icinga_services(service_object_id, host_object_id);


-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version) VALUES ('idoutils', '1.5.0') ON DUPLICATE KEY UPDATE version='1.5.0';

