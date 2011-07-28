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

ALTER TABLE icinga_timeperiod_timeranges DROP CONSTRAINT uq_timeperiod_timeranges;
ALTER TABLE icinga_host_parenthosts DROP CONSTRAINT uq_host_parenthosts;
ALTER TABLE icinga_host_contactgroups DROP CONSTRAINT uq_host_contactgroups;
ALTER TABLE icinga_service_contactgroups DROP CONSTRAINT uq_service_contactgroups;
ALTER TABLE icinga_hostgroup_members DROP CONSTRAINT uq_hostgroup_members;
ALTER TABLE icinga_servicegroup_members DROP CONSTRAINT uq_servicegroup_members;
ALTER TABLE icinga_contactgroup_members DROP CONSTRAINT uq_contactgroup_members;
ALTER TABLE icinga_runtimevariables DROP CONSTRAINT uq_runtimevariables;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.5.0');

