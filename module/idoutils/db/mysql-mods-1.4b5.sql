-- BEGIN 1.4b5 MODS 

--                                                   
-- Table structure for table `icinga_host_contactgroups`
--                                                   

CREATE TABLE IF NOT EXISTS `icinga_host_contactgroups` (
  `host_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `host_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`host_contactgroup_id`),
  UNIQUE KEY `instance_id` (`host_id`,`contactgroup_object_id`)
) ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Host contact groups';

-- --------------------------------------------------------

--                                                   
-- Table structure for table `icinga_hostescalation_contactgroups`
--                                                   

CREATE TABLE IF NOT EXISTS `icinga_hostescalation_contactgroups` (
  `hostescalation_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `hostescalation_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`hostescalation_contactgroup_id`),
  UNIQUE KEY `instance_id` (`hostescalation_id`,`contactgroup_object_id`)
) ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Host escalation contact groups';

-- --------------------------------------------------------

--                                                   
-- Table structure for table `icinga_service_contactgroups`
--                                                   

CREATE TABLE IF NOT EXISTS `icinga_service_contactgroups` (
  `service_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `service_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`service_contactgroup_id`),
  UNIQUE KEY `instance_id` (`service_id`,`contactgroup_object_id`)
) ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Service contact groups';

-- --------------------------------------------------------

--                                                   
-- Table structure for table `icinga_serviceescalation_contactgroups`
--                                                   

CREATE TABLE IF NOT EXISTS `icinga_serviceescalation_contactgroups` (
  `serviceescalation_contactgroup_id` int(11) NOT NULL auto_increment,
  `instance_id` smallint(6) NOT NULL default '0',
  `serviceescalation_id` int(11) NOT NULL default '0',
  `contactgroup_object_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`serviceescalation_contactgroup_id`),
  UNIQUE KEY `instance_id` (`serviceescalation_id`,`contactgroup_object_id`)
) ENGINE=MyISAM DEFAULT CHARSET=ascii COMMENT='Service escalation contact groups';

-- --------------------------------------------------------

ALTER TABLE `icinga_acknowledgements` TYPE = innodb;
ALTER TABLE `icinga_commands` TYPE = innodb;
ALTER TABLE `icinga_commenthistory` TYPE = innodb;
ALTER TABLE `icinga_comments` TYPE = innodb;
ALTER TABLE `icinga_configfiles` TYPE = innodb;
ALTER TABLE `icinga_configfilevariables` TYPE = innodb;
ALTER TABLE `icinga_conninfo` TYPE = innodb;
ALTER TABLE `icinga_contact_addresses` TYPE = innodb;
ALTER TABLE `icinga_contact_notificationcommands` TYPE = innodb;
ALTER TABLE `icinga_contactgroup_members` TYPE = innodb;
ALTER TABLE `icinga_contactgroups` TYPE = innodb;
ALTER TABLE `icinga_contactnotificationmethods` TYPE = innodb;
ALTER TABLE `icinga_contactnotifications` TYPE = innodb;
ALTER TABLE `icinga_contacts` TYPE = innodb;
ALTER TABLE `icinga_contactstatus` TYPE = innodb;
ALTER TABLE `icinga_customvariables` TYPE = innodb;
ALTER TABLE `icinga_customvariablestatus` TYPE = innodb;
ALTER TABLE `icinga_dbversion` TYPE = innodb;
ALTER TABLE `icinga_downtimehistory` TYPE = innodb;
ALTER TABLE `icinga_eventhandlers` TYPE = innodb;
ALTER TABLE `icinga_externalcommands` TYPE = innodb;
ALTER TABLE `icinga_flappinghistory` TYPE = innodb;
ALTER TABLE `icinga_host_contactgroups` TYPE = innodb;
ALTER TABLE `icinga_host_contacts` TYPE = innodb;
ALTER TABLE `icinga_host_parenthosts` TYPE = innodb;
ALTER TABLE `icinga_hostchecks` TYPE = innodb;
ALTER TABLE `icinga_hostdependencies` TYPE = innodb;
ALTER TABLE `icinga_hostescalation_contactgroups` TYPE = innodb;
ALTER TABLE `icinga_hostescalation_contacts` TYPE = innodb;
ALTER TABLE `icinga_hostescalations` TYPE = innodb;
ALTER TABLE `icinga_hostgroup_members` TYPE = innodb;
ALTER TABLE `icinga_hostgroups` TYPE = innodb;
ALTER TABLE `icinga_hosts` TYPE = innodb;
ALTER TABLE `icinga_hoststatus` TYPE = innodb;
ALTER TABLE `icinga_instances` TYPE = innodb;
ALTER TABLE `icinga_logentries` TYPE = innodb;
ALTER TABLE `icinga_notifications` TYPE = innodb;
ALTER TABLE `icinga_objects` TYPE = innodb;
ALTER TABLE `icinga_processevents` TYPE = innodb;
ALTER TABLE `icinga_programstatus` TYPE = innodb;
ALTER TABLE `icinga_runtimevariables` TYPE = innodb;
ALTER TABLE `icinga_scheduleddowntime` TYPE = innodb;
ALTER TABLE `icinga_service_contactgroups` TYPE = innodb;
ALTER TABLE `icinga_service_contacts` TYPE = innodb;
ALTER TABLE `icinga_servicechecks` TYPE = innodb;
ALTER TABLE `icinga_servicedependencies` TYPE = innodb;
ALTER TABLE `icinga_serviceescalation_contactgroups` TYPE = innodb;
ALTER TABLE `icinga_serviceescalation_contacts` TYPE = innodb;
ALTER TABLE `icinga_serviceescalations` TYPE = innodb;
ALTER TABLE `icinga_servicegroup_members` TYPE = innodb;
ALTER TABLE `icinga_servicegroups` TYPE = innodb;
ALTER TABLE `icinga_services` TYPE = innodb;
ALTER TABLE `icinga_servicestatus` TYPE = innodb;
ALTER TABLE `icinga_statehistory` TYPE = innodb;
ALTER TABLE `icinga_systemcommands` TYPE = innodb;
ALTER TABLE `icinga_timedeventqueue` TYPE = innodb;
ALTER TABLE `icinga_timedevents` TYPE = innodb;
ALTER TABLE `icinga_timeperiod_timeranges` TYPE = innodb;
ALTER TABLE `icinga_timeperiods` TYPE = innodb;

ALTER TABLE `icinga_statehistory` ADD `last_state` SMALLINT DEFAULT '-1' NOT NULL AFTER `max_check_attempts` ,
ADD `last_hard_state` SMALLINT DEFAULT '-1' NOT NULL AFTER `last_state` ;

-- END 1.4b5 MODS 
