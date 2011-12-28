/*  logging.c
 *
 *  Logging related functions.
 *
 *  (c) 2010-2011 Anton Olkhovik <ant007h@gmail.com>
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

#include "logging.h"

void log_message(MESSAGE_TYPE type, char *fmt, ...)
{
    va_list list;
    char *hdr = NULL;
    va_start(list, fmt);

    switch (type)
    {
    case MSG:
        break;
    case INFO:
        hdr = "[INFO]";
        break;
    case WARNING:
        hdr = "[WARN]";
        break;
    case ERROR:
        hdr = "[ERROR]";
        break;
    case DBG:
        hdr = "[DEBUG]";
        break;
    default:
        hdr = "[UNKNOWN]";
        break;
    }

    FILE *stream = (type == ERROR) ? stderr : stdout;
    if (hdr != NULL)
        fprintf(stream, "%s ", hdr);
    vfprintf(stream, fmt, list);
    fprintf(stream, "\n");

    va_end(list);
}
