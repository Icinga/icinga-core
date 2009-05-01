SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_hosts.host_object_id
,obj1.name1 AS host_name
FROM `icinga_hosts`
LEFT JOIN icinga_objects as obj1 ON icinga_hosts.host_object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_hosts.instance_id=icinga_instances.instance_id
WHERE icinga_hosts.config_type='1'
ORDER BY instance_name ASC, host_name ASC

