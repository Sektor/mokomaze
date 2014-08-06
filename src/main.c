/*  main.c
 *
 *  Program entry point.
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

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <glib-object.h>
#include "types.h"
#include "mainwindow.h"
#include "paramsloader.h"

#define LOG_MODULE "Init"
#include "logging.h"

int main(int argc, char *argv[])
{
    g_type_init();

    parse_command_line(argc, argv);
    if (!load_params())
    {
        log_error("Failed to load required data.");
        return EXIT_FAILURE;
    }
    if (GetGameLevelsCount() == 0)
    {
        log_error("Zero levels loaded. Exiting.");
        return EXIT_FAILURE;
    }

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        log_error("Couldn't initialise SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    if (TTF_Init() < 0)
    {
        log_error("Couldn't initialise SDL_ttf: %s", TTF_GetError());
        return EXIT_FAILURE;
    }

    render_window();

    TTF_Quit();
    SDL_Quit();

    return EXIT_SUCCESS;
}
