-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.12.0
--
-- -----------------------------------------
-- Copyright (c) 2014 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------


ALTER TABLE icinga_servicestatus ADD COLUMN check_source_object_id bigint default NULL;
ALTER TABLE icinga_hoststatus ADD COLUMN check_source_object_id bigint default NULL;
ALTER TABLE icinga_statehistory ADD COLUMN check_source_object_id bigint default NULL;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.12.0');

