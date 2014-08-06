/*  input_calibration.c
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

#include <math.h>
#include "../mazecore/mazehelpers.h"
#include "input_calibration.h"

static float sum_x = 0.0f;
static float sum_y = 0.0f;
static int samples = 0;

void input_calibration_reset()
{
    sum_x = 0.0f;
    sum_y = 0.0f;
    samples = 0;
}

int input_calibration_sample(InputCalibrationData *data, float *x, float *y, float *z)
{
    float tx = (x ? *x : 0);
    float ty = (y ? *y : 0);
    sum_x += asin(tx);
    sum_y += asin(ty);
    samples++;
    data->cal_x = sum_x / samples;
    data->cal_y = sum_y / samples;
    return samples;
}

void input_calibration_adjust(InputCalibrationData *data, float *x, float *y, float *z)
{
    float tx = (x ? *x : 0);
    float ty = (y ? *y : 0);

    if (data->cal_x != 0.0f)
        tx = sin(asin(tx)-data->cal_x);
    if (data->cal_y != 0.0f)
        ty = sin(asin(ty)-data->cal_y);

    if (data->swap_xy)
    {
        float tmpx = tx;
        tx = ty;
        ty = tmpx;
    }
    if (data->invert_x)
        tx = -tx;
    if (data->invert_y)
        ty = -ty;

    tx *= data->sens;
    ty *= data->sens;

    clamp(tx, -1, 1);
    clamp(ty, -1, 1);
    
    if (x) *x = tx;
    if (y) *y = ty;
}
