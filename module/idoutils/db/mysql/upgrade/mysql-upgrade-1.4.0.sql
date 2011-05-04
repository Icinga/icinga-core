-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.4.0
--
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------


-- --------------------------------------------------------
-- remove number limitations, change id autoincrement type to native serial type (bigint unsigned not null)
-- fixes Bug #1173: int(11) to small for some of ido tables https://dev.icinga.org/issues/1173
--       Bug #1401: integer not big enough for bytes_processed https://dev.icinga.org/issues/1401
-- remove NOT NULL constraints
-- Feature #1355 drop unnessary constraints and rename remaining https://dev.icinga.org/issues/1355
-- --------------------------------------------------------
\. mysql_alter_icinga13_numbers.sql

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version) VALUES ('idoutils', '1.4.0') ON DUPLICATE KEY UPDATE version='1.4.0';

