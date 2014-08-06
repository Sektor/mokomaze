/*  gui_settings.h
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

#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include <SDL/SDL.h>
#include "../types.h"

#ifdef __cplusplus
extern "C"
{
#endif

void settings_init(SDL_Surface *disp, int font_height, User *_user_set, User *_user_set_new);
void settings_show(bool *_calibration_requested, bool *_video_set_modified, bool *_input_set_modified, bool *_vibro_set_modified);
void settings_shutdown();

#ifdef __cplusplus
}
#endif

#endif /* GUI_SETTINGS_H */
