/*  vibro.h
 *
 *  Vibro feedback system.
 *
 *  (c) 2011-2012 Anton Olkhovik <ant007h@gmail.com>
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
 *  along with Mokomaze.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef VIBRO_H
#define VIBRO_H

#include "vibro_dummy.h"
#include "vibro_freerunner.h"

typedef enum {
    VIBRO_DUMMY,
    VIBRO_FREERUNNER
} VibroType;

#define VIBRO_DUMMY_STR "dummy"
#define VIBRO_FREERUNNER_STR "freerunner"

#endif /* VIBRO_H */
