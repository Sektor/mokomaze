/*  types.h
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

#ifndef TYPES_H
#define TYPES_H

#include "dirs.h"
#include "input/input.h"
#include "vibro/vibro.h"
#include "mazecore/mazetypes.h"

typedef enum {
    FULLSCREEN_NONE,
    FULLSCREEN_INGAME,
    FULLSCREEN_ALWAYS
} FullscreenMode;

#define FULLSCREEN_NONE_STR "none"
#define FULLSCREEN_INGAME_STR "ingame"
#define FULLSCREEN_ALWAYS_STR "always"

typedef struct {
    char *levelpack;
    int level;
    int geom_x;
    int geom_y;
    int bpp;
    bool scrolling;
    FullscreenMode fullscreen_mode;
    int frame_delay;
    InputType input_type;
    float ball_speed;
    float bump_min_speed;
    float bump_max_speed;
    InputCalibrationData input_calibration_data;
    InputJoystickData input_joystick_data;
    InputJoystickSdlData input_joystick_sdl_data;
    InputAccelData input_accel_data;
    VibroType vibro_type;
    VibroFreerunnerData vibro_freeerunner_data;
} User;

typedef struct {
    int level;
    bool level_set;
    InputType input_type;
    bool input_set;
    VibroType vibro_type;
    bool vibro_set;
    int geom_x;
    int geom_y;
    bool geom_set;
    int bpp;
    bool bpp_set;
    bool scrolling;
    bool scrolling_set;
    FullscreenMode fullscreen_mode;
    bool fullscreen_mode_set;
    bool cal_auto;
    bool cal_reset;
} Prompt;

#endif /* TYPES_H */
