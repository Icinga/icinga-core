-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.7.0
--
-- -----------------------------------------
-- Copyright (c) 2012 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #2274
-- -----------------------------------------
create index statehist_state_idx on icinga_statehistory(object_id,state);

-- -----------------------------------------
-- #2479
-- -----------------------------------------
alter table icinga_hosts modify FAILURE_PREDICTION_OPTIONS varchar(128) ;

-- -----------------------------------------
-- #2537
-- -----------------------------------------

alter table icinga_downtimehistory add is_in_effect smallint default 0;
alter table icinga_downtimehistory add trigger_time timestamp  default '0000-00-00 00:00:00';
alter table icinga_scheduleddowntime add is_in_effect smallint default 0;
alter table icinga_scheduleddowntime add trigger_time timestamp  default '0000-00-00 00:00:00';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version) VALUES ('idoutils', '1.7.0') ON DUPLICATE KEY UPDATE version='1.7.0';

