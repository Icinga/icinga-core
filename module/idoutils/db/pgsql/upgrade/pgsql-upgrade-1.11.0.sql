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
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.0');

