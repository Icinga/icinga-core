/************************************************************************
 *
 * LOGGING.H - IDOUTILS LOGGING Include File
 *
 * Copyright (c) 2011 Icinga Development Team (http://www.icinga.org)
 * 
 ************************************************************************/

#ifndef _IDO2DB_LOGGING_H
#define _IDO2DB_LOGGING_H

#include "ido2db.h"

/* logging */
int ido2db_open_debug_log(void);
int ido2db_close_debug_log(void);
int ido2db_log_debug_info(int , int , const char *, ...);

#endif
