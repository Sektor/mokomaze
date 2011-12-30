/*  input_joystick.c
 *
 *  (c) 2009-2011 Anton Olkhovik <ant007h@gmail.com>
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
#include <pthread.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include "../mazecore/mazehelpers.h"
#include "input_joystick.h"

#define LOG_MODULE "Input::Joystick"
#include "../logging.h"

static pthread_t thread;
static bool finished = false;
static float ac[3] = {0};
static InputJoystickData params = {0};

static void *input_work(void *data)
{
    bool *finished = (bool*)data;

    int fd = open(params.fname, O_RDONLY);
    if (fd < 0)
    {
        log_error("error opening file `%s'", params.fname);
        return NULL;
    }

    struct js_event js;
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
            rval = read(fd, &js, sizeof(js));
            if (rval != sizeof(js))
            {
                log_error("error reading data");
                return NULL;
            }

            int n;
            float v;
            switch (js.type & ~JS_EVENT_INIT)
            {
            //case JS_EVENT_BUTTON:
            //    break;
            case JS_EVENT_AXIS:
                n = js.number;
                clamp(n, 0, sizeof(ac)-1);
                v = js.value / params.max_axis;
                clamp(v, -1, 1);
                ac[n] = v;
                break;
            }
        }

        usleep(params.interval);
    }

    close(fd);
    return NULL;
}

static void input_init()
{
    finished = false;
    pthread_create(&thread, NULL, input_work, (void*)(&finished));
}

static void input_shutdown()
{
    finished = true;
    pthread_join(thread, NULL);
}

static void input_read(float *x, float *y, float *z)
{
    if (x) *x = ac[0];
    if (y) *y = ac[1];
    if (z) *z = ac[2];
}

void input_get_joystick(InputInterface *input, InputJoystickData *data)
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