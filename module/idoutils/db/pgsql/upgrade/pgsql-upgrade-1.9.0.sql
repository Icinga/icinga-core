-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.9.0
--
-- -----------------------------------------
-- Copyright (c) 2012-2013 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #3649 sla index
-- -----------------------------------------

CREATE INDEX sla_idx_sthist ON icinga_statehistory (object_id, state_time DESC);
CREATE INDEX sla_idx_dohist ON icinga_downtimehistory (object_id, actual_start_time, actual_end_time);
CREATE INDEX sla_idx_obj ON icinga_objects (objecttype_id, is_active, name1);

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.9.0');

