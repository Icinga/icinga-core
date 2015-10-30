/*****************************************************************************
 *
 * GETCGI.H -  Icinga CGI Input Routine Include File
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2015 Icinga Development Team (http://www.icinga.org)
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

#include "../include/cgiutils.h"

#ifdef __cplusplus
  extern "C" {
#endif

/** @brief html request struct
 *
 *  structure to hold html reqest data to prevent XSS attacks
**/
typedef struct html_request_struct {
	char *option;					/**< pointer to option string */
	char *value;					/**< pointer to value string */
	int is_valid;					/**< bool to mark if this request is valid */
	struct html_request_struct *next;		/**< next html_request entry */
} html_request;

html_request *getcgivars(void);
void free_html_request(html_request *);
void unescape_cgi_input(char *);
void sanitize_cgi_input(char **);
unsigned char hex_to_char(char *);

#ifdef __cplusplus
  }
#endif
