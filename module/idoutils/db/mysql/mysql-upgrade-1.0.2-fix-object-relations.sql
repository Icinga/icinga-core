-- --------------------------------------------------------
-- mysql-upgrade-1.0.2-fix-object-relationssql
-- upgrade queries for Icinga IDOUtils 1.0.2
-- fixes failed object inserts failure
-- --------------------------------------------------------
-- Copyright (c) 2010 Icinga Development Team (http://www.icinga.org)
--
-- Initial Revision: 2010-06-05 Michael Friedrich <michael.friedrich(at)univie.ac.at>
-- 
--
-- Please check http://docs.icinga.org for upgrading information!
-- --------------------------------------------------------

-- --------------------------------------------------------
-- hostchecks
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_hostchecks WHERE end_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update host_object_id
-- -----------------------------------------

UPDATE icinga_hostchecks i5 
JOIN (
      SELECT * 
      FROM (
            SELECT instance_id AS i_id, 
                   objecttype_id AS ot_id, 
                   name1 AS n1, 
                   name2 AS n2, 
                   is_active AS is_a, 
                   hc_id AS hostcheck_id 
            FROM (
                  SELECT host_object_id AS o_id, 
                         hostcheck_id AS hc_id 
                  FROM icinga_hostchecks i1
            ) i2 
           JOIN icinga_objects obj1 
           ON i2.o_id=obj1.object_id
           ) i3 
           JOIN icinga_objects obj2 
           ON i3.i_id=obj2.instance_id 
           AND i3.ot_id=obj2.objecttype_id 
           AND i3.n1=obj2.name1 
           AND i3.n2=obj2.name2 
           WHERE is_active=1
       ) i4 
ON i5.hostcheck_id=i4.hostcheck_id 
SET i5.host_object_id=i4.object_id;

-- -----------------------------------------
-- update command_object_id
-- -----------------------------------------

UPDATE icinga_hostchecks i5
JOIN (
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS hostcheck_id
            FROM (
                  SELECT command_object_id AS o_id,   
                         hostcheck_id AS hc_id
                  FROM icinga_hostchecks i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.hostcheck_id=i4.hostcheck_id
SET i5.command_object_id=i4.object_id;


-- --------------------------------------------------------
-- servicechecks
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_servicechecks WHERE end_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update service_object_id
-- -----------------------------------------

UPDATE icinga_servicechecks i5
JOIN (
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS servicecheck_id
            FROM (
                  SELECT service_object_id AS o_id,
                         servicecheck_id AS hc_id
                  FROM icinga_servicechecks i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.servicecheck_id=i4.servicecheck_id
SET i5.service_object_id=i4.object_id;

-- -----------------------------------------
-- update command_object_id
-- -----------------------------------------

UPDATE icinga_servicechecks i5
JOIN ( 
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS servicecheck_id
            FROM (
                  SELECT command_object_id AS o_id,
                         servicecheck_id AS hc_id
                  FROM icinga_servicechecks i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.servicecheck_id=i4.servicecheck_id
SET i5.command_object_id=i4.object_id;


-- --------------------------------------------------------
-- acknowledgements
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_acknowledgements WHERE entry_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE icinga_acknowledgements i5
JOIN ( 
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS acknowledgement_id
            FROM (
                  SELECT object_id AS o_id,
                         acknowledgement_id AS hc_id
                  FROM icinga_acknowledgements i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.acknowledgement_id=i4.acknowledgement_id
SET i5.object_id=i4.object_id;


-- --------------------------------------------------------
-- commenthistory
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_commenthistory WHERE deletion_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE icinga_commenthistory i5
JOIN (
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS commenthistory_id
            FROM (
                  SELECT object_id AS o_id,
                         commenthistory_id AS hc_id
                  FROM icinga_commenthistory i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.commenthistory_id=i4.commenthistory_id
SET i5.object_id=i4.object_id;


-- --------------------------------------------------------
-- downtimehistory
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_downtimehistory WHERE actual_end_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE icinga_downtimehistory i5
JOIN ( 
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS downtimehistory_id
            FROM (
                  SELECT object_id AS o_id,
                         downtimehistory_id AS hc_id
                  FROM icinga_downtimehistory i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.downtimehistory_id=i4.downtimehistory_id
SET i5.object_id=i4.object_id;


-- --------------------------------------------------------
-- eventhandlers
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_eventhandlers WHERE execution_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE icinga_eventhandlers i5
JOIN (
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS eventhandler_id
            FROM (
                  SELECT object_id AS o_id,
                         eventhandler_id AS hc_id
                  FROM icinga_eventhandlers i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.eventhandler_id=i4.eventhandler_id
SET i5.object_id=i4.object_id;


-- --------------------------------------------------------
-- flappinghistory
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_flappinghistory WHERE comment_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE icinga_flappinghistory i5
JOIN ( 
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS flappinghistory_id
            FROM (
                  SELECT object_id AS o_id,
                         flappinghistory_id AS hc_id
                  FROM icinga_flappinghistory i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.flappinghistory_id=i4.flappinghistory_id
SET i5.object_id=i4.object_id;


-- --------------------------------------------------------
-- notifications
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_notifications WHERE end_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE icinga_notifications i5
JOIN (
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS notification_id
            FROM (
                  SELECT object_id AS o_id,
                         notification_id AS hc_id
                  FROM icinga_notifications i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.notification_id=i4.notification_id
SET i5.object_id=i4.object_id;


-- --------------------------------------------------------
-- statehistory
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_statehistory WHERE state_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE icinga_statehistory i5
JOIN (
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS statehistory_id
            FROM (
                  SELECT object_id AS o_id,
                         statehistory_id AS hc_id
                  FROM icinga_statehistory i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.statehistory_id=i4.statehistory_id
SET i5.object_id=i4.object_id;


-- --------------------------------------------------------
-- timedevents
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_timedevents WHERE deletion_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE icinga_timedevents i5
JOIN (
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS timedevent_id
            FROM (
                  SELECT object_id AS o_id,
                         timedevent_id AS hc_id
                  FROM icinga_timedevents i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.timedevent_id=i4.timedevent_id
SET i5.object_id=i4.object_id;


-- --------------------------------------------------------
-- contactnotifications
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_contactnotifications WHERE end_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update contact_object_id
-- -----------------------------------------

UPDATE icinga_contactnotifications i5
JOIN (
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS contactnotification_id
            FROM (
                  SELECT contact_object_id AS o_id,
                         contactnotification_id AS hc_id
                  FROM icinga_contactnotifications i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.contactnotification_id=i4.contactnotification_id
SET i5.contact_object_id=i4.object_id;


-- --------------------------------------------------------
-- contactnotificationmethods
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM icinga_contactnotificationmethods WHERE end_time LIKE '%1970-01-01 01:00:00%';


-- -----------------------------------------
-- update command_object_id
-- -----------------------------------------

UPDATE icinga_contactnotificationmethods i5
JOIN (
      SELECT *
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS contactnotificationmethod_id
            FROM (
                  SELECT command_object_id AS o_id,
                         contactnotificationmethod_id AS hc_id
                  FROM icinga_contactnotificationmethods i1
            ) i2
           JOIN icinga_objects obj1
           ON i2.o_id=obj1.object_id
           ) i3
           JOIN icinga_objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE is_active=1
       ) i4
ON i5.contactnotificationmethod_id=i4.contactnotificationmethod_id
SET i5.command_object_id=i4.object_id;







-- -----------------------------------------
-- finished
-- -----------------------------------------

-- -----------------------------------------
-- delete all objects marked inactive
-- -----------------------------------------

DELETE FROM icinga_objects WHERE is_active=0;
