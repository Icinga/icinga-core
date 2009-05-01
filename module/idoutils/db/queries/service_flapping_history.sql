SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_flappinghistory.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,icinga_flappinghistory.*
FROM `icinga_flappinghistory`
LEFT JOIN icinga_objects as obj1 ON icinga_flappinghistory.object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_flappinghistory.instance_id=icinga_instances.instance_id
WHERE obj1.objecttype_id='2'
ORDER BY event_time DESC, event_time_usec DESC, flappinghistory_id DESC

