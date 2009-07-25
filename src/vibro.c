/*  vibro.c
 *
 *  Vibro feedback routines
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>

#include "vibro.h"

FILE* fvibro=NULL;
SDL_TimerID vibro_timer=0;

#define VIBRATION_TIME 33

int init_vibro()
{
    fvibro = fopen("/sys/class/leds/neo1973:vibrator/brightness", "w");
    if (fvibro != NULL) return 0;

    fvibro = fopen("/sys/devices/platform/neo1973-vibrator.0/leds/neo1973:vibrator/brightness", "w");
    if (fvibro != NULL) return 0;

    fprintf(stderr, "Vibro: can't init.\n");
    return 1;
}

Uint32 callback (Uint32 interval, void *param)
{
    if (fvibro)
    {
        fprintf(fvibro, "%d", 0);
        fflush(fvibro);
        if (vibro_timer)
        {
            SDL_RemoveTimer(vibro_timer);
            vibro_timer=0;
        }
    }
    return 0;
}

int set_vibro(BYTE level)
{
    if (fvibro) 
    {
        if (!vibro_timer)
        {
            fprintf(fvibro, "%d", level);
            fflush(fvibro);
            vibro_timer = SDL_AddTimer(VIBRATION_TIME, callback, NULL);
        }
    }
    return 0;
}

int close_vibro()
{
    if (fvibro)
    {
        fprintf(fvibro, "%d", 0);
        fflush(fvibro);
        if (vibro_timer)
        {
            SDL_RemoveTimer(vibro_timer);
            vibro_timer=0;
        }
        fclose(fvibro);
    }
    return 0;
}

