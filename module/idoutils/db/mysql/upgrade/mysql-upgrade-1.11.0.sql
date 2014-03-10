-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.11.0
--
-- -----------------------------------------
-- Copyright (c) 2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- config dump in progress
-- -----------------------------------------

ALTER TABLE icinga_programstatus ADD COLUMN config_dump_in_progress SMALLINT DEFAULT 0;

-- -----------------------------------------
-- #4985
-- -----------------------------------------
CREATE INDEX commenthistory_delete_idx ON icinga_commenthistory (instance_id, comment_time, internal_comment_id);

-- -----------------------------------------
-- #5612
-- -----------------------------------------
ALTER TABLE icinga_statehistory ADD COLUMN check_source varchar(255) character set latin1 default NULL;

-- -----------------------------------------
-- #5731
-- -----------------------------------------
ALTER TABLE icinga_conninfo MODIFY agent_version VARCHAR(32);
ALTER TABLE icinga_conninfo MODIFY disposition VARCHAR(32);
ALTER TABLE icinga_conninfo MODIFY connect_source VARCHAR(32);
ALTER TABLE icinga_conninfo MODIFY connect_type VARCHAR(32);

-- --------------------------------------------------------
-- Icinga 2 specific schema extensions
-- --------------------------------------------------------

--
-- Table structure for table icinga_endpoints
--

CREATE TABLE IF NOT EXISTS icinga_endpoints (
  endpoint_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  endpoint_object_id bigint(20) unsigned DEFAULT '0',
  config_type smallint(6) DEFAULT '0',
  identity varchar(255) DEFAULT NULL,
  node varchar(255) DEFAULT NULL,
  PRIMARY KEY  (endpoint_id)
) ENGINE=InnoDB COMMENT='Endpoint configuration';

-- --------------------------------------------------------

--
-- Table structure for table icinga_endpointstatus
--

CREATE TABLE IF NOT EXISTS icinga_endpointstatus (
  endpointstatus_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  endpoint_object_id bigint(20) unsigned DEFAULT '0',
  status_update_time timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  identity varchar(255) DEFAULT NULL,
  node varchar(255) DEFAULT NULL,
  is_connected smallint(6),
  PRIMARY KEY  (endpointstatus_id)
) ENGINE=InnoDB COMMENT='Endpoint status';


ALTER TABLE icinga_servicestatus ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_hoststatus ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_contactstatus ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_programstatus ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_comments ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_scheduleddowntime ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_runtimevariables ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_customvariablestatus ADD COLUMN endpoint_object_id bigint default NULL;

ALTER TABLE icinga_acknowledgements ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_commenthistory ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_contactnotifications ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_downtimehistory ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_eventhandlers ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_externalcommands ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_flappinghistory ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_hostchecks ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_logentries ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_notifications ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_processevents ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_servicechecks ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_statehistory ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_systemcommands ADD COLUMN endpoint_object_id bigint default NULL;


-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.11.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.11.0', modify_time=NOW();

