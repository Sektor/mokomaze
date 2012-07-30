/*  input_joystick_sdl.c
 *
 *  (c) 2012 Anton Olkhovik <ant007h@gmail.com>
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
#include "../mazecore/mazehelpers.h"
#include "input_joystick_sdl.h"

static SDL_Joystick *joystick;
static InputJoystickSdlData params = {0};

static void input_init()
{
    SDL_JoystickEventState(SDL_ENABLE);
    joystick = SDL_JoystickOpen(params.number);
}

static void input_shutdown()
{
    SDL_JoystickEventState(SDL_DISABLE);
    if (joystick)
        SDL_JoystickClose(joystick);
}

static void input_read(float *x, float *y, float *z)
{
    float kx = 0, ky = 0;
    if (joystick)
    {
        kx = SDL_JoystickGetAxis(joystick, 0) / params.max_axis;
        ky = SDL_JoystickGetAxis(joystick, 1) / params.max_axis;
        clamp(kx, -1, 1);
        clamp(ky, -1, 1);
    }
    if (x) *x = kx;
    if (y) *y = ky;
    if (z) *z = 0;
}

static void input_update(void *data)
{
}

void input_get_joystick_sdl(InputInterface *input, InputJoystickSdlData *data)
{
    params = *data;
    input->init = &input_init;
    input->shutdown = &input_shutdown;
    input->read = &input_read;
    input->update = &input_update;
}
