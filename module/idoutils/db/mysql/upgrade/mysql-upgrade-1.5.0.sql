-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.5.0
--
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- drop unique keys for multiline inserts
-- -----------------------------------------

ALTER TABLE icinga_timeperiod_timeranges DROP KEY instance_id;
ALTER TABLE icinga_host_parenthosts DROP KEY instance_id;
ALTER TABLE icinga_host_contactgroups DROP KEY instance_id;
ALTER TABLE icinga_service_contactgroups DROP KEY instance_id;
ALTER TABLE icinga_hostgroup_members DROP KEY instance_id;
ALTER TABLE icinga_servicegroup_members DROP KEY instance_id;
ALTER TABLE icinga_contactgroup_members DROP KEY instance_id;
ALTER TABLE icinga_runtimevariables DROP KEY instance_id;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version) VALUES ('idoutils', '1.5.0') ON DUPLICATE KEY UPDATE version='1.5.0';

