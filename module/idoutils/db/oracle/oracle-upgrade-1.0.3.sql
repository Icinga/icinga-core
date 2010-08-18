-- --------------------------------------------------------
-- oracle-upgrade-1.0.3.sql
-- DB definition for Oracle
--
-- set display_name to varchar2(255)
--
-- copy to $ORACLE_HOME
-- # sqlplus username/password
-- SQL> @oracle-upgrade-1.0.3.sql
--
-- current version: 2010-07-27 Michael Friedrich <michael.friedrich(at)univie.ac.at>
--
-- --------------------------------------------------------

-- --------------------------------------------------------
-- display_name upgrades
-- --------------------------------------------------------

ALTER TABLE services MODIFY display_name varchar2(255);
ALTER TABLE configfilevariables MODIFY varvalue varchar2(1024);
