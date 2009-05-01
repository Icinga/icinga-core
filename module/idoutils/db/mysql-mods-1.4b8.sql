; 05/03/08
ALTER TABLE `icinga_hosts` ADD INDEX ( `host_object_id` );

ALTER TABLE `icinga_hoststatus` ADD INDEX ( `instance_id` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `status_update_time` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `current_state` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `check_type` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `state_type` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `last_state_change` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `notifications_enabled` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `problem_has_been_acknowledged` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `active_checks_enabled` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `passive_checks_enabled` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `event_handler_enabled` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `flap_detection_enabled` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `is_flapping` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `percent_state_change` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `latency` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `execution_time` );
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `scheduled_downtime_depth` );

ALTER TABLE `icinga_services` ADD INDEX ( `service_object_id` );

ALTER TABLE `icinga_servicestatus` ADD INDEX ( `instance_id` ); 
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `status_update_time` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `current_state` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `check_type` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `state_type` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `last_state_change` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `notifications_enabled` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `problem_has_been_acknowledged` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `active_checks_enabled` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `passive_checks_enabled` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `event_handler_enabled` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `flap_detection_enabled` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `is_flapping` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `percent_state_change` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `latency` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `execution_time` );
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `scheduled_downtime_depth` );


ALTER TABLE `icinga_timedeventqueue` ADD INDEX ( `instance_id` );
ALTER TABLE `icinga_timedeventqueue` ADD INDEX ( `event_type` );
ALTER TABLE `icinga_timedeventqueue` ADD INDEX ( `scheduled_time` );
ALTER TABLE `icinga_timedeventqueue` ADD INDEX ( `object_id` );

ALTER TABLE `icinga_timedevents` DROP INDEX `instance_id` ;
ALTER TABLE `icinga_timedevents` ADD INDEX ( `instance_id` );
ALTER TABLE `icinga_timedevents` ADD INDEX ( `event_type` );
ALTER TABLE `icinga_timedevents` ADD INDEX ( `scheduled_time` );
ALTER TABLE `icinga_timedevents` ADD INDEX ( `object_id` );

ALTER TABLE `icinga_systemcommands` DROP INDEX `instance_id`;  
ALTER TABLE `icinga_systemcommands` ADD INDEX ( `instance_id` );

ALTER TABLE `icinga_servicechecks` DROP INDEX `instance_id`;
ALTER TABLE `icinga_servicechecks` ADD INDEX ( `instance_id` );
ALTER TABLE `icinga_servicechecks` ADD INDEX ( `service_object_id` );
ALTER TABLE `icinga_servicechecks` ADD INDEX ( `start_time` );

ALTER TABLE `icinga_configfilevariables` DROP INDEX `instance_id`;


