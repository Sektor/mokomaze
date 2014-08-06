/*  mazehelpers.h
 *
 *  (c) 2009-2012 Anton Olkhovik <ant007h@gmail.com>
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

#ifndef MAZEHELPERS_H
#define MAZEHELPERS_H

#include "mazetypes.h"

#define min(a, b) ((a<=b) ? a : b)
#define max(a, b) ((a>=b) ? a : b)
#define clamp_max(x,max) x = (x>max ? max : x)
#define clamp_min(x,min) x = (x<min ? min : x)
#define clamp(x,min,max) { clamp_min(x,min); clamp_max(x,max); }
#define calcdist(x1,y1, x2,y2) sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1))
#define calclen(x,y,z) sqrt(x*x + y*y + z*z)
#define sqr(x) (x)*(x)

int sign(float x, float delta);
bool inbox(float x, float y, Box box);
bool inbox_r(int x, int y, Point center, int r);

#endif //MAZEHELPERS_H