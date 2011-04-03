-- -----------------------------------------
-- mysql-upgrade-1.0.sql
-- upgrade path for Icinga IDOUtils 1.0
-- fixes unique key failure
-- -----------------------------------------
-- Copyright (c) 2009 Icinga Development Team (http://www.icinga.org)
--
-- Initial Revision: 2009-12-02 Michael Friedrich <michael.friedrich(at)univie.ac.at>
-- 
-- patch ideas partly taken from 
-- http://stackoverflow.com/questions/1073713/mysql-merge-amounts-in-a-2-rows
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- servicechecks unique key upgrade path
-- -----------------------------------------

-- alter table to add new temp column 'active'
ALTER TABLE `icinga_servicechecks` ADD `active` INT( 1 ) NOT NULL DEFAULT '1'; 

-- merge entries, only update changes from first row into second row
UPDATE icinga_servicechecks mtu
JOIN (
      SELECT minid, 
	     maxid, 
	     mtmin.start_time AS new_start_time,
	     mtmin.start_time_usec AS new_start_time_usec,
	     mtmin.command_object_id AS new_command_object_id,
	     mtmin.command_args AS new_command_args,
	     mtmin.command_line AS new_command_line
      FROM (
	    SELECT MIN(servicecheck_id) AS minid, MAX(servicecheck_id) as maxid
	    FROM icinga_servicechecks mti
	    GROUP BY instance_id, service_object_id, start_time, start_time_usec
	    HAVING COUNT(*) = 2
	    ) mt
      JOIN icinga_servicechecks mtmin
      ON mtmin.servicecheck_id = minid
      JOIN icinga_servicechecks mtmax
      ON mtmax.servicecheck_id = maxid
      ) mts
ON    servicecheck_id IN (minid, maxid)
SET   start_time = CASE servicecheck_id WHEN maxid THEN new_start_time END,
      start_time_usec = CASE servicecheck_id WHEN maxid THEN new_start_time_usec END,
      command_object_id = CASE servicecheck_id WHEN maxid THEN new_command_object_id END,
      command_args = CASE servicecheck_id WHEN maxid THEN new_command_args END,       
      command_line = CASE servicecheck_id WHEN maxid THEN new_command_line END,
      active = (servicecheck_id = maxid);

-- delete all rows marked not active
DELETE FROM icinga_servicechecks WHERE active = 0;

-- apply unique key patches
ALTER table icinga_servicechecks DROP KEY instance_id;
ALTER table icinga_servicechecks DROP KEY service_object_id;
ALTER table icinga_servicechecks DROP KEY start_time;

ALTER table icinga_servicechecks ADD UNIQUE KEY `instance_id` (`instance_id`,`service_object_id`,`start_time`,`start_time_usec`);

-- remove temp flag
ALTER TABLE `icinga_servicechecks` DROP `active`;


-- -----------------------------------------
-- systemcommands unique key upgrade path
-- -----------------------------------------

-- alter table to add new temp column 'active'
ALTER TABLE `icinga_systemcommands` ADD `active` INT( 1 ) NOT NULL DEFAULT '1'; 

-- merge entries, only update changes from first row into second row
UPDATE icinga_systemcommands mtu
JOIN (
      SELECT minid, 
	     maxid, 
	     mtmin.start_time AS new_start_time,
	     mtmin.start_time_usec AS new_start_time_usec,
	     mtmin.command_line AS new_command_line
      FROM (
	    SELECT MIN(systemcommand_id) AS minid, MAX(systemcommand_id) as maxid
	    FROM icinga_systemcommands mti
	    GROUP BY instance_id, start_time, start_time_usec
	    HAVING COUNT(*) = 2
	    ) mt
      JOIN icinga_systemcommands mtmin
      ON mtmin.systemcommand_id = minid
      JOIN icinga_systemcommands mtmax
      ON mtmax.systemcommand_id = maxid
      ) mts
ON    systemcommand_id IN (minid, maxid)
SET   start_time = CASE systemcommand_id WHEN maxid THEN new_start_time END,
      start_time_usec = CASE systemcommand_id WHEN maxid THEN new_start_time_usec END,      
      command_line = CASE systemcommand_id WHEN maxid THEN new_command_line END,
      active = (systemcommand_id = maxid);

-- delete all rows marked not active
DELETE FROM icinga_systemcommands WHERE active = 0;

-- apply unique key patches
ALTER table icinga_systemcommands DROP KEY instance_id;
ALTER table icinga_systemcommands DROP KEY start_time;

ALTER table icinga_systemcommands ADD UNIQUE KEY `instance_id` (`instance_id`,`start_time`,`start_time_usec`);

-- remove temp flag
ALTER TABLE icinga_systemcommands DROP `active`; 


-- -----------------------------------------
-- timedevents unique key upgrade path
-- -----------------------------------------

-- alter table to add new temp column 'active'
ALTER TABLE `icinga_timedevents` ADD `active` INT( 1 ) NOT NULL DEFAULT '1';

-- merge entries, only update changes from first row into second row
UPDATE icinga_timedevents mtu
JOIN (
      SELECT minid,
             maxid,
             mtmin.queued_time AS new_queued_time,
             mtmin.queued_time_usec AS new_queued_time_usec
      FROM (
            SELECT MIN(timedevent_id) AS minid, MAX(timedevent_id) as maxid
            FROM icinga_timedevents mti
            GROUP BY instance_id, event_type, scheduled_time, object_id
            HAVING COUNT(*) = 2
            ) mt
      JOIN icinga_timedevents mtmin
      ON mtmin.timedevent_id = minid
      JOIN icinga_timedevents mtmax
      ON mtmax.timedevent_id = maxid
      ) mts
ON    timedevent_id IN (minid, maxid)
SET   queued_time = CASE timedevent_id WHEN maxid THEN new_queued_time END,
      queued_time_usec = CASE timedevent_id WHEN maxid THEN new_queued_time_usec END,
      active = (timedevent_id = maxid);

-- delete all rows marked not active
DELETE FROM icinga_timedevents WHERE active = 0;

-- apply unique key patches
ALTER table icinga_timedevents DROP KEY instance_id;
ALTER table icinga_timedevents DROP KEY event_type;
ALTER table icinga_timedevents DROP KEY scheduled_time;

ALTER table icinga_timedevents ADD UNIQUE KEY `instance_id` (`instance_id`,`event_type`,`scheduled_time`,`object_id`);

-- remove temp flag
ALTER TABLE icinga_timedevents DROP `active`;


-- -----------------------------------------
-- timedeventqueue unique key upgrade path
-- -----------------------------------------

-- alter table to add new temp column 'active'
ALTER TABLE `icinga_timedeventqueue` ADD `active` INT( 1 ) NOT NULL DEFAULT '1';

-- merge entries, only update changes from first row into second row
UPDATE icinga_timedeventqueue mtu
JOIN (
      SELECT minid,
             maxid,
             mtmin.queued_time AS new_queued_time,
             mtmin.queued_time_usec AS new_queued_time_usec
      FROM (
            SELECT MIN(timedeventqueue_id) AS minid, MAX(timedeventqueue_id) as maxid
            FROM icinga_timedeventqueue mti
            GROUP BY instance_id, event_type, scheduled_time, object_id
            HAVING COUNT(*) = 2
            ) mt
      JOIN icinga_timedeventqueue mtmin
      ON mtmin.timedeventqueue_id = minid
      JOIN icinga_timedeventqueue mtmax
      ON mtmax.timedeventqueue_id = maxid
      ) mts
ON    timedeventqueue_id IN (minid, maxid)
SET   queued_time = CASE timedeventqueue_id WHEN maxid THEN new_queued_time END,
      queued_time_usec = CASE timedeventqueue_id WHEN maxid THEN new_queued_time_usec END,
      active = (timedeventqueue_id = maxid);

-- delete all rows marked not active
DELETE FROM icinga_timedeventqueue WHERE active = 0;

-- apply unique key patches
ALTER table icinga_timedeventqueue DROP KEY instance_id;
ALTER table icinga_timedeventqueue DROP KEY event_type;
ALTER table icinga_timedeventqueue DROP KEY scheduled_time;

ALTER table icinga_timedeventqueue ADD UNIQUE KEY `instance_id` (`instance_id`,`event_type`,`scheduled_time`,`object_id`);

-- remove temp flag
ALTER TABLE icinga_timedeventqueue DROP `active`;

