-- -----------------------------------------
-- upgrade path for Icinga IDOUtils 1.8.0
--
-- -----------------------------------------
-- Copyright (c) 2012 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #905
-- -----------------------------------------
alter table icinga_programstatus add disable_notif_expire_time timestamp with time zone default '1970-01-01 00:00:00';

-- -----------------------------------------
-- #2618
-- -----------------------------------------

CREATE INDEX cntgrpmbrs_cgid_coid ON icinga_contactgroup_members (contactgroup_id,contact_object_id);
CREATE INDEX hstgrpmbrs_hgid_hoid ON icinga_hostgroup_members (hostgroup_id,host_object_id);
CREATE INDEX hstcntgrps_hid_cgoid ON icinga_host_contactgroups (host_id,contactgroup_object_id);
CREATE INDEX hstprnthsts_hid_phoid ON icinga_host_parenthosts (host_id,parent_host_object_id);
CREATE INDEX runtimevars_iid_varn ON icinga_runtimevariables (instance_id,varname);
CREATE INDEX sgmbrs_sgid_soid ON icinga_servicegroup_members (servicegroup_id,service_object_id);
CREATE INDEX scgrps_sid_cgoid ON icinga_service_contactgroups (service_id,contactgroup_object_id);
CREATE INDEX tperiod_tid_d_ss_es ON icinga_timeperiod_timeranges (timeperiod_id,day,start_sec,end_sec);

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.8.0');

