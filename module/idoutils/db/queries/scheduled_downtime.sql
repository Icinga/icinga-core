SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_scheduleddowntime.object_id
,obj1.objecttype_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,icinga_scheduleddowntime.*
FROM `icinga_scheduleddowntime`
LEFT JOIN icinga_objects as obj1 ON icinga_scheduleddowntime.object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_scheduleddowntime.instance_id=icinga_instances.instance_id
ORDER BY scheduled_start_time DESC, scheduleddowntime_id DESC

