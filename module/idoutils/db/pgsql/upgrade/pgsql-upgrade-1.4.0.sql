-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.4.0
--
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.4.0');

-- -----------------------------------------
-- change integer to bigint
-- -----------------------------------------

ALTER TABLE icinga_conninfo ALTER COLUMN bytes_processed TYPE BIGINT;
ALTER TABLE icinga_conninfo ALTER COLUMN lines_processed TYPE BIGINT;

