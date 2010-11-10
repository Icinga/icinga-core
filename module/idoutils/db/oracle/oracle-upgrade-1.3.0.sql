-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.0.3
--
-- add index for statehistory
-- -----------------------------------------
-- Copyright (c) 2010 Icinga Development Team (http://www.icinga.org)
--
-- Initial Revision: 2010-11-04 Michael Friedrich <michael.friedrich(at)univie.ac.at>
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- add index for statehistory
-- -----------------------------------------

CREATE INDEX statehist_i_id_o_id_s_ty_s_ti on statehistory(instance_id, object_id, state_type, state_time);

