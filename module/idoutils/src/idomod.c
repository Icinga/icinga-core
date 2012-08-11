/*****************************************************************************
 *
 * IDOMOD.C - Icinga Data Output Event Broker Module
 *
 * Copyright (c) 2005-2007 Ethan Galstad
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 *****************************************************************************/

/* include our project's header files */
#include "../../../include/config.h"
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/idomod.h"

/* include (minimum required) event broker header files */
#include "../../../include/nebstructs.h"
#include "../../../include/nebmodules.h"
#include "../../../include/nebcallbacks.h"
#include "../../../include/broker.h"

/* include other Icinga header files for access to functions, data structs, etc. */
#include "../../../include/common.h"
#include "../../../include/icinga.h"
#include "../../../include/downtime.h"
#include "../../../include/comments.h"
#include "../../../include/macros.h"

/* specify event broker API version (required) */
NEB_API_VERSION(CURRENT_NEB_API_VERSION)



void *idomod_module_handle = NULL;
char *idomod_instance_name = NULL;
char *idomod_buffer_file = NULL;
char *idomod_sink_name = NULL;
int idomod_sink_type = IDO_SINK_UNIXSOCKET;
int idomod_sink_tcp_port = IDO_DEFAULT_TCP_PORT;
int idomod_sink_is_open = IDO_FALSE;
int idomod_sink_previously_open = IDO_FALSE;
int idomod_sink_fd = -1;
time_t idomod_sink_last_reconnect_attempt = 0L;
time_t idomod_sink_last_reconnect_warning = 0L;
unsigned long idomod_sink_connect_attempt = 0L;
unsigned long idomod_sink_reconnect_interval = 15;
unsigned long idomod_sink_reconnect_warning_interval = 900;
unsigned long idomod_sink_rotation_interval = 3600;
char *idomod_sink_rotation_command = NULL;
int idomod_sink_rotation_timeout = 60;
int idomod_allow_sink_activity = IDO_TRUE;
unsigned long idomod_process_options = IDOMOD_PROCESS_EVERYTHING;
int idomod_config_output_options = IDOMOD_CONFIG_DUMP_ALL;
unsigned long idomod_sink_buffer_slots = 5000;
idomod_sink_buffer sinkbuf;

char *idomod_debug_file = NULL;
int idomod_debug_level = IDOMOD_DEBUGL_NONE;
int idomod_debug_verbosity = IDOMOD_DEBUGV_BASIC;
FILE *idomod_debug_file_fp = NULL;
unsigned long idomod_max_debug_file_size = 0L;

int idomod_open_debug_log(void);
int idomod_close_debug_log(void);

extern int errno;

/**** Icinga VARIABLES ****/
extern command *command_list;
extern timeperiod *timeperiod_list;
extern contact *contact_list;
extern contactgroup *contactgroup_list;
extern host *host_list;
extern hostgroup *hostgroup_list;
extern service *service_list;
extern servicegroup *servicegroup_list;
extern hostescalation *hostescalation_list;
extern serviceescalation *serviceescalation_list;
extern hostdependency *hostdependency_list;
extern servicedependency *servicedependency_list;

extern char *config_file;
extern sched_info scheduling_info;
extern char *global_host_event_handler;
extern char *global_service_event_handler;

extern int __icinga_object_structure_version;

extern int use_ssl;

#define DEBUG_IDO 1



/* this function gets called when the module is loaded by the event broker */
int nebmodule_init(int flags, char *args, void *handle) {
	char temp_buffer[IDOMOD_MAX_BUFLEN];

	/* save our handle */
	idomod_module_handle = handle;

	/* set some info for the core to be checked */
	neb_set_module_info(idomod_module_handle, NEBMODULE_MODINFO_TITLE, IDOMOD_NAME);
	neb_set_module_info(idomod_module_handle, NEBMODULE_MODINFO_AUTHOR, "Ethan Galstad, Icinga Development Team");
	neb_set_module_info(idomod_module_handle, NEBMODULE_MODINFO_VERSION, IDO_VERSION);
	neb_set_module_info(idomod_module_handle, NEBMODULE_MODINFO_LICENSE, "GPL v2");
	neb_set_module_info(idomod_module_handle, NEBMODULE_MODINFO_DESC, "Icinga Data Out Module, sends data to socket for ido2db");

	/* log module info to the Icinga log file */
	snprintf(temp_buffer, sizeof(temp_buffer) - 1, "idomod: %s %s (%s) %s", IDOMOD_NAME, IDO_VERSION, IDO_DATE, IDO_COPYRIGHT);
	temp_buffer[sizeof(temp_buffer)-1] = '\x0';
	idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);

	/* check Icinga object structure version */
	if (idomod_check_icinga_object_version() == IDO_ERROR)
		return -1;

	/* process arguments */
	if (idomod_process_module_args(args) == IDO_ERROR) {
		idomod_write_to_logs("idomod: An error occurred while attempting to process module arguments.", NSLOG_INFO_MESSAGE);
		return -1;
	}

	if (idomod_sink_type == IDO_SINK_UNIXSOCKET && use_ssl == IDO_TRUE) {
		idomod_write_to_logs("idomod: use_ssl=1 while using socket_type=unix is not allowed. Aborting...", NSLOG_INFO_MESSAGE);
		return -1;
	}

	/* do some initialization stuff... */
	if (idomod_init() == IDO_ERROR) {
		idomod_write_to_logs("idomod: An error occurred while attempting to initialize.", NSLOG_INFO_MESSAGE);
		return -1;
	}

	return 0;
}


/* this function gets called when the module is unloaded by the event broker */
int nebmodule_deinit(int flags, int reason) {
	char temp_buffer[IDOMOD_MAX_BUFLEN];

	/* do some shutdown stuff... */
	idomod_deinit();

	/* log a message to the Icinga log file */
	snprintf(temp_buffer, sizeof(temp_buffer) - 1, "idomod: Shutdown complete.\n");
	temp_buffer[sizeof(temp_buffer)-1] = '\x0';
	idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);

	return 0;
}



/****************************************************************************/
/* INIT/DEINIT FUNCTIONS                                                    */
/****************************************************************************/

/* checks to make sure Icinga object version matches what we know about */
int idomod_check_icinga_object_version(void) {
	char temp_buffer[IDOMOD_MAX_BUFLEN];

	if (__icinga_object_structure_version != CURRENT_OBJECT_STRUCTURE_VERSION) {

		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "idomod: I've been compiled with support for revision %d of the internal Icinga object structures, but the Icinga daemon is currently using revision %d.  I'm going to unload so I don't cause any problems...\n", CURRENT_OBJECT_STRUCTURE_VERSION, __icinga_object_structure_version);
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);

		return IDO_ERROR;
	}

	return IDO_OK;
}


/* performs some initialization stuff */
int idomod_init(void) {
	char temp_buffer[IDOMOD_MAX_BUFLEN];
	time_t current_time;

	/* open debug log */
	idomod_open_debug_log();

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_init() start\n");

	/* initialize some vars (needed for restarts of daemon - why, if the module gets reloaded ???) */
	idomod_sink_is_open = IDO_FALSE;
	idomod_sink_previously_open = IDO_FALSE;
	idomod_sink_fd = -1;
	idomod_sink_last_reconnect_attempt = 0L;
	idomod_sink_last_reconnect_warning = 0L;
	idomod_allow_sink_activity = IDO_TRUE;

	/* initialize data sink buffer */
	idomod_sink_buffer_init(&sinkbuf, idomod_sink_buffer_slots);

	/* read unprocessed data from buffer file */
	idomod_load_unprocessed_data(idomod_buffer_file);

	/* open data sink and say hello */
	/* 05/04/06 - modified to flush buffer items that may have been read in from file */
	idomod_write_to_sink("\n", IDO_FALSE, IDO_TRUE);

	/* register callbacks */
	if (idomod_register_callbacks() == IDO_ERROR)
		return IDO_ERROR;

	if (idomod_sink_type == IDO_SINK_FILE) {

		/* make sure we have a rotation command defined... */
		if (idomod_sink_rotation_command == NULL) {

			/* log an error message to the Icinga log file */
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "idomod: Warning - No file rotation command defined.\n");
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
		}

		/* schedule a file rotation event */
		else {
			time(&current_time);
			schedule_new_event(EVENT_USER_FUNCTION, TRUE, current_time + idomod_sink_rotation_interval, TRUE, idomod_sink_rotation_interval, NULL, TRUE, (void *)idomod_rotate_sink_file, NULL, 0);
		}
	}

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_init() end\n");

	return IDO_OK;
}


/* performs some shutdown stuff */
int idomod_deinit(void) {

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_deinit() start\n");

	/* deregister callbacks */
	idomod_deregister_callbacks();

	/* save unprocessed data to buffer file */
	idomod_save_unprocessed_data(idomod_buffer_file);

	/* clear sink buffer */
	idomod_sink_buffer_deinit(&sinkbuf);

	/* close data sink */
	idomod_goodbye_sink();
	idomod_close_sink();

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_deinit() end\n");

	/* close debug log */
	idomod_close_debug_log();

	return IDO_OK;
}



/****************************************************************************/
/* CONFIG FUNCTIONS                                                         */
/****************************************************************************/

/* process arguments that were passed to the module at startup */
int idomod_process_module_args(char *args) {
	char *ptr = NULL;
	char **arglist = NULL;
	char **newarglist = NULL;
	int argcount = 0;
	int memblocks = 64;
	int arg = 0;

	if (args == NULL)
		return IDO_OK;


	/* get all the var/val argument pairs */

	/* allocate some memory */
	if ((arglist = (char **)malloc(memblocks * sizeof(char **))) == NULL)
		return IDO_ERROR;

	/* process all args */
	ptr = strtok(args, ",");
	while (ptr) {

		/* save the argument */
		arglist[argcount++] = strdup(ptr);

		/* allocate more memory if needed */
		if (!(argcount % memblocks)) {
			if ((newarglist = (char **)realloc(arglist, (argcount + memblocks) * sizeof(char **))) == NULL) {
				for (arg = 0; arg < argcount; arg++)
					free(arglist[argcount]);
				free(arglist);
				return IDO_ERROR;
			} else
				arglist = newarglist;
		}

		ptr = strtok(NULL, ",");
	}

	/* terminate the arg list */
	arglist[argcount] = '\x0';


	/* process each argument */
	for (arg = 0; arg < argcount; arg++) {
		if (idomod_process_config_var(arglist[arg]) == IDO_ERROR) {
			for (arg = 0; arg < argcount; arg++)
				free(arglist[arg]);
			free(arglist);
			return IDO_ERROR;
		}
	}

	/* free allocated memory */
	for (arg = 0; arg < argcount; arg++)
		free(arglist[arg]);
	free(arglist);

	return IDO_OK;
}


/* process all config vars in a file */
int idomod_process_config_file(char *filename) {
	ido_mmapfile *thefile = NULL;
	char *buf = NULL;
	char temp_buffer[IDOMOD_MAX_BUFLEN];
	int result = IDO_OK;

	/* open the file */
	if ((thefile = ido_mmap_fopen(filename)) == NULL) {
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "idomod: Unable to open configuration file %s: %s\n", filename, strerror(errno));
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
		return IDO_ERROR;
	}

	/* process each line of the file */
	while ((buf = ido_mmap_fgets(thefile))) {

		/* skip comments */
		if (buf[0] == '#') {
			free(buf);
			continue;
		}

		/* skip blank lines */
		if (!strcmp(buf, "")) {
			free(buf);
			continue;
		}

		/* process the variable */
		result = idomod_process_config_var(buf);

		/* free memory */
		free(buf);

		if (result != IDO_OK)
			break;
	}

	/* close the file */
	ido_mmap_fclose(thefile);

	return result;
}


/* process a single module config variable */
int idomod_process_config_var(char *arg) {
	char *var = NULL;
	char *val = NULL;

	char temp_buffer[IDOMOD_MAX_BUFLEN];

	/* split var/val */
	var = strtok(arg, "=");
	val = strtok(NULL, "\n");

	/* skip incomplete var/val pairs */
	if (var == NULL || val == NULL)
		return IDO_OK;

	/* strip var/val */
	idomod_strip(var);
	idomod_strip(val);

	/* process the variable... */

	if (!strcmp(var, "config_file"))
		return idomod_process_config_file(val);

	else if (!strcmp(var, "instance_name"))
		idomod_instance_name = strdup(val);

	else if (!strcmp(var, "output"))
		idomod_sink_name = strdup(val);

	else if (!strcmp(var, "output_type")) {
		if (!strcmp(val, "file"))
			idomod_sink_type = IDO_SINK_FILE;
		else if (!strcmp(val, "tcpsocket"))
			idomod_sink_type = IDO_SINK_TCPSOCKET;
		else
			idomod_sink_type = IDO_SINK_UNIXSOCKET;
	}

	else if (!strcmp(var, "tcp_port"))
		idomod_sink_tcp_port = atoi(val);

	else if (!strcmp(var, "output_buffer_items"))
		idomod_sink_buffer_slots = strtoul(val, NULL, 0);

	else if (!strcmp(var, "reconnect_interval"))
		idomod_sink_reconnect_interval = strtoul(val, NULL, 0);

	else if (!strcmp(var, "reconnect_warning_interval"))
		idomod_sink_reconnect_warning_interval = strtoul(val, NULL, 0);

	else if (!strcmp(var, "file_rotation_interval"))
		idomod_sink_rotation_interval = strtoul(val, NULL, 0);

	else if (!strcmp(var, "file_rotation_command"))
		idomod_sink_rotation_command = strdup(val);

	else if (!strcmp(var, "file_rotation_timeout"))
		idomod_sink_rotation_timeout = atoi(val);

	else if (!strcmp(var, "data_processing_options")) {
		if (!strcmp(val, "-1"))
			idomod_process_options = IDOMOD_PROCESS_EVERYTHING;
		else
			idomod_process_options = strtoul(val, NULL, 0);
	}

	else if (!strcmp(var, "config_output_options"))
		idomod_config_output_options = atoi(val);

	else if (!strcmp(var, "buffer_file"))
		idomod_buffer_file = strdup(val);

	else if (!strcmp(var, "debug_file")) {
		if ((idomod_debug_file = strdup(val)) == NULL)
			return IDO_ERROR;
	} else if (!strcmp(var, "debug_level"))
		idomod_debug_level = atoi(val);
	else if (!strcmp(var, "debug_verbosity"))
		idomod_debug_verbosity = atoi(val);
	else if (!strcmp(var, "max_debug_file_size"))
		idomod_max_debug_file_size = strtoul(val, NULL, 0);

	else if (!strcmp(var, "use_ssl")) {
		if (strlen(val) == 1) {
			if (isdigit((int)val[strlen(val)-1]) != IDO_FALSE)
				use_ssl = atoi(val);
			else
				use_ssl = 0;
		}
	}

	else {
		/* log an error message to the Icinga log file */
		snprintf(temp_buffer, sizeof(temp_buffer) - 1, "idomod: ERROR - Unknown config file variable '%s'.\n", var);
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);

		return IDO_ERROR;
	}

	return IDO_OK;
}



/****************************************************************************/
/* UTILITY FUNCTIONS                                                        */
/****************************************************************************/

/* writes a string to Icinga logs */
int idomod_write_to_logs(char *buf, int flags) {

	if (buf == NULL)
		return IDO_ERROR;

	return write_to_all_logs(buf, flags);
}



/****************************************************************************/
/* DATA SINK FUNCTIONS                                                      */
/****************************************************************************/

/* (re)open data sink */
int idomod_open_sink(void) {
	int flags = 0;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_open_sink() start\n");

	/* sink is already open... */
	if (idomod_sink_is_open == IDO_TRUE)
		return idomod_sink_fd;

	/* try and open sink */
	if (idomod_sink_type == IDO_SINK_FILE)
		flags = O_WRONLY | O_CREAT | O_APPEND;
	if (ido_sink_open(idomod_sink_name, 0, idomod_sink_type, idomod_sink_tcp_port, flags, &idomod_sink_fd) == IDO_ERROR)
		return IDO_ERROR;

	/* mark the sink as being open */
	idomod_sink_is_open = IDO_TRUE;

	/* mark the sink as having once been open */
	idomod_sink_previously_open = IDO_TRUE;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_open_sink() end\n");

	return IDO_OK;
}


/* (re)open data sink */
int idomod_close_sink(void) {

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_close_sink() start\n");

	/* sink is already closed... */
	if (idomod_sink_is_open == IDO_FALSE)
		return IDO_OK;

	/* flush sink */
	ido_sink_flush(idomod_sink_fd);

	/* close sink */
	ido_sink_close(idomod_sink_fd);

	/* mark the sink as being closed */
	idomod_sink_is_open = IDO_FALSE;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_close_sink() end\n");

	return IDO_OK;
}


/* say hello */
int idomod_hello_sink(int reconnect, int problem_disconnect) {
	char temp_buffer[IDOMOD_MAX_BUFLEN];
	char *connection_type = NULL;
	char *connect_type = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_hello_sink() start\n");

	/* get the connection type string */
	if (idomod_sink_type == IDO_SINK_FD || idomod_sink_type == IDO_SINK_FILE)
		connection_type = IDO_API_CONNECTION_FILE;
	else if (idomod_sink_type == IDO_SINK_TCPSOCKET)
		connection_type = IDO_API_CONNECTION_TCPSOCKET;
	else
		connection_type = IDO_API_CONNECTION_UNIXSOCKET;

	/* get the connect type string */
	if (reconnect == TRUE && problem_disconnect == TRUE)
		connect_type = IDO_API_CONNECTTYPE_RECONNECT;
	else
		connect_type = IDO_API_CONNECTTYPE_INITIAL;

	snprintf(temp_buffer, sizeof(temp_buffer) - 1
	         , "\n\n%s\n%s: %d\n%s: %s\n%s: %s\n%s: %lu\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s\n\n"
	         , IDO_API_HELLO
	         , IDO_API_PROTOCOL
	         , IDO_API_PROTOVERSION
	         , IDO_API_AGENT
	         , IDOMOD_NAME
	         , IDO_API_AGENTVERSION
	         , IDO_VERSION
	         , IDO_API_STARTTIME
	         , (unsigned long)time(NULL)
	         , IDO_API_DISPOSITION
	         , IDO_API_DISPOSITION_REALTIME
	         , IDO_API_CONNECTION
	         , connection_type
	         , IDO_API_CONNECTTYPE
	         , connect_type
	         , IDO_API_INSTANCENAME
	         , (idomod_instance_name == NULL) ? "default" : idomod_instance_name
	         , IDO_API_STARTDATADUMP
	        );

	temp_buffer[sizeof(temp_buffer)-1] = '\x0';

	idomod_write_to_sink(temp_buffer, IDO_FALSE, IDO_FALSE);

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_hello_sink() end\n");

	return IDO_OK;
}


/* say goodbye */
int idomod_goodbye_sink(void) {
	char temp_buffer[IDOMOD_MAX_BUFLEN];

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_goodbye_sink() start\n");

	snprintf(temp_buffer, sizeof(temp_buffer) - 1
	         , "\n%d\n%s: %lu\n%s\n\n"
	         , IDO_API_ENDDATADUMP
	         , IDO_API_ENDTIME
	         , (unsigned long)time(NULL)
	         , IDO_API_GOODBYE
	        );

	temp_buffer[sizeof(temp_buffer)-1] = '\x0';

	idomod_write_to_sink(temp_buffer, IDO_FALSE, IDO_TRUE);

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_goodbye_sink() end\n");

	return IDO_OK;
}


/* used to rotate data sink file on a regular basis */
int idomod_rotate_sink_file(void *args) {
	char *raw_command_line_3x = NULL;
	char *processed_command_line_3x = NULL;
	int early_timeout = FALSE;
	double exectime;
	icinga_macros *mac;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_rotate_sink_file() start\n");

	/* get global macros */
	mac = get_global_macros();

	/* close sink */
	idomod_goodbye_sink();
	idomod_close_sink();

	/* we shouldn't write any data to the sink while we're rotating it... */
	idomod_allow_sink_activity = IDO_FALSE;


	/****** ROTATE THE FILE *****/

	/* get the raw command line */
	get_raw_command_line_r(mac, find_command(idomod_sink_rotation_command), idomod_sink_rotation_command, &raw_command_line_3x, STRIP_ILLEGAL_MACRO_CHARS | ESCAPE_MACRO_CHARS);
	strip(raw_command_line_3x);

	/* process any macros in the raw command line */
	process_macros_r(mac, raw_command_line_3x, &processed_command_line_3x, STRIP_ILLEGAL_MACRO_CHARS | ESCAPE_MACRO_CHARS);

	/* run the command */
	my_system(processed_command_line_3x, idomod_sink_rotation_timeout, &early_timeout, &exectime, NULL, 0);


	/* allow data to be written to the sink */
	idomod_allow_sink_activity = IDO_TRUE;

	/* re-open sink */
	idomod_open_sink();
	idomod_hello_sink(TRUE, FALSE);

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_rotate_sink_file() end\n");

	return IDO_OK;
}

/* writes data to sink */
int idomod_write_to_sink(char *buf, int buffer_write, int flush_buffer) {
	char *temp_buffer = NULL;
	char *sbuf = NULL;
	int buflen = 0;
	int result = IDO_OK;
	time_t current_time;
	int reconnect = IDO_FALSE;
	unsigned long items_to_flush = 0L;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_to_sink() start\n");

	/* we have nothing to write... */
	if (buf == NULL)
		return IDO_OK;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_to_sink(%s)\n", buf);

	/* we shouldn't be messing with things... */
	if (idomod_allow_sink_activity == IDO_FALSE)
		return IDO_ERROR;

	/* open the sink if necessary... */
	if (idomod_sink_is_open == IDO_FALSE) {

		time(&current_time);

		/* are we reopening the sink? */
		if (idomod_sink_previously_open == IDO_TRUE)
			reconnect = IDO_TRUE;

		/* (re)connect to the sink if its time */
		if ((unsigned long)((unsigned long)current_time - idomod_sink_reconnect_interval) > (unsigned long)idomod_sink_last_reconnect_attempt) {

			result = idomod_open_sink();

			idomod_sink_last_reconnect_attempt = current_time;

			idomod_sink_connect_attempt++;

			/* sink was (re)opened... */
			if (result == IDO_OK) {

				if (reconnect == IDO_TRUE) {
					if (asprintf(&temp_buffer, "idomod: Successfully reconnected to data sink!  %lu items lost, %lu queued items to flush.", sinkbuf.overflow, sinkbuf.items) == -1)
						temp_buffer = NULL;

					idomod_hello_sink(TRUE, TRUE);

				} else {
					if (sinkbuf.overflow == 0L) {
						if (asprintf(&temp_buffer, "idomod: Successfully connected to data sink.  %lu queued items to flush.", sinkbuf.items) == -1)
							;//temp_buffer=NULL;
					} else {
						if (asprintf(&temp_buffer, "idomod: Successfully connected to data sink.  %lu items lost, %lu queued items to flush.", sinkbuf.overflow, sinkbuf.items) == -1)
							;//temp_buffer=NULL;
					}

					idomod_hello_sink(FALSE, FALSE);
				}

				idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
				free(temp_buffer);
				temp_buffer = NULL;

				/* reset sink overflow */
				sinkbuf.overflow = 0L;

				/* sink could not be (re)opened... */
			} else {

				if ((unsigned long)((unsigned long)current_time - idomod_sink_reconnect_warning_interval) > (unsigned long)idomod_sink_last_reconnect_warning) {
					if (reconnect == IDO_TRUE) {
						if (asprintf(&temp_buffer, "idomod: Still unable to reconnect to data sink.  %lu items lost, %lu queued items to flush. Is ido2db running and processing data?", sinkbuf.overflow, sinkbuf.items) == -1)
							temp_buffer = NULL;
					} else if (idomod_sink_connect_attempt == 1) {
						if (asprintf(&temp_buffer, "idomod: Could not open data sink!  I'll keep trying, but some output may get lost. Is ido2db running and processing data?") == -1)
							temp_buffer = NULL;
					} else {
						if (asprintf(&temp_buffer, "idomod: Still unable to connect to data sink.  %lu items lost, %lu queued items to flush. Is ido2db running and processing data?", sinkbuf.overflow, sinkbuf.items) == -1)
							temp_buffer = NULL;
					}

					idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
					free(temp_buffer);
					temp_buffer = NULL;

					idomod_sink_last_reconnect_warning = current_time;
				}
			}
		}
	}

	/* we weren't able to (re)connect */
	if (idomod_sink_is_open == IDO_FALSE) {

		/***** BUFFER OUTPUT FOR LATER *****/

		if (buffer_write == IDO_TRUE)
			idomod_sink_buffer_push(&sinkbuf, buf);

		return IDO_ERROR;
	}


	/***** FLUSH BUFFERED DATA FIRST *****/

	if (flush_buffer == IDO_TRUE && (items_to_flush = idomod_sink_buffer_items(&sinkbuf)) > 0) {

		while (idomod_sink_buffer_items(&sinkbuf) > 0) {

			/* get next item from buffer */
			sbuf = idomod_sink_buffer_peek(&sinkbuf);

			buflen = strlen(sbuf);
			result = ido_sink_write(idomod_sink_fd, sbuf, buflen);

			/* an error occurred... */
			if (result < 0) {

				/* sink problem! */
				if (errno != EAGAIN) {

					/* close the sink */
					idomod_close_sink();

					if (asprintf(&temp_buffer, "idomod: Error writing to data sink!  Some output may get lost.  %lu queued items to flush.", sinkbuf.items) == -1)
						temp_buffer = NULL;

					idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
					free(temp_buffer);
					temp_buffer = NULL;

					time(&current_time);
					idomod_sink_last_reconnect_attempt = current_time;
					idomod_sink_last_reconnect_warning = current_time;
				}

				/***** BUFFER ORIGINAL OUTPUT FOR LATER *****/

				if (buffer_write == IDO_TRUE)
					idomod_sink_buffer_push(&sinkbuf, buf);

				return IDO_ERROR;
			}

			/* buffer was written okay, so remove it from buffer */
			idomod_sink_buffer_pop(&sinkbuf);
		}

		if (asprintf(&temp_buffer, "idomod: Successfully flushed %lu queued items to data sink.", items_to_flush) == -1)
			temp_buffer = NULL;

		idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
		free(temp_buffer);
		temp_buffer = NULL;
	}


	/***** WRITE ORIGINAL DATA *****/

	/* write the data */
	buflen = strlen(buf);
	result = ido_sink_write(idomod_sink_fd, buf, buflen);

	/* an error occurred... */
	if (result < 0) {

		/* sink problem! */
		if (errno != EAGAIN) {

			/* close the sink */
			idomod_close_sink();

			time(&current_time);
			idomod_sink_last_reconnect_attempt = current_time;
			idomod_sink_last_reconnect_warning = current_time;

			if (asprintf(&temp_buffer, "idomod: Error writing to data sink!  Some output may get lost...") == -1)
				temp_buffer = NULL;

			idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
			free(temp_buffer);

			if (asprintf(&temp_buffer, "idomod: Please check remote ido2db log, database connection or SSL Parameters") == -1)
				temp_buffer = NULL;

			idomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
			free(temp_buffer);
			temp_buffer = NULL;
		}

		/***** BUFFER OUTPUT FOR LATER *****/

		if (buffer_write == IDO_TRUE)
			idomod_sink_buffer_push(&sinkbuf, buf);

		return IDO_ERROR;
	}

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_to_sink() end\n");

	return IDO_OK;
}



/* save unprocessed data to buffer file */
int idomod_save_unprocessed_data(char *f) {
	FILE *fp = NULL;
	char *buf = NULL;
	char *ebuf = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_save_unprocessed_data() start\n");

	/* no file */
	if (f == NULL)
		return IDO_OK;

	/* open the file for writing */
	if ((fp = fopen(f, "w")) == NULL)
		return IDO_ERROR;

	/* save all buffered items */
	while (idomod_sink_buffer_items(&sinkbuf) > 0) {

		/* get next item from buffer */
		buf = idomod_sink_buffer_pop(&sinkbuf);

		/* escape the string */
		ebuf = ido_escape_buffer(buf);

		/* write string to file */
		fputs(ebuf, fp);
		fputs("\n", fp);

		/* free memory */
		free(buf);
		buf = NULL;
		free(ebuf);
		ebuf = NULL;
	}

	fclose(fp);

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_save_unprocessed_data() end\n");

	return IDO_OK;
}



/* load unprocessed data from buffer file */
int idomod_load_unprocessed_data(char *f) {
	ido_mmapfile *thefile = NULL;
	char *ebuf = NULL;
	char *buf = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_load_unprocessed_data() start\n");

	/* open the file */
	if ((thefile = ido_mmap_fopen(f)) == NULL)
		return IDO_ERROR;

	/* process each line of the file */
	while ((ebuf = ido_mmap_fgets(thefile))) {

		/* unescape string */
		buf = ido_unescape_buffer(ebuf);

		/* save the data to the sink buffer */
		idomod_sink_buffer_push(&sinkbuf, buf);

		/* free memory */
		free(ebuf);
	}

	/* close the file */
	ido_mmap_fclose(thefile);

	/* remove the file so we don't process it again in the future */
	unlink(f);

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_load_unprocessed_data() end\n");

	return IDO_OK;
}



/* initializes sink buffer */
int idomod_sink_buffer_init(idomod_sink_buffer *sbuf, unsigned long maxitems) {
	unsigned long x;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_init() start\n");

	if (sbuf == NULL || maxitems <= 0)
		return IDO_ERROR;

	/* allocate memory for the buffer */
	if ((sbuf->buffer = (char **)malloc(sizeof(char *) * maxitems))) {
		for (x = 0; x < maxitems; x++)
			sbuf->buffer[x] = NULL;
	}

	sbuf->size = 0L;
	sbuf->head = 0L;
	sbuf->tail = 0L;
	sbuf->items = 0L;
	sbuf->maxitems = maxitems;
	sbuf->overflow = 0L;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_init() end\n");

	return IDO_OK;
}


/* deinitializes sink buffer */
int idomod_sink_buffer_deinit(idomod_sink_buffer *sbuf) {
	unsigned long x;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_deinit() start\n");

	if (sbuf == NULL)
		return IDO_ERROR;

	/* free any allocated memory */
	for (x = 0; x < sbuf->maxitems; x++)
		free(sbuf->buffer[x]);

	free(sbuf->buffer);
	sbuf->buffer = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_deinit() end\n");

	return IDO_OK;
}


/* buffers output */
int idomod_sink_buffer_push(idomod_sink_buffer *sbuf, char *buf) {

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_push() start\n");

	if (sbuf == NULL || buf == NULL)
		return IDO_ERROR;

	/* no space to store buffer */
	if (sbuf->buffer == NULL || sbuf->items == sbuf->maxitems) {
		sbuf->overflow++;
		return IDO_ERROR;
	}

	/* store buffer */
	sbuf->buffer[sbuf->head] = strdup(buf);
	sbuf->head = (sbuf->head + 1) % sbuf->maxitems;
	sbuf->items++;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_push() end\n");

	return IDO_OK;
}


/* gets and removes next item from buffer */
char *idomod_sink_buffer_pop(idomod_sink_buffer *sbuf) {
	char *buf = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_pop() start\n");

	if (sbuf == NULL)
		return NULL;

	if (sbuf->buffer == NULL)
		return NULL;

	if (sbuf->items == 0)
		return NULL;

	/* remove item from buffer */
	buf = sbuf->buffer[sbuf->tail];
	sbuf->buffer[sbuf->tail] = NULL;
	sbuf->tail = (sbuf->tail + 1) % sbuf->maxitems;
	sbuf->items--;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_pop() end\n");

	return buf;
}


/* gets next items from buffer */
char *idomod_sink_buffer_peek(idomod_sink_buffer *sbuf) {
	char *buf = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_peek() start\n");

	if (sbuf == NULL)
		return NULL;

	if (sbuf->buffer == NULL)
		return NULL;

	buf = sbuf->buffer[sbuf->tail];

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_peek() end\n");

	return buf;
}


/* returns number of items buffered */
int idomod_sink_buffer_items(idomod_sink_buffer *sbuf) {

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_items()\n");

	if (sbuf == NULL)
		return 0;
	else
		return sbuf->items;
}



/* gets number of items lost due to buffer overflow */
unsigned long idomod_sink_buffer_get_overflow(idomod_sink_buffer *sbuf) {

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_get_overflow()\n");

	if (sbuf == NULL)
		return 0;
	else
		return sbuf->overflow;
}


/* sets number of items lost due to buffer overflow */
int idomod_sink_buffer_set_overflow(idomod_sink_buffer *sbuf, unsigned long num) {

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_sink_buffer_set_overflow()\n");

	if (sbuf == NULL)
		return 0;
	else
		sbuf->overflow = num;

	return sbuf->overflow;
}



/****************************************************************************/
/* CALLBACK FUNCTIONS                                                       */
/****************************************************************************/

/* registers for callbacks */
int idomod_register_callbacks(void) {
	int priority = 0;
	int result = IDO_OK;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_register_callbacks() start\n");

	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_PROCESS_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_TIMED_EVENT_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_LOG_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_EVENT_HANDLER_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_NOTIFICATION_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_SERVICE_CHECK_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_HOST_CHECK_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_COMMENT_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_DOWNTIME_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_FLAPPING_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_PROGRAM_STATUS_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_HOST_STATUS_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_SERVICE_STATUS_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_ADAPTIVE_PROGRAM_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_ADAPTIVE_HOST_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_ADAPTIVE_SERVICE_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_AGGREGATED_STATUS_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_RETENTION_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_STATE_CHANGE_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_CONTACT_STATUS_DATA, idomod_module_handle, priority, idomod_broker_data);
	if (result == IDO_OK)
		result = neb_register_callback(NEBCALLBACK_ADAPTIVE_CONTACT_DATA, idomod_module_handle, priority, idomod_broker_data);

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_register_callbacks() end\n");

	return result;
}


/* deregisters callbacks */
int idomod_deregister_callbacks(void) {

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_deregister_callbacks() start\n");

	neb_deregister_callback(NEBCALLBACK_PROCESS_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_TIMED_EVENT_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_LOG_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_EVENT_HANDLER_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_COMMENT_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_DOWNTIME_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_FLAPPING_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_PROGRAM_STATUS_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_HOST_STATUS_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_SERVICE_STATUS_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_PROGRAM_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_HOST_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_SERVICE_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_AGGREGATED_STATUS_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_RETENTION_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA, idomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_CONTACT_DATA, idomod_broker_data);

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_deregister_callbacks() end\n");

	return IDO_OK;
}


/* handles brokered event data */
int idomod_broker_data(int event_type, void *data) {
	//char temp_buffer[IDOMOD_MAX_BUFLEN];
	static char *temp_buffer;
	ido_dbuf dbuf;
	int write_to_sink = IDO_TRUE;
	host *temp_host = NULL;
	service *temp_service = NULL;
	contact *temp_contact = NULL;
	char *es[9];
	int x = 0;
	scheduled_downtime *temp_downtime = NULL;
	comment *temp_comment = NULL;
	nebstruct_process_data *procdata = NULL;
	nebstruct_timed_event_data *eventdata = NULL;
	nebstruct_log_data *logdata = NULL;
	nebstruct_system_command_data *cmddata = NULL;
	nebstruct_event_handler_data *ehanddata = NULL;
	nebstruct_notification_data *notdata = NULL;
	nebstruct_service_check_data *scdata = NULL;
	nebstruct_host_check_data *hcdata = NULL;
	nebstruct_comment_data *comdata = NULL;
	nebstruct_downtime_data *downdata = NULL;
	nebstruct_flapping_data *flapdata = NULL;
	nebstruct_program_status_data *psdata = NULL;
	nebstruct_host_status_data *hsdata = NULL;
	nebstruct_service_status_data *ssdata = NULL;
	nebstruct_adaptive_program_data *apdata = NULL;
	nebstruct_adaptive_host_data *ahdata = NULL;
	nebstruct_adaptive_service_data *asdata = NULL;
	nebstruct_external_command_data *ecdata = NULL;
	nebstruct_aggregated_status_data *agsdata = NULL;
	nebstruct_retention_data *rdata = NULL;
	nebstruct_contact_notification_data *cnotdata = NULL;
	nebstruct_contact_notification_method_data *cnotmdata = NULL;
	nebstruct_acknowledgement_data *ackdata = NULL;
	nebstruct_statechange_data *schangedata = NULL;
	nebstruct_contact_status_data *csdata = NULL;
	nebstruct_adaptive_contact_data *acdata = NULL;
	customvariablesmember *temp_customvar = NULL;

	double retry_interval = 0.0;
	int last_state = -1;
	int last_hard_state = -1;

	if (temp_buffer == NULL) {
		temp_buffer = (char *)malloc(IDOMOD_MAX_BUFLEN);
		if (temp_buffer == NULL)
			return 0;
	}


	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_broker_data() start\n");

	if (data == NULL)
		return 0;

	/* should we handle this type of data? */
	switch (event_type) {

	case NEBCALLBACK_PROCESS_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_PROCESS_DATA))
			return 0;
		break;
	case NEBCALLBACK_TIMED_EVENT_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_TIMED_EVENT_DATA))
			return 0;
		break;
	case NEBCALLBACK_LOG_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_LOG_DATA))
			return 0;
		break;
	case NEBCALLBACK_SYSTEM_COMMAND_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_SYSTEM_COMMAND_DATA))
			return 0;
		break;
	case NEBCALLBACK_EVENT_HANDLER_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_EVENT_HANDLER_DATA))
			return 0;
		break;
	case NEBCALLBACK_NOTIFICATION_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_NOTIFICATION_DATA))
			return 0;
		break;
	case NEBCALLBACK_SERVICE_CHECK_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_SERVICE_CHECK_DATA))
			return 0;
		break;
	case NEBCALLBACK_HOST_CHECK_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_HOST_CHECK_DATA))
			return 0;
		break;
	case NEBCALLBACK_COMMENT_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_COMMENT_DATA))
			return 0;
		break;
	case NEBCALLBACK_DOWNTIME_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_DOWNTIME_DATA))
			return 0;
		break;
	case NEBCALLBACK_FLAPPING_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_FLAPPING_DATA))
			return 0;
		break;
	case NEBCALLBACK_PROGRAM_STATUS_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_PROGRAM_STATUS_DATA))
			return 0;
		break;
	case NEBCALLBACK_HOST_STATUS_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_HOST_STATUS_DATA))
			return 0;
		break;
	case NEBCALLBACK_SERVICE_STATUS_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_SERVICE_STATUS_DATA))
			return 0;
		break;
	case NEBCALLBACK_CONTACT_STATUS_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_CONTACT_STATUS_DATA))
			return 0;
		break;
	case NEBCALLBACK_ADAPTIVE_PROGRAM_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA))
			return 0;
		break;
	case NEBCALLBACK_ADAPTIVE_HOST_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_ADAPTIVE_HOST_DATA))
			return 0;
		break;
	case NEBCALLBACK_ADAPTIVE_SERVICE_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA))
			return 0;
		break;
	case NEBCALLBACK_ADAPTIVE_CONTACT_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA))
			return 0;
		break;
	case NEBCALLBACK_EXTERNAL_COMMAND_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_EXTERNAL_COMMAND_DATA))
			return 0;
		break;
	case NEBCALLBACK_AGGREGATED_STATUS_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_AGGREGATED_STATUS_DATA))
			return 0;
		break;
	case NEBCALLBACK_RETENTION_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_RETENTION_DATA))
			return 0;
		break;
	case NEBCALLBACK_CONTACT_NOTIFICATION_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_NOTIFICATION_DATA))
			return 0;
		break;
	case NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_NOTIFICATION_DATA))
			return 0;
		break;
	case NEBCALLBACK_ACKNOWLEDGEMENT_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA))
			return 0;
		break;
	case NEBCALLBACK_STATE_CHANGE_DATA:
		if (!(idomod_process_options & IDOMOD_PROCESS_STATECHANGE_DATA))
			return 0;
		break;
	default:
		break;
	}


	/* initialize escaped buffers */
	for (x = 0; x < 8; x++)
		es[x] = NULL;

	/* initialize dynamic buffer (2KB chunk size) */
	ido_dbuf_init(&dbuf, 2048);


	/* handle the event */
	switch (event_type) {

	case NEBCALLBACK_PROCESS_DATA:

		procdata = (nebstruct_process_data *)data;

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%lu\n%d\n\n"
		         , IDO_API_PROCESSDATA
		         , IDO_DATA_TYPE
		         , procdata->type
		         , IDO_DATA_FLAGS
		         , procdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , procdata->attr
		         , IDO_DATA_TIMESTAMP
		         , procdata->timestamp.tv_sec
		         , procdata->timestamp.tv_usec
		         , IDO_DATA_PROGRAMNAME
		         , "Icinga"
		         , IDO_DATA_PROGRAMVERSION
		         , get_program_version()
		         , IDO_DATA_PROGRAMDATE
		         , get_program_modification_date()
		         , IDO_DATA_PROCESSID
		         , (unsigned long)getpid()
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_TIMED_EVENT_DATA:

		eventdata = (nebstruct_timed_event_data *)data;

		switch (eventdata->event_type) {

		case EVENT_SERVICE_CHECK:
			temp_service = (service *)eventdata->event_data;

			es[0] = ido_escape_buffer(temp_service->host_name);
			es[1] = ido_escape_buffer(temp_service->description);

			snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
			         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d\n\n"
			         , IDO_API_TIMEDEVENTDATA
			         , IDO_DATA_TYPE
			         , eventdata->type
			         , IDO_DATA_FLAGS
			         , eventdata->flags
			         , IDO_DATA_ATTRIBUTES
			         , eventdata->attr
			         , IDO_DATA_TIMESTAMP
			         , eventdata->timestamp.tv_sec
			         , eventdata->timestamp.tv_usec
			         , IDO_DATA_EVENTTYPE
			         , eventdata->event_type
			         , IDO_DATA_RECURRING
			         , eventdata->recurring
			         , IDO_DATA_RUNTIME
			         , (unsigned long)eventdata->run_time
			         , IDO_DATA_HOST
			         , (es[0] == NULL) ? "" : es[0]
			         , IDO_DATA_SERVICE
			         , (es[1] == NULL) ? "" : es[1]
			         , IDO_API_ENDDATA
			        );

			break;

		case EVENT_HOST_CHECK:
			temp_host = (host *)eventdata->event_data;

			es[0] = ido_escape_buffer(temp_host->name);

			snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
			         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d\n\n"
			         , IDO_API_TIMEDEVENTDATA
			         , IDO_DATA_TYPE
			         , eventdata->type
			         , IDO_DATA_FLAGS
			         , eventdata->flags
			         , IDO_DATA_ATTRIBUTES
			         , eventdata->attr
			         , IDO_DATA_TIMESTAMP
			         , eventdata->timestamp.tv_sec
			         , eventdata->timestamp.tv_usec
			         , IDO_DATA_EVENTTYPE
			         , eventdata->event_type
			         , IDO_DATA_RECURRING
			         , eventdata->recurring
			         , IDO_DATA_RUNTIME
			         , (unsigned long)eventdata->run_time
			         , IDO_DATA_HOST
			         , (es[0] == NULL) ? "" : es[0]
			         , IDO_API_ENDDATA
			        );

			break;

		case EVENT_SCHEDULED_DOWNTIME:
			temp_downtime = find_downtime(ANY_DOWNTIME, (unsigned long)eventdata->event_data);

			if (temp_downtime != NULL) {
				es[0] = ido_escape_buffer(temp_downtime->host_name);
				es[1] = ido_escape_buffer(temp_downtime->service_description);
			}

			snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
			         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d\n\n"
			         , IDO_API_TIMEDEVENTDATA
			         , IDO_DATA_TYPE
			         , eventdata->type
			         , IDO_DATA_FLAGS
			         , eventdata->flags
			         , IDO_DATA_ATTRIBUTES
			         , eventdata->attr
			         , IDO_DATA_TIMESTAMP
			         , eventdata->timestamp.tv_sec
			         , eventdata->timestamp.tv_usec
			         , IDO_DATA_EVENTTYPE
			         , eventdata->event_type
			         , IDO_DATA_RECURRING
			         , eventdata->recurring
			         , IDO_DATA_RUNTIME
			         , (unsigned long)eventdata->run_time
			         , IDO_DATA_HOST
			         , (es[0] == NULL) ? "" : es[0]
			         , IDO_DATA_SERVICE
			         , (es[1] == NULL) ? "" : es[1]
			         , IDO_API_ENDDATA
			        );

			break;

		default:
			snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
			         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%d\n%d=%lu\n%d\n\n"
			         , IDO_API_TIMEDEVENTDATA
			         , IDO_DATA_TYPE
			         , eventdata->type
			         , IDO_DATA_FLAGS
			         , eventdata->flags
			         , IDO_DATA_ATTRIBUTES
			         , eventdata->attr
			         , IDO_DATA_TIMESTAMP
			         , eventdata->timestamp.tv_sec
			         , eventdata->timestamp.tv_usec
			         , IDO_DATA_EVENTTYPE
			         , eventdata->event_type
			         , IDO_DATA_RECURRING
			         , eventdata->recurring
			         , IDO_DATA_RUNTIME
			         , (unsigned long)eventdata->run_time
			         , IDO_API_ENDDATA
			        );
			break;
		}

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_LOG_DATA:

		logdata = (nebstruct_log_data *)data;

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%lu\n%d=%d\n%d=%s\n%d\n\n"
		         , IDO_API_LOGDATA
		         , IDO_DATA_TYPE
		         , logdata->type
		         , IDO_DATA_FLAGS
		         , logdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , logdata->attr
		         , IDO_DATA_TIMESTAMP
		         , logdata->timestamp.tv_sec
		         , logdata->timestamp.tv_usec
		         , IDO_DATA_LOGENTRYTIME
		         , logdata->entry_time
		         , IDO_DATA_LOGENTRYTYPE
		         , logdata->data_type
		         , IDO_DATA_LOGENTRY
		         , logdata->data
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_SYSTEM_COMMAND_DATA:

		cmddata = (nebstruct_system_command_data *)data;

		es[0] = ido_escape_buffer(cmddata->command_line);
		es[1] = ido_escape_buffer(cmddata->output);
		es[2] = ido_escape_buffer(cmddata->output);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%d\n%d=%.5lf\n%d=%d\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_SYSTEMCOMMANDDATA
		         , IDO_DATA_TYPE
		         , cmddata->type
		         , IDO_DATA_FLAGS
		         , cmddata->flags
		         , IDO_DATA_ATTRIBUTES
		         , cmddata->attr
		         , IDO_DATA_TIMESTAMP
		         , cmddata->timestamp.tv_sec
		         , cmddata->timestamp.tv_usec
		         , IDO_DATA_STARTTIME
		         , cmddata->start_time.tv_sec
		         , cmddata->start_time.tv_usec
		         , IDO_DATA_ENDTIME
		         , cmddata->end_time.tv_sec
		         , cmddata->end_time.tv_usec
		         , IDO_DATA_TIMEOUT
		         , cmddata->timeout
		         , IDO_DATA_COMMANDLINE
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_EARLYTIMEOUT
		         , cmddata->early_timeout
		         , IDO_DATA_EXECUTIONTIME
		         , cmddata->execution_time
		         , IDO_DATA_RETURNCODE
		         , cmddata->return_code
		         , IDO_DATA_OUTPUT
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_LONGOUTPUT
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_EVENT_HANDLER_DATA:

		ehanddata = (nebstruct_event_handler_data *)data;

		es[0] = ido_escape_buffer(ehanddata->host_name);
		es[1] = ido_escape_buffer(ehanddata->service_description);
		es[2] = ido_escape_buffer(ehanddata->command_name);
		es[3] = ido_escape_buffer(ehanddata->command_args);
		es[4] = ido_escape_buffer(ehanddata->command_line);
		es[5] = ido_escape_buffer(ehanddata->output);
		/* Preparing if eventhandler will have long_output in the future */
		es[6] = ido_escape_buffer(ehanddata->output);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%.5lf\n%d=%d\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_EVENTHANDLERDATA
		         , IDO_DATA_TYPE
		         , ehanddata->type
		         , IDO_DATA_FLAGS
		         , ehanddata->flags
		         , IDO_DATA_ATTRIBUTES
		         , ehanddata->attr
		         , IDO_DATA_TIMESTAMP
		         , ehanddata->timestamp.tv_sec
		         , ehanddata->timestamp.tv_usec
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_STATETYPE
		         , ehanddata->state_type
		         , IDO_DATA_STATE
		         , ehanddata->state
		         , IDO_DATA_STARTTIME
		         , ehanddata->start_time.tv_sec
		         , ehanddata->start_time.tv_usec
		         , IDO_DATA_ENDTIME
		         , ehanddata->end_time.tv_sec
		         , ehanddata->end_time.tv_usec
		         , IDO_DATA_TIMEOUT
		         , ehanddata->timeout
		         , IDO_DATA_COMMANDNAME
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_COMMANDARGS
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_COMMANDLINE
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_EARLYTIMEOUT
		         , ehanddata->early_timeout
		         , IDO_DATA_EXECUTIONTIME
		         , ehanddata->execution_time
		         , IDO_DATA_RETURNCODE
		         , ehanddata->return_code
		         , IDO_DATA_OUTPUT
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_DATA_LONGOUTPUT
		         , (es[6] == NULL) ? "" : es[6]
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_NOTIFICATION_DATA:

		notdata = (nebstruct_notification_data *)data;

		es[0] = ido_escape_buffer(notdata->host_name);
		es[1] = ido_escape_buffer(notdata->service_description);
		es[2] = ido_escape_buffer(notdata->output);
		/* Preparing if notifications will have long_output in the future */
		es[3] = ido_escape_buffer(notdata->output);
		es[4] = ido_escape_buffer(notdata->ack_author);
		es[5] = ido_escape_buffer(notdata->ack_data);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d\n\n"
		         , IDO_API_NOTIFICATIONDATA
		         , IDO_DATA_TYPE
		         , notdata->type
		         , IDO_DATA_FLAGS
		         , notdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , notdata->attr
		         , IDO_DATA_TIMESTAMP
		         , notdata->timestamp.tv_sec
		         , notdata->timestamp.tv_usec
		         , IDO_DATA_NOTIFICATIONTYPE
		         , notdata->notification_type
		         , IDO_DATA_STARTTIME
		         , notdata->start_time.tv_sec
		         , notdata->start_time.tv_usec
		         , IDO_DATA_ENDTIME
		         , notdata->end_time.tv_sec
		         , notdata->end_time.tv_usec
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_NOTIFICATIONREASON
		         , notdata->reason_type
		         , IDO_DATA_STATE
		         , notdata->state
		         , IDO_DATA_OUTPUT
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_LONGOUTPUT
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_ACKAUTHOR
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_ACKDATA
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_DATA_ESCALATED
		         , notdata->escalated
		         , IDO_DATA_CONTACTSNOTIFIED
		         , notdata->contacts_notified
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_SERVICE_CHECK_DATA:

		scdata = (nebstruct_service_check_data *)data;

		/* only pass NEBTYPE_SERVICECHECK_PROCESSED to ido2db */
		if (scdata->type != NEBTYPE_SERVICECHECK_PROCESSED)
			break;

		es[0] = ido_escape_buffer(scdata->host_name);
		es[1] = ido_escape_buffer(scdata->service_description);
		es[2] = ido_escape_buffer(scdata->command_name);
		es[3] = ido_escape_buffer(scdata->command_args);
		es[4] = ido_escape_buffer(scdata->command_line);
		es[5] = ido_escape_buffer(scdata->output);
		es[6] = ido_escape_buffer(scdata->long_output);
		es[7] = ido_escape_buffer(scdata->perf_data);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%d\n%d=%.5lf\n%d=%.5lf\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_SERVICECHECKDATA
		         , IDO_DATA_TYPE
		         , scdata->type
		         , IDO_DATA_FLAGS
		         , scdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , scdata->attr
		         , IDO_DATA_TIMESTAMP
		         , scdata->timestamp.tv_sec
		         , scdata->timestamp.tv_usec
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_CHECKTYPE
		         , scdata->check_type
		         , IDO_DATA_CURRENTCHECKATTEMPT
		         , scdata->current_attempt
		         , IDO_DATA_MAXCHECKATTEMPTS
		         , scdata->max_attempts
		         , IDO_DATA_STATETYPE
		         , scdata->state_type
		         , IDO_DATA_STATE
		         , scdata->state
		         , IDO_DATA_TIMEOUT
		         , scdata->timeout
		         , IDO_DATA_COMMANDNAME
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_COMMANDARGS
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_COMMANDLINE
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_STARTTIME
		         , scdata->start_time.tv_sec
		         , scdata->start_time.tv_usec
		         , IDO_DATA_ENDTIME
		         , scdata->end_time.tv_sec
		         , scdata->end_time.tv_usec
		         , IDO_DATA_EARLYTIMEOUT
		         , scdata->early_timeout
		         , IDO_DATA_EXECUTIONTIME
		         , scdata->execution_time
		         , IDO_DATA_LATENCY
		         , scdata->latency
		         , IDO_DATA_RETURNCODE
		         , scdata->return_code
		         , IDO_DATA_OUTPUT
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_DATA_LONGOUTPUT
		         , (es[6] == NULL) ? "" : es[6]
		         , IDO_DATA_PERFDATA
		         , (es[7] == NULL) ? "" : es[7]
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_HOST_CHECK_DATA:

		hcdata = (nebstruct_host_check_data *)data;

		/* only pass NEBTYPE_HOSTCHECK_PROCESSED to ido2db */
		if (hcdata->type != NEBTYPE_HOSTCHECK_PROCESSED)
			break;

		es[0] = ido_escape_buffer(hcdata->host_name);
		es[1] = ido_escape_buffer(hcdata->command_name);
		es[2] = ido_escape_buffer(hcdata->command_args);
		es[3] = ido_escape_buffer(hcdata->command_line);
		es[4] = ido_escape_buffer(hcdata->output);
		es[5] = ido_escape_buffer(hcdata->long_output);
		es[6] = ido_escape_buffer(hcdata->perf_data);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%d\n%d=%.5lf\n%d=%.5lf\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_HOSTCHECKDATA
		         , IDO_DATA_TYPE
		         , hcdata->type
		         , IDO_DATA_FLAGS
		         , hcdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , hcdata->attr
		         , IDO_DATA_TIMESTAMP
		         , hcdata->timestamp.tv_sec
		         , hcdata->timestamp.tv_usec
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_CHECKTYPE
		         , hcdata->check_type
		         , IDO_DATA_CURRENTCHECKATTEMPT
		         , hcdata->current_attempt
		         , IDO_DATA_MAXCHECKATTEMPTS
		         , hcdata->max_attempts
		         , IDO_DATA_STATETYPE
		         , hcdata->state_type
		         , IDO_DATA_STATE
		         , hcdata->state
		         , IDO_DATA_TIMEOUT
		         , hcdata->timeout
		         , IDO_DATA_COMMANDNAME
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_COMMANDARGS
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_COMMANDLINE
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_STARTTIME
		         , hcdata->start_time.tv_sec
		         , hcdata->start_time.tv_usec
		         , IDO_DATA_ENDTIME
		         , hcdata->end_time.tv_sec
		         , hcdata->end_time.tv_usec
		         , IDO_DATA_EARLYTIMEOUT
		         , hcdata->early_timeout
		         , IDO_DATA_EXECUTIONTIME
		         , hcdata->execution_time
		         , IDO_DATA_LATENCY
		         , hcdata->latency
		         , IDO_DATA_RETURNCODE
		         , hcdata->return_code
		         , IDO_DATA_OUTPUT
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_LONGOUTPUT
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_DATA_PERFDATA
		         , (es[6] == NULL) ? "" : es[6]
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_COMMENT_DATA:

		comdata = (nebstruct_comment_data *)data;

		es[0] = ido_escape_buffer(comdata->host_name);
		es[1] = ido_escape_buffer(comdata->service_description);
		es[2] = ido_escape_buffer(comdata->author_name);
		es[3] = ido_escape_buffer(comdata->comment_data);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%lu\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d\n\n"
		         , IDO_API_COMMENTDATA
		         , IDO_DATA_TYPE
		         , comdata->type
		         , IDO_DATA_FLAGS
		         , comdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , comdata->attr
		         , IDO_DATA_TIMESTAMP
		         , comdata->timestamp.tv_sec
		         , comdata->timestamp.tv_usec
		         , IDO_DATA_COMMENTTYPE
		         , comdata->comment_type
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_ENTRYTIME
		         , (unsigned long)comdata->entry_time
		         , IDO_DATA_AUTHORNAME
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_COMMENT
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_PERSISTENT
		         , comdata->persistent
		         , IDO_DATA_SOURCE
		         , comdata->source
		         , IDO_DATA_ENTRYTYPE
		         , comdata->entry_type
		         , IDO_DATA_EXPIRES
		         , comdata->expires
		         , IDO_DATA_EXPIRATIONTIME
		         , (unsigned long)comdata->expire_time
		         , IDO_DATA_COMMENTID
		         , comdata->comment_id
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_DOWNTIME_DATA:

		downdata = (nebstruct_downtime_data *)data;

		es[0] = ido_escape_buffer(downdata->host_name);
		es[1] = ido_escape_buffer(downdata->service_description);
		es[2] = ido_escape_buffer(downdata->author_name);
		es[3] = ido_escape_buffer(downdata->comment_data);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%lu\n%d=%s\n%d=%s\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d\n\n"
		         , IDO_API_DOWNTIMEDATA
		         , IDO_DATA_TYPE
		         , downdata->type
		         , IDO_DATA_FLAGS
		         , downdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , downdata->attr
		         , IDO_DATA_TIMESTAMP
		         , downdata->timestamp.tv_sec
		         , downdata->timestamp.tv_usec
		         , IDO_DATA_DOWNTIMETYPE
		         , downdata->downtime_type
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_ENTRYTIME
		         , downdata->entry_time
		         , IDO_DATA_AUTHORNAME
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_COMMENT
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_STARTTIME
		         , (unsigned long)downdata->start_time
		         , IDO_DATA_ENDTIME
		         , (unsigned long)downdata->end_time
		         , IDO_DATA_FIXED
		         , downdata->fixed
		         , IDO_DATA_DURATION
		         , (unsigned long)downdata->duration
		         , IDO_DATA_TRIGGEREDBY
		         , (unsigned long)downdata->triggered_by
		         , IDO_DATA_DOWNTIMEID
		         , (unsigned long)downdata->downtime_id
		         , IDO_DATA_DOWNTIMEISINEFFECT
		         , (int)downdata->is_in_effect
		         , IDO_DATA_DOWNTIMETRIGGERTIME
		         , (unsigned long)downdata->trigger_time
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_FLAPPING_DATA:

		flapdata = (nebstruct_flapping_data *)data;

		es[0] = ido_escape_buffer(flapdata->host_name);
		es[1] = ido_escape_buffer(flapdata->service_description);

		if (flapdata->flapping_type == HOST_FLAPPING)
			temp_comment = find_host_comment(flapdata->comment_id);
		else
			temp_comment = find_service_comment(flapdata->comment_id);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%.5lf\n%d=%.5lf\n%d=%.5lf\n%d=%lu\n%d=%lu\n%d\n\n"
		         , IDO_API_FLAPPINGDATA
		         , IDO_DATA_TYPE
		         , flapdata->type
		         , IDO_DATA_FLAGS
		         , flapdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , flapdata->attr
		         , IDO_DATA_TIMESTAMP
		         , flapdata->timestamp.tv_sec
		         , flapdata->timestamp.tv_usec
		         , IDO_DATA_FLAPPINGTYPE
		         , flapdata->flapping_type
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_PERCENTSTATECHANGE
		         , flapdata->percent_change
		         , IDO_DATA_HIGHTHRESHOLD
		         , flapdata->high_threshold
		         , IDO_DATA_LOWTHRESHOLD
		         , flapdata->low_threshold
		         , IDO_DATA_COMMENTTIME
		         , (temp_comment == NULL) ? 0L : (unsigned long)temp_comment->entry_time
		         , IDO_DATA_COMMENTID
		         , flapdata->comment_id
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_PROGRAM_STATUS_DATA:

		psdata = (nebstruct_program_status_data *)data;

		es[0] = ido_escape_buffer(psdata->global_host_event_handler);
		es[1] = ido_escape_buffer(psdata->global_service_event_handler);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%lu\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%s\n%d=%s\n%d=%lu\n%d\n\n"
		         , IDO_API_PROGRAMSTATUSDATA
		         , IDO_DATA_TYPE
		         , psdata->type
		         , IDO_DATA_FLAGS
		         , psdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , psdata->attr
		         , IDO_DATA_TIMESTAMP
		         , psdata->timestamp.tv_sec
		         , psdata->timestamp.tv_usec
		         , IDO_DATA_PROGRAMSTARTTIME
		         , (unsigned long)psdata->program_start
		         , IDO_DATA_PROCESSID
		         , psdata->pid
		         , IDO_DATA_DAEMONMODE
		         , psdata->daemon_mode
		         , IDO_DATA_LASTCOMMANDCHECK
		         , (unsigned long)psdata->last_command_check
		         , IDO_DATA_LASTLOGROTATION
		         , (unsigned long)psdata->last_log_rotation
		         , IDO_DATA_NOTIFICATIONSENABLED
		         , psdata->notifications_enabled
		         , IDO_DATA_ACTIVESERVICECHECKSENABLED
		         , psdata->active_service_checks_enabled
		         , IDO_DATA_PASSIVESERVICECHECKSENABLED
		         , psdata->passive_service_checks_enabled
		         , IDO_DATA_ACTIVEHOSTCHECKSENABLED
		         , psdata->active_host_checks_enabled
		         , IDO_DATA_PASSIVEHOSTCHECKSENABLED
		         , psdata->passive_host_checks_enabled
		         , IDO_DATA_EVENTHANDLERSENABLED
		         , psdata->event_handlers_enabled
		         , IDO_DATA_FLAPDETECTIONENABLED
		         , psdata->flap_detection_enabled
		         , IDO_DATA_FAILUREPREDICTIONENABLED
		         , psdata->failure_prediction_enabled
		         , IDO_DATA_PROCESSPERFORMANCEDATA
		         , psdata->process_performance_data
		         , IDO_DATA_OBSESSOVERHOSTS
		         , psdata->obsess_over_hosts
		         , IDO_DATA_OBSESSOVERSERVICES
		         , psdata->obsess_over_services
		         , IDO_DATA_MODIFIEDHOSTATTRIBUTES
		         , psdata->modified_host_attributes
		         , IDO_DATA_MODIFIEDSERVICEATTRIBUTES
		         , psdata->modified_service_attributes
		         , IDO_DATA_GLOBALHOSTEVENTHANDLER
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_GLOBALSERVICEEVENTHANDLER
		         , (es[1] == NULL) ? "" : es[1]
			 , IDO_DATA_DISABLED_NOTIFICATIONS_EXPIRE_TIME
			 , psdata->disable_notifications_expire_time
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_HOST_STATUS_DATA:

		hsdata = (nebstruct_host_status_data *)data;

		if ((temp_host = (host *)hsdata->object_ptr) == NULL) {
			ido_dbuf_free(&dbuf);
			return 0;
		}

		es[0] = ido_escape_buffer(temp_host->name);
		es[1] = ido_escape_buffer(temp_host->plugin_output);
		es[2] = ido_escape_buffer(temp_host->long_plugin_output);
		es[3] = ido_escape_buffer(temp_host->perf_data);
		es[4] = ido_escape_buffer(temp_host->event_handler);
		es[5] = ido_escape_buffer(temp_host->host_check_command);
		es[6] = ido_escape_buffer(temp_host->check_period);

		retry_interval = temp_host->retry_interval;

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%.5lf\n%d=%.5lf\n%d=%.5lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%s\n"
		         , IDO_API_HOSTSTATUSDATA
		         , IDO_DATA_TYPE
		         , hsdata->type
		         , IDO_DATA_FLAGS
		         , hsdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , hsdata->attr
		         , IDO_DATA_TIMESTAMP
		         , hsdata->timestamp.tv_sec
		         , hsdata->timestamp.tv_usec
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_OUTPUT
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_LONGOUTPUT
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_PERFDATA
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_CURRENTSTATE
		         , temp_host->current_state
		         , IDO_DATA_HASBEENCHECKED
		         , temp_host->has_been_checked
		         , IDO_DATA_SHOULDBESCHEDULED
		         , temp_host->should_be_scheduled
		         , IDO_DATA_CURRENTCHECKATTEMPT
		         , temp_host->current_attempt
		         , IDO_DATA_MAXCHECKATTEMPTS
		         , temp_host->max_attempts
		         , IDO_DATA_LASTHOSTCHECK
		         , (unsigned long)temp_host->last_check
		         , IDO_DATA_NEXTHOSTCHECK
		         , (unsigned long)temp_host->next_check
		         , IDO_DATA_CHECKTYPE
		         , temp_host->check_type
		         , IDO_DATA_LASTSTATECHANGE
		         , (unsigned long)temp_host->last_state_change
		         , IDO_DATA_LASTHARDSTATECHANGE
		         , (unsigned long)temp_host->last_hard_state_change
		         , IDO_DATA_LASTHARDSTATE
		         , temp_host->last_hard_state
		         , IDO_DATA_LASTTIMEUP
		         , (unsigned long)temp_host->last_time_up
		         , IDO_DATA_LASTTIMEDOWN
		         , (unsigned long)temp_host->last_time_down
		         , IDO_DATA_LASTTIMEUNREACHABLE
		         , (unsigned long)temp_host->last_time_unreachable
		         , IDO_DATA_STATETYPE
		         , temp_host->state_type
		         , IDO_DATA_LASTHOSTNOTIFICATION
		         , (unsigned long)temp_host->last_host_notification
		         , IDO_DATA_NEXTHOSTNOTIFICATION
		         , (unsigned long)temp_host->next_host_notification
		         , IDO_DATA_NOMORENOTIFICATIONS
		         , temp_host->no_more_notifications
		         , IDO_DATA_NOTIFICATIONSENABLED
		         , temp_host->notifications_enabled
		         , IDO_DATA_PROBLEMHASBEENACKNOWLEDGED
		         , temp_host->problem_has_been_acknowledged
		         , IDO_DATA_ACKNOWLEDGEMENTTYPE
		         , temp_host->acknowledgement_type
		         , IDO_DATA_CURRENTNOTIFICATIONNUMBER
		         , temp_host->current_notification_number
		         , IDO_DATA_PASSIVEHOSTCHECKSENABLED
		         , temp_host->accept_passive_host_checks
		         , IDO_DATA_EVENTHANDLERENABLED
		         , temp_host->event_handler_enabled
		         , IDO_DATA_ACTIVEHOSTCHECKSENABLED
		         , temp_host->checks_enabled
		         , IDO_DATA_FLAPDETECTIONENABLED
		         , temp_host->flap_detection_enabled
		         , IDO_DATA_ISFLAPPING
		         , temp_host->is_flapping
		         , IDO_DATA_PERCENTSTATECHANGE
		         , temp_host->percent_state_change
		         , IDO_DATA_LATENCY
		         , temp_host->latency
		         , IDO_DATA_EXECUTIONTIME
		         , temp_host->execution_time
		         , IDO_DATA_SCHEDULEDDOWNTIMEDEPTH
		         , temp_host->scheduled_downtime_depth
		         , IDO_DATA_FAILUREPREDICTIONENABLED
		         , temp_host->failure_prediction_enabled
		         , IDO_DATA_PROCESSPERFORMANCEDATA
		         , temp_host->process_performance_data
		         , IDO_DATA_OBSESSOVERHOST
		         , temp_host->obsess_over_host

		         , IDO_DATA_MODIFIEDHOSTATTRIBUTES
		         , temp_host->modified_attributes
		         , IDO_DATA_EVENTHANDLER
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_CHECKCOMMAND
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_DATA_NORMALCHECKINTERVAL
		         , (double)temp_host->check_interval
		         , IDO_DATA_RETRYCHECKINTERVAL
		         , (double)retry_interval
		         , IDO_DATA_HOSTCHECKPERIOD
		         , (es[6] == NULL) ? "" : es[6]
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		/* dump customvars */
		for (temp_customvar = temp_host->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {

			for (x = 0; x < 2; x++) {
				free(es[x]);
				es[x] = NULL;
			}

			es[0] = ido_escape_buffer(temp_customvar->variable_name);
			es[1] = ido_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
			         , "%d=%s:%d:%s\n"
			         , IDO_DATA_CUSTOMVARIABLE
			         , (es[0] == NULL) ? "" : es[0]
			         , temp_customvar->has_been_modified
			         , (es[1] == NULL) ? "" : es[1]
			        );

			temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);
		}

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_SERVICE_STATUS_DATA:

		ssdata = (nebstruct_service_status_data *)data;

		if ((temp_service = (service *)ssdata->object_ptr) == NULL) {
			ido_dbuf_free(&dbuf);
			return 0;
		}

		es[0] = ido_escape_buffer(temp_service->host_name);
		es[1] = ido_escape_buffer(temp_service->description);
		es[2] = ido_escape_buffer(temp_service->plugin_output);
		es[3] = ido_escape_buffer(temp_service->long_plugin_output);
		es[4] = ido_escape_buffer(temp_service->perf_data);
		es[5] = ido_escape_buffer(temp_service->event_handler);
		es[6] = ido_escape_buffer(temp_service->service_check_command);
		es[7] = ido_escape_buffer(temp_service->check_period);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%.5lf\n%d=%.5lf\n%d=%.5lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%s\n"
		         , IDO_API_SERVICESTATUSDATA
		         , IDO_DATA_TYPE
		         , ssdata->type
		         , IDO_DATA_FLAGS
		         , ssdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , ssdata->attr
		         , IDO_DATA_TIMESTAMP
		         , ssdata->timestamp.tv_sec
		         , ssdata->timestamp.tv_usec
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_OUTPUT
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_LONGOUTPUT
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_PERFDATA
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_CURRENTSTATE
		         , temp_service->current_state
		         , IDO_DATA_HASBEENCHECKED
		         , temp_service->has_been_checked
		         , IDO_DATA_SHOULDBESCHEDULED
		         , temp_service->should_be_scheduled
		         , IDO_DATA_CURRENTCHECKATTEMPT
		         , temp_service->current_attempt
		         , IDO_DATA_MAXCHECKATTEMPTS
		         , temp_service->max_attempts
		         , IDO_DATA_LASTSERVICECHECK
		         , (unsigned long)temp_service->last_check
		         , IDO_DATA_NEXTSERVICECHECK
		         , (unsigned long)temp_service->next_check
		         , IDO_DATA_CHECKTYPE
		         , temp_service->check_type
		         , IDO_DATA_LASTSTATECHANGE
		         , (unsigned long)temp_service->last_state_change
		         , IDO_DATA_LASTHARDSTATECHANGE
		         , (unsigned long)temp_service->last_hard_state_change
		         , IDO_DATA_LASTHARDSTATE
		         , temp_service->last_hard_state
		         , IDO_DATA_LASTTIMEOK
		         , (unsigned long)temp_service->last_time_ok
		         , IDO_DATA_LASTTIMEWARNING
		         , (unsigned long)temp_service->last_time_warning
		         , IDO_DATA_LASTTIMEUNKNOWN
		         , (unsigned long)temp_service->last_time_unknown
		         , IDO_DATA_LASTTIMECRITICAL
		         , (unsigned long)temp_service->last_time_critical
		         , IDO_DATA_STATETYPE
		         , temp_service->state_type
		         , IDO_DATA_LASTSERVICENOTIFICATION
		         , (unsigned long)temp_service->last_notification
		         , IDO_DATA_NEXTSERVICENOTIFICATION
		         , (unsigned long)temp_service->next_notification
		         , IDO_DATA_NOMORENOTIFICATIONS
		         , temp_service->no_more_notifications
		         , IDO_DATA_NOTIFICATIONSENABLED
		         , temp_service->notifications_enabled
		         , IDO_DATA_PROBLEMHASBEENACKNOWLEDGED
		         , temp_service->problem_has_been_acknowledged
		         , IDO_DATA_ACKNOWLEDGEMENTTYPE
		         , temp_service->acknowledgement_type
		         , IDO_DATA_CURRENTNOTIFICATIONNUMBER
		         , temp_service->current_notification_number
		         , IDO_DATA_PASSIVESERVICECHECKSENABLED
		         , temp_service->accept_passive_service_checks
		         , IDO_DATA_EVENTHANDLERENABLED
		         , temp_service->event_handler_enabled
		         , IDO_DATA_ACTIVESERVICECHECKSENABLED
		         , temp_service->checks_enabled
		         , IDO_DATA_FLAPDETECTIONENABLED
		         , temp_service->flap_detection_enabled
		         , IDO_DATA_ISFLAPPING
		         , temp_service->is_flapping
		         , IDO_DATA_PERCENTSTATECHANGE
		         , temp_service->percent_state_change
		         , IDO_DATA_LATENCY
		         , temp_service->latency
		         , IDO_DATA_EXECUTIONTIME
		         , temp_service->execution_time
		         , IDO_DATA_SCHEDULEDDOWNTIMEDEPTH
		         , temp_service->scheduled_downtime_depth
		         , IDO_DATA_FAILUREPREDICTIONENABLED
		         , temp_service->failure_prediction_enabled
		         , IDO_DATA_PROCESSPERFORMANCEDATA
		         , temp_service->process_performance_data
		         , IDO_DATA_OBSESSOVERSERVICE
		         , temp_service->obsess_over_service

		         , IDO_DATA_MODIFIEDSERVICEATTRIBUTES
		         , temp_service->modified_attributes
		         , IDO_DATA_EVENTHANDLER
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_DATA_CHECKCOMMAND
		         , (es[6] == NULL) ? "" : es[6]
		         , IDO_DATA_NORMALCHECKINTERVAL
		         , (double)temp_service->check_interval
		         , IDO_DATA_RETRYCHECKINTERVAL
		         , (double)temp_service->retry_interval
		         , IDO_DATA_SERVICECHECKPERIOD
		         , (es[7] == NULL) ? "" : es[7]
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		/* dump customvars */
		for (temp_customvar = temp_service->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {

			for (x = 0; x < 2; x++) {
				free(es[x]);
				es[x] = NULL;
			}

			es[0] = ido_escape_buffer(temp_customvar->variable_name);
			es[1] = ido_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
			         , "%d=%s:%d:%s\n"
			         , IDO_DATA_CUSTOMVARIABLE
			         , (es[0] == NULL) ? "" : es[0]
			         , temp_customvar->has_been_modified
			         , (es[1] == NULL) ? "" : es[1]
			        );

			temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);
		}

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_CONTACT_STATUS_DATA:

		csdata = (nebstruct_contact_status_data *)data;

		if ((temp_contact = (contact *)csdata->object_ptr) == NULL) {
			ido_dbuf_free(&dbuf);
			return 0;
		}

		es[0] = ido_escape_buffer(temp_contact->name);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n"
		         , IDO_API_CONTACTSTATUSDATA
		         , IDO_DATA_TYPE
		         , csdata->type
		         , IDO_DATA_FLAGS
		         , csdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , csdata->attr
		         , IDO_DATA_TIMESTAMP
		         , csdata->timestamp.tv_sec
		         , csdata->timestamp.tv_usec
		         , IDO_DATA_CONTACTNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_HOSTNOTIFICATIONSENABLED
		         , temp_contact->host_notifications_enabled
		         , IDO_DATA_SERVICENOTIFICATIONSENABLED
		         , temp_contact->service_notifications_enabled
		         , IDO_DATA_LASTHOSTNOTIFICATION
		         , temp_contact->last_host_notification
		         , IDO_DATA_LASTSERVICENOTIFICATION
		         , temp_contact->last_service_notification

		         , IDO_DATA_MODIFIEDCONTACTATTRIBUTES
		         , temp_contact->modified_attributes
		         , IDO_DATA_MODIFIEDHOSTATTRIBUTES
		         , temp_contact->modified_host_attributes
		         , IDO_DATA_MODIFIEDSERVICEATTRIBUTES
		         , temp_contact->modified_service_attributes
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		/* dump customvars */
		for (temp_customvar = temp_contact->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {

			for (x = 0; x < 2; x++) {
				free(es[x]);
				es[x] = NULL;
			}

			es[0] = ido_escape_buffer(temp_customvar->variable_name);
			es[1] = ido_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
			         , "%d=%s:%d:%s\n"
			         , IDO_DATA_CUSTOMVARIABLE
			         , (es[0] == NULL) ? "" : es[0]
			         , temp_customvar->has_been_modified
			         , (es[1] == NULL) ? "" : es[1]
			        );

			temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);
		}

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_ADAPTIVE_PROGRAM_DATA:

		apdata = (nebstruct_adaptive_program_data *)data;

		es[0] = ido_escape_buffer(global_host_event_handler);
		es[1] = ido_escape_buffer(global_service_event_handler);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_ADAPTIVEPROGRAMDATA
		         , IDO_DATA_TYPE, apdata->type
		         , IDO_DATA_FLAGS
		         , apdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , apdata->attr
		         , IDO_DATA_TIMESTAMP
		         , apdata->timestamp.tv_sec
		         , apdata->timestamp.tv_usec
		         , IDO_DATA_COMMANDTYPE
		         , apdata->command_type
		         , IDO_DATA_MODIFIEDHOSTATTRIBUTE
		         , apdata->modified_host_attribute
		         , IDO_DATA_MODIFIEDHOSTATTRIBUTES
		         , apdata->modified_host_attributes
		         , IDO_DATA_MODIFIEDSERVICEATTRIBUTE
		         , apdata->modified_service_attribute
		         , IDO_DATA_MODIFIEDSERVICEATTRIBUTES
		         , apdata->modified_service_attributes
		         , IDO_DATA_GLOBALHOSTEVENTHANDLER
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_GLOBALSERVICEEVENTHANDLER
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_ADAPTIVE_HOST_DATA:

		ahdata = (nebstruct_adaptive_host_data *)data;

		if ((temp_host = (host *)ahdata->object_ptr) == NULL) {
			ido_dbuf_free(&dbuf);
			return 0;
		}

		retry_interval = temp_host->retry_interval;

		es[0] = ido_escape_buffer(temp_host->name);
		es[1] = ido_escape_buffer(temp_host->event_handler);
		es[2] = ido_escape_buffer(temp_host->host_check_command);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%lu\n%d=%s\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%d\n%d\n\n"
		         , IDO_API_ADAPTIVEHOSTDATA
		         , IDO_DATA_TYPE
		         , ahdata->type
		         , IDO_DATA_FLAGS
		         , ahdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , ahdata->attr
		         , IDO_DATA_TIMESTAMP
		         , ahdata->timestamp.tv_sec
		         , ahdata->timestamp.tv_usec
		         , IDO_DATA_COMMANDTYPE
		         , ahdata->command_type
		         , IDO_DATA_MODIFIEDHOSTATTRIBUTE
		         , ahdata->modified_attribute
		         , IDO_DATA_MODIFIEDHOSTATTRIBUTES
		         , ahdata->modified_attributes
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_EVENTHANDLER
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_CHECKCOMMAND
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_NORMALCHECKINTERVAL
		         , (double)temp_host->check_interval
		         , IDO_DATA_RETRYCHECKINTERVAL
		         , retry_interval
		         , IDO_DATA_MAXCHECKATTEMPTS
		         , temp_host->max_attempts
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_ADAPTIVE_SERVICE_DATA:

		asdata = (nebstruct_adaptive_service_data *)data;

		if ((temp_service = (service *)asdata->object_ptr) == NULL) {
			ido_dbuf_free(&dbuf);
			return 0;
		}

		es[0] = ido_escape_buffer(temp_service->host_name);
		es[1] = ido_escape_buffer(temp_service->description);
		es[2] = ido_escape_buffer(temp_service->event_handler);
		es[3] = ido_escape_buffer(temp_service->service_check_command);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%lu\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%d\n%d\n\n"
		         , IDO_API_ADAPTIVESERVICEDATA
		         , IDO_DATA_TYPE
		         , asdata->type
		         , IDO_DATA_FLAGS
		         , asdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , asdata->attr
		         , IDO_DATA_TIMESTAMP
		         , asdata->timestamp.tv_sec
		         , asdata->timestamp.tv_usec
		         , IDO_DATA_COMMANDTYPE
		         , asdata->command_type
		         , IDO_DATA_MODIFIEDSERVICEATTRIBUTE
		         , asdata->modified_attribute
		         , IDO_DATA_MODIFIEDSERVICEATTRIBUTES
		         , asdata->modified_attributes
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_EVENTHANDLER
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_CHECKCOMMAND
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_NORMALCHECKINTERVAL
		         , (double)temp_service->check_interval
		         , IDO_DATA_RETRYCHECKINTERVAL
		         , (double)temp_service->retry_interval
		         , IDO_DATA_MAXCHECKATTEMPTS
		         , temp_service->max_attempts
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_ADAPTIVE_CONTACT_DATA:

		acdata = (nebstruct_adaptive_contact_data *)data;

		if ((temp_contact = (contact *)acdata->object_ptr) == NULL) {
			ido_dbuf_free(&dbuf);
			return 0;
		}

		es[0] = ido_escape_buffer(temp_contact->name);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%s\n%d=%d\n%d=%d\n%d\n\n"
		         , IDO_API_ADAPTIVECONTACTDATA
		         , IDO_DATA_TYPE
		         , acdata->type
		         , IDO_DATA_FLAGS
		         , acdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , acdata->attr
		         , IDO_DATA_TIMESTAMP
		         , acdata->timestamp.tv_sec
		         , acdata->timestamp.tv_usec
		         , IDO_DATA_COMMANDTYPE
		         , acdata->command_type
		         , IDO_DATA_MODIFIEDCONTACTATTRIBUTE
		         , acdata->modified_attribute
		         , IDO_DATA_MODIFIEDCONTACTATTRIBUTES
		         , acdata->modified_attributes
		         , IDO_DATA_MODIFIEDHOSTATTRIBUTE
		         , acdata->modified_host_attribute
		         , IDO_DATA_MODIFIEDHOSTATTRIBUTES
		         , acdata->modified_host_attributes
		         , IDO_DATA_MODIFIEDSERVICEATTRIBUTE
		         , acdata->modified_service_attribute
		         , IDO_DATA_MODIFIEDSERVICEATTRIBUTES
		         , acdata->modified_service_attributes
		         , IDO_DATA_CONTACTNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_HOSTNOTIFICATIONSENABLED
		         , temp_contact->host_notifications_enabled
		         , IDO_DATA_SERVICENOTIFICATIONSENABLED
		         , temp_contact->service_notifications_enabled
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_EXTERNAL_COMMAND_DATA:

		ecdata = (nebstruct_external_command_data *)data;

		es[0] = ido_escape_buffer(ecdata->command_string);
		es[1] = ido_escape_buffer(ecdata->command_args);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_EXTERNALCOMMANDDATA
		         , IDO_DATA_TYPE
		         , ecdata->type
		         , IDO_DATA_FLAGS
		         , ecdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , ecdata->attr
		         , IDO_DATA_TIMESTAMP
		         , ecdata->timestamp.tv_sec
		         , ecdata->timestamp.tv_usec
		         , IDO_DATA_COMMANDTYPE
		         , ecdata->command_type
		         , IDO_DATA_ENTRYTIME
		         , (unsigned long)ecdata->entry_time
		         , IDO_DATA_COMMANDSTRING
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_COMMANDARGS
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_AGGREGATED_STATUS_DATA:

		agsdata = (nebstruct_aggregated_status_data *)data;

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d\n\n"
		         , IDO_API_AGGREGATEDSTATUSDATA
		         , IDO_DATA_TYPE
		         , agsdata->type
		         , IDO_DATA_FLAGS
		         , agsdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , agsdata->attr
		         , IDO_DATA_TIMESTAMP
		         , agsdata->timestamp.tv_sec
		         , agsdata->timestamp.tv_usec
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_RETENTION_DATA:

		rdata = (nebstruct_retention_data *)data;

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d\n\n"
		         , IDO_API_RETENTIONDATA
		         , IDO_DATA_TYPE
		         , rdata->type
		         , IDO_DATA_FLAGS
		         , rdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , rdata->attr
		         , IDO_DATA_TIMESTAMP
		         , rdata->timestamp.tv_sec
		         , rdata->timestamp.tv_usec
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_CONTACT_NOTIFICATION_DATA:

		cnotdata = (nebstruct_contact_notification_data *)data;

		es[0] = ido_escape_buffer(cnotdata->host_name);
		es[1] = ido_escape_buffer(cnotdata->service_description);
		es[2] = ido_escape_buffer(cnotdata->output);
		/* Preparing if contact notifications will have long_output in the future */
		es[3] = ido_escape_buffer(cnotdata->output);
		es[4] = ido_escape_buffer(cnotdata->ack_author);
		es[5] = ido_escape_buffer(cnotdata->ack_data);
		es[6] = ido_escape_buffer(cnotdata->contact_name);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_CONTACTNOTIFICATIONDATA
		         , IDO_DATA_TYPE
		         , cnotdata->type
		         , IDO_DATA_FLAGS
		         , cnotdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , cnotdata->attr
		         , IDO_DATA_TIMESTAMP
		         , cnotdata->timestamp.tv_sec
		         , cnotdata->timestamp.tv_usec
		         , IDO_DATA_NOTIFICATIONTYPE
		         , cnotdata->notification_type
		         , IDO_DATA_STARTTIME
		         , cnotdata->start_time.tv_sec
		         , cnotdata->start_time.tv_usec
		         , IDO_DATA_ENDTIME
		         , cnotdata->end_time.tv_sec
		         , cnotdata->end_time.tv_usec
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_CONTACTNAME
		         , (es[6] == NULL) ? "" : es[6]
		         , IDO_DATA_NOTIFICATIONREASON
		         , cnotdata->reason_type
		         , IDO_DATA_STATE
		         , cnotdata->state
		         , IDO_DATA_OUTPUT
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_LONGOUTPUT
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_ACKAUTHOR
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_ACKDATA
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA:

		cnotmdata = (nebstruct_contact_notification_method_data *)data;

		es[0] = ido_escape_buffer(cnotmdata->host_name);
		es[1] = ido_escape_buffer(cnotmdata->service_description);
		es[2] = ido_escape_buffer(cnotmdata->output);
		/* Preparing if contact notifications method will have long_output in the future */
		es[3] = ido_escape_buffer(cnotmdata->output);
		es[4] = ido_escape_buffer(cnotmdata->ack_author);
		es[5] = ido_escape_buffer(cnotmdata->ack_data);
		es[6] = ido_escape_buffer(cnotmdata->contact_name);
		es[7] = ido_escape_buffer(cnotmdata->command_name);
		es[8] = ido_escape_buffer(cnotmdata->command_args);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_CONTACTNOTIFICATIONMETHODDATA
		         , IDO_DATA_TYPE
		         , cnotmdata->type
		         , IDO_DATA_FLAGS
		         , cnotmdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , cnotmdata->attr
		         , IDO_DATA_TIMESTAMP
		         , cnotmdata->timestamp.tv_sec
		         , cnotmdata->timestamp.tv_usec
		         , IDO_DATA_NOTIFICATIONTYPE
		         , cnotmdata->notification_type
		         , IDO_DATA_STARTTIME
		         , cnotmdata->start_time.tv_sec
		         , cnotmdata->start_time.tv_usec
		         , IDO_DATA_ENDTIME
		         , cnotmdata->end_time.tv_sec
		         , cnotmdata->end_time.tv_usec
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_CONTACTNAME
		         , (es[6] == NULL) ? "" : es[6]
		         , IDO_DATA_COMMANDNAME
		         , (es[7] == NULL) ? "" : es[7]
		         , IDO_DATA_COMMANDARGS
		         , (es[8] == NULL) ? "" : es[8]
		         , IDO_DATA_NOTIFICATIONREASON
		         , cnotmdata->reason_type
		         , IDO_DATA_STATE
		         , cnotmdata->state
		         , IDO_DATA_OUTPUT
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_LONGOUTPUT
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_ACKAUTHOR
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_ACKDATA
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_ACKNOWLEDGEMENT_DATA:

		ackdata = (nebstruct_acknowledgement_data *)data;

		es[0] = ido_escape_buffer(ackdata->host_name);
		es[1] = ido_escape_buffer(ackdata->service_description);
		es[2] = ido_escape_buffer(ackdata->author_name);
		es[3] = ido_escape_buffer(ackdata->comment_data);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%ld\n%d\n\n"
		         , IDO_API_ACKNOWLEDGEMENTDATA
		         , IDO_DATA_TYPE
		         , ackdata->type
		         , IDO_DATA_FLAGS
		         , ackdata->flags
		         , IDO_DATA_ATTRIBUTES
		         , ackdata->attr
		         , IDO_DATA_TIMESTAMP
		         , ackdata->timestamp.tv_sec
		         , ackdata->timestamp.tv_usec
		         , IDO_DATA_ACKNOWLEDGEMENTTYPE
		         , ackdata->acknowledgement_type
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_AUTHORNAME
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_COMMENT
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_STATE
		         , ackdata->state
		         , IDO_DATA_STICKY
		         , ackdata->is_sticky
		         , IDO_DATA_PERSISTENT
		         , ackdata->persistent_comment
		         , IDO_DATA_NOTIFYCONTACTS
		         , ackdata->notify_contacts
			 , IDO_DATA_END_TIME
			 , ackdata->end_time
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	case NEBCALLBACK_STATE_CHANGE_DATA:

		schangedata = (nebstruct_statechange_data *)data;

		/* get the last state info */
		if (schangedata->service_description == NULL) {
			if ((temp_host = (host *)schangedata->object_ptr) == NULL) {
				ido_dbuf_free(&dbuf);
				return 0;
			}
			last_state = temp_host->last_state;
			last_hard_state = temp_host->last_hard_state;
		} else {
			if ((temp_service = (service *)schangedata->object_ptr) == NULL) {
				ido_dbuf_free(&dbuf);
				return 0;
			}
			last_state = temp_service->last_state;
			last_hard_state = temp_service->last_hard_state;
		}

		es[0] = ido_escape_buffer(schangedata->host_name);
		es[1] = ido_escape_buffer(schangedata->service_description);
		es[2] = ido_escape_buffer(schangedata->output);
		es[3] = ido_escape_buffer(schangedata->long_output);

		snprintf(temp_buffer, IDOMOD_MAX_BUFLEN - 1
		         , "\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_STATECHANGEDATA
		         , IDO_DATA_TYPE
		         , schangedata->type
		         , IDO_DATA_FLAGS
		         , schangedata->flags
		         , IDO_DATA_ATTRIBUTES
		         , schangedata->attr
		         , IDO_DATA_TIMESTAMP
		         , schangedata->timestamp.tv_sec
		         , schangedata->timestamp.tv_usec
		         , IDO_DATA_STATECHANGETYPE
		         , schangedata->statechange_type
		         , IDO_DATA_HOST
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_STATECHANGE
		         , TRUE
		         , IDO_DATA_STATE
		         , schangedata->state
		         , IDO_DATA_STATETYPE
		         , schangedata->state_type
		         , IDO_DATA_CURRENTCHECKATTEMPT
		         , schangedata->current_attempt
		         , IDO_DATA_MAXCHECKATTEMPTS
		         , schangedata->max_attempts
		         , IDO_DATA_LASTSTATE
		         , last_state
		         , IDO_DATA_LASTHARDSTATE
		         , last_hard_state
		         , IDO_DATA_OUTPUT
		         , es[2]
		         , IDO_DATA_LONGOUTPUT
		         , es[3]
		         , IDO_API_ENDDATA
		        );

		temp_buffer[IDOMOD_MAX_BUFLEN-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		break;

	default:
		ido_dbuf_free(&dbuf);
		return 0;
		break;
	}

	/* free escaped buffers */
	for (x = 0; x < 8; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/* write data to sink */
	if (write_to_sink == IDO_TRUE)
		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

	/* free dynamic buffer */
	ido_dbuf_free(&dbuf);



	/* POST PROCESSING... */

	switch (event_type) {

	case NEBCALLBACK_PROCESS_DATA:

		procdata = (nebstruct_process_data *)data;

		/* process has passed pre-launch config verification, so dump original config */
		if (procdata->type == NEBTYPE_PROCESS_START) {
			idomod_write_config_files();
			idomod_write_config(IDOMOD_CONFIG_DUMP_ORIGINAL);
		}

		/* process is starting the event loop, so dump runtime vars */
		if (procdata->type == NEBTYPE_PROCESS_EVENTLOOPSTART) {
			idomod_write_runtime_variables();
		}

		break;

	case NEBCALLBACK_RETENTION_DATA:

		rdata = (nebstruct_retention_data *)data;

		/* retained config was just read, so dump it */
		if (rdata->type == NEBTYPE_RETENTIONDATA_ENDLOAD)
			idomod_write_config(IDOMOD_CONFIG_DUMP_RETAINED);

		break;

	default:
		break;
	}

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_broker_data() end\n");

	return 0;
}



/****************************************************************************/
/* CONFIG OUTPUT FUNCTIONS                                                  */
/****************************************************************************/

/* dumps all configuration data to sink */
int idomod_write_config(int config_type) {
	char temp_buffer[IDOMOD_MAX_BUFLEN];
	struct timeval now;
	int result;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_config() start\n");

	if (!(idomod_config_output_options & config_type))
		return IDO_OK;

	gettimeofday(&now, NULL);

	/* record start of config dump */
	snprintf(temp_buffer, sizeof(temp_buffer) - 1
	         , "\n\n%d:\n%d=%s\n%d=%ld.%ld\n%d\n\n"
	         , IDO_API_STARTCONFIGDUMP
	         , IDO_DATA_CONFIGDUMPTYPE
	         , (config_type == IDOMOD_CONFIG_DUMP_ORIGINAL) ? IDO_API_CONFIGDUMP_ORIGINAL : IDO_API_CONFIGDUMP_RETAINED
	         , IDO_DATA_TIMESTAMP
	         , now.tv_sec
	         , now.tv_usec
	         , IDO_API_ENDDATA
	        );
	temp_buffer[sizeof(temp_buffer)-1] = '\x0';
	idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);

	/* dump object config info */
	result = idomod_write_object_config(config_type);
	if (result != IDO_OK)
		return result;

	/* record end of config dump */
	snprintf(temp_buffer, sizeof(temp_buffer) - 1
	         , "\n\n%d:\n%d=%ld.%ld\n%d\n\n"
	         , IDO_API_ENDCONFIGDUMP
	         , IDO_DATA_TIMESTAMP
	         , now.tv_sec
	         , now.tv_usec
	         , IDO_API_ENDDATA
	        );
	temp_buffer[sizeof(temp_buffer)-1] = '\x0';
	idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_config() end\n");

	return result;
}


#define OBJECTCONFIG_ES_ITEMS 16

/* dumps object configuration data to sink */
int idomod_write_object_config(int config_type) {
	char temp_buffer[IDOMOD_MAX_BUFLEN];
	ido_dbuf dbuf;
	struct timeval now;
	int x = 0;
	char *es[OBJECTCONFIG_ES_ITEMS];
	command *temp_command = NULL;
	timeperiod *temp_timeperiod = NULL;
	timerange *temp_timerange = NULL;
	contact *temp_contact = NULL;
	commandsmember *temp_commandsmember = NULL;
	contactgroup *temp_contactgroup = NULL;
	host *temp_host = NULL;
	hostsmember *temp_hostsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	hostgroup *temp_hostgroup = NULL;
	service *temp_service = NULL;
	servicegroup *temp_servicegroup = NULL;
	hostescalation *temp_hostescalation = NULL;
	serviceescalation *temp_serviceescalation = NULL;
	hostdependency *temp_hostdependency = NULL;
	servicedependency *temp_servicedependency = NULL;
	int have_2d_coords = FALSE;
	int x_2d = 0;
	int y_2d = 0;
	int have_3d_coords = FALSE;
	double x_3d = 0.0;
	double y_3d = 0.0;
	double z_3d = 0.0;
	double first_notification_delay = 0.0;
	double retry_interval = 0.0;
	int notify_on_host_downtime = 0;
	int notify_on_service_downtime = 0;
	int host_notifications_enabled = 0;
	int service_notifications_enabled = 0;
	int can_submit_commands = 0;
	int flap_detection_on_up = 0;
	int flap_detection_on_down = 0;
	int flap_detection_on_unreachable = 0;
	int flap_detection_on_ok = 0;
	int flap_detection_on_warning = 0;
	int flap_detection_on_unknown = 0;
	int flap_detection_on_critical = 0;
	customvariablesmember *temp_customvar = NULL;
	contactsmember *temp_contactsmember = NULL;
	servicesmember *temp_servicesmember = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_object_config() start\n");

	if (!(idomod_process_options & IDOMOD_PROCESS_OBJECT_CONFIG_DATA))
		return IDO_OK;

	if (!(idomod_config_output_options & config_type))
		return IDO_OK;

	/* get current time */
	gettimeofday(&now, NULL);

	/* initialize dynamic buffer (2KB chunk size) */
	ido_dbuf_init(&dbuf, 2048);

	/* initialize buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++)
		es[x] = NULL;

	/****** dump command config ******/
	for (temp_command = command_list; temp_command != NULL; temp_command = temp_command->next) {

		es[0] = ido_escape_buffer(temp_command->name);
		es[1] = ido_escape_buffer(temp_command->command_line);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d\n\n"
		         , IDO_API_COMMANDDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_COMMANDNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_COMMANDLINE
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_API_ENDDATA
		        );

		/* write data to sink */
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);
	}

	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump timeperiod config ******/
	for (temp_timeperiod = timeperiod_list; temp_timeperiod != NULL; temp_timeperiod = temp_timeperiod->next) {

		es[0] = ido_escape_buffer(temp_timeperiod->name);
		es[1] = ido_escape_buffer(temp_timeperiod->alias);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n"
		         , IDO_API_TIMEPERIODDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_TIMEPERIODNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_TIMEPERIODALIAS
		         , (es[1] == NULL) ? "" : es[1]
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		/* dump timeranges for each day */
		for (x = 0; x < 7; x++) {
			for (temp_timerange = temp_timeperiod->days[x]; temp_timerange != NULL; temp_timerange = temp_timerange->next) {

				snprintf(temp_buffer, sizeof(temp_buffer) - 1
				         , "%d=%d:%lu-%lu\n"
				         , IDO_DATA_TIMERANGE
				         , x
				         , temp_timerange->range_start
				         , temp_timerange->range_end
				        );
				temp_buffer[sizeof(temp_buffer)-1] = '\x0';
				ido_dbuf_strcat(&dbuf, temp_buffer);
			}
		}

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump contact config ******/
	for (temp_contact = contact_list; temp_contact != NULL; temp_contact = temp_contact->next) {

		es[0] = ido_escape_buffer(temp_contact->name);
		es[1] = ido_escape_buffer(temp_contact->alias);
		es[2] = ido_escape_buffer(temp_contact->email);
		es[3] = ido_escape_buffer(temp_contact->pager);
		es[4] = ido_escape_buffer(temp_contact->host_notification_period);
		es[5] = ido_escape_buffer(temp_contact->service_notification_period);

		notify_on_service_downtime = temp_contact->notify_on_service_downtime;
		notify_on_host_downtime = temp_contact->notify_on_host_downtime;
		host_notifications_enabled = temp_contact->host_notifications_enabled;
		service_notifications_enabled = temp_contact->service_notifications_enabled;
		can_submit_commands = temp_contact->can_submit_commands;

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n"
		         , IDO_API_CONTACTDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_CONTACTNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_CONTACTALIAS
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_EMAILADDRESS
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_PAGERADDRESS
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_HOSTNOTIFICATIONPERIOD
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_SERVICENOTIFICATIONPERIOD
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_DATA_SERVICENOTIFICATIONSENABLED
		         , service_notifications_enabled
		         , IDO_DATA_HOSTNOTIFICATIONSENABLED
		         , host_notifications_enabled
		         , IDO_DATA_CANSUBMITCOMMANDS
		         , can_submit_commands
		         , IDO_DATA_NOTIFYSERVICEUNKNOWN
		         , temp_contact->notify_on_service_unknown
		         , IDO_DATA_NOTIFYSERVICEWARNING
		         , temp_contact->notify_on_service_warning
		         , IDO_DATA_NOTIFYSERVICECRITICAL
		         , temp_contact->notify_on_service_critical
		         , IDO_DATA_NOTIFYSERVICERECOVERY
		         , temp_contact->notify_on_service_recovery
		         , IDO_DATA_NOTIFYSERVICEFLAPPING
		         , temp_contact->notify_on_service_flapping
		         , IDO_DATA_NOTIFYSERVICEDOWNTIME
		         , notify_on_service_downtime
		         , IDO_DATA_NOTIFYHOSTDOWN
		         , temp_contact->notify_on_host_down
		         , IDO_DATA_NOTIFYHOSTUNREACHABLE
		         , temp_contact->notify_on_host_unreachable
		         , IDO_DATA_NOTIFYHOSTRECOVERY
		         , temp_contact->notify_on_host_recovery
		         , IDO_DATA_NOTIFYHOSTFLAPPING
		         , temp_contact->notify_on_host_flapping
		         , IDO_DATA_NOTIFYHOSTDOWNTIME
		         , notify_on_host_downtime
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		free(es[0]);
		es[0] = NULL;

		/* dump addresses for each contact */
		for (x = 0; x < MAX_CONTACT_ADDRESSES; x++) {

			es[0] = ido_escape_buffer(temp_contact->address[x]);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%d:%s\n"
			         , IDO_DATA_CONTACTADDRESS
			         , x + 1
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		/* dump host notification commands for each contact */
		for (temp_commandsmember = temp_contact->host_notification_commands; temp_commandsmember != NULL; temp_commandsmember = temp_commandsmember->next) {

			es[0] = ido_escape_buffer(temp_commandsmember->command);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_HOSTNOTIFICATIONCOMMAND
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		/* dump service notification commands for each contact */
		for (temp_commandsmember = temp_contact->service_notification_commands; temp_commandsmember != NULL; temp_commandsmember = temp_commandsmember->next) {

			es[0] = ido_escape_buffer(temp_commandsmember->command);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_SERVICENOTIFICATIONCOMMAND
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		/* dump customvars */
		for (temp_customvar = temp_contact->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {

			es[0] = ido_escape_buffer(temp_customvar->variable_name);
			es[1] = ido_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s:%d:%s\n"
			         , IDO_DATA_CUSTOMVARIABLE
			         , (es[0] == NULL) ? "" : es[0]
			         , temp_customvar->has_been_modified
			         , (es[1] == NULL) ? "" : es[1]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			for (x = 0; x < 2; x++) {
				free(es[x]);
				es[x] = NULL;
			}
		}

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump contactgroup config ******/
	for (temp_contactgroup = contactgroup_list; temp_contactgroup != NULL; temp_contactgroup = temp_contactgroup->next) {

		es[0] = ido_escape_buffer(temp_contactgroup->group_name);
		es[1] = ido_escape_buffer(temp_contactgroup->alias);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n"
		         , IDO_API_CONTACTGROUPDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_CONTACTGROUPNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_CONTACTGROUPALIAS
		         , (es[1] == NULL) ? "" : es[1]
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		free(es[0]);
		es[0] = NULL;

		/* dump members for each contactgroup */
		for (temp_contactsmember = temp_contactgroup->members; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {

			es[0] = ido_escape_buffer(temp_contactsmember->contact_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_CONTACTGROUPMEMBER
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump host config ******/
	for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {

		es[0] = ido_escape_buffer(temp_host->name);
		es[1] = ido_escape_buffer(temp_host->alias);
		es[2] = ido_escape_buffer(temp_host->address);
		es[3] = ido_escape_buffer(temp_host->host_check_command);
		es[4] = ido_escape_buffer(temp_host->event_handler);
		es[5] = ido_escape_buffer(temp_host->notification_period);
		es[6] = ido_escape_buffer(temp_host->check_period);
		es[7] = ido_escape_buffer(temp_host->failure_prediction_options);

		es[7] = ido_escape_buffer(temp_host->notes);
		es[8] = ido_escape_buffer(temp_host->notes_url);
		es[9] = ido_escape_buffer(temp_host->action_url);
		es[10] = ido_escape_buffer(temp_host->icon_image);
		es[11] = ido_escape_buffer(temp_host->icon_image_alt);
		es[12] = ido_escape_buffer(temp_host->vrml_image);
		es[13] = ido_escape_buffer(temp_host->statusmap_image);
		have_2d_coords = temp_host->have_2d_coords;
		x_2d = temp_host->x_2d;
		y_2d = temp_host->y_2d;
		have_3d_coords = temp_host->have_3d_coords;
		x_3d = temp_host->x_3d;
		y_3d = temp_host->y_3d;
		z_3d = temp_host->z_3d;

		first_notification_delay = temp_host->first_notification_delay;
		retry_interval = temp_host->retry_interval;
		notify_on_host_downtime = temp_host->notify_on_downtime;
		flap_detection_on_up = temp_host->flap_detection_on_up;
		flap_detection_on_down = temp_host->flap_detection_on_down;
		flap_detection_on_unreachable = temp_host->flap_detection_on_unreachable;
		es[14] = ido_escape_buffer(temp_host->display_name);
		es[15] = ido_escape_buffer(temp_host->address6);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%d\n%d=%lf\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lf\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lf\n%d=%lf\n%d=%lf\n"
		         , IDO_API_HOSTDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_HOSTNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_DISPLAYNAME
		         , (es[14] == NULL) ? "" : es[14]
		         , IDO_DATA_HOSTALIAS
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_HOSTADDRESS
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_HOSTADDRESS6
		         , (es[15] == NULL) ? "" : es[15]
		         , IDO_DATA_HOSTCHECKCOMMAND
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_HOSTEVENTHANDLER
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_HOSTNOTIFICATIONPERIOD
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_DATA_HOSTCHECKPERIOD
		         , (es[6] == NULL) ? "" : es[6]
		         , IDO_DATA_HOSTFAILUREPREDICTIONOPTIONS
		         , (es[7] == NULL) ? "" : es[7]
		         , IDO_DATA_HOSTCHECKINTERVAL
		         , (double)temp_host->check_interval
		         , IDO_DATA_HOSTRETRYINTERVAL
		         , (double)retry_interval
		         , IDO_DATA_HOSTMAXCHECKATTEMPTS
		         , temp_host->max_attempts
		         , IDO_DATA_FIRSTNOTIFICATIONDELAY
		         , first_notification_delay
		         , IDO_DATA_HOSTNOTIFICATIONINTERVAL
		         , (double)temp_host->notification_interval
		         , IDO_DATA_NOTIFYHOSTDOWN
		         , temp_host->notify_on_down
		         , IDO_DATA_NOTIFYHOSTUNREACHABLE
		         , temp_host->notify_on_unreachable
		         , IDO_DATA_NOTIFYHOSTRECOVERY
		         , temp_host->notify_on_recovery
		         , IDO_DATA_NOTIFYHOSTFLAPPING
		         , temp_host->notify_on_flapping
		         , IDO_DATA_NOTIFYHOSTDOWNTIME
		         , notify_on_host_downtime
		         , IDO_DATA_HOSTFLAPDETECTIONENABLED
		         , temp_host->flap_detection_enabled
		         , IDO_DATA_FLAPDETECTIONONUP
		         , flap_detection_on_up
		         , IDO_DATA_FLAPDETECTIONOIDOWN
		         , flap_detection_on_down
		         , IDO_DATA_FLAPDETECTIONONUNREACHABLE
		         , flap_detection_on_unreachable
		         , IDO_DATA_LOWHOSTFLAPTHRESHOLD
		         , temp_host->low_flap_threshold
		         , IDO_DATA_HIGHHOSTFLAPTHRESHOLD
		         , temp_host->high_flap_threshold
		         , IDO_DATA_STALKHOSTONUP
		         , temp_host->stalk_on_up
		         , IDO_DATA_STALKHOSTOIDOWN
		         , temp_host->stalk_on_down
		         , IDO_DATA_STALKHOSTONUNREACHABLE
		         , temp_host->stalk_on_unreachable
		         , IDO_DATA_HOSTFRESHNESSCHECKSENABLED
		         , temp_host->check_freshness
		         , IDO_DATA_HOSTFRESHNESSTHRESHOLD
		         , temp_host->freshness_threshold
		         , IDO_DATA_PROCESSHOSTPERFORMANCEDATA
		         , temp_host->process_performance_data
		         , IDO_DATA_ACTIVEHOSTCHECKSENABLED
		         , temp_host->checks_enabled
		         , IDO_DATA_PASSIVEHOSTCHECKSENABLED
		         , temp_host->accept_passive_host_checks
		         , IDO_DATA_HOSTEVENTHANDLERENABLED
		         , temp_host->event_handler_enabled
		         , IDO_DATA_RETAINHOSTSTATUSINFORMATION
		         , temp_host->retain_status_information
		         , IDO_DATA_RETAINHOSTNONSTATUSINFORMATION
		         , temp_host->retain_nonstatus_information
		         , IDO_DATA_HOSTNOTIFICATIONSENABLED
		         , temp_host->notifications_enabled
		         , IDO_DATA_HOSTFAILUREPREDICTIONENABLED
		         , temp_host->failure_prediction_enabled
		         , IDO_DATA_OBSESSOVERHOST
		         , temp_host->obsess_over_host
		         , IDO_DATA_NOTES
		         , (es[7] == NULL) ? "" : es[7]
		         , IDO_DATA_NOTESURL
		         , (es[8] == NULL) ? "" : es[8]
		         , IDO_DATA_ACTIONURL
		         , (es[9] == NULL) ? "" : es[9]
		         , IDO_DATA_ICONIMAGE
		         , (es[10] == NULL) ? "" : es[10]
		         , IDO_DATA_ICONIMAGEALT
		         , (es[11] == NULL) ? "" : es[11]
		         , IDO_DATA_VRMLIMAGE
		         , (es[12] == NULL) ? "" : es[12]
		         , IDO_DATA_STATUSMAPIMAGE
		         , (es[13] == NULL) ? "" : es[13]
		         , IDO_DATA_HAVE2DCOORDS
		         , have_2d_coords
		         , IDO_DATA_X2D
		         , x_2d
		         , IDO_DATA_Y2D
		         , y_2d
		         , IDO_DATA_HAVE3DCOORDS
		         , have_3d_coords
		         , IDO_DATA_X3D
		         , x_3d
		         , IDO_DATA_Y3D
		         , y_3d
		         , IDO_DATA_Z3D
		         , z_3d
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		free(es[0]);
		es[0] = NULL;

		/* dump parent hosts */
		for (temp_hostsmember = temp_host->parent_hosts; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {

			es[0] = ido_escape_buffer(temp_hostsmember->host_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_PARENTHOST
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		/* dump contactgroups */
		for (temp_contactgroupsmember = temp_host->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {

			es[0] = ido_escape_buffer(temp_contactgroupsmember->group_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_CONTACTGROUP
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		/* dump individual contacts */
		for (temp_contactsmember = temp_host->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {

			es[0] = ido_escape_buffer(temp_contactsmember->contact_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_CONTACT
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}


		/* dump customvars */
		for (temp_customvar = temp_host->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {

			es[0] = ido_escape_buffer(temp_customvar->variable_name);
			es[1] = ido_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s:%d:%s\n"
			         , IDO_DATA_CUSTOMVARIABLE
			         , (es[0] == NULL) ? "" : es[0]
			         , temp_customvar->has_been_modified
			         , (es[1] == NULL) ? "" : es[1]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			for (x = 0; x < 2; x++) {
				free(es[x]);
				es[x] = NULL;
			}
		}

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump hostgroup config ******/
	for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {

		es[0] = ido_escape_buffer(temp_hostgroup->group_name);
		es[1] = ido_escape_buffer(temp_hostgroup->alias);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n"
		         , IDO_API_HOSTGROUPDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_HOSTGROUPNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_HOSTGROUPALIAS
		         , (es[1] == NULL) ? "" : es[1]
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		free(es[0]);
		es[0] = NULL;

		/* dump members for each hostgroup */
		for (temp_hostsmember = temp_hostgroup->members; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {

			es[0] = ido_escape_buffer(temp_hostsmember->host_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_HOSTGROUPMEMBER
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump service config ******/
	for (temp_service = service_list; temp_service != NULL; temp_service = temp_service->next) {

		es[0] = ido_escape_buffer(temp_service->host_name);
		es[1] = ido_escape_buffer(temp_service->description);
		es[2] = ido_escape_buffer(temp_service->service_check_command);
		es[3] = ido_escape_buffer(temp_service->event_handler);
		es[4] = ido_escape_buffer(temp_service->notification_period);
		es[5] = ido_escape_buffer(temp_service->check_period);
		es[6] = ido_escape_buffer(temp_service->failure_prediction_options);
		es[7] = ido_escape_buffer(temp_service->notes);
		es[8] = ido_escape_buffer(temp_service->notes_url);
		es[9] = ido_escape_buffer(temp_service->action_url);
		es[10] = ido_escape_buffer(temp_service->icon_image);
		es[11] = ido_escape_buffer(temp_service->icon_image_alt);

		first_notification_delay = temp_service->first_notification_delay;
		notify_on_service_downtime = temp_service->notify_on_downtime;
		flap_detection_on_ok = temp_service->flap_detection_on_ok;
		flap_detection_on_warning = temp_service->flap_detection_on_warning;
		flap_detection_on_unknown = temp_service->flap_detection_on_unknown;
		flap_detection_on_critical = temp_service->flap_detection_on_critical;
		es[12] = ido_escape_buffer(temp_service->display_name);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%d\n%d=%lf\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lf\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n"
		         , IDO_API_SERVICEDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_HOSTNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_DISPLAYNAME
		         , (es[12] == NULL) ? "" : es[12]
		         , IDO_DATA_SERVICEDESCRIPTION
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_SERVICECHECKCOMMAND
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_SERVICEEVENTHANDLER
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_SERVICENOTIFICATIONPERIOD
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_SERVICECHECKPERIOD
		         , (es[5] == NULL) ? "" : es[5]
		         , IDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS
		         , (es[6] == NULL) ? "" : es[6]
		         , IDO_DATA_SERVICECHECKINTERVAL
		         , (double)temp_service->check_interval
		         , IDO_DATA_SERVICERETRYINTERVAL
		         , (double)temp_service->retry_interval
		         , IDO_DATA_MAXSERVICECHECKATTEMPTS
		         , temp_service->max_attempts
		         , IDO_DATA_FIRSTNOTIFICATIONDELAY
		         , first_notification_delay
		         , IDO_DATA_SERVICENOTIFICATIONINTERVAL
		         , (double)temp_service->notification_interval
		         , IDO_DATA_NOTIFYSERVICEUNKNOWN
		         , temp_service->notify_on_unknown
		         , IDO_DATA_NOTIFYSERVICEWARNING
		         , temp_service->notify_on_warning
		         , IDO_DATA_NOTIFYSERVICECRITICAL
		         , temp_service->notify_on_critical
		         , IDO_DATA_NOTIFYSERVICERECOVERY
		         , temp_service->notify_on_recovery
		         , IDO_DATA_NOTIFYSERVICEFLAPPING
		         , temp_service->notify_on_flapping
		         , IDO_DATA_NOTIFYSERVICEDOWNTIME
		         , notify_on_service_downtime
		         , IDO_DATA_STALKSERVICEONOK
		         , temp_service->stalk_on_ok
		         , IDO_DATA_STALKSERVICEONWARNING
		         , temp_service->stalk_on_warning
		         , IDO_DATA_STALKSERVICEONUNKNOWN
		         , temp_service->stalk_on_unknown
		         , IDO_DATA_STALKSERVICEONCRITICAL
		         , temp_service->stalk_on_critical
		         , IDO_DATA_SERVICEISVOLATILE
		         , temp_service->is_volatile
		         , IDO_DATA_SERVICEFLAPDETECTIONENABLED
		         , temp_service->flap_detection_enabled
		         , IDO_DATA_FLAPDETECTIONONOK
		         , flap_detection_on_ok
		         , IDO_DATA_FLAPDETECTIONONWARNING
		         , flap_detection_on_warning
		         , IDO_DATA_FLAPDETECTIONONUNKNOWN
		         , flap_detection_on_unknown
		         , IDO_DATA_FLAPDETECTIONONCRITICAL
		         , flap_detection_on_critical
		         , IDO_DATA_LOWSERVICEFLAPTHRESHOLD
		         , temp_service->low_flap_threshold
		         , IDO_DATA_HIGHSERVICEFLAPTHRESHOLD
		         , temp_service->high_flap_threshold
		         , IDO_DATA_PROCESSSERVICEPERFORMANCEDATA
		         , temp_service->process_performance_data
		         , IDO_DATA_SERVICEFRESHNESSCHECKSENABLED
		         , temp_service->check_freshness
		         , IDO_DATA_SERVICEFRESHNESSTHRESHOLD
		         , temp_service->freshness_threshold
		         , IDO_DATA_PASSIVESERVICECHECKSENABLED
		         , temp_service->accept_passive_service_checks
		         , IDO_DATA_SERVICEEVENTHANDLERENABLED
		         , temp_service->event_handler_enabled
		         , IDO_DATA_ACTIVESERVICECHECKSENABLED
		         , temp_service->checks_enabled
		         , IDO_DATA_RETAINSERVICESTATUSINFORMATION
		         , temp_service->retain_status_information
		         , IDO_DATA_RETAINSERVICENONSTATUSINFORMATION
		         , temp_service->retain_nonstatus_information
		         , IDO_DATA_SERVICENOTIFICATIONSENABLED
		         , temp_service->notifications_enabled
		         , IDO_DATA_OBSESSOVERSERVICE
		         , temp_service->obsess_over_service
		         , IDO_DATA_SERVICEFAILUREPREDICTIONENABLED
		         , temp_service->failure_prediction_enabled
		         , IDO_DATA_NOTES
		         , (es[7] == NULL) ? "" : es[7]
		         , IDO_DATA_NOTESURL
		         , (es[8] == NULL) ? "" : es[8]
		         , IDO_DATA_ACTIONURL
		         , (es[9] == NULL) ? "" : es[9]
		         , IDO_DATA_ICONIMAGE
		         , (es[10] == NULL) ? "" : es[10]
		         , IDO_DATA_ICONIMAGEALT
		         , (es[11] == NULL) ? "" : es[11]
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		free(es[0]);
		es[0] = NULL;

		/* dump contactgroups */
		for (temp_contactgroupsmember = temp_service->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {

			es[0] = ido_escape_buffer(temp_contactgroupsmember->group_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_CONTACTGROUP
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		/* dump individual contacts  */
		for (temp_contactsmember = temp_service->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {

			es[0] = ido_escape_buffer(temp_contactsmember->contact_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_CONTACT
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		/* dump customvars */
		for (temp_customvar = temp_service->custom_variables; temp_customvar != NULL; temp_customvar = temp_customvar->next) {

			es[0] = ido_escape_buffer(temp_customvar->variable_name);
			es[1] = ido_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s:%d:%s\n"
			         , IDO_DATA_CUSTOMVARIABLE
			         , (es[0] == NULL) ? "" : es[0]
			         , temp_customvar->has_been_modified
			         , (es[1] == NULL) ? "" : es[1]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			for (x = 0; x < 2; x++) {
				free(es[x]);
				es[x] = NULL;
			}
		}

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump servicegroup config ******/
	for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {

		es[0] = ido_escape_buffer(temp_servicegroup->group_name);
		es[1] = ido_escape_buffer(temp_servicegroup->alias);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n"
		         , IDO_API_SERVICEGROUPDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_SERVICEGROUPNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICEGROUPALIAS
		         , (es[1] == NULL) ? "" : es[1]
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		free(es[0]);
		free(es[1]);
		es[0] = NULL;
		es[1] = NULL;

		/* dump members for each servicegroup */
		for (temp_servicesmember = temp_servicegroup->members; temp_servicesmember != NULL; temp_servicesmember = temp_servicesmember->next) {

			es[0] = ido_escape_buffer(temp_servicesmember->host_name);
			es[1] = ido_escape_buffer(temp_servicesmember->service_description);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s;%s\n"
			         , IDO_DATA_SERVICEGROUPMEMBER
			         , (es[0] == NULL) ? "" : es[0]
			         , (es[1] == NULL) ? "" : es[1]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			free(es[1]);
			es[0] = NULL;
			es[1] = NULL;
		}

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump host escalation config ******/
	for (temp_hostescalation = hostescalation_list; temp_hostescalation != NULL; temp_hostescalation = temp_hostescalation->next) {

		es[0] = ido_escape_buffer(temp_hostescalation->host_name);
		es[1] = ido_escape_buffer(temp_hostescalation->escalation_period);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n"
		         , IDO_API_HOSTESCALATIONDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_HOSTNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_ESCALATIONPERIOD
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_FIRSTNOTIFICATION
		         , temp_hostescalation->first_notification
		         , IDO_DATA_LASTNOTIFICATION
		         , temp_hostescalation->last_notification
		         , IDO_DATA_NOTIFICATIONINTERVAL
		         , (double)temp_hostescalation->notification_interval
		         , IDO_DATA_ESCALATEONRECOVERY
		         , temp_hostescalation->escalate_on_recovery
		         , IDO_DATA_ESCALATEOIDOWN
		         , temp_hostescalation->escalate_on_down
		         , IDO_DATA_ESCALATEONUNREACHABLE
		         , temp_hostescalation->escalate_on_unreachable
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		free(es[0]);
		es[0] = NULL;

		/* dump contactgroups */
		for (temp_contactgroupsmember = temp_hostescalation->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {

			es[0] = ido_escape_buffer(temp_contactgroupsmember->group_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_CONTACTGROUP
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		/* dump individual contacts */
		for (temp_contactsmember = temp_hostescalation->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {

			es[0] = ido_escape_buffer(temp_contactsmember->contact_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_CONTACT
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump service escalation config ******/
	for (temp_serviceescalation = serviceescalation_list; temp_serviceescalation != NULL; temp_serviceescalation = temp_serviceescalation->next) {

		es[0] = ido_escape_buffer(temp_serviceescalation->host_name);
		es[1] = ido_escape_buffer(temp_serviceescalation->description);
		es[2] = ido_escape_buffer(temp_serviceescalation->escalation_period);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n"
		         , IDO_API_SERVICEESCALATIONDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_HOSTNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICEDESCRIPTION
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_ESCALATIONPERIOD
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_FIRSTNOTIFICATION
		         , temp_serviceescalation->first_notification
		         , IDO_DATA_LASTNOTIFICATION
		         , temp_serviceescalation->last_notification
		         , IDO_DATA_NOTIFICATIONINTERVAL
		         , (double)temp_serviceescalation->notification_interval
		         , IDO_DATA_ESCALATEONRECOVERY
		         , temp_serviceescalation->escalate_on_recovery
		         , IDO_DATA_ESCALATEONWARNING
		         , temp_serviceescalation->escalate_on_warning
		         , IDO_DATA_ESCALATEONUNKNOWN
		         , temp_serviceescalation->escalate_on_unknown
		         , IDO_DATA_ESCALATEONCRITICAL
		         , temp_serviceescalation->escalate_on_critical
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		free(es[0]);
		es[0] = NULL;

		/* dump contactgroups */
		for (temp_contactgroupsmember = temp_serviceescalation->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {

			es[0] = ido_escape_buffer(temp_contactgroupsmember->group_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_CONTACTGROUP
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		/* dump individual contacts */
		for (temp_contactsmember = temp_serviceescalation->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {

			es[0] = ido_escape_buffer(temp_contactsmember->contact_name);

			snprintf(temp_buffer, sizeof(temp_buffer) - 1
			         , "%d=%s\n"
			         , IDO_DATA_CONTACT
			         , (es[0] == NULL) ? "" : es[0]
			        );
			temp_buffer[sizeof(temp_buffer)-1] = '\x0';
			ido_dbuf_strcat(&dbuf, temp_buffer);

			free(es[0]);
			es[0] = NULL;
		}

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump host dependency config ******/
	for (temp_hostdependency = hostdependency_list; temp_hostdependency != NULL; temp_hostdependency = temp_hostdependency->next) {

		es[0] = ido_escape_buffer(temp_hostdependency->host_name);
		es[1] = ido_escape_buffer(temp_hostdependency->dependent_host_name);

		es[2] = ido_escape_buffer(temp_hostdependency->dependency_period);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n"
		         , IDO_API_HOSTDEPENDENCYDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_HOSTNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_DEPENDENTHOSTNAME
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_DEPENDENCYTYPE
		         , temp_hostdependency->dependency_type
		         , IDO_DATA_INHERITSPARENT
		         , temp_hostdependency->inherits_parent
		         , IDO_DATA_DEPENDENCYPERIOD
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_FAILONUP
		         , temp_hostdependency->fail_on_up
		         , IDO_DATA_FAILOIDOWN
		         , temp_hostdependency->fail_on_down
		         , IDO_DATA_FAILONUNREACHABLE
		         , temp_hostdependency->fail_on_unreachable
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	/****** dump service dependency config ******/
	for (temp_servicedependency = servicedependency_list; temp_servicedependency != NULL; temp_servicedependency = temp_servicedependency->next) {

		es[0] = ido_escape_buffer(temp_servicedependency->host_name);
		es[1] = ido_escape_buffer(temp_servicedependency->service_description);
		es[2] = ido_escape_buffer(temp_servicedependency->dependent_host_name);
		es[3] = ido_escape_buffer(temp_servicedependency->dependent_service_description);

		es[4] = ido_escape_buffer(temp_servicedependency->dependency_period);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n"
		         , IDO_API_SERVICEDEPENDENCYDEFINITION
		         , IDO_DATA_TIMESTAMP
		         , now.tv_sec
		         , now.tv_usec
		         , IDO_DATA_HOSTNAME
		         , (es[0] == NULL) ? "" : es[0]
		         , IDO_DATA_SERVICEDESCRIPTION
		         , (es[1] == NULL) ? "" : es[1]
		         , IDO_DATA_DEPENDENTHOSTNAME
		         , (es[2] == NULL) ? "" : es[2]
		         , IDO_DATA_DEPENDENTSERVICEDESCRIPTION
		         , (es[3] == NULL) ? "" : es[3]
		         , IDO_DATA_DEPENDENCYTYPE
		         , temp_servicedependency->dependency_type
		         , IDO_DATA_INHERITSPARENT
		         , temp_servicedependency->inherits_parent
		         , IDO_DATA_DEPENDENCYPERIOD
		         , (es[4] == NULL) ? "" : es[4]
		         , IDO_DATA_FAILONOK
		         , temp_servicedependency->fail_on_ok
		         , IDO_DATA_FAILONWARNING
		         , temp_servicedependency->fail_on_warning
		         , IDO_DATA_FAILONUNKNOWN
		         , temp_servicedependency->fail_on_unknown
		         , IDO_DATA_FAILONCRITICAL
		         , temp_servicedependency->fail_on_critical
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		snprintf(temp_buffer, sizeof(temp_buffer) - 1
		         , "%d\n\n"
		         , IDO_API_ENDDATA
		        );
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ido_dbuf_strcat(&dbuf, temp_buffer);

		idomod_write_to_sink(dbuf.buf, IDO_TRUE, IDO_TRUE);

		ido_dbuf_free(&dbuf);
	}


	/* free buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) {
		free(es[x]);
		es[x] = NULL;
	}

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_object_config() end\n");

	return IDO_OK;
}



/* dumps config files to data sink */
int idomod_write_config_files(void) {
	int result = IDO_OK;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_config_files() start\n");

	if ((result = idomod_write_main_config_file()) == IDO_ERROR)
		return IDO_ERROR;

	if ((result = idomod_write_resource_config_files()) == IDO_ERROR)
		return IDO_ERROR;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_config_files() end\n");

	return result;
}



/* dumps main config file data to sink */
int idomod_write_main_config_file(void) {
	char fbuf[IDOMOD_MAX_BUFLEN];
	char *temp_buffer;
	struct timeval now;
	FILE *fp;
	char *var = NULL;
	char *val = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_main_config_file() start\n");

	/* get current time */
	gettimeofday(&now, NULL);

	if (asprintf(&temp_buffer
	             , "\n%d:\n%d=%ld.%ld\n%d=%s\n"
	             , IDO_API_MAINCONFIGFILEVARIABLES
	             , IDO_DATA_TIMESTAMP
	             , now.tv_sec
	             , now.tv_usec
	             , IDO_DATA_CONFIGFILENAME
	             , config_file
	            ) == -1)
		temp_buffer = NULL;

	idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);
	free(temp_buffer);
	temp_buffer = NULL;

	/* write each var/val pair from config file */
	if ((fp = fopen(config_file, "r"))) {

		while ((fgets(fbuf, sizeof(fbuf), fp))) {

			/* skip blank lines */
			if (fbuf[0] == '\x0' || fbuf[0] == '\n' || fbuf[0] == '\r')
				continue;

			strip(fbuf);

			/* skip comments */
			if (fbuf[0] == '#' || fbuf[0] == ';')
				continue;

			if ((var = strtok(fbuf, "=")) == NULL)
				continue;
			val = strtok(NULL, "\n");

			if (asprintf(&temp_buffer
			             , "%d=%s=%s\n"
			             , IDO_DATA_CONFIGFILEVARIABLE
			             , var
			             , (val == NULL) ? "" : val
			            ) == -1)
				temp_buffer = NULL;

			idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);
			free(temp_buffer);
			temp_buffer = NULL;
		}

		fclose(fp);
	}

	if (asprintf(&temp_buffer
	             , "%d\n\n"
	             , IDO_API_ENDDATA
	            ) == -1)
		temp_buffer = NULL;

	idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);
	free(temp_buffer);
	temp_buffer = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_main_config_file() end\n");

	return IDO_OK;
}



/* dumps all resource config files to sink */
int idomod_write_resource_config_files(void) {

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_resource_config_files() start\n");

	/* TODO */
	/* loop through main config file to find all resource config files, and then process them */
	/* this should probably NOT be done, as the resource file is supposed to remain private... */

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_resource_config_files() end\n");

	return IDO_OK;
}



/* dumps a single resource config file to sink */
int idomod_write_resource_config_file(char *filename) {

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_resource_config_file() start\n");

	/* TODO */
	/* loop through main config file to find all resource config files, and then process them */
	/* this should probably NOT be done, as the resource file is supposed to remain private... */

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_resource_config_file() end\n");

	return IDO_OK;
}



/* dumps runtime variables to sink */
int idomod_write_runtime_variables(void) {
	char *temp_buffer = NULL;
	struct timeval now;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_runtime_variables() start\n");

	/* get current time */
	gettimeofday(&now, NULL);

	if (asprintf(&temp_buffer
	             , "\n%d:\n%d=%ld.%ld\n"
	             , IDO_API_RUNTIMEVARIABLES
	             , IDO_DATA_TIMESTAMP
	             , now.tv_sec
	             , now.tv_usec
	            ) == -1)
		temp_buffer = NULL;

	idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);
	free(temp_buffer);
	temp_buffer = NULL;

	/* write out main config file name */
	if (asprintf(&temp_buffer
	             , "%d=%s=%s\n"
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "config_file"
	             , config_file
	            ) == -1)
		temp_buffer = NULL;

	idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);
	free(temp_buffer);
	temp_buffer = NULL;

	/* write out vars determined after startup */
	if (asprintf(&temp_buffer
	             , "%d=%s=%d\n%d=%s=%d\n%d=%s=%d\n%d=%s=%d\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lu\n%d=%s=%lu\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%d\n%d=%s=%d\n%d=%s=%d\n"
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "total_services"
	             , scheduling_info.total_services
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "total_scheduled_services"
	             , scheduling_info.total_scheduled_services
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "total_hosts"
	             , scheduling_info.total_hosts
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "total_scheduled_hosts"
	             , scheduling_info.total_scheduled_hosts
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "average_services_per_host"
	             , scheduling_info.average_services_per_host
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "average_scheduled_services_per_host"
	             , scheduling_info.average_scheduled_services_per_host
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "service_check_interval_total"
	             , scheduling_info.service_check_interval_total
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "host_check_interval_total"
	             , scheduling_info.host_check_interval_total
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "average_service_check_interval"
	             , scheduling_info.average_service_check_interval
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "average_host_check_interval"
	             , scheduling_info.average_host_check_interval
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "average_service_inter_check_delay"
	             , scheduling_info.average_service_inter_check_delay
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "average_host_inter_check_delay"
	             , scheduling_info.average_host_inter_check_delay
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "service_inter_check_delay"
	             , scheduling_info.service_inter_check_delay
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "host_inter_check_delay"
	             , scheduling_info.host_inter_check_delay
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "service_interleave_factor"
	             , scheduling_info.service_interleave_factor
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "max_service_check_spread"
	             , scheduling_info.max_service_check_spread
	             , IDO_DATA_RUNTIMEVARIABLE
	             , "max_host_check_spread"
	             , scheduling_info.max_host_check_spread
	            ) == -1)
		temp_buffer = NULL;

	idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);
	free(temp_buffer);
	temp_buffer = NULL;

	if (asprintf(&temp_buffer
	             , "%d\n\n"
	             , IDO_API_ENDDATA
	            ) == -1)
		temp_buffer = NULL;

	idomod_write_to_sink(temp_buffer, IDO_TRUE, IDO_TRUE);
	free(temp_buffer);
	temp_buffer = NULL;

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_write_runtime_variables() end\n");

	return IDO_OK;
}


/****************************************************************************/
/* LOGGING ROUTINES                                                         */
/****************************************************************************/

/* opens the debug log for writing */
int idomod_open_debug_log(void) {

	/* don't do anything if we're not debugging */
	if (idomod_debug_level == IDOMOD_DEBUGL_NONE)
		return IDO_OK;

	if ((idomod_debug_file_fp = fopen(idomod_debug_file, "a+")) == NULL) {
		syslog(LOG_ERR, "Warning: Could not open debug file '%s' - '%s'", idomod_debug_file, strerror(errno));
		return IDO_ERROR;
	}

	idomod_log_debug_info(IDOMOD_DEBUGL_PROCESSINFO, 2, "idomod_open_debug_log()\n");

	return IDO_OK;
}


/* closes the debug log */
int idomod_close_debug_log(void) {

	if (idomod_debug_file_fp != NULL)
		fclose(idomod_debug_file_fp);

	idomod_debug_file_fp = NULL;

	return IDO_OK;
}


/* write to the debug log */
int idomod_log_debug_info(int level, int verbosity, const char *fmt, ...) {
	va_list ap;
	char *temp_path = NULL;
	struct timeval current_time;

	if (!(idomod_debug_level == IDOMOD_DEBUGL_ALL || (level & idomod_debug_level)))
		return IDO_OK;

	if (verbosity > idomod_debug_verbosity)
		return IDO_OK;

	if (idomod_debug_file_fp == NULL)
		return IDO_ERROR;

	/* write the timestamp */
	gettimeofday(&current_time, NULL);
	fprintf(idomod_debug_file_fp, "[%lu.%06lu] [%03d.%d] [pid=%lu] ", current_time.tv_sec, current_time.tv_usec, level, verbosity, (unsigned long)getpid());

	/* write the data */
	va_start(ap, fmt);
	vfprintf(idomod_debug_file_fp, fmt, ap);
	va_end(ap);

	/* flush, so we don't have problems tailing or when fork()ing */
	fflush(idomod_debug_file_fp);

	/* if file has grown beyond max, rotate it */
	if ((unsigned long)ftell(idomod_debug_file_fp) > idomod_max_debug_file_size && idomod_max_debug_file_size > 0L) {

		/* close the file */
		idomod_close_debug_log();

		/* rotate the log file */
		if (asprintf(&temp_path, "%s.old", idomod_debug_file) == -1)
			temp_path = NULL;

		if (temp_path) {

			/* unlink the old debug file */
			unlink(temp_path);

			/* rotate the debug file */
			my_rename(idomod_debug_file, temp_path);

			/* free memory */
			my_free(temp_path);
		}

		/* open a new file */
		idomod_open_debug_log();
	}

	return IDO_OK;
}

