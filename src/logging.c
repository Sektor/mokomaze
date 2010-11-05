/*  logging.c
 *
 *  Logging related functions.
 *
 *  (c) 2009 Peter Tworek <tworaz666@gmail.com>
 *
 *  This file is part of Mokomaze - labyrinth game.
 *
 *  Mokomaze is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mokomaze is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mokomaze.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "logging.h"

void log_message(MESSAGE_TYPE type, char* fmt, ...)
{
	va_list list;
	char* msg_fmt = NULL;
	char* fmt_hdr = NULL;
	char* type_fmt = NULL;
	int fmt_len = 0;
	bool use_color = true;
	va_start(list, fmt);

	switch (type) {
	case INFO:
		fmt_hdr = _("[INFO]");
		type_fmt = use_color ? INFO_FORMAT : "%s %s\n";
		break;
	case WARNING:
		fmt_hdr = _("[WARNING]");
		type_fmt = use_color ? WARN_FORMAT : "%s %s\n";
		break;
	case ERROR:
		fmt_hdr = _("[ERROR]");
		type_fmt = use_color ? ERROR_FORMAT : "%s %s\n";
		break;
#ifdef DEBUG
	case DBG:
		fmt_hdr = _("[DEBUG]");
		type_fmt = use_color ? DEBUG_FORMAT : "%s %s\n";
		break;
#endif // DEBUG
	default:
		fmt_hdr = _("[UNKNOWN]");
		type_fmt = "%s %s\n";
		break;
	}

	fmt_len = strlen(fmt_hdr) + strlen(fmt) + 15;
	msg_fmt = malloc(fmt_len);
	sprintf(msg_fmt, type_fmt, fmt_hdr, fmt);

	if (type == ERROR) {
		vfprintf(stderr, msg_fmt, list);
	} else {
		vfprintf(stdout, msg_fmt, list);
	}

	free(msg_fmt);
	va_end(list);
}

