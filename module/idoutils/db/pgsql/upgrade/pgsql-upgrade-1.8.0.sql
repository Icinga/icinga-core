-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.8.0
--
-- -----------------------------------------
-- Copyright (c) 2012 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #905
-- -----------------------------------------
alter table icinga_programstatus add disable_notif_expire_time timestamp with time zone default '1970-01-01 00:00:00';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.8.0');

