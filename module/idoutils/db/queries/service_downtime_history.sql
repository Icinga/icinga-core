SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_downtimehistory.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,icinga_downtimehistory.*
FROM `icinga_downtimehistory`
LEFT JOIN icinga_objects as obj1 ON icinga_downtimehistory.object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_downtimehistory.instance_id=icinga_instances.instance_id
WHERE obj1.objecttype_id='2'
ORDER BY scheduled_start_time DESC, actual_start_time DESC, actual_start_time_usec DESC, downtimehistory_id DESC

