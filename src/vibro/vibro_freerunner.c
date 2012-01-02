/*  vibro_freerunner.c
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

#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#include "vibro_freerunner.h"

#define LOG_MODULE "Vibro::Freerunner"
#include "../logging.h"

static FILE *fvibro = NULL;
static SDL_TimerID vibro_timer = 0;
static VibroFreerunnerData params;

static void vibro_init()
{
    fvibro = fopen("/sys/class/leds/gta02::vibrator/brightness", "w");
    if (fvibro != NULL) return;

    fvibro = fopen("/sys/devices/platform/leds_pwm/leds/gta02::vibrator/brightness", "w");
    if (fvibro != NULL) return;

    fvibro = fopen("/sys/class/leds/neo1973:vibrator/brightness", "w");
    if (fvibro != NULL) return;

    fvibro = fopen("/sys/devices/platform/neo1973-vibrator.0/leds/neo1973:vibrator/brightness", "w");
    if (fvibro != NULL) return;

    log_warning("can't init");
}

static void stop_vibro()
{
    fprintf(fvibro, "%d", 0);
    fflush(fvibro);
    if (vibro_timer)
    {
        SDL_RemoveTimer(vibro_timer);
        vibro_timer = 0;
    }
}

static Uint32 callback(Uint32 interval, void *param)
{
    if (fvibro)
        stop_vibro();
    return 0;
}

static void vibro_bump(uint8_t level)
{
    if (fvibro) 
    {
        if (!vibro_timer)
        {
            fprintf(fvibro, "%d", level);
            fflush(fvibro);
            vibro_timer = SDL_AddTimer(params.duration, callback, NULL);
        }
    }
}

static void vibro_shutdown()
{
    if (fvibro)
    {
        stop_vibro();
        fclose(fvibro);
        fvibro = NULL;
    }
}

void vibro_get_freerunner(VibroInterface *vibro, VibroFreerunnerData *data)
{
    params = *data;
    vibro->init = &vibro_init;
    vibro->shutdown = &vibro_shutdown;
    vibro->bump = &vibro_bump;
}
