SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_timedevents.event_type
,icinga_timedevents.scheduled_time
,icinga_timedevents.event_time
,icinga_timedevents.event_time_usec
,icinga_timedevents.recurring_event
,obj1.objecttype_id
,icinga_timedevents.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
FROM `icinga_timedevents`
LEFT JOIN icinga_objects as obj1 ON icinga_timedevents.object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_timedevents.instance_id=icinga_instances.instance_id
WHERE scheduled_time < NOW()
ORDER BY scheduled_time DESC, timedevent_id DESC

