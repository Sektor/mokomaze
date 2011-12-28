/*  mazetypes.h
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

#ifndef MAZETYPES_H
#define MAZETYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <ode/ode.h>

typedef enum {
    GAME_STATE_NORMAL,
    GAME_STATE_FAILED,
    GAME_STATE_WIN,
    GAME_STATE_SAVED
} GameState;

typedef enum {
    ANIMATION_NONE,
    ANIMATION_PLAYING,
    ANIMATION_FINISHED
} AnimationStage;

typedef struct {
    float time;
    float progress;
    AnimationStage stage;
    bool played;
} Animation;

//------------------------------------------------------------------------------

typedef struct {
    int wnd_w;
    int wnd_h;

    int ball_r;
    int hole_r;
    int key_r;
    int shadow;
} MazeConfig;

//------------------------------------------------------------------------------

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    int x1;
    int y1;

    int x2;
    int y2;
} Box;

typedef struct {
    int boxes_count;
    Box *boxes;

    int holes_count;
    Point *holes;

    int fins_count;
    Point *fins;

    Point init;

    int keys_count;
    Point *keys;
} Level;

#endif /* MAZETYPES_H */
