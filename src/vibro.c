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
#include <SDL/SDL.h>

#include "paramsloader.h"
#include "vibro.h"
#include "logging.h"

FILE* fvibro=NULL;
SDL_TimerID vibro_timer=0;

#define VIBRATION_TIME 33
int vib_int = VIBRATION_TIME;

int init_vibro()
{
    vib_int = VIBRATION_TIME * GetVibroInterval() / 100;

    fvibro = fopen("/sys/class/leds/gta02::vibrator/brightness", "w");
    if (fvibro != NULL) return 0;

    fvibro = fopen("/sys/devices/platform/leds_pwm/leds/gta02::vibrator/brightness", "w");
    if (fvibro != NULL) return 0;

    fvibro = fopen("/sys/class/leds/neo1973:vibrator/brightness", "w");
    if (fvibro != NULL) return 0;

    fvibro = fopen("/sys/devices/platform/neo1973-vibrator.0/leds/neo1973:vibrator/brightness", "w");
    if (fvibro != NULL) return 0;

    log_warning("Vibro: can't init.");
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
            vibro_timer = SDL_AddTimer(vib_int, callback, NULL);
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
        fvibro=NULL;
    }
    return 0;
}

