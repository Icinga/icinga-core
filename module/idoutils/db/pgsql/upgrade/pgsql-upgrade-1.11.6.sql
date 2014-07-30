-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.11.6
--
-- -----------------------------------------
-- Copyright (c) 2014 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- fixes only affect MySQL, schema version update required

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.6');

