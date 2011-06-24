-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.5.0
--
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.5.0');  

