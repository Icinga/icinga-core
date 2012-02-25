/************************************************************************
 *
 * IO.H - Common I/O Functions
 * Copyright (c) 2005 Ethan Galstad
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 ************************************************************************/

#ifndef _IDO_IO_H
#define _IDO_IO_H

#include "../../../include/config.h"


#define IDO_SINK_FILE         0
#define IDO_SINK_FD           1
#define IDO_SINK_UNIXSOCKET   2
#define IDO_SINK_TCPSOCKET    3

#define IDO_DEFAULT_TCP_PORT  5668

/* fix for sun os */
#ifndef SUN_LEN
#define SUN_LEN(ptr) ((size_t) (((struct sockaddr_un *) 0)->sun_path) + strlen ((ptr)->sun_path))
#endif

/* MMAPFILE structure - used for reading files via mmap() */
typedef struct ido_mmapfile_struct{
	char *path;
	int mode;
	int fd;
	unsigned long file_size;
	unsigned long current_position;
	unsigned long current_line;
	void *mmap_buf;
        }ido_mmapfile;


ido_mmapfile *ido_mmap_fopen(char *);
int ido_mmap_fclose(ido_mmapfile *);
char *ido_mmap_fgets(ido_mmapfile *);

int ido_sink_open(char *,int,int,int,int,int *);
int ido_sink_write(int,char *,int);
int ido_sink_write_newline(int);
int ido_sink_flush(int);
int ido_sink_close(int);
int ido_inet_aton(register const char *,struct in_addr *);

void ido_strip_buffer(char *);
char *ido_escape_buffer(char *);
char *ido_unescape_buffer(char *);

#endif
