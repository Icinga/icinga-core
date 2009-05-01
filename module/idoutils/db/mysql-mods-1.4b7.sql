-- BEGIN 1.4b7 MODS 

ALTER TABLE `icinga_configfilevariables` DROP INDEX `instance_id`; 
ALTER TABLE `icinga_configfilevariables` ADD INDEX ( `instance_id` , `configfile_id` ) ;

ALTER TABLE `icinga_timedeventqueue` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `icinga_statehistory` ADD INDEX ( `instance_id` , `object_id` ) ;

ALTER TABLE `icinga_servicestatus` ADD INDEX ( `instance_id` , `service_object_id` ) ;

ALTER TABLE `icinga_processevents` ADD INDEX ( `instance_id` , `event_type` ) ;

ALTER TABLE `icinga_logentries` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `icinga_hoststatus` ADD INDEX ( `instance_id` , `host_object_id` ) ;

ALTER TABLE `icinga_flappinghistory` ADD INDEX ( `instance_id` , `object_id` ) ;

ALTER TABLE `icinga_externalcommands` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `icinga_customvariablestatus` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `icinga_contactstatus` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `icinga_conninfo` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `icinga_acknowledgements` ADD INDEX ( `instance_id` , `object_id` ) ;

ALTER TABLE `icinga_objects` ADD INDEX ( `instance_id` ) ;

ALTER TABLE `icinga_logentries` ADD INDEX ( `logentry_time` ) ;

ALTER TABLE `icinga_commenthistory` CHANGE `comment_data` `comment_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_comments` CHANGE `comment_data` `comment_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_downtimehistory` CHANGE `comment_data` `comment_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_externalcommands` CHANGE `command_args` `command_args` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_hostchecks` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;
ALTER TABLE `icinga_hostchecks` CHANGE `perfdata` `perfdata` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_hoststatus` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;
ALTER TABLE `icinga_hoststatus` CHANGE `perfdata` `perfdata` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_logentries` CHANGE `logentry_data` `logentry_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_scheduleddowntime` CHANGE `comment_data` `comment_data` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_servicechecks` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;
ALTER TABLE `icinga_servicechecks` CHANGE `perfdata` `perfdata` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_servicestatus` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;
ALTER TABLE `icinga_servicestatus` CHANGE `perfdata` `perfdata` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_statehistory` CHANGE `output` `output` TEXT CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL ;

ALTER TABLE `icinga_processevents` ADD INDEX ( `event_time` , `event_time_usec` ) ;

ALTER TABLE `icinga_hoststatus` ADD INDEX ( `current_state` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `state_type` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `last_check` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `notifications_enabled` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `problem_has_been_acknowledged` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `passive_checks_enabled` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `active_checks_enabled` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `event_handler_enabled` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `flap_detection_enabled` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `is_flapping` ) ;
ALTER TABLE `icinga_hoststatus` ADD INDEX ( `scheduled_downtime_depth` ) ;

ALTER TABLE `icinga_servicestatus` ADD INDEX ( `current_state` ) ;
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `last_check` ) ;
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `notifications_enabled` ) ;
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `problem_has_been_acknowledged` ) ;
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `passive_checks_enabled` ) ;
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `active_checks_enabled` ) ;
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `event_handler_enabled` ) ;
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `flap_detection_enabled` ) ;
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `is_flapping` ) ;
ALTER TABLE `icinga_servicestatus` ADD INDEX ( `scheduled_downtime_depth` ) ;

ALTER TABLE `icinga_statehistory` ADD INDEX ( `state_time` , `state_time_usec` ) ;

ALTER TABLE `icinga_timedeventqueue` ADD INDEX ( `event_type` ) ;
ALTER TABLE `icinga_timedeventqueue` ADD INDEX ( `scheduled_time` ) ;

ALTER TABLE `icinga_logentries` ADD INDEX ( `entry_time` ) ;
ALTER TABLE `icinga_logentries` ADD INDEX ( `entry_time_usec` ) ;

ALTER TABLE `icinga_externalcommands` ADD INDEX ( `entry_time` ) ;

-- END 1.4b7 MODS 
