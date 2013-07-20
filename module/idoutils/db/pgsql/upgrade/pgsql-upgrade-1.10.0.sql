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
-- #4420 "integer" out-of-range
-- -----------------------------------------

CREATE OR REPLACE FUNCTION from_unixtime(bigint) RETURNS timestamp with time zone AS '
	SELECT to_timestamp($1) AS result
' LANGUAGE sql;

ALTER TABLE icinga_downtimehistory ALTER COLUMN duration TYPE BIGINT;
ALTER TABLE icinga_scheduleddowntime ALTER COLUMN duration TYPE BIGINT;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.10.0');

