-- --------------------------------------------------------
-- oracle-upgrade-1.0.2-fix-object-relationssql
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

DELETE FROM hostchecks WHERE end_time='01-01-70';

-- -----------------------------------------
-- update host_object_id
-- -----------------------------------------

UPDATE hostchecks i6
SET i6.host_object_id = (
SELECT i4.id FROM (
      SELECT id, hostcheck_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS hostcheck_id
            FROM (
                  SELECT host_object_id AS o_id,
                         id AS hc_id
                  FROM hostchecks i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.hostcheck_id
);

-- -----------------------------------------
-- update command_object_id
-- -----------------------------------------

UPDATE hostchecks i6
SET i6.command_object_id = (
SELECT i4.id FROM (
      SELECT id, hostcheck_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS hostcheck_id
            FROM (
                  SELECT command_object_id AS o_id,
                         id AS hc_id
                  FROM hostchecks i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.hostcheck_id
);


-- --------------------------------------------------------
-- servicechecks
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM servicechecks WHERE end_time='01-01-70';

-- -----------------------------------------
-- update service_object_id
-- -----------------------------------------

UPDATE servicechecks i6
SET i6.service_object_id = (
SELECT i4.id FROM (
      SELECT id, servicecheck_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS servicecheck_id
            FROM (
                  SELECT service_object_id AS o_id,
                         id AS hc_id
                  FROM servicechecks i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.servicecheck_id
);


-- -----------------------------------------
-- update command_object_id
-- -----------------------------------------

UPDATE servicechecks i6
SET i6.command_object_id = (
SELECT i4.id FROM (
      SELECT id, servicecheck_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS servicecheck_id
            FROM (
                  SELECT command_object_id AS o_id,
                         id AS hc_id
                  FROM servicechecks i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.servicecheck_id
);


-- --------------------------------------------------------
-- acknowledgements
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM acknowledgements WHERE entry_time='01-01-70';

-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE acknowledgements i6
SET i6.object_id = (
SELECT i4.id FROM (
      SELECT id, acknowledgement_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS acknowledgement_id
            FROM (
                  SELECT object_id AS o_id,
                         id AS hc_id
                  FROM acknowledgements i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.acknowledgement_id
);


-- --------------------------------------------------------
-- commenthistory
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM commenthistory WHERE deletion_time='01-01-70';

-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE commenthistory i6
SET i6.object_id = (
SELECT i4.id FROM (
      SELECT id, commenthistory_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS commenthistory_id
            FROM (
                  SELECT object_id AS o_id,
                         id AS hc_id
                  FROM commenthistory i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.commenthistory_id
);


-- --------------------------------------------------------
-- downtimehistory
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM downtimehistory WHERE actual_end_time='01-01-70';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE downtimehistory i6
SET i6.object_id = (
SELECT i4.id FROM (
      SELECT id, downtimehistory_id
      FROM ( 
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS downtimehistory_id
            FROM ( 
                  SELECT object_id AS o_id,
                         id AS hc_id
                  FROM downtimehistory i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.downtimehistory_id
);


-- --------------------------------------------------------
-- eventhandlers
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM eventhandlers WHERE execution_time='01-01-70';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE eventhandlers i6
SET i6.object_id = (
SELECT i4.id FROM (
      SELECT id, eventhandler_id
      FROM ( 
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS eventhandler_id
            FROM (
                  SELECT object_id AS o_id,
                         id AS hc_id
                  FROM eventhandlers i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.eventhandler_id
);


-- --------------------------------------------------------
-- flappinghistory
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM flappinghistory WHERE comment_time='01-01-70';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE flappinghistory i6
SET i6.object_id = (
SELECT i4.id FROM (
      SELECT id, flappinghistory_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS flappinghistory_id
            FROM (
                  SELECT object_id AS o_id,
                         id AS hc_id
                  FROM flappinghistory i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.flappinghistory_id
);


-- --------------------------------------------------------
-- notifications
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM notifications WHERE end_time='01-01-70';

-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE notifications i6
SET i6.object_id = (
SELECT i4.id FROM (
      SELECT id, notification_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS notification_id
            FROM (
                  SELECT object_id AS o_id,
                         id AS hc_id
                  FROM notifications i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.notification_id
);

-- --------------------------------------------------------
-- statehistory
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM statehistory WHERE state_time='01-01-70';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE statehistory i6
SET i6.object_id = (
SELECT i4.id FROM (
      SELECT id, statehistory_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS statehistory_id
            FROM (
                  SELECT object_id AS o_id,
                         id AS hc_id
                  FROM statehistory i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.statehistory_id
);


-- --------------------------------------------------------
-- timedevents
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM timedevents WHERE deletion_time='01-01-70';


-- -----------------------------------------
-- update object_id
-- -----------------------------------------

UPDATE timedevents i6
SET i6.object_id = (
SELECT i4.id FROM (
      SELECT id, timedevent_id
      FROM ( 
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS timedevent_id
            FROM ( 
                  SELECT object_id AS o_id,
                         id AS hc_id
                  FROM timedevents i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.timedevent_id
);


-- --------------------------------------------------------
-- contactnotifications
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM contactnotifications WHERE end_time='01-01-70';


-- -----------------------------------------
-- update contact_object_id
-- -----------------------------------------

UPDATE contactnotifications i6
SET i6.contact_object_id = (
SELECT i4.id FROM (
      SELECT id, contactnotification_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS contactnotification_id
            FROM (
                  SELECT contact_object_id AS o_id,
                         id AS hc_id
                  FROM contactnotifications i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.contactnotification_id
);


-- --------------------------------------------------------
-- contactnotificationmethods
-- --------------------------------------------------------

-- -----------------------------------------
-- delete broken unique constraints in case of 
-- failed restart first (end_time=0)
-- -----------------------------------------

DELETE FROM contactnotificationmethods WHERE end_time='01-01-70';

-- -----------------------------------------
-- update command_object_id
-- -----------------------------------------

UPDATE contactnotificationmethods i6
SET i6.command_object_id = (
SELECT i4.id FROM (
      SELECT id, contactnotificationmethod_id
      FROM (
            SELECT instance_id AS i_id,
                   objecttype_id AS ot_id,
                   name1 AS n1,
                   name2 AS n2,
                   is_active AS is_a,
                   hc_id AS contactnotificationmethod_id
            FROM (
                  SELECT command_object_id AS o_id,
                         id AS hc_id
                  FROM contactnotificationmethods i1
            ) i2
           JOIN objects obj1
           ON i2.o_id=obj1.id
           ) i3
           JOIN objects obj2
           ON i3.i_id=obj2.instance_id
           AND i3.ot_id=obj2.objecttype_id
           AND i3.n1=obj2.name1
           AND i3.n2=obj2.name2
           WHERE obj2.is_active=1
       ) i4
WHERE i6.id=i4.contactnotificationmethod_id
);




-- -----------------------------------------
-- finished
-- -----------------------------------------

-- -----------------------------------------
-- delete all objects marked inactive
-- -----------------------------------------

DELETE FROM objects WHERE is_active=0;
