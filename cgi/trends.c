/*****************************************************************************
 *
 * TRENDS.C -  Icinga State Trends CGI
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
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

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/comments.h"
#include "../include/statusdata.h"
#include "../include/readlogs.h"

#include "../include/cgiutils.h"
#include "../include/getcgi.h"
#include "../include/cgiauth.h"
#include "../include/skiplist.h"

#include <gd.h>			/* Boutell's GD library function */
#include <gdfonts.h>		/* GD library small font definition */

/*#define DEBUG			1*/

extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];
extern char physical_images_path[MAX_FILENAME_LENGTH];

extern int     log_rotation_method;

extern host *host_list;
extern service *service_list;

extern skiplist *object_skiplists[NUM_OBJECT_SKIPLISTS];

/* archived state types */
#define AS_CURRENT_STATE        -1   /* special case for initial assumed state */
#define AS_NO_DATA		0
#define AS_PROGRAM_END		1
#define AS_PROGRAM_START	2
#define AS_HOST_UP		3
#define AS_HOST_DOWN		4
#define AS_HOST_UNREACHABLE	5
#define AS_SVC_OK		6
#define AS_SVC_UNKNOWN		7
#define AS_SVC_WARNING		8
#define AS_SVC_CRITICAL		9

#define AS_SOFT_STATE           1
#define AS_HARD_STATE           2

/* display types */
#define DISPLAY_HOST_TRENDS	0
#define DISPLAY_SERVICE_TRENDS	1
#define DISPLAY_NO_TRENDS	2

/* input types */
#define GET_INPUT_NONE          0
#define GET_INPUT_TARGET_TYPE   1
#define GET_INPUT_HOST_TARGET   2
#define GET_INPUT_SERVICE_TARGET 3
#define GET_INPUT_OPTIONS       4

#define MIN_TIMESTAMP_SPACING	10

#define MAX_ARCHIVE_SPREAD	65
#define MAX_ARCHIVE		65
#define MAX_ARCHIVE_BACKTRACKS	60

authdata current_authdata;

typedef struct archived_state_struct {
	time_t  time_stamp;
	int     entry_type;
	int     processed_state;
	int     state_type;
	char    *state_info;
	struct archived_state_struct *next;
} archived_state;


archived_state *as_list = NULL;

time_t t1;
time_t t2;

int start_second = 0;
int start_minute = 0;
int start_hour = 0;
int start_day = 1;
int start_month = 1;
int start_year = 2000;
int end_second = 0;
int end_minute = 0;
int end_hour = 24;
int end_day = 1;
int end_month = 1;
int end_year = 2000;

int input_type = GET_INPUT_NONE;
int timeperiod_type = TIMEPERIOD_LAST24HOURS;
int compute_time_from_parts = FALSE;

int display_popups = TRUE;
int use_map = TRUE;
int small_image = FALSE;
extern int embedded;
extern int display_header;
extern int daemon_check;
extern int content_type;

int assume_initial_states = TRUE;
int assume_state_retention = TRUE;
int assume_states_during_notrunning = TRUE;
int include_soft_states = FALSE;
int ignore_daemon_restart = FALSE;


void graph_all_trend_data(void);
void graph_trend_data(int, int, time_t, time_t, time_t, char *);
void draw_timestamps(void);
void draw_timestamp(int, time_t);
void draw_time_breakdowns(void);
void draw_horizontal_grid_lines(void);
void draw_dashed_line(int, int, int, int, int);

int convert_host_state_to_archived_state(int);
int convert_service_state_to_archived_state(int);
void add_archived_state(int, int, time_t, char *);
void free_archived_state_list(void);
void read_archived_state_data(void);
void scan_log_file_for_archived_state_data(char *);
void scan_log_file_for_archived_state_data_old(char *);
void compute_report_times(void);
void get_time_breakdown_string(unsigned long, unsigned long, char *, char *buffer, int);

int process_cgivars(void);

gdImagePtr trends_image = 0;
int color_white = 0;
int color_black = 0;
int color_red = 0;
int color_green = 0;
int color_darkgreen = 0;
int color_yellow = 0;
int color_pink = 0;
int color_darkpink = 0;
FILE *image_file = NULL;

int image_width = 900;
int image_height = 300;

#define HOST_DRAWING_WIDTH	498
#define HOST_DRAWING_HEIGHT	70
#define HOST_DRAWING_X_OFFSET	116
#define HOST_DRAWING_Y_OFFSET	55

#define SVC_DRAWING_WIDTH	498
#define SVC_DRAWING_HEIGHT	90
#define SVC_DRAWING_X_OFFSET	116
#define SVC_DRAWING_Y_OFFSET	55

#define SMALL_HOST_DRAWING_WIDTH    500
#define SMALL_HOST_DRAWING_HEIGHT   20
#define SMALL_HOST_DRAWING_X_OFFSET 0
#define SMALL_HOST_DRAWING_Y_OFFSET 0

#define SMALL_SVC_DRAWING_WIDTH     500
#define SMALL_SVC_DRAWING_HEIGHT    20
#define SMALL_SVC_DRAWING_X_OFFSET  0
#define SMALL_SVC_DRAWING_Y_OFFSET  0

int drawing_width = 0;
int drawing_height = 0;

int drawing_x_offset = 0;
int drawing_y_offset = 0;

int last_known_state = AS_NO_DATA;

int zoom_factor = 4;
int backtrack_archives = 2;
int earliest_archive = 0;
time_t earliest_time;
time_t latest_time;
int earliest_state = AS_NO_DATA;
int latest_state = AS_NO_DATA;

int initial_assumed_host_state = AS_NO_DATA;
int initial_assumed_service_state = AS_NO_DATA;

unsigned long time_up = 0L;
unsigned long time_down = 0L;
unsigned long time_unreachable = 0L;
unsigned long time_ok = 0L;
unsigned long time_warning = 0L;
unsigned long time_unknown = 0L;
unsigned long time_critical = 0L;

int display_type = DISPLAY_NO_TRENDS;

char *host_name = "";
char *service_desc = "";

int CGI_ID = TRENDS_CGI_ID;

int main(int argc, char **argv) {
	int result = OK;
	char temp_buffer[MAX_INPUT_BUFFER];
	char image_template[MAX_INPUT_BUFFER];
	char start_time[MAX_INPUT_BUFFER];
	char end_time[MAX_INPUT_BUFFER];
	int string_width;
	int string_height;
	char start_timestring[MAX_INPUT_BUFFER];
	char end_timestring[MAX_INPUT_BUFFER];
	host *temp_host = NULL;
	service *temp_service = NULL;
	int is_authorized = TRUE;
	int problem_found = FALSE;
	int days, hours, minutes, seconds;
	time_t t3;
	time_t current_time;
	struct tm *t;
	time_t old_t1;
	time_t old_t2;
	archived_state *temp_as;
	time_t problem_t1 = 0;
	time_t problem_t2 = 0;
	time_t margin;

	/* reset internal CGI variables */
	reset_cgi_vars();

	/* read the CGI configuration file */
	result = read_cgi_config_file(get_cgi_config_location());
	if (result == ERROR) {
		if (content_type == HTML_CONTENT) {
			document_header(CGI_ID, FALSE, "Error");
			print_error(get_cgi_config_location(), ERROR_CGI_CFG_FILE);
			document_footer(CGI_ID);
		}
		return ERROR;
	}

	/* read the main configuration file */
	result = read_main_config_file(main_config_file);
	if (result == ERROR) {
		if (content_type == HTML_CONTENT) {
			document_header(CGI_ID, FALSE, "Error");
			print_error(main_config_file, ERROR_CGI_MAIN_CFG);
			document_footer(CGI_ID);
		}
		return ERROR;
	}


	/* initialize time period to last 24 hours */
	time(&current_time);
	t2 = current_time;
	t1 = (time_t)(current_time - (60 * 60 * 24));

	/* default number of backtracked archives */
	switch (log_rotation_method) {
	case LOG_ROTATION_MONTHLY:
		backtrack_archives = 1;
		break;
	case LOG_ROTATION_WEEKLY:
		backtrack_archives = 2;
		break;
	case LOG_ROTATION_DAILY:
		backtrack_archives = 4;
		break;
	case LOG_ROTATION_HOURLY:
		backtrack_archives = 8;
		break;
	default:
		backtrack_archives = 2;
		break;
	}

	/* get the arguments passed in the URL */
	process_cgivars();

	result = read_all_object_configuration_data(main_config_file, READ_ALL_OBJECT_DATA);
	if (result == ERROR) {
		if (content_type == HTML_CONTENT) {
			document_header(CGI_ID, FALSE, "Error");
			print_error(NULL, ERROR_CGI_OBJECT_DATA);
			document_footer(CGI_ID);
		}
		return ERROR;
	}

	/* read all status data */
	result = read_all_status_data(get_cgi_config_location(), READ_ALL_STATUS_DATA);
	if (result == ERROR && daemon_check == TRUE) {
		if (content_type == HTML_CONTENT) {
			document_header(CGI_ID, FALSE, "Error");
			print_error(NULL, ERROR_CGI_STATUS_DATA);
			document_footer(CGI_ID);
		}
		return ERROR;
	}

	document_header(CGI_ID, TRUE, "Trends");

	/* get authentication information */
	get_authentication_information(&current_authdata);

	if (compute_time_from_parts == TRUE)
		compute_report_times();

	/* make sure times are sane, otherwise swap them */
	if (t2 < t1) {
		t3 = t2;
		t2 = t1;
		t1 = t3;
	}

	/* don't let user create reports in the future */
	if (t2 > current_time) {
		t2 = current_time;
		if (t1 > t2)
			t1 = t2 - (60 * 60 * 24);
	}

	if (content_type == HTML_CONTENT && display_header == TRUE) {

		old_t1 = t1;
		old_t2 = t2;

		/* begin top table */
		printf("<table border=0 width=100%% cellspacing=0 cellpadding=0>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=33%%>\n");

		if (display_type == DISPLAY_HOST_TRENDS)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Host State Trends");
		else if (display_type == DISPLAY_SERVICE_TRENDS)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Service State Trends");
		else
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Host and Service State Trends");
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		display_info_table(temp_buffer, &current_authdata, daemon_check);

		if (timeperiod_type == TIMEPERIOD_NEXTPROBLEM) {
			problem_t1 = 0;
			problem_t2 = 0;

			t1 = t2;
			t2 = current_time;
			read_archived_state_data();

			if (display_type == DISPLAY_HOST_TRENDS) {
				for (temp_as = as_list; temp_as != NULL; temp_as = temp_as->next) {
					if ((temp_as->entry_type == HOST_DOWN || temp_as->entry_type == HOST_UNREACHABLE) && temp_as->time_stamp > t1) {
						problem_t1 = temp_as->time_stamp;
						problem_found = TRUE;
						break;
					}
				}

				if (problem_found == TRUE) {
					for (; temp_as != NULL; temp_as = temp_as->next) {
						if (temp_as->entry_type == AS_HOST_UP && temp_as->time_stamp > problem_t1) {
							problem_t2 = temp_as->time_stamp;
							break;
						}
					}
				}
			} else {
				for (temp_as = as_list; temp_as != NULL; temp_as = temp_as->next) {
					if ((temp_as->entry_type == AS_SVC_UNKNOWN || temp_as->entry_type == AS_SVC_CRITICAL || temp_as->entry_type == AS_SVC_WARNING) && temp_as->time_stamp > t1) {
						problem_t1 = temp_as->time_stamp;
						problem_found = TRUE;
						break;
					}
				}

				if (problem_found == TRUE) {
					for (; temp_as != NULL; temp_as = temp_as->next) {
						if (temp_as->entry_type == AS_SVC_OK && temp_as->time_stamp > problem_t1) {
							problem_t2 = temp_as->time_stamp;
							break;
						}
					}
				}
			}

			if (problem_found == TRUE) {
				if (problem_t2 == 0) {
					margin = 12 * 60 * 60;
					problem_t2 = problem_t1;
				} else {
					margin = (problem_t2 - problem_t1) / 2;
				}

				t1 = problem_t1 - margin;
				t2 = problem_t2 + margin;
			}
		}

		if (timeperiod_type == TIMEPERIOD_NEXTPROBLEM && problem_found == FALSE) {
			t1 = old_t1;
			t2 = old_t2;
		}

		if (display_type != DISPLAY_NO_TRENDS && input_type == GET_INPUT_NONE) {

			printf("<TABLE BORDER=1 CELLPADDING=0 CELLSPACING=0 CLASS='linkBox'>\n");
			printf("<TR><TD CLASS='linkBox'>\n");

			if (display_type == DISPLAY_HOST_TRENDS) {
				printf("<a href='%s?host=%s&t1=%lu&t2=%lu&includesoftstates=%s&assumestateretention=%s&assumeinitialstates=%s&assumestatesduringnotrunning=%s&initialassumedhoststate=%d&backtrack=%d&show_log_entries'>View <b>Availability Report</b> For <b>This Host</b></a><br>\n", AVAIL_CGI, url_encode(host_name), t1, t2, (include_soft_states == TRUE) ? "yes" : "no", (assume_state_retention == TRUE) ? "yes" : "no", (assume_initial_states == TRUE) ? "yes" : "no", (assume_states_during_notrunning == TRUE) ? "yes" : "no", initial_assumed_host_state, backtrack_archives);
#ifdef USE_HISTOGRAM
				printf("<a href='%s?host=%s&t1=%lu&t2=%lu&assumestateretention=%s'>View <b>Alert Histogram</b> For <b>This Host</b></a><br>\n", HISTOGRAM_CGI, url_encode(host_name), t1, t2, (assume_state_retention == TRUE) ? "yes" : "no");
#endif
				printf("<a href='%s?host=%s'>View <b>Service Status Detail</b> For <b>This Host</b></a><br>\n", STATUS_CGI, url_encode(host_name));
				printf("<a href='%s?type=%d&host=%s'>View <b>Information</b> For <b>This Host</b></a><br>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(host_name));
				printf("<a href='%s?host=%s'>View <b>Alert History</b> For <b>This Host</b></a><br>\n", HISTORY_CGI, url_encode(host_name));
				printf("<a href='%s?host=%s'>View <b>Notifications</b> For <b>This Host</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(host_name));
			} else {
				printf("<a href='%s?host=%s&t1=%lu&t2=%lu&includesoftstates=%s&assumestateretention=%s&assumeinitialstates=%s&assumestatesduringnotrunning=%s&initialassumedservicestate=%d&backtrack=%d'>View <b>Trends</b> For <b>This Host</b></a><br>\n", TRENDS_CGI, url_encode(host_name), t1, t2, (include_soft_states == TRUE) ? "yes" : "no", (assume_state_retention == TRUE) ? "yes" : "no", (assume_initial_states == TRUE) ? "yes" : "no", (assume_states_during_notrunning == TRUE) ? "yes" : "no", initial_assumed_service_state, backtrack_archives);
				printf("<a href='%s?host=%s", AVAIL_CGI, url_encode(host_name));
				printf("&service=%s&t1=%lu&t2=%lu&assumestateretention=%s&includesoftstates=%s&assumeinitialstates=%s&assumestatesduringnotrunning=%s&initialassumedservicestate=%d&backtrack=%d&show_log_entries'>View <b>Availability Report</b> For <b>This Service</b></a><br>\n", url_encode(service_desc), t1, t2, (include_soft_states == TRUE) ? "yes" : "no", (assume_state_retention == TRUE) ? "yes" : "no", (assume_initial_states == TRUE) ? "yes" : "no", (assume_states_during_notrunning == TRUE) ? "yes" : "no", initial_assumed_service_state, backtrack_archives);
#ifdef USE_HISTOGRAM
				printf("<a href='%s?host=%s&service=%s&t1=%lu&t2=%lu&assumestateretention=%s'>View <b>Alert Histogram</b> For <b>This Service</b></a><br>\n", HISTOGRAM_CGI, url_encode(host_name), url_encode(service_desc), t1, t2, (assume_state_retention == TRUE) ? "yes" : "no");
#endif
				printf("<a href='%s?type=%d&host=%s&service=%s'>View <b>Information</b> For <b>This Service</b></a><br>\n", EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(host_name), url_encode(service_desc));
				printf("<a href='%s?host=%s&service=%s'>View <b>Alert History</b> For <b>This Service</b></a><br>\n", HISTORY_CGI, url_encode(host_name), url_encode(service_desc));
				printf("<a href='%s?host=%s&service=%s'>View <b>Notifications</b> For <b>This Service</b></a><br>\n", NOTIFICATIONS_CGI, url_encode(host_name), url_encode(service_desc));
			}

			printf("</TD></TR>\n");
			printf("</TABLE>\n");
		}

		printf("</td>\n");

		/* center column of top row */
		printf("<td align=center valign=top width=33%%>\n");

		if (display_type != DISPLAY_NO_TRENDS && input_type == GET_INPUT_NONE) {


			/* find the host */
			temp_host = find_host(host_name);

			/* find the service */
			temp_service = find_service(host_name, service_desc);

			printf("<DIV ALIGN=CENTER CLASS='dataTitle'>\n");
			if (display_type == DISPLAY_HOST_TRENDS)
				printf("Host '%s'", (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);
			else if (display_type == DISPLAY_SERVICE_TRENDS)
				printf("Service '%s' On Host '%s'", (temp_service->display_name != NULL) ? temp_service->display_name : temp_service->description, (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);
			printf("</DIV>\n");

			printf("<BR>\n");

			printf("<IMG SRC='%s%s' BORDER=0 ALT='%s State Trends' TITLE='%s State Trends'>\n", url_images_path, TRENDS_ICON, (display_type == DISPLAY_HOST_TRENDS) ? "Host" : "Service", (display_type == DISPLAY_HOST_TRENDS) ? "Host" : "Service");

			printf("<BR CLEAR=ALL>\n");

			get_time_string(&t1, start_timestring, sizeof(start_timestring) - 1, SHORT_DATE_TIME);
			get_time_string(&t2, end_timestring, sizeof(end_timestring) - 1, SHORT_DATE_TIME);
			printf("<div align=center class='reportRange'>%s to %s</div>\n", start_timestring, end_timestring);

			get_time_breakdown((time_t)(t2 - t1), &days, &hours, &minutes, &seconds);
			printf("<div align=center class='reportDuration'>Duration: %dd %dh %dm %ds</div>\n", days, hours, minutes, seconds);
		}

		printf("</td>\n");

		/* right hand column of top row */
		printf("<td align=right valign=bottom width=33%%>\n");

		printf("<form method=\"GET\" action=\"%s\">\n", TRENDS_CGI);
		printf("<table border=0 CLASS='optBox'>\n");

		if (display_type != DISPLAY_NO_TRENDS && input_type == GET_INPUT_NONE) {

			printf("<tr><td CLASS='optBoxItem' valign=top align=left>First assumed %s state:</td><td CLASS='optBoxItem' valign=top align=left>Backtracked archives:</td></tr>\n", (display_type == DISPLAY_HOST_TRENDS) ? "host" : "service");
			printf("<tr><td CLASS='optBoxItem' valign=top align=left>");

			if (display_popups == FALSE)
				printf("<input type='hidden' name='nopopups' value=''>\n");
			if (use_map == FALSE)
				printf("<input type='hidden' name='nomap' value=''>\n");
			if (ignore_daemon_restart == TRUE)
				printf("<input type='hidden' name='ignorerestart' value=''>\n");
			printf("<input type='hidden' name='t1' value='%lu'>\n", (unsigned long)t1);
			printf("<input type='hidden' name='t2' value='%lu'>\n", (unsigned long)t2);
			printf("<input type='hidden' name='host' value='%s'>\n", escape_string(host_name));
			if (display_type == DISPLAY_SERVICE_TRENDS)
				printf("<input type='hidden' name='service' value='%s'>\n", escape_string(service_desc));

			printf("<input type='hidden' name='assumeinitialstates' value='%s'>\n", (assume_initial_states == TRUE) ? "yes" : "no");
			printf("<input type='hidden' name='assumestateretention' value='%s'>\n", (assume_state_retention == TRUE) ? "yes" : "no");
			printf("<input type='hidden' name='assumestatesduringnotrunning' value='%s'>\n", (assume_states_during_notrunning == TRUE) ? "yes" : "no");
			printf("<input type='hidden' name='includesoftstates' value='%s'>\n", (include_soft_states == TRUE) ? "yes" : "no");

			if (display_type == DISPLAY_HOST_TRENDS) {
				printf("<input type='hidden' name='initialassumedservicestate' value='%d'>", initial_assumed_service_state);
				printf("<select name='initialassumedhoststate'>\n");
				printf("<option value=%d %s>Unspecified\n", AS_NO_DATA, (initial_assumed_host_state == AS_NO_DATA) ? "SELECTED" : "");
				printf("<option value=%d %s>Current State\n", AS_CURRENT_STATE, (initial_assumed_host_state == AS_CURRENT_STATE) ? "SELECTED" : "");
				printf("<option value=%d %s>Host Up\n", AS_HOST_UP, (initial_assumed_host_state == AS_HOST_UP) ? "SELECTED" : "");
				printf("<option value=%d %s>Host Down\n", AS_HOST_DOWN, (initial_assumed_host_state == AS_HOST_DOWN) ? "SELECTED" : "");
				printf("<option value=%d %s>Host Unreachable\n", AS_HOST_UNREACHABLE, (initial_assumed_host_state == AS_HOST_UNREACHABLE) ? "SELECTED" : "");
			} else {
				printf("<input type='hidden' name='initialassumedhoststate' value='%d'>", initial_assumed_host_state);
				printf("<select name='initialassumedservicestate'>\n");
				printf("<option value=%d %s>Unspecified\n", AS_NO_DATA, (initial_assumed_service_state == AS_NO_DATA) ? "SELECTED" : "");
				printf("<option value=%d %s>Current State\n", AS_CURRENT_STATE, (initial_assumed_service_state == AS_CURRENT_STATE) ? "SELECTED" : "");
				printf("<option value=%d %s>Service Ok\n", AS_SVC_OK, (initial_assumed_service_state == AS_SVC_OK) ? "SELECTED" : "");
				printf("<option value=%d %s>Service Warning\n", AS_SVC_WARNING, (initial_assumed_service_state == AS_SVC_WARNING) ? "SELECTED" : "");
				printf("<option value=%d %s>Service Unknown\n", AS_SVC_UNKNOWN, (initial_assumed_service_state == AS_SVC_UNKNOWN) ? "SELECTED" : "");
				printf("<option value=%d %s>Service Critical\n", AS_SVC_CRITICAL, (initial_assumed_service_state == AS_SVC_CRITICAL) ? "SELECTED" : "");
			}
			printf("</select>\n");
			printf("</td><td CLASS='optBoxItem' valign=top align=left>\n");
			printf("<input type='text' name='backtrack' size='2' maxlength='2' value='%d'>\n", backtrack_archives);
			printf("</td></tr>\n");

			printf("<tr><td CLASS='optBoxItem' valign=top align=left>Report period:</td><td CLASS='optBoxItem' valign=top align=left>Zoom factor:</td></tr>\n");
			printf("<tr><td CLASS='optBoxItem' valign=top align=left>\n");
			printf("<select name='timeperiod'>\n");
			printf("<option>[ Current time range ]\n");
			printf("<option value=today %s>Today\n", (timeperiod_type == TIMEPERIOD_TODAY) ? "SELECTED" : "");
			printf("<option value=last24hours %s>Last 24 Hours\n", (timeperiod_type == TIMEPERIOD_LAST24HOURS) ? "SELECTED" : "");
			printf("<option value=yesterday %s>Yesterday\n", (timeperiod_type == TIMEPERIOD_YESTERDAY) ? "SELECTED" : "");
			printf("<option value=thisweek %s>This Week\n", (timeperiod_type == TIMEPERIOD_THISWEEK) ? "SELECTED" : "");
			printf("<option value=last7days %s>Last 7 Days\n", (timeperiod_type == TIMEPERIOD_LAST7DAYS) ? "SELECTED" : "");
			printf("<option value=lastweek %s>Last Week\n", (timeperiod_type == TIMEPERIOD_LASTWEEK) ? "SELECTED" : "");
			printf("<option value=thismonth %s>This Month\n", (timeperiod_type == TIMEPERIOD_THISMONTH) ? "SELECTED" : "");
			printf("<option value=last31days %s>Last 31 Days\n", (timeperiod_type == TIMEPERIOD_LAST31DAYS) ? "SELECTED" : "");
			printf("<option value=lastmonth %s>Last Month\n", (timeperiod_type == TIMEPERIOD_LASTMONTH) ? "SELECTED" : "");
			printf("<option value=thisyear %s>This Year\n", (timeperiod_type == TIMEPERIOD_THISYEAR) ? "SELECTED" : "");
			printf("<option value=lastyear %s>Last Year\n", (timeperiod_type == TIMEPERIOD_LASTYEAR) ? "SELECTED" : "");
			if (display_type == DISPLAY_HOST_TRENDS)
				printf("<option value=nextproblem %s>Next Host Problem\n", (timeperiod_type == TIMEPERIOD_NEXTPROBLEM) ? "SELECTED" : "");
			else
				printf("<option value=nextproblem %s>Next Service Problem\n", (timeperiod_type == TIMEPERIOD_NEXTPROBLEM) ? "SELECTED" : "");
			printf("</select>\n");
			printf("</td><td CLASS='optBoxItem' valign=top align=left>\n");
			printf("<select name='zoom'>\n");
			printf("<option value=%d selected>%d\n", zoom_factor, zoom_factor);
			printf("<option value=+2>+2\n");
			printf("<option value=+3>+3\n");
			printf("<option value=+4>+4\n");
			printf("<option value=-2>-2\n");
			printf("<option value=-3>-3\n");
			printf("<option value=-4>-4\n");
			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td CLASS='optBoxItem' valign=top align=left>\n");
			printf("</td><td CLASS='optBoxItem' valign=top align=left>\n");
			printf("<input type='submit' value='Update'>\n");
			printf("</td></tr>\n");
		}

		printf("</table>\n");
		printf("</form>\n");

		print_export_link(HTML_CONTENT, TRENDS_CGI, NULL);

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");
	}


#ifndef DEBUG

	/* check authorization... */
	if (display_type == DISPLAY_HOST_TRENDS) {
		temp_host = find_host(host_name);
		if (temp_host == NULL || is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			is_authorized = FALSE;
	} else if (display_type == DISPLAY_SERVICE_TRENDS) {
		temp_service = find_service(host_name, service_desc);
		if (temp_service == NULL || is_authorized_for_service(temp_service, &current_authdata) == FALSE)
			is_authorized = FALSE;
	}
	if (is_authorized == FALSE) {

		if (content_type == HTML_CONTENT) {
			if (display_type == DISPLAY_HOST_TRENDS)
				print_generic_error_message("It appears as though you are not authorized to view information for the specified host...", NULL, 0);
			else
				print_generic_error_message("It appears as though you are not authorized to view information for the specified service...", NULL, 0);
		}

		document_footer(CGI_ID);
		free_memory();
		return ERROR;
	}
#endif

	if (timeperiod_type == TIMEPERIOD_NEXTPROBLEM && problem_found == FALSE) {
		print_generic_error_message("No problem found between end of display and end of recording.", NULL, 0);

		document_footer(CGI_ID);
		free_memory();
		return ERROR;
	}

	/* set drawing parameters, etc */

	if (small_image == TRUE) {
		image_height = 20;
		image_width = 500;
	} else {
		image_height = 300;
		image_width = 900;
	}

	if (display_type == DISPLAY_HOST_TRENDS) {

		if (small_image == TRUE) {
			drawing_width = SMALL_HOST_DRAWING_WIDTH;
			drawing_height = SMALL_HOST_DRAWING_HEIGHT;
			drawing_x_offset = SMALL_HOST_DRAWING_X_OFFSET;
			drawing_y_offset = SMALL_HOST_DRAWING_Y_OFFSET;
		} else {
			drawing_width = HOST_DRAWING_WIDTH;
			drawing_height = HOST_DRAWING_HEIGHT;
			drawing_x_offset = HOST_DRAWING_X_OFFSET;
			drawing_y_offset = HOST_DRAWING_Y_OFFSET;
		}
	} else if (display_type == DISPLAY_SERVICE_TRENDS) {

		if (small_image == TRUE) {
			drawing_width = SMALL_SVC_DRAWING_WIDTH;
			drawing_height = SMALL_SVC_DRAWING_HEIGHT;
			drawing_x_offset = SMALL_SVC_DRAWING_X_OFFSET;
			drawing_y_offset = SMALL_SVC_DRAWING_Y_OFFSET;
		} else {
			drawing_width = SVC_DRAWING_WIDTH;
			drawing_height = SVC_DRAWING_HEIGHT;
			drawing_x_offset = SVC_DRAWING_X_OFFSET;
			drawing_y_offset = SVC_DRAWING_Y_OFFSET;
		}
	}

	/* last known state should always be initially set to indeterminate! */
	last_known_state = AS_NO_DATA;


	/* initialize PNG image */
	if (display_type != DISPLAY_NO_TRENDS && content_type == IMAGE_CONTENT) {

		if (small_image == TRUE) {
			trends_image = gdImageCreate(image_width, image_height);
			if (trends_image == NULL) {
#ifdef DEBUG
				printf("Error: Could not allocate memory for image\n");
#endif
				return ERROR;
			}
		}

		else {

			if (display_type == DISPLAY_HOST_TRENDS)
				snprintf(image_template, sizeof(image_template) - 1, "%s%s", physical_images_path, TRENDSHOSTS_IMAGE);
			else
				snprintf(image_template, sizeof(image_template) - 1, "%s%s", physical_images_path, TRENDSSERVICES_IMAGE);
			image_template[sizeof(image_template)-1] = '\x0';

			/* allocate buffer for storing image */
			trends_image = NULL;
			image_file = fopen(image_template, "r");
			if (image_file != NULL) {
				trends_image = gdImageCreateFromPng(image_file);
				fclose(image_file);
			}
			if (trends_image == NULL)
				trends_image = gdImageCreate(image_width, image_height);
			if (trends_image == NULL) {
#ifdef DEBUG
				printf("Error: Could not allocate memory for image\n");
#endif
				return ERROR;
			}
		}

		/* allocate colors used for drawing */
		color_white = gdImageColorAllocate(trends_image, 255, 255, 255);
		color_black = gdImageColorAllocate(trends_image, 0, 0, 0);
		color_red = gdImageColorAllocate(trends_image, 255, 0, 0);
		color_green = gdImageColorAllocate(trends_image, 0, 210, 0);
		color_darkgreen = gdImageColorAllocate(trends_image, 0, 128, 0);
		color_yellow = gdImageColorAllocate(trends_image, 176, 178, 20);
		color_pink = gdImageColorAllocate(trends_image, 224, 102, 255);
		color_darkpink = gdImageColorAllocate(trends_image, 166, 75, 189);

		/* set transparency index */
		gdImageColorTransparent(trends_image, color_white);

		/* make sure the graphic is interlaced */
		gdImageInterlace(trends_image, 1);

		if (small_image == FALSE) {

			/* find the host */
			temp_host = find_host(host_name);

			/* find the service */
			temp_service = find_service(host_name, service_desc);

			/* title */
			snprintf(start_time, sizeof(start_time) - 1, "%s", ctime(&t1));
			start_time[sizeof(start_time)-1] = '\x0';
			start_time[strlen(start_time)-1] = '\x0';
			snprintf(end_time, sizeof(end_time) - 1, "%s", ctime(&t2));
			end_time[sizeof(end_time)-1] = '\x0';
			end_time[strlen(end_time)-1] = '\x0';

			string_height = gdFontSmall->h;

			if (display_type == DISPLAY_HOST_TRENDS)
				snprintf(temp_buffer, sizeof(temp_buffer) - 1, "State History For Host '%s'", (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);
			else
				snprintf(temp_buffer, sizeof(temp_buffer) - 1, "State History For Service '%s' On Host '%s'", (temp_service->display_name != NULL) ? temp_service->display_name : temp_service->description, (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			string_width = gdFontSmall->w * strlen(temp_buffer);
			gdImageString(trends_image, gdFontSmall, (drawing_width / 2) - (string_width / 2) + drawing_x_offset, string_height, (unsigned char *)temp_buffer, color_black);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "%s to %s", start_time, end_time);
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			string_width = gdFontSmall->w * strlen(temp_buffer);
			gdImageString(trends_image, gdFontSmall, (drawing_width / 2) - (string_width / 2) + drawing_x_offset, (string_height * 2) + 5, (unsigned char *)temp_buffer, color_black);


			/* first time stamp */
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "%s", start_time);
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			string_width = gdFontSmall->w * strlen(temp_buffer);
			gdImageStringUp(trends_image, gdFontSmall, drawing_x_offset - (string_height / 2), drawing_y_offset + drawing_height + string_width + 5, (unsigned char *)temp_buffer, color_black);
		}
	}

	if (display_type != DISPLAY_NO_TRENDS && input_type == GET_INPUT_NONE) {


		if (content_type == IMAGE_CONTENT || (content_type == HTML_CONTENT && use_map == TRUE)) {

			/* read in all necessary archived state data */
			read_archived_state_data();

			/* graph archived state trend data */
			graph_all_trend_data();
		}

		/* print URL to image */
		if (content_type == HTML_CONTENT) {

			printf("<BR><BR>\n");
			printf("<DIV ALIGN=CENTER>\n");
			printf("<IMG SRC='%s?createimage&t1=%lu&t2=%lu", TRENDS_CGI, (unsigned long)t1, (unsigned long)t2);
			printf("&assumeinitialstates=%s", (assume_initial_states == TRUE) ? "yes" : "no");
			printf("&assumestatesduringnotrunning=%s", (assume_states_during_notrunning == TRUE) ? "yes" : "no");
			printf("&initialassumedhoststate=%d", initial_assumed_host_state);
			printf("&initialassumedservicestate=%d", initial_assumed_service_state);
			printf("&assumestateretention=%s", (assume_state_retention == TRUE) ? "yes" : "no");
			printf("&includesoftstates=%s", (include_soft_states == TRUE) ? "yes" : "no");
			printf("&host=%s", url_encode(host_name));
			if (display_type == DISPLAY_SERVICE_TRENDS)
				printf("&service=%s", url_encode(service_desc));
			if (backtrack_archives > 0)
				printf("&backtrack=%d", backtrack_archives);
			printf("&zoom=%d", zoom_factor);
			printf("' BORDER=0 name='trendsimage' useMap='#trendsmap' width=%d>\n", image_width);
			printf("</DIV>\n");
		}

		if (content_type == IMAGE_CONTENT || (content_type == HTML_CONTENT && use_map == TRUE)) {

			/* draw timestamps */
			draw_timestamps();

			/* draw horizontal lines */
			draw_horizontal_grid_lines();

			/* draw state time breakdowns */
			draw_time_breakdowns();
		}

		if (content_type == IMAGE_CONTENT) {

			/* use STDOUT for writing the image data... */
			image_file = stdout;

#ifndef DEBUG
			/* write the image to file */
			gdImagePng(trends_image, image_file);
#endif
#ifdef DEBUG
			if (display_type == DISPLAY_HOST_TRENDS)
				snprintf(image_template, sizeof(image_template) - 1, "/tmp/%s", TRENDSHOSTS_IMAGE);
			else
				snprintf(image_template, sizeof(image_template) - 1, "/tmp/%s", TRENDSSERVICES_IMAGE);
			image_template[sizeof(image_template)-1] = '\x0';

			image_file = fopen(image_template, "w");
			if (image_file == NULL)
				printf("Could not open \"%s\" for writing!\n", image_template);
			else {
				gdImagePng(trends_image, image_file);
				fclose(image_file);
			}
#endif

			/* free memory allocated to image */
			gdImageDestroy(trends_image);
		}
	}


	/* show user a selection of hosts and services to choose from... */
	if (display_type == DISPLAY_NO_TRENDS || input_type != GET_INPUT_NONE) {

		/* ask the user for what host they want a report for */
		if (input_type == GET_INPUT_HOST_TARGET) {

			printf("<DIV CLASS='reportSelectTitle'>Step 2: Select Host</DIV>\n");

			printf("<form method=\"GET\" action=\"%s\">\n", TRENDS_CGI);
			printf("<input type='hidden' name='input' value='getoptions'>\n");

			printf("<TABLE BORDER=0 cellspacing=0 cellpadding=10 align='center'>\n");
			printf("<tr><td class='reportSelectSubTitle' valign=center>Host:</td>\n");
			printf("<td class='reportSelectItem' valign=center>\n");
			printf("<select name='host'>\n");

			for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {
				if (is_authorized_for_host(temp_host, &current_authdata) == TRUE)
					printf("<option value='%s'>%s\n", escape_string(temp_host->name), (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name);
			}

			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td></td><td class='reportSelectItem'>\n");
			printf("<input type='submit' value='Continue to Step 3'>\n");
			printf("</td></tr>\n");

			printf("</TABLE>\n");
			printf("</form>\n");
		}

		/* ask the user for what service they want a report for */
		else if (input_type == GET_INPUT_SERVICE_TARGET) {

			printf("<DIV CLASS='reportSelectTitle'>Step 2: Select Service</DIV>\n");

			printf("<form method=\"POST\" action=\"%s\" name=\"serviceform\">\n", TRENDS_CGI);
			printf("<input type='hidden' name='input' value='getoptions'>\n");

			printf("<TABLE BORDER=0 cellpadding=5 align='center'>\n");
			printf("<tr><td class='reportSelectSubTitle'>Service:</td>\n");
			printf("<td class='reportSelectItem'>\n");
			printf("<select name='hostservice'>\n");

			for (temp_service = service_list; temp_service != NULL; temp_service = temp_service->next) {
				if (is_authorized_for_service(temp_service, &current_authdata) == TRUE)
					temp_host = find_host(temp_service->host_name);
				printf("<option value='%s^%s'>%s;%s\n", escape_string(temp_service->host_name), escape_string(temp_service->description), (temp_host->display_name != NULL) ? temp_host->display_name : temp_host->name, (temp_service->display_name != NULL) ? temp_service->display_name : temp_service->description);
			}

			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td></td><td class='reportSelectItem'>\n");
			printf("<input type='submit' value='Continue to Step 3'>\n");
			printf("</td></tr>\n");

			printf("</TABLE>\n");
			printf("</form>\n");
		}

		/* ask the user for report range and options */
		else if (input_type == GET_INPUT_OPTIONS) {

			time(&current_time);
			t = localtime(&current_time);

			start_day = 1;
			start_year = t->tm_year + 1900;
			end_day = t->tm_mday;
			end_year = t->tm_year + 1900;

			printf("<DIV CLASS='reportSelectTitle'>Step 3: Select Report Options</DIV>\n");

			printf("<form method=\"GET\" action=\"%s\">\n", TRENDS_CGI);
			printf("<input type='hidden' name='host' value='%s'>\n", escape_string(host_name));
			if (display_type == DISPLAY_SERVICE_TRENDS)
				printf("<input type='hidden' name='service' value='%s'>\n", escape_string(service_desc));

			printf("<TABLE BORDER=0 CELLPADDING=5 align='center'>\n");
			printf("<tr><td class='reportSelectSubTitle' align=right>Report period:</td>\n");
			printf("<td class='reportSelectItem'>\n");
			printf("<select name='timeperiod'>\n");
			printf("<option value=today>Today\n");
			printf("<option value=last24hours>Last 24 Hours\n");
			printf("<option value=yesterday>Yesterday\n");
			printf("<option value=thisweek>This Week\n");
			printf("<option value=last7days SELECTED>Last 7 Days\n");
			printf("<option value=lastweek>Last Week\n");
			printf("<option value=thismonth>This Month\n");
			printf("<option value=last31days>Last 31 Days\n");
			printf("<option value=lastmonth>Last Month\n");
			printf("<option value=thisyear>This Year\n");
			printf("<option value=lastyear>Last Year\n");
			printf("<option value=custom>* CUSTOM REPORT PERIOD *\n");
			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td valign=top class='reportSelectSubTitle'>If Custom Report Period...</td></tr>\n");

			printf("<tr>");
			printf("<td valign=top class='reportSelectSubTitle'>Start Date (Inclusive):</td>\n");
			printf("<td align=left valign=top class='reportSelectItem'>");
			printf("<select name='smon'>\n");
			printf("<option value='1' %s>January\n", (t->tm_mon == 0) ? "SELECTED" : "");
			printf("<option value='2' %s>February\n", (t->tm_mon == 1) ? "SELECTED" : "");
			printf("<option value='3' %s>March\n", (t->tm_mon == 2) ? "SELECTED" : "");
			printf("<option value='4' %s>April\n", (t->tm_mon == 3) ? "SELECTED" : "");
			printf("<option value='5' %s>May\n", (t->tm_mon == 4) ? "SELECTED" : "");
			printf("<option value='6' %s>June\n", (t->tm_mon == 5) ? "SELECTED" : "");
			printf("<option value='7' %s>July\n", (t->tm_mon == 6) ? "SELECTED" : "");
			printf("<option value='8' %s>August\n", (t->tm_mon == 7) ? "SELECTED" : "");
			printf("<option value='9' %s>September\n", (t->tm_mon == 8) ? "SELECTED" : "");
			printf("<option value='10' %s>October\n", (t->tm_mon == 9) ? "SELECTED" : "");
			printf("<option value='11' %s>November\n", (t->tm_mon == 10) ? "SELECTED" : "");
			printf("<option value='12' %s>December\n", (t->tm_mon == 11) ? "SELECTED" : "");
			printf("</select>\n ");
			printf("<input type='text' size='2' maxlength='2' name='sday' value='%d'> ", start_day);
			printf("<input type='text' size='4' maxlength='4' name='syear' value='%d'>", start_year);
			printf("<input type='hidden' name='shour' value='0'>\n");
			printf("<input type='hidden' name='smin' value='0'>\n");
			printf("<input type='hidden' name='ssec' value='0'>\n");
			printf("</td>\n");
			printf("</tr>\n");

			printf("<tr>");
			printf("<td valign=top class='reportSelectSubTitle'>End Date (Inclusive):</td>\n");
			printf("<td align=left valign=top class='reportSelectItem'>");
			printf("<select name='emon'>\n");
			printf("<option value='1' %s>January\n", (t->tm_mon == 0) ? "SELECTED" : "");
			printf("<option value='2' %s>February\n", (t->tm_mon == 1) ? "SELECTED" : "");
			printf("<option value='3' %s>March\n", (t->tm_mon == 2) ? "SELECTED" : "");
			printf("<option value='4' %s>April\n", (t->tm_mon == 3) ? "SELECTED" : "");
			printf("<option value='5' %s>May\n", (t->tm_mon == 4) ? "SELECTED" : "");
			printf("<option value='6' %s>June\n", (t->tm_mon == 5) ? "SELECTED" : "");
			printf("<option value='7' %s>July\n", (t->tm_mon == 6) ? "SELECTED" : "");
			printf("<option value='8' %s>August\n", (t->tm_mon == 7) ? "SELECTED" : "");
			printf("<option value='9' %s>September\n", (t->tm_mon == 8) ? "SELECTED" : "");
			printf("<option value='10' %s>October\n", (t->tm_mon == 9) ? "SELECTED" : "");
			printf("<option value='11' %s>November\n", (t->tm_mon == 10) ? "SELECTED" : "");
			printf("<option value='12' %s>December\n", (t->tm_mon == 11) ? "SELECTED" : "");
			printf("</select>\n ");
			printf("<input type='text' size='2' maxlength='2' name='eday' value='%d'> ", end_day);
			printf("<input type='text' size='4' maxlength='4' name='eyear' value='%d'>", end_year);
			printf("<input type='hidden' name='ehour' value='24'>\n");
			printf("<input type='hidden' name='emin' value='0'>\n");
			printf("<input type='hidden' name='esec' value='0'>\n");
			printf("</td>\n");
			printf("</tr>\n");

			printf("<tr><td colspan=2><br></td></tr>\n");

			printf("<tr><td class='reportSelectSubTitle' align=right>Assume Initial States:</td>\n");
			printf("<td class='reportSelectItem'>\n");
			printf("<select name='assumeinitialstates'>\n");
			printf("<option value=yes>Yes\n");
			printf("<option value=no>No\n");
			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td class='reportSelectSubTitle' align=right>Assume State Retention:</td>\n");
			printf("<td class='reportSelectItem'>\n");
			printf("<select name='assumestateretention'>\n");
			printf("<option value=yes>Yes\n");
			printf("<option value=no>No\n");
			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td class='reportSelectSubTitle' align=right>Assume States During Program Downtime:</td>\n");
			printf("<td class='reportSelectItem'>\n");
			printf("<select name='assumestatesduringnotrunning'>\n");
			printf("<option value=yes>Yes\n");
			printf("<option value=no>No\n");
			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td class='reportSelectSubTitle' align=right>Include Soft States:</td>\n");
			printf("<td class='reportSelectItem'>\n");
			printf("<select name='includesoftstates'>\n");
			printf("<option value=yes>Yes\n");
			printf("<option value=no SELECTED>No\n");
			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td class='reportSelectSubTitle' align=right>First Assumed %s State:</td>\n", (display_type == DISPLAY_HOST_TRENDS) ? "Host" : "Service");
			printf("<td class='reportSelectItem'>\n");
			if (display_type == DISPLAY_HOST_TRENDS) {
				printf("<select name='initialassumedhoststate'>\n");
				printf("<option value=%d>Unspecified\n", AS_NO_DATA);
				printf("<option value=%d>Current State\n", AS_CURRENT_STATE);
				printf("<option value=%d>Host Up\n", AS_HOST_UP);
				printf("<option value=%d>Host Down\n", AS_HOST_DOWN);
				printf("<option value=%d>Host Unreachable\n", AS_HOST_UNREACHABLE);
			} else {
				printf("<select name='initialassumedservicestate'>\n");
				printf("<option value=%d>Unspecified\n", AS_NO_DATA);
				printf("<option value=%d>Current State\n", AS_CURRENT_STATE);
				printf("<option value=%d>Service Ok\n", AS_SVC_OK);
				printf("<option value=%d>Service Warning\n", AS_SVC_WARNING);
				printf("<option value=%d>Service Unknown\n", AS_SVC_UNKNOWN);
				printf("<option value=%d>Service Critical\n", AS_SVC_CRITICAL);
			}
			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td class='reportSelectSubTitle' align=right>Backtracked Archives (To Scan For Initial States):</td>\n");
			printf("<td class='reportSelectItem'>\n");
			printf("<input type='text' name='backtrack' size='2' maxlength='2' value='%d'>\n", backtrack_archives);
			printf("</td></tr>\n");

			printf("<tr><td class='reportSelectSubTitle' align=right>Suppress image map:</td><td class='reportSelectItem'><input type='checkbox' name='nomap'></td></tr>");
			printf("<tr><td class='reportSelectSubTitle' align=right>Suppress popups:</td><td class='reportSelectItem'><input type='checkbox' name='nopopups'></td></tr>\n");

			printf("<tr><td class='reportSelectSubTitle' align=right>Ignore daemon starts/restarts:</td><td class='reportSelectItem'><input type='checkbox' name='ignorerestart'></td></tr>\n");

			printf("<tr><td></td><td class='reportSelectItem'><input type='submit' value='Create Report'></td></tr>\n");

			printf("</TABLE>\n");
			printf("</form>\n");

			/*
			printf("<P><DIV ALIGN=CENTER CLASS='helpfulHint'>\n");
			printf("Note: Choosing the 'suppress image map' option will make the report run approximately twice as fast as it would otherwise, but it will prevent you from being able to zoom in on specific time periods.\n");
			printf("</DIV></P>\n");
			*/
		}

		/* as the user whether they want a graph for a host or service */
		else {
			printf("<DIV CLASS='reportSelectTitle'>Step 1: Select Report Type</DIV>\n");

			printf("<form method=\"GET\" action=\"%s\">\n", TRENDS_CGI);
			printf("<TABLE BORDER=0 cellpadding=5 align='center'>\n");

			printf("<tr><td class='reportSelectSubTitle' align=right>Type:</td>\n");
			printf("<td class='reportSelectItem'>\n");
			printf("<select name='input'>\n");
			printf("<option value=gethost>Host\n");
			printf("<option value=getservice>Service\n");
			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td></td><td class='reportSelectItem'>\n");
			printf("<input type='submit' value='Continue to Step 2'>\n");
			printf("</td></tr>\n");

			printf("</TABLE>\n");
			printf("</form>\n");
		}

	}

	document_footer(CGI_ID);

	/* free memory allocated to the archived state data list */
	free_archived_state_list();

	/* free all other allocated memory */
	free_memory();

	return OK;
}

int process_cgivars(void) {
	char **variables;
	char *temp_buffer = NULL;
	int error = FALSE;
	int x;

	variables = getcgivars();

	for (x = 0; variables[x] != NULL; x++) {

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if (strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
			x++;
			continue;
		}

		/* we found the host argument */
		else if (!strcmp(variables[x], "host")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if ((host_name = (char *)strdup(variables[x])) == NULL)
				host_name = "";
			strip_html_brackets(host_name);

			display_type = DISPLAY_HOST_TRENDS;
		}

		/* we found the node width argument */
		else if (!strcmp(variables[x], "service")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if ((service_desc = (char *)strdup(variables[x])) == NULL)
				service_desc = "";
			strip_html_brackets(service_desc);

			display_type = DISPLAY_SERVICE_TRENDS;
		}

		/* we found a combined host/service */
		else if (!strcmp(variables[x], "hostservice")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			temp_buffer = strtok(variables[x], "^");

			if ((host_name = (char *)strdup(temp_buffer)) == NULL)
				host_name = "";
			else
				strip_html_brackets(host_name);

			temp_buffer = strtok(NULL, "");

			if ((service_desc = (char *)strdup(temp_buffer)) == NULL)
				service_desc = "";
			else
				strip_html_brackets(service_desc);

			display_type = DISPLAY_SERVICE_TRENDS;
		}

		/* we found first time argument */
		else if (!strcmp(variables[x], "t1")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			t1 = (time_t)strtoul(variables[x], NULL, 10);
			timeperiod_type = TIMEPERIOD_CUSTOM;
		}

		/* we found first time argument */
		else if (!strcmp(variables[x], "t2")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			t2 = (time_t)strtoul(variables[x], NULL, 10);
			timeperiod_type = TIMEPERIOD_CUSTOM;
		}

		/* we found the image creation option */
		else if (!strcmp(variables[x], "createimage")) {
			content_type = IMAGE_CONTENT;
		}

		/* we found the assume initial states option */
		else if (!strcmp(variables[x], "assumeinitialstates")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "yes"))
				assume_initial_states = TRUE;
			else
				assume_initial_states = FALSE;
		}

		/* we found the initial assumed host state option */
		else if (!strcmp(variables[x], "initialassumedhoststate")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			initial_assumed_host_state = atoi(variables[x]);
		}

		/* we found the initial assumed service state option */
		else if (!strcmp(variables[x], "initialassumedservicestate")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			initial_assumed_service_state = atoi(variables[x]);
		}

		/* we found the assume state during program not running option */
		else if (!strcmp(variables[x], "assumestatesduringnotrunning")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "yes"))
				assume_states_during_notrunning = TRUE;
			else
				assume_states_during_notrunning = FALSE;
		}

		/* we found the assume state retention option */
		else if (!strcmp(variables[x], "assumestateretention")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "yes"))
				assume_state_retention = TRUE;
			else
				assume_state_retention = FALSE;
		}

		/* we found the include soft states option */
		else if (!strcmp(variables[x], "includesoftstates")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "yes"))
				include_soft_states = TRUE;
			else
				include_soft_states = FALSE;
		}

		/* we found the zoom factor argument */
		else if (!strcmp(variables[x], "zoom")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			zoom_factor = atoi(variables[x]);
			if (zoom_factor == 0)
				zoom_factor = 1;
		}

		/* we found the backtrack archives argument */
		else if (!strcmp(variables[x], "backtrack")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			backtrack_archives = atoi(variables[x]);
			if (backtrack_archives < 0)
				backtrack_archives = 0;
			if (backtrack_archives > MAX_ARCHIVE_BACKTRACKS)
				backtrack_archives = MAX_ARCHIVE_BACKTRACKS;
		}

		/* we found the standard timeperiod argument */
		else if (!strcmp(variables[x], "timeperiod")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "today"))
				timeperiod_type = TIMEPERIOD_TODAY;
			else if (!strcmp(variables[x], "yesterday"))
				timeperiod_type = TIMEPERIOD_YESTERDAY;
			else if (!strcmp(variables[x], "thisweek"))
				timeperiod_type = TIMEPERIOD_THISWEEK;
			else if (!strcmp(variables[x], "lastweek"))
				timeperiod_type = TIMEPERIOD_LASTWEEK;
			else if (!strcmp(variables[x], "thismonth"))
				timeperiod_type = TIMEPERIOD_THISMONTH;
			else if (!strcmp(variables[x], "lastmonth"))
				timeperiod_type = TIMEPERIOD_LASTMONTH;
			else if (!strcmp(variables[x], "thisquarter"))
				timeperiod_type = TIMEPERIOD_THISQUARTER;
			else if (!strcmp(variables[x], "lastquarter"))
				timeperiod_type = TIMEPERIOD_LASTQUARTER;
			else if (!strcmp(variables[x], "thisyear"))
				timeperiod_type = TIMEPERIOD_THISYEAR;
			else if (!strcmp(variables[x], "lastyear"))
				timeperiod_type = TIMEPERIOD_LASTYEAR;
			else if (!strcmp(variables[x], "nextproblem"))
				timeperiod_type = TIMEPERIOD_NEXTPROBLEM;
			else if (!strcmp(variables[x], "last24hours"))
				timeperiod_type = TIMEPERIOD_LAST24HOURS;
			else if (!strcmp(variables[x], "last7days"))
				timeperiod_type = TIMEPERIOD_LAST7DAYS;
			else if (!strcmp(variables[x], "last31days"))
				timeperiod_type = TIMEPERIOD_LAST31DAYS;
			else if (!strcmp(variables[x], "custom"))
				timeperiod_type = TIMEPERIOD_CUSTOM;
			else
				continue;

			convert_timeperiod_to_times(timeperiod_type, &t1, &t2);
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "smon")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			start_month = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "sday")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			start_day = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "syear")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			start_year = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "smin")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			start_minute = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "ssec")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			start_second = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "shour")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			start_hour = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}


		/* we found time argument */
		else if (!strcmp(variables[x], "emon")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			end_month = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "eday")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			end_day = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "eyear")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			end_year = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "emin")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			end_minute = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "esec")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			end_second = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found time argument */
		else if (!strcmp(variables[x], "ehour")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (timeperiod_type != TIMEPERIOD_CUSTOM)
				continue;

			end_hour = atoi(variables[x]);
			timeperiod_type = TIMEPERIOD_CUSTOM;
			compute_time_from_parts = TRUE;
		}

		/* we found the embed option */
		else if (!strcmp(variables[x], "embedded"))
			embedded = TRUE;

		/* we found the noheader option */
		else if (!strcmp(variables[x], "noheader"))
			display_header = FALSE;

		/* we found the nopopups option */
		else if (!strcmp(variables[x], "nopopups"))
			display_popups = FALSE;

		/* we found the nomap option */
		else if (!strcmp(variables[x], "nomap")) {
			display_popups = FALSE;
			use_map = FALSE;
		}

		/* we found the input option */
		else if (!strcmp(variables[x], "input")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			if (!strcmp(variables[x], "gethost"))
				input_type = GET_INPUT_HOST_TARGET;
			else if (!strcmp(variables[x], "getservice"))
				input_type = GET_INPUT_SERVICE_TARGET;
			else if (!strcmp(variables[x], "getoptions"))
				input_type = GET_INPUT_OPTIONS;
			else
				input_type = GET_INPUT_TARGET_TYPE;
		}

		/* we found the small image option */
		else if (!strcmp(variables[x], "smallimage"))
			small_image = TRUE;

		/* we found the nodaemoncheck option */
		else if (!strcmp(variables[x], "nodaemoncheck"))
			daemon_check = FALSE;

		else if (!strcmp(variables[x], "ignorerestart"))
			ignore_daemon_restart = TRUE;

	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}



/* top level routine for graphic all trend data */
void graph_all_trend_data(void) {
	archived_state *temp_as;
	archived_state *last_as;
	time_t a;
	time_t b;
	time_t current_time;
	int current_state = AS_NO_DATA;
	int have_some_real_data = FALSE;
	hoststatus *hststatus = NULL;
	servicestatus *svcstatus = NULL;
	unsigned long wobble = 300;
	int first_real_state = AS_NO_DATA;
	time_t initial_assumed_time;
	int initial_assumed_state = AS_SVC_OK;
	int error = FALSE;


	time(&current_time);

	/* if left hand of graph is after current time, we can't do anything at all.... */
	if (t1 > current_time)
		return;

	/* find current state for host or service */
	if (display_type == DISPLAY_HOST_TRENDS)
		hststatus = find_hoststatus(host_name);
	else
		svcstatus = find_servicestatus(host_name, service_desc);


	/************************************/
	/* INSERT CURRENT STATE (IF WE CAN) */
	/************************************/

	/* if current time DOES NOT fall within graph bounds, so we can't do anything as far as assuming current state */
	/* the "wobble" value is necessary because when the CGI is called to do the PNG generation, t2 will actually be less that current_time by a bit */

	/* if we don't have any data, assume current state (if possible) */
	if (as_list == NULL && current_time > t1 && current_time < (t2 + wobble)) {

		/* we don't have any historical information, but the current time falls within the reporting period, so use */
		/* the current status of the host/service as the starting data */
		if (display_type == DISPLAY_HOST_TRENDS) {
			if (hststatus != NULL) {

				if (hststatus->status == HOST_DOWN)
					last_known_state = AS_HOST_DOWN;
				else if (hststatus->status == HOST_UNREACHABLE)
					last_known_state = AS_HOST_UNREACHABLE;
				else
					last_known_state = AS_HOST_UP;

				/* add a dummy archived state item, so something can get graphed */
				add_archived_state(last_known_state, AS_HARD_STATE, t1, "Current Host State Assumed (Faked Log Entry)");

				/* use the current state as the last known real state */
				first_real_state = last_known_state;
			}
		} else {
			if (svcstatus != NULL) {

				if (svcstatus->status == SERVICE_OK)
					last_known_state = AS_SVC_OK;
				else if (svcstatus->status == SERVICE_WARNING)
					last_known_state = AS_SVC_WARNING;
				else if (svcstatus->status == SERVICE_CRITICAL)
					last_known_state = AS_SVC_CRITICAL;
				else if (svcstatus->status == SERVICE_UNKNOWN)
					last_known_state = AS_SVC_UNKNOWN;

				/* add a dummy archived state item, so something can get graphed */
				add_archived_state(last_known_state, AS_HARD_STATE, t1, "Current Service State Assumed (Faked Log Entry)");

				/* use the current state as the last known real state */
				first_real_state = last_known_state;
			}
		}
	}


	/******************************************/
	/* INSERT FIRST ASSUMED STATE (IF WE CAN) */
	/******************************************/

	if ((display_type == DISPLAY_HOST_TRENDS && initial_assumed_host_state != AS_NO_DATA) || (display_type == DISPLAY_SERVICE_TRENDS && initial_assumed_service_state != AS_NO_DATA)) {

		/* see if its okay to assume initial state for this subject */
		error = FALSE;
		if (display_type == DISPLAY_SERVICE_TRENDS) {
			if (initial_assumed_service_state != AS_SVC_OK && initial_assumed_service_state != AS_SVC_WARNING && initial_assumed_service_state != AS_SVC_UNKNOWN && initial_assumed_service_state != AS_SVC_CRITICAL && initial_assumed_service_state != AS_CURRENT_STATE)
				error = TRUE;
			else
				initial_assumed_state = initial_assumed_service_state;
			if (initial_assumed_service_state == AS_CURRENT_STATE && svcstatus == NULL)
				error = TRUE;
		} else {
			if (initial_assumed_host_state != AS_HOST_UP && initial_assumed_host_state != AS_HOST_DOWN && initial_assumed_host_state != AS_HOST_UNREACHABLE && initial_assumed_host_state != AS_CURRENT_STATE)
				error = TRUE;
			else
				initial_assumed_state = initial_assumed_host_state;
			if (initial_assumed_host_state == AS_CURRENT_STATE && hststatus == NULL)
				error = TRUE;
		}

		/* get the current state if applicable */
		if (((display_type == DISPLAY_HOST_TRENDS && initial_assumed_host_state == AS_CURRENT_STATE) || (display_type == DISPLAY_SERVICE_TRENDS && initial_assumed_service_state == AS_CURRENT_STATE)) && error == FALSE) {
			if (display_type == DISPLAY_HOST_TRENDS) {
				switch (hststatus->status) {
				case HOST_DOWN:
					initial_assumed_state = AS_HOST_DOWN;
					break;
				case HOST_UNREACHABLE:
					initial_assumed_state = AS_HOST_UNREACHABLE;
					break;
				case HOST_UP:
					initial_assumed_state = AS_HOST_UP;
					break;
				default:
					error = TRUE;
					break;
				}
			} else {
				switch (svcstatus->status) {
				case SERVICE_OK:
					initial_assumed_state = AS_SVC_OK;
					break;
				case SERVICE_WARNING:
					initial_assumed_state = AS_SVC_WARNING;
					break;
				case SERVICE_UNKNOWN:
					initial_assumed_state = AS_SVC_UNKNOWN;
					break;
				case SERVICE_CRITICAL:
					initial_assumed_state = AS_SVC_CRITICAL;
					break;
				default:
					error = TRUE;
					break;
				}
			}
		}

		if (error == FALSE) {

			/* add this assumed state entry before any entries in the list and <= t1 */
			if (as_list == NULL)
				initial_assumed_time = t1;
			else if (as_list->time_stamp > t1)
				initial_assumed_time = t1;
			else
				initial_assumed_time = as_list->time_stamp - 1;

			if (display_type == DISPLAY_HOST_TRENDS)
				add_archived_state(initial_assumed_state, AS_HARD_STATE, initial_assumed_time, "First Host State Assumed (Faked Log Entry)");
			else
				add_archived_state(initial_assumed_state, AS_HARD_STATE, initial_assumed_time, "First Service State Assumed (Faked Log Entry)");
		}
	}




	/**************************************/
	/* BAIL OUT IF WE DON'T HAVE ANYTHING */
	/**************************************/

	have_some_real_data = FALSE;
	for (temp_as = as_list; temp_as != NULL; temp_as = temp_as->next) {
		if (temp_as->entry_type != AS_NO_DATA && temp_as->entry_type != AS_PROGRAM_START && temp_as->entry_type != AS_PROGRAM_END) {
			have_some_real_data = TRUE;
			break;
		}
	}
	if (have_some_real_data == FALSE)
		return;




	/* if we're creating the HTML, start map code... */
	if (content_type == HTML_CONTENT)
		printf("<MAP name='trendsmap'>\n");

	last_as = NULL;
	earliest_time = t2;
	latest_time = t1;



#ifdef DEBUG
	printf("--- BEGINNING/MIDDLE SECTION ---<BR>\n");
#endif

	/**********************************/
	/*    BEGINNING/MIDDLE SECTION    */
	/**********************************/

	for (temp_as = as_list; temp_as != NULL; temp_as = temp_as->next) {

		/* keep this as last known state if this is the first entry or if it occurs before the starting point of the graph */
		if ((temp_as->time_stamp <= t1 || temp_as == as_list) && (temp_as->entry_type != AS_NO_DATA && temp_as->entry_type != AS_PROGRAM_END && temp_as->entry_type != AS_PROGRAM_START)) {
			last_known_state = temp_as->entry_type;
#ifdef DEBUG
			printf("SETTING LAST KNOWN STATE=%d<br>\n", last_known_state);
#endif
		}

		/* skip this entry if it occurs before the starting point of the graph */
		if (temp_as->time_stamp <= t1) {
#ifdef DEBUG
			printf("SKIPPING PRE-EVENT: %d @ %lu<br>\n", temp_as->entry_type, temp_as->time_stamp);
#endif
			last_as = temp_as;
			continue;
		}

		/* graph this span if we're not on the first item */
		if (last_as != NULL) {

			a = last_as->time_stamp;
			b = temp_as->time_stamp;

			/* we've already passed the last time displayed in the graph */
			if (a > t2)
				break;

			/* only graph this data if its on the graph */
			else if (b > t1) {

				/* clip last time if it exceeds graph limits */
				if (b > t2)
					b = t2;

				/* clip first time if it precedes graph limits */
				if (a < t1)
					a = t1;

				/* save this time if its the earliest we've graphed */
				if (a < earliest_time) {
					earliest_time = a;
					earliest_state = last_as->entry_type;
				}

				/* save this time if its the latest we've graphed */
				if (b > latest_time) {
					latest_time = b;
					latest_state = last_as->entry_type;
				}

				/* compute availability times for this chunk */
				graph_trend_data(last_as->entry_type, temp_as->entry_type, last_as->time_stamp, a, b, last_as->state_info);

				/* return if we've reached the end of the graph limits */
				if (b >= t2) {
					last_as = temp_as;
					break;
				}
			}
		}


		/* keep track of the last item */
		last_as = temp_as;
	}


#ifdef DEBUG
	printf("--- END SECTION ---<BR>\n");
#endif

	/**********************************/
	/*           END SECTION          */
	/**********************************/

	if (last_as != NULL) {

		/* don't process an entry that is beyond the limits of the graph */
		if (last_as->time_stamp < t2) {

			time(&current_time);
			b = current_time;
			if (b > t2)
				b = t2;

			a = last_as->time_stamp;
			if (a < t1)
				a = t1;

			/* fake the current state (it doesn't really matter for graphing) */
			if (display_type == DISPLAY_HOST_TRENDS)
				current_state = AS_HOST_UP;
			else
				current_state = AS_SVC_OK;

			/* compute availability times for last state */
			graph_trend_data(last_as->entry_type, current_state, a, a, b, last_as->state_info);
		}
	}



	/* if we're creating the HTML, close the map code */
	if (content_type == HTML_CONTENT)
		printf("</MAP>\n");

	return;
}



/* graphs trend data */
void graph_trend_data(int first_state, int last_state, time_t real_start_time, time_t start_time, time_t end_time, char *state_info) {
	int start_state;
	int end_state;
	int start_pixel = 0;
	int end_pixel = 0;
	int color_to_use = 0;
	int height = 0;
	double start_pixel_ratio;
	double end_pixel_ratio;
	char temp_buffer[MAX_INPUT_BUFFER];
	char state_string[MAX_INPUT_BUFFER];
	char end_timestring[MAX_INPUT_BUFFER];
	char start_timestring[MAX_INPUT_BUFFER];
	time_t center_time;
	time_t next_start_time;
	time_t next_end_time;
	int days = 0;
	int hours = 0;
	int minutes = 0;
	int seconds = 0;

	/* can't graph if we don't have data... */
	if (first_state == AS_NO_DATA || last_state == AS_NO_DATA)
		return;
	if (first_state == AS_PROGRAM_START && (last_state == AS_PROGRAM_END || last_state == AS_PROGRAM_START)) {
		if (assume_initial_states == FALSE)
			return;
	}
	if (first_state == AS_PROGRAM_END) {
		if (assume_states_during_notrunning == TRUE)
			first_state = last_known_state;
		else
			return;
	}

	/* special case if first entry was program start */
	if (first_state == AS_PROGRAM_START) {
#ifdef DEBUG
		printf("First state=program start!\n");
#endif
		if (assume_initial_states == TRUE) {
#ifdef DEBUG
			printf("\tWe are assuming initial states...\n");
#endif
			if (assume_state_retention == TRUE) {
				start_state = last_known_state;
#ifdef DEBUG
				printf("\tWe are assuming state retention (%d)...\n", start_state);
#endif
			} else {
#ifdef DEBUG
				printf("\tWe are NOT assuming state retention...\n");
#endif
				if (display_type == DISPLAY_HOST_TRENDS)
					start_state = AS_HOST_UP;
				else
					start_state = AS_SVC_OK;
			}
		} else {
#ifdef DEBUG
			printf("We ARE NOT assuming initial states!\n");
#endif
			return;
		}
	} else {
		start_state = first_state;
		last_known_state = first_state;
	}

	/* special case if last entry was program stop */
	if (last_state == AS_PROGRAM_END)
		end_state = first_state;
	else
		end_state = last_state;

#ifdef DEBUG
	printf("Graphing state %d\n", start_state);
	printf("\tfrom %s", ctime(&start_time));
	printf("\tto %s", ctime(&end_time));
#endif

	if (start_time < t1)
		start_time = t1;
	if (end_time > t2)
		end_time = t2;
	if (end_time < t1 || start_time > t2)
		return;

	/* calculate the first and last pixels to use */
	if (start_time == t1)
		start_pixel = 0;
	else {
		start_pixel_ratio = ((double)(start_time - t1)) / ((double)(t2 - t1));
		start_pixel = (int)(start_pixel_ratio * (drawing_width - 1));
	}
	if (end_time == t1)
		end_pixel = 0;
	else {
		end_pixel_ratio = ((double)(end_time - t1)) / ((double)(t2 - t1));
		end_pixel = (int)(end_pixel_ratio * (drawing_width - 1));
	}

#ifdef DEBUG
	printf("\tPixel %d to %d\n\n", start_pixel, end_pixel);
#endif


	/* we're creating the image, so draw... */
	if (content_type == IMAGE_CONTENT) {

		/* figure out the color to use for drawing */
		switch (start_state) {
		case AS_HOST_UP:
			color_to_use = color_green;
			height = 60;
			break;
		case AS_HOST_DOWN:
			color_to_use = color_red;
			height = 40;
			break;
		case AS_HOST_UNREACHABLE:
			color_to_use = color_pink;
			height = 20;
			break;
		case AS_SVC_OK:
			color_to_use = color_green;
			height = 80;
			break;
		case AS_SVC_WARNING:
			color_to_use = color_yellow;
			height = 60;
			break;
		case AS_SVC_UNKNOWN:
			color_to_use = color_pink;
			height = 40;
			break;
		case AS_SVC_CRITICAL:
			color_to_use = color_red;
			height = 20;
			break;
		default:
			color_to_use = color_black;
			height = 0;
			break;
		}

		/* draw a rectangle */
		if (start_state != AS_NO_DATA)
			gdImageFilledRectangle(trends_image, start_pixel + drawing_x_offset, drawing_height - height + drawing_y_offset, end_pixel + drawing_x_offset, drawing_height + drawing_y_offset, color_to_use);
	}

	/* else we're creating the HTML, so write map area code... */
	else {


		/* figure out the the state string to use */
		switch (start_state) {
		case AS_HOST_UP:
			strcpy(state_string, "UP");
			height = 60;
			break;
		case AS_HOST_DOWN:
			strcpy(state_string, "DOWN");
			height = 40;
			break;
		case AS_HOST_UNREACHABLE:
			strcpy(state_string, "UNREACHABLE");
			height = 20;
			break;
		case AS_SVC_OK:
			strcpy(state_string, "OK");
			height = 80;
			break;
		case AS_SVC_WARNING:
			strcpy(state_string, "WARNING");
			height = 60;
			break;
		case AS_SVC_UNKNOWN:
			strcpy(state_string, "UNKNOWN");
			height = 40;
			break;
		case AS_SVC_CRITICAL:
			strcpy(state_string, "CRITICAL");
			height = 20;
			break;
		default:
			strcpy(state_string, "?");
			height = 5;
			break;
		}

		/* get the center of this time range */
		center_time = start_time + ((end_time - start_time) / 2);

		/* determine next start and end time range with zoom factor */
		if (zoom_factor > 0) {
			next_start_time = center_time - (((t2 - t1) / 2) / zoom_factor);
			next_end_time = center_time + (((t2 - t1) / 2) / zoom_factor);
		} else {
			next_start_time = center_time + (((t2 - t1) / 2) * zoom_factor);
			next_end_time = center_time - (((t2 - t1) / 2) * zoom_factor);
		}

		printf("<AREA shape='rect' ");

		printf("coords='%d,%d,%d,%d' ", drawing_x_offset + start_pixel, drawing_y_offset + (drawing_height - height), drawing_x_offset + end_pixel, drawing_y_offset + drawing_height);

		printf("href='%s?t1=%lu&t2=%lu&host=%s", TRENDS_CGI, (unsigned long)next_start_time, (unsigned long)next_end_time, url_encode(host_name));
		if (display_type == DISPLAY_SERVICE_TRENDS)
			printf("&service=%s", url_encode(service_desc));
		printf("&assumeinitialstates=%s", (assume_initial_states == TRUE) ? "yes" : "no");
		printf("&initialassumedhoststate=%d", initial_assumed_host_state);
		printf("&initialassumedservicestate=%d", initial_assumed_service_state);
		printf("&assumestateretention=%s", (assume_state_retention == TRUE) ? "yes" : "no");
		printf("&assumestatesduringnotrunning=%s", (assume_states_during_notrunning == TRUE) ? "yes" : "no");
		printf("&includesoftstates=%s", (include_soft_states == TRUE) ? "yes" : "no");
		if (backtrack_archives > 0)
			printf("&backtrack=%d", backtrack_archives);
		printf("&zoom=%d", zoom_factor);

		printf("' ");

		/* display popup text */
		if (display_popups == TRUE) {

			snprintf(start_timestring, sizeof(start_timestring) - 1, "%s", ctime(&real_start_time));
			start_timestring[sizeof(start_timestring)-1] = '\x0';
			start_timestring[strlen(start_timestring)-1] = '\x0';

			snprintf(end_timestring, sizeof(end_timestring) - 1, "%s", ctime(&end_time));
			end_timestring[sizeof(end_timestring)-1] = '\x0';
			end_timestring[strlen(end_timestring)-1] = '\x0';

			/* calculate total time in this state */
			get_time_breakdown((time_t)(end_time - start_time), &days, &hours, &minutes, &seconds);

			/* sanitize plugin output */
			sanitize_plugin_output(state_info);

			printf("onMouseOver='showPopup(\"");
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "<B><U>%s</U></B><BR><B>Time Range</B>: <I>%s</I> to <I>%s</I><BR><B>Duration</B>: <I>%dd %dh %dm %ds</I><BR><B>State Info</B>: <I>%s</I>", state_string, start_timestring, end_timestring, days, hours, minutes, seconds, (state_info == NULL) ? "N/A" : state_info);
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			printf("%s", temp_buffer);
			printf("\",event)' onMouseOut='hidePopup()'");
		}

		printf(">\n");

	}


	/* calculate time in this state */
	switch (start_state) {
	case AS_HOST_UP:
		time_up += (unsigned long)(end_time - start_time);
		break;
	case AS_HOST_DOWN:
		time_down += (unsigned long)(end_time - start_time);
		break;
	case AS_HOST_UNREACHABLE:
		time_unreachable += (unsigned long)(end_time - start_time);
		break;
	case AS_SVC_OK:
		time_ok += (unsigned long)(end_time - start_time);
		break;
	case AS_SVC_WARNING:
		time_warning += (unsigned long)(end_time - start_time);
		break;
	case AS_SVC_UNKNOWN:
		time_unknown += (unsigned long)(end_time - start_time);
		break;
	case AS_SVC_CRITICAL:
		time_critical += (unsigned long)(end_time - start_time);
		break;
	default:
		break;
	}

	return;
}



/* convert current host state to archived state value */
int convert_host_state_to_archived_state(int current_status) {

	if (current_status == HOST_UP)
		return AS_HOST_UP;
	if (current_status == HOST_DOWN)
		return AS_HOST_DOWN;
	if (current_status == HOST_UNREACHABLE)
		return AS_HOST_UNREACHABLE;

	return AS_NO_DATA;
}


/* convert current service state to archived state value */
int convert_service_state_to_archived_state(int current_status) {

	if (current_status == SERVICE_OK)
		return AS_SVC_OK;
	if (current_status == SERVICE_UNKNOWN)
		return AS_SVC_UNKNOWN;
	if (current_status == SERVICE_WARNING)
		return AS_SVC_WARNING;
	if (current_status == SERVICE_CRITICAL)
		return AS_SVC_CRITICAL;

	return AS_NO_DATA;
}



/* adds an archived state entry */
void add_archived_state(int entry_type, int state_type, time_t time_stamp, char *state_info) {
	archived_state *last_as = NULL;
	archived_state *temp_as = NULL;
	archived_state *new_as = NULL;

#ifdef DEBUG
	printf("Added state %d @ %s", state_type, ctime(&time_stamp));
#endif

	/* allocate memory for the new entry */
	new_as = (archived_state *)malloc(sizeof(archived_state));
	if (new_as == NULL)
		return;

	/* allocate memory fo the state info */
	if (state_info != NULL) {
		new_as->state_info = (char *)malloc(strlen(state_info) + 1);
		if (new_as->state_info != NULL)
			strcpy(new_as->state_info, state_info);
	} else new_as->state_info = NULL;

	new_as->entry_type = entry_type;
	new_as->processed_state = entry_type;
	new_as->state_type = state_type;
	new_as->time_stamp = time_stamp;

	/* add the new entry to the list in memory, sorted by time */
	last_as = as_list;
	for (temp_as = as_list; temp_as != NULL; temp_as = temp_as->next) {
		if (new_as->time_stamp < temp_as->time_stamp) {
			new_as->next = temp_as;
			if (temp_as == as_list)
				as_list = new_as;
			else
				last_as->next = new_as;
			break;
		} else
			last_as = temp_as;
	}
	if (as_list == NULL) {
		new_as->next = NULL;
		as_list = new_as;
	} else if (temp_as == NULL) {
		new_as->next = NULL;
		last_as->next = new_as;
	}

	return;
}


/* frees memory allocated to the archived state list */
void free_archived_state_list(void) {
	archived_state *this_as = NULL;
	archived_state *next_as = NULL;

	for (this_as = as_list; this_as != NULL;) {
		next_as = this_as->next;
		if (this_as->state_info != NULL)
			free(this_as->state_info);
		free(this_as);
		this_as = next_as;
	}

	as_list = NULL;

	return;
}


/* reads log files for archived state data */
void read_archived_state_data(void) {
	char entry_host_name[MAX_INPUT_BUFFER];
	char entry_service_desc[MAX_INPUT_BUFFER];
	char *plugin_output = NULL;
	char *temp_buffer = NULL;
	char *error_text = NULL;
	logentry *temp_entry = NULL;
	logentry *entry_list = NULL;
	logfilter *filter_list = NULL;
	int state_type = 0;
	int status = READLOG_OK;

	/* print something so browser doesn't time out */
	if (content_type == HTML_CONTENT) {
		printf(" ");
		fflush(NULL);
	}

	/* Service filter */
	add_log_filter(&filter_list, LOGENTRY_SERVICE_OK, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_SERVICE_WARNING, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_SERVICE_CRITICAL, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_SERVICE_UNKNOWN, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_SERVICE_RECOVERY, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_SERVICE_INITIAL_STATE, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_SERVICE_CURRENT_STATE, LOGFILTER_INCLUDE);

	/* Host filter */
	add_log_filter(&filter_list, LOGENTRY_HOST_UP, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_HOST_DOWN, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_HOST_UNREACHABLE, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_HOST_RECOVERY, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_HOST_INITIAL_STATE, LOGFILTER_INCLUDE);
	add_log_filter(&filter_list, LOGENTRY_HOST_CURRENT_STATE, LOGFILTER_INCLUDE);

	if (ignore_daemon_restart == FALSE) {
		add_log_filter(&filter_list, LOGENTRY_STARTUP, LOGFILTER_INCLUDE);
		add_log_filter(&filter_list, LOGENTRY_RESTART, LOGFILTER_INCLUDE);
		add_log_filter(&filter_list, LOGENTRY_SHUTDOWN, LOGFILTER_INCLUDE);
		add_log_filter(&filter_list, LOGENTRY_BAILOUT, LOGFILTER_INCLUDE);
	}

	status = get_log_entries(&entry_list, &filter_list, &error_text, NULL, FALSE, t1 - (60 * 60 * 24 * backtrack_archives), t2);

	free_log_filters(&filter_list);

	if (status != READLOG_ERROR_FATAL) {

		for (temp_entry = entry_list; temp_entry != NULL; temp_entry = temp_entry->next) {

			if (ignore_daemon_restart == FALSE) {
				/* program starts/restarts */
				if (temp_entry->type == LOGENTRY_STARTUP)
					add_archived_state(AS_PROGRAM_START, AS_NO_DATA, temp_entry->timestamp, "Program start");
				if (temp_entry->type == LOGENTRY_RESTART)
					add_archived_state(AS_PROGRAM_START, AS_NO_DATA, temp_entry->timestamp, "Program restart");

				/* program stops */
				if (temp_entry->type == LOGENTRY_SHUTDOWN)
					add_archived_state(AS_PROGRAM_END, AS_NO_DATA, temp_entry->timestamp, "Normal program termination");
				if (temp_entry->type == LOGENTRY_BAILOUT)
					add_archived_state(AS_PROGRAM_END, AS_NO_DATA, temp_entry->timestamp, "Abnormal program termination");
			}

			if (display_type == DISPLAY_HOST_TRENDS) {

				switch (temp_entry->type) {

					/* normal host alerts and initial/current states */
				case LOGENTRY_HOST_DOWN:
				case LOGENTRY_HOST_UNREACHABLE:
				case LOGENTRY_HOST_RECOVERY:
				case LOGENTRY_HOST_UP:
				case LOGENTRY_HOST_INITIAL_STATE:
				case LOGENTRY_HOST_CURRENT_STATE:

					/* get host name */
					temp_buffer = my_strtok(temp_entry->entry_text, ":");
					temp_buffer = my_strtok(NULL, ";");
					strncpy(entry_host_name, (temp_buffer == NULL) ? "" : temp_buffer + 1, sizeof(entry_host_name));
					entry_host_name[sizeof(entry_host_name)-1] = '\x0';

					if (strcmp(host_name, entry_host_name))
						break;

					/* state types */
					if (strstr(temp_entry->entry_text, ";SOFT;")) {
						if (include_soft_states == FALSE)
							break;
						state_type = AS_SOFT_STATE;
					}
					if (strstr(temp_entry->entry_text, ";HARD;"))
						state_type = AS_HARD_STATE;

					/* get the plugin output */
					temp_buffer = my_strtok(NULL, ";");
					temp_buffer = my_strtok(NULL, ";");
					temp_buffer = my_strtok(NULL, ";");
					plugin_output = my_strtok(NULL, "\n");

					if (strstr(temp_entry->entry_text, ";DOWN;"))
						add_archived_state(AS_HOST_DOWN, state_type, temp_entry->timestamp, plugin_output);
					else if (strstr(temp_entry->entry_text, ";UNREACHABLE;"))
						add_archived_state(AS_HOST_UNREACHABLE, state_type, temp_entry->timestamp, plugin_output);
					else if (strstr(temp_entry->entry_text, ";RECOVERY;") || strstr(temp_entry->entry_text, ";UP;"))
						add_archived_state(AS_HOST_UP, state_type, temp_entry->timestamp, plugin_output);
					else
						add_archived_state(AS_NO_DATA, AS_NO_DATA, temp_entry->timestamp, plugin_output);

					break;
				}
			}

			if (display_type == DISPLAY_SERVICE_TRENDS) {

				switch (temp_entry->type) {

					/* normal service alerts and initial/current states */
				case LOGENTRY_SERVICE_CRITICAL:
				case LOGENTRY_SERVICE_WARNING:
				case LOGENTRY_SERVICE_UNKNOWN:
				case LOGENTRY_SERVICE_RECOVERY:
				case LOGENTRY_SERVICE_OK:
				case LOGENTRY_SERVICE_INITIAL_STATE:
				case LOGENTRY_SERVICE_CURRENT_STATE:

					/* get host name */
					temp_buffer = my_strtok(temp_entry->entry_text, ":");
					temp_buffer = my_strtok(NULL, ";");
					strncpy(entry_host_name, (temp_buffer == NULL) ? "" : temp_buffer + 1, sizeof(entry_host_name));
					entry_host_name[sizeof(entry_host_name)-1] = '\x0';

					if (strcmp(host_name, entry_host_name))
						break;

					/* get service description */
					temp_buffer = my_strtok(NULL, ";");
					strncpy(entry_service_desc, (temp_buffer == NULL) ? "" : temp_buffer, sizeof(entry_service_desc));
					entry_service_desc[sizeof(entry_service_desc)-1] = '\x0';

					if (strcmp(service_desc, entry_service_desc))
						break;

					/* state types */
					if (strstr(temp_entry->entry_text, ";SOFT;")) {
						if (include_soft_states == FALSE)
							break;
						state_type = AS_SOFT_STATE;
					}
					if (strstr(temp_entry->entry_text, ";HARD;"))
						state_type = AS_HARD_STATE;

					/* get the plugin output */
					temp_buffer = my_strtok(NULL, ";");
					temp_buffer = my_strtok(NULL, ";");
					temp_buffer = my_strtok(NULL, ";");
					plugin_output = my_strtok(NULL, "\n");

					if (strstr(temp_entry->entry_text, ";CRITICAL;"))
						add_archived_state(AS_SVC_CRITICAL, state_type, temp_entry->timestamp, plugin_output);
					else if (strstr(temp_entry->entry_text, ";WARNING;"))
						add_archived_state(AS_SVC_WARNING, state_type, temp_entry->timestamp, plugin_output);
					else if (strstr(temp_entry->entry_text, ";UNKNOWN;"))
						add_archived_state(AS_SVC_UNKNOWN, state_type, temp_entry->timestamp, plugin_output);
					else if (strstr(temp_entry->entry_text, ";RECOVERY;") || strstr(temp_entry->entry_text, ";OK;"))
						add_archived_state(AS_SVC_OK, state_type, temp_entry->timestamp, plugin_output);
					else
						add_archived_state(AS_NO_DATA, AS_NO_DATA, temp_entry->timestamp, plugin_output);

					break;
				}
			}
		}
		free_log_entries(&entry_list);
	}
	return;
}


/* write timestamps */
void draw_timestamps(void) {
	int last_timestamp = 0;
	archived_state *temp_as;
	double start_pixel_ratio;
	int start_pixel;

	if (content_type != IMAGE_CONTENT)
		return;

	/* draw first timestamp */
	draw_timestamp(0, t1);
	last_timestamp = 0;

	for (temp_as = as_list; temp_as != NULL; temp_as = temp_as->next) {

		if (temp_as->time_stamp < t1 || temp_as->time_stamp > t2)
			continue;

		start_pixel_ratio = ((double)(temp_as->time_stamp - t1)) / ((double)(t2 - t1));
		start_pixel = (int)(start_pixel_ratio * (drawing_width - 1));

		/* draw start timestamp if possible */
		if ((start_pixel > last_timestamp + MIN_TIMESTAMP_SPACING) && (start_pixel < drawing_width - 1 - MIN_TIMESTAMP_SPACING)) {
			draw_timestamp(start_pixel, temp_as->time_stamp);
			last_timestamp = start_pixel;
		}
	}

	/* draw last timestamp */
	draw_timestamp(drawing_width - 1, t2);

	return;
}


/* write timestamp below graph */
void draw_timestamp(int ts_pixel, time_t ts_time) {
	char temp_buffer[MAX_INPUT_BUFFER];
	int string_height;
	int string_width;

	snprintf(temp_buffer, sizeof(temp_buffer) - 1, "%s", ctime(&ts_time));
	temp_buffer[sizeof(temp_buffer)-1] = '\x0';
	temp_buffer[strlen(temp_buffer)-1] = '\x0';

	string_height = gdFontSmall->h;
	string_width = gdFontSmall->w * strlen(temp_buffer);

	if (small_image == FALSE)
		gdImageStringUp(trends_image, gdFontSmall, ts_pixel + drawing_x_offset - (string_height / 2), drawing_y_offset + drawing_height + string_width + 5, (unsigned char *)temp_buffer, color_black);

	/* draw a dashed vertical line at this point */
	if (ts_pixel > 0 && ts_pixel < (drawing_width - 1))
		draw_dashed_line(ts_pixel + drawing_x_offset, drawing_y_offset, ts_pixel + drawing_x_offset, drawing_y_offset + drawing_height, color_black);

	return;
}



/* draw total state times */
void draw_time_breakdowns(void) {
	char temp_buffer[MAX_INPUT_BUFFER];
	unsigned long total_time = 0L;
	unsigned long total_state_time;
	unsigned long time_indeterminate = 0L;
	int string_height;

	if (content_type == HTML_CONTENT)
		return;

	if (small_image == TRUE)
		return;

	total_time = (unsigned long)(t2 - t1);

	if (display_type == DISPLAY_HOST_TRENDS)
		total_state_time = time_up + time_down + time_unreachable;
	else
		total_state_time = time_ok + time_warning + time_unknown + time_critical;

	if (total_state_time >= total_time)
		time_indeterminate = 0L;
	else
		time_indeterminate = total_time - total_state_time;

	string_height = gdFontSmall->h;

	if (display_type == DISPLAY_HOST_TRENDS) {

		get_time_breakdown_string(total_time, time_up, "Up", &temp_buffer[0], sizeof(temp_buffer));
		gdImageString(trends_image, gdFontSmall, drawing_x_offset + drawing_width + 20, drawing_y_offset + 5, (unsigned char *)temp_buffer, color_darkgreen);
		gdImageString(trends_image, gdFontSmall, drawing_x_offset - 10 - (gdFontSmall->w * 2), drawing_y_offset + 5, (unsigned char *)"Up", color_darkgreen);

		get_time_breakdown_string(total_time, time_down, "Down", &temp_buffer[0], sizeof(temp_buffer));
		gdImageString(trends_image, gdFontSmall, drawing_x_offset + drawing_width + 20, drawing_y_offset + 25, (unsigned char *)temp_buffer, color_red);
		gdImageString(trends_image, gdFontSmall, drawing_x_offset - 10 - (gdFontSmall->w * 4), drawing_y_offset + 25, (unsigned char *)"Down", color_red);

		get_time_breakdown_string(total_time, time_unreachable, "Unreachable", &temp_buffer[0], sizeof(temp_buffer));
		gdImageString(trends_image, gdFontSmall, drawing_x_offset + drawing_width + 20, drawing_y_offset + 45, (unsigned char *)temp_buffer, color_darkpink);
		gdImageString(trends_image, gdFontSmall, drawing_x_offset - 10 - (gdFontSmall->w * 11), drawing_y_offset + 45, (unsigned char *)"Unreachable", color_darkpink);

		get_time_breakdown_string(total_time, time_indeterminate, "Indeterminate", &temp_buffer[0], sizeof(temp_buffer));
		gdImageString(trends_image, gdFontSmall, drawing_x_offset + drawing_width + 20, drawing_y_offset + 65, (unsigned char *)temp_buffer, color_black);
		gdImageString(trends_image, gdFontSmall, drawing_x_offset - 10 - (gdFontSmall->w * 13), drawing_y_offset + 65, (unsigned char *)"Indeterminate", color_black);
	} else {
		get_time_breakdown_string(total_time, time_ok, "Ok", &temp_buffer[0], sizeof(temp_buffer));
		gdImageString(trends_image, gdFontSmall, drawing_x_offset + drawing_width + 20, drawing_y_offset + 5, (unsigned char *)temp_buffer, color_darkgreen);
		gdImageString(trends_image, gdFontSmall, drawing_x_offset - 10 - (gdFontSmall->w * 2), drawing_y_offset + 5, (unsigned char *)"Ok", color_darkgreen);

		get_time_breakdown_string(total_time, time_warning, "Warning", &temp_buffer[0], sizeof(temp_buffer));
		gdImageString(trends_image, gdFontSmall, drawing_x_offset + drawing_width + 20, drawing_y_offset + 25, (unsigned char *)temp_buffer, color_yellow);
		gdImageString(trends_image, gdFontSmall, drawing_x_offset - 10 - (gdFontSmall->w * 7), drawing_y_offset + 25, (unsigned char *)"Warning", color_yellow);

		get_time_breakdown_string(total_time, time_unknown, "Unknown", &temp_buffer[0], sizeof(temp_buffer));
		gdImageString(trends_image, gdFontSmall, drawing_x_offset + drawing_width + 20, drawing_y_offset + 45, (unsigned char *)temp_buffer, color_darkpink);
		gdImageString(trends_image, gdFontSmall, drawing_x_offset - 10 - (gdFontSmall->w * 7), drawing_y_offset + 45, (unsigned char *)"Unknown", color_darkpink);

		get_time_breakdown_string(total_time, time_critical, "Critical", &temp_buffer[0], sizeof(temp_buffer));
		gdImageString(trends_image, gdFontSmall, drawing_x_offset + drawing_width + 20, drawing_y_offset + 65, (unsigned char *)temp_buffer, color_red);
		gdImageString(trends_image, gdFontSmall, drawing_x_offset - 10 - (gdFontSmall->w * 8), drawing_y_offset + 65, (unsigned char *)"Critical", color_red);

		get_time_breakdown_string(total_time, time_indeterminate, "Indeterminate", &temp_buffer[0], sizeof(temp_buffer));
		gdImageString(trends_image, gdFontSmall, drawing_x_offset + drawing_width + 20, drawing_y_offset + 85, (unsigned char *)temp_buffer, color_black);
		gdImageString(trends_image, gdFontSmall, drawing_x_offset - 10 - (gdFontSmall->w * 13), drawing_y_offset + 85, (unsigned char *)"Indeterminate", color_black);
	}

	return;
}


void get_time_breakdown_string(unsigned long total_time, unsigned long state_time, char *state_string, char *buffer, int buffer_length) {
	int days;
	int hours;
	int minutes;
	int seconds;
	double percent_time;

	get_time_breakdown(state_time, &days, &hours, &minutes, &seconds);
	if (total_time == 0L)
		percent_time = 0.0;
	else
		percent_time = ((double)state_time / total_time) * 100.0;
	snprintf(buffer, buffer_length - 1, "%-13s: (%.3f%%) %dd %dh %dm %ds", state_string, percent_time, days, hours, minutes, seconds);
	buffer[buffer_length-1] = '\x0';

	return;
}


void compute_report_times(void) {
	time_t current_time;
	struct tm *st;
	struct tm *et;

	/* get the current time */
	time(&current_time);

	st = localtime(&current_time);

	st->tm_sec = start_second;
	st->tm_min = start_minute;
	st->tm_hour = start_hour;
	st->tm_mday = start_day;
	st->tm_mon = start_month - 1;
	st->tm_year = start_year - 1900;
	st->tm_isdst = -1;

	t1 = mktime(st);

	et = localtime(&current_time);

	et->tm_sec = end_second;
	et->tm_min = end_minute;
	et->tm_hour = end_hour;
	et->tm_mday = end_day;
	et->tm_mon = end_month - 1;
	et->tm_year = end_year - 1900;
	et->tm_isdst = -1;

	t2 = mktime(et);
}



/* draws a dashed line */
void draw_dashed_line(int x1, int y1, int x2, int y2, int color) {
	int styleDashed[12];

	styleDashed[0] = color;
	styleDashed[1] = color;
	styleDashed[2] = gdTransparent;
	styleDashed[3] = gdTransparent;
	styleDashed[4] = color;
	styleDashed[5] = color;
	styleDashed[6] = gdTransparent;
	styleDashed[7] = gdTransparent;
	styleDashed[8] = color;
	styleDashed[9] = color;
	styleDashed[10] = gdTransparent;
	styleDashed[11] = gdTransparent;

	/* sets current style to a dashed line */
	gdImageSetStyle(trends_image, styleDashed, 12);

	/* draws a line (dashed) */
	gdImageLine(trends_image, x1, y1, x2, y2, gdStyled);

	return;
}


/* draws horizontal grid lines */
void draw_horizontal_grid_lines(void) {

	if (content_type == HTML_CONTENT)
		return;

	if (small_image == TRUE)
		return;

	draw_dashed_line(drawing_x_offset, drawing_y_offset + 10, drawing_x_offset + drawing_width, drawing_y_offset + 10, color_black);
	draw_dashed_line(drawing_x_offset, drawing_y_offset + 30, drawing_x_offset + drawing_width, drawing_y_offset + 30, color_black);
	draw_dashed_line(drawing_x_offset, drawing_y_offset + 50, drawing_x_offset + drawing_width, drawing_y_offset + 50, color_black);
	if (display_type == DISPLAY_SERVICE_TRENDS)
		draw_dashed_line(drawing_x_offset, drawing_y_offset + 70, drawing_x_offset + drawing_width, drawing_y_offset + 70, color_black);

	return;
}
