-- --------------------------------------------------------
-- oracle-upgrade-1.0.1.sql
-- DB definition for Oracle
-- 
-- set command_line to varchar2(1024)
-- sets index on several tables for improved delete
-- removes triggers and adds squences instead
--
-- requires ocilib, oracle (instantclient) libs+sdk  to work
-- specify oracle (instantclient) libs+sdk in ocilib configure
-- ./configure \
--      --with-oracle-headers-path=/opt/oracle/product/instantclient/instantclient_11_1/sdk/include \
--      --with-oracle-lib-path=/opt/oracle/product/instantclient/instantclient_11_1/
--
-- enable ocilib in Icinga with
-- ./configure --enable-idoutils --enable--oracle
-- 
-- copy to $ORACLE_HOME 
-- # sqlplus username/password
-- SQL> @oracle-upgrade-1.0.1.sql
--
-- current version: 2010-02-03 Michael Friedrich <michael.friedrich(at)univie.ac.at>
--
-- -- --------------------------------------------------------

--
-- command_line
--

ALTER TABLE hostchecks MODIFY command_line varchar2(1024);
ALTER TABLE servicechecks MODIFY command_line varchar2(1024);
ALTER TABLE systemcommands MODIFY command_line varchar2(1024);
ALTER TABLE eventhandlers MODIFY command_line varchar2(1024);


-- -----------------------------------------
-- add index (delete)
-- -----------------------------------------

-- for periodic delete 
-- instance_id and
-- TIMEDEVENTS => scheduled_time
-- SYSTEMCOMMANDS, SERVICECHECKS, HOSTCHECKS, EVENTHANDLERS  => start_time
-- EXTERNALCOMMANDS => entry_time

-- instance_id
CREATE INDEX timedevents_i_id_idx on timedevents(instance_id);
CREATE INDEX timedeventq_i_id_idx on timedeventqueue(instance_id);
CREATE INDEX systemcommands_i_id_idx on systemcommands(instance_id);
CREATE INDEX servicechecks_i_id_idx on servicechecks(instance_id);
CREATE INDEX hostchecks_i_id_idx on hostchecks(instance_id);
CREATE INDEX eventhandlers_i_id_idx on eventhandlers(instance_id);
CREATE INDEX externalcommands_i_id_idx on externalcommands(instance_id);

-- time
CREATE INDEX timedevents_time_id_idx on timedevents(scheduled_time);
CREATE INDEX timedeventq_time_id_idx on timedeventqueue(scheduled_time);
CREATE INDEX systemcommands_time_id_idx on systemcommands(start_time);
CREATE INDEX servicechecks_time_id_idx on servicechecks(start_time);
CREATE INDEX hostchecks_time_id_idx on hostchecks(start_time);
CREATE INDEX eventhandlers_time_id_idx on eventhandlers(start_time);
CREATE INDEX externalcommands_time_id_idx on externalcommands(entry_time);


-- for starting cleanup - referenced in dbhandler.c:882
-- instance_id only

-- realtime data
-- CREATE INDEX programstatus_i_id_idx on programstatus(instance_id); -- unique constraint
CREATE INDEX hoststatus_i_id_idx on hoststatus(instance_id);
CREATE INDEX servicestatus_i_id_idx on servicestatus(instance_id);
CREATE INDEX contactstatus_i_id_idx on contactstatus(instance_id);
-- CREATE INDEX timedeventqueue_i_id_idx on timedeventqueue(instance_id); -- defined above
CREATE INDEX comments_i_id_idx on comments(instance_id);
CREATE INDEX scheduleddowntime_i_id_idx on scheduleddowntime(instance_id);
CREATE INDEX runtimevariables_i_id_idx on runtimevariables(instance_id);
CREATE INDEX customvariablestatus_i_id_idx on customvariablestatus(instance_id);

-- config data
CREATE INDEX configfiles_i_id_idx on configfiles(instance_id);
CREATE INDEX configfilevariables_i_id_idx on configfilevariables(instance_id);
CREATE INDEX customvariables_i_id_idx on customvariables(instance_id);
CREATE INDEX commands_i_id_idx on commands(instance_id);
CREATE INDEX timeperiods_i_id_idx on timeperiods(instance_id);
CREATE INDEX timeperiod_timeranges_i_id_idx on timeperiod_timeranges(instance_id);
CREATE INDEX contactgroups_i_id_idx on contactgroups(instance_id);
CREATE INDEX contactgroup_members_i_id_idx on contactgroup_members(instance_id);
CREATE INDEX hostgroups_i_id_idx on hostgroups(instance_id);
CREATE INDEX hostgroup_members_i_id_idx on hostgroup_members(instance_id);
CREATE INDEX servicegroups_i_id_idx on servicegroups(instance_id);
CREATE INDEX servicegroup_members_i_id_idx on servicegroup_members(instance_id);
CREATE INDEX hostesc_i_id_idx on hostescalations(instance_id);
CREATE INDEX hostesc_contacts_i_id_idx on hostescalation_contacts(instance_id);
CREATE INDEX serviceesc_i_id_idx on serviceescalations(instance_id);
CREATE INDEX serviceesc_contacts_i_id_idx on serviceescalation_contacts(instance_id);
CREATE INDEX hostdependencies_i_id_idx on hostdependencies(instance_id);
CREATE INDEX contacts_i_id_idx on contacts(instance_id);
CREATE INDEX contact_addresses_i_id_idx on contact_addresses(instance_id);
CREATE INDEX contact_notifcommands_i_id_idx on contact_notificationcommands(instance_id);
CREATE INDEX hosts_i_id_idx on hosts(instance_id);
CREATE INDEX host_parenthosts_i_id_idx on host_parenthosts(instance_id);
CREATE INDEX host_contacts_i_id_idx on host_contacts(instance_id);
CREATE INDEX services_i_id_idx on services(instance_id);
CREATE INDEX service_contacts_i_id_idx on service_contacts(instance_id);
CREATE INDEX service_contactgroups_i_id_idx on service_contactgroups(instance_id);
CREATE INDEX host_contactgroups_i_id_idx on host_contactgroups(instance_id);
CREATE INDEX hostesc_cgroups_i_id_idx on hostescalation_contactgroups(instance_id);
CREATE INDEX serviceesc_cgroups_i_id_idx on serviceescalationcontactgroups(instance_id);


-- -----------------------------------------
-- more index stuff (WHERE clauses)
-- -----------------------------------------

-- hosts
CREATE INDEX hosts_host_object_id_idx on hosts(host_object_id);

-- hoststatus
CREATE INDEX hoststatus_stat_upd_time_idx on hoststatus(status_update_time);
CREATE INDEX hoststatus_current_state_idx on hoststatus(current_state);
CREATE INDEX hoststatus_check_type_idx on hoststatus(check_type);
CREATE INDEX hoststatus_state_type_idx on hoststatus(state_type);
CREATE INDEX hoststatus_last_state_chg_idx on hoststatus(last_state_change);
CREATE INDEX hoststatus_notif_enabled_idx on hoststatus(notifications_enabled);
CREATE INDEX hoststatus_problem_ack_idx on hoststatus(problem_has_been_acknowledged);
CREATE INDEX hoststatus_act_chks_en_idx on hoststatus(active_checks_enabled);
CREATE INDEX hoststatus_pas_chks_en_idx on hoststatus(passive_checks_enabled);
CREATE INDEX hoststatus_event_hdl_en_idx on hoststatus(event_handler_enabled);
CREATE INDEX hoststatus_flap_det_en_idx on hoststatus(flap_detection_enabled);
CREATE INDEX hoststatus_is_flapping_idx on hoststatus(is_flapping);
CREATE INDEX hoststatus_p_state_chg_idx on hoststatus(percent_state_change);
CREATE INDEX hoststatus_latency_idx on hoststatus(latency);
CREATE INDEX hoststatus_ex_time_idx on hoststatus(execution_time);
CREATE INDEX hoststatus_sch_downt_d_idx on hoststatus(scheduled_downtime_depth);

-- services
CREATE INDEX services_host_object_id_idx on services(host_object_id);

--servicestatus
CREATE INDEX srvcstatus_stat_upd_time_idx on servicestatus(status_update_time);
CREATE INDEX srvcstatus_current_state_idx on servicestatus(current_state);
CREATE INDEX srvcstatus_check_type_idx on servicestatus(check_type);
CREATE INDEX srvcstatus_state_type_idx on servicestatus(state_type);
CREATE INDEX srvcstatus_last_state_chg_idx on servicestatus(last_state_change);
CREATE INDEX srvcstatus_notif_enabled_idx on servicestatus(notifications_enabled);
CREATE INDEX srvcstatus_problem_ack_idx on servicestatus(problem_has_been_acknowledged);
CREATE INDEX srvcstatus_act_chks_en_idx on servicestatus(active_checks_enabled);
CREATE INDEX srvcstatus_pas_chks_en_idx on servicestatus(passive_checks_enabled);
CREATE INDEX srvcstatus_event_hdl_en_idx on servicestatus(event_handler_enabled);
CREATE INDEX srvcstatus_flap_det_en_idx on servicestatus(flap_detection_enabled);
CREATE INDEX srvcstatus_is_flapping_idx on servicestatus(is_flapping);
CREATE INDEX srvcstatus_p_state_chg_idx on servicestatus(percent_state_change);
CREATE INDEX srvcstatus_latency_idx on servicestatus(latency);
CREATE INDEX srvcstatus_ex_time_idx on servicestatus(execution_time);
CREATE INDEX srvcstatus_sch_downt_d_idx on servicestatus(scheduled_downtime_depth);

-- timedeventqueue
CREATE INDEX timed_e_q_event_type_idx on timedeventqueue(event_type);
-- CREATE INDEX timed_e_q_sched_time_idx on timedeventqueue(scheduled_time); -- defined above
CREATE INDEX timed_e_q_object_id_idx on timedeventqueue(object_id);
CREATE INDEX timed_e_q_rec_ev_id_idx on timedeventqueue(recurring_event);

-- timedevents
CREATE INDEX timed_e_event_type_idx on timedevents(event_type);
--CREATE INDEX timed_e_sched_time_idx on timedevents(scheduled_time); -- already set for delete
CREATE INDEX timed_e_object_id_idx on timedevents(object_id);
CREATE INDEX timed_e_rec_ev_idx on timedevents(recurring_event);

-- hostchecks
CREATE INDEX hostchks_h_obj_id_idx on hostchecks(host_object_id);

-- servicechecks
CREATE INDEX servicechks_s_obj_id_idx on servicechecks(service_object_id);

-- objects
CREATE INDEX objects_objtype_id_idx ON objects(objecttype_id);
CREATE INDEX objects_name1_idx ON objects(name1);
CREATE INDEX objects_name2_idx ON objects(name2);
CREATE INDEX objects_inst_id_idx ON objects(instance_id);


-- hostchecks
-- CREATE INDEX hostchks_h_obj_id_idx on hostchecks(host_object_id);

-- servicechecks
-- CREATE INDEX servicechks_s_obj_id_idx on servicechecks(service_object_id);


-- instances
-- CREATE INDEX instances_name_idx on instances(instance_name);

-- logentries
-- CREATE INDEX loge_instance_id_idx on logentries(instance_id);
-- CREATE INDEX loge_time_idx on logentries(logentry_time);
-- CREATE INDEX loge_data_idx on logentries(logentry_data);

-- commenthistory
-- CREATE INDEX c_hist_instance_id_idx on logentries(instance_id);
-- CREATE INDEX c_hist_c_time_idx on logentries(comment_time);
-- CREATE INDEX c_hist_i_c_id_idx on logentries(internal_comment_id);

-- downtimehistory
-- CREATE INDEX d_t_hist_nstance_id_idx on downtimehistory(instance_id);
-- CREATE INDEX d_t_hist_type_idx on downtimehistory(downtime_type);
-- CREATE INDEX d_t_hist_object_id_idx on downtimehistory(object_id);
-- CREATE INDEX d_t_hist_entry_time_idx on downtimehistory(entry_time);
-- CREATE INDEX d_t_hist_sched_start_idx on downtimehistory(scheduled_start_time);
-- CREATE INDEX d_t_hist_sched_end_idx on downtimehistory(scheduled_end_time);

-- scheduleddowntime
-- CREATE INDEX sched_d_t_downtime_type_idx on scheduleddowntime(downtime_type);
-- CREATE INDEX sched_d_t_object_id_idx on scheduleddowntime(object_id);
-- CREATE INDEX sched_d_t_entry_time_idx on scheduleddowntime(entry_time);
-- CREATE INDEX sched_d_t_start_time_idx on scheduleddowntime(scheduled_start_time);
-- CREATE INDEX sched_d_t_end_time_idx on scheduleddowntime(scheduled_end_time);


-- -----------------------------------------
-- triggers/sequences
-- -----------------------------------------


