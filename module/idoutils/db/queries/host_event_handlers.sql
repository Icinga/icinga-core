SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_eventhandlers.object_id
,obj1.name1 AS host_name
,icinga_eventhandlers.*
FROM `icinga_eventhandlers`
LEFT JOIN icinga_objects as obj1 ON icinga_eventhandlers.object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_eventhandlers.instance_id=icinga_instances.instance_id
WHERE obj1.objecttype_id='1'
ORDER BY start_time DESC, start_time_usec DESC, eventhandler_id DESC

