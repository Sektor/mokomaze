/*  input.h
 *
 *  Input system.
 *
 *  (c) 2011 Anton Olkhovik <ant007h@gmail.com>
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

#ifndef INPUT_H
#define INPUT_H

#include "input_calibration.h"
#include "input_dummy.h"
#include "input_keyboard.h"
#include "input_joystick.h"

typedef enum {
    INPUT_DUMMY,
    INPUT_KEYBOARD,
    INPUT_JOYSTICK
} InputType;

#define INPUT_DUMMY_STR "dummy"
#define INPUT_KEYBOARD_STR "keyboard"
#define INPUT_JOYSTICK_STR "joystick"

#endif /* INPUT_H */
