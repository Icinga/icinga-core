/*****************************************************************************
 *
 * SRETENTION.H - Header for state retention routines
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2013 Nagios Core Development Team and Community Contributors
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

#ifdef __cplusplus
  extern "C" {
#endif

int initialize_retention_data(char *);
int cleanup_retention_data(char *);
int save_state_information(int);                 /* saves all host and state information */
int read_initial_state_information(void);        /* reads in initial host and state information */
int sync_state_information(void);                /* syncs hosts and state information from sync file */

#ifdef __cplusplus
  }
#endif
