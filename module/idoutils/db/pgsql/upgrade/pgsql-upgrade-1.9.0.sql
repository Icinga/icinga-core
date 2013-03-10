-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.9.0
--
-- -----------------------------------------
-- Copyright (c) 2012-2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #3810 executed_command
-- -----------------------------------------

ALTER TABLE icinga_hoststatus ADD executed_command TEXT;
ALTER TABLE icinga_servicestatus ADD executed_command TEXT;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.9.0');

