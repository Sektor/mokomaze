/*  logging.h
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

#ifndef LOGGING_H
#define LOGGING_H

// Colors used by logger
#define INFO_FORMAT  "\e[1;32m%s\e[00m %s\n"
#define WARN_FORMAT  "\e[1;33m%s\e[00m %s\n"
#define ERROR_FORMAT "\e[1;31m%s\e[00m %s\n"
#define DEBUG_FORMAT "\e[1;36m%s\e[00m %s\n"

// Supported message types.
typedef enum {
	INFO,
	WARNING,
	ERROR,
#ifdef DEBUG
	DBG,
#endif // DEBUG
} MESSAGE_TYPE;

// Main logging routine. Should not be used directly.
// Please use log_<type> macros.
void log_message(MESSAGE_TYPE type, char* fmt, ...);

// Logging macros.
#define log_info(format, args...)    log_message(INFO, format, ## args)
#define log_warning(format, args...) log_message(WARNING, format, ## args)
#define log_error(format, args...)   log_message(ERROR, format, ## args)
#ifdef DEBUG
#define log_debug(format, args...)   log_message(DBG, format, ## args)
#else
#define log_debug(format, args...)
#endif // DEBUG

#endif // LOGGING_H

