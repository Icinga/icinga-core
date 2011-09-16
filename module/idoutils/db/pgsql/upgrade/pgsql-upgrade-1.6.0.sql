-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.6.0
--
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- add end_time for acknowledgements
-- -----------------------------------------

ALTER TABLE icinga_acknowledgements ADD end_time timestamp default '1970-01-01 00:00:00';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.5.0');

