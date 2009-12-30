-- -----------------------------------------
-- pgsql-upgrade-1.0.1.sql
-- upgrade path for Icinga IDOUtils 1.0.1
-- fixes output missing in table systemcommands
-- -----------------------------------------
-- Copyright (c) 2009 Icinga Development Team (http://www.icinga.org)
--
-- Initial Revision: 2009-12-30 Michael Friedrich <michael.friedrich(at)univie.ac.at>
-- 
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- systemcommands upgrade path
-- -----------------------------------------

ALTER TABLE icinga_systemcommands ADD output VARCHAR(255) NOT NULL default '';
