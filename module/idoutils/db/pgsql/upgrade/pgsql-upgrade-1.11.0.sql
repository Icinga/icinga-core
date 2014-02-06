-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.11.0
--
-- -----------------------------------------
-- Copyright (c) 2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #5050 config dump in progress
-- -----------------------------------------

ALTER TABLE icinga_programstatus ADD config_dump_in_progress INTEGER DEFAULT 0;

-- -----------------------------------------
-- #4985
-- -----------------------------------------
CREATE INDEX commenthistory_delete_idx ON icinga_commenthistory (instance_id, comment_time, internal_comment_id);

-- -----------------------------------------
-- #5612
-- -----------------------------------------
ALTER TABLE icinga_statehistory ADD COLUMN check_source text default NULL;

ALTER TABLE icinga_acknowledgements ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_commenthistory ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_contactnotifications ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_downtimehistory ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_eventhandlers ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_externalcommands ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_flappinghistory ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_hostchecks ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_logentries ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_notifications ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_processevents ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_servicechecks ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_statehistory ADD COLUMN icinga_node text default NULL;
ALTER TABLE icinga_systemcommands ADD COLUMN icinga_node text default NULL;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.0');

