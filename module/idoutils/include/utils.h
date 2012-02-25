/************************************************************************
 *
 * UTILS.H - IDO utilities header file
 * Copyright (c) 2005-2008 Ethan Galstad
 * Copyright (c) 2009-2012 Icinga Development Team (http://www.icinga.org)
 *
 ************************************************************************/

#ifndef _IDO_UTILS_H
#define _IDO_UTILS_H

/* my_free has been freed from bondage as a function */
#define my_free(ptr) do { if(ptr) { free(ptr); ptr = NULL; } } while(0)

typedef struct ido_dbuf_struct{
	char *buf;
	unsigned long used_size;
	unsigned long allocated_size;
	unsigned long chunk_size;
        }ido_dbuf;


int ido_dbuf_init(ido_dbuf *,int);
int ido_dbuf_free(ido_dbuf *);
int ido_dbuf_strcat(ido_dbuf *,char *);

int my_rename(char *,char *);

void idomod_strip(char *);

#endif
