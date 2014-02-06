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

ALTER TABLE icinga_acknowledgements ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_commenthistory ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_contactnotifications ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_downtimehistory ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_eventhandlers ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_externalcommands ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_flappinghistory ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_hostchecks ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_logentries ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_notifications ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_processevents ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_servicechecks ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_statehistory ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;
ALTER TABLE icinga_systemcommands ADD COLUMN icinga_node varchar(255) character set latin1 default NULL;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.11.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.11.0', modify_time=NOW();

