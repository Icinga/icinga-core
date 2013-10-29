-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.9.0
--
-- -----------------------------------------
-- Copyright (c) 2012-2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #4363 deprecate enable_sla
-- -----------------------------------------

-- DROP TABLE icinga_slahistory;

-- -----------------------------------------
-- #4420 "integer" out-of-range
-- -----------------------------------------

CREATE OR REPLACE FUNCTION from_unixtime(bigint) RETURNS timestamp with time zone AS '
	SELECT to_timestamp($1) AS result
' LANGUAGE sql;

ALTER TABLE icinga_downtimehistory ALTER COLUMN duration TYPE BIGINT;
ALTER TABLE icinga_scheduleddowntime ALTER COLUMN duration TYPE BIGINT;

-- -----------------------------------------
-- #4482 deprecate timedevent* tables
-- -----------------------------------------

-- drop index too, if seperate tbs
DROP INDEX IF EXISTS timedevents_i_id_idx;
DROP INDEX IF EXISTS timedevents_time_id_idx;
DROP INDEX IF EXISTS timed_e_event_type_idx;
DROP INDEX IF EXISTS timed_e_object_id_idx;
DROP INDEX IF EXISTS timed_e_rec_ev_idx;

DROP INDEX IF EXISTS timedeventq_i_id_idx;
DROP INDEX IF EXISTS timedeventq_time_id_idx;
DROP INDEX IF EXISTS timedeventqueue_i_id_idx;
DROP INDEX IF EXISTS timed_e_q_event_type_idx;
DROP INDEX IF EXISTS timed_e_q_sched_time_idx;
DROP INDEX IF EXISTS timed_e_q_object_id_idx;
DROP INDEX IF EXISTS timed_e_q_rec_ev_id_idx;

DROP TABLE IF EXISTS icinga_timedevents;
DROP TABLE IF EXISTS icinga_timedeventqueue;

-- -----------------------------------------
-- #4544 icinga_comments table UK
-- -----------------------------------------

ALTER TABLE icinga_comments DROP CONSTRAINT IF EXISTS uq_comments;
ALTER TABLE icinga_commenthistory DROP CONSTRAINT IF EXISTS uq_commenthistory;

ALTER TABLE icinga_comments ADD CONSTRAINT uq_comments UNIQUE (instance_id,object_id,comment_time,internal_comment_id);
ALTER TABLE icinga_commenthistory ADD CONSTRAINT uq_commenthistory UNIQUE (instance_id,object_id,comment_time,internal_comment_id);

-- -----------------------------------------
-- #4709 add check source
-- -----------------------------------------

ALTER TABLE icinga_hoststatus ADD check_source TEXT default '';
ALTER TABLE icinga_servicestatus ADD check_source TEXT default '';

-- -----------------------------------------
-- #4754 add logentries object_id
-- -----------------------------------------

ALTER TABLE icinga_logentries ADD object_id bigint default NULL;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.10.0');

