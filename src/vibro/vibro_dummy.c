/*  vibro_dummy.c
 *
 *  (c) 2011 Anton Olkhovik <ant007h@gmail.com>
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

#include "vibro_dummy.h"

static void vibro_init()
{
}

static void vibro_bump(uint8_t level)
{
}

static void vibro_shutdown()
{
}

void vibro_get_dummy(VibroInterface *vibro)
{
    vibro->init = &vibro_init;
    vibro->shutdown = &vibro_shutdown;
    vibro->bump = &vibro_bump;
}
