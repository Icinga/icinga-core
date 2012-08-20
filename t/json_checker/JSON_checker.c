/***********************************************************************
 *
 * JSON_CHECKER.C
 *
 * Copyright (c) 20012-2012 Icinga Development Team (http://www.icinga.org)
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
 ***********************************************************************/

#include "../../include/config.h"
#include "JSON_checker_lib.h"


#define RETURN_TEXT_LENGTH 80

int main(int argc, char* argv[]) {
	char json_text[RETURN_TEXT_LENGTH + 1];
	long unsigned position = 0;
	int started = 0;
	int i=0;
	int char_since_newline = 0;
	int error_position = 0;

	/*
		Read STDIN. Exit with a message if the input is not well-formed JSON text.
		jc will contain a JSON_checker with a maximum depth of 20.
	*/

	JSON_checker jc = new_JSON_checker(20);
	for (;;) {
		int next_char = getchar();
		if (next_char <= 0) {
			break;
		}

		/* keep last RETURN_TEXT_LENGTH characters to get an idea of the error position */
		if (position < RETURN_TEXT_LENGTH - 1) {
			json_text[position] = next_char;
			json_text[position + 1] = '\0';
		} else {
			memcpy(json_text, json_text + 1, RETURN_TEXT_LENGTH);
			json_text[RETURN_TEXT_LENGTH -1] = next_char;
			json_text[RETURN_TEXT_LENGTH] = '\0';
		}

		/* needed to point at correct ERROR position */
		if ( next_char == '\n' )
			char_since_newline = 0;
		else
			char_since_newline++;

		position++;

		// skip HTML header
		if ( started == 0 ) {
			if (next_char == '{')
				started = 1;
			else
				continue;
		}

		/* check all characters */
		if (!JSON_checker_char(jc, next_char)) {
		    	fprintf(stderr, "JSON_checker_char: syntax error: at character \"%c\" at position \"%lu\"\nJSON TEXT:\n%s\n", (char)next_char, position, json_text);

			/* point to position were error occurs */
			if (char_since_newline > RETURN_TEXT_LENGTH)
					error_position = RETURN_TEXT_LENGTH;
				else
					error_position = char_since_newline;

			if (error_position >= 18)
				fprintf(stderr, "ERROR POSITION ");

		    	for(i = 0; i < error_position - ((error_position >= 18) ? 16 : 1); i++)
		    		fprintf(stderr, "%s", (error_position >= 18) ? "-" : " ");

		    	fprintf(stderr, "^%s\n", (error_position >= 18) ? "" : "-- ERROR POSITION");
		    	exit(1);
		}
	}

	/* unexpected end of JSON data */
	if (!JSON_checker_done(jc)) {
		fprintf(stderr, "JSON_checker_end: syntax error. Unexpected end of JSON data.\n");
		exit(1);
	}

	printf("OK: Test passed.\n");
	exit(0);
}
