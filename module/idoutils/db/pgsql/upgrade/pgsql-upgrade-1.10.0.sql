-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.10.0
--
-- -----------------------------------------
-- Copyright (c) 2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #4363 deprecate enable_sla
-- -----------------------------------------

-- DROP TABLE icinga_slahistory;

-- -----------------------------------------
-- #1883 expanded values
-- -----------------------------------------

ALTER TABLE icinga_hosts ADD notes_expanded TEXT  default '';
ALTER TABLE icinga_hosts ADD notes_url_expanded TEXT  default '';
ALTER TABLE icinga_hosts ADD action_url_expanded TEXT  default '';
ALTER TABLE icinga_hosts ADD icon_image_expanded TEXT  default '';

ALTER TABLE icinga_services ADD notes_expanded TEXT  default '';
ALTER TABLE icinga_services ADD notes_url_expanded TEXT  default '';
ALTER TABLE icinga_services ADD action_url_expanded TEXT  default '';
ALTER TABLE icinga_services ADD icon_image_expanded TEXT  default '';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.10.0');

