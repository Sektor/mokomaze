/*  input_accel.c
 *
 *  Accelerometer stuff
 *
 *  (c) 2009-2012 Anton Olkhovik <ant007h@gmail.com>
 *
 *  Based on original accelerometers.c from OpenMooCow - accelerometer moobox simulator.
 *  (c) 2008 Thomas White <taw27@srcf.ucam.org>
 *  (c) 2008 Joachim Breitner <mail@joachim-breitner.de>
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

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "../mazecore/mazehelpers.h"
#include "input_accel.h"

#define LOG_MODULE "Input::Accel"
#include "../logging.h"

struct input_event {
    struct timeval time;
    uint16_t type;
    uint16_t code;
    int32_t value;
};

#define EV_SYN (0x00)
#define EV_REL (0x02)
#define EV_ABS (0x03)
#define SYN_REPORT (0x00)
#define REL_X (0x00)
#define REL_Y (0x01)
#define REL_Z (0x02)
#define ABS_X (0x00)
#define ABS_Y (0x01)
#define ABS_Z (0x02)

static SDL_Thread *thread = NULL;
static bool finished = false;
static float ac[3] = {0};
static InputAccelData params = {0};

static int input_work(void *data)
{
    bool *finished = (bool*)data;

    int fd = open(params.fname, O_RDONLY, O_NONBLOCK);

    if (fd < 0)
    {
        log_error("error opening file `%s'", params.fname);
        return 0;
    }

    int ac_cache[3] = {0};
    struct input_event ev;
    while (!(*finished))
    {
        size_t rval;
        fd_set fds;
        struct timeval t;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        t.tv_sec = 0;
        t.tv_usec = 0;
        select(1 + fd, &fds, NULL, NULL, &t);

        if (FD_ISSET(fd, &fds))
        {
            rval = read(fd, &ev, sizeof(ev));

            if (rval != sizeof(ev))
            {
                log_error("error reading data");
                break;
            }

            if (ev.type == EV_REL)
            {
                if (ev.code == REL_X)
                    ac_cache[0] = ev.value;
                else if (ev.code == REL_Y)
                    ac_cache[1] = ev.value;
                else if (ev.code == REL_Z)
                    ac_cache[2] = ev.value;
            }
            else if (ev.type == EV_ABS)
            {
                if (ev.code == ABS_X)
                    ac_cache[0] = ev.value;
                else if (ev.code == ABS_Y)
                    ac_cache[1] = ev.value;
                else if (ev.code == ABS_Z)
                    ac_cache[2] = ev.value;
            }
            else if (ev.type == EV_SYN && ev.code == SYN_REPORT)
            {
                float acx = (float)ac_cache[0] / params.max_axis;
                float acy = (float)ac_cache[1] / params.max_axis;
                float acz = (float)ac_cache[2] / params.max_axis;
                clamp(acx, -1, 1);
                clamp(acy, -1, 1);
                clamp(acz, -1, 1);
                ac[0] = acx;
                ac[1] = acy;
                ac[2] = acz;
            }
        }

        SDL_Delay(params.interval);
    }

    close(fd);
    return 0;
}

static void input_init()
{
    finished = false;
    ac[0] = 0;
    ac[1] = 0;
    ac[2] = 0;
    thread = SDL_CreateThread(input_work, (void*)(&finished));
}

static void input_shutdown()
{
    finished = true;
    SDL_WaitThread(thread, NULL);
    thread = NULL;
}

static void input_read(float *x, float *y, float *z)
{
    if (x) *x = ac[0];
    if (y) *y = ac[1];
    if (z) *z = ac[2];
}

void input_get_accel(InputInterface *input, InputAccelData *data)
{
    if (params.fname)
    {
        free(params.fname);
        params.fname = NULL;
    }
    params = *data;
    params.fname = strdup(data->fname);

    input->init = &input_init;
    input->shutdown = &input_shutdown;
    input->read = &input_read;
}
