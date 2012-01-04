/*  mazecore.h
 *
 *  Game logic routines.
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
 *  along with Mokomaze.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAZECORE_H
#define MAZECORE_H

#include "mazetypes.h"

GameState maze_step(int delta_ticks);
void maze_set_level(int n);
void maze_restart_level();
void maze_reload_level();
void maze_set_config(MazeConfig cfg);
void maze_set_levels_data(Level *lvls, int levels_count);
void maze_set_vibro_callback(void (*f)(float));
void maze_set_tilt(float x, float y, float z);
void maze_set_speed(float s);
void maze_get_ball(int *x, int *y, int *z, const dReal **rot);
void maze_get_animations(Animation **keys, Animation *final);
bool maze_is_keys_passed();
void maze_init();
void maze_quit();

#endif
