-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.7.0
--
-- -----------------------------------------
-- Copyright (c) 2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
--#2274
-- -----------------------------------------
create index statehist_state_idx on icinga_statehistory(object_id,state);

-- -----------------------------------------
--#2479
-- -----------------------------------------
alter table icinga_hosts modify FAILURE_PREDICTION_OPTIONS varchar(128) ;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version) VALUES ('idoutils', '1.7.0') ON DUPLICATE KEY UPDATE version='1.7.0';

