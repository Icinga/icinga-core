/************************************************************************
 *
 * IDOMOD.H - IDO NEB Module Include File
 * Copyright (c) 2005-2006 Ethan Galstad
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 ************************************************************************/

#ifndef _NDBXT_IDOMOD_H
#define _NDBXT_IDOMOD_H

/************** misc definitions *************/

/* this is needed for access to daemon's internal data */
#define NSCORE 1

#define IDOMOD_MAX_BUFLEN   49152

/************** structures *******************/

typedef struct idomod_sink_buffer_struct{
	char **buffer;
	unsigned long size;
	unsigned long head;
	unsigned long tail;
	unsigned long items;
	unsigned long maxitems;
	unsigned long overflow;
        }idomod_sink_buffer;


/************* types of process data ***********/

#define IDOMOD_PROCESS_PROCESS_DATA                   1
#define IDOMOD_PROCESS_TIMED_EVENT_DATA               2
#define IDOMOD_PROCESS_LOG_DATA                       4
#define IDOMOD_PROCESS_SYSTEM_COMMAND_DATA            8
#define IDOMOD_PROCESS_EVENT_HANDLER_DATA             16
#define IDOMOD_PROCESS_NOTIFICATION_DATA              32
#define IDOMOD_PROCESS_SERVICE_CHECK_DATA             64
#define IDOMOD_PROCESS_HOST_CHECK_DATA                128
#define IDOMOD_PROCESS_COMMENT_DATA                   256
#define IDOMOD_PROCESS_DOWNTIME_DATA                  512
#define IDOMOD_PROCESS_FLAPPING_DATA                  1024
#define IDOMOD_PROCESS_PROGRAM_STATUS_DATA            2048
#define IDOMOD_PROCESS_HOST_STATUS_DATA               4096
#define IDOMOD_PROCESS_SERVICE_STATUS_DATA            8192
#define IDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA          16384
#define IDOMOD_PROCESS_ADAPTIVE_HOST_DATA             32768
#define IDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA          65536
#define IDOMOD_PROCESS_EXTERNAL_COMMAND_DATA          131072
#define IDOMOD_PROCESS_OBJECT_CONFIG_DATA             262144
#define IDOMOD_PROCESS_MAIN_CONFIG_DATA               524288
#define IDOMOD_PROCESS_AGGREGATED_STATUS_DATA         1048576
#define IDOMOD_PROCESS_RETENTION_DATA                 2097152
#define IDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA           4194304
#define IDOMOD_PROCESS_STATECHANGE_DATA               8388608
#define IDOMOD_PROCESS_CONTACT_STATUS_DATA            16777216
#define IDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA          33554432

#define IDOMOD_PROCESS_EVERYTHING                     67108863


/************* types of config dump ************/

#define IDOMOD_CONFIG_DUMP_NONE                       0
#define IDOMOD_CONFIG_DUMP_ORIGINAL                   1
#define IDOMOD_CONFIG_DUMP_RETAINED                   2
#define IDOMOD_CONFIG_DUMP_ALL                        3


/************* debugging levels ****************/

#define IDOMOD_DEBUGL_ALL                      -1
#define IDOMOD_DEBUGL_NONE                     0
#define IDOMOD_DEBUGL_PROCESSINFO              1
#define IDOMOD_DEBUGL_SQL                      2

#define IDOMOD_DEBUGV_BASIC                    0
#define IDOMOD_DEBUGV_MORE                     1
#define IDOMOD_DEBUGV_MOST                     2


/************* functions ***********************/

int nebmodule_init(int,char *,void *);
int nebmodule_deinit(int,int);

int idomod_init(void);
int idomod_deinit(void);

int idomod_check_icinga_object_version(void);

int idomod_write_to_logs(char *,int);

int idomod_process_module_args(char *);
int idomod_process_config_var(char *);
int idomod_process_config_file(char *);

int idomod_open_sink(void);
int idomod_close_sink(void);
int idomod_write_to_sink(char *,int,int);
int idomod_rotate_sink_file(void *);
int idomod_hello_sink(int,int);
int idomod_goodbye_sink(void);

int idomod_sink_buffer_init(idomod_sink_buffer *sbuf,unsigned long);
int idomod_sink_buffer_deinit(idomod_sink_buffer *sbuf);
int idomod_sink_buffer_push(idomod_sink_buffer *sbuf,char *);
char *idomod_sink_buffer_peek(idomod_sink_buffer *sbuf);
char *idomod_sink_buffer_pop(idomod_sink_buffer *sbuf);
int idomod_sink_buffer_items(idomod_sink_buffer *sbuf);
unsigned long idomod_sink_buffer_get_overflow(idomod_sink_buffer *sbuf);
int idomod_sink_buffer_set_overflow(idomod_sink_buffer *sbuf,unsigned long);

int idomod_load_unprocessed_data(char *);
int idomod_save_unprocessed_data(char *);

int idomod_register_callbacks(void);
int idomod_deregister_callbacks(void);

int idomod_broker_data(int,void *);

int idomod_write_config(int);
int idomod_write_object_config(int);

int idomod_write_config_files(void);
int idomod_write_main_config_file(void);
int idomod_write_resource_config_files(void);
int idomod_write_resource_config_file(char *);

int idomod_write_runtime_variables(void);

int idomod_log_debug_info(int , int , const char *, ...);

#endif
