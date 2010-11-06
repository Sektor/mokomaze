/*  main.c
 *
 *  Program entry point
 *
 *  (c) 2009 Anton Olkhovik <ant007h@gmail.com>
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "types.h"
#include "json.h"
#include "mainwindow.h"
#include "paramsloader.h"
#include "vibro.h"
#include "logging.h"

int main(int argc, char *argv[]) {

        parse_command_line(argc, argv);
	load_params();
        if (GetGameLevelsCount() == 0) return 0;

        char* exec_init = GetExecInit();
        char* exec_final = GetExecFinal();
        if (strlen(exec_init) > 0) 
        {
            log_info("Init: executing initialization script");
            system(exec_init);
        }

        Prompt arguments = GetArguments();
        int start_level = 0;

        if (arguments.level_set)
        {
            start_level = arguments.level - 1;
        }
        else
        {
            start_level = GetUserSettings().level - 1;
        }
        if (start_level < 0) start_level=0;
        if (start_level >= GetGameLevelsCount()) start_level = GetGameLevelsCount()-1;

        if (GetVibroEnabled()) init_vibro();

        if ( SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0 ) {
		log_error("Couldn't initialise SDL: %s", SDL_GetError());
		return 1;
	}

        TTF_Init();

        render_window(start_level);

        TTF_Quit();
	SDL_Quit();
        close_vibro();

        if (strlen(exec_final) > 0) 
        {
            log_info("Init: executing finalization script");
            system(exec_final);
        }

	return 0;

}

