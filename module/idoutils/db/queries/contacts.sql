SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_contacts.contact_object_id
,obj1.name1 AS contact_name
FROM `icinga_contacts`
LEFT JOIN icinga_objects as obj1 ON icinga_contacts.contact_object_id=obj1.object_id
LEFT JOIN icinga_instances ON icinga_contacts.instance_id=icinga_instances.instance_id
WHERE icinga_contacts.config_type='1'
ORDER BY instance_name ASC, contact_name ASC

