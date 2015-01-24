/*****************************************************************************
 *
 * EPN_ICINGA.H - Embedded Perl Header File
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2015 Icinga Development Team (http://www.icinga.org)
 *
 * License:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *****************************************************************************/

/* needed for location of p1.pl */
#include "locations.h"

/******** BEGIN EMBEDDED PERL INTERPRETER DECLARATIONS ********/

#include <EXTERN.h>
#include <perl.h>

#include <fcntl.h>
#undef DEBUG /* epn-compiled Icinga spews just - this has a side effect of potentially disabling debug output on epn systems */
#undef ctime    /* don't need perl's threaded version */
#undef printf   /* can't use perl's printf until initialized */

/* In perl.h (or friends) there is a macro that defines sighandler as Perl_sighandler, so we must #undef it so we can use our sighandler() function */
#undef sighandler

/* and we don't need perl's reentrant versions */
#undef localtime
#undef getpwnam
#undef getgrnam
#undef strerror

#ifdef aTHX
EXTERN_C void xs_init(pTHX);
#else
EXTERN_C void xs_init(void);
#endif

/******** END EMBEDDED PERL INTERPRETER DECLARATIONS ********/
