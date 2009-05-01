SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_timedeventqueue.event_type
,icinga_timedeventqueue.scheduled_time
,icinga_timedeventqueue.recurring_event
,obj1.objecttype_id
,icinga_timedeventqueue.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
FROM `icinga_timedeventqueue`
LEFT JOIN icinga_objects as obj1 ON icinga_timedeventqueue.object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_timedeventqueue.instance_id=icinga_instances.instance_id
ORDER BY scheduled_time ASC, timedeventqueue_id ASC

