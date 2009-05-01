SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_services.host_object_id
,obj1.name1 AS host_name
,icinga_services.service_object_id
,obj1.name2 AS service_description
FROM `icinga_services`
LEFT JOIN icinga_objects as obj1 ON icinga_services.service_object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_services.instance_id=icinga_instances.instance_id
WHERE icinga_services.config_type='1'
ORDER BY instance_name ASC, host_name ASC, service_description ASC

