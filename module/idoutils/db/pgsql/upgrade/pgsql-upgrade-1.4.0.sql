-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.4.0
--
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- remove number limitations, change serial to bigserial type
-- fixes Bug #1173: int(11) to small for some of ido tables https://dev.icinga.org/issues/1173
--       Bug #1401: integer not big enough for bytes_processed https://dev.icinga.org/issues/1401
-- remove NOT NULL constraints
-- Feature #1355 drop unnessary constraints and rename remaining https://dev.icinga.org/issues/1355
-- -----------------------------------------
\i pgsql_alter_icinga13_types.sql

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.4.0');


