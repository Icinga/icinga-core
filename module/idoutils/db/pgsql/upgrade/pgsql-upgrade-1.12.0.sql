-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.12.0
--
-- -----------------------------------------
-- Copyright (c) 2014 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------


ALTER TABLE icinga_hosts ADD COLUMN check_service_object_id bigint default NULL;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.12.0');

