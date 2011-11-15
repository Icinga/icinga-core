/***************************************************************
 * LOGGING.C - logging routines for IDO2DB daemon
 *
 * Copyright (c) 2011 Icinga Development Team (http://www.icinga.org)
 *
 **************************************************************/

/* include our project's header files */
#include "../../../include/config.h"
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ido2db.h"
#include "../include/logging.h"

/* Icinga header files */
#include "../../../include/icinga.h"
#include "../../../include/broker.h"
#include "../../../include/comments.h"

extern int errno;

extern char *ido2db_debug_file;
extern int ido2db_debug_level;
extern int ido2db_debug_verbosity;
extern FILE *ido2db_debug_file_fp;
extern unsigned long ido2db_max_debug_file_size;

extern int ido2db_debug_readable_timestamp;

/****************************************************************************/
/* LOGGING ROUTINES                                                         */
/****************************************************************************/

/* opens the debug log for writing */
int ido2db_open_debug_log(void) {

        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_open_debug_log() start\n");

        /* don't do anything if we're not debugging */
        if (ido2db_debug_level == IDO2DB_DEBUGL_NONE)
                return IDO_OK;

        if ((ido2db_debug_file_fp = fopen(ido2db_debug_file, "a+")) == NULL) {
                syslog(LOG_ERR, "Warning: Could not open debug file '%s' - '%s'", ido2db_debug_file, strerror(errno));
                return IDO_ERROR;
        }

        ido2db_log_debug_info(IDO2DB_DEBUGL_PROCESSINFO, 2, "ido2db_open_debug_log() end\n");

        return IDO_OK;
}


/* closes the debug log */
int ido2db_close_debug_log(void) {

        if (ido2db_debug_file_fp != NULL)
                fclose(ido2db_debug_file_fp);

        ido2db_debug_file_fp = NULL;

        return IDO_OK;
}

/* write to the debug log */
int ido2db_log_debug_info(int level, int verbosity, const char *fmt, ...) {
        va_list ap;
        char *temp_path = NULL;
        time_t t;
        struct tm *tm;
        char temp_time[80];
        struct timeval current_time;

        if (!(ido2db_debug_level == IDO2DB_DEBUGL_ALL || (level & ido2db_debug_level)))
                return IDO_OK;

        if (verbosity > ido2db_debug_verbosity)
                return IDO_OK;

        if (ido2db_debug_file_fp == NULL)
                return IDO_ERROR;

        /* write the timestamp */
        gettimeofday(&current_time, NULL);

        time(&t);
        tm=localtime(&t);
        strftime(temp_time, 80, "%c", tm);

	if (ido2db_debug_readable_timestamp)
	        fprintf(ido2db_debug_file_fp, "%s .%06lu [%03d.%d] [pid=%lu] [tid=%lld] ", temp_time, current_time.tv_usec, level, verbosity, (unsigned long)getpid(), (unsigned long int)pthread_self());
	else
        	fprintf(ido2db_debug_file_fp, "[%lu.%06lu] [%03d.%d] [pid=%lu] [tid=%ld] ", current_time.tv_sec, current_time.tv_usec, level, verbosity, (unsigned long)getpid(), (unsigned long int)pthread_self());

        /* write the data */
        va_start(ap, fmt);
        vfprintf(ido2db_debug_file_fp, fmt, ap);
        va_end(ap);

        /* flush, so we don't have problems tailing or when fork()ing */
        fflush(ido2db_debug_file_fp);

        /* if file has grown beyond max, rotate it */
        if ((unsigned long)ftell(ido2db_debug_file_fp) > ido2db_max_debug_file_size && ido2db_max_debug_file_size > 0L) {

                /* close the file */
                ido2db_close_debug_log();

                /* rotate the log file */
                if (asprintf(&temp_path, "%s.old", ido2db_debug_file) == -1)
                        temp_path = NULL;

                if (temp_path) {

                        /* unlink the old debug file */
                        unlink(temp_path);

                        /* rotate the debug file */
                        my_rename(ido2db_debug_file, temp_path);

                        /* free memory */
                        my_free(temp_path);
                }

                /* open a new file */
                ido2db_open_debug_log();
        }

        return IDO_OK;
}






