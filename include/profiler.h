/*****************************************************************************
 *
 * PROFILER.H - Event Profiler For Icinga
 *
 * Copyright: (c) 2009 Intellectual Reserve Inc.
 * Author:  Steven D. Morrey (nagios-devel@lists.sourceforge.net)
 *
 * Description:
 *
 * Nagios is a network monitoring tool that will check hosts and services
 * that you specify.  This utility adds the ability to profile the events
 * occuring within the application itself
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
#ifndef __PROFILER__HEADER__INCLUDED__
#define __PROFILER__HEADER__INCLUDED__

/* make sure gcc3 won't hit here */
#ifndef GCCTOOOLD


#define EVENT_LOOP_EMPTY_LIST                   100
#define EVENT_LOOP_EVENT_LIST_HIGH_EXECUTED     101
#define EVENT_LOOP_FAIL_NO_SERVICE_CHECKS       102
#define EVENT_LOOP_FAIL_MAX_CHECKS              103
#define EVENT_LOOP_FAIL_PARALLEL                104
#define EVENT_LOOP_FAIL_NO_HOST_CHECKS          105
#define EVENT_LOOP_EXECUTED_EVENT               106
#define EVENT_LOOP_NO_EXECUTION                 107
#define EVENT_LOOP_IDLED                        108
#define EVENT_LOOP_COMPLETION                   109


typedef struct profiler_item{
	int   state;
	int    counter;
	double elapsed;
	char * name;
}profiler_item;

extern profiler_item * profiler;

void profiler_init();
void profiler_enable_core();
void profiler_enable_all();
void profiler_full_reset(profiler_item *p[]);
void profiler_item_reset(profiler_item *p);
void profiler_add(int event, char *name);
void profiler_setstate(int event,int state);
void profiler_rename(int p, char * name);
void profiler_update(int event, struct timeval start);
void profiler_output(FILE* fp);

#endif

#endif
