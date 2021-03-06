/*  input_calibration.h
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

#ifndef INPUT_CALIBRATION_H
#define INPUT_CALIBRATION_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool swap_xy;
    bool invert_x;
    bool invert_y;
    float cal_x;
    float cal_y;
    float sens;
} InputCalibrationData;

void input_calibration_reset();
int input_calibration_sample(InputCalibrationData *data, float *x, float *y, float *z);
void input_calibration_adjust(InputCalibrationData *data, float *x, float *y, float *z);

#endif /* INPUT_CALIBRATION_H */
