/*****************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*
*****************************************************************************/

#define NSCORE 1
#include "config.h"
#include "common.h"
#include "icinga.h"
#include "downtime.h"
#include "stub_broker.c"
#include "stub_comments.c"
#include "stub_objects.c"
#include "stub_statusdata.c"
#include "stub_notifications.c"
#include "stub_shared.c"
#include "stub_events.c"
#include "tap.h"

void logit(int data_type, int display, const char *fmt, ...) {}
int log_debug_info(int level, int verbosity, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	/* vprintf( fmt, ap ); */
	va_end(ap);
}

timed_event *event_list_high = NULL;
timed_event *event_list_high_tail = NULL;

unsigned long next_downtime_id = 1L;

extern scheduled_downtime *scheduled_downtime_list;

/*
service *svc1 = NULL, *svc2 = NULL;
host *host1 = NULL;

void
setup_objects(time_t time) {
        timed_event *new_event = NULL;

        host1 = (host *)calloc(1, sizeof(host));
        host1->name = strdup("Host1");
        host1->address = strdup("127.0.0.1");
        host1->retry_interval = 1;
        host1->check_interval = 5;
        host1->check_options = 0;
        host1->state_type = SOFT_STATE;
        host1->current_state = HOST_DOWN;
        host1->has_been_checked = TRUE;
        host1->last_check = time;
        host1->next_check = time;
        host1->plugin_output = strdup("Initial state");
        host1->long_plugin_output = strdup("Initial state");
        host1->perf_data = NULL;

        svc1 = (service *)calloc(1, sizeof(service));
        svc1->host_name = strdup("Host1");
        svc1->host_ptr = host1;
        svc1->description = strdup("Normal service");
        svc1->check_options = 0;
        svc1->next_check = time;
        svc1->state_type = SOFT_STATE;
        svc1->current_state = STATE_CRITICAL;
        svc1->retry_interval = 1;
        svc1->check_interval = 5;
        svc1->current_attempt = 1;
        svc1->max_attempts = 4;
        svc1->last_state_change = 0;
        svc1->last_state_change = 0;
        svc1->last_check = (time_t)1234560000;
        svc1->host_problem_at_last_check = FALSE;
        svc1->plugin_output = strdup("Initial state");
        svc1->last_hard_state_change = (time_t)1111111111;

        svc2 = (service *)calloc(1, sizeof(service));
        svc2->host_name = strdup("Host1");
        svc2->description = strdup("To be nudged");
        svc2->check_options = 0;
        svc2->next_check = time;
        svc2->state_type = SOFT_STATE;
        svc2->current_state = STATE_OK;
        svc2->retry_interval = 1;
        svc2->check_interval = 5;

}
*/

int
main(int argc, char **argv) {
	time_t now = 0L;
	time_t temp_start_time = 1234567890L;
	time_t temp_end_time = 2134567890L;
	unsigned long downtime_id = 0L;
	scheduled_downtime *temp_downtime;
	int i = 0;

	plan(38);

	time(&now);

	schedule_downtime(HOST_DOWNTIME, "host1", NULL, temp_start_time, "user", "test comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 1L, "Got host1 downtime: %lu", downtime_id);
	schedule_downtime(HOST_DOWNTIME, "host2", NULL, temp_start_time, "user", "test comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 2L, "Got host2 downtime: %lu", downtime_id);
	schedule_downtime(HOST_DOWNTIME, "host3", NULL, temp_start_time, "user", "diff comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 3L, "Got host3 downtime: %lu", downtime_id);
	schedule_downtime(HOST_DOWNTIME, "host4", NULL, temp_start_time, "user", "test comment", temp_start_time + 1, temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 4L, "Got host4 downtime: %lu", downtime_id);

	schedule_downtime(SERVICE_DOWNTIME, "host1", "svc", temp_start_time, "user", "svc comment",  temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 5L, "Got host1::svc downtime: %lu", downtime_id);
	schedule_downtime(SERVICE_DOWNTIME, "host2", "svc", temp_start_time, "user", "diff comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 6L, "Got host2::svc downtime: %lu", downtime_id);
	schedule_downtime(SERVICE_DOWNTIME, "host3", "svc", temp_start_time, "user", "svc comment",  temp_start_time + 1, temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 7L, "Got host3::svc downtime: %lu", downtime_id);
	schedule_downtime(SERVICE_DOWNTIME, "host4", "svc", temp_start_time, "user", "uniq comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 8L, "Got host4::svc downtime: %lu", downtime_id);

	for (temp_downtime = scheduled_downtime_list, i = 0; temp_downtime != NULL; temp_downtime = temp_downtime->next, i++) {}
	ok(i == 8, "Got 8 downtimes: %d", i);

	i = delete_downtime_by_hostname_service_description_start_time_comment(NULL, NULL, 0, NULL);
	ok(i == 0, "No deletions") || diag("%d", i);

	i = delete_downtime_by_hostname_service_description_start_time_comment(NULL, NULL, 0, NULL);
	ok(i == 0, "No deletions");

	i = delete_downtime_by_hostname_service_description_start_time_comment(NULL, NULL, temp_start_time, "test comment");
	ok(i == 2, "Deleted 2 downtimes");

	for (temp_downtime = scheduled_downtime_list, i = 0; temp_downtime != NULL; temp_downtime = temp_downtime->next, i++) {}
	ok(i == 6, "Got 6 downtimes left: %d", i);

	i = delete_downtime_by_hostname_service_description_start_time_comment(NULL, NULL, temp_start_time + 200, "test comment");
	ok(i == 0, "Nothing matched, so 0 downtimes deleted");

	i = delete_downtime_by_hostname_service_description_start_time_comment(NULL, NULL, temp_start_time + 1, NULL);
	ok(i == 2, "Deleted 2 by start_time only: %d", i);

	i = delete_downtime_by_hostname_service_description_start_time_comment(NULL, NULL, 0, "uniq comment");
	ok(i == 1, "Deleted 1 by unique comment: %d", i);

	for (temp_downtime = scheduled_downtime_list, i = 0; temp_downtime != NULL; temp_downtime = temp_downtime->next, i++) {
		//diag("downtime id: %d", temp_downtime->downtime_id);
	}
	ok(i == 3, "Got 3 downtimes left: %d", i);

	unschedule_downtime(HOST_DOWNTIME, 3);
	unschedule_downtime(SERVICE_DOWNTIME, 5);
	unschedule_downtime(SERVICE_DOWNTIME, 6);

	for (temp_downtime = scheduled_downtime_list, i = 0; temp_downtime != NULL; temp_downtime = temp_downtime->next, i++) {}
	ok(i == 0, "No downtimes left");


	/* Set all downtimes up again */
	schedule_downtime(HOST_DOWNTIME, "host1", NULL, temp_start_time, "user", "test comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 9L, "Got host1 downtime: %lu", downtime_id);
	schedule_downtime(HOST_DOWNTIME, "host2", NULL, temp_start_time, "user", "test comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 10L, "Got host2 downtime: %lu", downtime_id);
	schedule_downtime(HOST_DOWNTIME, "host3", NULL, temp_start_time, "user", "diff comment", temp_start_time + 1,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 11L, "Got host3 downtime: %lu", downtime_id);
	schedule_downtime(HOST_DOWNTIME, "host4", NULL, temp_start_time, "user", "test comment", temp_start_time + 1, temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 12L, "Got host4 downtime: %lu", downtime_id);

	schedule_downtime(SERVICE_DOWNTIME, "host1", "svc", temp_start_time, "user", "svc comment",  temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 13L, "Got host1::svc downtime: %lu", downtime_id);
	schedule_downtime(SERVICE_DOWNTIME, "host2", "svc", temp_start_time, "user", "diff comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 14L, "Got host2::svc downtime: %lu", downtime_id);
	schedule_downtime(SERVICE_DOWNTIME, "host3", "svc", temp_start_time, "user", "svc comment",  temp_start_time, temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 15L, "Got host3::svc downtime: %lu", downtime_id);
	schedule_downtime(SERVICE_DOWNTIME, "host4", "svc", temp_start_time, "user", "uniq comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 16L, "Got host4::svc downtime: %lu", downtime_id);

	schedule_downtime(SERVICE_DOWNTIME, "host1", "svc2", temp_start_time, "user", "svc2 comment",  temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 17L, "Got host1::svc2 downtime: %lu", downtime_id);
	schedule_downtime(SERVICE_DOWNTIME, "host2", "svc2", temp_start_time, "user", "test comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 18L, "Got host2::svc2 downtime: %lu", downtime_id);
	schedule_downtime(SERVICE_DOWNTIME, "host3", "svc2", temp_start_time, "user", "svc2 comment",  temp_start_time + 1, temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 19L, "Got host3::svc2 downtime: %lu", downtime_id);
	schedule_downtime(SERVICE_DOWNTIME, "host4", "svc2", temp_start_time, "user", "test comment", temp_start_time,  temp_end_time, 0, 0, 0, &downtime_id);
	ok(downtime_id == 20L, "Got host4::svc2 downtime: %lu", downtime_id);

	i = delete_downtime_by_hostname_service_description_start_time_comment("host2", NULL, 0, "test comment");
	ok(i == 2, "Deleted 2");

	i = delete_downtime_by_hostname_service_description_start_time_comment("host1", "svc", 0, NULL);
	ok(i == 1, "Deleted 1") || diag("Actually deleted: %d", i);

	i = delete_downtime_by_hostname_service_description_start_time_comment("host3", NULL, temp_start_time + 1, NULL);
	ok(i == 2, "Deleted 2");

	i = delete_downtime_by_hostname_service_description_start_time_comment(NULL, "svc2", 0, NULL);
	ok(i == 2, "Deleted 2") || diag("Actually deleted: %d", i);

	i = delete_downtime_by_hostname_service_description_start_time_comment("host4", NULL, 0, "test comment");
	ok(i == 1, "Deleted 1") || diag("Actually deleted: %d", i);

	i = delete_downtime_by_hostname_service_description_start_time_comment("host4", NULL, 0, "svc comment");
	ok(i == 0, "Deleted 0") || diag("Actually deleted: %d", i);

	for (temp_downtime = scheduled_downtime_list, i = 0; temp_downtime != NULL; temp_downtime = temp_downtime->next, i++) {
		//diag("downtime id: %d", temp_downtime->downtime_id);
	}
	ok(i == 4, "Got 4 downtimes left: %d", i);

	unschedule_downtime(HOST_DOWNTIME, 9);
	unschedule_downtime(SERVICE_DOWNTIME, 14);
	unschedule_downtime(SERVICE_DOWNTIME, 15);
	unschedule_downtime(SERVICE_DOWNTIME, 16);

	for (temp_downtime = scheduled_downtime_list, i = 0; temp_downtime != NULL; temp_downtime = temp_downtime->next, i++) {}
	ok(i == 0, "No downtimes left") || diag("Left: %d", i);

	/* add tests for #2536 */
	/*
	setup_objects(now);
	*/

	/* int schedule_downtime(int type, char *host_name, char *service_description, time_t entry_time, char *author, char *comment_data, time_t start_time, time_t end_time, int fixed, unsigned long triggered_by, unsigned long duration, unsigned long *new_downtime_id) { */
	/*
        schedule_downtime(HOST_DOWNTIME, "host1", NULL, now, "user", "test comment", now, now+20, 0, 0, 10, &downtime_id);
	ok(downtime_id == 21L, "Got host1 downtime: %lu", downtime_id);
	*/

	/* now set the host to down */
	/*
	host1->current_state = HOST_DOWN;
	*/
	/* FIXME how to pass the host object to the test function? it will bail early if the downtime->hst|svc entry cannot be found */

	/* handle the downtime, as this would be in events.c */
	/*
	handle_scheduled_downtime_by_id(downtime_id);
	temp_downtime = scheduled_downtime_list;

	printf("id: %lu, in_effect: %lu", temp_downtime->downtime_id, temp_downtime->is_in_effect);

	ok(temp_downtime->is_in_effect == 1, "host1 downtime in effect with trigger_time: %lu", temp_downtime->trigger_time);
	*/

	return exit_status();
}


