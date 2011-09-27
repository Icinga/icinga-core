-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.6.0
--
-- -----------------------------------------
-- Copyright (c) 2010-2011 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- add end_time for acknowledgements
-- -----------------------------------------

ALTER TABLE icinga_acknowledgements ADD end_time timestamp default '1970-01-01 00:00:00';


-- --------------------------------------------------------

--
-- Table structure for table icinga_slahistory
--
                                                                                
CREATE TABLE icinga_slahistory (
  slahistory_id serial,
  instance_id bigint default 0,
  start_time timestamp default '1970-01-01 00:00:00',
  end_time timestamp default '1970-01-01 00:00:00',
  acknowledgement_time timestamp default '1970-01-01 00:00:00',
  object_id bigint default 0,
  state INTEGER default 0,
  state_type INTEGER default '0',
  scheduled_downtime INTEGER default 0,
  CONSTRAINT PK_slahistory_id PRIMARY KEY (slahistory_id)
) ;

-- SLA statehistory
CREATE INDEX slahist_i_id_o_id_s_ti_s_s_ti_e on icinga_slahistory(instance_id,object_id,start_time,end_time);


-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.5.0');

