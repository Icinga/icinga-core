/*****************************************************************************
 *
 * CGIAUTH.H - Authorization utilities header file
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

#ifndef _AUTH_H
#define _AUTH_H

#include "common.h"
#include "objects.h"


#ifdef __cplusplus
  extern "C" {
#endif

typedef struct authdata_struct{
	char *username;
	int authorized_for_all_hosts;
	int authorized_for_all_host_commands;
	int authorized_for_all_services;
	int authorized_for_all_service_commands;
	int authorized_for_system_information;
	int authorized_for_system_commands;
	int authorized_for_configuration_information;
	int authorized_for_full_command_resolution;
	int authorized_for_read_only;
	int authorized_for_comments_read_only;
	int authorized_for_downtimes_read_only;
	int authenticated;
        }authdata;



int get_authentication_information(authdata *);       /* gets current authentication information */

int is_authorized_for_host(host *,authdata *);
int is_authorized_for_service(service *,authdata *);

int is_authorized_for_all_hosts(authdata *);
int is_authorized_for_all_services(authdata *);

int is_authorized_for_system_information(authdata *);
int is_authorized_for_system_commands(authdata *);
int is_authorized_for_host_commands(host *,authdata *);
int is_authorized_for_service_commands(service *,authdata *);

int is_authorized_for_hostgroup(hostgroup *,authdata *);
int is_authorized_for_servicegroup(servicegroup *,authdata *);

int is_authorized_for_hostgroup_commands(hostgroup *,authdata *);
int is_authorized_for_servicegroup_commands(servicegroup *,authdata *);

int is_authorized_for_configuration_information(authdata *);
int is_authorized_for_full_command_resolution(authdata *);

int is_authorized_for_read_only(authdata *);
int is_authorized_for_comments_read_only(authdata *);
int is_authorized_for_downtimes_read_only(authdata *);

#ifdef __cplusplus
  }
#endif

#endif
