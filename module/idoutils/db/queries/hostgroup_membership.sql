SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_hostgroups.hostgroup_id
,icinga_hostgroups.hostgroup_object_id
,obj1.name1 AS hostgroup_name
,icinga_hostgroups.alias AS hostgroup_alias
,icinga_hosts.host_object_id 
,obj2.name1 AS host_name
FROM `icinga_hostgroups` 
INNER JOIN icinga_hostgroup_members ON icinga_hostgroups.hostgroup_id=icinga_hostgroup_members.hostgroup_id 
INNER JOIN icinga_hosts ON icinga_hostgroup_members.host_object_id=icinga_hosts.host_object_id
INNER JOIN icinga_objects as obj1 ON icinga_hostgroups.hostgroup_object_id=obj1.object_id
INNER JOIN icinga_objects as obj2 ON icinga_hostgroup_members.host_object_id=obj2.object_id
INNER JOIN icinga_instances ON icinga_hostgroups.instance_id=icinga_instances.instance_id
