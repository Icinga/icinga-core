-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.10.0
--
-- -----------------------------------------
-- Copyright (c) 2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #4363 deprecate enable_sla
-- -----------------------------------------

-- DROP TABLE icinga_slahistory;

-- -----------------------------------------
-- #4482 deprecate timedevent* tables
-- -----------------------------------------

-- drop index too, if seperate tbs
ALTER TABLE icinga_timedevents DROP INDEX timedevents_i_id_idx;
ALTER TABLE icinga_timedevents DROP INDEX timedevents_time_id_idx;
ALTER TABLE icinga_timedevents DROP INDEX timed_e_event_type_idx;
ALTER TABLE icinga_timedevents DROP INDEX timed_e_object_id_idx;
ALTER TABLE icinga_timedevents DROP INDEX timed_e_rec_ev_idx;

ALTER TABLE icinga_timedeventqueue DROP INDEX timedeventq_i_id_idx;
ALTER TABLE icinga_timedeventqueue DROP INDEX timedeventq_time_id_idx;
ALTER TABLE icinga_timedeventqueue DROP INDEX timedeventqueue_i_id_idx;
ALTER TABLE icinga_timedeventqueue DROP INDEX timed_e_q_event_type_idx;
ALTER TABLE icinga_timedeventqueue DROP INDEX timed_e_q_sched_time_idx;
ALTER TABLE icinga_timedeventqueue DROP INDEX timed_e_q_object_id_idx;
ALTER TABLE icinga_timedeventqueue DROP INDEX timed_e_q_rec_ev_id_idx;

DROP TABLE icinga_timedevents;
DROP TABLE icinga_timedeventqueue;

-- -----------------------------------------
-- #4544 icinga_comments table UK
-- -----------------------------------------

ALTER TABLE icinga_comments DROP INDEX instance_id;
ALTER TABLE icinga_commenthistory DROP INDEX instance_id;

CREATE UNIQUE INDEX instance_id ON icinga_comments(instance_id,object_id,comment_time,internal_comment_id);
CREATE UNIQUE INDEX instance_id ON icinga_commenthistory(instance_id,object_id,comment_time,internal_comment_id);

-- -----------------------------------------
-- #4709 add check source
-- -----------------------------------------

ALTER TABLE icinga_hoststatus ADD check_source TEXT character set latin1  default ''; 
ALTER TABLE icinga_servicestatus ADD check_source TEXT character set latin1  default ''; 

-- -----------------------------------------
-- #4754 add logentries object_id
-- -----------------------------------------

ALTER TABLE icinga_logentries ADD object_id bigint unsigned default NULL;

-- -----------------------------------------
-- #4939 duration column too small
-- -----------------------------------------

ALTER TABLE icinga_downtimehistory MODIFY duration BIGINT(20);
ALTER TABLE icinga_scheduleddowntime MODIFY duration BIGINT(20);

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.10.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.10.0', modify_time=NOW();

