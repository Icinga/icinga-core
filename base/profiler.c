/*****************************************************************************
 *
 * PROFILER.C - Event Profiler For Icinga
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

#include "../include/icinga.h"

/* make sure gcc3 won't hit here */
#ifndef GCCTOOOLD

#include "../include/profiler.h"

int profiler_item_count = -1;

int profiler_core_event_types[] = {
	EVENT_SERVICE_CHECK,
	EVENT_HOST_CHECK,
	EVENT_COMMAND_CHECK,
	EVENT_LOG_ROTATION,
	EVENT_PROGRAM_SHUTDOWN,
	EVENT_PROGRAM_RESTART,
	EVENT_EXPIRE_DISABLED_NOTIFICATIONS,
	EVENT_CHECK_REAPER,
	EVENT_ORPHAN_CHECK,
	EVENT_RETENTION_SAVE,
	EVENT_STATUS_SAVE,
	EVENT_SCHEDULED_DOWNTIME,
	EVENT_SFRESHNESS_CHECK,
	EVENT_HFRESHNESS_CHECK,
	EVENT_EXPIRE_DOWNTIME,
	EVENT_RESCHEDULE_CHECKS,
	EVENT_EXPIRE_COMMENT,
	EVENT_EXPIRE_ACKNOWLEDGEMENT,
	EVENT_USER_FUNCTION,
	EVENT_LOOP_COMPLETION
};

profiler_item * profiler;


void profiler_init() {
	profiler = (profiler_item*)calloc(++profiler_item_count, sizeof(profiler_item));
	profiler_add(EVENT_SERVICE_CHECK, "EVENT_SERVICE_CHECK");
	profiler_add(EVENT_HOST_CHECK, "EVENT_HOST_CHECK");
	profiler_add(EVENT_COMMAND_CHECK, "EVENT_COMMAND_CHECK");
	profiler_add(EVENT_LOG_ROTATION, "EVENT_LOG_ROTATION");
	profiler_add(EVENT_PROGRAM_SHUTDOWN, "EVENT_PROGRAM_SHUTDOWN");
	profiler_add(EVENT_PROGRAM_RESTART, "EVENT_PROGRAM_RESTART");
	profiler_add(EVENT_EXPIRE_DISABLED_NOTIFICATIONS, "EVENT_EXPIRE_DISABLED_NOTIFICATIONS");
	profiler_add(EVENT_CHECK_REAPER, "EVENT_CHECK_REAPER");
	profiler_add(EVENT_ORPHAN_CHECK, "EVENT_ORPHAN_CHECK");
	profiler_add(EVENT_RETENTION_SAVE, "EVENT_RETENTION_SAVE");
	profiler_add(EVENT_STATUS_SAVE, "EVENT_STATUS_SAVE");
	profiler_add(EVENT_SCHEDULED_DOWNTIME, "EVENT_SCHEDULED_DOWNTIME");
	profiler_add(EVENT_SFRESHNESS_CHECK, "EVENT_SFRESHNESS_CHECK");
	profiler_add(EVENT_HFRESHNESS_CHECK, "EVENT_HFRESHNESS_CHECK");
	profiler_add(EVENT_EXPIRE_DOWNTIME, "EVENT_EXPIRE_DOWNTIME");
	profiler_add(EVENT_RESCHEDULE_CHECKS, "EVENT_RESCHEDULE_CHECKS");
	profiler_add(EVENT_EXPIRE_COMMENT, "EVENT_EXPIRE_COMMENT");
	profiler_add(EVENT_EXPIRE_ACKNOWLEDGEMENT, "EVENT_EXPIRE_ACKNOWLEDGEMENT");
	profiler_add(EVENT_USER_FUNCTION, "EVENT_USER_FUNCTION");
	profiler_add(EVENT_LOOP_COMPLETION, "EVENT_LOOP_COMPLETION");
	profiler_enable_core();
}

void profiler_enable_core() {
	int x = 0;
	int core_event_count = sizeof(profiler_core_event_types) / sizeof(int);
	while (x <= core_event_count) {
		profiler_setstate(profiler_core_event_types[x], 1);
		x++;
	}
}

void profiler_enable_all() {
	int x = 0;
	while (x <= profiler_item_count) {
		profiler_setstate(x, 1);
		x++;
	}
}

void profiler_full_reset(profiler_item *p[]) {
	int x = 0;
	while (x <= profiler_item_count) {
		profiler_item_reset(p[x]);
		x++;
	}
}

void profiler_item_reset(profiler_item *p) {
	p->state = 0;
	p->counter = 0;
	p->elapsed = 0;
}

void profiler_free(profiler_item *p[], int count) {
	//Free the old memory
	int x = count;
	for (; x > 0; x--) {
		free(p[x]->name);
		free(p[x]);
	}
}

void profiler_add(int event, char *name) {
	if (event >= profiler_item_count) {
		profiler_item_count = event;
		profiler = realloc(profiler, (sizeof(profiler_item) * (++profiler_item_count)));
		if (profiler == NULL) {
			printf("Could not allocate sufficient memory, exiting now!\n");
			exit(0);
		}

	}

	profiler[event].state = 0;
	profiler[event].counter = 0;
	profiler[event].elapsed = 0;
	profiler[event].name = strdup(name);
}

void profiler_setstate(int event, int state) {
	profiler[event].state = state;
}

void profiler_rename(int p, char * name) {
	free(profiler[p].name);
	profiler[p].name = strdup(name);
}

void profiler_update(int event, struct timeval start) {
	static int counter;
	struct timeval end;
	gettimeofday(&end, NULL);

	//We do this to prevent segfaulting since it could be a we end up with a sparse array
	//It's ugly but it saves having to try and know everything that may be profiled in advance.
	//It's only slow the first time a new event type is profiled.
	while (event > profiler_item_count) {
		char name[20];
		memset(name, '\0', 20);
		sprintf(name, "UNKNOWN_%d", counter++);
		profiler_add(event, (char*)&name);
	}

	if (profiler[event].state) {
		profiler[event].elapsed += (double)((end.tv_usec - start.tv_usec) / 1000000) + ((end.tv_sec - start.tv_sec));
		profiler[event].counter++;
	}
}

void profiler_output(FILE* fp) {
	int c;
	for (c = 0; c < profiler_item_count; c++) {
		//Only print those that are turned on.
		if (profiler[c].state) {
			if (profiler[c].name) {
				fprintf(fp, "\tPROFILE_COUNTER_%s=%d\n", profiler[c].name, profiler[c].counter);
				fprintf(fp, "\tPROFILE_ELAPSED_%s=%f\n", profiler[c].name, profiler[c].elapsed);
			}
		}
	}

}

#endif
