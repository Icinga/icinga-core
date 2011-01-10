-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.3.0
--
-- add index for statehistory
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------


-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

ALTER TABLE dbversion ADD id number(11);
ALTER TABLE dbversion ADD CONSTRAINT PK_DBVERSION PRIMARY KEY (id);


MERGE INTO dbversion
USING DUAL ON (name='idoutils')
WHEN MATCHED THEN
UPDATE SET version='1.3.0'
WHEN NOT MATCHED THEN
INSERT (name, version) VALUES ('idoutils', '1.3.0');

-- -----------------------------------------
-- add index for statehistory
-- -----------------------------------------

CREATE INDEX statehist_i_id_o_id_s_ty_s_ti on statehistory(instance_id, object_id, state_type, state_time);

