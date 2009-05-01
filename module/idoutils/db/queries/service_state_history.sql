SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_statehistory.object_id
,obj1.name1 AS host_name
,obj1.name2 AS service_description
,icinga_statehistory.*
FROM `icinga_statehistory`
LEFT JOIN icinga_objects as obj1 ON icinga_statehistory.object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_statehistory.instance_id=icinga_instances.instance_id
WHERE obj1.objecttype_id='2'
ORDER BY state_time DESC, state_time_usec DESC

