/*  paramsloader.h
 *
 *  Config and level pack loader.
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

#ifndef PARAMSLOADER_H
#define PARAMSLOADER_H

#include "types.h"

void parse_command_line(int argc, char *argv[]);
bool load_params();
bool TouchDir(char *dir);
MazeConfig GetGameConfig();
Level* GetGameLevels();
int GetGameLevelsCount();
User* GetUserSettings();
Prompt GetArguments();
void SaveUserSettings();
char* GetSaveDir();
char* GetCacheDir();

#endif
