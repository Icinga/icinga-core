SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_notifications.object_id AS host_object_id
,obj1.name1 AS host_name
,icinga_notifications.*
FROM `icinga_notifications`
LEFT JOIN icinga_objects as obj1 ON icinga_notifications.object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_notifications.instance_id=icinga_instances.instance_id
WHERE obj1.objecttype_id='1'
ORDER BY start_time DESC, start_time_usec DESC
