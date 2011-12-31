/*  mazehelpers.c
 *
 *  (c) 2009-2011 Anton Olkhovik <ant007h@gmail.com>
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

#include "mazehelpers.h"

int sign(float x, float delta)
{
    if (x<-delta) return -1;
    if (x>+delta) return +1;
    return 0;
}

bool inbox(float x, float y, Box box)
{
    return ( (x>=box.x1) && (x<=box.x2) &&
             (y>=box.y1) && (y<=box.y2) );
}

bool inbox_r(int x, int y, Point center, int r)
{
    return ( (x>=center.x-r) && (x<=center.x+r) &&
             (y>=center.y-r) && (y<=center.y+r) );
}
