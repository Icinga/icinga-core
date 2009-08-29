/*
# pgsql.sql
#   - modfied mysql.sql to work with postgres 
#   - INSERT_OR_UPDATE function
#
# mm/2009-05-13
# mfriedrich/2009-07-18 
#
#-----------------------------------------------------
# database installation instructions
# 
# as user postgres: 
# 
# postgres=# CREATE USER icingauser;
# postgres=# CREATE DATABASE icinga;
# 
# pg_hba.conf 
#
# local   icinga      icingauser                        trust
#
#/etc/init.d/postgresql-8.3 reload
#
# psql -U icingauser -d icinga < pgsql.sql  
#
#
*/

-- IF EXISTS DROP DATABASE icinga;
-- CREATE DATABASE icinga;


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
  instance_id INTEGER NOT NULL default '0',
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time_usec INTEGER NOT NULL default '0',
  acknowledgement_type INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  state INTEGER NOT NULL default '0',
  author_name varchar(64) NOT NULL default '',
  comment_data varchar(255) NOT NULL default '',
  is_sticky INTEGER NOT NULL default '0',
  persistent_comment INTEGER NOT NULL default '0',
  notify_contacts INTEGER NOT NULL default '0',
  PRIMARY KEY  (acknowledgement_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_commands
--

CREATE TABLE  icinga_commands (
  command_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  command_line varchar(1024) NOT NULL default '',
--  PRIMARY KEY  (command_id)
  PRIMARY KEY  (command_id),
  UNIQUE (instance_id,object_id,config_type)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_commenthistory
--

CREATE TABLE  icinga_commenthistory (
  commenthistory_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time_usec INTEGER NOT NULL default '0',
  comment_type INTEGER NOT NULL default '0',
  entry_type INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  comment_time timestamp NOT NULL default '1970-01-01 00:00:00',
  internal_comment_id INTEGER NOT NULL default '0',
  author_name varchar(64) NOT NULL default '',
  comment_data varchar(255) NOT NULL default '',
  is_persistent INTEGER NOT NULL default '0',
  comment_source INTEGER NOT NULL default '0',
  expires INTEGER NOT NULL default '0',
  expiration_time timestamp NOT NULL default '1970-01-01 00:00:00',
  deletion_time timestamp NOT NULL default '1970-01-01 00:00:00',
  deletion_time_usec INTEGER NOT NULL default '0',
--  PRIMARY KEY  (commenthistory_id)
  PRIMARY KEY  (commenthistory_id),
  UNIQUE (instance_id,comment_time,internal_comment_id)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_comments
--

CREATE TABLE  icinga_comments (
  comment_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time_usec INTEGER NOT NULL default '0',
  comment_type INTEGER NOT NULL default '0',
  entry_type INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  comment_time timestamp NOT NULL default '1970-01-01 00:00:00',
  internal_comment_id INTEGER NOT NULL default '0',
  author_name varchar(64) NOT NULL default '',
  comment_data varchar(255) NOT NULL default '',
  is_persistent INTEGER NOT NULL default '0',
  comment_source INTEGER NOT NULL default '0',
  expires INTEGER NOT NULL default '0',
  expiration_time timestamp NOT NULL default '1970-01-01 00:00:00',
--  PRIMARY KEY  (comment_id)
  PRIMARY KEY  (comment_id),
  UNIQUE (instance_id,comment_time,internal_comment_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_configfiles
--

CREATE TABLE  icinga_configfiles (
  configfile_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  configfile_type INTEGER NOT NULL default '0',
  configfile_path varchar(255) NOT NULL default '',
--  PRIMARY KEY  (configfile_id)
  PRIMARY KEY  (configfile_id),
  UNIQUE (instance_id,configfile_type,configfile_path)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_configfilevariables
--

CREATE TABLE  icinga_configfilevariables (
  configfilevariable_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  configfile_id INTEGER NOT NULL default '0',
  varname varchar(64) NOT NULL default '',
  varvalue varchar(255) NOT NULL default '',
--  PRIMARY KEY  (configfilevariable_id)
  PRIMARY KEY  (configfilevariable_id),
  UNIQUE (instance_id,configfile_id,varname)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_conninfo
--

CREATE TABLE  icinga_conninfo (
  conninfo_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  agent_name varchar(32) NOT NULL default '',
  agent_version varchar(8) NOT NULL default '',
  disposition varchar(16) NOT NULL default '',
  connect_source varchar(16) NOT NULL default '',
  connect_type varchar(16) NOT NULL default '',
  connect_time timestamp NOT NULL default '1970-01-01 00:00:00',
  disconnect_time timestamp NOT NULL default '1970-01-01 00:00:00',
  last_checkin_time timestamp NOT NULL default '1970-01-01 00:00:00',
  data_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  data_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  bytes_processed INTEGER NOT NULL default '0',
  lines_processed INTEGER NOT NULL default '0',
  entries_processed INTEGER NOT NULL default '0',
  PRIMARY KEY  (conninfo_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactgroups
--

CREATE TABLE  icinga_contactgroups (
  contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  contactgroup_object_id INTEGER NOT NULL default '0',
  alias varchar(255) NOT NULL default '',
--  PRIMARY KEY  (contactgroup_id)
  PRIMARY KEY  (contactgroup_id),
  UNIQUE (instance_id,config_type,contactgroup_object_id)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactgroup_members
--

CREATE TABLE  icinga_contactgroup_members (
  contactgroup_member_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  contactgroup_id INTEGER NOT NULL default '0',
  contact_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (contactgroup_member_id)
  PRIMARY KEY  (contactgroup_member_id),
  UNIQUE (contactgroup_id,contact_object_id)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactnotificationmethods
--

CREATE TABLE  icinga_contactnotificationmethods (
  contactnotificationmethod_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  contactnotification_id INTEGER NOT NULL default '0',
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default '0',
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default '0',
  command_object_id INTEGER NOT NULL default '0',
  command_args varchar(255) NOT NULL default '',
--  PRIMARY KEY  (contactnotificationmethod_id)
  PRIMARY KEY  (contactnotificationmethod_id),
  UNIQUE (instance_id,contactnotification_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactnotifications
--

CREATE TABLE  icinga_contactnotifications (
  contactnotification_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  notification_id INTEGER NOT NULL default '0',
  contact_object_id INTEGER NOT NULL default '0',
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default '0',
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default '0',
--  PRIMARY KEY  (contactnotification_id)
  PRIMARY KEY  (contactnotification_id),
  UNIQUE (instance_id,contact_object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contacts
--

CREATE TABLE  icinga_contacts (
  contact_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  contact_object_id INTEGER NOT NULL default '0',
  alias varchar(64) NOT NULL default '',
  email_address varchar(255) NOT NULL default '',
  pager_address varchar(64) NOT NULL default '',
  host_timeperiod_object_id INTEGER NOT NULL default '0',
  service_timeperiod_object_id INTEGER NOT NULL default '0',
  host_notifications_enabled INTEGER NOT NULL default '0',
  service_notifications_enabled INTEGER NOT NULL default '0',
  can_submit_commands INTEGER NOT NULL default '0',
  notify_service_recovery INTEGER NOT NULL default '0',
  notify_service_warning INTEGER NOT NULL default '0',
  notify_service_unknown INTEGER NOT NULL default '0',
  notify_service_critical INTEGER NOT NULL default '0',
  notify_service_flapping INTEGER NOT NULL default '0',
  notify_service_downtime INTEGER NOT NULL default '0',
  notify_host_recovery INTEGER NOT NULL default '0',
  notify_host_down INTEGER NOT NULL default '0',
  notify_host_unreachable INTEGER NOT NULL default '0',
  notify_host_flapping INTEGER NOT NULL default '0',
  notify_host_downtime INTEGER NOT NULL default '0',
--  PRIMARY KEY  (contact_id)
  PRIMARY KEY  (contact_id),
  UNIQUE (instance_id,config_type,contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactstatus
--

CREATE TABLE  icinga_contactstatus (
  contactstatus_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  contact_object_id INTEGER NOT NULL default '0',
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  host_notifications_enabled INTEGER NOT NULL default '0',
  service_notifications_enabled INTEGER NOT NULL default '0',
  last_host_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  last_service_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  modified_attributes INTEGER NOT NULL default '0',
  modified_host_attributes INTEGER NOT NULL default '0',
  modified_service_attributes INTEGER NOT NULL default '0',
--  PRIMARY KEY  (contactstatus_id)
  PRIMARY KEY  (contactstatus_id),
  UNIQUE  (contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contact_addresses
--

CREATE TABLE  icinga_contact_addresses (
  contact_address_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  contact_id INTEGER NOT NULL default '0',
  address_number INTEGER NOT NULL default '0',
  address varchar(255) NOT NULL default '',
--  PRIMARY KEY  (contact_address_id)
  PRIMARY KEY  (contact_address_id),
  UNIQUE  (contact_id,address_number)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contact_notificationcommands
--

CREATE TABLE  icinga_contact_notificationcommands (
  contact_notificationcommand_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  contact_id INTEGER NOT NULL default '0',
  notification_type INTEGER NOT NULL default '0',
  command_object_id INTEGER NOT NULL default '0',
  command_args varchar(255) NOT NULL default '',
--  PRIMARY KEY  (contact_notificationcommand_id)
  PRIMARY KEY  (contact_notificationcommand_id),
  UNIQUE  (contact_id,notification_type,command_object_id,command_args)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_customvariables
--

CREATE TABLE  icinga_customvariables (
  customvariable_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  has_been_modified INTEGER NOT NULL default '0',
  varname varchar(255) NOT NULL default '',
  varvalue varchar(255) NOT NULL default '',
--  PRIMARY KEY  (customvariable_id)
  PRIMARY KEY  (customvariable_id),
  UNIQUE (object_id,config_type,varname)
--  PRIMARY KEY  (customvariable_id),
--  UNIQUE  (object_id,config_type,varname),
--  UNIQUE (varname)
) ;
CREATE INDEX icinga_customvariables_i ON icinga_customvariables(varname);

-- --------------------------------------------------------

--
-- Table structure for table icinga_customvariablestatus
--

CREATE TABLE  icinga_customvariablestatus (
  customvariablestatus_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  has_been_modified INTEGER NOT NULL default '0',
  varname varchar(255) NOT NULL default '',
  varvalue varchar(255) NOT NULL default '',
--  PRIMARY KEY  (customvariablestatus_id)
  PRIMARY KEY  (customvariablestatus_id),
  UNIQUE (object_id,varname)
--  PRIMARY KEY  (customvariablestatus_id),
--  UNIQUE (object_id,varname),
--  UNIQUE (varname)
) ;
CREATE INDEX icinga_customvariablestatus_i ON icinga_customvariablestatus(varname);


-- --------------------------------------------------------

--
-- Table structure for table icinga_dbversion
--

CREATE TABLE  icinga_dbversion (
  name varchar(10) NOT NULL default '',
  version varchar(10) NOT NULL default ''
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_downtimehistory
--

CREATE TABLE  icinga_downtimehistory (
  downtimehistory_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  downtime_type INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  author_name varchar(64) NOT NULL default '',
  comment_data varchar(255) NOT NULL default '',
  internal_downtime_id INTEGER NOT NULL default '0',
  triggered_by_id INTEGER NOT NULL default '0',
  is_fixed INTEGER NOT NULL default '0',
  duration INTEGER NOT NULL default '0',
  scheduled_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  scheduled_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  was_started INTEGER NOT NULL default '0',
  actual_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  actual_start_time_usec INTEGER NOT NULL default '0',
  actual_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  actual_end_time_usec INTEGER NOT NULL default '0',
  was_cancelled INTEGER NOT NULL default '0',
--  PRIMARY KEY  (downtimehistory_id)
  PRIMARY KEY  (downtimehistory_id),
  UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_eventhandlers
--

CREATE TABLE  icinga_eventhandlers (
  eventhandler_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  eventhandler_type INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  state INTEGER NOT NULL default '0',
  state_type INTEGER NOT NULL default '0',
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default '0',
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default '0',
  command_object_id INTEGER NOT NULL default '0',
  command_args varchar(255) NOT NULL default '',
  command_line varchar(255) NOT NULL default '',
  timeout INTEGER NOT NULL default '0',
  early_timeout INTEGER NOT NULL default '0',
  execution_time double precision NOT NULL default '0',
  return_code INTEGER NOT NULL default '0',
  output varchar(255) NOT NULL default '',
  long_output varchar(8192) NOT NULL default '',
--  PRIMARY KEY  (eventhandler_id)
  PRIMARY KEY  (eventhandler_id),
  UNIQUE (instance_id,object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_externalcommands
--

CREATE TABLE  icinga_externalcommands (
  externalcommand_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  command_type INTEGER NOT NULL default '0',
  command_name varchar(128) NOT NULL default '',
  command_args varchar(255) NOT NULL default '',
  PRIMARY KEY  (externalcommand_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_flappinghistory
--

CREATE TABLE  icinga_flappinghistory (
  flappinghistory_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  event_time timestamp NOT NULL default '1970-01-01 00:00:00',
  event_time_usec INTEGER NOT NULL default '0',
  event_type INTEGER NOT NULL default '0',
  reason_type INTEGER NOT NULL default '0',
  flapping_type INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  percent_state_change double precision NOT NULL default '0',
  low_threshold double precision NOT NULL default '0',
  high_threshold double precision NOT NULL default '0',
  comment_time timestamp NOT NULL default '1970-01-01 00:00:00',
  internal_comment_id INTEGER NOT NULL default '0',
  PRIMARY KEY  (flappinghistory_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostchecks
--

CREATE TABLE  icinga_hostchecks (
  hostcheck_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  host_object_id INTEGER NOT NULL default '0',
  check_type INTEGER NOT NULL default '0',
  is_raw_check INTEGER NOT NULL default '0',
  current_check_attempt INTEGER NOT NULL default '0',
  max_check_attempts INTEGER NOT NULL default '0',
  state INTEGER NOT NULL default '0',
  state_type INTEGER NOT NULL default '0',
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default '0',
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default '0',
  command_object_id INTEGER NOT NULL default '0',
  command_args varchar(255) NOT NULL default '',
  command_line varchar(255) NOT NULL default '',
  timeout INTEGER NOT NULL default '0',
  early_timeout INTEGER NOT NULL default '0',
  execution_time double precision NOT NULL default '0',
  latency double precision NOT NULL default '0',
  return_code INTEGER NOT NULL default '0',
  output varchar(255) NOT NULL default '',
  long_output varchar(8192) NOT NULL default '',
  perfdata varchar(255) NOT NULL default '',
--  PRIMARY KEY  (hostcheck_id)
  PRIMARY KEY  (hostcheck_id),
  UNIQUE (instance_id,host_object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostdependencies
--

CREATE TABLE  icinga_hostdependencies (
  hostdependency_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  host_object_id INTEGER NOT NULL default '0',
  dependent_host_object_id INTEGER NOT NULL default '0',
  dependency_type INTEGER NOT NULL default '0',
  inherits_parent INTEGER NOT NULL default '0',
  timeperiod_object_id INTEGER NOT NULL default '0',
  fail_on_up INTEGER NOT NULL default '0',
  fail_on_down INTEGER NOT NULL default '0',
  fail_on_unreachable INTEGER NOT NULL default '0',
--  PRIMARY KEY  (hostdependency_id)
  PRIMARY KEY  (hostdependency_id),
  UNIQUE (instance_id,config_type,host_object_id,dependent_host_object_id,dependency_type,inherits_parent,fail_on_up,fail_on_down,fail_on_unreachable)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalations
--

CREATE TABLE  icinga_hostescalations (
  hostescalation_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  host_object_id INTEGER NOT NULL default '0',
  timeperiod_object_id INTEGER NOT NULL default '0',
  first_notification INTEGER NOT NULL default '0',
  last_notification INTEGER NOT NULL default '0',
  notification_interval double precision NOT NULL default '0',
  escalate_on_recovery INTEGER NOT NULL default '0',
  escalate_on_down INTEGER NOT NULL default '0',
  escalate_on_unreachable INTEGER NOT NULL default '0',
--  PRIMARY KEY  (hostescalation_id)
  PRIMARY KEY  (hostescalation_id),
  UNIQUE (instance_id,config_type,host_object_id,timeperiod_object_id,first_notification,last_notification)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalation_contactgroups
--

CREATE TABLE  icinga_hostescalation_contactgroups (
  hostescalation_contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  hostescalation_id INTEGER NOT NULL default '0',
  contactgroup_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (hostescalation_contactgroup_id)
  PRIMARY KEY  (hostescalation_contactgroup_id),
  UNIQUE (hostescalation_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalation_contacts
--

CREATE TABLE  icinga_hostescalation_contacts (
  hostescalation_contact_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  hostescalation_id INTEGER NOT NULL default '0',
  contact_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (hostescalation_contact_id)
  PRIMARY KEY  (hostescalation_contact_id),
  UNIQUE (instance_id,hostescalation_id,contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostgroups
--

CREATE TABLE  icinga_hostgroups (
  hostgroup_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  hostgroup_object_id INTEGER NOT NULL default '0',
  alias varchar(255) NOT NULL default '',
--  PRIMARY KEY  (hostgroup_id)
  PRIMARY KEY  (hostgroup_id),
  UNIQUE (instance_id,hostgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostgroup_members
--

CREATE TABLE  icinga_hostgroup_members (
  hostgroup_member_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  hostgroup_id INTEGER NOT NULL default '0',
  host_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (hostgroup_member_id)
  PRIMARY KEY  (hostgroup_member_id),
  UNIQUE (hostgroup_id,host_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hosts
--

CREATE TABLE  icinga_hosts (
  host_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  host_object_id INTEGER NOT NULL default '0',
  alias varchar(64) NOT NULL default '',
  display_name varchar(64) NOT NULL default '',
  address varchar(128) NOT NULL default '',
  check_command_object_id INTEGER NOT NULL default '0',
  check_command_args varchar(255) NOT NULL default '',
  eventhandler_command_object_id INTEGER NOT NULL default '0',
  eventhandler_command_args varchar(255) NOT NULL default '',
  notification_timeperiod_object_id INTEGER NOT NULL default '0',
  check_timeperiod_object_id INTEGER NOT NULL default '0',
  failure_prediction_options varchar(64) NOT NULL default '',
  check_interval double precision NOT NULL default '0',
  retry_interval double precision NOT NULL default '0',
  max_check_attempts INTEGER NOT NULL default '0',
  first_notification_delay double precision NOT NULL default '0',
  notification_interval double precision NOT NULL default '0',
  notify_on_down INTEGER NOT NULL default '0',
  notify_on_unreachable INTEGER NOT NULL default '0',
  notify_on_recovery INTEGER NOT NULL default '0',
  notify_on_flapping INTEGER NOT NULL default '0',
  notify_on_downtime INTEGER NOT NULL default '0',
  stalk_on_up INTEGER NOT NULL default '0',
  stalk_on_down INTEGER NOT NULL default '0',
  stalk_on_unreachable INTEGER NOT NULL default '0',
  flap_detection_enabled INTEGER NOT NULL default '0',
  flap_detection_on_up INTEGER NOT NULL default '0',
  flap_detection_on_down INTEGER NOT NULL default '0',
  flap_detection_on_unreachable INTEGER NOT NULL default '0',
  low_flap_threshold double precision NOT NULL default '0',
  high_flap_threshold double precision NOT NULL default '0',
  process_performance_data INTEGER NOT NULL default '0',
  freshness_checks_enabled INTEGER NOT NULL default '0',
  freshness_threshold INTEGER NOT NULL default '0',
  passive_checks_enabled INTEGER NOT NULL default '0',
  event_handler_enabled INTEGER NOT NULL default '0',
  active_checks_enabled INTEGER NOT NULL default '0',
  retain_status_information INTEGER NOT NULL default '0',
  retain_nonstatus_information INTEGER NOT NULL default '0',
  notifications_enabled INTEGER NOT NULL default '0',
  obsess_over_host INTEGER NOT NULL default '0',
  failure_prediction_enabled INTEGER NOT NULL default '0',
  notes varchar(255) NOT NULL default '',
  notes_url varchar(255) NOT NULL default '',
  action_url varchar(255) NOT NULL default '',
  icon_image varchar(255) NOT NULL default '',
  icon_image_alt varchar(255) NOT NULL default '',
  vrml_image varchar(255) NOT NULL default '',
  statusmap_image varchar(255) NOT NULL default '',
  have_2d_coords INTEGER NOT NULL default '0',
  x_2d INTEGER NOT NULL default '0',
  y_2d INTEGER NOT NULL default '0',
  have_3d_coords INTEGER NOT NULL default '0',
  x_3d double precision NOT NULL default '0',
  y_3d double precision NOT NULL default '0',
  z_3d double precision NOT NULL default '0',
--  PRIMARY KEY  (host_id)
  PRIMARY KEY  (host_id),
  UNIQUE (instance_id,config_type,host_object_id)
--  UNIQUE (instance_id,config_type,host_object_id),
--  UNIQUE (host_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hoststatus
--

CREATE TABLE  icinga_hoststatus (
  hoststatus_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  host_object_id INTEGER NOT NULL default '0',
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  output varchar(255) NOT NULL default '',
  long_output varchar(8192) NOT NULL default '',
  perfdata varchar(255) NOT NULL default '',
  current_state INTEGER NOT NULL default '0',
  has_been_checked INTEGER NOT NULL default '0',
  should_be_scheduled INTEGER NOT NULL default '0',
  current_check_attempt INTEGER NOT NULL default '0',
  max_check_attempts INTEGER NOT NULL default '0',
  last_check timestamp NOT NULL default '1970-01-01 00:00:00',
  next_check timestamp NOT NULL default '1970-01-01 00:00:00',
  check_type INTEGER NOT NULL default '0',
  last_state_change timestamp NOT NULL default '1970-01-01 00:00:00',
  last_hard_state_change timestamp NOT NULL default '1970-01-01 00:00:00',
  last_hard_state INTEGER NOT NULL default '0',
  last_time_up timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_down timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_unreachable timestamp NOT NULL default '1970-01-01 00:00:00',
  state_type INTEGER NOT NULL default '0',
  last_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  next_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  no_more_notifications INTEGER NOT NULL default '0',
  notifications_enabled INTEGER NOT NULL default '0',
  problem_has_been_acknowledged INTEGER NOT NULL default '0',
  acknowledgement_type INTEGER NOT NULL default '0',
  current_notification_number INTEGER NOT NULL default '0',
  passive_checks_enabled INTEGER NOT NULL default '0',
  active_checks_enabled INTEGER NOT NULL default '0',
  event_handler_enabled INTEGER NOT NULL default '0',
  flap_detection_enabled INTEGER NOT NULL default '0',
  is_flapping INTEGER NOT NULL default '0',
  percent_state_change double precision NOT NULL default '0',
  latency double precision NOT NULL default '0',
  execution_time double precision NOT NULL default '0',
  scheduled_downtime_depth INTEGER NOT NULL default '0',
  failure_prediction_enabled INTEGER NOT NULL default '0',
  process_performance_data INTEGER NOT NULL default '0',
  obsess_over_host INTEGER NOT NULL default '0',
  modified_host_attributes INTEGER NOT NULL default '0',
  event_handler varchar(255) NOT NULL default '',
  check_command varchar(255) NOT NULL default '',
  normal_check_interval double precision NOT NULL default '0',
  retry_check_interval double precision NOT NULL default '0',
  check_timeperiod_object_id INTEGER NOT NULL default '0',
  PRIMARY KEY  (hoststatus_id),
  UNIQUE (host_object_id)
--  PRIMARY KEY  (hoststatus_id),
--  UNIQUE (host_object_id),
--  UNIQUE (instance_id),
--  UNIQUE (status_update_time),
--  UNIQUE (current_state),
--  UNIQUE (check_type),
--  UNIQUE (state_type),
--  UNIQUE (last_state_change),
--  UNIQUE (notifications_enabled),
--  UNIQUE (problem_has_been_acknowledged),
--  UNIQUE (active_checks_enabled),
--  UNIQUE (passive_checks_enabled),
--  UNIQUE (event_handler_enabled),
--  UNIQUE (flap_detection_enabled),
--  UNIQUE (is_flapping),
--  UNIQUE (percent_state_change),
--  UNIQUE (latency),
--  UNIQUE (execution_time),
--  UNIQUE (scheduled_downtime_depth)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_contactgroups
--

CREATE TABLE  icinga_host_contactgroups (
  host_contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  host_id INTEGER NOT NULL default '0',
  contactgroup_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (host_contactgroup_id)
  PRIMARY KEY  (host_contactgroup_id),
  UNIQUE (host_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_contacts
--

CREATE TABLE  icinga_host_contacts (
  host_contact_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  host_id INTEGER NOT NULL default '0',
  contact_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (host_contact_id)
  PRIMARY KEY  (host_contact_id),
  UNIQUE (instance_id,host_id,contact_object_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_parenthosts
--

CREATE TABLE  icinga_host_parenthosts (
  host_parenthost_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  host_id INTEGER NOT NULL default '0',
  parent_host_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (host_parenthost_id)
  PRIMARY KEY  (host_parenthost_id),
  UNIQUE (host_id,parent_host_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_instances
--

CREATE TABLE  icinga_instances (
  instance_id SERIAL,
  instance_name varchar(64) NOT NULL default '',
  instance_description varchar(128) NOT NULL default '',
  PRIMARY KEY  (instance_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_logentries
--

CREATE TABLE  icinga_logentries (
  logentry_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  logentry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  entry_time_usec INTEGER NOT NULL default '0',
  logentry_type INTEGER NOT NULL default '0',
  logentry_data varchar(255) NOT NULL default '',
  realtime_data INTEGER NOT NULL default '0',
  inferred_data_extracted INTEGER NOT NULL default '0',
  PRIMARY KEY  (logentry_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_notifications
--

CREATE TABLE  icinga_notifications (
  notification_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  notification_type INTEGER NOT NULL default '0',
  notification_reason INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default '0',
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default '0',
  state INTEGER NOT NULL default '0',
  output varchar(255) NOT NULL default '',
  long_output varchar(8192) NOT NULL default '',
  escalated INTEGER NOT NULL default '0',
  contacts_notified INTEGER NOT NULL default '0',
--  PRIMARY KEY  (notification_id)
  PRIMARY KEY  (notification_id),
  UNIQUE (instance_id,object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_objects
--

CREATE TABLE  icinga_objects (
  object_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  objecttype_id INTEGER NOT NULL default '0',
  name1 varchar(128),
  name2 varchar(128),
  is_active INTEGER NOT NULL default '0',
  PRIMARY KEY  (object_id)
--  PRIMARY KEY  (object_id),
--  UNIQUE (objecttype_id,name1,name2)
) ;
CREATE INDEX icinga_objects_i ON icinga_objects(objecttype_id,name1,name2);

-- --------------------------------------------------------

--
-- Table structure for table icinga_processevents
--

CREATE TABLE  icinga_processevents (
  processevent_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  event_type INTEGER NOT NULL default '0',
  event_time timestamp NOT NULL default '1970-01-01 00:00:00',
  event_time_usec INTEGER NOT NULL default '0',
  process_id INTEGER NOT NULL default '0',
  program_name varchar(16) NOT NULL default '',
  program_version varchar(20) NOT NULL default '',
  program_date varchar(10) NOT NULL default '',
  PRIMARY KEY  (processevent_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_programstatus
--

CREATE TABLE  icinga_programstatus (
  programstatus_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  program_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  program_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  is_currently_running INTEGER NOT NULL default '0',
  process_id INTEGER NOT NULL default '0',
  daemon_mode INTEGER NOT NULL default '0',
  last_command_check timestamp NOT NULL default '1970-01-01 00:00:00',
  last_log_rotation timestamp NOT NULL default '1970-01-01 00:00:00',
  notifications_enabled INTEGER NOT NULL default '0',
  active_service_checks_enabled INTEGER NOT NULL default '0',
  passive_service_checks_enabled INTEGER NOT NULL default '0',
  active_host_checks_enabled INTEGER NOT NULL default '0',
  passive_host_checks_enabled INTEGER NOT NULL default '0',
  event_handlers_enabled INTEGER NOT NULL default '0',
  flap_detection_enabled INTEGER NOT NULL default '0',
  failure_prediction_enabled INTEGER NOT NULL default '0',
  process_performance_data INTEGER NOT NULL default '0',
  obsess_over_hosts INTEGER NOT NULL default '0',
  obsess_over_services INTEGER NOT NULL default '0',
  modified_host_attributes INTEGER NOT NULL default '0',
  modified_service_attributes INTEGER NOT NULL default '0',
  global_host_event_handler varchar(255) NOT NULL default '',
  global_service_event_handler varchar(255) NOT NULL default '',
--  PRIMARY KEY  (programstatus_id)
  PRIMARY KEY  (programstatus_id),
  UNIQUE (instance_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_runtimevariables
--

CREATE TABLE  icinga_runtimevariables (
  runtimevariable_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  varname varchar(64) NOT NULL default '',
  varvalue varchar(255) NOT NULL default '',
--  PRIMARY KEY  (runtimevariable_id)
  PRIMARY KEY  (runtimevariable_id),
  UNIQUE (instance_id,varname)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_scheduleddowntime
--

CREATE TABLE  icinga_scheduleddowntime (
  scheduleddowntime_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  downtime_type INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  entry_time timestamp NOT NULL default '1970-01-01 00:00:00',
  author_name varchar(64) NOT NULL default '',
  comment_data varchar(255) NOT NULL default '',
  internal_downtime_id INTEGER NOT NULL default '0',
  triggered_by_id INTEGER NOT NULL default '0',
  is_fixed INTEGER NOT NULL default '0',
  duration INTEGER NOT NULL default '0',
  scheduled_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  scheduled_end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  was_started INTEGER NOT NULL default '0',
  actual_start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  actual_start_time_usec INTEGER NOT NULL default '0',
--  PRIMARY KEY  (scheduleddowntime_id)
  PRIMARY KEY  (scheduleddowntime_id),
  UNIQUE (instance_id,object_id,entry_time,internal_downtime_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicechecks
--

CREATE TABLE  icinga_servicechecks (
  servicecheck_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  service_object_id INTEGER NOT NULL default '0',
  check_type INTEGER NOT NULL default '0',
  current_check_attempt INTEGER NOT NULL default '0',
  max_check_attempts INTEGER NOT NULL default '0',
  state INTEGER NOT NULL default '0',
  state_type INTEGER NOT NULL default '0',
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default '0',
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default '0',
  command_object_id INTEGER NOT NULL default '0',
  command_args varchar(255) NOT NULL default '',
  command_line varchar(255) NOT NULL default '',
  timeout INTEGER NOT NULL default '0',
  early_timeout INTEGER NOT NULL default '0',
  execution_time double precision NOT NULL default '0',
  latency double precision NOT NULL default '0',
  return_code INTEGER NOT NULL default '0',
  output varchar(255) NOT NULL default '',
  long_output varchar(8192) NOT NULL default '',
  perfdata varchar(255) NOT NULL default '',
--  PRIMARY KEY  (servicecheck_id)
  PRIMARY KEY  (servicecheck_id),
  UNIQUE (instance_id,service_object_id,start_time,start_time_usec)
--  UNIQUE (instance_id),
--  UNIQUE (service_object_id),
--  UNIQUE (start_time)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicedependencies
--

CREATE TABLE  icinga_servicedependencies (
  servicedependency_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  service_object_id INTEGER NOT NULL default '0',
  dependent_service_object_id INTEGER NOT NULL default '0',
  dependency_type INTEGER NOT NULL default '0',
  inherits_parent INTEGER NOT NULL default '0',
  timeperiod_object_id INTEGER NOT NULL default '0',
  fail_on_ok INTEGER NOT NULL default '0',
  fail_on_warning INTEGER NOT NULL default '0',
  fail_on_unknown INTEGER NOT NULL default '0',
  fail_on_critical INTEGER NOT NULL default '0',
--  PRIMARY KEY  (servicedependency_id)
  PRIMARY KEY  (servicedependency_id),
  UNIQUE (instance_id,config_type,service_object_id,dependent_service_object_id,dependency_type,inherits_parent,fail_on_ok,fail_on_warning,fail_on_unknown,fail_on_critical)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalations
--

CREATE TABLE  icinga_serviceescalations (
  serviceescalation_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  service_object_id INTEGER NOT NULL default '0',
  timeperiod_object_id INTEGER NOT NULL default '0',
  first_notification INTEGER NOT NULL default '0',
  last_notification INTEGER NOT NULL default '0',
  notification_interval double precision NOT NULL default '0',
  escalate_on_recovery INTEGER NOT NULL default '0',
  escalate_on_warning INTEGER NOT NULL default '0',
  escalate_on_unknown INTEGER NOT NULL default '0',
  escalate_on_critical INTEGER NOT NULL default '0',
--  PRIMARY KEY  (serviceescalation_id)
  PRIMARY KEY  (serviceescalation_id),
  UNIQUE (instance_id,config_type,service_object_id,timeperiod_object_id,first_notification,last_notification)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalation_contactgroups
--

CREATE TABLE  icinga_serviceescalation_contactgroups (
  serviceescalation_contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  serviceescalation_id INTEGER NOT NULL default '0',
  contactgroup_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (serviceescalation_contactgroup_id)
  PRIMARY KEY  (serviceescalation_contactgroup_id),
  UNIQUE (serviceescalation_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalation_contacts
--

CREATE TABLE  icinga_serviceescalation_contacts (
  serviceescalation_contact_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  serviceescalation_id INTEGER NOT NULL default '0',
  contact_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (serviceescalation_contact_id)
  PRIMARY KEY  (serviceescalation_contact_id),
  UNIQUE (instance_id,serviceescalation_id,contact_object_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicegroups
--

CREATE TABLE  icinga_servicegroups (
  servicegroup_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  servicegroup_object_id INTEGER NOT NULL default '0',
  alias varchar(255) NOT NULL default '',
--  PRIMARY KEY  (servicegroup_id)
  PRIMARY KEY  (servicegroup_id),
  UNIQUE (instance_id,config_type,servicegroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicegroup_members
--

CREATE TABLE  icinga_servicegroup_members (
  servicegroup_member_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  servicegroup_id INTEGER NOT NULL default '0',
  service_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (servicegroup_member_id)
  PRIMARY KEY  (servicegroup_member_id),
  UNIQUE (servicegroup_id,service_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_services
--

CREATE TABLE  icinga_services (
  service_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  host_object_id INTEGER NOT NULL default '0',
  service_object_id INTEGER NOT NULL default '0',
  display_name varchar(64) NOT NULL default '',
  check_command_object_id INTEGER NOT NULL default '0',
  check_command_args varchar(255) NOT NULL default '',
  eventhandler_command_object_id INTEGER NOT NULL default '0',
  eventhandler_command_args varchar(255) NOT NULL default '',
  notification_timeperiod_object_id INTEGER NOT NULL default '0',
  check_timeperiod_object_id INTEGER NOT NULL default '0',
  failure_prediction_options varchar(64) NOT NULL default '',
  check_interval double precision NOT NULL default '0',
  retry_interval double precision NOT NULL default '0',
  max_check_attempts INTEGER NOT NULL default '0',
  first_notification_delay double precision NOT NULL default '0',
  notification_interval double precision NOT NULL default '0',
  notify_on_warning INTEGER NOT NULL default '0',
  notify_on_unknown INTEGER NOT NULL default '0',
  notify_on_critical INTEGER NOT NULL default '0',
  notify_on_recovery INTEGER NOT NULL default '0',
  notify_on_flapping INTEGER NOT NULL default '0',
  notify_on_downtime INTEGER NOT NULL default '0',
  stalk_on_ok INTEGER NOT NULL default '0',
  stalk_on_warning INTEGER NOT NULL default '0',
  stalk_on_unknown INTEGER NOT NULL default '0',
  stalk_on_critical INTEGER NOT NULL default '0',
  is_volatile INTEGER NOT NULL default '0',
  flap_detection_enabled INTEGER NOT NULL default '0',
  flap_detection_on_ok INTEGER NOT NULL default '0',
  flap_detection_on_warning INTEGER NOT NULL default '0',
  flap_detection_on_unknown INTEGER NOT NULL default '0',
  flap_detection_on_critical INTEGER NOT NULL default '0',
  low_flap_threshold double precision NOT NULL default '0',
  high_flap_threshold double precision NOT NULL default '0',
  process_performance_data INTEGER NOT NULL default '0',
  freshness_checks_enabled INTEGER NOT NULL default '0',
  freshness_threshold INTEGER NOT NULL default '0',
  passive_checks_enabled INTEGER NOT NULL default '0',
  event_handler_enabled INTEGER NOT NULL default '0',
  active_checks_enabled INTEGER NOT NULL default '0',
  retain_status_information INTEGER NOT NULL default '0',
  retain_nonstatus_information INTEGER NOT NULL default '0',
  notifications_enabled INTEGER NOT NULL default '0',
  obsess_over_service INTEGER NOT NULL default '0',
  failure_prediction_enabled INTEGER NOT NULL default '0',
  notes varchar(255) NOT NULL default '',
  notes_url varchar(255) NOT NULL default '',
  action_url varchar(255) NOT NULL default '',
  icon_image varchar(255) NOT NULL default '',
  icon_image_alt varchar(255) NOT NULL default '',
--  PRIMARY KEY  (service_id)
  PRIMARY KEY  (service_id),
  UNIQUE (instance_id,config_type,service_object_id)
--  UNIQUE (instance_id,config_type,service_object_id),
--  UNIQUE (service_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicestatus
--

CREATE TABLE  icinga_servicestatus (
  servicestatus_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  service_object_id INTEGER NOT NULL default '0',
  status_update_time timestamp NOT NULL default '1970-01-01 00:00:00',
  output varchar(255) NOT NULL default '',
  long_output varchar(8192) NOT NULL default '',
  perfdata varchar(255) NOT NULL default '',
  current_state INTEGER NOT NULL default '0',
  has_been_checked INTEGER NOT NULL default '0',
  should_be_scheduled INTEGER NOT NULL default '0',
  current_check_attempt INTEGER NOT NULL default '0',
  max_check_attempts INTEGER NOT NULL default '0',
  last_check timestamp NOT NULL default '1970-01-01 00:00:00',
  next_check timestamp NOT NULL default '1970-01-01 00:00:00',
  check_type INTEGER NOT NULL default '0',
  last_state_change timestamp NOT NULL default '1970-01-01 00:00:00',
  last_hard_state_change timestamp NOT NULL default '1970-01-01 00:00:00',
  last_hard_state INTEGER NOT NULL default '0',
  last_time_ok timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_warning timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_unknown timestamp NOT NULL default '1970-01-01 00:00:00',
  last_time_critical timestamp NOT NULL default '1970-01-01 00:00:00',
  state_type INTEGER NOT NULL default '0',
  last_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  next_notification timestamp NOT NULL default '1970-01-01 00:00:00',
  no_more_notifications INTEGER NOT NULL default '0',
  notifications_enabled INTEGER NOT NULL default '0',
  problem_has_been_acknowledged INTEGER NOT NULL default '0',
  acknowledgement_type INTEGER NOT NULL default '0',
  current_notification_number INTEGER NOT NULL default '0',
  passive_checks_enabled INTEGER NOT NULL default '0',
  active_checks_enabled INTEGER NOT NULL default '0',
  event_handler_enabled INTEGER NOT NULL default '0',
  flap_detection_enabled INTEGER NOT NULL default '0',
  is_flapping INTEGER NOT NULL default '0',
  percent_state_change double precision NOT NULL default '0',
  latency double precision NOT NULL default '0',
  execution_time double precision NOT NULL default '0',
  scheduled_downtime_depth INTEGER NOT NULL default '0',
  failure_prediction_enabled INTEGER NOT NULL default '0',
  process_performance_data INTEGER NOT NULL default '0',
  obsess_over_service INTEGER NOT NULL default '0',
  modified_service_attributes INTEGER NOT NULL default '0',
  event_handler varchar(255) NOT NULL default '',
  check_command varchar(255) NOT NULL default '',
  normal_check_interval double precision NOT NULL default '0',
  retry_check_interval double precision NOT NULL default '0',
  check_timeperiod_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (servicestatus_id)
  PRIMARY KEY  (servicestatus_id),
  UNIQUE (service_object_id)
--  UNIQUE (service_object_id),
--  UNIQUE (instance_id),
--  UNIQUE (status_update_time),
--  UNIQUE (current_state),
--  UNIQUE (check_type),
--  UNIQUE (state_type),
--  UNIQUE (last_state_change),
--  UNIQUE (notifications_enabled),
--  UNIQUE (problem_has_been_acknowledged),
--  UNIQUE (active_checks_enabled),
--  UNIQUE (passive_checks_enabled),
--  UNIQUE (event_handler_enabled),
--  UNIQUE (flap_detection_enabled),
--  UNIQUE (is_flapping),
--  UNIQUE (percent_state_change),
--  UNIQUE (latency),
--  UNIQUE (execution_time),
--  UNIQUE (scheduled_downtime_depth)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_service_contactgroups
--

CREATE TABLE  icinga_service_contactgroups (
  service_contactgroup_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  service_id INTEGER NOT NULL default '0',
  contactgroup_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (service_contactgroup_id)
  PRIMARY KEY  (service_contactgroup_id),
  UNIQUE (service_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_service_contacts
--

CREATE TABLE  icinga_service_contacts (
  service_contact_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  service_id INTEGER NOT NULL default '0',
  contact_object_id INTEGER NOT NULL default '0',
--  PRIMARY KEY  (service_contact_id)
  PRIMARY KEY  (service_contact_id),
  UNIQUE (instance_id,service_id,contact_object_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_statehistory
--

CREATE TABLE  icinga_statehistory (
  statehistory_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  state_time timestamp NOT NULL default '1970-01-01 00:00:00',
  state_time_usec INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  state_change INTEGER NOT NULL default '0',
  state INTEGER NOT NULL default '0',
  state_type INTEGER NOT NULL default '0',
  current_check_attempt INTEGER NOT NULL default '0',
  max_check_attempts INTEGER NOT NULL default '0',
  last_state INTEGER NOT NULL default '-1',
  last_hard_state INTEGER NOT NULL default '-1',
  output varchar(255) NOT NULL default '',
  long_output varchar(8192) NOT NULL default '',
  PRIMARY KEY  (statehistory_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_systemcommands
--

CREATE TABLE  icinga_systemcommands (
  systemcommand_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  start_time timestamp NOT NULL default '1970-01-01 00:00:00',
  start_time_usec INTEGER NOT NULL default '0',
  end_time timestamp NOT NULL default '1970-01-01 00:00:00',
  end_time_usec INTEGER NOT NULL default '0',
  command_line varchar(255) NOT NULL default '',
  timeout INTEGER NOT NULL default '0',
  early_timeout INTEGER NOT NULL default '0',
  execution_time double precision NOT NULL default '0',
  return_code INTEGER NOT NULL default '0',
  long_output varchar(8192) NOT NULL default '',
--  PRIMARY KEY  (systemcommand_id)
  PRIMARY KEY  (systemcommand_id),
  UNIQUE (instance_id,start_time,start_time_usec)
--  UNIQUE (instance_id),
--  UNIQUE (start_time)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timedeventqueue
--

CREATE TABLE  icinga_timedeventqueue (
  timedeventqueue_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  event_type INTEGER NOT NULL default '0',
  queued_time timestamp NOT NULL default '1970-01-01 00:00:00',
  queued_time_usec INTEGER NOT NULL default '0',
  scheduled_time timestamp NOT NULL default '1970-01-01 00:00:00',
  recurring_event INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  PRIMARY KEY  (timedeventqueue_id)
--  PRIMARY KEY  (timedeventqueue_id),
--  UNIQUE (instance_id),
--  UNIQUE (event_type),
--  UNIQUE (scheduled_time),
--  UNIQUE (object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timedevents
--

CREATE TABLE  icinga_timedevents (
  timedevent_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  event_type INTEGER NOT NULL default '0',
  queued_time timestamp NOT NULL default '1970-01-01 00:00:00',
  queued_time_usec INTEGER NOT NULL default '0',
  event_time timestamp NOT NULL default '1970-01-01 00:00:00',
  event_time_usec INTEGER NOT NULL default '0',
  scheduled_time timestamp NOT NULL default '1970-01-01 00:00:00',
  recurring_event INTEGER NOT NULL default '0',
  object_id INTEGER NOT NULL default '0',
  deletion_time timestamp NOT NULL default '1970-01-01 00:00:00',
  deletion_time_usec INTEGER NOT NULL default '0',
--  PRIMARY KEY  (timedevent_id)
  PRIMARY KEY  (timedevent_id),
  UNIQUE (instance_id,event_type,scheduled_time,object_id)
--  UNIQUE (instance_id),
--  UNIQUE (event_type),
--  UNIQUE (scheduled_time),
--  UNIQUE (object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timeperiods
--

CREATE TABLE  icinga_timeperiods (
  timeperiod_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  config_type INTEGER NOT NULL default '0',
  timeperiod_object_id INTEGER NOT NULL default '0',
  alias varchar(255) NOT NULL default '',
--  PRIMARY KEY  (timeperiod_id)
  PRIMARY KEY  (timeperiod_id),
  UNIQUE (instance_id,config_type,timeperiod_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timeperiod_timeranges
--

CREATE TABLE  icinga_timeperiod_timeranges (
  timeperiod_timerange_id SERIAL,
  instance_id INTEGER NOT NULL default '0',
  timeperiod_id INTEGER NOT NULL default '0',
  day INTEGER NOT NULL default '0',
  start_sec INTEGER NOT NULL default '0',
  end_sec INTEGER NOT NULL default '0',
--  PRIMARY KEY  (timeperiod_timerange_id)
  PRIMARY KEY  (timeperiod_timerange_id),
  UNIQUE (timeperiod_id,day,start_sec,end_sec)
) ;




