SELECT 
icinga_instances.instance_id
,icinga_instances.instance_name
,icinga_contacts.contact_object_id
,obj1.name1 AS contact_name
,icinga_contactstatus.*
FROM `icinga_contactstatus`
LEFT JOIN icinga_objects as obj1 ON icinga_contactstatus.contact_object_id=obj1.object_id
LEFT JOIN icinga_contacts ON icinga_contactstatus.contact_object_id=icinga_contacts.contact_object_id
LEFT JOIN icinga_instances ON icinga_contacts.instance_id=icinga_instances.instance_id
WHERE icinga_contacts.config_type='1'
ORDER BY instance_name ASC, contact_name ASC
