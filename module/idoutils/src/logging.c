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

static pthread_mutex_t ido2db_debug_fp_lock;

/*
 * since we don't want child processes to hang indefinitely
 * in case they inherit a locked lock, we use soft-locking
 * here, which basically tries to acquire the lock for a
 * short while and then gives up, returning -1 to signal
 * the error
 */
static inline int soft_lock(pthread_mutex_t *lock) {
        int i;

        for (i = 0; i < 5; i++) {
                if (!pthread_mutex_trylock(lock)) {
                        /* success */
                        return 0;
                }

                if (errno == EDEADLK) {
                        /* we already have the lock */
                        return 0;
                }

                /* sleep briefly */
                usleep(30);
        }

        return -1; /* we failed to get the lock. Nothing to do */
}


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
        struct tm *tm, tm_s;
        char temp_time[80];
        struct timeval current_time;
        unsigned long tid;
        unsigned long pid;

        if (!(ido2db_debug_level == IDO2DB_DEBUGL_ALL || (level & ido2db_debug_level)))
                return IDO_OK;

        if (verbosity > ido2db_debug_verbosity)
                return IDO_OK;

        if (ido2db_debug_file_fp == NULL)
                return IDO_ERROR;

        /*
         * lock it so concurrent threads don't stomp on each other's
         * writings. We maintain the lock until we've (optionally)
         * renamed the file.
         * If soft_lock() fails we return early.
         */
        if (soft_lock(&ido2db_debug_fp_lock) < 0)
                return ERROR;

        /* write the timestamp */
        gettimeofday(&current_time, NULL);

        time(&t);

        tm=localtime_r(&t, &tm_s);

        strftime(temp_time, 80, "%c", tm);

        tid=pthread_self();
        pid=getpid();
	if (ido2db_debug_readable_timestamp)
	        fprintf(ido2db_debug_file_fp, "%s .%06lu [%03d.%d] [pid=%lu] [tid=%lu] ", temp_time, current_time.tv_usec, level, verbosity, pid, tid);
	else
        	fprintf(ido2db_debug_file_fp, "[%lu.%06lu] [%03d.%d] [pid=%lu] [tid=%lu] ", current_time.tv_sec, current_time.tv_usec, level, verbosity, pid, tid);

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

	pthread_mutex_unlock(&ido2db_debug_fp_lock);

        return IDO_OK;
}






