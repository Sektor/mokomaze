/*  accelerometers.h
 *
 *  Accelerometer stuff
 *
 *  (c) 2009 Anton Olkhovik <ant007h@gmail.com>
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

#ifndef ACCELEROMETERS_H
#define ACCELEROMETERS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#include "types.h"

void accelerometer_start();
void accelerometer_stop();

double getacx();
double getacy();
double getacz();

#endif /* ACCELEROMETERS_H */

