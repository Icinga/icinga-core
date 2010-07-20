-- --------------------------------------------------------
-- pgsql.sql
-- DB definition for Postgresql
--
-- Copyright (c) 2009-2010 Icinga Development Team (http://www.icinga.org)
--
-- initial version: 2009-05-13 Markus Manzke
-- current version: 2010-07-20 Michael Friedrich <michael.friedrich@univie.ac.at>
--
-- --------------------------------------------------------

--
-- Functions
--

CREATE OR REPLACE FUNCTION from_unixtime(integer) RETURNS timestamp AS '
	 SELECT to_timestamp($1)::timestamp AS result
' LANGUAGE 'SQL';

-- timestamp without time zone (i.e. 1973-11-29 21:33:09)
CREATE OR REPLACE FUNCTION unix_timestamp(timestamp) RETURNS bigint AS '
	SELECT EXTRACT(EPOCH FROM $1)::bigint AS result;
' LANGUAGE 'SQL';


--
-- Database: icinga
--

-- --------------------------------------------------------

--
-- Table structure for table icinga_acknowledgements
--

CREATE TABLE  icinga_acknowledgements (
  acknowledgement_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time_usec INTEGER NOT NULL default 0,
  acknowledgement_type INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  state INTEGER NOT NULL default 0,
  author_name TEXT NOT NULL default '',
  comment_data TEXT NOT NULL default '',
  is_sticky INTEGER NOT NULL default 0,
  persistent_comment INTEGER NOT NULL default 0,
  notify_contacts INTEGER NOT NULL default 0,
  PRIMARY KEY  (acknowledgement_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_commands
--

CREATE TABLE  icinga_commands (
  command_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  command_line TEXT NOT NULL default '',
  PRIMARY KEY  (command_id),
  UNIQUE (instance_id,object_id,config_type)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_commenthistory
--

CREATE TABLE  icinga_commenthistory (
  commenthistory_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time_usec INTEGER NOT NULL default 0,
  comment_type INTEGER NOT NULL default 0,
  entry_type INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  comment_time timestamp NOT NULL default '1970-01-01 00:00:00',
  internal_comment_id INTEGER NOT NULL default 0,
  author_name TEXT NOT NULL default '',
  comment_data TEXT NOT NULL default '',
  is_persistent INTEGER NOT NULL default 0,
  comment_source INTEGER NOT NULL default 0,
  expires INTEGER NOT NULL default 0,
  expiration_time timestamp NOT NULL default '1970-01-01 00:00:00',
  deletion_time timestamp NOT NULL default '1970-01-01 00:00:00',
  deletion_time_usec INTEGER NOT NULL default 0,
  PRIMARY KEY  (commenthistory_id),
  UNIQUE (instance_id,comment_time,internal_comment_id)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_comments
--

CREATE TABLE  icinga_comments (
  comment_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time_usec INTEGER NOT NULL default 0,
  comment_type INTEGER NOT NULL default 0,
  entry_type INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  comment_time timestamp NOT NULL default '1970-01-01 00:00:00',
  internal_comment_id INTEGER NOT NULL default 0,
  author_name TEXT NOT NULL default '',
  comment_data TEXT NOT NULL default '',
  is_persistent INTEGER NOT NULL default 0,
  comment_source INTEGER NOT NULL default 0,
  expires INTEGER NOT NULL default 0,
  expiration_time timestamp NOT NULL default '1970-01-01 00:00:00',
  PRIMARY KEY  (comment_id),
  UNIQUE (instance_id,comment_time,internal_comment_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_configfiles
--

CREATE TABLE  icinga_configfiles (
  configfile_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  configfile_type INTEGER NOT NULL default 0,
  configfile_path TEXT NOT NULL default '',
  PRIMARY KEY  (configfile_id),
  UNIQUE (instance_id,configfile_type,configfile_path)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_configfilevariables
--

CREATE TABLE  icinga_configfilevariables (
  configfilevariable_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  configfile_id INTEGER NOT NULL default 0,
  varname TEXT NOT NULL default '',
  varvalue TEXT NOT NULL default '',
  PRIMARY KEY  (configfilevariable_id)
  --UNIQUE (instance_id,configfile_id) 
  -- varname/varvalue are not unique!
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_conninfo
--

CREATE TABLE  icinga_conninfo (
  conninfo_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  agent_name TEXT NOT NULL default '',
  agent_version TEXT NOT NULL default '',
  disposition TEXT NOT NULL default '',
  connect_source TEXT NOT NULL default '',
  connect_type TEXT NOT NULL default '',
  connect_time timestamp NOT NULL default '1970-01-01 00:00:00',
  disconnect_time timestamp NOT NULL default '1970-01-01 00:00:00',
  last_checkin_time timestamp NOT NULL default '1970-01-01 00:00:00',
  data_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  data_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  bytes_processed INTEGER NOT NULL default 0,
  lines_processed INTEGER NOT NULL default 0,
  entries_processed INTEGER NOT NULL default 0,
  PRIMARY KEY  (conninfo_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactgroups
--

CREATE TABLE  icinga_contactgroups (
  contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  contactgroup_object_id INTEGER NOT NULL default 0,
  alias TEXT NOT NULL default '',
  PRIMARY KEY  (contactgroup_id),
  UNIQUE (instance_id,config_type,contactgroup_object_id)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactgroup_members
--

CREATE TABLE  icinga_contactgroup_members (
  contactgroup_member_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  contactgroup_id INTEGER NOT NULL default 0,
  contact_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (contactgroup_member_id),
  UNIQUE (contactgroup_id,contact_object_id)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactnotificationmethods
--

CREATE TABLE  icinga_contactnotificationmethods (
  contactnotificationmethod_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  contactnotification_id INTEGER NOT NULL default 0,
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default 0,
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default 0,
  command_object_id INTEGER NOT NULL default 0,
  command_args TEXT NOT NULL default '',
  PRIMARY KEY  (contactnotificationmethod_id),
  UNIQUE (instance_id,contactnotification_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactnotifications
--

CREATE TABLE  icinga_contactnotifications (
  contactnotification_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  notification_id INTEGER NOT NULL default 0,
  contact_object_id INTEGER NOT NULL default 0,
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default 0,
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default 0,
  PRIMARY KEY  (contactnotification_id),
  UNIQUE (instance_id,contact_object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contacts
--

CREATE TABLE  icinga_contacts (
  contact_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  contact_object_id INTEGER NOT NULL default 0,
  alias TEXT NOT NULL default '',
  email_address TEXT NOT NULL default '',
  pager_address TEXT NOT NULL default '',
  host_timeperiod_object_id INTEGER NOT NULL default 0,
  service_timeperiod_object_id INTEGER NOT NULL default 0,
  host_notifications_enabled INTEGER NOT NULL default 0,
  service_notifications_enabled INTEGER NOT NULL default 0,
  can_submit_commands INTEGER NOT NULL default 0,
  notify_service_recovery INTEGER NOT NULL default 0,
  notify_service_warning INTEGER NOT NULL default 0,
  notify_service_unknown INTEGER NOT NULL default 0,
  notify_service_critical INTEGER NOT NULL default 0,
  notify_service_flapping INTEGER NOT NULL default 0,
  notify_service_downtime INTEGER NOT NULL default 0,
  notify_host_recovery INTEGER NOT NULL default 0,
  notify_host_down INTEGER NOT NULL default 0,
  notify_host_unreachable INTEGER NOT NULL default 0,
  notify_host_flapping INTEGER NOT NULL default 0,
  notify_host_downtime INTEGER NOT NULL default 0,
  PRIMARY KEY  (contact_id),
  UNIQUE (instance_id,config_type,contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactstatus
--

CREATE TABLE  icinga_contactstatus (
  contactstatus_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  contact_object_id INTEGER NOT NULL default 0,
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  host_notifications_enabled INTEGER NOT NULL default 0,
  service_notifications_enabled INTEGER NOT NULL default 0,
  last_host_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  last_service_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  modified_attributes INTEGER NOT NULL default 0,
  modified_host_attributes INTEGER NOT NULL default 0,
  modified_service_attributes INTEGER NOT NULL default 0,
  PRIMARY KEY  (contactstatus_id),
  UNIQUE  (contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contact_addresses
--

CREATE TABLE  icinga_contact_addresses (
  contact_address_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  contact_id INTEGER NOT NULL default 0,
  address_number INTEGER NOT NULL default 0,
  address TEXT NOT NULL default '',
  PRIMARY KEY  (contact_address_id),
  UNIQUE  (contact_id,address_number)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contact_notificationcommands
--

CREATE TABLE  icinga_contact_notificationcommands (
  contact_notificationcommand_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  contact_id INTEGER NOT NULL default 0,
  notification_type INTEGER NOT NULL default 0,
  command_object_id INTEGER NOT NULL default 0,
  command_args TEXT NOT NULL default '',
  PRIMARY KEY  (contact_notificationcommand_id),
  UNIQUE  (contact_id,notification_type,command_object_id,command_args)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_customvariables
--

CREATE TABLE  icinga_customvariables (
  customvariable_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  has_been_modified INTEGER NOT NULL default 0,
  varname TEXT NOT NULL default '',
  varvalue TEXT NOT NULL default '',
  PRIMARY KEY  (customvariable_id),
  UNIQUE (object_id,config_type,varname)
) ;
CREATE INDEX icinga_customvariables_i ON icinga_customvariables(varname);

-- --------------------------------------------------------

--
-- Table structure for table icinga_customvariablestatus
--

CREATE TABLE  icinga_customvariablestatus (
  customvariablestatus_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  has_been_modified INTEGER NOT NULL default 0,
  varname TEXT NOT NULL default '',
  varvalue TEXT NOT NULL default '',
  PRIMARY KEY  (customvariablestatus_id),
  UNIQUE (object_id,varname)
) ;
CREATE INDEX icinga_customvariablestatus_i ON icinga_customvariablestatus(varname);


-- --------------------------------------------------------

--
-- Table structure for table icinga_dbversion
--

CREATE TABLE  icinga_dbversion (
  name TEXT NOT NULL default '',
  version TEXT NOT NULL default ''
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_downtimehistory
--

CREATE TABLE  icinga_downtimehistory (
  downtimehistory_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  downtime_type INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  author_name TEXT NOT NULL default '',
  comment_data TEXT NOT NULL default '',
  internal_downtime_id INTEGER NOT NULL default 0,
  triggered_by_id INTEGER NOT NULL default 0,
  is_fixed INTEGER NOT NULL default 0,
  duration INTEGER NOT NULL default 0,
  scheduled_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  scheduled_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  was_started INTEGER NOT NULL default 0,
  actual_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  actual_start_time_usec INTEGER NOT NULL default 0,
  actual_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  actual_end_time_usec INTEGER NOT NULL default 0,
  was_cancelled INTEGER NOT NULL default 0,
  PRIMARY KEY  (downtimehistory_id),
  UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_eventhandlers
--

CREATE TABLE  icinga_eventhandlers (
  eventhandler_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  eventhandler_type INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  state INTEGER NOT NULL default 0,
  state_type INTEGER NOT NULL default 0,
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default 0,
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default 0,
  command_object_id INTEGER NOT NULL default 0,
  command_args TEXT NOT NULL default '',
  command_line TEXT NOT NULL default '',
  timeout INTEGER NOT NULL default 0,
  early_timeout INTEGER NOT NULL default 0,
  execution_time double precision NOT NULL default 0,
  return_code INTEGER NOT NULL default 0,
  output TEXT NOT NULL default '',
  long_output TEXT NOT NULL default '',
  PRIMARY KEY  (eventhandler_id),
  UNIQUE (instance_id,object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_externalcommands
--

CREATE TABLE  icinga_externalcommands (
  externalcommand_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  command_type INTEGER NOT NULL default 0,
  command_name TEXT NOT NULL default '',
  command_args TEXT NOT NULL default '',
  PRIMARY KEY  (externalcommand_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_flappinghistory
--

CREATE TABLE  icinga_flappinghistory (
  flappinghistory_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  event_time timestamp NOT NULL default '1970-01-01 00:00:00',
  event_time_usec INTEGER NOT NULL default 0,
  event_type INTEGER NOT NULL default 0,
  reason_type INTEGER NOT NULL default 0,
  flapping_type INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  percent_state_change double precision NOT NULL default 0,
  low_threshold double precision NOT NULL default 0,
  high_threshold double precision NOT NULL default 0,
  comment_time timestamp NOT NULL default '1970-01-01 00:00:00',
  internal_comment_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (flappinghistory_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostchecks
--

CREATE TABLE  icinga_hostchecks (
  hostcheck_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  host_object_id INTEGER NOT NULL default 0,
  check_type INTEGER NOT NULL default 0,
  is_raw_check INTEGER NOT NULL default 0,
  current_check_attempt INTEGER NOT NULL default 0,
  max_check_attempts INTEGER NOT NULL default 0,
  state INTEGER NOT NULL default 0,
  state_type INTEGER NOT NULL default 0,
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default 0,
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default 0,
  command_object_id INTEGER NOT NULL default 0,
  command_args TEXT NOT NULL default '',
  command_line TEXT NOT NULL default '',
  timeout INTEGER NOT NULL default 0,
  early_timeout INTEGER NOT NULL default 0,
  execution_time double precision NOT NULL default 0,
  latency double precision NOT NULL default 0,
  return_code INTEGER NOT NULL default 0,
  output TEXT NOT NULL default '',
  long_output TEXT NOT NULL default '',
  perfdata TEXT NOT NULL default '',
  PRIMARY KEY  (hostcheck_id),
  UNIQUE (instance_id,host_object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostdependencies
--

CREATE TABLE  icinga_hostdependencies (
  hostdependency_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  host_object_id INTEGER NOT NULL default 0,
  dependent_host_object_id INTEGER NOT NULL default 0,
  dependency_type INTEGER NOT NULL default 0,
  inherits_parent INTEGER NOT NULL default 0,
  timeperiod_object_id INTEGER NOT NULL default 0,
  fail_on_up INTEGER NOT NULL default 0,
  fail_on_down INTEGER NOT NULL default 0,
  fail_on_unreachable INTEGER NOT NULL default 0,
  PRIMARY KEY  (hostdependency_id),
  UNIQUE (instance_id,config_type,host_object_id,dependent_host_object_id,dependency_type,inherits_parent,fail_on_up,fail_on_down,fail_on_unreachable)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalations
--

CREATE TABLE  icinga_hostescalations (
  hostescalation_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  host_object_id INTEGER NOT NULL default 0,
  timeperiod_object_id INTEGER NOT NULL default 0,
  first_notification INTEGER NOT NULL default 0,
  last_notification INTEGER NOT NULL default 0,
  notification_interval double precision NOT NULL default 0,
  escalate_on_recovery INTEGER NOT NULL default 0,
  escalate_on_down INTEGER NOT NULL default 0,
  escalate_on_unreachable INTEGER NOT NULL default 0,
  PRIMARY KEY  (hostescalation_id),
  UNIQUE (instance_id,config_type,host_object_id,timeperiod_object_id,first_notification,last_notification)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalation_contactgroups
--

CREATE TABLE  icinga_hostescalation_contactgroups (
  hostescalation_contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  hostescalation_id INTEGER NOT NULL default 0,
  contactgroup_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (hostescalation_contactgroup_id),
  UNIQUE (hostescalation_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalation_contacts
--

CREATE TABLE  icinga_hostescalation_contacts (
  hostescalation_contact_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  hostescalation_id INTEGER NOT NULL default 0,
  contact_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (hostescalation_contact_id),
  UNIQUE (instance_id,hostescalation_id,contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostgroups
--

CREATE TABLE  icinga_hostgroups (
  hostgroup_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  hostgroup_object_id INTEGER NOT NULL default 0,
  alias TEXT NOT NULL default '',
  PRIMARY KEY  (hostgroup_id),
  UNIQUE (instance_id,hostgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostgroup_members
--

CREATE TABLE  icinga_hostgroup_members (
  hostgroup_member_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  hostgroup_id INTEGER NOT NULL default 0,
  host_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (hostgroup_member_id),
  UNIQUE (hostgroup_id,host_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hosts
--

CREATE TABLE  icinga_hosts (
  host_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  host_object_id INTEGER NOT NULL default 0,
  alias TEXT NOT NULL default '',
  display_name TEXT NOT NULL default '',
  address TEXT NOT NULL default '',
  check_command_object_id INTEGER NOT NULL default 0,
  check_command_args TEXT NOT NULL default '',
  eventhandler_command_object_id INTEGER NOT NULL default 0,
  eventhandler_command_args TEXT NOT NULL default '',
  notification_timeperiod_object_id INTEGER NOT NULL default 0,
  check_timeperiod_object_id INTEGER NOT NULL default 0,
  failure_prediction_options TEXT NOT NULL default '',
  check_interval double precision NOT NULL default 0,
  retry_interval double precision NOT NULL default 0,
  max_check_attempts INTEGER NOT NULL default 0,
  first_notification_delay double precision NOT NULL default 0,
  notification_interval double precision NOT NULL default 0,
  notify_on_down INTEGER NOT NULL default 0,
  notify_on_unreachable INTEGER NOT NULL default 0,
  notify_on_recovery INTEGER NOT NULL default 0,
  notify_on_flapping INTEGER NOT NULL default 0,
  notify_on_downtime INTEGER NOT NULL default 0,
  stalk_on_up INTEGER NOT NULL default 0,
  stalk_on_down INTEGER NOT NULL default 0,
  stalk_on_unreachable INTEGER NOT NULL default 0,
  flap_detection_enabled INTEGER NOT NULL default 0,
  flap_detection_on_up INTEGER NOT NULL default 0,
  flap_detection_on_down INTEGER NOT NULL default 0,
  flap_detection_on_unreachable INTEGER NOT NULL default 0,
  low_flap_threshold double precision NOT NULL default 0,
  high_flap_threshold double precision NOT NULL default 0,
  process_performance_data INTEGER NOT NULL default 0,
  freshness_checks_enabled INTEGER NOT NULL default 0,
  freshness_threshold INTEGER NOT NULL default 0,
  passive_checks_enabled INTEGER NOT NULL default 0,
  event_handler_enabled INTEGER NOT NULL default 0,
  active_checks_enabled INTEGER NOT NULL default 0,
  retain_status_information INTEGER NOT NULL default 0,
  retain_nonstatus_information INTEGER NOT NULL default 0,
  notifications_enabled INTEGER NOT NULL default 0,
  obsess_over_host INTEGER NOT NULL default 0,
  failure_prediction_enabled INTEGER NOT NULL default 0,
  notes TEXT NOT NULL default '',
  notes_url TEXT NOT NULL default '',
  action_url TEXT NOT NULL default '',
  icon_image TEXT NOT NULL default '',
  icon_image_alt TEXT NOT NULL default '',
  vrml_image TEXT NOT NULL default '',
  statusmap_image TEXT NOT NULL default '',
  have_2d_coords INTEGER NOT NULL default 0,
  x_2d INTEGER NOT NULL default 0,
  y_2d INTEGER NOT NULL default 0,
  have_3d_coords INTEGER NOT NULL default 0,
  x_3d double precision NOT NULL default 0,
  y_3d double precision NOT NULL default 0,
  z_3d double precision NOT NULL default 0,
  PRIMARY KEY  (host_id),
  UNIQUE (instance_id,config_type,host_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hoststatus
--

CREATE TABLE  icinga_hoststatus (
  hoststatus_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  host_object_id INTEGER NOT NULL default 0,
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  output TEXT NOT NULL default '',
  long_output TEXT NOT NULL default '',
  perfdata TEXT NOT NULL default '',
  current_state INTEGER NOT NULL default 0,
  has_been_checked INTEGER NOT NULL default 0,
  should_be_scheduled INTEGER NOT NULL default 0,
  current_check_attempt INTEGER NOT NULL default 0,
  max_check_attempts INTEGER NOT NULL default 0,
  last_check timestamp NOT NULL default '1970-01-01 00:00:00',
  next_check timestamp NOT NULL default '1970-01-01 00:00:00',
  check_type INTEGER NOT NULL default 0,
  last_state_change timestamp NOT NULL default '1970-01-01 00:00:00',
  last_hard_state_change timestamp NOT NULL default '1970-01-01 00:00:00',
  last_hard_state INTEGER NOT NULL default 0,
  last_time_up timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_down timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_unreachable timestamp NOT NULL default '1970-01-01 00:00:00',
  state_type INTEGER NOT NULL default 0,
  last_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  next_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  no_more_notifications INTEGER NOT NULL default 0,
  notifications_enabled INTEGER NOT NULL default 0,
  problem_has_been_acknowledged INTEGER NOT NULL default 0,
  acknowledgement_type INTEGER NOT NULL default 0,
  current_notification_number INTEGER NOT NULL default 0,
  passive_checks_enabled INTEGER NOT NULL default 0,
  active_checks_enabled INTEGER NOT NULL default 0,
  event_handler_enabled INTEGER NOT NULL default 0,
  flap_detection_enabled INTEGER NOT NULL default 0,
  is_flapping INTEGER NOT NULL default 0,
  percent_state_change double precision NOT NULL default 0,
  latency double precision NOT NULL default 0,
  execution_time double precision NOT NULL default 0,
  scheduled_downtime_depth INTEGER NOT NULL default 0,
  failure_prediction_enabled INTEGER NOT NULL default 0,
  process_performance_data INTEGER NOT NULL default 0,
  obsess_over_host INTEGER NOT NULL default 0,
  modified_host_attributes INTEGER NOT NULL default 0,
  event_handler TEXT NOT NULL default '',
  check_command TEXT NOT NULL default '',
  normal_check_interval double precision NOT NULL default 0,
  retry_check_interval double precision NOT NULL default 0,
  check_timeperiod_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (hoststatus_id),
  UNIQUE (host_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_contactgroups
--

CREATE TABLE  icinga_host_contactgroups (
  host_contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  host_id INTEGER NOT NULL default 0,
  contactgroup_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (host_contactgroup_id),
  UNIQUE (host_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_contacts
--

CREATE TABLE  icinga_host_contacts (
  host_contact_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  host_id INTEGER NOT NULL default 0,
  contact_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (host_contact_id),
  UNIQUE (instance_id,host_id,contact_object_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_parenthosts
--

CREATE TABLE  icinga_host_parenthosts (
  host_parenthost_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  host_id INTEGER NOT NULL default 0,
  parent_host_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (host_parenthost_id),
  UNIQUE (host_id,parent_host_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_instances
--

CREATE TABLE  icinga_instances (
  instance_id SERIAL,
  instance_name TEXT NOT NULL default '',
  instance_description TEXT NOT NULL default '',
  PRIMARY KEY  (instance_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_logentries
--

CREATE TABLE  icinga_logentries (
  logentry_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  logentry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time_usec INTEGER NOT NULL default 0,
  logentry_type INTEGER NOT NULL default 0,
  logentry_data TEXT NOT NULL default '',
  realtime_data INTEGER NOT NULL default 0,
  inferred_data_extracted INTEGER NOT NULL default 0,
  PRIMARY KEY  (logentry_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_notifications
--

CREATE TABLE  icinga_notifications (
  notification_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  notification_type INTEGER NOT NULL default 0,
  notification_reason INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default 0,
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default 0,
  state INTEGER NOT NULL default 0,
  output TEXT NOT NULL default '',
  long_output TEXT NOT NULL default '',
  escalated INTEGER NOT NULL default 0,
  contacts_notified INTEGER NOT NULL default 0,
  PRIMARY KEY  (notification_id),
  UNIQUE (instance_id,object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_objects
--

CREATE TABLE  icinga_objects (
  object_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  objecttype_id INTEGER NOT NULL default 0,
  name1 TEXT,
  name2 TEXT,
  is_active INTEGER NOT NULL default 0,
  PRIMARY KEY  (object_id)
--  UNIQUE (objecttype_id,name1,name2)
) ;
CREATE INDEX icinga_objects_i ON icinga_objects(objecttype_id,name1,name2);

-- --------------------------------------------------------

--
-- Table structure for table icinga_processevents
--

CREATE TABLE  icinga_processevents (
  processevent_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  event_type INTEGER NOT NULL default 0,
  event_time timestamp NOT NULL default '1970-01-01 00:00:00',
  event_time_usec INTEGER NOT NULL default 0,
  process_id INTEGER NOT NULL default 0,
  program_name TEXT NOT NULL default '',
  program_version TEXT NOT NULL default '',
  program_date TEXT NOT NULL default '',
  PRIMARY KEY  (processevent_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_programstatus
--

CREATE TABLE  icinga_programstatus (
  programstatus_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  program_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  program_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  is_currently_running INTEGER NOT NULL default 0,
  process_id INTEGER NOT NULL default 0,
  daemon_mode INTEGER NOT NULL default 0,
  last_command_check timestamp NOT NULL default '1970-01-01 00:00:00',
  last_log_rotation timestamp NOT NULL default '1970-01-01 00:00:00',
  notifications_enabled INTEGER NOT NULL default 0,
  active_service_checks_enabled INTEGER NOT NULL default 0,
  passive_service_checks_enabled INTEGER NOT NULL default 0,
  active_host_checks_enabled INTEGER NOT NULL default 0,
  passive_host_checks_enabled INTEGER NOT NULL default 0,
  event_handlers_enabled INTEGER NOT NULL default 0,
  flap_detection_enabled INTEGER NOT NULL default 0,
  failure_prediction_enabled INTEGER NOT NULL default 0,
  process_performance_data INTEGER NOT NULL default 0,
  obsess_over_hosts INTEGER NOT NULL default 0,
  obsess_over_services INTEGER NOT NULL default 0,
  modified_host_attributes INTEGER NOT NULL default 0,
  modified_service_attributes INTEGER NOT NULL default 0,
  global_host_event_handler TEXT NOT NULL default '',
  global_service_event_handler TEXT NOT NULL default '',
  PRIMARY KEY  (programstatus_id),
  UNIQUE (instance_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_runtimevariables
--

CREATE TABLE  icinga_runtimevariables (
  runtimevariable_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  varname TEXT NOT NULL default '',
  varvalue TEXT NOT NULL default '',
  PRIMARY KEY  (runtimevariable_id),
  UNIQUE (instance_id,varname)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_scheduleddowntime
--

CREATE TABLE  icinga_scheduleddowntime (
  scheduleddowntime_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  downtime_type INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  author_name TEXT NOT NULL default '',
  comment_data TEXT NOT NULL default '',
  internal_downtime_id INTEGER NOT NULL default 0,
  triggered_by_id INTEGER NOT NULL default 0,
  is_fixed INTEGER NOT NULL default 0,
  duration INTEGER NOT NULL default 0,
  scheduled_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  scheduled_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  was_started INTEGER NOT NULL default 0,
  actual_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  actual_start_time_usec INTEGER NOT NULL default 0,
  PRIMARY KEY  (scheduleddowntime_id),
  UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicechecks
--

CREATE TABLE  icinga_servicechecks (
  servicecheck_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  service_object_id INTEGER NOT NULL default 0,
  check_type INTEGER NOT NULL default 0,
  current_check_attempt INTEGER NOT NULL default 0,
  max_check_attempts INTEGER NOT NULL default 0,
  state INTEGER NOT NULL default 0,
  state_type INTEGER NOT NULL default 0,
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default 0,
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default 0,
  command_object_id INTEGER NOT NULL default 0,
  command_args TEXT NOT NULL default '',
  command_line TEXT NOT NULL default '',
  timeout INTEGER NOT NULL default 0,
  early_timeout INTEGER NOT NULL default 0,
  execution_time double precision NOT NULL default 0,
  latency double precision NOT NULL default 0,
  return_code INTEGER NOT NULL default 0,
  output TEXT NOT NULL default '',
  long_output TEXT NOT NULL default '',
  perfdata TEXT NOT NULL default '',
  PRIMARY KEY  (servicecheck_id),
  UNIQUE (instance_id,service_object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicedependencies
--

CREATE TABLE  icinga_servicedependencies (
  servicedependency_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  service_object_id INTEGER NOT NULL default 0,
  dependent_service_object_id INTEGER NOT NULL default 0,
  dependency_type INTEGER NOT NULL default 0,
  inherits_parent INTEGER NOT NULL default 0,
  timeperiod_object_id INTEGER NOT NULL default 0,
  fail_on_ok INTEGER NOT NULL default 0,
  fail_on_warning INTEGER NOT NULL default 0,
  fail_on_unknown INTEGER NOT NULL default 0,
  fail_on_critical INTEGER NOT NULL default 0,
  PRIMARY KEY  (servicedependency_id),
  UNIQUE (instance_id,config_type,service_object_id,dependent_service_object_id,dependency_type,inherits_parent,fail_on_ok,fail_on_warning,fail_on_unknown,fail_on_critical)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalations
--

CREATE TABLE  icinga_serviceescalations (
  serviceescalation_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  service_object_id INTEGER NOT NULL default 0,
  timeperiod_object_id INTEGER NOT NULL default 0,
  first_notification INTEGER NOT NULL default 0,
  last_notification INTEGER NOT NULL default 0,
  notification_interval double precision NOT NULL default 0,
  escalate_on_recovery INTEGER NOT NULL default 0,
  escalate_on_warning INTEGER NOT NULL default 0,
  escalate_on_unknown INTEGER NOT NULL default 0,
  escalate_on_critical INTEGER NOT NULL default 0,
  PRIMARY KEY  (serviceescalation_id),
  UNIQUE (instance_id,config_type,service_object_id,timeperiod_object_id,first_notification,last_notification)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalation_contactgroups
--

CREATE TABLE  icinga_serviceescalation_contactgroups (
  serviceescalation_contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  serviceescalation_id INTEGER NOT NULL default 0,
  contactgroup_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (serviceescalation_contactgroup_id),
  UNIQUE (serviceescalation_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalation_contacts
--

CREATE TABLE  icinga_serviceescalation_contacts (
  serviceescalation_contact_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  serviceescalation_id INTEGER NOT NULL default 0,
  contact_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (serviceescalation_contact_id),
  UNIQUE (instance_id,serviceescalation_id,contact_object_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicegroups
--

CREATE TABLE  icinga_servicegroups (
  servicegroup_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  servicegroup_object_id INTEGER NOT NULL default 0,
  alias TEXT NOT NULL default '',
  PRIMARY KEY  (servicegroup_id),
  UNIQUE (instance_id,config_type,servicegroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicegroup_members
--

CREATE TABLE  icinga_servicegroup_members (
  servicegroup_member_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  servicegroup_id INTEGER NOT NULL default 0,
  service_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (servicegroup_member_id),
  UNIQUE (servicegroup_id,service_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_services
--

CREATE TABLE  icinga_services (
  service_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  host_object_id INTEGER NOT NULL default 0,
  service_object_id INTEGER NOT NULL default 0,
  display_name TEXT NOT NULL default '',
  check_command_object_id INTEGER NOT NULL default 0,
  check_command_args TEXT NOT NULL default '',
  eventhandler_command_object_id INTEGER NOT NULL default 0,
  eventhandler_command_args TEXT NOT NULL default '',
  notification_timeperiod_object_id INTEGER NOT NULL default 0,
  check_timeperiod_object_id INTEGER NOT NULL default 0,
  failure_prediction_options TEXT NOT NULL default '',
  check_interval double precision NOT NULL default 0,
  retry_interval double precision NOT NULL default 0,
  max_check_attempts INTEGER NOT NULL default 0,
  first_notification_delay double precision NOT NULL default 0,
  notification_interval double precision NOT NULL default 0,
  notify_on_warning INTEGER NOT NULL default 0,
  notify_on_unknown INTEGER NOT NULL default 0,
  notify_on_critical INTEGER NOT NULL default 0,
  notify_on_recovery INTEGER NOT NULL default 0,
  notify_on_flapping INTEGER NOT NULL default 0,
  notify_on_downtime INTEGER NOT NULL default 0,
  stalk_on_ok INTEGER NOT NULL default 0,
  stalk_on_warning INTEGER NOT NULL default 0,
  stalk_on_unknown INTEGER NOT NULL default 0,
  stalk_on_critical INTEGER NOT NULL default 0,
  is_volatile INTEGER NOT NULL default 0,
  flap_detection_enabled INTEGER NOT NULL default 0,
  flap_detection_on_ok INTEGER NOT NULL default 0,
  flap_detection_on_warning INTEGER NOT NULL default 0,
  flap_detection_on_unknown INTEGER NOT NULL default 0,
  flap_detection_on_critical INTEGER NOT NULL default 0,
  low_flap_threshold double precision NOT NULL default 0,
  high_flap_threshold double precision NOT NULL default 0,
  process_performance_data INTEGER NOT NULL default 0,
  freshness_checks_enabled INTEGER NOT NULL default 0,
  freshness_threshold INTEGER NOT NULL default 0,
  passive_checks_enabled INTEGER NOT NULL default 0,
  event_handler_enabled INTEGER NOT NULL default 0,
  active_checks_enabled INTEGER NOT NULL default 0,
  retain_status_information INTEGER NOT NULL default 0,
  retain_nonstatus_information INTEGER NOT NULL default 0,
  notifications_enabled INTEGER NOT NULL default 0,
  obsess_over_service INTEGER NOT NULL default 0,
  failure_prediction_enabled INTEGER NOT NULL default 0,
  notes TEXT NOT NULL default '',
  notes_url TEXT NOT NULL default '',
  action_url TEXT NOT NULL default '',
  icon_image TEXT NOT NULL default '',
  icon_image_alt TEXT NOT NULL default '',
  PRIMARY KEY  (service_id),
  UNIQUE (instance_id,config_type,service_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicestatus
--

CREATE TABLE  icinga_servicestatus (
  servicestatus_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  service_object_id INTEGER NOT NULL default 0,
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  output TEXT NOT NULL default '',
  long_output TEXT NOT NULL default '',
  perfdata TEXT NOT NULL default '',
  current_state INTEGER NOT NULL default 0,
  has_been_checked INTEGER NOT NULL default 0,
  should_be_scheduled INTEGER NOT NULL default 0,
  current_check_attempt INTEGER NOT NULL default 0,
  max_check_attempts INTEGER NOT NULL default 0,
  last_check timestamp NOT NULL default '1970-01-01 00:00:00',
  next_check timestamp NOT NULL default '1970-01-01 00:00:00',
  check_type INTEGER NOT NULL default 0,
  last_state_change timestamp NOT NULL default '1970-01-01 00:00:00',
  last_hard_state_change timestamp NOT NULL default '1970-01-01 00:00:00',
  last_hard_state INTEGER NOT NULL default 0,
  last_time_ok timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_warning timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_unknown timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_critical timestamp NOT NULL default '1970-01-01 00:00:00',
  state_type INTEGER NOT NULL default 0,
  last_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  next_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  no_more_notifications INTEGER NOT NULL default 0,
  notifications_enabled INTEGER NOT NULL default 0,
  problem_has_been_acknowledged INTEGER NOT NULL default 0,
  acknowledgement_type INTEGER NOT NULL default 0,
  current_notification_number INTEGER NOT NULL default 0,
  passive_checks_enabled INTEGER NOT NULL default 0,
  active_checks_enabled INTEGER NOT NULL default 0,
  event_handler_enabled INTEGER NOT NULL default 0,
  flap_detection_enabled INTEGER NOT NULL default 0,
  is_flapping INTEGER NOT NULL default 0,
  percent_state_change double precision NOT NULL default 0,
  latency double precision NOT NULL default 0,
  execution_time double precision NOT NULL default 0,
  scheduled_downtime_depth INTEGER NOT NULL default 0,
  failure_prediction_enabled INTEGER NOT NULL default 0,
  process_performance_data INTEGER NOT NULL default 0,
  obsess_over_service INTEGER NOT NULL default 0,
  modified_service_attributes INTEGER NOT NULL default 0,
  event_handler TEXT NOT NULL default '',
  check_command TEXT NOT NULL default '',
  normal_check_interval double precision NOT NULL default 0,
  retry_check_interval double precision NOT NULL default 0,
  check_timeperiod_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (servicestatus_id),
  UNIQUE (service_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_service_contactgroups
--

CREATE TABLE  icinga_service_contactgroups (
  service_contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  service_id INTEGER NOT NULL default 0,
  contactgroup_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (service_contactgroup_id),
  UNIQUE (service_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_service_contacts
--

CREATE TABLE  icinga_service_contacts (
  service_contact_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  service_id INTEGER NOT NULL default 0,
  contact_object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (service_contact_id),
  UNIQUE (instance_id,service_id,contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_statehistory
--

CREATE TABLE  icinga_statehistory (
  statehistory_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  state_time timestamp NOT NULL default '1970-01-01 00:00:00',
  state_time_usec INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  state_change INTEGER NOT NULL default 0,
  state INTEGER NOT NULL default 0,
  state_type INTEGER NOT NULL default 0,
  current_check_attempt INTEGER NOT NULL default 0,
  max_check_attempts INTEGER NOT NULL default 0,
  last_state INTEGER NOT NULL default '-1',
  last_hard_state INTEGER NOT NULL default '-1',
  output TEXT NOT NULL default '',
  long_output TEXT NOT NULL default '',
  PRIMARY KEY  (statehistory_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_systemcommands
--

CREATE TABLE  icinga_systemcommands (
  systemcommand_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default 0,
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default 0,
  command_line TEXT NOT NULL default '',
  timeout INTEGER NOT NULL default 0,
  early_timeout INTEGER NOT NULL default 0,
  execution_time double precision NOT NULL default 0,
  return_code INTEGER NOT NULL default 0,
  output TEXT NOT NULL default '',
  long_output TEXT NOT NULL default '',
  PRIMARY KEY  (systemcommand_id),
  UNIQUE (instance_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timedeventqueue
--

CREATE TABLE  icinga_timedeventqueue (
  timedeventqueue_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  event_type INTEGER NOT NULL default 0,
  queued_time timestamp NOT NULL default '1970-01-01 00:00:00',
  queued_time_usec INTEGER NOT NULL default 0,
  scheduled_time timestamp NOT NULL default '1970-01-01 00:00:00',
  recurring_event INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  PRIMARY KEY  (timedeventqueue_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timedevents
--

CREATE TABLE  icinga_timedevents (
  timedevent_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  event_type INTEGER NOT NULL default 0,
  queued_time timestamp NOT NULL default '1970-01-01 00:00:00',
  queued_time_usec INTEGER NOT NULL default 0,
  event_time timestamp NOT NULL default '1970-01-01 00:00:00',
  event_time_usec INTEGER NOT NULL default 0,
  scheduled_time timestamp NOT NULL default '1970-01-01 00:00:00',
  recurring_event INTEGER NOT NULL default 0,
  object_id INTEGER NOT NULL default 0,
  deletion_time timestamp NOT NULL default '1970-01-01 00:00:00',
  deletion_time_usec INTEGER NOT NULL default 0,
  PRIMARY KEY  (timedevent_id),
  UNIQUE (instance_id,event_type,scheduled_time,object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timeperiods
--

CREATE TABLE  icinga_timeperiods (
  timeperiod_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  config_type INTEGER NOT NULL default 0,
  timeperiod_object_id INTEGER NOT NULL default 0,
  alias TEXT NOT NULL default '',
  PRIMARY KEY  (timeperiod_id),
  UNIQUE (instance_id,config_type,timeperiod_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timeperiod_timeranges
--

CREATE TABLE  icinga_timeperiod_timeranges (
  timeperiod_timerange_id SERIAL,
  instance_id INTEGER NOT NULL default 0,
  timeperiod_id INTEGER NOT NULL default 0,
  day INTEGER NOT NULL default 0,
  start_sec INTEGER NOT NULL default 0,
  end_sec INTEGER NOT NULL default 0,
  PRIMARY KEY  (timeperiod_timerange_id),
  UNIQUE (timeperiod_id,day,start_sec,end_sec)
) ;

-- -----------------------------------------
-- add index (delete)
-- -----------------------------------------

-- for periodic delete
-- instance_id and
-- TIMEDEVENTS => scheduled_time
-- SYSTEMCOMMANDS, SERVICECHECKS, HOSTCHECKS, EVENTHANDLERS  => start_time
-- EXTERNALCOMMANDS => entry_time

-- instance_id
CREATE INDEX timedevents_i_id_idx on icinga_timedevents(instance_id);
CREATE INDEX timedeventq_i_id_idx on icinga_timedeventqueue(instance_id);
CREATE INDEX systemcommands_i_id_idx on icinga_systemcommands(instance_id);
CREATE INDEX servicechecks_i_id_idx on icinga_servicechecks(instance_id);
CREATE INDEX hostchecks_i_id_idx on icinga_hostchecks(instance_id);
CREATE INDEX eventhandlers_i_id_idx on icinga_eventhandlers(instance_id);
CREATE INDEX externalcommands_i_id_idx on icinga_externalcommands(instance_id);

-- time
CREATE INDEX timedevents_time_id_idx on icinga_timedevents(scheduled_time);
CREATE INDEX timedeventq_time_id_idx on icinga_timedeventqueue(scheduled_time);
CREATE INDEX systemcommands_time_id_idx on icinga_systemcommands(start_time);
CREATE INDEX servicechecks_time_id_idx on icinga_servicechecks(start_time);
CREATE INDEX hostchecks_time_id_idx on icinga_hostchecks(start_time);
CREATE INDEX eventhandlers_time_id_idx on icinga_eventhandlers(start_time);
CREATE INDEX externalcommands_time_id_idx on icinga_externalcommands(entry_time);


-- for starting cleanup - referenced in dbhandler.c:882
-- instance_id only

-- realtime data
CREATE INDEX programstatus_i_id_idx on icinga_programstatus(instance_id);
CREATE INDEX hoststatus_i_id_idx on icinga_hoststatus(instance_id);
CREATE INDEX servicestatus_i_id_idx on icinga_servicestatus(instance_id);
CREATE INDEX contactstatus_i_id_idx on icinga_contactstatus(instance_id);
CREATE INDEX timedeventqueue_i_id_idx on icinga_timedeventqueue(instance_id);
CREATE INDEX comments_i_id_idx on icinga_comments(instance_id);
CREATE INDEX scheduleddowntime_i_id_idx on icinga_scheduleddowntime(instance_id);
CREATE INDEX runtimevariables_i_id_idx on icinga_runtimevariables(instance_id);
CREATE INDEX customvariablestatus_i_id_idx on icinga_customvariablestatus(instance_id);

-- config data
CREATE INDEX configfiles_i_id_idx on icinga_configfiles(instance_id);
CREATE INDEX configfilevariables_i_id_idx on icinga_configfilevariables(instance_id);
CREATE INDEX customvariables_i_id_idx on icinga_customvariables(instance_id);
CREATE INDEX commands_i_id_idx on icinga_commands(instance_id);
CREATE INDEX timeperiods_i_id_idx on icinga_timeperiods(instance_id);
CREATE INDEX timeperiod_timeranges_i_id_idx on icinga_timeperiod_timeranges(instance_id);
CREATE INDEX contactgroups_i_id_idx on icinga_contactgroups(instance_id);
CREATE INDEX contactgroup_members_i_id_idx on icinga_contactgroup_members(instance_id);
CREATE INDEX hostgroups_i_id_idx on icinga_hostgroups(instance_id);
CREATE INDEX hostgroup_members_i_id_idx on icinga_hostgroup_members(instance_id);
CREATE INDEX servicegroups_i_id_idx on icinga_servicegroups(instance_id);
CREATE INDEX servicegroup_members_i_id_idx on icinga_servicegroup_members(instance_id);
CREATE INDEX hostesc_i_id_idx on icinga_hostescalations(instance_id);
CREATE INDEX hostesc_contacts_i_id_idx on icinga_hostescalation_contacts(instance_id);
CREATE INDEX serviceesc_i_id_idx on icinga_serviceescalations(instance_id);
CREATE INDEX serviceesc_contacts_i_id_idx on icinga_serviceescalation_contacts(instance_id);
CREATE INDEX hostdependencies_i_id_idx on icinga_hostdependencies(instance_id);
CREATE INDEX contacts_i_id_idx on icinga_contacts(instance_id);
CREATE INDEX contact_addresses_i_id_idx on icinga_contact_addresses(instance_id);
CREATE INDEX contact_notifcommands_i_id_idx on icinga_contact_notificationcommands(instance_id);
CREATE INDEX hosts_i_id_idx on icinga_hosts(instance_id);
CREATE INDEX host_parenthosts_i_id_idx on icinga_host_parenthosts(instance_id);
CREATE INDEX host_contacts_i_id_idx on icinga_host_contacts(instance_id);
CREATE INDEX services_i_id_idx on icinga_services(instance_id);
CREATE INDEX service_contacts_i_id_idx on icinga_service_contacts(instance_id);
CREATE INDEX service_contactgroups_i_id_idx on icinga_service_contactgroups(instance_id);
CREATE INDEX host_contactgroups_i_id_idx on icinga_host_contactgroups(instance_id);
CREATE INDEX hostesc_cgroups_i_id_idx on icinga_hostescalation_contactgroups(instance_id);
CREATE INDEX serviceesc_cgroups_i_id_idx on icinga_serviceescalation_contactgroups(instance_id);

-- -----------------------------------------
-- more index stuff (WHERE clauses)
-- -----------------------------------------

-- hosts
CREATE INDEX hosts_host_object_id_idx on icinga_hosts(host_object_id);

-- hoststatus
CREATE INDEX hoststatus_stat_upd_time_idx on icinga_hoststatus(status_update_time);
CREATE INDEX hoststatus_current_state_idx on icinga_hoststatus(current_state);
CREATE INDEX hoststatus_check_type_idx on icinga_hoststatus(check_type);
CREATE INDEX hoststatus_state_type_idx on icinga_hoststatus(state_type);
CREATE INDEX hoststatus_last_state_chg_idx on icinga_hoststatus(last_state_change);
CREATE INDEX hoststatus_notif_enabled_idx on icinga_hoststatus(notifications_enabled);
CREATE INDEX hoststatus_problem_ack_idx on icinga_hoststatus(problem_has_been_acknowledged);
CREATE INDEX hoststatus_act_chks_en_idx on icinga_hoststatus(active_checks_enabled);
CREATE INDEX hoststatus_pas_chks_en_idx on icinga_hoststatus(passive_checks_enabled);
CREATE INDEX hoststatus_event_hdl_en_idx on icinga_hoststatus(event_handler_enabled);
CREATE INDEX hoststatus_flap_det_en_idx on icinga_hoststatus(flap_detection_enabled);
CREATE INDEX hoststatus_is_flapping_idx on icinga_hoststatus(is_flapping);
CREATE INDEX hoststatus_p_state_chg_idx on icinga_hoststatus(percent_state_change);
CREATE INDEX hoststatus_latency_idx on icinga_hoststatus(latency);
CREATE INDEX hoststatus_ex_time_idx on icinga_hoststatus(execution_time);
CREATE INDEX hoststatus_sch_downt_d_idx on icinga_hoststatus(scheduled_downtime_depth);

-- services
CREATE INDEX services_host_object_id_idx on icinga_services(host_object_id);

--servicestatus
CREATE INDEX srvcstatus_stat_upd_time_idx on icinga_servicestatus(status_update_time);
CREATE INDEX srvcstatus_current_state_idx on icinga_servicestatus(current_state);
CREATE INDEX srvcstatus_check_type_idx on icinga_servicestatus(check_type);
CREATE INDEX srvcstatus_state_type_idx on icinga_servicestatus(state_type);
CREATE INDEX srvcstatus_last_state_chg_idx on icinga_servicestatus(last_state_change);
CREATE INDEX srvcstatus_notif_enabled_idx on icinga_servicestatus(notifications_enabled);
CREATE INDEX srvcstatus_problem_ack_idx on icinga_servicestatus(problem_has_been_acknowledged);
CREATE INDEX srvcstatus_act_chks_en_idx on icinga_servicestatus(active_checks_enabled);
CREATE INDEX srvcstatus_pas_chks_en_idx on icinga_servicestatus(passive_checks_enabled);
CREATE INDEX srvcstatus_event_hdl_en_idx on icinga_servicestatus(event_handler_enabled);
CREATE INDEX srvcstatus_flap_det_en_idx on icinga_servicestatus(flap_detection_enabled);
CREATE INDEX srvcstatus_is_flapping_idx on icinga_servicestatus(is_flapping);
CREATE INDEX srvcstatus_p_state_chg_idx on icinga_servicestatus(percent_state_change);
CREATE INDEX srvcstatus_latency_idx on icinga_servicestatus(latency);
CREATE INDEX srvcstatus_ex_time_idx on icinga_servicestatus(execution_time);
CREATE INDEX srvcstatus_sch_downt_d_idx on icinga_servicestatus(scheduled_downtime_depth);

-- timedeventqueue
CREATE INDEX timed_e_q_event_type_idx on icinga_timedeventqueue(event_type);
CREATE INDEX timed_e_q_sched_time_idx on icinga_timedeventqueue(scheduled_time);
CREATE INDEX timed_e_q_object_id_idx on icinga_timedeventqueue(object_id);
CREATE INDEX timed_e_q_rec_ev_id_idx on icinga_timedeventqueue(recurring_event);

-- timedevents
CREATE INDEX timed_e_event_type_idx on icinga_timedevents(event_type);
--CREATE INDEX timed_e_sched_time_idx on icinga_timedevents(scheduled_time); --already set for delete
CREATE INDEX timed_e_object_id_idx on icinga_timedevents(object_id);
CREATE INDEX timed_e_rec_ev_idx on icinga_timedevents(recurring_event);

-- hostchecks
CREATE INDEX hostchks_h_obj_id_idx on icinga_hostchecks(host_object_id);

-- servicechecks
CREATE INDEX servicechks_s_obj_id_idx on icinga_servicechecks(service_object_id);

-- objects
CREATE INDEX objects_objtype_id_idx ON icinga_objects(objecttype_id);
CREATE INDEX objects_name1_idx ON icinga_objects(name1);
CREATE INDEX objects_name2_idx ON icinga_objects(name2);
CREATE INDEX objects_inst_id_idx ON icinga_objects(instance_id);


-- hostchecks
-- CREATE INDEX hostchks_h_obj_id_idx on icinga_hostchecks(host_object_id);

-- servicechecks
-- CREATE INDEX servicechks_s_obj_id_idx on icinga_servicechecks(service_object_id);


-- instances
-- CREATE INDEX instances_name_idx on icinga_instances(instance_name);

-- logentries
-- CREATE INDEX loge_instance_id_idx on icinga_logentries(instance_id);
-- #236
CREATE INDEX loge_time_idx on icinga_logentries(logentry_time);
-- CREATE INDEX loge_data_idx on icinga_logentries(logentry_data);

-- commenthistory
-- CREATE INDEX c_hist_instance_id_idx on icinga_logentries(instance_id);
-- CREATE INDEX c_hist_c_time_idx on icinga_logentries(comment_time);
-- CREATE INDEX c_hist_i_c_id_idx on icinga_logentries(internal_comment_id);

-- downtimehistory
-- CREATE INDEX d_t_hist_nstance_id_idx on icinga_downtimehistory(instance_id);
-- CREATE INDEX d_t_hist_type_idx on icinga_downtimehistory(downtime_type);
-- CREATE INDEX d_t_hist_object_id_idx on icinga_downtimehistory(object_id);
-- CREATE INDEX d_t_hist_entry_time_idx on icinga_downtimehistory(entry_time);
-- CREATE INDEX d_t_hist_sched_start_idx on icinga_downtimehistory(scheduled_start_time);
-- CREATE INDEX d_t_hist_sched_end_idx on icinga_downtimehistory(scheduled_end_time);

-- scheduleddowntime
-- CREATE INDEX sched_d_t_downtime_type_idx on icinga_scheduleddowntime(downtime_type);
-- CREATE INDEX sched_d_t_object_id_idx on icinga_scheduleddowntime(object_id);
-- CREATE INDEX sched_d_t_entry_time_idx on icinga_scheduleddowntime(entry_time);
-- CREATE INDEX sched_d_t_start_time_idx on icinga_scheduleddowntime(scheduled_start_time);
-- CREATE INDEX sched_d_t_end_time_idx on icinga_scheduleddowntime(scheduled_end_time);



