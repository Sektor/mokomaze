/*  input_keyboard.c
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

#include <SDL/SDL.h>
#include "input_keyboard.h"

static InputKeyboardData params = {0};

static void input_init()
{
}

static void input_shutdown()
{
}

static void input_read(float *x, float *y, float *z)
{
    double kx = 0, ky = 0;
    Uint8 *keystate = SDL_GetKeyState(NULL);

    if ( keystate[SDLK_UP] )
        ky = -params.g;
    else if ( keystate[SDLK_DOWN] )
        ky = params.g;

    if ( keystate[SDLK_LEFT] )
        kx = -params.g;
    else if ( keystate[SDLK_RIGHT] )
        kx = params.g;

    if (x) *x = kx;
    if (y) *y = ky;
    if (z) *z = 0;
}

void input_get_keyboard(InputInterface *input, InputKeyboardData *data)
{
    params = *data;
    input->init = &input_init;
    input->shutdown = &input_shutdown;
    input->read = &input_read;
}
