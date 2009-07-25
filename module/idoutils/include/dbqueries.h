/************************************************************************
 *
 * DBQUERIES.H - IDO2DB DB QUERY Handler Include File
 *
 * Copyright (c) 2009 Icinga Development Team (http://www.icinga.org)
 *
 * Last Modified: 07-25-2009
 ************************************************************************/

#ifndef _IDO2DB_DBQUERIES_H
#define _IDO2DB_DBQUERIES_H

#include "ido2db.h"

#define NAGIOS_SIZEOF_ARRAY(var)       (sizeof(var)/sizeof(var[0]))

int ido2db_query_insert_or_update_timedevents(ndo2db_idi *idi, void **data);

#endif
